#include "Population.h"
#include "PopulationPolicy.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"

#include <array>
#include <cmath>
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {

static_assert(static_cast<unsigned int>(ResidentRole::Carpenter) == 0);
static_assert(static_cast<unsigned int>(ResidentRole::Tenant) == 1);
static_assert(static_cast<unsigned int>(ResidentRole::Supplier) == 2);
static_assert(static_cast<unsigned int>(ResidentRole::Count) == 3);

struct ResidentPlacement {
    ResidentRole role;
    Vec3f position;
    s16 yaw;
};

// Authored positions in the lower village: worksite edge, residential lane,
// and approach from Hyrule Field. Validate floor and clearance in the loaded
// scene before spawning; these are not copies of existing quest actors.
static constexpr std::array<ResidentPlacement, 3> kKakarikoResidents = { {
    { ResidentRole::Carpenter, { -680.0f, 0.0f, 610.0f }, 0x4000 },
    { ResidentRole::Tenant, { -190.0f, 80.0f, 930.0f }, -0x4000 },
    { ResidentRole::Supplier, { -1570.0f, 80.0f, 780.0f }, 0x4000 },
} };

static uint8_t DesiredResidents(const PlayState* play) {
    const PopulationPhase phase = LINK_IS_CHILD                              ? PopulationPhase::Child
                                  : CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW) ? PopulationPhase::AdultRecovered
                                                                             : PopulationPhase::AdultCrisis;
    const bool normalScene = play != nullptr && gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum >= 0 &&
                             gSaveContext.fileNum <= 2 && !IS_CUTSCENE_LAYER;
    return ResidentMaskFor(CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), 0) != 0,
                           IS_VANILLA || IS_MASTER_QUEST, play != nullptr && play->sceneNum == SCENE_KAKARIKO_VILLAGE,
                           normalScene, IS_DAY, phase);
}

bool ShouldResidentBePresent(const PlayState* play, ResidentRole role) {
    const auto index = static_cast<unsigned int>(role);
    return index < static_cast<unsigned int>(ResidentRole::Count) && (DesiredResidents(play) & (1u << index)) != 0;
}

static bool FindClearGround(PlayState* play, const Vec3f& candidate, Vec3f& ground) {
    Vec3f probe = candidate;
    probe.y += 40.0f;
    CollisionPoly* floor = nullptr;
    const float height = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
    if (floor == nullptr || !std::isfinite(height) || height <= BGCHECK_Y_MIN ||
        std::abs(height - candidate.y) > 24.0f || floor->normal.y < 26000) {
        return false;
    }
    ground = { candidate.x, height, candidate.z };

    // Reject narrow ledges, slopes, walls, and low ceilings around the body.
    static constexpr std::array<Vec3f, 4> offsets = { {
        { 32.0f, 0.0f, 0.0f },
        { -32.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 32.0f },
        { 0.0f, 0.0f, -32.0f },
    } };
    for (const Vec3f& offset : offsets) {
        probe = { ground.x + offset.x, ground.y + 24.0f, ground.z + offset.z };
        floor = nullptr;
        const float edgeHeight = BgCheck_EntityRaycastFloor1(&play->colCtx, &floor, &probe);
        if (floor == nullptr || !std::isfinite(edgeHeight) || std::abs(edgeHeight - height) > 8.0f) {
            return false;
        }
    }
    for (const float bodyHeight : { 24.0f, 60.0f }) {
        probe = { ground.x, ground.y + bodyHeight, ground.z };
        if (BgCheck_SphVsFirstPoly(&play->colCtx, &probe, 20.0f)) {
            return false;
        }
    }

    // Keep doorways, quest characters, props, and Link clear. Never move a
    // vanilla actor or force an NPC into an occupied position.
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        if (category != ACTORCAT_NPC && category != ACTORCAT_DOOR && category != ACTORCAT_PROP &&
            category != ACTORCAT_PLAYER && category != ACTORCAT_BG) {
            continue;
        }
        for (Actor* actor = play->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
            if (actor->update == nullptr) {
                continue;
            }
            const Vec3f& pos = actor->world.pos;
            const float dx = pos.x - ground.x;
            const float dz = pos.z - ground.z;
            const float clearance = category == ACTORCAT_DOOR || category == ACTORCAT_PLAYER ? 150.0f : 90.0f;
            if (std::abs(pos.y - ground.y) < 90.0f && dx * dx + dz * dz < clearance * clearance) {
                return false;
            }
        }
    }
    return true;
}

static uint8_t sSpawnCooldown = 20;

static void UpdatePopulation() {
    if (!GameInteractor::IsSaveLoaded(false)) {
        return;
    }
    if (sSpawnCooldown != 0) {
        --sSpawnCooldown;
        return;
    }
    sSpawnCooldown = 40;
    const uint8_t desired = DesiredResidents(gPlayState);
    if (desired == 0 || GameInteractor::IsGameplayPaused() || gPlayState->pauseCtx.debugState != 0 ||
        gPlayState->gameOverCtx.state != GAMEOVER_INACTIVE || gPlayState->transitionTrigger != TRANS_TRIGGER_OFF ||
        gPlayState->transitionMode != TRANS_MODE_OFF || gSaveContext.health <= 0) {
        return;
    }

    uint8_t present = 0;
    for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
        if (IsResidentActor(actor) && actor->update != nullptr) {
            const auto role = static_cast<unsigned int>(GetResidentRole(actor));
            if (role < static_cast<unsigned int>(ResidentRole::Count)) {
                present |= static_cast<uint8_t>(1u << role);
            }
        }
    }
    for (const ResidentPlacement& placement : kKakarikoResidents) {
        const uint8_t bit = static_cast<uint8_t>(1u << static_cast<unsigned int>(placement.role));
        if ((desired & bit) == 0 || (present & bit) != 0) {
            continue;
        }
        Vec3f ground{};
        if (FindClearGround(gPlayState, placement.position, ground) &&
            SpawnResident(gPlayState, placement.role, ground, placement.yaw) != nullptr) {
            present |= bit;
        }
    }
}

static void RegisterPopulation() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    RegisterResidentActor();
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { sSpawnCooldown = 20; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdatePopulation);
}

static RegisterShipInitFunc initFunc(RegisterPopulation);

} // namespace LivingHyrule
