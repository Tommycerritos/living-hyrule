#pragma once

#include "Properties.h"
#include <cstdint>

namespace LivingHyrule {

enum class PropertySceneryStage : uint8_t { Hidden, AwaitingRepairs, Operating };
inline constexpr uint32_t kPropertyScenerySceneLimit = 3;

inline PropertySceneryStage PropertySceneryStageFor(const EconomyState& economy, uint32_t propertyId,
                                                    const WorldProgress& world, bool supportedScene,
                                                    bool traderActive) {
    if (!supportedScene || !traderActive || !IsValidState(economy) || economy.enabled != 1 ||
        propertyId >= kProperties.size() || !OwnsProperty(economy, propertyId) ||
        !RegionOpen(kProperties[propertyId].region, world)) {
        return PropertySceneryStage::Hidden;
    }
    return PropertyOperating(economy, propertyId, world) ? PropertySceneryStage::Operating
                                                         : PropertySceneryStage::AwaitingRepairs;
}

constexpr bool CanAddPropertyScenery(uint32_t presentProperties, uint32_t arrangementCount, uint32_t propertyId) {
    return propertyId < 16 && (presentProperties & ~0xffffu) == 0 && arrangementCount < kPropertyScenerySceneLimit &&
           (presentProperties & (1u << propertyId)) == 0;
}

} // namespace LivingHyrule
