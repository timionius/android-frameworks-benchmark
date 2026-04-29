// view_locator.cpp
#include "view_locator.h"
#include "jni_helpers.h"
#include <android/log.h>

#define LOG_TAG "PixelSampler"

ViewLocator& ViewLocator::getInstance() {
    static ViewLocator instance;
    return instance;
}

// ================== MAIN ENTRY POINT ==================
jobject ViewLocator::findRootViewWithActivity(JNIEnv* env, jobject activity) {
    if (mCachedDecorView != nullptr) {
        LOGI(LOG_TAG, "Using cached root view");
        return mCachedDecorView;
    }

    LOGI(LOG_TAG, "Trying to find root view using passed Activity...");

    // Strategy 1: Direct from passed Activity (most reliable)
    jobject decor = getWindowDecorViewFromActivity(env, activity);
    if (decor != nullptr) {
        mCachedDecorView = env->NewGlobalRef(decor);
        LOGI(LOG_TAG, "✅ Root view found via Activity.getWindow().getDecorView()");
        return mCachedDecorView;
    }

    // Strategy 2: Content root fallback (good for MIUI)
    decor = getContentRootViewFromActivity(env, activity);
    if (decor != nullptr) {
        mCachedDecorView = env->NewGlobalRef(decor);
        LOGI(LOG_TAG, "✅ Root view found via android.R.id.content -> getRootView()");
        return mCachedDecorView;
    }

    LOGE(LOG_TAG, "❌ All strategies failed to find root view");
    return nullptr;
}

// Legacy version (for backward compatibility)
jobject ViewLocator::findRootView(JNIEnv* env) {
    LOGW(LOG_TAG, "findRootView() called without Activity - using fallback");
    return findRootViewWithActivity(env, nullptr);
}

// ================== HELPERS ==================
jobject ViewLocator::getWindowDecorViewFromActivity(JNIEnv* env, jobject activity) {
    if (activity == nullptr) return nullptr;

    jclass activityClass = env->GetObjectClass(activity);
    jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
    if (getWindow == nullptr) {
        env->DeleteLocalRef(activityClass);
        return nullptr;
    }

    jobject window = env->CallObjectMethod(activity, getWindow);
    env->DeleteLocalRef(activityClass);
    if (window == nullptr) return nullptr;

    jclass windowClass = env->GetObjectClass(window);
    jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");

    jobject decorView = env->CallObjectMethod(window, getDecorView);

    env->DeleteLocalRef(window);
    env->DeleteLocalRef(windowClass);

    return decorView;
}

jobject ViewLocator::getContentRootViewFromActivity(JNIEnv* env, jobject activity) {
    if (activity == nullptr) return nullptr;

    jclass activityClass = env->GetObjectClass(activity);
    jmethodID findViewById = env->GetMethodID(activityClass, "findViewById", "(I)Landroid/view/View;");
    if (findViewById == nullptr) {
        env->DeleteLocalRef(activityClass);
        return nullptr;
    }

    jobject contentView = env->CallObjectMethod(activity, findViewById, 0x01020002); // android.R.id.content
    env->DeleteLocalRef(activityClass);

    if (contentView == nullptr) return nullptr;

    jclass viewClass = env->GetObjectClass(contentView);
    jmethodID getRootView = env->GetMethodID(viewClass, "getRootView", "()Landroid/view/View;");
    jobject root = env->CallObjectMethod(contentView, getRootView);

    env->DeleteLocalRef(contentView);
    env->DeleteLocalRef(viewClass);

    return root;
}