#pragma once

#include "SocialPolicy.h"

namespace LivingHyrule {

enum class GiftKind : uint8_t { Provisions, Supplies, Keepsake, Count };
struct ResidentGift {
    const char* name;
    uint32_t price;
};
inline constexpr std::array<ResidentGift, kGiftKinds> kResidentGifts = { {
    { "Regional provisions", 60 },
    { "Work supplies", 180 },
    { "Handmade keepsake", 350 },
} };
inline constexpr std::array<GiftKind, kSocialResidentCount> kPreferredGifts = {
    GiftKind::Supplies, GiftKind::Supplies,   GiftKind::Supplies,   GiftKind::Provisions, GiftKind::Supplies,
    GiftKind::Supplies, GiftKind::Provisions, GiftKind::Supplies,   GiftKind::Provisions, GiftKind::Supplies,
    GiftKind::Supplies, GiftKind::Keepsake,   GiftKind::Provisions, GiftKind::Supplies,   GiftKind::Provisions,
    GiftKind::Supplies, GiftKind::Supplies,   GiftKind::Keepsake,   GiftKind::Supplies,   GiftKind::Keepsake,
    GiftKind::Keepsake, GiftKind::Supplies,   GiftKind::Keepsake,
};
constexpr bool ValidGiftKind(GiftKind kind) {
    return kind < GiftKind::Count;
}
constexpr uint8_t GiftBit(GiftKind kind) {
    return ValidGiftKind(kind) ? static_cast<uint8_t>(1u << static_cast<uint8_t>(kind)) : 0;
}
inline bool GiftAlreadyGiven(const EconomyState& state, ResidentId resident, GiftKind kind) {
    return IsValidResident(resident) && (state.givenGifts[static_cast<uint32_t>(resident)] & GiftBit(kind)) != 0;
}
inline const char* GiftNameFor(ResidentId resident, GiftKind kind) {
    if (!ValidGiftKind(kind))
        return "Unknown gift";
    if (kind == GiftKind::Provisions && (resident == ResidentId::Doron || resident == ResidentId::Brakka))
        return "A basket of mineral rations";
    return kResidentGifts[static_cast<uint8_t>(kind)].name;
}
inline GiftKind NextResidentGift(const EconomyState& state, ResidentId resident) {
    if (!IsValidState(state) || !IsValidResident(resident))
        return GiftKind::Count;
    const auto preferred = kPreferredGifts[static_cast<uint32_t>(resident)];
    if (!GiftAlreadyGiven(state, resident, preferred))
        return preferred;
    for (uint8_t kind = 0; kind < kGiftKinds; ++kind) {
        if (!GiftAlreadyGiven(state, resident, static_cast<GiftKind>(kind)))
            return static_cast<GiftKind>(kind);
    }
    return GiftKind::Count;
}

// The runtime must additionally require the actual recipient's fresh, owned
// conversation and show this same bank-funded price before confirming.
inline Result GiveResidentGift(EconomyState& state, ResidentId resident, GiftKind kind, const WorldProgress& world) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (!IsValidResident(resident) || !ValidGiftKind(kind))
        return Result::InvalidAmount;
    if (!ResidentAccessible(resident, world))
        return Result::Unavailable;
    if (GiftAlreadyGiven(state, resident, kind))
        return Result::AlreadyCompleted;
    const uint32_t price = kResidentGifts[static_cast<uint8_t>(kind)].price;
    if (state.bankRupees < price)
        return Result::InsufficientBank;
    auto next = state;
    next.bankRupees -= price;
    next.metResidents |= ResidentBit(resident);
    next.givenGifts[static_cast<uint32_t>(resident)] |= GiftBit(kind);
    const int appreciation = kPreferredGifts[static_cast<uint32_t>(resident)] == kind ? 8 : 4;
    AdjustRapport(next, resident, appreciation);
    state = next;
    return Result::Success;
}

} // namespace LivingHyrule
