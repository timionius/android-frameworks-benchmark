#include "pixel_sampler.h"
#include "jni_helpers.h"
#include "choreographer_callback.h"

#include <media/NdkImageReader.h>
#include <media/NdkImage.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <atomic>
#include <vector>
#include <cstring>

namespace pixelsampler {

    constexpr int SAMPLE_W = 100;
    constexpr int SAMPLE_H = 100;
    constexpr int SAMPLE_BYTES = SAMPLE_W * SAMPLE_H * 4;

    static AImageReader* g_imageReader = nullptr;
    static ANativeWindow* g_nativeWindow = nullptr;
    static AImage* g_latestImage = nullptr;

    static std::vector<uint8_t> g_prevPixels(SAMPLE_BYTES, 0);
    static std::atomic<int> g_stableCount{0};
    static std::atomic<bool> g_isStable{false};
    static int g_frameCount = 0;

    void nativeSetSurface(JNIEnv* env, jobject /*thiz*/, jobject surface) {

        if (g_nativeWindow) {
            ANativeWindow_release(g_nativeWindow);
        }
        if (g_nativeWindow) ANativeWindow_release(g_nativeWindow);

        g_nativeWindow = ANativeWindow_fromSurface(env, surface);
        if (!g_nativeWindow) {
            LOGE("Failed to create ANativeWindow from Surface");
            return;
        }

        media_status_t status = AImageReader_new(
                SAMPLE_W, SAMPLE_H, AIMAGE_FORMAT_RGBA_8888, 6, &g_imageReader);

        if (status == AMEDIA_OK && g_imageReader) {
            LOGI("✅ Native AImageReader (%dx%d) created successfully", SAMPLE_W, SAMPLE_H);
        } else {
            LOGE("Failed to create AImageReader");
        }
    }

    // ===================================================================
    // Choreographer Frame Callback
    // ===================================================================
    void onFrameCallback(int64_t /*frameTimeNanos*/) {
        if (g_isStable.load() || !g_imageReader) return;

        g_frameCount++;

        AImage* image = nullptr;
        media_status_t status = AImageReader_acquireLatestImage(g_imageReader, &image);

        if (status != AMEDIA_OK || !image) {
            if (g_frameCount % 30 == 0) {
                LOGD("No new image available (frame %d)", g_frameCount);
            }
            return;
        }

        uint8_t* data = nullptr;
        int32_t len = 0;
        int32_t pixelStride = 0;

        AImage_getPlaneData(image, 0, &data, &len);
        AImage_getPlanePixelStride(image, 0, &pixelStride);

        if (data && len >= SAMPLE_BYTES && pixelStride == 4) {
            if (std::memcmp(data, g_prevPixels.data(), SAMPLE_BYTES) == 0) {
                g_stableCount.fetch_add(1);
            } else {
                std::memcpy(g_prevPixels.data(), data, SAMPLE_BYTES);
                g_stableCount.store(1);

                // Debug: print first 8 bytes to see if content is changing
                if (g_frameCount % 20 == 0) {
                    LOGI("Frame %d - Pixel data changed (first bytes: %02X %02X %02X %02X ...)",
                            g_frameCount, data[0], data[1], data[2], data[3]);
                }
            }

            if (g_stableCount.load() >= 6 && !g_isStable.load()) {
                g_isStable.store(true);
                LOGI("🎯 STABLE RENDER DETECTED after %d frames!", g_frameCount);
            }
        }

        AImage_delete(image);
    }

    // ===================================================================
    // Release
    // ===================================================================
    void releasePixelSampler() {
        if (g_latestImage) AImage_delete(g_latestImage);
        if (g_imageReader) AImageReader_delete(g_imageReader);
        if (g_nativeWindow) ANativeWindow_release(g_nativeWindow);

        g_imageReader = nullptr;
        g_nativeWindow = nullptr;
        g_latestImage = nullptr;

        g_prevPixels.assign(SAMPLE_BYTES, 0);
        g_stableCount.store(0);
        g_isStable.store(false);
        g_frameCount = 0;

        LOGI("Native PixelSampler released");
    }

} // namespace pixelsampler

// ==================== JNI Bridge (Global extern "C") ====================

extern "C" {

// Surface
JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_nativeSetSurface(
        JNIEnv* env, jobject thiz, jobject surface) {
    pixelsampler::nativeSetSurface(env, thiz, surface);
}

// Choreographer
JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_startChoreographerCallback(
        JNIEnv* env, jobject thiz, jobject activity) {
    choreographer::start(env, activity);
}

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_stopChoreographerNative(
        JNIEnv* env, jobject thiz) {
    choreographer::stop();
    pixelsampler::releasePixelSampler();
}

}