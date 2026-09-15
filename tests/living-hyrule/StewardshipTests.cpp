#include "StewardshipPolicy.h"
#include <iostream>
#include <limits>

using namespace LivingHyrule;
int failures = 0;
#define CHECK(x)                                              \
    do {                                                      \
        if (!(x)) {                                           \
            std::cerr << "Line " << __LINE__ << ": " #x "\n"; \
            ++failures;                                       \
        }                                                     \
    } while (0)

int main() {
    WorldProgress world{ true, true, true, true, true, true, true, true, true };
    EconomyState economy{};
    StewardshipState state{};
    economy.enabled = 1;
    economy.bankRupees = 1000000;
    CHECK(IsValidStewardshipState(state));
    uint32_t combined = 0;
    uint64_t spent = 0;
    for (uint8_t index = 0; index < kStewardshipRegionCount; ++index) {
        const auto region = static_cast<Region>(index);
        const uint32_t mask = RegionalPropertyMask(region);
        CHECK(mask != 0 && (mask & combined) == 0);
        combined |= mask;
        CHECK(CharterEligibility(economy, state, region, Region::Count, world, true) == StewardshipResult::NotLocal);
        CHECK(CharterEligibility(economy, state, region, region, {}, true) == StewardshipResult::AwaitingRecovery);
        CHECK(CharterEligibility(economy, state, region, region, world, true) == StewardshipResult::MissingHoldings);
        economy.ownedProperties |= mask;
        if (region == Region::Kakariko)
            economy.ownsKakarikoCottage = 1;
        CHECK(CharterEligibility(economy, state, region, region, world, true) == StewardshipResult::RepairsRequired);
        economy.repairedProperties |= mask;
        if (region == Region::Market)
            CHECK(CharterEligibility(economy, state, region, region, world, false) ==
                  StewardshipResult::MarketNotRestored);
        const auto before = economy.bankRupees;
        CHECK(BuyRegionalCharter(economy, state, region, region, world, true) == StewardshipResult::Success);
        spent += kRegionalCharterPrices[index];
        CHECK(before - economy.bankRupees == kRegionalCharterPrices[index]);
        CHECK(BuyRegionalCharter(economy, state, region, region, world, true) == StewardshipResult::AlreadyChartered);
        CHECK(economy.bankRupees == 1000000 - spent);
        CHECK(RegionalCharterCount(state) == index + 1);
        CHECK(CreditRegionalDues(state, index, 90) == 9);
        CHECK(TransferRegionalTreasury(economy, state, region, region, world, 9, false) == StewardshipResult::Success);
        CHECK(state.treasury[index] == 0);
        CHECK(TransferRegionalTreasury(economy, state, region, region, world, 9, true) == StewardshipResult::Success);
        CHECK(economy.bankRupees == 1000000 - spent);
    }
    CHECK(combined == 0xffffu && state.charterMask == 0xff && RegionalCharterCount(state) == 8);
    for (uint32_t tick = 0; tick < 100000; ++tick) {
        const auto index = static_cast<uint8_t>(tick % 8);
        const auto region = static_cast<Region>(index);
        const uint64_t amount = tick % 257 + 1;
        const uint64_t sum = economy.bankRupees + state.treasury[index];
        CHECK(TransferRegionalTreasury(economy, state, region, region, world, amount, true) ==
              StewardshipResult::Success);
        CHECK(economy.bankRupees + state.treasury[index] == sum);
        CHECK(TransferRegionalTreasury(economy, state, region, region, world, amount, false) ==
              StewardshipResult::Success);
        CHECK(economy.bankRupees + state.treasury[index] == sum);
    }
    state.treasury[0] = kTreasuryLimit - 1;
    CHECK(CreditRegionalDues(state, 0, UINT32_MAX) == 1);
    CHECK(CreditRegionalDues(state, 0, UINT32_MAX) == 0);
    CHECK(state.treasury[0] == kTreasuryLimit);
    auto bad = state;
    bad.charterMask = static_cast<uint8_t>(bad.charterMask & 0xfeu);
    CHECK(!IsValidStewardshipState(bad));
    CHECK(CreditRegionalDues(bad, 0, 100) == 0);
    CHECK(CreditRegionalDues(state, 255, 100) == 0);
    const auto bank = economy.bankRupees;
    CHECK(TransferRegionalTreasury(economy, state, Region::Market, Region::Market, world, UINT64_MAX, false) ==
          StewardshipResult::InsufficientTreasury);
    CHECK(economy.bankRupees == bank && state.treasury[0] == kTreasuryLimit);
    economy.bankRupees = kBankLimit;
    CHECK(TransferRegionalTreasury(economy, state, Region::Market, Region::Market, world, 1, false) ==
          StewardshipResult::BankFull);
    CHECK(TransferRegionalTreasury(economy, state, Region::Market, Region::Market, world, 1, true) ==
          StewardshipResult::TreasuryFull);
    CHECK(economy.bankRupees == kBankLimit && state.treasury[0] == kTreasuryLimit);
    if (failures)
        return 1;
    std::cout << "Regional charter gates, finite dues and treasury conservation checks passed\n";
    return 0;
}
