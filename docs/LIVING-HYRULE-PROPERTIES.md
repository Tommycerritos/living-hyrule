# Regional property rules

Sixteen new property/business deeds accompany the original Kakariko cottage.
The current source adds resident purchase and adult repair conversations for
fifteen of those deeds, plus cottage and bank conversations in Kakariko. The
cloth workshop (property 8) remains ledger-only. This stage's regional actors,
dialogue, and supply props are being integrated for a combined build; their
appearance and controls await the owner's final gameplay test.

Enable the economy for the loaded save. With **Additional residents** enabled,
speak to the contact below when their schedule and regional story gate permit
it. The offer shows its bank-rupee cost and a **Yes / Not now** choice. Purchases
and repairs recheck the current file, bank, ownership, and story conditions when
confirmed; a repeated dialogue update cannot charge the same offer twice.

The ledger remains available at **Enhancements > Living Hyrule > Open Living
Hyrule**, under **Property and businesses across Hyrule**. Buying or repairing
requires visiting that region's outdoor hub. The original quest NPCs and shop
functions keep their own roles.

| ID | Region | Property | Resident contact | Purchase | Income | Adult repairs |
| --- | --- | --- | --- | ---: | ---: | ---: |
| 0 | Castle Town | Market produce stall | Vessa | 3,600 | 90 | 900 |
| 1 | Castle Town | Market guesthouse | Hadrin | 9,000 | 225 | 2,250 |
| 2 | Hyrule Field | South road orchard | Caro | 1,800 | 45 | 450 |
| 3 | Hyrule Field | Caravan supply yard | Hollis | 3,200 | 80 | 800 |
| 4 | Lon Lon Ranch | Pasture lease | Nessa | 6,000 | 150 | 1,500 |
| 5 | Lon Lon Ranch | Dairy partnership | Wren | 8,000 | 200 | 2,000 |
| 6 | Kokiri Forest | Seed garden | Fenn | 600 | 15 | 150 |
| 7 | Kokiri Forest | Woodland workshop | Luma | 1,000 | 25 | 250 |
| 8 | Kakariko | Cloth workshop | Ledger only | 2,400 | 60 | 600 |
| 9 | Kakariko | Builders' yard | Tavin, after cottage ownership | 4,000 | 100 | 1,000 |
| 10 | Death Mountain | Goron stoneworks | Doron | 4,800 | 120 | 1,200 |
| 11 | Death Mountain | Goron kiln partnership | Brakka | 6,400 | 160 | 1,600 |
| 12 | Lake and Zora lands | Fishing cooperative | Vero | 2,800 | 70 | 700 |
| 13 | Lake and Zora lands | Waterway supplies | Lethra | 4,400 | 110 | 1,100 |
| 14 | Gerudo lands | Caravan partnership | Rasha | 7,200 | 180 | 1,800 |
| 15 | Gerudo lands | Textile workshop | Kesra | 5,600 | 140 | 1,400 |

All figures are rupees. Income is per ten minutes of eligible active gameplay,
deposited automatically into the bank. Initial tuning uses income of 2.5% of
purchase price per period and repairs at 25% of purchase price. Final balance
needs actual play. There are no offline or seven-year-transition payouts.

Child Link can purchase outside Gerudo lands. In adulthood businesses suspend
until their region's crisis has been resolved and that particular property's
repairs have been paid. An adult purchase still needs repairs; the UI lists
both costs. Repair status survives traveling back and forth in time.

| Adult region | Unlock requirement |
| --- | --- |
| Castle Town | Saved final-boss defeat timestamp |
| Hyrule Field, Kokiri Forest | Forest Medallion |
| Lon Lon Ranch | Epona obtained |
| Kakariko | Shadow Medallion |
| Death Mountain | Fire Medallion |
| Lake and Zora lands | Water Medallion |
| Gerudo lands | Gerudo membership and Spirit Medallion |

Purchases/repairs use the outdoor market, field, ranch, forest, Kakariko,
Goron City/Death Mountain Trail, Lake Hylia/Zora's River/Domain, or Gerudo
Valley/Fortress, as appropriate. Sacred spaces and dungeons are not trade hubs.
Those are the ledger's regional hubs; a resident is only available at its own
authored stop, not every hub in the region. Lethra is on a dry lower river bank,
Rasha on the field-side valley approach, and Kesra at the lower fortress approach.
New Zora residents do not occupy the frozen adult Domain or Fountain.

Most contacts work by day. Adult Gerudo contacts additionally require all four
carpenter rescues and membership to appear; they can converse before Spirit
recovery, but cannot trade yet. Child Link can meet Rasha on the public valley
side without buying a Gerudo deed or gaining fortress access. Wren remains at the
ranch during its adult crisis and Lethra remains at the river during its crisis,
while their business offers wait for recovery. Vessa and Hadrin return after the
market relief gate even before their businesses are purchased or repaired.
See the [population tables](LIVING-HYRULE-POPULATION.md) for all schedules.

Every business keeps its own partial income period while suspended. Economy
pause, conversations, blocking cutscenes, transitions, death, pause screens and
the port menu stop eligible ticks. At the bank cap, a completed income period
is consumed; uncredited money does not become a future debt. Normal game saves
persist the bank, deeds, repairs, progress and lifetime credited income together.

The original 1,200-rupee Kakariko cottage remains a legacy deed with its prior
25-rupee income and free resumption after the Shadow Medallion, so older saves
retain their original behavior.

Tavin offers that cottage while it is unowned, then switches to the builders'
yard. Orlen offers to deposit the wallet balance, limited by bank space. Bram
offers to withdraw enough to fill the wallet, limited by available bank funds.
These are explicit one-time conversation choices; the ledger also supports
bank transactions. Pella, Edda, and Neris have conversation roles without a deed
sale, and no original shopkeeper is replaced.

## World changes and current limits

With Living Hyrule active, defeated Ganon and Adult Link, Redeads are removed
from the ruined market without payment. Other scenes' enemies are unaffected.
The code reads the saved final-boss defeat timestamp. A genuine final victory
queues a statistics-only save so it survives the ending and restart; ordinary
progress, wallet and ledger still use normal saves. Custom time-split completion
cannot unlock relief, and the original ending is preserved.

### Decorative supply arrangements

The seven property contacts in the Hylian regional module (Vessa, Hadrin, Caro,
Hollis, Nessa, Wren, and Vero) can have modest supply arrangements beside their
active worksite. The property must be owned, its region open, and the economy
enabled. One standard-scale wooden crate marks supplies awaiting adult repair;
three separated ground-level crates mark an operating business. Child businesses
use the operating arrangement where the child region is open.

There is at most one arrangement per property and three per scene. Each crate
needs a dry, sufficiently level floor, body clearance, and space from existing
actors, doors, and the player's interaction area. Supplies follow the presence
of the active resident; a failed placement check simply leaves them absent.
Changing scenes, disabling the feature, closing the property, or losing its
resident removes the decoration. No positions or new scenery fields are saved.

These props are business supplies, **not reconstructed buildings**. They have
no physical collision barrier, collectible drops, or quest behavior. They do not
add interiors, restore walls, create construction crews, thaw the Domain, or
restore the ruined town. Other property contacts and the legacy cottage do not
yet receive this decoration.

### What remains

The deeds, income, paid repairs, resident offers, and limited supplies are source
implementations of separate parts of the larger vision. Physical rebuilding,
interiors, staffing simulation, relationships, and population growth still need
their own systems. Water Temple completion permits business operations under
the stated rules without changing frozen geometry. The saved Ganon-defeat
timestamp supports limited market relief, not a new ending or complete postgame mode.

Native tests cover the economic state and save migration. This stage also adds
pure-policy checks for transaction choices, regional resident gates, scenery
ownership/recovery, and duplicate/density limits. Test source does not establish
that the combined build passed or that geometry and conversations work in play.
The integration build and the owner's final gameplay acceptance remain separate
checks; no gameplay automation was run for this stage.
