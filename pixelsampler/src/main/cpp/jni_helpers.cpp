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

jobject getCurrentActivity(JNIEnv* env) {
    // Get ActivityThread
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(
            activityThreadClass,
            "currentActivityThread",
            "()Landroid/app/ActivityThread;"
    );
    jobject activityThread = env->CallStaticObjectMethod(activityThreadClass, currentActivityThread);

    // Get activities list
    jmethodID getActivities = env->GetMethodID(
            activityThreadClass,
            "getActivities",
            "()Ljava/util/List;"
    );
    jobject activities = env->CallObjectMethod(activityThread, getActivities);

    // Simplified - get first activity
    // Full implementation would iterate through the list

    env->DeleteLocalRef(activityThread);
    if (activities != nullptr) env->DeleteLocalRef(activities);

    return nullptr; // Simplified for now
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    sJavaVM = vm;
    LOGI("JNI_OnLoad called - Native library loaded");
    return JNI_VERSION_1_6;
}