# Living Hyrule test-build checkpoint

## Current request and frozen scope

The owner explicitly requested the graphics compatibility follow-up after asking
about 60 FPS and HD packs. That fix is now implemented, built and installed. The
game was not started. Automatic overnight development remains PAUSED; the owner
tests the installed milestone. No texture pack, FPS or resolution change was made.

Installed source: 6d91da1b4874179b6e8748b234531452e852334a from
feature/living-hyrule-graphics-compatibility; embedded revision 6d91da1.
All 25 native/local-resource suites, strict compiler checks, full configure/build,
Stage and installation verification passed. Installed UTC: 2026-09-15T18:11:15.9650120+00:00.
C:\ZeldaDev\docs\LATEST-BUILD.json is the installed-build authority.

## Changes in this follow-up

- Texture-only packs and the alternate-assets preference no longer suppress the
  three added encounters. Checks target each enemy's actual native rig, limbs,
  all its animations and the scene collision. Binary replacements and metadata
  aliases are checked too; unrelated Link models are allowed.
- Rig shape and cached limb pointers must remain compatible. Added enemies keep
  their native rig fixed for their lifetime, and compatibility is checked again
  on asset toggles. Existing native death/breakup finishes; toggles never reset
  the once-per-entry budget. Structural enemy/scene overhauls remain excluded.
- Regional dyes use the tunic-color channel with custom Link models. Fixed-color
  textures may ignore the tint. Explicit cosmetic colors and connected Anchor
  appearances still take priority. The wardrobe explains this in the game.
- Save schema stays five. All previous recovery, royal garden/estate, gifts,
  bank/cottage/business/resident/favor/Market/dye/charter features remain included.

## Verification

Test.ps1 passed 25/25 suites, including the five local native-archive suites.
New cases cover texture packs, unrelated Link models, missing/overridden
structural assets, metadata aliases, toggles without extra spawns, custom-model
dyes and cosmetic/network precedence. Native archive checks verify the entire
animation manifest and embedded rig sizes. RegionalEncounters, RegionalWardrobe
and LivingHyruleWindow passed strict MSVC /W3 /WX syntax checks; the final UI text
also compiled in the full successful build.

No third-party texture/model pack was installed or visually tested. Automated
checks and a successful build do not establish gameplay or visual acceptance.
The owner's latest saves/settings were fingerprinted immediately before this
installation; preservation checks use that fresh baseline, not an older build.

## Installed artifact and preservation

Backup: C:\ZeldaDev\backups\before-graphics-compatibility-20260915-175139UTC.
It contains both prior executables, port archives, settings, imgui preferences,
save copies, the prior receipt and preservation fingerprints. All 13 protected
files match afterward: four saves, both settings/ImGui files, both local game
archives, the ROM, and vanilla executable/port archive.

Both modded executables and port archives match the compiled artifacts. Nine
revision/feature strings were verified in the executable. No game was launched.

- Executable size: 106,370,048 bytes.
- Executable SHA256: eeab46b36609e1faae73071062dc2f86f1d4c2dabd9be965606af258e272fe47.
- Port archive SHA256: 3717e816c431e7af3689ca52d1e6078e0d8d67d844aeb32c3780f1e64e66499b.
- Main runtime: C:\ZeldaDev\runtime\development\soh.exe.
- Alternate runtime: C:\ZeldaDev\runtime\living-hyrule-playtest\soh.exe.
- Receipt: C:\ZeldaDev\docs\BUILD-GRAPHICS-COMPATIBILITY-6d91da1.json.
- Logs: C:\ZeldaDev\logs\graphics-compatibility-*.log.
- Owner guide: C:\ZeldaDev\docs\PLAY-LIVING-HYRULE.md.

The desktop Living Hyrule (Modded) shortcut opens the main runtime. Existing
preferences, including residents/challenge and cheats, are preserved. Never use
Build.ps1 Run or All without a new explicit request to launch the game.

## Remaining scope

Full castle/shop interiors, wider contextual population and properties, more
daily routines, equipment models and deeper combat remain unfinished. The
110-scene population plan separates implemented residents from future locations.
Do not resume broader development or upload local ROMs/assets/saves/binaries.
The repository ignore rules and source-only Git guards remain enabled.
