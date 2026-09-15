# Living Hyrule population: implementation and plan

## Scope and implementation status

This is a design for all **110 scene IDs**, `0x00` through `0x6D`, in the
[current scene table](../soh/include/tables/scene_table.h). It covers places that
should gain residents and places that should deliberately remain empty. Room,
entrance, and age variants within a scene still need individual placement work.

**The current stage has 23 unique resident identities:** three in the original
Kakariko module, nine Hylian townsfolk and travelers, four Kokiri/Goron residents,
and four Zora/Gerudo residents, plus Zelda, Captain Aren and Maelin in a postgame
royal audience. Shared favors, relationships and rent choices join direct trade.
The tables headed **Implemented in source** describe that code; they do not
claim a successful build or gameplay acceptance. The owner will test the final
playable result. No game launch is part of this implementation pass.

This remains a small population in selected settlements, not worldwide coverage
of the 110 scenes. The later cast, interiors, wider walking routes, deeper social
systems, population growth, and full reconstruction remain **Planned**.

The goal from the [local creative vision](C:/ZeldaDev/docs/LIVING-HYRULE-VISION.md)
is to make Hyrule feel inhabited while preserving its original adventure. Existing
quest actors, story items, songs, dungeons, rewards, and required routes keep their
roles. A new resident has a new identity and dialogue; reusing a model does not
make that resident a duplicate of its original character.

## Implemented in source: Kakariko cast

The **Additional residents** checkbox is independent of the per-file economy
enable control and defaults off. The initial three appear outdoors in Kakariko
in daylight, with the following story conditions:

| Resident | Work and character | Initial appearance | Child Link | Adult crisis | After Shadow Medallion |
| --- | --- | --- | --- | --- | --- |
| Tavin | Property carpenter; proud of sound roofs, plainspoken about the cottage and repairs | Carpenter model, distinct head, warm ochre clothing tint and a slightly taller stance | Present | Away | Returns |
| Bram | Boot-mender and cottage tenant; measures a recovery by how many people need walking shoes | Carpenter model, another head, slate-blue clothing tint and a shorter, slower idle | Present | Remains and discusses the disruption | Remains and discusses recovery |
| Orlen | Traveling supplier; remembers roads, prices, and which households need a delivery | Carpenter model, third head, moss-green clothing tint and a livelier idle | Present | Off the unsafe route | Returns |

All three are absent outdoors at night; no indoor
relocation is implied. They reuse the existing carpenter skeleton, head variants,
and clothing color support through new actor instances with their own behavior.
The palette descriptions are the art direction, not a promise of new textures or
equipment. Their dialogue can react to age, recovery, and cottage ownership.
They do not inherit the four original carpenters' rescue flags or schedules.

The shared transaction dialogue now gives Tavin the cottage purchase, then the
builders' yard deed and adult repairs after the cottage is owned. Bram offers a
bank withdrawal up to the wallet's available space; Orlen offers a deposit up to
the wallet balance and bank limit. Each offer shows the amount and a choice.
The ledger remains available, including for the cloth workshop, which has no
resident seller yet. No new building interior is implied by these residents.

## Implemented in source: regional cast and schedules

All families use the same **Additional residents** option. Ordinary daytime
residents can appear while the per-file economy is paused; purchases, repairs,
business-funded night shifts, and adult market relief require an enabled,
readable economy. Unsupported adventure modes, debug saves, cutscene scene
layers, and unrecognized locations are excluded. Actual spawning also needs
clear floor and interaction space, so an eligible schedule does not guarantee
that a candidate will be placed.

### Nine Hylian townsfolk and travelers

| Resident | Place and work | Child schedule | Adult schedule | Property dialogue |
| --- | --- | --- | --- | --- |
| Vessa | Market grocer | Daytime market | Daytime after the saved Ganon-defeat timestamp, with economy enabled; available before buying or repairing | Market produce stall (0) |
| Hadrin | Market porter | Daytime market | Same market relief gate as Vessa | Market guesthouse (1) |
| Pella | Market lantern keeper | Night market | Night after the same market relief gate | Conversation only |
| Caro | Field road courier | Daytime | Daytime after Forest Medallion | South road orchard (2) |
| Hollis | Field supply trader | Daytime | Daytime after Forest Medallion | Caravan supply yard (3) |
| Nessa | Ranch feed buyer | Daytime | Daytime after the original Epona escape | Pasture lease (4) |
| Wren | Ranch stable hand | Daytime; night if the dairy operates | Remains by day through the ranch crisis; night if the dairy operates | Dairy partnership (5), subject to the ranch trade gate |
| Vero | Lakeside net mender | Daytime | Daytime after Water Medallion | Fishing cooperative (12) |
| Edda | Lakeside research assistant | Daytime; night if the fishing cooperative operates | Remains by day during the water crisis; night if the cooperative operates | Conversation only |

These actors use compatible civilian skeleton/head families with individual
clothing palettes, proportions, and idle poses. They stand at authored work or
travel stops; walking journeys and indoor relocation are not implemented.
Adult market relief reads the saved final-boss defeat timestamp. Genuine final
victory records statistics separately so that evidence survives restarting.
This does not reconstruct the market or automatically save wallet/ledger changes.

### Four Kokiri and Gorons

| Resident | Place and work | Daytime availability | Night availability | Property dialogue |
| --- | --- | --- | --- | --- |
| Fenn | Kokiri Forest seed sorter | Child; adult after Forest Medallion | Absent | Seed garden (6) |
| Luma | Kokiri Forest gatherer and craft worker | Child; adult after Forest Medallion | Absent | Woodland workshop (7) |
| Doron | Goron City stone grader | Child; adult after Fire Medallion | Absent | Goron stoneworks (10) |
| Brakka | Goron City kiln tender | Child; adult after Fire Medallion | Present only with an operating kiln | Goron kiln partnership (11) |

The forest pair is limited to Kokiri Forest room 0 and retains Kokiri bodies in
both eras. The Goron pair uses the main cavern's lower walkway in room 3, with
child dialogue acknowledging the food shortage before the Goron Ruby. Neither
pair appears at its worksite during the corresponding adult crisis; no hidden
indoor schedule is implied. These modules use their native Kokiri and Goron art
families with their own behavior, not the original actors' quest or reward logic.

### Four Zoras and Gerudos

| Resident | Place and work | Daytime availability | Night availability | Property dialogue |
| --- | --- | --- | --- | --- |
| Lethra | Spring keeper on a dry lower Zora's River bank | Child and adult; remains as a refugee during the water crisis | Absent | Waterway supplies (13); adult trade waits for Water Medallion |
| Neris | Zora courier on the same river bank | Child; adult after Water Medallion | Present only with operating waterway supplies | Conversation only |
| Rasha | Caravan quartermaster on the public, field-side high ground in Gerudo Valley | Child as a nontrading visitor; adult after all four carpenter rescues and membership | Adult with those access conditions and an operating caravan partnership | Caravan partnership (14); adult trade also needs Spirit Medallion |
| Kesra | Cloth trader at the lower fortress common approach | Adult after all four carpenter rescues and membership | Same access conditions and an operating textile workshop | Textile workshop (15); trade also needs Spirit Medallion |

Zoras use their native skeleton and skin; Gerudos use compatible civilian art,
distinct hairstyles, poses, and modest scale differences. New actors have no
arrest, rescue, membership-card, archery-reward, or gate authority. Daytime Gerudo
conversation before Spirit recovery does not permit a purchase. No new Zoras
spawn in the frozen Domain or Fountain, and no thaw is implemented.

### Visible business supplies

The seven Hylian property contacts (Vessa, Hadrin, Caro, Hollis, Nessa, Wren, and
Vero) can anchor decorative supplies when their property is owned, its region is
open, and the economy is enabled. One standard-size wooden crate represents
supplies awaiting adult repairs; three ground-level crates mark an operating
business. At most three arrangements appear in one scene, with individual floor,
water, body, actor, door, and interaction-space checks. If no candidate passes,
no arrangement appears.

Supplies are removed when their trader or property is unavailable. They have no
collision barrier, drops, quest behavior, or saved placement. They do not restore
buildings, add interiors, or depict working construction crews. The other resident
families and the legacy cottage do not yet have these supply props.

## Population rules for later regions

### Story and recovery

**C** below means Child Link, **A** means the adult region during its crisis,
**R** means its story problem has been resolved, and **P** means broader planned
persistent postgame life. The implemented market relief gate above is a limited
use of the saved final-boss defeat timestamp, not that wider postgame system.

- **Childhood:** settlements are busy at sensible hours. Preserve local problems,
  such as Goron hunger before Dodongo's Cavern is resolved; prosperity is not a
  reason to erase the original story.
- **Forest:** adults find fewer outdoor Kokiri and more danger. Forest Temple
  completion permits a return to paths and work areas, without relocating Saria
  or changing her role.
- **Death Mountain:** Goron capture and the Fire Temple crisis empty work sites.
  Story recovery allows the new workers to return. Paid business repairs reopen
  income; physical restoration and construction work remain later phases.
- **Water region:** the Water Temple allows the lake economy to recover. Zora's
  Domain remains visibly frozen in the original adult game, so the medallion
  alone must not place swimming residents inside ice. Thawing or restored geometry
  needs a separate implemented recovery stage.
- **Kakariko:** the first population uses the Shadow Medallion as its adult return
  condition. Bram can remain during the crisis without enabling rent or sales.
- **Gerudo territory:** membership, the rescued carpenters, local security, and
  Spirit Temple recovery each have distinct meanings. A friendly new resident
  must not bypass fortress access or turn every Gerudo into a shopkeeper.
- **Castle Town:** ordinary civilian life does not return to the ruined adult
  market during the main quest. The current relief population requires the saved
  Ganon-defeat timestamp and enabled economy, matching the market enemy cleanup.
  Building restoration remains separate work.
- **Postgame:** entering an ending map is not proof of adventure completion.
  Current relief reads the existing saved flag; new ending flow, postgame save
  creation, castle life, and broad restored-world behavior remain future work.

### Density, time, and routes

- The numbers below refer to **additional** residents. Start with two or three
  in a village, one or two in a small interior, and isolated pairs on safe roads.
  Busy areas can grow only after actor count and performance checks.
- Give each resident a reason to be there: work, a delivery, shelter, a visit,
  worship, fishing, study, or travel. Standing everywhere is not a schedule.
- Most civilian work happens by day. Night shifts belong to guards, inn staff,
  a few late traders, and residents near lit entrances. Streets become quieter.
- Use a single resident identity across outdoor and indoor schedules so that the
  same person cannot appear in both places. Do not swap an actor while speaking,
  in view, or holding the player's attention; reevaluate at a safe boundary.
- Time does not advance uniformly in all scenes. Read the current day/night
  state, but do not silently introduce a second clock or offline travel simulation.
- Keep doorways, ladders, narrow bridges, shops' interaction space, cutscene
  camera paths, Cucco routes, minigame lanes, and puzzle objects clear.
- Dangerous spaces remain dangerous. A solitary lookout at an entrance can make
  a place feel connected to society without filling its dungeon with civilians.

### Asset families and distinct identities

Reuse assets already available in the player's local game. Prefer per-instance
clothing colors, compatible head variants, modest proportions, facing, and idle
poses. Do not recolor a shared texture or change a global model table in a way
that alters original NPCs. New external assets require the repository's normal
authorship and permission review.

| Region or role | Planned visual family | Variation and boundaries |
| --- | --- | --- |
| Kakariko, market, ranch, Hylian travelers | `OBJECT_DAIKU` workers and the `En_Hy` civilian families (`CNE`, `BOJ`, `AHG`, `BJI`, and others) | Mix working and civilian silhouettes. Use compatible heads and clothing palettes; do not import dog, bottle, trade, or reward logic. |
| Kokiri settlements and forest paths | `OBJECT_KM1`, `OBJECT_KW1`, and their compatible Kokiri heads | Forest colors, tunic and boot variation, playful or watchful idle poses. Preserve the Kokiri's childlike appearance across Link's age change. |
| Goron City and mountain work sites | `OBJECT_OF1D_MAP` / Goron skeleton | Adult-sized workers with restrained scale and pose variation; sitting, resting, and work roles. Do not clone Biggoron's trading state or rolling hazards. |
| Zora waterways | `OBJECT_ZO` / Zora skeleton | Bank-side, wading, or swimming roles only where geometry supports them. Gentle color and pose changes; do not treat different species as palette swaps. |
| Gerudo settlements and caravans | `OBJECT_GE1` / white-clothed Gerudo skeleton and supported hairstyles | Bob, straight, and spiky hairstyles; individual clothing palettes and jobs. Exclude arrest, gate, membership, and archery reward behavior from new civilian actors. |
| Guards, clergy, caretakers | Appropriate existing Hylian bodies and outfits | Roles should be recognizable without copying named story characters, unique royal silhouettes, or their story authority. |
| Fairies and sacred locations | Existing fairy effects only where appropriate | Preserve solitude and the original reward ceremonies; no ordinary residents crowding a fountain. |

## Wider regional cast and future roles

The 20 names in the implementation tables above now have source implementations
for only those locations and schedules. All other names below, and additional
roles or places for existing names, remain proposals. New names are working names.

| Region | Cast and daily life |
| --- | --- |
| Kakariko and graveyard | **Mira**, a cloth-mender who trades hems for village news; **Sella**, a night steward at the guest house; **Orris**, a mason who tends paths and gravestones; **Iven**, a young courier learning the mountain road. Mira is intended for a civilian female model, expanding the initial carpenter-family silhouettes. |
| Castle Town and castle | **Vessa**, a grocer who remembers displaced customers; **Hadrin**, a porter who knows every delivery entrance; **Pella**, a lantern keeper; **Corren**, a junior guard; **Meret**, a records clerk; **Alda**, a garden worker. In the future postgame they discuss safety, rebuilding, and who has returned. |
| Hyrule Field and Lon Lon Ranch | **Nessa**, a feed buyer; **Jory**, a fence repairer; **Caro**, a road courier; **Hollis**, a mule-and-cart trader represented initially without a new vehicle; **Wren**, a stable hand who worries about frightened horses. Their routes connect settlements instead of creating permanent roadside crowds. |
| Kokiri Forest and Lost Woods | **Fenn**, a seed sorter; **Luma**, a berry gatherer; **Tavi**, a path watcher; **Nell**, a game organizer; **Bori**, a leaf-and-bark craftsperson. They stay within believable forest boundaries and shelter during the adult crisis. |
| Death Mountain and Goron City | **Doron**, a stone grader; **Brakka**, a kiln tender; **Muro**, a hauling foreman; **Gorrin**, a young apprentice; **Rukka**, a rest-stop cook. Their jobs give the food shortage, captivity, and later recovery an everyday meaning. |
| Zora's River, Domain, and Lake Hylia | **Sori**, a reed fisher; **Lethra**, a Zora spring keeper; **Neris**, a Zora courier; **Vero**, a net mender; **Edda**, a lakeside research assistant. Separate water-adapted residents from Hylian shore workers. |
| Gerudo Valley, Fortress, and desert | **Rasha**, a caravan quartermaster; **Demi**, a well tender; **Kesra**, a cloth trader; **Tareh**, a local scout; **Mava**, a trainer; **Suri**, a shrine caretaker. They have their own community and priorities, not merely services for Link. |
| Shared small venues | **Traveling apprentices, visiting relatives, pilgrims, customers, and relief workers** fill limited roles from these regional casts. Reuse an identified traveler across a route instead of inventing the same anonymous crowd in every shop or grotto. |

## Scene-by-scene coverage

Every entry below is **Planned** unless explicitly marked **Source**.
**Source** means implemented code awaiting this stage's build and gameplay checks.
**Excluded** means no additional population is proposed in that
scene; the original actors remain. A recovered location is not automatically a
safe NPC location: room layout and quest interactions still need review.

### Dungeons, trials, and the treasure shop

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x00` | `SCENE_DEKU_TREE` | No new civilians in the dungeon. Fenn may discuss it outside. | Preserve C dungeon and boss access; no adult or night resettlement. |
| `0x01` | `SCENE_DODONGOS_CAVERN` | Planned: at most one Goron surveyor in a proven safe entrance room after clearance. | No workers before the original danger ends; no traffic through puzzles. |
| `0x02` | `SCENE_JABU_JABU` | Excluded from added residents. | A living creature and quest dungeon, not a place to populate. |
| `0x03` | `SCENE_FOREST_TEMPLE` | No ordinary residents; optional future study visit limited to a safe outer vestibule. | Only R, daytime, and after room review; preserve mystery and all puzzle rooms. |
| `0x04` | `SCENE_FIRE_TEMPLE` | Preserve original captive Gorons; do not duplicate them. | Planned R inspection at a safe entrance only; no workers during captivity or among hazards. |
| `0x05` | `SCENE_WATER_TEMPLE` | No ordinary residents or swimming crowds. | Optional R Zora observer in a safe entrance area after water-state review. |
| `0x06` | `SCENE_SPIRIT_TEMPLE` | Planned: Suri may attend an outer shrine space after resolution. | Keep both ages' puzzle routes, Nabooru events, and interior combat clear. |
| `0x07` | `SCENE_SHADOW_TEMPLE` | Excluded from everyday civilian population. | Retain the dungeon's isolation even after R. Memorial activity belongs outside. |
| `0x08` | `SCENE_BOTTOM_OF_THE_WELL` | Excluded from added residents. | No civilian should block child access, hidden routes, or horror scenes. |
| `0x09` | `SCENE_ICE_CAVERN` | No ordinary residents. | A future escorted survey is separate content, not ambient population; no spawning into ice. |
| `0x0A` | `SCENE_GANONS_TOWER` | Excluded. | Endgame combat and story route stay unchanged. |
| `0x0B` | `SCENE_GERUDO_TRAINING_GROUND` | Planned: Mava can supervise near a safe entrance after authorized access. | No new residents inside tests, locked routes, or reward rooms; daylight training context. |
| `0x0C` | `SCENE_THIEVES_HIDEOUT` | No additional people during the rescue mission. | Planned R off-duty Gerudo presence in an audited common room only; preserve prison and patrol logic. |
| `0x0D` | `SCENE_INSIDE_GANONS_CASTLE` | Excluded. | No civilian trade or recovery crowd among the trials. |
| `0x0E` | `SCENE_GANONS_TOWER_COLLAPSE_INTERIOR` | Excluded. | Timed escape and Zelda's path take precedence. |
| `0x0F` | `SCENE_INSIDE_GANONS_CASTLE_COLLAPSE` | Excluded. | No added actors in collapse sequences. |
| `0x10` | `SCENE_TREASURE_BOX_SHOP` | Planned: one waiting customer only if a separate lobby space permits it. | Open hours only; never in box rooms or attached to the minigame's outcome flags. |

### Boss arenas and collapse exteriors

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x11` | `SCENE_DEKU_TREE_BOSS` | Excluded. | Gohma, rewards, and exit warp remain alone. |
| `0x12` | `SCENE_DODONGOS_CAVERN_BOSS` | Excluded. | King Dodongo arena, lava, rewards, and exit warp stay clear. |
| `0x13` | `SCENE_JABU_JABU_BOSS` | Excluded. | Preserve Barinade and the original exit sequence. |
| `0x14` | `SCENE_FOREST_TEMPLE_BOSS` | Excluded. | Preserve Phantom Ganon's arena and rewards. |
| `0x15` | `SCENE_FIRE_TEMPLE_BOSS` | Excluded. | Preserve Volvagia's arena and rewards. |
| `0x16` | `SCENE_WATER_TEMPLE_BOSS` | Excluded. | Preserve Morpha's arena and changing water context. |
| `0x17` | `SCENE_SPIRIT_TEMPLE_BOSS` | Excluded. | Preserve Twinrova's battle and story sequence. |
| `0x18` | `SCENE_SHADOW_TEMPLE_BOSS` | Excluded. | Preserve Bongo Bongo's platform, battle, and rewards. |
| `0x19` | `SCENE_GANONDORF_BOSS` | Excluded. | No additional actors during the confrontation. |
| `0x1A` | `SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR` | Excluded. | Timed escape, camera paths, and rubble remain unobstructed. |

### Castle Town and Temple of Time exteriors

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x1B` | `SCENE_MARKET_ENTRANCE_DAY` | Planned: Hadrin unloading, Corren checking deliveries, one passing courier. | C day; keep the gate, guard, and bridge approaches clear. |
| `0x1C` | `SCENE_MARKET_ENTRANCE_NIGHT` | Planned: Pella and one relief guard. | C night; preserve the original gate restriction and night-guard interaction. |
| `0x1D` | `SCENE_MARKET_ENTRANCE_RUINS` | No new civilian traffic in A. Planned P relief deliveries after safety is implemented. | No automatic recovery during the main quest; quieter guarded nights in future P. |
| `0x1E` | `SCENE_BACK_ALLEY_DAY` | Planned: Mira visiting a tailor, Hadrin with deliveries, one resident at a doorway. | C day; retain room for original dog, trade, and shop interactions. |
| `0x1F` | `SCENE_BACK_ALLEY_NIGHT` | Planned: Pella tending lights and one late-returning resident. | C night; a quieter alley, not a second daytime market. |
| `0x20` | `SCENE_MARKET_DAY` | **Source:** Vessa and Hadrin. Additional customers remain planned. | Child day; adult only with the saved Ganon-defeat timestamp and enabled economy. |
| `0x21` | `SCENE_MARKET_NIGHT` | **Source:** Pella tending lights. | Child night; adult only with the saved Ganon-defeat timestamp and enabled economy. Preserve the dog search and doors. |
| `0x22` | `SCENE_MARKET_RUINS` | **Source:** Vessa/Hadrin by day and Pella by night after the relief gate. | Adult, saved Ganon-defeat timestamp, enabled economy. No civilians before that gate, no rebuilt geometry or construction crews. |
| `0x23` | `SCENE_TEMPLE_OF_TIME_EXTERIOR_DAY` | Planned: Meret consulting records and one quiet pilgrim away from the entrance. | C day; no crowd over the story approach or Gossip Stones. |
| `0x24` | `SCENE_TEMPLE_OF_TIME_EXTERIOR_NIGHT` | Planned: one watchful caretaker near an existing safe edge. | C night; keep the temple's stillness. |
| `0x25` | `SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS` | No added everyday visitors in A; planned P caretaker after town safety. | Preserve adult story arrival and the ruined landscape. |

### Homes, shops, and working interiors

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x26` | `SCENE_KNOW_IT_ALL_BROS_HOUSE` | Planned: one visiting Kokiri learner, such as Fenn. | C day or A shelter; no replacement for the original tutorials. |
| `0x27` | `SCENE_TWINS_HOUSE` | Planned: Luma or Nell on a rotating visit. | One visitor maximum; evening shelter instead of duplicate outdoor presence. |
| `0x28` | `SCENE_MIDOS_HOUSE` | No permanent extra residents; occasional planned craft delivery after access review. | Respect Mido's home and original chests; no forced social quest. |
| `0x29` | `SCENE_SARIAS_HOUSE` | Preserve as Saria's personal space. | A crisis and R do not turn her absence into a busy public building. |
| `0x2A` | `SCENE_KAKARIKO_CENTER_GUEST_HOUSE` | Planned: Sella and a rotating traveler; later an indoor schedule for the Kakariko cast if space allows. | Night lodging and A refuge; preserve Talon and the original carpenters. No current indoor implementation. |
| `0x2B` | `SCENE_BACK_ALLEY_HOUSE` | Planned: a small family or mender visit appropriate to the actual entrance. | One additional person; verify scene reuse and household identity before assigning names. |
| `0x2C` | `SCENE_BAZAAR` | Planned: one customer or stock clerk. | Distinguish Market and Kakariko entrances; local cast and business hours, no duplicate shopkeeper or shelf logic. |
| `0x2D` | `SCENE_KOKIRI_SHOP` | Planned: Bori delivering craft materials or one Kokiri customer. | Day trade, quiet at night; preserve the shopkeeper and stock. |
| `0x2E` | `SCENE_GORON_SHOP` | Planned: Doron checking supplies when trade is available. | Reflect food shortage, adult crisis, and R; never obstruct the original counter. |
| `0x2F` | `SCENE_ZORA_SHOP` | Planned: Lethra or a Zora customer only in an accessible, visibly safe variant. | Frozen adult access is not cured by a population flag. |
| `0x30` | `SCENE_POTION_SHOP_KAKARIKO` | Planned: a regional apprentice behind a safe work area, or one patient. | A crisis can increase concern, not block required medicine and trade. |
| `0x31` | `SCENE_POTION_SHOP_MARKET` | Planned: one apprentice or customer tied to Vessa's neighborhood. | C open hours; keep buying and required shop navigation clear. |
| `0x32` | `SCENE_BOMBCHU_SHOP` | Planned: one discreet late customer if clearance permits. | Night business; no new Bombchu inventory, prices, or access rules implied. |
| `0x33` | `SCENE_HAPPY_MASK_SHOP` | Planned: one curious child or visitor at the entrance side. | C open hours; preserve every mask-trade interaction and original owner. |
| `0x34` | `SCENE_LINKS_HOUSE` | Excluded from automatic ambient visitors. | Link's home stays private; invited guests require a future explicit interaction. |
| `0x35` | `SCENE_DOG_LADY_HOUSE` | No extra permanent household. | Preserve the lost-dog quest and its interaction space; a future neighbor visit needs separate review. |
| `0x36` | `SCENE_STABLE` | Planned: Wren tending the stable and occasional Nessa delivery. | C work and night care; A behavior follows ranch control, with horse routes kept clear. |
| `0x37` | `SCENE_IMPAS_HOUSE` | Planned: at most a visiting mender in an audited common space. | Preserve Impa's identity, existing occupants, exits, and collectible access. |
| `0x38` | `SCENE_LAKESIDE_LABORATORY` | Planned: Edda recording samples. | C active study; A concern over the lake; R renewed work. No interference with diving or trade. |
| `0x39` | `SCENE_CARPENTERS_TENT` | Planned: one new logistics worker or Orlen on a scheduled supply visit. | Adult rescue context first; do not add a fifth rescued carpenter or rewrite rescue flags. |
| `0x3A` | `SCENE_GRAVEKEEPERS_HUT` | No automatic permanent resident. Planned caretaker visit only after household review. | Preserve Dampe's place and story; Orris works outside first. |

### Fountains, grottos, and tombs

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x3B` | `SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC` | Excluded from added ordinary residents. | Keep the Great Fairy ceremony, instrument spot, and reward sequence unobstructed. |
| `0x3C` | `SCENE_FAIRYS_FOUNTAIN` | Preserve as a quiet refuge with its existing fairies. | No trader crowds or extra people in shared fountain entrances. |
| `0x3D` | `SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS` | Excluded from added ordinary residents. | Each entrance keeps its original spell and story context. |
| `0x3E` | `SCENE_GROTTOS` | Planned only by explicit entrance/content identity: a rare known traveler in a safe shelter grotto. | Default zero; no residents in hostile, flooded, scrub-trade, or puzzle variants. Scene ID alone is insufficient. |
| `0x3F` | `SCENE_REDEAD_GRAVE` | Excluded. | Preserve danger and the grave's purpose; no living tenant. |
| `0x40` | `SCENE_GRAVE_WITH_FAIRYS_FOUNTAIN` | Excluded from added ordinary residents. | Quiet hidden refuge, with original fairy and exit behavior. |
| `0x41` | `SCENE_ROYAL_FAMILYS_TOMB` | Excluded. | Royal dead, combat, and the song puzzle retain their space and tone. |

### Public venues, sacred interiors, and story maps

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x42` | `SCENE_SHOOTING_GALLERY` | Planned: one waiting competitor behind the firing line. | Distinguish Market and Kakariko entrance/age variants; never count as a target or block the lane. |
| `0x43` | `SCENE_TEMPLE_OF_TIME` | No permanent extra crowd. A future silent caretaker stays well away from the altar. | Exclude all story/age-change setups and the Master Sword approach. |
| `0x44` | `SCENE_CHAMBER_OF_THE_SAGES` | Excluded. | Sacred story stage, not a population scene. |
| `0x45` | `SCENE_CASTLE_COURTYARD_GUARDS_DAY` | Excluded from additional patrols or servants in the first population passes. | Preserve the exact child stealth challenge and sightlines. |
| `0x46` | `SCENE_CASTLE_COURTYARD_GUARDS_NIGHT` | Excluded. | Preserve the original night restriction and guards. |
| `0x47` | `SCENE_CUTSCENE_MAP` | Excluded. | No ambient spawning in cutscene-only staging. |
| `0x48` | `SCENE_WINDMILL_AND_DAMPES_GRAVE` | Planned: one windmill-side maintenance visitor only after room/entrance discrimination. | Zero additions in Dampe's race route, grave, or song interaction space. |
| `0x49` | `SCENE_FISHING_POND` | Planned: Sori or one shore spectator, never an extra competing fishing actor initially. | Day leisure, quiet nights; preserve proprietor, fish AI, records, and prizes. |
| `0x4A` | `SCENE_CASTLE_COURTYARD_ZELDA` | Excluded during the original story. | Future P castle life needs its own explicit setup; do not place a second Zelda here. |
| `0x4B` | `SCENE_BOMBCHU_BOWLING_ALLEY` | Planned: one spectator in an audited lobby or side area. | Open hours; bowling lanes, camera, chickens, targets, and rewards stay clear. |
| `0x4C` | `SCENE_LON_LON_BUILDINGS` | Planned: Nessa visiting the household or Wren at a work area, selected per room. | Preserve Talon's game, cows, bedroom context, and day/night occupancy. |
| `0x4D` | `SCENE_MARKET_GUARD_HOUSE` | Planned: Corren or a relief guard near a clear wall. | C duty changes; preserve pots, collectible access, and the adult scene's actual condition. |
| `0x4E` | `SCENE_POTION_SHOP_GRANNY` | No permanent extra cast initially. | Preserve Granny's secluded shop and adult trade timing; a future apprentice requires room review. |
| `0x4F` | `SCENE_GANON_BOSS` | Excluded. | Final battle, Zelda, collapse geometry, and ending stay untouched. |
| `0x50` | `SCENE_HOUSE_OF_SKULLTULA` | No new ordinary residents during the curse. | Planned optional visitors only after a separately checked safe household state; no duplicate reward family. |

### Overworld and settlements

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x51` | `SCENE_HYRULE_FIELD` | **Source:** Caro and Hollis at fixed travel stops. Walking routes and further pairs remain planned. | Day only: child or adult with Forest Medallion. No night additions among Stalchildren or across Epona routes. |
| `0x52` | `SCENE_KAKARIKO_VILLAGE` | **Source:** Tavin, Bram, and Orlen; additional regional cast remains planned. | Day only: child all three; adult crisis Bram only; Shadow recovery all three. No current night or indoor schedule. |
| `0x53` | `SCENE_GRAVEYARD` | Planned: Orris tending paths and one respectful visitor. | Day work; sparse dusk mourning; no blocking tombs, race access, Dampe, or the Shadow Temple route. |
| `0x54` | `SCENE_ZORAS_RIVER` | **Source:** Lethra and Neris on a dry lower bank. Sori and other routes remain planned. | Day: Lethra in both eras, Neris as child or after Water Medallion. Night: Neris only with operating waterway supplies. |
| `0x55` | `SCENE_KOKIRI_FOREST` | **Source:** Fenn and Luma in room 0. Nell, Bori, and further work groups remain planned. | Day only: child or adult with Forest Medallion. No adult-crisis or indoor substitute placement. |
| `0x56` | `SCENE_SACRED_FOREST_MEADOW` | Planned: at most Tavi near the safe approach after Forest R. | Sparse by design; no villagers in the maze during danger or beside Saria's story position. |
| `0x57` | `SCENE_LAKE_HYLIA` | **Source:** Vero and Edda at lakeside stops. Sori and further visitors remain planned. | Day: Edda in both eras; Vero as child or after Water Medallion. Night: Edda only with an operating fishing cooperative. |
| `0x58` | `SCENE_ZORAS_DOMAIN` | Planned: Lethra, Neris, and small Zora households on appropriate ledges and water routes. | C inhabited; frozen A sparse/absent as geometry demands. R return requires actual safe/thawed areas, not the Water Medallion alone. |
| `0x59` | `SCENE_ZORAS_FOUNTAIN` | Planned: one Zora spring keeper at a safe outer ledge. | C quiet stewardship; A no invented open-water work through ice; later restoration stage must match geometry. Jabu-Jabu and access routes remain clear. |
| `0x5A` | `SCENE_GERUDO_VALLEY` | **Source:** Rasha on field-side high ground. Orlen's visit and a lookout remain planned. | Child daytime conversation only. Adult requires all four rescues and membership; Spirit additionally gates trade. Adult night needs an operating caravan partnership. |
| `0x5B` | `SCENE_LOST_WOODS` | Planned: Tavi at a known safe junction and rare Kokiri gathering visits. | C sparse; A more caution; Forest R limited return. Never mark every exit with helpful crowds or intrude on Skull Kid and trade encounters. |
| `0x5C` | `SCENE_DESERT_COLOSSUS` | Planned: Suri near a safe outer shrine and a rare Gerudo expedition. | Desert travel stays exceptional; Spirit R cautious visits. No day/night village crowd or obstruction of warp, oasis, bean, and temple routes. |
| `0x5D` | `SCENE_GERUDOS_FORTRESS` | **Source:** Kesra at a lower common approach. Demi, Mava, and other workers remain planned. | Adult only, after all four rescues and membership. Spirit additionally gates trade; night needs an operating textile workshop. No rescue or patrol override. |
| `0x5E` | `SCENE_HAUNTED_WASTELAND` | Planned: one Tareh-led caravan rest point only if a safe route is deliberately defined. | Default sparse or zero; no crowd breadcrumb trail, free guide, or override of the original navigation challenge. |
| `0x5F` | `SCENE_HYRULE_CASTLE` | Planned: Alda or Hadrin in publicly reachable outer work areas. | C day labor outside the stealth route; night sparse. Future P castle workforce needs explicit restored access and geometry. |
| `0x60` | `SCENE_DEATH_MOUNTAIN_TRAIL` | Planned: Doron and a hauling partner at safe work bays, Iven on the lower road. | C shortage-aware; A reduced labor; Fire R return. No workers in falling-rock zones, narrow climbing lanes, or Biggoron's trade space. |
| `0x61` | `SCENE_DEATH_MOUNTAIN_CRATER` | Planned: at most a Goron specialist near a proven safe ledge after Fire R. | Hazardous region stays sparse; no ordinary Hylian work crew or actors on heat/warp routes. |
| `0x62` | `SCENE_GORON_CITY` | **Source:** Doron and Brakka on the main cavern's lower walkway, room 3. Other workers and mountain routes remain planned. | Child or adult after Fire Medallion: both by day; Brakka at night only with an operating kiln. No ordinary workers during adult captivity. |
| `0x63` | `SCENE_LON_LON_RANCH` | **Source:** Nessa and Wren. Jory, work animations, and indoor relocation remain planned. | Day: Wren in both eras; Nessa as child or after Epona escape. Night: Wren only with an operating dairy. Races and horse access retain their original roles. |
| `0x64` | `SCENE_OUTSIDE_GANONS_CASTLE` | **Source:** Zelda, Captain Aren and Maelin after the saved Ganon victory. | Adult normal room0, valid enabled economy and Additional residents; day all3, night Aren. Verified lower approach positions; original bridge and castle interior remain untouched. |

### Debug-only scenes

| ID | Scene | Population decision | Story and schedule |
| --- | --- | --- | --- |
| `0x65` | `SCENE_TEST01` | Excluded from production population. | Development scene; no ambient schedule. |
| `0x66` | `SCENE_BESITU` | Excluded from production population. | Development scene; no ambient schedule. |
| `0x67` | `SCENE_DEPTH_TEST` | Excluded from production population. | Rendering test scene; no ambient schedule. |
| `0x68` | `SCENE_SYOTES` | Excluded from production population. | Development scene; no ambient schedule. |
| `0x69` | `SCENE_SYOTES2` | Excluded from production population. | Development scene; no ambient schedule. |
| `0x6A` | `SCENE_SUTARU` | Excluded from production population. | Development scene; no ambient schedule. |
| `0x6B` | `SCENE_HAIRAL_NIWA2` | Excluded from production population. | Debug courtyard is not the playable castle's population target. |
| `0x6C` | `SCENE_SASATEST` | Excluded from production population. | Development scene; no ambient schedule. |
| `0x6D` | `SCENE_TESTROOM` | Excluded from production population. | Development scene; no ambient schedule. |

## Implementation architecture

### New actor identity, reused art

Register a dedicated actor through `ActorDBInit` with a unique name and a dynamic
ID (`id = -1`). Store the returned ID; never replace an existing actor database
entry. Keep its runtime storage fixed-size, beginning with `Actor`, and supply
its own initialization, update, dialogue, draw, and destruction functions.
The [Ivan registration](../soh/soh/Enhancements/ExtraModes/IvanCoop.cpp) demonstrates
the supported registration API.

The initial carpenter family can reuse `object_daiku_Skel_007958` and its four
compatible head display lists. Its renderer already supports clothing color
variation. Reuse this visual structure, not `EnDaikuKakariko_Init` or its talking
and path actions: the original actor is age/time dependent and connected to the
original cast. Similarly, `EnHy_UpdateTalkState` changes vanilla information
flags and can award dog/bottle rewards; it is not a general-purpose new-resident
dialogue function.

Add a small data-driven visual profile for each later family. A profile identifies
compatible skeleton, animation, heads, draw callbacks, colors, scale limits, and
collider dimensions. Do not swap unrelated skeletons into the same joint table.
Ensure required objects are loaded before initialization; `Object_GetIndex` can
return `-1`, and `Object_Spawn` has finite bank and memory limits. Missing resources
or allocation failure should skip an optional resident, not break scene loading.

### Resident definitions and schedules

The current modules define stable resident IDs, visual profiles, explicit scene
candidates, day/night and story policies, and custom dialogue. Future route and
interior definitions also need room/entrance conditions and home locations.
Scene transforms and runtime actor IDs are not resident identities and should
not be saved as such.

The current population modules register their own actors and use throttled
gameplay hooks with scene/room and live-actor checks. Deduplicate by resident ID,
and defer schedule removal until pending/open conversation has finished.
`OnSceneSpawnActors` is another available hook for future room-specific work;
it can run again on room loading. Exclude title, file-select, debug saves,
unsupported adventure modes, and cutscene scene layers. Do not spawn during an
active transition, blocking story sequence, or before collision and resource
dependencies are available.

Shared scenes require more than their scene number: the two bazaars, shooting
galleries, fountains, grottos, house variants, and the windmill/grave combination
must inspect the entrance, room, spawn, or content identity. Default to no extra
residents when the context is not explicitly recognized.

### Dialogue and transactions

Use `Npc_UpdateTalking` with new callbacks or an equivalent small talk state
machine. It offers normal interaction within range and on screen. Give new
residents reserved custom text IDs and serve them through `OnOpenText` with
`CustomMessage::AutoFormat`, `LoadIntoFont`, and `loadFromMessageTable = false`.
Do not globally replace a vanilla text ID shared by unrelated NPCs.

The shared `TradeDialogue` flow quotes a purchase, repair, or bank amount and
consumes an explicit choice once, with fresh balance, actor, file, and story
checks in the engine action. Cancellation changes no money. Dialogue must not award rupees,
items, recovery flags, or rapport merely because its closing state runs again.
On initial text opening, `Message_StartTextbox` assigns `msgCtx.talkActor` after
opening the text; identify the custom text using its own IDs or explicit context,
not that potentially stale pointer.

### Placement evidence and checks

Tracked Kakariko source provides these existing Cucco ground-position anchors:
`(-1697, 80, 870)`, `(57, 320, -673)`, `(796, 80, 1639)`,
`(1417, 465, 169)`, `(-60, 0, -46)`, `(-247, 80, 854)`, and
`(1079, 80, -47)`. They establish coordinate scale and ground regions; they are
**not approved new-NPC positions**. Do not stand new residents on original Cuccos,
the quest pen, grotto holes, or path crossings. The tracked scene asset headers
contain resource names rather than the original room actor coordinates.

For every candidate placement, find a valid floor with a collision raycast,
reject out-of-bounds or hazardous positions, allow body and interaction clearance,
and compare against original actor/transition entries. A floor hit alone does not
prove a location is safe: it can land on a roof, above a doorway, or inside a
quest route. Label unreviewed coordinates as candidates. Do not claim successful
placement or gameplay behavior without the owner's acceptance.

## Source references and next increments

- Current actors: [Kakariko](../soh/soh/Enhancements/living-hyrule/ResidentActor.cpp),
  [Hylian regional residents](../soh/soh/Enhancements/living-hyrule/WorldResidents.cpp),
  [Kokiri/Goron](../soh/soh/Enhancements/living-hyrule/ForestMountainResidents.cpp),
  and [Zora/Gerudo](../soh/soh/Enhancements/living-hyrule/WaterDesertResidents.cpp).
- [Shared transaction dialogue](../soh/soh/Enhancements/living-hyrule/TradeDialogue.cpp),
  [decorative supplies](../soh/soh/Enhancements/living-hyrule/PropertyScenery.cpp),
  and [property rules and contact list](LIVING-HYRULE-PROPERTIES.md).
- [Actor database API](../soh/soh/ActorDB.h) and
  [implementation](../soh/soh/ActorDB.cpp): additional actor identities.
- [Actor spawning and NPC talking](../soh/src/code/z_actor.c): lifecycle, room
  spawning hook, `Actor_Spawn`, `Npc_UpdateTalking`, and talk offers.
- [Object bank](../soh/src/code/z_scene.c): resource lookup and capacity limits.
- [Kakariko carpenter](../soh/src/overlays/actors/ovl_En_Daiku_Kakariko/z_en_daiku_kakariko.c):
  skeleton, clothing colors, compatible heads, and original lifecycle constraints.
- [Hylian civilians](../soh/src/overlays/actors/ovl_En_Hy/z_en_hy.c): compatible
  heads, skeletons, palettes, animations, and behavior that must remain separate.
- [Kokiri](../soh/src/overlays/actors/ovl_En_Ko/z_en_ko.c),
  [Gorons](../soh/src/overlays/actors/ovl_En_Go/z_en_go.c),
  [Zoras](../soh/src/overlays/actors/ovl_En_Zo/z_en_zo.c), and
  [Gerudo](../soh/src/overlays/actors/ovl_En_Ge1/z_en_ge1.c): regional visual families.
- [Custom dialogue example](../soh/soh/Enhancements/TimeSavers/MarketSneak.cpp)
  and [message lifecycle](../soh/src/code/z_message_PAL.c).
- [Kakariko Cucco anchors](../soh/src/overlays/actors/ovl_En_Niw/z_en_niw.c) and
  [scene-based extra actor example](../soh/soh/Enhancements/QoL/DaytimeGS.cpp).

Next work can add Mira as the missing cloth-workshop contact, a carefully bounded
indoor schedule, and limited walking or work behavior. Keep deployment
incremental and review the owner's final gameplay feedback before describing
placements or presentation as accepted. Physical reconstruction, relationships,
population growth, staffing simulation, broader persistent postgame, Zelda's
daily life, and castle ownership still require their own systems and save design.
