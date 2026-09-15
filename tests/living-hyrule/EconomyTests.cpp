#include "Economy.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>

namespace {
using namespace LivingHyrule;
int failures = 0;

void Check(bool condition, const char* expression, int line) {
    if (!condition) {
        std::cerr << "Line " << line << ": " << expression << '\n';
        ++failures;
    }
}

#define CHECK(expression) Check((expression), #expression, __LINE__)

bool Equal(const EconomyState& a, const EconomyState& b) {
    // Compare fields, not padding bytes in the C-compatible save struct.
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

using Transfer = Result (*)(EconomyState&, int16_t&, int, uint32_t);

void RejectTransfer(Transfer operation, EconomyState state, int16_t wallet, int capacity, uint32_t amount,
                    Result expected) {
    const EconomyState before = state;
    const int16_t walletBefore = wallet;
    CHECK(operation(state, wallet, capacity, amount) == expected);
    CHECK(Equal(state, before));
    CHECK(wallet == walletBefore);
}

void RejectPurchase(EconomyState state, Result expected) {
    const EconomyState before = state;
    CHECK(BuyCottage(state) == expected);
    CHECK(Equal(state, before));
}

void TestValidationAndAtomicFailures() {
    EconomyState fresh{};
    CHECK(IsValidState(fresh));
    RejectTransfer(Deposit, fresh, 99, 99, 1, Result::Disabled);
    RejectTransfer(Withdraw, fresh, 0, 99, 1, Result::Disabled);
    RejectPurchase(fresh, Result::Disabled);

    for (Transfer operation : { Deposit, Withdraw }) {
        RejectTransfer(operation, Enabled(500), 50, 99, 0, Result::InvalidAmount);
        RejectTransfer(operation, Enabled(500), 50, 99, static_cast<uint32_t>(kBankLimit + 1), Result::InvalidAmount);
        RejectTransfer(operation, Enabled(500), 50, 99, (std::numeric_limits<uint32_t>::max)(), Result::InvalidAmount);
        RejectTransfer(operation, Enabled(500), -1, 99, 1, Result::InvalidWallet);
        RejectTransfer(operation, Enabled(500), 0, -1, 1, Result::InvalidWallet);
        RejectTransfer(operation, Enabled(500), 0, 32768, 1, Result::InvalidWallet);
        RejectTransfer(operation, Enabled(500), 100, 99, 1, Result::InvalidWallet);
    }

    for (int invalidField = 0; invalidField < 4; ++invalidField) {
        EconomyState corrupt = Enabled(5000);
        switch (invalidField) {
            case 0:
                corrupt.enabled = 2;
                break;
            case 1:
                corrupt.ownsKakarikoCottage = 2;
                break;
            case 2:
                corrupt.bankRupees = kBankLimit + 1;
                break;
            case 3:
                corrupt.rentalFrames = kFramesPerRentPeriod;
                break;
        }
        CHECK(!IsValidState(corrupt));
        RejectTransfer(Deposit, corrupt, 50, 99, 1, Result::InvalidState);
        RejectTransfer(Withdraw, corrupt, 50, 99, 1, Result::InvalidState);
        RejectPurchase(corrupt, Result::InvalidState);
        const EconomyState before = corrupt;
        CHECK(TickRent(corrupt) == 0);
        CHECK(Equal(corrupt, before));
    }
}

void TestTransfersAndLimits() {
    EconomyState state = Enabled();
    int16_t wallet = 99;
    CHECK(Deposit(state, wallet, 99, 99) == Result::Success);
    CHECK(state.bankRupees == 99 && wallet == 0);
    CHECK(Withdraw(state, wallet, 99, 99) == Result::Success);
    CHECK(state.bankRupees == 0 && wallet == 99);

    RejectTransfer(Deposit, Enabled(), 99, 99, 100, Result::InsufficientWallet);
    RejectTransfer(Deposit, Enabled(), 0, 0, 1, Result::InsufficientWallet);
    RejectTransfer(Withdraw, Enabled(10), 0, 99, 11, Result::InsufficientBank);
    RejectTransfer(Withdraw, Enabled(100), 99, 99, 1, Result::WalletFull);
    RejectTransfer(Withdraw, Enabled(100), 90, 99, 10, Result::WalletFull);
    RejectTransfer(Withdraw, Enabled(100), 0, 0, 1, Result::WalletFull);
    RejectTransfer(Deposit, Enabled(kBankLimit), 99, 99, 1, Result::BankFull);
    RejectTransfer(Deposit, Enabled(kBankLimit - 9), 99, 99, 10, Result::BankFull);

    state = Enabled(kBankLimit - 99);
    wallet = 99;
    CHECK(Deposit(state, wallet, 99, 99) == Result::Success);
    CHECK(state.bankRupees == kBankLimit && wallet == 0);
    CHECK(Withdraw(state, wallet, 99, 99) == Result::Success);
    CHECK(state.bankRupees == kBankLimit - 99 && wallet == 99);

    state = Enabled(32767);
    wallet = 0;
    CHECK(Withdraw(state, wallet, 32767, 32767) == Result::Success);
    CHECK(state.bankRupees == 0 && wallet == 32767);
    CHECK(Deposit(state, wallet, 32767, 32767) == Result::Success);
    CHECK(state.bankRupees == 32767 && wallet == 0);
}

void TestPurchaseAndRent() {
    RejectPurchase(Enabled(kCottagePrice - 1), Result::InsufficientBank);
    EconomyState state = Enabled(kCottagePrice);
    CHECK(BuyCottage(state) == Result::Success);
    CHECK(state.bankRupees == 0 && state.ownsKakarikoCottage == 1 && state.rentalFrames == 0);
    RejectPurchase(state, Result::AlreadyOwned);
    state.bankRupees = kCottagePrice + 100;
    RejectPurchase(state, Result::AlreadyOwned);

    state.bankRupees = 0;
    for (uint32_t frame = 1; frame < kFramesPerRentPeriod; ++frame) {
        CHECK(TickRent(state) == 0);
        CHECK(state.rentalFrames == frame && state.bankRupees == 0 && state.totalRentEarned == 0);
    }
    CHECK(TickRent(state) == kRentPerPeriod);
    CHECK(state.rentalFrames == 0 && state.bankRupees == kRentPerPeriod && state.totalRentEarned == kRentPerPeriod);
    for (uint32_t frame = 1; frame < kFramesPerRentPeriod; ++frame) {
        CHECK(TickRent(state) == 0);
    }
    CHECK(TickRent(state) == kRentPerPeriod);
    CHECK(state.bankRupees == 2 * kRentPerPeriod && state.totalRentEarned == 2 * kRentPerPeriod);

    state = Enabled(100);
    const EconomyState unowned = state;
    for (uint32_t frame = 0; frame < kFramesPerRentPeriod + 1; ++frame) {
        CHECK(TickRent(state) == 0);
    }
    CHECK(Equal(state, unowned));
}

void TestReloadAndFileIsolation() {
    EconomyState firstFile = Enabled(kCottagePrice);
    CHECK(BuyCottage(firstFile) == Result::Success);
    for (int frame = 0; frame < 4000; ++frame) {
        CHECK(TickRent(firstFile) == 0);
    }

    // The engine snapshots SaveContext by copying it. Verify partial periods
    // survive the same kind of copy, without any hidden process-global timer.
    EconomyState reloaded{};
    std::memcpy(&reloaded, &firstFile, sizeof(reloaded));
    CHECK(Equal(firstFile, reloaded));
    for (int frame = 0; frame < 7999; ++frame) {
        CHECK(TickRent(reloaded) == 0);
    }
    CHECK(TickRent(reloaded) == kRentPerPeriod);
    CHECK(reloaded.rentalFrames == 0 && reloaded.bankRupees == kRentPerPeriod);
    CHECK(firstFile.rentalFrames == 4000 && firstFile.bankRupees == 0);

    EconomyState secondFile{};
    const EconomyState secondBefore = secondFile;
    CHECK(TickRent(secondFile) == 0);
    CHECK(Equal(secondFile, secondBefore));
    RejectTransfer(Deposit, secondFile, 99, 99, 10, Result::Disabled);
    RejectPurchase(secondFile, Result::Disabled);
    CHECK(firstFile.rentalFrames == 4000 && firstFile.ownsKakarikoCottage == 1);

    reloaded.rentalFrames = kFramesPerRentPeriod - 1;
    reloaded.enabled = 0;
    const EconomyState disabledBefore = reloaded;
    CHECK(IsValidState(reloaded));
    for (int frame = 0; frame < 100; ++frame) {
        CHECK(TickRent(reloaded) == 0);
    }
    CHECK(Equal(reloaded, disabledBefore));
    RejectTransfer(Withdraw, reloaded, 0, 99, 1, Result::Disabled);
    reloaded.enabled = 1;
    CHECK(TickRent(reloaded) == kRentPerPeriod);
    CHECK(reloaded.bankRupees == 2 * kRentPerPeriod);
}

void TestRentSaturation() {
    EconomyState state = Enabled(kBankLimit - 10);
    state.ownsKakarikoCottage = 1;
    state.rentalFrames = kFramesPerRentPeriod - 1;
    state.totalRentEarned = (std::numeric_limits<uint64_t>::max)() - 3;
    CHECK(TickRent(state) == 10);
    CHECK(state.bankRupees == kBankLimit && state.rentalFrames == 0);
    CHECK(state.totalRentEarned == (std::numeric_limits<uint64_t>::max)());

    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == 0);
    CHECK(state.bankRupees == kBankLimit && state.rentalFrames == 0);
    CHECK(state.totalRentEarned == (std::numeric_limits<uint64_t>::max)());
    int16_t wallet = 0;
    CHECK(Withdraw(state, wallet, 99, 10) == Result::Success);
    CHECK(TickRent(state) == 0);
    CHECK(state.bankRupees == kBankLimit - 10 && state.rentalFrames == 1);

    state = Enabled(kBankLimit - kRentPerPeriod);
    state.ownsKakarikoCottage = 1;
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == kRentPerPeriod);
    CHECK(state.bankRupees == kBankLimit && state.totalRentEarned == kRentPerPeriod);
}

void TestRandomizedConservation() {
    std::mt19937 random(0x4C485952u);
    for (int capacity : { 0, 99, 200, 500, 999, 32767 }) {
        for (uint64_t initialBank : { uint64_t{ 0 }, uint64_t{ 1200 }, kBankLimit - 100, kBankLimit }) {
            EconomyState state = Enabled(initialBank);
            int16_t wallet = static_cast<int16_t>(capacity);
            const uint64_t combined = state.bankRupees + static_cast<uint64_t>(wallet);
            for (int operation = 0; operation < 20000; ++operation) {
                const uint32_t amount = static_cast<uint32_t>(random() % 40000);
                const EconomyState before = state;
                const int16_t walletBefore = wallet;
                const Result result = (random() & 1) ? Deposit(state, wallet, capacity, amount)
                                                     : Withdraw(state, wallet, capacity, amount);
                if (result != Result::Success) {
                    CHECK(Equal(state, before));
                    CHECK(wallet == walletBefore);
                }
                CHECK(IsValidState(state));
                CHECK(wallet >= 0 && wallet <= capacity);
                CHECK(state.bankRupees + static_cast<uint64_t>(wallet) == combined);
            }
        }
    }
}

void TestRentTermsAndRapport() {
    auto state = Enabled(kCottagePrice);
    CHECK(BuyCottage(state) == Result::Success);
    CHECK(EffectiveCottageRent(state) == 25);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(SetCottageRentPolicy(state, true) == Result::Success);
    CHECK(state.currentPeriodPolicy == 0 && state.cottageRentPolicy == 1);
    CHECK(TickRent(state) == 25);
    CHECK(GetRapport(state, ResidentId::Bram) == 1 && state.currentPeriodPolicy == 1);
    CHECK(EffectiveCottageRent(state) == 40);
    for (int period = 0; period < 4; ++period) {
        state.rentalFrames = kFramesPerRentPeriod - 1;
        CHECK(TickRent(state) == 40);
    }
    CHECK(GetRapport(state, ResidentId::Bram) == -11);
    CHECK(EffectiveCottageRent(state) == 15);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    for (int toggle = 0; toggle < 100; ++toggle)
        CHECK(SetCottageRentPolicy(state, (toggle % 2) != 0) == Result::Success);
    CHECK(SetCottageRentPolicy(state, false) == Result::Success);
    CHECK(TickRent(state) == 15); // Last high period cannot be rewritten by the toggle.
    CHECK(GetRapport(state, ResidentId::Bram) == -14 && state.currentPeriodPolicy == 0);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == 25);
    CHECK(GetRapport(state, ResidentId::Bram) == -13);

    CHECK(AdjustRapport(state, ResidentId::Bram, 32) == Result::Success);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == 25);
    CHECK(GetRapport(state, ResidentId::Bram) == 20);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == 25 && GetRapport(state, ResidentId::Bram) == 20);
    CHECK(AdjustRapport(state, ResidentId::Bram, 40) == Result::Success);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == 25 && GetRapport(state, ResidentId::Bram) == 60);

    state.bankRupees = kBankLimit;
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(SetCottageRentPolicy(state, true) == Result::Success);
    CHECK(TickRent(state) == 0);
    CHECK(GetRapport(state, ResidentId::Bram) == 60 && state.currentPeriodPolicy == 1);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == 0 && GetRapport(state, ResidentId::Bram) == 60);
    state.bankRupees = kBankLimit - 1;
    state.rentalFrames = kFramesPerRentPeriod - 1;
    CHECK(TickRent(state) == 1 && GetRapport(state, ResidentId::Bram) == 57);
    state.rentalFrames = kFramesPerRentPeriod - 1;
    state.enabled = 0;
    const auto disabled = state;
    CHECK(SetCottageRentPolicy(state, false) == Result::Disabled);
    CHECK(TickRent(state) == 0 && Equal(state, disabled));
    state = Enabled();
    const auto unowned = state;
    CHECK(SetCottageRentPolicy(state, true) == Result::NotOwned && Equal(state, unowned));

    // Both the active period and requested change survive a copied save/slot.
    state = Enabled();
    state.ownsKakarikoCottage = 1;
    state.currentPeriodPolicy = 1;
    state.rentalFrames = kFramesPerRentPeriod - 1;
    auto secondSlot = state;
    CHECK(TickRent(secondSlot) == 40);
    CHECK(state.currentPeriodPolicy == 1 && state.rentalFrames == kFramesPerRentPeriod - 1);
    CHECK(GetRapport(state, ResidentId::Bram) == 0 && GetRapport(secondSlot, ResidentId::Bram) == -3);
    CHECK(secondSlot.currentPeriodPolicy == 0);
}

} // namespace

int main() {
    TestValidationAndAtomicFailures();
    TestTransfersAndLimits();
    TestPurchaseAndRent();
    TestReloadAndFileIsolation();
    TestRentSaturation();
    TestRentTermsAndRapport();
    TestRandomizedConservation();
    if (failures != 0) {
        std::cerr << failures << " checks failed.\n";
        return 1;
    }
    std::cout << "Living Hyrule economy: all checks passed (480,000 randomized transfers).\n";
    return 0;
}
