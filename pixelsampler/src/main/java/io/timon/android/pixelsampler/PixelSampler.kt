package io.timon.android.pixelsampler

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.ImageReader
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.Build
import android.util.Log
import android.view.Surface
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts

object PixelSampler {

    private const val TAG = "PixelSampler"

    private var mediaProjection: MediaProjection? = null
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null

    private var isInitialized = false
    private var projectionLauncher: ActivityResultLauncher<Intent>? = null

    init {
        try {
            System.loadLibrary("pixelsampler")
            Log.i(TAG, "✅ Native library 'pixelsampler' loaded successfully")
        } catch (e: Exception) {
            Log.e(TAG, "❌ Failed to load native library", e)
        }
    }

    /** Must be called from Activity's onCreate() */
    fun register(activity: ComponentActivity) {
        projectionLauncher = activity.registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            if (result.resultCode == Activity.RESULT_OK && result.data != null) {
                val mgr = activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
                mediaProjection = mgr.getMediaProjection(result.resultCode, result.data!!)
                setupCapture(activity)
            } else {
                Log.e(TAG, "❌ User denied MediaProjection permission")
            }
        }
    }

    fun start(activity: ComponentActivity) {
        if (isInitialized) return

        val serviceIntent = Intent(activity, PixelSamplerService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            activity.startForegroundService(serviceIntent)
        } else {
            activity.startService(serviceIntent)
        }

        register(activity)  // Register launcher first

        val mgr = activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        val intent = mgr.createScreenCaptureIntent()
        projectionLauncher?.launch(intent)
    }

    private fun setupCapture(activity: Activity) {
        val metrics = activity.resources.displayMetrics

        imageReader = ImageReader.newInstance(
            100, 100,
            PixelFormat.RGBA_8888,
            4
        )

        nativeSetSurface(imageReader!!.surface)

        virtualDisplay = mediaProjection?.createVirtualDisplay(
            "PixelSampler",
            100, 100,
            metrics.densityDpi,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
            imageReader?.surface,
            null,
            null
        )

        isInitialized = true
        startChoreographerCallback(activity)

        Log.i(TAG, "✅ MediaProjection + VirtualDisplay + Native ImageReader initialized")
    }

    fun release() {
        virtualDisplay?.release()
        imageReader?.close()
        mediaProjection?.stop()
        stopChoreographerNative()
        isInitialized = false
        Log.i(TAG, "PixelSampler released")
    }

    // Native bridge
    private external fun nativeSetSurface(surface: android.view.Surface)
    private external fun startChoreographerCallback(activity: Activity)
    private external fun stopChoreographerNative()

}