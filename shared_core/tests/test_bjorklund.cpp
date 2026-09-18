#include "bjorklund.hpp"
#include <iostream>
#include <cassert>
#include <vector>

void print_pattern(const std::string& name, const std::vector<polyrhythm::BeatAccent>& accents) {
    std::cout << "  " << name << ": [";
    for (size_t i = 0; i < accents.size(); ++i) {
        if (accents[i] == polyrhythm::BeatAccent::Downbeat) std::cout << "D";
        else if (accents[i] == polyrhythm::BeatAccent::Normal) std::cout << "X";
        else std::cout << ".";
    }
    std::cout << "]" << std::endl;
}

int main() {
    std::cout << "[TEST] Running Bjorklund Euclidean Algorithm Test..." << std::endl;

    // 1. E(3, 8) - Cuban Tresillo [D . . X . . X .]
    auto e3_8 = polyrhythm::Bjorklund::generate(3, 8, true);
    print_pattern("E(3, 8)", e3_8);
    assert(e3_8.size() == 8);
    assert(e3_8[0] == polyrhythm::BeatAccent::Downbeat);
    assert(e3_8[3] == polyrhythm::BeatAccent::Normal);
    assert(e3_8[6] == polyrhythm::BeatAccent::Normal);
    assert(e3_8[1] == polyrhythm::BeatAccent::Mute && e3_8[2] == polyrhythm::BeatAccent::Mute);

    // 2. E(5, 8) - Cinquillo [D . X X . X X .]
    auto e5_8 = polyrhythm::Bjorklund::generate(5, 8, true);
    print_pattern("E(5, 8)", e5_8);
    assert(e5_8.size() == 8);
    size_t active_count = 0;
    for (auto a : e5_8) {
        if (a != polyrhythm::BeatAccent::Mute) active_count++;
    }
    assert(active_count == 5);

    // 3. E(4, 12) - Evenly spaced [D . . X . . X . . X . .]
    auto e4_12 = polyrhythm::Bjorklund::generate(4, 12, true);
    print_pattern("E(4, 12)", e4_12);
    assert(e4_12[0] == polyrhythm::BeatAccent::Downbeat);
    assert(e4_12[3] == polyrhythm::BeatAccent::Normal);
    assert(e4_12[6] == polyrhythm::BeatAccent::Normal);
    assert(e4_12[9] == polyrhythm::BeatAccent::Normal);

    // 4. Sınır durumları
    auto e0_8 = polyrhythm::Bjorklund::generate(0, 8, true);
    for (auto a : e0_8) assert(a == polyrhythm::BeatAccent::Mute);

    auto e8_8 = polyrhythm::Bjorklund::generate(8, 8, true);
    assert(e8_8[0] == polyrhythm::BeatAccent::Downbeat);
    for (size_t i = 1; i < 8; ++i) assert(e8_8[i] == polyrhythm::BeatAccent::Normal);

    std::cout << "[PASS] Bjorklund Euclidean tests passed successfully!" << std::endl;
    return 0;
}
