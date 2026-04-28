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

    // Get decor view (works for all frameworks)
    jobject decorView = getWindowDecorView(env);
    if (decorView != nullptr) {
        mCachedDecorView = env->NewGlobalRef(decorView);
        LOGI("Found decor view");
        return mCachedDecorView;
    }

    LOGE("Could not find root view");
    return nullptr;
}

jobject ViewLocator::getWindowDecorView(JNIEnv* env) {
    jobject activity = getCurrentActivity(env);
    if (activity == nullptr) return nullptr;

    jclass activityClass = env->GetObjectClass(activity);
    jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
    jobject window = env->CallObjectMethod(activity, getWindow);

    if (window == nullptr) return nullptr;

    jclass windowClass = env->GetObjectClass(window);
    jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
    jobject decorView = env->CallObjectMethod(window, getDecorView);

    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(window);

    return decorView;
}

jobject ViewLocator::findViewByClass(JNIEnv* env, const char* targetClassName) {
    jobject decorView = getWindowDecorView(env);
    if (decorView == nullptr) return nullptr;

    return findViewRecursive(env, decorView, targetClassName);
}

jobject ViewLocator::findViewRecursive(JNIEnv* env, jobject view, const char* targetClassName) {
    if (view == nullptr) return nullptr;

    // Check current view's class name
    jclass viewClass = env->GetObjectClass(view);
    jmethodID getClass = env->GetMethodID(viewClass, "getClass", "()Ljava/lang/Class;");
    jobject viewClassObj = env->CallObjectMethod(view, getClass);

    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getName = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jstring nameStr = (jstring)env->CallObjectMethod(viewClassObj, getName);

    const char* className = env->GetStringUTFChars(nameStr, nullptr);

    if (strcmp(className, targetClassName) == 0) {
        env->ReleaseStringUTFChars(nameStr, className);
        return view;
    }

    env->ReleaseStringUTFChars(nameStr, className);

    // Check children
    jmethodID getChildCount = env->GetMethodID(viewClass, "getChildCount", "()I");
    jmethodID getChildAt = env->GetMethodID(viewClass, "getChildAt", "(I)Landroid/view/View;");

    jint childCount = env->CallIntMethod(view, getChildCount);
    for (jint i = 0; i < childCount; i++) {
        jobject child = env->CallObjectMethod(view, getChildAt, i);
        jobject found = findViewRecursive(env, child, targetClassName);
        if (found != nullptr) return found;
        env->DeleteLocalRef(child);
    }

    return nullptr;
}

ViewLocator::ViewType ViewLocator::detectFramework(JNIEnv* env) {
    if (findViewByClass(env, "androidx.compose.ui.platform.ComposeView") != nullptr) {
        return VIEW_TYPE_COMPOSE;
    }
    if (findViewByClass(env, "io.flutter.view.FlutterView") != nullptr) {
        return VIEW_TYPE_FLUTTER;
    }
    if (findViewByClass(env, "com.facebook.react.ReactRootView") != nullptr) {
        return VIEW_TYPE_REACT_NATIVE;
    }
    return VIEW_TYPE_XML;
}