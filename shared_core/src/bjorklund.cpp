#include "bjorklund.hpp"
#include <algorithm>
#include <vector>

namespace polyrhythm {

void Bjorklund::generate(uint16_t pulses, uint16_t steps, BeatAccent* accents, bool downbeat_first) {
    if (steps == 0 || accents == nullptr) return;

    if (pulses == 0) {
        for (uint16_t i = 0; i < steps; ++i) {
            accents[i] = BeatAccent::Mute;
        }
        return;
    }

    if (pulses >= steps) {
        accents[0] = downbeat_first ? BeatAccent::Downbeat : BeatAccent::Normal;
        for (uint16_t i = 1; i < steps; ++i) {
            accents[i] = BeatAccent::Normal;
        }
        return;
    }

    // Bjorklund algoritması:
    // Başlangıç: k adet [1] ve (steps - k) adet [0]
    std::vector<std::vector<bool>> sequences;
    sequences.reserve(steps);

    for (uint16_t i = 0; i < pulses; ++i) {
        sequences.push_back({true});
    }
    for (uint16_t i = 0; i < (steps - pulses); ++i) {
        sequences.push_back({false});
    }

    size_t count_ones = pulses;
    size_t count_zeros = steps - pulses;

    while (count_zeros > 1) {
        size_t num_pairs = std::min(count_ones, count_zeros);
        for (size_t i = 0; i < num_pairs; ++i) {
            // Son elemanları öndekilere ekle
            sequences[i].insert(
                sequences[i].end(),
                sequences[sequences.size() - 1 - i].begin(),
                sequences[sequences.size() - 1 - i].end()
            );
        }
        sequences.resize(sequences.size() - num_pairs);
        count_zeros = sequences.size() - num_pairs;
        count_ones = num_pairs;
    }

    // Düzleştir ve BeatAccent çıktısına dönüştür
    size_t out_idx = 0;
    bool is_first_hit = true;

    for (const auto& seq : sequences) {
        for (bool hit : seq) {
            if (out_idx >= steps) break;

            if (hit) {
                if (is_first_hit && downbeat_first) {
                    accents[out_idx] = BeatAccent::Downbeat;
                    is_first_hit = false;
                } else {
                    accents[out_idx] = BeatAccent::Normal;
                    is_first_hit = false;
                }
            } else {
                accents[out_idx] = BeatAccent::Mute;
            }
            out_idx++;
        }
    }
}

std::vector<BeatAccent> Bjorklund::generate(uint16_t pulses, uint16_t steps, bool downbeat_first) {
    if (steps == 0) return {};
    std::vector<BeatAccent> result(steps, BeatAccent::Mute);
    generate(pulses, steps, result.data(), downbeat_first);
    return result;
}

} // namespace polyrhythm
