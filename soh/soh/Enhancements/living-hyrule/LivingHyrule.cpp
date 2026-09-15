#include "LivingHyrule.h"
#include "SaveCodec.h"
#include "ResidentActor.h"
#include "WorldResidents.h"
#include "ForestMountainResidents.h"
#include "WaterDesertResidents.h"

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

static int pendingVictoryFile = -1;
static bool victoryQueuedThisScene = false;

static void InitSave(bool) {
    pendingVictoryFile = -1;
    victoryQueuedThisScene = false;
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

static bool CanPersistVictory(int fileNum) {
    return GameInteractor::IsSaveLoaded(false) && IsSupportedAdventure() && LINK_IS_ADULT &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && fileNum >= 0 && fileNum <= 2 &&
           gSaveContext.fileNum == fileNum && IsValidState(gSaveContext.ship.livingHyrule) &&
           gSaveContext.ship.livingHyrule.enabled == 1 && SaveManager::Instance->SaveFile_Exist(fileNum);
}

static void QueueVictoryRecord(void*) {
    if (victoryQueuedThisScene || !CanPersistVictory(gSaveContext.fileNum) ||
        gPlayState->sceneNum != SCENE_GANON_BOSS) {
        return;
    }
    // This hook runs only on the genuine final-boss defeat event. Defer until
    // all handlers, including the engine's timestamp handler, have completed.
    pendingVictoryFile = gSaveContext.fileNum;
    victoryQueuedThisScene = true;
}

static void PersistVictoryRecord() {
    if (pendingVictoryFile < 0) {
        return;
    }
    const int fileNum = pendingVictoryFile;
    pendingVictoryFile = -1;
    if (!CanPersistVictory(fileNum)) {
        return;
    }
    auto& timestamp = gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_GANON];
    // A debug adventure can reach the real victory event with a zero timer.
    // Zero means "never defeated" in the existing persistent statistics format.
    if (timestamp == 0) {
        timestamp = 1;
    }
    // The ending has no unconditional normal save. Persist only statistics:
    // unsaved base-game progress, wallet and ledger keep their usual semantics.
    SaveManager::Instance->SaveSection(fileNum, SECTION_ID_STATS, true);
}

static bool IsKakarikoTradeOpen() {
    return LINK_IS_CHILD || CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW);
}

WorldProgress GetWorldProgress() {
    WorldProgress world;
    world.adult = LINK_IS_ADULT;
    world.forest = CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST);
    world.fire = CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE);
    world.water = CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER);
    world.shadow = CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW);
    world.spirit = CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT);
    world.gerudoMembership = CHECK_QUEST_ITEM(QUEST_GERUDO_CARD);
    world.ranchFreed = Flags_GetEventChkInf(EVENTCHKINF_EPONA_OBTAINED);
    // gameComplete also marks custom time-split completion and is reset on
    // loading a save. Only the persisted final-boss record proves victory.
    world.ganonDefeated = gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_GANON] != 0;
    return world;
}

static Region CurrentRegion() {
    switch (gPlayState->sceneNum) {
        case SCENE_MARKET_DAY:
        case SCENE_MARKET_NIGHT:
        case SCENE_MARKET_RUINS:
            return Region::Market;
        case SCENE_HYRULE_FIELD:
            return Region::Field;
        case SCENE_LON_LON_RANCH:
            return Region::Ranch;
        case SCENE_KOKIRI_FOREST:
            return Region::Forest;
        case SCENE_KAKARIKO_VILLAGE:
            return Region::Kakariko;
        case SCENE_GORON_CITY:
        case SCENE_DEATH_MOUNTAIN_TRAIL:
            return Region::Mountain;
        case SCENE_LAKE_HYLIA:
        case SCENE_ZORAS_DOMAIN:
        case SCENE_ZORAS_RIVER:
            return Region::Water;
        case SCENE_GERUDOS_FORTRESS:
        case SCENE_GERUDO_VALLEY:
            return Region::Desert;
        default:
            return Region::Count;
    }
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
    status.world = GetWorldProgress();
    status.currentRegion = CurrentRegion();

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
        case Result::Unavailable:
            return "Resolve this region's crisis before trading or repairing adult-era property.";
        case Result::NotOwned:
            return "Buy this property before commissioning its repairs.";
        case Result::AlreadyRepaired:
            return "This property is already repaired.";
    }
    return "The transaction could not be completed.";
}

static std::string ApplyAction(Action action, uint32_t amount, const Status& status) {
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
        case Action::BuyProperty:
        case Action::RepairProperty:
            if (amount >= kProperties.size())
                return "That property is not available.";
            if (status.currentRegion != kProperties[amount].region)
                return "Visit the property's region to complete this transaction.";
            return ExplainResult(action == Action::BuyProperty ? BuyProperty(economy, amount, status.world)
                                                               : RepairProperty(economy, amount, status.world));
    }
    return "The transaction could not be completed.";
}

std::string PerformAction(Action action, uint32_t amount) {
    const auto status = GetStatus();
    return status.canUseLedger ? ApplyAction(action, amount, status) : status.reason;
}

std::string PerformConversationAction(Actor* actor, uint16_t quoteTextId, Action action, uint32_t amount) {
    if (!GameInteractor::IsSaveLoaded(false) || !IsSupportedAdventure() || actor == nullptr ||
        actor->update == nullptr ||
        (!IsResidentActor(actor) && !IsWorldResidentActor(actor) && !IsForestMountainResidentActor(actor) &&
         !IsWaterDesertResidentActor(actor))) {
        return "Please speak directly to a Living Hyrule trader.";
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr || gPlayState->msgCtx.talkActor != actor || player->talkActor != actor ||
        !(player->stateFlags1 & PLAYER_STATE1_TALKING) || gPlayState->msgCtx.textId != quoteTextId ||
        Message_GetState(&gPlayState->msgCtx) != TEXT_STATE_CHOICE || gPlayState->msgCtx.choiceIndex != 0 ||
        !(gPlayState->state.input[0].press.button & BTN_A) || (gPlayState->state.input[0].cur.button & BTN_B) ||
        (gPlayState->state.input[0].press.button & BTN_CUP)) {
        return "The offer has expired. Speak to the trader again.";
    }
    // Ordinary talking sets IN_CUTSCENE. Allow only that exact owned dialogue;
    // do not clear player flags or use the ledger's no-dialogue readiness rule.
    if (gPlayState->pauseCtx.state != 0 || gPlayState->pauseCtx.debugState != 0 ||
        gPlayState->gameOverCtx.state != GAMEOVER_INACTIVE || gPlayState->transitionTrigger != TRANS_TRIGGER_OFF ||
        gPlayState->transitionMode != TRANS_MODE_OFF || gPlayState->csCtx.state != CS_STATE_IDLE ||
        player->csAction != PLAYER_CSACTION_NONE || IS_CUTSCENE_LAYER || player->unk_6AD == 4 ||
        (player->stateFlags1 &
         (PLAYER_STATE1_DEAD | PLAYER_STATE1_LOADING | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_ITEM_CS)) ||
        (player->stateFlags3 & PLAYER_STATE3_FLYING_WITH_HOOKSHOT) || gSaveContext.health <= 0 ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.rupeeAccumulator != 0) {
        return "We can finish this business when it is safe to talk.";
    }
    if (action == Action::Enable || action == Action::Disable || !IsValidState(gSaveContext.ship.livingHyrule) ||
        !SaveManager::Instance->SaveFile_Exist(gSaveContext.fileNum)) {
        return "This account is not available for trading.";
    }
    return ApplyAction(action, amount, GetStatus());
}

static void UpdateRent() {
    if (!IsSafeGameplay() || !IsSupportedAdventure() ||
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetMenuOrMenubarVisible()) {
        return;
    }
    auto& economy = gSaveContext.ship.livingHyrule;
    // Do not continue a deleted permanent-death save. Check disk only at a
    // payment boundary, rather than making a filesystem request every frame.
    bool paymentDue = economy.rentalFrames == kFramesPerRentPeriod - 1;
    for (uint32_t frames : economy.businessFrames)
        paymentDue |= frames == kFramesPerRentPeriod - 1;
    if (paymentDue && !SaveManager::Instance->SaveFile_Exist(gSaveContext.fileNum)) {
        return;
    }
    const uint32_t rent = IsKakarikoTradeOpen() ? TickRent(economy) : 0;
    if (rent != 0) {
        Notification::Emit({ .message = "Kakariko rent: " + std::to_string(rent) + " rupees deposited." });
    }
    if (const uint32_t income = TickBusinesses(economy, GetWorldProgress()); income != 0) {
        Notification::Emit({ .message = "Business income: " + std::to_string(income) + " rupees deposited." });
    }
}

static void ClearMarketThreats() {
    if (!IsSupportedAdventure() || !GameInteractor::IsSaveLoaded(false) || IS_CUTSCENE_LAYER)
        return;
    const auto& economy = gSaveContext.ship.livingHyrule;
    if (!ShouldClearMarketThreats(GetWorldProgress(), IsValidState(economy) && economy.enabled == 1,
                                  gPlayState->sceneNum == SCENE_MARKET_RUINS))
        return;
    // Clear every ruined-market Redead, including actors whose own update has
    // been culled offscreen. The world can then safely receive relief workers.
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (Actor* actor = gPlayState->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor->id == ACTOR_EN_RD && actor->update != nullptr)
                Actor_Kill(actor);
        }
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
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(ClearMarketThreats);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnBossDefeat>(ACTOR_BOSS_GANON2,
                                                                                  QueueVictoryRecord);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(PersistVictoryRecord);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) {
        pendingVictoryFile = -1;
        victoryQueuedThisScene = false;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([]() {
        pendingVictoryFile = -1;
        victoryQueuedThisScene = false;
    });
}

static RegisterShipInitFunc initFunc(RegisterLivingHyrule);

} // namespace LivingHyrule
