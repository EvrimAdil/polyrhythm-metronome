#pragma once

#include <cstdint>
#include <algorithm>

namespace polyrhythm {

struct TempoTrainerConfig {
    bool enabled{false};
    double start_bpm{120.0};
    double target_bpm{160.0};
    double step_bpm{2.0};       // Her adımda artış/azalış miktarı
    uint32_t bars_interval{4};   // Kaç ölçüde bir tempo değişecek
    bool auto_reverse{true};    // Hedefe ulaşınca yönü tersine çevir
};

struct MuteTrainerConfig {
    bool enabled{false};
    uint32_t play_bars{4};       // Sesli çalınacak ölçü sayısı
    uint32_t mute_bars{2};       // Sessiz çalınacak ölçü sayısı
};

class TrainerEngine {
public:
    TrainerEngine() = default;

    void set_tempo_trainer(const TempoTrainerConfig& config);
    void set_mute_trainer(const MuteTrainerConfig& config);

    [[nodiscard]] const TempoTrainerConfig& tempo_trainer() const noexcept { return tempo_cfg_; }
    [[nodiscard]] const MuteTrainerConfig& mute_trainer() const noexcept { return mute_cfg_; }

    /**
     * @brief Called at every measure (bar) boundary.
     * @param current_bpm Current BPM reference, modified if tempo trainer is active.
     * @return true if audible, false if currently in muted training mode.
     */
    bool on_bar_completed(double& current_bpm) noexcept;

    void reset() noexcept;

private:
    TempoTrainerConfig tempo_cfg_{};
    MuteTrainerConfig mute_cfg_{};

    uint32_t bar_counter_tempo_{0};
    bool tempo_increasing_{true};

    uint32_t bar_counter_mute_{0};
    bool is_currently_muted_{false};
};

} // namespace polyrhythm
