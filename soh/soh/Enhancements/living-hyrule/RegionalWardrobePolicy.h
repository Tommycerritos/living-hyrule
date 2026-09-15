#pragma once

#include "Properties.h"
#include "RegionalWardrobeTypes.h"
#include <array>
#include <cstdint>
#include <type_traits>

namespace LivingHyrule {

// These identities and their bit positions are permanent once persisted.
enum class StyleId : uint8_t {
    Original = 0,
    Forest = 1,
    Mountain = 2,
    Water = 3,
    Desert = 4,
    Kakariko = 5,
    Ranch = 6,
    Field = 7,
    Market = 8,
};

struct WardrobeColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
struct RegionalStyle {
    StyleId id;
    const char* name;
    const char* description;
    Region region;
    uint32_t price;
    WardrobeColor color;
};

// Native Link clothing and hat keep their original shape. These are fabric
// colors in both eras, not armor, tunic equipment, or new protective effects.
inline constexpr std::array<RegionalStyle, kRegionalStyleCount> kRegionalStyles = { {
    { StyleId::Forest,
      "Kokiri fern dye",
      "A bright woodland green for familiar trails.",
      Region::Forest,
      150,
      { 76, 137, 71 } },
    { StyleId::Mountain,
      "Goron ember dye",
      "Warm clay orange inspired by the mountain kilns.",
      Region::Mountain,
      600,
      { 172, 74, 32 } },
    { StyleId::Water,
      "Zora river dye",
      "Clear turquoise inspired by the river shallows.",
      Region::Water,
      650,
      { 30, 135, 153 } },
    { StyleId::Desert,
      "Gerudo sand dye",
      "Golden sand cloth from the desert textile trade.",
      Region::Desert,
      800,
      { 191, 140, 67 } },
    { StyleId::Kakariko,
      "Kakariko slate dye",
      "A cool slate blue for a village working day.",
      Region::Kakariko,
      450,
      { 91, 99, 129 } },
    { StyleId::Ranch,
      "Lon Lon dusk dye",
      "Soft heather cloth for evenings at the ranch.",
      Region::Ranch,
      250,
      { 143, 87, 114 } },
    { StyleId::Field,
      "Caravan ochre dye",
      "Earthy ochre carried along Hyrule's trading roads.",
      Region::Field,
      300,
      { 136, 111, 52 } },
    { StyleId::Market,
      "Market festival dye",
      "Deep plum cloth for a town gathering.",
      Region::Market,
      1000,
      { 113, 51, 135 } },
} };

constexpr const RegionalStyle* GetRegionalStyle(uint8_t id) {
    return id >= 1 && id <= kRegionalStyleCount ? &kRegionalStyles[id - 1] : nullptr;
}

// The engine additionally owns the current save, safe transaction moment, and
// any vendor conversation. Region and story gates are rechecked here before the
// bank and independent wardrobe snapshot are changed together on the game thread.
inline Result BuyRegionalStyle(EconomyState& economy, WardrobeState& wardrobe, uint8_t id, Region currentRegion,
                               const WorldProgress& world) {
    if (!IsValidState(economy) || !IsValidWardrobeState(wardrobe))
        return Result::InvalidState;
    if (!economy.enabled)
        return Result::Disabled;
    const auto* style = GetRegionalStyle(id);
    if (style == nullptr)
        return Result::InvalidAmount;
    if (OwnsRegionalStyle(wardrobe, id))
        return Result::AlreadyOwned;
    if (currentRegion != style->region || !RegionOpen(style->region, world))
        return Result::Unavailable;
    if (economy.bankRupees < style->price)
        return Result::InsufficientBank;
    economy.bankRupees -= style->price;
    wardrobe.ownedStyles |= RegionalStyleBit(id);
    return Result::Success;
}

// A purchased dye works in both ages and remains available after leaving its
// region. Original appearance is always owned. Equipping never charges money.
inline Result EquipRegionalStyle(const EconomyState& economy, WardrobeState& wardrobe, uint8_t id) {
    if (!IsValidState(economy) || !IsValidWardrobeState(wardrobe))
        return Result::InvalidState;
    if (!economy.enabled)
        return Result::Disabled;
    if (id > kRegionalStyleCount)
        return Result::InvalidAmount;
    if (!OwnsRegionalStyle(wardrobe, id))
        return Result::NotOwned;
    wardrobe.equippedStyle = id;
    return Result::Success;
}

enum class WardrobeVisualStatus : uint8_t {
    Active,
    Original,
    Disabled,
    Unavailable,
    Unreadable,
    CosmeticOverride,
    CustomModel,
    NetworkAppearance,
};
struct WardrobeVisualContext {
    bool localPlayerOrPreview = false;
    bool readableEconomy = false;
    bool enabled = false;
    bool cosmeticOverride = false;
    bool customModel = false;
    bool anchorConnected = false;
};
inline WardrobeVisualStatus EvaluateRegionalWardrobe(const WardrobeState* wardrobe,
                                                     const WardrobeVisualContext& context) {
    if (!context.localPlayerOrPreview || wardrobe == nullptr)
        return WardrobeVisualStatus::Unavailable;
    if (!context.readableEconomy || !IsValidWardrobeState(*wardrobe))
        return WardrobeVisualStatus::Unreadable;
    if (!context.enabled)
        return WardrobeVisualStatus::Disabled;
    if (wardrobe->equippedStyle == 0)
        return WardrobeVisualStatus::Original;
    if (context.anchorConnected)
        return WardrobeVisualStatus::NetworkAppearance;
    if (context.cosmeticOverride)
        return WardrobeVisualStatus::CosmeticOverride;
    if (context.customModel)
        return WardrobeVisualStatus::CustomModel;
    return WardrobeVisualStatus::Active;
}

} // namespace LivingHyrule
