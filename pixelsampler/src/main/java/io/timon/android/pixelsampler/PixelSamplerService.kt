package io.timon.android.pixelsampler

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.IBinder
import android.util.Log

private const val TAG = "PixelSamplerService"

class PixelSamplerService : Service() {
    companion object {
        const val CHANNEL_ID = "pixel_sampler_channel"
        const val NOTIFICATION_ID = 1001
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground()
        Log.i(TAG, "PixelSamplerService started in foreground")
    }

    private fun startForeground() {
        val notification =
            Notification
                .Builder(this, CHANNEL_ID)
                .setContentTitle("PixelSampler")
                .setContentText("Screen sampling active")
                .setSmallIcon(android.R.drawable.ic_dialog_info) // Replace with your icon
                .setOngoing(true)
                .build()

        startForeground(
            NOTIFICATION_ID,
            notification,
            ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION,
        )
    }

    private fun createNotificationChannel() {
        val channel =
            NotificationChannel(
                CHANNEL_ID,
                "PixelSampler Screen Capture",
                NotificationManager.IMPORTANCE_LOW,
            ).apply {
                description = "Used for stable render detection"
                setShowBadge(false)
            }
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(channel)
    }

    override fun onStartCommand(
        intent: Intent?,
        flags: Int,
        startId: Int,
    ): Int = START_STICKY

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        super.onDestroy()
        PixelSampler.stop()
        Log.i(TAG, "PixelSamplerService destroyed")
    }
}
