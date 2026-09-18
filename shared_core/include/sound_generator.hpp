#pragma once

#include "audio_types.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>

namespace polyrhythm {

struct PCMSampleData {
    std::vector<float> samples; // Stereo interleaved float
    uint32_t sample_rate{48000};
    uint16_t channels{2};
    size_t frame_count{0};
};

struct ActiveVoice {
    bool is_active{false};
    SoundType type{SoundType::SyntheticSine};
    BeatAccent accent{BeatAccent::Normal};
    
    // Sentez parametreleri
    float phase{0.0f};
    float phase_inc{0.0f};
    float base_freq{1000.0f};
    float current_freq{1000.0f};
    float decay_rate{0.999f};
    float amplitude{0.8f};
    
    // Sample oynatma parametreleri
    const PCMSampleData* pcm_data{nullptr};
    double pcm_playback_index{0.0};
    double pcm_pitch_ratio{1.0};
    
    // Mikser parametreleri (Constant Power Panning)
    float gain_left{0.707f};
    float gain_right{0.707f};
};

class SoundGenerator {
public:
    static constexpr size_t MAX_VOICES = 32;
    static constexpr size_t MAX_SAMPLE_SLOTS = 16;

    SoundGenerator();
    ~SoundGenerator() = default;

    void init(uint32_t sample_rate);
    void reset();

    /**
     * @brief Triggers a sound voice. Real-Time safe.
     */
    void trigger(const LayerConfig& layer, BeatAccent accent);

    /**
     * @brief Mixes active voices into the destination stereo buffer. Real-Time safe.
     */
    void render(float* buffer, size_t num_frames);

    /**
     * @brief Loads a PCM WAV buffer into a sample slot. (Called outside RT loop).
     */
    bool load_sample(size_t slot, const float* data, size_t frame_count, uint16_t channels, uint32_t sample_rate);

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    std::array<ActiveVoice, MAX_VOICES> voices_{};
    std::array<PCMSampleData, MAX_SAMPLE_SLOTS> sample_slots_{};
};

} // namespace polyrhythm
