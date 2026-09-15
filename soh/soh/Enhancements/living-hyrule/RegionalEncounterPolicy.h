#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace LivingHyrule {

enum class EncounterPlace : uint8_t { None, Field, Trail, River, Colossus };
enum class RegionalEnemyKind : uint8_t { RedTektite, BlueTektite, SmallLeever };

struct RegionalEncounterSite {
    EncounterPlace place;
    RegionalEnemyKind enemy;
    float x, y, z, floorY;
    float leashRadius;
    uint16_t lifetime;
};

// Verified offsets, not original actor positions. Field is intentionally absent:
// its ordinary object lists do not load the native Wolfos object.
inline constexpr std::array<RegionalEncounterSite, 3> kRegionalEncounterSites = { {
    { EncounterPlace::Trail, RegionalEnemyKind::RedTektite, -1600, 1048, 1300, 1048, 110, 600 },
    { EncounterPlace::River, RegionalEnemyKind::BlueTektite, 1300, 180, -500, 140, 90, 600 },
    { EncounterPlace::Colossus, RegionalEnemyKind::SmallLeever, -500, -31, 600, -31, 180, 160 },
} };

struct RegionalEncounterContext {
    bool enabled = false;
    bool supportedAdventure = false;
    bool realSave = false;
    bool normalScene = false;
    bool gameplayActive = false;
    bool enemyOverride = false;
    bool adult = false;
    bool daytime = false;
    bool fireRestored = false;
    bool waterStone = false;
    bool spiritRestored = false;
    bool gerudoMembership = false;
    EncounterPlace place = EncounterPlace::None;
};

struct RegionalEncounterBudget {
    bool attempted = false;
    uint16_t graceTicks = 20;
};

struct RegionalLeeverDeath {
    bool defeatNotified = false;
    bool finished = false;
};

enum class RegionalLeeverCompletion : uint8_t { None, Kill, KillAndNotify };

inline RegionalLeeverCompletion FinishRegionalLeeverDeath(RegionalLeeverDeath& state, bool nativeDropFinished) {
    if (!nativeDropFinished || state.finished)
        return RegionalLeeverCompletion::None;
    state.finished = true;
    if (state.defeatNotified)
        return RegionalLeeverCompletion::Kill;
    state.defeatNotified = true;
    return RegionalLeeverCompletion::KillAndNotify;
}

constexpr const RegionalEncounterSite* GetRegionalEncounterSite(EncounterPlace place) {
    for (const auto& site : kRegionalEncounterSites) {
        if (site.place == place)
            return &site;
    }
    return nullptr;
}

inline bool RegionalEncounterAllowed(const RegionalEncounterContext& context) {
    if (!context.enabled || !context.supportedAdventure || !context.realSave || !context.normalScene ||
        !context.gameplayActive || context.enemyOverride)
        return false;
    switch (context.place) {
        case EncounterPlace::Trail:
            return context.adult && context.fireRestored && !context.daytime;
        case EncounterPlace::River:
            // Only the child header has OBJECT_TITE; adult River is excluded.
            return !context.adult && context.waterStone && !context.daytime;
        case EncounterPlace::Colossus:
            return context.adult && context.spiritRestored && context.gerudoMembership && context.daytime;
        default:
            return false;
    }
}

inline bool InsideRegionalEncounterPocket(const RegionalEncounterSite& site, float x, float y, float z) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        return false;
    const float dx = x - site.x, dz = z - site.z;
    return dx * dx + dz * dz <= site.leashRadius * site.leashRadius && std::abs(y - site.y) <= 120;
}

inline bool RegionalEncounterPlayerInRange(const RegionalEncounterSite& site, float x, float y, float z) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        return false;
    const float dx = x - site.x, dz = z - site.z;
    const float distanceSquared = dx * dx + dz * dz;
    return distanceSquared >= 220 * 220 && distanceSquared <= 600 * 600 && std::abs(y - site.y) <= 80;
}

// Do not compete with Colossus's global, player-relative vanilla spawner. A
// native resting interval must outlast the extra Leever's entire bounded life.
inline bool RegionalSpawnerQuiet(int nativeChildren, bool nativeBigLeever, int cooldown, bool starting) {
    return nativeChildren == 0 && !nativeBigLeever && cooldown >= (starting ? 200 : 20);
}

inline void AdvanceRegionalEncounterGrace(RegionalEncounterBudget& budget) {
    if (budget.graceTicks != 0)
        --budget.graceTicks;
}

inline bool ConsumeRegionalEncounterAttempt(RegionalEncounterBudget& budget, const RegionalEncounterContext& context,
                                            bool playerInRange, bool nativeWindow) {
    if (budget.attempted || budget.graceTicks != 0 || !RegionalEncounterAllowed(context) || !playerInRange ||
        !nativeWindow)
        return false;
    // Consume before resource, geometry, actor-clearance and allocation checks.
    // Failed attempts, defeats and toggles never issue another enemy this entry.
    budget.attempted = true;
    return true;
}

inline bool CanDismissRegionalEnemy(bool dying, bool pendingBodyBreak) {
    // Native Tektite breakup owns temporary arrays and earns the normal drop.
    // Let death finish even when a preference, timer or leash would dismiss it.
    return !dying && !pendingBodyBreak;
}

} // namespace LivingHyrule
