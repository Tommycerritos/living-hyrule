#pragma once

#include "RegionalWardrobePolicy.h"

namespace LivingHyrule {

// Save integration supplies this accessor when it embeds WardrobeState in its
// copied save POD. Return nullptr when no usable wardrobe save is available.
// The renderer reads it afresh and never retains the returned pointer.
WardrobeState* GetRegionalWardrobeStateForSave();

WardrobeVisualStatus GetRegionalWardrobeVisualStatus();
const char* RegionalWardrobeVisualStatusText(WardrobeVisualStatus status);
void RegisterRegionalWardrobe();

} // namespace LivingHyrule
