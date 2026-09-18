#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* PolyrhythmEngineHandle;

typedef struct {
    uint64_t sample_timestamp;
    uint8_t layer_index;
    uint16_t step_index;
    uint16_t total_steps;
    uint8_t accent; // 0: Mute, 1: Normal, 2: Downbeat
    float current_phase;
    uint32_t bpm;
} CBeatEvent;

typedef struct {
    uint8_t layer_id;
    uint8_t mode; // 0: Polyrhythmic, 1: Polymetric, 2: Euclidean
    uint16_t ratio_pulses;
    uint16_t total_steps;
    uint16_t subdivision;
    uint8_t accents[64];
    uint16_t accent_count;
    float volume;
    float pan;
    float pitch_shift;
    float synth_frequency;
    uint8_t sound_type; // 0: SynthSine, 1: SynthWood, 2: PCMSample
    int32_t sample_id;
    bool is_muted;
    bool is_solo;
} CLayerConfig;

typedef struct {
    bool enabled;
    double start_bpm;
    double target_bpm;
    double step_bpm;
    uint32_t bars_interval;
    bool auto_reverse;
} CTempoTrainerConfig;

typedef struct {
    bool enabled;
    uint32_t play_bars;
    uint32_t mute_bars;
} CMuteTrainerConfig;

// Yaşam Döngüsü
PolyrhythmEngineHandle polyrhythm_create(uint32_t sample_rate);
void polyrhythm_destroy(PolyrhythmEngineHandle handle);
void polyrhythm_start(PolyrhythmEngineHandle handle);
void polyrhythm_stop(PolyrhythmEngineHandle handle);
void polyrhythm_reset(PolyrhythmEngineHandle handle);
bool polyrhythm_is_playing(PolyrhythmEngineHandle handle);

// Tempo ve Zamanlama
void polyrhythm_set_bpm(PolyrhythmEngineHandle handle, double bpm);
double polyrhythm_get_bpm(PolyrhythmEngineHandle handle);
void polyrhythm_set_time_signature(PolyrhythmEngineHandle handle, uint16_t numerator, uint16_t denominator);

// Katman Yönetimi
size_t polyrhythm_add_layer(PolyrhythmEngineHandle handle, const CLayerConfig* config);
bool polyrhythm_set_layer_config(PolyrhythmEngineHandle handle, size_t layer_idx, const CLayerConfig* config);
bool polyrhythm_get_layer_config(PolyrhythmEngineHandle handle, size_t layer_idx, CLayerConfig* out_config);
void polyrhythm_remove_layer(PolyrhythmEngineHandle handle, size_t layer_idx);
void polyrhythm_clear_layers(PolyrhythmEngineHandle handle);
size_t polyrhythm_get_layer_count(PolyrhythmEngineHandle handle);

// Pratik Modları
void polyrhythm_set_tempo_trainer(PolyrhythmEngineHandle handle, const CTempoTrainerConfig* config);
void polyrhythm_set_mute_trainer(PolyrhythmEngineHandle handle, const CMuteTrainerConfig* config);

// WAV Sample Yükleme
bool polyrhythm_load_sample(PolyrhythmEngineHandle handle, size_t slot, const float* data, size_t frame_count, uint16_t channels, uint32_t sample_rate);

// UI Event Oku (Lock-Free)
bool polyrhythm_pop_beat_event(PolyrhythmEngineHandle handle, CBeatEvent* out_event);

// Gerçek Zamanlı Ses Render Çağrısı (RT Safe)
void polyrhythm_render(PolyrhythmEngineHandle handle, float* stereo_output, size_t num_frames);

// Geometrik Görselleştirme Faz Bilgisi
float polyrhythm_get_layer_phase(PolyrhythmEngineHandle handle, size_t layer_idx);
float polyrhythm_get_bar_phase(PolyrhythmEngineHandle handle);

// Mikser Master
void polyrhythm_set_master_volume(PolyrhythmEngineHandle handle, float volume);
float polyrhythm_get_master_volume(PolyrhythmEngineHandle handle);

// Euclidean Yardımcı Fonksiyonu
void polyrhythm_generate_euclidean(uint16_t pulses, uint16_t steps, uint8_t* out_accents, bool downbeat_first);

#ifdef __cplusplus
}
#endif
