#pragma once
#include <jni.h>

namespace choreographer {

    void start(JNIEnv* env, jobject activity);
    void stop();

}