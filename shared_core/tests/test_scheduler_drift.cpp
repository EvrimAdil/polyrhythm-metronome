#include "audio_scheduler.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
    std::cout << "[TEST] Running Zero-Drift AudioScheduler Test..." << std::endl;

    polyrhythm::AudioScheduler scheduler;
    scheduler.init(48000);
    scheduler.set_bpm(133.333333); // İrrasyonel/periyodik BPM (drift stres testi)
    scheduler.set_time_signature(4, 4);
    scheduler.start();

    const double expected_bar_samples = (60.0 / 133.333333) * 48000.0 * 4.0;
    std::cout << "  Sample Rate: 48000 Hz, BPM: 133.333333" << std::endl;
    std::cout << "  Expected Bar Samples: " << expected_bar_samples << std::endl;

    constexpr uint64_t TOTAL_TEST_SAMPLES = 10'000'000; // 10 Milyon numune
    uint64_t completed_bars = 0;
    uint64_t last_bar_sample = 0;
    double max_drift = 0.0;

    for (uint64_t s = 0; s < TOTAL_TEST_SAMPLES; ++s) {
        bool bar_completed = false;
        bool is_audible = true;
        scheduler.advance_sample(bar_completed, is_audible);

        if (bar_completed) {
            completed_bars++;
            uint64_t current_sample = scheduler.total_samples_rendered();
            double theoretical_sample = static_cast<double>(completed_bars) * expected_bar_samples;
            double drift = std::abs(static_cast<double>(current_sample) - theoretical_sample);
            if (drift > max_drift) {
                max_drift = drift;
            }
            last_bar_sample = current_sample;
        }
    }

    std::cout << "  Completed Bars in 10M samples: " << completed_bars << std::endl;
    std::cout << "  Last Bar Sample: " << last_bar_sample << std::endl;
    std::cout << "  Maximum Drift (samples): " << max_drift << std::endl;

    // Numune hassasiyetinde maksimum hata 1 numuneden (yuvarlama sınırı) küçük olmalıdır
    assert(max_drift < 1.0);
    assert(completed_bars > 0);

    std::cout << "[PASS] Zero-Drift Scheduler verification passed! Absolute drift < 1.0 sample over 10M samples." << std::endl;
    return 0;
}
