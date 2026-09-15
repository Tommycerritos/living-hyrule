#pragma once

#include "Economy.h"
#include <array>

namespace LivingHyrule {

enum class Region : uint8_t { Market, Field, Ranch, Forest, Kakariko, Mountain, Water, Desert, Count };
struct Property {
    const char* name;
    Region region;
    uint32_t price;
    uint32_t income;
    uint32_t repairCost;
};

// Indices are permanent save identities. Append a new schema to change capacity;
// never reorder these entries. These deeds do not evict original quest actors.
inline constexpr std::array<Property, 16> kProperties = { {
    { "Market produce stall", Region::Market, 3600, 90, 900 },
    { "Market guesthouse", Region::Market, 9000, 225, 2250 },
    { "South road orchard", Region::Field, 1800, 45, 450 },
    { "Caravan supply yard", Region::Field, 3200, 80, 800 },
    { "Lon Lon pasture lease", Region::Ranch, 6000, 150, 1500 },
    { "Lon Lon dairy partnership", Region::Ranch, 8000, 200, 2000 },
    { "Kokiri seed garden", Region::Forest, 600, 15, 150 },
    { "Kokiri woodland workshop", Region::Forest, 1000, 25, 250 },
    { "Kakariko cloth workshop", Region::Kakariko, 2400, 60, 600 },
    { "Kakariko builders' yard", Region::Kakariko, 4000, 100, 1000 },
    { "Goron stoneworks", Region::Mountain, 4800, 120, 1200 },
    { "Goron kiln partnership", Region::Mountain, 6400, 160, 1600 },
    { "Lake fishing cooperative", Region::Water, 2800, 70, 700 },
    { "Zora waterway supplies", Region::Water, 4400, 110, 1100 },
    { "Gerudo caravan partnership", Region::Desert, 7200, 180, 1800 },
    { "Gerudo textile workshop", Region::Desert, 5600, 140, 1400 },
} };
inline constexpr std::array<const char*, 8> kRegionNames = { "Castle Town",         "Hyrule Field", "Lon Lon Ranch",
                                                             "Kokiri Forest",       "Kakariko",     "Death Mountain",
                                                             "Lake and Zora lands", "Gerudo lands" };

struct WorldProgress {
    bool adult = false;
    bool forest = false;
    bool fire = false;
    bool water = false;
    bool shadow = false;
    bool spirit = false;
    bool gerudoMembership = false;
    bool ranchFreed = false;
    bool ganonDefeated = false;
};

inline bool ShouldClearMarketThreats(const WorldProgress& world, bool enabled, bool marketRuins) {
    return enabled && marketRuins && world.adult && world.ganonDefeated;
}

inline bool RegionOpen(Region region, const WorldProgress& world) {
    if (region >= Region::Count)
        return false;
    if (region == Region::Desert)
        return world.adult && world.gerudoMembership && world.spirit;
    if (!world.adult)
        return true;
    switch (region) {
        case Region::Market:
            return world.ganonDefeated;
        case Region::Field:
            return world.forest;
        case Region::Ranch:
            return world.ranchFreed;
        case Region::Forest:
            return world.forest;
        case Region::Kakariko:
            return world.shadow;
        case Region::Mountain:
            return world.fire;
        case Region::Water:
            return world.water;
        default:
            return false;
    }
}

inline bool OwnsProperty(const EconomyState& state, uint32_t id) {
    return id < kProperties.size() && (state.ownedProperties & (1u << id)) != 0;
}
inline constexpr std::array<ResidentId, 16> kPropertyManagers = {
    ResidentId::Vessa, ResidentId::Hadrin, ResidentId::Caro,  ResidentId::Hollis, ResidentId::Nessa, ResidentId::Wren,
    ResidentId::Fenn,  ResidentId::Luma,   ResidentId::Count, ResidentId::Tavin,  ResidentId::Doron, ResidentId::Brakka,
    ResidentId::Vero,  ResidentId::Lethra, ResidentId::Rasha, ResidentId::Kesra,
};
constexpr ResidentId PropertyManager(uint32_t id) {
    return id < kPropertyManagers.size() ? kPropertyManagers[id] : ResidentId::Count;
}
inline uint32_t EffectiveRepairPrice(const EconomyState& state, uint32_t id) {
    if (!IsValidState(state) || id >= kProperties.size())
        return 0;
    const uint32_t price = kProperties[id].repairCost;
    return GetRapport(state, PropertyManager(id)) >= kTrustedRapport
               ? static_cast<uint32_t>((static_cast<uint64_t>(price) * 90u) / 100u)
               : price;
}
inline bool PropertyOperating(const EconomyState& state, uint32_t id, const WorldProgress& world) {
    return OwnsProperty(state, id) && RegionOpen(kProperties[id].region, world) &&
           (!world.adult || (state.repairedProperties & (1u << id)) != 0);
}
inline Result BuyProperty(EconomyState& state, uint32_t id, const WorldProgress& world) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (id >= kProperties.size())
        return Result::InvalidAmount;
    if (OwnsProperty(state, id))
        return Result::AlreadyOwned;
    if (!RegionOpen(kProperties[id].region, world))
        return Result::Unavailable;
    if (state.bankRupees < kProperties[id].price)
        return Result::InsufficientBank;
    state.bankRupees -= kProperties[id].price;
    state.ownedProperties |= 1u << id;
    return Result::Success;
}
inline Result RepairProperty(EconomyState& state, uint32_t id, const WorldProgress& world) {
    if (!IsValidState(state))
        return Result::InvalidState;
    if (!state.enabled)
        return Result::Disabled;
    if (id >= kProperties.size())
        return Result::InvalidAmount;
    if (!OwnsProperty(state, id))
        return Result::NotOwned;
    if (!world.adult || !RegionOpen(kProperties[id].region, world))
        return Result::Unavailable;
    if ((state.repairedProperties & (1u << id)) != 0)
        return Result::AlreadyRepaired;
    const uint32_t repairPrice = EffectiveRepairPrice(state, id);
    if (state.bankRupees < repairPrice)
        return Result::InsufficientBank;
    state.bankRupees -= repairPrice;
    state.repairedProperties |= 1u << id;
    if (repairPrice != 0 && IsValidResident(PropertyManager(id)))
        AdjustRapport(state, PropertyManager(id), 5);
    return Result::Success;
}

// Each property retains its own progress while its region is closed. Payments
// consume their period even when the bank is full, with no hidden back payments.
inline uint32_t TickBusinesses(EconomyState& state, const WorldProgress& world) {
    if (!IsValidState(state) || !state.enabled)
        return 0;
    uint32_t credited = 0;
    for (uint32_t id = 0; id < kProperties.size(); ++id) {
        if (!PropertyOperating(state, id, world))
            continue;
        if (++state.businessFrames[id] < kFramesPerRentPeriod)
            continue;
        state.businessFrames[id] = 0;
        const uint64_t room = kBankLimit - state.bankRupees;
        const uint32_t payment = room < kProperties[id].income ? static_cast<uint32_t>(room) : kProperties[id].income;
        state.bankRupees += payment;
        credited += payment;
        if (world.adult)
            CreditRegionalDues(state.stewardship, static_cast<uint8_t>(kProperties[id].region), kProperties[id].income);
    }
    const uint64_t earningsRoom = (std::numeric_limits<uint64_t>::max)() - state.totalBusinessEarned;
    state.totalBusinessEarned += earningsRoom < credited ? earningsRoom : credited;
    return credited;
}

} // namespace LivingHyrule
