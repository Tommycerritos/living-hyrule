#ifndef LIVING_HYRULE_ECONOMY_H
#define LIVING_HYRULE_ECONOMY_H

#include "SocialTypes.h"
#include "RegionalWardrobeTypes.h"
#include "StewardshipTypes.h"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace LivingHyrule {

// The C-compatible save member has no constructor: initialize fresh state with {}.
using EconomyState = LivingHyruleSaveData;
static_assert(std::is_trivially_copyable<EconomyState>::value, "Economy saves must be trivially copyable");
static_assert(std::is_standard_layout<EconomyState>::value, "Economy saves must have standard layout");

inline constexpr uint64_t kBankLimit = 999999999;
inline constexpr uint32_t kCottagePrice = 1200;
inline constexpr uint32_t kRentPerPeriod = 25;
inline constexpr uint32_t kHighRentPerPeriod = 40;
inline constexpr uint32_t kStrainedRentPerPeriod = 15;
inline constexpr uint32_t kFramesPerRentPeriod = 12000; // Ten minutes at 20 active gameplay ticks per second.

enum class Result {
    Success,
    Disabled,
    InvalidState,
    InvalidAmount,
    InvalidWallet,
    InsufficientWallet,
    InsufficientBank,
    WalletFull,
    BankFull,
    AlreadyOwned,
    Unavailable,
    NotOwned,
    AlreadyRepaired,
    AlreadyCompleted,
    FavorInProgress,
    NoActiveFavor,
    WrongResident,
    AlreadyRestored,
};

// Disabled files may retain their assets and partial rent period. Never repair
// unknown/corrupt state implicitly: callers can reject it during save loading.
inline bool IsValidState(const EconomyState& state) {
    if (state.enabled > 1 || state.ownsKakarikoCottage > 1 || state.bankRupees > kBankLimit ||
        state.rentalFrames >= kFramesPerRentPeriod || (state.ownedProperties & ~0xffffu) != 0 ||
        (state.repairedProperties & ~state.ownedProperties) != 0 || !IsValidSocialState(state) ||
        !IsValidWardrobeState(state.wardrobe) || !IsValidStewardshipState(state.stewardship)) {
        return false;
    }
    for (unsigned int i = 0; i < 16; ++i) {
        if (state.businessFrames[i] >= kFramesPerRentPeriod ||
            (!(state.ownedProperties & (1u << i)) && state.businessFrames[i] != 0)) {
            return false;
        }
    }
    return true;
}

inline bool IsValidWallet(int16_t wallet, int walletCapacity) {
    return walletCapacity >= 0 && walletCapacity <= (std::numeric_limits<int16_t>::max)() && wallet >= 0 &&
           wallet <= walletCapacity;
}

inline Result MarkResidentMet(EconomyState& state, ResidentId id) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (!IsValidResident(id))
        return Result::InvalidAmount;
    state.metResidents |= ResidentBit(id);
    return Result::Success;
}

inline Result AdjustRapport(EconomyState& state, ResidentId id, int delta) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (!IsValidResident(id))
        return Result::InvalidAmount;
    // Widen before adding so even a caller's extreme delta cannot overflow.
    const int64_t next = static_cast<int64_t>(GetRapport(state, id)) + delta;
    state.rapport[static_cast<uint32_t>(id)] = static_cast<int8_t>(next < kRapportMinimum   ? kRapportMinimum
                                                                   : next > kRapportMaximum ? kRapportMaximum
                                                                                            : next);
    return Result::Success;
}

inline Result SetCottageRentPolicy(EconomyState& state, bool high) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (!state.ownsKakarikoCottage)
        return Result::NotOwned;
    state.cottageRentPolicy = high ? 1 : 0;
    return Result::Success;
}

// Current-period terms, before the bank-cap limit. Readable disabled files may
// still display their retained terms; TickRent separately checks eligibility.
inline uint32_t EffectiveCottageRent(const EconomyState& state) {
    if (!IsValidState(state))
        return 0;
    if (!state.currentPeriodPolicy)
        return kRentPerPeriod;
    return GetRapport(state, ResidentId::Bram) <= -10 ? kStrainedRentPerPeriod : kHighRentPerPeriod;
}

// Every failure leaves both the save state and the wallet untouched.
inline Result Deposit(EconomyState& state, int16_t& wallet, int walletCapacity, uint32_t amount) {
    if (!IsValidState(state)) {
        return Result::InvalidState;
    }
    if (!state.enabled) {
        return Result::Disabled;
    }
    if (amount == 0 || amount > kBankLimit) {
        return Result::InvalidAmount;
    }
    if (!IsValidWallet(wallet, walletCapacity)) {
        return Result::InvalidWallet;
    }
    if (amount > static_cast<uint32_t>(wallet)) {
        return Result::InsufficientWallet;
    }
    if (amount > kBankLimit - state.bankRupees) {
        return Result::BankFull;
    }
    state.bankRupees += amount;
    wallet = static_cast<int16_t>(wallet - amount);
    return Result::Success;
}

inline Result Withdraw(EconomyState& state, int16_t& wallet, int walletCapacity, uint32_t amount) {
    if (!IsValidState(state)) {
        return Result::InvalidState;
    }
    if (!state.enabled) {
        return Result::Disabled;
    }
    if (amount == 0 || amount > kBankLimit) {
        return Result::InvalidAmount;
    }
    if (!IsValidWallet(wallet, walletCapacity)) {
        return Result::InvalidWallet;
    }
    if (amount > state.bankRupees) {
        return Result::InsufficientBank;
    }
    if (amount > static_cast<uint32_t>(walletCapacity - wallet)) {
        return Result::WalletFull;
    }
    state.bankRupees -= amount;
    wallet = static_cast<int16_t>(wallet + amount);
    return Result::Success;
}

inline Result BuyCottage(EconomyState& state) {
    if (!IsValidState(state)) {
        return Result::InvalidState;
    }
    if (!state.enabled) {
        return Result::Disabled;
    }
    if (state.ownsKakarikoCottage) {
        return Result::AlreadyOwned;
    }
    if (state.bankRupees < kCottagePrice) {
        return Result::InsufficientBank;
    }
    state.bankRupees -= kCottagePrice;
    state.ownsKakarikoCottage = 1;
    state.rentalFrames = 0;
    state.cottageRentPolicy = 0;
    state.currentPeriodPolicy = 0;
    return Result::Success;
}

// Call once per eligible, active gameplay tick. Returns rupees actually credited.
// A completed period is consumed even at the bank limit; uncredited rent does
// not accumulate as a debt. Lifetime earnings count credits and saturate safely.
inline uint32_t TickRent(EconomyState& state) {
    if (!IsValidState(state) || !state.enabled || !state.ownsKakarikoCottage) {
        return 0;
    }
    ++state.rentalFrames;
    if (state.rentalFrames < kFramesPerRentPeriod) {
        return 0;
    }
    state.rentalFrames = 0;
    const uint64_t bankRoom = kBankLimit - state.bankRupees;
    const uint32_t payment = EffectiveCottageRent(state);
    const uint32_t credited = bankRoom < payment ? static_cast<uint32_t>(bankRoom) : payment;
    state.bankRupees += credited;
    const uint64_t earningsRoom = (std::numeric_limits<uint64_t>::max)() - state.totalRentEarned;
    state.totalRentEarned += earningsRoom < credited ? earningsRoom : credited;
    if (credited != 0) {
        if (state.currentPeriodPolicy)
            AdjustRapport(state, ResidentId::Bram, -3);
        else if (GetRapport(state, ResidentId::Bram) < 20)
            AdjustRapport(state, ResidentId::Bram, 1);
    }
    // A bank-full period is still consumed. Changing requested terms cannot
    // rewrite a nearly completed period or earn rapport without credited rent.
    state.currentPeriodPolicy = state.cottageRentPolicy;
    return credited;
}

} // namespace LivingHyrule

#endif // LIVING_HYRULE_ECONOMY_H
