#pragma once

#include "audio_types.hpp"
#include "bjorklund.hpp"
#include <cstdint>
#include <vector>

namespace polyrhythm {

class RhythmLayer {
public:
    RhythmLayer();
    explicit RhythmLayer(uint8_t layer_id);
    ~RhythmLayer() = default;

    void configure(const LayerConfig& config);
    [[nodiscard]] const LayerConfig& config() const noexcept { return config_; }
    LayerConfig& config_mut() noexcept { return config_; }

    void reset() noexcept;

    /**
     * @brief Recalculates Euclidean accents if in Euclidean mode.
     */
    void update_euclidean();

    /**
     * @brief Advances phase and checks if a beat is triggered in this frame offset.
     * @param master_bar_samples Total samples in one master measure (for Polyrhythmic)
     * @param quarter_note_samples Samples per quarter note at current BPM
     * @param current_sample Current global sample counter
     * @param out_event Output event if beat is triggered
     * @return true if a beat triggered at this exact sample
     */
    bool check_and_trigger(
        double master_bar_samples,
        double quarter_note_samples,
        uint64_t current_sample,
        BeatEvent& out_event
    ) noexcept;

    /**
     * @brief Gets current continuous phase [0.0f, 1.0f] for polygon visualization.
     */
    [[nodiscard]] float get_current_phase(double master_bar_samples, uint64_t current_sample) const noexcept;

    void set_muted(bool muted) noexcept { config_.is_muted = muted; }
    void set_volume(float vol) noexcept { config_.volume = vol; }
    void set_pan(float pan) noexcept { config_.pan = pan; }
    void set_pitch_shift(float pitch) noexcept { config_.pitch_shift = pitch; }

private:
    LayerConfig config_{};
    
    // Sample-accurate timing durumları
    uint64_t next_trigger_sample_{0};
    uint16_t current_step_{0};
    uint64_t last_bar_start_sample_{0};
};

} // namespace polyrhythm
