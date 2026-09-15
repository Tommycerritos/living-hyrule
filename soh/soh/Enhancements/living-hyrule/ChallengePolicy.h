#pragma once

#include <cstdint>

namespace LivingHyrule {

struct ChallengeContext {
    bool enabled = false;
    bool supportedAdventure = false;
    bool realSave = false;
    bool normalScene = false;
    bool gameplayActive = false;
};

constexpr bool IsChallengeActive(const ChallengeContext& context) {
    return context.enabled && context.supportedAdventure && context.realSave && context.normalScene &&
           context.gameplayActive;
}

struct ChallengeDamageConflicts {
    bool damageMultiplier = false;
    bool defenseModifier = false;
    bool oneHitKO = false;
    bool infiniteHealth = false;
    bool permanentHeartLoss = false;
};

constexpr bool HasChallengeDamageConflict(const ChallengeDamageConflicts& conflicts) {
    return conflicts.damageMultiplier || conflicts.defenseModifier || conflicts.oneHitKO || conflicts.infiniteHealth ||
           conflicts.permanentHeartLoss;
}

// Player_Update moves either signed timer one step toward zero before consuming
// pending collision damage. +/-1 therefore expires on this damage frame.
constexpr bool IsChallengeInvulnerableOnDamageFrame(int16_t timer) {
    return timer < -1 || timer > 1;
}

constexpr bool ShouldBoostChallengeDamage(const ChallengeContext& context, const ChallengeDamageConflicts& conflicts,
                                          bool enemyCollision, bool invulnerable, bool shieldBlocked,
                                          bool scriptedKnockback) {
    return IsChallengeActive(context) && !HasChallengeDamageConflict(conflicts) && enemyCollision && !invulnerable &&
           !shieldBlocked && !scriptedKnockback;
}

// Collision damage is an unsigned byte. Widen before multiplying so strong
// attacks saturate instead of wrapping into weaker attacks or zero damage.
constexpr uint8_t ScaleChallengeCollisionDamage(uint8_t damage, bool boost) {
    if (!boost) {
        return damage;
    }
    const uint32_t scaled = static_cast<uint32_t>(damage) * 2u;
    return scaled > 255 ? uint8_t{ 255 } : static_cast<uint8_t>(scaled);
}

struct ChallengeHeartDrop {
    bool recoveryHeart = false;
    bool temporary = false;
    bool unflagged = false;
    bool directAward = false;
    bool alreadyCollected = false;
};

constexpr bool IsChallengeHeartCandidate(const ChallengeHeartDrop& drop) {
    return drop.recoveryHeart && drop.temporary && drop.unflagged && !drop.directAward && !drop.alreadyCollected;
}

// Ordinals begin at zero for each scene. Keep the first ordinary loose heart,
// suppress the next, and repeat without consuming the game's random numbers.
// Keep emergency hearts at one heart (16 health units) or less.
constexpr bool ShouldSuppressChallengeHeart(const ChallengeContext& context, bool existingDropOverride,
                                            const ChallengeHeartDrop& drop, int16_t health, uint32_t ordinal) {
    return IsChallengeActive(context) && !existingDropOverride && IsChallengeHeartCandidate(drop) && health > 16 &&
           (ordinal & 1u) != 0;
}

} // namespace LivingHyrule
