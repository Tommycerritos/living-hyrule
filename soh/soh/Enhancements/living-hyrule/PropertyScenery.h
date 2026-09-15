#pragma once

struct Actor;

namespace LivingHyrule {

// Decorative business supplies, registered automatically through ShipInit.
// These have no collider, collectible drops, building changes, or saved state.
bool IsPropertySceneryActor(const Actor* actor);

} // namespace LivingHyrule
