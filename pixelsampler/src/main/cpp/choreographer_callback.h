#ifndef PIXELSAMPLER_CHOREOGRAPHER_CALLBACK_H
#define PIXELSAMPLER_CHOREOGRAPHER_CALLBACK_H

#include <jni.h>
#include <functional>

class ChoreographerCallback {
public:
    using FrameCallback = std::function<void(jlong frameTimeNanos)>;

    static ChoreographerCallback& getInstance();

    void start(FrameCallback callback);
    void stop();
    bool isRunning() const { return mIsRunning; }

private:
    ChoreographerCallback() = default;
    ~ChoreographerCallback() = default;

    static void nativeFrameCallback(jlong frameTimeNanos, void* data);

    FrameCallback mCallback;
    bool mIsRunning{false};
};

#endif