#include "choreographer_callback.h"
#include "jni_helpers.h"

// Java class and method IDs (cached for performance)
static jclass gChoreographerClass = nullptr;
static jmethodID gGetInstanceMethod = nullptr;
static jmethodID gPostFrameCallbackMethod = nullptr;

ChoreographerCallback& ChoreographerCallback::getInstance() {
    static ChoreographerCallback instance;
    return instance;
}

void ChoreographerCallback::start(FrameCallback callback) {
    if (mIsRunning) {
        LOGW("Choreographer callback already running");
        return;
    }

    mCallback = callback;
    mIsRunning = true;

    JNIEnv* env = getJNIEnv();
    if (env == nullptr) {
        LOGE("Failed to get JNIEnv");
        return;
    }

    // Cache class and method IDs
    if (gChoreographerClass == nullptr) {
        jclass localClass = env->FindClass("android/view/Choreographer");
        gChoreographerClass = (jclass)env->NewGlobalRef(localClass);
        gGetInstanceMethod = env->GetStaticMethodID(gChoreographerClass, "getInstance", "()Landroid/view/Choreographer;");
        gPostFrameCallbackMethod = env->GetMethodID(gChoreographerClass, "postFrameCallback", "(Landroid/view/Choreographer$FrameCallback;)V");
    }

    // Get Choreographer instance
    jobject choreographer = env->CallStaticObjectMethod(gChoreographerClass, gGetInstanceMethod);
    mChoreographer = env->NewGlobalRef(choreographer);

    // Create FrameCallback proxy
    // Note: Full implementation would require a Java proxy class
    // For now, using the native callback approach

    LOGI("Choreographer callback started");
}

void ChoreographerCallback::stop() {
    if (!mIsRunning) return;

    mIsRunning = false;
    mCallback = nullptr;

    JNIEnv* env = getJNIEnv();
    if (env) {
        if (mChoreographer) {
            env->DeleteGlobalRef(mChoreographer);
            mChoreographer = nullptr;
        }
        if (mFrameCallback) {
            env->DeleteGlobalRef(mFrameCallback);
            mFrameCallback = nullptr;
        }
    }

    LOGI("Choreographer callback stopped");
}

void ChoreographerCallback::nativeFrameCallback(jlong frameTimeNanos, void* data) {
    auto* instance = static_cast<ChoreographerCallback*>(data);
    if (instance && instance->mCallback) {
        instance->mCallback(frameTimeNanos);
    }
}