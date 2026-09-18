#pragma once

#include "audio_types.hpp"
#include "trainers.hpp"
#include "lockfree_ring_buffer.hpp"
#include <cstdint>
#include <atomic>

namespace polyrhythm {

class AudioScheduler {
public:
    AudioScheduler();
    ~AudioScheduler() = default;

    void init(uint32_t sample_rate);
    void set_bpm(double bpm) noexcept;
    [[nodiscard]] double bpm() const noexcept { return bpm_.load(std::memory_order_relaxed); }

    void set_time_signature(uint16_t numerator, uint16_t denominator) noexcept;
    void get_time_signature(uint16_t& numerator, uint16_t& denominator) const noexcept;

    void start() noexcept;
    void stop() noexcept;
    void reset() noexcept;
    [[nodiscard]] bool is_playing() const noexcept { return is_playing_.load(std::memory_order_relaxed); }

    [[nodiscard]] uint64_t total_samples_rendered() const noexcept { return total_samples_rendered_; }
    [[nodiscard]] uint32_t sample_rate() const noexcept { return sample_rate_; }
    [[nodiscard]] double quarter_note_samples() const noexcept;
    [[nodiscard]] double master_bar_samples() const noexcept;

    TrainerEngine& trainer() noexcept { return trainer_; }
    [[nodiscard]] const TrainerEngine& trainer() const noexcept { return trainer_; }

    /**
     * @brief Advances scheduler by 1 sample.
     * @param bar_completed Outputs true if a master measure (bar) boundary was reached.
     * @param is_audible Outputs false if Mute Trainer currently silences the metronome.
     */
    void advance_sample(bool& bar_completed, bool& is_audible) noexcept;

    /**
     * @brief Gets continuous phase [0.0, 1.0] of current measure for smooth polygon rotation.
     */
    [[nodiscard]] float get_bar_phase() const noexcept;

private:
    void recalculate_timing() noexcept;

    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    std::atomic<double> bpm_{120.0};
    std::atomic<uint16_t> ts_numerator_{4};
    std::atomic<uint16_t> ts_denominator_{4};
    std::atomic<bool> is_playing_{false};

    uint64_t total_samples_rendered_{0};
    uint64_t next_bar_sample_{0};
    uint64_t current_bar_start_{0};
    uint64_t bars_elapsed_{0};

    double quarter_note_samples_{24000.0};
    double master_bar_samples_{96000.0};

    TrainerEngine trainer_{};
    bool is_currently_audible_{true};
};

} // namespace polyrhythm
