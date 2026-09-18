#include "trainers.hpp"
#include <cmath>

namespace polyrhythm {

void TrainerEngine::set_tempo_trainer(const TempoTrainerConfig& config) {
    tempo_cfg_ = config;
    bar_counter_tempo_ = 0;
    tempo_increasing_ = (config.target_bpm >= config.start_bpm);
}

void TrainerEngine::set_mute_trainer(const MuteTrainerConfig& config) {
    mute_cfg_ = config;
    bar_counter_mute_ = 0;
    is_currently_muted_ = false;
}

void TrainerEngine::reset() noexcept {
    bar_counter_tempo_ = 0;
    bar_counter_mute_ = 0;
    is_currently_muted_ = false;
}

bool TrainerEngine::on_bar_completed(double& current_bpm) noexcept {
    // 1. Tempo Trainer Mantığı
    if (tempo_cfg_.enabled && tempo_cfg_.bars_interval > 0) {
        bar_counter_tempo_++;
        if (bar_counter_tempo_ >= tempo_cfg_.bars_interval) {
            bar_counter_tempo_ = 0;

            if (tempo_increasing_) {
                current_bpm += tempo_cfg_.step_bpm;
                if (current_bpm >= tempo_cfg_.target_bpm) {
                    current_bpm = tempo_cfg_.target_bpm;
                    if (tempo_cfg_.auto_reverse) {
                        tempo_increasing_ = false;
                    }
                }
            } else {
                current_bpm -= tempo_cfg_.step_bpm;
                if (current_bpm <= tempo_cfg_.start_bpm) {
                    current_bpm = tempo_cfg_.start_bpm;
                    if (tempo_cfg_.auto_reverse) {
                        tempo_increasing_ = true;
                    }
                }
            }
        }
    }

    // 2. Mute Trainer Mantığı
    if (mute_cfg_.enabled) {
        bar_counter_mute_++;
        if (!is_currently_muted_) {
            if (bar_counter_mute_ >= mute_cfg_.play_bars) {
                is_currently_muted_ = true;
                bar_counter_mute_ = 0;
            }
        } else {
            if (bar_counter_mute_ >= mute_cfg_.mute_bars) {
                is_currently_muted_ = false;
                bar_counter_mute_ = 0;
            }
        }
    } else {
        is_currently_muted_ = false;
    }

    return !is_currently_muted_;
}

} // namespace polyrhythm
