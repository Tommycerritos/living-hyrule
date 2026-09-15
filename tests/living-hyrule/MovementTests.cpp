#include "MovementPolicy.h"
#include <iostream>
#include <limits>
#include <type_traits>

namespace {
using namespace LivingHyrule;
int checks = 0;
int failures = 0;

void Check(bool condition, const char *message) {
  ++checks;
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

float Distance(const MovementPoint &a, const MovementPoint &b) {
  const float dx = b.x - a.x, dz = b.z - a.z;
  return std::sqrt(dx * dx + dz * dz);
}

void GatesAndTimers() {
  for (unsigned bits = 0; bits < 64; ++bits) {
    const MovementConditions gates{(bits & 1) != 0,  (bits & 2) != 0,
                                   (bits & 4) != 0,  (bits & 8) != 0,
                                   (bits & 16) != 0, (bits & 32) != 0};
    Check(MovementAllowed(gates) == (bits == 15),
          "all four positive gates and both stop gates are required");
    MovementProgress progress{73, 1, 5};
    Check(!TickMovementWait(progress, MovementAllowed(gates)),
          "waiting never authorizes a movement step");
    Check(progress.waitFrames == (bits == 15 ? 72 : 73),
          "pauses and conversations freeze the wait timer");
    Check(progress.outward == 1 && progress.strideDistance == 5,
          "waiting cannot turn or animate a stride");
  }
  MovementProgress progress{0, 1, 0};
  Check(!TickMovementWait(progress, false),
        "a paused expired timer cannot restart movement");
  Check(TickMovementWait(progress, true),
        "an eligible expired timer allows a checked step");
}

void RoutesAndDisplacement() {
  Check(GetMovementRoute(ResidentMovementRoutine::None) == nullptr,
        "ordinary residents retain stationary behavior");
  Check(GetMovementRoute(static_cast<ResidentMovementRoutine>(255)) == nullptr,
        "unknown routine fails closed");
  for (const auto routine :
       {ResidentMovementRoutine::Pella, ResidentMovementRoutine::Edda}) {
    const auto &route = *GetMovementRoute(routine);
    Check(ValidMovementRoute(route),
          "each authored route satisfies the short, slow route bounds");
    Check(InsideMovementCorridor(route, route.home) &&
              InsideMovementCorridor(route, route.destination),
          "both stops lie on the route");
    MovementProgress progress{0, 1, 0};
    auto point = route.home;
    point.x += 7;
    Check(PlanMovementStep(route, progress, point).valid,
          "small collision displacement can be recovered on foot");
    point = route.home;
    point.x += 200;
    Check(!PlanMovementStep(route, progress, point).valid,
          "large displacement never teleports back to the route");
    point = route.home;
    point.y += 13;
    Check(!PlanMovementStep(route, progress, point).valid,
          "airborne or replaced-height routes stay idle");
    point = route.home;
    point.z = std::numeric_limits<float>::quiet_NaN();
    Check(!PlanMovementStep(route, progress, point).valid,
          "nonfinite coordinates fail closed");
    progress.outward = 2;
    Check(!PlanMovementStep(route, progress, route.home).valid,
          "invalid direction fails closed");
    progress = {0, 1, std::numeric_limits<float>::infinity()};
    Check(!PlanMovementStep(route, progress, route.home).valid,
          "nonfinite gait progress fails closed");
    auto invalid = route;
    invalid.speed = 0;
    Check(!ValidMovementRoute(invalid), "zero speed is rejected");
    invalid.speed = 0.61f;
    Check(!ValidMovementRoute(invalid),
          "a fast step cannot bypass collision sampling");
    invalid = route;
    invalid.destination.x += 1000;
    Check(!ValidMovementRoute(invalid), "unbounded navigation is rejected");
  }
}

void RoundTrips() {
  for (const auto routine :
       {ResidentMovementRoutine::Pella, ResidentMovementRoutine::Edda}) {
    const auto &route = *GetMovementRoute(routine);
    MovementProgress progress{route.stopFrames, 1, 0};
    auto current = route.home;
    unsigned homeVisits = 0, awayVisits = 0;
    for (unsigned frame = 0; frame < 15000; ++frame) {
      // Simulate sustained pauses, player approach and conversation time.
      const bool active = frame % 1000 >= 100;
      const auto old = progress;
      if (!TickMovementWait(progress, active)) {
        Check(!active ? progress.waitFrames == old.waitFrames
                      : progress.waitFrames + 1 == old.waitFrames,
              "wait progress advances only during eligible play");
        continue;
      }
      const auto step = PlanMovementStep(route, progress, current);
      Check(step.valid, "every round-trip step stays plannable");
      Check(Distance(current, step.position) <= route.speed + 0.001f,
            "no frame overshoots the speed bound");
      Check(InsideMovementCorridor(route, step.position),
            "long-running routines cannot wander off-route");
      Check(step.position.y == current.y,
            "planner leaves actual floor resolution to the engine");
      // A rejected geometry/actor check does not consume the step, turn
      // around early, or advance the gait while standing still.
      if (frame % 31 == 0) {
        Check(progress.outward == old.outward &&
                  progress.strideDistance == old.strideDistance,
              "unaccepted steps preserve direction and gait");
        continue;
      }
      current = step.position;
      AcceptMovementStep(progress, route, step);
      Check(progress.strideDistance >= 0 && progress.strideDistance < 24,
            "gait progress stays bounded forever");
      if (step.arrives) {
        const auto &stop = old.outward ? route.destination : route.home;
        Check(Distance(current, stop) < 0.001f,
              "turnaround happens exactly at a safely reached stop");
        Check(progress.outward != old.outward &&
                  progress.waitFrames == route.stopFrames,
              "each arrival starts the opposite leg after a proper rest");
        old.outward ? ++awayVisits : ++homeVisits;
      } else {
        Check(progress.outward == old.outward,
              "walking cannot turn around before arrival");
      }
    }
    Check(homeVisits > 10 && awayVisits > 10,
          "each routine completes many interrupted round trips");
  }
}

void GaitAndInvalidAcceptance() {
  static_assert(std::is_trivial_v<MovementProgress>);
  for (int sample = 0; sample <= 240; ++sample) {
    const float distance = sample / 10.0f;
    const auto gait = MovementGaitFor(distance, 1);
    const auto opposite = MovementGaitFor(distance + 12, 1);
    const auto repeat = MovementGaitFor(distance + 24, 1);
    Check(std::abs(gait.leftHip) <= 1700.01f &&
              std::abs(gait.rightHip) <= 1700.01f,
          "hip swing remains small enough for this civilian body");
    Check(gait.leftKnee >= 0 && gait.leftKnee <= 1800.01f &&
              gait.rightKnee >= 0 && gait.rightKnee <= 1800.01f,
          "knees bend forward within the intended small range");
    Check(std::abs(gait.leftHip + gait.leftKnee + gait.leftAnkle) < 0.001f &&
              std::abs(gait.rightHip + gait.rightKnee + gait.rightAnkle) <
                  0.001f,
          "ankles cancel hip and knee rotation to keep soles level");
    Check(std::abs(gait.leftHip - opposite.rightHip) < 0.02f &&
              std::abs(gait.leftKnee - opposite.rightKnee) < 0.02f,
          "left and right feet alternate half a stride apart");
    Check(std::abs(gait.leftHip - repeat.leftHip) < 0.02f,
          "the gait wraps continuously between strides");
    const auto idle = MovementGaitFor(distance, 0);
    Check(idle.leftHip == 0 && idle.leftKnee == 0 && idle.rightHip == 0 &&
              idle.rightKnee == 0 && idle.rootDrop == 0,
          "stopping removes all procedural pose offsets");
  }
  const auto &route = *GetMovementRoute(ResidentMovementRoutine::Pella);
  MovementProgress progress{0, 1, 0};
  auto bad = PlanMovementStep(route, progress, route.home);
  bad.distance = 100;
  bad.arrives = true;
  AcceptMovementStep(progress, route, bad);
  Check(progress.outward == 1 && progress.waitFrames == 0 &&
            progress.strideDistance == 0,
        "an invalid committed step cannot change phase or direction");
  bad = PlanMovementStep(route, progress, route.home);
  bad.position.x = 10000;
  AcceptMovementStep(progress, route, bad);
  Check(progress.strideDistance == 0, "out-of-route acceptance is rejected");
  const auto invalidGait =
      MovementGaitFor(std::numeric_limits<float>::quiet_NaN(), 1);
  Check(invalidGait.leftHip == 0 && invalidGait.rootDrop == 0,
        "nonfinite gait input becomes an idle pose");
}

} // namespace

int main() {
  GatesAndTimers();
  RoutesAndDisplacement();
  RoundTrips();
  GaitAndInvalidAcceptance();
  std::cout << checks << " movement checks, " << failures << " failures\n";
  return failures == 0 ? 0 : 1;
}
