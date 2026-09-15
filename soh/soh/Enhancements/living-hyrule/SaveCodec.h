#pragma once

#include "Economy.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace LivingHyrule {

namespace SaveCodecDetail {

template <typename T> inline bool ReadUnsigned(const nlohmann::json& value, T& output) {
    uint64_t parsed;
    if (value.is_number_unsigned()) {
        parsed = value.get<uint64_t>();
    } else if (value.is_number_integer()) {
        const int64_t signedValue = value.get<int64_t>();
        if (signedValue < 0) {
            return false;
        }
        parsed = static_cast<uint64_t>(signedValue);
    } else {
        return false;
    }

    if (parsed > (std::numeric_limits<T>::max)()) {
        return false;
    }
    output = static_cast<T>(parsed);
    return true;
}

inline bool ReadRapport(const nlohmann::json& value, int8_t& output) {
    int64_t parsed;
    if (value.is_number_unsigned()) {
        const uint64_t unsignedValue = value.get<uint64_t>();
        if (unsignedValue > static_cast<uint64_t>(kRapportMaximum))
            return false;
        parsed = static_cast<int64_t>(unsignedValue);
    } else if (value.is_number_integer()) {
        parsed = value.get<int64_t>();
    } else {
        return false;
    }
    if (parsed < kRapportMinimum || parsed > kRapportMaximum)
        return false;
    output = static_cast<int8_t>(parsed);
    return true;
}

} // namespace SaveCodecDetail

inline nlohmann::json EncodeEconomy(const EconomyState& state) {
    if (!IsValidState(state)) {
        throw std::invalid_argument("Cannot encode invalid Living Hyrule economy state");
    }

    return {
        { "schemaVersion", 5 },
        { "enabled", state.enabled != 0 },
        { "bankRupees", state.bankRupees },
        { "ownsKakarikoCottage", state.ownsKakarikoCottage != 0 },
        { "rentalFrames", state.rentalFrames },
        { "totalRentEarned", state.totalRentEarned },
        { "ownedProperties", state.ownedProperties },
        { "repairedProperties", state.repairedProperties },
        { "businessFrames", state.businessFrames },
        { "totalBusinessEarned", state.totalBusinessEarned },
        { "rapport", state.rapport },
        { "metResidents", state.metResidents },
        { "completedFavors", state.completedFavors },
        { "activeFavor", state.activeFavor },
        { "cottageRentPolicy", state.cottageRentPolicy },
        { "currentPeriodPolicy", state.currentPeriodPolicy },
        { "marketRestored", state.marketRestored != 0 },
        { "wardrobe",
          { { "ownedStyles", state.wardrobe.ownedStyles }, { "equippedStyle", state.wardrobe.equippedStyle } } },
        { "stewardship",
          { { "charterMask", state.stewardship.charterMask }, { "treasury", state.stewardship.treasury } } },
        { "zoraRestored", state.zoraRestored != 0 },
        { "castleEstateOwned", state.castleEstateOwned != 0 },
        { "givenGifts", state.givenGifts },
        { "royalRecognition", state.royalRecognition },
    };
}

inline bool DecodeEconomy(const nlohmann::json& data, EconomyState& output) noexcept {
    try {
        if (!data.is_object()) {
            return false;
        }
        for (const char* name :
             { "schemaVersion", "enabled", "bankRupees", "ownsKakarikoCottage", "rentalFrames", "totalRentEarned" }) {
            if (!data.contains(name)) {
                return false;
            }
        }

        uint64_t schemaVersion = 0;
        if (!SaveCodecDetail::ReadUnsigned(data.at("schemaVersion"), schemaVersion) ||
            (schemaVersion < 1 || schemaVersion > 5) || !data.at("enabled").is_boolean() ||
            !data.at("ownsKakarikoCottage").is_boolean()) {
            return false;
        }

        EconomyState candidate{};
        candidate.enabled = data.at("enabled").get<bool>();
        candidate.ownsKakarikoCottage = data.at("ownsKakarikoCottage").get<bool>();
        if (!SaveCodecDetail::ReadUnsigned(data.at("bankRupees"), candidate.bankRupees) ||
            !SaveCodecDetail::ReadUnsigned(data.at("rentalFrames"), candidate.rentalFrames) ||
            !SaveCodecDetail::ReadUnsigned(data.at("totalRentEarned"), candidate.totalRentEarned)) {
            return false;
        }

        // Schema one migrates without inventing deeds, earnings or repairs.
        if (schemaVersion >= 2) {
            if (!SaveCodecDetail::ReadUnsigned(data.at("ownedProperties"), candidate.ownedProperties) ||
                !SaveCodecDetail::ReadUnsigned(data.at("repairedProperties"), candidate.repairedProperties) ||
                !SaveCodecDetail::ReadUnsigned(data.at("totalBusinessEarned"), candidate.totalBusinessEarned))
                return false;
            const auto& frames = data.at("businessFrames");
            if (!frames.is_array() || frames.size() != 16)
                return false;
            for (unsigned int i = 0; i < 16; ++i) {
                if (!SaveCodecDetail::ReadUnsigned(frames[i], candidate.businessFrames[i]))
                    return false;
            }
        }
        // Earlier schemas retain every existing asset and start with neutral
        // relationships, no errands, fair rent and no funded reconstruction.
        if (schemaVersion >= 3) {
            if (!SaveCodecDetail::ReadUnsigned(data.at("metResidents"), candidate.metResidents) ||
                !SaveCodecDetail::ReadUnsigned(data.at("completedFavors"), candidate.completedFavors) ||
                !SaveCodecDetail::ReadUnsigned(data.at("activeFavor"), candidate.activeFavor) ||
                !SaveCodecDetail::ReadUnsigned(data.at("cottageRentPolicy"), candidate.cottageRentPolicy) ||
                !SaveCodecDetail::ReadUnsigned(data.at("currentPeriodPolicy"), candidate.currentPeriodPolicy) ||
                !data.at("marketRestored").is_boolean())
                return false;
            candidate.marketRestored = data.at("marketRestored").get<bool>();
            const auto& rapport = data.at("rapport");
            if (!rapport.is_array() || rapport.size() != kSocialResidentCount)
                return false;
            for (uint32_t i = 0; i < kSocialResidentCount; ++i) {
                if (!SaveCodecDetail::ReadRapport(rapport[i], candidate.rapport[i]))
                    return false;
            }
        }
        // Schema four embeds both modules in the same copied save snapshot.
        // Older files receive original clothing and no charters or treasury.
        if (schemaVersion >= 4) {
            const auto& wardrobe = data.at("wardrobe");
            const auto& stewardship = data.at("stewardship");
            if (!wardrobe.is_object() || !stewardship.is_object() ||
                !SaveCodecDetail::ReadUnsigned(wardrobe.at("ownedStyles"), candidate.wardrobe.ownedStyles) ||
                !SaveCodecDetail::ReadUnsigned(wardrobe.at("equippedStyle"), candidate.wardrobe.equippedStyle) ||
                !SaveCodecDetail::ReadUnsigned(stewardship.at("charterMask"), candidate.stewardship.charterMask))
                return false;
            const auto& treasury = stewardship.at("treasury");
            if (!treasury.is_array() || treasury.size() != kStewardshipRegionCount)
                return false;
            for (uint8_t region = 0; region < kStewardshipRegionCount; ++region) {
                if (!SaveCodecDetail::ReadUnsigned(treasury[region], candidate.stewardship.treasury[region]))
                    return false;
            }
        }
        // Schema five records finite gifts and further recovery. Earlier files
        // keep their progress without inventing gifts, recognition or purchases.
        if (schemaVersion >= 5) {
            if (!data.at("zoraRestored").is_boolean() || !data.at("castleEstateOwned").is_boolean() ||
                !SaveCodecDetail::ReadUnsigned(data.at("royalRecognition"), candidate.royalRecognition))
                return false;
            candidate.zoraRestored = data.at("zoraRestored").get<bool>();
            candidate.castleEstateOwned = data.at("castleEstateOwned").get<bool>();
            const auto& gifts = data.at("givenGifts");
            if (!gifts.is_array() || gifts.size() != kSocialResidentCount)
                return false;
            for (uint32_t resident = 0; resident < kSocialResidentCount; ++resident) {
                if (!SaveCodecDetail::ReadUnsigned(gifts[resident], candidate.givenGifts[resident]))
                    return false;
            }
        }
        if (!IsValidState(candidate))
            return false;

        output = candidate;
        return true;
    } catch (...) {
        // A malformed optional module section must never abort loading the game save.
        return false;
    }
}

} // namespace LivingHyrule
