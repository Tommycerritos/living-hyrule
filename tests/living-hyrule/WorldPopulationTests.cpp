#include "WorldPopulationPolicy.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {
using namespace LivingHyrule;
int checks = 0;
int failures = 0;

void Check(bool condition, const char* description) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}

WorldPopulationContext Normal(WorldPopulationPlace place, bool day = true) {
    WorldPopulationContext context;
    context.enabled = context.supportedAdventure = context.normalScene = true;
    context.daytime = day;
    context.place = place;
    return context;
}

bool Present(const WorldPopulationContext& context, WorldResidentId id) {
    return (WorldResidentMaskFor(context) & WorldResidentBit(id)) != 0;
}

void SetBusiness(WorldPopulationContext& context, unsigned int property, bool repaired) {
    context.economy.enabled = 1;
    context.economy.ownedProperties |= 1u << property;
    if (repaired) context.economy.repairedProperties |= 1u << property;
}

void GatesAndBounds() {
    for (unsigned int place = 0; place <= static_cast<unsigned int>(WorldPopulationPlace::Count); ++place) {
        for (unsigned int scenario = 0; scenario < 4; ++scenario) {
            for (unsigned int gates = 0; gates < 8; ++gates) {
                auto context = Normal(static_cast<WorldPopulationPlace>(place), (scenario & 1) != 0);
                context.world.adult = (scenario & 2) != 0;
                context.world.forest = context.world.water = context.world.ranchFreed = context.world.ganonDefeated = true;
                SetBusiness(context, 0, true);
                SetBusiness(context, 5, true);
                SetBusiness(context, 12, true);
                context.enabled = (gates & 1) != 0;
                context.supportedAdventure = (gates & 2) != 0;
                context.normalScene = (gates & 4) != 0;
                const auto mask = WorldResidentMaskFor(context);
                Check((mask & ~((1u << static_cast<unsigned int>(WorldResidentId::Count)) - 1)) == 0,
                      "schedule contains only declared identities");
                if (gates != 7) Check(mask == 0, "each opt-in/adventure/normal-scene gate suppresses all residents");
            }
        }
    }
    for (const auto place : { WorldPopulationPlace::None, WorldPopulationPlace::Count,
                              static_cast<WorldPopulationPlace>(255) }) {
        Check(WorldResidentMaskFor(Normal(place)) == 0, "unsupported place remains empty");
    }
    Check(WorldResidentBit(WorldResidentId::Count) == 0, "sentinel identity cannot acquire a bit");
    Check(WorldResidentBit(static_cast<WorldResidentId>(255)) == 0, "unknown identity fails closed");
}

void ChildAndNight() {
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::MarketDay)) ==
              (WorldResidentBit(WorldResidentId::Vessa) | WorldResidentBit(WorldResidentId::Hadrin)),
          "child market has grocer and porter even with economy disabled");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::MarketNight, false)) == WorldResidentBit(WorldResidentId::Pella),
          "night market has lantern keeper");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::MarketDay, false)) == 0, "day scene cannot leak night workers");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::MarketNight)) == 0, "night scene cannot leak day workers");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::MarketRuins)) == 0, "child debug visit cannot populate ruins");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::Field)) ==
              (WorldResidentBit(WorldResidentId::Caro) | WorldResidentBit(WorldResidentId::Hollis)), "child road trade is active");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::Field, false)) == 0, "travelers stay off night roads");
    Check(Present(Normal(WorldPopulationPlace::Ranch), WorldResidentId::Nessa), "child ranch has feed buyer");
    Check(Present(Normal(WorldPopulationPlace::Ranch), WorldResidentId::Wren), "child ranch has stablehand");
    Check(Present(Normal(WorldPopulationPlace::Lake), WorldResidentId::Vero), "child lake has cooperative representative");
    Check(Present(Normal(WorldPopulationPlace::Lake), WorldResidentId::Edda), "child lake has research assistant");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::Ranch, false)) == 0, "unfunded ranch has no extra evening shift");
    Check(WorldResidentMaskFor(Normal(WorldPopulationPlace::Lake, false)) == 0, "unfunded lake has no evening observer");
}

void RegionalRecovery() {
    auto field = Normal(WorldPopulationPlace::Field);
    field.world.adult = true;
    Check(WorldResidentMaskFor(field) == 0, "adult road crisis suppresses travelers");
    field.world.water = field.world.fire = field.world.shadow = true;
    Check(WorldResidentMaskFor(field) == 0, "unrelated medallions do not reopen road trade");
    field.world.forest = true;
    Check(Present(field, WorldResidentId::Caro) && Present(field, WorldResidentId::Hollis), "Forest recovery returns road traders");
    field.daytime = false;
    Check(WorldResidentMaskFor(field) == 0, "recovery still respects dangerous night roads");

    auto ranch = Normal(WorldPopulationPlace::Ranch);
    ranch.world.adult = true;
    Check(WorldResidentMaskFor(ranch) == WorldResidentBit(WorldResidentId::Wren), "stablehand remains through ranch crisis");
    ranch.world.forest = ranch.world.water = true;
    Check(!Present(ranch, WorldResidentId::Nessa), "medallions cannot replace Epona recovery");
    ranch.world.ranchFreed = true;
    Check(Present(ranch, WorldResidentId::Nessa), "Epona recovery returns feed buyer");

    auto lake = Normal(WorldPopulationPlace::Lake);
    lake.world.adult = true;
    Check(WorldResidentMaskFor(lake) == WorldResidentBit(WorldResidentId::Edda), "research assistant remains on high ground");
    lake.world.forest = lake.world.fire = true;
    Check(!Present(lake, WorldResidentId::Vero), "unrelated dungeons cannot restore fishing");
    lake.world.water = true;
    Check(Present(lake, WorldResidentId::Vero), "Water recovery returns net-mender");
}

void ReconstructionAndInvalidState() {
    auto market = Normal(WorldPopulationPlace::MarketRuins);
    market.world.adult = true;
    SetBusiness(market, 0, true);
    Check(WorldResidentMaskFor(market) == 0, "paid property cannot bypass Ganon prerequisite");
    market.world.ganonDefeated = true;
    market.economy.enabled = 0;
    Check(WorldResidentMaskFor(market) == 0, "postgame relief requires enabled cleanup economy");
    market.economy.enabled = 1;
    market.economy.repairedProperties = 0;
    Check(WorldResidentMaskFor(market) ==
              (WorldResidentBit(WorldResidentId::Hadrin) | WorldResidentBit(WorldResidentId::Vessa)),
          "safe ruins keep both sellers accessible before repairs");
    market.economy.repairedProperties = 1;
    Check(Present(market, WorldResidentId::Vessa), "grocer stays available after paid stall reconstruction");
    market.daytime = false;
    Check(WorldResidentMaskFor(market) == WorldResidentBit(WorldResidentId::Pella), "night relief uses lantern keeper");
    market.economy.bankRupees = kBankLimit + 1;
    Check(WorldResidentMaskFor(market) == 0, "invalid ledger cannot authorize presence in ruins");

    for (const auto place : { WorldPopulationPlace::Ranch, WorldPopulationPlace::Lake }) {
        const unsigned int property = place == WorldPopulationPlace::Ranch ? 5 : 12;
        const auto worker = place == WorldPopulationPlace::Ranch ? WorldResidentId::Wren : WorldResidentId::Edda;
        auto context = Normal(place, false);
        SetBusiness(context, property, false);
        Check(Present(context, worker), "working childhood investment funds an evening shift");
        context.world.adult = true;
        Check(WorldResidentMaskFor(context) == 0, "seven-year transition suspends the evening shift");
        context.economy.repairedProperties |= 1u << property;
        Check(WorldResidentMaskFor(context) == 0, "repair bits alone cannot bypass regional story recovery");
        context.world.water = context.world.ranchFreed = true;
        Check(Present(context, worker), "story recovery plus paid repair restores evening work");
        context.economy.repairedProperties = 0;
        Check(WorldResidentMaskFor(context) == 0, "story recovery alone does not fund evening work");
        context.economy.repairedProperties = 1u << property;
        context.economy.enabled = 0;
        Check(WorldResidentMaskFor(context) == 0, "paused economy stops business-dependent shifts");
        context.economy.enabled = 1;
        context.economy.ownedProperties = 0;
        Check(WorldResidentMaskFor(context) == 0, "orphan repair bit cannot fund evening work");
        context.economy = {};
        context.daytime = true;
        Check(Present(context, worker), "base regional cast remains independent of bank opt-in");
    }
}
} // namespace

int main() {
    GatesAndBounds();
    ChildAndNight();
    RegionalRecovery();
    ReconstructionAndInvalidState();
    if (failures != 0) {
        std::cerr << failures << " of " << checks << " world population checks failed.\n";
        return 1;
    }
    std::cout << "Living Hyrule world population: all " << checks << " checks passed.\n";
    return 0;
}
