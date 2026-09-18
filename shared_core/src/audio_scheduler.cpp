#include "audio_scheduler.hpp"
#include <cmath>
#include <algorithm>

namespace polyrhythm {

AudioScheduler::AudioScheduler() {
    init(DEFAULT_SAMPLE_RATE);
}

void AudioScheduler::init(uint32_t sample_rate) {
    sample_rate_ = (sample_rate > 0) ? sample_rate : DEFAULT_SAMPLE_RATE;
    recalculate_timing();
    reset();
}

void AudioScheduler::set_bpm(double bpm) noexcept {
    double clamped = std::clamp(bpm, 20.0, 400.0);
    bpm_.store(clamped, std::memory_order_relaxed);
    recalculate_timing();
    if (total_samples_rendered_ == 0) {
        next_bar_sample_ = static_cast<uint64_t>(std::round(master_bar_samples_));
    }
}

void AudioScheduler::set_time_signature(uint16_t numerator, uint16_t denominator) noexcept {
    ts_numerator_.store(std::max(static_cast<uint16_t>(1), numerator), std::memory_order_relaxed);
    ts_denominator_.store((denominator > 0) ? denominator : 4, std::memory_order_relaxed);
    recalculate_timing();
    if (total_samples_rendered_ == 0) {
        next_bar_sample_ = static_cast<uint64_t>(std::round(master_bar_samples_));
    }
}

void AudioScheduler::get_time_signature(uint16_t& numerator, uint16_t& denominator) const noexcept {
    numerator = ts_numerator_.load(std::memory_order_relaxed);
    denominator = ts_denominator_.load(std::memory_order_relaxed);
}

void AudioScheduler::recalculate_timing() noexcept {
    double current_bpm = bpm_.load(std::memory_order_relaxed);
    uint16_t num = ts_numerator_.load(std::memory_order_relaxed);
    uint16_t den = ts_denominator_.load(std::memory_order_relaxed);

    quarter_note_samples_ = (60.0 / current_bpm) * static_cast<double>(sample_rate_);
    master_bar_samples_ = quarter_note_samples_ * (4.0 / static_cast<double>(den)) * static_cast<double>(num);
}

void AudioScheduler::start() noexcept {
    is_playing_.store(true, std::memory_order_release);
}

void AudioScheduler::stop() noexcept {
    is_playing_.store(false, std::memory_order_release);
}

void AudioScheduler::reset() noexcept {
    total_samples_rendered_ = 0;
    current_bar_start_ = 0;
    bars_elapsed_ = 0;
    recalculate_timing();
    next_bar_sample_ = static_cast<uint64_t>(std::round(master_bar_samples_));
    trainer_.reset();
    is_currently_audible_ = true;
}

double AudioScheduler::quarter_note_samples() const noexcept {
    return quarter_note_samples_;
}

double AudioScheduler::master_bar_samples() const noexcept {
    return master_bar_samples_;
}

void AudioScheduler::advance_sample(bool& bar_completed, bool& is_audible) noexcept {
    bar_completed = false;
    is_audible = is_currently_audible_;

    if (!is_playing_.load(std::memory_order_relaxed)) {
        return;
    }

    total_samples_rendered_++;

    if (total_samples_rendered_ >= next_bar_sample_) {
        bar_completed = true;
        bars_elapsed_++;
        current_bar_start_ = next_bar_sample_;

        double current_b = bpm_.load(std::memory_order_relaxed);
        double old_b = current_b;
        is_currently_audible_ = trainer_.on_bar_completed(current_b);
        is_audible = is_currently_audible_;

        if (std::abs(current_b - old_b) > 0.001) {
            bpm_.store(current_b, std::memory_order_relaxed);
            recalculate_timing();
        }

        next_bar_sample_ = static_cast<uint64_t>(std::round(static_cast<double>(bars_elapsed_ + 1) * master_bar_samples_));
    }
}

float AudioScheduler::get_bar_phase() const noexcept {
    if (master_bar_samples_ <= 0.0) return 0.0f;
    uint64_t elapsed = (total_samples_rendered_ >= current_bar_start_) ? (total_samples_rendered_ - current_bar_start_) : 0;
    double phase = static_cast<double>(elapsed) / master_bar_samples_;
    return static_cast<float>(phase - std::floor(phase));
}

} // namespace polyrhythm
