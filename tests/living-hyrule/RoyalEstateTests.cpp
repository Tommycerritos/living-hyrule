#include "RoyalAudiencePolicy.h"
#include "RoyalEstatePolicy.h"
#include <cmath>
#include <iostream>

namespace {
using namespace LivingHyrule;
unsigned int checks = 0;
unsigned int failures = 0;
void Check(bool condition, const char *reason) {
  ++checks;
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << reason << '\n';
  }
}

void EntryGates() {
  for (unsigned int flags = 0; flags < 128; ++flags) {
    Check(RoyalGardenVisitUnlocked(flags & 1, flags & 2, flags & 4, flags & 8,
                                   flags & 16, flags & 32,
                                   flags & 64) == (flags == 127),
          "a garden invitation requires every independent story/save/ledger "
          "gate");
  }
  Check(RoyalGardenOriginAllowed(0x64, false, false),
        "castle approach invitation does not require a Market latch");
  Check(!RoyalGardenOriginAllowed(0x22, false, false),
        "funding alone cannot make ruined Market an active origin");
  Check(RoyalGardenOriginAllowed(0x22, true, false),
        "an active restored Market is a supported invitation origin");
  Check(!RoyalGardenOriginAllowed(0x4A, false, false),
        "an unprepared courtyard cannot authorize a purchase");
  Check(RoyalGardenOriginAllowed(0x4A, false, true),
        "prepared garden supports the separately implemented purchase");
  for (int scene = 0; scene < 110; ++scene) {
    if (scene != 0x64 && scene != 0x22 && scene != 0x4A)
      Check(!RoyalGardenOriginAllowed(scene, true, true),
            "other locations cannot inherit an old scene latch");
  }
}

void ResumedVisitAndStoryIsolation() {
  for (unsigned int flags = 0; flags < 16; ++flags) {
    for (int spawn = -1; spawn <= 3; ++spawn) {
      for (int entrance :
           {0x400, 0x402, 0x403, 0x5F0, 0x5F2, 0x138, 0x296, 0xFFF}) {
        const bool expected =
            flags == 15 && ((entrance == 0x400 && spawn == 0) ||
                            (entrance == 0x5F0 && spawn == 1));
        Check(RoyalGardenLoadAllowed(flags & 1, flags & 2, flags & 4, flags & 8,
                                     0x4A, spawn, entrance) == expected,
              "sanitize only native normal adult garden spawns, never child or "
              "ending setups");
      }
    }
  }
  for (int scene = 0; scene < 110; ++scene)
    Check(RoyalGardenLoadAllowed(true, true, true, true, scene, 0, 0x400) ==
              (scene == 0x4A),
          "the safety latch belongs only to the loaded garden");
  // Reload sanitation intentionally has no economy, victory, or invitation
  // gate. A remembered visit still has an exit if its ledger is disabled or
  // unavailable after loading. Only new invitations require those gates.
  Check(RoyalGardenLoadAllowed(true, true, true, true, 0x4A, 0, 0x400) &&
            !RoyalGardenVisitUnlocked(true, true, true, true, false, false,
                                      false),
        "a disabled remembered garden remains an escape-safe shell, without "
        "unlocking a new visit");
}

void Household() {
  RoyalAudienceContext context;
  context.enabled = context.supportedAdventure = context.normalScene =
      context.royalGarden = true;
  context.world.adult = context.world.ganonDefeated = true;
  context.economy.enabled = 1;
  context.economy.marketRestored = 1;
  context.daytime = true;
  Check(RoyalResidentMaskFor(context) == 7,
        "garden receives the same three original household identities");
  context.daytime = false;
  Check(RoyalResidentMaskFor(context) == 2,
        "Aren keeps a night return watch while Zelda and Maelin rest");
  context.normalScene = false;
  Check(RoyalResidentMaskFor(context) == 0,
        "the garden location does not override ending exclusion");
  context.normalScene = true;
  context.economy.enabled = 0;
  Check(RoyalResidentMaskFor(context) == 0,
        "disabling the economy removes optional residents, not the exit");
  context.economy.enabled = 1;
  context.world.ganonDefeated = false;
  Check(RoyalResidentMaskFor(context) == 0,
        "a safety-only resumed garden never invents postgame household access");
  for (size_t first = 0; first < kRoyalGardenPlacements.size(); ++first) {
    const auto &a = kRoyalGardenPlacements[first];
    Check(a.x < 500,
          "household stays out of the native east entrance and return lane");
    for (size_t second = first + 1; second < kRoyalGardenPlacements.size();
         ++second) {
      const auto &b = kRoyalGardenPlacements[second];
      Check(std::hypot(a.x - b.x, a.z - b.z) > 200,
            "household placements cannot suppress one another");
    }
  }
}
} // namespace

int main() {
  EntryGates();
  ResumedVisitAndStoryIsolation();
  Household();
  std::cout << "Living Hyrule royal estate: " << checks << " checks, "
            << failures << " failures.\n";
  return failures != 0;
}
