# Living Hyrule population plan

## Scope and implementation status

This is a design for all **110 scene IDs**, `0x00` through `0x6D`, in the
[current scene table](../soh/include/tables/scene_table.h). It covers places that
should gain residents and places that should deliberately remain empty. Room,
entrance, and age variants within a scene still need individual placement work.

**The first implementation scope is three additional Kakariko residents only.**
The wider cast, regional schedules, new shops, recovery populations, and postgame
placements below are **Planned**, not implemented. This document does not claim
that the new residents have been playtested in the running game. Gameplay
acceptance belongs to the project owner.

The goal from the [local creative vision](C:/ZeldaDev/docs/LIVING-HYRULE-VISION.md)
is to make Hyrule feel inhabited while preserving its original adventure. Existing
quest actors, story items, songs, dungeons, rewards, and required routes keep their
roles. A new resident has a new identity and dialogue; reusing a model does not
make that resident a duplicate of its original character.

## First Kakariko cast

The **Additional residents** checkbox is independent of the per-file economy
enable control and defaults off. The initial three appear outdoors in Kakariko
in daylight, with the following story conditions:

| Resident | Work and character | Initial appearance | Child Link | Adult crisis | After Shadow Medallion |
| --- | --- | --- | --- | --- | --- |
| Tavin | Property carpenter; proud of sound roofs, plainspoken about the cottage and repairs | Carpenter model, distinct head, warm ochre clothing tint and a slightly taller stance | Present | Away | Returns |
| Bram | Boot-mender and cottage tenant; measures a recovery by how many people need walking shoes | Carpenter model, another head, slate-blue clothing tint and a shorter, slower idle | Present | Remains and discusses the disruption | Remains and discusses recovery |
| Orlen | Traveling supplier; remembers roads, prices, and which households need a delivery | Carpenter model, third head, moss-green clothing tint and a livelier idle | Present | Off the unsafe route | Returns |

All three are absent outdoors at night in this first increment; no indoor
relocation is implied. They reuse the existing carpenter skeleton, head variants,
and clothing color support through new actor instances with their own behavior.
The palette descriptions are the art direction, not a promise of new textures or
equipment. Their dialogue can react to age, recovery, and cottage ownership.
They do not inherit the four original carpenters' rescue flags or schedules.

Tavin's presence introduces a person behind the property system. Bram gives the
cottage a social context. Orlen makes a returning trade route visible. The bank
and cottage ledger remain the current economy interface until an NPC transaction
flow is implemented and checked. No new building interior is implied by these
residents.

## Population rules for later regions

### Story and recovery

**C** below means Child Link, **A** means the adult region during its crisis,
**R** means its story problem has been resolved, and **P** means a future
persistent postgame. These are design stages, not a new set of completed features.

- **Childhood:** settlements are busy at sensible hours. Preserve local problems,
  such as Goron hunger before Dodongo's Cavern is resolved; prosperity is not a
  reason to erase the original story.
- **Forest:** adults find fewer outdoor Kokiri and more danger. Forest Temple
  completion permits a return to paths and work areas, without relocating Saria
  or changing her role.
- **Death Mountain:** Goron capture and the Fire Temple crisis empty work sites.
  Story recovery allows workers to return; paid restoration and new businesses
  are a later phase.
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
  market during the main quest. A future persistent post-Ganon state must first
  make the area safe; residents return before paid rebuilding makes it prosperous.
- **Postgame:** no scene or resident may infer a completed persistent postgame
  merely from entering an ending map. That state and its save behavior are future
  architecture work.

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

## Planned regional cast

Except for the first three Kakariko residents above, every person in this section
is a proposed character, not an implemented NPC. Names are working names.

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

Every entry below is **Planned** unless explicitly identified as the first
Kakariko scope. **Excluded** means no additional population is proposed in that
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
| `0x20` | `SCENE_MARKET_DAY` | Planned: Vessa, customers, a porter, and a visiting regional trader. | C day; distribute small groups without blocking the original market cast. |
| `0x21` | `SCENE_MARKET_NIGHT` | Planned: sparse closing staff and a night steward. | C night; preserve the dog search, doors, and night atmosphere. |
| `0x22` | `SCENE_MARKET_RUINS` | No added civilians in A. Planned P relief camp followed by paid reconstruction workers. | Safety before occupancy; rebuilding stages must match visible geometry. |
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
| `0x2A` | `SCENE_KAKARIKO_CENTER_GUEST_HOUSE` | Planned: Sella and a rotating traveler; later an indoor schedule for the first cast if space allows. | Night lodging and A refuge; preserve Talon and the original carpenters. Not in the first three-resident increment. |
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
| `0x51` | `SCENE_HYRULE_FIELD` | Planned: Caro, Hollis, and occasional pairs on selected roads or rest points. | C day travel; A fewer travelers near safe edges; R regional traffic; P linked routes. No night crowds among Stalchildren or across Epona routes. |
| `0x52` | `SCENE_KAKARIKO_VILLAGE` | **First scope:** Tavin, Bram, and Orlen; additional regional cast remains Planned. | Day only: C all three; A Bram only; Shadow R all three. Night zero for this increment; future visits and lodgings need placement. |
| `0x53` | `SCENE_GRAVEYARD` | Planned: Orris tending paths and one respectful visitor. | Day work; sparse dusk mourning; no blocking tombs, race access, Dampe, or the Shadow Temple route. |
| `0x54` | `SCENE_ZORAS_RIVER` | Planned: Sori at a safe bank, Neris on a Zora route, one daytime courier. | C modest travel; A reduced trade; R activity only on usable banks. Frogs, waterfall access, bean spots, and jump routes stay clear. |
| `0x55` | `SCENE_KOKIRI_FOREST` | Planned: Fenn, Luma, Nell, and Bori in small task groups. | C lively; A shelter and watchfulness; Forest R outdoor work. Do not block Mido, the Deku Tree route, shop, or practice areas. |
| `0x56` | `SCENE_SACRED_FOREST_MEADOW` | Planned: at most Tavi near the safe approach after Forest R. | Sparse by design; no villagers in the maze during danger or beside Saria's story position. |
| `0x57` | `SCENE_LAKE_HYLIA` | Planned: Vero at the shore, Sori fishing, Edda taking samples, occasional trader. | C water-based work; A damaged livelihoods; Water R gradual return. Use correct water-level geometry and keep owl/warp/scarecrow routes open. |
| `0x58` | `SCENE_ZORAS_DOMAIN` | Planned: Lethra, Neris, and small Zora households on appropriate ledges and water routes. | C inhabited; frozen A sparse/absent as geometry demands. R return requires actual safe/thawed areas, not the Water Medallion alone. |
| `0x59` | `SCENE_ZORAS_FOUNTAIN` | Planned: one Zora spring keeper at a safe outer ledge. | C quiet stewardship; A no invented open-water work through ice; later restoration stage must match geometry. Jabu-Jabu and access routes remain clear. |
| `0x5A` | `SCENE_GERUDO_VALLEY` | Planned: Rasha on the trade approach, Orlen by a future supply stop, one local lookout. | C limited crossing context; A broken bridge and rescue context; R repairs/trade once access is real. Keep bridge and canyon hazards clear. |
| `0x5B` | `SCENE_LOST_WOODS` | Planned: Tavi at a known safe junction and rare Kokiri gathering visits. | C sparse; A more caution; Forest R limited return. Never mark every exit with helpful crowds or intrude on Skull Kid and trade encounters. |
| `0x5C` | `SCENE_DESERT_COLOSSUS` | Planned: Suri near a safe outer shrine and a rare Gerudo expedition. | Desert travel stays exceptional; Spirit R cautious visits. No day/night village crowd or obstruction of warp, oasis, bean, and temple routes. |
| `0x5D` | `SCENE_GERUDOS_FORTRESS` | Planned: Demi, Kesra, Mava, and off-duty workers in authorized common areas. | Respect child/adult access and membership; no friendly actor creates safe passage through the rescue challenge. Night watch replaces daytime trade. |
| `0x5E` | `SCENE_HAUNTED_WASTELAND` | Planned: one Tareh-led caravan rest point only if a safe route is deliberately defined. | Default sparse or zero; no crowd breadcrumb trail, free guide, or override of the original navigation challenge. |
| `0x5F` | `SCENE_HYRULE_CASTLE` | Planned: Alda or Hadrin in publicly reachable outer work areas. | C day labor outside the stealth route; night sparse. Future P castle workforce needs explicit restored access and geometry. |
| `0x60` | `SCENE_DEATH_MOUNTAIN_TRAIL` | Planned: Doron and a hauling partner at safe work bays, Iven on the lower road. | C shortage-aware; A reduced labor; Fire R return. No workers in falling-rock zones, narrow climbing lanes, or Biggoron's trade space. |
| `0x61` | `SCENE_DEATH_MOUNTAIN_CRATER` | Planned: at most a Goron specialist near a proven safe ledge after Fire R. | Hazardous region stays sparse; no ordinary Hylian work crew or actors on heat/warp routes. |
| `0x62` | `SCENE_GORON_CITY` | Planned: Brakka, Muro, Gorrin, and Rukka with distinct resting/work routines. | C hunger shapes dialogue; A captivity empties worksites; Fire R homecoming. Preserve rolling routes, Darunia, Goron Link, doors, and urn access. |
| `0x63` | `SCENE_LON_LON_RANCH` | Planned: Nessa, Jory, and Wren doing farm work. | C working ranch; A reflects Ingo's control; later labor changes follow the relevant ranch story. Keep races, horses, fences, and Epona access clear. |
| `0x64` | `SCENE_OUTSIDE_GANONS_CASTLE` | No added civilian population during the main quest. | Planned P rebuilding belongs to an explicit safe/restored scene state; do not put workers above the abyss or in the bridge sequence. |

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

A future resident definition should contain a stable resident ID, home region,
role, visual profile, scene/room/entrance conditions, transform, permitted time
windows, child/adult/recovery conditions, and dialogue keys. Scene transforms and
runtime actor IDs are not resident identities and should not be saved as such.

Use `OnSceneSpawnActors` after the original room actor list is spawned. It can run
again on room loading, so deduplicate by resident ID and room lifetime. Clear
runtime references on actor destruction and play destruction. Exclude title,
file-select, debug saves, unsupported adventure modes, and cutscene scene layers.
Do not spawn during an active transition, blocking story sequence, or before the
room's collision and resource dependencies are available.

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

Initial dialogue can describe the world without writing quest flags. Any later
property purchase must call the existing validated economy action once, after a
clear choice, with fresh balance/story checks. Dialogue must not award rupees,
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

After the three-resident increment, add Mira and one carefully bounded indoor
schedule, then a ranch/road connection, then one region-specific actor family at
a time. Keep worldwide deployment incremental. Paid reconstruction, relationships,
population growth, business staffing, persistent postgame, Zelda's daily life,
and castle ownership require their own implemented systems and save design.
