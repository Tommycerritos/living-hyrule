# Living Hyrule test-build checkpoint

## Current request and frozen scope

The owner asked to stop further development, finish a stable checkpoint, install
its mods into the game and let them test. Do not launch the game. The overnight
heartbeat living-hyrule-overnight-development is PAUSED. Do not resume automatic
development while the owner tests unless they ask.

Installed source: e575c936c2240635421293d6923b3adcac2d6a9d from
feature/living-hyrule-kingdom-recovery; embedded revision e575c93.
All 25 native/local-resource suites, full configure/compile/link, Stage and
installation verification passed. Installed UTC: 2026-09-15T14:18:56Z.
C:\ZeldaDev\docs\LATEST-BUILD.json is the installed-build authority.

## Implemented in the installed build

- Funded adult Zora Domain water restoration, preserving King Zora, red ice,
  Skulltula, required progression and a closed underwater Lake shortcut.
  Lethra and Neris also occupy verified restored-Domain walkways by day.
- Safe postgame royal garden visits through Captain Aren, a persistent return
  route and the household's garden placements. Maelin sells the estate deed
  after all eight charters; Zelda's trust can discount it. Full castle rooms
  remain unfinished.
- Three finite gifts per resident, saved gift memories, context-sensitive royal
  conversations and eight once-only acknowledgments of completed deeds.
- Three bounded optional native outdoor encounters on the Trail, River and
  Colossus; original enemy room-clear bookkeeping remains unchanged. The
  Colossus addition requires a rare original-spawner rest window. No Field or
  dungeon additions.
- Save schema five validates new state and migrates supported schemas one
  through four. Unknown/future/malformed state remains preserved read-only.
- All prior bank/cottage/business/resident/favor/Market/dye/charter systems.

## Verification and integration

Final Test.ps1: 25/25 passed, including five read-only local native-archive suites.
Nine integrated engine translation units passed strict /W3 /WX syntax; the
encounter module and its ChallengeMode integration passed separate strict checks.
Peer reviews fixed pre-player Domain funding detection, delayed travel safety,
remembered-garden fallback handling and a parentless Leever repeat-drop defect.
Royal text buffer bounds were checked. These are source/compiler/resource
checks; gameplay and visual acceptance still belong to the owner.

The new commits contain only source/tests. Source-only history audit found no
ROM, media, game archive, binary, personal settings or save additions. Hooks and
ignore rules remain enabled; upstream push stays disabled.

## Installed artifact and preservation

Backup: C:\ZeldaDev\backups\before-kingdom-recovery-20260915-134929UTC.
It contains both previous executables/port archives/settings, the previous receipt
and all four existing save fingerprints. The prior 46c17fa executable SHA256 is
70838903feb8826cd8195c03970a262d2bfaea9a66f930710d1c655316b7fcd0.

Build.ps1 -Action Stage and the external Verify-KingdomInstall.ps1 completed.
All three executable copies and both installed port archives match the build.
Eight feature/revision markers were found in the actual executable. Both
settings files and all four saves were verified unchanged. Never use Build.ps1
Run or All unless the owner explicitly asks to start the game.

- Executable size: 106,360,320 bytes.
- Executable SHA256:
  6d313f623efe8272c771a8dc1892a1b96ddf4d5c6e3a1d9324cf93376ab6f027.
- Port archive SHA256:
  b2618289f65599259c2451c22f5495e46b769529b91299c206c284ee8e843eb6.
- Receipts: C:\ZeldaDev\docs\LATEST-BUILD.json and
  C:\ZeldaDev\docs\BUILD-KINGDOM-RECOVERY-e575c93.json.
- Alternate executable: C:\ZeldaDev\runtime\living-hyrule-playtest\soh.exe.

Desktop shortcut Living Hyrule (Modded) points to
C:\ZeldaDev\runtime\development\soh.exe with the matching working directory.
Additional residents and the optional challenge are enabled in that profile.
User settings, including Infinite Health, are preserved. Vanilla and ROM remain
separate and untouched. No game process has been started.

Logs: C:\ZeldaDev\logs\overnight-kingdom-recovery-{tests,configure,build,stage}-final.log.
Owner guide: C:\ZeldaDev\docs\PLAY-LIVING-HYRULE.md.

## Remaining scope

This is a testable development milestone, not the whole finished vision. Full
castle/shop interiors, wider contextual population and properties, more daily
routines, equipment models and deeper combat remain. The 110-scene population
plan distinguishes the implemented cast from future locations.

An account-usage interruption stopped work during part of the overnight period.
Work resumed after the owner's continue message, integrated the current stage,
and froze scope when they asked to test. Do not report uninterrupted overnight
implementation or gameplay verification.
