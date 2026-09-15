#include "MarketRestoration.h"
#include "MarketRestorationPolicy.h"
#include "SocialPolicy.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"
#include "soh/resource/type/CollisionHeader.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetEntranceList.h"
#include "soh/resource/type/scenecommand/SetExitList.h"
#include "soh/resource/type/scenecommand/SetLightingSettings.h"
#include "soh/resource/type/scenecommand/SetMesh.h"
#include "soh/resource/type/scenecommand/SetStartPositionList.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/gameplay_field_keep/gameplay_field_keep.h"
#include "objects/gameplay_dangeon_keep/gameplay_dangeon_keep.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

constexpr const char* kCollisionPaths[] = {
    "scenes/shared/market_day_scene/market_day_sceneCollisionHeader_002640",
    "scenes/shared/market_night_scene/market_night_sceneCollisionHeader_0025F8",
    "scenes/shared/market_ruins_scene/market_ruins_sceneCollisionHeader_0015F8",
};
constexpr uint64_t kCollisionHashes[] = {
    UINT64_C(0xd5db47d7be704d74),
    UINT64_C(0xe6c2154e570e39a5),
    UINT64_C(0xa742d89f91aecedd),
};
constexpr const char* kVariants[] = { "day", "night" };

struct Geometry {
    CollisionHeader header{};
    std::vector<Vec3s> vertices;
    std::vector<CollisionPoly> polygons;
    std::vector<SurfaceType> surfaces;
    std::vector<CamData> cameras;
    std::vector<Vec3s> cameraPositions;
    std::shared_ptr<SOH::SetMesh> mesh;
    std::shared_ptr<SOH::SetLightingSettings> lighting;
};

// Immutable once published, retained for process lifetime. OnPlayDestroy runs
// before actor teardown, so clearing these buffers there would be unsafe.
struct PreparedAssets {
    std::array<Geometry, 2> variants;
    std::shared_ptr<SOH::CollisionHeader> original;
    std::vector<std::shared_ptr<Ship::IResource>> retainedArt;
};
std::unique_ptr<PreparedAssets> prepared;
bool preparationAttempted = false;
PlayState* activePlay = nullptr;
Geometry* activeGeometry = nullptr;
bool activeNight = false;

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

std::shared_ptr<Ship::IResource> Load(const std::string& path) {
    auto resource = ResourceMgr_GetResourceByNameHandlingMQ(path.c_str());
    // SOH::Scene deliberately has no raw pointer; it is a command container.
    if (!resource)
        throw std::runtime_error("Missing restoration resource");
    return resource;
}

std::shared_ptr<Ship::IResource> LoadArt(const std::string& path) {
    auto resource = Load(path);
    if (!resource->GetRawPointer())
        throw std::runtime_error("Missing restoration art data");
    return resource;
}

void RetainNativeGroup(PreparedAssets& assets, const char* prefix, size_t count, uint64_t expected) {
    const auto files = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->ListFiles(
        std::string(prefix) + "*");
    if (!files)
        throw std::runtime_error("Missing native resource group");
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
        hash *= UINT64_C(1099511628211); // NUL separator.
    }
    if (names.size() != count || hash != expected)
        throw std::runtime_error("Incomplete native resource group");
    for (const auto& name : names) {
        // The native exporter emits these four empty address labels. Their
        // data lives in the parent scene/skeleton resource, not a loadable file.
        if (name == "scenes/shared/market_day_scene/market_day_room_0ActorEntry_000060" ||
            name == "scenes/shared/market_night_scene/market_night_room_0ActorEntry_000050" ||
            name == "objects/gameplay_field_keep/gButterflySkelLimbs" ||
            name == "objects/gameplay_field_keep/gFieldUnusedFishSkelLimbs")
            continue;
        assets.retainedArt.push_back(Load(name));
    }
}

template <typename T> std::shared_ptr<T> LoadAs(const std::string& path) {
    auto resource = std::dynamic_pointer_cast<T>(Load(path));
    if (!resource)
        throw std::runtime_error("Unexpected restoration resource type");
    return resource;
}

template <typename T> std::shared_ptr<T> FindCommand(const SOH::Scene& scene, SOH::SceneCommandID id) {
    std::shared_ptr<T> result;
    for (const auto& command : scene.commands) {
        if (command && command->cmdId == id) {
            if (result)
                throw std::runtime_error("Duplicate restoration scene command");
            result = std::dynamic_pointer_cast<T>(command);
        }
    }
    if (!result)
        throw std::runtime_error("Missing restoration scene command");
    return result;
}

void BuildGeometry(Geometry& output, const SOH::CollisionHeader& source, const SOH::CollisionHeader& ruins) {
    output.vertices = source.vertices;
    for (const auto& value : source.surfaceTypes) {
        SurfaceType surface{};
        surface.data[0] = RestorationSurface(value.data[0]);
        surface.data[1] = value.data[1];
        output.surfaces.push_back(surface);
    }
    const auto append = [&](const SOH::CollisionPoly& value, uint16_t type, uint16_t a, uint16_t b, uint16_t c) {
        CollisionPoly polygon{};
        polygon.type = type;
        polygon.flags_vIA = a;
        polygon.flags_vIB = b;
        polygon.vIC = c;
        polygon.normal = value.normal;
        polygon.dist = value.dist;
        output.polygons.push_back(polygon);
    };
    for (const auto& value : source.polygons)
        append(value, value.type, value.flags_vIA, value.flags_vIB, value.vIC);

    SurfaceType closureSurface{};
    closureSurface.data[0] = 1; // Adult no-camera-change, no exit, ordinary wall.
    const auto closureType = static_cast<uint16_t>(output.surfaces.size());
    output.surfaces.push_back(closureSurface);
    // The complete native fingerprints above validate these exact sixteen
    // triangles: six closed storefronts and both originally blocked alleys.
    for (size_t index = 197; index <= 212; ++index) {
        const auto& value = ruins.polygons.at(index);
        const auto first = static_cast<uint16_t>(output.vertices.size());
        output.vertices.push_back(ruins.vertices.at(value.flags_vIA & 0x1fff));
        output.vertices.push_back(ruins.vertices.at(value.flags_vIB & 0x1fff));
        output.vertices.push_back(ruins.vertices.at(value.vIC & 0x1fff));
        append(value, closureType, first, first + 1, first + 2);
    }
    output.cameraPositions = ruins.camPosData;
    for (size_t index = 0; index < ruins.camData.size(); ++index) {
        const auto& value = ruins.camData[index];
        output.cameras.push_back(
            { value.cameraSType, value.numCameras, &output.cameraPositions.at(ruins.camPosDataIndices.at(index)) });
    }
    output.header.minBounds = source.collisionHeaderData.minBounds;
    output.header.maxBounds = source.collisionHeaderData.maxBounds;
    output.header.numVertices = static_cast<uint16_t>(output.vertices.size());
    output.header.vtxList = output.vertices.data();
    output.header.numPolygons = static_cast<uint16_t>(output.polygons.size());
    output.header.polyList = output.polygons.data();
    output.header.surfaceTypeList = output.surfaces.data();
    output.header.cameraDataList = output.cameras.data();
    output.header.cameraDataListLen = output.cameras.size();
}

bool PrepareAssets() {
    if (preparationAttempted)
        return prepared != nullptr;
    preparationAttempted = true;
    try {
        auto candidate = std::make_unique<PreparedAssets>();
        // Retain the native sublists, vertices and textures too: display-list
        // resource parsing alone defers hashed dependencies until drawing.
        RetainNativeGroup(*candidate, "scenes/shared/market_day_scene/", 44, UINT64_C(0xff17c2a15e8c71a1));
        RetainNativeGroup(*candidate, "scenes/shared/market_night_scene/", 47, UINT64_C(0x220df8e517158ffe));
        RetainNativeGroup(*candidate, "objects/gameplay_field_keep/", 99, UINT64_C(0x3f7d3018d3a7d7b6));
        RetainNativeGroup(*candidate, "objects/gameplay_dangeon_keep/", 97, UINT64_C(0xc403cb7c046edaa9));
        std::array<std::shared_ptr<SOH::CollisionHeader>, 3> collision;
        for (size_t index = 0; index < collision.size(); ++index) {
            collision[index] = LoadAs<SOH::CollisionHeader>(kCollisionPaths[index]);
            if (CollisionFingerprint(*collision[index]) != kCollisionHashes[index])
                throw std::runtime_error("Unrecognized Market collision layout");
        }
        candidate->original = collision[2];
        auto originalScene = LoadAs<SOH::Scene>("scenes/shared/market_ruins_scene/market_ruins_scene");
        auto exits = FindCommand<SOH::SetExitList>(*originalScene, SOH::SceneCommandID::SetExitList);
        if (exits->exits != std::vector<uint16_t>{ 0x33, 0x138, 0x171, 0xad, 0x29a, 0 })
            throw std::runtime_error("Unrecognized adult exits");
        auto starts = FindCommand<SOH::SetStartPositionList>(*originalScene, SOH::SceneCommandID::SetStartPositionList);
        auto entrances = FindCommand<SOH::SetEntranceList>(*originalScene, SOH::SceneCommandID::SetEntranceList);
        constexpr Vec3s entryPositions[] = { { -4, 0, 768 }, { -2, 0, -762 }, { 477, 0, -367 } };
        if (starts->startPositions.size() != 11 || entrances->entrances.size() != 12)
            throw std::runtime_error("Unrecognized adult entries");
        for (size_t index = 0; index < 3; ++index) {
            const auto& position = starts->startPositions[index].pos;
            const auto& expected = entryPositions[index];
            const auto& entrance = entrances->entrances[index];
            if (position.x != expected.x || position.y != expected.y || position.z != expected.z ||
                entrance.spawn != index || entrance.room != 0)
                throw std::runtime_error("Unrecognized adult spawn");
        }
        for (size_t index = 0; index < candidate->variants.size(); ++index) {
            auto& geometry = candidate->variants[index];
            const std::string prefix = "scenes/shared/market_" + std::string(kVariants[index]) + "_scene/";
            const std::string name = "market_" + std::string(kVariants[index]);
            auto scene = LoadAs<SOH::Scene>(prefix + name + "_scene");
            auto room = LoadAs<SOH::Scene>(prefix + name + "_room_0");
            geometry.mesh = FindCommand<SOH::SetMesh>(*room, SOH::SceneCommandID::SetMesh);
            geometry.lighting = FindCommand<SOH::SetLightingSettings>(*scene, SOH::SceneCommandID::SetLightingSettings);
            const std::string display = prefix + name + (index == 0 ? "_room_0DL_0057D8" : "_room_0DL_005708");
            if (geometry.mesh->meshHeader.base.type != 0 || geometry.mesh->dlists.size() != 1 ||
                geometry.mesh->opaPaths != std::vector<std::string>{ "__OTR__" + display } ||
                !geometry.mesh->xluPaths.empty() || geometry.lighting->settings.size() != 1)
                throw std::runtime_error("Unrecognized Market mesh");
            candidate->retainedArt.push_back(LoadArt(display));
            const std::string texPrefix = index == 0 ? "textures/vr_MDVR" : "textures/vr_MNVR";
            const std::string texName = index == 0 ? "gMarketDay" : "gMarketNight";
            for (int face = 1; face <= 4; ++face) {
                candidate->retainedArt.push_back(
                    LoadArt(texPrefix + "_static/" + texName + (face == 1 ? "" : std::to_string(face)) + "BgTex"));
                candidate->retainedArt.push_back(LoadArt(texPrefix + "_pal_static/" + texName + "Bg" +
                                                         (face == 1 ? "" : std::to_string(face)) + "TLUT"));
            }
            BuildGeometry(geometry, *collision[index], *collision[2]);
        }
        candidate->retainedArt.push_back(LoadArt(gFieldDoorLeftDL));
        candidate->retainedArt.push_back(LoadArt(gFieldDoorRightDL));
        candidate->retainedArt.push_back(LoadArt(gSmallWoodenBoxDL));
        prepared = std::move(candidate);
        return true;
    } catch (const std::exception&) {
        // Payment and the saved investment are untouched, including if a later
        // session lacks the native assets. Restart after correcting resources.
        return false;
    }
}

bool Victory() {
    return gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_GANON] != 0;
}
bool EnabledState() {
    return IsValidState(gSaveContext.ship.livingHyrule) && gSaveContext.ship.livingHyrule.enabled == 1;
}
bool NativeBackgrounds() {
    return CVarGetInteger(CVAR_ENHANCEMENT("3DSceneRender"), 0) == 0;
}

void SelectCollision(PlayState* play, CollisionHeader** header) {
    activePlay = nullptr;
    activeGeometry = nullptr;
    if (!RestorationEntryAllowed(
            IS_VANILLA || IS_MASTER_QUEST,
            gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && gSaveContext.gameMode == GAMEMODE_NORMAL,
            !IS_CUTSCENE_LAYER, LINK_IS_ADULT, Victory(), EnabledState(),
            gSaveContext.ship.livingHyrule.marketRestored == 1, NativeBackgrounds(), play->sceneNum, play->curSpawn) ||
        !PrepareAssets() || header == nullptr ||
        *header != reinterpret_cast<CollisionHeader*>(prepared->original->GetPointer()))
        return;
    activeNight = gSaveContext.nightFlag != 0;
    activeGeometry = &prepared->variants[activeNight ? 1 : 0];
    activePlay = play;
    *header = &activeGeometry->header;
}

void ApplyPresentation(int16_t) {
    if (!IsMarketRestorationActive())
        return;
    auto* play = activePlay;
    // First call is after scene commands and before Skybox_Init. Room calls
    // must not reinitialize skybox storage or execute any child actor commands.
    if (play->roomCtx.curRoom.num < 0) {
        play->skyboxId = activeNight ? SKYBOX_MARKET_CHILD_NIGHT : SKYBOX_MARKET_CHILD_DAY;
        play->envCtx.lightSettingsList = reinterpret_cast<EnvLightSettings*>(activeGeometry->lighting->GetPointer());
    } else if (play->roomCtx.curRoom.num == 0 && play->roomCtx.curRoom.segment != nullptr) {
        play->roomCtx.curRoom.meshHeader = reinterpret_cast<MeshHeader*>(activeGeometry->mesh->GetPointer());
    }
}

struct DoorPlacement {
    Vec3f position;
    s16 yaw;
};
constexpr std::array<DoorPlacement, 6> kDoors = { {
    { { -482, 0, 615 }, -32768 },
    { { 534, 0, -120 }, -16384 },
    { { -240, 0, -616 }, 0 },
    { { 320, 0, -579 }, 0 },
    { { 534, 0, 220 }, -16384 },
    { { -480, 0, 0 }, 16384 },
} };

extern "C" void DrawMarketClosures() {
    if (!IsMarketRestorationActive() || activePlay->roomCtx.curRoom.num != 0)
        return;
    auto* play = activePlay;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    for (const auto& door : kDoors) {
        Matrix_Push();
        Matrix_Translate(door.position.x, door.position.y, door.position.z, MTXMODE_NEW);
        Matrix_RotateY(door.yaw * (static_cast<float>(M_PI) / 32768.0f), MTXMODE_APPLY);
        Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
        // Native gDoorSkel's closed pose: hinge at -2700, then limb X=-0x4000.
        Matrix_Translate(-2700.0f, 0.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateX(-static_cast<float>(M_PI) / 2.0f, MTXMODE_APPLY);
        Vec3f position = door.position;
        const s16 viewAngle = door.yaw - Math_Vec3f_Yaw(&play->view.eye, &position);
        const char* display = std::abs(static_cast<int>(viewAngle)) < 0x4000 ? gFieldDoorLeftDL : gFieldDoorRightDL;
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD);
        gSPDisplayList(POLY_OPA_DISP++, reinterpret_cast<Gfx*>(const_cast<char*>(display)));
        Matrix_Pop();
    }
    // Supplies visibly mark the two retained native alley barriers. Crates keep
    // their stock 0.1 scale; all physical blocking comes from the native caps.
    for (int side : { -1, 1 }) {
        for (int stack = 0; stack < 7; ++stack) {
            const float along = (stack + 0.5f) / 7.0f;
            for (int level = 0; level < 4; ++level) {
                Matrix_Push();
                Matrix_Translate(-460.0f - 100.0f * along + 12.0f, 2.0f + 27.0f * level,
                                 side * (440.0f + 160.0f * along - 8.0f), MTXMODE_NEW);
                Matrix_Scale(0.1f, 0.1f, 0.1f, MTXMODE_APPLY);
                gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                          G_MTX_MODELVIEW | G_MTX_LOAD);
                gSPDisplayList(POLY_OPA_DISP++, reinterpret_cast<Gfx*>(const_cast<char*>(gSmallWoodenBoxDL)));
                Matrix_Pop();
            }
        }
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

void RegisterRestoration() {
    // Settings import invokes ShipInit again. A duplicate collision hook would
    // clear the first hook's presentation latch after it has replaced the header.
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneCollisionLoad>(SelectCollision);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::AfterSceneCommands>(SCENE_MARKET_RUINS,
                                                                                        ApplyPresentation);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>(DrawMarketClosures);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        activePlay = nullptr;
        activeGeometry = nullptr; // Keep prepared buffers alive through actor destruction.
    });
}
RegisterShipInitFunc initRestoration(RegisterRestoration);

} // namespace

bool IsMarketRestorationActive() {
    return activePlay != nullptr && activePlay == gPlayState && activeGeometry != nullptr &&
           activePlay->sceneNum == SCENE_MARKET_RUINS && activePlay->colCtx.colHeader == &activeGeometry->header;
}

MarketRestorationReadiness GetMarketRestorationReadiness() {
    if (!GameInteractor::IsSaveLoaded(false))
        return MarketRestorationReadiness::NoLoadedGame;
    if (!(IS_VANILLA || IS_MASTER_QUEST))
        return MarketRestorationReadiness::UnsupportedAdventure;
    if (gPlayState->sceneNum != SCENE_MARKET_RUINS || !LINK_IS_ADULT || IS_CUTSCENE_LAYER)
        return MarketRestorationReadiness::WrongPlace;
    if (!Victory())
        return MarketRestorationReadiness::AwaitingVictory;
    if (!EnabledState())
        return MarketRestorationReadiness::EconomyUnavailable;
    if (!NativeBackgrounds())
        return MarketRestorationReadiness::ThreeDimensionalBackgrounds;
    if (gPlayState->curSpawn < 0 || gPlayState->curSpawn > 2)
        return MarketRestorationReadiness::UnsafeEntrance;
    if (!PrepareAssets() ||
        (!IsMarketRestorationActive() &&
         gPlayState->colCtx.colHeader != reinterpret_cast<CollisionHeader*>(prepared->original->GetPointer())))
        return MarketRestorationReadiness::ResourcesUnavailable;
    return MarketRestorationReadiness::Ready;
}

const char* MarketRestorationReadinessText(MarketRestorationReadiness readiness) {
    switch (readiness) {
        case MarketRestorationReadiness::Ready:
            return "Restores the square on your next visit. Shops and alleys remain closed.";
        case MarketRestorationReadiness::NoLoadedGame:
            return "Load a regular adventure save first.";
        case MarketRestorationReadiness::UnsupportedAdventure:
            return "Restoration supports normal and Master Quest adventures.";
        case MarketRestorationReadiness::WrongPlace:
            return "Visit the adult Market square to fund its restoration.";
        case MarketRestorationReadiness::AwaitingVictory:
            return "The square can be restored after Ganon's defeat is recorded.";
        case MarketRestorationReadiness::EconomyUnavailable:
            return "Enable a readable Living Hyrule account first.";
        case MarketRestorationReadiness::ThreeDimensionalBackgrounds:
            return "Restoration uses the original town panorama. Turn off 3D pre-rendered scenes before funding it.";
        case MarketRestorationReadiness::ResourcesUnavailable:
            return "Compatible native Market assets are unavailable. Your investment is preserved; restart after "
                   "correcting the assets.";
        case MarketRestorationReadiness::UnsafeEntrance:
            return "Return through the town gate, castle road, or Temple of Time approach first.";
    }
    return "Restoration is unavailable.";
}

} // namespace LivingHyrule
