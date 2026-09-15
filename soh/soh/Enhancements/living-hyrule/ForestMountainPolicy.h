#pragma once

#include "Properties.h"
#include <cstdint>

namespace LivingHyrule {

enum class ForestMountainResidentId : uint8_t { Fenn, Luma, Doron, Brakka, Count };
enum class ForestMountainPlace : uint8_t { None, KokiriForest, GoronCity, Count };

struct ForestMountainContext {
    bool enabled = false;
    bool supportedAdventure = false;
    bool normalScene = false;
    bool daytime = false;
    ForestMountainPlace place = ForestMountainPlace::None;
    WorldProgress world{};
    EconomyState economy{};
};

constexpr uint8_t ForestMountainResidentBit(ForestMountainResidentId id) {
    return id < ForestMountainResidentId::Count ? static_cast<uint8_t>(1u << static_cast<unsigned int>(id)) : 0;
}

constexpr int ForestMountainPropertyId(ForestMountainResidentId id) {
    switch (id) {
        case ForestMountainResidentId::Fenn:
            return 6;
        case ForestMountainResidentId::Luma:
            return 7;
        case ForestMountainResidentId::Doron:
            return 10;
        case ForestMountainResidentId::Brakka:
            return 11;
        default:
            return -1;
    }
}

inline uint8_t ForestMountainResidentMaskFor(const ForestMountainContext& context) {
    if (!context.enabled || !context.supportedAdventure || !context.normalScene)
        return 0;
    const auto bit = ForestMountainResidentBit;
    switch (context.place) {
        case ForestMountainPlace::KokiriForest:
            // Kokiri retain their childlike bodies in both eras. During the
            // adult monster crisis these two shelter indoors, outside this scope.
            if (!context.daytime || (context.world.adult && !context.world.forest))
                return 0;
            return static_cast<uint8_t>(bit(ForestMountainResidentId::Fenn) | bit(ForestMountainResidentId::Luma));
        case ForestMountainPlace::GoronCity:
            // Captivity leaves the ordinary worksites empty until Fire recovery.
            if (context.world.adult && !context.world.fire)
                return 0;
            if (context.daytime)
                return static_cast<uint8_t>(bit(ForestMountainResidentId::Doron) |
                                            bit(ForestMountainResidentId::Brakka));
            // A funded kiln has an evening tender. Both sellers remain available
            // by day before purchase/repair, avoiding a circular access rule.
            return IsValidState(context.economy) && context.economy.enabled == 1 &&
                           PropertyOperating(context.economy, 11, context.world)
                       ? bit(ForestMountainResidentId::Brakka)
                       : 0;
        default:
            return 0;
    }
}

} // namespace LivingHyrule
