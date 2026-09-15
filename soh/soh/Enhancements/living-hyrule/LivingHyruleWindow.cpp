#include "LivingHyrule.h"
#include "ChallengeMode.h"
#include "SocialPolicy.h"
#include "MarketRestoration.h"
#include "RegionalWardrobe.h"
#include "Stewardship.h"
#include "ResidentGiftsPolicy.h"
#include "RoyalProgressionPolicy.h"
#include "RoyalEstate.h"
#include "ZoraRestoration.h"

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
    void DrawJournal(Status& status);
    void DrawRestoration(Status& status);
    void DrawWardrobe(Status& status);
    void DrawDomainRestoration(Status& status);
    void DrawRoyalEstate(Status& status);

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
        ImGui::Text("Current period: %s | Expected collection: %u rupees",
                    status.economy.currentPeriodPolicy ? "high rent" : "fair rent",
                    EffectiveCottageRent(status.economy));
        ImGui::Text("Next period: %s", status.economy.cottageRentPolicy ? "high rent (40)" : "fair rent (25)");
        ImGui::TextWrapped("Fair rent slowly builds Bram's trust. High rent costs trust each time it is collected; "
                           "at -10 trust or below Bram can pay only 15. A change applies after the current period.");
        DrawActionButton(status.economy.cottageRentPolicy ? "Choose fair rent" : "Choose high rent",
                         status.economy.cottageRentPolicy ? Action::SetFairRent : Action::SetHighRent, 0,
                         !status.economy.enabled, status);
    }

    ImGui::TextWrapped("Rent goes into your bank every ten minutes of active play. No offline payout.");
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
                const uint32_t repairPrice = EffectiveRepairPrice(status.economy, id);
                ImGui::Text("Owned; repairs required: %u rupees", repairPrice);
                if (repairPrice < property.repairCost)
                    ImGui::TextUnformatted("Trusted customer: 10% repair discount");
                DrawActionButton("Commission repairs", Action::RepairProperty, id,
                                 !status.economy.enabled || !open || !local || status.economy.bankRupees < repairPrice,
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

void LivingHyruleWindow::DrawJournal(Status& status) {
    ImGui::Separator();
    if (!ImGui::CollapsingHeader("People and favors", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    ImGui::TextWrapped("Speak to residents to accept and deliver favors. Choose Something else during a "
                       "conversation to move between business, favors and gifts. Repeat greetings do not earn trust.");
    unsigned int completed = 0;
    for (uint8_t id = 1; id <= kFavorCount; ++id)
        completed += FavorCompleted(status.economy, id) ? 1u : 0u;
    ImGui::Text("Favors completed: %u / %u", completed, static_cast<unsigned int>(kFavorCount));
    if (const auto* favor = GetFavor(status.economy.activeFavor); favor != nullptr) {
        ImGui::Text("Current favor: %s", favor->name);
        ImGui::TextWrapped("%s", favor->instructions);
        ImGui::TextWrapped("Deliveries are recorded in this journal. They do not replace an adventure item.");
        DrawActionButton("Cancel this delivery", Action::AbandonFavor, 0, !status.economy.enabled, status);
    } else {
        ImGui::TextUnformatted("No delivery in progress.");
    }
    ImGui::TextWrapped("Each completed favor earns 10 trust with its sender and recipient, once. A manager who "
                       "trusts you gives a 10%% repair discount. A first paid repair earns 5 trust.");
    unsigned int met = 0;
    for (uint32_t index = 0; index < kSocialResidentCount; ++index) {
        const auto id = static_cast<ResidentId>(index);
        if (!HasMetResident(status.economy, id))
            continue;
        ++met;
        const int rapport = GetRapport(status.economy, id);
        ImGui::BulletText("%s: %s (%+d)", GetSocialResidentName(id),
                          rapport >= kTrustedRapport ? "trusted"
                          : rapport < 0              ? "strained"
                                                     : "acquainted",
                          rapport);
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::TreeNode("Gifts remembered")) {
            for (uint8_t kind = 0; kind < kGiftKinds; ++kind) {
                const auto gift = static_cast<GiftKind>(kind);
                ImGui::BulletText("%s: %s | %u bank rupees", GiftNameFor(id, gift),
                                  GiftAlreadyGiven(status.economy, id, gift) ? "already given"
                                                                             : "available in conversation",
                                  kResidentGifts[kind].price);
            }
            ImGui::TextWrapped(
                "Each gift can be given once to this person. A preferred gift earns 8 trust; others earn 4.");
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (met == 0)
        ImGui::TextWrapped("Your journal will remember the Living Hyrule residents you speak with.");
    if (HasMetResident(status.economy, ResidentId::Zelda) && ImGui::TreeNode("Zelda's record of your work")) {
        for (uint8_t bit = 0; bit < kRoyalDeedNames.size(); ++bit)
            ImGui::BulletText("%s: %s", kRoyalDeedNames[bit],
                              (status.economy.royalRecognition & (1u << bit)) != 0 ? "recognized"
                                                                                   : "not yet recognized");
        ImGui::TextWrapped("After your victory, Zelda recognizes completed work when you visit. Each deed earns 5 "
                           "trust once. At 50 trust the household grants a 10%% estate discount.");
        ImGui::TreePop();
    }
}

void LivingHyruleWindow::DrawDomainRestoration(Status& status) {
    ImGui::Separator();
    if (!ImGui::CollapsingHeader("Restoring Zora's Domain"))
        return;
    ImGui::TextWrapped("Clear the Water Temple and use its blue warp, then fund work on the Domain's ordinary pools "
                       "and waterfalls. Lethra by the river can commission it. King Zora's red ice, the shop's ice and "
                       "the Lake shortcut stay separate.");
    if (status.economy.zoraRestored) {
        ImGui::TextWrapped(IsZoraRestorationActive()
                               ? "The Domain's water restoration is active on this visit."
                               : "Restoration is funded. Enter the Domain again as an adult to see the work.");
        return;
    }
    ImGui::Text("Investment: %u bank rupees", kZoraRestorationPrice);
    const auto readiness = GetZoraRestorationReadiness();
    if (readiness != ZoraRestorationReadiness::Ready)
        ImGui::TextWrapped("%s", ZoraRestorationReadinessText(readiness));
    if (status.currentRegion != Region::Water)
        ImGui::TextWrapped("Visit the river, Domain or lake to fund this work.");
    DrawActionButton("Fund Domain water restoration", Action::RestoreZora, 0,
                     readiness != ZoraRestorationReadiness::Ready || status.currentRegion != Region::Water ||
                         status.economy.bankRupees < kZoraRestorationPrice,
                     status);
}

void LivingHyruleWindow::DrawRoyalEstate(Status& status) {
    ImGui::Separator();
    // A return route remains visible and independent of the optional ledger.
    if (IsRoyalEstateActive() && ImGui::Button("Return to the castle approach")) {
        mFeedback = PerformAction(Action::ReturnEstate);
        status = GetStatus();
    }
    if (!ImGui::CollapsingHeader("The royal household and castle estate"))
        return;
    ImGui::TextWrapped("After your victory and the Market restoration, Captain Aren can lead you to the royal garden. "
                       "Zelda and Maelin receive visitors there by day; Aren keeps watch overnight. The garden "
                       "entrance and this window both offer a route back.");
    const auto readiness = GetRoyalEstateReadiness();
    if (readiness != RoyalEstateReadiness::Ready)
        ImGui::TextWrapped("%s", RoyalEstateReadinessText(readiness));
    if (!IsRoyalEstateActive())
        DrawActionButton("Visit the royal garden", Action::EnterEstate, 0, readiness != RoyalEstateReadiness::Ready,
                         status);
    if (!IsValidState(status.economy))
        return;
    if (status.economy.castleEstateOwned) {
        ImGui::TextUnformatted("Castle estate: owned.");
        ImGui::TextWrapped("Zelda and the household remain at home here. Your estate deed and standing are recorded "
                           "with your other holdings.");
        return;
    }
    ImGui::Text("Castle estate: %u bank rupees", CastleEstatePrice(status.economy));
    ImGui::TextWrapped(
        "Hold all eight regional charters, restore the Market and visit the royal garden to purchase the estate. The "
        "royal household stays. The deed includes the garden estate; further castle rooms still need rebuilding.");
    if (GetRapport(status.economy, ResidentId::Zelda) >= kRoyalTrustedRapport)
        ImGui::TextUnformatted("Household friendship: 10% estate discount applied.");
    const auto eligibility = CastleEstateEligibility(status.economy, status.world,
                                                     IsRoyalEstateActive() && readiness == RoyalEstateReadiness::Ready);
    DrawActionButton("Purchase castle estate", Action::BuyCastleEstate, 0, eligibility != Result::Success, status);
}

void LivingHyruleWindow::DrawRestoration(Status& status) {
    ImGui::Separator();
    if (!ImGui::CollapsingHeader("Rebuilding Castle Town"))
        return;
    ImGui::TextWrapped("After Ganon's defeat, Hadrin can commission the Market square's streets and facades. "
                       "The work appears when you leave and return. Shop interiors and alleys remain closed.");
    if (status.economy.marketRestored) {
        ImGui::TextUnformatted("Market restoration funded.");
        ImGui::TextWrapped("Visit the Market again to see the work. The castle is a separate project.");
    } else {
        ImGui::Text("Investment: %u bank rupees", kMarketRestorationPrice);
        const auto readiness = GetMarketRestorationReadiness();
        if (readiness != MarketRestorationReadiness::Ready)
            ImGui::TextWrapped("%s", MarketRestorationReadinessText(readiness));
        DrawActionButton("Fund Market restoration", Action::RestoreMarket, 0,
                         readiness != MarketRestorationReadiness::Ready ||
                             status.economy.bankRupees < kMarketRestorationPrice,
                         status);
    }
    ImGui::TextWrapped("Zelda receives visitors on the castle approach by day after your victory. Captain Aren "
                       "keeps watch overnight, and Maelin records the kingdom's working livelihoods.");
}

void LivingHyruleWindow::DrawWardrobe(Status& status) {
    ImGui::Separator();
    if (!ImGui::CollapsingHeader("Regional clothing dyes"))
        return;
    ImGui::TextWrapped(
        "Buy local cloth colors while visiting their region, then switch freely between owned "
        "dyes. They color Link's native clothing and hat in both ages. Equipment protection stays the same.");
    ImGui::TextWrapped("%s", RegionalWardrobeVisualStatusText(GetRegionalWardrobeVisualStatus()));
    DrawActionButton("Use original appearance", Action::EquipDye, 0,
                     !status.economy.enabled || status.economy.wardrobe.equippedStyle == 0, status);
    ImGui::TextWrapped(
        "Existing custom Link models, tunic cosmetic colors and connected Anchor appearances take priority.");
    for (const auto& style : kRegionalStyles) {
        const auto id = static_cast<uint8_t>(style.id);
        ImGui::PushID(1000 + static_cast<int>(id));
        const ImVec4 color(style.color.r / 255.0f, style.color.g / 255.0f, style.color.b / 255.0f, 1.0f);
        ImGui::ColorButton("Cloth color", color, ImGuiColorEditFlags_NoTooltip, ImVec2(18.0f, 18.0f));
        ImGui::SameLine();
        ImGui::TextUnformatted(style.name);
        ImGui::TextWrapped("%s", style.description);
        if (OwnsRegionalStyle(status.economy.wardrobe, id)) {
            if (status.economy.wardrobe.equippedStyle == id)
                ImGui::TextUnformatted("Selected");
            else
                DrawActionButton("Wear this color", Action::EquipDye, id, !status.economy.enabled, status);
        } else {
            ImGui::Text("%u bank rupees | %s", style.price, kRegionNames[static_cast<uint8_t>(style.region)]);
            DrawActionButton("Buy dye", Action::BuyDye, id,
                             !status.economy.enabled || status.currentRegion != style.region ||
                                 !RegionOpen(style.region, status.world) || status.economy.bankRupees < style.price,
                             status);
        }
        ImGui::Spacing();
        ImGui::PopID();
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
        "Work shifts and returning traders follow the region's story and business recovery. "
        "After Ganon's defeat, Zelda and the royal household receive visitors on the castle approach.");
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
    DrawJournal(status);
    DrawProperties(status);
    DrawRestoration(status);
    DrawDomainRestoration(status);
    DrawRoyalEstate(status);
    DrawWardrobe(status);
    DrawStewardshipControls(status);

    ImGui::Separator();
    ImGui::TextWrapped("Save normally to keep your bank, deeds, repairs, gifts, relationships, favors, restoration, "
                       "clothing and regional treasuries.");
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
