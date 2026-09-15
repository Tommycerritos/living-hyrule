#include "Stewardship.h"

#include "MarketRestoration.h"
#include "ResidentSocial.h"
#include "StewardshipPolicy.h"
#include "soh/SaveManager.h"

#include <algorithm>
#include <imgui.h>
#include <utility>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

int lastFileNum = -2;
int transferAmount = 100;
std::string feedback;

enum class TreasuryAction { BuyCharter, Deposit, Withdraw };

void ResetFeedbackForFile(int fileNum) {
    if (lastFileNum == fileNum)
        return;
    lastFileNum = fileNum;
    transferAmount = 100;
    feedback.clear();
}

bool RealAdventure(const Status& status) {
    return status.loaded && gPlayState != nullptr && status.fileNum >= 0 && status.fileNum <= 2 &&
           gSaveContext.fileNum == status.fileNum && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           (IS_VANILLA || IS_MASTER_QUEST) && !IS_CUTSCENE_LAYER && SaveManager::Instance != nullptr &&
           SaveManager::Instance->SaveFile_Exist(status.fileNum);
}

const char* ExplainStewardshipResult(StewardshipResult result) {
    switch (result) {
        case StewardshipResult::Success:
            return "Ready.";
        case StewardshipResult::InvalidState:
            return "This ledger could not be read. Your stored records are preserved.";
        case StewardshipResult::Disabled:
            return "Enable the economy for this save to manage a charter or treasury.";
        case StewardshipResult::InvalidRegion:
            return "Choose one of the eight regions.";
        case StewardshipResult::NotLocal:
            return "Visit this region to buy its charter or transfer treasury funds.";
        case StewardshipResult::AwaitingRecovery:
            return "Return as an adult after this region's recovery requirement is complete.";
        case StewardshipResult::MissingHoldings:
            return "Own every listed business first. Kakariko also requires the cottage.";
        case StewardshipResult::RepairsRequired:
            return "Complete the adult-era repairs on every listed business first.";
        case StewardshipResult::MarketNotRestored:
            return "Fund the Market square's restoration, then leave and return to see the work before buying its "
                   "charter.";
        case StewardshipResult::InsufficientBank:
            return "You do not have enough rupees in the bank.";
        case StewardshipResult::AlreadyChartered:
            return "You already hold this regional charter.";
        case StewardshipResult::NotChartered:
            return "Purchase this region's charter to open its treasury.";
        case StewardshipResult::InvalidAmount:
            return "Choose a positive number of rupees.";
        case StewardshipResult::InsufficientTreasury:
            return "This regional treasury does not contain that many rupees.";
        case StewardshipResult::TreasuryFull:
            return "That amount will not fit in this regional treasury.";
        case StewardshipResult::BankFull:
            return "That amount will not fit in your bank account.";
    }
    return "The transaction could not be completed.";
}

const char* RecoveryRequirement(Region region) {
    switch (region) {
        case Region::Market:
            return "Defeat Ganon, then restore the Market square and return to it.";
        case Region::Field:
        case Region::Forest:
            return "Clear the Forest Temple as an adult.";
        case Region::Ranch:
            return "Win Epona's freedom as an adult.";
        case Region::Kakariko:
            return "Clear the Shadow Temple as an adult.";
        case Region::Mountain:
            return "Clear the Fire Temple as an adult.";
        case Region::Water:
            return "Clear the Water Temple as an adult.";
        case Region::Desert:
            return "Earn Gerudo membership and clear the Spirit Temple as an adult.";
        default:
            return "Visit one of the eight regions.";
    }
}

StewardshipResult PreviewTransfer(const Status& status, Region region, uint64_t amount, bool deposit) {
    // The policy's complete transfer check is also used for button availability.
    // This copy is discarded; drawing controls never changes the live balances.
    auto preview = status.economy;
    return TransferRegionalTreasury(preview, preview.stewardship, region, status.currentRegion, status.world, amount,
                                    deposit);
}

std::string PerformTreasuryAction(TreasuryAction action, Region region, uint32_t amount, Status& status) {
    const int clickedFile = status.fileNum;
    status = GetStatus();
    if (status.fileNum != clickedFile)
        return "The save file changed. Choose the action again for this file.";
    if (!status.canUseLedger)
        return status.reason != nullptr && status.reason[0] != '\0' ? status.reason : "The ledger is unavailable.";
    if (!RealAdventure(status))
        return "Load an existing normal adventure or Master Quest save to manage stewardship.";

    // No yield, hook or asynchronous work occurs between this fresh safety
    // check and the atomic policy operation on the live copied-save POD. Normal
    // game saves capture bank, charters and treasuries together.
    auto& economy = gSaveContext.ship.livingHyrule;
    const auto result = action == TreasuryAction::BuyCharter
                            ? BuyRegionalCharter(economy, economy.stewardship, region, status.currentRegion,
                                                 status.world, IsMarketRestorationActive())
                            : TransferRegionalTreasury(economy, economy.stewardship, region, status.currentRegion,
                                                       status.world, amount, action == TreasuryAction::Deposit);
    status = GetStatus();
    if (result != StewardshipResult::Success)
        return ExplainStewardshipResult(result);
    if (action == TreasuryAction::BuyCharter) {
        const std::string title = RegionalCharterCount(status.economy.stewardship) == kStewardshipRegionCount
                                      ? "High Steward of Hyrule"
                                      : "Steward of " + std::string(kRegionNames[static_cast<uint8_t>(region)]);
        return "Charter purchased. You are recognized as " + title + ". Save normally to keep your charter.";
    }
    return std::to_string(amount) + (action == TreasuryAction::Deposit ? " rupees deposited in the regional treasury."
                                                                       : " rupees transferred to your bank.");
}

void DrawTreasuryButton(const char* label, TreasuryAction action, Region region, uint32_t amount, bool disabled,
                        Status& status) {
    ImGui::BeginDisabled(disabled || !status.canUseLedger);
    const bool clicked = ImGui::Button(label);
    ImGui::EndDisabled();
    if (!clicked)
        return;
    auto result = PerformTreasuryAction(action, region, amount, status);
    ResetFeedbackForFile(status.fileNum);
    feedback = std::move(result);
}

void DrawHoldings(const Status& status, Region region) {
    unsigned int total = 0, owned = 0, repaired = 0;
    for (uint32_t id = 0; id < kProperties.size(); ++id) {
        if (kProperties[id].region != region)
            continue;
        ++total;
        owned += OwnsProperty(status.economy, id) ? 1 : 0;
        repaired += (status.economy.repairedProperties & (1u << id)) != 0 ? 1 : 0;
    }
    ImGui::Text("Businesses owned: %u / %u | Repaired: %u / %u", owned, total, repaired, total);
    for (uint32_t id = 0; id < kProperties.size(); ++id) {
        if (kProperties[id].region != region)
            continue;
        const char* condition = !OwnsProperty(status.economy, id) ? "not owned"
                                : (status.economy.repairedProperties & (1u << id)) != 0
                                    ? "owned and repaired"
                                    : "repairs required as an adult";
        ImGui::BulletText("%s: %s", kProperties[id].name, condition);
    }
    if (region == Region::Kakariko)
        ImGui::BulletText("Kakariko cottage: %s", status.economy.ownsKakarikoCottage ? "owned" : "not owned");
}

Region ResidentRegion(ResidentId resident) {
    switch (resident) {
        case ResidentId::Tavin:
        case ResidentId::Bram:
        case ResidentId::Orlen:
            return Region::Kakariko;
        case ResidentId::Vessa:
        case ResidentId::Hadrin:
        case ResidentId::Pella:
            return Region::Market;
        case ResidentId::Caro:
        case ResidentId::Hollis:
            return Region::Field;
        case ResidentId::Nessa:
        case ResidentId::Wren:
            return Region::Ranch;
        case ResidentId::Fenn:
        case ResidentId::Luma:
            return Region::Forest;
        case ResidentId::Doron:
        case ResidentId::Brakka:
            return Region::Mountain;
        case ResidentId::Vero:
        case ResidentId::Edda:
        case ResidentId::Lethra:
        case ResidentId::Neris:
            return Region::Water;
        case ResidentId::Rasha:
        case ResidentId::Kesra:
            return Region::Desert;
        default:
            return Region::Count;
    }
}

} // namespace

void DrawStewardshipControls(Status& status) {
    ResetFeedbackForFile(status.fileNum);
    ImGui::Separator();
    if (!ImGui::CollapsingHeader("Regional stewardship"))
        return;
    ImGui::PushID("LivingHyruleStewardship");
    if (!status.loaded || !IsValidState(status.economy) || !IsValidStewardshipState(status.economy.stewardship)) {
        ImGui::TextWrapped("Load a readable save ledger to view regional charters and treasuries.");
        ImGui::PopID();
        return;
    }
    const auto count = RegionalCharterCount(status.economy.stewardship);
    ImGui::Text("Regional charters: %u / %u", static_cast<unsigned int>(count),
                static_cast<unsigned int>(kStewardshipRegionCount));
    if (count == kStewardshipRegionCount)
        ImGui::TextColored(ImVec4(0.90f, 0.78f, 0.40f, 1.0f), "High Steward of Hyrule");
    ImGui::TextWrapped("Own and repair every business in a recovered region, then purchase its charter while "
                       "visiting. A charter grants a regional title, resident recognition and a separate treasury.");
    ImGui::TextWrapped("Working adult businesses add dues equal to 10%% of each income period to their chartered "
                       "region's treasury, rounded down. Transfers between a treasury and your bank are local.");
    ImGui::Text("Bank: %llu rupees | Treasury limit: %llu per region",
                static_cast<unsigned long long>(status.economy.bankRupees),
                static_cast<unsigned long long>(kTreasuryLimit));
    if (!status.world.adult)
        ImGui::TextWrapped("Regional charters become available in adulthood, after each region recovers.");

    for (uint8_t index = 0; index < kStewardshipRegionCount; ++index) {
        const auto region = static_cast<Region>(index);
        ImGui::PushID(index);
        const bool local = status.currentRegion == region;
        const bool expanded = ImGui::TreeNodeEx(kRegionNames[index], local ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        ImGui::SameLine();
        ImGui::TextUnformatted(HoldsRegionalCharter(status.economy.stewardship, index) ? "Chartered" : "No charter");
        if (expanded) {
            DrawHoldings(status, region);
            if (!HoldsRegionalCharter(status.economy.stewardship, index)) {
                ImGui::Text("Charter price: %u bank rupees", kRegionalCharterPrices[index]);
                if (!status.world.adult || !RegionOpen(region, status.world))
                    ImGui::TextWrapped("%s", RecoveryRequirement(region));
                const auto eligibility =
                    CharterEligibility(status.economy, status.economy.stewardship, region, status.currentRegion,
                                       status.world, IsMarketRestorationActive());
                if (eligibility != StewardshipResult::Success)
                    ImGui::TextWrapped("%s", ExplainStewardshipResult(eligibility));
                DrawTreasuryButton("Purchase regional charter", TreasuryAction::BuyCharter, region, 0,
                                   eligibility != StewardshipResult::Success, status);
            }
            // Re-read the refreshed status after a possible purchase above.
            if (HoldsRegionalCharter(status.economy.stewardship, index)) {
                ImGui::Text("Treasury: %llu rupees",
                            static_cast<unsigned long long>(status.economy.stewardship.treasury[index]));
                if (!local) {
                    ImGui::TextWrapped("Visit this region to transfer treasury funds.");
                } else if (!status.world.adult || !RegionOpen(region, status.world)) {
                    ImGui::TextWrapped("Your charter and treasury are kept. Return as an adult after local recovery.");
                } else {
                    ImGui::SetNextItemWidth(170.0f);
                    ImGui::InputInt("Transfer amount", &transferAmount, 100, 1000);
                    transferAmount = std::clamp(transferAmount, 1, static_cast<int>(kTreasuryLimit));
                    const auto amount = static_cast<uint32_t>(transferAmount);
                    const auto deposit = PreviewTransfer(status, region, amount, true);
                    DrawTreasuryButton("Deposit from bank", TreasuryAction::Deposit, region, amount,
                                       deposit != StewardshipResult::Success, status);
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                        deposit != StewardshipResult::Success)
                        ImGui::SetTooltip("%s", ExplainStewardshipResult(deposit));
                    ImGui::SameLine();
                    const auto withdraw = PreviewTransfer(status, region, amount, false);
                    DrawTreasuryButton("Withdraw to bank", TreasuryAction::Withdraw, region, amount,
                                       withdraw != StewardshipResult::Success, status);
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                        withdraw != StewardshipResult::Success)
                        ImGui::SetTooltip("%s", ExplainStewardshipResult(withdraw));
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::TextWrapped("Save normally to keep charter purchases and treasury transfers.");
    if (!feedback.empty())
        ImGui::TextWrapped("%s", feedback.c_str());
    ImGui::PopID();
}

std::string StewardshipGreeting(ResidentId resident) {
    const auto status = GetStatus();
    if (!IsValidResident(resident) || !RealAdventure(status) || !status.world.adult || !IsValidState(status.economy) ||
        !IsValidStewardshipState(status.economy.stewardship) || status.economy.enabled != 1)
        return {};
    const Player* player = GET_PLAYER(gPlayState);
    Actor* speaker = player != nullptr ? player->talkActor : nullptr;
    if (speaker == nullptr || speaker->update == nullptr || !(player->stateFlags1 & PLAYER_STATE1_TALKING) ||
        GetSocialResidentId(speaker) != resident)
        return {};
    const auto& charters = status.economy.stewardship;
    const auto count = RegionalCharterCount(charters);
    if (count == 0)
        return {};

    if (resident == ResidentId::Zelda || resident == ResidentId::Aren || resident == ResidentId::Maelin) {
        if (!status.world.ganonDefeated || gPlayState->sceneNum != SCENE_OUTSIDE_GANONS_CASTLE)
            return {};
        if (count == kStewardshipRegionCount) {
            if (resident == ResidentId::Zelda)
                return "^All eight regions bear your charter. High Steward of Hyrule, let that title mean care "
                       "for the people who live there.";
            if (resident == ResidentId::Aren)
                return "^High Steward of Hyrule. Eight regional charters are recorded under your name; the "
                       "royal household recognizes the title.";
            return "^Eight charters, eight regional treasuries. The register names you High Steward of Hyrule.";
        }
        const std::string holdings = std::to_string(count) + (count == 1 ? " regional charter" : " regional charters");
        if (resident == ResidentId::Zelda)
            return "^You now hold " + holdings + ". Those places will need your patience as much as your coin.";
        if (resident == ResidentId::Aren)
            return "^The household register recognizes your " + holdings + ". Welcome, Steward.";
        return "^I have recorded " + holdings + " in your name. Each region keeps its own treasury.";
    }
    const Region region = ResidentRegion(resident);
    if (region >= Region::Count || status.currentRegion != region || !RegionOpen(region, status.world) ||
        !HoldsRegionalCharter(charters, static_cast<uint8_t>(region)))
        return {};
    switch (region) {
        case Region::Market:
            return "^Steward of Castle Town. We know who holds this district's charter; keep its working people "
                   "in mind when you count the day's rupees.";
        case Region::Field:
            return "^You hold the roadlands' charter now, Steward. A good journey begins with people who look "
                   "after the places between towns.";
        case Region::Ranch:
            return "^The ranch charter bears your name, Steward. Pastures and dairy work deserve steady care.";
        case Region::Forest:
            return "^You have the forest charter! Steward is a big word. I hope it means you will keep looking "
                   "after our little growing places.";
        case Region::Kakariko:
            return resident == ResidentId::Bram
                       ? "^You hold Kakariko's charter now. Please remember the households behind those deeds."
                       : "^Steward of Kakariko, welcome. The cottage and workshops are part of the same village; "
                         "it is good to see you taking an interest in all of them.";
        case Region::Mountain:
            return "^Mountain Steward! Stonework and kilnwork both carry your mark now. Strong foundations "
                   "need someone willing to tend them.";
        case Region::Water:
            return "^Your waterway charter is recognized here, Steward. The lake's workers and the river's "
                   "suppliers share more than the water between them.";
        case Region::Desert:
            return "^We recognize your caravan and textile charter, Steward. Our partnership is worth the care "
                   "you put into it.";
        default:
            return {};
    }
}

} // namespace LivingHyrule
