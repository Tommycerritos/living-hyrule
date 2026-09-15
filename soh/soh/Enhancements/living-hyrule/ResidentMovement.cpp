#include "ResidentMovement.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <array>
#include <type_traits>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

static_assert(std::is_trivial_v<ResidentMovementState>);
static_assert(std::is_standard_layout_v<ResidentMovementState>);

constexpr std::array<MovementPoint, 9> footprint = { {
    { 0, 0, 0 },
    { 32, 0, 0 },
    { -32, 0, 0 },
    { 0, 0, 32 },
    { 0, 0, -32 },
    { 23, 0, 23 },
    { 23, 0, -23 },
    { -23, 0, 23 },
    { -23, 0, -23 },
} };

bool CorrectScene(PlayState* play, Actor* actor, ResidentMovementRoutine routine) {
    if (play != gPlayState || gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum < 0 ||
        gSaveContext.fileNum > 2 || !(IS_VANILLA || IS_MASTER_QUEST) || IS_CUTSCENE_LAYER ||
        play->roomCtx.curRoom.num != 0 || actor->room != 0)
        return false;
    return routine == ResidentMovementRoutine::Pella
               ? play->sceneNum == SCENE_MARKET_NIGHT || play->sceneNum == SCENE_MARKET_RUINS
               : routine == ResidentMovementRoutine::Edda && play->sceneNum == SCENE_LAKE_HYLIA;
}

bool ActivePlay(PlayState* play, const Player* player) {
    return player != nullptr && !GameInteractor::IsGameplayPaused() && play->pauseCtx.debugState == 0 &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && gSaveContext.health > 0 &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->csCtx.state == CS_STATE_IDLE && !Player_InCsMode(play) &&
           !(player->stateFlags1 &
             (PLAYER_STATE1_LOADING | PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD |
              PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE)) &&
           !(player->stateFlags2 & (PLAYER_STATE2_PAUSE_MOST_UPDATING | PLAYER_STATE2_FROZEN |
                                    PLAYER_STATE2_FORCED_VOID_OUT | PLAYER_STATE2_OCARINA_PLAYING));
}

bool HasConversation(PlayState* play, Actor* actor, const Player* player) {
    // The player's pending talk ownership starts before msgCtx.talkActor is
    // assigned. Movement must stop during that item-put-away interval too.
    return (actor->flags & ACTOR_FLAG_TALK) != 0 ||
           (player != nullptr && player->talkActor == actor && (player->stateFlags1 & PLAYER_STATE1_TALKING)) ||
           Message_GetState(&play->msgCtx) != TEXT_STATE_NONE || play->msgCtx.msgMode != 0;
}

bool SafeFloor(PlayState* play, const MovementPoint& point, float referenceHeight, float& height) {
    Vec3f probe = { point.x, referenceHeight + 16.0f, point.z };
    CollisionPoly* floor = nullptr;
    s32 bgId = BGCHECK_SCENE;
    height = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &bgId, &probe);
    if (floor == nullptr || bgId != BGCHECK_SCENE || !std::isfinite(height) || height <= BGCHECK_Y_MIN ||
        std::abs(height - referenceHeight) > 5.0f || floor->normal.y < 30000 ||
        SurfaceType_GetSceneExitIndex(&play->colCtx, floor, bgId) != 0 ||
        SurfaceType_GetFloorType(&play->colCtx, floor, bgId) != FLOOR_TYPE_0 ||
        func_80041D70(&play->colCtx, floor, bgId) != 0 || SurfaceType_IsConveyor(&play->colCtx, floor, bgId))
        return false;
    float waterHeight = 0;
    WaterBox* water = nullptr;
    return !WaterBox_GetSurface1(play, &play->colCtx, point.x, point.z, &waterHeight, &water) ||
           (std::isfinite(waterHeight) && waterHeight <= height - 2.0f);
}

float SegmentDistanceSquared(const Vec3f& position, const MovementPoint& start, const MovementPoint& end) {
    const float dx = end.x - start.x, dz = end.z - start.z;
    const float lengthSquared = dx * dx + dz * dz;
    const float t =
        lengthSquared == 0
            ? 0
            : std::clamp(((position.x - start.x) * dx + (position.z - start.z) * dz) / lengthSquared, 0.0f, 1.0f);
    const float x = position.x - start.x - t * dx, z = position.z - start.z - t * dz;
    return x * x + z * z;
}

bool ActorsClear(PlayState* play, Actor* self, const MovementPoint& start, const MovementPoint& end) {
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        const bool hostile = category == ACTORCAT_ENEMY || category == ACTORCAT_BOSS;
        const bool explosive = category == ACTORCAT_EXPLOSIVE;
        if (!hostile && !explosive && category != ACTORCAT_PLAYER && category != ACTORCAT_NPC &&
            category != ACTORCAT_DOOR && category != ACTORCAT_PROP && category != ACTORCAT_BG &&
            category != ACTORCAT_MISC)
            continue;
        for (Actor* other = play->actorCtx.actorLists[category].head; other != nullptr; other = other->next) {
            if (other == self || other->update == nullptr)
                continue;
            const float verticalClearance = hostile ? 250.0f : 110.0f;
            if (std::abs(other->world.pos.y - end.y) >= verticalClearance)
                continue;
            float clearance = hostile                       ? 450.0f
                              : explosive                   ? 200.0f
                              : category == ACTORCAT_DOOR   ? 180.0f
                              : category == ACTORCAT_PLAYER ? 160.0f
                                                            : 100.0f;
            clearance = std::max(clearance, static_cast<float>(other->colChkInfo.cylRadius) + 50.0f);
            if (SegmentDistanceSquared(other->world.pos, start, end) < clearance * clearance)
                return false;
        }
    }
    return true;
}

bool ValidateStep(PlayState* play, Actor* actor, const MovementRoute& route, const MovementPoint& current,
                  MovementStep& step) {
    float height;
    if (!SafeFloor(play, step.position, current.y, height) || std::abs(height - current.y) > 3.0f)
        return false;
    step.position.y = height;
    if (!InsideMovementCorridor(route, step.position))
        return false;
    // Check all nine floor/water probes, including diagonals. An unavailable
    // floor, dynamic platform, exit surface or small ledge means wait in place.
    for (const auto& edge : footprint) {
        MovementPoint probe = { step.position.x + edge.x, height, step.position.z + edge.z };
        float edgeHeight;
        if (!SafeFloor(play, probe, height, edgeHeight))
            return false;
    }
    for (const float bodyHeight : { 28.0f, 50.0f, 72.0f }) {
        Vec3f center = { step.position.x, height + bodyHeight, step.position.z };
        if (BgCheck_SphVsFirstPoly(&play->colCtx, &center, 25.0f))
            return false;
        Vec3f from = { current.x, current.y + bodyHeight, current.z };
        Vec3f hit;
        CollisionPoly* polygon = nullptr;
        s32 bgId;
        // Each step is at most 0.6 units, far smaller than the overlapping body
        // spheres; the swept line also rejects crossing a thin two-sided wall.
        if (BgCheck_EntityLineTest2(&play->colCtx, &from, &center, &hit, &polygon, true, true, true, false, &bgId,
                                    actor))
            return false;
    }
    return ActorsClear(play, actor, current, step.position);
}

void ApplyGait(SkelAnime* skeleton, const ResidentMovementState& state) {
    const auto gait = MovementGaitFor(state.progress.strideDistance, state.gaitWeight);
    auto* pose = skeleton->jointTable;
    pose[2].z += static_cast<s16>(gait.leftHip);
    pose[3].z += static_cast<s16>(gait.leftKnee);
    pose[4].z += static_cast<s16>(gait.leftAnkle);
    pose[5].z += static_cast<s16>(gait.rightHip);
    pose[6].z += static_cast<s16>(gait.rightKnee);
    pose[7].z += static_cast<s16>(gait.rightAnkle);
    pose[0].y -= static_cast<s16>(gait.rootDrop);
}

} // namespace

void ResetResidentMovement(ResidentMovementState& state) {
    state = {};
}

bool UpdateResidentMovement(PlayState* play, Actor* actor, ResidentMovementState& state,
                            ResidentMovementRoutine routine, SkelAnime* skeleton, bool schedulePresent) {
    if (play == nullptr || actor == nullptr || skeleton == nullptr || skeleton->jointTable == nullptr) {
        ResetResidentMovement(state);
        return false;
    }
    const auto* route = GetMovementRoute(routine);
    if (!state.initialized || state.scene != play->sceneNum || state.room != actor->room ||
        state.file != gSaveContext.fileNum || state.routine != routine) {
        ResetResidentMovement(state);
        state.initialized = 1;
        state.scene = play->sceneNum;
        state.room = actor->room;
        state.file = static_cast<int8_t>(gSaveContext.fileNum);
        state.routine = routine;
        state.progress.outward = 1;
        state.progress.waitFrames = route != nullptr ? route->stopFrames : 0;
    }
    if (state.frameSeen && state.lastFrame == play->gameplayFrames)
        return state.advancing != 0;
    state.frameSeen = 1;
    state.lastFrame = play->gameplayFrames;
    state.advancing = 0;

    // Re-evaluating the unchanged idle first removes last frame's gait. No
    // animation pointer, play speed, starting fraction or shared data changes.
    SkelAnime_Update(skeleton);
    if (route == nullptr)
        return false;
    actor->speedXZ = 0;
    actor->velocity.x = actor->velocity.z = 0;
    Player* player = GET_PLAYER(play);
    const MovementPoint current = { actor->world.pos.x, actor->world.pos.y, actor->world.pos.z };
    const float playerDistance =
        player != nullptr ? SegmentDistanceSquared(player->actor.world.pos, current, current) : 0;
    const MovementConditions conditions = {
        schedulePresent,
        ActivePlay(play, player),
        CorrectScene(play, actor, routine),
        (actor->bgCheckFlags & BGCHECKFLAG_GROUND) != 0 && std::abs(actor->velocity.y) < 0.1f &&
            actor->freezeTimer == 0,
        HasConversation(play, actor, player),
        player == nullptr || playerDistance < 160.0f * 160.0f,
    };
    const bool allowed =
        skeleton->limbCount == 16 && MovementAllowed(conditions) && InsideMovementCorridor(*route, current);
    if (!TickMovementWait(state.progress, allowed)) {
        state.gaitWeight = 0;
        return false;
    }
    auto step = PlanMovementStep(*route, state.progress, current);
    if (!step.valid || !ValidateStep(play, actor, *route, current, step)) {
        state.progress.waitFrames = 30;
        state.gaitWeight = 0;
        return false;
    }
    if (step.distance > 0.001f) {
        Vec3f target = { step.position.x, step.position.y, step.position.z };
        const s16 heading = Math_Vec3f_Yaw(&actor->world.pos, &target);
        Math_SmoothStepToS(&actor->shape.rot.y, heading, 1, 600, 1);
        actor->world.rot.y = actor->shape.rot.y;
        if (std::abs(static_cast<int>(static_cast<s16>(heading - actor->shape.rot.y))) > 0x200) {
            state.gaitWeight = 0;
            state.advancing = 1;
            return true;
        }
    }
    actor->world.pos = { step.position.x, step.position.y, step.position.z };
    actor->velocity.y = 0;
    AcceptMovementStep(state.progress, *route, step);
    if (step.distance > 0.001f) {
        state.gaitWeight = std::min(1.0f, state.gaitWeight + 0.2f);
        ApplyGait(skeleton, state);
        state.advancing = 1;
    }
    return state.advancing != 0;
}

} // namespace LivingHyrule
