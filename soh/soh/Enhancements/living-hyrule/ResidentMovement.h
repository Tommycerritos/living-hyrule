#pragma once

#include "MovementPolicy.h"

extern "C" {
#include "z64.h"
}

namespace LivingHyrule {

// Embedded, zero-initialized actor state. No actor/resource pointers survive a
// frame and nothing here is persisted in a save file.
struct ResidentMovementState {
    MovementProgress progress;
    uint32_t lastFrame;
    int16_t scene;
    int8_t room;
    int8_t file;
    ResidentMovementRoutine routine;
    uint8_t initialized;
    uint8_t frameSeen;
    uint8_t advancing;
    float gaitWeight;
};

void ResetResidentMovement(ResidentMovementState& state);

// Replace the actor's SkelAnime_Update with this call, before ordinary gravity,
// collider updates and talking. It advances the skeleton exactly once, including
// routine None; callers must not advance it a second time. Selected routines only
// modify an already initialized 16-joint CNE pose, and preserve its idle clip,
// speed and phase. Returns true while walking or turning toward the next leg:
// suppress body tracking in that case so it cannot override the route heading.
// Pass the current schedule/Additional residents result as schedulePresent, even
// when the caller retains an actor to finish an active or pending conversation.
// Call ResetResidentMovement before skeleton cleanup in the actor destroy hook.
bool UpdateResidentMovement(PlayState* play, Actor* actor, ResidentMovementState& state,
                            ResidentMovementRoutine routine, SkelAnime* skeleton, bool schedulePresent);

} // namespace LivingHyrule
