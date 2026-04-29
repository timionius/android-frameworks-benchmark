#include "choreographer_callback.h"
#include "pixel_sampler.h"
#include "jni_helpers.h"

#include <android/choreographer.h>

static AChoreographer* g_choreographer = nullptr;

namespace choreographer {

    static void onFrameCallback(int64_t frameTimeNanos, void* /*data*/) {
        pixelsampler::onFrameCallback(frameTimeNanos);

        // Re-post for continuous callbacks
        if (g_choreographer) {
            AChoreographer_postFrameCallback64(g_choreographer, onFrameCallback, nullptr);
        }
    }

    void start(JNIEnv* /*env*/, jobject /*activity*/) {
        if (g_choreographer) return;

        g_choreographer = AChoreographer_getInstance();
        if (g_choreographer) {
            AChoreographer_postFrameCallback64(g_choreographer, onFrameCallback, nullptr);
            LOGI("✅ Choreographer frame callbacks started (continuous)");
        } else {
            LOGE("Failed to get AChoreographer instance");
        }
    }

    void stop() {
        g_choreographer = nullptr;
    }

}