#pragma once

#include <jni.h>

namespace pixelsampler {

    void startCapture(JNIEnv* env, jobject mediaProjection, jint width, jint height, jint dpi);
    void stopCapture();
    void onFrameCallback(int64_t frameTimeNanos);

}