# Living Hyrule overnight checkpoint

## User direction and continuation

The owner wants the full Living Hyrule vision implemented as real gameplay code.
**Never launch the game or automate gameplay.** Compile/test/install finished
increments; the owner will test final results. Preserve vanilla, saves and local
ROM-derived assets outside Git. Do not stop at plans or placeholder menus.

Heartbeat living-hyrule-overnight-development is ACTIVE every15minutes in this
task until **08:00 September15,2026 America/Chihuahua =14:00UTC**. Windows reports
UTC-07 rather than the named zone's UTC-06, so use UTC/the named zone for cutoff.
At cutoff finish a safe checkpoint, pause the heartbeat and report completed vs
remaining work honestly. Parallel agents are authorized; coordinate ownership.
Read C:\ZeldaDev\docs\LIVING-HYRULE-VISION.md and the110-scene population plan.

## Last installed build: world life

**Full configure, compile, link and Stage completed successfully; ten native
suites passed. No game launch occurred.**

- Code commit: 002be29ff631cc6cab20ef7858df643ccff8ce9c.
- Feature branch: feature/living-hyrule-world-life; embedded revision002be29.
- Installed UTC:2026-09-15T06:51:19Z.
- Primary executable:C:\ZeldaDev\runtime\development\soh.exe.
- Alternate:C:\ZeldaDev\runtime\living-hyrule-playtest\soh.exe.
- SHA256 for compiled and both installed copies:
  aa2528734af57f640c7d72f5f3c8b8bb81ad82cb62ba8f23496e2500d4f96af9.
- Size104,611,328 bytes; seven feature-name markers checked in the actual binary.
- Desktop launcher:C:\Users\Guest\Desktop\Living Hyrule (Modded).lnk.
- Receipt:C:\ZeldaDev\docs\LATEST-BUILD.json, also BUILD-WORLD-LIFE-002be29.json.
- Logs:C:\ZeldaDev\logs\overnight-world-life-configure-final.log,
  overnight-world-life-build-final.log, overnight-world-life-stage.log,
  overnight-world-life-tests.log.

The first build exposed C++ graphics segment pointer conversions; these were
fixed and the subsequent complete build passed. Ten suites cover economy, codec,
old/new/cultural population, properties, trade, supplies and challenge policies.
Read-only reviews checked native skeleton counts, materials, actor lifecycle,
conversation timing, and local collision footprints/stock actor clearance.
These checks do not establish live rendering, dialogue or gameplay acceptance.

## Actual installed behavior

- Persistent bank,1200-rupee cottage and16regional deeds. Regional crises suspend
  income; original recovery plus paid adult repairs resumes operation.
- Twenty original residents across ten named locations. Native Hylian, Kokiri,
  Goron, Zora and Gerudo silhouettes; original conversations; contextual schedules.
- Fifteen deeds plus cottage transact through participating managers. Orlen
  deposits wallet rupees; Bram withdraws. Tavin offers cottage then buildersyard.
  Cloth workshop remains ledger-only. Shared frozen offers require a fresh Yes/A,
  correct live actor/save/text ownership, valid funds and world state.
- Seven Hylian managers may have one repair-supply crate or three operating
  crates beside their owned business. Safe ground and density checks apply.
  These are decorative supplies, not reconstructed buildings or new interiors.
- Free adult ruined-Market Redead cleanup and returning relief residents after
  a saved final-boss defeat timestamp. Generic gameComplete is unsuitable: custom
  time-splits set it and loading resets it. Genuine ACTOR_BOSS_GANON2 defeat in
  SCENE_GANON_BOSS queues one statistics-only save after boss hooks finish.
  Save identity/validity is rechecked and queues clear on scene/play/load reset.
  Normal unsaved wallet, ledger and base progress keep their ordinary semantics.
- Optional double ordinary enemy/boss collision damage and alternating eligible
  temporary loose-heart filtering. Existing damage cheats/modifiers take priority,
  emergency hearts and permanent pickups keep their normal rules.

The source keeps normal/MQ scope and original quest behavior. Zora residents use
dry river banks; Water completion does not thaw the Domain. Gerudo residents
respect rescue/membership access and separate Spirit business recovery.

## Preservation and local preferences

Prior working runtime/config/archive/receipt are backed up at
C:\ZeldaDev\backups\before-world-life-20260915-0640UTC. The name is a checkpoint
label; the actual installation time is in the receipt. Alternate runtime files
are also backed up there before replacement. Prior executable hash:
e4cf088245f7b3c8aa72f357f1fe8ca9dedfd8f5b8f913079b6dd822603c3257.

Exact JSON comparison confirmed Stage preserved every personal setting. Only
CVars.gEnhancements.LivingHyruleChallenge was then set to1 in the development
profile. Residents, ledger window and valid file1 economy were already enabled
by the previous checkpoint. Existing InfiniteHealth/InfiniteMoney etc remain:
the UI explains that InfiniteHealth suspends extra damage. No current save file
was edited in this world-life installation. Vanilla remains untouched.

## Active next work and architecture findings

A separate source worktree now exists:
C:\ZeldaDev\worktrees\royal-audience, branch feature/living-hyrule-royal-audience,
based on002be29ff. game_ui_audit owns ONLY new RoyalAudience.h/.cpp,
RoyalAudiencePolicy.h and RoyalAudienceTests.cpp there. It is implementing Zelda,
CaptainAren and stewardMaelin as independent postgame audience actors on the
verified adult ruined-castle approach. Do not mistake this pending branch for an
installed feature; root must review/integrate/test/build it.

Detailed investigated next steps, resources and hazards are saved in:
**C:\ZeldaDev\docs\NEXT-IMPLEMENTATION.md**. Read before restarting research.

1. Persistent rapport, ten finite delivery favors covering all20current residents,
   cottage fair/high rent with real income/relationship consequences, and trusted
   repair pricing. Design canonical resident IDs including future royal residents.
   Schema3 must preserve all old balances/deeds/timers and unknown-data behavior.
2. Royal audience plus later Zelda rapport, staff and eventual castle ownership.
   Native Zelda has a seven-matrix hair segment; never run original quest AI.
3. Real paid Market district restoration on next entry. Native child geometry is
   a panoramic presentation. Raw collision swaps are UNSAFE: exit-table mismatch,
   camera data, blocked shops/alleys, resource lifetime and3DSceneRender preference
   need deliberate handling. The external note gives exact hooks/resources.
4. Wider property/expenses/stewardship/treasury systems and optional regional
   equipment/encounters remain unfinished. Do not claim whole-game completion.

## Build discipline

Use tools/living-hyrule/Build.ps1 Configure/Build/Stage and Test.ps1. Never Run.
Commit code before Configure when practical so embedded revision identifies it.
Do not alter shared save ABI during a build or stage over a running game.
Do not start overlapping builds. Native tests use external build directories.

Windows post-link metadata copies now use safe Robocopy without mirror/deletion
flags, reducing that step from minutes to seconds. Stage still scans/copies
more than20,000 support files and may take a few minutes. Check child process
state before declaring a stall. Retain the last successful runtime on failures.

Source-only Git guards remain required; never bypass hooks or upload ROMs,
archives, extracted assets, personal configuration, saves or build outputs.
