#pragma once
#include <jni.h>

namespace pixelsampler {
    void onFrameCallback(int64_t frameTimeNanos);
    void nativeSetSurface(JNIEnv* env, jobject thiz, jobject surface);
    void releasePixelSampler();
}