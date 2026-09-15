# Living Hyrule

Development foundation for an Ocarina of Time expansion based on
[HarbourMasters/Shipwright](https://github.com/HarbourMasters/Shipwright).

**Status: environment setup; no mod features implemented.**

- [Project scope, repository policy, and next-phase handoff](docs/LIVING-HYRULE.md)
- [Windows build and staging helper](tools/living-hyrule/Build.ps1)
- [Repository safety checks](tools/living-hyrule/check_repository.py)
- [Official build instructions](docs/BUILDING.md)
- [Official code-modding guide](docs/MODDING.md)

Game ROMs, extracted Nintendo assets, runtime data, and build outputs are not
distributed by this project. Use your own supported local ROM and keep its data
outside the source checkout.

On the configured Windows workstation, the workspace is `C:\ZeldaDev`.
`living-hyrule` is the integration branch; create `feature/<topic>` branches for
future implementation after the architecture review.
