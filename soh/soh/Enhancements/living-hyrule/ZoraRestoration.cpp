#include "ZoraRestoration.h"
#include "ZoraRestorationPolicy.h"
#include "Economy.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"
#include "soh/resource/type/CollisionHeader.h"
#include "soh/resource/type/Array.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetActorList.h"
#include "soh/resource/type/scenecommand/SetCollisionHeader.h"
#include "soh/resource/type/scenecommand/SetEntranceList.h"
#include "soh/resource/type/scenecommand/SetExitList.h"
#include "soh/resource/type/scenecommand/SetMesh.h"
#include "soh/resource/type/scenecommand/SetStartPositionList.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <fast/lus_gbi.h>
#include <fast/resource/type/DisplayList.h>
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
#include "objects/object_spot06_objects/object_spot06_objects.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

constexpr const char* kScenePrefix = "scenes/shared/spot07_scene/";
constexpr const char* kCollisionPaths[] = {
    "scenes/shared/spot07_scene/spot07_sceneCollisionHeader_003824",
    "objects/object_spot07_object/object_spot07_object_Col_002590",
    "objects/object_spot07_object/object_spot07_object_Col_0038FC",
    "objects/object_spot06_objects/gLakeHyliaZoraShortcutIceblockCol",
};
constexpr uint64_t kCollisionHashes[] = { UINT64_C(0xfb86681709703c4d), UINT64_C(0x3a9c4a753979d621),
                                          UINT64_C(0x6bddd95748b89ebf), UINT64_C(0xf7850481f98ba469) };

struct PreparedAssets {
    CollisionHeader header{};
    std::vector<Vec3s> vertices;
    std::vector<CollisionPoly> polygons;
    std::vector<SurfaceType> surfaces;
    std::vector<CamData> cameras;
    std::vector<Vec3s> cameraPositions;
    std::vector<WaterBox> water;
    std::shared_ptr<SOH::CollisionHeader> original;
    std::vector<std::shared_ptr<Ship::IResource>> retained;
};

// Published only after all validation/allocation succeeds. These buffers outlive
// every scene and actor: OnPlayDestroy precedes native actor destruction.
std::unique_ptr<PreparedAssets> prepared;
bool preparationAttempted = false;
PlayState* activePlay = nullptr;

uint64_t CollisionFingerprint(const SOH::CollisionHeader& source) {
    uint64_t hash = kZoraRestorationHashStart;
    const auto word = [&](int64_t value) { hash = ZoraRestorationHashWord(hash, static_cast<uint32_t>(value)); };
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
    if (!resource)
        throw std::runtime_error("Missing Domain restoration resource");
    return resource;
}

template <typename T> std::shared_ptr<T> LoadAs(const std::string& path) {
    auto resource = std::dynamic_pointer_cast<T>(Load(path));
    if (!resource)
        throw std::runtime_error("Unexpected Domain resource type");
    return resource;
}

template <typename T> std::shared_ptr<T> FindCommand(const SOH::Scene& scene, SOH::SceneCommandID id) {
    std::shared_ptr<T> result;
    for (const auto& command : scene.commands) {
        if (command && command->cmdId == id) {
            if (result)
                throw std::runtime_error("Duplicate Domain command");
            result = std::dynamic_pointer_cast<T>(command);
        }
    }
    if (!result)
        throw std::runtime_error("Missing Domain command");
    return result;
}

void RetainGroup(PreparedAssets& output, const char* prefix, size_t count, uint64_t expected) {
    const auto files = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->ListFiles(
        std::string(prefix) + "*");
    if (!files)
        throw std::runtime_error("Missing Domain resource group");
    std::vector<std::string> names;
    for (const auto& name : *files) {
        if (name.compare(0, std::char_traits<char>::length(prefix), prefix) == 0 &&
            (name.size() < 5 || name.substr(name.size() - 5) != ".meta"))
            names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    uint64_t hash = kZoraRestorationHashStart;
    for (const auto& name : names) {
        for (unsigned char byte : name)
            hash = (hash ^ byte) * UINT64_C(1099511628211);
        hash *= UINT64_C(1099511628211);
    }
    if (names.size() != count || hash != expected)
        throw std::runtime_error("Incomplete Domain native assets");
    for (const auto& name : names) {
        // Native exporter address labels have no separate data. Their parent
        // room resource owns the actual actor list, validated below.
        if (name == std::string(kScenePrefix) + "spot07_room_0ActorEntry_000068" ||
            name == std::string(kScenePrefix) + "spot07_room_0ActorEntry_000270" ||
            name == std::string(kScenePrefix) + "spot07_room_0ActorEntry_000348" ||
            name == std::string(kScenePrefix) + "spot07_room_1ActorEntry_000068" ||
            name == std::string(kScenePrefix) + "spot07_room_1ActorEntry_0003B0" ||
            name == std::string(kScenePrefix) + "spot07_room_1ActorEntry_000508")
            continue;
        auto resource = Load(name);
        if (!std::dynamic_pointer_cast<SOH::Scene>(resource) && !resource->GetRawPointer())
            throw std::runtime_error("Missing Domain art data");
        output.retained.push_back(std::move(resource));
    }
}

uint64_t ActorFingerprint(const SOH::SetActorList& actors) {
    uint64_t hash = ZoraRestorationHashWord(kZoraRestorationHashStart, static_cast<uint32_t>(actors.actorList.size()));
    const auto word = [&](int32_t value) { hash = ZoraRestorationHashWord(hash, static_cast<uint32_t>(value)); };
    for (const auto& actor : actors.actorList) {
        word(static_cast<uint16_t>(actor.id));
        word(actor.pos.x);
        word(actor.pos.y);
        word(actor.pos.z);
        word(actor.rot.x);
        word(actor.rot.y);
        word(actor.rot.z);
        word(static_cast<uint16_t>(actor.params));
    }
    return hash;
}

void ValidateCommands() {
    auto scene = LoadAs<SOH::Scene>(std::string(kScenePrefix) + "spot07_sceneSet_003A40");
    auto exits = FindCommand<SOH::SetExitList>(*scene, SOH::SceneCommandID::SetExitList);
    auto collision = FindCommand<SOH::SetCollisionHeader>(*scene, SOH::SceneCommandID::SetCollisionHeader);
    if (exits->exits != std::vector<uint16_t>{ 0x19d, 0x225, 0x380, 0x560 } ||
        collision->fileName != kCollisionPaths[0])
        throw std::runtime_error("Unrecognized Domain routes");
    auto starts = FindCommand<SOH::SetStartPositionList>(*scene, SOH::SceneCommandID::SetStartPositionList);
    auto entries = FindCommand<SOH::SetEntranceList>(*scene, SOH::SceneCommandID::SetEntranceList);
    constexpr Vec3s positions[] = { { -1150, 210, -150 }, { 537, 996, -2501 }, { 520, 52, 248 }, { 617, 947, -1507 } };
    constexpr uint8_t rooms[] = { 1, 0, 1, 0 };
    if (starts->startPositions.size() != 5 || entries->entrances.size() != 6)
        throw std::runtime_error("Unrecognized Domain entries");
    for (size_t index = 0; index < 4; ++index) {
        const auto& position = starts->startPositions[index].pos;
        const auto& expected = positions[index];
        if (position.x != expected.x || position.y != expected.y || position.z != expected.z ||
            entries->entrances[index].spawn != index || entries->entrances[index].room != rooms[index])
            throw std::runtime_error("Unrecognized Domain spawn");
    }
    constexpr const char* adultRooms[] = { "spot07_room_0Set_000220", "spot07_room_1Set_000360" };
    constexpr uint64_t actors[] = { UINT64_C(0xa7551307b3117fc5), UINT64_C(0x8959061eac5892d9) };
    for (size_t index = 0; index < 2; ++index) {
        auto adult = LoadAs<SOH::Scene>(std::string(kScenePrefix) + adultRooms[index]);
        auto child = LoadAs<SOH::Scene>(std::string(kScenePrefix) + "spot07_room_" + std::to_string(index));
        if (ActorFingerprint(*FindCommand<SOH::SetActorList>(*adult, SOH::SceneCommandID::SetActorList)) !=
            actors[index])
            throw std::runtime_error("Unrecognized Domain adult actors");
        auto adultMesh = FindCommand<SOH::SetMesh>(*adult, SOH::SceneCommandID::SetMesh);
        auto childMesh = FindCommand<SOH::SetMesh>(*child, SOH::SceneCommandID::SetMesh);
        if (adultMesh->meshHeader.base.type != 2 || childMesh->meshHeader.base.type != 2 ||
            adultMesh->dlists2.size() != (index == 0 ? 12 : 20) || adultMesh->opaPaths != childMesh->opaPaths ||
            adultMesh->xluPaths != childMesh->xluPaths)
            throw std::runtime_error("Unrecognized Domain permanent mesh");
    }
}

void ValidateArt() {
    constexpr const char* paths[] = {
        "objects/object_spot06_objects/gLakeHyliaZoraShortcutIceblockDL",
        "objects/object_spot07_object/object_spot07_object_DL_000460",
        "objects/object_spot07_object/object_spot07_object_DL_000BE0",
    };
    constexpr uint64_t hashes[] = { UINT64_C(0xdfc1178a25d8eee0), UINT64_C(0x0b1fac08e451c764),
                                    UINT64_C(0x1f039ecff6cb927f) };
    for (size_t index = 0; index < 3; ++index) {
        auto display = LoadAs<Fast::DisplayList>(paths[index]);
        uint64_t hash = kZoraRestorationHashStart;
        for (const auto& instruction : display->Instructions) {
            hash = ZoraRestorationHashWord(hash, static_cast<uint32_t>(instruction.words.w0));
            hash = ZoraRestorationHashWord(hash, static_cast<uint32_t>(instruction.words.w1));
        }
        if (hash != hashes[index])
            throw std::runtime_error("Unrecognized Domain water display list");
    }
    auto vertices = LoadAs<SOH::Array>("objects/object_spot06_objects/object_spot06_objectsVtx_001120");
    constexpr Vec3s expected[] = { { -300, 0, 0 }, { 300, 723, 0 }, { -300, 723, 0 }, { 300, 0, 0 } };
    if (vertices->ArrayType != SOH::ArrayResourceType::Vertex || vertices->Vertices.size() != 4)
        throw std::runtime_error("Unrecognized Domain closure model");
    for (size_t index = 0; index < 4; ++index) {
        const auto& vertex = vertices->Vertices[index].v;
        if (vertex.ob[0] != expected[index].x || vertex.ob[1] != expected[index].y || vertex.ob[2] != expected[index].z)
            throw std::runtime_error("Domain closure model differs from its collision");
    }
}

void BuildGeometry(PreparedAssets& output) {
    const auto& source = *output.original;
    output.vertices = source.vertices;
    for (const auto& surface : source.surfaceTypes)
        output.surfaces.push_back({ { ZoraRestorationSurface(surface.data[0]), surface.data[1] } });
    for (const auto& polygon : source.polygons) {
        CollisionPoly copy{};
        copy.type = polygon.type;
        copy.flags_vIA = polygon.flags_vIA;
        copy.flags_vIB = polygon.flags_vIB;
        copy.vIC = polygon.vIC;
        copy.normal = polygon.normal;
        copy.dist = polygon.dist;
        output.polygons.push_back(copy);
    }
    const auto first = static_cast<uint16_t>(output.vertices.size());
    for (const auto& vertex : kZoraClosureVertices)
        output.vertices.push_back({ vertex[0], vertex[1], vertex[2] });
    const auto type = static_cast<uint16_t>(output.surfaces.size());
    output.surfaces.push_back({ { 1, 0 } }); // Original no-camera-change index, no exit, ordinary wall.
    for (const std::array<uint16_t, 3>& indices : { std::array<uint16_t, 3>{ 0, 1, 2 }, { 0, 3, 1 } }) {
        CollisionPoly polygon{};
        polygon.type = type;
        polygon.flags_vIA = first + indices[0];
        polygon.flags_vIB = first + indices[1];
        polygon.vIC = first + indices[2];
        const auto& a = output.vertices[polygon.flags_vIA];
        const auto& b = output.vertices[polygon.flags_vIB];
        const auto& c = output.vertices[polygon.vIC];
        const double nx = (b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y);
        const double ny = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
        const double nz = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        const double length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (length < 1.0)
            throw std::runtime_error("Invalid Domain closure");
        polygon.normal = { static_cast<int16_t>(std::lround(nx * 32767.0 / length)),
                           static_cast<int16_t>(std::lround(ny * 32767.0 / length)),
                           static_cast<int16_t>(std::lround(nz * 32767.0 / length)) };
        polygon.dist = static_cast<int16_t>(std::lround(-(nx * a.x + ny * a.y + nz * a.z) / length));
        output.polygons.push_back(polygon);
    }
    output.cameraPositions = source.camPosData;
    for (size_t index = 0; index < source.camData.size(); ++index) {
        const auto& camera = source.camData[index];
        // Native zero-count entries still point to pose0. Preserve that pointer
        // contract: some camera queries fetch positions before checking count.
        output.cameras.push_back(
            { camera.cameraSType, camera.numCameras, &output.cameraPositions.at(source.camPosDataIndices.at(index)) });
    }
    for (const auto& water : source.waterBoxes)
        output.water.push_back(
            { water.xMin, water.ySurface, water.zMin, water.xLength, water.zLength, water.properties });
    output.header.minBounds = source.collisionHeaderData.minBounds;
    output.header.maxBounds = source.collisionHeaderData.maxBounds;
    output.header.numVertices = static_cast<uint16_t>(output.vertices.size());
    output.header.vtxList = output.vertices.data();
    output.header.numPolygons = static_cast<uint16_t>(output.polygons.size());
    output.header.polyList = output.polygons.data();
    output.header.surfaceTypeList = output.surfaces.data();
    output.header.cameraDataList = output.cameras.data();
    output.header.cameraDataListLen = output.cameras.size();
    output.header.numWaterBoxes = static_cast<uint16_t>(output.water.size());
    output.header.waterBoxes = output.water.data();
}

bool PrepareAssets() {
    if (preparationAttempted)
        return prepared != nullptr;
    preparationAttempted = true;
    try {
        auto candidate = std::make_unique<PreparedAssets>();
        // Loading a display list alone does not resolve its hashed texture and
        // vertex dependencies. Retain complete small native resource groups.
        RetainGroup(*candidate, kScenePrefix, 196, UINT64_C(0x3644526f41e93edd));
        RetainGroup(*candidate, "objects/object_spot07_object/", 25, UINT64_C(0xa7d640eb3bd059e6));
        RetainGroup(*candidate, "objects/object_spot06_objects/", 16, UINT64_C(0x74486ef104396a16));
        for (size_t index = 0; index < 4; ++index) {
            auto collision = LoadAs<SOH::CollisionHeader>(kCollisionPaths[index]);
            if (CollisionFingerprint(*collision) != kCollisionHashes[index])
                throw std::runtime_error("Unrecognized Domain collision layout");
            if (index == 0)
                candidate->original = collision;
        }
        ValidateCommands();
        ValidateArt();
        BuildGeometry(*candidate);
        prepared = std::move(candidate);
        return true;
    } catch (const std::exception&) {
        // Keep the saved investment. If an archive changes or is unavailable,
        // native frozen geometry remains in use until a compatible restart.
        return false;
    }
}

bool EnabledState() {
    return IsValidState(gSaveContext.ship.livingHyrule) && gSaveContext.ship.livingHyrule.enabled == 1;
}
bool WaterMedallion() {
    return CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER);
}
bool WaterBlueWarp() {
    return Flags_GetEventChkInf(EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP);
}

void SelectCollision(PlayState* play, CollisionHeader** header) {
    activePlay = nullptr;
    const bool eligible = ZoraRestorationEligible(
        IS_VANILLA || IS_MASTER_QUEST,
        gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && gSaveContext.gameMode == GAMEMODE_NORMAL,
        gSaveContext.sceneLayer, LINK_IS_ADULT, WaterMedallion(), WaterBlueWarp(), EnabledState());
    if (!ZoraRestorationEntryAllowed(eligible, HasFundedZoraRestoration(), play->sceneNum, play->curSpawn) ||
        !PrepareAssets() || header == nullptr ||
        *header != reinterpret_cast<CollisionHeader*>(prepared->original->GetPointer()))
        return;
    activePlay = play;
    *header = &prepared->header;
}

void FrozenBehavior(GIVanillaBehavior, bool* should, va_list args) {
    auto* play = va_arg(args, PlayState*);
    auto* actor = va_arg(args, Actor*);
    if (!IsZoraRestorationActive() || play != activePlay)
        return;
    if (actor != nullptr) {
        // Only the two verified decorative water actors. Red ice and the King
        // have their own untouched behavior and still require original Blue Fire.
        if (actor->id != ACTOR_BG_SPOT07_TAKI || actor->world.rot.x != 0 || actor->world.rot.y != 0 ||
            actor->world.rot.z != 0)
            return;
        const Vec3f expected = actor->params == 0 ? Vec3f{ 0, 0, 0 } : Vec3f{ 445, 1008, -1742 };
        if ((actor->params != 0 && actor->params != 1) || actor->world.pos.x != expected.x ||
            actor->world.pos.y != expected.y || actor->world.pos.z != expected.z)
            return;
    }
    *should = false;
}

extern "C" void DrawZoraShortcutClosure() {
    if (!IsZoraRestorationActive() || (activePlay->roomCtx.curRoom.num != 1 && activePlay->roomCtx.prevRoom.num != 1))
        return;
    auto* play = activePlay;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(kZoraClosurePosition[0], kZoraClosurePosition[1], kZoraClosurePosition[2], MTXMODE_NEW);
    Matrix_RotateY(kZoraClosureYaw * (static_cast<float>(M_PI) / 32768.0f), MTXMODE_APPLY);
    Matrix_Scale(kZoraClosureScale[0], kZoraClosureScale[1], kZoraClosureScale[2], MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    gSPDisplayList(POLY_OPA_DISP++, reinterpret_cast<Gfx*>(const_cast<char*>(gLakeHyliaZoraShortcutIceblockDL)));
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}

void RegisterRestoration() {
    static bool registered = false; // ShipInit also runs on settings import.
    if (registered)
        return;
    registered = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneCollisionLoad>(SelectCollision);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnVanillaBehavior>(VB_ZORAS_DOMAIN_FROZEN,
                                                                                       FrozenBehavior);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>(DrawZoraShortcutClosure);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { activePlay = nullptr; });
}
RegisterShipInitFunc initZoraRestoration(RegisterRestoration);

} // namespace

bool IsZoraRestorationActive() {
    return activePlay != nullptr && activePlay == gPlayState && prepared != nullptr &&
           activePlay->sceneNum == SCENE_ZORAS_DOMAIN && activePlay->colCtx.colHeader == &prepared->header;
}

ZoraRestorationReadiness GetZoraRestorationReadiness() {
    if (!GameInteractor::IsSaveLoaded(false))
        return ZoraRestorationReadiness::NoLoadedGame;
    if (!(IS_VANILLA || IS_MASTER_QUEST))
        return ZoraRestorationReadiness::UnsupportedAdventure;
    if (!LINK_IS_ADULT || (gSaveContext.sceneLayer != 2 && gSaveContext.sceneLayer != 3))
        return ZoraRestorationReadiness::WrongAgeOrLayer;
    if (!WaterMedallion() || !WaterBlueWarp())
        return ZoraRestorationReadiness::AwaitingWaterRecovery;
    if (!EnabledState())
        return ZoraRestorationReadiness::EconomyUnavailable;
    if (gPlayState->sceneNum == SCENE_ZORAS_DOMAIN && (gPlayState->curSpawn < 0 || gPlayState->curSpawn > 3))
        return ZoraRestorationReadiness::UnsafeEntrance;
    if (!PrepareAssets() ||
        (gPlayState->sceneNum == SCENE_ZORAS_DOMAIN && !IsZoraRestorationActive() &&
         gPlayState->colCtx.colHeader != reinterpret_cast<CollisionHeader*>(prepared->original->GetPointer())))
        return ZoraRestorationReadiness::ResourcesUnavailable;
    return ZoraRestorationReadiness::Ready;
}

const char* ZoraRestorationReadinessText(ZoraRestorationReadiness readiness) {
    switch (readiness) {
        case ZoraRestorationReadiness::Ready:
            return "Thaws the Domain's ordinary ice on your next visit. Red ice keeps its Blue Fire rules; the Lake "
                   "shortcut stays closed.";
        case ZoraRestorationReadiness::NoLoadedGame:
            return "Load a regular adventure save first.";
        case ZoraRestorationReadiness::UnsupportedAdventure:
            return "Restoration supports normal and Master Quest adventures.";
        case ZoraRestorationReadiness::WrongAgeOrLayer:
            return "Return during a regular adult adventure.";
        case ZoraRestorationReadiness::AwaitingWaterRecovery:
            return "Complete the Water Temple and return through its blue warp first.";
        case ZoraRestorationReadiness::EconomyUnavailable:
            return "Enable a readable Living Hyrule account first.";
        case ZoraRestorationReadiness::ResourcesUnavailable:
            return "Compatible native Domain assets are unavailable. Your investment is preserved; restart after "
                   "correcting the assets.";
        case ZoraRestorationReadiness::UnsafeEntrance:
            return "Return through the River, Fountain, or shop approach first.";
    }
    return "Restoration is unavailable.";
}

} // namespace LivingHyrule
