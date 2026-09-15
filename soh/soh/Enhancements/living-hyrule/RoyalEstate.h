#pragma once

#include <string>

namespace LivingHyrule {

enum class RoyalEstateReadiness {
    Ready,
    NoLoadedGame,
    UnsupportedAdventure,
    WrongPlace,
    AwaitingVictory,
    EconomyUnavailable,
    MarketNotRestored,
    ResourcesUnavailable,
    UnsafeEntrance,
};

// Read-only preparation also succeeds inside an eligible, safely loaded garden.
// Call immediately before any separately implemented estate purchase.
RoyalEstateReadiness GetRoyalEstateReadiness();
const char* RoyalEstateReadinessText(RoyalEstateReadiness readiness);

// The safe garden/return route stays latched for the visit even if its economy
// or residents are disabled. No scene resource or global entrance is modified.
// An unprepared remembered visit is an empty escape shell: readiness is not Ready.
bool IsRoyalEstateActive();
// Shared by immediate controls and invitations deferred until dialogue ends.
bool IsRoyalEstateTravelSafe();
std::string EnterRoyalEstate();
std::string ReturnFromRoyalEstate();

} // namespace LivingHyrule
