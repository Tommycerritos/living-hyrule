#include "soh/Enhancements/living-hyrule/SaveCodec.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

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
           left.totalRentEarned == right.totalRentEarned;
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

void Reject(const json& data, const std::string& description) {
    EconomyState output = Holdings(false);
    const EconomyState original = output;
    Check(!DecodeEconomy(data, output), description + " is rejected");
    Check(Equal(output, original), description + " leaves output unchanged");
}

void TestRoundTrips() {
    for (const EconomyState state : { EconomyState{}, Holdings(), Holdings(false) }) {
        Check(LivingHyrule::IsValidState(state), "round-trip fixture is valid");
        const json encoded = EncodeEconomy(state);
        Check(encoded.is_object() && encoded.size() == 6, "encoding has exactly six fields");
        Check(encoded.at("enabled").is_boolean(), "enabled encodes as JSON boolean");
        Check(encoded.at("ownsKakarikoCottage").is_boolean(), "ownership encodes as JSON boolean");
        Check(encoded.at("schemaVersion") == 1, "encoding uses schema version one");
        EconomyState decoded{};
        Check(DecodeEconomy(json::parse(encoded.dump()), decoded), "serialized round trip succeeds");
        Check(Equal(state, decoded), "round trip preserves every field, including full-width rent total");
    }

    json extended = EncodeEconomy(Holdings());
    extended["futureOptionalField"] = { { "anything", json::array({ 1, false, "value" }) } };
    EconomyState decoded{};
    Check(DecodeEconomy(extended, decoded), "schema one accepts additional fields");
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
         { "schemaVersion", "enabled", "bankRupees", "ownsKakarikoCottage", "rentalFrames", "totalRentEarned" }) {
        json missing = valid;
        missing.erase(name);
        Reject(missing, std::string("missing ") + name);
    }

    for (const char* name : { "enabled", "ownsKakarikoCottage" }) {
        for (const json& wrongType :
             { json(0), json(1), json(-1), json(1.0), json("true"), json(), json::array(), json::object() }) {
            json malformed = valid;
            malformed[name] = wrongType;
            Reject(malformed, std::string(name) + " with non-boolean type " + wrongType.dump());
        }
    }

    for (const char* name : { "schemaVersion", "bankRupees", "rentalFrames", "totalRentEarned" }) {
        for (const json& badNumber : { json(-1), json((std::numeric_limits<int64_t>::min)()), json(0.0), json(1.0),
                                       json(1.5), json("1"), json(true), json(), json::array(), json::object(),
                                       json::parse("18446744073709551616"), json::parse("-18446744073709551616") }) {
            json malformed = valid;
            malformed[name] = badNumber;
            Reject(malformed, std::string(name) + " with invalid integer " + badNumber.dump());
        }
    }

    for (const uint64_t version : { uint64_t{ 0 }, uint64_t{ 2 }, (std::numeric_limits<uint64_t>::max)() }) {
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

} // namespace

int main() {
    try {
        TestRoundTrips();
        TestMalformedData();
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
