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

} // namespace SaveCodecDetail

inline nlohmann::json EncodeEconomy(const EconomyState& state) {
    if (!IsValidState(state)) {
        throw std::invalid_argument("Cannot encode invalid Living Hyrule economy state");
    }

    return {
        { "schemaVersion", 2 },
        { "enabled", state.enabled != 0 },
        { "bankRupees", state.bankRupees },
        { "ownsKakarikoCottage", state.ownsKakarikoCottage != 0 },
        { "rentalFrames", state.rentalFrames },
        { "totalRentEarned", state.totalRentEarned },
        { "ownedProperties", state.ownedProperties },
        { "repairedProperties", state.repairedProperties },
        { "businessFrames", state.businessFrames },
        { "totalBusinessEarned", state.totalBusinessEarned },
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
            (schemaVersion != 1 && schemaVersion != 2) || !data.at("enabled").is_boolean() ||
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
        if (schemaVersion == 2) {
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
