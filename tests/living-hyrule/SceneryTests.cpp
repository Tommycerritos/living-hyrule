#include "SceneryPolicy.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace {
using namespace LivingHyrule;
int failures = 0;

void Check(bool condition, const char* expression, int line) {
    if (!condition) {
        std::cerr << "Line " << line << ": " << expression << '\n';
        ++failures;
    }
}
#define CHECK(expression) Check((expression), #expression, __LINE__)

EconomyState Own(uint32_t propertyId) {
    EconomyState state{};
    state.enabled = 1;
    state.ownedProperties = 1u << propertyId;
    return state;
}

WorldProgress RecoveredAdult() {
    return { true, true, true, true, true, true, true, true, true };
}

void TestOwnershipAndRepairStages() {
    const auto adult = RecoveredAdult();
    for (uint32_t id = 0; id < kProperties.size(); ++id) {
        EconomyState economy = Own(id);
        CHECK(PropertySceneryStageFor(economy, id, adult, true, true) == PropertySceneryStage::AwaitingRepairs);
        economy.repairedProperties |= 1u << id;
        CHECK(PropertySceneryStageFor(economy, id, adult, true, true) == PropertySceneryStage::Operating);
        for (uint32_t other = 0; other < kProperties.size(); ++other) {
            if (other != id) {
                CHECK(PropertySceneryStageFor(economy, other, adult, true, true) == PropertySceneryStage::Hidden);
            }
        }
        economy.repairedProperties = 0;
        WorldProgress child{};
        CHECK(PropertySceneryStageFor(economy, id, child, true, true) ==
              (kProperties[id].region == Region::Desert ? PropertySceneryStage::Hidden
                                                        : PropertySceneryStage::Operating));
        WorldProgress crisis{};
        crisis.adult = true;
        CHECK(PropertySceneryStageFor(economy, id, crisis, true, true) == PropertySceneryStage::Hidden);
    }
}

void TestAvailabilityAndInvalidState() {
    for (uint32_t id = 0; id < kProperties.size(); ++id) {
        EconomyState economy = Own(id);
        economy.repairedProperties = economy.ownedProperties;
        const auto adult = RecoveredAdult();
        for (unsigned int gates = 0; gates < 8; ++gates) {
            economy.enabled = (gates & 1u) != 0;
            CHECK(PropertySceneryStageFor(economy, id, adult, (gates & 2u) != 0, (gates & 4u) != 0) ==
                  (gates == 7 ? PropertySceneryStage::Operating : PropertySceneryStage::Hidden));
        }
        economy.enabled = 2;
        CHECK(PropertySceneryStageFor(economy, id, adult, true, true) == PropertySceneryStage::Hidden);
        economy = Own(id);
        economy.bankRupees = kBankLimit + 1;
        CHECK(PropertySceneryStageFor(economy, id, adult, true, true) == PropertySceneryStage::Hidden);
        economy = Own(id);
        economy.repairedProperties |= 1u << ((id + 1) % 16);
        CHECK(PropertySceneryStageFor(economy, id, adult, true, true) == PropertySceneryStage::Hidden);
    }
    EconomyState valid = Own(0);
    CHECK(PropertySceneryStageFor(valid, 16, RecoveredAdult(), true, true) == PropertySceneryStage::Hidden);
    CHECK(PropertySceneryStageFor(valid, (std::numeric_limits<uint32_t>::max)(), RecoveredAdult(), true, true) ==
          PropertySceneryStage::Hidden);
    CHECK(PropertySceneryStageFor({}, 0, RecoveredAdult(), true, true) == PropertySceneryStage::Hidden);
}

void TestDensityAndDuplicates() {
    for (uint32_t id = 0; id < 16; ++id) {
        CHECK(CanAddPropertyScenery(0, 0, id));
        CHECK(CanAddPropertyScenery(0, kPropertyScenerySceneLimit - 1, id));
        CHECK(!CanAddPropertyScenery(0, kPropertyScenerySceneLimit, id));
        CHECK(!CanAddPropertyScenery(0, (std::numeric_limits<uint32_t>::max)(), id));
        CHECK(!CanAddPropertyScenery(1u << id, 1, id));
        CHECK(!CanAddPropertyScenery(0x10000u, 0, id));
    }
    CHECK(!CanAddPropertyScenery(0, 0, 16));
    CHECK(!CanAddPropertyScenery(0, 0, (std::numeric_limits<uint32_t>::max)()));
    uint32_t present = 0;
    uint32_t count = 0;
    for (uint32_t id = 0; id < 16; ++id) {
        if (CanAddPropertyScenery(present, count, id)) {
            present |= 1u << id;
            ++count;
        }
    }
    CHECK(count == 3);
    CHECK(present == 0b111);
}

} // namespace

int main() {
    TestOwnershipAndRepairStages();
    TestAvailabilityAndInvalidState();
    TestDensityAndDuplicates();
    if (failures != 0) {
        std::cerr << failures << " scenery policy checks failed.\n";
        return 1;
    }
    std::cout << "Living Hyrule scenery: all ownership, recovery, availability and density checks passed.\n";
    return 0;
}
