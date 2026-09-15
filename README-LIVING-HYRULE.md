# Living Hyrule

An optional life and economy expansion for Ocarina of Time, based on
[HarbourMasters/Shipwright](https://github.com/HarbourMasters/Shipwright).
The original adventure, required equipment and story progression remain intact.

## Current implementation

The current source adds people, property, relationships and local recovery around
the original adventure:

- Twenty-three residents across eleven named locations, using compatible Hylian,
  Kokiri, Goron, Zora and Gerudo models with individual appearances and dialogue.
  Their presence follows time of day, local access and the original story.
- A persistent bank, rental cottage and sixteen regional business deeds. Buy and
  repair property through participating managers or the ledger; real rupees leave
  your bank, and operating businesses pay active-play income.
- Free removal of ruined-market Redeads after the recorded Ganon victory,
  returning relief workers, and paid reopening of owned adult-era businesses.
- Supply crates beside seven participating business managers: one while repairs
  await, three when operating, subject to available safe ground. These are supply
  markers; building reconstruction and new interiors remain unfinished.
- Optional double damage from ordinary enemy/boss collision hits and fewer
  temporary loose hearts, with compatibility checks for existing damage cheats.
- Ten finite delivery favors, persistent relationships, trusted repair discounts,
  and cottage rent choices with real consequences for Bram and the bank.
- A postgame audience with Zelda, Captain Aren and steward Maelin on the ruined
  castle approach. Their dialogue recognizes the player's recovery work.
- A funded Market exterior restoration: pay 25,000 bank rupees after Ganon's
  defeat, then leave and return to see restored streets and facades. Shop
  interiors, alleys and the castle remain closed or unfinished.
- Short walking routines for Pella in the Market and Edda at Lake Hylia. They
  wait when a route is blocked or a conversation needs their attention.
- Eight purchasable regional cloth dyes for Link's native clothing in both ages.
  Original equipment protection, custom models and cosmetic settings are preserved.
- Eight regional charters, local treasury transfers, business dues and resident
  recognition, culminating in the title **High Steward of Hyrule**.

**Verification:** all **17 native and local-resource test suites passed** for the
combined schema-four source. The complete game configured, compiled, linked and
installed successfully as **46c17fa** at 08:09 UTC on September 15. Both modded
runtime executables match the compiled file; settings and saves are unchanged.
No game was started during this implementation pass.

See the [worklog](docs/LIVING-HYRULE-WORKLOG.md) for the latest build and installed
executable verification. Code implementation, successful compilation and gameplay
acceptance are separate milestones. The owner tests finished builds; development
does not launch the game automatically.

## Using the features after the next staged build

On the configured workstation, use the **Living Hyrule (Modded)** desktop
shortcut when you are ready. The vanilla installation has its own separate directory.

1. Load a development save in a normal adventure or Master Quest.
2. Open **Enhancements > Living Hyrule > Open Living Hyrule** in the port menu.
3. Enable **Additional residents** and enable the economy for that save.
4. Deposit rupees through the ledger, or speak to Orlen in daytime Kakariko to
   deposit your wallet. Bram can refill your wallet from the bank.
5. Speak to a participating property manager. Choose **Yes** and press a fresh
   A to accept the quoted purchase or repair; **Not now** or B cancels. When a
   resident has several topics, **Something else** cycles through them.
6. Accept a favor from its sender and hand it over to its named recipient. The
   **People and favors** journal keeps the instructions and remembers residents.
7. Use **Regional clothing dyes** to buy a local color and select an owned dye.
   After recovering a region and owning and repairing all its businesses, visit
   it to purchase a charter under **Regional stewardship**.
8. Save normally to keep money, deeds, repairs, relationships, current favors,
   restoration funding, dyes, charters and treasuries.

Tavin sells the **1,200-rupee cottage**, then manages the builders' yard. The
cottage pays **25 rupees per ten minutes of active play** on fair terms. High
rent pays 40 but damages Bram's trust; if he falls behind, collection drops to 15.
Rent changes start after the current period. Sixteen other deeds
have their own prices and income. Adult crises suspend trade and income; freeing
the region and paying for individual repairs restores business operations.
Ownership survives the age transition. There is no offline income.

The **Dangerous combat and scarce recovery** preference is independent of the
economy. Infinite Health and other damage overrides take priority, as explained
in the window. Placed hearts, heart pieces, containers and fairies keep their
normal rules. The owner's existing cheats are preserved.

## Development and remaining scope

Full building restoration, additional interiors, castle ownership/restoration,
gifts and deeper relationships, wider daily routines, new equipment models and
deeper combat changes remain in development. The [110-scene population plan](docs/LIVING-HYRULE-POPULATION.md)
distinguishes current deployments, planned people and intentional exclusions.

- [Feature behavior and save architecture](docs/LIVING-HYRULE.md)
- [Properties and their managers](docs/LIVING-HYRULE-PROPERTIES.md)
- [Changelog](docs/LIVING-HYRULE-CHANGELOG.md)
- [Current build/worklog](docs/LIVING-HYRULE-WORKLOG.md)
- [Full creative vision on this workstation](C:/ZeldaDev/docs/LIVING-HYRULE-VISION.md)
- [Windows build/staging helper](tools/living-hyrule/Build.ps1)
- [Native tests](tools/living-hyrule/Test.ps1)
- [Repository guards](tools/living-hyrule/check_repository.py)

The living-hyrule branch integrates bounded feature branches; develop tracks the
official upstream. ROMs, extracted Nintendo assets, saves, personal settings,
backups and builds stay outside Git in C:\ZeldaDev. This repository distributes
source only, never the user's local game data.
