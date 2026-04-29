package io.timon.android.pixelsampler

import android.media.Image
import android.media.ImageReader
import android.os.Handler
import android.os.Looper
import android.view.Surface

object PixelSampler {

    init {
        System.loadLibrary("pixelsampler")
    }

    private var imageReader: ImageReader? = null
    private var onRenderComplete: ((Long) -> Unit)? = null

    fun start(onComplete: (Long) -> Unit) {
        onRenderComplete = onComplete
        startNative()
    }

    fun stop() {
        stopNative()
        cleanupImageReader()
    }

    private fun cleanupImageReader() {
        imageReader?.close()
        imageReader = null
    }

    // Called from Native
    @JvmStatic
    fun createImageReader(width: Int, height: Int): Surface? {
        cleanupImageReader()

        imageReader = ImageReader.newInstance(
            width, height,
            android.graphics.ImageFormat.FLEX_RGBA_8888,
            3, // max images
            android.hardware.HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or android.hardware.HardwareBuffer.USAGE_CPU_READ_OFTEN
        )

        imageReader?.setOnImageAvailableListener({ reader ->
            reader.acquireLatestImage()?.let { image ->
                processImageNative(image)
                image.close()
            }
        }, Handler(Looper.getMainLooper()))

        return imageReader?.surface
    }

    @JvmStatic
    private external fun startNative()
    @JvmStatic
    private external fun stopNative()
    @JvmStatic
    private external fun processImageNative(image: Image)
}