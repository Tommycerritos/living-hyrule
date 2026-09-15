#include "RoyalAudience.h"
#include "LivingHyrule.h"
#include "ResidentSocial.h"

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
#include "objects/object_zl2/object_zl2.h"
#include "objects/object_zl2_anime2/object_zl2_anime2.h"
#include "objects/object_sd/object_sd.h"
#include "objects/object_cne/object_cne.h"
#include "objects/object_os_anime/object_os_anime.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {
constexpr uint16_t kFirstText = 0x9900;
// 0x9910..0x9912 and 0x9920..0x9922 are reserved for later offers and replies.
// The audience shares resident greetings and recorded recovery relationships.
// It does not sell a castle deed or run Zelda's original escape behavior.
constexpr int kResidentCount = static_cast<int>(RoyalResidentId::Count);
constexpr float kRadians = 3.14159265358979323846f / 32768.0f;
std::array<int, kResidentCount> actorIds = { -1, -1, -1 };
uint8_t spawnCooldown = 20;

// Arena allocated and zeroed by Actor_Spawn. Zelda uses 15 entries, Aren 17,
// and Maelin 16. The model resources are shared; behavior and pose are our own.
struct RoyalActor {
    Actor actor;
    SkelAnime skelAnime;
    ColliderCylinder collider;
    NpcInteractInfo interact;
    Vec3s joints[17];
    Vec3s morphs[17];
    s16 talkState;
    u16 ticks;
    uint8_t eyeIndex;
    bool initialized;
};
static_assert(std::is_trivial_v<RoyalActor>);
static_assert(std::is_standard_layout_v<RoyalActor>);

bool ValidIdentity(const Actor* actor) {
    return actor != nullptr && actor->params >= 0 && actor->params < kResidentCount;
}

uint8_t DesiredResidents(const PlayState* play) {
    RoyalAudienceContext context;
    context.enabled = CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), 0) != 0;
    context.supportedAdventure = IS_VANILLA || IS_MASTER_QUEST;
    context.normalScene = play != nullptr && play == gPlayState && gSaveContext.gameMode == GAMEMODE_NORMAL &&
                          gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && !IS_CUTSCENE_LAYER &&
                          play->roomCtx.curRoom.num == 0;
    context.castleApproach = play != nullptr && play->sceneNum == SCENE_OUTSIDE_GANONS_CASTLE;
    context.daytime = IS_DAY;
    context.world = GetWorldProgress();
    context.economy = gSaveContext.ship.livingHyrule;
    return RoyalResidentMaskFor(context);
}

bool PlayerHasTalk(PlayState* play, const Actor* actor) {
    return GET_PLAYER(play) != nullptr && GET_PLAYER(play)->talkActor == actor &&
           (GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_TALKING) != 0;
}

std::string RecoveryDialogue(RoyalResidentId id) {
    const auto& economy = gSaveContext.ship.livingHyrule;
    const auto world = GetWorldProgress();
    if (!IsValidState(economy))
        return "";
    unsigned int owned = 0;
    unsigned int working = 0;
    for (uint32_t property = 0; property < kProperties.size(); ++property) {
        owned += OwnsProperty(economy, property) ? 1u : 0u;
        working += PropertyOperating(economy, property, world) ? 1u : 0u;
    }
    const unsigned int marketWorking =
        (PropertyOperating(economy, 0, world) ? 1u : 0u) + (PropertyOperating(economy, 1, world) ? 1u : 0u);
    if (id == RoyalResidentId::Zelda) {
        if (economy.marketRestored)
            return "^The Market's restoration is funded. There are still homes and livelihoods to mend, but "
                   "you have given the returning families a beginning. The castle remains a separate task.";
        if (marketWorking == 2)
            return "^Vessa's stall and Hadrin's guesthouse are working again through your investment. Those small "
                   "beginnings matter. The castle itself still lies in ruin.";
        if (marketWorking != 0)
            return "^One of your Market businesses is working again. A returned livelihood is a real beginning, "
                   "even while the castle above us remains in ruin.";
        if (OwnsProperty(economy, 0) || OwnsProperty(economy, 1))
            return "^You hold property in the Market. Speak with its managers about repairs when you are ready; "
                   "a deed alone cannot put their rooms and stalls back to work.";
        return "^If you wish to help the Market, Vessa and Hadrin can explain what their businesses need. "
               "You need no title or fortune to visit me here.";
    }
    if (id == RoyalResidentId::Maelin) {
        std::string text =
            "^Your regional deeds: " + std::to_string(owned) + ". Working businesses: " + std::to_string(working) + ".";
        if (economy.ownsKakarikoCottage)
            text += " Your Kakariko cottage is recorded separately.";
        if (owned > working)
            text += " Some holdings still await repairs or their region's recovery.";
        text += "^Repairs belong to the local managers. I keep the household's records; there is no castle deed "
                "to purchase here.";
        return text;
    }
    return marketWorking != 0
               ? "^Your reopened Market business gives returning families a foothold. Our watch stays on this "
                 "approach; the castle ruins are not a new residence yet."
               : "^The Market is safe after your victory, but its recovery still needs patient work. The castle "
                 "has not been rebuilt.";
}

std::string BuildDialogue(RoyalResidentId id) {
    std::string text = "%g" + std::string(GetRoyalResidentName(id)) + "%w. ";
    switch (id) {
        case RoyalResidentId::Zelda:
            text += "It is good to see you again. Defeating Ganon gave Hyrule a future; now we must care for the "
                    "people who will live in it.";
            text += "^I meet visitors here by day while the royal household works from the castle approach. "
                    "Please tell me what you have seen on your travels.";
            break;
        case RoyalResidentId::Aren:
            text += IS_DAY ? "Captain of the relief watch. Zelda is receiving visitors here today. Keep the "
                             "road clear for the families bringing their petitions."
                           : "The household has finished its day's work. Zelda and Maelin will return to this "
                             "spot by daylight. I will keep watch until then.";
            break;
        case RoyalResidentId::Maelin:
            text += "Steward of the royal household. I keep two lists: what we have, and what people still need. "
                    "The second is longer, but every working livelihood helps.";
            break;
        default:
            return "Safe travels.";
    }
    return text + RecoveryDialogue(id);
}

void LoadText(uint16_t* textId, bool* loadFromMessageTable) {
    if (*textId < kFirstText || *textId >= kFirstText + kResidentCount)
        return;
    const auto id = static_cast<RoyalResidentId>(*textId - kFirstText);
    // Player owns the speaker before OnOpenText; msgCtx may still reference an
    // earlier conversation. Validate the actor family and identity before text.
    Actor* speaker =
        gPlayState != nullptr && GET_PLAYER(gPlayState) != nullptr ? GET_PLAYER(gPlayState)->talkActor : nullptr;
    CustomMessage message(IsRoyalResidentActor(speaker) && GetRoyalResidentId(speaker) == id
                              ? BuildDialogue(id) + ResidentGreeting(speaker)
                              : "Let's speak again in a moment.");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

u16 GetTextId(PlayState*, Actor* actor) {
    return static_cast<u16>(kFirstText + actor->params);
}

s16 UpdateTalkState(PlayState* play, Actor* actor) {
    // Preserve the accepted request while Link puts away an item. There may
    // still be no textbox, or msgCtx may belong to the previous speaker.
    if (PlayerHasTalk(play, actor))
        return NPC_TALK_STATE_TALKING;
    const auto state = Message_GetState(&play->msgCtx);
    return play->msgCtx.talkActor != actor || state == TEXT_STATE_NONE || state == TEXT_STATE_CLOSING
               ? NPC_TALK_STATE_IDLE
               : NPC_TALK_STATE_TALKING;
}

ColliderCylinderInit cylinderInit = {
    { COLTYPE_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_2, COLSHAPE_CYLINDER },
    { ELEMTYPE_UNK0, { 0, 0, 0 }, { 0, 0, 0 }, TOUCH_NONE, BUMP_NONE, OCELEM_ON },
    { 20, 70, 0, { 0, 0, 0 } },
};
CollisionCheckInfoInit2 collisionInfo = { 0, 0, 0, 0, MASS_IMMOVABLE };

void InitRoyalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<RoyalActor*>(actor);
    const auto id = static_cast<RoyalResidentId>(actor->params);
    if (!ValidIdentity(actor) || (DesiredResidents(play) & RoyalResidentBit(id)) == 0) {
        Actor_Kill(actor);
        return;
    }
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, 25.0f);
    Actor_SetScale(actor, 0.01f);
    // Native asset references resolve directly through the resource bridge.
    // None of the original Zelda escape, ending, or guard state machines run.
    if (id == RoyalResidentId::Zelda) {
        SkelAnime_InitFlex(play, &resident->skelAnime, (FlexSkeletonHeader*)gZelda2Skel,
                           (AnimationHeader*)gZelda2Anime2Anim_009FBC, resident->joints, resident->morphs, 15);
        resident->skelAnime.playSpeed = 0.8f;
    } else if (id == RoyalResidentId::Aren) {
        SkelAnime_Init(play, &resident->skelAnime, (SkeletonHeader*)gEnHeishiSkel, (AnimationHeader*)gEnHeishiIdleAnim,
                       resident->joints, resident->morphs, 17);
        resident->skelAnime.curFrame = resident->skelAnime.endFrame * 0.3f;
    } else {
        SkelAnime_InitFlex(play, &resident->skelAnime, (FlexSkeletonHeader*)gCneSkel, (AnimationHeader*)gObjOsAnim_4E90,
                           resident->joints, resident->morphs, 16);
        resident->skelAnime.playSpeed = 0.65f;
        resident->skelAnime.curFrame = resident->skelAnime.endFrame * 0.6f;
    }
    Collider_InitCylinder(play, &resident->collider);
    Collider_SetCylinder(play, &resident->collider, actor, &cylinderInit);
    resident->collider.dim.radius = id == RoyalResidentId::Zelda ? 25 : id == RoyalResidentId::Aren ? 15 : 20;
    resident->collider.dim.height = id == RoyalResidentId::Zelda ? 80 : id == RoyalResidentId::Aren ? 70 : 62;
    CollisionCheck_SetInfo2(&actor->colChkInfo, nullptr, &collisionInfo);
    resident->initialized = true;
    actor->targetMode = 6;
    actor->gravity = -1.0f;
    actor->uncullZoneForward = 1800.0f;
    actor->textId = GetTextId(play, actor);
    Actor_SetFocus(actor, 60.0f);
}

void DestroyRoyalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<RoyalActor*>(actor);
    if (!resident->initialized)
        return;
    ResourceMgr_UnregisterSkeleton(&resident->skelAnime);
    Collider_DestroyCylinder(play, &resident->collider);
    resident->initialized = false;
}

void UpdateRoyalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<RoyalActor*>(actor);
    if (!resident->initialized || play != gPlayState)
        return;
    const bool present = ValidIdentity(actor) &&
                         (DesiredResidents(play) & RoyalResidentBit(static_cast<RoyalResidentId>(actor->params))) != 0;
    const bool talking = PlayerHasTalk(play, actor) ||
                         (play->msgCtx.talkActor == actor && Message_GetState(&play->msgCtx) != TEXT_STATE_NONE);
    const bool requested = (actor->flags & ACTOR_FLAG_TALK) != 0;
    if (!present && !talking && !requested) {
        Actor_Kill(actor);
        return;
    }
    SkelAnime_Update(&resident->skelAnime);
    ++resident->ticks;
    const unsigned int blink = resident->ticks % 127u;
    resident->eyeIndex = blink == 0 ? 1 : blink == 1 ? 2 : blink == 2 ? 1 : 0;
    Actor_MoveXZGravity(actor);
    Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 50.0f, 4);
    Collider_UpdateCylinder(actor, &resident->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &resident->collider.base);
    Actor_SetFocus(actor, 60.0f);
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

// The adult Zelda head draws seven articulated hair pieces using segment 0xC.
// Adapt only the native static-hair geometry from En_Zl3::func_80B5944C. Its
// cutscene physics and state are neither required nor copied into our actor.
extern "C" s32 OverrideRoyalZeldaLimb(PlayState* play, s32 limb, Gfx**, Vec3f* pos, Vec3s* rot, void* actorRef,
                                      Gfx** gfx) {
    auto* resident = static_cast<RoyalActor*>(actorRef);
    if (limb == 7) {
        rot->x += resident->interact.torsoRot.y;
        rot->z += resident->interact.torsoRot.x;
    }
    if (limb != 14)
        return false;
    // Matrix_ToMtx's debug filename parameter predates const-correctness.
    static char sourceFile[] = __FILE__;
    auto* matrices = static_cast<Mtx*>(Graph_Alloc(play->state.gfxCtx, 7 * sizeof(Mtx)));
    gSPSegment((*gfx)++, 0x0C, reinterpret_cast<uintptr_t>(matrices));
    rot->x += resident->interact.headRot.y;
    rot->z += resident->interact.headRot.x;
    Matrix_Push();
    Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_APPLY);
    Matrix_RotateZYX(rot->x, rot->y, rot->z, MTXMODE_APPLY);
    Matrix_Push();
    Matrix_Translate(174.0f, -317.0f, 0.0f, MTXMODE_APPLY);
    Matrix_ToMtx(&matrices[0], sourceFile, __LINE__);
    Matrix_Translate(-410.0f, -184.0f, 0.0f, MTXMODE_APPLY);
    Matrix_ToMtx(&matrices[1], sourceFile, __LINE__);
    Matrix_Translate(-1019.0f, -26.0f, 0.0f, MTXMODE_APPLY);
    Matrix_ToMtx(&matrices[2], sourceFile, __LINE__);
    Matrix_Pop();
    Matrix_Push();
    Matrix_Translate(40.0f, 264.0f, 386.0f, MTXMODE_APPLY);
    Matrix_ToMtx(&matrices[3], sourceFile, __LINE__);
    Matrix_Translate(-446.0f, -52.0f, 84.0f, MTXMODE_APPLY);
    Matrix_ToMtx(&matrices[4], sourceFile, __LINE__);
    Matrix_Pop();
    Matrix_Push();
    Matrix_Translate(40.0f, 264.0f, -386.0f, MTXMODE_APPLY);
    Matrix_ToMtx(&matrices[5], sourceFile, __LINE__);
    Matrix_Translate(-446.0f, -52.0f, -84.0f, MTXMODE_APPLY);
    Matrix_ToMtx(&matrices[6], sourceFile, __LINE__);
    Matrix_Pop();
    Matrix_Pop();
    return false;
}

extern "C" void PostRoyalZeldaLimb(PlayState*, s32 limb, Gfx**, Vec3s*, void* actorRef, Gfx**) {
    if (limb != 14)
        return;
    auto* resident = static_cast<RoyalActor*>(actorRef);
    Vec3f focus = { 0, 10, 0 };
    Matrix_MultVec3f(&focus, &resident->actor.focus.pos);
}

s32 OverrideRoyalStaffLimb(PlayState*, s32 limb, Gfx** list, Vec3f*, Vec3s* rot, void* actorRef) {
    auto* resident = static_cast<RoyalActor*>(actorRef);
    if (resident->actor.params == static_cast<s16>(RoyalResidentId::Aren)) {
        if (limb == 9)
            rot->x += resident->interact.torsoRot.y;
        if (limb == 16) {
            rot->x += resident->interact.headRot.y;
            rot->z += resident->interact.headRot.x;
        }
    } else if (limb == 15) {
        *list = reinterpret_cast<Gfx*>(const_cast<char*>(gCneHeadBrownHairDL));
        Matrix_Translate(1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateX(resident->interact.headRot.y * kRadians, MTXMODE_APPLY);
        Matrix_RotateZ(resident->interact.headRot.x * kRadians, MTXMODE_APPLY);
        Matrix_Translate(-1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
    } else if (limb == 8) {
        Matrix_RotateX(-resident->interact.torsoRot.y * kRadians, MTXMODE_APPLY);
        Matrix_RotateZ(resident->interact.torsoRot.x * kRadians, MTXMODE_APPLY);
    }
    return false;
}

extern "C" void PostRoyalStaffLimb(PlayState*, s32 limb, Gfx**, Vec3s*, void* actorRef) {
    auto* resident = static_cast<RoyalActor*>(actorRef);
    if (resident->actor.params != static_cast<s16>(RoyalResidentId::Maelin) || limb != 15)
        return;
    Vec3f focus = { 400, 0, 0 };
    Matrix_MultVec3f(&focus, &resident->actor.focus.pos);
}

Gfx* StaffMaterial(GraphicsContext* graphics, uint8_t r, uint8_t g, uint8_t b) {
    auto* list = static_cast<Gfx*>(Graph_Alloc(graphics, 2 * sizeof(Gfx)));
    gDPSetEnvColor(list, r, g, b, 0);
    gSPEndDisplayList(list + 1);
    return list;
}

extern "C" void DrawRoyalResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<RoyalActor*>(actor);
    if (!resident->initialized || !ValidIdentity(actor))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    if (actor->params == static_cast<s16>(RoyalResidentId::Zelda)) {
        static const char* eyes[] = { gZelda2EyeOpenTex, gZelda2EyeHalfTex, gZelda2EyeShutTex };
        gSPSegment(POLY_OPA_DISP++, 0x08, reinterpret_cast<uintptr_t>(eyes[resident->eyeIndex]));
        gSPSegment(POLY_OPA_DISP++, 0x09, reinterpret_cast<uintptr_t>(eyes[resident->eyeIndex]));
        gSPSegment(POLY_OPA_DISP++, 0x0A, reinterpret_cast<uintptr_t>(gZelda2MouthSeriousTex));
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
        gSPSegment(POLY_OPA_DISP++, 0x0B, reinterpret_cast<uintptr_t>(&D_80116280[2]));
        POLY_OPA_DISP =
            SkelAnime_DrawFlex(play, resident->skelAnime.skeleton, resident->joints, resident->skelAnime.dListCount,
                               OverrideRoyalZeldaLimb, PostRoyalZeldaLimb, resident, POLY_OPA_DISP);
    } else {
        if (actor->params == static_cast<s16>(RoyalResidentId::Maelin)) {
            Matrix_Translate(0.0f, 0.0f, 700.0f, MTXMODE_APPLY);
            gSPSegment(POLY_OPA_DISP++, 0x08,
                       reinterpret_cast<uintptr_t>(StaffMaterial(play->state.gfxCtx, 56, 65, 110)));
            gSPSegment(POLY_OPA_DISP++, 0x09,
                       reinterpret_cast<uintptr_t>(StaffMaterial(play->state.gfxCtx, 216, 204, 171)));
            gSPSegment(POLY_OPA_DISP++, 0x0A,
                       reinterpret_cast<uintptr_t>(StaffMaterial(play->state.gfxCtx, 56, 65, 110)));
        }
        SkelAnime_DrawSkeletonOpa(play, &resident->skelAnime, OverrideRoyalStaffLimb, PostRoyalStaffLimb, resident);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

struct Placement {
    RoyalResidentId id;
    Vec3f position;
    s16 yaw;
};
// Verified against the local ganon_tou base collision and actor entries. This
// roadside audience is not a restored castle interior. No scene is replaced.
constexpr std::array<Placement, kResidentCount> placements = { {
    { RoyalResidentId::Zelda, { -180, 1241, 1980 }, 0 },
    { RoyalResidentId::Aren, { -400, 1233, 2000 }, 0 },
    { RoyalResidentId::Maelin, { 0, 1250, 2000 }, 0 },
} };

bool ClearGround(PlayState* play, const Vec3f& candidate, Vec3f& ground) {
    Vec3f probe = { candidate.x, candidate.y + 40.0f, candidate.z };
    CollisionPoly* floor = nullptr;
    const float y = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
    if (floor == nullptr || !std::isfinite(y) || y <= BGCHECK_Y_MIN || std::abs(y - candidate.y) > 24.0f ||
        floor->normal.y < 26000)
        return false;
    ground = { candidate.x, y, candidate.z };
    constexpr std::array<Vec3f, 8> offsets = { { { 44, 0, 0 },
                                                 { -44, 0, 0 },
                                                 { 0, 0, 44 },
                                                 { 0, 0, -44 },
                                                 { 32, 0, 32 },
                                                 { -32, 0, 32 },
                                                 { 32, 0, -32 },
                                                 { -32, 0, -32 } } };
    for (const auto& offset : offsets) {
        probe = { ground.x + offset.x, y + 24.0f, ground.z + offset.z };
        floor = nullptr;
        const float edge = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
        if (floor == nullptr || !std::isfinite(edge) || std::abs(edge - y) > 8.0f || floor->normal.y < 26000)
            return false;
    }
    for (const float height : { 32.0f, 60.0f, 85.0f }) {
        probe = { ground.x, y + height, ground.z };
        if (BgCheck_SphVsFirstPoly(&play->colCtx, &probe, 25.0f))
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
        gPlayState->transitionMode != TRANS_MODE_OFF || gSaveContext.health <= 0 ||
        gPlayState->csCtx.state != CS_STATE_IDLE || Player_InCsMode(gPlayState) ||
        Message_GetState(&gPlayState->msgCtx) != TEXT_STATE_NONE)
        return;
    uint8_t present = 0;
    for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
        if (actor->update != nullptr && IsRoyalResidentActor(actor))
            present |= RoyalResidentBit(GetRoyalResidentId(actor));
    }
    for (const auto& placement : placements) {
        const uint8_t bit = RoyalResidentBit(placement.id);
        if ((desired & bit) == 0 || (present & bit) != 0)
            continue;
        const int actorId = actorIds[static_cast<size_t>(placement.id)];
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

void RegisterRoyalAudience() {
    static bool registered = false;
    if (registered || ActorDB::Instance == nullptr || GameInteractor::Instance == nullptr)
        return;
    static constexpr const char* names[] = { "En_LivingHyruleRoyalZelda", "En_LivingHyruleRoyalGuard",
                                             "En_LivingHyruleRoyalSteward" };
    static constexpr s16 objects[] = { OBJECT_ZL2, OBJECT_SD, OBJECT_CNE };
    for (int id = 0; id < kResidentCount; ++id) {
        ActorDBInit entry;
        entry.name = names[id];
        entry.desc = GetRoyalResidentName(static_cast<RoyalResidentId>(id));
        entry.category = ACTORCAT_NPC;
        entry.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
        entry.objectId = objects[id];
        entry.instanceSize = sizeof(RoyalActor);
        entry.init = InitRoyalResident;
        entry.destroy = DestroyRoyalResident;
        entry.update = UpdateRoyalResident;
        entry.draw = DrawRoyalResident;
        actorIds[id] = ActorDB::Instance->AddEntry(entry).entry.id;
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(kFirstText + id, LoadText);
    }
    registered = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { spawnCooldown = 20; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdatePopulation);
}
RegisterShipInitFunc initRoyalAudience(RegisterRoyalAudience);
} // namespace

bool IsRoyalResidentActor(const Actor* actor) {
    if (!ValidIdentity(actor))
        return false;
    const int expectedId = actorIds[static_cast<size_t>(actor->params)];
    return expectedId >= 0 && actor->id == expectedId;
}

RoyalResidentId GetRoyalResidentId(const Actor* actor) {
    return IsRoyalResidentActor(actor) ? static_cast<RoyalResidentId>(actor->params) : RoyalResidentId::Count;
}

const char* GetRoyalResidentName(RoyalResidentId id) {
    static constexpr const char* names[] = { "Zelda", "Captain Aren", "Maelin" };
    return id < RoyalResidentId::Count ? names[static_cast<size_t>(id)] : "Royal resident";
}

} // namespace LivingHyrule
