#include "choreographer_callback.h"
#include "pixel_sampler.h"
#include "jni_helpers.h"

#include <media/NdkImageReader.h>
#include <media/NdkImage.h>
#include <android/native_window_jni.h>

#include <atomic>
#include <vector>
#include <cstring>

namespace pixelsampler {

    constexpr int CAPTURE_WIDTH = 100;
    constexpr int CAPTURE_HEIGHT = 100;
    constexpr int CAPTURE_DPI = 160;

    // Global resources
    static AImageReader* g_imageReader = nullptr;
    static jobject g_virtualDisplay = nullptr;

    // Frame tracking
    static std::vector<uint8_t> g_prevPixels;
    static std::atomic<int> g_stableCount{0};
    static std::atomic<bool> g_isStable{false};
    static int g_emptyBufferTicks = 0;
    static int g_frameCount = 0;
    static double g_lastAnimation = 0;
    static bool g_isCapturing = false;

    // Forward declarations
    void createVirtualDisplay(JNIEnv* env, jobject mediaProjection);

    // ===================================================================
    // Start Capture - Called from JNI
    // ===================================================================
    void startCapture(JNIEnv* env, jobject mediaProjection, jint width, jint height, jint dpi) {
        if (g_isCapturing) {
            LOGW("Already capturing");
            return;
        }

        choreographer::start();
        LOGI("✅ Choreographer started");

        LOGI("Starting capture: %dx%d, dpi=%d", width, height, dpi);

        // Initialize frame tracking
        g_prevPixels.assign(width * height * 4, 0);
        g_stableCount.store(0);
        g_isStable.store(false);
        g_frameCount = 0;
        g_emptyBufferTicks = 0;
        g_lastAnimation = 0;

        // Create VirtualDisplay and start capture
        createVirtualDisplay(env, mediaProjection);

        if (g_imageReader) {
            g_isCapturing = true;
            LOGI("✅ Capture started successfully");
        } else {
            LOGE("Failed to start capture");
        }
    }

    // ===================================================================
    // Create VirtualDisplay using stored MediaProjection
    // ===================================================================
    void createVirtualDisplay(JNIEnv* env, jobject mediaProjection) {
        // 1. Create AImageReader
        media_status_t status = AImageReader_new(
                CAPTURE_WIDTH, CAPTURE_HEIGHT,
                AIMAGE_FORMAT_RGBA_8888, 4,
                &g_imageReader);

        if (status != AMEDIA_OK || !g_imageReader) {
            LOGE("Failed to create AImageReader: %d", status);
            return;
        }

        LOGI("✅ AImageReader created: %dx%d", CAPTURE_WIDTH, CAPTURE_HEIGHT);

        // Get the ANativeWindow from AImageReader
        ANativeWindow* nativeWindow = nullptr;
        AImageReader_getWindow(g_imageReader, &nativeWindow);
        jobject surfaceObj = ANativeWindow_toSurface(env, nativeWindow);

        if (!nativeWindow) {
            LOGE("Failed to get ANativeWindow from AImageReader");
            return;
        }

        LOGI("✅ Got ANativeWindow: %p", nativeWindow);

        // 3. Create VirtualDisplay using MediaProjection
        jclass mediaProjectionClass = env->GetObjectClass(mediaProjection);
        jmethodID createVirtualDisplay = env->GetMethodID(
                mediaProjectionClass,
                "createVirtualDisplay",
                "(Ljava/lang/String;IIIILandroid/view/Surface;Landroid/hardware/display/VirtualDisplay$Callback;Landroid/os/Handler;)Landroid/hardware/display/VirtualDisplay;");

        int publicFlag = getVirtualDisplayFlagPublic(env);

        jstring name = env->NewStringUTF("PixelSampler");
        jobject virtualDisplay = env->CallObjectMethod(
                mediaProjection, //g_mediaProjection,
                createVirtualDisplay,
                name,
                CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_DPI,
                publicFlag,
                surfaceObj,
                nullptr,
                nullptr
        );

        env->DeleteLocalRef(name);
        env->DeleteLocalRef(surfaceObj);
        if (virtualDisplay) {
            g_virtualDisplay = env->NewGlobalRef(virtualDisplay);
            env->DeleteLocalRef(virtualDisplay);
            LOGI("✅ VirtualDisplay created successfully");
        } else {
            LOGE("Failed to create VirtualDisplay");
        }
    }

    // ===================================================================
    // Frame Callback - Called from Choreographer
    // ===================================================================
    void onFrameCallback(int64_t /*frameTimeNanos*/) {
        LOGI("Callback entered: isCapturing=%d, isStable=%d, reader=%p",
                g_isCapturing, g_isStable.load(), g_imageReader);
        if (!g_isCapturing || g_isStable.load() || !g_imageReader) {
            return;
        }
        LOGI("✅ pixel_sampler onFrameCallback entry");

        g_frameCount++;

        // Acquire latest image
        AImage* image = nullptr;
        media_status_t status = AImageReader_acquireLatestImage(g_imageReader, &image);

        if (status == AMEDIA_OK && image != nullptr) {
            LOGI("✅ pixel_sampler scene got changed");
            g_lastAnimation = getElapsedRealtimeMs();
            g_emptyBufferTicks = 0;
        }

        if (status == AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE) {
            g_emptyBufferTicks++;
        }

        if (g_emptyBufferTicks == 3) {
            LOGI("🎯 STABLE SCENE DETECTED after %d frames!", g_frameCount);
            notifyStableDetected(g_lastAnimation);
        }

        AImage_delete(image);
    }

    // ===================================================================
    // Stop Capture - Called from JNI
    // ===================================================================
    void stopCapture() {
        if (!g_isCapturing) {
            return;
        }

        // Stop choreographer to prevent new callbacks
        choreographer::stop();
        LOGI("Stopping capture...");

        g_isCapturing = false;
        g_isStable.store(true);

        // Clean up resources
        JNIEnv* env = getJNIEnv();

        if (env && g_virtualDisplay) {
            jclass displayClass = env->GetObjectClass(g_virtualDisplay);
            jmethodID releaseMethod = env->GetMethodID(displayClass, "release", "()V");
            if (releaseMethod) {
                env->CallVoidMethod(g_virtualDisplay, releaseMethod);
            }
            env->DeleteGlobalRef(g_virtualDisplay);
            g_virtualDisplay = nullptr;
        }

        if (g_imageReader) {
            AImageReader_delete(g_imageReader);
            g_imageReader = nullptr;
        }

        LOGI("✅ Capture stopped");
    }

} // namespace pixelsampler

// ===================================================================
// JNI Entry Points
// ===================================================================
// pixel_sampler.cpp - Only these JNI methods

extern "C" {

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_nativeInitCapture(
        JNIEnv* env, jobject thiz, jobject mediaProjection, jint width, jint height, jint dpi) {
    pixelsampler::startCapture(env, mediaProjection, width, height, dpi);
}

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_nativeReleaseCapture(
        JNIEnv* env, jobject thiz) {
    pixelsampler::stopCapture();
}

}