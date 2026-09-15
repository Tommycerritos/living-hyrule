#pragma once

#include "SocialPolicy.h"
#include "StewardshipPolicy.h"

namespace LivingHyrule {

inline constexpr uint32_t kZoraRestorationPrice = 18000;
inline constexpr uint32_t kCastleEstatePrice = 500000;
inline constexpr int kRoyalTrustedRapport = 50;
inline constexpr std::array<const char*, 8> kRoyalDeedNames = {
    "Forest Temple recovery", "Fire Temple recovery", "Water Temple recovery",    "Shadow Temple recovery",
    "Spirit Temple recovery", "Market restoration",   "Domain water restoration", "All eight regional charters",
};

inline uint8_t EligibleRoyalRecognition(const EconomyState& state, const WorldProgress& world) {
    if (!IsValidState(state) || state.enabled != 1 || !world.adult || !world.ganonDefeated)
        return 0;
    const bool completed[] = { world.forest,
                               world.fire,
                               world.water,
                               world.shadow,
                               world.spirit,
                               state.marketRestored != 0,
                               state.zoraRestored != 0,
                               RegionalCharterCount(state.stewardship) == kStewardshipRegionCount };
    uint8_t mask = 0;
    for (uint8_t bit = 0; bit < kRoyalDeedNames.size(); ++bit)
        if (completed[bit])
            mask |= static_cast<uint8_t>(1u << bit);
    return mask;
}

// Only Zelda's actual owned conversation may call this. Each recorded deed
// earns five trust once; repeated greetings never manufacture points.
inline uint8_t RecognizeRoyalDeeds(EconomyState& state, const WorldProgress& world) {
    const uint8_t newDeeds = EligibleRoyalRecognition(state, world) & ~state.royalRecognition;
    if (newDeeds == 0)
        return 0;
    auto next = state;
    next.metResidents |= ResidentBit(ResidentId::Zelda);
    next.royalRecognition |= newDeeds;
    for (uint8_t bit = 0; bit < kRoyalDeedNames.size(); ++bit)
        if ((newDeeds & (1u << bit)) != 0)
            AdjustRapport(next, ResidentId::Zelda, 5);
    state = next;
    return newDeeds;
}

inline Result FundZoraRestoration(EconomyState& state, const WorldProgress& world, bool completedWaterBlueWarp) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (state.zoraRestored)
        return Result::AlreadyRestored;
    if (!world.adult || !world.water || !completedWaterBlueWarp)
        return Result::Unavailable;
    if (state.bankRupees < kZoraRestorationPrice)
        return Result::InsufficientBank;
    auto next = state;
    next.bankRupees -= kZoraRestorationPrice;
    next.zoraRestored = 1;
    AdjustRapport(next, ResidentId::Lethra, 10);
    AdjustRapport(next, ResidentId::Neris, 10);
    state = next;
    return Result::Success;
}

inline uint32_t CastleEstatePrice(const EconomyState& state) {
    if (!IsValidState(state))
        return 0;
    return GetRapport(state, ResidentId::Zelda) >= kRoyalTrustedRapport ? kCastleEstatePrice * 9u / 10u
                                                                        : kCastleEstatePrice;
}
inline Result CastleEstateEligibility(const EconomyState& state, const WorldProgress& world, bool estateReady) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (state.castleEstateOwned)
        return Result::AlreadyOwned;
    if (!world.adult || !world.ganonDefeated || !state.marketRestored || !estateReady ||
        RegionalCharterCount(state.stewardship) != kStewardshipRegionCount)
        return Result::Unavailable;
    return state.bankRupees < CastleEstatePrice(state) ? Result::InsufficientBank : Result::Success;
}
inline Result BuyCastleEstate(EconomyState& state, const WorldProgress& world, bool estateReady) {
    const auto result = CastleEstateEligibility(state, world, estateReady);
    if (result != Result::Success)
        return result;
    auto next = state;
    next.bankRupees -= CastleEstatePrice(state);
    next.castleEstateOwned = 1;
    AdjustRapport(next, ResidentId::Zelda, 10);
    AdjustRapport(next, ResidentId::Aren, 10);
    AdjustRapport(next, ResidentId::Maelin, 10);
    state = next;
    return Result::Success;
}

} // namespace LivingHyrule
