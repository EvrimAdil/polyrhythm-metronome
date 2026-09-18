#pragma once

#include "audio_types.hpp"
#include <vector>
#include <cstdint>

namespace polyrhythm {

class Bjorklund {
public:
    /**
     * @brief Computes Euclidean rhythm E(k, n) using the Bjorklund algorithm.
     * @param pulses Number of active pulses (k)
     * @param steps Total number of steps (n)
     * @param accents Output array of BeatAccent (allocated size at least steps)
     * @param downbeat_first If true, first active pulse is set to BeatAccent::Downbeat
     */
    static void generate(uint16_t pulses, uint16_t steps, BeatAccent* accents, bool downbeat_first = true);

    /**
     * @brief Vector-returning overload for convenience outside RT loop.
     */
    static std::vector<BeatAccent> generate(uint16_t pulses, uint16_t steps, bool downbeat_first = true);
};

} // namespace polyrhythm
