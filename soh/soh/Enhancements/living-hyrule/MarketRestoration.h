#pragma once

namespace LivingHyrule {

enum class MarketRestorationReadiness {
    Ready,
    NoLoadedGame,
    UnsupportedAdventure,
    WrongPlace,
    AwaitingVictory,
    EconomyUnavailable,
    ThreeDimensionalBackgrounds,
    ResourcesUnavailable,
    UnsafeEntrance,
};

// Read-only. Validates and retains native assets once per process. Call again
// immediately before committing the payment; this function never changes money.
MarketRestorationReadiness GetMarketRestorationReadiness();
const char* MarketRestorationReadinessText(MarketRestorationReadiness readiness);

// The loaded scene keeps its geometry until it exits, including if the option,
// saved investment, or time changes while Link is standing in the square.
bool IsMarketRestorationActive();

} // namespace LivingHyrule
