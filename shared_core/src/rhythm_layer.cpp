#include "rhythm_layer.hpp"
#include <cmath>
#include <algorithm>

namespace polyrhythm {

RhythmLayer::RhythmLayer() : RhythmLayer(0) {}

RhythmLayer::RhythmLayer(uint8_t layer_id) {
    config_.layer_id = layer_id;
    config_.mode = RhythmMode::Polyrhythmic;
    config_.ratio_pulses = 4;
    config_.total_steps = 4;
    config_.subdivision = 1;
    config_.volume = 0.8f;
    config_.pan = 0.0f;
    config_.pitch_shift = 1.0f;
    config_.synth_frequency = 1000.0f;
    config_.sound_type = SoundType::SyntheticSine;
    config_.accent_count = 4;
    config_.accents[0] = BeatAccent::Downbeat;
    for (size_t i = 1; i < MAX_STEPS_PER_LAYER; ++i) {
        config_.accents[i] = BeatAccent::Normal;
    }
    reset();
}

void RhythmLayer::configure(const LayerConfig& config) {
    config_ = config;
    if (config_.mode == RhythmMode::Euclidean) {
        update_euclidean();
    }
    reset();
}

void RhythmLayer::update_euclidean() {
    uint16_t pulses = std::min(config_.ratio_pulses, static_cast<uint16_t>(MAX_STEPS_PER_LAYER));
    uint16_t steps = std::clamp(config_.total_steps, static_cast<uint16_t>(1), static_cast<uint16_t>(MAX_STEPS_PER_LAYER));
    config_.accent_count = steps;
    Bjorklund::generate(pulses, steps, config_.accents, true);
}

void RhythmLayer::reset() noexcept {
    next_trigger_sample_ = 0;
    current_step_ = 0;
    last_bar_start_sample_ = 0;
}

bool RhythmLayer::check_and_trigger(
    double master_bar_samples,
    double quarter_note_samples,
    uint64_t current_sample,
    BeatEvent& out_event
) noexcept {
    if (master_bar_samples <= 0.0 || quarter_note_samples <= 0.0) return false;

    bool triggered = false;

    if (config_.mode == RhythmMode::Polyrhythmic) {
        uint16_t pulses = (config_.ratio_pulses > 0) ? config_.ratio_pulses : 1;
        
        // Ölçü başı güncellemesi
        if (current_sample >= last_bar_start_sample_ + static_cast<uint64_t>(std::round(master_bar_samples))) {
            last_bar_start_sample_ += static_cast<uint64_t>(std::round(master_bar_samples));
            current_step_ = 0;
            next_trigger_sample_ = last_bar_start_sample_;
        }

        if (current_sample >= next_trigger_sample_) {
            triggered = true;

            // Vurgu tespiti
            BeatAccent accent = BeatAccent::Normal;
            if (current_step_ < config_.accent_count) {
                accent = config_.accents[current_step_];
            } else {
                accent = (current_step_ == 0) ? BeatAccent::Downbeat : BeatAccent::Normal;
            }

            out_event.sample_timestamp = current_sample;
            out_event.layer_index = config_.layer_id;
            out_event.step_index = current_step_;
            out_event.total_steps = pulses;
            out_event.accent = accent;
            out_event.current_phase = static_cast<float>(current_step_) / static_cast<float>(pulses);

            // Sonraki vuruşun rasyonel sıfır-kaymalı (zero-drift) mutlak numunesi
            current_step_++;
            if (current_step_ < pulses) {
                double frac_offset = (static_cast<double>(current_step_) * master_bar_samples) / static_cast<double>(pulses);
                next_trigger_sample_ = last_bar_start_sample_ + static_cast<uint64_t>(std::round(frac_offset));
            } else {
                next_trigger_sample_ = last_bar_start_sample_ + static_cast<uint64_t>(std::round(master_bar_samples));
            }
        }
    } 
    else if (config_.mode == RhythmMode::Polymetric) {
        uint16_t total_steps = (config_.total_steps > 0) ? config_.total_steps : 4;
        uint16_t subdiv = (config_.subdivision > 0) ? config_.subdivision : 1;
        double step_duration = quarter_note_samples / static_cast<double>(subdiv);

        if (current_sample >= next_trigger_sample_) {
            triggered = true;

            BeatAccent accent = BeatAccent::Normal;
            if (current_step_ < config_.accent_count) {
                accent = config_.accents[current_step_];
            } else {
                accent = (current_step_ == 0) ? BeatAccent::Downbeat : BeatAccent::Normal;
            }

            out_event.sample_timestamp = current_sample;
            out_event.layer_index = config_.layer_id;
            out_event.step_index = current_step_;
            out_event.total_steps = total_steps;
            out_event.accent = accent;
            out_event.current_phase = static_cast<float>(current_step_) / static_cast<float>(total_steps);

            current_step_++;
            if (current_step_ >= total_steps) {
                current_step_ = 0;
            }

            // Zero-drift step adımı
            next_trigger_sample_ = current_sample + static_cast<uint64_t>(std::round(step_duration));
        }
    }
    else if (config_.mode == RhythmMode::Euclidean) {
        uint16_t steps = (config_.total_steps > 0) ? config_.total_steps : 8;
        
        if (current_sample >= last_bar_start_sample_ + static_cast<uint64_t>(std::round(master_bar_samples))) {
            last_bar_start_sample_ += static_cast<uint64_t>(std::round(master_bar_samples));
            current_step_ = 0;
            next_trigger_sample_ = last_bar_start_sample_;
        }

        if (current_sample >= next_trigger_sample_) {
            triggered = true;

            BeatAccent accent = (current_step_ < config_.accent_count) ? config_.accents[current_step_] : BeatAccent::Mute;

            out_event.sample_timestamp = current_sample;
            out_event.layer_index = config_.layer_id;
            out_event.step_index = current_step_;
            out_event.total_steps = steps;
            out_event.accent = accent;
            out_event.current_phase = static_cast<float>(current_step_) / static_cast<float>(steps);

            current_step_++;
            if (current_step_ < steps) {
                double frac_offset = (static_cast<double>(current_step_) * master_bar_samples) / static_cast<double>(steps);
                next_trigger_sample_ = last_bar_start_sample_ + static_cast<uint64_t>(std::round(frac_offset));
            } else {
                next_trigger_sample_ = last_bar_start_sample_ + static_cast<uint64_t>(std::round(master_bar_samples));
            }
        }
    }

    return triggered;
}

float RhythmLayer::get_current_phase(double master_bar_samples, uint64_t current_sample) const noexcept {
    if (master_bar_samples <= 0.0) return 0.0f;
    uint64_t elapsed = (current_sample >= last_bar_start_sample_) ? (current_sample - last_bar_start_sample_) : 0;
    double phase = static_cast<double>(elapsed) / master_bar_samples;
    return static_cast<float>(phase - std::floor(phase));
}

} // namespace polyrhythm
