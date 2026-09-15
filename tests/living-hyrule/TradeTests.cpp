#include "TradePolicy.h"
#include <iostream>

using namespace LivingHyrule;
int failures = 0;
#define CHECK(x)                                              \
    do {                                                      \
        if (!(x)) {                                           \
            std::cerr << "Line " << __LINE__ << ": " #x "\n"; \
            ++failures;                                       \
        }                                                     \
    } while (0)

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
    // Native three-choice navigation is separate from the selected trade. No
    // topic-navigation row, skip input, cancellation, or repeated event buys it.
    for (uint8_t count = 1; count <= kResidentOfferCapacity; ++count) {
        for (uint8_t choice = 0; choice < 4; ++choice) {
            CHECK(DecideResidentChoice(true, true, true, false, choice, count) == TradeDecision::Wait);
            CHECK(DecideResidentChoice(false, false, true, false, choice, count) == TradeDecision::Wait);
            CHECK(DecideResidentChoice(false, true, false, false, choice, count) == TradeDecision::Wait);
            CHECK(DecideResidentChoice(false, true, true, true, choice, count) == TradeDecision::Cancel);
            const auto expected = choice == 0                ? TradeDecision::Confirm
                                  : choice == 1 && count > 1 ? TradeDecision::Cycle
                                                             : TradeDecision::Cancel;
            CHECK(DecideResidentChoice(false, true, true, false, choice, count) == expected);
        }
    }
    CHECK(DecideResidentChoice(false, true, true, false, 0, 0) == TradeDecision::Wait);
    CHECK(DecideResidentChoice(false, true, true, false, 0, kResidentOfferCapacity + 1) == TradeDecision::Wait);
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
    state = {};
    state.enabled = 1;
    state.bankRupees = 100000;
    state.ownedProperties = 1u;
    AdjustRapport(state, ResidentId::Vessa, kTrustedRapport);
    const auto discount = PropertyOffer(state, 0, adult);
    CHECK(discount.kind == TradeKind::Repair && discount.amount == 810);
    const uint64_t beforeRepair = state.bankRupees;
    CHECK(RepairProperty(state, 0, adult) == Result::Success);
    CHECK(state.bankRupees == beforeRepair - discount.amount);

    for (uint8_t id = 1; id <= kFavorCount; ++id) {
        state = {};
        state.enabled = 1;
        const Favor* favor = GetFavor(id);
        const auto quote = FavorOffer(state, favor->issuer, adult);
        CHECK(quote.kind == TradeKind::AcceptFavor && quote.propertyId == id);
        CHECK(AcceptFavor(state, id, adult) == Result::Success);
        for (uint32_t index = 0; index < kSocialResidentCount; ++index) {
            const auto resident = static_cast<ResidentId>(index);
            const auto delivery = FavorOffer(state, resident, adult);
            CHECK(delivery.kind == (resident == favor->recipient ? TradeKind::CompleteFavor : TradeKind::None));
        }
        CHECK(CompleteFavor(state, id, favor->recipient, adult) == Result::Success);
        CHECK(FavorOffer(state, favor->issuer, adult).kind == TradeKind::None);
    }
    CHECK(FavorOffer(state, ResidentId::Count, adult).kind == TradeKind::None);
    state.enabled = 2;
    CHECK(PropertyOffer(state, 0, adult).kind == TradeKind::None);
    if (failures)
        return 1;
    std::cout << "Trade quote and deliberate confirmation policy tests passed\n";
    return 0;
}
