#pragma once

#include "Properties.h"
#include <cstdint>

namespace LivingHyrule {

enum class WorldResidentId : uint8_t { Vessa, Hadrin, Pella, Caro, Hollis, Nessa, Wren, Vero, Edda, Count };
enum class WorldPopulationPlace : uint8_t { None, MarketDay, MarketNight, MarketRuins, Field, Ranch, Lake, Count };

struct WorldPopulationContext {
    bool enabled = false;
    bool supportedAdventure = false;
    bool normalScene = false;
    bool daytime = false;
    WorldPopulationPlace place = WorldPopulationPlace::None;
    WorldProgress world{};
    EconomyState economy{};
};

constexpr uint16_t WorldResidentBit(WorldResidentId id) {
    return id < WorldResidentId::Count ? static_cast<uint16_t>(1u << static_cast<unsigned int>(id)) : 0;
}

// Population is independently optional. Only business-dependent schedules and
// relief in dangerous adult Castle Town require an enabled, readable economy.
inline uint16_t WorldResidentMaskFor(const WorldPopulationContext& context) {
    if (!context.enabled || !context.supportedAdventure || !context.normalScene)
        return 0;
    const auto bit = WorldResidentBit;
    const auto& world = context.world;
    const bool ledgerActive = IsValidState(context.economy) && context.economy.enabled == 1;
    const auto working = [&](uint32_t property) {
        return ledgerActive && PropertyOperating(context.economy, property, world);
    };
    const bool safeAdultMarket = world.adult && world.ganonDefeated && ledgerActive;
    switch (context.place) {
        case WorldPopulationPlace::MarketDay:
            if (!context.daytime)
                return 0;
            if (!world.adult)
                return static_cast<uint16_t>(bit(WorldResidentId::Vessa) | bit(WorldResidentId::Hadrin));
            return static_cast<uint16_t>(safeAdultMarket ? bit(WorldResidentId::Hadrin) | bit(WorldResidentId::Vessa)
                                                         : 0);
        case WorldPopulationPlace::MarketNight:
            return !context.daytime && (!world.adult || safeAdultMarket) ? bit(WorldResidentId::Pella) : 0;
        case WorldPopulationPlace::MarketRuins:
            // Matches the opt-in postgame cleanup prerequisite. Children and
            // debug visits before the ending must not populate the ruins.
            if (!safeAdultMarket)
                return 0;
            if (!context.daytime)
                return bit(WorldResidentId::Pella);
            // Keep both sellers available before investment: neither buying nor
            // repairing their business should require visiting the ledger first.
            return static_cast<uint16_t>(bit(WorldResidentId::Hadrin) | bit(WorldResidentId::Vessa));
        case WorldPopulationPlace::Field:
            return static_cast<uint16_t>(context.daytime && (!world.adult || world.forest)
                                             ? bit(WorldResidentId::Caro) | bit(WorldResidentId::Hollis)
                                             : 0);
        case WorldPopulationPlace::Ranch:
            if (!context.daytime)
                return working(5) ? bit(WorldResidentId::Wren) : 0;
            // One stable worker stays through Ingo's rule; commercial visitors
            // return only after the original Epona escape is complete.
            return static_cast<uint16_t>(bit(WorldResidentId::Wren) |
                                         (!world.adult || world.ranchFreed ? bit(WorldResidentId::Nessa) : 0));
        case WorldPopulationPlace::Lake:
            if (!context.daytime)
                return working(12) ? bit(WorldResidentId::Edda) : 0;
            // The research assistant remains on high ground in the crisis.
            return static_cast<uint16_t>(bit(WorldResidentId::Edda) |
                                         (!world.adult || world.water ? bit(WorldResidentId::Vero) : 0));
        default:
            return 0;
    }
}

} // namespace LivingHyrule
