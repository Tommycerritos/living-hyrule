#include "PopulationPolicy.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

namespace {

using LivingHyrule::PopulationPhase;
using LivingHyrule::ResidentMaskFor;

constexpr uint8_t kCarpenter = 1u << 0;
constexpr uint8_t kTenant = 1u << 1;
constexpr uint8_t kSupplier = 1u << 2;
constexpr uint8_t kAllResidents = kCarpenter | kTenant | kSupplier;

// These also verify that the engine-independent policy is usable at compile time.
static_assert(ResidentMaskFor(true, true, true, true, true, PopulationPhase::Child) == kAllResidents);
static_assert(ResidentMaskFor(true, true, true, true, true, PopulationPhase::AdultCrisis) == kTenant);
static_assert(ResidentMaskFor(true, true, true, true, true, PopulationPhase::AdultRecovered) == kAllResidents);
static_assert(ResidentMaskFor(true, true, true, true, true, static_cast<PopulationPhase>(-1)) == 0);

struct PhaseCase {
    PopulationPhase phase;
    uint8_t expectedResidents;
    const char* name;
};

int failures = 0;
int cases = 0;

void CheckMask(bool enabled, bool supportedAdventure, bool inKakariko, bool normalScene, bool daytime,
               const PhaseCase& phase, uint8_t expected) {
    ++cases;
    const uint8_t actual = ResidentMaskFor(enabled, supportedAdventure, inKakariko, normalScene, daytime, phase.phase);
    if (actual != expected) {
        ++failures;
        std::cerr << "FAIL: " << phase.name << ", enabled=" << enabled << ", supportedAdventure=" << supportedAdventure
                  << ", inKakariko=" << inKakariko << ", normalScene=" << normalScene << ", daytime=" << daytime
                  << ": expected mask " << static_cast<unsigned int>(expected) << ", got "
                  << static_cast<unsigned int>(actual) << '\n';
    }
}

void CheckAllGateCombinations(const PhaseCase& phase) {
    // Each independently false gate must suppress the entire population. This
    // covers disabled saves, unsupported adventures, other scenes, cutscenes,
    // nighttime, and every combination of those restrictions.
    for (unsigned int gates = 0; gates < 32; ++gates) {
        const bool enabled = (gates & (1u << 0)) != 0;
        const bool supportedAdventure = (gates & (1u << 1)) != 0;
        const bool inKakariko = (gates & (1u << 2)) != 0;
        const bool normalScene = (gates & (1u << 3)) != 0;
        const bool daytime = (gates & (1u << 4)) != 0;
        // Only the all-true truth-table row may populate a supported phase.
        const uint8_t expected = gates == 31 ? phase.expectedResidents : uint8_t{ 0 };
        CheckMask(enabled, supportedAdventure, inKakariko, normalScene, daytime, phase, expected);
    }
}

} // namespace

int main() {
    constexpr std::array<PhaseCase, 3> supportedPhases = {{
        { PopulationPhase::Child, kAllResidents, "child" },
        { PopulationPhase::AdultCrisis, kTenant, "adult crisis" },
        { PopulationPhase::AdultRecovered, kAllResidents, "adult recovered" },
    }};
    for (const PhaseCase& phase : supportedPhases) {
        CheckAllGateCombinations(phase);
    }

    // Unknown values must fail closed even when every scene gate permits NPCs.
    // Include adjacent, negative, byte-sized, and extreme values so an unknown
    // phase cannot accidentally share the recovered-adult/default branch.
    using PhaseValue = std::underlying_type_t<PopulationPhase>;
    constexpr std::array<PhaseCase, 4> unsupportedPhases = {{
        { static_cast<PopulationPhase>(3), 0, "unknown phase 3" },
        { static_cast<PopulationPhase>(-1), 0, "unknown negative phase" },
        { static_cast<PopulationPhase>(255), 0, "unknown phase 255" },
        { static_cast<PopulationPhase>((std::numeric_limits<PhaseValue>::max)()), 0, "unknown maximum phase" },
    }};
    for (const PhaseCase& phase : unsupportedPhases) {
        CheckAllGateCombinations(phase);
    }

    if (failures != 0) {
        std::cerr << failures << " of " << cases << " population policy cases failed.\n";
        return 1;
    }
    std::cout << "Living Hyrule population: all " << cases << " gate and phase combinations passed.\n";
    return 0;
}
