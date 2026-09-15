#include "Properties.h"
#include <iostream>

using namespace LivingHyrule;
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << "Line " << __LINE__ << ": " #x "\n"; ++failures; } } while (0)

int main() {
    WorldProgress child{};
    WorldProgress crisis{};
    crisis.adult = true;
    WorldProgress recovered{ true, true, true, true, true, true, true, true, true };
    for (uint32_t id = 0; id < kProperties.size(); ++id) {
        EconomyState state{};
        state.enabled = 1;
        state.bankRupees = 100000;
        const bool desert = kProperties[id].region == Region::Desert;
        CHECK(BuyProperty(state, id, crisis) == Result::Unavailable);
        CHECK(state.bankRupees == 100000 && state.ownedProperties == 0);
        CHECK(BuyProperty(state, id, desert ? recovered : child) == Result::Success);
        CHECK(state.bankRupees == 100000 - kProperties[id].price);
        CHECK(BuyProperty(state, id, recovered) == Result::AlreadyOwned);
        CHECK(RepairProperty(state, id, crisis) == Result::Unavailable);
        CHECK(!PropertyOperating(state, id, crisis));
        if (!desert) {
            for (uint32_t i = 0; i < kFramesPerRentPeriod - 1; ++i) CHECK(TickBusinesses(state, child) == 0);
            CHECK(TickBusinesses(state, child) == kProperties[id].income);
        }
        state.businessFrames[id] = 123;
        for (int i = 0; i < 100; ++i) CHECK(TickBusinesses(state, crisis) == 0);
        CHECK(state.businessFrames[id] == 123);
        CHECK(!PropertyOperating(state, id, recovered));
        CHECK(RepairProperty(state, id, recovered) == Result::Success);
        CHECK(RepairProperty(state, id, recovered) == Result::AlreadyRepaired);
        CHECK(PropertyOperating(state, id, recovered));
        state.businessFrames[id] = kFramesPerRentPeriod - 1;
        CHECK(TickBusinesses(state, recovered) == kProperties[id].income);
        CHECK(state.businessFrames[id] == 0);
        state.bankRupees = kBankLimit - 2;
        state.totalBusinessEarned = UINT64_MAX - 1;
        state.businessFrames[id] = kFramesPerRentPeriod - 1;
        CHECK(TickBusinesses(state, recovered) == 2);
        CHECK(state.bankRupees == kBankLimit && state.totalBusinessEarned == UINT64_MAX);
        state.enabled = 0;
        CHECK(TickBusinesses(state, recovered) == 0);
        CHECK(state.businessFrames[id] == 0);
    }
    EconomyState poor{};
    poor.enabled = 1;
    CHECK(BuyProperty(poor, 0, child) == Result::InsufficientBank);
    CHECK(poor.ownedProperties == 0 && poor.bankRupees == 0);
    CHECK(BuyProperty(poor, 16, recovered) == Result::InvalidAmount);
    CHECK(RepairProperty(poor, 0, recovered) == Result::NotOwned);
    poor.ownedProperties = 1;
    CHECK(RepairProperty(poor, 0, recovered) == Result::InsufficientBank);
    CHECK(poor.repairedProperties == 0);
    auto onlyForest = crisis;
    onlyForest.forest = true;
    CHECK(RegionOpen(Region::Forest, onlyForest));
    CHECK(!RegionOpen(Region::Mountain, onlyForest));
    CHECK(!RegionOpen(Region::Market, onlyForest));
    CHECK(!RegionOpen(Region::Desert, child));
    auto cardOnly = crisis;
    cardOnly.gerudoMembership = true;
    CHECK(!RegionOpen(Region::Desert, cardOnly));
    CHECK(!RegionOpen(Region::Count, recovered));
    CHECK(ShouldClearMarketThreats(recovered, true, true));
    CHECK(!ShouldClearMarketThreats(crisis, true, true));
    CHECK(!ShouldClearMarketThreats(child, true, true));
    CHECK(!ShouldClearMarketThreats(recovered, false, true));
    CHECK(!ShouldClearMarketThreats(recovered, true, false));
    CHECK(!PropertyOperating(poor, UINT32_MAX, recovered));
    if (failures) return 1;
    std::cout << "Regional properties, crisis, repairs and income tests passed\n";
    return 0;
}
