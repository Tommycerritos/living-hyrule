#include "WorldResidents.h"
#include "LivingHyrule.h"
#include "TradeDialogue.h"
#include "MarketRestoration.h"
#include "ResidentMovement.h"

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
#include "objects/object_bji/object_bji.h"
#include "objects/object_boj/object_boj.h"
#include "objects/object_cne/object_cne.h"
#include "objects/object_os_anime/object_os_anime.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

constexpr uint16_t kFirstText = 0x9600;
constexpr uint16_t kFirstReply = 0x9620;
constexpr size_t kResidentCount = static_cast<size_t>(WorldResidentId::Count);
constexpr int kLimbCount = 16;
constexpr float kAngleToRadians = 3.14159265358979323846f / 32768.0f;
int worldActorId = -1;
uint8_t spawnCooldown = 20;

// The engine allocates and zeroes actor memory; no owning C++ members belong here.
struct WorldResidentActor {
    Actor actor;
    SkelAnime skelAnime;
    ColliderCylinder collider;
    NpcInteractInfo interactInfo;
    Vec3s jointTable[kLimbCount];
    Vec3s morphTable[kLimbCount];
    TradeDialogueState trade;
    ResidentMovementState movement;
    s16 talkState;
    u16 idleTicks;
    bool initialized;
};
static_assert(std::is_trivial_v<WorldResidentActor>);
static_assert(std::is_standard_layout_v<WorldResidentActor>);

struct Appearance {
    const char* skeleton;
    const char* head;
    const char* animation;
    Color_RGBA8 primary;
    Color_RGBA8 secondary;
    Color_RGBA8 accent;
    Vec3f modelOffset;
    float scale;
    float animationSpeed;
    float startFraction;
    s16 torsoTilt;
};

// Compatible civilian skeleton/head pairs and idle animations only. No En_Hy
// lifecycle, item exchange, dog quest, path data, or progression flags are reused.
// Material display lists are allocated per draw; shared resources stay immutable.
const std::array<Appearance, kResidentCount> appearances = { {
    { gCneSkel,
      gCneHeadBrownHairDL,
      gObjOsAnim_4E90,
      { 114, 139, 60, 0 },
      { 213, 181, 117, 0 },
      { 114, 139, 60, 0 },
      { 0, 0, 700 },
      0.0100f,
      0.80f,
      0.00f,
      -80 }, // Vessa, green apron
    { object_bji_Skel_0000F0,
      object_bji_DL_003F68,
      gObjOsAnim_1F18,
      { 116, 86, 65, 0 },
      { 151, 153, 176, 0 },
      { 255, 255, 255, 0 },
      { -100, 0, 800 },
      0.0103f,
      0.72f,
      0.35f,
      80 }, // Hadrin, broad porter
    { gCneSkel,
      gCneHeadOrangeHairDL,
      gObjOsAnim_4408,
      { 66, 76, 126, 0 },
      { 222, 174, 81, 0 },
      { 245, 225, 167, 0 },
      { 0, 0, 700 },
      0.0097f,
      0.70f,
      0.60f,
      40 }, // Pella, dusk blue
    { object_boj_Skel_0000F0,
      object_boj_DL_0059B0,
      gObjOsAnim_2DC0,
      { 136, 73, 54, 0 },
      { 209, 194, 154, 0 },
      { 255, 255, 255, 0 },
      { -200, 0, -200 },
      0.0098f,
      0.94f,
      0.10f,
      -100 }, // Caro, road red
    { object_bji_Skel_0000F0,
      object_bji_DL_003F68,
      gObjOsAnim_1F18,
      { 88, 103, 101, 0 },
      { 189, 161, 92, 0 },
      { 255, 255, 255, 0 },
      { -100, 0, 800 },
      0.0097f,
      0.88f,
      0.70f,
      -40 }, // Hollis, slate and brass
    { gCneSkel,
      gCneHeadOrangeHairDL,
      gObjOsAnim_4408,
      { 126, 88, 52, 0 },
      { 213, 202, 157, 0 },
      { 235, 222, 184, 0 },
      { 0, 0, 700 },
      0.0102f,
      0.86f,
      0.30f,
      -60 }, // Nessa, oat colors
    { object_boj_Skel_0000F0,
      object_boj_DL_005738,
      gObjOsAnim_2D0C,
      { 76, 108, 76, 0 },
      { 177, 165, 138, 0 },
      { 255, 255, 255, 0 },
      { 0, 0, -300 },
      0.0096f,
      0.78f,
      0.55f,
      100 }, // Wren, stable green
    { object_boj_Skel_0000F0,
      object_boj_DL_0059B0,
      gObjOsAnim_2DC0,
      { 49, 107, 120, 0 },
      { 199, 186, 152, 0 },
      { 255, 255, 255, 0 },
      { -200, 0, -200 },
      0.0101f,
      0.74f,
      0.80f,
      60 }, // Vero, lake teal
    { gCneSkel,
      gCneHeadBrownHairDL,
      gObjOsAnim_4E90,
      { 116, 86, 133, 0 },
      { 202, 207, 194, 0 },
      { 116, 86, 133, 0 },
      { 0, 0, 700 },
      0.0098f,
      0.68f,
      0.45f,
      0 }, // Edda, ink purple
} };

ColliderCylinderInit cylinderInit = {
    { COLTYPE_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_2, COLSHAPE_CYLINDER },
    { ELEMTYPE_UNK0, { 0, 0, 0 }, { 0, 0, 0 }, TOUCH_NONE, BUMP_NONE, OCELEM_ON },
    { 20, 62, 0, { 0, 0, 0 } },
};
CollisionCheckInfoInit2 collisionInfo = { 0, 0, 0, 0, MASS_IMMOVABLE };

bool ValidIdentity(const Actor* actor) {
    return actor != nullptr && actor->params >= 0 && actor->params < static_cast<s16>(WorldResidentId::Count);
}

WorldPopulationPlace PlaceForScene(int scene) {
    switch (scene) {
        case SCENE_MARKET_DAY:
            return WorldPopulationPlace::MarketDay;
        case SCENE_MARKET_NIGHT:
            return WorldPopulationPlace::MarketNight;
        case SCENE_MARKET_RUINS:
            return WorldPopulationPlace::MarketRuins;
        case SCENE_HYRULE_FIELD:
            return WorldPopulationPlace::Field;
        case SCENE_LON_LON_RANCH:
            return WorldPopulationPlace::Ranch;
        case SCENE_LAKE_HYLIA:
            return WorldPopulationPlace::Lake;
        default:
            return WorldPopulationPlace::None;
    }
}

uint16_t DesiredResidents(const PlayState* play) {
    WorldPopulationContext context;
    context.enabled = CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), 0) != 0;
    context.supportedAdventure = IS_VANILLA || IS_MASTER_QUEST;
    context.normalScene = play != nullptr && play == gPlayState && gSaveContext.gameMode == GAMEMODE_NORMAL &&
                          gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && !IS_CUTSCENE_LAYER &&
                          play->roomCtx.curRoom.num == 0;
    context.daytime = IS_DAY;
    context.place = play != nullptr ? PlaceForScene(play->sceneNum) : WorldPopulationPlace::None;
    context.world = GetWorldProgress();
    context.economy = gSaveContext.ship.livingHyrule;
    return WorldResidentMaskFor(context);
}

bool IsPresent(const PlayState* play, WorldResidentId id) {
    return (DesiredResidents(play) & WorldResidentBit(id)) != 0;
}

std::string BusinessDialogue(WorldResidentId id, const WorldProgress& world) {
    const int property = GetWorldResidentPropertyId(id);
    const auto& economy = gSaveContext.ship.livingHyrule;
    if (property < 0 || !IsValidState(economy) || !OwnsProperty(economy, property))
        return {};
    if (!RegionOpen(kProperties[property].region, world))
        return "^Your deed is safe. Work must wait until the trouble here is settled.";
    if (world.adult && !PropertyOperating(economy, property, world))
        return "^You still own the business. Its damaged equipment needs paid repairs before we can earn again.";
    return economy.enabled ? "^Your business is working again. Its earnings go into your bank."
                           : "^Your deed is safe while your account rests. We can settle the work when you resume it.";
}

std::string BuildDialogue(WorldResidentId id) {
    const WorldProgress world = GetWorldProgress();
    const bool restoredSquare = IsMarketRestorationActive();
    std::string text = "%g" + std::string(GetWorldResidentName(id)) + "%w. ";
    switch (id) {
        case WorldResidentId::Vessa:
            if (!world.adult) {
                text += "I sell the vegetables that survive the trip from the farms. Bruised turnips go into my soup, "
                        "never into a customer's basket.";
            } else if (IsValidState(gSaveContext.ship.livingHyrule) &&
                       PropertyOperating(gSaveContext.ship.livingHyrule, 0, world)) {
                text += restoredSquare ? "The streets are sound again, and this stall is working. A basket of "
                                         "fresh food gives people a reason to come back tomorrow."
                                       : "The stall is working, though the town around it is still scarred. A "
                                         "basket of fresh food gives people a reason to return.";
            } else {
                text += "I used to sell fresh produce here. For now I help the returning families. With an investment "
                        "and sound equipment, the stall could feed this square again.";
            }
            break;
        case WorldResidentId::Hadrin:
            text += restoredSquare ? "A square fit to walk through again. I still sort supplies by the old "
                                     "guesthouse; the rooms and alleys will need their own work."
                    : world.adult  ? "I used to carry guests' trunks. Now I carry supplies for the people coming home. "
                                     "The town is safe, but these ruins need more than courage."
                                   : "Guesthouse porter. I can tell a traveler's journey by the mud on their luggage. "
                                     "Yours would make quite a story.";
            break;
        case WorldResidentId::Pella:
            text += restoredSquare ? "The square has its shape back. I make my rounds for the people returning "
                                     "after sunset. We will light the town one evening at a time."
                    : world.adult  ? "I keep the relief workers' lamps trimmed. A little light belongs in this ruined "
                                     "square, even before the houses are ready."
                                  : "The lantern keeper. The sellers count coins at sunset; I count wicks. Someone has "
                                    "to make sure the last traveler can find the way.";
            break;
        case WorldResidentId::Caro:
            text += world.adult ? "Road courier. Since the forest quieted, letters have started moving again. I still "
                                  "finish my rounds before dark."
                                : "Road courier. An orchard lease pays slowly, but a traveler remembers where the good "
                                  "fruit grows. I handle the south road growers' papers.";
            break;
        case WorldResidentId::Hollis:
            text += world.adult ? "The wagons are returning, one cautious driver at a time. A repaired supply yard "
                                  "means dry ropes and a place to sort a load."
                                : "Caravan outfitter. Rope, canvas, spare wheels. Nothing glamorous, until you're "
                                  "stranded without it. The supply yard could use a steady owner.";
            break;
        case WorldResidentId::Nessa:
            text += world.adult ? "Feed buyer. The ranch has breathing room again. Keeping pasture healthy is work for "
                                  "every season, not just the harvest."
                                : "Feed buyer. A horse notices poor hay before its rider notices a poor bargain. I "
                                  "help arrange the pasture leases.";
            break;
        case WorldResidentId::Wren:
            text += world.adult && !world.ranchFreed ? "Stablehand. Orders change, but the animals still need clean "
                                                       "water. I keep my head down and see to them."
                    : IS_DAY ? "Stablehand. The dairy partnership helps pay for feed and clean stalls. I stay with the "
                               "animals whoever holds the papers."
                             : "The evening dairy shift is mine. Your working partnership keeps this extra round "
                               "worthwhile. Quiet voices, please; the horses are settling.";
            break;
        case WorldResidentId::Vero:
            text += world.adult ? "Net-mender. With water back in the lake, the fishing cooperative has a future "
                                  "again. First we put the worn gear right."
                                : "Net-mender. We share the catch and the cost of good gear. A fishing cooperative "
                                  "beats arguing over every torn net.";
            break;
        case WorldResidentId::Edda:
            text += world.adult && !world.water ? "Research assistant. I measure the falling water from this high "
                                                  "ground. These readings are worse than any ordinary dry season."
                    : IS_DAY ? "Research assistant. The fishermen bring observations; I bring notebooks. We learn more "
                               "by listening than by guessing."
                             : "The cooperative's evening work gives me fresh observations to record. Fish do not "
                               "arrange their lives around our daylight hours.";
            break;
        default:
            return "Safe travels.";
    }
    return text + BusinessDialogue(id, world);
}

WorldResidentActor* TalkingResident(WorldResidentId id) {
    if (gPlayState == nullptr || GET_PLAYER(gPlayState) == nullptr)
        return nullptr;
    // Player owns the new talkActor before Message_OpenText; msgCtx still points
    // at the preceding conversation at this stage. Never borrow another quote.
    Actor* actor = GET_PLAYER(gPlayState)->talkActor;
    return IsWorldResidentActor(actor) && GetWorldResidentId(actor) == id ? reinterpret_cast<WorldResidentActor*>(actor)
                                                                          : nullptr;
}

void LoadText(uint16_t* textId, bool* loadFromMessageTable) {
    const bool reply = *textId >= kFirstReply && *textId < kFirstReply + kResidentCount;
    if (!reply && (*textId < kFirstText || *textId >= kFirstText + kResidentCount))
        return;
    const auto id = static_cast<WorldResidentId>(*textId - (reply ? kFirstReply : kFirstText));
    auto* resident = TalkingResident(id);
    std::string text = "Let's speak again in a moment.";
    if (resident != nullptr) {
        text = reply ? std::string(resident->trade.response)
                     : DescribeResidentDialogue(&resident->actor, resident->trade, BuildDialogue(id));
    }
    CustomMessage message(text);
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

u16 GetTextId(PlayState* play, Actor* actor) {
    auto* resident = reinterpret_cast<WorldResidentActor*>(actor);
    const u16 textId = static_cast<u16>(kFirstText + actor->params);
    const bool talkReserved = GET_PLAYER(play) != nullptr && GET_PLAYER(play)->talkActor == actor &&
                              (GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_TALKING) != 0;
    if (resident->talkState == NPC_TALK_STATE_IDLE && !talkReserved) {
        PreparePropertyTrade(resident->trade, GetWorldResidentPropertyId(static_cast<WorldResidentId>(actor->params)),
                             textId);
        PrepareResidentDialogue(resident->trade, actor);
    }
    return textId;
}

s16 UpdateTalkState(PlayState* play, Actor* actor) {
    auto* resident = reinterpret_cast<WorldResidentActor*>(actor);
    const auto state = Message_GetState(&play->msgCtx);
    if (play->msgCtx.talkActor == actor && state != TEXT_STATE_NONE && state != TEXT_STATE_CLOSING) {
        HandleTradeChoice(play, actor, resident->trade, static_cast<u16>(kFirstReply + actor->params));
    }
    // Link may first put away an item before opening the textbox. The request
    // has already been consumed, so keep the helper in TALKING until it opens.
    if (GET_PLAYER(play) != nullptr && GET_PLAYER(play)->talkActor == actor &&
        (GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_TALKING) != 0)
        return NPC_TALK_STATE_TALKING;
    return play->msgCtx.talkActor != actor || state == TEXT_STATE_NONE || state == TEXT_STATE_CLOSING
               ? NPC_TALK_STATE_IDLE
               : NPC_TALK_STATE_TALKING;
}

void InitWorldResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WorldResidentActor*>(actor);
    if (!ValidIdentity(actor) || !IsPresent(play, static_cast<WorldResidentId>(actor->params))) {
        Actor_Kill(actor);
        return;
    }
    const auto& look = appearances[actor->params];
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, 24.0f);
    Actor_SetScale(actor, look.scale);
    // OTR resources resolve skeletons, animations, limb geometry, and textures
    // directly. Do not consume scarce scene object-bank slots for visual reuse.
    SkelAnime_InitFlex(play, &resident->skelAnime, (FlexSkeletonHeader*)look.skeleton, (AnimationHeader*)look.animation,
                       resident->jointTable, resident->morphTable, kLimbCount);
    resident->skelAnime.playSpeed = look.animationSpeed;
    resident->skelAnime.curFrame = resident->skelAnime.endFrame * look.startFraction;
    Collider_InitCylinder(play, &resident->collider);
    Collider_SetCylinder(play, &resident->collider, actor, &cylinderInit);
    CollisionCheck_SetInfo2(&actor->colChkInfo, nullptr, &collisionInfo);
    resident->initialized = true;
    actor->targetMode = 6;
    actor->gravity = -1.0f;
    actor->uncullZoneForward = 1500.0f;
    actor->textId = GetTextId(play, actor);
    Actor_SetFocus(actor, 60.0f);
}

void DestroyWorldResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WorldResidentActor*>(actor);
    ResetResidentMovement(resident->movement);
    if (!resident->initialized)
        return;
    ResourceMgr_UnregisterSkeleton(&resident->skelAnime);
    Collider_DestroyCylinder(play, &resident->collider);
    resident->initialized = false;
}

void UpdateWorldResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WorldResidentActor*>(actor);
    if (!resident->initialized || play != gPlayState)
        return;
    const bool present = ValidIdentity(actor) && IsPresent(play, static_cast<WorldResidentId>(actor->params));
    const bool talking = play->msgCtx.talkActor == actor && Message_GetState(&play->msgCtx) != TEXT_STATE_NONE;
    const bool reserved = GET_PLAYER(play) != nullptr && GET_PLAYER(play)->talkActor == actor &&
                          (GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_TALKING) != 0;
    const bool requested = (actor->flags & ACTOR_FLAG_TALK) != 0;
    if (!present && !talking && !requested && !reserved) {
        Actor_Kill(actor);
        return;
    }
    const auto id = GetWorldResidentId(actor);
    const auto routine = id == WorldResidentId::Pella  ? ResidentMovementRoutine::Pella
                         : id == WorldResidentId::Edda ? ResidentMovementRoutine::Edda
                                                       : ResidentMovementRoutine::None;
    const bool walking =
        UpdateResidentMovement(play, actor, resident->movement, routine, &resident->skelAnime, present);
    ++resident->idleTicks;
    Actor_MoveXZGravity(actor);
    Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 50.0f, 4);
    Collider_UpdateCylinder(actor, &resident->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &resident->collider.base);
    Actor_SetFocus(actor, 60.0f);
    if (GET_PLAYER(play) == nullptr)
        return;
    resident->interactInfo.trackPos = GET_PLAYER(play)->actor.focus.pos;
    Npc_TrackPoint(actor, &resident->interactInfo, 0,
                   walking                          ? NPC_TRACKING_NONE
                   : talking                        ? NPC_TRACKING_FULL_BODY
                   : actor->xzDistToPlayer < 180.0f ? NPC_TRACKING_HEAD_AND_TORSO
                                                    : NPC_TRACKING_NONE);
    if (present || resident->talkState != NPC_TALK_STATE_IDLE || requested || reserved) {
        Npc_UpdateTalking(play, actor, &resident->talkState, 100.0f, GetTextId, UpdateTalkState);
    }
}

s32 OverrideWorldLimb(PlayState*, s32 limbIndex, Gfx** displayList, Vec3f*, Vec3s* rotation, void* actorRef) {
    auto* resident = static_cast<WorldResidentActor*>(actorRef);
    const auto& look = appearances[resident->actor.params];
    if (limbIndex == 15) {
        *displayList = reinterpret_cast<Gfx*>(const_cast<char*>(look.head));
        Matrix_Translate(1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateX(resident->interactInfo.headRot.y * kAngleToRadians, MTXMODE_APPLY);
        Matrix_RotateZ(resident->interactInfo.headRot.x * kAngleToRadians, MTXMODE_APPLY);
        Matrix_Translate(-1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
    } else if (limbIndex == 8) {
        Matrix_RotateX(-resident->interactInfo.torsoRot.y * kAngleToRadians, MTXMODE_APPLY);
        Matrix_RotateZ(-resident->interactInfo.torsoRot.x * kAngleToRadians, MTXMODE_APPLY);
        rotation->z += look.torsoTilt;
        rotation->z += static_cast<s16>(Math_SinS(static_cast<s16>(resident->idleTicks * 280u)) * 45.0f);
    }
    return false;
}

extern "C" void PostWorldLimb(PlayState*, s32 limbIndex, Gfx**, Vec3s*, void* actorRef) {
    if (limbIndex != 15)
        return;
    auto* resident = static_cast<WorldResidentActor*>(actorRef);
    Vec3f focus = { 400.0f, 0.0f, 0.0f };
    Matrix_MultVec3f(&focus, &resident->actor.focus.pos);
}

Gfx* MaterialColor(GraphicsContext* graphics, const Color_RGBA8& color) {
    auto* list = static_cast<Gfx*>(Graph_Alloc(graphics, 2 * sizeof(Gfx)));
    gDPSetEnvColor(list, color.r, color.g, color.b, color.a);
    gSPEndDisplayList(list + 1);
    return list;
}

extern "C" void DrawWorldResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<WorldResidentActor*>(actor);
    if (!resident->initialized || !ValidIdentity(actor))
        return;
    const auto& look = appearances[actor->params];
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Translate(look.modelOffset.x, look.modelOffset.y, look.modelOffset.z, MTXMODE_APPLY);
    gSPSegment(POLY_OPA_DISP++, 0x08, reinterpret_cast<uintptr_t>(MaterialColor(play->state.gfxCtx, look.primary)));
    gSPSegment(POLY_OPA_DISP++, 0x09, reinterpret_cast<uintptr_t>(MaterialColor(play->state.gfxCtx, look.secondary)));
    gSPSegment(POLY_OPA_DISP++, 0x0A, reinterpret_cast<uintptr_t>(MaterialColor(play->state.gfxCtx, look.accent)));
    SkelAnime_DrawSkeletonOpa(play, &resident->skelAnime, OverrideWorldLimb, PostWorldLimb, resident);
    CLOSE_DISPS(play->state.gfxCtx);
}

struct Placement {
    WorldResidentId id;
    WorldPopulationPlace place;
    Vec3f position;
    s16 yaw;
};
// Ground heights and flat footprints checked offline against the local scene
// collision resources. Runtime guards also account for live actors and doors.
// No physical business interior or rebuilt town geometry is implied here.
constexpr std::array<Placement, 12> placements = { {
    { WorldResidentId::Vessa, WorldPopulationPlace::MarketDay, { -280, 0, 400 }, 0x4000 },
    { WorldResidentId::Hadrin, WorldPopulationPlace::MarketDay, { 260, 0, 100 }, -0x4000 },
    { WorldResidentId::Pella, WorldPopulationPlace::MarketNight, { -300, 0, 400 }, 0x4000 },
    { WorldResidentId::Vessa, WorldPopulationPlace::MarketRuins, { -280, 0, 400 }, 0x4000 },
    { WorldResidentId::Hadrin, WorldPopulationPlace::MarketRuins, { 260, 0, 100 }, -0x4000 },
    { WorldResidentId::Pella, WorldPopulationPlace::MarketRuins, { -300, 0, 400 }, 0x4000 },
    { WorldResidentId::Caro, WorldPopulationPlace::Field, { 500, 0, 1700 }, -0x4000 },
    { WorldResidentId::Hollis, WorldPopulationPlace::Field, { -600, 0, 2400 }, 0x4000 },
    { WorldResidentId::Nessa, WorldPopulationPlace::Ranch, { -650, 0, -2300 }, 0x4000 },
    { WorldResidentId::Wren, WorldPopulationPlace::Ranch, { 1000, 0, 1050 }, -0x4000 },
    { WorldResidentId::Vero, WorldPopulationPlace::Lake, { -850, -1243, 7550 }, -0x4000 },
    { WorldResidentId::Edda, WorldPopulationPlace::Lake, { -2900, -1033, 3400 }, 0x4000 },
} };

bool FindClearGround(PlayState* play, const Vec3f& candidate, Vec3f& ground) {
    Vec3f probe = { candidate.x, candidate.y + 40.0f, candidate.z };
    CollisionPoly* floor = nullptr;
    const float height = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
    if (floor == nullptr || !std::isfinite(height) || height <= BGCHECK_Y_MIN ||
        std::abs(height - candidate.y) > 24.0f || floor->normal.y < 26000)
        return false;
    ground = { candidate.x, height, candidate.z };
    constexpr std::array<Vec3f, 4> edges = { { { 32, 0, 0 }, { -32, 0, 0 }, { 0, 0, 32 }, { 0, 0, -32 } } };
    for (const auto& edge : edges) {
        probe = { ground.x + edge.x, height + 24.0f, ground.z + edge.z };
        floor = nullptr;
        const float edgeHeight = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
        if (floor == nullptr || !std::isfinite(edgeHeight) || std::abs(edgeHeight - height) > 8.0f ||
            floor->normal.y < 26000)
            return false;
    }
    for (const float bodyHeight : { 24.0f, 60.0f }) {
        probe = { ground.x, height + bodyHeight, ground.z };
        if (BgCheck_SphVsFirstPoly(&play->colCtx, &probe, 22.0f))
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
            category != ACTORCAT_BOSS)
            continue;
        for (Actor* actor = play->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor->update == nullptr)
                continue;
            const auto& pos = actor->world.pos;
            const float dx = pos.x - ground.x, dz = pos.z - ground.z;
            const bool hostile = category == ACTORCAT_ENEMY || category == ACTORCAT_BOSS;
            const float clearance = hostile                       ? 450.0f
                                    : category == ACTORCAT_DOOR   ? 180.0f
                                    : category == ACTORCAT_PLAYER ? 150.0f
                                                                  : 100.0f;
            if (std::abs(pos.y - height) < (hostile ? 250.0f : 100.0f) && dx * dx + dz * dz < clearance * clearance)
                return false;
        }
    }
    return true;
}

void UpdateWorldPopulation() {
    if (!GameInteractor::IsSaveLoaded(false))
        return;
    if (spawnCooldown != 0) {
        --spawnCooldown;
        return;
    }
    spawnCooldown = 40;
    const uint16_t desired = DesiredResidents(gPlayState);
    if (desired == 0 || GameInteractor::IsGameplayPaused() || gPlayState->pauseCtx.debugState != 0 ||
        gPlayState->gameOverCtx.state != GAMEOVER_INACTIVE || gPlayState->transitionTrigger != TRANS_TRIGGER_OFF ||
        gPlayState->transitionMode != TRANS_MODE_OFF || gSaveContext.health <= 0 || worldActorId < 0 ||
        worldActorId > INT16_MAX)
        return;
    uint16_t present = 0;
    for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
        if (actor->update != nullptr && IsWorldResidentActor(actor))
            present |= WorldResidentBit(GetWorldResidentId(actor));
    }
    const auto place = PlaceForScene(gPlayState->sceneNum);
    for (const auto& placement : placements) {
        const uint16_t bit = WorldResidentBit(placement.id);
        if (placement.place != place || (desired & bit) == 0 || (present & bit) != 0)
            continue;
        Vec3f ground{};
        if (!FindClearGround(gPlayState, placement.position, ground))
            continue;
        Actor* actor = Actor_Spawn(&gPlayState->actorCtx, gPlayState, static_cast<s16>(worldActorId), ground.x,
                                   ground.y, ground.z, 0, placement.yaw, 0, static_cast<s16>(placement.id));
        if (actor != nullptr && actor->update != nullptr)
            present |= bit;
    }
}

void RegisterWorldResidents() {
    if (worldActorId >= 0 || ActorDB::Instance == nullptr || GameInteractor::Instance == nullptr)
        return;
    ActorDBInit entry;
    entry.name = "En_LivingHyruleWorldResident";
    entry.desc = "Living Hyrule townsfolk and travelers";
    entry.category = ACTORCAT_NPC;
    entry.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    entry.objectId = OBJECT_GAMEPLAY_KEEP;
    entry.instanceSize = sizeof(WorldResidentActor);
    entry.init = InitWorldResident;
    entry.destroy = DestroyWorldResident;
    entry.update = UpdateWorldResident;
    entry.draw = DrawWorldResident;
    worldActorId = ActorDB::Instance->AddEntry(entry).entry.id;
    for (size_t id = 0; id < kResidentCount; ++id) {
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
            static_cast<int32_t>(kFirstText + id), LoadText);
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
            static_cast<int32_t>(kFirstReply + id), LoadText);
    }
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { spawnCooldown = 20; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdateWorldPopulation);
}
RegisterShipInitFunc initWorldResidents(RegisterWorldResidents);

} // namespace

bool IsWorldResidentActor(const Actor* actor) {
    return worldActorId >= 0 && actor != nullptr && actor->id == worldActorId && ValidIdentity(actor);
}

WorldResidentId GetWorldResidentId(const Actor* actor) {
    return IsWorldResidentActor(actor) ? static_cast<WorldResidentId>(actor->params) : WorldResidentId::Count;
}

const char* GetWorldResidentName(WorldResidentId id) {
    static constexpr std::array<const char*, kResidentCount> names = { "Vessa", "Hadrin", "Pella", "Caro", "Hollis",
                                                                       "Nessa", "Wren",   "Vero",  "Edda" };
    return id < WorldResidentId::Count ? names[static_cast<size_t>(id)] : "Resident";
}

int GetWorldResidentPropertyId(WorldResidentId id) {
    static constexpr std::array<int, kResidentCount> properties = { 0, 1, -1, 2, 3, 4, 5, 12, -1 };
    return id < WorldResidentId::Count ? properties[static_cast<size_t>(id)] : -1;
}

} // namespace LivingHyrule
