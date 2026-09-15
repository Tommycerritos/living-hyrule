#pragma once

#include "SocialTypes.h"

namespace LivingHyrule {

inline constexpr uint8_t kGiftKinds = 3;
inline constexpr uint8_t kGiftMask = (1u << kGiftKinds) - 1u;

inline bool IsValidRecoveryState(const LivingHyruleSaveData& state) {
    if (state.zoraRestored > 1 || state.castleEstateOwned > 1)
        return false;
    for (uint32_t resident = 0; resident < kSocialResidentCount; ++resident) {
        if ((state.givenGifts[resident] & ~kGiftMask) != 0 ||
            (state.givenGifts[resident] != 0 && (state.metResidents & (1u << resident)) == 0))
            return false;
    }
    if (((state.royalRecognition & (1u << 5)) != 0 && !state.marketRestored) ||
        ((state.royalRecognition & (1u << 6)) != 0 && !state.zoraRestored) ||
        ((state.royalRecognition & (1u << 7)) != 0 && state.stewardship.charterMask != 0xffu))
        return false;
    return !state.castleEstateOwned || (state.marketRestored && state.stewardship.charterMask == 0xffu);
}

} // namespace LivingHyrule
