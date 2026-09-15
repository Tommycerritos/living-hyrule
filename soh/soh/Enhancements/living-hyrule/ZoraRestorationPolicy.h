#pragma once

#include <array>
#include <cstdint>

namespace LivingHyrule {

// Retain every camera/material bit and every other exit. The fourth native
// surface exit is the underwater shortcut, which remains closed for adults.
constexpr uint32_t ZoraRestorationSurface(uint32_t original) {
    return ((original >> 8) & 31u) == 4u ? original & ~0x1f00u : original;
}

constexpr bool ZoraRestorationEligible(bool normalAdventure, bool realFile, int layer, bool adult, bool waterMedallion,
                                       bool waterBlueWarp, bool validEnabledState) {
    return normalAdventure && realFile && (layer == 2 || layer == 3) && adult && waterMedallion && waterBlueWarp &&
           validEnabledState;
}

constexpr bool ZoraRestorationEntryAllowed(bool eligible, bool funded, int scene, int spawn) {
    return eligible && funded && scene == 0x58 && spawn >= 0 && spawn <= 3;
}

// Native Lake shortcut ice model fitted to the existing tunnel mouth. Collision
// uses rounded vertices; the resource test checks their coverage against the
// actual native mouth and rendered model, including the signed-angle transform.
inline constexpr std::array<float, 3> kZoraClosurePosition{ -175.0f, -222.0f, -203.0f };
inline constexpr std::array<float, 3> kZoraClosureScale{ 0.14f, 0.12f, 0.10f };
inline constexpr int16_t kZoraClosureYaw = -29829;
inline constexpr std::array<std::array<int16_t, 3>, 4> kZoraClosureVertices{ {
    { -135, -222, -215 },
    { -215, -135, -191 },
    { -135, -135, -215 },
    { -215, -222, -191 },
} };

constexpr uint64_t ZoraRestorationHashWord(uint64_t hash, uint32_t word) {
    for (unsigned int byte = 0; byte < 4; ++byte)
        hash = (hash ^ ((word >> (byte * 8)) & 255u)) * UINT64_C(1099511628211);
    return hash;
}
inline constexpr uint64_t kZoraRestorationHashStart = UINT64_C(14695981039346656037);

} // namespace LivingHyrule
