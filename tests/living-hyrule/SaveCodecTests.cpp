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
           left.currentPeriodPolicy == right.currentPeriodPolicy && left.marketRestored == right.marketRestored;
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

void Reject(const json& data, const std::string& description) {
    EconomyState output = SocialHoldings(false);
    const EconomyState original = output;
    Check(!DecodeEconomy(data, output), description + " is rejected");
    Check(Equal(output, original), description + " leaves output unchanged");
}

void TestRoundTrips() {
    for (const EconomyState state :
         { EconomyState{}, Holdings(), Holdings(false), SocialHoldings(), SocialHoldings(false) }) {
        Check(LivingHyrule::IsValidState(state), "round-trip fixture is valid");
        const json encoded = EncodeEconomy(state);
        Check(encoded.is_object() && encoded.size() == 17, "encoding has exactly seventeen fields");
        Check(encoded.at("enabled").is_boolean(), "enabled encodes as JSON boolean");
        Check(encoded.at("ownsKakarikoCottage").is_boolean(), "ownership encodes as JSON boolean");
        Check(encoded.at("schemaVersion") == 3, "encoding uses schema version three");
        Check(encoded.at("marketRestored").is_boolean(), "restoration encodes as a JSON boolean");
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
    for (const char* name :
         { "schemaVersion", "enabled", "bankRupees", "ownsKakarikoCottage", "rentalFrames", "totalRentEarned",
           "ownedProperties", "repairedProperties", "businessFrames", "totalBusinessEarned", "rapport", "metResidents",
           "completedFavors", "activeFavor", "cottageRentPolicy", "currentPeriodPolicy", "marketRestored" }) {
        json missing = valid;
        missing.erase(name);
        Reject(missing, std::string("missing ") + name);
    }

    for (const char* name : { "enabled", "ownsKakarikoCottage", "marketRestored" }) {
        for (const json& wrongType :
             { json(0), json(1), json(-1), json(1.0), json("true"), json(), json::array(), json::object() }) {
            json malformed = valid;
            malformed[name] = wrongType;
            Reject(malformed, std::string(name) + " with non-boolean type " + wrongType.dump());
        }
    }

    for (const char* name : { "schemaVersion", "bankRupees", "rentalFrames", "totalRentEarned", "ownedProperties",
                              "repairedProperties", "totalBusinessEarned", "metResidents", "completedFavors",
                              "activeFavor", "cottageRentPolicy", "currentPeriodPolicy" }) {
        for (const json& badNumber : { json(-1), json((std::numeric_limits<int64_t>::min)()), json(0.0), json(1.0),
                                       json(1.5), json("1"), json(true), json(), json::array(), json::object(),
                                       json::parse("18446744073709551616"), json::parse("-18446744073709551616") }) {
            json malformed = valid;
            malformed[name] = badNumber;
            Reject(malformed, std::string(name) + " with invalid integer " + badNumber.dump());
        }
    }

    for (const uint64_t version : { uint64_t{ 0 }, uint64_t{ 4 }, (std::numeric_limits<uint64_t>::max)() }) {
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
        Check(EncodeEconomy(output)["schemaVersion"] == 3, "migrated output writes schema three");
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

} // namespace

int main() {
    try {
        TestRoundTrips();
        TestMalformedData();
        TestSocialPersistenceAndMigration();
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
