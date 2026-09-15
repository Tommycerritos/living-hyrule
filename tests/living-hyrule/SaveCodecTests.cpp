#include "soh/Enhancements/living-hyrule/SaveCodec.h"

#include <algorithm>
#include <iterator>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using LivingHyrule::DecodeEconomy;
using LivingHyrule::EconomyState;
using LivingHyrule::EncodeEconomy;
using nlohmann::json;

int failures = 0;

void Check(bool condition, const std::string& description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++failures;
    }
}

bool Equal(const EconomyState& left, const EconomyState& right) {
    return left.enabled == right.enabled && left.bankRupees == right.bankRupees &&
           left.ownsKakarikoCottage == right.ownsKakarikoCottage && left.rentalFrames == right.rentalFrames &&
           left.totalRentEarned == right.totalRentEarned && left.ownedProperties == right.ownedProperties &&
           left.repairedProperties == right.repairedProperties &&
           left.totalBusinessEarned == right.totalBusinessEarned &&
           std::equal(std::begin(left.businessFrames), std::end(left.businessFrames),
                      std::begin(right.businessFrames)) &&
           std::equal(std::begin(left.rapport), std::end(left.rapport), std::begin(right.rapport)) &&
           left.metResidents == right.metResidents && left.completedFavors == right.completedFavors &&
           left.activeFavor == right.activeFavor && left.cottageRentPolicy == right.cottageRentPolicy &&
           left.currentPeriodPolicy == right.currentPeriodPolicy && left.marketRestored == right.marketRestored &&
           left.wardrobe.ownedStyles == right.wardrobe.ownedStyles &&
           left.wardrobe.equippedStyle == right.wardrobe.equippedStyle &&
           left.stewardship.charterMask == right.stewardship.charterMask &&
           std::equal(std::begin(left.stewardship.treasury), std::end(left.stewardship.treasury),
                      std::begin(right.stewardship.treasury)) &&
           left.zoraRestored == right.zoraRestored && left.castleEstateOwned == right.castleEstateOwned &&
           std::equal(std::begin(left.givenGifts), std::end(left.givenGifts), std::begin(right.givenGifts)) &&
           left.royalRecognition == right.royalRecognition;
}

EconomyState Holdings(bool enabled = true) {
    EconomyState state{};
    state.enabled = enabled;
    state.bankRupees = 100;
    state.ownsKakarikoCottage = 1;
    state.rentalFrames = 1;
    state.totalRentEarned = (std::numeric_limits<uint64_t>::max)();
    return state;
}

EconomyState SocialHoldings(bool enabled = true) {
    auto state = Holdings(enabled);
    state.ownedProperties = 0xffff;
    state.repairedProperties = 0xaaaa;
    state.totalBusinessEarned = UINT64_MAX;
    for (unsigned int i = 0; i < 16; ++i)
        state.businessFrames[i] = i * 600;
    for (uint32_t i = 0; i < LivingHyrule::kSocialResidentCount; ++i)
        state.rapport[i] = static_cast<int8_t>(static_cast<int>(i) * 9 - 100);
    state.rapport[22] = 100;
    state.metResidents = LivingHyrule::kMetResidentsMask;
    state.completedFavors = 0x155;
    state.activeFavor = 10;
    state.cottageRentPolicy = 1;
    state.currentPeriodPolicy = 0;
    state.marketRestored = 1;
    return state;
}

EconomyState RegionalHoldings(bool enabled = true) {
    auto state = SocialHoldings(enabled);
    state.wardrobe = { 0xff, 8 };
    state.stewardship.charterMask = 0xff;
    for (uint8_t i = 0; i < LivingHyrule::kStewardshipRegionCount; ++i)
        state.stewardship.treasury[i] = i * 7919;
    state.stewardship.treasury[7] = LivingHyrule::kTreasuryLimit;
    return state;
}

EconomyState RecoveryHoldings(bool enabled = true) {
    auto state = RegionalHoldings(enabled);
    state.zoraRestored = 1;
    state.castleEstateOwned = 1;
    for (uint32_t resident = 0; resident < LivingHyrule::kSocialResidentCount; ++resident)
        state.givenGifts[resident] = static_cast<uint8_t>(resident % 7 + 1);
    state.royalRecognition = 0xff;
    return state;
}

void Reject(const json& data, const std::string& description) {
    EconomyState output = RecoveryHoldings(false);
    const EconomyState original = output;
    const json originalData = data;
    Check(!DecodeEconomy(data, output), description + " is rejected");
    Check(Equal(output, original), description + " leaves output unchanged");
    Check(data == originalData, description + " preserves original payload for read-only handling");
}

void TestRoundTrips() {
    for (const EconomyState state :
         { EconomyState{}, Holdings(), Holdings(false), SocialHoldings(), SocialHoldings(false), RegionalHoldings(),
           RegionalHoldings(false), RecoveryHoldings(), RecoveryHoldings(false) }) {
        Check(LivingHyrule::IsValidState(state), "round-trip fixture is valid");
        const json encoded = EncodeEconomy(state);
        Check(encoded.is_object() && encoded.size() == 23, "encoding has exactly twenty-three fields");
        Check(encoded.at("enabled").is_boolean(), "enabled encodes as JSON boolean");
        Check(encoded.at("ownsKakarikoCottage").is_boolean(), "ownership encodes as JSON boolean");
        Check(encoded.at("schemaVersion") == 5, "encoding uses schema version five");
        Check(encoded.at("marketRestored").is_boolean(), "restoration encodes as a JSON boolean");
        Check(encoded.at("wardrobe").is_object() && encoded.at("wardrobe").size() == 2,
              "wardrobe encodes as its two-field object");
        Check(encoded.at("stewardship").is_object() && encoded.at("stewardship").size() == 2 &&
                  encoded.at("stewardship").at("treasury").is_array() &&
                  encoded.at("stewardship").at("treasury").size() == 8,
              "stewardship encodes as an object with eight treasury entries");
        Check(encoded.at("zoraRestored").is_boolean() && encoded.at("castleEstateOwned").is_boolean(),
              "recovery ownership and restoration encode as JSON booleans");
        Check(encoded.at("givenGifts").is_array() && encoded.at("givenGifts").size() == 23,
              "gifts encode as exactly twenty-three entries");
        Check(encoded.at("royalRecognition").is_number_unsigned(), "recognition encodes as an unsigned mask");
        EconomyState decoded{};
        Check(DecodeEconomy(json::parse(encoded.dump()), decoded), "serialized round trip succeeds");
        Check(Equal(state, decoded), "round trip preserves every field, including full-width rent total");
    }

    json extended = EncodeEconomy(Holdings());
    extended["futureOptionalField"] = { { "anything", json::array({ 1, false, "value" }) } };
    EconomyState decoded{};
    Check(DecodeEconomy(extended, decoded), "current schema accepts additional fields");
    Check(Equal(decoded, Holdings()), "additional fields leave known state intact");

    json signedIntegers = EncodeEconomy(Holdings());
    signedIntegers["schemaVersion"] = int64_t{ 1 };
    signedIntegers["bankRupees"] = int64_t{ 100 };
    signedIntegers["rentalFrames"] = int64_t{ 1 };
    signedIntegers["totalRentEarned"] = int64_t{ 0 };
    Check(DecodeEconomy(signedIntegers, decoded), "nonnegative signed JSON integers are accepted");
    Check(decoded.totalRentEarned == 0, "signed zero is preserved");
}

void TestMalformedData() {
    for (const json& malformed : { json(), json::array(), json(42), json("state"), json(false) }) {
        Reject(malformed, "non-object payload");
    }

    const json valid = EncodeEconomy(Holdings());
    for (const char* name : { "schemaVersion",     "enabled",
                              "bankRupees",        "ownsKakarikoCottage",
                              "rentalFrames",      "totalRentEarned",
                              "ownedProperties",   "repairedProperties",
                              "businessFrames",    "totalBusinessEarned",
                              "rapport",           "metResidents",
                              "completedFavors",   "activeFavor",
                              "cottageRentPolicy", "currentPeriodPolicy",
                              "marketRestored",    "wardrobe",
                              "stewardship",       "zoraRestored",
                              "castleEstateOwned", "givenGifts",
                              "royalRecognition" }) {
        json missing = valid;
        missing.erase(name);
        Reject(missing, std::string("missing ") + name);
    }

    for (const char* name :
         { "enabled", "ownsKakarikoCottage", "marketRestored", "zoraRestored", "castleEstateOwned" }) {
        for (const json& wrongType :
             { json(0), json(1), json(-1), json(1.0), json("true"), json(), json::array(), json::object() }) {
            json malformed = valid;
            malformed[name] = wrongType;
            Reject(malformed, std::string(name) + " with non-boolean type " + wrongType.dump());
        }
    }

    for (const char* name : { "schemaVersion", "bankRupees", "rentalFrames", "totalRentEarned", "ownedProperties",
                              "repairedProperties", "totalBusinessEarned", "metResidents", "completedFavors",
                              "activeFavor", "cottageRentPolicy", "currentPeriodPolicy", "royalRecognition" }) {
        for (const json& badNumber : { json(-1), json((std::numeric_limits<int64_t>::min)()), json(0.0), json(1.0),
                                       json(1.5), json("1"), json(true), json(), json::array(), json::object(),
                                       json::parse("18446744073709551616"), json::parse("-18446744073709551616") }) {
            json malformed = valid;
            malformed[name] = badNumber;
            Reject(malformed, std::string(name) + " with invalid integer " + badNumber.dump());
        }
    }

    for (const uint64_t version : { uint64_t{ 0 }, uint64_t{ 6 }, (std::numeric_limits<uint64_t>::max)() }) {
        json unsupported = valid;
        unsupported["schemaVersion"] = version;
        Reject(unsupported, "unsupported schema version " + std::to_string(version));
    }

    json overflow = valid;
    overflow["rentalFrames"] = uint64_t{ (std::numeric_limits<uint32_t>::max)() } + 1;
    Reject(overflow, "rental frames exceeding uint32");
    overflow["rentalFrames"] = (std::numeric_limits<uint64_t>::max)();
    Reject(overflow, "rental frames at uint64 maximum");

    json invalidState = valid;
    invalidState["bankRupees"] = (std::numeric_limits<uint64_t>::max)();
    Reject(invalidState, "bank balance exceeding economy limit");
    invalidState = valid;
    invalidState["rentalFrames"] = (std::numeric_limits<uint32_t>::max)();
    Reject(invalidState, "rental timer exceeding economy limit");

    EconomyState invalid = Holdings();
    invalid.enabled = 2;
    bool threw = false;
    try {
        (void)EncodeEconomy(invalid);
    } catch (const std::invalid_argument&) { threw = true; }
    Check(threw, "encoding rejects invalid read-only sentinel instead of coercing it to true");
}

void TestSocialPersistenceAndMigration() {
    const auto portfolio = SocialHoldings(false);
    const char* socialKeys[] = { "rapport",           "metResidents",        "completedFavors", "activeFavor",
                                 "cottageRentPolicy", "currentPeriodPolicy", "marketRestored" };
    for (int version : { 1, 2 }) {
        auto legacy = EncodeEconomy(portfolio);
        legacy["schemaVersion"] = version;
        for (const char* key : socialKeys)
            legacy.erase(key);
        auto expected = portfolio;
        std::fill(std::begin(expected.rapport), std::end(expected.rapport), int8_t{ 0 });
        expected.metResidents = 0;
        expected.completedFavors = 0;
        expected.activeFavor = 0;
        expected.cottageRentPolicy = 0;
        expected.currentPeriodPolicy = 0;
        expected.marketRestored = 0;
        if (version == 1) {
            for (const char* key : { "ownedProperties", "repairedProperties", "businessFrames", "totalBusinessEarned" })
                legacy.erase(key);
            expected = Holdings(false);
        }
        auto output = SocialHoldings();
        Check(DecodeEconomy(json::parse(legacy.dump()), output), "older schema migrates to social format");
        Check(Equal(output, expected), "migration preserves assets and disables every newly added feature");
        Check(EncodeEconomy(output)["schemaVersion"] == 5, "migrated output writes schema five");
    }
    const auto valid = EncodeEconomy(portfolio);
    for (const json& bad : { json(), json::object(), json(23), json::array(), json::array({ 0 }),
                             json(std::vector<int>(22, 0)), json(std::vector<int>(24, 0)) }) {
        auto malformed = valid;
        malformed["rapport"] = bad;
        Reject(malformed, "incorrect rapport array shape");
    }
    for (const json& bad : { json(-101), json(101), json(-128), json(127), json(255), json(UINT64_MAX), json(INT64_MIN),
                             json(0.0), json(1.5), json("10"), json(true), json(), json::array(), json::object(),
                             json::parse("18446744073709551616") }) {
        auto malformed = valid;
        malformed["rapport"][22] = bad;
        Reject(malformed, "invalid rapport element " + bad.dump());
    }
    for (const char* key : { "cottageRentPolicy", "currentPeriodPolicy" }) {
        for (const uint64_t bad : { uint64_t{ 2 }, uint64_t{ 256 }, UINT64_MAX }) {
            auto malformed = valid;
            malformed[key] = bad;
            Reject(malformed, "rent policy out of bounds");
        }
    }
    for (const char* key : { "metResidents", "completedFavors", "activeFavor" }) {
        auto malformed = valid;
        malformed[key] = UINT64_MAX;
        Reject(malformed, "integer destination overflow");
    }
    auto malformed = valid;
    malformed["metResidents"] = 1u << 23;
    Reject(malformed, "unknown resident bit");
    malformed = valid;
    malformed["completedFavors"] = 1u << 10;
    Reject(malformed, "unknown completed favor bit");
    malformed = valid;
    malformed["activeFavor"] = 11;
    Reject(malformed, "unknown active favor");
    malformed = valid;
    malformed["activeFavor"] = 1;
    Reject(malformed, "favor both active and already completed");
    auto invalid = portfolio;
    invalid.rapport[22] = 101;
    bool threw = false;
    try {
        (void)EncodeEconomy(invalid);
    } catch (const std::invalid_argument&) { threw = true; }
    Check(threw, "social invalid state cannot be encoded");
}

void TestRegionalMigrationAndSnapshots() {
    const auto original = RegionalHoldings(false);
    for (int version : { 1, 2, 3 }) {
        auto legacy = EncodeEconomy(original);
        legacy["schemaVersion"] = version;
        legacy.erase("wardrobe");
        legacy.erase("stewardship");
        auto expected = original;
        expected.wardrobe = {};
        expected.stewardship = {};
        if (version < 3) {
            for (const char* key : { "rapport", "metResidents", "completedFavors", "activeFavor", "cottageRentPolicy",
                                     "currentPeriodPolicy", "marketRestored" })
                legacy.erase(key);
            std::fill(std::begin(expected.rapport), std::end(expected.rapport), int8_t{ 0 });
            expected.metResidents = 0;
            expected.completedFavors = 0;
            expected.activeFavor = 0;
            expected.cottageRentPolicy = 0;
            expected.currentPeriodPolicy = 0;
            expected.marketRestored = 0;
        }
        if (version == 1) {
            for (const char* key : { "ownedProperties", "repairedProperties", "businessFrames", "totalBusinessEarned" })
                legacy.erase(key);
            expected = Holdings(false);
        }
        auto output = RegionalHoldings();
        Check(DecodeEconomy(json::parse(legacy.dump()), output),
              "schema " + std::to_string(version) + " migrates to five");
        Check(Equal(output, expected), "migration retains every previous field and clears only added modules");
        Check(EncodeEconomy(output)["schemaVersion"] == 5, "migrated regional snapshot writes schema five");

        // Unknown optional fields in older schemas are never misinterpreted as
        // a later schema's authoritative module, even if their shape differs.
        legacy["wardrobe"] = "an unrelated old extension";
        legacy["stewardship"] = true;
        output = RegionalHoldings();
        Check(DecodeEconomy(legacy, output) && Equal(output, expected), "legacy extensions do not invent new assets");
    }
    for (uint8_t requested = 0; requested < 2; ++requested) {
        for (uint8_t locked = 0; locked < 2; ++locked) {
            auto expected = SocialHoldings(false);
            expected.cottageRentPolicy = requested;
            expected.currentPeriodPolicy = locked;
            auto legacy = EncodeEconomy(expected);
            legacy["schemaVersion"] = 3;
            legacy.erase("wardrobe");
            legacy.erase("stewardship");
            auto output = RegionalHoldings();
            Check(DecodeEconomy(legacy, output) && Equal(output, expected),
                  "schema three preserves separately requested and locked rent terms");
        }
    }

    auto live = RegionalHoldings();
    const auto snapshot = live;
    live.bankRupees += 55;
    live.wardrobe.ownedStyles = 1;
    live.wardrobe.equippedStyle = 1;
    live.stewardship.treasury[7] = 0;
    live.stewardship.charterMask &= 0x7f;
    EconomyState loaded{};
    Check(DecodeEconomy(json::parse(EncodeEconomy(snapshot).dump()), loaded), "copied regional snapshot decodes");
    Check(Equal(loaded, snapshot) && !Equal(loaded, live), "later live mutations cannot change copied save assets");
    const EconomyState emptyFile{};
    Check(DecodeEconomy(EncodeEconomy(emptyFile), loaded) && Equal(loaded, emptyFile),
          "new slot has no inherited assets");

    auto extended = EncodeEconomy(original);
    extended["wardrobe"]["futureOptional"] = json::array({ true, "color" });
    extended["stewardship"]["futureOptional"] = { { "note", "retained by future schema" } };
    Check(DecodeEconomy(extended, loaded) && Equal(loaded, original),
          "current nested objects accept unknown optional keys");

    auto signedValues = EncodeEconomy(original);
    signedValues["wardrobe"]["ownedStyles"] = int64_t{ 255 };
    signedValues["wardrobe"]["equippedStyle"] = int64_t{ 8 };
    signedValues["stewardship"]["charterMask"] = int64_t{ 255 };
    signedValues["stewardship"]["treasury"][7] = static_cast<int64_t>(LivingHyrule::kTreasuryLimit);
    Check(DecodeEconomy(signedValues, loaded) && Equal(loaded, original),
          "bounded nonnegative signed module integers decode");
}

void TestRegionalMalformedData() {
    const auto valid = EncodeEconomy(RegionalHoldings());
    for (const char* key : { "wardrobe", "stewardship" }) {
        for (const json& bad :
             { json(), json::array(), json::object(), json(1), json(1.0), json(true), json("object") }) {
            auto malformed = valid;
            malformed[key] = bad;
            Reject(malformed, std::string(key) + " malformed object");
        }
    }
    for (const auto& field : std::vector<std::pair<const char*, const char*>>{ { "wardrobe", "ownedStyles" },
                                                                               { "wardrobe", "equippedStyle" },
                                                                               { "stewardship", "charterMask" },
                                                                               { "stewardship", "treasury" } }) {
        auto malformed = valid;
        malformed[field.first].erase(field.second);
        Reject(malformed, std::string("missing nested ") + field.first + "." + field.second);
    }
    for (const char* path : { "/wardrobe/ownedStyles", "/wardrobe/equippedStyle", "/stewardship/charterMask",
                              "/stewardship/treasury/0", "/stewardship/treasury/7" }) {
        for (const json& bad :
             { json(-1), json(INT64_MIN), json(0.0), json(1.0), json(1.5), json("1"), json(true), json(), json::array(),
               json::object(), json::parse("18446744073709551616"), json::parse("-18446744073709551616") }) {
            auto malformed = valid;
            malformed[json::json_pointer(path)] = bad;
            Reject(malformed, std::string("invalid nested integer ") + path + "=" + bad.dump());
        }
    }
    for (const json& bad : { json(), json::object(), json(true), json("treasury"), json(8), json::array(),
                             json(std::vector<uint64_t>(7, 0)), json(std::vector<uint64_t>(9, 0)) }) {
        auto malformed = valid;
        malformed["stewardship"]["treasury"] = bad;
        Reject(malformed, "treasury requires exactly eight integer entries");
    }
    for (uint8_t index = 0; index < LivingHyrule::kStewardshipRegionCount; ++index) {
        for (uint64_t bad : { LivingHyrule::kTreasuryLimit + 1, UINT64_MAX }) {
            auto malformed = valid;
            malformed["stewardship"]["treasury"][index] = bad;
            Reject(malformed, "treasury element exceeds regional limit");
        }
        auto malformed = valid;
        malformed["stewardship"]["charterMask"] = 0xffu & ~(1u << index);
        malformed["stewardship"]["treasury"][index] = 1;
        Reject(malformed, "treasury funds require their regional charter");
    }
    for (const auto& field :
         std::vector<std::pair<const char*, uint64_t>>{ { "/wardrobe/ownedStyles", 256 },
                                                        { "/wardrobe/ownedStyles", 65536 },
                                                        { "/wardrobe/equippedStyle", 9 },
                                                        { "/wardrobe/equippedStyle", 256 },
                                                        { "/stewardship/charterMask", 256 },
                                                        { "/wardrobe/ownedStyles", UINT64_MAX },
                                                        { "/wardrobe/equippedStyle", UINT64_MAX },
                                                        { "/stewardship/charterMask", UINT64_MAX } }) {
        auto malformed = valid;
        malformed[json::json_pointer(field.first)] = field.second;
        Reject(malformed, "module integer exceeds storage or semantic limit");
    }
    auto malformed = valid;
    malformed["wardrobe"]["ownedStyles"] = 1;
    Reject(malformed, "equipped dye must be owned");

    for (int module = 0; module < 2; ++module) {
        auto invalid = RegionalHoldings();
        if (module == 0)
            invalid.wardrobe.equippedStyle = 9;
        else
            invalid.stewardship.treasury[7] = LivingHyrule::kTreasuryLimit + 1;
        bool threw = false;
        try {
            (void)EncodeEconomy(invalid);
        } catch (const std::invalid_argument&) { threw = true; }
        Check(threw, "encoding refuses invalid nested assets");
    }
}

void TestRecoveryMigrationAndSnapshots() {
    for (bool enabled : { false, true }) {
        for (int version : { 1, 2, 3, 4 }) {
            auto legacy = EncodeEconomy(RecoveryHoldings(enabled));
            legacy["schemaVersion"] = version;
            auto expected = RegionalHoldings(enabled);
            for (const char* key : { "zoraRestored", "castleEstateOwned", "givenGifts", "royalRecognition" })
                legacy.erase(key);
            if (version < 4) {
                legacy.erase("wardrobe");
                legacy.erase("stewardship");
                expected.wardrobe = {};
                expected.stewardship = {};
            }
            if (version < 3) {
                for (const char* key : { "rapport", "metResidents", "completedFavors", "activeFavor",
                                         "cottageRentPolicy", "currentPeriodPolicy", "marketRestored" })
                    legacy.erase(key);
                std::fill(std::begin(expected.rapport), std::end(expected.rapport), int8_t{ 0 });
                expected.metResidents = 0;
                expected.completedFavors = 0;
                expected.activeFavor = 0;
                expected.cottageRentPolicy = 0;
                expected.currentPeriodPolicy = 0;
                expected.marketRestored = 0;
            }
            if (version == 1) {
                for (const char* key :
                     { "ownedProperties", "repairedProperties", "businessFrames", "totalBusinessEarned" })
                    legacy.erase(key);
                expected = Holdings(enabled);
            }
            auto output = RecoveryHoldings(!enabled);
            Check(DecodeEconomy(json::parse(legacy.dump()), output) && Equal(output, expected),
                  "schema " + std::to_string(version) + " retains all prior fields and resets new recovery fields");
            Check(EncodeEconomy(output)["schemaVersion"] == 5, "legacy recovery migration writes schema five");

            legacy["zoraRestored"] = "older optional text";
            legacy["castleEstateOwned"] = 999;
            legacy["givenGifts"] = json::object();
            legacy["royalRecognition"] = true;
            output = RecoveryHoldings(!enabled);
            Check(DecodeEconomy(legacy, output) && Equal(output, expected),
                  "prior optional extensions do not create gifts, recognition or restored property");
        }
    }
    for (uint8_t requested = 0; requested < 2; ++requested) {
        for (uint8_t locked = 0; locked < 2; ++locked) {
            auto expected = RegionalHoldings(false);
            expected.cottageRentPolicy = requested;
            expected.currentPeriodPolicy = locked;
            auto legacy = EncodeEconomy(expected);
            legacy["schemaVersion"] = 4;
            for (const char* key : { "zoraRestored", "castleEstateOwned", "givenGifts", "royalRecognition" })
                legacy.erase(key);
            auto output = RecoveryHoldings();
            Check(DecodeEconomy(legacy, output) && Equal(output, expected),
                  "schema four retains both independently locked rent policies and regional assets");
        }
    }

    auto live = RecoveryHoldings();
    const auto snapshot = live;
    live.bankRupees += 75;
    live.zoraRestored = 0;
    live.castleEstateOwned = 0;
    std::fill(std::begin(live.givenGifts), std::end(live.givenGifts), uint8_t{ 0 });
    live.royalRecognition = 0;
    EconomyState loaded{};
    Check(DecodeEconomy(json::parse(EncodeEconomy(snapshot).dump()), loaded), "copied recovery snapshot decodes");
    Check(Equal(loaded, snapshot) && !Equal(loaded, live), "recovery snapshot is independent of later live changes");
    Check(DecodeEconomy(EncodeEconomy(EconomyState{}), loaded) && Equal(loaded, EconomyState{}),
          "loading a fresh slot clears every prior recovery and gift field");

    auto signedValues = EncodeEconomy(snapshot);
    for (uint32_t resident = 0; resident < LivingHyrule::kSocialResidentCount; ++resident)
        signedValues["givenGifts"][resident] = static_cast<int64_t>(snapshot.givenGifts[resident]);
    signedValues["royalRecognition"] = int64_t{ 255 };
    Check(DecodeEconomy(signedValues, loaded) && Equal(loaded, snapshot),
          "bounded signed integers retain gifts and all eight recognition bits");
}

void TestRecoveryMalformedData() {
    const auto valid = EncodeEconomy(RecoveryHoldings());
    for (const json& bad : { json(), json::object(), json(true), json("gifts"), json(23), json::array(),
                             json(std::vector<uint8_t>(22, 0)), json(std::vector<uint8_t>(24, 0)) }) {
        auto malformed = valid;
        malformed["givenGifts"] = bad;
        Reject(malformed, "gifts require exactly twenty-three integer entries");
    }
    for (uint32_t resident = 0; resident < LivingHyrule::kSocialResidentCount; ++resident) {
        for (const json& bad :
             { json(-1), json(INT64_MIN), json(8), json(255), json(256), json(UINT64_MAX), json(0.0), json(1.5),
               json("1"), json(true), json(), json::array(), json::object(), json::parse("18446744073709551616") }) {
            auto malformed = valid;
            malformed["givenGifts"][resident] = bad;
            Reject(malformed, "invalid gift mask for resident " + std::to_string(resident) + "=" + bad.dump());
        }
        auto malformed = valid;
        malformed["metResidents"] = LivingHyrule::kMetResidentsMask & ~(1u << resident);
        Reject(malformed, "gift cannot precede meeting its resident");
    }
    for (uint64_t bad : { uint64_t{ 256 }, UINT64_MAX }) {
        auto malformed = valid;
        malformed["royalRecognition"] = bad;
        Reject(malformed, "recognition exceeds its eight-bit storage");
    }
    for (uint8_t giftMask = 0; giftMask <= LivingHyrule::kGiftMask; ++giftMask) {
        auto state = RecoveryHoldings();
        std::fill(std::begin(state.givenGifts), std::end(state.givenGifts), giftMask);
        EconomyState loaded{};
        Check(DecodeEconomy(EncodeEconomy(state), loaded) && Equal(loaded, state),
              "all combinations of the three finite gift bits survive serialization");
    }
    for (uint16_t recognition = 0; recognition <= 0xff; ++recognition) {
        auto state = RecoveryHoldings();
        state.royalRecognition = static_cast<uint8_t>(recognition);
        EconomyState loaded{};
        Check(DecodeEconomy(EncodeEconomy(state), loaded) && Equal(loaded, state),
              "every valid combination of eight recognition bits survives serialization");
    }
    for (int scenario = 0; scenario < 9; ++scenario) {
        auto invalid = RegionalHoldings();
        switch (scenario) {
            case 0:
                invalid.zoraRestored = 2;
                break;
            case 1:
                invalid.castleEstateOwned = 2;
                break;
            case 2:
                invalid.givenGifts[22] = 8;
                break;
            case 3:
                invalid.givenGifts[22] = 1;
                invalid.metResidents &= ~(1u << 22);
                break;
            case 4:
                invalid.royalRecognition = 1u << 5;
                invalid.marketRestored = 0;
                break;
            case 5:
                invalid.royalRecognition = 1u << 6;
                break;
            case 6:
                invalid.royalRecognition = 1u << 7;
                invalid.stewardship.charterMask = 0x7f;
                invalid.stewardship.treasury[7] = 0;
                break;
            case 7:
                invalid.castleEstateOwned = 1;
                invalid.marketRestored = 0;
                break;
            case 8:
                invalid.castleEstateOwned = 1;
                invalid.stewardship.charterMask = 0x7f;
                invalid.stewardship.treasury[7] = 0;
                break;
        }
        Check(!LivingHyrule::IsValidState(invalid), "invalid recovery prerequisite rejects the entire economy");
        bool threw = false;
        try {
            (void)EncodeEconomy(invalid);
        } catch (const std::invalid_argument&) { threw = true; }
        Check(threw, "encoding refuses invalid recovery state");

        auto malformed = EncodeEconomy(RegionalHoldings());
        malformed["zoraRestored"] = invalid.zoraRestored != 0;
        malformed["castleEstateOwned"] = invalid.castleEstateOwned != 0;
        malformed["givenGifts"] = invalid.givenGifts;
        malformed["royalRecognition"] = invalid.royalRecognition;
        malformed["metResidents"] = invalid.metResidents;
        malformed["marketRestored"] = invalid.marketRestored != 0;
        malformed["stewardship"]["charterMask"] = invalid.stewardship.charterMask;
        malformed["stewardship"]["treasury"] = invalid.stewardship.treasury;
        if (scenario >= 2)
            Reject(malformed, "recovery prerequisite fails atomically");
    }

    auto future = valid;
    future["schemaVersion"] = 6;
    future["futureRecovery"] = { { "title", "must remain untouched" }, { "value", UINT64_MAX } };
    Reject(future, "unknown future recovery schema");
    auto readOnly = RecoveryHoldings();
    readOnly.enabled = 2;
    const auto original = readOnly;
    Check(!DecodeEconomy(future, readOnly) && Equal(readOnly, original),
          "future data cannot alter an existing read-only sentinel or its held assets");
}

} // namespace

int main() {
    try {
        TestRoundTrips();
        TestMalformedData();
        TestSocialPersistenceAndMigration();
        TestRegionalMigrationAndSnapshots();
        TestRegionalMalformedData();
        TestRecoveryMigrationAndSnapshots();
        TestRecoveryMalformedData();
        auto legacy = EncodeEconomy(Holdings());
        legacy["schemaVersion"] = 1;
        for (const char* key : { "ownedProperties", "repairedProperties", "businessFrames", "totalBusinessEarned" })
            legacy.erase(key);
        EconomyState migrated{};
        Check(DecodeEconomy(legacy, migrated), "legacy economy migrates");
        Check(Equal(migrated, Holdings()), "legacy cottage, bank and timer survive migration");
        auto portfolio = Holdings();
        portfolio.ownedProperties = 0xffff;
        portfolio.repairedProperties = 0xaaaa;
        portfolio.totalBusinessEarned = UINT64_MAX;
        for (unsigned int i = 0; i < 16; ++i)
            portfolio.businessFrames[i] = i * 600;
        EconomyState restored{};
        Check(DecodeEconomy(json::parse(EncodeEconomy(portfolio).dump()), restored), "portfolio decodes");
        Check(Equal(portfolio, restored), "every portfolio field survives snapshot serialization");
        for (const char* key : { "ownedProperties", "repairedProperties", "businessFrames", "totalBusinessEarned" }) {
            auto missing = EncodeEconomy(portfolio);
            missing.erase(key);
            Reject(missing, key);
        }
        for (const json& invalid : { json::array(), json::array({ 0 }), json::object(), json(42) }) {
            auto malformed = EncodeEconomy(portfolio);
            malformed["businessFrames"] = invalid;
            Reject(malformed, "invalid timers");
        }
        auto malformed = EncodeEconomy(portfolio);
        malformed["businessFrames"][15] = 12000;
        Reject(malformed, "out of range timer");
        malformed = EncodeEconomy(portfolio);
        malformed["ownedProperties"] = 0x10000;
        Reject(malformed, "unknown property bit");
        malformed = EncodeEconomy(Holdings());
        malformed["repairedProperties"] = 1;
        Reject(malformed, "repair without ownership");
        malformed = EncodeEconomy(Holdings());
        malformed["businessFrames"][0] = 1;
        Reject(malformed, "income timer without ownership");
    } catch (const std::exception& error) {
        std::cerr << "Unexpected test exception: " << error.what() << '\n';
        return 1;
    }
    if (failures != 0) {
        std::cerr << failures << " save codec checks failed\n";
        return 1;
    }
    std::cout << "Living Hyrule save codec tests passed\n";
    return 0;
}
