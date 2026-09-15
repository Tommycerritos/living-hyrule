#include "PropertyScenery.h"
#include "LivingHyrule.h"
#include "SceneryPolicy.h"
#include "WorldResidents.h"

#include "soh/ActorDB.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"

#include <array>
#include <cmath>
#include <type_traits>
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/gameplay_dangeon_keep/gameplay_dangeon_keep.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

int sceneryActorId = -1;
uint8_t spawnCooldown = 40;

// Native ObjKibako uses this display list at scale 0.1 with a 12 x 27
// cylinder. Preserve that model scale; the placement checks use wider margins.
constexpr float kCrateScale = 0.1f;
constexpr std::array<Vec3f, 3> kOperatingOffsets = { {
    { -28.0f, 0.0f, -14.0f },
    { 28.0f, 0.0f, -14.0f },
    { 0.0f, 0.0f, 28.0f },
} };
constexpr std::array<Vec3f, 5> kTraderOffsets = { {
    { -112.0f, 0.0f, -144.0f },
    { 112.0f, 0.0f, -144.0f },
    { -184.0f, 0.0f, 0.0f },
    { 184.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, -192.0f },
} };

struct PropertySceneryActor {
    Actor actor;
    Vec3f crateOffsets[3];
    Vec3f anchorPosition;
    int32_t fileNum;
    uint8_t propertyId;
    uint8_t validationCooldown;
    PropertySceneryStage stage;
    bool initialized;
};
static_assert(std::is_trivial_v<PropertySceneryActor>);
static_assert(std::is_standard_layout_v<PropertySceneryActor>);

bool IsSupportedScene() {
    return GameInteractor::IsSaveLoaded(false) && (IS_VANILLA || IS_MASTER_QUEST) && !IS_CUTSCENE_LAYER &&
           CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), 0) != 0;
}

Actor* FindTrader(PlayState* play, WorldResidentId residentId) {
    if (residentId >= WorldResidentId::Count) {
        return nullptr;
    }
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
        if (actor->update != nullptr && IsWorldResidentActor(actor) && GetWorldResidentId(actor) == residentId) {
            return actor;
        }
    }
    return nullptr;
}

Vec3f OffsetPosition(const Vec3f& origin, const Vec3f& offset, s16 yaw) {
    const float sine = Math_SinS(yaw);
    const float cosine = Math_CosS(yaw);
    return { origin.x + cosine * offset.x + sine * offset.z, origin.y + offset.y,
             origin.z + cosine * offset.z - sine * offset.x };
}

bool FindDryFloor(PlayState* play, const Vec3f& candidate, Vec3f& floorPosition) {
    if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z)) {
        return false;
    }
    Vec3f probe = { candidate.x, candidate.y + 40.0f, candidate.z };
    CollisionPoly* floor = nullptr;
    const float height = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
    if (floor == nullptr || !std::isfinite(height) || height <= BGCHECK_Y_MIN ||
        std::abs(height - candidate.y) > 16.0f || floor->normal.y < 31000) {
        return false;
    }
    float waterHeight = 0.0f;
    WaterBox* water = nullptr;
    if (WaterBox_GetSurface1(play, &play->colCtx, candidate.x, candidate.z, &waterHeight, &water) &&
        (!std::isfinite(waterHeight) || waterHeight > height + 0.5f)) {
        return false;
    }
    floorPosition = { candidate.x, height, candidate.z };
    return true;
}

bool ValidateCrateGround(PlayState* play, const Vec3f& candidate, Vec3f& ground) {
    if (!FindDryFloor(play, candidate, ground)) {
        return false;
    }
    constexpr std::array<Vec3f, 4> edges = { {
        { 24.0f, 0.0f, 0.0f },
        { -24.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 24.0f },
        { 0.0f, 0.0f, -24.0f },
    } };
    for (const auto& edge : edges) {
        const Vec3f point = { ground.x + edge.x, ground.y, ground.z + edge.z };
        Vec3f edgeFloor{};
        if (!FindDryFloor(play, point, edgeFloor) || std::abs(edgeFloor.y - ground.y) > 3.0f) {
            return false;
        }
    }
    for (const float height : { 22.0f, 46.0f }) {
        Vec3f center = { ground.x, ground.y + height, ground.z };
        if (BgCheck_SphVsFirstPoly(&play->colCtx, &center, 18.0f)) {
            return false;
        }
    }
    return true;
}

bool HasActorClearance(PlayState* play, const Vec3f& position, const Actor* ignore, bool spawning) {
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (const Actor* actor = play->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor == ignore || actor->update == nullptr || (!spawning && category == ACTORCAT_PLAYER)) {
                continue;
            }
            const float dx = actor->world.pos.x - position.x;
            const float dz = actor->world.pos.z - position.z;
            const float clearance = category == ACTORCAT_DOOR                                 ? 200.0f
                                    : category == ACTORCAT_PLAYER || category == ACTORCAT_NPC ? 155.0f
                                                                                              : 125.0f;
            if (std::abs(actor->world.pos.y - position.y) < 90.0f && dx * dx + dz * dz < clearance * clearance) {
                return false;
            }
        }
    }
    return true;
}

bool FindClearArrangement(PlayState* play, const Actor* trader, const Vec3f& candidate, s16 yaw, const Actor* ignore,
                          bool spawning, Vec3f& ground, Vec3f* offsets) {
    if (!ValidateCrateGround(play, candidate, ground) || !HasActorClearance(play, ground, ignore, spawning)) {
        return false;
    }
    // The supplies must be reachable from their trader, not on the far side of
    // a wall that happens to have a floor at the same elevation.
    Vec3f from = { trader->world.pos.x, trader->world.pos.y + 30.0f, trader->world.pos.z };
    Vec3f to = { ground.x, ground.y + 30.0f, ground.z };
    Vec3f hit{};
    CollisionPoly* wall = nullptr;
    if (BgCheck_AnyLineTest1(&play->colCtx, &from, &to, &hit, &wall, false)) {
        return false;
    }
    // Validate the larger arrangement even while only the repair crate is
    // visible. A repair transaction can then change the display safely.
    for (size_t i = 0; i < kOperatingOffsets.size(); ++i) {
        Vec3f crateGround{};
        const Vec3f point = OffsetPosition(ground, kOperatingOffsets[i], yaw);
        if (!ValidateCrateGround(play, point, crateGround) || std::abs(crateGround.y - ground.y) > 8.0f ||
            !HasActorClearance(play, crateGround, ignore, spawning)) {
            return false;
        }
        offsets[i] = kOperatingOffsets[i];
        offsets[i].y = crateGround.y - ground.y;
    }
    return true;
}

PropertySceneryStage CurrentStage(uint32_t propertyId, bool traderActive) {
    return PropertySceneryStageFor(gSaveContext.ship.livingHyrule, propertyId, GetWorldProgress(), IsSupportedScene(),
                                   traderActive);
}

void InitScenery(Actor* actor, PlayState* play) {
    auto* scenery = reinterpret_cast<PropertySceneryActor*>(actor);
    if (play != gPlayState || !IsSupportedScene() || actor->params < 0 ||
        actor->params >= static_cast<s16>(WorldResidentId::Count)) {
        Actor_Kill(actor);
        return;
    }
    const auto residentId = static_cast<WorldResidentId>(actor->params);
    const int propertyId = GetWorldResidentPropertyId(residentId);
    Actor* trader = FindTrader(play, residentId);
    if (propertyId < 0 || trader == nullptr ||
        CurrentStage(static_cast<uint32_t>(propertyId), true) == PropertySceneryStage::Hidden) {
        Actor_Kill(actor);
        return;
    }
    Vec3f ground{};
    if (!FindClearArrangement(play, trader, actor->world.pos, actor->shape.rot.y, actor, true, ground,
                              scenery->crateOffsets)) {
        Actor_Kill(actor);
        return;
    }
    actor->world.pos = ground;
    scenery->anchorPosition = trader->world.pos;
    scenery->propertyId = static_cast<uint8_t>(propertyId);
    scenery->fileNum = gSaveContext.fileNum;
    scenery->stage = CurrentStage(scenery->propertyId, true);
    scenery->validationCooldown = 20;
    scenery->initialized = true;
    Actor_SetScale(actor, 1.0f);
    actor->uncullZoneForward = 1100.0f;
    actor->uncullZoneScale = 100.0f;
    actor->uncullZoneDownward = 100.0f;
    // No collider, dyna collision, pickup, attack, switch, or quest behavior.
}

void DestroyScenery(Actor*, PlayState*) {
    // Only embedded data and the shared display-list reference are used.
}

void UpdateScenery(Actor* actor, PlayState* play) {
    auto* scenery = reinterpret_cast<PropertySceneryActor*>(actor);
    if (!scenery->initialized || play != gPlayState || scenery->fileNum != gSaveContext.fileNum || actor->params < 0 ||
        actor->params >= static_cast<s16>(WorldResidentId::Count) ||
        GetWorldResidentPropertyId(static_cast<WorldResidentId>(actor->params)) != scenery->propertyId) {
        Actor_Kill(actor);
        return;
    }
    Actor* trader = FindTrader(play, static_cast<WorldResidentId>(actor->params));
    scenery->stage = CurrentStage(scenery->propertyId, trader != nullptr);
    if (scenery->stage == PropertySceneryStage::Hidden || trader == nullptr) {
        Actor_Kill(actor);
        return;
    }
    const float dx = trader->world.pos.x - scenery->anchorPosition.x;
    const float dz = trader->world.pos.z - scenery->anchorPosition.z;
    if (dx * dx + dz * dz > 40.0f * 40.0f || std::abs(trader->world.pos.y - scenery->anchorPosition.y) > 16.0f) {
        Actor_Kill(actor);
        return;
    }
    if (scenery->validationCooldown != 0) {
        --scenery->validationCooldown;
        return;
    }
    scenery->validationCooldown = 20;
    Vec3f ground{};
    if (!FindClearArrangement(play, trader, actor->world.pos, actor->shape.rot.y, actor, false, ground,
                              scenery->crateOffsets)) {
        Actor_Kill(actor);
        return;
    }
    actor->world.pos = ground;
}

extern "C" void DrawPropertySupplyCrate(PlayState* play, const Vec3f& offset) {
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(offset.x, offset.y, offset.z, MTXMODE_APPLY);
    Matrix_Scale(kCrateScale, kCrateScale, kCrateScale, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    // Ship's display-list bridge accepts the immutable __OTR__ resource name
    // through the native display-list pointer parameter.
    gSPDisplayList(POLY_OPA_DISP++, reinterpret_cast<Gfx*>(const_cast<char*>(gSmallWoodenBoxDL)));
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}

void DrawScenery(Actor* actor, PlayState* play) {
    const auto* scenery = reinterpret_cast<const PropertySceneryActor*>(actor);
    if (!scenery->initialized || scenery->stage == PropertySceneryStage::Hidden) {
        return;
    }
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    if (scenery->stage == PropertySceneryStage::AwaitingRepairs) {
        DrawPropertySupplyCrate(play, { 0.0f, 0.0f, 0.0f });
    } else if (scenery->stage == PropertySceneryStage::Operating) {
        for (const auto& offset : scenery->crateOffsets) {
            DrawPropertySupplyCrate(play, offset);
        }
    }
}

void UpdatePropertyScenery() {
    if (!IsSupportedScene()) {
        return;
    }
    if (spawnCooldown != 0) {
        --spawnCooldown;
        return;
    }
    spawnCooldown = 40;
    if (GameInteractor::IsGameplayPaused() || gPlayState->pauseCtx.debugState != 0 ||
        gPlayState->gameOverCtx.state != GAMEOVER_INACTIVE || gSaveContext.health <= 0 ||
        gPlayState->transitionTrigger != TRANS_TRIGGER_OFF || gPlayState->transitionMode != TRANS_MODE_OFF) {
        return;
    }
    uint32_t present = 0;
    uint32_t count = 0;
    for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].head; actor != nullptr; actor = actor->next) {
        if (actor->update != nullptr && IsPropertySceneryActor(actor)) {
            ++count;
            // Params is already assigned even if an object-bank load has
            // delayed InitScenery; include pending actors in duplicate checks.
            const int propertyId = actor->params >= 0 && actor->params < static_cast<s16>(WorldResidentId::Count)
                                       ? GetWorldResidentPropertyId(static_cast<WorldResidentId>(actor->params))
                                       : -1;
            if (propertyId >= 0 && static_cast<size_t>(propertyId) < kProperties.size()) {
                present |= 1u << static_cast<uint32_t>(propertyId);
            }
        }
    }
    for (Actor* trader = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; trader != nullptr; trader = trader->next) {
        if (trader->update == nullptr || !IsWorldResidentActor(trader)) {
            continue;
        }
        const auto residentId = GetWorldResidentId(trader);
        const int propertyId = GetWorldResidentPropertyId(residentId);
        if (propertyId < 0 || !CanAddPropertyScenery(present, count, static_cast<uint32_t>(propertyId)) ||
            CurrentStage(static_cast<uint32_t>(propertyId), true) == PropertySceneryStage::Hidden) {
            continue;
        }
        for (const auto& offset : kTraderOffsets) {
            const s16 yaw = trader->home.rot.y;
            const Vec3f candidate = OffsetPosition(trader->world.pos, offset, yaw);
            Vec3f ground{};
            Vec3f crateOffsets[3]{};
            if (!FindClearArrangement(gPlayState, trader, candidate, yaw, nullptr, true, ground, crateOffsets)) {
                continue;
            }
            Actor* spawned = Actor_Spawn(&gPlayState->actorCtx, gPlayState, static_cast<s16>(sceneryActorId), ground.x,
                                         ground.y, ground.z, 0, yaw, 0, static_cast<s16>(residentId));
            if (spawned != nullptr && spawned->update != nullptr) {
                present |= 1u << static_cast<uint32_t>(propertyId);
                ++count;
                break;
            }
        }
    }
}

void RegisterPropertyScenery() {
    static bool registered = false;
    if (registered || ActorDB::Instance == nullptr || GameInteractor::Instance == nullptr) {
        return;
    }
    ActorDBInit entry;
    entry.name = "En_LivingHyrulePropertySupplies";
    entry.desc = "Living Hyrule decorative business supplies";
    entry.category = ACTORCAT_PROP;
    entry.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    entry.objectId = OBJECT_GAMEPLAY_DANGEON_KEEP;
    entry.instanceSize = sizeof(PropertySceneryActor);
    entry.init = InitScenery;
    entry.destroy = DestroyScenery;
    entry.update = UpdateScenery;
    entry.draw = DrawScenery;
    sceneryActorId = ActorDB::Instance->AddEntry(entry).entry.id;
    if (sceneryActorId < 0 || sceneryActorId > INT16_MAX) {
        return;
    }
    registered = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdatePropertyScenery);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { spawnCooldown = 40; });
}

static RegisterShipInitFunc initFunc(RegisterPropertyScenery);

} // namespace

bool IsPropertySceneryActor(const Actor* actor) {
    return actor != nullptr && sceneryActorId >= 0 && actor->id == sceneryActorId;
}

} // namespace LivingHyrule
