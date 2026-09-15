# Living Hyrule development

## Implemented systems

Living Hyrule extends a normal Ocarina of Time or Master Quest adventure while
preserving the main quest. Randomizer and Boss Rush are outside this increment's
supported scope. The [worklog](LIVING-HYRULE-WORKLOG.md) identifies the last
successfully installed build; the [changelog](LIVING-HYRULE-CHANGELOG.md) records
individual stages. The installed schema-five build **e575c93** (source
**e575c936c**) passed all **25 native and local-resource suites**, full compilation
and installation verification. Both modded runtimes match the compiled files;
both settings files and all four saves are unchanged. Development is paused for
the owner's test. No game was started; automated checks do not establish gameplay
acceptance.

### People and direct trade

Twenty-three identities use independent custom actor behavior. Three live in
Kakariko; nine cover the Market, Hyrule Field, ranch and lake; four use native
Kokiri/Goron models; four use native Zora/Gerudo models; Zelda, Captain Aren and
Maelin receive postgame visitors on the castle approach and in the royal garden.
Lethra and Neris also have daytime stops in the actively restored Domain. See the
[population document](LIVING-HYRULE-POPULATION.md) for exact placement and gates.
Compatible skeletons, heads, idles and material segments are reused locally;
original quest actors and shared resource data are not modified.

**Additional residents** is a global preference, independent of each save's
economy switch. Schedulers check story/access/time conditions, floor, ledges,
body clearance, nearby actors and duplicate identities. Blocked positions are
retried. Residents finish active or pending conversations before leaving when a
shift or setting changes. Existing NPCs and doors are not moved.

Pella in the Market and Edda at Lake Hylia now take short walks between two
nearby stops. Each step checks ground, water, walls and nearby actors. They stop
for the player, pending/open dialogue, unsafe gameplay or an obstruction; they
do not navigate around obstacles or move between scenes. Other residents keep
their established work stops and time/story schedules. Broader routines and
indoor relocation remain unfinished.

Participating managers quote a bank-funded deed or repair. Tavin first sells the
cottage, then offers the builders' yard; Orlen deposits wallet rupees; Bram
withdraws enough savings to fill the wallet. The ledger remains available for all
properties, including the cloth workshop without a manager in this increment.

Each actor freezes its offer and save slot when preparing the conversation.
Only its own live choice prompt and a fresh A press authorize payment. B and C-up
cancel. Three-choice pages use Yes / Something else / Not now; the middle choice
cycles a frozen offer list without payment. Simultaneous navigation and A cancels
because the engine applies navigation during drawing. The handler consumes a quote before continuing to the
result textbox. The shared action path rechecks ownership, story, location,
funds, wallet capacity, current save, dialogue ownership and safe gameplay.
Ordinary talking is supported without clearing Link's state flags. Pending
item-putaway conversations retain their quote and actor until the textbox opens.

### Relationships and favors

Twenty-three permanent save identities track meetings and signed trust (-100 to 100).
Meetings add no points. Ten once-per-save deliveries connect the original twenty
residents; only one can be carried at a time. Acceptance requires both people to
be reachable in the current age/story. Delivery requires the recipient's owned
conversation. The journal can cancel a delivery without a reward; it cannot
accept or finish one remotely. Successful delivery adds 10 trust with both people.

A manager trusted at 10 or above quotes repairs at 90% of the normal price. The
engine checks the current price against the frozen quote before charging. A first
paid repair earns 5 trust; repeat repairs are rejected. Fair cottage rent pays 25
and restores 1 trust while Bram is below 20 on a credited period. High rent pays
40, costs 3 trust, and falls to 15 when Bram is at -10 or below. Requested terms and current-period
terms are distinct, preventing a last-moment switch from changing a due payment.

The royal audience appears only after the saved Ganon-defeat timestamp: all three
by day, Aren overnight. Independent actors reuse compatible native skeletons and
rendering helpers, never the original Zelda escape, guard or quest state machines.
Their garden and approach placements use independently checked native ground;
neither site adds finished castle rooms.

Each of the 23 residents accepts three finite gift kinds through an owned
conversation: regional provisions (60 bank rupees), work supplies (180), and a
handmade keepsake (350). Each kind can be given to each person once. A preferred
gift earns 8 trust and another gift earns 4; repeat gifts are rejected. Zelda
also recognizes eight finite deeds, each worth 5 trust once: the five adult
temple recoveries, Market restoration, Domain water restoration and all eight
regional charters. Recognition occurs in her actual conversation, never from
repeated journal checks or ordinary greetings.

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
return, and businesses can be purchased/repaired. The saved final-boss timestamp
is authoritative: the generic gameComplete flag also marks custom timer goals
and resets on loading. A genuine final victory queues one statistics-only save
after the engine's boss hooks finish. This preserves defeat evidence through
restart while leaving unsaved wallet, ledger and normal progress alone. The
original ending remains intact.

The separate **25,000-rupee Market restoration** can then be funded through
Hadrin's conversation or the local ledger. On leaving and returning, compatible
native resources provide restored square streets and building facades while
preserving the adult scene, original story state and supported exit routes.
Shop fronts remain closed, alley barriers remain, and no new interiors or
castle restoration are included. Resource/background compatibility and entrance
checks can withhold the work; payment is refused if it cannot be prepared safely.
Funding is per save and requires a normal save to persist.

The separate **18,000-rupee Domain restoration** requires the Water Medallion
and the original Water Temple blue-warp completion flag. Fund it through Lethra
or the recovery controls. On the next eligible adult Domain entry, native
ordinary ice becomes flowing pools and waterfalls. Collision and drawing are
selected together before the scene is initialized and remain fixed for that
visit if the economy setting changes. Missing or incompatible resources leave
the native frozen Domain in place and preserve the investment.

King Zora, his Blue Fire treatment, the shop's red ice, adult Skulltula and
original quest actors remain intact. The underwater Lake shortcut stays sealed,
with matching visible and physical blocking. Lethra and Neris appear by day on
dry room-one walkways only while the actual restoration is active and the
economy is enabled; their River stops remain available under existing schedules.

### Royal garden and castle estate

After recorded Ganon victory and Market restoration funding, adult Link can
ask Aren or use the ledger to visit the native royal garden from the castle
approach or actively restored Market. Visits are free and require no estate
deed. The garden uses an ordinary adult entrance without changing Link's age,
the original ending, or canonical quest flags. Original courtyard story actors
are suppressed only in this adult visit; child and ending setups retain them.

Zelda and Maelin receive visitors by day; Aren keeps watch at night. Native
garden time remains paused. The east doorway, Aren's return topic and the ledger
return control lead back to the adult castle approach. Travel chosen in dialogue
waits for that conversation to end and rechecks safe play. The return route
persists if the economy or residents are disabled. A remembered garden visit
with unprepared resources keeps an empty escape route while withholding the
household and purchase; it does not load the original story actors.

The **castle estate deed costs 500,000 bank rupees** and requires recorded Ganon
victory, funded Market restoration, all eight regional charters and an actively
prepared garden. Purchase through Maelin or the garden's ledger controls.
Zelda's trust at **50** lowers the price by ten percent to **450,000**. Ownership
adds permanent standing and household recognition. Zelda and her staff remain
at home; the deed grants no finished castle rooms and evicts nobody.

### Property supplies

Property supplies are independent, non-colliding decorative actors anchored to
seven WorldResidents managers. An owned adult business awaiting repairs shows
one standard wooden crate; an operating business shows three separate crates.
At most three arrangements appear per scene, subject to floor, water, wall and
actor clearance. Leaving traders or invalid/disabled/closed businesses remove
their supplies. These props add no drops, geometry, doors or ownership flags.

### Regional clothing dyes

Eight dyes can be bought with bank rupees while visiting the matching region
after its existing trade/recovery gate opens. Once owned, a dye can be selected
freely anywhere and in either age; original appearance is always available.
Purchasing does not automatically equip it. These are clothing and hat colors
on Link's native model, with no new meshes, weapons, shields or resistance effects.

| Region | Dye | Price in bank rupees |
| --- | --- | ---: |
| Kokiri Forest | Kokiri fern | 150 |
| Death Mountain | Goron ember | 600 |
| Lake and Zora lands | Zora river | 650 |
| Gerudo lands | Gerudo sand | 800 |
| Kakariko | Kakariko slate | 450 |
| Lon Lon Ranch | Lon Lon dusk | 250 |
| Hyrule Field | Caravan ochre | 300 |
| Castle Town | Market festival | 1,000 |

The render hook changes a temporary color for the current player or equipment
preview draw. It does not modify the shared tunic palette, actual equipped tunic,
protection rules or user CVars. An active tunic cosmetic override, a custom Link
model or a connected Anchor appearance takes priority; the purchased selection
is retained and the ledger explains why it is not currently displayed.

### Regional charters and treasuries

In adulthood, own and repair every listed business in a recovered region, then
visit it to buy its charter. Kakariko also requires the cottage. Castle Town
requires the funded exterior restoration to be active after leaving and returning.
Each charter brings a regional title, recognition in resident conversations and
a separate treasury. Holding all eight grants **High Steward of Hyrule**, also
recognized by the royal audience.

| Region | Charter price in bank rupees |
| --- | ---: |
| Castle Town | 120,000 |
| Hyrule Field | 35,000 |
| Lon Lon Ranch | 85,000 |
| Kokiri Forest | 18,000 |
| Kakariko | 60,000 |
| Death Mountain | 65,000 |
| Lake and Zora lands | 55,000 |
| Gerudo lands | 95,000 |

An operating adult business credits additional dues equal to 10% of its normal
period income, rounded down, to its chartered region. This uses the existing
business period once; it does not reduce bank income or tax cottage rent.
Treasuries hold up to **99,999,999 rupees per region**. A full bank does not
prevent dues, but a full treasury consumes the period without a queued payment.
The ledger transfers funds between the bank and a treasury only while visiting
that recovered region as an adult. Each transfer conserves money and changes both
balances together. Charters and balances survive age changes and an economy pause.

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
The same preference enables three small encounters using native enemy behavior:

| Place | Added enemy | Story and time requirement |
| --- | --- | --- |
| Death Mountain Trail | One red Tektite on an upper overlook | Adult, Fire Medallion, night |
| Zora's River | One blue Tektite in a water pocket | Child, Zora's Sapphire, night |
| Desert Colossus | One small Leever away from the temple approach | Adult, Spirit Medallion, Gerudo membership, day |

Hyrule Field has no added encounter. Each scene entry permits at most one
attempt, after arrival and only while the player is nearby. Unsafe ground,
nearby actors, unavailable native resources or conflicting enemy/asset/network
settings skip it. The Colossus also requires a rare quiet interval long enough
in the original Leever spawner; no extra Leever is guaranteed on a visit.
Added enemies have short lifetimes and bounded areas. Native encounters, quest
flags and room-clear rewards remain separate. No new enemy intelligence or
dungeon encounters are included.

## Persistence

**Save normally after transactions.** There is no transaction autosave; quitting
without saving discards later changes under the game's usual rules.

LivingHyruleSaveData, inline at SaveContext.ship.livingHyrule, is a fixed-size
plain C structure. It holds the enable flag, bank, cottage, rent ticks/earnings,
property/repair masks, sixteen business timers and business earnings, plus 23
signed trust values, meeting/favor masks, one active delivery, locked/requested
rent terms, Market and Domain restoration funding, castle-estate ownership,
23 per-person gift masks and the eight-deed royal recognition mask.
Nested wardrobe data holds eight ownership bits and one equipped style; nested
stewardship data holds eight charter bits and eight treasury balances. It has no
pointers or dynamic containers and is safe for the engine's copied save snapshot.
The background save callback reads that snapshot, capturing wallet and ledger
from the same moment. Global preferences are separate from per-save money.

The optional named section is livingHyrule, outer version **1**. Its
data.economy payload uses inner **schemaVersion 5**. Schemas **1 through 4**
migrate while preserving their existing money, property, timers, relationships,
rent terms, restoration funding, dyes, charters and treasuries. New schema-five
fields begin with no Domain investment, estate deed, gifts or royal recognition.
Earlier migrations retain their neutral trust, fair rent and empty module
defaults. The outer section remains version 1.
New saves without the section start with an empty, disabled economy.

The codec validates booleans, integer types/ranges, balances, ownership masks,
repair subsets and timer invariants before assigning live state. Current saves
require wardrobe/stewardship objects, exactly eight treasury entries, a valid
owned equipped style and a charter for every nonempty treasury. Schema five
also validates restoration/estate booleans, exactly 23 gift masks and the finite
recognition mask. Unknown optional keys within a supported schema are accepted.
Future versions or malformed payloads/envelopes make the ledger read-only.
The opt-in SaveManager fallback and snapshot save condition preserve the original section when the
rest of the game saves. Unrelated sections retain upstream behavior; file-wide
JSON corruption still belongs to upstream recovery.

Dynamic actor IDs, frozen dialogue state, scenery and heart-drop tracking remain
transient. Permanent social IDs are explicitly mapped from validated actor families.

## Build and verification

Use tools/living-hyrule/Build.ps1 actions Configure, Build and Stage on the
configured Windows workstation. Never use Run without an explicit user request.
Test.ps1 builds native policy/codec suites for banking, persistence, regional
properties, relationships, trade confirmation, population, movement, supplies,
challenge rules, wardrobe, stewardship and local restoration resources. Full
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

Full castle rooms, new shop interiors and broader building reconstruction;
broader property coverage and balanced expenses; deeper relationships and royal
daily life; wider walking/work/home routines, staffing and population growth;
new regional equipment models and deeper combat. Three bounded regional
encounters are installed; broader encounter variety remains unfinished. The full
[creative vision](C:/ZeldaDev/docs/LIVING-HYRULE-VISION.md) remains the direction.
The [population plan](LIVING-HYRULE-POPULATION.md) evaluates all 110 scene IDs,
including places that should retain solitude, puzzle space or quest atmosphere.
