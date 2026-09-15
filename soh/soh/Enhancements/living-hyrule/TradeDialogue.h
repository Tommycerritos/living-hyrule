#pragma once

#include "TradePolicy.h"
#include <string>

struct Actor;
struct PlayState;

namespace LivingHyrule {

// Actor memory is zeroed by the engine, so no constructors or owning members.
struct TradeDialogueState {
    TradeOffer offer;
    TradeOffer offers[kResidentOfferCapacity];
    uint8_t offerCount;
    uint8_t offerIndex;
    bool cycled;
    bool handledChoice;
    uint32_t lastChoiceFrame;
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
// Append favors and household choices after the existing business preparation.
void PrepareResidentDialogue(TradeDialogueState& trade, Actor* actor);
std::string DescribeResidentDialogue(Actor* actor, TradeDialogueState& trade, const std::string& introduction);
bool HandleTradeChoice(PlayState* play, Actor* actor, TradeDialogueState& trade, uint16_t replyTextId);

} // namespace LivingHyrule
