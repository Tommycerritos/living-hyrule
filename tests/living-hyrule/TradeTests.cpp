#include "TradePolicy.h"
#include <iostream>

using namespace LivingHyrule;
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << "Line " << __LINE__ << ": " #x "\n"; ++failures; } } while (0)

int main() {
    CHECK(DecideTradeChoice(false, true, true, true, true, false) == TradeDecision::Confirm);
    CHECK(DecideTradeChoice(false, true, true, true, false, false) == TradeDecision::Wait);
    CHECK(DecideTradeChoice(false, true, true, true, false, true) == TradeDecision::Cancel);
    CHECK(DecideTradeChoice(false, true, true, true, true, true) == TradeDecision::Cancel);
    CHECK(DecideTradeChoice(false, true, true, false, true, false) == TradeDecision::Wait);
    CHECK(DecideTradeChoice(false, false, true, true, true, false) == TradeDecision::Wait);
    CHECK(DecideTradeChoice(false, true, false, true, true, false) == TradeDecision::Wait);
    for (int mask = 0; mask < 64; ++mask) {
        CHECK(DecideTradeChoice(true, mask & 1, mask & 2, mask & 4, mask & 8, mask & 16) == TradeDecision::Wait);
    }
    EconomyState state{};
    WorldProgress child{};
    WorldProgress adult{ true, true, true, true, true, true, true, true, true };
    CHECK(PropertyOffer(state, 0, child).kind == TradeKind::None);
    state.enabled = 1;
    for (uint32_t id = 0; id < kProperties.size(); ++id) {
        const auto quote = PropertyOffer(state, id, adult);
        CHECK(quote.kind == TradeKind::Property);
        CHECK(quote.propertyId == id && quote.amount == kProperties[id].price);
        state.ownedProperties |= 1u << id;
        CHECK(PropertyOffer(state, id, child).kind == TradeKind::None);
        const auto repair = PropertyOffer(state, id, adult);
        CHECK(repair.kind == TradeKind::Repair && repair.amount == kProperties[id].repairCost);
        state.repairedProperties |= 1u << id;
        CHECK(PropertyOffer(state, id, adult).kind == TradeKind::None);
    }
    CHECK(PropertyOffer(state, UINT32_MAX, adult).kind == TradeKind::None);
    state.enabled = 2;
    CHECK(PropertyOffer(state, 0, adult).kind == TradeKind::None);
    if (failures) return 1;
    std::cout << "Trade quote and deliberate confirmation policy tests passed\n";
    return 0;
}
