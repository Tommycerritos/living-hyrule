#pragma once

#include "Properties.h"

namespace LivingHyrule {

enum class TradeKind : uint8_t { None, Cottage, Property, Repair, Deposit, Withdraw };
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
        return { TradeKind::Repair, id, kProperties[id].repairCost };
    return {};
}

enum class TradeDecision { Wait, Cancel, Confirm };
inline TradeDecision DecideTradeChoice(bool consumed, bool ownsDialogue, bool atChoice, bool shouldAdvance, bool freshA,
                                       bool cancel) {
    if (consumed || !ownsDialogue || !atChoice || !shouldAdvance)
        return TradeDecision::Wait;
    if (cancel)
        return TradeDecision::Cancel;
    return freshA ? TradeDecision::Confirm : TradeDecision::Wait;
}

} // namespace LivingHyrule
