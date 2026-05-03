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
import androidx.annotation.Keep
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import java.util.concurrent.atomic.AtomicBoolean

object PixelSampler {

    private const val TAG = "PixelSampler"
    private const val CAPTURE_WIDTH = 100
    private const val CAPTURE_HEIGHT = 100

    private var mediaProjection: MediaProjection? = null
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null  // Store reference to prevent GC
    private var isInitialized = false
    private var projectionLauncher: ActivityResultLauncher<Intent>? = null
    private var displayCallback: VirtualDisplay.Callback? = null
    private val isReleased = AtomicBoolean(false)

    init {
        try {
            System.loadLibrary("pixelsampler")
            Log.i(TAG, "✅ Native library loaded")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load native library", e)
        }
    }

    // ✅ Called from native code (via JNI_OnLoad registered callback)
    @Keep
    private fun onNativeStable() {
        Log.i(TAG, "🎯 STABLE RENDER DETECTED - Auto-releasing resources")

        // Notify callback if set
        CoroutineScope(Dispatchers.Main).async {
            release()
        }
    }

    fun start(activity: ComponentActivity) {
        if (isInitialized) return

        isReleased.set(false)

        // Start foreground service
        val serviceIntent = Intent(activity, PixelSamplerService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            activity.startForegroundService(serviceIntent)
        } else {
            activity.startService(serviceIntent)
        }

        // Register for permission result
        projectionLauncher = activity.registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            if (result.resultCode == Activity.RESULT_OK && result.data != null) {
                val mgr = activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
                mediaProjection = mgr.getMediaProjection(result.resultCode, result.data!!)

                // Register MediaProjection callback
                mediaProjection?.registerCallback(object : MediaProjection.Callback() {
                    override fun onStop() {
                        Log.w(TAG, "MediaProjection stopped")
                        release()
                    }
                }, null)

                // Setup capture
                setupCapture(activity)
            } else {
                Log.e(TAG, "User denied permission")
            }
        }

        // Request permission
        val mgr = activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        projectionLauncher?.launch(mgr.createScreenCaptureIntent())
    }

    private fun setupCapture(activity: Activity) {
        // Create ImageReader in Kotlin
        imageReader = ImageReader.newInstance(
            CAPTURE_WIDTH,
            CAPTURE_HEIGHT,
            PixelFormat.RGBA_8888,
            4
        )

        // Pass Surface to native code
        nativeSetSurface(imageReader!!.surface)

        displayCallback = object : VirtualDisplay.Callback() {
            override fun onStopped() {
                Log.w(TAG, "VirtualDisplay stopped")
                isInitialized = false
            }
        }

        // Create VirtualDisplay that mirrors to our ImageReader's Surface
        virtualDisplay = mediaProjection?.createVirtualDisplay(
            "PixelSampler",
            CAPTURE_WIDTH, CAPTURE_HEIGHT,
            160,  // Default DPI (ignored for mirroring)
            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
            imageReader!!.surface,
            displayCallback,
            null
        )

        if (virtualDisplay != null) {
            isInitialized = true
            nativeStartCapture()
            Log.i(TAG, "✅ Capture initialized successfully")
        } else {
            Log.e(TAG, "Failed to create VirtualDisplay")
        }
    }

    fun release() {
        if (!isReleased.compareAndSet(false, true)) {
            Log.d(TAG, "Already released, ignoring")
            return
        }

        virtualDisplay?.release()
        virtualDisplay = null

        imageReader?.close()
        imageReader = null

        mediaProjection?.stop()
        mediaProjection = null

        nativeRelease()

        isInitialized = false
    }

    // Native methods
    private external fun nativeSetSurface(surface: Surface)
    private external fun nativeStartCapture()
    private external fun nativeRelease()
}