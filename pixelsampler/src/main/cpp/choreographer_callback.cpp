// choreographer_callback.cpp
#include "choreographer_callback.h"
#include "jni_helpers.h"

ChoreographerCallback& ChoreographerCallback::getInstance() {
    static ChoreographerCallback instance;
    return instance;
}

void ChoreographerCallback::start(FrameCallback callback) {
    if (mIsRunning) {
        LOGW("Choreographer callback already running");
        return;
    }

    mCallback = std::move(callback);
    mIsRunning = true;
    LOGI("Choreographer callback started");
}

void ChoreographerCallback::stop() {
    if (!mIsRunning) return;

    mIsRunning = false;
    mCallback = nullptr;
    LOGI("Choreographer callback stopped");
}

void ChoreographerCallback::nativeFrameCallback(jlong frameTimeNanos, void* data) {
    auto* instance = static_cast<ChoreographerCallback*>(data);
    if (instance && instance->mCallback) {
        instance->mCallback(frameTimeNanos);
    }
}