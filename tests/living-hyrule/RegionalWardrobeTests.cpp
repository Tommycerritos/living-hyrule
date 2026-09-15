#include "RegionalWardrobePolicy.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>

namespace {
using namespace LivingHyrule;
int failures = 0;
int checks = 0;
void Check(bool value, const char* expression, int line) {
    ++checks;
    if (!value) {
        std::cerr << "Line " << line << ": " << expression << '\n';
        ++failures;
    }
}
#define CHECK(x) Check((x), #x, __LINE__)

EconomyState Account(uint64_t bank = 0) {
    EconomyState state{};
    state.enabled = 1;
    state.bankRupees = bank;
    state.ownsKakarikoCottage = 1;
    state.rentalFrames = 241;
    state.totalRentEarned = UINT64_MAX;
    state.ownedProperties = 0xffff;
    state.repairedProperties = 0xaaaa;
    state.totalBusinessEarned = UINT64_MAX;
    for (uint32_t i = 0; i < 16; ++i)
        state.businessFrames[i] = i * 300;
    return state;
}
bool SameWardrobe(const WardrobeState& a, const WardrobeState& b) {
    return a.ownedStyles == b.ownedStyles && a.equippedStyle == b.equippedStyle;
}
template <typename Operation>
void Reject(EconomyState& economy, WardrobeState& wardrobe, Result result, Operation operation) {
    // Snapshot the same object's representation, rather than comparing the
    // unspecified padding of two separately constructed C-compatible structs.
    std::array<unsigned char, sizeof(EconomyState)> before{};
    std::memcpy(before.data(), &economy, before.size());
    const auto wardrobeBefore = wardrobe;
    CHECK(operation() == result);
    CHECK(std::memcmp(before.data(), &economy, before.size()) == 0);
    CHECK(SameWardrobe(wardrobe, wardrobeBefore));
}
const WorldProgress recovered{ true, true, true, true, true, true, true, true, true };

void TestCatalogueAndValidity() {
    static_assert(kRegionalStyleCount == 8 && kRegionalStyleMask == 0xff);
    static_assert(static_cast<uint8_t>(StyleId::Forest) == 1 && static_cast<uint8_t>(StyleId::Market) == 8);
    CHECK(IsValidWardrobeState({}));
    CHECK(OwnsRegionalStyle({}, 0));
    uint16_t regions = 0;
    constexpr WardrobeColor vanilla[] = { { 30, 105, 27 }, { 100, 20, 0 }, { 0, 60, 100 } };
    for (uint8_t id = 1; id <= kRegionalStyleCount; ++id) {
        const auto* style = GetRegionalStyle(id);
        CHECK(style != nullptr && static_cast<uint8_t>(style->id) == id);
        CHECK(style->name[0] != '\0' && style->description[0] != '\0');
        CHECK(style->price > 0 && style->price <= kBankLimit);
        CHECK(style->region < Region::Count);
        CHECK((regions & (1u << static_cast<unsigned int>(style->region))) == 0);
        regions |= static_cast<uint16_t>(1u << static_cast<unsigned int>(style->region));
        CHECK(RegionalStyleBit(id) == (1u << (id - 1)));
        CHECK(!OwnsRegionalStyle({}, id));
        for (const auto& color : vanilla) {
            const int distance = std::abs(int(style->color.r) - color.r) + std::abs(int(style->color.g) - color.g) +
                                 std::abs(int(style->color.b) - color.b);
            CHECK(distance >= 70); // Every sold dye differs substantially from every native tunic color.
        }
        for (uint8_t other = 1; other < id; ++other) {
            const auto& color = GetRegionalStyle(other)->color;
            CHECK(style->color.r != color.r || style->color.g != color.g || style->color.b != color.b);
        }
    }
    CHECK(regions == 0xff);
    for (uint8_t id : { uint8_t{ 0 }, uint8_t{ 9 }, uint8_t{ 255 } })
        CHECK(GetRegionalStyle(id) == nullptr && RegionalStyleBit(id) == 0);
    CHECK(!IsValidWardrobeState({ 0x100, 0 }));
    CHECK(!IsValidWardrobeState({ 0xffff, 0 }));
    CHECK(!IsValidWardrobeState({ 0, 1 }));
    CHECK(!IsValidWardrobeState({ 0xff, 9 }));
    CHECK(!IsValidWardrobeState({ 0xff, 255 }));
    CHECK(IsValidWardrobeState({ 0xff, 8 }));
}

void TestPurchasesAndStory() {
    const WorldProgress child{};
    WorldProgress crisis{};
    crisis.adult = true;
    for (uint8_t id = 1; id <= kRegionalStyleCount; ++id) {
        const auto& style = *GetRegionalStyle(id);
        auto economy = Account(style.price);
        WardrobeState wardrobe{};
        Reject(economy, wardrobe, Result::NotOwned, [&] { return EquipRegionalStyle(economy, wardrobe, id); });
        for (uint8_t place = 0; place <= static_cast<uint8_t>(Region::Count); ++place) {
            const auto region = static_cast<Region>(place);
            if (region != style.region)
                Reject(economy, wardrobe, Result::Unavailable,
                       [&] { return BuyRegionalStyle(economy, wardrobe, id, region, recovered); });
        }
        Reject(economy, wardrobe, Result::Unavailable,
               [&] { return BuyRegionalStyle(economy, wardrobe, id, style.region, crisis); });
        if (style.region == Region::Desert)
            Reject(economy, wardrobe, Result::Unavailable,
                   [&] { return BuyRegionalStyle(economy, wardrobe, id, style.region, child); });
        --economy.bankRupees;
        Reject(economy, wardrobe, Result::InsufficientBank,
               [&] { return BuyRegionalStyle(economy, wardrobe, id, style.region, recovered); });
        ++economy.bankRupees;
        const auto expectedAccount = Account(0);
        CHECK(BuyRegionalStyle(economy, wardrobe, id, style.region,
                               style.region == Region::Desert ? recovered : child) == Result::Success);
        CHECK(economy.bankRupees == expectedAccount.bankRupees && economy.rentalFrames == expectedAccount.rentalFrames);
        CHECK(economy.ownedProperties == expectedAccount.ownedProperties &&
              economy.repairedProperties == expectedAccount.repairedProperties);
        CHECK(economy.totalRentEarned == UINT64_MAX && economy.totalBusinessEarned == UINT64_MAX);
        CHECK(wardrobe.ownedStyles == RegionalStyleBit(id) && wardrobe.equippedStyle == 0);
        CHECK(EquipRegionalStyle(economy, wardrobe, id) == Result::Success);
        CHECK(wardrobe.equippedStyle == id && economy.bankRupees == 0);
        // No age field or restriction in the wardrobe: the same purchased dye
        // can be equipped after changing ages, away from its original region.
        CHECK(EquipRegionalStyle(economy, wardrobe, 0) == Result::Success);
        CHECK(EquipRegionalStyle(economy, wardrobe, id) == Result::Success);
        Reject(economy, wardrobe, Result::AlreadyOwned,
               [&] { return BuyRegionalStyle(economy, wardrobe, id, style.region, recovered); });
        CHECK(IsValidWardrobeState(wardrobe) && IsValidState(economy));
    }
    for (int flags = 0; flags < 8; ++flags) {
        auto world = child;
        world.adult = (flags & 1) != 0;
        world.spirit = (flags & 2) != 0;
        world.gerudoMembership = (flags & 4) != 0;
        auto economy = Account(1000);
        WardrobeState wardrobe{};
        CHECK((BuyRegionalStyle(economy, wardrobe, 4, Region::Desert, world) == Result::Success) == (flags == 7));
    }
}

void TestInvalidAndDisabled() {
    auto economy = Account(10000);
    WardrobeState wardrobe{ 1, 1 };
    for (uint8_t id : { uint8_t{ 0 }, uint8_t{ 9 }, uint8_t{ 255 } })
        Reject(economy, wardrobe, Result::InvalidAmount,
               [&] { return BuyRegionalStyle(economy, wardrobe, id, Region::Forest, recovered); });
    for (uint8_t id : { uint8_t{ 9 }, uint8_t{ 255 } })
        Reject(economy, wardrobe, Result::InvalidAmount, [&] { return EquipRegionalStyle(economy, wardrobe, id); });
    economy.enabled = 0;
    Reject(economy, wardrobe, Result::Disabled,
           [&] { return BuyRegionalStyle(economy, wardrobe, 2, Region::Mountain, recovered); });
    Reject(economy, wardrobe, Result::Disabled, [&] { return EquipRegionalStyle(economy, wardrobe, 0); });
    economy.enabled = 2;
    Reject(economy, wardrobe, Result::InvalidState,
           [&] { return BuyRegionalStyle(economy, wardrobe, 2, Region::Mountain, recovered); });
    economy = Account(kBankLimit + 1);
    Reject(economy, wardrobe, Result::InvalidState, [&] { return EquipRegionalStyle(economy, wardrobe, 0); });
    economy = Account(10000);
    for (const WardrobeState invalid : { WardrobeState{ 0, 1 }, WardrobeState{ 0x100, 0 }, WardrobeState{ 0xff, 9 } }) {
        wardrobe = invalid;
        Reject(economy, wardrobe, Result::InvalidState,
               [&] { return BuyRegionalStyle(economy, wardrobe, 2, Region::Mountain, recovered); });
        Reject(economy, wardrobe, Result::InvalidState, [&] { return EquipRegionalStyle(economy, wardrobe, 0); });
    }
}

void TestDrawCompatibilityAndSnapshots() {
    WardrobeState wardrobe{ 0xff, 7 };
    const auto snapshot = wardrobe;
    WardrobeVisualContext context{ true, true, true, false, false, false };
    CHECK(EvaluateRegionalWardrobe(&wardrobe, context) == WardrobeVisualStatus::Active);
    CHECK(EvaluateRegionalWardrobe(nullptr, context) == WardrobeVisualStatus::Unavailable);
    for (unsigned int flags = 0; flags < 64; ++flags) {
        const WardrobeVisualContext trial{ (flags & 1) != 0, (flags & 2) != 0,  (flags & 4) != 0,
                                           (flags & 8) != 0, (flags & 16) != 0, (flags & 32) != 0 };
        const auto result = EvaluateRegionalWardrobe(&wardrobe, trial);
        CHECK((result == WardrobeVisualStatus::Active) == (flags == 7));
        CHECK(RegionalWardrobeAppliesColor(result) == (flags == 7 || flags == 23));
        CHECK(SameWardrobe(wardrobe, snapshot)); // Conflicts do not erase ownership/selection.
    }
    auto conflict = context;
    conflict.anchorConnected = true;
    CHECK(EvaluateRegionalWardrobe(&wardrobe, conflict) == WardrobeVisualStatus::NetworkAppearance);
    conflict = context;
    conflict.customModel = true;
    CHECK(EvaluateRegionalWardrobe(&wardrobe, conflict) == WardrobeVisualStatus::CustomModelTint);
    CHECK(RegionalWardrobeAppliesColor(EvaluateRegionalWardrobe(&wardrobe, conflict)));
    conflict.cosmeticOverride = true;
    CHECK(!RegionalWardrobeAppliesColor(EvaluateRegionalWardrobe(&wardrobe, conflict)));
    conflict.cosmeticOverride = false;
    conflict.anchorConnected = true;
    CHECK(!RegionalWardrobeAppliesColor(EvaluateRegionalWardrobe(&wardrobe, conflict)));
    conflict = context;
    conflict.cosmeticOverride = true;
    CHECK(EvaluateRegionalWardrobe(&wardrobe, conflict) == WardrobeVisualStatus::CosmeticOverride);
    wardrobe.equippedStyle = 0;
    CHECK(EvaluateRegionalWardrobe(&wardrobe, context) == WardrobeVisualStatus::Original);
    wardrobe.equippedStyle = 9;
    CHECK(EvaluateRegionalWardrobe(&wardrobe, context) == WardrobeVisualStatus::Unreadable);
    auto firstFile = snapshot;
    auto secondFile = WardrobeState{};
    const auto economy = Account();
    CHECK(EquipRegionalStyle(economy, firstFile, 1) == Result::Success);
    CHECK(firstFile.equippedStyle == 1 && snapshot.equippedStyle == 7 && secondFile.ownedStyles == 0);
    CHECK(EquipRegionalStyle(economy, secondFile, 1) == Result::NotOwned);
    CHECK(SameWardrobe(secondFile, {}));
}

void TestRandomizedConservation() {
    std::mt19937 random(0x44594553u);
    for (unsigned int run = 0; run < 100; ++run) {
        const uint64_t initialBank = random() % 10000;
        auto economy = Account(initialBank);
        WardrobeState wardrobe{};
        uint64_t spent = 0;
        for (unsigned int step = 0; step < 1000; ++step) {
            const uint8_t id = static_cast<uint8_t>(random() % 11);
            if (random() & 1) {
                const auto region = static_cast<Region>(random() % 9);
                const auto* style = GetRegionalStyle(id);
                if (BuyRegionalStyle(economy, wardrobe, id, region, recovered) == Result::Success) {
                    CHECK(style != nullptr);
                    spent += style->price;
                }
            } else {
                const auto bank = economy.bankRupees;
                (void)EquipRegionalStyle(economy, wardrobe, id);
                CHECK(economy.bankRupees == bank);
            }
            CHECK(economy.bankRupees + spent == initialBank);
            CHECK(IsValidState(economy) && IsValidWardrobeState(wardrobe));
        }
    }
}
} // namespace

int main() {
    TestCatalogueAndValidity();
    TestPurchasesAndStory();
    TestInvalidAndDisabled();
    TestDrawCompatibilityAndSnapshots();
    TestRandomizedConservation();
    if (failures != 0) {
        std::cerr << failures << " / " << checks << " regional wardrobe checks failed\n";
        return 1;
    }
    std::cout << checks << " regional wardrobe checks passed, including 100,000 randomized operations\n";
    return 0;
}
