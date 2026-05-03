#pragma once

#include <jni.h>
#include <cstdint>

namespace pixelsampler {

    void setSurface(JNIEnv* env, jobject surface);
    void startCapture();
    void releaseCapture();
    void onFrameCallback(int64_t frameTimeNanos);

}