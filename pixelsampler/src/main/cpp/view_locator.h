#ifndef PIXELSAMPLER_VIEW_LOCATOR_H
#define PIXELSAMPLER_VIEW_LOCATOR_H

#include <jni.h>

class ViewLocator {
public:
    static ViewLocator& getInstance();

    // Find the topmost root view (works for any framework)
    jobject findRootView(JNIEnv* env);

    // Find view by class name (FlutterView, ReactRootView, ComposeView)
    jobject findViewByClass(JNIEnv* env, const char* className);

    // Find view by type from view hierarchy
    enum ViewType {
        VIEW_TYPE_UNKNOWN,
        VIEW_TYPE_COMPOSE,
        VIEW_TYPE_FLUTTER,
        VIEW_TYPE_REACT_NATIVE,
        VIEW_TYPE_XML
    };

    ViewType detectFramework(JNIEnv* env);

private:
    ViewLocator() = default;
    ~ViewLocator() = default;

    jobject getWindowDecorView(JNIEnv* env);
    jobject findViewRecursive(JNIEnv* env, jobject view, const char* targetClass);

    jobject mCachedDecorView{nullptr};
};

#endif //PIXELSAMPLER_VIEW_LOCATOR_H