#ifndef LIVING_HYRULE_SAVE_H
#define LIVING_HYRULE_SAVE_H

#include <stdint.h>
#include "living_hyrule_wardrobe.h"
#include "living_hyrule_stewardship.h"

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
    int8_t rapport[23];
    uint32_t metResidents;
    uint16_t completedFavors;
    uint8_t activeFavor;         /* 0: none; 1..10: permanent favor identity. */
    uint8_t cottageRentPolicy;   /* Requested next-period terms: 0 fair, 1 high. */
    uint8_t currentPeriodPolicy; /* Terms locked for the current rent period. */
    uint8_t marketRestored;
    LivingHyruleWardrobeData wardrobe;
    LivingHyruleStewardshipData stewardship;
    uint8_t zoraRestored;
    uint8_t castleEstateOwned;
    uint8_t givenGifts[23];   /* Three once-per-resident gift kinds, low three bits. */
    uint8_t royalRecognition; /* Eight once-per-save acknowledgments of completed work. */
} LivingHyruleSaveData;

#endif
