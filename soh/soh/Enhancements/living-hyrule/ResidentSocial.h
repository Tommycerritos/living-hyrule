#pragma once

#include "SocialTypes.h"
#include <string>

struct Actor;

namespace LivingHyrule {

ResidentId GetSocialResidentId(const Actor* actor);
// Called only as an owned resident textbox opens. Meeting adds no rapport points.
std::string ResidentGreeting(Actor* actor);

} // namespace LivingHyrule
