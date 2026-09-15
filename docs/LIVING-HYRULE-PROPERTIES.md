# Regional property rules

Sixteen new property/business deeds accompany the original Kakariko cottage.
Open **Enhancements > Living Hyrule > Open Living Hyrule** and expand a region
under **Property and businesses across Hyrule**. Buying a deed requires visiting
that region's outdoor hub and having enough bank rupees. Existing quest NPCs
and shop functions remain available.

| Region | Property | Purchase | Income | Adult repairs |
| --- | --- | ---: | ---: | ---: |
| Castle Town | Market produce stall | 3,600 | 90 | 900 |
| Castle Town | Market guesthouse | 9,000 | 225 | 2,250 |
| Hyrule Field | South road orchard | 1,800 | 45 | 450 |
| Hyrule Field | Caravan supply yard | 3,200 | 80 | 800 |
| Lon Lon Ranch | Pasture lease | 6,000 | 150 | 1,500 |
| Lon Lon Ranch | Dairy partnership | 8,000 | 200 | 2,000 |
| Kokiri Forest | Seed garden | 600 | 15 | 150 |
| Kokiri Forest | Woodland workshop | 1,000 | 25 | 250 |
| Kakariko | Cloth workshop | 2,400 | 60 | 600 |
| Kakariko | Builders' yard | 4,000 | 100 | 1,000 |
| Death Mountain | Goron stoneworks | 4,800 | 120 | 1,200 |
| Death Mountain | Goron kiln partnership | 6,400 | 160 | 1,600 |
| Lake and Zora lands | Fishing cooperative | 2,800 | 70 | 700 |
| Lake and Zora lands | Waterway supplies | 4,400 | 110 | 1,100 |
| Gerudo lands | Caravan partnership | 7,200 | 180 | 1,800 |
| Gerudo lands | Textile workshop | 5,600 | 140 | 1,400 |

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
| Castle Town | Saved adventure-completion flag from defeating Ganon |
| Hyrule Field, Kokiri Forest | Forest Medallion |
| Lon Lon Ranch | Epona obtained |
| Kakariko | Shadow Medallion |
| Death Mountain | Fire Medallion |
| Lake and Zora lands | Water Medallion |
| Gerudo lands | Gerudo membership and Spirit Medallion |

Purchases/repairs use the outdoor market, field, ranch, forest, Kakariko,
Goron City/Death Mountain Trail, Lake Hylia/Zora's River/Domain, or Gerudo
Valley/Fortress, as appropriate. Sacred spaces and dungeons are not trade hubs.

Every business keeps its own partial income period while suspended. Economy
pause, conversations, blocking cutscenes, transitions, death, pause screens and
the port menu stop eligible ticks. At the bank cap, a completed income period
is consumed; uncredited money does not become a future debt. Normal game saves
persist the bank, deeds, repairs, progress and lifetime credited income together.

The original 1,200-rupee Kakariko cottage remains a legacy deed with its prior
25-rupee income and free resumption after the Shadow Medallion, so older saves
retain their original behavior.

## World changes and current limits

With Living Hyrule active, defeated Ganon and Adult Link, Redeads are removed
from the ruined market without payment. Other scenes' enemies are unaffected.
The code reads the existing adventure-completion flag; it does not replace the
ending, create a postgame save, or mark an unfinished adventure complete.

The new deeds, income, suspension and repairs execute as economy systems inside
the game. Transactions currently use the in-game ledger. Buildings do not yet
change appearance or gain interiors, and repairs do not yet spawn crews. Water
Temple completion reopens these businesses but does not thaw Domain geometry.
Owner/manager interactions and visible reconstruction are subsequent work.

Automated checks cover economic state and save migration. Runtime appearance,
control usability and the market enemy rule await the owner's gameplay test.
