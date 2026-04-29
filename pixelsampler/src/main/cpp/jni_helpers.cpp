// jni_helpers.cpp
#include "jni_helpers.h"

static JavaVM* sJavaVM = nullptr;

JNIEnv* getJNIEnv() {
    if (sJavaVM == nullptr) return nullptr;

    JNIEnv* env = nullptr;
    jint result = sJavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (result == JNI_EDETACHED) {
        result = sJavaVM->AttachCurrentThread(&env, nullptr);
        if (result != JNI_OK) return nullptr;
    }
    return env;
}

JavaVM* getJavaVM() {
    return sJavaVM;
}

// Ultra-safe version - avoids crashing on MIUI / restricted devices
jobject getCurrentActivity(JNIEnv* env) {
    if (env == nullptr) return nullptr;

    env->ExceptionClear();

    // Try ActivityThread -> getTopActivity (most devices)
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (activityThreadClass != nullptr) {
        jmethodID current = env->GetStaticMethodID(activityThreadClass,
                "currentActivityThread", "()Landroid/app/ActivityThread;");

        if (current != nullptr) {
            jobject activityThread = env->CallStaticObjectMethod(activityThreadClass, current);
            if (activityThread != nullptr) {
                jmethodID getTop = env->GetMethodID(env->GetObjectClass(activityThread),
                        "getTopActivity", "()Landroid/app/Activity;");

                if (getTop != nullptr) {
                    jobject activity = env->CallObjectMethod(activityThread, getTop);
                    env->DeleteLocalRef(activityThread);
                    env->DeleteLocalRef(activityThreadClass);
                    return activity;
                }
                env->DeleteLocalRef(activityThread);
            }
        }
        env->DeleteLocalRef(activityThreadClass);
    }

    env->ExceptionClear();
    LOGW("Could not get current Activity via standard APIs. Root view finding will be limited.");
    return nullptr;
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    sJavaVM = vm;
    LOGI("JNI_OnLoad called - PixelSampler native library loaded successfully");
    return JNI_VERSION_1_6;
}