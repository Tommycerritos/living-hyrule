#pragma once

#include "StewardshipTypes.h"
#include "Properties.h"
#include <array>

namespace LivingHyrule {

inline constexpr std::array<uint32_t, kStewardshipRegionCount> kRegionalCharterPrices = { 120000, 35000, 85000, 18000,
                                                                                          60000,  65000, 55000, 95000 };
static_assert(static_cast<uint8_t>(Region::Count) == kStewardshipRegionCount);

enum class StewardshipResult {
    Success,
    InvalidState,
    Disabled,
    InvalidRegion,
    NotLocal,
    AwaitingRecovery,
    MissingHoldings,
    RepairsRequired,
    MarketNotRestored,
    InsufficientBank,
    AlreadyChartered,
    NotChartered,
    InvalidAmount,
    InsufficientTreasury,
    TreasuryFull,
    BankFull
};

inline uint32_t RegionalPropertyMask(Region region) {
    uint32_t mask = 0;
    for (uint32_t id = 0; id < kProperties.size(); ++id)
        if (kProperties[id].region == region)
            mask |= 1u << id;
    return mask;
}

inline StewardshipResult CharterEligibility(const EconomyState& economy, const StewardshipState& state, Region region,
                                            Region currentRegion, const WorldProgress& world, bool marketRestored) {
    if (!IsValidState(economy) || !IsValidStewardshipState(state))
        return StewardshipResult::InvalidState;
    if (!economy.enabled)
        return StewardshipResult::Disabled;
    if (region >= Region::Count)
        return StewardshipResult::InvalidRegion;
    if (HoldsRegionalCharter(state, static_cast<uint8_t>(region)))
        return StewardshipResult::AlreadyChartered;
    if (region != currentRegion)
        return StewardshipResult::NotLocal;
    if (!world.adult || !RegionOpen(region, world))
        return StewardshipResult::AwaitingRecovery;
    const uint32_t holdings = RegionalPropertyMask(region);
    if (holdings == 0 || (economy.ownedProperties & holdings) != holdings ||
        (region == Region::Kakariko && !economy.ownsKakarikoCottage))
        return StewardshipResult::MissingHoldings;
    if ((economy.repairedProperties & holdings) != holdings)
        return StewardshipResult::RepairsRequired;
    if (region == Region::Market && !marketRestored)
        return StewardshipResult::MarketNotRestored;
    if (economy.bankRupees < kRegionalCharterPrices[static_cast<uint8_t>(region)])
        return StewardshipResult::InsufficientBank;
    return StewardshipResult::Success;
}

inline StewardshipResult BuyRegionalCharter(EconomyState& economy, StewardshipState& state, Region region,
                                            Region currentRegion, const WorldProgress& world, bool marketRestored) {
    const auto result = CharterEligibility(economy, state, region, currentRegion, world, marketRestored);
    if (result != StewardshipResult::Success)
        return result;
    const auto index = static_cast<uint8_t>(region);
    economy.bankRupees -= kRegionalCharterPrices[index];
    state.charterMask |= static_cast<uint8_t>(1u << index);
    return StewardshipResult::Success;
}

inline StewardshipResult TransferRegionalTreasury(EconomyState& economy, StewardshipState& state, Region region,
                                                  Region currentRegion, const WorldProgress& world, uint64_t amount,
                                                  bool deposit) {
    if (!IsValidState(economy) || !IsValidStewardshipState(state))
        return StewardshipResult::InvalidState;
    if (!economy.enabled)
        return StewardshipResult::Disabled;
    if (region >= Region::Count)
        return StewardshipResult::InvalidRegion;
    if (region != currentRegion)
        return StewardshipResult::NotLocal;
    if (!world.adult || !RegionOpen(region, world))
        return StewardshipResult::AwaitingRecovery;
    const auto index = static_cast<uint8_t>(region);
    if (!HoldsRegionalCharter(state, index))
        return StewardshipResult::NotChartered;
    if (amount == 0)
        return StewardshipResult::InvalidAmount;
    if (deposit) {
        if (amount > economy.bankRupees)
            return StewardshipResult::InsufficientBank;
        if (amount > kTreasuryLimit - state.treasury[index])
            return StewardshipResult::TreasuryFull;
        economy.bankRupees -= amount;
        state.treasury[index] += amount;
    } else {
        if (amount > state.treasury[index])
            return StewardshipResult::InsufficientTreasury;
        if (amount > kBankLimit - economy.bankRupees)
            return StewardshipResult::BankFull;
        state.treasury[index] -= amount;
        economy.bankRupees += amount;
    }
    return StewardshipResult::Success;
}

} // namespace LivingHyrule
