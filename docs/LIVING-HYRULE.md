# Living Hyrule development

## Implemented systems

Living Hyrule extends a normal Ocarina of Time or Master Quest adventure while
preserving the main quest. Randomizer and Boss Rush are outside this increment's
supported scope. The [worklog](LIVING-HYRULE-WORKLOG.md) identifies the last
successfully installed build; the [changelog](LIVING-HYRULE-CHANGELOG.md) records
individual stages. No automated gameplay acceptance is claimed.

### People and direct trade

Twenty new identities use independent custom actor behavior. Three live in
Kakariko; nine cover the Market, Hyrule Field, ranch and lake; four use native
Kokiri/Goron models; four use native Zora/Gerudo models. See the
[population document](LIVING-HYRULE-POPULATION.md) for exact placement and gates.
Compatible skeletons, heads, idles and material segments are reused locally;
original quest actors and shared resource data are not modified.

**Additional residents** is a global preference, independent of each save's
economy switch. Schedulers check story/access/time conditions, floor, ledges,
body clearance, nearby actors and duplicate identities. Blocked positions are
retried. Residents finish active or pending conversations before leaving when a
shift or setting changes. Existing NPCs and doors are not moved.

Participating managers quote a bank-funded deed or repair. Tavin first sells the
cottage, then offers the builders' yard; Orlen deposits wallet rupees; Bram
withdraws enough savings to fill the wallet. The ledger remains available for all
properties, including the cloth workshop without a manager in this increment.

Each actor freezes its offer and save slot when preparing the conversation.
Only its own live Yes/No prompt and a fresh A press authorize payment. B, C-up or
the second choice cancel. The handler consumes a quote before continuing to the
result textbox. The shared action path rechecks ownership, story, location,
funds, wallet capacity, current save, dialogue ownership and safe gameplay.
Ordinary talking is supported without clearing Link's state flags. Pending
item-putaway conversations retain their quote and actor until the textbox opens.

### Banking, property and recovery

The bank holds up to 999,999,999 rupees. Transfers require positive amounts,
sufficient funds, room in the destination and a settled wallet counter. Money
changes together on the game thread. The cottage costs 1,200 bank rupees and pays
25 every ten minutes of active play. Its adult trade/rent resumes after Shadow.

[Sixteen additional deeds](LIVING-HYRULE-PROPERTIES.md) cover eight regions, each
with an independent income timer and repair state. Child ownership persists.
Adult regional crises suspend operations; the original recovery requirement and
paid individual repairs reopen businesses. Pausing the economy preserves all
balances, ownership, repair state and partial timers. Prices remain provisional.

Income counts eligible player updates at twenty ticks per second. Dialogue,
blocking cutscenes, pause screens, transitions, game over, the port menu and
regional shutdowns stop the relevant timer. There is no offline or seven-year
windfall. Bank capacity limits actual credits; lifetime earnings saturate safely.

After the game's recorded Ganon victory, enabled Living Hyrule removes the
ruined Market's Redeads for free, including offscreen ones. Relief residents can
return, and businesses can be purchased/repaired. The mod does not set the
completion flag or replace the ending. The surrounding town remains ruined.

Property supplies are independent, non-colliding decorative actors anchored to
seven WorldResidents managers. An owned adult business awaiting repairs shows
one standard wooden crate; an operating business shows three separate crates.
At most three arrangements appear per scene, subject to floor, water, wall and
actor clearance. Leaving traders or invalid/disabled/closed businesses remove
their supplies. These props add no drops, geometry, doors or ownership flags.

### Combat and recovery supplies

**Dangerous combat and scarce recovery** is a separate global preference.
Before an ordinary player update, eligible live enemy/boss collision damage is
doubled with safe byte saturation. The existing damage path still handles
shields, Double Defense, fairy revival and death. Falls, burning, grabs, scripted
damage and enemy health are unchanged. Expiring one-frame invincibility timers
are evaluated at the engine's actual damage-consumption frame.

Damage Multiplier, external defense effects, One-hit KO, Infinite Health and
Permanent Heart Loss take priority. The UI explains conflicts without rewriting
those settings. No Heart Drops and No Random Drops likewise override filtering.

Every second eligible temporary, unflagged loose heart is removed before its
first eligible update. Emergency drops at one heart or less are kept. Placed
hearts, direct item awards, heart pieces, containers and fairies are excluded.
The filter consumes no random numbers and clears identity tracking on scene exit.
This is the first combat increment, not a claim of redesigned enemy intelligence.

## Persistence

**Save normally after transactions.** There is no transaction autosave; quitting
without saving discards later changes under the game's usual rules.

LivingHyruleSaveData, inline at SaveContext.ship.livingHyrule, is a fixed-size
plain C structure. It holds the enable flag, bank, cottage, rent ticks/earnings,
property/repair masks, sixteen business timers and business earnings. It has no
pointers or dynamic containers and is safe for the engine's copied save snapshot.
The background save callback reads that snapshot, capturing wallet and ledger
from the same moment. Global preferences are separate from per-save money.

The optional named section is livingHyrule, outer version **1**. Its
data.economy payload uses inner **schemaVersion 2**. Schema one migrates by
preserving every old balance/ownership/timer and initializing new fields to zero.
New saves without the section start with an empty, disabled economy.

The codec validates booleans, integer types/ranges, balances, ownership masks,
repair subsets and timer invariants before assigning live state. Unknown or
malformed payloads/envelopes make the ledger read-only. The opt-in SaveManager
fallback and snapshot save condition preserve the original section when the
rest of the game saves. Unrelated sections retain upstream behavior; file-wide
JSON corruption still belongs to upstream recovery.

Resident identities, dialogue state, scenery and heart-drop tracking are transient
actor/runtime state. This world-life stage does not change the persistent POD.

## Build and verification

Use tools/living-hyrule/Build.ps1 actions Configure, Build and Stage on the
configured Windows workstation. Never use Run without an explicit user request.
Test.ps1 builds native policy/codec suites for banking, persistence, regional
properties, trade confirmation, population, supplies and challenge rules. Full
engine compilation checks actual hook/actor integration.

Tests and compilation do not verify live rendering, dialogue timing, actor
placement or final balance. The owner performs the final gameplay checks. Keep
the last successful runtime until its replacement builds, then compare installed
and compiled executable hashes. Receipts and logs live outside Git.

The engine discovers module sources recursively. Self-registering startup hooks
connect them to real player, actor, scene, message and save events. The only
upstream save changes are the POD member and optional load/save preservation
hooks. Windows metadata copying uses timestamp/size-aware Robocopy without
mirror/delete flags; other hosts retain the CMake copy fallback.

## Source and local data

- origin: https://github.com/Tommycerritos/living-hyrule.git (source-only fork).
- upstream: https://github.com/HarbourMasters/Shipwright.git (fetch only).
- develop: official baseline; living-hyrule: reviewed integration.
- feature branches: bounded work; historical branches retain milestones.
- baseline/shipwright-2026-09-14: starting source revision.

Fetch upstream deliberately; review engine changes and recorded submodules before
integration. Do not automatically rebase published integration history. Toolchain
revisions are recorded in LIVING-HYRULE-TOOLCHAIN.json; vcpkg and build outputs
remain external. Source guards run at commit and push and inspect outgoing history.
On a new clone set core.hooksPath to .githooks and livinghyrule.python to Python.

C:\ZeldaDev\roms, assets, runtime, build and backups are outside Git.
Preserve the vanilla installation and keep development saves/settings separate.
Never share ROM-derived O2R/OTR archives, extracted data or runtime backups.
Keep upstream asset headers intact. New media need reviewed provenance; file
extensions alone cannot prove ownership. GitHub Actions remain disabled until
a source-only workflow is deliberately designed.

## Still in development

Full building reconstruction and new interiors; broader property coverage and
balanced expenses; persistent relationships, favors, gifts and rent treatment;
regional stewardship and treasuries; persistent postgame Zelda and castle life;
optional regional equipment and more deliberate encounter changes. The full
[creative vision](C:/ZeldaDev/docs/LIVING-HYRULE-VISION.md) remains the direction.
The [population plan](LIVING-HYRULE-POPULATION.md) evaluates all 110 scene IDs,
including places that should retain solitude, puzzle space or quest atmosphere.
