#include <jni.h>
#include <string>
#include <memory>
#include "c_bridge.h"
#include "OboeAudioDriver.hpp"

static std::unique_ptr<OboeAudioDriver> sDriver = nullptr;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeInit(
    JNIEnv* /*env*/, jobject /*thiz*/, jint sampleRate) {
    if (!sDriver) {
        sDriver = std::make_unique<OboeAudioDriver>();
    }
    return sDriver->start(static_cast<uint32_t>(sampleRate)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeStart(
    JNIEnv* /*env*/, jobject /*thiz*/) {
    if (sDriver) {
        polyrhythm_start(sDriver->getEngineHandle());
    }
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeStop(
    JNIEnv* /*env*/, jobject /*thiz*/) {
    if (sDriver) {
        polyrhythm_stop(sDriver->getEngineHandle());
    }
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeReset(
    JNIEnv* /*env*/, jobject /*thiz*/) {
    if (sDriver) {
        polyrhythm_reset(sDriver->getEngineHandle());
    }
}

JNIEXPORT jboolean JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeIsPlaying(
    JNIEnv* /*env*/, jobject /*thiz*/) {
    if (sDriver) {
        return polyrhythm_is_playing(sDriver->getEngineHandle()) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeSetBpm(
    JNIEnv* /*env*/, jobject /*thiz*/, jdouble bpm) {
    if (sDriver) {
        polyrhythm_set_bpm(sDriver->getEngineHandle(), bpm);
    }
}

JNIEXPORT jdouble JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeGetBpm(
    JNIEnv* /*env*/, jobject /*thiz*/) {
    if (sDriver) {
        return polyrhythm_get_bpm(sDriver->getEngineHandle());
    }
    return 120.0;
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeSetTimeSignature(
    JNIEnv* /*env*/, jobject /*thiz*/, jint numerator, jint denominator) {
    if (sDriver) {
        polyrhythm_set_time_signature(sDriver->getEngineHandle(), static_cast<uint16_t>(numerator), static_cast<uint16_t>(denominator));
    }
}

JNIEXPORT jint JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeAddLayer(
    JNIEnv* /*env*/, jobject /*thiz*/,
    jint mode, jint pulses, jint totalSteps, jint subdiv,
    jfloat volume, jfloat pan, jfloat pitch, jfloat synthFreq) {
    if (!sDriver) return -1;

    CLayerConfig cfg{};
    cfg.mode = static_cast<uint8_t>(mode);
    cfg.ratio_pulses = static_cast<uint16_t>(pulses);
    cfg.total_steps = static_cast<uint16_t>(totalSteps);
    cfg.subdivision = static_cast<uint16_t>(subdiv);
    cfg.volume = volume;
    cfg.pan = pan;
    cfg.pitch_shift = pitch;
    cfg.synth_frequency = synthFreq;
    cfg.sound_preset = 0; // Digital default
    cfg.accent_count = cfg.total_steps;
    cfg.accents[0] = 2; // Downbeat
    for (size_t i = 1; i < 64; ++i) {
        cfg.accents[i] = 1; // Normal
    }

    return static_cast<jint>(polyrhythm_add_layer(sDriver->getEngineHandle(), &cfg));
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeSetLayerSoundPreset(
    JNIEnv* /*env*/, jobject /*thiz*/, jint layerIdx, jint presetId) {
    if (sDriver) {
        polyrhythm_set_layer_sound_preset(sDriver->getEngineHandle(), static_cast<size_t>(layerIdx), static_cast<uint8_t>(presetId));
    }
}

JNIEXPORT jint JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeGetLayerSoundPreset(
    JNIEnv* /*env*/, jobject /*thiz*/, jint layerIdx) {
    if (sDriver) {
        return static_cast<jint>(polyrhythm_get_layer_sound_preset(sDriver->getEngineHandle(), static_cast<size_t>(layerIdx)));
    }
    return 0;
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeRemoveLayer(
    JNIEnv* /*env*/, jobject /*thiz*/, jint layerIdx) {
    if (sDriver) {
        polyrhythm_remove_layer(sDriver->getEngineHandle(), static_cast<size_t>(layerIdx));
    }
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeSetTempoTrainer(
    JNIEnv* /*env*/, jobject /*thiz*/,
    jboolean enabled, jdouble startBpm, jdouble targetBpm, jdouble stepBpm, jint interval, jboolean autoReverse) {
    if (!sDriver) return;
    CTempoTrainerConfig cfg{};
    cfg.enabled = enabled;
    cfg.start_bpm = startBpm;
    cfg.target_bpm = targetBpm;
    cfg.step_bpm = stepBpm;
    cfg.bars_interval = static_cast<uint32_t>(interval);
    cfg.auto_reverse = autoReverse;
    polyrhythm_set_tempo_trainer(sDriver->getEngineHandle(), &cfg);
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeSetMuteTrainer(
    JNIEnv* /*env*/, jobject /*thiz*/,
    jboolean enabled, jint playBars, jint muteBars) {
    if (!sDriver) return;
    CMuteTrainerConfig cfg{};
    cfg.enabled = enabled;
    cfg.play_bars = static_cast<uint32_t>(playBars);
    cfg.mute_bars = static_cast<uint32_t>(muteBars);
    polyrhythm_set_mute_trainer(sDriver->getEngineHandle(), &cfg);
}

JNIEXPORT jboolean JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativePopBeatEvent(
    JNIEnv* env, jobject /*thiz*/, jlongArray outArray) {
    if (!sDriver) return JNI_FALSE;

    CBeatEvent evt{};
    if (polyrhythm_pop_beat_event(sDriver->getEngineHandle(), &evt)) {
        jlong buf[7];
        buf[0] = static_cast<jlong>(evt.sample_timestamp);
        buf[1] = static_cast<jlong>(evt.layer_index);
        buf[2] = static_cast<jlong>(evt.step_index);
        buf[3] = static_cast<jlong>(evt.total_steps);
        buf[4] = static_cast<jlong>(evt.accent);
        buf[5] = static_cast<jlong>(evt.current_phase * 1000.0f); // Promille phase
        buf[6] = static_cast<jlong>(evt.bpm);
        env->SetLongArrayRegion(outArray, 0, 7, buf);
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

JNIEXPORT jfloat JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeGetLayerPhase(
    JNIEnv* /*env*/, jobject /*thiz*/, jint layerIdx) {
    if (sDriver) {
        return polyrhythm_get_layer_phase(sDriver->getEngineHandle(), static_cast<size_t>(layerIdx));
    }
    return 0.0f;
}

JNIEXPORT jfloat JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeGetBarPhase(
    JNIEnv* /*env*/, jobject /*thiz*/) {
    if (sDriver) {
        return polyrhythm_get_bar_phase(sDriver->getEngineHandle());
    }
    return 0.0f;
}

JNIEXPORT void JNICALL
Java_com_polyrhythm_metronome_audio_MetronomeNativeBridge_nativeRenderDirect(
    JNIEnv* env, jobject /*thiz*/, jfloatArray buffer, jint numFrames) {
    if (sDriver && buffer != nullptr) {
        jfloat* rawBuf = env->GetFloatArrayElements(buffer, nullptr);
        sDriver->renderAudio(rawBuf, numFrames);
        env->ReleaseFloatArrayElements(buffer, rawBuf, 0);
    }
}

} // extern "C"
