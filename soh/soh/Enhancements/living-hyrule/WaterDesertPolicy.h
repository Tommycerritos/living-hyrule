#pragma once

#include "Properties.h"
#include <cstdint>

namespace LivingHyrule {

enum class WaterDesertResidentId : uint8_t { Lethra, Neris, Rasha, Kesra, Count };
enum class WaterDesertPlace : uint8_t { None, RiverBank, ValleyApproach, Fortress, Count };

struct WaterDesertContext {
    bool enabled = false;
    bool supportedAdventure = false;
    bool normalScene = false;
    bool daytime = false;
    bool carpentersFreed = false;
    WaterDesertPlace place = WaterDesertPlace::None;
    WorldProgress world{};
    EconomyState economy{};
};

constexpr uint8_t WaterDesertBit(WaterDesertResidentId id) {
    return id < WaterDesertResidentId::Count ? static_cast<uint8_t>(1u << static_cast<unsigned int>(id)) : 0;
}

inline uint8_t WaterDesertMaskFor(const WaterDesertContext& context) {
    if (!context.enabled || !context.supportedAdventure || !context.normalScene)
        return 0;
    const auto bit = WaterDesertBit;
    const auto& world = context.world;
    const bool active = IsValidState(context.economy) && context.economy.enabled == 1;
    const auto working = [&](uint32_t property) {
        return active && PropertyOperating(context.economy, property, world);
    };
    // Rescuing the carpenters, receiving permission, and solving the Spirit
    // Temple are separate requirements. A friendly trader never grants access.
    const bool invited = world.adult && context.carpentersFreed && world.gerudoMembership;
    switch (context.place) {
        case WaterDesertPlace::RiverBank:
            if (!context.daytime)
                return working(13) ? bit(WaterDesertResidentId::Neris) : 0;
            return static_cast<uint8_t>(bit(WaterDesertResidentId::Lethra) |
                                        (!world.adult || world.water ? bit(WaterDesertResidentId::Neris) : 0));
        case WaterDesertPlace::ValleyApproach:
            // This placement is on the public, field-side high ground. A child
            // can meet a quartermaster without crossing the guarded bridge.
            if (!world.adult)
                return context.daytime ? bit(WaterDesertResidentId::Rasha) : 0;
            return invited && (context.daytime || working(14)) ? bit(WaterDesertResidentId::Rasha) : 0;
        case WaterDesertPlace::Fortress:
            return invited && (context.daytime || working(15)) ? bit(WaterDesertResidentId::Kesra) : 0;
        default:
            // In particular, no frozen Domain/Fountain or wasteland setup is
            // made habitable by a medallion or a paid repair bit.
            return 0;
    }
}

} // namespace LivingHyrule
