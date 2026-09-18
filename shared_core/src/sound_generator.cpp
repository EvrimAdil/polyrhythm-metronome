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
}

void SoundGenerator::reset() {
    for (auto& voice : voices_) {
        voice.is_active = false;
    }
}

void SoundGenerator::trigger(const LayerConfig& layer, BeatAccent accent) {
    if (accent == BeatAccent::Mute || layer.is_muted) {
        return;
    }

    // Boş ses kanalı (voice) bul veya en sessiz olanı al
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
    v.type = layer.sound_type;

    // Pan hesaplaması: Equal-power panning (-1.0 Sol ... 0.0 Merkez ... +1.0 Sağ)
    float clamped_pan = std::clamp(layer.pan, -1.0f, 1.0f);
    float pan_rad = (clamped_pan + 1.0f) * 0.25f * static_cast<float>(std::numbers::pi);
    float base_gain = layer.volume * ((accent == BeatAccent::Downbeat) ? 1.3f : 0.85f);
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
        // Sentetik Transient Sinüs Klik
        v.type = SoundType::SyntheticSine;
        v.pcm_data = nullptr;
        v.phase = 0.0f;
        v.amplitude = 1.0f;

        float freq_mult = (accent == BeatAccent::Downbeat) ? 1.6f : 1.0f;
        v.base_freq = std::clamp(layer.synth_frequency * freq_mult * layer.pitch_shift, 60.0f, 16000.0f);
        v.current_freq = v.base_freq * 1.5f; // İlk vuruş anında hızlı pitch atağı (perküsif transient)

        // Sönüm süresi: yaklaşık 15-25 ms
        float decay_time_sec = (accent == BeatAccent::Downbeat) ? 0.025f : 0.015f;
        v.decay_rate = std::exp(-1.0f / (decay_time_sec * static_cast<float>(sample_rate_)));
        v.phase_inc = (2.0f * static_cast<float>(std::numbers::pi) * v.current_freq) / static_cast<float>(sample_rate_);
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
