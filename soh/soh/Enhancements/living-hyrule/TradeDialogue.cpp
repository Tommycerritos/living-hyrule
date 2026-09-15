#include "TradeDialogue.h"
#include "LivingHyrule.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

#include <algorithm>
#include <cstdio>
#include <type_traits>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {

static_assert(std::is_trivial_v<TradeDialogueState>);

static bool Prepare(TradeDialogueState& trade, uint16_t textId) {
    trade = {};
    trade.quoteTextId = textId;
    trade.fileNum = gSaveContext.fileNum;
    return GameInteractor::IsSaveLoaded(false) && IsValidState(gSaveContext.ship.livingHyrule) &&
           gSaveContext.ship.livingHyrule.enabled == 1;
}

void PreparePropertyTrade(TradeDialogueState& trade, int propertyId, uint16_t quoteTextId) {
    if (!Prepare(trade, quoteTextId) || propertyId < 0)
        return;
    trade.offer = PropertyOffer(gSaveContext.ship.livingHyrule, static_cast<uint32_t>(propertyId), GetWorldProgress());
}

void PrepareCottageTrade(TradeDialogueState& trade, uint16_t quoteTextId) {
    if (!Prepare(trade, quoteTextId) || gSaveContext.ship.livingHyrule.ownsKakarikoCottage ||
        !RegionOpen(Region::Kakariko, GetWorldProgress()))
        return;
    trade.offer = { TradeKind::Cottage, 0, kCottagePrice };
}

void PrepareBankTrade(TradeDialogueState& trade, bool deposit, uint16_t quoteTextId) {
    if (!Prepare(trade, quoteTextId))
        return;
    const auto& state = gSaveContext.ship.livingHyrule;
    const int capacity = CUR_CAPACITY(UPG_WALLET);
    if (!IsValidWallet(gSaveContext.rupees, capacity))
        return;
    const uint32_t amount =
        deposit ? static_cast<uint32_t>(std::min<uint64_t>(gSaveContext.rupees, kBankLimit - state.bankRupees))
                : static_cast<uint32_t>(std::min<uint64_t>(capacity - gSaveContext.rupees, state.bankRupees));
    if (amount != 0)
        trade.offer = { deposit ? TradeKind::Deposit : TradeKind::Withdraw, 0, amount };
}

std::string DescribeTradeOffer(const TradeOffer& offer) {
    const std::string amount = std::to_string(offer.amount);
    std::string text;
    switch (offer.kind) {
        case TradeKind::None:
            return {};
        case TradeKind::Cottage:
            text = "Buy the rental cottage for " + amount + " bank rupees?";
            break;
        case TradeKind::Property:
            if (offer.propertyId >= kProperties.size())
                return {};
            text = "Buy " + std::string(kProperties[offer.propertyId].name) + " for " + amount + " bank rupees?";
            break;
        case TradeKind::Repair:
            if (offer.propertyId >= kProperties.size())
                return {};
            text = "Repair " + std::string(kProperties[offer.propertyId].name) + " for " + amount + " bank rupees?";
            break;
        case TradeKind::Deposit:
            text = "Deposit " + amount + " wallet rupees into your bank?";
            break;
        case TradeKind::Withdraw:
            text = "Withdraw " + amount + " bank rupees into your wallet?";
            break;
    }
    return "^" + text + "\x1B%gYes&Not now%w";
}

bool HandleTradeChoice(PlayState* play, Actor* actor, TradeDialogueState& trade, uint16_t replyTextId) {
    if (play == nullptr || play != gPlayState || actor == nullptr || trade.offer.kind == TradeKind::None ||
        trade.consumed || trade.fileNum != gSaveContext.fileNum || play->msgCtx.talkActor != actor ||
        play->msgCtx.textId != trade.quoteTextId || Message_GetState(&play->msgCtx) != TEXT_STATE_CHOICE)
        return false;

    const auto& input = play->state.input[0];
    const bool freshA = (input.press.button & BTN_A) != 0;
    const bool cancel =
        play->msgCtx.choiceIndex != 0 || (input.cur.button & BTN_B) != 0 || (input.press.button & BTN_CUP) != 0;
    const auto decision = DecideTradeChoice(trade.consumed, true, true, Message_ShouldAdvance(play), freshA, cancel);
    if (decision == TradeDecision::Wait)
        return false;

    // Enter terminal state before mutating money or continuing the textbox.
    // Held skip buttons and repeated update calls cannot charge a second time.
    trade.consumed = true;
    std::string response = "No problem. We can discuss it another time.";
    if (decision == TradeDecision::Confirm) {
        Action action = Action::Deposit;
        uint32_t amount = trade.offer.amount;
        switch (trade.offer.kind) {
            case TradeKind::Cottage:
                action = Action::BuyCottage;
                amount = 0;
                break;
            case TradeKind::Property:
                action = Action::BuyProperty;
                amount = trade.offer.propertyId;
                break;
            case TradeKind::Repair:
                action = Action::RepairProperty;
                amount = trade.offer.propertyId;
                break;
            case TradeKind::Deposit:
                action = Action::Deposit;
                break;
            case TradeKind::Withdraw:
                action = Action::Withdraw;
                break;
            default:
                return false;
        }
        response = PerformConversationAction(actor, trade.quoteTextId, action, amount);
    }
    std::snprintf(trade.response, sizeof(trade.response), "%s", response.c_str());
    Message_ContinueTextbox(play, replyTextId);
    return true;
}

} // namespace LivingHyrule
