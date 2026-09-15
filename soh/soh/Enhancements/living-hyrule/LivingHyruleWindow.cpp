#include "LivingHyrule.h"

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

// This first prototype buys the cottage through a ledger. It does not add a
// physical seller NPC, change an existing NPC's dialogue, or add a cottage interior.
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
        "Meet Tavin, Bram, and Orlen in Kakariko during the day. Their presence and conversations change "
        "as the village recovers.");
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

    ImGui::Separator();
    ImGui::TextWrapped("Save your game normally to save your bank balance, cottage, and rent progress.");
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
                     .Tooltip("Manage your bank account and Kakariko Cottage.")
                     .EmbedWindow(false));
}

static RegisterMenuInitFunc menuInit(RegisterLivingHyruleMenu);

} // namespace LivingHyrule
