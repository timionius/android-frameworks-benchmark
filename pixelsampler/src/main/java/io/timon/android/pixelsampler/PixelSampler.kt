package io.timon.android.pixelsampler

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts

object PixelSampler {

    private const val TAG = "PixelSampler"

    private var permissionLauncher: ActivityResultLauncher<Intent>? = null
    private var appStartTime: Long = 0

    init {
        try {
            System.loadLibrary("pixelsampler")
            Log.i(TAG, "✅ Native library loaded")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load native library", e)
        }
    }

    fun start(activity: ComponentActivity) {
        if (permissionLauncher != null) {
            Log.w(TAG, "Already started")
            return
        }

        appStartTime = System.currentTimeMillis()

        val serviceIntent = Intent(activity, PixelSamplerService::class.java)
        activity.startForegroundService(serviceIntent)

        permissionLauncher = activity.registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            if (result.resultCode == Activity.RESULT_OK && result.data != null) {
//                val projectionManager =
                activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE)
                    .let {
                        (it as MediaProjectionManager).getMediaProjection(result.resultCode, result.data!!)
                    }
                    ?.let {
                        Log.i(TAG, "✅ Permission granted")
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
    private fun onNativeStable() {
        val totalTime = System.currentTimeMillis() - appStartTime
        Log.i(TAG, "🎯 STABLE RENDER DETECTED after ${totalTime}ms")
        stop()
    }

    // Native methods with clear, unique names
    private external fun nativeInitCapture(mediaProjection: MediaProjection, width: Int, height: Int, dpi: Int)
    private external fun nativeReleaseCapture()
}