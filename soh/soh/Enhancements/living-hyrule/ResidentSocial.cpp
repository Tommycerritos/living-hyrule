#include "ResidentSocial.h"
#include "LivingHyrule.h"
#include "ResidentActor.h"
#include "WorldResidents.h"
#include "ForestMountainResidents.h"
#include "WaterDesertResidents.h"
#include "RoyalAudience.h"
#include "SocialPolicy.h"
#include "RoyalProgressionPolicy.h"
#include "soh/SaveManager.h"

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {

ResidentId GetSocialResidentId(const Actor* actor) {
    if (IsResidentActor(actor)) {
        switch (GetResidentRole(actor)) {
            case ResidentRole::Carpenter:
                return ResidentId::Tavin;
            case ResidentRole::Tenant:
                return ResidentId::Bram;
            case ResidentRole::Supplier:
                return ResidentId::Orlen;
            default:
                return ResidentId::Count;
        }
    }
    // The family APIs validate params before these arrays are indexed. Explicit
    // maps keep save identities independent of ActorDB registration order.
    constexpr ResidentId world[] = { ResidentId::Vessa, ResidentId::Hadrin, ResidentId::Pella,
                                     ResidentId::Caro,  ResidentId::Hollis, ResidentId::Nessa,
                                     ResidentId::Wren,  ResidentId::Vero,   ResidentId::Edda };
    constexpr ResidentId forest[] = { ResidentId::Fenn, ResidentId::Luma, ResidentId::Doron, ResidentId::Brakka };
    constexpr ResidentId water[] = { ResidentId::Lethra, ResidentId::Neris, ResidentId::Rasha, ResidentId::Kesra };
    constexpr ResidentId royal[] = { ResidentId::Zelda, ResidentId::Aren, ResidentId::Maelin };
    static_assert(std::size(world) == static_cast<size_t>(WorldResidentId::Count));
    static_assert(std::size(forest) == static_cast<size_t>(ForestMountainResidentId::Count));
    static_assert(std::size(water) == static_cast<size_t>(WaterDesertResidentId::Count));
    static_assert(std::size(royal) == static_cast<size_t>(RoyalResidentId::Count));
    if (IsWorldResidentActor(actor))
        return world[static_cast<size_t>(GetWorldResidentId(actor))];
    if (IsForestMountainResidentActor(actor))
        return forest[static_cast<size_t>(GetForestMountainResidentId(actor))];
    if (IsWaterDesertResidentActor(actor))
        return water[static_cast<size_t>(GetWaterDesertResidentId(actor))];
    if (IsRoyalResidentActor(actor))
        return royal[static_cast<size_t>(GetRoyalResidentId(actor))];
    return ResidentId::Count;
}

std::string ResidentGreeting(Actor* actor) {
    const ResidentId id = GetSocialResidentId(actor);
    const Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
    auto& economy = gSaveContext.ship.livingHyrule;
    if (!IsValidResident(id) || actor->update == nullptr || player == nullptr || player->talkActor != actor ||
        !(player->stateFlags1 & PLAYER_STATE1_TALKING) || !(IS_VANILLA || IS_MASTER_QUEST) || IS_CUTSCENE_LAYER ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2 ||
        SaveManager::Instance == nullptr || !SaveManager::Instance->SaveFile_Exist(gSaveContext.fileNum) ||
        !IsValidState(economy) || economy.enabled != 1)
        return {};
    const bool known = HasMetResident(economy, id);
    MarkResidentMet(economy, id);
    if (id == ResidentId::Zelda) {
        const uint8_t newDeeds = RecognizeRoyalDeeds(economy, GetWorldProgress());
        if (newDeeds != 0) {
            std::string text = "^Word of your work has reached the household: ";
            unsigned int mentioned = 0, remaining = 0;
            for (uint8_t bit = 0; bit < kRoyalDeedNames.size(); ++bit) {
                if ((newDeeds & (1u << bit)) == 0)
                    continue;
                if (mentioned == 2) {
                    ++remaining;
                    continue;
                }
                if (mentioned != 0)
                    text += ", ";
                text += kRoyalDeedNames[bit];
                ++mentioned;
            }
            if (remaining != 0)
                text += ", and " + std::to_string(remaining) + " other deeds";
            return text + ". I will remember the care you have shown Hyrule.";
        }
        if (economy.castleEstateOwned)
            return "^This estate is in your care now. My household remains here with you; let these gardens be a place "
                   "where Hyrule can gather again.";
        if (GetRapport(economy, id) >= kRoyalTrustedRapport)
            return "^You have earned the household's trust. Maelin will reduce the castle estate price by a tenth when "
                   "your regional work is complete.";
    }
    const int rapport = GetRapport(economy, id);
    if (id == ResidentId::Bram && rapport <= -10)
        return "^That rent leaves little for leather and thread. I am falling behind. Please give me room to work.";
    if (id == ResidentId::Bram && rapport < 0)
        return "^The higher rent is hard on my trade. A full purse is not the only thing a household needs.";
    if (rapport >= kTrustedRapport) {
        if (economy.zoraRestored && (id == ResidentId::Lethra || id == ResidentId::Neris))
            return "^You funded the work to free our ordinary pools and waterfalls. The red ice is another matter, but "
                   "the Domain has a future again.";
        if (economy.marketRestored) {
            switch (id) {
                case ResidentId::Zelda:
                    return "^You helped give the Market its streets back. Hyrule needs that kind of care as much "
                           "as it needed courage. You are always welcome here.";
                case ResidentId::Aren:
                    return "^Your work in the Market gives our watch something worth protecting. The household "
                           "knows you as a friend.";
                case ResidentId::Maelin:
                    return "^The Market restoration is in the records. I have put your name beside it with pride.";
                case ResidentId::Vessa:
                    return "^Your investment gives this square a future. I will do my part to fill it with good food.";
                case ResidentId::Hadrin:
                    return "^You funded the square's restoration. That is more than a deed in a book. People will "
                           "walk those streets because you cared.";
                case ResidentId::Pella:
                    return "^I can picture lamps along restored streets again. You have given us good work to do.";
                default:
                    break;
            }
        }
        constexpr const char* trusted[] = {
            "You carried those measurements carefully. I trust you to see a job through.",
            "You have treated this household fairly. There is always a place to rest your feet here.",
            "Hollis got the invoice. With you on the road, I can keep the yard moving.",
            "Caro's notice spared the growers a wasted journey. I remember a kindness like that.",
            "Pella says the lamps are arranged. You look after the things people overlook.",
            "The lamp request arrived safely. Our evening work goes easier thanks to you.",
            "Vessa received the growers' notice. You have the makings of a dependable courier.",
            "Orlen's invoice saved an argument over the timber. I am glad you stopped by.",
            "Wren has the feed figures now. Good care begins with someone paying attention.",
            "The feed tally is in order. The animals may not say thank you, so I will.",
            "Edda has our catch notes. A good neighbor carries more than their own load.",
            "Your observations helped join the fishermen's work with ours. Keep watching the water.",
            "Luma got the seeds! A little care can grow into something much bigger.",
            "Fenn's seeds are safe with me. You belong among friends in this clearing.",
            "Brakka received the stone. Strong work begins with trust as much as muscle.",
            "That sample was just what the kiln needed. You keep your word, little brother.",
            "Neris has the readings. You helped us listen to the river together.",
            "Lethra's figures filled a gap in my notes. You have been a patient friend to the river.",
            "Kesra received the cloth tally. A promise kept travels farther than a caravan.",
            "The cloth count matches Rasha's tally. You have earned a welcome at my workbench.",
            "Hyrule remembers the people who help it heal.",
            "The watch knows you as a friend of this household.",
            "There is room in these records for kindness as well as coin."
        };
        static_assert(std::size(trusted) == kSocialResidentCount);
        constexpr uint8_t residentFavor[] = { 1, 1, 2, 3, 4, 4, 3, 2, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 0, 0, 0 };
        static_assert(std::size(residentFavor) == kSocialResidentCount);
        const uint8_t favor = residentFavor[static_cast<size_t>(id)];
        if (favor != 0 && !FavorCompleted(economy, favor))
            return "^You have shown care for our work. I count you as a friend, and I will remember your kindness.";
        return "^" + std::string(trusted[static_cast<size_t>(id)]);
    }
    return known ? "^It is good to see a familiar face." : std::string{};
}

} // namespace LivingHyrule
