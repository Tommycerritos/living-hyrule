#pragma once

#include "Properties.h"
#include <cstdint>

namespace LivingHyrule {

enum class RoyalResidentId : uint8_t { Zelda, Aren, Maelin, Count };

struct RoyalAudienceContext {
    bool enabled = false;
    bool supportedAdventure = false;
    bool normalScene = false;
    bool castleApproach = false;
    bool royalGarden = false;
    bool daytime = false;
    WorldProgress world{};
    EconomyState economy{};
};

constexpr uint8_t RoyalResidentBit(RoyalResidentId id) {
    return id < RoyalResidentId::Count ? static_cast<uint8_t>(1u << static_cast<unsigned int>(id)) : 0;
}

// The audience belongs to the optional postgame world. Ownership, medallions,
// and a completed time-split list cannot substitute for actual Ganon evidence.
// The engine adapter supplies that evidence through GetWorldProgress().
inline uint8_t RoyalResidentMaskFor(const RoyalAudienceContext& context) {
    if (!context.enabled || !context.supportedAdventure || !context.normalScene ||
        (!context.castleApproach && !context.royalGarden) || !context.world.adult || !context.world.ganonDefeated ||
        !IsValidState(context.economy) || context.economy.enabled != 1) {
        return 0;
    }
    const uint8_t guard = RoyalResidentBit(RoyalResidentId::Aren);
    return context.daytime ? static_cast<uint8_t>(guard | RoyalResidentBit(RoyalResidentId::Zelda) |
                                                  RoyalResidentBit(RoyalResidentId::Maelin))
                           : guard;
}

} // namespace LivingHyrule
