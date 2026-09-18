#pragma once

#include "audio_types.hpp"
#include "audio_scheduler.hpp"
#include "rhythm_layer.hpp"
#include "sound_generator.hpp"
#include "lockfree_ring_buffer.hpp"
#include <array>
#include <cstdint>
#include <atomic>

namespace polyrhythm {

class PolyrhythmEngine {
public:
    PolyrhythmEngine();
    ~PolyrhythmEngine() = default;

    void init(uint32_t sample_rate);
    void start() noexcept;
    void stop() noexcept;
    void reset() noexcept;
    [[nodiscard]] bool is_playing() const noexcept;

    void set_bpm(double bpm) noexcept;
    [[nodiscard]] double bpm() const noexcept;

    void set_time_signature(uint16_t numerator, uint16_t denominator) noexcept;

    // Katman Yönetimi
    size_t add_layer(const LayerConfig& config);
    bool set_layer_config(size_t layer_idx, const LayerConfig& config);
    bool get_layer_config(size_t layer_idx, LayerConfig& out_config) const;
    void remove_layer(size_t layer_idx);
    void clear_layers();
    [[nodiscard]] size_t layer_count() const noexcept;

    // Pratik Modları
    void set_tempo_trainer(const TempoTrainerConfig& config);
    void set_mute_trainer(const MuteTrainerConfig& config);

    // WAV Ses Yükleme
    bool load_sample(size_t slot, const float* data, size_t frame_count, uint16_t channels, uint32_t sample_rate);

    // UI Olay İletişimi (Lock-free Ring Buffer)
    bool pop_beat_event(BeatEvent& event) noexcept;

    // Gerçek Zamanlı Ses Render Döngüsü (Audio Callback - REAL TIME SAFE)
    void render(float* stereo_output, size_t num_frames) noexcept;

    // UI görselleştirme için anlık faz bilgisi
    [[nodiscard]] float get_layer_phase(size_t layer_idx) const noexcept;
    [[nodiscard]] float get_bar_phase() const noexcept;

    void set_master_volume(float volume) noexcept;
    [[nodiscard]] float master_volume() const noexcept;

private:
    AudioScheduler scheduler_{};
    SoundGenerator sound_gen_{};

    std::array<RhythmLayer, MAX_LAYERS> layers_{};
    std::atomic<size_t> active_layer_count_{0};

    LockFreeRingBuffer<BeatEvent, 1024> ui_event_queue_{};
    std::atomic<float> master_volume_{1.0f};
};

} // namespace polyrhythm
