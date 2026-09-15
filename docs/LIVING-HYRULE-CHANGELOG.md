# Living Hyrule changelog

## 2026-09-15 - Kingdom recovery and royal estate (schema five; installed)

- Added an 18,000-bank-rupee Domain restoration after the Water Medallion and
  original Water Temple blue warp. Ordinary pools and waterfalls thaw on the
  next eligible visit, with collision and drawing selected together. King Zora,
  Blue Fire red ice, shop ice, adult quest actors and the closed Lake shortcut
  retain their original progression requirements.
- Added daytime Domain work stops for Lethra and Neris only when the loaded
  restoration is active. Their River stops and existing schedules remain a
  fallback; no residents are placed inside frozen water or the Fountain.
- Added free royal garden visits through Aren or the ledger after recorded
  Ganon victory and Market restoration funding. Zelda and Maelin appear by day,
  Aren keeps night watch, and native garden time remains paused. The east exit
  and explicit return choices lead back to the adult castle approach. Remembered
  visits preserve an escape route and suppress original story actors even if
  resource preparation fails. Child story and ending setups are untouched.
- Added a real castle-estate deed for 500,000 bank rupees, purchased in the
  prepared garden after all eight regional charters. Zelda's trust at 50 reduces
  it to 450,000. The household stays at home and recognizes the ownership;
  finished castle rooms are not included.
- Added three once-per-person gifts for all 23 residents: provisions for 60,
  work supplies for 180 and a keepsake for 350 bank rupees. Preferred gifts earn
  8 trust; other gifts earn 4. Zelda recognizes eight finite recovery deeds once
  each for 5 trust, without granting points for repeated ordinary conversation.
- Connected royal gifts, Aren's queued travel and Maelin's purchase to owned
  quote/reply dialogue. Shorter royal messages include garden/approach, trust,
  ownership and charter context without repeating deed recognition on cycling.
- Inner schema five migrates schemas 1 through 4, preserving their existing
  state and adding initially empty restoration, estate, gift and recognition
  fields. Outer version one and malformed/future-payload preservation remain.
- Added three bounded native-enemy encounters under the challenge preference:
  adult Trail red Tektite at night after Fire; child River blue Tektite at night
  after Zora's Sapphire; adult Colossus small Leever by day after Spirit and
  Gerudo membership. Field is excluded. Each entry allows at most one attempt,
  with resource/clearance checks; Colossus needs a rare native-spawner rest window.
- **Installed verification:** commit **e575c936c**, embedded **e575c93**, passed
  all **25 native and local-resource suites**, full compilation and installation.
  At 14:18:56 UTC both modded runtimes matched the compiled executable and port
  archive. Both settings files and all four saves were unchanged. The prior
  executable is backed up; development is paused for the owner's test.

No game was launched. Full castle rooms, new shop interiors, broader populations
and daily schedules, new equipment models and deeper combat remain unfinished.
The three regional encounters are included in this installed milestone.

## 2026-09-15 - Relationships, reconstruction and regional stewardship (schema four; installed)

- Added permanent identities and trust for 23 residents, ten finite delivery
  favors, an active-favor journal and actor-owned acceptance/handover.
- Added real fair/high cottage rent, locked current-period terms, trust changes
  and reduced collection when Bram falls behind; trusted managers quote and
  charge 10% less for repairs.
- Added frozen conversation topic cycling with native three-choice prompts,
  fresh confirmation, speaker checks and protection against simultaneous movement
  and confirmation input.
- Added independent postgame Zelda, Captain Aren and Maelin actors on the ruined
  castle approach, with recovery dialogue and shared recorded meetings.
- Added a 25,000-bank-rupee Market square restoration after recorded Ganon victory.
  Compatible streets and facades appear on re-entry; closed shop fronts and alley
  barriers preserve the adult setup. Interiors and castle rebuilding are unfinished.
- Added short walking routines for Pella in the Market and Edda at Lake Hylia,
  with step-by-step clearance checks and pauses for nearby players or dialogue.
- Added eight locally purchased clothing dyes, per-save ownership and selection,
  and draw-local native Link colors in both ages. Existing cosmetic overrides,
  custom Link models and connected Anchor appearances take priority. Equipment
  protection and shared palettes are preserved.
- Added eight regional charters, separate capped treasuries, local bank transfers,
  once-per-adult-business-period dues, regional greetings and the High Steward of
  Hyrule title. Charter purchases require recovered, owned and repaired holdings.
- Added nested wardrobe/stewardship data to the engine's copied save snapshot.
  Inner schema four migrates schemas one, two and three, preserving all earlier
  money, deeds, repairs, timers, relationships, favors, rent terms and restoration
  funding. New modules start empty. The outer section stays version one; strict
  validation and unreadable/future-section preservation remain in place.
- **17 of 17 native and local-resource suites passed**, covering the combined
  increment. Source commit **46c17fa9e** configured, compiled, linked and installed
  successfully at 08:09 UTC. Both modded runtime executables match the build;
  both settings files and all four existing save files are unchanged. An initial
  strict floating-point warning in Market door drawing was corrected before the
  successful build. Receipts and logs are recorded in the worklog.

No game was started. The successful installation and automated checks do not
establish gameplay acceptance. At this stage, full building/interior/castle
restoration, wider daily routines and deeper combat remained unfinished.

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
- Required the saved final-boss timestamp for relief, rather than the generic
  completion flag used by custom timer goals. A genuine Ganon defeat queues a
  statistics-only save so postgame evidence survives restart without autosaving
  the wallet, ledger or base-game progress.
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
