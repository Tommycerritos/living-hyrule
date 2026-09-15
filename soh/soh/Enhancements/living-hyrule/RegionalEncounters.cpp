#include "RegionalEncounters.h"
#include "RegionalEncounterPolicy.h"
#include "GraphicsCompatibilityPolicy.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Network/Anchor/Anchor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"
#include "soh/resource/type/Skeleton.h"
#include "soh/cvar_prefixes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/object_tite/object_tite.h"
#include "objects/object_reeba/object_reeba.h"
#include "overlays/actors/ovl_En_Tite/z_en_tite.h"
#include "overlays/actors/ovl_En_Reeba/z_en_reeba.h"
#include "overlays/actors/ovl_En_Encount1/z_en_encount1.h"
extern PlayState* gPlayState;
void EnReeba_Die(EnReeba* leever, PlayState* play);
}

namespace LivingHyrule {
namespace {

struct OwnedEncounter {
    Actor* actor = nullptr;
    PlayState* play = nullptr;
    const RegionalEncounterSite* site = nullptr;
    s16 actorId = -1;
    int32_t file = -1;
    uint32_t spawnedAt = 0;
    RegionalLeeverDeath leeverDeath;
    bool graphicsCompatible = true;
};

OwnedEncounter owned;
RegionalEncounterBudget budget;
bool tearingDown = false;

constexpr std::array<std::array<float, 2>, 9> footprint = { {
    { 0, 0 },
    { 50, 0 },
    { -50, 0 },
    { 0, 50 },
    { 0, -50 },
    { 36, 36 },
    { -36, 36 },
    { 36, -36 },
    { -36, -36 },
} };

EncounterPlace PlaceForScene(s16 scene) {
    switch (scene) {
        case SCENE_DEATH_MOUNTAIN_TRAIL:
            return EncounterPlace::Trail;
        case SCENE_ZORAS_RIVER:
            return EncounterPlace::River;
        case SCENE_DESERT_COLOSSUS:
            return EncounterPlace::Colossus;
        default:
            return EncounterPlace::None;
    }
}

bool ActiveGameplay(PlayState* play) {
    const auto* player = GET_PLAYER(play);
    return player != nullptr && !GameInteractor::IsGameplayPaused() && play->pauseCtx.debugState == 0 &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && gSaveContext.health > 0 &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->csCtx.state == CS_STATE_IDLE && !Player_InCsMode(play) &&
           Message_GetState(&play->msgCtx) == TEXT_STATE_NONE && play->msgCtx.msgMode == 0 &&
           gSaveContext.timerState == TIMER_STATE_OFF && gSaveContext.subTimerState == SUBTIMER_STATE_OFF &&
           gSaveContext.minigameState == 0 &&
           !(player->stateFlags1 & (PLAYER_STATE1_LOADING | PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_TALKING |
                                    PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_ITEM_CS |
                                    PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_ON_HORSE)) &&
           !(player->stateFlags2 & (PLAYER_STATE2_PAUSE_MOST_UPDATING | PLAYER_STATE2_FROZEN |
                                    PLAYER_STATE2_FORCED_VOID_OUT | PLAYER_STATE2_OCARINA_PLAYING));
}

RegionalEncounterContext CurrentContext() {
    RegionalEncounterContext context;
    context.enabled = CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleChallenge"), 0) != 0;
    context.realSave = GameInteractor::IsSaveLoaded(false);
    if (!context.realSave || gPlayState == nullptr || tearingDown)
        return context;
    context.supportedAdventure = IS_VANILLA || IS_MASTER_QUEST;
    context.normalScene = gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum >= 0 &&
                          gSaveContext.fileNum <= 2 && !IS_CUTSCENE_LAYER && gPlayState->roomCtx.curRoom.num == 0;
    context.gameplayActive = ActiveGameplay(gPlayState);
    context.enemyOverride = CVarGetInteger(CVAR_ENHANCEMENT("RandomizedEnemies"), 0) != 0 ||
                            CVarGetInteger(CVAR_ENHANCEMENT("RandomizedEnemySizes"), 0) != 0 ||
                            CVarGetInteger(CVAR_ENHANCEMENT("HyperEnemies"), 0) != 0 ||
                            CVarGetInteger(CVAR_ENHANCEMENT("LeeverSpawnRate"), 0) != 0 ||
                            (Anchor::Instance != nullptr && Anchor::Instance->isConnected);
    context.adult = LINK_IS_ADULT;
    context.daytime = IS_DAY;
    context.fireRestored = CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE);
    context.waterStone = CHECK_QUEST_ITEM(QUEST_ZORA_SAPPHIRE);
    context.spiritRestored = CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT);
    context.gerudoMembership = CHECK_QUEST_ITEM(QUEST_GERUDO_CARD);
    context.place = PlaceForScene(gPlayState->sceneNum);
    return context;
}

bool NativeWindow(PlayState* play, const RegionalEncounterSite& site, bool starting) {
    bool foundLeeverSpawner = false;
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (const Actor* actor = play->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor->update == nullptr || actor == owned.actor)
                continue;
            if (actor->id == ACTOR_EN_ENCOUNT2)
                return false;
            if (actor->id != ACTOR_EN_ENCOUNT1)
                continue;
            const auto* spawner = reinterpret_cast<const EnEncount1*>(actor);
            if (site.enemy != RegionalEnemyKind::SmallLeever || spawner->spawnType != SPAWNER_LEEVER ||
                !RegionalSpawnerQuiet(spawner->curNumSpawn, spawner->bigLeever != nullptr, spawner->timer, starting))
                return false;
            foundLeeverSpawner = true;
        }
    }
    return site.enemy != RegionalEnemyKind::SmallLeever || foundLeeverSpawner;
}

NativeGraphicsResource InspectStructuralResource(const std::string& name) {
    const std::string path = name.starts_with("__OTR__") ? name.substr(7) : name;
    const auto archives = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager();
    const auto archive = archives->GetArchiveFromFile(path);
    // A binary model replacement can report IsCustom=false. Check the owning
    // archive and .meta aliases as well, before loading any replacement data.
    return { archive != nullptr, archive != nullptr && archive->HasGameVersion(), archives->HasFile(path + ".meta"),
             archives->HasFile("alt/" + path) || archives->HasFile("alt/" + path + ".meta") };
}

bool CompatibleAssets(PlayState* play, const RegionalEncounterSite& site) {
    const s16 objectId = site.enemy == RegionalEnemyKind::SmallLeever ? OBJECT_REEBA : OBJECT_TITE;
    const int bank = Object_GetIndex(&play->objectCtx, objectId);
    if (bank < 0 || !Object_IsLoaded(&play->objectCtx, bank))
        return false;
    const char* skeleton =
        site.enemy == RegionalEnemyKind::SmallLeever ? object_reeba_Skel_001EE8 : object_tite_Skel_003A20;
    const char* collision =
        site.place == EncounterPlace::Trail   ? "scenes/shared/spot16_scene/spot16_sceneCollisionHeader_003D10"
        : site.place == EncounterPlace::River ? "scenes/shared/spot03_scene/spot03_sceneCollisionHeader_006580"
                                              : "scenes/shared/spot11_scene/spot11_sceneCollisionHeader_004EE4";
    const bool alternatives = ResourceMgr_IsAltAssetsEnabled();
    const std::array<const char*, 2> structure = { skeleton, collision };
    // Include every animation used during this native enemy's lifetime, not
    // just its initial idle. Alternate textures and Link models remain free.
    const std::array<const char*, 6> tektiteAnimations = {
        object_tite_Anim_0004F8, object_tite_Anim_00069C, object_tite_Anim_00083C,
        object_tite_Anim_000A14, object_tite_Anim_000C70, object_tite_Anim_0012E4,
    };
    const std::array<const char*, 1> leeverAnimations = { object_reeba_Anim_0001E4 };
    if (!NativeEncounterGraphicsAvailable(structure, alternatives, InspectStructuralResource) ||
        !(site.enemy == RegionalEnemyKind::SmallLeever
              ? NativeEncounterGraphicsAvailable(leeverAnimations, alternatives, InspectStructuralResource)
              : NativeEncounterGraphicsAvailable(tektiteAnimations, alternatives, InspectStructuralResource)))
        return false;
    const auto rig = std::dynamic_pointer_cast<SOH::Skeleton>(ResourceMgr_GetResourceByNameHandlingMQ(skeleton));
    const size_t limbs = site.enemy == RegionalEnemyKind::SmallLeever ? 17 : 24;
    if (rig == nullptr || rig->type != SOH::SkeletonType::Normal || rig->limbCount != limbs ||
        rig->limbTable.size() != limbs || rig->skeletonHeaderSegments.size() != limbs ||
        !NativeEncounterGraphicsAvailable(rig->limbTable, alternatives, InspectStructuralResource))
        return false;
    for (size_t i = 0; i < limbs; ++i) {
        const auto limb = ResourceMgr_GetResourceByNameHandlingMQ(rig->limbTable[i].c_str());
        // A native skeleton cached while alternate limbs were enabled can still
        // hold those pointers after the pack is disabled. Do not reuse that rig.
        if (limb == nullptr || limb->GetRawPointer() != rig->skeletonHeaderSegments[i])
            return false;
    }
    return true;
}

bool FloorAt(PlayState* play, const RegionalEncounterSite& site, float x, float z, float& height) {
    Vec3f probe = { x, site.floorY + 40.0f, z };
    CollisionPoly* floor = nullptr;
    s32 bgId = BGCHECK_SCENE;
    height = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &bgId, &probe);
    if (floor == nullptr || bgId != BGCHECK_SCENE || !std::isfinite(height) || height <= BGCHECK_Y_MIN ||
        std::abs(height - site.floorY) > 25 || floor->normal.y < 30000 ||
        SurfaceType_GetSceneExitIndex(&play->colCtx, floor, bgId) != 0 ||
        func_80041D70(&play->colCtx, floor, bgId) != 0 || SurfaceType_IsConveyor(&play->colCtx, floor, bgId))
        return false;
    const u32 type = SurfaceType_GetFloorType(&play->colCtx, floor, bgId);
    if (site.enemy == RegionalEnemyKind::SmallLeever ? (type != 4 && type != 7) : type != 0)
        return false;
    WaterBox* water = nullptr;
    float waterHeight = 0;
    const bool wet = WaterBox_GetSurface1(play, &play->colCtx, x, z, &waterHeight, &water);
    if (site.enemy == RegionalEnemyKind::BlueTektite)
        return wet && std::isfinite(waterHeight) && std::abs(waterHeight - site.y) <= 1 && waterHeight - height >= 10 &&
               waterHeight - height <= 80;
    return !wet || (std::isfinite(waterHeight) && waterHeight < height - 2);
}

bool GroundClear(PlayState* play, const RegionalEncounterSite& site, float x, float z, bool body) {
    float height;
    if (!FloorAt(play, site, x, z, height))
        return false;
    for (const auto& offset : footprint) {
        float edge;
        if (!FloorAt(play, site, x + offset[0], z + offset[1], edge) || std::abs(edge - height) > 8)
            return false;
    }
    if (body) {
        const float base = site.enemy == RegionalEnemyKind::BlueTektite ? site.y : height;
        for (float dy : { 35.0f, 75.0f, 110.0f }) {
            Vec3f center = { x, base + dy, z };
            if (BgCheck_SphVsFirstPoly(&play->colCtx, &center, 30.0f))
                return false;
        }
    }
    return true;
}

bool ActorsClear(PlayState* play, const RegionalEncounterSite& site, const Vec3f& position, bool starting) {
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (const Actor* actor = play->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor == owned.actor || actor->update == nullptr || actor->category == ACTORCAT_PLAYER)
                continue;
            // Non-solid rupee triggers and wind flags do not obstruct the river
            // or desert pocket. Their collectible/switch state is never touched.
            if (actor->id == ACTOR_EN_WONDER_ITEM || actor->id == ACTOR_EN_HATA || actor->id == ACTOR_EN_ENCOUNT1)
                continue;
            const bool hostile = (actor->flags & ACTOR_FLAG_HOSTILE) != 0 || category == ACTORCAT_ENEMY ||
                                 category == ACTORCAT_BOSS || actor->id == ACTOR_EN_REEBA;
            const bool important = category == ACTORCAT_NPC || category == ACTORCAT_DOOR;
            const float vertical = hostile || important ? 250.0f : 160.0f;
            if (std::abs(actor->world.pos.y - position.y) > vertical)
                continue;
            float clearance = hostile ? (starting ? 600.0f : 300.0f) : important ? 400.0f : 100.0f;
            if (category == ACTORCAT_EXPLOSIVE)
                clearance = 250.0f;
            clearance = std::max(clearance, static_cast<float>(actor->colChkInfo.cylRadius) + 60.0f);
            const float dx = actor->world.pos.x - position.x, dz = actor->world.pos.z - position.z;
            if (dx * dx + dz * dz < clearance * clearance)
                return false;
        }
    }
    // Keep the Colossus temple/entrance approach around z=0 open. The Trail
    // overlook is vertically separate from the lower required ascent; River's
    // pocket stays in the water, away from its north-bank frog game and workers.
    return site.place != EncounterPlace::Colossus || position.z >= 350;
}

bool PendingDeath(const Actor* actor) {
    if (actor->colChkInfo.health == 0)
        return true;
    if (actor->id != ACTOR_EN_TITE)
        return false;
    const auto* tektite = reinterpret_cast<const EnTite*>(actor);
    return !CanDismissRegionalEnemy(false, tektite->bodyBreak.val != BODYBREAK_STATUS_FINISHED);
}

void BeforeEnemyUpdate(void* pointer, bool* shouldUpdate) {
    auto* actor = static_cast<Actor*>(pointer);
    if (!IsRegionalEncounterActor(actor) || !*shouldUpdate || actor->update == nullptr || tearingDown)
        return;
    // Never interrupt breakup allocation/drawing/part spawning. Ordinary scene
    // teardown still owns the actor and its arena, just as for a vanilla enemy.
    if (PendingDeath(actor))
        return;
    const auto context = CurrentContext();
    const auto& site = *owned.site;
    const auto& position = actor->world.pos;
    const bool retained = owned.graphicsCompatible && RegionalEncounterAllowed(context) &&
                          context.place == site.place && owned.file == gSaveContext.fileNum &&
                          gPlayState->gameplayFrames - owned.spawnedAt < site.lifetime &&
                          InsideRegionalEncounterPocket(site, position.x, position.y, position.z) &&
                          NativeWindow(gPlayState, site, false) && ActorsClear(gPlayState, site, position, false) &&
                          GroundClear(gPlayState, site, position.x, position.z, false);
    if (!retained) {
        Actor_Kill(actor);
        *shouldUpdate = false;
    }
}

void OnOwnedDestroy(void* pointer) {
    auto* actor = static_cast<Actor*>(pointer);
    if (actor != owned.actor)
        return;
    // These native profiles use embedded pose tables. SkelAnime_Free would
    // incorrectly free those arrays; unregister only the tracked patcher entry.
    if (actor->id == ACTOR_EN_TITE)
        ResourceMgr_UnregisterSkeleton(&reinterpret_cast<EnTite*>(actor)->skelAnime);
    else if (actor->id == ACTOR_EN_REEBA)
        ResourceMgr_UnregisterSkeleton(&reinterpret_cast<EnReeba*>(actor)->skelanime);
    owned = {};
}

void AfterLeeverUpdate(void* pointer) {
    auto* actor = static_cast<Actor*>(pointer);
    if (!IsRegionalEncounterActor(actor) || actor->id != ACTOR_EN_REEBA || tearingDown)
        return;
    const auto* leever = reinterpret_cast<const EnReeba*>(actor);
    const bool terminal = actor->parent == nullptr && leever->isBig == 0 && leever->actionfunc == EnReeba_Die &&
                          leever->waitTimer == 0 && std::isfinite(leever->scale) && leever->scale < 0.01f;
    const auto completion = FinishRegionalLeeverDeath(owned.leeverDeath, terminal);
    if (completion == RegionalLeeverCompletion::None)
        return;
    // OnActorUpdate runs after the native update. EnReeba_Die has now produced
    // exactly one normal drop/effect, but its Actor_Kill is inside parent!=NULL.
    // Finish only our parentless instance before it can repeat that reward.
    Actor_Kill(actor);
    // The native stun-death path already announces defeat before shrinking;
    // the ordinary weapon path needs the missing terminal notification once.
    if (completion == RegionalLeeverCompletion::KillAndNotify)
        GameInteractor_ExecuteOnEnemyDefeat(actor);
}

void UpdateRegionalEncounters() {
    const auto context = CurrentContext();
    if (!context.realSave || tearingDown)
        return;
    AdvanceRegionalEncounterGrace(budget);
    const auto* site = GetRegionalEncounterSite(context.place);
    if (site == nullptr || owned.actor != nullptr || budget.attempted || !RegionalEncounterAllowed(context))
        return;
    const auto& playerPosition = GET_PLAYER(gPlayState)->actor.world.pos;
    if (!ConsumeRegionalEncounterAttempt(
            budget, context,
            RegionalEncounterPlayerInRange(*site, playerPosition.x, playerPosition.y, playerPosition.z),
            NativeWindow(gPlayState, *site, true)))
        return;
    const Vec3f position = { site->x, site->y, site->z };
    if (!CompatibleAssets(gPlayState, *site) || !GroundClear(gPlayState, *site, site->x, site->z, true) ||
        !ActorsClear(gPlayState, *site, position, true) || Flags_GetClear(gPlayState, 0))
        return;
    const s16 id = site->enemy == RegionalEnemyKind::SmallLeever ? ACTOR_EN_REEBA : ACTOR_EN_TITE;
    const s16 params = site->enemy == RegionalEnemyKind::SmallLeever   ? 0
                       : site->enemy == RegionalEnemyKind::BlueTektite ? -2
                                                                       : -1;
    // Parent must remain null: the native destructors assume any parent is an
    // EnEncount1. Require the real loaded object bank; never force a spawn flag.
    Actor* actor =
        Actor_Spawn(&gPlayState->actorCtx, gPlayState, id, position.x, position.y, position.z, 0, 0, 0, params);
    if (actor == nullptr)
        return;
    owned = { actor, gPlayState, site, id, gSaveContext.fileNum, gPlayState->gameplayFrames, {} };
    // Keep this short-lived native rig fixed if the owner toggles a model pack
    // mid-encounter. Texture changes still apply. Native death/BodyBreak can
    // finish using the original pose tables without the global skeleton patcher
    // swapping a different rig into their embedded buffers.
    ResourceMgr_UnregisterSkeleton(id == ACTOR_EN_TITE ? &reinterpret_cast<EnTite*>(actor)->skelAnime
                                                       : &reinterpret_cast<EnReeba*>(actor)->skelanime);
    // Remove only this added instance from vanilla enemy-room accounting. Set
    // room first so changing category cannot set the original temporary-clear.
    actor->room = -1;
    if (actor->category == ACTORCAT_ENEMY)
        Actor_ChangeCategory(gPlayState, &gPlayState->actorCtx, actor, ACTORCAT_MISC);
}

RegisterShipInitFunc initEncounters(RegisterRegionalEncounters);

} // namespace

bool IsRegionalEncounterActor(const Actor* actor) {
    return actor != nullptr && actor == owned.actor && owned.play == gPlayState && actor->id == owned.actorId &&
           actor->update != nullptr;
}

void RegisterRegionalEncounters() {
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) {
        // OnSceneInit follows the previous play's actor teardown. Toggling the
        // challenge, killing an enemy, or failing allocation never resets this.
        owned = {};
        budget = {};
        tearingDown = false;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { tearingDown = true; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdateRegionalEncounters);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnAssetAltChange>([] {
        if (IsRegionalEncounterActor(owned.actor) && !tearingDown)
            owned.graphicsCompatible = CompatibleAssets(gPlayState, *owned.site);
    });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(ACTOR_EN_REEBA, AfterLeeverUpdate);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnEnemyDefeat>(ACTOR_EN_REEBA, [](void* actor) {
        if (actor == owned.actor)
            owned.leeverDeath.defeatNotified = true;
    });
    for (s16 id : { static_cast<s16>(ACTOR_EN_TITE), static_cast<s16>(ACTOR_EN_REEBA) }) {
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorUpdate>(id, BeforeEnemyUpdate);
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorDestroy>(id, OnOwnedDestroy);
    }
}

} // namespace LivingHyrule
