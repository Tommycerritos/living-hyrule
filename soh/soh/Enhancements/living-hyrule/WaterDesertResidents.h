#pragma once

#include "WaterDesertPolicy.h"

struct Actor;

namespace LivingHyrule {
bool IsWaterDesertResidentActor(const Actor* actor);
WaterDesertResidentId GetWaterDesertResidentId(const Actor* actor);
int GetWaterDesertResidentPropertyId(WaterDesertResidentId id);
const char* GetWaterDesertResidentName(WaterDesertResidentId id);
} // namespace LivingHyrule
