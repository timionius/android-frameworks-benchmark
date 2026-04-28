#include "pixel_sampler.h"
#include "choreographer_callback.h"
#include "view_locator.h"
#include "jni_helpers.h"

PixelSampler& PixelSampler::getInstance() {
    static PixelSampler instance;
    return instance;
}

void PixelSampler::startSampling(JNIEnv* env, jobject callback) {
    if (mIsActive) {
        LOGW("Sampling already active");
        return;
    }

    // Store callback reference
    if (mJavaCallback != nullptr) {
        env->DeleteGlobalRef(mJavaCallback);
    }
    mJavaCallback = env->NewGlobalRef(callback);

    // Find root view
    mRootView = ViewLocator::getInstance().findRootView(env);
    if (mRootView == nullptr) {
        LOGE("Failed to find root view");
        notifyComplete(-1);
        return;
    }
    mRootView = env->NewGlobalRef(mRootView);

    // Reset state
    mIsActive = true;
    mFrameCount = 0;
    mConsecutiveUnchangedCount = 0;
    mLastHash = 0;
    mStartTimeNs = 0;
    mLastSampleTimeNs = 0;

    // Start Choreographer callback
    ChoreographerCallback::getInstance().start([](jlong frameTimeNanos) {
        PixelSampler::getInstance().onFrameCallback(frameTimeNanos);
    });

    LOGI("Sampling started");
}

void PixelSampler::stopSampling() {
    if (!mIsActive) return;

    mIsActive = false;
    ChoreographerCallback::getInstance().stop();

    JNIEnv* env = getJNIEnv();
    if (env && mJavaCallback) {
        env->DeleteGlobalRef(mJavaCallback);
        mJavaCallback = nullptr;
    }
    if (env && mRootView) {
        env->DeleteGlobalRef(mRootView);
        mRootView = nullptr;
    }

    LOGI("Sampling stopped");
}

void PixelSampler::onFrameCallback(jlong frameTimeNanos) {
    if (!mIsActive) return;

    mFrameCount++;

    // Sample every 5th frame to reduce overhead
    if (mFrameCount % 5 != 0) return;

    // Time-based throttling (maintain 60fps sampling)
    if (mLastSampleTimeNs > 0) {
        jlong elapsed = frameTimeNanos - mLastSampleTimeNs;
        if (elapsed < SAMPLE_INTERVAL_NS) {
            return; // Too soon
        }
    }
    mLastSampleTimeNs = frameTimeNanos;

    // Capture and process pixel
    JNIEnv* env = getJNIEnv();
    if (env == nullptr) return;

    jlong hash = captureCenterPixel(env);
    processPixel(hash);

    // Log progress
    if (mFrameCount <= 25) {
        LOGD("Frame %d: hash=%lld", mFrameCount, (long long)hash);
    }
}

jlong PixelSampler::captureCenterPixel(JNIEnv* env) {
    if (mRootView == nullptr) return 0L;

    // Get view dimensions
    jclass viewClass = env->GetObjectClass(mRootView);
    jmethodID getWidth = env->GetMethodID(viewClass, "getWidth", "()I");
    jmethodID getHeight = env->GetMethodID(viewClass, "getHeight", "()I");

    jint width = env->CallIntMethod(mRootView, getWidth);
    jint height = env->CallIntMethod(mRootView, getHeight);

    if (width == 0 || height == 0) return 0L;

    jint centerX = width / 2;
    jint centerY = height / 2;

    // Create 1x1 bitmap
    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmap = env->GetStaticMethodID(
            bitmapClass, "createBitmap",
            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;"
    );

    jclass configClass = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID argb8888Field = env->GetStaticFieldID(configClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
    jobject config = env->GetStaticObjectField(configClass, argb8888Field);

    jobject bitmap = env->CallStaticObjectMethod(bitmapClass, createBitmap, 1, 1, config);

    // Create canvas and draw view at center point
    jclass canvasClass = env->FindClass("android/graphics/Canvas");
    jmethodID canvasInit = env->GetMethodID(canvasClass, "<init>", "(Landroid/graphics/Bitmap;)V");
    jobject canvas = env->NewObject(canvasClass, canvasInit, bitmap);

    jmethodID translate = env->GetMethodID(canvasClass, "translate", "(FF)V");
    env->CallVoidMethod(canvas, translate, (jfloat)-centerX, (jfloat)-centerY);

    jmethodID draw = env->GetMethodID(viewClass, "draw", "(Landroid/graphics/Canvas;)V");
    env->CallVoidMethod(mRootView, draw, canvas);

    // Extract pixel
    jmethodID getPixel = env->GetMethodID(bitmapClass, "getPixel", "(II)I");
    jint pixel = env->CallIntMethod(bitmap, getPixel, 0, 0);

    // Cleanup
    env->DeleteLocalRef(bitmap);
    env->DeleteLocalRef(canvas);

    return (jlong)pixel & 0xFFFFFFFF;
}

void PixelSampler::processPixel(jlong hash) {
    // Skip empty/black frames
    if (hash == 0) return;

    // First non-zero hash - record start time
    if (mStartTimeNs == 0) {
        mStartTimeNs = mLastSampleTimeNs;
        LOGI("First content detected");
    }

    if (hash == mLastHash) {
        mConsecutiveUnchangedCount++;

        LOGD("Stable frame %d/%d", mConsecutiveUnchangedCount.load(), REQUIRED_STABLE_FRAMES);

        if (mConsecutiveUnchangedCount >= REQUIRED_STABLE_FRAMES) {
            jlong totalTimeMs = (mLastSampleTimeNs - mStartTimeNs) / 1'000'000;
            LOGI("Render complete after %d frames (~%lld ms)",
                    mFrameCount.load(), (long long)totalTimeMs);
            notifyComplete(totalTimeMs);
            stopSampling();
        }
    } else {
        mConsecutiveUnchangedCount = 0;
        mLastHash = hash;
    }
}

void PixelSampler::notifyComplete(jlong totalTimeMs) {
    JNIEnv* env = getJNIEnv();
    if (env == nullptr || mJavaCallback == nullptr) return;

    jclass callbackClass = env->GetObjectClass(mJavaCallback);
    jmethodID onComplete = env->GetMethodID(
            callbackClass,
            "onRenderComplete",
            "(J)V"
    );

    if (onComplete != nullptr) {
        env->CallVoidMethod(mJavaCallback, onComplete, totalTimeMs);
    }
}