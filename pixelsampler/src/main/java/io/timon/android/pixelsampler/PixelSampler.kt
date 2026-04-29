package io.timon.android.pixelsampler

import android.app.Activity
import android.media.ImageReader
import android.view.ViewTreeObserver
import android.util.Log
import android.view.Surface

object PixelSampler {

    private const val TAG = "PixelSampler"

    private var globalLayoutListener: ViewTreeObserver.OnGlobalLayoutListener? = null
    private var renderCallback: ((Long) -> Unit)? = null
    private var isInitialized = false

    init {
        try {
            System.loadLibrary("pixelsampler")
            Log.i(TAG, "✅ Native library 'pixelsampler' loaded successfully")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "❌ Failed to load native library 'pixelsampler'", e)
        } catch (e: Exception) {
            Log.e(TAG, "Unexpected error while loading library", e)
        }
    }

    fun start(activity: Activity, onRenderComplete: (timeMs: Long) -> Unit) {
        if (isInitialized) {
            Log.w(TAG, "Already initialized")
            return
        }

        renderCallback = onRenderComplete
        Log.i(TAG, "PixelSampler.start() called - using delayed direct call (MIUI friendly)")

        android.os.Handler(activity.mainLooper).postDelayed({
            try {
                Log.i(TAG, "Delayed call → findRootViewNative")
                val rootFound = findRootViewNative(activity)   // ← Pass real Activity

                Log.i(TAG, "findRootViewNative returned: $rootFound")

                if (rootFound) {
                    isInitialized = true
                    Log.i(TAG, "✅ Root view found → starting Choreographer")
                    startChoreographerCallback(activity)
                } else {
                    Log.e(TAG, "❌ Failed to find root view")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error in native call", e)
            }
        }, 300)
    }

    /** Called from native */
    fun onRenderCompleted(timeMs: Long) {
        renderCallback?.invoke(timeMs)
    }

    /** Called from native C++ - creates 1x1 ImageReader Surface */
    @JvmStatic
    fun createImageReader(width: Int, height: Int): Surface? {
        return try {
            val imageReader = ImageReader.newInstance(width, height, android.graphics.PixelFormat.RGBA_8888, 2)
            val surface = imageReader.surface
            Log.i(TAG, "ImageReader created successfully ($width x $height)")
            surface
        } catch (e: Exception) {
            Log.e(TAG, "Failed to create ImageReader", e)
            null
        }
    }

    private fun retry(activity: Activity, delayMs: Long = 120) {
        android.os.Handler(activity.mainLooper).postDelayed({
            renderCallback?.let { start(activity, it) }
        }, delayMs)
    }

    fun release() {
        globalLayoutListener = null
        renderCallback = null
        isInitialized = false
    }

    private external fun findRootViewNative(activity: Activity): Boolean
    private external fun startChoreographerCallback(activity: Activity)

}