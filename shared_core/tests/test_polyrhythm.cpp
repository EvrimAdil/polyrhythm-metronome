#include "polyrhythm_engine.hpp"
#include "c_bridge.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>

int main() {
    std::cout << "[TEST] Running Polyrhythm Engine & C-Bridge Integration Test..." << std::endl;

    polyrhythm::PolyrhythmEngine engine;
    engine.init(48000);
    engine.set_bpm(120.0);
    engine.set_time_signature(4, 4);

    // Katman 1: 3 Vuruş (Polyrhythm 3)
    polyrhythm::LayerConfig l1{};
    l1.layer_id = 0;
    l1.mode = polyrhythm::RhythmMode::Polyrhythmic;
    l1.ratio_pulses = 3;
    l1.volume = 0.8f;
    l1.pan = -0.5f; // Sola yatık
    l1.synth_frequency = 800.0f;
    engine.add_layer(l1);

    // Katman 2: 4 Vuruş (Polyrhythm 4)
    polyrhythm::LayerConfig l2{};
    l2.layer_id = 1;
    l2.mode = polyrhythm::RhythmMode::Polyrhythmic;
    l2.ratio_pulses = 4;
    l2.volume = 0.8f;
    l2.pan = 0.5f;  // Sağa yatık
    l2.synth_frequency = 1200.0f;
    engine.add_layer(l2);

    assert(engine.layer_count() == 2);
    engine.start();

    // 1 Ölçü (96000 numune @ 120 BPM, 4/4) simüle et
    constexpr size_t BUFFER_SIZE = 512;
    constexpr size_t NUM_BUFFERS = 96000 / BUFFER_SIZE;
    std::vector<float> stereo_buffer(BUFFER_SIZE * 2, 0.0f);

    size_t layer0_events = 0;
    size_t layer1_events = 0;
    size_t downbeat_coincidences = 0;
    float max_peak = 0.0f;

    for (size_t b = 0; b < NUM_BUFFERS; ++b) {
        engine.render(stereo_buffer.data(), BUFFER_SIZE);

        for (float s : stereo_buffer) {
            float abs_s = std::abs(s);
            if (abs_s > max_peak) max_peak = abs_s;
            assert(!std::isnan(s));
            assert(!std::isinf(s));
        }

        polyrhythm::BeatEvent evt;
        while (engine.pop_beat_event(evt)) {
            if (evt.layer_index == 0) {
                layer0_events++;
            } else if (evt.layer_index == 1) {
                layer1_events++;
            }
            if (evt.step_index == 0) {
                downbeat_coincidences++;
            }
        }
    }

    std::cout << "  Layer 0 (3 pulses) Events: " << layer0_events << std::endl;
    std::cout << "  Layer 1 (4 pulses) Events: " << layer1_events << std::endl;
    std::cout << "  Downbeat (Beat 1) Matches: " << downbeat_coincidences << std::endl;
    std::cout << "  Max Peak Amplitude: " << max_peak << std::endl;

    assert(layer0_events == 3);
    assert(layer1_events == 4);
    assert(downbeat_coincidences >= 2); // Her iki katmanın ilk vuruşları
    assert(max_peak > 0.01f); // Ses üretimi gerçekleşti

    // C-Bridge Sanity Check
    PolyrhythmEngineHandle c_handle = polyrhythm_create(48000);
    assert(c_handle != nullptr);
    polyrhythm_set_bpm(c_handle, 140.0);
    assert(std::abs(polyrhythm_get_bpm(c_handle) - 140.0) < 0.01);
    polyrhythm_destroy(c_handle);

    std::cout << "[PASS] Polyrhythm Engine & C-Bridge tests passed successfully!" << std::endl;
    return 0;
}
