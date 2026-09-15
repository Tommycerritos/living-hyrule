#include "RegionalEncounterPolicy.h"

#include <iostream>
#include <limits>

namespace {
using namespace LivingHyrule;
int checks = 0;
int failures = 0;

#define CHECK(expression)                                              \
    do {                                                               \
        ++checks;                                                      \
        if (!(expression)) {                                           \
            std::cerr << "Line " << __LINE__ << ": " #expression "\n"; \
            ++failures;                                                \
        }                                                              \
    } while (0)

RegionalEncounterContext Ready(EncounterPlace place) {
    RegionalEncounterContext context;
    context.enabled = context.supportedAdventure = context.realSave = context.normalScene = context.gameplayActive =
        true;
    context.adult = place != EncounterPlace::River;
    context.daytime = place == EncounterPlace::Colossus;
    context.fireRestored = context.waterStone = context.spiritRestored = context.gerudoMembership = true;
    context.place = place;
    return context;
}

void TestSceneAndStoryGates() {
    CHECK(GetRegionalEncounterSite(EncounterPlace::Field) == nullptr);
    CHECK(GetRegionalEncounterSite(EncounterPlace::None) == nullptr);
    CHECK(GetRegionalEncounterSite(static_cast<EncounterPlace>(255)) == nullptr);
    CHECK(!RegionalEncounterAllowed(Ready(EncounterPlace::Field)));
    CHECK(!RegionalEncounterAllowed(Ready(static_cast<EncounterPlace>(255))));
    for (const auto& site : kRegionalEncounterSites) {
        const auto ready = Ready(site.place);
        CHECK(RegionalEncounterAllowed(ready));
        for (int gate = 0; gate < 6; ++gate) {
            auto blocked = ready;
            switch (gate) {
                case 0:
                    blocked.enabled = false;
                    break;
                case 1:
                    blocked.supportedAdventure = false;
                    break;
                case 2:
                    blocked.realSave = false;
                    break;
                case 3:
                    blocked.normalScene = false;
                    break;
                case 4:
                    blocked.gameplayActive = false;
                    break;
                case 5:
                    blocked.enemyOverride = true;
                    break;
            }
            CHECK(!RegionalEncounterAllowed(blocked));
        }
        for (unsigned int bits = 0; bits < 64; ++bits) {
            auto context = ready;
            context.adult = (bits & 1) != 0;
            context.daytime = (bits & 2) != 0;
            context.fireRestored = (bits & 4) != 0;
            context.waterStone = (bits & 8) != 0;
            context.spiritRestored = (bits & 16) != 0;
            context.gerudoMembership = (bits & 32) != 0;
            const bool expected =
                site.place == EncounterPlace::Trail ? context.adult && !context.daytime && context.fireRestored
                : site.place == EncounterPlace::River
                    ? !context.adult && !context.daytime && context.waterStone
                    : context.adult && context.daytime && context.spiritRestored && context.gerudoMembership;
            CHECK(RegionalEncounterAllowed(context) == expected);
        }
    }
}

void TestEntryBudget() {
    for (const auto& site : kRegionalEncounterSites) {
        auto context = Ready(site.place);
        RegionalEncounterBudget budget;
        for (int tick = 0; tick < 20; ++tick) {
            CHECK(!ConsumeRegionalEncounterAttempt(budget, context, true, true));
            CHECK(!budget.attempted);
            AdvanceRegionalEncounterGrace(budget);
        }
        CHECK(budget.graceTicks == 0);
        AdvanceRegionalEncounterGrace(budget);
        CHECK(budget.graceTicks == 0); // No underflow and no automatic rearming.
        CHECK(!ConsumeRegionalEncounterAttempt(budget, context, false, true));
        CHECK(!ConsumeRegionalEncounterAttempt(budget, context, true, false));
        context.enabled = false;
        CHECK(!ConsumeRegionalEncounterAttempt(budget, context, true, true));
        context.enabled = true;
        CHECK(ConsumeRegionalEncounterAttempt(budget, context, true, true));
        CHECK(budget.attempted);
        // A failed geometry/allocation attempt has the same spent budget as a
        // defeat. Neither toggling nor elapsed time creates a farming loop.
        for (int repeat = 0; repeat < 10000; ++repeat) {
            context.enabled = (repeat & 1) != 0;
            AdvanceRegionalEncounterGrace(budget);
            CHECK(!ConsumeRegionalEncounterAttempt(budget, context, true, true));
        }
        RegionalEncounterBudget newEntry;
        for (int tick = 0; tick < 20; ++tick)
            AdvanceRegionalEncounterGrace(newEntry);
        context.enabled = true;
        CHECK(ConsumeRegionalEncounterAttempt(newEntry, context, true, true));
        CHECK(budget.attempted); // Separate visits have separate transient state.
    }
}

void TestPocketAndNativeWindow() {
    for (const auto& site : kRegionalEncounterSites) {
        CHECK(InsideRegionalEncounterPocket(site, site.x, site.y, site.z));
        CHECK(InsideRegionalEncounterPocket(site, site.x + site.leashRadius, site.y, site.z));
        CHECK(!InsideRegionalEncounterPocket(site, site.x + site.leashRadius + 1, site.y, site.z));
        CHECK(!InsideRegionalEncounterPocket(site, site.x, site.y + 121, site.z));
        CHECK(!RegionalEncounterPlayerInRange(site, site.x + 219, site.y, site.z));
        CHECK(RegionalEncounterPlayerInRange(site, site.x + 220, site.y, site.z));
        CHECK(RegionalEncounterPlayerInRange(site, site.x + 600, site.y + 80, site.z));
        CHECK(!RegionalEncounterPlayerInRange(site, site.x + 601, site.y, site.z));
        CHECK(!RegionalEncounterPlayerInRange(site, site.x + 300, site.y + 81, site.z));
        for (float invalid : { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                               -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max() }) {
            CHECK(!InsideRegionalEncounterPocket(site, invalid, site.y, site.z));
            CHECK(!InsideRegionalEncounterPocket(site, site.x, invalid, site.z));
            CHECK(!InsideRegionalEncounterPocket(site, site.x, site.y, invalid));
            CHECK(!RegionalEncounterPlayerInRange(site, invalid, site.y, site.z));
            CHECK(!RegionalEncounterPlayerInRange(site, site.x, invalid, site.z));
            CHECK(!RegionalEncounterPlayerInRange(site, site.x, site.y, invalid));
        }
    }
    const auto* desert = GetRegionalEncounterSite(EncounterPlace::Colossus);
    CHECK(desert->z - desert->leashRadius >= 350); // Original temple approach remains outside the leash.
    CHECK(desert->lifetime + 20 < 200);            // Native spawner cooldown outlasts the added Leever.
    CHECK(!RegionalSpawnerQuiet(0, false, 199, true));
    CHECK(RegionalSpawnerQuiet(0, false, 200, true));
    CHECK(!RegionalSpawnerQuiet(0, false, 19, false));
    CHECK(RegionalSpawnerQuiet(0, false, 20, false));
    CHECK(!RegionalSpawnerQuiet(1, false, 600, true));
    CHECK(!RegionalSpawnerQuiet(-1, false, 600, true));
    CHECK(!RegionalSpawnerQuiet(0, true, 600, true));
    CHECK(!RegionalSpawnerQuiet(0, false, -1, true));
    CHECK(CanDismissRegionalEnemy(false, false));
    CHECK(!CanDismissRegionalEnemy(true, false));
    CHECK(!CanDismissRegionalEnemy(false, true));
    CHECK(!CanDismissRegionalEnemy(true, true));
}

void TestLeeverTerminalDeath() {
    for (bool nativeStunNotification : { false, true }) {
        RegionalLeeverDeath state;
        int notifications = nativeStunNotification ? 1 : 0;
        state.defeatNotified = nativeStunNotification;
        for (int frame = 0; frame < 50; ++frame) {
            CHECK(FinishRegionalLeeverDeath(state, false) == RegionalLeeverCompletion::None);
            CHECK(!state.finished && state.defeatNotified == nativeStunNotification);
        }
        // The native actor has emitted its first terminal drop. The hook must
        // remove it now, reporting defeat only if the stun path did not already.
        const auto result = FinishRegionalLeeverDeath(state, true);
        CHECK(result ==
              (nativeStunNotification ? RegionalLeeverCompletion::Kill : RegionalLeeverCompletion::KillAndNotify));
        if (result == RegionalLeeverCompletion::KillAndNotify)
            ++notifications;
        CHECK(notifications == 1 && state.finished && state.defeatNotified);
        for (int repeat = 0; repeat < 1000; ++repeat)
            CHECK(FinishRegionalLeeverDeath(state, true) == RegionalLeeverCompletion::None);
        CHECK(FinishRegionalLeeverDeath(state, false) == RegionalLeeverCompletion::None);
        RegionalLeeverDeath newVisit;
        CHECK(FinishRegionalLeeverDeath(newVisit, true) == RegionalLeeverCompletion::KillAndNotify);
    }
}
} // namespace

int main() {
    TestSceneAndStoryGates();
    TestEntryBudget();
    TestPocketAndNativeWindow();
    TestLeeverTerminalDeath();
    if (failures != 0) {
        std::cerr << failures << " of " << checks << " regional encounter checks failed\n";
        return 1;
    }
    std::cout << checks << " regional encounter gates, budgets, bounds and native-death checks passed\n";
    return 0;
}
