#include "RegionalWardrobe.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Network/Anchor/Anchor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include <cstdarg>
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "soh_assets.h"
#include "objects/object_link_boy/object_link_boy.h"
#include "objects/object_link_child/object_link_child.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

bool HasCustomClothingModel(int tunic) {
    if (tunic < PLAYER_TUNIC_KOKIRI || tunic > PLAYER_TUNIC_ZORA)
        return true;
    const bool adult = LINK_IS_ADULT;
    const char* base = adult ? gLinkAdultSkel : gLinkChildSkel;
    const char* variants[2][3] = {
        { gLinkChildKokiriTunicSkel, gLinkChildGoronTunicSkel, gLinkChildZoraTunicSkel },
        { gLinkAdultKokiriTunicSkel, gLinkAdultGoronTunicSkel, gLinkAdultZoraTunicSkel },
    };
    const char* variant = variants[adult ? 1 : 0][tunic];
    const bool alternatives = ResourceMgr_IsAltAssetsEnabled();
    // Tunic-specific custom skeletons can exist without replacing the base
    // skeleton, so Player_IsCustomLinkModel alone is insufficient here.
    return ResourceGetIsCustomByName(base) || (alternatives && ResourceMgr_FileAltExists(base)) ||
           ResourceMgr_FileExists(variant) || (alternatives && ResourceMgr_FileAltExists(variant));
}

WardrobeVisualContext VisualContext(PlayState* play, const void* drawing, int tunic, bool pausePreview) {
    WardrobeVisualContext context;
    context.localPlayerOrPreview = play != nullptr && play == gPlayState && GameInteractor::IsSaveLoaded(false) &&
                                   (IS_VANILLA || IS_MASTER_QUEST) &&
                                   (pausePreview || drawing == static_cast<void*>(GET_PLAYER(play)));
    if (!context.localPlayerOrPreview)
        return context;
    const auto& economy = gSaveContext.ship.livingHyrule;
    context.readableEconomy = IsValidState(economy);
    context.enabled = economy.enabled == 1;
    context.anchorConnected = Anchor::Instance != nullptr && Anchor::Instance->isConnected;
    const char* changed[] = { CVAR_COSMETIC("Link.KokiriTunic.Changed"), CVAR_COSMETIC("Link.GoronTunic.Changed"),
                              CVAR_COSMETIC("Link.ZoraTunic.Changed") };
    context.cosmeticOverride =
        tunic >= PLAYER_TUNIC_KOKIRI && tunic <= PLAYER_TUNIC_ZORA && CVarGetInteger(changed[tunic], 0) != 0;
    context.customModel = HasCustomClothingModel(tunic);
    return context;
}

void ApplyRegionalColor(GIVanillaBehavior, bool* should, va_list originalArgs) {
    if (should == nullptr || !*should)
        return;
    va_list args;
    va_copy(args, originalArgs);
    // Player_DrawImpl retains the original two arguments and appends drawing
    // context. Its first argument is a small equipment array in the pause
    // preview, so compare raw identity and never cast/dereference it as Player.
    const void* drawing = va_arg(args, void*);
    Color_RGB8* color = va_arg(args, Color_RGB8*);
    PlayState* play = va_arg(args, PlayState*);
    const int tunic = va_arg(args, int);
    const bool pausePreview = va_arg(args, int) != 0;
    va_end(args);
    if (color == nullptr)
        return;
    const auto context = VisualContext(play, drawing, tunic, pausePreview);
    if (!context.localPlayerOrPreview)
        return;
    const WardrobeState* wardrobe = GetRegionalWardrobeStateForSave();
    if (EvaluateRegionalWardrobe(wardrobe, context) != WardrobeVisualStatus::Active)
        return;
    const auto& dye = GetRegionalStyle(wardrobe->equippedStyle)->color;
    // This pointer addresses Player_DrawImpl's stack copy, never sTunicColors,
    // cosmetic CVars, shared display lists, or the actual equipped tunic.
    *color = { dye.r, dye.g, dye.b };
}

RegisterShipInitFunc initWardrobe(RegisterRegionalWardrobe);
} // namespace

WardrobeVisualStatus GetRegionalWardrobeVisualStatus() {
    if (!GameInteractor::IsSaveLoaded(false))
        return WardrobeVisualStatus::Unavailable;
    const auto context = VisualContext(gPlayState, GET_PLAYER(gPlayState),
                                       TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC)), false);
    return EvaluateRegionalWardrobe(GetRegionalWardrobeStateForSave(), context);
}

const char* RegionalWardrobeVisualStatusText(WardrobeVisualStatus status) {
    switch (status) {
        case WardrobeVisualStatus::Active:
            return "Your selected regional dye colors Link's native clothing and hat. Equipment protection is "
                   "unchanged.";
        case WardrobeVisualStatus::Original:
            return "Original clothing appearance is selected.";
        case WardrobeVisualStatus::Disabled:
            return "Living Hyrule is paused. Your purchased dyes and selection are kept.";
        case WardrobeVisualStatus::Unreadable:
            return "This wardrobe could not be read. Its stored data must be preserved.";
        case WardrobeVisualStatus::CosmeticOverride:
            return "Your existing tunic cosmetic color takes priority. The regional dye remains selected.";
        case WardrobeVisualStatus::CustomModel:
            return "Your custom Link model takes priority. Regional dyes use the native clothing model.";
        case WardrobeVisualStatus::NetworkAppearance:
            return "Anchor's player color takes priority while connected. Your regional dye remains selected.";
        default:
            return "Load a normal adventure or Master Quest to use the regional wardrobe.";
    }
}

void RegisterRegionalWardrobe() {
    static bool registered = false;
    if (registered || GameInteractor::Instance == nullptr)
        return;
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnVanillaBehavior>(VB_APPLY_TUNIC_COLOR,
                                                                                       ApplyRegionalColor);
    registered = true;
}

} // namespace LivingHyrule
