#include "sound_generator.hpp"
#include <cmath>
#include <algorithm>
#include <numbers>

namespace polyrhythm {

SoundGenerator::SoundGenerator() {
    reset();
}

void SoundGenerator::init(uint32_t sample_rate) {
    sample_rate_ = (sample_rate > 0) ? sample_rate : DEFAULT_SAMPLE_RATE;
    reset();
    generate_preset_samples();
}

void SoundGenerator::reset() {
    for (auto& voice : voices_) {
        voice.is_active = false;
    }
}

void SoundGenerator::generate_preset_samples() {
    const float sr = static_cast<float>(sample_rate_);
    const float two_pi = 2.0f * static_cast<float>(std::numbers::pi);

    // Simple deterministic LCG random generator for noise transients
    uint32_t lcg_state = 123456789;
    auto next_noise = [&lcg_state]() -> float {
        lcg_state = lcg_state * 1664525u + 1013904223u;
        return (static_cast<float>(lcg_state & 0x7FFFFFFF) / 1073741824.0f) - 1.0f; // [-1.0, 1.0]
    };

    auto normalize_and_store = [](PCMSampleData& pcm, std::vector<float>& buf, uint32_t srate) {
        float max_val = 0.0f;
        for (float v : buf) {
            float av = std::abs(v);
            if (av > max_val) max_val = av;
        }
        if (max_val > 0.0001f) {
            float scale = 0.95f / max_val;
            for (float& v : buf) v *= scale;
        }
        pcm.channels = 1;
        pcm.sample_rate = srate;
        pcm.frame_count = buf.size();
        pcm.samples = std::move(buf);
    };

    // 0: DIGITAL Downbeat
    {
        size_t frames = static_cast<size_t>(0.035f * sr);
        std::vector<float> buf(frames, 0.0f);
        float phase = 0.0f;
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float freq = 1600.0f + 800.0f * std::exp(-t / 0.005f);
            phase += (two_pi * freq) / sr;
            float env = std::exp(-t / 0.020f);
            buf[i] = std::sin(phase) * env;
        }
        normalize_and_store(preset_samples_[0], buf, sample_rate_);
    }

    // 1: DIGITAL Normal
    {
        size_t frames = static_cast<size_t>(0.025f * sr);
        std::vector<float> buf(frames, 0.0f);
        float phase = 0.0f;
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float freq = 1000.0f + 500.0f * std::exp(-t / 0.004f);
            phase += (two_pi * freq) / sr;
            float env = std::exp(-t / 0.012f);
            buf[i] = std::sin(phase) * env;
        }
        normalize_and_store(preset_samples_[1], buf, sample_rate_);
    }

    // 2: WOODBLOCK Downbeat (High Claves / Resonant Woodblock)
    {
        size_t frames = static_cast<size_t>(0.045f * sr);
        std::vector<float> buf(frames, 0.0f);
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float attack_click = (i < 48) ? next_noise() * (1.0f - static_cast<float>(i) / 48.0f) : 0.0f;
            float s1 = std::sin(two_pi * 2150.0f * t) * 0.65f;
            float s2 = std::sin(two_pi * 3250.0f * t) * 0.30f;
            float s3 = std::sin(two_pi * 4300.0f * t) * 0.15f;
            float env = std::exp(-t / 0.022f);
            buf[i] = (s1 + s2 + s3 + attack_click * 0.45f) * env;
        }
        normalize_and_store(preset_samples_[2], buf, sample_rate_);
    }

    // 3: WOODBLOCK Normal (Dry Woodblock Tap)
    {
        size_t frames = static_cast<size_t>(0.030f * sr);
        std::vector<float> buf(frames, 0.0f);
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float attack_click = (i < 32) ? next_noise() * (1.0f - static_cast<float>(i) / 32.0f) : 0.0f;
            float s1 = std::sin(two_pi * 1400.0f * t) * 0.70f;
            float s2 = std::sin(two_pi * 2100.0f * t) * 0.25f;
            float s3 = std::sin(two_pi * 2900.0f * t) * 0.10f;
            float env = std::exp(-t / 0.015f);
            buf[i] = (s1 + s2 + s3 + attack_click * 0.35f) * env;
        }
        normalize_and_store(preset_samples_[3], buf, sample_rate_);
    }

    // 4: MECHANICAL Downbeat (Pyramid Metronome Bell Ping + Wood Thump)
    {
        size_t frames = static_cast<size_t>(0.080f * sr);
        std::vector<float> buf(frames, 0.0f);
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float click = (i < 24) ? next_noise() * 0.8f : 0.0f;
            float bell1 = std::sin(two_pi * 1760.0f * t) * std::exp(-t / 0.055f) * 0.60f;
            float bell2 = std::sin(two_pi * 3520.0f * t) * std::exp(-t / 0.035f) * 0.25f;
            float thump = std::sin(two_pi * 340.0f * t) * std::exp(-t / 0.025f) * 0.50f;
            buf[i] = bell1 + bell2 + thump + click * 0.3f;
        }
        normalize_and_store(preset_samples_[4], buf, sample_rate_);
    }

    // 5: MECHANICAL Normal (Escapement Mechanism Tick)
    {
        size_t frames = static_cast<size_t>(0.028f * sr);
        std::vector<float> buf(frames, 0.0f);
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float t_esc = std::abs(t - 0.0028f);
            float dual_click = std::exp(-t / 0.001f) + 0.6f * std::exp(-t_esc / 0.0008f);
            float cavity1 = std::sin(two_pi * 750.0f * t) * 0.65f;
            float cavity2 = std::sin(two_pi * 1150.0f * t) * 0.35f;
            float env = std::exp(-t / 0.012f);
            buf[i] = (cavity1 + cavity2) * env + dual_click * 0.4f;
        }
        normalize_and_store(preset_samples_[5], buf, sample_rate_);
    }

    // 6: SNARE_RIM Downbeat (Tok Akustik Rimshot / Snare)
    {
        size_t frames = static_cast<size_t>(0.085f * sr);
        std::vector<float> buf(frames, 0.0f);
        float phase_head = 0.0f;
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float freq_head = 210.0f + 170.0f * std::exp(-t / 0.012f);
            phase_head += (two_pi * freq_head) / sr;
            float head = std::sin(phase_head) * std::exp(-t / 0.050f) * 0.70f;
            float rim = (std::sin(two_pi * 1350.0f * t) * 0.55f + std::sin(two_pi * 2600.0f * t) * 0.25f) * std::exp(-t / 0.018f);
            float wires = next_noise() * std::exp(-t / 0.065f) * 0.45f;
            buf[i] = head + rim + wires;
        }
        normalize_and_store(preset_samples_[6], buf, sample_rate_);
    }

    // 7: SNARE_RIM Normal (Kuru Cross-Stick / Sidestick)
    {
        size_t frames = static_cast<size_t>(0.035f * sr);
        std::vector<float> buf(frames, 0.0f);
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float click = (i < 20) ? next_noise() * 0.8f : 0.0f;
            float rim1 = std::sin(two_pi * 1650.0f * t) * 0.70f * std::exp(-t / 0.018f);
            float rim2 = std::sin(two_pi * 3300.0f * t) * 0.35f * std::exp(-t / 0.012f);
            float shell = std::sin(two_pi * 580.0f * t) * 0.45f * std::exp(-t / 0.024f);
            buf[i] = rim1 + rim2 + shell + click * 0.4f;
        }
        normalize_and_store(preset_samples_[7], buf, sample_rate_);
    }

    // 8: HIHAT Downbeat (Yarı Açık / Vurgulu Pedal Hat)
    {
        size_t frames = static_cast<size_t>(0.130f * sr);
        std::vector<float> buf(frames, 0.0f);
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            // Inharmonic square/metallic partials
            float p1 = (std::sin(two_pi * 295.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p2 = (std::sin(two_pi * 545.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p3 = (std::sin(two_pi * 800.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p4 = (std::sin(two_pi * 1200.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p5 = (std::sin(two_pi * 3600.0f * t) > 0 ? 1.0f : -1.0f) * 0.15f;
            float p6 = (std::sin(two_pi * 5800.0f * t) > 0 ? 1.0f : -1.0f) * 0.15f;
            float metallic = p1 + p2 + p3 + p4 + p5 + p6;
            float noise = next_noise() * 0.5f;
            float env = std::exp(-t / 0.095f);
            buf[i] = (metallic * 0.45f + noise * 0.55f) * env;
        }
        normalize_and_store(preset_samples_[8], buf, sample_rate_);
    }

    // 9: HIHAT Normal (Keskin Kapalı Hi-Hat)
    {
        size_t frames = static_cast<size_t>(0.032f * sr);
        std::vector<float> buf(frames, 0.0f);
        for (size_t i = 0; i < frames; ++i) {
            float t = static_cast<float>(i) / sr;
            float p1 = (std::sin(two_pi * 295.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p2 = (std::sin(two_pi * 545.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p3 = (std::sin(two_pi * 800.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p4 = (std::sin(two_pi * 1200.0f * t) > 0 ? 1.0f : -1.0f) * 0.2f;
            float p5 = (std::sin(two_pi * 3600.0f * t) > 0 ? 1.0f : -1.0f) * 0.15f;
            float metallic = p1 + p2 + p3 + p4 + p5;
            float noise = next_noise() * 0.55f;
            float env = std::exp(-t / 0.018f);
            buf[i] = (metallic * 0.4f + noise * 0.6f) * env;
        }
        normalize_and_store(preset_samples_[9], buf, sample_rate_);
    }
}

void SoundGenerator::trigger(const LayerConfig& layer, BeatAccent accent) {
    if (accent == BeatAccent::Mute || layer.is_muted) {
        return;
    }

    size_t target_idx = 0;
    float min_amp = 1.0f;

    for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (!voices_[i].is_active) {
            target_idx = i;
            break;
        }
        if (voices_[i].amplitude < min_amp) {
            min_amp = voices_[i].amplitude;
            target_idx = i;
        }
    }

    ActiveVoice& v = voices_[target_idx];
    v.is_active = true;
    v.accent = accent;
    v.type = SoundType::PCMSample;

    float clamped_pan = std::clamp(layer.pan, -1.0f, 1.0f);
    float pan_rad = (clamped_pan + 1.0f) * 0.25f * static_cast<float>(std::numbers::pi);
    float base_gain = layer.volume * ((accent == BeatAccent::Downbeat) ? 1.25f : 0.85f);
    v.gain_left = std::cos(pan_rad) * base_gain;
    v.gain_right = std::sin(pan_rad) * base_gain;

    if (layer.sound_type == SoundType::PCMSample && layer.sample_id >= 0 && 
        static_cast<size_t>(layer.sample_id) < MAX_SAMPLE_SLOTS && 
        !sample_slots_[layer.sample_id].samples.empty()) {
        
        v.pcm_data = &sample_slots_[layer.sample_id];
        v.pcm_playback_index = 0.0;
        double sr_ratio = static_cast<double>(v.pcm_data->sample_rate) / static_cast<double>(sample_rate_);
        v.pcm_pitch_ratio = sr_ratio * std::clamp(static_cast<double>(layer.pitch_shift), 0.25, 4.0);
        v.amplitude = 1.0f;
    } else {
        // Built-in Studio Sound Preset
        size_t preset_id = static_cast<size_t>(layer.sound_preset);
        if (preset_id >= NUM_SOUND_PRESETS) preset_id = 0;
        size_t slot_idx = preset_id * 2 + ((accent == BeatAccent::Downbeat) ? 0 : 1);

        if (!preset_samples_[slot_idx].samples.empty()) {
            v.pcm_data = &preset_samples_[slot_idx];
            v.pcm_playback_index = 0.0;
            double sr_ratio = static_cast<double>(v.pcm_data->sample_rate) / static_cast<double>(sample_rate_);
            v.pcm_pitch_ratio = sr_ratio * std::clamp(static_cast<double>(layer.pitch_shift), 0.25, 4.0);
            v.amplitude = 1.0f;
        } else {
            // Fallback synthetic sine if presets uninitialized
            v.type = SoundType::SyntheticSine;
            v.pcm_data = nullptr;
            v.phase = 0.0f;
            v.amplitude = 1.0f;
            float freq_mult = (accent == BeatAccent::Downbeat) ? 1.6f : 1.0f;
            v.base_freq = std::clamp(layer.synth_frequency * freq_mult * layer.pitch_shift, 60.0f, 16000.0f);
            v.current_freq = v.base_freq * 1.5f;
            float decay_time_sec = (accent == BeatAccent::Downbeat) ? 0.025f : 0.015f;
            v.decay_rate = std::exp(-1.0f / (decay_time_sec * static_cast<float>(sample_rate_)));
            v.phase_inc = (2.0f * static_cast<float>(std::numbers::pi) * v.current_freq) / static_cast<float>(sample_rate_);
        }
    }
}

void SoundGenerator::render(float* buffer, size_t num_frames) {
    if (buffer == nullptr || num_frames == 0) return;

    for (size_t f = 0; f < num_frames; ++f) {
        float mix_l = 0.0f;
        float mix_r = 0.0f;

        for (auto& v : voices_) {
            if (!v.is_active) continue;

            float sample_val = 0.0f;

            if (v.pcm_data != nullptr) {
                // PCM Sample Çalma (Linear Interpolation)
                size_t idx0 = static_cast<size_t>(v.pcm_playback_index);
                size_t idx1 = idx0 + 1;
                float frac = static_cast<float>(v.pcm_playback_index - idx0);

                if (idx1 >= v.pcm_data->frame_count) {
                    v.is_active = false;
                    continue;
                }

                if (v.pcm_data->channels == 2) {
                    float s0_l = v.pcm_data->samples[idx0 * 2];
                    float s1_l = v.pcm_data->samples[idx1 * 2];
                    float s0_r = v.pcm_data->samples[idx0 * 2 + 1];
                    float s1_r = v.pcm_data->samples[idx1 * 2 + 1];

                    float val_l = s0_l + frac * (s1_l - s0_l);
                    float val_r = s0_r + frac * (s1_r - s0_r);

                    mix_l += val_l * v.gain_left;
                    mix_r += val_r * v.gain_right;
                } else {
                    float s0 = v.pcm_data->samples[idx0];
                    float s1 = v.pcm_data->samples[idx1];
                    float mono_val = s0 + frac * (s1 - s0);

                    mix_l += mono_val * v.gain_left;
                    mix_r += mono_val * v.gain_right;
                }

                v.pcm_playback_index += v.pcm_pitch_ratio;
            } else {
                // Sentetik Transient Klik
                sample_val = std::sin(v.phase) * v.amplitude;

                mix_l += sample_val * v.gain_left;
                mix_r += sample_val * v.gain_right;

                // Faz ilerletme ve üssel sönüm
                v.phase += v.phase_inc;
                if (v.phase > 2.0f * static_cast<float>(std::numbers::pi)) {
                    v.phase -= 2.0f * static_cast<float>(std::numbers::pi);
                }

                // Transient frekans yumuşatma
                v.current_freq = v.current_freq * 0.96f + v.base_freq * 0.04f;
                v.phase_inc = (2.0f * static_cast<float>(std::numbers::pi) * v.current_freq) / static_cast<float>(sample_rate_);

                v.amplitude *= v.decay_rate;
                if (v.amplitude < 0.0005f) {
                    v.is_active = false;
                }
            }
        }

        // Stereo Interleaved Buffer'a ekle
        buffer[f * 2]     += mix_l;
        buffer[f * 2 + 1] += mix_r;
    }
}

bool SoundGenerator::load_sample(size_t slot, const float* data, size_t frame_count, uint16_t channels, uint32_t sample_rate) {
    if (slot >= MAX_SAMPLE_SLOTS || data == nullptr || frame_count == 0 || channels == 0) {
        return false;
    }

    PCMSampleData& pcm = sample_slots_[slot];
    pcm.channels = channels;
    pcm.sample_rate = (sample_rate > 0) ? sample_rate : DEFAULT_SAMPLE_RATE;
    pcm.frame_count = frame_count;
    pcm.samples.assign(data, data + (frame_count * channels));
    return true;
}

} // namespace polyrhythm
