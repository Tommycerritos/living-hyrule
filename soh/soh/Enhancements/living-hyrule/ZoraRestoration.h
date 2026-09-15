#pragma once

namespace LivingHyrule {

enum class ZoraRestorationReadiness {
    Ready,
    NoLoadedGame,
    UnsupportedAdventure,
    WrongAgeOrLayer,
    AwaitingWaterRecovery,
    EconomyUnavailable,
    ResourcesUnavailable,
    UnsafeEntrance,
};

// The save/action bridge owns this query. This geometry module never edits a
// save, charges money, or changes a quest flag.
bool HasFundedZoraRestoration();

// May be called outside the Domain (including Lethra's River conversation).
// Prepares and retains compatible native assets before a payment is accepted.
// Recheck immediately before payment. Preparation failure lasts until restart.
ZoraRestorationReadiness GetZoraRestorationReadiness();
const char* ZoraRestorationReadinessText(ZoraRestorationReadiness readiness);

// Collision, ordinary ice and water animation are latched together on entry.
// Changes to the account or investment take effect on the next Domain visit.
bool IsZoraRestorationActive();

} // namespace LivingHyrule
