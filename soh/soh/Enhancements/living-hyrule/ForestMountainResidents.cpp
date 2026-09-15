#include "ForestMountainResidents.h"
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
#include <limits>
#include <string>
#include <type_traits>
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/object_km1/object_km1.h"
#include "objects/object_kw1/object_kw1.h"
#include "objects/object_oF1d_map/object_oF1d_map.h"
#include "objects/object_os_anime/object_os_anime.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

constexpr uint16_t kFirstText = 0x9700;
constexpr uint16_t kFirstReply = 0x9720;
constexpr size_t kResidentCount = static_cast<size_t>(ForestMountainResidentId::Count);
constexpr float kAngleToRadians = 3.14159265358979323846f / 32768.0f;
int residentActorId = -1;
uint8_t spawnCooldown = 20;

// Actor_Spawn allocates zeroed memory without calling constructors. The largest
// family has 18 joints; the Kokiri initializer receives its exact count of 16.
struct ForestMountainActor {
    Actor actor;
    SkelAnime skelAnime;
    ColliderCylinder collider;
    NpcInteractInfo interactInfo;
    Vec3s jointTable[18];
    Vec3s morphTable[18];
    TradeDialogueState trade;
    s16 talkState;
    u16 idleTicks;
    bool initialized;
};
static_assert(std::is_trivial_v<ForestMountainActor>);
static_assert(std::is_standard_layout_v<ForestMountainActor>);

struct Appearance {
    const char* skeleton;
    const char* head;
    const char* animation;
    Color_RGBA8 tunic;
    Color_RGBA8 boots;
    float scale;
    float animationSpeed;
    float startFraction;
    s16 torsoTilt;
    s16 colliderRadius;
    s16 colliderHeight;
    u8 jointCount;
    bool goron;
};

// Complete native body/head families keep all OTR limb references compatible.
// The Gorons retain their own skin materials and the fully uncurled resting
// frame. Only their proportions, expression, and gentle posture vary.
const std::array<Appearance, kResidentCount> appearances = { {
    { gKm1Skel,
      gKm1DL,
      gKokiriStandingHandOnChestAnim,
      { 64, 100, 40, 255 },
      { 119, 86, 52, 255 },
      0.0098f,
      0.80f,
      0.0f,
      -70,
      18,
      46,
      16,
      false },
    { gKw1Skel,
      object_kw1_DL_002C10,
      gKokiriStandingArmsBehindBackAnim,
      { 93, 121, 56, 255 },
      { 153, 130, 77, 255 },
      0.0102f,
      0.72f,
      0.4f,
      90,
      19,
      48,
      16,
      false },
    { gGoronSkel, nullptr, gGoronAnim_004930, {}, {}, 0.0104f, 0.0f, 1.0f, -85, 32, 72, 18, true },
    { gGoronSkel, nullptr, gGoronAnim_004930, {}, {}, 0.0098f, 0.0f, 1.0f, 105, 30, 68, 18, true },
} };

ColliderCylinderInit cylinderInit = {
    { COLTYPE_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_2, COLSHAPE_CYLINDER },
    { ELEMTYPE_UNK0, { 0, 0, 0 }, { 0, 0, 0 }, TOUCH_NONE, BUMP_NONE, OCELEM_ON },
    { 20, 48, 0, { 0, 0, 0 } },
};
CollisionCheckInfoInit2 collisionInfo = { 0, 0, 0, 0, MASS_IMMOVABLE };

bool ValidIdentity(const Actor* actor) {
    return actor != nullptr && actor->params >= 0 && actor->params < static_cast<s16>(ForestMountainResidentId::Count);
}

ForestMountainPlace PlaceFor(const PlayState* play) {
    if (play == nullptr)
        return ForestMountainPlace::None;
    if (play->sceneNum == SCENE_KOKIRI_FOREST && play->roomCtx.curRoom.num == 0)
        return ForestMountainPlace::KokiriForest;
    // The inhabited cavern is room 3; room 0 is the boulder/chest maze.
    if (play->sceneNum == SCENE_GORON_CITY && play->roomCtx.curRoom.num == 3)
        return ForestMountainPlace::GoronCity;
    return ForestMountainPlace::None;
}

uint8_t DesiredResidents(const PlayState* play) {
    if (play == nullptr || play != gPlayState || !GameInteractor::IsSaveLoaded(false))
        return 0;
    ForestMountainContext context;
    context.enabled = CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), 0) != 0;
    context.supportedAdventure = IS_VANILLA || IS_MASTER_QUEST;
    context.normalScene = gSaveContext.gameMode == GAMEMODE_NORMAL && !IS_CUTSCENE_LAYER;
    context.daytime = IS_DAY;
    context.place = PlaceFor(play);
    context.world = GetWorldProgress();
    context.economy = gSaveContext.ship.livingHyrule;
    return ForestMountainResidentMaskFor(context);
}

bool IsPresent(const PlayState* play, ForestMountainResidentId id) {
    return (DesiredResidents(play) & ForestMountainResidentBit(id)) != 0;
}

bool PlayerOwnsTalk(PlayState* play, const Actor* actor) {
    const Player* player = GET_PLAYER(play);
    return player != nullptr && player->talkActor == actor && (player->stateFlags1 & PLAYER_STATE1_TALKING) != 0;
}

std::string BusinessDialogue(ForestMountainResidentId id, const WorldProgress& world) {
    const auto& economy = gSaveContext.ship.livingHyrule;
    const int property = ForestMountainPropertyId(id);
    if (property < 0 || !IsValidState(economy) || !OwnsProperty(economy, property))
        return {};
    if (!economy.enabled)
        return "^Your share is safe while the account rests. We will resume paid work when you do.";
    if (!RegionOpen(kProperties[property].region, world))
        return "^Your deed is safe. We must settle the danger here before work can resume.";
    if (world.adult && !PropertyOperating(economy, property, world))
        return "^You still hold the deed. The old equipment needs repairs before earnings can reach your bank.";
    return "^Your partnership is working. Its earnings go into your bank as we tend the business.";
}

std::string BuildDialogue(ForestMountainResidentId id) {
    const auto world = GetWorldProgress();
    std::string text = "%g" + std::string(GetForestMountainResidentName(id)) + "%w. ";
    switch (id) {
        case ForestMountainResidentId::Fenn:
            text += world.adult ? "Seed sorter. We kept the seeds dry while the monsters roamed. Now the forest is "
                                  "quiet enough to plant them again."
                                : "Seed sorter. The round ones go here, the pointed ones there. You cannot hurry a "
                                  "seed by staring at it. I have tried.";
            text += "^I look after the seed garden's papers. A partner helps us replace tools and keep young plants "
                    "healthy.";
            break;
        case ForestMountainResidentId::Luma:
            text += world.adult ? "Berry gatherer. We sheltered behind our doors for so long. I am glad to hear birds "
                                  "over the paths again."
                                : "Berry gatherer. The best berries are never in the first bush. Leave some for "
                                  "tomorrow, and some for the birds.";
            text += "^Our woodland workshop makes baskets and repairs gathering tools. I arrange its partnerships.";
            break;
        case ForestMountainResidentId::Doron:
            text += world.adult ? "Stone grader. Coming home from captivity is one thing. Putting our tools back in "
                                  "order is another. We take it one sound stone at a time."
                    : CHECK_QUEST_ITEM(QUEST_GORON_RUBY)
                        ? "Stone grader. Good eating rocks are reaching us again. Now I can judge a building stone "
                          "without wishing it were lunch."
                        : "Stone grader. These rocks are for building, not eating. With the good food cut off, I have "
                          "to remind myself of that.";
            text += "^I handle the stoneworks partnership. We keep our work clear of the rolling paths above.";
            break;
        case ForestMountainResidentId::Brakka:
            text += !IS_DAY ? "Kiln tender. Your working partnership pays for this evening shift. Quiet work suits a "
                              "steady fire."
                    : world.adult ? "Kiln tender. The city has its voices back. Our kiln needs careful hands and sound "
                                    "tools before it can serve everyone again."
                    : CHECK_QUEST_ITEM(QUEST_GORON_RUBY) ? "Kiln tender. Fed workers make better work. I can finally "
                                                           "think about the next firing instead of the next meal."
                                                         : "Kiln tender. I save our strength while food is scarce. A "
                                                           "patient fire wastes less fuel than an impatient one.";
            text += "^The kiln partnership helps cover equipment and repairs. I can arrange the papers.";
            break;
        default:
            return "Safe travels.";
    }
    return text + BusinessDialogue(id, world);
}

void LoadText(uint16_t* textId, bool* loadFromMessageTable) {
    const bool reply = *textId >= kFirstReply && *textId < kFirstReply + kResidentCount;
    if (!reply && (*textId < kFirstText || *textId >= kFirstText + kResidentCount))
        return;
    const auto id = static_cast<ForestMountainResidentId>(*textId - (reply ? kFirstReply : kFirstText));
    const Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
    Actor* actor = player != nullptr ? player->talkActor : nullptr;
    std::string text = "Let's speak again in a moment.";
    // The player's talkActor is set before the first OnOpenText callback. The
    // message context can still refer to the preceding conversation then.
    if (IsForestMountainResidentActor(actor) && GetForestMountainResidentId(actor) == id && actor->update != nullptr) {
        auto* resident = reinterpret_cast<ForestMountainActor*>(actor);
        text = reply ? std::string(resident->trade.response)
                     : BuildDialogue(id) + DescribeTradeOffer(resident->trade.offer);
    }
    CustomMessage message(text);
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

u16 GetTextId(PlayState* play, Actor* actor) {
    const auto textId = static_cast<u16>(kFirstText + actor->params);
    auto* resident = reinterpret_cast<ForestMountainActor*>(actor);
    if (resident->talkState == NPC_TALK_STATE_IDLE && !PlayerOwnsTalk(play, actor))
        PreparePropertyTrade(resident->trade,
                             ForestMountainPropertyId(static_cast<ForestMountainResidentId>(actor->params)), textId);
    return textId;
}

s16 UpdateTalkState(PlayState* play, Actor* actor) {
    const auto state = Message_GetState(&play->msgCtx);
    if (play->msgCtx.talkActor == actor && state != TEXT_STATE_NONE && state != TEXT_STATE_CLOSING) {
        auto* resident = reinterpret_cast<ForestMountainActor*>(actor);
        HandleTradeChoice(play, actor, resident->trade, static_cast<u16>(kFirstReply + actor->params));
    }
    // Putting away an item can delay the initial textbox after the one talk
    // request is consumed. Keep that reservation alive until the player exits.
    if (PlayerOwnsTalk(play, actor))
        return NPC_TALK_STATE_TALKING;
    return play->msgCtx.talkActor != actor || state == TEXT_STATE_NONE || state == TEXT_STATE_CLOSING
               ? NPC_TALK_STATE_IDLE
               : NPC_TALK_STATE_TALKING;
}

void InitResident(Actor* actor, PlayState* play) {
    if (!ValidIdentity(actor) || !IsPresent(play, static_cast<ForestMountainResidentId>(actor->params))) {
        Actor_Kill(actor);
        return;
    }
    auto* resident = reinterpret_cast<ForestMountainActor*>(actor);
    const auto& look = appearances[actor->params];
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, look.goron ? 29.0f : 18.0f);
    Actor_SetScale(actor, look.scale);
    SkelAnime_InitFlex(play, &resident->skelAnime, (FlexSkeletonHeader*)look.skeleton, (AnimationHeader*)look.animation,
                       resident->jointTable, resident->morphTable, look.jointCount);
    resident->skelAnime.playSpeed = look.animationSpeed;
    resident->skelAnime.curFrame = resident->skelAnime.endFrame * look.startFraction;
    SkelAnime_Update(&resident->skelAnime);
    Collider_InitCylinder(play, &resident->collider);
    Collider_SetCylinder(play, &resident->collider, actor, &cylinderInit);
    resident->collider.dim.radius = look.colliderRadius;
    resident->collider.dim.height = look.colliderHeight;
    CollisionCheck_SetInfo2(&actor->colChkInfo, nullptr, &collisionInfo);
    resident->initialized = true;
    resident->idleTicks = static_cast<u16>(actor->params * 41);
    actor->targetMode = 6;
    actor->gravity = -1.0f;
    actor->uncullZoneForward = 1400.0f;
    actor->textId = GetTextId(play, actor);
    Actor_SetFocus(actor, static_cast<float>(look.colliderHeight));
}

void DestroyResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<ForestMountainActor*>(actor);
    if (!resident->initialized)
        return;
    ResourceMgr_UnregisterSkeleton(&resident->skelAnime);
    // Both frame tables are embedded in raw actor memory, never arena-free them.
    Collider_DestroyCylinder(play, &resident->collider);
    resident->initialized = false;
}

void UpdateResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<ForestMountainActor*>(actor);
    if (!resident->initialized || play != gPlayState)
        return;
    const bool present = ValidIdentity(actor) && IsPresent(play, static_cast<ForestMountainResidentId>(actor->params));
    const bool talking = (play->msgCtx.talkActor == actor && Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) ||
                         PlayerOwnsTalk(play, actor);
    const bool requested = (actor->flags & ACTOR_FLAG_TALK) != 0;
    if (!present && !talking && !requested) {
        Actor_Kill(actor);
        return;
    }
    const auto& look = appearances[actor->params];
    SkelAnime_Update(&resident->skelAnime);
    ++resident->idleTicks;
    Actor_MoveXZGravity(actor);
    Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 50.0f, 4);
    Collider_UpdateCylinder(actor, &resident->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &resident->collider.base);
    Actor_SetFocus(actor, static_cast<float>(look.colliderHeight));
    if (GET_PLAYER(play) == nullptr)
        return;
    resident->interactInfo.trackPos = GET_PLAYER(play)->actor.focus.pos;
    Npc_TrackPoint(actor, &resident->interactInfo, 0,
                   talking                          ? NPC_TRACKING_FULL_BODY
                   : actor->xzDistToPlayer < 180.0f ? NPC_TRACKING_HEAD_AND_TORSO
                                                    : NPC_TRACKING_NONE);
    if (present || resident->talkState != NPC_TALK_STATE_IDLE || requested || talking)
        Npc_UpdateTalking(play, actor, &resident->talkState, 100.0f, GetTextId, UpdateTalkState);
}

s32 OverrideLimb(PlayState*, s32 limb, Gfx** displayList, Vec3f*, Vec3s* rotation, void* actorRef) {
    auto* resident = static_cast<ForestMountainActor*>(actorRef);
    const auto& look = appearances[resident->actor.params];
    if (limb == (look.goron ? 17 : 15)) {
        if (look.head != nullptr)
            *displayList = reinterpret_cast<Gfx*>(const_cast<char*>(look.head));
        const float pivot = look.goron ? 2800.0f : 1200.0f;
        Matrix_Translate(pivot, 0.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateX(resident->interactInfo.headRot.y * kAngleToRadians, MTXMODE_APPLY);
        Matrix_RotateZ(resident->interactInfo.headRot.x * kAngleToRadians, MTXMODE_APPLY);
        Matrix_Translate(-pivot, 0.0f, 0.0f, MTXMODE_APPLY);
    }
    if (limb == (look.goron ? 10 : 8)) {
        if (look.goron) {
            Matrix_RotateY(resident->interactInfo.torsoRot.y * kAngleToRadians, MTXMODE_APPLY);
            Matrix_RotateX(resident->interactInfo.torsoRot.x * kAngleToRadians, MTXMODE_APPLY);
        } else {
            Matrix_RotateX(-resident->interactInfo.torsoRot.y * kAngleToRadians, MTXMODE_APPLY);
            Matrix_RotateZ(resident->interactInfo.torsoRot.x * kAngleToRadians, MTXMODE_APPLY);
        }
        rotation->z += look.torsoTilt;
        rotation->z += static_cast<s16>(Math_SinS(static_cast<s16>(resident->idleTicks * 290u)) * 55.0f);
    }
    return false;
}

void PostLimb(PlayState*, s32 limb, Gfx**, Vec3s*, void* actorRef) {
    auto* resident = static_cast<ForestMountainActor*>(actorRef);
    const bool goron = appearances[resident->actor.params].goron;
    if (limb != (goron ? 17 : 15))
        return;
    Vec3f focus = { goron ? 600.0f : 0.0f, 0.0f, 0.0f };
    Matrix_MultVec3f(&focus, &resident->actor.focus.pos);
}

Gfx* MaterialColor(GraphicsContext* graphics, const Color_RGBA8& color) {
    auto* list = static_cast<Gfx*>(Graph_Alloc(graphics, 2 * sizeof(Gfx)));
    gDPSetEnvColor(list, color.r, color.g, color.b, color.a);
    gSPEndDisplayList(list + 1);
    return list;
}

extern "C" void LivingHyruleForestMountain_Draw(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<ForestMountainActor*>(actor);
    if (!resident->initialized || !ValidIdentity(actor))
        return;
    const auto& look = appearances[actor->params];
    const unsigned int blink = resident->idleTicks % 160u;
    const unsigned int eye = blink < 2 ? 1 : blink < 4 ? 2 : blink < 6 ? 1 : 0;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    if (look.goron) {
        static const char* eyes[] = { gGoronCsEyeOpenTex, gGoronCsEyeHalfTex, gGoronCsEyeClosedTex };
        gSPSegment(POLY_OPA_DISP++, 0x08, eyes[eye]);
        gSPSegment(POLY_OPA_DISP++, 0x09, gGoronCsMouthNeutralTex);
    } else {
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
        gSPSegment(POLY_OPA_DISP++, 0x08, MaterialColor(play->state.gfxCtx, look.tunic));
        gSPSegment(POLY_OPA_DISP++, 0x09, MaterialColor(play->state.gfxCtx, look.boots));
        static const char* eyes[] = { gKw1EyeOpenTex, gKw1EyeHalfTex, gKw1EyeClosedTex };
        gSPSegment(POLY_OPA_DISP++, 0x0A, eyes[eye]);
        // Native Kokiri materials call segment 0x0C for their opaque pass.
        // Supply the same empty list as func_80034BA0; do not inherit old state.
        auto* end = static_cast<Gfx*>(Graph_Alloc(play->state.gfxCtx, sizeof(Gfx)));
        gSPEndDisplayList(end);
        gSPSegment(POLY_OPA_DISP++, 0x0C, end);
    }
    SkelAnime_DrawSkeletonOpa(play, &resident->skelAnime, OverrideLimb, PostLimb, resident);
    CLOSE_DISPS(play->state.gfxCtx);
}

struct Placement {
    ForestMountainResidentId id;
    ForestMountainPlace place;
    Vec3f position;
    s16 yaw;
};
// Local spot04/spot18 collision resources confirm these footprints and body
// clearances. The two Gorons stand on the lower (~197) walkway, below every
// original rolling route (~400), away from the central urn and Darunia's door.
const std::array<Placement, kResidentCount> placements = { {
    { ForestMountainResidentId::Fenn, ForestMountainPlace::KokiriForest, { -300, 0, 300 }, 0x4000 },
    { ForestMountainResidentId::Luma, ForestMountainPlace::KokiriForest, { 900, 0, 800 }, -0x4000 },
    { ForestMountainResidentId::Doron, ForestMountainPlace::GoronCity, { -300, 197, -250 }, 0x4000 },
    { ForestMountainResidentId::Brakka, ForestMountainPlace::GoronCity, { 350, 197, 0 }, -0x4000 },
} };

bool FindClearGround(PlayState* play, const Placement& placement, Vec3f& ground) {
    const auto& look = appearances[static_cast<size_t>(placement.id)];
    const auto& candidate = placement.position;
    Vec3f probe = { candidate.x, candidate.y + 40.0f, candidate.z };
    CollisionPoly* floor = nullptr;
    const float height = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
    if (floor == nullptr || !std::isfinite(height) || height <= BGCHECK_Y_MIN ||
        std::abs(height - candidate.y) > 24.0f || floor->normal.y < 26000)
        return false;
    ground = { candidate.x, height, candidate.z };
    const float footprint = look.goron ? 44.0f : 32.0f;
    const std::array<Vec3f, 8> edges = { { { footprint, 0, 0 },
                                           { -footprint, 0, 0 },
                                           { 0, 0, footprint },
                                           { 0, 0, -footprint },
                                           { footprint, 0, footprint },
                                           { -footprint, 0, footprint },
                                           { footprint, 0, -footprint },
                                           { -footprint, 0, -footprint } } };
    for (const auto& edge : edges) {
        probe = { ground.x + edge.x, height + 24.0f, ground.z + edge.z };
        floor = nullptr;
        const float edgeHeight = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
        if (floor == nullptr || !std::isfinite(edgeHeight) || std::abs(edgeHeight - height) > 8.0f ||
            floor->normal.y < 26000)
            return false;
    }
    const float radius = look.goron ? 32.0f : 20.0f;
    for (const float bodyHeight : { radius + 8.0f, look.goron ? 74.0f : 46.0f }) {
        probe = { ground.x, height + bodyHeight, ground.z };
        if (BgCheck_SphVsFirstPoly(&play->colCtx, &probe, radius))
            return false;
    }
    float waterHeight = 0.0f;
    WaterBox* water = nullptr;
    if (WaterBox_GetSurface1(play, &play->colCtx, ground.x, ground.z, &waterHeight, &water) &&
        waterHeight > height + 2.0f)
        return false;
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        if (category != ACTORCAT_NPC && category != ACTORCAT_DOOR && category != ACTORCAT_PROP &&
            category != ACTORCAT_PLAYER && category != ACTORCAT_BG && category != ACTORCAT_ENEMY &&
            category != ACTORCAT_BOSS && category != ACTORCAT_EXPLOSIVE)
            continue;
        for (const Actor* actor = play->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor->update == nullptr)
                continue;
            const auto& pos = actor->world.pos;
            const float dx = pos.x - ground.x, dz = pos.z - ground.z;
            const bool dangerous =
                category == ACTORCAT_ENEMY || category == ACTORCAT_BOSS || category == ACTORCAT_EXPLOSIVE;
            const float clearance = dangerous                     ? 450.0f
                                    : category == ACTORCAT_DOOR   ? 180.0f
                                    : category == ACTORCAT_PLAYER ? 150.0f
                                                                  : 120.0f;
            if (std::abs(pos.y - height) < (dangerous ? 250.0f : 110.0f) && dx * dx + dz * dz < clearance * clearance)
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
        gPlayState->transitionMode != TRANS_MODE_OFF || gSaveContext.health <= 0 || residentActorId < 0 ||
        residentActorId > (std::numeric_limits<s16>::max)())
        return;
    uint8_t present = 0;
    for (const Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr;
         actor = actor->next) {
        if (actor->update != nullptr && IsForestMountainResidentActor(actor))
            present |= ForestMountainResidentBit(GetForestMountainResidentId(actor));
    }
    const auto place = PlaceFor(gPlayState);
    for (const auto& placement : placements) {
        const uint8_t bit = ForestMountainResidentBit(placement.id);
        if (placement.place != place || (desired & bit) == 0 || (present & bit) != 0)
            continue;
        Vec3f ground{};
        if (!FindClearGround(gPlayState, placement, ground))
            continue;
        Actor* actor = Actor_Spawn(&gPlayState->actorCtx, gPlayState, static_cast<s16>(residentActorId), ground.x,
                                   ground.y, ground.z, 0, placement.yaw, 0, static_cast<s16>(placement.id));
        if (actor != nullptr && actor->update != nullptr)
            present |= bit;
    }
}

void RegisterResidents() {
    if (residentActorId >= 0 || ActorDB::Instance == nullptr || GameInteractor::Instance == nullptr)
        return;
    ActorDBInit entry;
    entry.name = "En_LivingHyruleForestMountainResident";
    entry.desc = "Living Hyrule Kokiri and Goron workers";
    entry.category = ACTORCAT_NPC;
    entry.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    entry.objectId = OBJECT_GAMEPLAY_KEEP;
    entry.instanceSize = sizeof(ForestMountainActor);
    entry.init = InitResident;
    entry.destroy = DestroyResident;
    entry.update = UpdateResident;
    entry.draw = LivingHyruleForestMountain_Draw;
    residentActorId = ActorDB::Instance->AddEntry(entry).entry.id;
    for (size_t id = 0; id < kResidentCount; ++id) {
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
            static_cast<int32_t>(kFirstText + id), LoadText);
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
            static_cast<int32_t>(kFirstReply + id), LoadText);
    }
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { spawnCooldown = 20; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdatePopulation);
}
RegisterShipInitFunc initResidents(RegisterResidents);

} // namespace

bool IsForestMountainResidentActor(const Actor* actor) {
    return residentActorId >= 0 && actor != nullptr && actor->id == residentActorId && ValidIdentity(actor);
}

ForestMountainResidentId GetForestMountainResidentId(const Actor* actor) {
    return IsForestMountainResidentActor(actor) ? static_cast<ForestMountainResidentId>(actor->params)
                                                : ForestMountainResidentId::Count;
}

const char* GetForestMountainResidentName(ForestMountainResidentId id) {
    static constexpr std::array<const char*, kResidentCount> names = { "Fenn", "Luma", "Doron", "Brakka" };
    return id < ForestMountainResidentId::Count ? names[static_cast<size_t>(id)] : "Resident";
}

} // namespace LivingHyrule
