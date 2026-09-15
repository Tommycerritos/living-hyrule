#include "ZoraRestorationPolicy.h"
#include <cmath>
#include <iostream>

int main() {
    using namespace LivingHyrule;
    unsigned int checks = 0;
    const auto check = [&](bool condition) {
        ++checks;
        if (!condition)
            std::cerr << "Domain restoration failed at check " << checks << '\n';
        return condition;
    };
    for (unsigned int gates = 0; gates < 64; ++gates) {
        for (int layer = -1; layer <= 20; ++layer) {
            const bool eligible = ZoraRestorationEligible((gates & 1) != 0, (gates & 2) != 0, layer, (gates & 4) != 0,
                                                          (gates & 8) != 0, (gates & 16) != 0, (gates & 32) != 0);
            const bool expected = gates == 63 && (layer == 2 || layer == 3);
            if (!check(eligible == expected))
                return 1;
            for (int scene : { -1, 0x57, 0x58, 0x59, 0x60, 0x64 }) {
                for (int spawn : { -1, 0, 1, 2, 3, 4, 5, 255 }) {
                    for (bool funded : { false, true }) {
                        if (!check(ZoraRestorationEntryAllowed(eligible, funded, scene, spawn) ==
                                   (expected && funded && scene == 0x58 && spawn >= 0 && spawn <= 3)))
                            return 1;
                    }
                }
            }
        }
    }
    for (uint32_t high : { 0u, 0xffffe000u, 0x92346000u, 0x55554000u }) {
        for (uint32_t exit = 0; exit < 32; ++exit) {
            for (uint32_t camera = 0; camera < 256; ++camera) {
                const uint32_t original = high | (exit << 8) | camera;
                const uint32_t result = ZoraRestorationSurface(original);
                if (!check((result & ~0x1f00u) == (original & ~0x1f00u)) ||
                    !check(((result >> 8) & 31u) == (exit == 4 ? 0 : exit)) ||
                    !check(ZoraRestorationSurface(result) == result))
                    return 1;
            }
        }
    }
    // All original Domain surfaces except the Lake exit remain byte-identical.
    if (!check(ZoraRestorationSurface(1029) == 5) || !check(ZoraRestorationSurface(0xffffffffu) == 0xffffffffu))
        return 1;
    for (const auto& vertex : kZoraClosureVertices) {
        if (!check(vertex[0] >= -215 && vertex[0] <= -135 && vertex[1] >= -222 && vertex[1] <= -135 &&
                   vertex[2] >= -215 && vertex[2] <= -191))
            return 1;
    }
    // Geometry-facing invariants: vertical cap, two nondegenerate coplanar faces,
    // outward normal toward the main pool. Native-data tests verify the fit.
    const auto& a = kZoraClosureVertices[0];
    const auto& b = kZoraClosureVertices[1];
    const auto& c = kZoraClosureVertices[2];
    const auto& d = kZoraClosureVertices[3];
    const int nx = (b[1] - a[1]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[1] - a[1]);
    const int nz = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    if (!check(nx < 0 && nz < 0) || !check(nx * (d[0] - a[0]) + nz * (d[2] - a[2]) == 0))
        return 1;
    std::cout << checks << " Domain restoration policy checks passed.\n";
    return 0;
}
