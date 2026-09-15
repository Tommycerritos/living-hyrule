#include "SocialPolicy.h"

#include <algorithm>
#include <climits>
#include <iostream>
#include <iterator>
#include <string>

namespace {
using namespace LivingHyrule;
int failures = 0;
#define CHECK(x)                                              \
    do {                                                      \
        if (!(x)) {                                           \
            std::cerr << "Line " << __LINE__ << ": " #x "\n"; \
            ++failures;                                       \
        }                                                     \
    } while (0)

bool Equal(const EconomyState& a, const EconomyState& b) {
    return a.enabled == b.enabled && a.bankRupees == b.bankRupees && a.ownsKakarikoCottage == b.ownsKakarikoCottage &&
           a.rentalFrames == b.rentalFrames && a.totalRentEarned == b.totalRentEarned &&
           a.ownedProperties == b.ownedProperties && a.repairedProperties == b.repairedProperties &&
           a.totalBusinessEarned == b.totalBusinessEarned &&
           std::equal(std::begin(a.businessFrames), std::end(a.businessFrames), std::begin(b.businessFrames)) &&
           std::equal(std::begin(a.rapport), std::end(a.rapport), std::begin(b.rapport)) &&
           a.metResidents == b.metResidents && a.completedFavors == b.completedFavors &&
           a.activeFavor == b.activeFavor && a.cottageRentPolicy == b.cottageRentPolicy &&
           a.currentPeriodPolicy == b.currentPeriodPolicy && a.marketRestored == b.marketRestored;
}
EconomyState Enabled(uint64_t bank = 0) {
    EconomyState state{};
    state.enabled = 1;
    state.bankRupees = bank;
    return state;
}
template <typename Operation> void Reject(EconomyState& state, Result expected, Operation operation) {
    const auto before = state;
    CHECK(operation() == expected);
    CHECK(Equal(state, before));
}

void TestMeetingAndValidation() {
    auto state = Enabled();
    static_assert(kSocialResidentCount == 23 && kFavorCount == 10);
    static_assert(static_cast<uint8_t>(ResidentId::Zelda) == 20 && static_cast<uint8_t>(ResidentId::Maelin) == 22);
    for (uint32_t i = 0; i < kSocialResidentCount; ++i) {
        const auto id = static_cast<ResidentId>(i);
        CHECK(IsValidResident(id) && ResidentBit(id) == (1u << i));
        CHECK(!HasMetResident(state, id));
        CHECK(std::string(GetSocialResidentName(id)) != "Unknown resident");
        for (int repeat = 0; repeat < 100; ++repeat)
            CHECK(MarkResidentMet(state, id) == Result::Success);
        CHECK(HasMetResident(state, id) && GetRapport(state, id) == 0);
        CHECK(AdjustRapport(state, id, INT_MAX) == Result::Success && GetRapport(state, id) == 100);
        CHECK(AdjustRapport(state, id, INT_MIN) == Result::Success && GetRapport(state, id) == -100);
        CHECK(AdjustRapport(state, id, 103) == Result::Success && GetRapport(state, id) == 3);
    }
    CHECK(state.metResidents == kMetResidentsMask);
    for (auto invalid : { ResidentId::Count, static_cast<ResidentId>(255) }) {
        CHECK(!IsValidResident(invalid) && ResidentBit(invalid) == 0 && !HasMetResident(state, invalid));
        Reject(state, Result::InvalidAmount, [&] { return MarkResidentMet(state, invalid); });
        Reject(state, Result::InvalidAmount, [&] { return AdjustRapport(state, invalid, 10); });
    }
    state.enabled = 0;
    Reject(state, Result::Disabled, [&] { return MarkResidentMet(state, ResidentId::Tavin); });
    Reject(state, Result::Disabled, [&] { return AdjustRapport(state, ResidentId::Tavin, 10); });

    for (unsigned int field = 0; field < 9; ++field) {
        auto invalid = Enabled();
        switch (field) {
            case 0:
                invalid.rapport[22] = -101;
                break;
            case 1:
                invalid.rapport[0] = 101;
                break;
            case 2:
                invalid.metResidents = 1u << 23;
                break;
            case 3:
                invalid.completedFavors = 1u << 10;
                break;
            case 4:
                invalid.activeFavor = 11;
                break;
            case 5:
                invalid.activeFavor = 1;
                invalid.completedFavors = 1;
                break;
            case 6:
                invalid.cottageRentPolicy = 2;
                break;
            case 7:
                invalid.currentPeriodPolicy = 2;
                break;
            case 8:
                invalid.marketRestored = 2;
                break;
        }
        CHECK(!IsValidState(invalid));
        Reject(invalid, Result::InvalidState, [&] { return AdjustRapport(invalid, ResidentId::Tavin, 10); });
        Reject(invalid, Result::InvalidState, [&] { return AcceptFavor(invalid, 1, {}); });
    }
}

void TestFiniteDeliveries() {
    const WorldProgress recovered{ true, true, true, true, true, true, true, true, true };
    auto state = Enabled(12345);
    uint32_t participants = 0;
    for (uint8_t id = 1; id <= kFavorCount; ++id) {
        const auto* favor = GetFavor(id);
        CHECK(favor != nullptr && favor->name[0] != '\0' && favor->instructions[0] != '\0');
        CHECK((participants & (ResidentBit(favor->issuer) | ResidentBit(favor->recipient))) == 0);
        participants |= ResidentBit(favor->issuer) | ResidentBit(favor->recipient);
        CHECK(CanAcceptFavor(state, id, recovered));
        Reject(state, Result::NoActiveFavor, [&] { return CompleteFavor(state, id, favor->recipient, recovered); });
        CHECK(AcceptFavor(state, id, recovered) == Result::Success);
        CHECK(state.activeFavor == id && GetRapport(state, favor->issuer) == 0);
        CHECK(!CanAcceptFavor(state, id, recovered));
        Reject(state, Result::FavorInProgress, [&] { return AcceptFavor(state, id, recovered); });
        Reject(state, Result::WrongResident, [&] { return CompleteFavor(state, id, favor->issuer, recovered); });
        Reject(state, Result::WrongResident, [&] { return CompleteFavor(state, id, ResidentId::Count, recovered); });
        if (id != kFavorCount)
            Reject(state, Result::Unavailable, [&] {
                return CompleteFavor(state, static_cast<uint8_t>(id + 1), GetFavor(id + 1)->recipient, recovered);
            });
        // Active delivery state survives the same value copy used by save snapshots.
        const auto snapshot = state;
        CHECK(CompleteFavor(state, id, favor->recipient, recovered) == Result::Success);
        CHECK(state.activeFavor == 0 && FavorCompleted(state, id));
        CHECK(GetRapport(state, favor->issuer) == 10 && GetRapport(state, favor->recipient) == 10);
        CHECK(snapshot.activeFavor == id && !FavorCompleted(snapshot, id));
        CHECK(GetRapport(snapshot, favor->recipient) == 0);
        Reject(state, Result::AlreadyCompleted, [&] { return CompleteFavor(state, id, favor->recipient, recovered); });
        Reject(state, Result::AlreadyCompleted, [&] { return AcceptFavor(state, id, recovered); });
        CHECK(state.bankRupees == 12345 && state.metResidents == 0);
    }
    CHECK(participants == 0xfffffu && state.completedFavors == kCompletedFavorsMask);
    CHECK(GetRapport(state, ResidentId::Zelda) == 0 && GetRapport(state, ResidentId::Aren) == 0 &&
          GetRapport(state, ResidentId::Maelin) == 0);
    CHECK(IsValidState(state));
    auto otherSlot = Enabled();
    Reject(otherSlot, Result::NoActiveFavor, [&] { return CompleteFavor(otherSlot, 1, ResidentId::Bram, recovered); });
    Reject(otherSlot, Result::NoActiveFavor, [&] { return AbandonFavor(otherSlot); });
    for (uint8_t invalid : { uint8_t{ 0 }, uint8_t{ 11 }, uint8_t{ 255 } }) {
        CHECK(GetFavor(invalid) == nullptr && FavorBit(invalid) == 0 && !CanAcceptFavor(otherSlot, invalid, recovered));
        Reject(otherSlot, Result::InvalidAmount, [&] { return AcceptFavor(otherSlot, invalid, recovered); });
        Reject(otherSlot, Result::InvalidAmount,
               [&] { return CompleteFavor(otherSlot, invalid, ResidentId::Bram, recovered); });
    }
    for (int repeat = 0; repeat < 100; ++repeat) {
        CHECK(AcceptFavor(otherSlot, 1, recovered) == Result::Success);
        CHECK(AbandonFavor(otherSlot) == Result::Success);
    }
    CHECK(Equal(otherSlot, Enabled())); // Accept/abandon cannot farm rapport or money.
    CHECK(AcceptFavor(otherSlot, 1, recovered) == Result::Success);
    otherSlot.enabled = 0;
    Reject(otherSlot, Result::Disabled, [&] { return CompleteFavor(otherSlot, 1, ResidentId::Bram, recovered); });
    Reject(otherSlot, Result::Disabled, [&] { return AbandonFavor(otherSlot); });
    Reject(otherSlot, Result::Disabled, [&] { return AcceptFavor(otherSlot, 2, recovered); });
}

void TestStoryAvailability() {
    const WorldProgress child{};
    WorldProgress crisis{};
    crisis.adult = true;
    auto state = Enabled();
    for (uint8_t id = 1; id <= kFavorCount; ++id) {
        CHECK(CanAcceptFavor(state, id, child) == (id < 10));
        CHECK(!CanAcceptFavor(state, id, crisis));
        Reject(state, Result::Unavailable, [&] { return AcceptFavor(state, id, crisis); });
    }
    CHECK(ResidentAccessible(ResidentId::Bram, crisis));
    CHECK(ResidentAccessible(ResidentId::Wren, crisis));
    CHECK(ResidentAccessible(ResidentId::Edda, crisis));
    CHECK(ResidentAccessible(ResidentId::Lethra, crisis));
    CHECK(!ResidentAccessible(ResidentId::Count, child));
    auto restored = crisis;
    restored.shadow = true;
    CHECK(CanAcceptFavor(state, 1, restored));
    CHECK(!CanAcceptFavor(state, 2, restored)); // Hollis is still absent.
    restored.forest = true;
    CHECK(CanAcceptFavor(state, 2, restored) && CanAcceptFavor(state, 7, restored));
    CHECK(!CanAcceptFavor(state, 3, restored)); // Market recipient is still absent.
    restored.ganonDefeated = true;
    CHECK(CanAcceptFavor(state, 3, restored) && CanAcceptFavor(state, 4, restored));
    CHECK(ResidentAccessible(ResidentId::Zelda, restored));
    restored.ranchFreed = true;
    restored.water = true;
    restored.fire = true;
    CHECK(CanAcceptFavor(state, 5, restored) && CanAcceptFavor(state, 6, restored));
    CHECK(CanAcceptFavor(state, 8, restored) && CanAcceptFavor(state, 9, restored));
    restored.gerudoMembership = true;
    CHECK(!CanAcceptFavor(state, 10, restored));
    restored.spirit = true;
    CHECK(CanAcceptFavor(state, 10, restored));
    restored.gerudoMembership = false;
    CHECK(!CanAcceptFavor(state, 10, restored));

    CHECK(AcceptFavor(state, 4, child) == Result::Success); // Pella can be reached after dark.
    Reject(state, Result::Unavailable, [&] { return CompleteFavor(state, 4, ResidentId::Pella, crisis); });
    CHECK(CompleteFavor(state, 4, ResidentId::Pella, restored) == Result::Success);
    CHECK(AcceptFavor(state, 5, child) == Result::Success);
    CHECK(CompleteFavor(state, 5, ResidentId::Wren, crisis) == Result::Success); // Recipient remains reachable.
}

void TestMarketRestoration() {
    const WorldProgress recovered{ true, true, true, true, true, true, true, true, true };
    auto state = Enabled(kMarketRestorationPrice);
    Reject(state, Result::Unavailable, [&] { return FundMarketRestoration(state, {}); });
    auto noVictory = recovered;
    noVictory.ganonDefeated = false;
    Reject(state, Result::Unavailable, [&] { return FundMarketRestoration(state, noVictory); });
    auto childVictory = recovered;
    childVictory.adult = false;
    Reject(state, Result::Unavailable, [&] { return FundMarketRestoration(state, childVictory); });
    state.enabled = 0;
    Reject(state, Result::Disabled, [&] { return FundMarketRestoration(state, recovered); });
    state.enabled = 1;
    --state.bankRupees;
    Reject(state, Result::InsufficientBank, [&] { return FundMarketRestoration(state, recovered); });
    ++state.bankRupees;
    CHECK(FundMarketRestoration(state, recovered) == Result::Success);
    CHECK(state.bankRupees == 0 && state.marketRestored == 1 && state.ownedProperties == 0);
    CHECK(state.repairedProperties == 0 && state.metResidents == 0 && state.completedFavors == 0);
    CHECK(GetRapport(state, ResidentId::Zelda) == 20);
    CHECK(GetRapport(state, ResidentId::Hadrin) == 10 && GetRapport(state, ResidentId::Maelin) == 10);
    CHECK(GetRapport(state, ResidentId::Vessa) == 10 && GetRapport(state, ResidentId::Pella) == 10);
    CHECK(GetRapport(state, ResidentId::Aren) == 10 && GetRapport(state, ResidentId::Tavin) == 0);
    state.bankRupees = kMarketRestorationPrice;
    Reject(state, Result::AlreadyRestored, [&] { return FundMarketRestoration(state, recovered); });
    state.marketRestored = 2;
    Reject(state, Result::InvalidState, [&] { return FundMarketRestoration(state, recovered); });
}
} // namespace

int main() {
    TestMeetingAndValidation();
    TestFiniteDeliveries();
    TestStoryAvailability();
    TestMarketRestoration();
    if (failures != 0) {
        std::cerr << failures << " social checks failed\n";
        return 1;
    }
    std::cout << "Persistent relationships, finite deliveries and restoration funding tests passed\n";
    return 0;
}
