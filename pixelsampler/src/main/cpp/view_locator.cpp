// view_locator.cpp
#include "view_locator.h"
#include "jni_helpers.h"
#include <cstring>

ViewLocator& ViewLocator::getInstance() {
    static ViewLocator instance;
    return instance;
}

jobject ViewLocator::findRootView(JNIEnv* env) {
    if (mCachedDecorView != nullptr) {
        return mCachedDecorView;
    }

    jobject decorView = getWindowDecorView(env);
    if (decorView != nullptr) {
        mCachedDecorView = env->NewGlobalRef(decorView);
        LOGI("Found root decor view successfully");
        return mCachedDecorView;
    }

    LOGE("Failed to find root view");
    return nullptr;
}

jobject ViewLocator::getWindowDecorView(JNIEnv* env) {
    jobject activity = getCurrentActivity(env);
    if (activity == nullptr) return nullptr;

    jclass activityClass = env->GetObjectClass(activity);
    jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
    if (getWindow == nullptr) {
        env->DeleteLocalRef(activity);
        return nullptr;
    }

    jobject window = env->CallObjectMethod(activity, getWindow);
    env->DeleteLocalRef(activity);
    if (window == nullptr) return nullptr;

    jclass windowClass = env->GetObjectClass(window);
    jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");

    jobject decorView = env->CallObjectMethod(window, getDecorView);

    env->DeleteLocalRef(window);
    env->DeleteLocalRef(windowClass);

    return decorView;
}

// Keep the rest of your helper functions if needed
jobject ViewLocator::findViewByClass(JNIEnv* env, const char* targetClassName) {
    // ... your existing implementation
    return nullptr;
}