#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

namespace polyrhythm {

constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
constexpr size_t NUM_STEREO_CHANNELS = 2;
constexpr size_t MAX_LAYERS = 16;
constexpr size_t MAX_STEPS_PER_LAYER = 64;

enum class BeatAccent : uint8_t {
    Mute = 0,
    Normal = 1,
    Downbeat = 2  // Vurgulu ilk vuruş / aksan
};

enum class RhythmMode : uint8_t {
    Polyrhythmic = 0, // Ortak süre penceresinde X:Y:Z oranları
    Polymetric = 1,   // Ortak alt bölüntü zamanı, bağımsız ölçü uzunlukları
    Euclidean = 2     // Bjorklund algoritması tabanlı k vuruş / n adım
};

enum class SoundType : uint8_t {
    SyntheticSine = 0,  // Üssel sönümlü transient sinüs klik
    SyntheticWood = 1,  // Yüksek rezonanslı klik
    PCMSample = 2       // RAM'e yüklenmiş WAV ses örneği
};

struct BeatEvent {
    uint64_t sample_timestamp{0};
    uint8_t layer_index{0};
    uint16_t step_index{0};
    uint16_t total_steps{0};
    BeatAccent accent{BeatAccent::Normal};
    float current_phase{0.0f}; // 0.0f - 1.0f (poligon animasyonu için)
    uint32_t bpm{120};
};

struct LayerConfig {
    uint8_t layer_id{0};
    RhythmMode mode{RhythmMode::Polyrhythmic};
    
    // Poliritmik / Polimetrik parametreleri
    uint16_t ratio_pulses{4};    // Poliritmik pay veya Euclidean vuruş sayısı (k)
    uint16_t total_steps{4};     // Polimetrik ölçü adımı veya Euclidean toplam adım (n)
    uint16_t subdivision{1};     // 1 = Çeyrek, 2 = Sekizlik, 3 = Üçleme, 4 = Onaltılık
    
    // 3 Seviyeli vurgu dizisi (maksimum 64 adım)
    BeatAccent accents[MAX_STEPS_PER_LAYER]{BeatAccent::Downbeat, BeatAccent::Normal, BeatAccent::Normal, BeatAccent::Normal};
    uint16_t accent_count{4};
    
    // Mikser parametreleri
    float volume{0.8f};          // 0.0f - 1.5f
    float pan{0.0f};             // -1.0f (Sol) ... 0.0f (Merkez) ... +1.0f (Sağ)
    float pitch_shift{1.0f};     // 0.5f ... 2.0f
    float synth_frequency{1000.0f}; // Hz (Sentetik klik için frekans)
    SoundType sound_type{SoundType::SyntheticSine};
    int32_t sample_id{-1};       // WAV sample slot indeksi (-1 sentetik)
    bool is_muted{false};
    bool is_solo{false};

    LayerConfig() {
        accents[0] = BeatAccent::Downbeat;
        for (size_t i = 1; i < MAX_STEPS_PER_LAYER; ++i) {
            accents[i] = BeatAccent::Normal;
        }
    }
};

struct MetronomeConfig {
    double bpm{120.0};
    uint32_t sample_rate{DEFAULT_SAMPLE_RATE};
    uint16_t time_signature_numerator{4};
    uint16_t time_signature_denominator{4};
    bool is_playing{false};
};

} // namespace polyrhythm
