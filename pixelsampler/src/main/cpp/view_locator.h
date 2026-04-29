// view_locator.h
#pragma once

#include <jni.h>

class ViewLocator {
public:
    static ViewLocator& getInstance();

    // Main entry point - prefers passed Activity
    jobject findRootViewWithActivity(JNIEnv* env, jobject activity);

    // Legacy (kept for compatibility)
    jobject findRootView(JNIEnv* env);

private:
    ViewLocator() = default;
    ~ViewLocator() = default;
    ViewLocator(const ViewLocator&) = delete;
    ViewLocator& operator=(const ViewLocator&) = delete;

    jobject mCachedDecorView = nullptr;

    // Internal helpers
    jobject getWindowDecorViewFromActivity(JNIEnv* env, jobject activity);
    jobject getContentRootViewFromActivity(JNIEnv* env, jobject activity);
};