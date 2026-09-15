# Living Hyrule project guidance

## Product direction

Living Hyrule adds property, economy, people, relationships, reconstruction, and
postgame life around Ocarina of Time. Preserve the original adventure, required
items, songs, dungeon progression, and story flags. Read `README-LIVING-HYRULE.md`
and `docs/LIVING-HYRULE.md` for the current implementation. The full local creative
vision is at `C:\ZeldaDev\docs\LIVING-HYRULE-VISION.md`.

Consider every location in context. `docs/LIVING-HYRULE-POPULATION.md` covers all
110 scene IDs and distinguishes planned populations from implemented residents.
Use appropriate cultures, jobs, day/night activity, and child/adult/recovery
conditions. Sacred spaces, puzzle rooms, boss arenas, and story scenes should
retain their intended atmosphere and progression.

## Working preferences

The owner wants development to proceed with compiler and automated checks, and
will playtest finished playable milestones personally. Do not spend iterations
automating gameplay unless the owner asks. Continue implementation after suitable
automated checks; do not require the owner to test each intermediate change.
Always distinguish implemented code, successful builds, and actual gameplay
acceptance. Never claim the latter from a compile or synthetic test alone.

Keep user-facing updates plain and concise. Document substantial limitations and
the next development increment without turning them into repeated approval steps.

## Implementation

- Keep additions modular under `soh/soh/Enhancements/living-hyrule`.
- Use feature branches and focused commits. `living-hyrule` is the integration
  branch; `develop` tracks upstream. The resident feature branch includes the
  earlier economy feature. Check current Git state before changing branches.
- Custom residents should own their behavior and identity. Reuse compatible
  skeletons, heads, per-instance colors, proportions, and poses; do not copy a
  quest actor's state machine or mutate shared visual assets globally.
- Keep persistent state compatible with the engine's copied save snapshot.
  Validate save input, preserve unknown versions, and test meaningful money and
  persistence invariants when changing them.
- Use `tools/living-hyrule/Build.ps1` for the configured Windows build and staging
  workflow, and `tools/living-hyrule/Test.ps1` for native automated checks.
- Keep ROMs, extracted Nintendo assets, saves, personal settings, and build output
  outside Git. Do not bypass the repository guards or upload local game assets.
