#pragma once

#include "Properties.h"
#include <array>

namespace LivingHyrule {

struct Favor {
    ResidentId issuer;
    ResidentId recipient;
    const char* name;
    const char* instructions;
};

// IDs 1..10 are permanent. The carried item is mod-owned save data, never a
// vanilla inventory item or quest flag. Each delivery rewards its two people once.
inline constexpr std::array<Favor, kFavorCount> kFavors = { {
    { ResidentId::Tavin, ResidentId::Bram, "Cottage measurements", "Take Tavin's measurements to Bram in Kakariko." },
    { ResidentId::Orlen, ResidentId::Hollis, "Timber invoice",
      "Take Orlen's timber invoice to Hollis in Hyrule Field." },
    { ResidentId::Caro, ResidentId::Vessa, "Growers' notice", "Take Caro's growers' notice to Vessa in the Market." },
    { ResidentId::Hadrin, ResidentId::Pella, "Evening lamps",
      "Take Hadrin's lamp request to Pella in the Market after dark." },
    { ResidentId::Nessa, ResidentId::Wren, "Feed tally", "Take Nessa's feed tally to Wren at Lon Lon Ranch." },
    { ResidentId::Vero, ResidentId::Edda, "Catch observations",
      "Take Vero's catch observations to Edda at Lake Hylia." },
    { ResidentId::Fenn, ResidentId::Luma, "Woodland seeds", "Take Fenn's seed packet to Luma in Kokiri Forest." },
    { ResidentId::Doron, ResidentId::Brakka, "Stone sample", "Take Doron's stone sample to Brakka in Goron City." },
    { ResidentId::Lethra, ResidentId::Neris, "Water readings",
      "Take Lethra's water readings to Neris beside Zora's River." },
    { ResidentId::Rasha, ResidentId::Kesra, "Cloth tally", "Take Rasha's cloth tally to Kesra at Gerudo Fortress." },
} };
inline constexpr uint32_t kMarketRestorationPrice = 25000;

constexpr const Favor* GetFavor(uint8_t id) {
    return id >= 1 && id <= kFavorCount ? &kFavors[id - 1] : nullptr;
}
inline bool FavorCompleted(const EconomyState& state, uint8_t id) {
    return (state.completedFavors & FavorBit(id)) != 0;
}

// Availability over a full day, not presence at the current time. A nighttime
// recipient remains a valid errand destination. The engine must separately
// verify the current owned conversation and normal/adventure scene conditions.
inline bool ResidentAccessible(ResidentId id, const WorldProgress& world) {
    switch (id) {
        case ResidentId::Bram:
        case ResidentId::Wren:
        case ResidentId::Edda:
        case ResidentId::Lethra:
            return true;
        case ResidentId::Tavin:
        case ResidentId::Orlen:
            return !world.adult || world.shadow;
        case ResidentId::Vessa:
        case ResidentId::Hadrin:
        case ResidentId::Pella:
            return !world.adult || world.ganonDefeated;
        case ResidentId::Caro:
        case ResidentId::Hollis:
        case ResidentId::Fenn:
        case ResidentId::Luma:
            return !world.adult || world.forest;
        case ResidentId::Nessa:
            return !world.adult || world.ranchFreed;
        case ResidentId::Vero:
        case ResidentId::Neris:
            return !world.adult || world.water;
        case ResidentId::Doron:
        case ResidentId::Brakka:
            return !world.adult || world.fire;
        case ResidentId::Rasha:
        case ResidentId::Kesra:
            return world.adult && world.gerudoMembership && world.spirit;
        case ResidentId::Zelda:
        case ResidentId::Aren:
        case ResidentId::Maelin:
            return world.adult && world.ganonDefeated;
        default:
            return false;
    }
}

inline Result FavorAcceptanceResult(const EconomyState& state, uint8_t id, const WorldProgress& world) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    const Favor* favor = GetFavor(id);
    if (favor == nullptr)
        return Result::InvalidAmount;
    if (FavorCompleted(state, id))
        return Result::AlreadyCompleted;
    if (state.activeFavor != 0)
        return Result::FavorInProgress;
    if (!ResidentAccessible(favor->issuer, world) || !ResidentAccessible(favor->recipient, world))
        return Result::Unavailable;
    return Result::Success;
}
inline bool CanAcceptFavor(const EconomyState& state, uint8_t id, const WorldProgress& world) {
    return FavorAcceptanceResult(state, id, world) == Result::Success;
}
inline Result AcceptFavor(EconomyState& state, uint8_t id, const WorldProgress& world) {
    const Result result = FavorAcceptanceResult(state, id, world);
    if (result != Result::Success)
        return result;
    state.activeFavor = id;
    return Result::Success;
}
inline Result CompleteFavor(EconomyState& state, uint8_t id, ResidentId speaker, const WorldProgress& world) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    const Favor* favor = GetFavor(id);
    if (favor == nullptr)
        return Result::InvalidAmount;
    if (FavorCompleted(state, id))
        return Result::AlreadyCompleted;
    if (state.activeFavor == 0)
        return Result::NoActiveFavor;
    if (state.activeFavor != id)
        return Result::Unavailable;
    if (speaker != favor->recipient)
        return Result::WrongResident;
    if (!ResidentAccessible(speaker, world))
        return Result::Unavailable;
    state.activeFavor = 0;
    state.completedFavors |= FavorBit(id);
    AdjustRapport(state, favor->issuer, 10);
    AdjustRapport(state, favor->recipient, 10);
    return Result::Success;
}
inline Result AbandonFavor(EconomyState& state) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (state.activeFavor == 0)
        return Result::NoActiveFavor;
    state.activeFavor = 0;
    return Result::Success;
}

// The engine caller additionally validates a safe local reconstruction site and
// compatible native resources before committing this bank payment.
inline Result FundMarketRestoration(EconomyState& state, const WorldProgress& world) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (state.marketRestored)
        return Result::AlreadyRestored;
    if (!world.adult || !world.ganonDefeated)
        return Result::Unavailable;
    if (state.bankRupees < kMarketRestorationPrice)
        return Result::InsufficientBank;
    state.bankRupees -= kMarketRestorationPrice;
    state.marketRestored = 1;
    // A real district investment is remembered by its workers and the royal
    // household. The paid flag above prevents collecting this trust twice.
    AdjustRapport(state, ResidentId::Zelda, 20);
    for (const auto resident :
         { ResidentId::Vessa, ResidentId::Hadrin, ResidentId::Pella, ResidentId::Aren, ResidentId::Maelin })
        AdjustRapport(state, resident, 10);
    return Result::Success;
}

} // namespace LivingHyrule
