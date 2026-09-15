#ifndef LIVING_HYRULE_ECONOMY_H
#define LIVING_HYRULE_ECONOMY_H

#include "living_hyrule_save.h"

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
};

// Disabled files may retain their assets and partial rent period. Never repair
// unknown/corrupt state implicitly: callers can reject it during save loading.
inline bool IsValidState(const EconomyState& state) {
    return state.enabled <= 1 && state.ownsKakarikoCottage <= 1 && state.bankRupees <= kBankLimit &&
           state.rentalFrames < kFramesPerRentPeriod;
}

inline bool IsValidWallet(int16_t wallet, int walletCapacity) {
    return walletCapacity >= 0 && walletCapacity <= (std::numeric_limits<int16_t>::max)() && wallet >= 0 &&
           wallet <= walletCapacity;
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
    const uint32_t credited = bankRoom < kRentPerPeriod ? static_cast<uint32_t>(bankRoom) : kRentPerPeriod;
    state.bankRupees += credited;
    const uint64_t earningsRoom = (std::numeric_limits<uint64_t>::max)() - state.totalRentEarned;
    state.totalRentEarned += earningsRoom < credited ? earningsRoom : credited;
    return credited;
}

} // namespace LivingHyrule

#endif // LIVING_HYRULE_ECONOMY_H
