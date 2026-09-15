#pragma once

#include "living_hyrule_stewardship.h"
#include <cstdint>
#include <type_traits>

namespace LivingHyrule {

using StewardshipState = LivingHyruleStewardshipData;
inline constexpr uint8_t kStewardshipRegionCount = 8;
inline constexpr uint64_t kTreasuryLimit = 99999999;
static_assert(std::is_trivial_v<StewardshipState>);
static_assert(std::is_standard_layout_v<StewardshipState>);

constexpr bool HoldsRegionalCharter(const StewardshipState& state, uint8_t region) {
    return region < kStewardshipRegionCount && (state.charterMask & (1u << region)) != 0;
}
inline bool IsValidStewardshipState(const StewardshipState& state) {
    for (uint8_t region = 0; region < kStewardshipRegionCount; ++region) {
        if (state.treasury[region] > kTreasuryLimit ||
            (!HoldsRegionalCharter(state, region) && state.treasury[region] != 0))
            return false;
    }
    return true;
}
constexpr uint8_t RegionalCharterCount(const StewardshipState& state) {
    uint8_t count = 0;
    for (uint8_t region = 0; region < kStewardshipRegionCount; ++region)
        count += HoldsRegionalCharter(state, region) ? 1 : 0;
    return count;
}

// Called once when an operating adult business completes its existing income
// period. Charter dues are a 10% reward, kept in that region's separate treasury.
// Bank saturation never changes the size of a period or creates a later debt.
inline uint32_t CreditRegionalDues(StewardshipState& state, uint8_t region, uint32_t businessIncome) {
    if (!IsValidStewardshipState(state) || !HoldsRegionalCharter(state, region))
        return 0;
    const uint32_t due = businessIncome / 10u;
    const uint64_t room = kTreasuryLimit - state.treasury[region];
    const auto credited = room < due ? static_cast<uint32_t>(room) : due;
    state.treasury[region] += credited;
    return credited;
}

} // namespace LivingHyrule
