#pragma once

#include "TradePolicy.h"
#include <string>

struct Actor;
struct PlayState;

namespace LivingHyrule {

// Actor memory is zeroed by the engine, so no constructors or owning members.
struct TradeDialogueState {
    TradeOffer offer;
    int fileNum;
    uint16_t quoteTextId;
    bool consumed;
    char response[256];
};

// Call while offering a new talk prompt, never while the actor is conversing.
void PreparePropertyTrade(TradeDialogueState& trade, int propertyId, uint16_t quoteTextId);
void PrepareCottageTrade(TradeDialogueState& trade, uint16_t quoteTextId);
void PrepareBankTrade(TradeDialogueState& trade, bool deposit, uint16_t quoteTextId);
std::string DescribeTradeOffer(const TradeOffer& offer);
bool HandleTradeChoice(PlayState* play, Actor* actor, TradeDialogueState& trade, uint16_t replyTextId);

} // namespace LivingHyrule
