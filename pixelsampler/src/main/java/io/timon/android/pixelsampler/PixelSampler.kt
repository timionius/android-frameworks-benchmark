package io.timon.android.pixelsampler

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.SystemClock
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts

enum class BenchmarkEvent {
    APP_START,
    FRAMEWORK_ENTRY,
    RENDER_COMPLETE,
}

private const val TAG = "PixelSampler"

object PixelSampler {
    private var permissionLauncher: ActivityResultLauncher<Intent>? = null
    private var appStartTime: Double = 0.0

    init {
        appStartTime = currentTimeMillis()
        markEvent(BenchmarkEvent.APP_START)
        try {
            System.loadLibrary("pixelsampler")
            Log.d(TAG, "✅ Native library loaded")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load native library", e)
        }
    }

    fun init() { /* left intentionally for managed launch at Application start */ }

    fun start(activity: ComponentActivity) {
        if (permissionLauncher != null) {
            Log.w(TAG, "Already started")
            return
        }
        markEvent(BenchmarkEvent.FRAMEWORK_ENTRY)

        val serviceIntent = Intent(activity, PixelSamplerService::class.java)
        activity.startForegroundService(serviceIntent)

        permissionLauncher =
            activity.registerForActivityResult(
                ActivityResultContracts.StartActivityForResult(),
            ) { result ->
                if (result.resultCode == Activity.RESULT_OK && result.data != null) {
                    activity
                        .getSystemService(Context.MEDIA_PROJECTION_SERVICE)
                        .let {
                            (it as MediaProjectionManager).getMediaProjection(result.resultCode, result.data!!)
                        }?.let {
                            Log.d(TAG, "✅ Permission granted")
                            nativeInitCapture(it, 100, 100, 160)
                        }
                } else {
                    Log.e(TAG, "❌ Permission denied")
                    cleanup()
                }
            }

        val projectionManager = activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        permissionLauncher?.launch(projectionManager.createScreenCaptureIntent())
    }

    fun stop() {
        nativeReleaseCapture()
        cleanup()
    }

    private fun cleanup() {
        permissionLauncher = null
    }

    @Suppress("unused")
    private fun onNativeStable(lastMoveMs: Double) {
        val relativeMs = lastMoveMs - appStartTime
        @SuppressLint("LogConditional")
        Log.i(TAG, "🎯 STABLE RENDER DETECTED")
        @SuppressLint("LogConditional")
        Log.i(TAG, String.format("✅ [BENCHMARK] Total time: %.3fms", relativeMs))
        stop()
    }

    private fun markEvent(event: BenchmarkEvent) {
        val currentTime = currentTimeMillis()
        val relativeMs = currentTime - appStartTime
        @SuppressLint("LogConditional")
        Log.i(TAG, String.format("✅ [BENCHMARK] %s: %.3fms", event.name, relativeMs))
    }

    /**
     * Get current monotonic time in milliseconds (similar to CACurrentMediaTime)
     */
    private fun currentTimeMillis(): Double = SystemClock.elapsedRealtimeNanos() / 1_000_000.0

    private external fun nativeInitCapture(
        mediaProjection: MediaProjection,
        width: Int,
        height: Int,
        dpi: Int,
    )

    private external fun nativeReleaseCapture()
}
