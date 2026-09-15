#include "ForestMountainPolicy.h"
#include "TradePolicy.h"

#include <iostream>

namespace {
using namespace LivingHyrule;
int checks = 0;
int failures = 0;

void Check(bool condition, const char* description) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

ForestMountainContext Normal(ForestMountainPlace place, bool daytime = true) {
    ForestMountainContext context;
    context.enabled = context.supportedAdventure = context.normalScene = true;
    context.daytime = daytime;
    context.place = place;
    return context;
}

bool Present(const ForestMountainContext& context, ForestMountainResidentId id) {
    return (ForestMountainResidentMaskFor(context) & ForestMountainResidentBit(id)) != 0;
}

void GatesAndIdentities() {
    for (unsigned int place = 0; place <= static_cast<unsigned int>(ForestMountainPlace::Count); ++place) {
        for (unsigned int scenario = 0; scenario < 8; ++scenario) {
            for (unsigned int gates = 0; gates < 8; ++gates) {
                auto context = Normal(static_cast<ForestMountainPlace>(place), (scenario & 1) != 0);
                context.world.adult = (scenario & 2) != 0;
                context.world.forest = context.world.fire = (scenario & 4) != 0;
                context.enabled = (gates & 1) != 0;
                context.supportedAdventure = (gates & 2) != 0;
                context.normalScene = (gates & 4) != 0;
                const auto mask = ForestMountainResidentMaskFor(context);
                Check((mask & ~0x0fu) == 0, "schedule only contains the four declared residents");
                if (gates != 7)
                    Check(mask == 0, "each required runtime gate independently suppresses residents");
            }
        }
    }
    for (auto place : { ForestMountainPlace::None, ForestMountainPlace::Count, static_cast<ForestMountainPlace>(255) })
        Check(ForestMountainResidentMaskFor(Normal(place)) == 0, "other locations stay unpopulated");
    for (auto id : { ForestMountainResidentId::Count, static_cast<ForestMountainResidentId>(255) }) {
        Check(ForestMountainResidentBit(id) == 0, "invalid identity has no population bit");
        Check(ForestMountainPropertyId(id) == -1, "invalid identity cannot offer a deed");
    }
    Check(ForestMountainPropertyId(ForestMountainResidentId::Fenn) == 6, "Fenn owns the seed-garden offer");
    Check(ForestMountainPropertyId(ForestMountainResidentId::Luma) == 7, "Luma owns the woodland-workshop offer");
    Check(ForestMountainPropertyId(ForestMountainResidentId::Doron) == 10, "Doron owns the stoneworks offer");
    Check(ForestMountainPropertyId(ForestMountainResidentId::Brakka) == 11, "Brakka owns the kiln offer");
}

void RegionalRecovery() {
    auto forest = Normal(ForestMountainPlace::KokiriForest);
    auto mountain = Normal(ForestMountainPlace::GoronCity);
    Check(Present(forest, ForestMountainResidentId::Fenn) && Present(forest, ForestMountainResidentId::Luma),
          "child forest residents do not require an enabled bank");
    Check(Present(mountain, ForestMountainResidentId::Doron) && Present(mountain, ForestMountainResidentId::Brakka),
          "child mountain workers are present through the food shortage");
    Check(!Present(forest, ForestMountainResidentId::Doron) && !Present(mountain, ForestMountainResidentId::Fenn),
          "regional model families stay in their own settlements");
    forest.world.adult = mountain.world.adult = true;
    Check(ForestMountainResidentMaskFor(forest) == 0, "Kokiri shelter through the adult monster crisis");
    Check(ForestMountainResidentMaskFor(mountain) == 0, "Goron captivity leaves worksites empty");
    forest.world.fire = forest.world.water = true;
    mountain.world.forest = mountain.world.shadow = true;
    Check(ForestMountainResidentMaskFor(forest) == 0, "unrelated temples do not reopen forest work");
    Check(ForestMountainResidentMaskFor(mountain) == 0, "unrelated temples do not free Goron workers");
    forest.world.forest = mountain.world.fire = true;
    Check(Present(forest, ForestMountainResidentId::Luma), "Forest recovery returns Kokiri work");
    Check(Present(mountain, ForestMountainResidentId::Doron), "Fire recovery returns Goron work");
    forest.daytime = false;
    Check(ForestMountainResidentMaskFor(forest) == 0, "Kokiri stay sheltered at night after recovery too");
}

void EveningKilnAndTradeAccess() {
    auto context = Normal(ForestMountainPlace::GoronCity, false);
    Check(ForestMountainResidentMaskFor(context) == 0, "an unfunded kiln has no evening shift");
    context.economy.enabled = 1;
    context.economy.ownedProperties = 1u << 11;
    Check(ForestMountainResidentMaskFor(context) == ForestMountainResidentBit(ForestMountainResidentId::Brakka),
          "child operating kiln funds just its evening tender");
    context.world.adult = true;
    context.economy.repairedProperties = 1u << 11;
    Check(ForestMountainResidentMaskFor(context) == 0, "paid repairs cannot bypass the adult captivity gate");
    context.world.fire = true;
    context.economy.repairedProperties = 0;
    Check(ForestMountainResidentMaskFor(context) == 0, "damaged adult kiln has no evening shift");
    context.economy.repairedProperties = 1u << 11;
    Check(Present(context, ForestMountainResidentId::Brakka), "repair restores the evening shift");
    context.economy.enabled = 0;
    Check(ForestMountainResidentMaskFor(context) == 0, "paused account pauses the funded shift");
    context.economy.enabled = 2;
    Check(ForestMountainResidentMaskFor(context) == 0, "unreadable account cannot fund an evening shift");
    context.daytime = true;
    Check(Present(context, ForestMountainResidentId::Brakka), "ordinary daytime presence survives unreadable economy");

    for (auto id : { ForestMountainResidentId::Fenn, ForestMountainResidentId::Luma, ForestMountainResidentId::Doron,
                     ForestMountainResidentId::Brakka }) {
        const int property = ForestMountainPropertyId(id);
        auto day = Normal(property < 10 ? ForestMountainPlace::KokiriForest : ForestMountainPlace::GoronCity);
        day.world.adult = day.world.forest = day.world.fire = true;
        day.economy.enabled = 1;
        Check(Present(day, id), "seller is present before investment");
        Check(PropertyOffer(day.economy, property, day.world).kind == TradeKind::Property,
              "recovered resident offers an unowned property");
        day.economy.ownedProperties = 1u << property;
        Check(Present(day, id), "seller stays present before adult repairs");
        Check(PropertyOffer(day.economy, property, day.world).kind == TradeKind::Repair,
              "same resident offers repairs for a damaged adult business");
    }
}
} // namespace

int main() {
    GatesAndIdentities();
    RegionalRecovery();
    EveningKilnAndTradeAccess();
    std::cout << "Forest/mountain population: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
