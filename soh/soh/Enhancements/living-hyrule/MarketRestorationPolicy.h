#pragma once

#include <cstdint>

namespace LivingHyrule {

// The original adult square has three usable exits and a two-entry camera
// table. Preserve its no-camera-change surface (index 1), not the child balcony.
constexpr uint32_t RestorationSurface(uint32_t original) {
    const uint32_t exit = (original >> 8) & 31u;
    return (original & ~0x1fffu) | (exit <= 3 ? exit << 8 : 0u) | 1u;
}

constexpr bool RestorationEntryAllowed(bool normalAdventure, bool realFile, bool normalLayer, bool adult, bool victory,
                                       bool validEnabledState, bool funded, bool nativeBackgrounds, int scene,
                                       int spawn) {
    return normalAdventure && realFile && normalLayer && adult && victory && validEnabledState && funded &&
           nativeBackgrounds && scene == 0x22 && spawn >= 0 && spawn <= 2;
}

// Deterministic fingerprints use numeric words, never native padding or pointers.
constexpr uint64_t RestorationHashWord(uint64_t hash, uint32_t word) {
    for (unsigned int byte = 0; byte < 4; ++byte) {
        hash = (hash ^ ((word >> (byte * 8)) & 255u)) * UINT64_C(1099511628211);
    }
    return hash;
}
inline constexpr uint64_t kRestorationHashStart = UINT64_C(14695981039346656037);

} // namespace LivingHyrule
