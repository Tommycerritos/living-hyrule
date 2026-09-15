#include "TradeDialogue.h"
#include "LivingHyrule.h"
#include "ResidentSocial.h"
#include "Stewardship.h"
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

void PrepareResidentDialogue(TradeDialogueState& trade, Actor* actor) {
    const auto& state = gSaveContext.ship.livingHyrule;
    const ResidentId speaker = GetSocialResidentId(actor);
    if (!IsValidState(state) || state.enabled != 1 || !IsValidResident(speaker))
        return;
    const auto append = [&trade](TradeOffer offer) {
        if (offer.kind != TradeKind::None && trade.offerCount < std::size(trade.offers))
            trade.offers[trade.offerCount++] = offer;
    };
    append(trade.offer);
    append(FavorOffer(state, speaker, GetWorldProgress()));
    if (speaker == ResidentId::Bram && state.ownsKakarikoCottage)
        append({ state.cottageRentPolicy ? TradeKind::FairRent : TradeKind::HighRent, 0,
                 state.cottageRentPolicy ? kRentPerPeriod : kHighRentPerPeriod });
    if (speaker == ResidentId::Hadrin && !state.marketRestored && GetWorldProgress().adult &&
        GetWorldProgress().ganonDefeated)
        append({ TradeKind::RestoreMarket, 0, kMarketRestorationPrice });
    if (trade.offerCount != 0)
        trade.offer = trade.offers[0];
}

static std::string DescribeOfferDetails(const TradeOffer& offer) {
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
        case TradeKind::AcceptFavor:
            if (const auto* favor = GetFavor(static_cast<uint8_t>(offer.propertyId)); favor != nullptr)
                text = std::string(favor->instructions) + "^Will you carry this for me? There is no fee.";
            break;
        case TradeKind::CompleteFavor:
            if (const auto* favor = GetFavor(static_cast<uint8_t>(offer.propertyId)); favor != nullptr)
                text =
                    "You brought " + std::string(GetSocialResidentName(favor->issuer)) + "'s delivery. Hand it over?";
            break;
        case TradeKind::FairRent:
            text = "Set cottage rent to 25 rupees per period? Fair terms help Bram's trust recover. "
                   "The change begins after the current rent period.";
            break;
        case TradeKind::HighRent:
            text = "Set cottage rent to 40 rupees per period? Bram will lose trust. If he falls behind, "
                   "only 15 can be collected. New terms begin after the current period.";
            break;
        case TradeKind::RestoreMarket:
            text = "Fund the Market square's restoration for " + amount +
                   " bank rupees? The work restores "
                   "the streets and facades on your next visit. Shops and alleys remain closed.";
            break;
    }
    return text;
}

std::string DescribeTradeOffer(const TradeOffer& offer) {
    const auto text = DescribeOfferDetails(offer);
    return text.empty() ? std::string{} : "^" + text + "\x1B%gYes&Not now%w";
}

std::string DescribeResidentDialogue(Actor* actor, TradeDialogueState& trade, const std::string& introduction) {
    std::string text = trade.cycled
                           ? std::string{}
                           : introduction + ResidentGreeting(actor) + StewardshipGreeting(GetSocialResidentId(actor));
    if (trade.offerCount == 0)
        return text;
    if (trade.offerCount == 1)
        return text + DescribeTradeOffer(trade.offer);
    // AutoFormat aligns two-choice prompts only. Give three choices their own
    // four-line page so every native cursor row matches its visible answer.
    return (text.empty() ? std::string{} : text + "^") + DescribeOfferDetails(trade.offer) +
           "^Proceed?&\x1C%gYes&Something else&Not now%w";
}

bool HandleTradeChoice(PlayState* play, Actor* actor, TradeDialogueState& trade, uint16_t replyTextId) {
    if (play == nullptr || play != gPlayState || actor == nullptr || trade.offer.kind == TradeKind::None ||
        trade.consumed || trade.offerCount == 0 || trade.offerCount > 3 || trade.offerIndex >= trade.offerCount ||
        (trade.handledChoice && trade.lastChoiceFrame == play->gameplayFrames) ||
        trade.fileNum != gSaveContext.fileNum || play->msgCtx.talkActor != actor ||
        play->msgCtx.textId != trade.quoteTextId || Message_GetState(&play->msgCtx) != TEXT_STATE_CHOICE)
        return false;

    const Player* player = GET_PLAYER(play);
    if (player == nullptr || player->talkActor != actor || !(player->stateFlags1 & PLAYER_STATE1_TALKING))
        return false;

    const auto& input = play->state.input[0];
    const bool freshA = (input.press.button & BTN_A) != 0;
    // Choice navigation happens during drawing, after this update. A simultaneous
    // direction and A must not purchase the previously highlighted answer.
    const bool navigating =
        std::abs(static_cast<int>(input.rel.stick_y)) >= 30 || (input.press.button & (BTN_DUP | BTN_DDOWN)) != 0;
    const bool cancel = (input.cur.button & BTN_B) != 0 || (input.press.button & BTN_CUP) != 0 || navigating;
    const auto decision = DecideResidentChoice(trade.consumed, Message_ShouldAdvance(play), freshA, cancel,
                                               play->msgCtx.choiceIndex, trade.offerCount);
    if (decision == TradeDecision::Wait)
        return false;

    trade.handledChoice = true;
    trade.lastChoiceFrame = play->gameplayFrames;
    if (decision == TradeDecision::Cycle) {
        trade.offerIndex = static_cast<uint8_t>((trade.offerIndex + 1) % trade.offerCount);
        trade.offer = trade.offers[trade.offerIndex];
        trade.cycled = true;
        Message_ContinueTextbox(play, trade.quoteTextId);
        return true;
    }

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
            case TradeKind::AcceptFavor:
                action = Action::AcceptFavor;
                amount = trade.offer.propertyId;
                break;
            case TradeKind::CompleteFavor:
                action = Action::CompleteFavor;
                amount = trade.offer.propertyId;
                break;
            case TradeKind::FairRent:
                action = Action::SetFairRent;
                amount = 0;
                break;
            case TradeKind::HighRent:
                action = Action::SetHighRent;
                amount = 0;
                break;
            case TradeKind::RestoreMarket:
                action = Action::RestoreMarket;
                amount = 0;
                break;
            default:
                return false;
        }
        response = PerformConversationAction(actor, trade.quoteTextId, action, amount, trade.offer.amount);
    }
    std::snprintf(trade.response, sizeof(trade.response), "%s", response.c_str());
    Message_ContinueTextbox(play, replyTextId);
    return true;
}

} // namespace LivingHyrule
