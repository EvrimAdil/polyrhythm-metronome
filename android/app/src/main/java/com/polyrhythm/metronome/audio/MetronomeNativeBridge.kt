package com.polyrhythm.metronome.audio

import kotlinx.coroutines.*
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.asSharedFlow

data class BeatEventData(
    val timestamp: Long,
    val layerIndex: Int,
    val stepIndex: Int,
    val totalSteps: Int,
    val accent: Int, // 0: Mute, 1: Normal, 2: Downbeat
    val currentPhase: Float,
    val bpm: Int
)

enum class SoundPreset(val id: Int) {
    DIGITAL(0),
    WOODBLOCK(1),
    MECHANICAL(2),
    SNARE_RIM(3),
    HIHAT(4)
}

class MetronomeNativeBridge {
    companion object {
        init {
            System.loadLibrary("polyrhythm_native")
        }
    }

    private val _beatEvents = MutableSharedFlow<BeatEventData>(extraBufferCapacity = 64)
    val beatEvents: SharedFlow<BeatEventData> = _beatEvents.asSharedFlow()

    private var eventPollingJob: Job? = null
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())

    fun initialize(sampleRate: Int = 48000): Boolean {
        return nativeInit(sampleRate)
    }

    fun start() {
        nativeStart()
        startEventPolling()
    }

    fun stop() {
        nativeStop()
        stopEventPolling()
    }

    fun reset() = nativeReset()
    fun isPlaying(): Boolean = nativeIsPlaying()

    fun setBpm(bpm: Double) = nativeSetBpm(bpm)
    fun getBpm(): Double = nativeGetBpm()

    fun setTimeSignature(num: Int, den: Int) = nativeSetTimeSignature(num, den)

    fun addLayer(
        mode: Int, pulses: Int, totalSteps: Int, subdiv: Int = 1,
        volume: Float = 0.8f, pan: Float = 0.0f, pitch: Float = 1.0f, synthFreq: Float = 1000.0f
    ): Int {
        return nativeAddLayer(mode, pulses, totalSteps, subdiv, volume, pan, pitch, synthFreq)
    }

    fun setLayerSoundPreset(layerIdx: Int, preset: SoundPreset) {
        nativeSetLayerSoundPreset(layerIdx, preset.id)
    }

    fun getLayerSoundPreset(layerIdx: Int): SoundPreset {
        val id = nativeGetLayerSoundPreset(layerIdx)
        return SoundPreset.values().firstOrNull { it.id == id } ?: SoundPreset.DIGITAL
    }

    fun removeLayer(layerIdx: Int) = nativeRemoveLayer(layerIdx)

    fun setTempoTrainer(enabled: Boolean, startBpm: Double, targetBpm: Double, stepBpm: Double, interval: Int, autoReverse: Boolean) {
        nativeSetTempoTrainer(enabled, startBpm, targetBpm, stepBpm, interval, autoReverse)
    }

    fun setMuteTrainer(enabled: Boolean, playBars: Int, muteBars: Int) {
        nativeSetMuteTrainer(enabled, playBars, muteBars)
    }

    fun getLayerPhase(layerIdx: Int): Float = nativeGetLayerPhase(layerIdx)
    fun getBarPhase(): Float = nativeGetBarPhase()

    private fun startEventPolling() {
        if (eventPollingJob?.isActive == true) return
        eventPollingJob = scope.launch {
            val buf = LongArray(7)
            while (isActive && isPlaying()) {
                while (nativePopBeatEvent(buf)) {
                    val event = BeatEventData(
                        timestamp = buf[0],
                        layerIndex = buf[1].toInt(),
                        stepIndex = buf[2].toInt(),
                        totalSteps = buf[3].toInt(),
                        accent = buf[4].toInt(),
                        currentPhase = buf[5].toFloat() / 1000.0f,
                        bpm = buf[6].toInt()
                    )
                    _beatEvents.emit(event)
                }
                delay(8) // ~120 Hz polling rate for ultra smooth visual sync
            }
        }
    }

    private fun stopEventPolling() {
        eventPollingJob?.cancel()
        eventPollingJob = null
    }

    // Native JNI Declerations
    private external fun nativeInit(sampleRate: Int): Boolean
    private external fun nativeStart()
    private external fun nativeStop()
    private external fun nativeReset()
    private external fun nativeIsPlaying(): Boolean
    private external fun nativeSetBpm(bpm: Double)
    private external fun nativeGetBpm(): Double
    private external fun nativeSetTimeSignature(numerator: Int, denominator: Int)
    private external fun nativeAddLayer(
        mode: Int, pulses: Int, totalSteps: Int, subdiv: Int,
        volume: Float, pan: Float, pitch: Float, synthFreq: Float
    ): Int
    private external fun nativeSetLayerSoundPreset(layerIdx: Int, presetId: Int)
    private external fun nativeGetLayerSoundPreset(layerIdx: Int): Int
    private external fun nativeRemoveLayer(layerIdx: Int)
    private external fun nativeSetTempoTrainer(
        enabled: Boolean, startBpm: Double, targetBpm: Double, stepBpm: Double, interval: Int, autoReverse: Boolean
    )
    private external fun nativeSetMuteTrainer(enabled: Boolean, playBars: Int, muteBars: Int)
    private external fun nativePopBeatEvent(outArray: LongArray): Boolean
    private external fun nativeGetLayerPhase(layerIdx: Int): Float
    private external fun nativeGetBarPhase(): Float
    private external fun nativeRenderDirect(buffer: FloatArray, numFrames: Int)
}
