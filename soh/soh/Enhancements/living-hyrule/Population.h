#pragma once

#include "ResidentActor.h"

namespace LivingHyrule {

// Includes user preference, adventure type, location, day/night, and story phase.
// An actor already in conversation should finish talking before leaving.
bool ShouldResidentBePresent(const PlayState* play, ResidentRole role);

} // namespace LivingHyrule
