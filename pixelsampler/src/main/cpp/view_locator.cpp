#include "view_locator.h"
#include <android/log.h>

#define LOG_TAG "ViewLocator"

namespace view_locator {

    static jobject g_rootView = nullptr;

    bool findRootView(JNIEnv* env, jobject activity) {
        if (g_rootView) return true;

        jclass activityClass = env->GetObjectClass(activity);
        jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
        jobject window = env->CallObjectMethod(activity, getWindow);

        jclass windowClass = env->GetObjectClass(window);
        jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
        jobject decorView = env->CallObjectMethod(window, getDecorView);

        if (decorView) {
            g_rootView = env->NewGlobalRef(decorView);
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "✅ Root view found successfully");
            return true;
        }

        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to find root view");
        return false;
    }
}