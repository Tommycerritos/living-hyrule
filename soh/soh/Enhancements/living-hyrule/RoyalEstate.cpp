#include "RoyalEstate.h"
#include "RoyalEstatePolicy.h"
#include "LivingHyrule.h"
#include "MarketRestoration.h"
#include "MarketRestorationPolicy.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"
#include "soh/resource/type/CollisionHeader.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetAlternateHeaders.h"
#include "soh/resource/type/scenecommand/SetCollisionHeader.h"
#include "soh/resource/type/scenecommand/SetEntranceList.h"
#include "soh/resource/type/scenecommand/SetExitList.h"
#include "soh/resource/type/scenecommand/SetMesh.h"
#include "soh/resource/type/scenecommand/SetRoomList.h"
#include "soh/resource/type/scenecommand/SetStartPositionList.h"
#include "soh/resource/type/scenecommand/SetTimeSettings.h"
#include <array>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {
constexpr const char* kScenePath = "scenes/shared/nakaniwa_scene/nakaniwa_scene";
constexpr const char* kRoomPath = "scenes/shared/nakaniwa_scene/nakaniwa_room_0";
constexpr const char* kCollisionPath = "scenes/shared/nakaniwa_scene/nakaniwa_sceneCollisionHeader_001BC8";
constexpr const char* kOpaquePath = "scenes/shared/nakaniwa_scene/nakaniwa_room_0DL_007178";
constexpr const char* kTranslucentPath = "scenes/shared/nakaniwa_scene/nakaniwa_room_0DL_014E98";

struct GardenAssets {
    std::shared_ptr<SOH::Scene> scene;
    std::shared_ptr<SOH::Scene> room;
    std::shared_ptr<SOH::CollisionHeader> collision;
    std::vector<std::shared_ptr<Ship::IResource>> art;
};
// Retained through PlayState/actor teardown. Only the loaded scene's pointer is
// redirected; the archive's child/story exit list and all quest flags survive.
std::unique_ptr<GardenAssets> assets;
bool preparationAttempted = false;
std::array<int16_t, 2> gardenExits = { kRoyalGardenReturnEntrance, 0 };
PlayState* activePlay = nullptr;
int activeFile = -1;
bool verifiedVisit = false;

uint64_t CollisionFingerprint(const SOH::CollisionHeader& source) {
    uint64_t hash = kRestorationHashStart;
    const auto word = [&](int64_t value) { hash = RestorationHashWord(hash, static_cast<uint32_t>(value)); };
    const auto vector = [&](const Vec3s& value) {
        word(value.x);
        word(value.y);
        word(value.z);
    };
    vector(source.collisionHeaderData.minBounds);
    vector(source.collisionHeaderData.maxBounds);
    word(source.vertices.size());
    for (const auto& value : source.vertices)
        vector(value);
    word(source.polygons.size());
    for (const auto& value : source.polygons) {
        word(value.type);
        word(value.flags_vIA);
        word(value.flags_vIB);
        word(value.vIC);
        vector(value.normal);
        word(value.dist);
    }
    word(source.surfaceTypes.size());
    for (const auto& value : source.surfaceTypes) {
        word(value.data[0]);
        word(value.data[1]);
    }
    word(source.camData.size());
    for (size_t index = 0; index < source.camData.size(); ++index) {
        word(source.camData[index].cameraSType);
        word(source.camData[index].numCameras);
        word(source.camPosDataIndices.at(index));
    }
    word(source.camPosData.size());
    for (const auto& value : source.camPosData)
        vector(value);
    word(source.waterBoxes.size());
    for (const auto& value : source.waterBoxes) {
        word(value.xMin);
        word(value.ySurface);
        word(value.zMin);
        word(value.xLength);
        word(value.zLength);
        word(value.properties);
    }
    return hash;
}

void Require(bool condition) {
    if (!condition)
        throw std::runtime_error("Unsupported royal garden resource");
}

template <typename T> std::shared_ptr<T> Load(const char* path) {
    auto resource = std::dynamic_pointer_cast<T>(ResourceMgr_GetResourceByNameHandlingMQ(path));
    Require(resource != nullptr);
    return resource;
}

template <typename T> std::shared_ptr<T> Command(const SOH::Scene& scene, SOH::SceneCommandID id) {
    std::shared_ptr<T> result;
    for (const auto& command : scene.commands) {
        Require(command != nullptr);
        if (command->cmdId == id) {
            Require(result == nullptr);
            result = std::dynamic_pointer_cast<T>(command);
        }
    }
    Require(result != nullptr);
    return result;
}

void ValidateOrdinaryHeader(const SOH::Scene& scene) {
    const auto alternates = Command<SOH::SetAlternateHeaders>(scene, SOH::SceneCommandID::SetAlternateHeaders);
    Require(alternates->headers.size() >= 3);
    for (size_t index = 0; index < 3; ++index)
        Require(alternates->headers[index] == nullptr);
    for (const auto& command : scene.commands) {
        // Room transition actors initialize before AfterSceneCommands. The
        // native garden has none; never accept a replacement that adds them.
        Require(command->cmdId != SOH::SceneCommandID::SetTransitionActorList &&
                command->cmdId != SOH::SceneCommandID::SetCutscenes);
    }
}

void RetainGardenArt(GardenAssets& candidate) {
    constexpr const char* prefix = "scenes/shared/nakaniwa_scene/";
    const auto files = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->ListFiles(
        std::string(prefix) + "*");
    Require(files != nullptr);
    std::vector<std::string> names;
    for (const auto& name : *files) {
        if (name.compare(0, std::char_traits<char>::length(prefix), prefix) == 0 &&
            (name.size() < 5 || name.substr(name.size() - 5) != ".meta"))
            names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    uint64_t hash = kRestorationHashStart;
    for (const auto& name : names) {
        for (unsigned char byte : name)
            hash = (hash ^ byte) * UINT64_C(1099511628211);
        hash *= UINT64_C(1099511628211);
    }
    Require(names.size() == 105 && hash == UINT64_C(0x1ce2f8b9e056309a));
    for (const auto& name : names) {
        // Four exporter address labels have no data file. The corresponding
        // actor lists are contained in the retained room command resources.
        if (name == std::string(prefix) + "nakaniwa_room_0ActorEntry_000070" ||
            name == std::string(prefix) + "nakaniwa_room_0ActorEntry_000164" ||
            name == std::string(prefix) + "nakaniwa_room_0ActorEntry_0001F4" ||
            name == std::string(prefix) + "nakaniwa_room_0ActorEntry_000284")
            continue;
        auto resource = ResourceMgr_GetResourceByNameHandlingMQ(name.c_str());
        Require(resource != nullptr);
        candidate.art.push_back(resource);
    }
}

bool PrepareAssets() {
    if (preparationAttempted)
        return assets != nullptr;
    preparationAttempted = true;
    try {
        auto candidate = std::make_unique<GardenAssets>();
        candidate->scene = Load<SOH::Scene>(kScenePath);
        candidate->room = Load<SOH::Scene>(kRoomPath);
        candidate->collision = Load<SOH::CollisionHeader>(kCollisionPath);
        Require(CollisionFingerprint(*candidate->collision) == kRoyalGardenCollisionHash);
        ValidateOrdinaryHeader(*candidate->scene);
        ValidateOrdinaryHeader(*candidate->room);
        const auto collision =
            Command<SOH::SetCollisionHeader>(*candidate->scene, SOH::SceneCommandID::SetCollisionHeader);
        Require(collision->fileName == kCollisionPath);
        const auto rooms = Command<SOH::SetRoomList>(*candidate->scene, SOH::SceneCommandID::SetRoomList);
        Require(rooms->fileNames.size() == 1 && rooms->fileNames[0] == kRoomPath);
        const auto exits = Command<SOH::SetExitList>(*candidate->scene, SOH::SceneCommandID::SetExitList);
        Require(exits->exits == std::vector<uint16_t>{ 0x296, 0 });
        const auto entrances = Command<SOH::SetEntranceList>(*candidate->scene, SOH::SceneCommandID::SetEntranceList);
        Require(entrances->entrances.size() == 2);
        for (size_t index = 0; index < 2; ++index)
            Require(entrances->entrances[index].spawn == index && entrances->entrances[index].room == 0);
        const auto starts =
            Command<SOH::SetStartPositionList>(*candidate->scene, SOH::SceneCommandID::SetStartPositionList);
        Require(starts->startPositions.size() == 2);
        constexpr Vec3s positions[] = { { 604, 44, 8 }, { -429, 84, 0 } };
        for (size_t index = 0; index < 2; ++index) {
            const auto& start = starts->startPositions[index];
            Require(start.id == ACTOR_PLAYER && start.pos.x == positions[index].x &&
                    start.pos.y == positions[index].y && start.pos.z == positions[index].z && start.rot.x == 0 &&
                    start.rot.y == -16384 && start.rot.z == 0 && start.params == (index == 0 ? 0xFFF : 0xDFF));
        }
        const auto mesh = Command<SOH::SetMesh>(*candidate->room, SOH::SceneCommandID::SetMesh);
        Require(mesh->meshHeader.base.type == 0 && mesh->dlists.size() == 1 &&
                mesh->opaPaths == std::vector<std::string>{ std::string("__OTR__") + kOpaquePath } &&
                mesh->xluPaths == std::vector<std::string>{ std::string("__OTR__") + kTranslucentPath });
        const auto time = Command<SOH::SetTimeSettings>(*candidate->room, SOH::SceneCommandID::SetTimeSettings);
        Require(time->settings.hour == 255 && time->settings.minute == 255 && time->settings.timeIncrement == 0);
        for (const char* path : { kOpaquePath, kTranslucentPath }) {
            auto art = ResourceMgr_GetResourceByNameHandlingMQ(path);
            Require(art != nullptr && art->GetRawPointer() != nullptr);
            candidate->art.push_back(art);
        }
        RetainGardenArt(*candidate);
        assets = std::move(candidate);
    } catch (const std::exception&) { assets.reset(); }
    return assets != nullptr;
}

bool RealFile() {
    return gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && SaveManager::Instance != nullptr &&
           SaveManager::Instance->SaveFile_Exist(gSaveContext.fileNum);
}

bool NativeReturnEntrance() {
    for (int layer : { 2, 3 }) {
        const auto& approach = gEntranceTable[kRoyalGardenReturnEntrance + layer];
        if (approach.scene != SCENE_OUTSIDE_GANONS_CASTLE || approach.spawn != 0)
            return false;
    }
    return true;
}

bool NativeEntrances() {
    for (int layer : { 2, 3 }) {
        const auto& garden = gEntranceTable[kRoyalGardenEntrance + layer];
        if (garden.scene != SCENE_CASTLE_COURTYARD_ZELDA || garden.spawn != 0)
            return false;
    }
    return NativeReturnEntrance();
}

bool OrdinaryGarden(PlayState* play) {
    return play != nullptr && play == gPlayState && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           RoyalGardenLoadAllowed(IS_VANILLA || IS_MASTER_QUEST, RealFile(), LINK_IS_ADULT, !IS_CUTSCENE_LAYER,
                                  play->sceneNum, play->curSpawn, gSaveContext.entranceIndex);
}

void PrepareVisit(PlayState* play, CollisionHeader** header) {
    activePlay = nullptr;
    activeFile = -1;
    verifiedVisit = false;
    // Deliberately independent of the economy switch/paid state: normal save,
    // death and Remember Save Location can reload a previously visited garden.
    // Even an empty resumed visit must not initialize Zelda's original story AI.
    if (!OrdinaryGarden(play))
        return;
    activePlay = play;
    activeFile = gSaveContext.fileNum;
    // Missing/altered art must not restore native quest AI on a remembered
    // visit. Its escape shell is mandatory; invitations, deeds and household
    // placement require the separate complete-resource proof.
    verifiedVisit = PrepareAssets() && NativeEntrances() && header != nullptr &&
                    *header == reinterpret_cast<CollisionHeader*>(assets->collision->GetPointer());
}

void SanitizeGarden(int16_t) {
    if (activePlay == nullptr || activePlay != gPlayState || activeFile != gSaveContext.fileNum)
        return;
    // The first call follows scene commands; room calls follow the actor-list
    // command but precede Actor_UpdateAll's deferred actor initialization. The
    // verified native header has no transition-door actors. A separate pre-Init
    // guard suppresses the original story actors even in an unprepared shell.
    activePlay->numSetupActors = 0;
    activePlay->setupActorList = nullptr;
    activePlay->setupExitList = gardenExits.data();
}

bool SafeTravel() {
    if (!GameInteractor::IsSaveLoaded(false) || !RealFile() || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        IS_CUTSCENE_LAYER || !LINK_IS_ADULT || !(IS_VANILLA || IS_MASTER_QUEST))
        return false;
    const auto* play = gPlayState;
    const auto* player = GET_PLAYER(play);
    return player != nullptr && gSaveContext.health > 0 && !GameInteractor::IsGameplayPaused() &&
           play->pauseCtx.debugState == 0 && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->csCtx.state == CS_STATE_IDLE && gSaveContext.cutsceneTrigger == 0 &&
           gSaveContext.cutsceneIndex < 0xFFF0 && !Player_InCsMode(gPlayState) &&
           (gSaveContext.nextCutsceneIndex == 0xFFEF || gSaveContext.nextCutsceneIndex == 0) &&
           Message_GetState(&gPlayState->msgCtx) == TEXT_STATE_NONE &&
           !(player->stateFlags1 &
             (PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_CARRYING_ACTOR |
              PLAYER_STATE1_IN_WATER | PLAYER_STATE1_HANGING_OFF_LEDGE | PLAYER_STATE1_CLIMBING_LEDGE |
              PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_DAMAGED)) &&
           (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && player->actor.velocity.y <= 0.0f &&
           player->actor.parent == nullptr && gSaveContext.rupeeAccumulator == 0;
}

void TravelTo(int entrance) {
    // Ordinary scene transition, without save, age spoofing, story flags, or
    // changing the return/death respawn records. Player_Init sets those normally.
    gSaveContext.nextCutsceneIndex = 0;
    gPlayState->nextEntranceIndex = entrance;
    gPlayState->transitionType = TRANS_TYPE_FADE_BLACK;
    gPlayState->transitionTrigger = TRANS_TRIGGER_START;
}

void RegisterRoyalEstate() {
    static bool registered = false;
    if (registered || GameInteractor::Instance == nullptr)
        return;
    registered = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneCollisionLoad>(PrepareVisit);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::AfterSceneCommands>(SCENE_CASTLE_COURTYARD_ZELDA,
                                                                                        SanitizeGarden);
    for (int actorId : { ACTOR_EN_WONDER_ITEM, ACTOR_EN_HEISHI2, ACTOR_DEMO_KANKYO, ACTOR_DEMO_EFFECT, ACTOR_EN_ZL4,
                         ACTOR_EN_ZL1, ACTOR_DEMO_IM, ACTOR_EN_VIEWER }) {
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorInit>(
            actorId, [](void* pointer, bool* should) {
                if (!OrdinaryGarden(gPlayState))
                    return;
                // Actor_Kill follows a rejected Init. Do not run an original
                // destructor against a collider/skeleton that never initialized.
                static_cast<Actor*>(pointer)->destroy = nullptr;
                *should = false;
            });
    }
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        activePlay = nullptr;
        activeFile = -1;
        verifiedVisit = false;
    });
}
RegisterShipInitFunc initRoyalEstate(RegisterRoyalEstate);
} // namespace

bool IsRoyalEstateActive() {
    return activePlay != nullptr && activePlay == gPlayState && activeFile == gSaveContext.fileNum &&
           activePlay->sceneNum == SCENE_CASTLE_COURTYARD_ZELDA && !IS_CUTSCENE_LAYER &&
           activePlay->setupExitList == gardenExits.data();
}

RoyalEstateReadiness GetRoyalEstateReadiness() {
    if (!GameInteractor::IsSaveLoaded(false) || !RealFile())
        return RoyalEstateReadiness::NoLoadedGame;
    if (!(IS_VANILLA || IS_MASTER_QUEST))
        return RoyalEstateReadiness::UnsupportedAdventure;
    if (gSaveContext.gameMode != GAMEMODE_NORMAL || IS_CUTSCENE_LAYER || !LINK_IS_ADULT ||
        !RoyalGardenOriginAllowed(gPlayState->sceneNum, IsMarketRestorationActive(), IsRoyalEstateActive()))
        return RoyalEstateReadiness::WrongPlace;
    const auto& economy = gSaveContext.ship.livingHyrule;
    const bool victory = GetWorldProgress().ganonDefeated;
    const bool enabled = IsValidState(economy) && economy.enabled == 1;
    if (!RoyalGardenVisitUnlocked(true, true, true, true, victory, enabled, economy.marketRestored == 1)) {
        if (!victory)
            return RoyalEstateReadiness::AwaitingVictory;
        return enabled ? RoyalEstateReadiness::MarketNotRestored : RoyalEstateReadiness::EconomyUnavailable;
    }
    if (!NativeEntrances())
        return RoyalEstateReadiness::UnsafeEntrance;
    if (!PrepareAssets() || (IsRoyalEstateActive() && !verifiedVisit))
        return RoyalEstateReadiness::ResourcesUnavailable;
    return RoyalEstateReadiness::Ready;
}

const char* RoyalEstateReadinessText(RoyalEstateReadiness readiness) {
    switch (readiness) {
        case RoyalEstateReadiness::Ready:
            return "The royal garden is open. Zelda receives visitors by day.";
        case RoyalEstateReadiness::NoLoadedGame:
            return "Load a saved adventure to visit the royal garden.";
        case RoyalEstateReadiness::UnsupportedAdventure:
            return "The royal garden is available in a normal adventure or Master Quest.";
        case RoyalEstateReadiness::WrongPlace:
            return "Visit from the castle approach or restored Market as adult Link.";
        case RoyalEstateReadiness::AwaitingVictory:
            return "The royal garden opens after Ganon is defeated.";
        case RoyalEstateReadiness::EconomyUnavailable:
            return "Enable a readable Living Hyrule ledger for this adventure.";
        case RoyalEstateReadiness::MarketNotRestored:
            return "Fund the Market's restoration before visiting the royal garden.";
        case RoyalEstateReadiness::ResourcesUnavailable:
            return "The native royal garden could not be prepared for a safe visit.";
        case RoyalEstateReadiness::UnsafeEntrance:
            return "The royal garden's entrance or return route is unavailable.";
    }
    return "The royal garden is unavailable.";
}

std::string EnterRoyalEstate() {
    if (IsRoyalEstateActive())
        return "You are already in the royal garden.";
    const auto readiness = GetRoyalEstateReadiness();
    if (readiness != RoyalEstateReadiness::Ready)
        return RoyalEstateReadinessText(readiness);
    if (!SafeTravel())
        return "Finish the conversation or pause screen and stand safely on foot before traveling.";
    TravelTo(kRoyalGardenEntrance);
    return "Traveling to the royal garden. Use the east doorway to return to the castle approach.";
}

std::string ReturnFromRoyalEstate() {
    if (!IsRoyalEstateActive())
        return "You are not in the royal garden.";
    if (!SafeTravel() || !NativeReturnEntrance())
        return "Finish the conversation or pause screen and stand safely on foot before returning.";
    TravelTo(kRoyalGardenReturnEntrance);
    return "Returning to the castle approach.";
}

} // namespace LivingHyrule
