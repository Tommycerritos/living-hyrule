#include "ResidentActor.h"
#include "Economy.h"
#include "Population.h"
#include "TradeDialogue.h"

#include "soh/ActorDB.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/frame_interpolation.h"

#include <cmath>
#include <string>
#include <type_traits>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/object_daiku/object_daiku.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {

namespace {

constexpr uint16_t kFirstResidentText = 0x9400;
constexpr uint16_t kFirstResidentReply = 0x9420;
constexpr size_t kResidentRoleCount = static_cast<size_t>(ResidentRole::Count);
constexpr s32 kCarpenterLimbCount = 17;
constexpr float kAngleToRadians = 3.14159265358979323846f / 32768.0f;
int residentActorId = -1;

// Actor_Spawn zeroes raw arena memory; resident instances must not own C++ objects.
struct ResidentActor {
    Actor actor;
    SkelAnime skelAnime;
    ColliderCylinder collider;
    NpcInteractInfo interactInfo;
    Vec3s jointTable[kCarpenterLimbCount];
    Vec3s morphTable[kCarpenterLimbCount];
    s16 talkState;
    u16 idleTicks;
    bool initialized;
    TradeDialogueState trade;
};

static_assert(std::is_trivial_v<ResidentActor>);
static_assert(std::is_standard_layout_v<ResidentActor>);

struct ResidentAppearance {
    const char* headDisplayList;
    Color_RGBA8 clothes;
    float scale;
    float idleSpeed;
    float initialFrameFraction;
    s16 restingTorsoTilt;
};

// These heads share the same carpenter skeleton. Colors and posture belong to
// each instance, so the original villagers' shared assets remain unchanged.
const ResidentAppearance appearances[kResidentRoleCount] = {
    { object_daiku_DL_005990, { 126, 86, 42, 255 }, 0.0103f, 0.85f, 0.00f, -120 }, // Tavin
    { object_daiku_DL_005880, { 79, 110, 143, 255 }, 0.0097f, 0.70f, 0.45f, 220 }, // Bram
    { object_daiku_DL_005AC0, { 92, 120, 68, 255 }, 0.0100f, 0.95f, 0.75f, 80 },   // Orlen
};

ColliderCylinderInit cylinderInit = {
    { COLTYPE_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_2, COLSHAPE_CYLINDER },
    { ELEMTYPE_UNK0, { 0, 0, 0 }, { 0, 0, 0 }, TOUCH_NONE, BUMP_NONE, OCELEM_ON },
    { 18, 66, 0, { 0, 0, 0 } },
};
CollisionCheckInfoInit2 collisionInfo = { 0, 0, 0, 0, MASS_IMMOVABLE };

bool IsValidRole(ResidentRole role) {
    return static_cast<size_t>(role) < kResidentRoleCount;
}

bool HasValidRole(const Actor* actor) {
    return actor->params >= 0 && actor->params < static_cast<s16>(ResidentRole::Count);
}

bool OwnsCottage() {
    const auto& economy = gSaveContext.ship.livingHyrule;
    return IsValidState(economy) && economy.ownsKakarikoCottage != 0;
}

std::string BuildDialogue(ResidentRole role) {
    const bool child = LINK_AGE_IN_YEARS == YEARS_CHILD;
    const bool restored = !child && CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW);
    const bool ownsCottage = OwnsCottage();

    switch (role) {
        case ResidentRole::Carpenter: {
            std::string text = "%gTavin%w, carpenter. ";
            text += child      ? "A village grows one sound roof at a time."
                    : restored ? "The village feels steadier now. There are roofs to mend and lives to rebuild."
                               : "Good beams are worth keeping close in hard times.";
            text += ownsCottage ? "^Your cottage deed is in order. Bram knows where to bring the rent."
                                : "^I handle the rental cottage deed. Bram would make a dependable tenant.";
            return text;
        }
        case ResidentRole::Tenant: {
            std::string text = "%gBram%w. I mend boots. ";
            text += child      ? "A worn sole tells you how far someone has walked."
                    : restored ? "People are venturing out again. That means honest work for a boot-mender."
                               : "We keep close to home these days. Even so, someone has to keep the village walking.";
            text += ownsCottage
                        ? "^So you hold the cottage deed now? A dry roof and a quiet corner suit me well. I can help "
                          "you draw spending money from the bank."
                        : "^I have my eye on that little rental cottage. A quiet corner would make a fine workshop.";
            return text;
        }
        case ResidentRole::Supplier: {
            std::string text = "%gOrlen%w. Timber, nails, and the patience to count them. ";
            text += child      ? "Tavin gets the straight beams. I keep the bent ones for smaller jobs."
                    : restored ? "Deliveries are moving again. We can think beyond the next repair."
                               : "I count every plank twice while the roads are uncertain.";
            text += ownsCottage
                        ? "^A landlord should know the tradesfolk. I can lodge your spare wallet rupees with the bank."
                        : "^Saving for a roof? I can lodge your spare wallet rupees with the bank.";
            return text;
        }
        case ResidentRole::Count:
            break;
    }
    return "Safe travels.";
}

void LoadResidentText(uint16_t* textId, bool* loadFromMessageTable) {
    const bool reply = *textId >= kFirstResidentReply && *textId < kFirstResidentReply + kResidentRoleCount;
    if (!reply && (*textId < kFirstResidentText || *textId >= kFirstResidentText + kResidentRoleCount)) {
        return;
    }
    const auto role = static_cast<ResidentRole>(*textId - (reply ? kFirstResidentReply : kFirstResidentText));
    Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
    Actor* actor = player != nullptr ? player->talkActor : nullptr;
    std::string text = "Please speak to me again when you are ready.";
    // Player's talkActor is assigned before the initial textbox opens; the
    // message context's talkActor can still point to an earlier conversation.
    if (IsResidentActor(actor) && GetResidentRole(actor) == role && actor->update != nullptr) {
        auto* resident = reinterpret_cast<ResidentActor*>(actor);
        text = reply ? std::string(resident->trade.response)
                     : BuildDialogue(role) + DescribeTradeOffer(resident->trade.offer);
    }
    CustomMessage message(text);
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

u16 GetTextId(PlayState*, Actor* actor) {
    const auto textId = static_cast<u16>(kFirstResidentText + actor->params);
    auto* resident = reinterpret_cast<ResidentActor*>(actor);
    Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
    if (player != nullptr && player->talkActor == actor && (player->stateFlags1 & PLAYER_STATE1_TALKING))
        return textId;
    switch (static_cast<ResidentRole>(actor->params)) {
        case ResidentRole::Carpenter:
            if (OwnsCottage())
                PreparePropertyTrade(resident->trade, 9, textId);
            else
                PrepareCottageTrade(resident->trade, textId);
            break;
        case ResidentRole::Tenant:
            PrepareBankTrade(resident->trade, false, textId);
            break;
        case ResidentRole::Supplier:
            PrepareBankTrade(resident->trade, true, textId);
            break;
        default:
            resident->trade = {};
            break;
    }
    return textId;
}

s16 UpdateTalkState(PlayState* play, Actor* actor) {
    const auto state = Message_GetState(&play->msgCtx);
    const Player* player = GET_PLAYER(play);
    const bool playerTalking =
        player != nullptr && player->talkActor == actor && (player->stateFlags1 & PLAYER_STATE1_TALKING);
    if (play->msgCtx.talkActor != actor || state == TEXT_STATE_NONE) {
        // Link may still be putting away an item after consuming the actor's
        // talk request. Preserve this phase until Player_SetupTalk opens text.
        return playerTalking ? NPC_TALK_STATE_TALKING : NPC_TALK_STATE_IDLE;
    }
    if (state == TEXT_STATE_CLOSING) {
        return NPC_TALK_STATE_IDLE;
    }
    auto* resident = reinterpret_cast<ResidentActor*>(actor);
    HandleTradeChoice(play, actor, resident->trade, static_cast<uint16_t>(kFirstResidentReply + actor->params));
    return NPC_TALK_STATE_TALKING;
}

void InitResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<ResidentActor*>(actor);
    const auto role = static_cast<ResidentRole>(actor->params);
    if (!HasValidRole(actor) || !ShouldResidentBePresent(play, role)) {
        Actor_Kill(actor);
        return;
    }

    const auto& appearance = appearances[static_cast<size_t>(role)];
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, 36.0f);
    Actor_SetScale(actor, appearance.scale);
    SkelAnime_InitFlex(play, &resident->skelAnime, (FlexSkeletonHeader*)object_daiku_Skel_007958,
                       (AnimationHeader*)object_daiku_Anim_001AB0, resident->jointTable, resident->morphTable,
                       kCarpenterLimbCount);
    resident->skelAnime.playSpeed = appearance.idleSpeed;
    resident->skelAnime.curFrame = resident->skelAnime.endFrame * appearance.initialFrameFraction;
    Collider_InitCylinder(play, &resident->collider);
    Collider_SetCylinder(play, &resident->collider, actor, &cylinderInit);
    CollisionCheck_SetInfo2(&actor->colChkInfo, nullptr, &collisionInfo);
    resident->initialized = true;
    actor->targetMode = 6;
    actor->gravity = -1.0f;
    actor->uncullZoneForward = 1200.0f;
    actor->textId = GetTextId(play, actor);
    Actor_SetFocus(actor, 60.0f);
}

void DestroyResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<ResidentActor*>(actor);
    if (!resident->initialized) {
        return;
    }
    // A dynamic ActorDB ID is absent from the stock skeleton-cleanup switch.
    // Joint/morph tables are embedded: unregister them, never arena-free them.
    ResourceMgr_UnregisterSkeleton(&resident->skelAnime);
    Collider_DestroyCylinder(play, &resident->collider);
    resident->initialized = false;
}

void UpdateResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<ResidentActor*>(actor);
    if (!resident->initialized || play != gPlayState) {
        return;
    }
    const auto role = static_cast<ResidentRole>(actor->params);
    const bool present = HasValidRole(actor) && ShouldResidentBePresent(play, role);
    const Player* player = GET_PLAYER(play);
    const bool talking =
        (play->msgCtx.talkActor == actor && Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) ||
        (player != nullptr && player->talkActor == actor && (player->stateFlags1 & PLAYER_STATE1_TALKING));
    const bool requested = (actor->flags & ACTOR_FLAG_TALK) != 0;
    if (!present && !talking && !requested) {
        Actor_Kill(actor);
        return;
    }

    SkelAnime_Update(&resident->skelAnime);
    ++resident->idleTicks;
    Actor_MoveXZGravity(actor);
    Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 50.0f, 4);
    Collider_UpdateCylinder(actor, &resident->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &resident->collider.base);
    Actor_SetFocus(actor, 60.0f);

    if (GET_PLAYER(play) != nullptr) {
        resident->interactInfo.trackPos = GET_PLAYER(play)->actor.focus.pos;
        Npc_TrackPoint(actor, &resident->interactInfo, 0,
                       talking                          ? NPC_TRACKING_FULL_BODY
                       : actor->xzDistToPlayer < 180.0f ? NPC_TRACKING_HEAD_AND_TORSO
                                                        : NPC_TRACKING_NONE);
        // Once a schedule expires, finish existing dialogue without offering a
        // fresh conversation. The standard helper owns the normal talk prompt.
        if (present || resident->talkState != NPC_TALK_STATE_IDLE || requested) {
            Npc_UpdateTalking(play, actor, &resident->talkState, 100.0f, GetTextId, UpdateTalkState);
        }
    }
}

s32 OverrideLimb(PlayState*, s32 limbIndex, Gfx**, Vec3f*, Vec3s* rotation, void* actorRef) {
    auto* resident = static_cast<ResidentActor*>(actorRef);
    const auto& appearance = appearances[static_cast<size_t>(resident->actor.params)];
    if (limbIndex == 8) {
        Matrix_RotateX(-resident->interactInfo.torsoRot.y * kAngleToRadians, MTXMODE_APPLY);
        Matrix_RotateZ(-resident->interactInfo.torsoRot.x * kAngleToRadians, MTXMODE_APPLY);
        rotation->z += appearance.restingTorsoTilt;
        rotation->z += static_cast<s16>(Math_SinS(static_cast<s16>(resident->idleTicks * 350u)) * 60.0f);
    } else if (limbIndex == 15) {
        Matrix_Translate(1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateX(resident->interactInfo.headRot.y * kAngleToRadians, MTXMODE_APPLY);
        Matrix_RotateZ(resident->interactInfo.headRot.x * kAngleToRadians, MTXMODE_APPLY);
        Matrix_Translate(-1400.0f, 0.0f, 0.0f, MTXMODE_APPLY);
    }
    return false;
}

extern "C" void PostLimb(PlayState* play, s32 limbIndex, Gfx**, Vec3s*, void* actorRef) {
    if (limbIndex != 15) {
        return;
    }
    auto* resident = static_cast<ResidentActor*>(actorRef);
    const auto& appearance = appearances[static_cast<size_t>(resident->actor.params)];
    Vec3f focus = { 700.0f, 1100.0f, 0.0f };
    Matrix_MultVec3f(&focus, &resident->actor.focus.pos);
    OPEN_DISPS(play->state.gfxCtx);
    // Ship's graphics bridge accepts an __OTR__ resource name through the same
    // pointer parameter as a native display list; it does not modify the name.
    gSPDisplayList(POLY_OPA_DISP++, reinterpret_cast<Gfx*>(const_cast<char*>(appearance.headDisplayList)));
    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" void DrawResident(Actor* actor, PlayState* play) {
    auto* resident = reinterpret_cast<ResidentActor*>(actor);
    if (!resident->initialized || !HasValidRole(actor)) {
        return;
    }
    const auto& appearance = appearances[static_cast<size_t>(actor->params)];
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, appearance.clothes.r, appearance.clothes.g, appearance.clothes.b, 255);
    SkelAnime_DrawSkeletonOpa(play, &resident->skelAnime, OverrideLimb, PostLimb, resident);
    CLOSE_DISPS(play->state.gfxCtx);
}

} // namespace

int RegisterResidentActor() {
    if (residentActorId >= 0) {
        return residentActorId;
    }
    if (ActorDB::Instance == nullptr || GameInteractor::Instance == nullptr) {
        return -1;
    }
    ActorDBInit entry;
    entry.name = "En_LivingHyruleResident";
    entry.desc = "Living Hyrule resident";
    entry.category = ACTORCAT_NPC;
    entry.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    entry.objectId = OBJECT_DAIKU;
    entry.instanceSize = sizeof(ResidentActor);
    entry.init = InitResident;
    entry.destroy = DestroyResident;
    entry.update = UpdateResident;
    entry.draw = DrawResident;
    residentActorId = ActorDB::Instance->AddEntry(entry).entry.id;
    for (size_t role = 0; role < kResidentRoleCount; ++role) {
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
            static_cast<int32_t>(kFirstResidentText + role), LoadResidentText);
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
            static_cast<int32_t>(kFirstResidentReply + role), LoadResidentText);
    }
    return residentActorId;
}

Actor* SpawnResident(PlayState* play, ResidentRole role, const Vec3f& position, s16 yaw) {
    if (play == nullptr || play != gPlayState || !GameInteractor::IsSaveLoaded(false) || !IsValidRole(role) ||
        !ShouldResidentBePresent(play, role) || !std::isfinite(position.x) || !std::isfinite(position.y) ||
        !std::isfinite(position.z)) {
        return nullptr;
    }
    const int actorId = RegisterResidentActor();
    if (actorId < 0 || actorId > INT16_MAX) {
        return nullptr;
    }
    return Actor_Spawn(&play->actorCtx, play, static_cast<s16>(actorId), position.x, position.y, position.z, 0, yaw, 0,
                       static_cast<s16>(role));
}

bool IsResidentActor(const Actor* actor) {
    return actor != nullptr && residentActorId >= 0 && actor->id == residentActorId && HasValidRole(actor);
}

ResidentRole GetResidentRole(const Actor* actor) {
    return IsResidentActor(actor) ? static_cast<ResidentRole>(actor->params) : ResidentRole::Count;
}

} // namespace LivingHyrule
