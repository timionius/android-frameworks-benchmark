#ifndef PIXELSAMPLER_JNI_HELPERS_H
#define PIXELSAMPLER_JNI_HELPERS_H

#include <jni.h>
#include <android/log.h>

// Logging macros
#define LOG_TAG "PixelSampler"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// JNI helper functions
JNIEnv* getJNIEnv();
void notifyStableDetected(double lastMoveMs);
int getVirtualDisplayFlagPublic(JNIEnv* env);
double getElapsedRealtimeMs();
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved);

#endif