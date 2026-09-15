#include "LivingHyrule.h"
#include "SaveCodec.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Notification/Notification.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"

#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <spdlog/spdlog.h>
#include <type_traits>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {

static_assert(std::is_trivially_copyable_v<EconomyState>);
static_assert(std::is_trivially_copyable_v<SaveContext>);

static void InitSave(bool) {
    gSaveContext.ship.livingHyrule = {};
}

static void PreserveUnreadableSave() {
    gSaveContext.ship.livingHyrule = {};
    gSaveContext.ship.livingHyrule.enabled = 2;
    SPDLOG_ERROR("Living Hyrule economy data is unsupported or invalid; keeping the original section unchanged.");
}

static void LoadSave() {
    // Inspect JSON before narrowing any numbers. A malformed mod section must
    // not throw into the base game's save-corruption handler.
    try {
        nlohmann::json data;
        SaveManager::Instance->LoadData("economy", data);
        EconomyState loaded{};
        if (DecodeEconomy(data, loaded)) {
            gSaveContext.ship.livingHyrule = loaded;
            return;
        }
    } catch (const std::exception& exception) {
        SPDLOG_ERROR("Living Hyrule could not read its economy section: {}", exception.what());
    }
    // An invalid flag makes the ledger read-only. SaveSave deliberately leaves
    // the original JSON payload untouched, so ordinary game saves preserve it.
    PreserveUnreadableSave();
}

static bool CanSave(const SaveContext* snapshot) {
    return IsValidState(snapshot->ship.livingHyrule);
}

static void SaveSave(SaveContext* snapshot, int, bool) {
    const auto& economy = snapshot->ship.livingHyrule;
    if (IsValidState(economy)) {
        SaveManager::Instance->SaveData("economy", EncodeEconomy(economy));
    }
}

static bool IsSupportedAdventure() {
    return IS_VANILLA || IS_MASTER_QUEST;
}

static bool IsKakarikoTradeOpen() {
    return LINK_IS_CHILD || CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW);
}

static bool IsSafeGameplay() {
    return GameInteractor::IsSaveLoaded(false) && !GameInteractor::IsGameplayPaused() &&
           gPlayState->pauseCtx.debugState == 0 && gPlayState->gameOverCtx.state == GAMEOVER_INACTIVE &&
           gPlayState->transitionTrigger == TRANS_TRIGGER_OFF && gPlayState->transitionMode == TRANS_MODE_OFF &&
           gSaveContext.health > 0;
}

Status GetStatus() {
    Status status;
    if (!GameInteractor::IsSaveLoaded(false)) {
        return status;
    }
    status.loaded = true;
    status.fileNum = gSaveContext.fileNum;
    status.economy = gSaveContext.ship.livingHyrule;
    status.walletRupees = gSaveContext.rupees;
    status.walletCapacity = CUR_CAPACITY(UPG_WALLET);
    status.inKakariko = gPlayState->sceneNum == SCENE_KAKARIKO_VILLAGE;
    status.cottageTradeOpen = IsKakarikoTradeOpen();

    if (!IsSupportedAdventure()) {
        status.reason = "Living Hyrule is available in a normal adventure or Master Quest.";
    } else if (!IsValidState(status.economy)) {
        status.reason = "This ledger could not be read. Your stored ledger has been preserved.";
    } else if (!SaveManager::Instance->SaveFile_Exist(status.fileNum)) {
        status.reason = "This save file is no longer available.";
    } else if (!IsSafeGameplay()) {
        status.reason = "Finish the conversation, transition, or pause screen to use the ledger.";
    } else if (gSaveContext.rupeeAccumulator != 0) {
        status.reason = "Wait for your wallet to finish counting rupees.";
    } else {
        status.canUseLedger = true;
        status.reason = "";
    }
    return status;
}

static const char* ExplainResult(Result result) {
    switch (result) {
        case Result::Success:
            return "Transaction complete. Save your game to keep your progress.";
        case Result::Disabled:
            return "Enable Living Hyrule for this save first.";
        case Result::InvalidState:
            return "This ledger could not be read. Your stored ledger has been preserved.";
        case Result::InvalidAmount:
            return "Choose a positive number of rupees.";
        case Result::InvalidWallet:
            return "Your wallet balance is outside its current capacity.";
        case Result::InsufficientWallet:
            return "You do not have enough rupees in your wallet.";
        case Result::InsufficientBank:
            return "You do not have enough rupees in the bank.";
        case Result::WalletFull:
            return "That amount will not fit in your wallet.";
        case Result::BankFull:
            return "That amount will not fit in your bank account.";
        case Result::AlreadyOwned:
            return "You already own this property.";
    }
    return "The transaction could not be completed.";
}

std::string PerformAction(Action action, uint32_t amount) {
    // Recheck on every click; never trust a previously drawn menu's availability.
    const auto status = GetStatus();
    if (!status.canUseLedger) {
        return status.reason;
    }
    auto& economy = gSaveContext.ship.livingHyrule;
    switch (action) {
        case Action::Enable:
            economy.enabled = 1;
            return "Living Hyrule is active for this save. Save your game to keep this choice.";
        case Action::Disable:
            economy.enabled = 0;
            return "Economy paused. Your money and ownership are kept. Save your game to keep this choice.";
        case Action::Deposit:
            // The settled wallet and bank change together on the game thread.
            // Normal saves then capture both in the same SaveContext snapshot.
            return ExplainResult(Deposit(economy, gSaveContext.rupees, status.walletCapacity, amount));
        case Action::Withdraw:
            return ExplainResult(Withdraw(economy, gSaveContext.rupees, status.walletCapacity, amount));
        case Action::BuyCottage:
            if (!status.inKakariko) {
                return "Visit Kakariko Village to buy this property.";
            }
            if (!status.cottageTradeOpen) {
                return "Clear the Shadow Temple before Kakariko property sales can resume.";
            }
            if (const auto result = BuyCottage(economy); result != Result::Success) {
                return ExplainResult(result);
            }
            return "The Kakariko rental cottage is yours. Rent will go to your bank. Save your game to keep the deed.";
    }
    return "The transaction could not be completed.";
}

static void UpdateRent() {
    if (!IsSafeGameplay() || !IsSupportedAdventure() || !IsKakarikoTradeOpen() ||
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetMenuOrMenubarVisible()) {
        return;
    }
    auto& economy = gSaveContext.ship.livingHyrule;
    // Do not continue a deleted permanent-death save. Check disk only at a
    // payment boundary, rather than making a filesystem request every frame.
    if (economy.rentalFrames == kFramesPerRentPeriod - 1 &&
        !SaveManager::Instance->SaveFile_Exist(gSaveContext.fileNum)) {
        return;
    }
    if (const uint32_t rent = TickRent(economy); rent != 0) {
        Notification::Emit({ .message = "Kakariko rent: " + std::to_string(rent) + " rupees deposited." });
    }
}

static void RegisterLivingHyrule() {
    // ShipInit also runs on settings import; persistence hooks register once.
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    SaveManager::Instance->AddInitFunction(InitSave);
    SaveManager::Instance->AddLoadFunction("livingHyrule", 1, LoadSave);
    SaveManager::Instance->AddLoadFunction("livingHyrule", SECTION_VERSION_FALLBACK, PreserveUnreadableSave);
    SaveManager::Instance->AddSaveFunction("livingHyrule", 1, SaveSave, true, SECTION_PARENT_NONE, CanSave);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdateRent);
}

static RegisterShipInitFunc initFunc(RegisterLivingHyrule);

} // namespace LivingHyrule
