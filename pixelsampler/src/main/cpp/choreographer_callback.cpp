// choreographer_callback.cpp
#include "choreographer_callback.h"
#include "jni_helpers.h"
#include <android/choreographer.h>
#include <android/log.h>

ChoreographerCallback& ChoreographerCallback::getInstance() {
    static ChoreographerCallback instance;
    return instance;
}

void ChoreographerCallback::start(FrameCallback callback) {
    if (mIsRunning) {
        LOGW("Choreographer callback already running - ignoring start()");
        return;
    }

    mCallback = std::move(callback);
    mIsRunning = true;

    LOGI("=== ChoreographerCallback::start() called ===");
    LOGI("Registering frame callback with NDK Choreographer...");

    AChoreographer* ac = AChoreographer_getInstance();
    if (ac != nullptr) {
        AChoreographer_postFrameCallback(ac, nativeFrameCallback, this);
        LOGI("✅ Choreographer frame callback posted successfully");
    } else {
        LOGE("❌ Failed to get AChoreographer instance");
    }
}

void ChoreographerCallback::stop() {
    if (!mIsRunning) return;
    mIsRunning = false;
    mCallback = nullptr;
    LOGI("ChoreographerCallback stopped");
}

void ChoreographerCallback::nativeFrameCallback(jlong frameTimeNanos, void* data) {
    auto* instance = static_cast<ChoreographerCallback*>(data);

    LOGI("nativeFrameCallback fired! frameTimeNanos = %lld", frameTimeNanos);

    if (instance && instance->mCallback && instance->mIsRunning) {
        LOGI("Calling user callback (PixelSampler::onFrameCallback)");
        instance->mCallback(frameTimeNanos);

        // ←←← IMPORTANT: Re-post for next frame
        AChoreographer* ac = AChoreographer_getInstance();
        if (ac != nullptr) {
            AChoreographer_postFrameCallback(ac, nativeFrameCallback, instance);
        }
    } else {
        LOGW("Callback skipped (not running or null)");
    }
}