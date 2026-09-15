#include "ChallengePolicy.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace {

using namespace LivingHyrule;
int failures = 0;

void Check(bool condition, const char* expression, int line) {
    if (!condition) {
        std::cerr << "Line " << line << ": " << expression << '\n';
        ++failures;
    }
}

#define CHECK(expression) Check((expression), #expression, __LINE__)

constexpr ChallengeContext kActive = { true, true, true, true, true };
constexpr ChallengeHeartDrop kLooseHeart = { true, true, true, false, false };

static_assert(!IsChallengeActive({}));
static_assert(ScaleChallengeCollisionDamage(4, true) == 8);
static_assert(ScaleChallengeCollisionDamage(128, true) == 255);
static_assert(!ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 16, 1));

void TestContextGates() {
    for (unsigned int gates = 0; gates < 32; ++gates) {
        const ChallengeContext context = { (gates & 1u) != 0, (gates & 2u) != 0, (gates & 4u) != 0,
                                           (gates & 8u) != 0, (gates & 16u) != 0 };
        const bool expected = gates == 31;
        CHECK(IsChallengeActive(context) == expected);
        CHECK(ShouldBoostChallengeDamage(context, {}, true, false, false, false) == expected);
        CHECK(ShouldSuppressChallengeHeart(context, false, kLooseHeart, 48, 1) == expected);
    }
}

void TestDifficultyAndCheatPriority() {
    for (unsigned int flags = 0; flags < 32; ++flags) {
        const ChallengeDamageConflicts conflicts = { (flags & 1u) != 0, (flags & 2u) != 0, (flags & 4u) != 0,
                                                     (flags & 8u) != 0, (flags & 16u) != 0 };
        CHECK(HasChallengeDamageConflict(conflicts) == (flags != 0));
        CHECK(ShouldBoostChallengeDamage(kActive, conflicts, true, false, false, false) == (flags == 0));
    }
    // Enemy health, healing, falls, scripted knockback and blocked/invulnerable
    // hits cannot be scaled by this policy even if every mode gate is open.
    for (unsigned int flags = 0; flags < 16; ++flags) {
        const bool enemy = (flags & 1u) != 0;
        const bool invulnerable = (flags & 2u) != 0;
        const bool blocked = (flags & 4u) != 0;
        const bool scripted = (flags & 8u) != 0;
        CHECK(ShouldBoostChallengeDamage(kActive, {}, enemy, invulnerable, blocked, scripted) == (flags == 1));
    }
}

void TestDamageArithmetic() {
    CHECK(!IsChallengeInvulnerableOnDamageFrame(-1));
    CHECK(!IsChallengeInvulnerableOnDamageFrame(0));
    CHECK(!IsChallengeInvulnerableOnDamageFrame(1));
    CHECK(IsChallengeInvulnerableOnDamageFrame(-2));
    CHECK(IsChallengeInvulnerableOnDamageFrame(2));
    CHECK(IsChallengeInvulnerableOnDamageFrame(INT16_MIN));
    CHECK(IsChallengeInvulnerableOnDamageFrame(INT16_MAX));
    uint8_t previous = 0;
    for (unsigned int damage = 0; damage <= 255; ++damage) {
        const auto original = static_cast<uint8_t>(damage);
        const auto scaled = ScaleChallengeCollisionDamage(original, true);
        CHECK(ScaleChallengeCollisionDamage(original, false) == original);
        CHECK(scaled == (damage <= 127 ? damage * 2 : 255));
        CHECK(scaled >= original);
        CHECK(scaled >= previous);
        previous = scaled;
    }
    CHECK(ScaleChallengeCollisionDamage(0, true) == 0);
    CHECK(ScaleChallengeCollisionDamage(127, true) == 254);
    CHECK(ScaleChallengeCollisionDamage(255, true) == 255);
}

void TestRewardPreservation() {
    for (unsigned int flags = 0; flags < 32; ++flags) {
        const ChallengeHeartDrop drop = { (flags & 1u) != 0, (flags & 2u) != 0, (flags & 4u) != 0,
                                          (flags & 8u) != 0, (flags & 16u) != 0 };
        // Only an ordinary temporary, unflagged, uncollected recovery heart can
        // be removed; all other pickups/rewards fail at least one of these gates.
        CHECK(IsChallengeHeartCandidate(drop) == (flags == 7));
        CHECK(ShouldSuppressChallengeHeart(kActive, false, drop, 48, 1) == (flags == 7));
        CHECK(!ShouldSuppressChallengeHeart(kActive, true, drop, 48, 1));
    }
    const ChallengeHeartDrop placedHeart = { true, false, true, false, false };
    const ChallengeHeartDrop flaggedReward = { true, true, false, false, false };
    const ChallengeHeartDrop heartContainer = { false, true, true, false, false };
    CHECK(!ShouldSuppressChallengeHeart(kActive, false, placedHeart, 320, 1));
    CHECK(!ShouldSuppressChallengeHeart(kActive, false, flaggedReward, 320, 1));
    CHECK(!ShouldSuppressChallengeHeart(kActive, false, heartContainer, 320, 1));
}

void TestScarcityAndEmergencyRecovery() {
    unsigned int removed = 0;
    for (uint32_t ordinal = 0; ordinal < 10000; ++ordinal) {
        removed += ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 48, ordinal) ? 1u : 0u;
        CHECK(!ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 16, ordinal));
        CHECK(!ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 1, ordinal));
        CHECK(!ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 0, ordinal));
        CHECK(!ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, -1, ordinal));
    }
    CHECK(removed == 5000);
    CHECK(!ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 48, 0));
    CHECK(ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 17, 1));
    CHECK(ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 320, 1));
    const uint32_t lastOrdinal = (std::numeric_limits<uint32_t>::max)();
    CHECK(ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 48, lastOrdinal));
    CHECK(!ShouldSuppressChallengeHeart(kActive, false, kLooseHeart, 48, lastOrdinal + uint32_t{ 1 }));
}

} // namespace

int main() {
    TestContextGates();
    TestDifficultyAndCheatPriority();
    TestDamageArithmetic();
    TestRewardPreservation();
    TestScarcityAndEmergencyRecovery();
    if (failures != 0) {
        std::cerr << failures << " challenge policy checks failed.\n";
        return 1;
    }
    std::cout << "Living Hyrule challenge: all arithmetic, gate, reward and recovery policy checks passed.\n";
    return 0;
}
