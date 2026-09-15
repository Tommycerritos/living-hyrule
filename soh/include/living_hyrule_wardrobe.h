#ifndef LIVING_HYRULE_WARDROBE_H
#define LIVING_HYRULE_WARDROBE_H

#include <stdint.h>

/* Standalone POD for the next save schema; no pointers or engine equipment. */
typedef struct {
    uint16_t ownedStyles;  /* Low eight bits, style 1 at bit 0. */
    uint8_t equippedStyle; /* 0: original appearance; 1..8: permanent dye identity. */
} LivingHyruleWardrobeData;

#endif
