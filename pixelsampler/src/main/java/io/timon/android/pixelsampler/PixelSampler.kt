package io.timon.android.pixelsampler

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.SystemClock
import android.util.Log
import java.lang.ref.WeakReference

enum class BenchmarkEvent {
    APP_START,
    FRAMEWORK_ENTRY,
    RENDER_COMPLETE,
}

private const val TAG = "PixelSampler"
private const val REQUEST_CODE_SCREEN_CAPTURE = 1001

object PixelSampler {
    private var appStartTime: Double = 0.0
    private var currentActivityRef: WeakReference<Activity>? = null
    private var mediaProjection: MediaProjection? = null
    private var pendingPermissionCallback: ((resultCode: Int, data: Intent?) -> Unit)? = null

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

    fun init() { /* intentionally left for explicit initialization */ }

    /**
     * Start sampling - works with ANY Activity type
     * (FlutterActivity, ComponentActivity, ReactActivity, etc.)
     */
    fun start(activity: Activity) {
        if (pendingPermissionCallback != null) {
            Log.w(TAG, "Already started")
            return
        }

        currentActivityRef = WeakReference(activity)

        // Start foreground service
        val serviceIntent = Intent(activity, PixelSamplerService::class.java)
        activity.startForegroundService(serviceIntent)

        // Request screen capture permission
        requestScreenCapturePermission(activity)
    }

    private fun requestScreenCapturePermission(activity: Activity) {
        pendingPermissionCallback = { resultCode, data ->
            handlePermissionResult(resultCode, data)
        }

        val projectionManager = activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        val intent = projectionManager.createScreenCaptureIntent()

        activity.startActivityForResult(intent, REQUEST_CODE_SCREEN_CAPTURE)
    }

    /**
     * IMPORTANT: Call this from your Activity's onActivityResult
     * Required for ALL activity types (Flutter, Compose, React Native, etc.)
     */
    fun onActivityResult(
        requestCode: Int,
        resultCode: Int,
        data: Intent?,
    ) {
        if (requestCode == REQUEST_CODE_SCREEN_CAPTURE) {
            pendingPermissionCallback?.invoke(resultCode, data)
            pendingPermissionCallback = null
        }
    }

    private fun handlePermissionResult(
        resultCode: Int,
        data: Intent?,
    ) {
        val activity =
            currentActivityRef?.get() ?: run {
                Log.e(TAG, "Activity reference lost")
                cleanup()
                return
            }

        if (resultCode == Activity.RESULT_OK && data != null) {
            val projectionManager = activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
            mediaProjection = projectionManager.getMediaProjection(resultCode, data)

            if (mediaProjection != null) {
                Log.d(TAG, "✅ Permission granted")
                nativeInitCapture(mediaProjection!!, 100, 100, 160)
            } else {
                Log.e(TAG, "❌ Failed to get MediaProjection")
                cleanup()
            }
        } else {
            Log.e(TAG, "❌ Permission denied")
            cleanup()
        }
    }

    fun stop() {
        nativeReleaseCapture()
        cleanup()
    }

    private fun cleanup() {
        pendingPermissionCallback = null
        currentActivityRef = null
        mediaProjection = null
    }

    @Suppress("unused")
    private fun onNativeStable(lastMoveMs: Double) {
        val relativeMs = lastMoveMs - appStartTime
        Log.i(TAG, "🎯 STABLE RENDER DETECTED")
        Log.i(TAG, "✅ [BENCHMARK] Total time: ${String.format("%.3f", relativeMs)}ms")
        stop()
    }

    private fun markEvent(event: BenchmarkEvent) {
        val currentTime = currentTimeMillis()
        val relativeMs = currentTime - appStartTime
        Log.i(TAG, "✅ [BENCHMARK] ${event.name}: ${String.format("%.3f", relativeMs)}ms")
    }

    private fun currentTimeMillis(): Double = SystemClock.elapsedRealtimeNanos() / 1_000_000.0

    private external fun nativeInitCapture(
        mediaProjection: MediaProjection,
        width: Int,
        height: Int,
        dpi: Int,
    )

    private external fun nativeReleaseCapture()
}
