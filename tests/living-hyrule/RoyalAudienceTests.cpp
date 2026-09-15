#include "RoyalAudiencePolicy.h"
#include <cstdint>
#include <iostream>

namespace {
using namespace LivingHyrule;
int checks = 0;
int failures = 0;
void Check(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
RoyalAudienceContext Audience(bool daytime = true) {
    RoyalAudienceContext context;
    context.enabled = context.supportedAdventure = context.normalScene = context.castleApproach = true;
    context.world.adult = context.world.ganonDefeated = true;
    context.economy.enabled = 1;
    context.daytime = daytime;
    return context;
}

void GateCombinations() {
    // Independently vary every eligibility gate. Successful night access is
    // deliberately useful: the guard explains the daytime audience schedule.
    for (unsigned int flags = 0; flags < 256; ++flags) {
        auto context = Audience();
        context.enabled = (flags & 1) != 0;
        context.supportedAdventure = (flags & 2) != 0;
        context.normalScene = (flags & 4) != 0;
        context.castleApproach = (flags & 8) != 0;
        context.world.adult = (flags & 16) != 0;
        context.world.ganonDefeated = (flags & 32) != 0;
        context.economy.enabled = (flags & 64) != 0;
        context.daytime = (flags & 128) != 0;
        const uint8_t expected = (flags & 127) != 127 ? 0 : context.daytime ? 7 : 2;
        Check(RoyalResidentMaskFor(context) == expected, "audience requires every independent postgame gate");
    }
    Check(RoyalResidentBit(RoyalResidentId::Count) == 0, "sentinel has no actor bit");
    Check(RoyalResidentBit(static_cast<RoyalResidentId>(255)) == 0, "unknown identity fails closed");
}

void StoryAndSchedules() {
    auto context = Audience();
    context.world.ganonDefeated = false;
    context.world.forest = context.world.fire = context.world.water = context.world.shadow = context.world.spirit =
        true;
    context.world.gerudoMembership = context.world.ranchFreed = true;
    context.economy.ownsKakarikoCottage = 1;
    context.economy.ownedProperties = context.economy.repairedProperties = 0xffffu;
    context.economy.bankRupees = kBankLimit;
    Check(RoyalResidentMaskFor(context) == 0, "all deeds and story rewards cannot substitute for Ganon victory");
    context.world.ganonDefeated = true;
    Check(RoyalResidentMaskFor(context) == 7, "recorded victory opens daytime audience without castle purchase");
    context.economy = {};
    context.economy.enabled = 1;
    Check(RoyalResidentMaskFor(context) == 7, "poor and uninvested postgame visitors may meet Zelda");
    context.daytime = false;
    Check(RoyalResidentMaskFor(context) == RoyalResidentBit(RoyalResidentId::Aren),
          "only the captain keeps night watch");
    context.daytime = true;
    Check(RoyalResidentMaskFor(context) == 7, "daybreak restores both civilian audience members");
    context.world.adult = false;
    Check(RoyalResidentMaskFor(context) == 0, "postgame evidence never inserts adult Zelda into childhood scenes");
    context.world.adult = true;
    context.castleApproach = false;
    Check(RoyalResidentMaskFor(context) == 0, "courtyard and ending maps do not acquire the audience");
}

void LedgerSafety() {
    for (const bool daytime : { false, true }) {
        auto context = Audience(daytime);
        context.economy.enabled = 0;
        Check(RoyalResidentMaskFor(context) == 0, "paused economy also pauses the optional postgame audience");
        context.economy.enabled = 2;
        Check(RoyalResidentMaskFor(context) == 0, "malformed enable value fails closed");
        context.economy.enabled = 1;
        context.economy.bankRupees = kBankLimit + 1;
        Check(RoyalResidentMaskFor(context) == 0, "invalid balance suppresses the audience");
        context.economy.bankRupees = 0;
        context.economy.repairedProperties = 1;
        Check(RoyalResidentMaskFor(context) == 0, "orphan repairs suppress the audience");
        context.economy.repairedProperties = 0;
        context.economy.businessFrames[0] = 1;
        Check(RoyalResidentMaskFor(context) == 0, "invalid business progress suppresses the audience");
        context.economy.businessFrames[0] = 0;
        context.economy.rentalFrames = kFramesPerRentPeriod;
        Check(RoyalResidentMaskFor(context) == 0, "invalid cottage progress suppresses the audience");
        context.economy.rentalFrames = 0;
        Check(RoyalResidentMaskFor(context) == (daytime ? 7 : 2), "valid retained state restores the schedule");
    }
}
} // namespace

int main() {
    GateCombinations();
    StoryAndSchedules();
    LedgerSafety();
    if (failures != 0) {
        std::cerr << failures << " of " << checks << " royal audience checks failed.\n";
        return 1;
    }
    std::cout << "Living Hyrule royal audience: all " << checks << " checks passed.\n";
    return 0;
}
