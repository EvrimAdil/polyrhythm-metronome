#include "polyrhythm_engine.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace polyrhythm {

PolyrhythmEngine::PolyrhythmEngine() {
    init(DEFAULT_SAMPLE_RATE);
}

void PolyrhythmEngine::init(uint32_t sample_rate) {
    scheduler_.init(sample_rate);
    sound_gen_.init(sample_rate);
    reset();
}

void PolyrhythmEngine::start() noexcept {
    scheduler_.start();
}

void PolyrhythmEngine::stop() noexcept {
    scheduler_.stop();
    sound_gen_.reset();
}

void PolyrhythmEngine::reset() noexcept {
    scheduler_.reset();
    sound_gen_.reset();
    ui_event_queue_.clear();
    for (size_t i = 0; i < active_layer_count_.load(std::memory_order_relaxed); ++i) {
        layers_[i].reset();
    }
}

bool PolyrhythmEngine::is_playing() const noexcept {
    return scheduler_.is_playing();
}

void PolyrhythmEngine::set_bpm(double bpm) noexcept {
    scheduler_.set_bpm(bpm);
}

double PolyrhythmEngine::bpm() const noexcept {
    return scheduler_.bpm();
}

void PolyrhythmEngine::set_time_signature(uint16_t numerator, uint16_t denominator) noexcept {
    scheduler_.set_time_signature(numerator, denominator);
}

size_t PolyrhythmEngine::add_layer(const LayerConfig& config) {
    size_t current_count = active_layer_count_.load(std::memory_order_relaxed);
    if (current_count >= MAX_LAYERS) return MAX_LAYERS;

    LayerConfig cfg = config;
    cfg.layer_id = static_cast<uint8_t>(current_count);
    layers_[current_count].configure(cfg);
    active_layer_count_.store(current_count + 1, std::memory_order_release);
    return current_count;
}

bool PolyrhythmEngine::set_layer_config(size_t layer_idx, const LayerConfig& config) {
    if (layer_idx >= active_layer_count_.load(std::memory_order_relaxed)) {
        return false;
    }
    LayerConfig cfg = config;
    cfg.layer_id = static_cast<uint8_t>(layer_idx);
    layers_[layer_idx].configure(cfg);
    return true;
}

bool PolyrhythmEngine::get_layer_config(size_t layer_idx, LayerConfig& out_config) const {
    if (layer_idx >= active_layer_count_.load(std::memory_order_relaxed)) {
        return false;
    }
    out_config = layers_[layer_idx].config();
    return true;
}

void PolyrhythmEngine::remove_layer(size_t layer_idx) {
    size_t count = active_layer_count_.load(std::memory_order_relaxed);
    if (layer_idx >= count) return;

    for (size_t i = layer_idx; i < count - 1; ++i) {
        LayerConfig next_cfg = layers_[i + 1].config();
        next_cfg.layer_id = static_cast<uint8_t>(i);
        layers_[i].configure(next_cfg);
    }
    active_layer_count_.store(count - 1, std::memory_order_release);
}

void PolyrhythmEngine::clear_layers() {
    active_layer_count_.store(0, std::memory_order_release);
}

size_t PolyrhythmEngine::layer_count() const noexcept {
    return active_layer_count_.load(std::memory_order_relaxed);
}

void PolyrhythmEngine::set_tempo_trainer(const TempoTrainerConfig& config) {
    scheduler_.trainer().set_tempo_trainer(config);
}

void PolyrhythmEngine::set_mute_trainer(const MuteTrainerConfig& config) {
    scheduler_.trainer().set_mute_trainer(config);
}

bool PolyrhythmEngine::load_sample(size_t slot, const float* data, size_t frame_count, uint16_t channels, uint32_t sample_rate) {
    return sound_gen_.load_sample(slot, data, frame_count, channels, sample_rate);
}

bool PolyrhythmEngine::pop_beat_event(BeatEvent& event) noexcept {
    return ui_event_queue_.pop(event);
}

void PolyrhythmEngine::render(float* stereo_output, size_t num_frames) noexcept {
    if (stereo_output == nullptr || num_frames == 0) return;

    // Buffer'ı temizle (Sıfırla)
    std::fill_n(stereo_output, num_frames * 2, 0.0f);

    if (!scheduler_.is_playing()) {
        return;
    }

    const size_t num_layers = active_layer_count_.load(std::memory_order_relaxed);
    const double master_bar = scheduler_.master_bar_samples();
    const double quarter_note = scheduler_.quarter_note_samples();

    // Numune bazlı zamanlama ve vuruş tetikleme
    for (size_t f = 0; f < num_frames; ++f) {
        bool bar_completed = false;
        bool is_audible = true;

        scheduler_.advance_sample(bar_completed, is_audible);
        uint64_t current_sample = scheduler_.total_samples_rendered();

        for (size_t l = 0; l < num_layers; ++l) {
            BeatEvent event{};
            bool triggered = layers_[l].check_and_trigger(master_bar, quarter_note, current_sample, event);

            if (triggered) {
                event.bpm = static_cast<uint32_t>(scheduler_.bpm());
                ui_event_queue_.push(event);

                if (is_audible) {
                    sound_gen_.trigger(layers_[l].config(), event.accent);
                }
            }
        }
    }

    // Hibrit ses sentezi ve sample çalma
    sound_gen_.render(stereo_output, num_frames);

    // Master Volume & Soft-clipping Limiter (Tanh saturasyon)
    float master_gain = master_volume_.load(std::memory_order_relaxed);
    for (size_t i = 0; i < num_frames * 2; ++i) {
        float sample = stereo_output[i] * master_gain;
        // Yumuşak limiter: Aşırı genlikte sert distorsiyonu önler
        stereo_output[i] = std::clamp(std::tanh(sample), -1.0f, 1.0f);
    }
}

float PolyrhythmEngine::get_layer_phase(size_t layer_idx) const noexcept {
    if (layer_idx >= active_layer_count_.load(std::memory_order_relaxed)) return 0.0f;
    return layers_[layer_idx].get_current_phase(scheduler_.master_bar_samples(), scheduler_.total_samples_rendered());
}

float PolyrhythmEngine::get_bar_phase() const noexcept {
    return scheduler_.get_bar_phase();
}

void PolyrhythmEngine::set_master_volume(float volume) noexcept {
    master_volume_.store(std::clamp(volume, 0.0f, 2.0f), std::memory_order_relaxed);
}

float PolyrhythmEngine::master_volume() const noexcept {
    return master_volume_.load(std::memory_order_relaxed);
}

} // namespace polyrhythm
