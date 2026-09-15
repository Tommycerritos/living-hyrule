#include "ResidentGiftsPolicy.h"
#include "RoyalProgressionPolicy.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

using namespace LivingHyrule;
namespace {
int failures = 0;
#define CHECK(x)                                              \
    do {                                                      \
        if (!(x)) {                                           \
            std::cerr << "Line " << __LINE__ << ": " #x "\n"; \
            ++failures;                                       \
        }                                                     \
    } while (0)

WorldProgress Recovered() {
    return { true, true, true, true, true, true, true, true, true };
}
EconomyState NewState(uint64_t bank = 1000000) {
    EconomyState state{};
    state.enabled = 1;
    state.bankRupees = bank;
    return state;
}
bool Equal(const EconomyState& a, const EconomyState& b) {
    return a.enabled == b.enabled && a.bankRupees == b.bankRupees && a.ownsKakarikoCottage == b.ownsKakarikoCottage &&
           a.rentalFrames == b.rentalFrames && a.totalRentEarned == b.totalRentEarned &&
           a.ownedProperties == b.ownedProperties && a.repairedProperties == b.repairedProperties &&
           a.totalBusinessEarned == b.totalBusinessEarned &&
           std::equal(std::begin(a.businessFrames), std::end(a.businessFrames), std::begin(b.businessFrames)) &&
           std::equal(std::begin(a.rapport), std::end(a.rapport), std::begin(b.rapport)) &&
           a.metResidents == b.metResidents && a.completedFavors == b.completedFavors &&
           a.activeFavor == b.activeFavor && a.cottageRentPolicy == b.cottageRentPolicy &&
           a.currentPeriodPolicy == b.currentPeriodPolicy && a.marketRestored == b.marketRestored &&
           a.wardrobe.ownedStyles == b.wardrobe.ownedStyles && a.wardrobe.equippedStyle == b.wardrobe.equippedStyle &&
           a.stewardship.charterMask == b.stewardship.charterMask &&
           std::equal(std::begin(a.stewardship.treasury), std::end(a.stewardship.treasury),
                      std::begin(b.stewardship.treasury)) &&
           a.zoraRestored == b.zoraRestored && a.castleEstateOwned == b.castleEstateOwned &&
           a.royalRecognition == b.royalRecognition &&
           std::equal(std::begin(a.givenGifts), std::end(a.givenGifts), std::begin(b.givenGifts));
}
template <typename Op> void Reject(EconomyState& state, Result result, Op operation) {
    auto before = state;
    CHECK(operation() == result);
    CHECK(Equal(state, before));
}

void Gifts() {
    auto world = Recovered();
    for (uint32_t index = 0; index < kSocialResidentCount; ++index) {
        auto state = NewState();
        auto resident = static_cast<ResidentId>(index);
        CHECK(NextResidentGift(state, resident) == kPreferredGifts[index]);
        uint32_t spent = 0;
        for (uint8_t gift = 0; gift < kGiftKinds; ++gift) {
            auto kind = NextResidentGift(state, resident);
            CHECK(ValidGiftKind(kind));
            auto price = kResidentGifts[static_cast<uint8_t>(kind)].price;
            auto poor = state;
            poor.bankRupees = price - 1;
            Reject(poor, Result::InsufficientBank, [&] { return GiveResidentGift(poor, resident, kind, world); });
            CHECK(GiveResidentGift(state, resident, kind, world) == Result::Success);
            spent += price;
            CHECK(state.bankRupees == 1000000 - spent);
            CHECK(HasMetResident(state, resident) && GiftAlreadyGiven(state, resident, kind));
            Reject(state, Result::AlreadyCompleted, [&] { return GiveResidentGift(state, resident, kind, world); });
        }
        CHECK(spent == 590 && state.givenGifts[index] == 7 && GetRapport(state, resident) == 16);
        CHECK(NextResidentGift(state, resident) == GiftKind::Count);
        for (uint32_t other = 0; other < kSocialResidentCount; ++other)
            if (other != index)
                CHECK(state.givenGifts[other] == 0 && state.rapport[other] == 0);
    }
    auto state = NewState();
    Reject(state, Result::Unavailable,
           [&] { return GiveResidentGift(state, ResidentId::Zelda, GiftKind::Keepsake, {}); });
    Reject(state, Result::InvalidAmount,
           [&] { return GiveResidentGift(state, ResidentId::Count, GiftKind::Supplies, world); });
    Reject(state, Result::InvalidAmount,
           [&] { return GiveResidentGift(state, ResidentId::Bram, static_cast<GiftKind>(255), world); });
    CHECK(std::string(GiftNameFor(ResidentId::Doron, GiftKind::Provisions)).find("mineral") != std::string::npos);
    state.enabled = 0;
    Reject(state, Result::Disabled,
           [&] { return GiveResidentGift(state, ResidentId::Bram, GiftKind::Supplies, world); });
    state.enabled = 1;
    state.rapport[1] = 99;
    CHECK(GiveResidentGift(state, ResidentId::Bram, GiftKind::Supplies, world) == Result::Success);
    CHECK(state.rapport[1] == 100);
    state.givenGifts[5] = 8;
    Reject(state, Result::InvalidState,
           [&] { return GiveResidentGift(state, ResidentId::Pella, GiftKind::Supplies, world); });
}

void Recognition() {
    auto state = NewState();
    auto world = Recovered();
    auto before = state;
    CHECK(RecognizeRoyalDeeds(state, {}) == 0 && Equal(before, state));
    CHECK(RecognizeRoyalDeeds(state, world) == 0x1f);
    CHECK(GetRapport(state, ResidentId::Zelda) == 25);
    before = state;
    for (int i = 0; i < 10000; ++i)
        CHECK(RecognizeRoyalDeeds(state, world) == 0 && Equal(state, before));
    state.marketRestored = 1;
    CHECK(RecognizeRoyalDeeds(state, world) == 0x20);
    state.zoraRestored = 1;
    CHECK(RecognizeRoyalDeeds(state, world) == 0x40);
    state.stewardship.charterMask = 0xff;
    CHECK(RecognizeRoyalDeeds(state, world) == 0x80);
    CHECK(state.royalRecognition == 0xff && GetRapport(state, ResidentId::Zelda) == 40);
    CHECK(state.bankRupees == 1000000 && state.metResidents == ResidentBit(ResidentId::Zelda));
    for (unsigned mask = 0; mask < 32; ++mask) {
        auto partial = NewState();
        WorldProgress progress{};
        progress.adult = progress.ganonDefeated = true;
        progress.forest = mask & 1;
        progress.fire = mask & 2;
        progress.water = mask & 4;
        progress.shadow = mask & 8;
        progress.spirit = mask & 16;
        CHECK(RecognizeRoyalDeeds(partial, progress) == mask);
        CHECK(partial.royalRecognition == mask);
    }
}

void RestorationAndEstate() {
    auto world = Recovered();
    auto state = NewState(kZoraRestorationPrice - 1);
    Reject(state, Result::InsufficientBank, [&] { return FundZoraRestoration(state, world, true); });
    state.bankRupees = kZoraRestorationPrice;
    Reject(state, Result::Unavailable, [&] { return FundZoraRestoration(state, world, false); });
    Reject(state, Result::Unavailable, [&] { return FundZoraRestoration(state, {}, true); });
    CHECK(FundZoraRestoration(state, world, true) == Result::Success);
    CHECK(state.bankRupees == 0 && state.zoraRestored == 1);
    CHECK(GetRapport(state, ResidentId::Lethra) == 10 && GetRapport(state, ResidentId::Neris) == 10);
    Reject(state, Result::AlreadyRestored, [&] { return FundZoraRestoration(state, world, true); });
    state = NewState();
    CHECK(CastleEstatePrice(state) == 500000);
    Reject(state, Result::Unavailable, [&] { return BuyCastleEstate(state, world, true); });
    state.marketRestored = 1;
    for (unsigned charters = 0; charters < 255; ++charters) {
        state.stewardship.charterMask = static_cast<uint8_t>(charters);
        Reject(state, Result::Unavailable, [&] { return BuyCastleEstate(state, world, true); });
    }
    state.stewardship.charterMask = 255;
    Reject(state, Result::Unavailable, [&] { return BuyCastleEstate(state, world, false); });
    Reject(state, Result::Unavailable, [&] { return BuyCastleEstate(state, {}, true); });
    state.bankRupees = 499999;
    Reject(state, Result::InsufficientBank, [&] { return BuyCastleEstate(state, world, true); });
    state.rapport[static_cast<uint32_t>(ResidentId::Zelda)] = 49;
    CHECK(CastleEstatePrice(state) == 500000);
    state.rapport[static_cast<uint32_t>(ResidentId::Zelda)] = 50;
    CHECK(CastleEstatePrice(state) == 450000);
    CHECK(BuyCastleEstate(state, world, true) == Result::Success);
    CHECK(state.bankRupees == 49999 && state.castleEstateOwned == 1 && IsValidState(state));
    CHECK(GetRapport(state, ResidentId::Zelda) == 60);
    Reject(state, Result::AlreadyOwned, [&] { return BuyCastleEstate(state, world, true); });
    CHECK(RecognizeRoyalDeeds(state, world) == 0xbf && IsValidState(state));
    state.stewardship.charterMask = 254;
    CHECK(!IsValidState(state));
}
} // namespace

int main() {
    Gifts();
    Recognition();
    RestorationAndEstate();
    std::cout << "Living Hyrule gift, royal recognition, recovery funding and castle policy: " << failures
              << " failures\n";
    return failures == 0 ? 0 : 1;
}
