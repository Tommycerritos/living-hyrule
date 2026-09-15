#pragma once

#include "living_hyrule_save.h"
#include <array>
#include <cstdint>

namespace LivingHyrule {

// Permanent save identities, independent of dynamically registered actor IDs.
enum class ResidentId : uint8_t {
    Tavin = 0,
    Bram = 1,
    Orlen = 2,
    Vessa = 3,
    Hadrin = 4,
    Pella = 5,
    Caro = 6,
    Hollis = 7,
    Nessa = 8,
    Wren = 9,
    Vero = 10,
    Edda = 11,
    Fenn = 12,
    Luma = 13,
    Doron = 14,
    Brakka = 15,
    Lethra = 16,
    Neris = 17,
    Rasha = 18,
    Kesra = 19,
    Zelda = 20,
    Aren = 21,
    Maelin = 22,
    Count = 23,
};
inline constexpr uint32_t kSocialResidentCount = static_cast<uint32_t>(ResidentId::Count);
inline constexpr uint8_t kFavorCount = 10;
inline constexpr int kRapportMinimum = -100;
inline constexpr int kRapportMaximum = 100;
inline constexpr int kTrustedRapport = 10;
inline constexpr uint32_t kMetResidentsMask = (1u << kSocialResidentCount) - 1u;
inline constexpr uint16_t kCompletedFavorsMask = (1u << kFavorCount) - 1u;
inline constexpr std::array<const char*, kSocialResidentCount> kSocialResidentNames = {
    "Tavin", "Bram", "Orlen", "Vessa",  "Hadrin", "Pella", "Caro",  "Hollis", "Nessa", "Wren",         "Vero",  "Edda",
    "Fenn",  "Luma", "Doron", "Brakka", "Lethra", "Neris", "Rasha", "Kesra",  "Zelda", "Captain Aren", "Maelin"
};

constexpr bool IsValidResident(ResidentId id) {
    return id < ResidentId::Count;
}
constexpr uint32_t ResidentBit(ResidentId id) {
    return IsValidResident(id) ? 1u << static_cast<uint32_t>(id) : 0;
}
constexpr uint16_t FavorBit(uint8_t id) {
    return id >= 1 && id <= kFavorCount ? static_cast<uint16_t>(1u << (id - 1)) : 0;
}
inline const char* GetSocialResidentName(ResidentId id) {
    return IsValidResident(id) ? kSocialResidentNames[static_cast<uint32_t>(id)] : "Unknown resident";
}
inline bool HasMetResident(const LivingHyruleSaveData& state, ResidentId id) {
    return (state.metResidents & ResidentBit(id)) != 0;
}
inline int GetRapport(const LivingHyruleSaveData& state, ResidentId id) {
    return IsValidResident(id) ? state.rapport[static_cast<uint32_t>(id)] : 0;
}
inline bool IsValidSocialState(const LivingHyruleSaveData& state) {
    if ((state.metResidents & ~kMetResidentsMask) != 0 || (state.completedFavors & ~kCompletedFavorsMask) != 0 ||
        state.activeFavor > kFavorCount || (state.completedFavors & FavorBit(state.activeFavor)) != 0 ||
        state.cottageRentPolicy > 1 || state.currentPeriodPolicy > 1 || state.marketRestored > 1) {
        return false;
    }
    for (const auto rapport : state.rapport) {
        if (rapport < kRapportMinimum || rapport > kRapportMaximum)
            return false;
    }
    return true;
}

} // namespace LivingHyrule
