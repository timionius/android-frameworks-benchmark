#pragma once

#include <jni.h>

namespace view_locator {
    bool findRootView(JNIEnv* env, jobject activity);
}