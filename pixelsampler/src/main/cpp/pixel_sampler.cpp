#include "choreographer_callback.h"
#include "jni_helpers.h"
#include "pixel_sampler.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <atomic>
#include <cstring>
#include <vector>

namespace pixelsampler {

    constexpr int SAMPLE_W = 100;
    constexpr int SAMPLE_H = 100;
    constexpr int SAMPLE_BYTES = SAMPLE_W * SAMPLE_H * 4;

    static ANativeWindow* g_nativeWindow = nullptr;

    static std::vector<uint8_t> g_prevPixels(SAMPLE_BYTES, 0);
    static std::atomic<int> g_stableCount{0};
    static std::atomic<bool> g_isStable{false};
    static int g_frameCount = 0;

    static std::atomic<bool> g_isReleased{false};

    static std::chrono::steady_clock::time_point g_lastFrameTime;
    static constexpr int MIN_FRAME_INTERVAL_MS = 33; // ~30 fps max

    // ===================================================================
    // Set Surface - Called from Kotlin with Surface from ImageReader
    // ===================================================================
    void setSurface(JNIEnv* env, jobject surface) {
        // Release existing window if any
        if (g_nativeWindow) {
            ANativeWindow_release(g_nativeWindow);
            g_nativeWindow = nullptr;
        }

        if (surface) {
            g_nativeWindow = ANativeWindow_fromSurface(env, surface);
            if (g_nativeWindow) {
                LOGI("✅ Surface set from Kotlin ImageReader");
            } else {
                LOGE("Failed to create ANativeWindow from Surface");
            }
        }

        g_isReleased.store(false);
        g_stableCount.store(0);
        g_frameCount = 0;
        std::fill(g_prevPixels.begin(), g_prevPixels.end(), 0);
    }

    // ===================================================================
    // Start Capture - Called from Kotlin after VirtualDisplay is ready
    // ===================================================================
    void startCapture() {
        g_isReleased.store(false);
        choreographer::start();
        LOGI("✅ Choreographer started");
    }

    // ===================================================================
    // Release Capture - Clean up all resources
    // ===================================================================
    void releaseCapture() {
        g_isReleased.store(true);

        // Stop choreographer to prevent new callbacks
        choreographer::stop();

        // Then release resources
        if (g_nativeWindow) {
            ANativeWindow_release(g_nativeWindow);
            g_nativeWindow = nullptr;
        }

        LOGI("✅ Native capture released");
    }


    // ===================================================================
    // Frame Callback - Called from Choreographer on every frame
    // ===================================================================
    void onFrameCallback(int64_t /*frameTimeNanos*/) {
        if (g_isReleased.load()) {
            return;
        }

        if (!g_nativeWindow) {
            static bool warned = false;
            if (!warned) {
                LOGE("No Surface available - call setSurface first");
                warned = true;
            }
            return;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastFrameTime).count();

        if (elapsed < MIN_FRAME_INTERVAL_MS) {
            // Skip this frame, we're capturing too fast
            return;
        }
        g_lastFrameTime = now;

        g_frameCount++;

        // Lock the ANativeWindow to access pixel buffer
        ANativeWindow_Buffer buffer;
        int result = ANativeWindow_lock(g_nativeWindow, &buffer, nullptr);

        if (result == 0) {
            if (buffer.bits) {
                // Log occasionally
                if (g_frameCount % 30 == 0) {
                    LOGI("📸 Frame %d captured - size: %dx%d, stride: %d",
                            g_frameCount, buffer.width, buffer.height, buffer.stride);

                    // Print first few pixels to verify content
                    uint8_t* pixels = static_cast<uint8_t*>(buffer.bits);
                    LOGI("First 16 bytes: %02X %02X %02X %02X %02X %02X %02X %02X",
                            pixels[0], pixels[1], pixels[2], pixels[3],
                            pixels[4], pixels[5], pixels[6], pixels[7]);
                }

                // Check for stability (compare first SAMPLE_BYTES)
                uint8_t* currentPixels = static_cast<uint8_t*>(buffer.bits);
                if (std::memcmp(currentPixels, g_prevPixels.data(), SAMPLE_BYTES) == 0) {
                    int stableCount = g_stableCount.fetch_add(1) + 1;
                    if (stableCount == 6 && !g_isStable.load()) {
                        g_isStable.store(true);
                        LOGI("🎯 STABLE RENDER DETECTED after %d frames!", g_frameCount);
                        notifyStableDetected();
                    }
                } else {
                    std::memcpy(g_prevPixels.data(), currentPixels, SAMPLE_BYTES);
                    g_stableCount.store(1);

                    if (g_isStable.load()) {
                        g_isStable.store(false);
                        LOGI("🔄 Render changed at frame %d", g_frameCount);
                    }
                }
            }
            ANativeWindow_unlockAndPost(g_nativeWindow);
        } else {
            if (g_frameCount % 60 == 0) {
                LOGD("Failed to lock ANativeWindow (frame %d)", g_frameCount);
            }
        }
    }

} // namespace pixelsampler

// ==================== JNI Bridge ====================

extern "C" {

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_nativeSetSurface(
        JNIEnv* env, jobject thiz, jobject surface) {
    pixelsampler::setSurface(env, surface);
}

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_nativeStartCapture(
        JNIEnv* env, jobject thiz) {
    pixelsampler::startCapture();
}

JNIEXPORT void JNICALL
Java_io_timon_android_pixelsampler_PixelSampler_nativeRelease(
        JNIEnv* env, jobject thiz) {
    pixelsampler::releaseCapture();
}

}