#ifndef LIVING_HYRULE_SAVE_H
#define LIVING_HYRULE_SAVE_H

#include <stdint.h>

/* Fixed-size data travels with SaveManager's copy of the game save. */
typedef struct {
    uint8_t enabled;
    uint64_t bankRupees;
    uint8_t ownsKakarikoCottage;
    uint32_t rentalFrames;
    uint64_t totalRentEarned;
    uint32_t ownedProperties;
    uint32_t repairedProperties;
    uint32_t businessFrames[16];
    uint64_t totalBusinessEarned;
} LivingHyruleSaveData;

#endif
