# Living Hyrule

An optional life and economy expansion for Ocarina of Time, based on
[HarbourMasters/Shipwright](https://github.com/HarbourMasters/Shipwright).

**Status: banking, the first rental property, and three additional residents
are implemented on `feature/living-hyrule-residents`. All three native automated
test suites passed, and the full game compiled and linked successfully.** Gameplay
acceptance is left to the project owner; the new mod has not been runtime-playtested.
The original main quest, dungeons, story items, and progression remain the foundation.

The resident branch includes the earlier `feature/living-hyrule-economy` work.
The three original Kakariko residents reuse character models with individual
colors, heads, proportions, posture, and conversations. The population design
covers every scene in the game; the current code places residents only in Kakariko.

## Try the prototype

1. Load a disposable development save in a normal adventure or Master Quest.
2. Open the port menu, then **Enhancements > Living Hyrule > Open Living Hyrule**.
3. Enable the economy for that save file. Deposit or withdraw rupees through the ledger.
4. Save **1,200 rupees** in the bank, visit Kakariko Village, and buy the cottage.
5. Save your game normally to keep your bank balance, ownership, and rent progress.

Enable **Additional residents** in the same window to meet **Tavin** the
carpenter, **Bram** the boot-mender, and **Orlen** the supplier during the day in
Kakariko. During the adult crisis Bram remains; the other two return after the
Shadow Medallion. Their dialogue also recognizes cottage ownership. This setting
is independent of the bank's per-save enable switch. Placement and dialogue
still require gameplay acceptance.

The cottage pays **25 rupees per ten minutes of active play** into the bank.
These prices and rates are provisional tuning. There is no offline rent. Child
Link can buy and earn rent; Adult Link's property sales and rent remain suspended
until the Shadow Medallion is obtained. Existing ownership and savings are kept.
Pausing the economy also preserves balances, ownership, and partial rent progress.

This first cottage is a ledger entry representing ownership. Tavin discusses the
listing, while purchases still use the ledger; there is no cottage interior yet.
The increment covers banking, one property's ownership and rent, normal saving,
and the first three additional residents. The wider Living Hyrule vision remains
future work.

## Project information

- [Living Hyrule changelog](docs/LIVING-HYRULE-CHANGELOG.md)
- [Population and character design for all 110 scenes](docs/LIVING-HYRULE-POPULATION.md)
- [Prototype behavior, save architecture, repository policy, and next phases](docs/LIVING-HYRULE.md)
- [Creative vision (local workstation)](C:/ZeldaDev/docs/LIVING-HYRULE-VISION.md)
- [Windows build and staging helper](tools/living-hyrule/Build.ps1)
- [Repository safety checks](tools/living-hyrule/check_repository.py)
- [Official build instructions](docs/BUILDING.md)
- [Official code-modding guide](docs/MODDING.md)

Game ROMs, extracted Nintendo assets, runtime data, and build outputs are not
distributed by this project. Use your own supported local ROM and keep its data
outside the source checkout.

On the configured Windows workstation, the workspace is `C:\ZeldaDev`.
`living-hyrule` is the integration branch; create `feature/<topic>` branches for
bounded implementation and review each change before integration.
