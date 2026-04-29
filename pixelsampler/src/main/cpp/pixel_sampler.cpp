// pixel_sampler.cpp
#include "pixel_sampler.h"
#include "choreographer_callback.h"
#include "view_locator.h"
#include "jni_helpers.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <media/NdkImageReader.h>
#include <media/NdkImage.h>

static AImageReader* gImageReader = nullptr;
static ANativeWindow* gNativeWindow = nullptr;

PixelSampler& PixelSampler::getInstance() {
    static PixelSampler instance;
    return instance;
}

void PixelSampler::startSampling(JNIEnv* env, jobject activity) {
    if (mIsActive.load()) {
        LOGW("Sampling already active");
        return;
    }

    LOGI("=== PixelSampler::startSampling() started with Activity: %p ===", activity);

    // Root View - now using passed Activity
    if (mRootView == nullptr) {
        LOGI("Finding root view using passed Activity...");
        jobject root = ViewLocator::getInstance().findRootViewWithActivity(env, activity);
        if (root != nullptr) {
            mRootView = env->NewGlobalRef(root);
            LOGI("✅ Root view acquired and cached");
        } else {
            LOGE("❌ Failed to find root view");
            notifyComplete(-1);
            return;
        }
    }

    // Reset state
    mIsActive.store(true);
    mFrameCount.store(0);
    mConsecutiveUnchangedCount.store(0);
    mLastHash.store(0);
    mStartTimeNs = 0;
    mLastSampleTimeNs = 0;

    // Create ImageReader
    LOGI("Creating 1x1 ImageReader...");
    jobject surface = createImageReader(env, 1, 1);
    if (surface != nullptr) {
        gNativeWindow = ANativeWindow_fromSurface(env, surface);
        env->DeleteLocalRef(surface);
        LOGI("✅ ImageReader created");
    } else {
        LOGE("❌ Failed to create ImageReader");
        notifyComplete(-2);
        return;
    }

    // Start Choreographer
    LOGI("Starting ChoreographerCallback...");
    ChoreographerCallback::getInstance().start([this](jlong frameTimeNanos) {
        this->onFrameCallback(frameTimeNanos);
    });

    LOGI("✅ PixelSampler fully started!");
}

void PixelSampler::stopSampling() {
    if (!mIsActive.load()) return;

    mIsActive.store(false);
    ChoreographerCallback::getInstance().stop();

    JNIEnv* env = getJNIEnv();
    if (env) {
        if (mJavaCallback) {
            env->DeleteGlobalRef(mJavaCallback);
            mJavaCallback = nullptr;
        }
        if (mRootView) {
            env->DeleteGlobalRef(mRootView);
            mRootView = nullptr;
        }
    }

    // Cleanup ImageReader
    if (gNativeWindow) {
        ANativeWindow_release(gNativeWindow);
        gNativeWindow = nullptr;
    }
    if (gImageReader) {
        AImageReader_delete(gImageReader);
        gImageReader = nullptr;
    }

    LOGI("PixelSampler stopped");
}

void PixelSampler::onFrameCallback(jlong frameTimeNanos) {
    if (!mIsActive.load()) return;

    mFrameCount.fetch_add(1);

    LOGI("onFrameCallback #%d", mFrameCount.load());

    if (mFrameCount.load() % 5 != 0) {
        return;
    }

    if (mLastSampleTimeNs > 0) {
        jlong elapsed = frameTimeNanos - mLastSampleTimeNs;
        if (elapsed < SAMPLE_INTERVAL_NS) return;
    }

    mLastSampleTimeNs = frameTimeNanos;

    jlong hash = captureCenterPixel();
    processPixel(hash);
}

jlong PixelSampler::captureCenterPixel() {
    if (!gImageReader) return 0;

    AImage* image = nullptr;
    if (AImageReader_acquireLatestImage(gImageReader, &image) != AMEDIA_OK || image == nullptr) {
        return 0;
    }

    AHardwareBuffer* buffer = nullptr;
    AImage_getHardwareBuffer(image, &buffer);

    jlong pixel = 0;
    if (buffer) {
        void* data = nullptr;
        AHardwareBuffer_lock(buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, &data);
        if (data) {
            pixel = *reinterpret_cast<uint32_t*>(data) & 0xFFFFFFFFULL;
            AHardwareBuffer_unlock(buffer, nullptr);
        }
    }

    AImage_delete(image);
    return pixel;
}

void PixelSampler::processPixel(jlong hash) {
    if (hash == 0) return;

    if (mStartTimeNs == 0) {
        mStartTimeNs = mLastSampleTimeNs;
        LOGI("First content detected");
    }

    if (hash == mLastHash.load()) {
        int count = mConsecutiveUnchangedCount.fetch_add(1) + 1;

        if (count >= REQUIRED_STABLE_FRAMES) {
            jlong totalTimeMs = (mLastSampleTimeNs - mStartTimeNs) / 1'000'000;
            LOGI("Render complete after %d frames (~%lld ms)", mFrameCount.load(), (long long)totalTimeMs);
            notifyComplete(totalTimeMs);
            stopSampling();
        }
    } else {
        mConsecutiveUnchangedCount.store(0);
        mLastHash.store(hash);
    }
}

void PixelSampler::notifyComplete(jlong totalTimeMs) {
    JNIEnv* env = getJNIEnv();
    if (env == nullptr) return;

    jclass samplerClass = env->FindClass("io/timon/android/pixelsampler/PixelSampler");
    if (samplerClass == nullptr) {
        env->ExceptionClear();
        LOGE("Failed to find PixelSampler class");
        return;
    }

    // Get the singleton INSTANCE of the Kotlin object
    jfieldID instanceField = env->GetStaticFieldID(samplerClass,
            "INSTANCE", "Lio/timon/android/pixelsampler/PixelSampler;");

    if (instanceField == nullptr) {
        env->DeleteLocalRef(samplerClass);
        LOGE("Failed to find INSTANCE field");
        return;
    }

    jobject samplerInstance = env->GetStaticObjectField(samplerClass, instanceField);
    if (samplerInstance == nullptr) {
        env->DeleteLocalRef(samplerClass);
        LOGE("PixelSampler.INSTANCE is null");
        return;
    }

    // Now call the instance method
    jmethodID onCompleted = env->GetMethodID(samplerClass,
            "onRenderCompleted", "(J)V");

    if (onCompleted != nullptr) {
        env->CallVoidMethod(samplerInstance, onCompleted, totalTimeMs);
        LOGI("Successfully notified Kotlin: render completed in %lld ms", totalTimeMs);
    } else {
        LOGE("Could not find onRenderCompleted method");
    }

    env->DeleteLocalRef(samplerInstance);
    env->DeleteLocalRef(samplerClass);
}

jobject PixelSampler::createImageReader(JNIEnv* env, jint width, jint height) {
    jclass clazz = env->FindClass("io/timon/android/pixelsampler/PixelSampler");
    if (clazz == nullptr) {
        env->ExceptionClear();
        LOGE("Cannot find PixelSampler class");
        return nullptr;
    }

    jmethodID method = env->GetStaticMethodID(clazz, "createImageReader", "(II)Landroid/view/Surface;");
    if (method == nullptr) {
        env->DeleteLocalRef(clazz);
        env->ExceptionClear();
        LOGE("Cannot find createImageReader method");
        return nullptr;
    }

    jobject surface = env->CallStaticObjectMethod(clazz, method, width, height);
    env->DeleteLocalRef(clazz);
    return surface;
}

// ================== JNI EXPORTS ==================
extern "C" {

JNIEXPORT jboolean JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_findRootViewNative(
        JNIEnv* env, jobject thiz, jobject activity) {

    LOGI("findRootViewNative() called with Activity: %p", activity);

    if (activity == nullptr) {
        LOGE("Activity passed from Java is null");
        return JNI_FALSE;
    }

    // Delegate to our ViewLocator
    jobject root = ViewLocator::getInstance().findRootViewWithActivity(env, activity);

    bool success = (root != nullptr);

    LOGI("findRootViewNative result: %s", success ? "SUCCESS" : "FAILED");
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_startChoreographerCallback(
        JNIEnv* env, jobject thiz, jobject activity) {

    LOGI("startChoreographerCallback() called with Activity: %p", activity);

    PixelSampler::getInstance().startSampling(env, activity);
}

} // extern "C"