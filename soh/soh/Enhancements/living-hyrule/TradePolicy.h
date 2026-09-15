#pragma once

#include "SocialPolicy.h"

namespace LivingHyrule {

enum class TradeKind : uint8_t {
    None,
    Cottage,
    Property,
    Repair,
    Deposit,
    Withdraw,
    AcceptFavor,
    CompleteFavor,
    FairRent,
    HighRent,
    RestoreMarket
};
struct TradeOffer {
    TradeKind kind;
    uint32_t propertyId;
    uint32_t amount;
};

inline TradeOffer PropertyOffer(const EconomyState& state, uint32_t id, const WorldProgress& world) {
    if (!IsValidState(state) || !state.enabled || id >= kProperties.size() ||
        !RegionOpen(kProperties[id].region, world))
        return {};
    if (!OwnsProperty(state, id))
        return { TradeKind::Property, id, kProperties[id].price };
    if (world.adult && !(state.repairedProperties & (1u << id)))
        return { TradeKind::Repair, id, EffectiveRepairPrice(state, id) };
    return {};
}

enum class TradeDecision { Wait, Cancel, Confirm, Cycle };
inline TradeDecision DecideTradeChoice(bool consumed, bool ownsDialogue, bool atChoice, bool shouldAdvance, bool freshA,
                                       bool cancel) {
    if (consumed || !ownsDialogue || !atChoice || !shouldAdvance)
        return TradeDecision::Wait;
    if (cancel)
        return TradeDecision::Cancel;
    return freshA ? TradeDecision::Confirm : TradeDecision::Wait;
}

// Three choices reserve the middle row for another topic. Cancel buttons win
// even if A is pressed at the same time; navigation never confirms an offer.
inline TradeDecision DecideResidentChoice(bool consumed, bool shouldAdvance, bool freshA, bool cancelButton,
                                          uint8_t choice, uint8_t offerCount) {
    if (consumed || !shouldAdvance || offerCount == 0 || offerCount > 3)
        return TradeDecision::Wait;
    if (cancelButton)
        return TradeDecision::Cancel;
    if (!freshA)
        return TradeDecision::Wait;
    if (choice == 0)
        return TradeDecision::Confirm;
    if (offerCount > 1 && choice == 1)
        return TradeDecision::Cycle;
    return TradeDecision::Cancel;
}

inline TradeOffer FavorOffer(const EconomyState& state, ResidentId speaker, const WorldProgress& world) {
    if (!IsValidState(state) || !state.enabled || !IsValidResident(speaker))
        return {};
    if (const auto* active = GetFavor(state.activeFavor); active != nullptr) {
        if (active->recipient == speaker && ResidentAccessible(speaker, world))
            return { TradeKind::CompleteFavor, state.activeFavor, 0 };
        return {};
    }
    for (uint8_t id = 1; id <= kFavorCount; ++id) {
        if (GetFavor(id)->issuer == speaker && CanAcceptFavor(state, id, world))
            return { TradeKind::AcceptFavor, id, 0 };
    }
    return {};
}

} // namespace LivingHyrule
