# Living Hyrule

An optional life and economy expansion for Ocarina of Time, based on
[HarbourMasters/Shipwright](https://github.com/HarbourMasters/Shipwright).
The original adventure, required equipment and story progression remain intact.

## Current implementation

The world-life increment adds actual game actors, conversations and transactions:

- Twenty original residents across ten named locations, using compatible Hylian,
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

See the [worklog](docs/LIVING-HYRULE-WORKLOG.md) for the latest build and installed
executable verification. Code implementation, successful compilation and gameplay
acceptance are separate milestones. The owner tests finished builds; development
does not launch the game automatically.

## Play the installed development build

On the configured workstation, use the **Living Hyrule (Modded)** desktop
shortcut when you are ready. The vanilla installation has its own separate directory.

1. Load a development save in a normal adventure or Master Quest.
2. Open **Enhancements > Living Hyrule > Open Living Hyrule** in the port menu.
3. Enable **Additional residents** and enable the economy for that save.
4. Deposit rupees through the ledger, or speak to Orlen in daytime Kakariko to
   deposit your wallet. Bram can refill your wallet from the bank.
5. Speak to a participating property manager. Choose **Yes** and press a fresh
   A to accept the quoted purchase or repair; **Not now** or B cancels.
6. Save normally to keep money, deeds, repairs and accumulated income progress.

Tavin sells the **1,200-rupee cottage**, then manages the builders' yard. The
cottage pays **25 rupees per ten minutes of active play**. Sixteen other deeds
have their own prices and income. Adult crises suspend trade and income; freeing
the region and paying for individual repairs restores business operations.
Ownership survives the age transition. There is no offline income.

The **Dangerous combat and scarce recovery** preference is independent of the
economy. Infinite Health and other damage overrides take priority, as explained
in the window. Placed hearts, heart pieces, containers and fairies keep their
normal rules. The owner's existing cheats are preserved.

## Development and remaining scope

Full building restoration, additional interiors, persistent relationships,
regional stewardship, postgame Zelda/castle life, optional gear and wider encounter
changes remain in development. The [110-scene population plan](docs/LIVING-HYRULE-POPULATION.md)
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
