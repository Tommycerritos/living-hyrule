#pragma once

#include "Properties.h"
#include <string>

namespace LivingHyrule {

enum class Action { Enable, Disable, Deposit, Withdraw, BuyCottage, BuyProperty, RepairProperty };

struct Status {
    EconomyState economy{};
    int fileNum = -1;
    int walletRupees = 0;
    int walletCapacity = 0;
    bool loaded = false;
    bool canUseLedger = false;
    bool inKakariko = false;
    bool cottageTradeOpen = false;
    Region currentRegion = Region::Count;
    WorldProgress world{};
    const char* reason = "Load a save file to begin.";
};

Status GetStatus();
std::string PerformAction(Action action, uint32_t amount = 0);

} // namespace LivingHyrule
