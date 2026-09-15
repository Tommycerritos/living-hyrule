#include "MarketRestorationPolicy.h"
#include <cstdint>
#include <iostream>

int main() {
    using namespace LivingHyrule;
    unsigned int checks = 0;
    const auto check = [&](bool value) {
        ++checks;
        if (!value) {
            std::cerr << "Restoration policy failure at check " << checks << '\n';
            return false;
        }
        return true;
    };
    for (unsigned int gates = 0; gates < 256; ++gates) {
        for (int scene : { -1, 0x20, 0x21, 0x22, 0x64 }) {
            for (int spawn : { -1, 0, 1, 2, 3, 10, 255 }) {
                const bool allowed = RestorationEntryAllowed((gates & 1) != 0, (gates & 2) != 0, (gates & 4) != 0,
                                                             (gates & 8) != 0, (gates & 16) != 0, (gates & 32) != 0,
                                                             (gates & 64) != 0, (gates & 128) != 0, scene, spawn);
                if (!check(allowed == (gates == 255 && scene == 0x22 && spawn >= 0 && spawn <= 2)))
                    return 1;
            }
        }
    }
    // Any native camera value must land in the adult two-entry table. Preserve
    // only the three original adult exits and every unrelated material bit.
    for (uint32_t high : { 0u, 0xffffe000u, 0x92346000u, 0x55554000u }) {
        for (uint32_t exit = 0; exit < 32; ++exit) {
            for (uint32_t camera = 0; camera < 256; ++camera) {
                const uint32_t original = high | (exit << 8) | camera;
                const uint32_t result = RestorationSurface(original);
                if (!check((result & 255u) == 1))
                    return 1;
                if (!check(((result >> 8) & 31u) == (exit <= 3 ? exit : 0)))
                    return 1;
                if (!check((result & ~0x1fffu) == high))
                    return 1;
                if (!check(RestorationSurface(result) == result))
                    return 1;
            }
        }
    }
    std::cout << checks << " restoration policy checks passed.\n";
    return 0;
}
