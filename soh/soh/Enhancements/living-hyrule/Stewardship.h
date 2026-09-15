#pragma once

#include "LivingHyrule.h"

namespace LivingHyrule {

// Call inside the Living Hyrule ledger. Each clicked action validates the live
// save again and refreshes status before any following control is drawn.
void DrawStewardshipControls(Status& status);

// Read-only recognition for the current owned resident conversation. Returns an
// optional new dialogue page; it does not award rapport, items or story flags.
std::string StewardshipGreeting(ResidentId resident);

} // namespace LivingHyrule
