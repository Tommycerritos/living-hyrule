#ifndef LIVING_HYRULE_STEWARDSHIP_H
#define LIVING_HYRULE_STEWARDSHIP_H

#include <stdint.h>

// Inline save data only: the engine copies its save context before writing it.
typedef struct LivingHyruleStewardshipData {
    uint8_t charterMask;
    uint64_t treasury[8];
} LivingHyruleStewardshipData;

#endif
