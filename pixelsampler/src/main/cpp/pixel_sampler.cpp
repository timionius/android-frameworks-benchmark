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

void PixelSampler::startSampling(JNIEnv* env, jobject callback) {
    if (mIsActive.load()) {
        LOGW("Sampling already active");
        return;
    }

    // Store Java callback
    if (mJavaCallback != nullptr) {
        env->DeleteGlobalRef(mJavaCallback);
    }
    mJavaCallback = env->NewGlobalRef(callback);

    // Find root view
    mRootView = ViewLocator::getInstance().findRootView(env);
    if (mRootView != nullptr) {
        mRootView = env->NewGlobalRef(mRootView);
    } else {
        LOGE("Failed to find root view");
        notifyComplete(-1);
        return;
    }

    // Reset state
    mIsActive.store(true);
    mFrameCount.store(0);
    mConsecutiveUnchangedCount.store(0);
    mLastHash.store(0);
    mStartTimeNs = 0;
    mLastSampleTimeNs = 0;

    // Create ImageReader (1x1 is enough for center pixel sampling)
    jobject surface = createImageReader(env, 1, 1);
    if (surface != nullptr) {
        gNativeWindow = ANativeWindow_fromSurface(env, surface);
        LOGI("ImageReader created successfully");
    } else {
        LOGE("Failed to create ImageReader");
    }

    // Start Choreographer
    ChoreographerCallback::getInstance().start([this](jlong frameTimeNanos) {
        this->onFrameCallback(frameTimeNanos);
    });

    LOGI("PixelSampler started (ImageReader mode)");
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

    if (mFrameCount.load() % 5 != 0) return;  // Sample every 5th frame

    if (mLastSampleTimeNs > 0) {
        jlong elapsed = frameTimeNanos - mLastSampleTimeNs;
        if (elapsed < SAMPLE_INTERVAL_NS) return;
    }
    mLastSampleTimeNs = frameTimeNanos;

    jlong hash = captureCenterPixel();
    processPixel(hash);

    if (mFrameCount.load() <= 30) {
        LOGD("Frame %d: hash=%lld", mFrameCount.load(), (long long)hash);
    }
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
    if (env == nullptr || mJavaCallback == nullptr) return;

    jclass callbackClass = env->GetObjectClass(mJavaCallback);
    jmethodID onComplete = env->GetMethodID(callbackClass, "onRenderComplete", "(J)V");

    if (onComplete != nullptr) {
        env->CallVoidMethod(mJavaCallback, onComplete, totalTimeMs);
    }
    env->DeleteLocalRef(callbackClass);
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

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_startNative(
        JNIEnv* env, jclass clazz) {
    PixelSampler::getInstance().startSampling(env, nullptr);  // callback will be handled differently
}

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_stopNative(
        JNIEnv* env, jclass clazz) {
    PixelSampler::getInstance().stopSampling();
}

}