# Living Hyrule changelog

## 2026-09-15 - World life, direct trade and combat challenge

- Added seventeen residents beyond the original three, across the Market,
  Field, ranch, lake, Kokiri Forest, Goron City, Zora River, Gerudo Valley and
  Fortress. Native regional silhouettes, compatible idles and original dialogue
  retain cultural and story context. Schedules respect recovery and access.
- Connected fifteen regional deeds plus the cottage to manager conversations.
  Confirmed offers purchase or repair with actual bank rupees. Bram and Orlen
  provide wallet withdrawal/deposit services; the cloth workshop remains in
  the ledger. Dialogue ownership and fresh-button checks prevent repeated or
  accidental charges, including pending item-putaway conversations.
- Added non-colliding repair/operating supply crates beside seven business
  managers, subject to safe ground and density limits. Building restoration
  and new interiors remain unfinished.
- Extended free post-Ganon Market Redead removal to offscreen enemies so
  returning relief workers do not depend on enemy update visibility.
- Added optional double ordinary enemy/boss collision damage and alternating
  temporary loose-heart filtering. Existing cheats/modifiers take priority;
  emergency hearts and permanent pickups keep their normal rules.
- Passed ten native suites covering economy, saves, properties, confirmation,
  world/cultural schedules, property supplies and challenge policy. Full
  integration/build results are recorded in the worklog. No game launch.
- Reduced repeated Windows metadata-copy work while preserving extra local
  files and retaining the portable CMake fallback.

The wider relationship, reconstruction, stewardship, Zelda/castle and equipment
vision remains in progress. This entry describes actual source implementation;
the worklog identifies which executable has successfully built and been installed.

## 2026-09-14 - Regional property economy

- Implemented sixteen bank-funded deeds across eight regions, with local purchase
  requirements, independent income timers, crisis shutdowns and paid repairs.
- Used actual medallion, Epona, Gerudo membership and adventure-completion flags
  to decide which adult regions can trade. Ownership survives the age transition.
- Added validated schema-two persistence with legacy economy migration.
- Added the post-Ganon ruined-market Redead removal rule, free of reconstruction
  charges. This does not change the ending or set completion flags.
- Added a fourth automated suite covering regional purchase/repair behavior,
  income suspension, bank caps, earnings overflow and market-safety policy.
- Enabled existing residents and the valid file1 economy in the local development
  profile, preserving backups outside Git. No game launch performed.

Full build/staging status is recorded in the worklog. Property repairs currently
change operating state; physical reconstruction and owner-NPC transactions are
still unfinished.

## 2026-09-14 — First additional residents

- Added Tavin the carpenter, Bram the boot-mender, and Orlen the supplier in
  Kakariko, controlled by an independent **Additional residents** option.
- Reused the existing carpenter skeleton with per-character head choices,
  clothing tints, proportions, and idle poses. New actor behavior owns its
  dialogue and does not run the original carpenters' quest logic.
- Added daytime schedules: all three in childhood, Bram during the adult
  crisis, and all three again after the Shadow Medallion.
- Added conversations that react to age, recovery, and cottage ownership.
- Added floor, body-clearance, nearby-actor, and duplicate-resident checks before
  placing residents. Schedule changes allow an active conversation to finish.
- Passed the native population policy suite, covering 224 combinations of
  location, story, time, and enable conditions.
- Compiled and linked the full Windows game with the economy and residents.
- Documented contextual population plans for all 110 scene IDs. Other regions,
  indoor schedules, and additional model families remain planned.

Gameplay placement, appearance, and dialogue acceptance are left to the owner.
The residents introduce people behind the economy; property purchases continue
through the ledger in this increment.

## 2026-09-14 — First economy increment

- Added an optional bank account for each normal or Master Quest save file.
- Added deposits, withdrawals, deposit-all, and fill-wallet actions. Transfers
  respect wallet capacity and bank limits and change both balances together.
- Added a Kakariko rental cottage purchased from the ledger while in the village.
  The prototype price is 1,200 rupees; rent is 25 rupees per ten minutes of eligible
  active play, credited directly to the bank.
- Preserved ownership across the age transition. Adult Kakariko sales and rent
  remain suspended until the Shadow Medallion is obtained.
- Added normal-save persistence for balances, ownership, rent progress, and
  lifetime rent earnings. Pausing the economy retains these values.
- Added strict save-data validation and an opt-in compatibility fallback that
  preserves an unreadable Living Hyrule section during subsequent game saves.
- Added automated economy and save-codec checks; both native test suites passed.
- Compiled and linked the full Windows Debug game. Gameplay acceptance is
  reserved for the project owner; the new mod has not been runtime-playtested.

This increment establishes the banking and property foundation. The cottage is
currently a ledger purchase without a physical seller or new interior. Broader
property ownership, paid reconstruction, population, relationships, postgame,
equipment, and combat changes remain in development scope for later increments.

## 2026-09-14 — Development environment

- Preserved the clean Ship release and local ROM outside Git.
- Established the pinned Windows toolchain, upstream/fork remotes, integration
  branch, source build, isolated runtime, and repository asset guards.
- Preserved the Living Hyrule creative vision and documented the development
  workflow.
