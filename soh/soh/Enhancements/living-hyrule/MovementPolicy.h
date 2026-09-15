#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace LivingHyrule {

enum class ResidentMovementRoutine : uint8_t { None, Pella, Edda };

struct MovementPoint {
    float x, y, z;
};

struct MovementRoute {
    MovementPoint home;
    MovementPoint destination;
    float speed;
    uint16_t stopFrames;
};

// Deliberately short out-and-back routines. These do not navigate around an
// obstruction, follow original scene paths, or move a property manager's anchor.
// Quarter-unit offline samples checked the Market day/night/ruins collision
// resources and spot06's high bank, including full footprints and body clearance.
// Live collision/actor checks remain mandatory with different resource packs.
inline const MovementRoute* GetMovementRoute(ResidentMovementRoutine routine) {
    static constexpr MovementRoute pella = { { -300, 0, 400 }, { -300, 0, 280 }, 0.55f, 100 };
    static constexpr MovementRoute edda = { { -2900, -1033, 3400 }, { -2980, -1033, 3440 }, 0.45f, 140 };
    switch (routine) {
        case ResidentMovementRoutine::Pella:
            return &pella;
        case ResidentMovementRoutine::Edda:
            return &edda;
        default:
            return nullptr;
    }
}

struct MovementConditions {
    bool present;
    bool activePlay;
    bool correctScene;
    bool grounded;
    bool conversation;
    bool playerNearby;
};

inline bool MovementAllowed(const MovementConditions& conditions) {
    return conditions.present && conditions.activePlay && conditions.correctScene && conditions.grounded &&
           !conditions.conversation && !conditions.playerNearby;
}

struct MovementProgress {
    uint16_t waitFrames;
    uint8_t outward;
    float strideDistance;
};

inline bool FiniteMovementPoint(const MovementPoint& point) {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

inline bool ValidMovementRoute(const MovementRoute& route) {
    const float dx = route.destination.x - route.home.x, dz = route.destination.z - route.home.z;
    const float lengthSquared = dx * dx + dz * dz;
    return FiniteMovementPoint(route.home) && FiniteMovementPoint(route.destination) && std::isfinite(route.speed) &&
           route.speed > 0 && route.speed <= 0.6f && lengthSquared >= 1 && lengthSquared <= 150 * 150 &&
           std::abs(route.destination.y - route.home.y) <= 12;
}

inline bool InsideMovementCorridor(const MovementRoute& route, const MovementPoint& point) {
    if (!ValidMovementRoute(route) || !FiniteMovementPoint(point))
        return false;
    const float dx = route.destination.x - route.home.x, dz = route.destination.z - route.home.z;
    const float t =
        std::clamp(((point.x - route.home.x) * dx + (point.z - route.home.z) * dz) / (dx * dx + dz * dz), 0.0f, 1.0f);
    const float offsetX = point.x - route.home.x - t * dx, offsetZ = point.z - route.home.z - t * dz;
    const float height = route.home.y + t * (route.destination.y - route.home.y);
    // Small collision corrections may be recovered by walking; displaced or
    // differently placed actors stay idle instead of teleporting onto a route.
    return offsetX * offsetX + offsetZ * offsetZ <= 8 * 8 && std::abs(point.y - height) <= 12;
}

struct MovementStep {
    MovementPoint position;
    float distance;
    bool valid;
    bool arrives;
};

inline MovementStep PlanMovementStep(const MovementRoute& route, const MovementProgress& progress,
                                     const MovementPoint& current) {
    MovementStep step{ current, 0, false, false };
    if (!InsideMovementCorridor(route, current) || progress.outward > 1 || !std::isfinite(progress.strideDistance) ||
        progress.strideDistance < 0 || progress.strideDistance >= 24)
        return step;
    const auto& target = progress.outward ? route.destination : route.home;
    const float dx = target.x - current.x, dz = target.z - current.z;
    const float distance = std::sqrt(dx * dx + dz * dz);
    step.distance = std::min(route.speed, distance);
    step.arrives = distance <= route.speed;
    if (distance > 0) {
        step.position.x += dx * (step.distance / distance);
        step.position.z += dz * (step.distance / distance);
    }
    // The engine adapter must resolve and validate the new ground height.
    step.valid = InsideMovementCorridor(route, step.position);
    return step;
}

inline bool TickMovementWait(MovementProgress& progress, bool allowed) {
    if (!allowed)
        return false;
    if (progress.waitFrames != 0) {
        --progress.waitFrames;
        return false;
    }
    return true;
}

inline void AcceptMovementStep(MovementProgress& progress, const MovementRoute& route, const MovementStep& step) {
    if (!step.valid || !InsideMovementCorridor(route, step.position) || !std::isfinite(step.distance) ||
        step.distance < 0 || step.distance > route.speed || !std::isfinite(progress.strideDistance) ||
        progress.strideDistance < 0 || progress.strideDistance >= 24 || progress.outward > 1)
        return;
    progress.strideDistance = std::fmod(progress.strideDistance + step.distance, 24.0f);
    if (step.arrives) {
        progress.outward = progress.outward ? 0 : 1;
        progress.waitFrames = route.stopFrames;
    }
}

struct MovementGait {
    float leftHip, leftKnee, leftAnkle;
    float rightHip, rightKnee, rightAnkle;
    float rootDrop;
};

// An original small-step gait for the compatible CNE leg hierarchy. The source
// idle animation remains intact. Angles use the engine's 32768-units-per-pi
// convention; ankle counter-rotation keeps the soles level. No native animation
// values or shared resource buffers are copied or changed.
inline MovementGait MovementGaitFor(float distance, float weight) {
    if (!std::isfinite(distance) || !std::isfinite(weight))
        return {};
    weight = std::clamp(weight, 0.0f, 1.0f);
    const float phase = distance * (6.283185307179586f / 24.0f);
    const float swing = std::sin(phase), lift = std::cos(phase);
    const float leftHip = swing * 1700.0f * weight;
    const float rightHip = -leftHip;
    const float leftKnee = std::max(0.0f, -lift) * 1800.0f * weight;
    const float rightKnee = std::max(0.0f, lift) * 1800.0f * weight;
    return { leftHip,
             leftKnee,
             -leftHip - leftKnee,
             rightHip,
             rightKnee,
             -rightHip - rightKnee,
             (1.0f - std::cos(phase * 2.0f)) * 35.0f * weight };
}

} // namespace LivingHyrule
