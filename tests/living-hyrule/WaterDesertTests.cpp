#include "WaterDesertPolicy.h"
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
WaterDesertContext Normal(WaterDesertPlace place, bool daytime = true) {
    WaterDesertContext context;
    context.enabled = context.supportedAdventure = context.normalScene = true;
    context.daytime = daytime;
    context.place = place;
    return context;
}
bool Present(const WaterDesertContext& context, WaterDesertResidentId id) {
    return (WaterDesertMaskFor(context) & WaterDesertBit(id)) != 0;
}
void Invest(WaterDesertContext& context, unsigned int property, bool repaired) {
    context.economy.enabled = 1;
    context.economy.ownedProperties |= 1u << property;
    if (repaired)
        context.economy.repairedProperties |= 1u << property;
}

void GeneralGates() {
    for (unsigned int place = 0; place <= static_cast<unsigned int>(WaterDesertPlace::Count); ++place) {
        for (unsigned int scenario = 0; scenario < 4; ++scenario) {
            for (unsigned int gates = 0; gates < 8; ++gates) {
                auto context = Normal(static_cast<WaterDesertPlace>(place), (scenario & 1) != 0);
                context.world.adult = (scenario & 2) != 0;
                context.world.water = context.world.spirit = context.world.gerudoMembership = context.carpentersFreed =
                    true;
                Invest(context, 13, true);
                Invest(context, 14, true);
                Invest(context, 15, true);
                context.enabled = (gates & 1) != 0;
                context.supportedAdventure = (gates & 2) != 0;
                context.normalScene = (gates & 4) != 0;
                const auto mask = WaterDesertMaskFor(context);
                Check((mask & ~0xFu) == 0, "only four declared identities can appear");
                if (gates != 7)
                    Check(mask == 0, "every opt-in/adventure/normal-scene gate is required");
            }
        }
    }
    for (const auto place : { WaterDesertPlace::None, WaterDesertPlace::Count, static_cast<WaterDesertPlace>(255) }) {
        auto context = Normal(place);
        context.world.adult = context.world.water = context.world.spirit = context.world.gerudoMembership = true;
        context.carpentersFreed = true;
        context.domainRestored = true;
        Invest(context, 13, true);
        Check(WaterDesertMaskFor(context) == 0, "unsupported scenes stay empty even after recovery and repair");
    }
    Check(WaterDesertBit(WaterDesertResidentId::Count) == 0, "sentinel cannot acquire population bit");
    Check(WaterDesertBit(static_cast<WaterDesertResidentId>(255)) == 0, "unknown identity fails closed");
}

void RiverRecovery() {
    auto river = Normal(WaterDesertPlace::RiverBank);
    Check(Present(river, WaterDesertResidentId::Lethra) && Present(river, WaterDesertResidentId::Neris),
          "child river has both Zoras independently of bank account");
    river.daytime = false;
    Check(WaterDesertMaskFor(river) == 0, "unfunded river has no evening courier");
    Invest(river, 13, false);
    Check(WaterDesertMaskFor(river) == WaterDesertBit(WaterDesertResidentId::Neris),
          "child supply business funds courier evening shift");
    river.world.adult = true;
    Check(WaterDesertMaskFor(river) == 0, "age transition suspends evening supply work");
    river.daytime = true;
    Check(WaterDesertMaskFor(river) == WaterDesertBit(WaterDesertResidentId::Lethra),
          "adult crisis retains one refugee on the dry lower bank");
    river.world.forest = river.world.fire = river.world.shadow = river.world.spirit = true;
    Check(!Present(river, WaterDesertResidentId::Neris), "unrelated medallions do not reopen river supply travel");
    river.world.water = true;
    Check(Present(river, WaterDesertResidentId::Neris), "Water recovery returns daytime courier");
    river.daytime = false;
    Check(WaterDesertMaskFor(river) == 0, "Water recovery alone cannot replace paid repairs");
    river.economy.repairedProperties = 1u << 13;
    Check(Present(river, WaterDesertResidentId::Neris), "story recovery and repairs restore evening courier");
    river.economy.enabled = 0;
    Check(WaterDesertMaskFor(river) == 0, "paused account removes business-dependent evening work");
    river.economy.enabled = 1;
    river.economy.bankRupees = kBankLimit + 1;
    Check(WaterDesertMaskFor(river) == 0, "invalid ledger cannot fund courier shift");
    river.daytime = true;
    Check(Present(river, WaterDesertResidentId::Lethra), "invalid ledger does not erase independent daytime cast");
}

void GerudoAccess() {
    auto valley = Normal(WaterDesertPlace::ValleyApproach);
    Check(Present(valley, WaterDesertResidentId::Rasha), "child may meet quartermaster on public approach");
    Check(!RegionOpen(Region::Desert, valley.world), "child meeting does not permit property trade");
    Check(WaterDesertMaskFor(Normal(WaterDesertPlace::Fortress)) == 0, "child fortress has no friendly added trader");

    // Enumerate age/rescue/membership/Spirit independently. Presence needs the
    // access prerequisites; business permission additionally needs Spirit.
    for (const auto place : { WaterDesertPlace::ValleyApproach, WaterDesertPlace::Fortress }) {
        for (unsigned int flags = 0; flags < 16; ++flags) {
            auto context = Normal(place);
            context.world.adult = (flags & 1) != 0;
            context.carpentersFreed = (flags & 2) != 0;
            context.world.gerudoMembership = (flags & 4) != 0;
            context.world.spirit = (flags & 8) != 0;
            const bool invited = context.world.adult && context.carpentersFreed && context.world.gerudoMembership;
            const bool expected = invited || (!context.world.adult && place == WaterDesertPlace::ValleyApproach);
            Check((WaterDesertMaskFor(context) != 0) == expected,
                  "Gerudo daytime presence respects independent access prerequisites");
            if (invited)
                Check(RegionOpen(Region::Desert, context.world) == context.world.spirit,
                      "membership and rescue do not replace Spirit recovery for trade");
        }
    }
}

void GerudoEveningWork() {
    for (const auto place : { WaterDesertPlace::ValleyApproach, WaterDesertPlace::Fortress }) {
        const unsigned int property = place == WaterDesertPlace::ValleyApproach ? 14 : 15;
        const auto id =
            place == WaterDesertPlace::ValleyApproach ? WaterDesertResidentId::Rasha : WaterDesertResidentId::Kesra;
        auto context = Normal(place, false);
        context.world.adult = context.world.spirit = context.world.gerudoMembership = context.carpentersFreed = true;
        Check(WaterDesertMaskFor(context) == 0, "no unfunded Gerudo evening shift");
        Invest(context, property, false);
        Check(WaterDesertMaskFor(context) == 0, "adult purchase without repair cannot fund evening shift");
        context.economy.repairedProperties |= 1u << property;
        Check(Present(context, id), "operating local business enables evening work");
        context.carpentersFreed = false;
        Check(WaterDesertMaskFor(context) == 0, "repair and membership cannot replace rescued carpenters");
        context.carpentersFreed = true;
        context.world.gerudoMembership = false;
        Check(WaterDesertMaskFor(context) == 0, "repair and rescue cannot replace invitation");
        context.world.gerudoMembership = true;
        context.world.spirit = false;
        Check(WaterDesertMaskFor(context) == 0, "access and repair cannot replace Spirit recovery");
        context.world.spirit = true;
        context.economy.enabled = 0;
        Check(WaterDesertMaskFor(context) == 0, "paused economy stops evening work");
        context.economy.enabled = 1;
        context.economy.ownedProperties = 0;
        Check(WaterDesertMaskFor(context) == 0, "orphan repair cannot fund evening work");
        context.economy = {};
        Invest(context, property == 14 ? 15 : 14, true);
        Check(WaterDesertMaskFor(context) == 0, "a different business cannot fund this trader's evening shift");
    }
}

void DomainRecovery() {
    // Independent gates: an investment, medallion or repaired business alone
    // never places residents on the frozen pool. Only the loaded thaw qualifies.
    for (unsigned int gates = 0; gates < 256; ++gates) {
        auto domain = Normal(WaterDesertPlace::RestoredDomain);
        domain.enabled = (gates & 1) != 0;
        domain.supportedAdventure = (gates & 2) != 0;
        domain.normalScene = (gates & 4) != 0;
        domain.daytime = (gates & 8) != 0;
        domain.world.adult = (gates & 16) != 0;
        domain.world.water = (gates & 32) != 0;
        domain.domainRestored = (gates & 64) != 0;
        domain.economy.enabled = (gates & 128) != 0;
        const auto both = static_cast<uint8_t>(WaterDesertBit(WaterDesertResidentId::Lethra) |
                                               WaterDesertBit(WaterDesertResidentId::Neris));
        Check(WaterDesertMaskFor(domain) == (gates == 255 ? both : 0),
              "Domain requires actual restoration, adult recovery and every daytime account/scene gate");
        domain.economy.bankRupees = kBankLimit + 1;
        Check(WaterDesertMaskFor(domain) == 0, "unreadable ledger never supports Domain residents");
    }
    for (bool adult : { false, true }) {
        for (bool water : { false, true }) {
            for (bool daytime : { false, true }) {
                auto river = Normal(WaterDesertPlace::RiverBank, daytime);
                river.world.adult = adult;
                river.world.water = water;
                Invest(river, 13, true);
                const auto original = WaterDesertMaskFor(river);
                river.domainRestored = true;
                Check(WaterDesertMaskFor(river) == original,
                      "River remains reachable after restoration, including child and evening schedules");
            }
        }
    }
    Check(kDomainResidentPlacements.size() == 2 && kDomainResidentPlacements[0].id == WaterDesertResidentId::Lethra &&
              kDomainResidentPlacements[1].id == WaterDesertResidentId::Neris &&
              kDomainResidentPlacements[0].id != kDomainResidentPlacements[1].id,
          "Domain placements reuse exactly two existing identities without duplicate entries");
}
} // namespace

int main() {
    GeneralGates();
    RiverRecovery();
    GerudoAccess();
    GerudoEveningWork();
    DomainRecovery();
    if (failures != 0) {
        std::cerr << failures << " of " << checks << " Water/Desert checks failed.\n";
        return 1;
    }
    std::cout << "Living Hyrule Water/Desert population: all " << checks << " checks passed.\n";
    return 0;
}
