#pragma once

struct Actor;

namespace LivingHyrule {

// Exact live identity owned by the optional encounter controller. Vanilla
// enemies of the same type never acquire this classification.
bool IsRegionalEncounterActor(const Actor* actor);
void RegisterRegionalEncounters();

} // namespace LivingHyrule
