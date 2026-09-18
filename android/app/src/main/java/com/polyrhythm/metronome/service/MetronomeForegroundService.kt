package com.polyrhythm.metronome.service

import android.app.*
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.media.AudioAttributes
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import androidx.core.app.NotificationCompat
import com.polyrhythm.metronome.audio.MetronomeNativeBridge

class MetronomeForegroundService : Service() {

    companion object {
        const val CHANNEL_ID = "polyrhythm_metronome_playback"
        const val NOTIFICATION_ID = 1001
        const val ACTION_START = "ACTION_START"
        const val ACTION_STOP = "ACTION_STOP"
        const val ACTION_TOGGLE = "ACTION_TOGGLE"

        val audioBridge = MetronomeNativeBridge()
    }

    private var wakeLock: PowerManager.WakeLock? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        acquireWakeLock()
        audioBridge.initialize(48000)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                audioBridge.start()
                startForegroundNotification(isPlaying = true)
            }
            ACTION_STOP -> {
                audioBridge.stop()
                startForegroundNotification(isPlaying = false)
                stopForeground(STOP_FOREGROUND_DETACH)
            }
            ACTION_TOGGLE -> {
                if (audioBridge.isPlaying()) {
                    audioBridge.stop()
                    startForegroundNotification(isPlaying = false)
                } else {
                    audioBridge.start()
                    startForegroundNotification(isPlaying = true)
                }
            }
        }
        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        audioBridge.stop()
        releaseWakeLock()
        super.onDestroy()
    }

    private fun acquireWakeLock() {
        val powerManager = getSystemService(Context.POWER_SERVICE) as PowerManager
        wakeLock = powerManager.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "PolyrhythmMetronome::AudioPlaybackWakeLock"
        ).apply {
            setReferenceCounted(false)
            acquire(12 * 60 * 60 * 1000L) // 12 Saat maksimum
        }
    }

    private fun releaseWakeLock() {
        wakeLock?.let {
            if (it.isHeld) it.release()
        }
        wakeLock = null
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "Polyrhythm Metronome Audio Playback",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Kesintisiz poliritmik ses motoru bildirimi"
                setSound(null, null)
            }
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(channel)
        }
    }

    private fun startForegroundNotification(isPlaying: Boolean) {
        val toggleIntent = Intent(this, MetronomeForegroundService::class.java).apply {
            action = ACTION_TOGGLE
        }
        val togglePendingIntent = PendingIntent.getService(
            this, 0, toggleIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Polyrhythm Metronome")
            .setContentText(if (isPlaying) "Ritim Çalıyor (${audioBridge.getBpm().toInt()} BPM)" else "Duraklatıldı")
            .setSmallIcon(android.R.drawable.ic_media_play)
            .addAction(
                if (isPlaying) android.R.drawable.ic_media_pause else android.R.drawable.ic_media_play,
                if (isPlaying) "Duraklat" else "Başlat",
                togglePendingIntent
            )
            .setOngoing(isPlaying)
            .build()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(
                NOTIFICATION_ID,
                notification,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK
            )
        } else {
            startForeground(NOTIFICATION_ID, notification)
        }
    }
}
