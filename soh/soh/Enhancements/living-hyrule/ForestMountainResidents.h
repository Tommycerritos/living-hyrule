#pragma once

#include "ForestMountainPolicy.h"

struct Actor;

namespace LivingHyrule {

// Add this identity check to the shared conversation transaction whitelist.
bool IsForestMountainResidentActor(const Actor* actor);
ForestMountainResidentId GetForestMountainResidentId(const Actor* actor);
const char* GetForestMountainResidentName(ForestMountainResidentId id);

} // namespace LivingHyrule
