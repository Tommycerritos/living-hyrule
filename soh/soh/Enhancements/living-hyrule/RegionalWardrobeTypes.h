#pragma once

#include "living_hyrule_wardrobe.h"
#include <cstdint>
#include <type_traits>

namespace LivingHyrule {

using WardrobeState = LivingHyruleWardrobeData;
static_assert(std::is_trivial_v<WardrobeState> && std::is_standard_layout_v<WardrobeState>);
inline constexpr uint8_t kRegionalStyleCount = 8;
inline constexpr uint16_t kRegionalStyleMask = (1u << kRegionalStyleCount) - 1u;

constexpr uint16_t RegionalStyleBit(uint8_t id) {
    return id >= 1 && id <= kRegionalStyleCount ? static_cast<uint16_t>(1u << (id - 1)) : 0;
}
inline bool OwnsRegionalStyle(const WardrobeState& state, uint8_t id) {
    return id == 0 || (state.ownedStyles & RegionalStyleBit(id)) != 0;
}
inline bool IsValidWardrobeState(const WardrobeState& state) {
    return (state.ownedStyles & ~kRegionalStyleMask) == 0 && state.equippedStyle <= kRegionalStyleCount &&
           OwnsRegionalStyle(state, state.equippedStyle);
}

} // namespace LivingHyrule
