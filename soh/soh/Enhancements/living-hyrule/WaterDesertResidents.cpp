#include "WaterDesertResidents.h"
#include "LivingHyrule.h"
#include "TradeDialogue.h"

#include "soh/ActorDB.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/frame_interpolation.h"
#include <array>
#include <cmath>
#include <string>
#include <type_traits>
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/object_zo/object_zo.h"
#include "objects/object_ge1/object_ge1.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {
constexpr uint16_t kQuoteText = 0x9800;
constexpr uint16_t kReplyText = 0x9820;
constexpr int kResidentCount = static_cast<int>(WaterDesertResidentId::Count);
constexpr float kRadians = 3.14159265358979323846f / 32768.0f;
int actorIds[2] = { -1, -1 };
uint8_t spawnCooldown = 20;

// One raw arena layout supports both native skeleton sizes. Only the first 16
// table entries are used by GE1; Zora uses all 20. There are no owning members.
struct WaterDesertActor {
    Actor actor;
    SkelAnime skelAnime;
    ColliderCylinder collider;
    NpcInteractInfo interact;
    Vec3s joints[20];
    Vec3s morphs[20];
    TradeDialogueState trade;
    s16 talkState;
    u16 ticks;
    uint8_t eyeIndex;
    bool initialized;
};
static_assert(std::is_trivial_v<WaterDesertActor>);
static_assert(std::is_standard_layout_v<WaterDesertActor>);

bool ValidIdentity(const Actor* actor) {
    return actor != nullptr && actor->params >= 0 && actor->params < kResidentCount;
}
bool IsZora(WaterDesertResidentId id) {
    return id == WaterDesertResidentId::Lethra || id == WaterDesertResidentId::Neris;
}

struct Appearance {
    const char* animation;
    const char* hair;
    float scale;
    float speed;
    float phase;
    s16 tilt;
};
// Native species silhouettes and compatible animations. Gerudo hairstyles are
// actual GE1 variations, not body swaps. Zora identity uses posture and timing;
// the native skin palette stays intact rather than inventing unsupported dyes.
const std::array<Appearance, kResidentCount> appearances = { {
    { gZoraIdleAnim, nullptr, 0.0102f, 0.70f, 0.00f, -100 },
    { gZoraHandsOnHipsTappingFootAnim, nullptr, 0.0098f, 0.85f, 0.45f, 100 },
    { gGerudoWhiteIdleAnim, gGerudoWhiteHairstyleBobDL, 0.0102f, 0.80f, 0.20f, -80 },
    { gGerudoWhiteUnusedFoldingArmsAnim, gGerudoWhiteHairstyleStraightFringeDL, 0.0099f, 0.65f, 0.60f, 50 },
} };
ColliderCylinderInit cylinderInit = {
    { COLTYPE_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_2, COLSHAPE_CYLINDER },
    { ELEMTYPE_UNK0, { 0, 0, 0 }, { 0, 0, 0 }, TOUCH_NONE, BUMP_NONE, OCELEM_ON },
    { 20, 76, 0, { 0, 0, 0 } },
};
CollisionCheckInfoInit2 collisionInfo = { 0, 0, 0, 0, MASS_IMMOVABLE };

WaterDesertPlace PlaceForScene(int scene) {
    switch (scene) {
        case SCENE_ZORAS_RIVER:
            return WaterDesertPlace::RiverBank;
        case SCENE_GERUDO_VALLEY:
            return WaterDesertPlace::ValleyApproach;
        case SCENE_GERUDOS_FORTRESS:
            return WaterDesertPlace::Fortress;
        default:
            return WaterDesertPlace::None;
    }
}

uint8_t DesiredResidents(const PlayState* play) {
    WaterDesertContext context;
    context.enabled = CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), 0) != 0;
    context.supportedAdventure = IS_VANILLA || IS_MASTER_QUEST;
    context.normalScene = play != nullptr && play == gPlayState && gSaveContext.gameMode == GAMEMODE_NORMAL &&
                          gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && !IS_CUTSCENE_LAYER &&
                          play->roomCtx.curRoom.num == 0;
    context.daytime = IS_DAY;
    context.carpentersFreed = (gSaveContext.eventChkInf[9] & 0xF) == 0xF;
    context.place = play != nullptr ? PlaceForScene(play->sceneNum) : WaterDesertPlace::None;
    context.world = GetWorldProgress();
    context.economy = gSaveContext.ship.livingHyrule;
    return WaterDesertMaskFor(context);
}

bool PlayerHasTalk(PlayState* play, Actor* actor) {
    return GET_PLAYER(play) != nullptr && GET_PLAYER(play)->talkActor == actor &&
           (GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_TALKING) != 0;
}

std::string BuildDialogue(WaterDesertResidentId id) {
    const auto world = GetWorldProgress();
    std::string text = "%g" + std::string(GetWaterDesertResidentName(id)) + "%w. ";
    switch (id) {
        case WaterDesertResidentId::Lethra:
            text += !world.adult   ? "Spring keeper. A clean waterway begins far upstream. I arrange supplies for the "
                                     "families who tend its banks."
                    : !world.water ? "I have taken refuge beside the lower river. The Domain is frozen, and trouble "
                                     "downstream has cut our supply routes. We cannot trade our way past that."
                                   : "The lake's trouble has eased, and the open river can carry supplies again. The "
                                     "Domain is still frozen; I keep our work here on these usable banks.";
            break;
        case WaterDesertResidentId::Neris:
            text += IS_DAY ? "Waterway courier. Lethra counts tools; I count bends in the river. A reliable delivery "
                             "is a promise kept to someone you may never meet."
                           : "The supply partnership supports an evening delivery. I follow the open river, never the "
                             "icebound passages upstream.";
            break;
        case WaterDesertResidentId::Rasha:
            text += !world.adult    ? "Caravan quartermaster. This approach is where I count our supplies before the "
                                      "crossing. A visit here is no invitation to the fortress."
                    : !world.spirit ? "Your welcome among us is earned. Our caravans still wait for the danger around "
                                      "the Spirit Temple to pass before taking new investments."
                    : IS_DAY ? "Caravan quartermaster. With the temple's danger settled, we can put our routes in "
                               "order. A partnership buys a share of the work, not command of our people."
                             : "The late loading shift is underway. A working caravan partnership pays for careful "
                               "preparation before the desert heat.";
            break;
        case WaterDesertResidentId::Kesra:
            text += !world.spirit ? "Cloth trader. You are welcome to talk, but our distant customers cannot travel "
                                    "safely yet. Temple business must be settled before workshop investments."
                    : IS_DAY ? "Cloth trader. Desert cloth must shade without weighing down the traveler. My workshop "
                               "remains Gerudo work, whoever invests in its looms."
                             : "The workshop can afford an evening shift again. I check each seam while the air is "
                               "cool. Good cloth earns its price.";
            break;
        default:
            return "Safe travels.";
    }
    const int property = GetWaterDesertResidentPropertyId(id);
    const auto& economy = gSaveContext.ship.livingHyrule;
    if (property >= 0 && IsValidState(economy) && OwnsProperty(economy, property)) {
        if (!RegionOpen(kProperties[property].region, world))
            text += "^Your ownership is recorded. Work waits on the region's recovery.";
        else if (world.adult && !PropertyOperating(economy, property, world))
            text += "^The business still needs paid repairs before it can earn again.";
        else
            text += economy.enabled ? "^Your working partnership sends its earnings to your bank."
                                    : "^Your ownership remains safe while your account is paused.";
    }
    return text;
}

void LoadText(uint16_t* textId, bool* loadFromMessageTable) {
    const bool reply = *textId >= kReplyText && *textId < kReplyText + kResidentCount;
    if (!reply && (*textId < kQuoteText || *textId >= kQuoteText + kResidentCount))
        return;
    const auto id = static_cast<WaterDesertResidentId>(*textId - (reply ? kReplyText : kQuoteText));
    Actor* actor =
        gPlayState != nullptr && GET_PLAYER(gPlayState) != nullptr ? GET_PLAYER(gPlayState)->talkActor : nullptr;
    std::string text = "Let's speak again in a moment.";
    if (IsWaterDesertResidentActor(actor) && GetWaterDesertResidentId(actor) == id) {
        auto* resident = reinterpret_cast<WaterDesertActor*>(actor);
        text = reply ? std::string(resident->trade.response)
                     : BuildDialogue(id) + DescribeTradeOffer(resident->trade.offer);
    }
    CustomMessage message(text);
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

u16 GetTextId(PlayState* play, Actor* actor) {
    auto* resident = reinterpret_cast<WaterDesertActor*>(actor);
    const u16 text = static_cast<u16>(kQuoteText + actor->params);
    if (resident->talkState == NPC_TALK_STATE_IDLE && !PlayerHasTalk(play, actor))
        PreparePropertyTrade(resident->trade,
                             GetWaterDesertResidentPropertyId(static_cast<WaterDesertResidentId>(actor->params)), text);
    return text;
}

s16 UpdateTalkState(PlayState* play, Actor* actor) {
    auto* resident = reinterpret_cast<WaterDesertActor*>(actor);
    const auto state = Message_GetState(&play->msgCtx);
    if (play->msgCtx.talkActor == actor && state != TEXT_STATE_NONE && state != TEXT_STATE_CLOSING)
        HandleTradeChoice(play, actor, resident->trade, static_cast<u16>(kReplyText + actor->params));
    // An accepted request may wait for Link to put away an item before text.
    if (PlayerHasTalk(play, actor))
        return NPC_TALK_STATE_TALKING;
    return play->msgCtx.talkActor != actor || state == TEXT_STATE_NONE || state == TEXT_STATE_CLOSING
               ? NPC_TALK_STATE_IDLE
               : NPC_TALK_STATE_TALKING;
}

void InitRegionalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WaterDesertActor*>(actor);
    const auto id = static_cast<WaterDesertResidentId>(actor->params);
    if (!ValidIdentity(actor) || (DesiredResidents(play) & WaterDesertBit(id)) == 0) {
        Actor_Kill(actor);
        return;
    }
    const auto& look = appearances[actor->params];
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, IsZora(id) ? 24.0f : 30.0f);
    Actor_SetScale(actor, look.scale);
    SkelAnime_InitFlex(play, &resident->skelAnime, (FlexSkeletonHeader*)(IsZora(id) ? gZoraSkel : gGerudoWhiteSkel),
                       (AnimationHeader*)look.animation, resident->joints, resident->morphs, IsZora(id) ? 20 : 16);
    resident->skelAnime.playSpeed = look.speed;
    resident->skelAnime.curFrame = resident->skelAnime.endFrame * look.phase;
    // The supported folded-arms animation is a gesture into a resting pose.
    // Hold its final pose rather than repeatedly folding and unfolding arms.
    if (id == WaterDesertResidentId::Kesra) {
        resident->skelAnime.curFrame = resident->skelAnime.endFrame;
        resident->skelAnime.playSpeed = 0.0f;
    }
    Collider_InitCylinder(play, &resident->collider);
    Collider_SetCylinder(play, &resident->collider, actor, &cylinderInit);
    if (!IsZora(id))
        resident->collider.dim.height = 66;
    CollisionCheck_SetInfo2(&actor->colChkInfo, nullptr, &collisionInfo);
    resident->initialized = true;
    actor->targetMode = 6;
    actor->gravity = -1.0f;
    actor->uncullZoneForward = 1600.0f;
    actor->textId = GetTextId(play, actor);
    Actor_SetFocus(actor, IsZora(id) ? 70.0f : 60.0f);
}

void DestroyRegionalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WaterDesertActor*>(actor);
    if (!resident->initialized)
        return;
    ResourceMgr_UnregisterSkeleton(&resident->skelAnime);
    Collider_DestroyCylinder(play, &resident->collider);
    resident->initialized = false;
}

void UpdateRegionalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WaterDesertActor*>(actor);
    if (!resident->initialized || play != gPlayState)
        return;
    const auto id = static_cast<WaterDesertResidentId>(actor->params);
    const bool present = ValidIdentity(actor) && (DesiredResidents(play) & WaterDesertBit(id)) != 0;
    const bool talking = PlayerHasTalk(play, actor) ||
                         (play->msgCtx.talkActor == actor && Message_GetState(&play->msgCtx) != TEXT_STATE_NONE);
    const bool requested = (actor->flags & ACTOR_FLAG_TALK) != 0;
    if (!present && !talking && !requested) {
        Actor_Kill(actor);
        return;
    }
    SkelAnime_Update(&resident->skelAnime);
    ++resident->ticks;
    const unsigned int blink = (resident->ticks + actor->params * 31u) % 113u;
    resident->eyeIndex = blink == 0 ? 1 : blink == 1 ? 2 : blink == 2 ? 1 : 0;
    Actor_MoveXZGravity(actor);
    Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 50.0f, 4);
    Collider_UpdateCylinder(actor, &resident->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &resident->collider.base);
    Actor_SetFocus(actor, IsZora(id) ? 70.0f : 60.0f);
    if (GET_PLAYER(play) == nullptr)
        return;
    resident->interact.trackPos = GET_PLAYER(play)->actor.focus.pos;
    Npc_TrackPoint(actor, &resident->interact, 0,
                   talking                          ? NPC_TRACKING_FULL_BODY
                   : actor->xzDistToPlayer < 180.0f ? NPC_TRACKING_HEAD_AND_TORSO
                                                    : NPC_TRACKING_NONE);
    if (present || resident->talkState != NPC_TALK_STATE_IDLE || requested || talking)
        Npc_UpdateTalking(play, actor, &resident->talkState, 105.0f, GetTextId, UpdateTalkState);
}

s32 OverrideRegionalLimb(PlayState*, s32 limb, Gfx**, Vec3f*, Vec3s* rotation, void* actorRef) {
    auto* resident = static_cast<WaterDesertActor*>(actorRef);
    const auto id = static_cast<WaterDesertResidentId>(resident->actor.params);
    if (limb == 15) {
        if (IsZora(id)) {
            Matrix_Translate(1800.0f, 0.0f, 0.0f, MTXMODE_APPLY);
            Matrix_RotateX(resident->interact.headRot.y * kRadians, MTXMODE_APPLY);
            Matrix_RotateZ(resident->interact.headRot.x * kRadians, MTXMODE_APPLY);
            Matrix_Translate(-1800.0f, 0.0f, 0.0f, MTXMODE_APPLY);
        } else {
            rotation->x += resident->interact.headRot.y;
            rotation->z += resident->interact.headRot.x;
        }
    } else if (limb == 8) {
        Matrix_RotateX(-resident->interact.torsoRot.y * kRadians, MTXMODE_APPLY);
        Matrix_RotateZ(-resident->interact.torsoRot.x * kRadians, MTXMODE_APPLY);
        rotation->z += appearances[resident->actor.params].tilt;
        rotation->z += static_cast<s16>(Math_SinS(static_cast<s16>(resident->ticks * 270u)) * 55.0f);
    }
    return false;
}

extern "C" void PostRegionalLimb(PlayState* play, s32 limb, Gfx**, Vec3s*, void* actorRef) {
    if (limb != 15)
        return;
    auto* resident = static_cast<WaterDesertActor*>(actorRef);
    const bool zora = IsZora(static_cast<WaterDesertResidentId>(resident->actor.params));
    Vec3f focus = zora ? Vec3f{ 0, 600, 0 } : Vec3f{ 600, 700, 0 };
    Matrix_MultVec3f(&focus, &resident->actor.focus.pos);
    if (!zora) {
        OPEN_DISPS(play->state.gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++,
                       reinterpret_cast<Gfx*>(const_cast<char*>(appearances[resident->actor.params].hair)));
        CLOSE_DISPS(play->state.gfxCtx);
    }
}

// Native Zora geometry needs the opaque material segment provided by
// func_80034BA0, whose limb callbacks also receive the current display list.
s32 OverrideZoraLimb(PlayState* play, s32 limb, Gfx** list, Vec3f* pos, Vec3s* rot, void* actor, Gfx**) {
    return OverrideRegionalLimb(play, limb, list, pos, rot, actor);
}
extern "C" void PostZoraLimb(PlayState* play, s32 limb, Gfx** list, Vec3s* rot, void* actor, Gfx**) {
    PostRegionalLimb(play, limb, list, rot, actor);
}

extern "C" void DrawRegionalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WaterDesertActor*>(actor);
    if (!resident->initialized || !ValidIdentity(actor))
        return;
    static const char* zoraEyes[] = { gZoraEyeOpenTex, gZoraEyeHalfTex, gZoraEyeClosedTex };
    static const char* gerudoEyes[] = { gGerudoWhiteEyeOpenTex, gGerudoWhiteEyeHalfTex, gGerudoWhiteEyeClosedTex };
    const bool zora = IsZora(static_cast<WaterDesertResidentId>(actor->params));
    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x08, zora ? zoraEyes[resident->eyeIndex] : gerudoEyes[resident->eyeIndex]);
    if (zora) {
        func_80034BA0(play, &resident->skelAnime, OverrideZoraLimb, PostZoraLimb, actor, 255);
    } else {
        Gfx_SetupDL_37Opa(play->state.gfxCtx);
        SkelAnime_DrawSkeletonOpa(play, &resident->skelAnime, OverrideRegionalLimb, PostRegionalLimb, resident);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

struct Placement {
    WaterDesertResidentId id;
    WaterDesertPlace place;
    Vec3f position;
    s16 yaw;
};
// Authored dry ground, checked against local collision and water-box resources.
// These are stationary bank/approach workers, not swimmers or quest guards.
constexpr std::array<Placement, kResidentCount> placements = { {
    { WaterDesertResidentId::Lethra, WaterDesertPlace::RiverBank, { -1350, 100, -200 }, 0x4000 },
    { WaterDesertResidentId::Neris, WaterDesertPlace::RiverBank, { -1100, 100, -150 }, -0x4000 },
    { WaterDesertResidentId::Rasha, WaterDesertPlace::ValleyApproach, { 1300, 40, -400 }, -0x4000 },
    { WaterDesertResidentId::Kesra, WaterDesertPlace::Fortress, { -900, 16, -500 }, 0x4000 },
} };

bool ClearGround(PlayState* play, const Vec3f& candidate, Vec3f& ground) {
    Vec3f probe = { candidate.x, candidate.y + 40.0f, candidate.z };
    CollisionPoly* floor = nullptr;
    const float y = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
    if (floor == nullptr || !std::isfinite(y) || y <= BGCHECK_Y_MIN || std::abs(y - candidate.y) > 24.0f ||
        floor->normal.y < 26000)
        return false;
    ground = { candidate.x, y, candidate.z };
    constexpr std::array<Vec3f, 4> offsets = { { { 32, 0, 0 }, { -32, 0, 0 }, { 0, 0, 32 }, { 0, 0, -32 } } };
    for (const auto& offset : offsets) {
        probe = { ground.x + offset.x, y + 24.0f, ground.z + offset.z };
        floor = nullptr;
        const float edge = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
        if (floor == nullptr || !std::isfinite(edge) || std::abs(edge - y) > 8.0f || floor->normal.y < 26000)
            return false;
    }
    for (const float height : { 24.0f, 60.0f, 85.0f }) {
        probe = { ground.x, y + height, ground.z };
        if (BgCheck_SphVsFirstPoly(&play->colCtx, &probe, 22.0f))
            return false;
    }
    float waterHeight = 0.0f;
    WaterBox* water = nullptr;
    if (WaterBox_GetSurface1(play, &play->colCtx, ground.x, ground.z, &waterHeight, &water) && waterHeight > y + 2.0f)
        return false;
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        if (category != ACTORCAT_NPC && category != ACTORCAT_DOOR && category != ACTORCAT_PROP &&
            category != ACTORCAT_BG && category != ACTORCAT_PLAYER && category != ACTORCAT_ENEMY &&
            category != ACTORCAT_BOSS)
            continue;
        for (Actor* actor = play->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor->update == nullptr)
                continue;
            const bool hostile = category == ACTORCAT_ENEMY || category == ACTORCAT_BOSS;
            const float radius = hostile                       ? 450.0f
                                 : category == ACTORCAT_DOOR   ? 180.0f
                                 : category == ACTORCAT_PLAYER ? 150.0f
                                                               : 110.0f;
            const float dx = actor->world.pos.x - ground.x, dz = actor->world.pos.z - ground.z;
            if (std::abs(actor->world.pos.y - y) < (hostile ? 250.0f : 110.0f) && dx * dx + dz * dz < radius * radius)
                return false;
        }
    }
    return true;
}

void UpdatePopulation() {
    if (!GameInteractor::IsSaveLoaded(false))
        return;
    if (spawnCooldown != 0) {
        --spawnCooldown;
        return;
    }
    spawnCooldown = 40;
    const uint8_t desired = DesiredResidents(gPlayState);
    if (desired == 0 || GameInteractor::IsGameplayPaused() || gPlayState->pauseCtx.debugState != 0 ||
        gPlayState->gameOverCtx.state != GAMEOVER_INACTIVE || gPlayState->transitionTrigger != TRANS_TRIGGER_OFF ||
        gPlayState->transitionMode != TRANS_MODE_OFF || gSaveContext.health <= 0)
        return;
    uint8_t present = 0;
    for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
        if (actor->update != nullptr && IsWaterDesertResidentActor(actor))
            present |= WaterDesertBit(GetWaterDesertResidentId(actor));
    }
    for (const auto& placement : placements) {
        const uint8_t bit = WaterDesertBit(placement.id);
        if (placement.place != PlaceForScene(gPlayState->sceneNum) || (desired & bit) == 0 || (present & bit) != 0)
            continue;
        const int actorId = actorIds[IsZora(placement.id) ? 0 : 1];
        if (actorId < 0 || actorId > INT16_MAX)
            continue;
        Vec3f ground{};
        if (!ClearGround(gPlayState, placement.position, ground))
            continue;
        Actor* actor = Actor_Spawn(&gPlayState->actorCtx, gPlayState, static_cast<s16>(actorId), ground.x, ground.y,
                                   ground.z, 0, placement.yaw, 0, static_cast<s16>(placement.id));
        if (actor != nullptr && actor->update != nullptr)
            present |= bit;
    }
}

void RegisterResidents() {
    static bool registered = false;
    if (registered || ActorDB::Instance == nullptr || GameInteractor::Instance == nullptr)
        return;
    for (int family = 0; family < 2; ++family) {
        ActorDBInit entry;
        entry.name = family == 0 ? "En_LivingHyruleZoraResident" : "En_LivingHyruleGerudoResident";
        entry.desc = family == 0 ? "Living Hyrule Zora resident" : "Living Hyrule Gerudo resident";
        entry.category = ACTORCAT_NPC;
        entry.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
        entry.objectId = family == 0 ? OBJECT_ZO : OBJECT_GE1;
        entry.instanceSize = sizeof(WaterDesertActor);
        entry.init = InitRegionalResident;
        entry.destroy = DestroyRegionalResident;
        entry.update = UpdateRegionalResident;
        entry.draw = DrawRegionalResident;
        actorIds[family] = ActorDB::Instance->AddEntry(entry).entry.id;
    }
    registered = true;
    for (int id = 0; id < kResidentCount; ++id) {
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(kQuoteText + id, LoadText);
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(kReplyText + id, LoadText);
    }
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { spawnCooldown = 20; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdatePopulation);
}
RegisterShipInitFunc initWaterDesertResidents(RegisterResidents);
} // namespace

bool IsWaterDesertResidentActor(const Actor* actor) {
    if (!ValidIdentity(actor))
        return false;
    const int expectedId = actorIds[IsZora(static_cast<WaterDesertResidentId>(actor->params)) ? 0 : 1];
    return expectedId >= 0 && actor->id == expectedId;
}
WaterDesertResidentId GetWaterDesertResidentId(const Actor* actor) {
    return IsWaterDesertResidentActor(actor) ? static_cast<WaterDesertResidentId>(actor->params)
                                             : WaterDesertResidentId::Count;
}
int GetWaterDesertResidentPropertyId(WaterDesertResidentId id) {
    static constexpr std::array<int, kResidentCount> properties = { 13, -1, 14, 15 };
    return id < WaterDesertResidentId::Count ? properties[static_cast<size_t>(id)] : -1;
}
const char* GetWaterDesertResidentName(WaterDesertResidentId id) {
    static constexpr std::array<const char*, kResidentCount> names = { "Lethra", "Neris", "Rasha", "Kesra" };
    return id < WaterDesertResidentId::Count ? names[static_cast<size_t>(id)] : "Resident";
}
} // namespace LivingHyrule
