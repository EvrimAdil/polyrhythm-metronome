#include "c_bridge.h"
#include "polyrhythm_engine.hpp"
#include "bjorklund.hpp"
#include <memory>
#include <cstring>

using namespace polyrhythm;

static LayerConfig to_cpp_layer_config(const CLayerConfig& c) {
    LayerConfig cpp{};
    cpp.layer_id = c.layer_id;
    cpp.mode = static_cast<RhythmMode>(c.mode);
    cpp.ratio_pulses = c.ratio_pulses;
    cpp.total_steps = c.total_steps;
    cpp.subdivision = c.subdivision;
    cpp.accent_count = std::min(c.accent_count, static_cast<uint16_t>(MAX_STEPS_PER_LAYER));
    for (size_t i = 0; i < cpp.accent_count; ++i) {
        cpp.accents[i] = static_cast<BeatAccent>(c.accents[i]);
    }
    cpp.volume = c.volume;
    cpp.pan = c.pan;
    cpp.pitch_shift = c.pitch_shift;
    cpp.synth_frequency = c.synth_frequency;
    cpp.sound_type = static_cast<SoundType>(c.sound_type);
    cpp.sample_id = c.sample_id;
    cpp.is_muted = c.is_muted;
    cpp.is_solo = c.is_solo;
    return cpp;
}

static CLayerConfig to_c_layer_config(const LayerConfig& cpp) {
    CLayerConfig c{};
    c.layer_id = cpp.layer_id;
    c.mode = static_cast<uint8_t>(cpp.mode);
    c.ratio_pulses = cpp.ratio_pulses;
    c.total_steps = cpp.total_steps;
    c.subdivision = cpp.subdivision;
    c.accent_count = cpp.accent_count;
    for (size_t i = 0; i < MAX_STEPS_PER_LAYER; ++i) {
        c.accents[i] = static_cast<uint8_t>(cpp.accents[i]);
    }
    c.volume = cpp.volume;
    c.pan = cpp.pan;
    c.pitch_shift = cpp.pitch_shift;
    c.synth_frequency = cpp.synth_frequency;
    c.sound_type = static_cast<uint8_t>(cpp.sound_type);
    c.sample_id = cpp.sample_id;
    c.is_muted = cpp.is_muted;
    c.is_solo = cpp.is_solo;
    return c;
}

PolyrhythmEngineHandle polyrhythm_create(uint32_t sample_rate) {
    auto engine = std::make_unique<PolyrhythmEngine>();
    engine->init(sample_rate);
    return reinterpret_cast<PolyrhythmEngineHandle>(engine.release());
}

void polyrhythm_destroy(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        delete reinterpret_cast<PolyrhythmEngine*>(handle);
    }
}

void polyrhythm_start(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->start();
    }
}

void polyrhythm_stop(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->stop();
    }
}

void polyrhythm_reset(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->reset();
    }
}

bool polyrhythm_is_playing(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->is_playing();
    }
    return false;
}

void polyrhythm_set_bpm(PolyrhythmEngineHandle handle, double bpm) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->set_bpm(bpm);
    }
}

double polyrhythm_get_bpm(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->bpm();
    }
    return 120.0;
}

void polyrhythm_set_time_signature(PolyrhythmEngineHandle handle, uint16_t numerator, uint16_t denominator) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->set_time_signature(numerator, denominator);
    }
}

size_t polyrhythm_add_layer(PolyrhythmEngineHandle handle, const CLayerConfig* config) {
    if (handle != nullptr && config != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->add_layer(to_cpp_layer_config(*config));
    }
    return MAX_LAYERS;
}

bool polyrhythm_set_layer_config(PolyrhythmEngineHandle handle, size_t layer_idx, const CLayerConfig* config) {
    if (handle != nullptr && config != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->set_layer_config(layer_idx, to_cpp_layer_config(*config));
    }
    return false;
}

bool polyrhythm_get_layer_config(PolyrhythmEngineHandle handle, size_t layer_idx, CLayerConfig* out_config) {
    if (handle != nullptr && out_config != nullptr) {
        LayerConfig cpp_cfg{};
        if (reinterpret_cast<PolyrhythmEngine*>(handle)->get_layer_config(layer_idx, cpp_cfg)) {
            *out_config = to_c_layer_config(cpp_cfg);
            return true;
        }
    }
    return false;
}

void polyrhythm_remove_layer(PolyrhythmEngineHandle handle, size_t layer_idx) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->remove_layer(layer_idx);
    }
}

void polyrhythm_clear_layers(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->clear_layers();
    }
}

size_t polyrhythm_get_layer_count(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->layer_count();
    }
    return 0;
}

void polyrhythm_set_tempo_trainer(PolyrhythmEngineHandle handle, const CTempoTrainerConfig* config) {
    if (handle != nullptr && config != nullptr) {
        TempoTrainerConfig cpp_cfg{};
        cpp_cfg.enabled = config->enabled;
        cpp_cfg.start_bpm = config->start_bpm;
        cpp_cfg.target_bpm = config->target_bpm;
        cpp_cfg.step_bpm = config->step_bpm;
        cpp_cfg.bars_interval = config->bars_interval;
        cpp_cfg.auto_reverse = config->auto_reverse;
        reinterpret_cast<PolyrhythmEngine*>(handle)->set_tempo_trainer(cpp_cfg);
    }
}

void polyrhythm_set_mute_trainer(PolyrhythmEngineHandle handle, const CMuteTrainerConfig* config) {
    if (handle != nullptr && config != nullptr) {
        MuteTrainerConfig cpp_cfg{};
        cpp_cfg.enabled = config->enabled;
        cpp_cfg.play_bars = config->play_bars;
        cpp_cfg.mute_bars = config->mute_bars;
        reinterpret_cast<PolyrhythmEngine*>(handle)->set_mute_trainer(cpp_cfg);
    }
}

bool polyrhythm_load_sample(PolyrhythmEngineHandle handle, size_t slot, const float* data, size_t frame_count, uint16_t channels, uint32_t sample_rate) {
    if (handle != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->load_sample(slot, data, frame_count, channels, sample_rate);
    }
    return false;
}

bool polyrhythm_pop_beat_event(PolyrhythmEngineHandle handle, CBeatEvent* out_event) {
    if (handle != nullptr && out_event != nullptr) {
        BeatEvent cpp_event{};
        if (reinterpret_cast<PolyrhythmEngine*>(handle)->pop_beat_event(cpp_event)) {
            out_event->sample_timestamp = cpp_event.sample_timestamp;
            out_event->layer_index = cpp_event.layer_index;
            out_event->step_index = cpp_event.step_index;
            out_event->total_steps = cpp_event.total_steps;
            out_event->accent = static_cast<uint8_t>(cpp_event.accent);
            out_event->current_phase = cpp_event.current_phase;
            out_event->bpm = cpp_event.bpm;
            return true;
        }
    }
    return false;
}

void polyrhythm_render(PolyrhythmEngineHandle handle, float* stereo_output, size_t num_frames) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->render(stereo_output, num_frames);
    }
}

float polyrhythm_get_layer_phase(PolyrhythmEngineHandle handle, size_t layer_idx) {
    if (handle != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->get_layer_phase(layer_idx);
    }
    return 0.0f;
}

float polyrhythm_get_bar_phase(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->get_bar_phase();
    }
    return 0.0f;
}

void polyrhythm_set_master_volume(PolyrhythmEngineHandle handle, float volume) {
    if (handle != nullptr) {
        reinterpret_cast<PolyrhythmEngine*>(handle)->set_master_volume(volume);
    }
}

float polyrhythm_get_master_volume(PolyrhythmEngineHandle handle) {
    if (handle != nullptr) {
        return reinterpret_cast<PolyrhythmEngine*>(handle)->master_volume();
    }
    return 1.0f;
}

void polyrhythm_generate_euclidean(uint16_t pulses, uint16_t steps, uint8_t* out_accents, bool downbeat_first) {
    if (out_accents == nullptr || steps == 0) return;
    std::vector<BeatAccent> result(steps, BeatAccent::Mute);
    Bjorklund::generate(pulses, steps, result.data(), downbeat_first);
    for (size_t i = 0; i < steps; ++i) {
        out_accents[i] = static_cast<uint8_t>(result[i]);
    }
}
