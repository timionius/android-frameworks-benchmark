#include "choreographer_callback.h"
#include "pixel_sampler.h"
#include "jni_helpers.h"

#include <android/choreographer.h>
#include <atomic>

static AChoreographer* g_choreographer = nullptr;
static std::atomic<bool> g_isRunning{false};

namespace choreographer {

    static void onFrameCallback(int64_t frameTimeNanos, void* /*data*/) {
        // ✅ Don't process if not running
        LOGI("✅ Choreographer onFrameCallback entry");
        if (!g_isRunning.load()) {
            return;
        }

        // Process the frame
        pixelsampler::onFrameCallback(frameTimeNanos);

        // ✅ Only re-post if still running
        if (g_isRunning.load() && g_choreographer) {
            AChoreographer_postFrameCallback64(g_choreographer, onFrameCallback, nullptr);
            LOGI("✅ Choreographer onFrameCallback post");
        }
    }

    void start() {
        if (g_isRunning.load()) {
            LOGI("Choreographer already running");
            return;
        }

        g_choreographer = AChoreographer_getInstance();
        if (g_choreographer) {
            g_isRunning.store(true);
            AChoreographer_postFrameCallback64(g_choreographer, onFrameCallback, nullptr);
            LOGI("✅ Choreographer frame callbacks started (continuous)");
        } else {
            LOGE("Failed to get AChoreographer instance");
        }
    }

    void stop() {
        // ✅ Just set the flag - no immediate cleanup
        g_isRunning.store(false);
        // Don't clear g_choreographer yet - let in-flight callbacks finish
        LOGI("Choreographer stop requested - callbacks will cease");
    }

} // namespace choreographer