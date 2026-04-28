#ifndef PIXELSAMPLER_PIXEL_SAMPLER_H
#define PIXELSAMPLER_PIXEL_SAMPLER_H

#include <jni.h>
#include <atomic>
#include <functional>

class PixelSampler {
public:
    static PixelSampler& getInstance();

    // Core sampling methods
    void startSampling(JNIEnv* env, jobject callback);
    void stopSampling();
    bool isActive() const { return mIsActive; }

private:
    PixelSampler() = default;
    ~PixelSampler() = default;

    // Frame callback handler (called by Choreographer)
    void onFrameCallback(jlong frameTimeNanos);

    // Core sampling logic
    jlong captureCenterPixel(JNIEnv* env);
    void processPixel(jlong hash);
    void notifyComplete(jlong totalTimeMs);

    // View finding
    jobject findRootView(JNIEnv* env);

    // State variables
    std::atomic<bool> mIsActive{false};
    std::atomic<int> mFrameCount{0};
    std::atomic<int> mConsecutiveUnchangedCount{0};
    std::atomic<jlong> mLastHash{0};
    jobject mJavaCallback{nullptr};
    jobject mRootView{nullptr};
    jlong mStartTimeNs{0};
    jlong mLastSampleTimeNs{0};

    // Configuration
    static constexpr int REQUIRED_STABLE_FRAMES = 4;
    static constexpr long SAMPLE_INTERVAL_NS = 16'666'666L; // ~60fps
};

#endif //PIXELSAMPLER_PIXEL_SAMPLER_H