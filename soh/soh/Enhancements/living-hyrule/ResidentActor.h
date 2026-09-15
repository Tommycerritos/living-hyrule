#pragma once

#include "z64actor.h"

#include <cstdint>

namespace LivingHyrule {

enum class ResidentRole : uint8_t {
    Carpenter,
    Tenant,
    Supplier,
    Count,
};

// Registration is idempotent and requires ActorDB and GameInteractor to exist.
int RegisterResidentActor();

// Returns nullptr for an unavailable role, invalid position, or failed spawn.
// The actor's params stores its validated ResidentRole value.
Actor* SpawnResident(PlayState* play, ResidentRole role, const Vec3f& position, s16 yaw);
bool IsResidentActor(const Actor* actor);
ResidentRole GetResidentRole(const Actor* actor);

} // namespace LivingHyrule
