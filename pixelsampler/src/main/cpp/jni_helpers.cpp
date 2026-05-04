#include "jni_helpers.h"
#include <android/log.h>
#include <time.h>

JavaVM* sJavaVM = nullptr;

// Global references
static jclass g_pixelSamplerClass = nullptr;
static jmethodID g_onNativeStableMethod = nullptr;
static jobject g_pixelSamplerInstance = nullptr;
static int g_flagPublic = -1;

// ===================================================================
// JNI Environment Helpers
// ===================================================================

JNIEnv* getJNIEnv() {
    if (sJavaVM == nullptr) {
        LOGE("getJNIEnv: sJavaVM is null");
        return nullptr;
    }

    JNIEnv* env = nullptr;
    jint result = sJavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (result == JNI_EDETACHED) {
        result = sJavaVM->AttachCurrentThread(&env, nullptr);
        if (result != JNI_OK) {
            LOGE("getJNIEnv: Failed to attach current thread");
            return nullptr;
        }
    }

    return env;
}

JavaVM* getJavaVM() {
    return sJavaVM;
}

// ===================================================================
// Callback Notification
// ===================================================================

void notifyStableDetected(double lastMoveMs) {
    JNIEnv* env = getJNIEnv();

    if (!env) {
        LOGE("notifyStableDetected: Failed to get JNIEnv");
        return;
    }

    if (!g_pixelSamplerInstance) {
        LOGE("notifyStableDetected: PixelSampler instance is null");
        return;
    }

    if (!g_onNativeStableMethod) {
        LOGE("notifyStableDetected: onNativeStable method ID is null");
        return;
    }

    env->CallVoidMethod(g_pixelSamplerInstance, g_onNativeStableMethod, lastMoveMs);

    // Check for exceptions
    if (env->ExceptionCheck()) {
        LOGE("notifyStableDetected: Exception occurred during callback");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}


// ===================================================================
// VIRTUAL_DISPLAY_FLAG_PUBLIC
// ===================================================================

// Helper to get flag values
static int getDisplayManagerFlag(JNIEnv* env, const char* fieldName) {
    jclass displayManagerClass = env->FindClass("android/hardware/display/DisplayManager");
    if (!displayManagerClass) {
        LOGE("Failed to find DisplayManager class");
        return 0;
    }

    jfieldID fieldId = env->GetStaticFieldID(displayManagerClass, fieldName, "I");
    if (!fieldId) {
        LOGE("Failed to find field: %s", fieldName);
        return 0;
    }

    int value = env->GetStaticIntField(displayManagerClass, fieldId);
    env->DeleteLocalRef(displayManagerClass);

    return value;
}

int getVirtualDisplayFlagPublic(JNIEnv* env) {
    if (g_flagPublic == -1) {
        g_flagPublic = getDisplayManagerFlag(env, "VIRTUAL_DISPLAY_FLAG_PUBLIC");
        LOGI("VIRTUAL_DISPLAY_FLAG_PUBLIC = 0x%x", g_flagPublic);
    }
    return g_flagPublic;
}

// ===================================================================
// Benchmark time count
// ===================================================================

// Get milliseconds as double (matching Kotlin)
double getElapsedRealtimeMs() {
    struct timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);  // ← Consistent source
    return static_cast<double>(ts.tv_sec) * 1000.0 +
            static_cast<double>(ts.tv_nsec) / 1000000.0;
}

// ===================================================================
// JNI_OnLoad
// ===================================================================

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    sJavaVM = vm;
    LOGI("JNI_OnLoad called - PixelSampler native library loading");

    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnLoad: Failed to get JNIEnv");
        return JNI_ERR;
    }

    // ===================================================================
    // 1. Find the PixelSampler Kotlin object class
    // ===================================================================
    jclass localClass = env->FindClass("io/timon/android/pixelsampler/PixelSampler");
    if (!localClass) {
        LOGE("JNI_OnLoad: Failed to find PixelSampler class");
        return JNI_ERR;
    }

    // Store global reference to the class
    g_pixelSamplerClass = (jclass)env->NewGlobalRef(localClass);
    env->DeleteLocalRef(localClass);

    if (!g_pixelSamplerClass) {
        LOGE("JNI_OnLoad: Failed to create global reference for PixelSampler class");
        return JNI_ERR;
    }

    // ===================================================================
    // 2. Get the onNativeStable method ID
    // ===================================================================
    g_onNativeStableMethod = env->GetMethodID(
            g_pixelSamplerClass,
            "onNativeStable",
            "(D)V"
    );

    if (!g_onNativeStableMethod) {
        LOGE("JNI_OnLoad: Failed to find onNativeStable method");
        return JNI_ERR;
    }

    // ===================================================================
    // 3. Get the singleton instance via INSTANCE field (Kotlin object)
    // ===================================================================
    jfieldID instanceField = env->GetStaticFieldID(
            g_pixelSamplerClass,
            "INSTANCE",
            "Lio/timon/android/pixelsampler/PixelSampler;"
    );

    if (!instanceField) {
        LOGE("JNI_OnLoad: Failed to find INSTANCE field");
        return JNI_ERR;
    }

    jobject instance = env->GetStaticObjectField(g_pixelSamplerClass, instanceField);
    if (!instance) {
        LOGE("JNI_OnLoad: INSTANCE field is null");
        return JNI_ERR;
    }

    // Store global reference to the instance
    g_pixelSamplerInstance = env->NewGlobalRef(instance);
    env->DeleteLocalRef(instance);

    if (!g_pixelSamplerInstance) {
        LOGE("JNI_OnLoad: Failed to create global reference for PixelSampler instance");
        return JNI_ERR;
    }

    getVirtualDisplayFlagPublic(env);

    LOGI("✅ JNI_OnLoad completed successfully");
    LOGI("   - PixelSampler class registered");
    LOGI("   - onNativeStable method cached");
    LOGI("   - PixelSampler instance obtained");

    return JNI_VERSION_1_6;
}

// ===================================================================
// JNI_OnUnload
// ===================================================================

void JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnUnload called - Cleaning up resources");

    JNIEnv* env = getJNIEnv();

    if (env) {
        if (g_pixelSamplerClass) {
            env->DeleteGlobalRef(g_pixelSamplerClass);
            g_pixelSamplerClass = nullptr;
            LOGD("Deleted global reference for PixelSampler class");
        }

        if (g_pixelSamplerInstance) {
            env->DeleteGlobalRef(g_pixelSamplerInstance);
            g_pixelSamplerInstance = nullptr;
            LOGD("Deleted global reference for PixelSampler instance");
        }
    } else {
        // If we can't get JNIEnv, just null out the pointers
        g_pixelSamplerClass = nullptr;
        g_pixelSamplerInstance = nullptr;
        LOGW("JNI_OnUnload: Could not get JNIEnv, skipping DeleteGlobalRef");
    }

    g_onNativeStableMethod = nullptr;
    sJavaVM = nullptr;

    LOGI("✅ JNI_OnUnload completed");
}