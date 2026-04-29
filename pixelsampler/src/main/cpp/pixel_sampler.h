#ifndef PIXELSAMPLER_PIXEL_SAMPLER_H
#define PIXELSAMPLER_PIXEL_SAMPLER_H

#include <jni.h>
#include <atomic>
#include <cstdint>

class PixelSampler {
public:
    static PixelSampler& getInstance();

    void startSampling(JNIEnv* env, jobject activity);
    void stopSampling();

    // Called from Java
    jobject createImageReader(JNIEnv* env, jint width, jint height);

private:
    PixelSampler() = default;
    ~PixelSampler() = default;

    void onFrameCallback(jlong frameTimeNanos);
    jlong captureCenterPixel();
    void processPixel(jlong hash);
    void notifyComplete(jlong totalTimeMs);

    std::atomic<bool> mIsActive{false};
    std::atomic<int>  mFrameCount{0};
    std::atomic<int>  mConsecutiveUnchangedCount{0};
    std::atomic<jlong> mLastHash{0};

    jobject mJavaCallback{nullptr};
    jobject mRootView{nullptr};

    jlong mStartTimeNs{0};
    jlong mLastSampleTimeNs{0};

    static constexpr int REQUIRED_STABLE_FRAMES = 4;
    static constexpr jlong SAMPLE_INTERVAL_NS = 16'666'666LL;
};

#endif