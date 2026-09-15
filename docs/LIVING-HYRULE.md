# Living Hyrule development

## Current scope

Environment setup only. No game feature code has been changed. The original quest,
dungeon progression, and required items must remain intact. Living Hyrule adds an
optional economy, property ownership, social systems, regional recovery, and a
post-Ganon world around that foundation.

## Repository and branches

- `upstream`: https://github.com/HarbourMasters/Shipwright.git (fetch only).
- `origin`: https://github.com/Tommycerritos/living-hyrule.git (personal fork).
- `develop`: untouched official baseline, tracking `upstream/develop`.
- `living-hyrule`: integration branch for reviewed project changes.
- `feature/<topic>`: one bounded feature per branch and pull request into `living-hyrule`.
- `fix/<topic>` and `docs/<topic>`: focused fixes and documentation.
- `baseline/shipwright-2026-09-14`: pinned original source commit for comparison.

Fetch upstream explicitly. Review upstream changes, update submodules to their
recorded revisions, and validate a clean build before merging engine updates.
Do not automatically rebase published integration history. Release tags should
identify validated builds; do not commit binaries or package game assets.

## Local data policy

`C:\ZeldaDev\roms`, `assets`, `runtime`, `build`, and `backups` are outside Git.
The preserved vanilla copy is a reference; launch a test copy when checking it.
Development uses its own configuration and saves. Never share ROM-derived O2R/OTR
archives, extracted game data, or runtime backups.

Upstream already tracks source asset headers and some project-owned graphics.
Keep those intact. New graphics/audio/models are blocked by the repository guard
until their original authorship or license is reviewed and a narrow policy change
is approved as part of normal development. File extensions cannot prove ownership.

`.gitignore` prevents ordinary accidental additions. `.githooks/pre-commit` and
`pre-push` additionally check staged files and outgoing history, including common
ROM signatures and renamed archives. Git hooks can be bypassed; they are not an
absolute security boundary. Always review `git diff --cached` before committing.
On a new clone enable them with `git config core.hooksPath .githooks` and set
`git config livinghyrule.python <absolute-path-to-python.exe>`.

GitHub Actions are disabled on the fork until a source-only CI design is reviewed.
Do not copy the upstream asset-upload workflow or register this PC as a public
self-hosted runner. Later CI may compile source and run synthetic-data tests;
ROM-dependent playtests remain local.

## Handoff: architecture first

The full reference design is preserved locally at
`C:\ZeldaDev\docs\LIVING-HYRULE-REFERENCE.md`.

Before implementing features, investigate the pinned source's mod hooks, custom
save serialization and versioning, actor spawning, dialogue, UI, wallet arithmetic,
and post-Ganon state. Produce an architecture proposal and phased plan.

Starting points in the pinned source (verify suitability during the audit):
`soh/soh/SaveManager.h`, `soh/soh/SaveManager.cpp`,
`soh/soh/Enhancements/game-interactor/GameInteractor.h`, and
`soh/soh/SohGui/SohMenu.h`. See upstream `docs/MODDING.md` for its code-mod workflow.

First proposed vertical slice: **one Kakariko property, one seller, bank account,
purchase, persistent ownership, periodic rent, and save/reload**. Define rollback,
save migration, time advancement, and balance rules before coding. Use a disposable
development save. Test that vanilla quest progression and existing saves survive.

Later phases: generalized properties, businesses, rapport, population, regional
reconstruction, persistent postgame, regional stewardship, Zelda/castle expansion,
and optional equipment. Asset reuse requires permission and attribution; Nintendo
assets stay on each player's own computer.
