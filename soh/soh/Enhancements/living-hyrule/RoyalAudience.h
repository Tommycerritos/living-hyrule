#pragma once

#include "RoyalAudiencePolicy.h"

struct Actor;

namespace LivingHyrule {

bool IsRoyalResidentActor(const Actor* actor);
RoyalResidentId GetRoyalResidentId(const Actor* actor);
const char* GetRoyalResidentName(RoyalResidentId id);

} // namespace LivingHyrule
