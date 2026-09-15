#pragma once

#include <cstdint>

namespace LivingHyrule {

enum class PopulationPhase { Child, AdultCrisis, AdultRecovered };

// Bits match ResidentRole: carpenter, tenant, supplier. Explicit location and
// scene gates keep the first population increment out of unrelated adventures.
constexpr uint8_t ResidentMaskFor(bool enabled, bool supportedAdventure, bool inKakariko, bool normalScene,
                                  bool daytime, PopulationPhase phase) {
    if (!enabled || !supportedAdventure || !inKakariko || !normalScene || !daytime) {
        return 0;
    }
    switch (phase) {
        case PopulationPhase::Child:
        case PopulationPhase::AdultRecovered:
            return 0b111;
        case PopulationPhase::AdultCrisis:
            return 0b010;
    }
    return 0;
}

} // namespace LivingHyrule
