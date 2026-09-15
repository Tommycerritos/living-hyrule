#include "LivingHyrule.h"
#include "ChallengeMode.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <ship/window/gui/GuiWindow.h>

#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/cvar_prefixes.h"

namespace LivingHyrule {

// The ledger and resident conversations share the same validated transactions.
class LivingHyruleWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override;

  private:
    void DrawActionButton(const char* label, Action action, uint32_t amount, bool disabled, Status& status);
    void DrawBank(Status& status);
    void DrawCottage(Status& status);
    void DrawProperties(Status& status);

    int mAmount = 10;
    int mLastFileNum = -2;
    std::string mFeedback;
};

void LivingHyruleWindow::DrawActionButton(const char* label, Action action, uint32_t amount, bool disabled,
                                          Status& status) {
    ImGui::BeginDisabled(disabled || !status.canUseLedger);
    const bool clicked = ImGui::Button(label);
    ImGui::EndDisabled();
    if (clicked) {
        mFeedback = PerformAction(action, amount);
        // Every following control uses the resulting state, including a pending
        // wallet update. A second click must never reuse the previous balance.
        status = GetStatus();
    }
}

void LivingHyruleWindow::DrawBank(Status& status) {
    ImGui::Separator();
    ImGui::TextUnformatted("Your bank account");
    ImGui::Text("Wallet: %d / %d rupees", status.walletRupees, status.walletCapacity);
    ImGui::Text("Bank: %llu rupees", static_cast<unsigned long long>(status.economy.bankRupees));

    ImGui::SetNextItemWidth(170.0f);
    ImGui::InputInt("Amount", &mAmount, 10, 100);
    mAmount = std::clamp(mAmount, 1, static_cast<int>(kBankLimit));
    const uint32_t amount = static_cast<uint32_t>(mAmount);

    const uint64_t depositRoom = kBankLimit - std::min<uint64_t>(status.economy.bankRupees, kBankLimit);
    DrawActionButton("Deposit", Action::Deposit, amount,
                     !status.economy.enabled || amount > static_cast<uint32_t>(std::max(0, status.walletRupees)) ||
                         amount > depositRoom,
                     status);
    ImGui::SameLine();
    DrawActionButton("Withdraw", Action::Withdraw, amount,
                     !status.economy.enabled || amount > status.economy.bankRupees ||
                         amount > static_cast<uint32_t>(std::max(0, status.walletCapacity - status.walletRupees)),
                     status);

    const uint32_t wallet = static_cast<uint32_t>(std::max(0, status.walletRupees));
    const uint64_t allDepositRoom = kBankLimit - std::min<uint64_t>(status.economy.bankRupees, kBankLimit);
    DrawActionButton("Deposit all", Action::Deposit, wallet,
                     !status.economy.enabled || wallet == 0 || wallet > allDepositRoom, status);
    ImGui::SameLine();
    const uint32_t walletRoom = static_cast<uint32_t>(std::max(0, status.walletCapacity - status.walletRupees));
    const uint32_t fillAmount = static_cast<uint32_t>(std::min<uint64_t>(walletRoom, status.economy.bankRupees));
    DrawActionButton("Fill wallet", Action::Withdraw, fillAmount, !status.economy.enabled || fillAmount == 0, status);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Withdraw as much as your wallet can hold, up to your bank balance.");
    }

    if (status.economy.bankRupees >= kBankLimit) {
        ImGui::TextWrapped("Your bank is full. Withdraw rupees to make room for more.");
    }
}

void LivingHyruleWindow::DrawCottage(Status& status) {
    ImGui::Separator();
    ImGui::TextUnformatted("Kakariko Cottage");
    if (!status.cottageTradeOpen) {
        ImGui::TextWrapped("Kakariko rent and property sales resume after you clear the Shadow Temple.");
    }

    if (!status.economy.ownsKakarikoCottage) {
        ImGui::Text("Price: %u rupees from your bank account", static_cast<unsigned int>(kCottagePrice));
        if (status.cottageTradeOpen && !status.inKakariko) {
            ImGui::TextWrapped("Visit Kakariko Village to buy the cottage.");
        } else if (status.cottageTradeOpen && status.economy.bankRupees < kCottagePrice) {
            ImGui::TextWrapped("Save enough in your bank account to buy the cottage.");
        }
        DrawActionButton("Buy cottage", Action::BuyCottage, 0,
                         !status.economy.enabled || !status.inKakariko || !status.cottageTradeOpen ||
                             status.economy.bankRupees < kCottagePrice,
                         status);
    }

    if (status.economy.ownsKakarikoCottage) {
        ImGui::TextColored(ImVec4(0.65f, 0.85f, 0.55f, 1.0f), "Owned");
        const uint32_t elapsedFrames = std::min(status.economy.rentalFrames, kFramesPerRentPeriod);
        const float progress = static_cast<float>(elapsedFrames) / static_cast<float>(kFramesPerRentPeriod);
        const uint32_t remainingSeconds = static_cast<uint32_t>(
            (static_cast<uint64_t>(kFramesPerRentPeriod - elapsedFrames) * 600 + kFramesPerRentPeriod - 1) /
            kFramesPerRentPeriod);
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f),
                           status.cottageTradeOpen ? "Next rent payment" : "Rent progress saved");
        if (status.cottageTradeOpen) {
            ImGui::Text("Next rent in %u:%02u of active play", remainingSeconds / 60, remainingSeconds % 60);
        } else {
            ImGui::TextUnformatted("Rent suspended.");
            ImGui::Text("Rent resumes with %u:%02u remaining.", remainingSeconds / 60, remainingSeconds % 60);
        }
        ImGui::Text("Total rent earned: %llu rupees", static_cast<unsigned long long>(status.economy.totalRentEarned));
    }

    ImGui::TextWrapped("Rent: %u rupees into your bank every ten minutes of active play. No offline payout.",
                       static_cast<unsigned int>(kRentPerPeriod));
}

void LivingHyruleWindow::DrawProperties(Status& status) {
    ImGui::Separator();
    ImGui::TextUnformatted("Property and businesses across Hyrule");
    ImGui::TextWrapped("Visit a region to purchase its deeds. Income goes to your bank every ten minutes of active "
                       "play. Adult-era businesses need their region freed and their premises repaired.");
    ImGui::Text("Lifetime business income: %llu rupees",
                static_cast<unsigned long long>(status.economy.totalBusinessEarned));
    for (unsigned int region = 0; region < static_cast<unsigned int>(Region::Count); ++region) {
        if (!ImGui::CollapsingHeader(kRegionNames[region]))
            continue;
        const bool open = RegionOpen(static_cast<Region>(region), status.world);
        if (!open) {
            static constexpr const char* requirements[] = {
                "Defeat Ganon to reopen Castle Town trade.",
                "Clear the Forest Temple to secure the roads.",
                "Win Epona's freedom to reopen ranch trade.",
                "Clear the Forest Temple.",
                "Clear the Shadow Temple.",
                "Clear the Fire Temple.",
                "Clear the Water Temple. The Domain's ice is a separate reconstruction project.",
                "Earn Gerudo membership and clear the Spirit Temple as an adult."
            };
            ImGui::TextWrapped("%s", requirements[region]);
        }
        for (uint32_t id = 0; id < kProperties.size(); ++id) {
            const auto& property = kProperties[id];
            if (static_cast<unsigned int>(property.region) != region)
                continue;
            ImGui::PushID(static_cast<int>(id));
            ImGui::TextUnformatted(property.name);
            ImGui::Text("Price: %u | Income: %u rupees per period", property.price, property.income);
            const bool owned = OwnsProperty(status.economy, id);
            const bool local = status.currentRegion == property.region;
            if (!owned) {
                DrawActionButton(
                    "Buy deed", Action::BuyProperty, id,
                    !status.economy.enabled || !open || !local || status.economy.bankRupees < property.price, status);
            } else if (status.world.adult && !(status.economy.repairedProperties & (1u << id))) {
                ImGui::Text("Owned; repairs required: %u rupees", property.repairCost);
                DrawActionButton("Commission repairs", Action::RepairProperty, id,
                                 !status.economy.enabled || !open || !local ||
                                     status.economy.bankRupees < property.repairCost,
                                 status);
            } else {
                ImGui::TextUnformatted("Deed owned");
            }
            if (!local)
                ImGui::TextWrapped("Visit this region for purchases and repairs.");
            if (OwnsProperty(status.economy, id)) {
                const bool operating = status.economy.enabled && PropertyOperating(status.economy, id, status.world);
                ImGui::TextUnformatted(operating ? "Operating" : "Income suspended; your deed is kept");
                const float progress = static_cast<float>(status.economy.businessFrames[id]) / kFramesPerRentPeriod;
                ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "Income period");
            }
            ImGui::Spacing();
            ImGui::PopID();
        }
    }
}

void LivingHyruleWindow::DrawElement() {
    Status status = GetStatus();
    if (status.fileNum != mLastFileNum) {
        mLastFileNum = status.fileNum;
        mFeedback.clear();
        mAmount = 10;
    }

    ImGui::TextWrapped("Save rupees, own a cottage, and get to know the people of Hyrule.");
    bool residents = CVarGetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), 0) != 0;
    if (ImGui::Checkbox("Additional residents", &residents)) {
        CVarSetInteger(CVAR_ENHANCEMENT("LivingHyruleResidents"), residents ? 1 : 0);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    ImGui::TextWrapped(
        "Meet residents in Kakariko, Castle Town, Hyrule Field, Lon Lon Ranch, Kokiri Forest, Goron City, "
        "Zora's River, Lake Hylia, Gerudo Valley and Gerudo's Fortress. "
        "Work shifts and returning traders follow the region's story and business recovery.");
    DrawChallengeControls();
    ImGui::Separator();
    if (!status.loaded) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", status.reason);
        return;
    }

    ImGui::Text("Save file %d", status.fileNum + 1);
    const bool enabled = status.economy.enabled != 0;
    DrawActionButton(enabled ? "Pause economy" : "Enable economy", enabled ? Action::Disable : Action::Enable, 0, false,
                     status);
    if (status.economy.enabled) {
        ImGui::TextUnformatted("Economy active for this save file.");
    } else {
        ImGui::TextWrapped("Economy paused. Your savings, cottage, and rent progress are kept.");
    }

    if (!status.canUseLedger && status.reason != nullptr && status.reason[0] != '\0') {
        ImGui::TextWrapped("%s", status.reason);
    }

    DrawBank(status);
    DrawCottage(status);
    DrawProperties(status);

    ImGui::Separator();
    ImGui::TextWrapped("Save your game normally to keep your bank balance, deeds, repairs and income progress.");
    if (!mFeedback.empty()) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", mFeedback.c_str());
    }
}

static void RegisterLivingHyruleMenu() {
    auto gui = Ship::Context::GetRawInstance()->GetWindow()->GetGui();
    gui->AddGuiWindow(
        std::make_shared<LivingHyruleWindow>(CVAR_WINDOW("LivingHyrule"), "Living Hyrule", ImVec2(540.0f, 690.0f)));

    auto menu = SohGui::GetSohMenu();
    menu->AddSidebarEntry("Enhancements", "Living Hyrule", 1);
    WidgetPath path{ "Enhancements", "Living Hyrule", SECTION_COLUMN_1 };
    menu->AddWidget(path, "Open Living Hyrule", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("LivingHyrule"))
        .WindowName("Living Hyrule")
        .Options(UIWidgets::WindowButtonOptions()
                     .Tooltip("Manage savings, properties, residents and combat difficulty.")
                     .EmbedWindow(false));
}

static RegisterMenuInitFunc menuInit(RegisterLivingHyruleMenu);

} // namespace LivingHyrule
