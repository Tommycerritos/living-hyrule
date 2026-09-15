# Living Hyrule development

## Current scope

The first playable economy increment is implemented on
`feature/living-hyrule-economy`. It adds an optional bank account, one Kakariko
cottage purchase, persistent ownership, and periodic rent. Implementation and
native automated tests are complete, and the full game compiled and linked
successfully. Gameplay acceptance is left to the project owner; the new mod has
not been runtime-playtested.

The original quest, dungeon progression, songs, medallions, spiritual stones, and
required items must remain intact. The wider life, social, recovery, and post-Ganon
systems are future work. The creative direction is preserved in the
[local creative vision](C:/ZeldaDev/docs/LIVING-HYRULE-VISION.md); the earlier
[reference design](C:/ZeldaDev/docs/LIVING-HYRULE-REFERENCE.md) is also retained.

The subsequent `feature/living-hyrule-residents` branch adds Tavin, Bram, and Orlen
as independent Kakariko actors with existing carpenter visuals, individual head
variants, clothing tints, proportions, and idle posture. They have original
conversations that recognize age, the Shadow Medallion, and cottage ownership.
The [population plan](LIVING-HYRULE-POPULATION.md) covers all 110 scene IDs;
worldwide deployment remains planned.

The **Additional residents** checkbox is a global preference, default off and
independent of economy state. They appear outdoors by day in normal adventures
and Master Quest: three during childhood, Bram during the adult crisis, three
after the Shadow Medallion. They do not appear in cutscene layers. The scheduler
validates floor and body clearance and checks nearby actors before placing them;
blocked candidate positions are retried later. No existing actor is moved.
Placement is provisional until gameplay acceptance. There are no night-time
indoor schedules in this increment, and NPC conversations do not transact money.

## Using the first prototype

Use a disposable development save in a normal adventure or Master Quest. Open
the port menu, then **Enhancements > Living Hyrule > Open Living Hyrule**. Enable
the economy separately for each save file. Existing saves without a Living Hyrule
section begin with an empty, disabled ledger. Randomizer and Boss Rush are outside
the supported prototype scope.

The ledger shows the wallet, bank balance, cottage ownership, time to the next
rent payment, and total rent earned. Banking is available throughout Hyrule;
buying the cottage requires being in Kakariko Village. This is a ledger purchase
representing ownership. Tavin can discuss the listing, but purchases still use
the ledger. There is no new interior or alteration to an existing building yet.

| Rule | Prototype behavior |
| --- | --- |
| Cottage price | 1,200 rupees, paid from the bank |
| Rent | 25 rupees into the bank per ten minutes of active play |
| Bank limit | 999,999,999 rupees |
| Wallet | Existing wallet capacity; withdrawals must fit |
| Child Link | Property purchase and rent are available |
| Adult Link | Purchase and rent stop until the Shadow Medallion is obtained |
| Recovery | Ownership and savings survive the crisis; resuming after the Shadow Temple is free in this prototype |
| Economy pause | Preserves money, ownership, lifetime earnings, and partial rent progress |

Prices and income are provisional tuning, not a finished economic balance.
There is no offline payout or payment for the seven-year age transition. Rent
counts eligible player updates at 20 ticks per second, stopping during dialogue,
blocking cutscenes, pause screens, transitions, game over, the port menu, and
Kakariko's adult crisis. A completed rent period is consumed even if the bank has
no room; only rupees actually credited count toward total earnings.

Deposits and withdrawals require a positive amount, sufficient funds, and room
in the destination. The ledger waits for the game's current rupee-counting
animation to finish before allowing a transaction. Paused gameplay and unreadable
ledger data also make its controls unavailable.

**Save your game normally after making changes.** A transaction does not trigger
an immediate autosave. The normal full save stores the wallet and Living Hyrule
state together; quitting or reloading without saving discards subsequent changes
under the game's usual save rules.

## Save architecture and compatibility

The economy is a fixed-size, plain C data structure (`LivingHyruleSaveData`) stored
inline at `SaveContext.ship.livingHyrule`. It contains the enabled flag, bank
balance, cottage ownership, partial rent ticks, and lifetime credited rent. It
has no pointers or dynamic containers and remains safe to copy with the game's
save snapshot.

`SaveManager` takes a copy of `SaveContext` before its background save work. The
Living Hyrule save callback reads that snapshot, so a full save captures the
wallet and ledger from the same point in time. Transfers change the settled
wallet and bank together on the game thread. The prototype does not persist the
ledger separately from the wallet or store economy state in global settings.

The registered custom section is `livingHyrule`, with a stable outer section
version of **1**. Its `data.economy` payload currently uses inner
`schemaVersion: 1`. Future economy migrations should dispatch on the inner schema
while keeping the outer registration stable, unless an engine-level version
change has been explicitly designed and reviewed.

The save codec validates required fields, boolean types, unsigned integer ranges,
the bank limit, and rent progress before changing the live state. Unsupported or
malformed inner economy payloads make the ledger read-only. A small, opt-in
`SaveManager` fallback handles invalid, empty, or future-version Living Hyrule
section envelopes the same way. The snapshot-based save condition runs before
the section's version or data can be rewritten, retaining the unreadable section
when the rest of the game is saved instead of replacing it with a fresh account.
Existing saves with no optional section remain valid and opt out by default.

These paths preserve unsupported data; they do not interpret or migrate an
unknown format. The registered outer version remains **1**, and unrelated saves
and sections retain their upstream handling. File-wide malformed JSON or damage
outside the optional Living Hyrule section still belongs to upstream save
recovery. Keep development backups when checking save compatibility and rollback.

Implementation is isolated under `soh/soh/Enhancements/living-hyrule`, apart from
the fixed-size save member in `soh/include/z64save.h`, its C header, and the opt-in
fallback loader and save condition added to `SaveManager`.
`RegisterMenuInitFunc` adds the ledger window and sidebar, `RegisterShipInitFunc`
registers persistence and gameplay hooks, and `OnPlayerUpdate` advances rent only
when the gameplay conditions above are met. The build's recursive source discovery
finds the new module without a manual source list.

## Verification and user acceptance

Native economy-model, save-codec, and population-policy tests passed (three suites).
The population suite covers 224 combinations of enable, adventure, location,
scene, daytime, and story-phase conditions. Full-game compilation and linking
succeeded for the combined economy and resident implementation.

The project owner will perform gameplay acceptance. No runtime playtest of the
new mod is claimed, and further agent playtesting is not part of this handoff.
The ledger controls, normal save/reload, save-slot separation, NPC placement,
appearance and dialogue, Child/Adult Link recovery behavior, and original quest progression remain for the owner's
playthrough. Use a disposable development save when trying the increment. The
successful automated checks and build establish implementation readiness without
claiming those gameplay paths have been exercised in the running game.

## Repository and branches

- `upstream`: https://github.com/HarbourMasters/Shipwright.git (fetch only).
- `origin`: https://github.com/Tommycerritos/living-hyrule.git (personal fork).
- `develop`: untouched official baseline, tracking `upstream/develop`.
- `living-hyrule`: integration branch for reviewed project changes.
- `feature/living-hyrule-economy`: current bank and first cottage prototype.
- `feature/living-hyrule-residents`: three new residents, including the economy branch.
- `feature/<topic>`: one bounded feature per branch and pull request into `living-hyrule`.
- `fix/<topic>` and `docs/<topic>`: focused fixes and documentation.
- `baseline/shipwright-2026-09-14`: pinned original source commit for comparison.

Fetch upstream explicitly. Review upstream changes, update submodules to their
recorded revisions, and validate a clean build before merging engine updates.
Do not automatically rebase published integration history. Release tags should
identify validated builds; do not commit binaries or package game assets.

The starting toolchain and dependency-manager revisions are recorded in
`docs/LIVING-HYRULE-TOOLCHAIN.json`. To reproduce the dependency setup elsewhere,
clone Microsoft vcpkg into the external workspace's `tools/vcpkg`, check out the
recorded commit in detached mode, and bootstrap it before running `Build.ps1`.
Use `git submodule update --init --recursive` in the source checkout. The installed
library inventory is recorded locally in `C:\ZeldaDev\docs\dependency-versions.txt`.

## Local data policy

`C:\ZeldaDev\roms`, `assets`, `runtime`, `build`, and `backups` are outside Git.
The preserved vanilla copy is a reference; launch a test copy when checking it.
Development uses its own configuration and saves. Never share ROM-derived O2R/OTR
archives, extracted game data, or runtime backups.

Upstream already tracks source asset headers and some project-owned graphics.
Keep those intact. New graphics/audio/models are blocked by the repository guard
until their original authorship or license is reviewed and a narrow policy change
is approved as part of normal development. File extensions cannot prove ownership.

`.gitignore` prevents ordinary accidental additions. `.githooks/pre-commit` and
`pre-push` additionally check staged files and outgoing history, including common
ROM signatures and renamed archives. Git hooks can be bypassed; they are not an
absolute security boundary. Always review `git diff --cached` before committing.
On a new clone enable them with `git config core.hooksPath .githooks` and set
`git config livinghyrule.python <absolute-path-to-python.exe>`.

GitHub Actions are disabled on the fork until a source-only CI design is reviewed.
Do not copy the upstream asset-upload workflow or register this PC as a public
self-hosted runner. Later CI may compile source and run synthetic-data tests;
ROM-dependent playtests remain local.

## Next phases

These systems are planned, not implemented by the economy prototype:

1. **Cottage seller and interaction:** connect the ledger transaction to a
   physical NPC seller and dialogue, choose the property's location and access,
   and validate ownership through save/reload without disrupting existing actors
   or quest dialogue.
2. **Paid regional reconstruction:** keep the original dungeon or story solution
   as the first recovery requirement, then add reconstruction paid for by Link.
   The current free Shadow Temple trade reopening is only the first story link.
3. **More properties and livelihoods:** homes, shops, businesses, farms, regional
   price and income tuning, expenses, recovering population, and travelers.
4. **Rapport and regional influence:** relationships, reputation, tenant treatment,
   gifts and quests, dialogue consequences, and eventual regional stewardship.
5. **Persistent postgame:** a safe but ruined Castle Town after Ganondorf, staged
   restoration, Zelda available in the world, castle life and rapport, and eventual
   castle ownership that preserves Zelda's place there.
6. **Equipment and harder combat:** optional regional gear, more dangerous and
   varied encounters, meaningful healing and preparation costs, and carefully
   tuned consequences. Original story items and required progression remain intact.

Review each phase's architecture, save migration, compatibility, asset permissions,
and balance before integrating it. See upstream `docs/MODDING.md` for its code-mod
workflow. Asset reuse requires permission and attribution; Nintendo assets stay
on each player's own computer.
