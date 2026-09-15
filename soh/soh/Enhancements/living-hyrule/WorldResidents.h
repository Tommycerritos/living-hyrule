#pragma once

#include "WorldPopulationPolicy.h"

struct Actor;

namespace LivingHyrule {

bool IsWorldResidentActor(const Actor* actor);
WorldResidentId GetWorldResidentId(const Actor* actor);
const char* GetWorldResidentName(WorldResidentId id);
// -1 means this resident has no property transaction to offer.
int GetWorldResidentPropertyId(WorldResidentId id);

} // namespace LivingHyrule
