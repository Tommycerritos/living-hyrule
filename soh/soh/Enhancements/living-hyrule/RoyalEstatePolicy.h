#pragma once

#include <array>
#include <cstdint>

namespace LivingHyrule {

inline constexpr int kRoyalGardenScene = 0x4A;
inline constexpr int kRoyalGardenEntrance = 0x400;
inline constexpr int kRoyalGardenReturnEntrance = 0x138;
inline constexpr uint64_t kRoyalGardenCollisionHash = UINT64_C(0xfccffc007b957c78);

// A visit uses the ordinary adult header only. Child Zelda's audience and all
// ending/song/cutscene setups retain their original actors and exits.
constexpr bool RoyalGardenLoadAllowed(bool supportedAdventure, bool realFile, bool adult, bool normalLayer, int scene,
                                      int spawn, int entrance) {
    return supportedAdventure && realFile && adult && normalLayer && scene == kRoyalGardenScene &&
           ((entrance == kRoyalGardenEntrance && spawn == 0) || (entrance == 0x5F0 && spawn == 1));
}

// Ownership is a separate prestige purchase. The restored Market and genuine
// final-boss record unlock a visit regardless of bank balance or charters.
constexpr bool RoyalGardenVisitUnlocked(bool supportedAdventure, bool realFile, bool adult, bool normalLayer,
                                        bool victory, bool validEnabledEconomy, bool restoredMarket) {
    return supportedAdventure && realFile && adult && normalLayer && victory && validEnabledEconomy && restoredMarket;
}

constexpr bool RoyalGardenOriginAllowed(int scene, bool activeMarket, bool activeGarden) {
    return scene == 0x64 || (scene == 0x22 && activeMarket) || (scene == kRoyalGardenScene && activeGarden);
}

struct RoyalGardenPlacement {
    float x, y, z;
    int16_t yaw;
};
// Zelda, Aren, Maelin: dry native walkways, outside the entrance and exit lane.
inline constexpr std::array<RoyalGardenPlacement, 3> kRoyalGardenPlacements = { {
    { -450, 84, 0, 16384 },
    { 320, 44, -40, 16384 },
    { 0, 44, 220, -32768 },
} };

} // namespace LivingHyrule
