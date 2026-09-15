# Living Hyrule overnight checkpoint

## User direction and continuation

The owner asked for the full Living Hyrule vision implemented overnight, and
explicitly requires real gameplay code compiled and installed into the game.
**Do not launch the game.** Compile and run automated tests; the owner will
perform final gameplay testing. Do not claim runtime acceptance from a build.

Overnight continuation is scheduled every fifteen minutes in this task, automation ID
`living-hyrule-overnight-development`, until 08:00 September 15, 2026 in
America/Chihuahua. At the cutoff finish a safe checkpoint and pause it. No new
subagents are authorized. Read the full vision at
`C:\ZeldaDev\docs\LIVING-HYRULE-VISION.md` and the 110-scene population plan.

## Current work, September 14, regional-property increment

Code commit: `f71940314285356c2faa781126d17a175d30555d`, feature branch
`feature/living-hyrule-regional-properties`. **All four automated test suites
passed; full configuration, compilation, linking and staging completed with
exit code zero.** The new development executable is installed at
`C:\ZeldaDev\runtime\development\soh.exe` and its SHA256 matches the compiled
output: `e4cf088245f7b3c8aa72f357f1fe8ca9dedfd8f5b8f913079b6dd822603c3257`.
The code is backed up on the source-only GitHub feature branch. See the local
`C:\ZeldaDev\docs\LATEST-BUILD.json` receipt. The embedded revision is 5598595
because configuration preceded the source commit; the receipt identifies the
actual compiled code. The game has not been started.

Implemented in source in this increment:

- Sixteen persistent property/business deeds across eight regions. Purchases
  require visiting the region and deduct actual bank rupees. Existing actors,
  shops and quest ownership flags are not replaced.
- Each business has its own active-play income timer. Adult crises suspend
  operations without losing ownership or progress. Each region uses its actual
  medallion, Epona, membership or game-completion progression requirement.
- Adult premises need paid repairs after the region is freed; repair payment
  changes persistent operating state and resumes income. This increment does
  not yet alter building geometry or show construction workers.
- Save schema two preserves all old balances, cottage ownership and rent while
  migrating schema one. Malformed/future data remains read-only and preserved.
- The completed-adventure flag removes Redeads from the adult ruined market
  while Living Hyrule is enabled. It does not mark the game complete itself.
- The ledger exposes all deeds, region availability, prices, repair costs,
  ownership, income state and timers. These are actual economy actions, not
  placeholder buttons; interior entry and owner-NPC transactions remain absent.

Development profile fixes: residents enabled, Living Hyrule window enabled,
existing valid file1 economy enabled. Other save fields and configuration were
preserved, backed up under `C:\ZeldaDev\backups\before-overnight-20260914-221608`.
The game was closed before changes and has not been started.

## Verification and preservation

The four suites cover banking, strict save migration, population policy, and
regional property/repair/income behavior. Full-game integration compiled and
linked successfully. They do not prove runtime rendering or gameplay acceptance.
The local collision resource was inspected without executing the game: all
three existing Kakariko candidates have nearby walkable ground at the expected
heights. Full body clearance and actor placement remain runtime checks.

The previous successful executable is backed up alongside the original config
and save in `C:\ZeldaDev\backups\before-overnight-20260914-221608`.
Exact JSON comparison confirms only the two intended configuration leaves and
file1's economy enable flag changed. No game progress or money was altered.
The current debug profile has existing money/health cheats; those were preserved.

Build timing: changing the shared save structure triggers a full engine rebuild.
The upstream packaging steps also scan/copy more than 20,000 metadata files and
can spend several minutes without new log output. Check child process state
before assuming a stalled build. Do not start a second concurrent build.

## Next concrete implementation priorities

Continue implementing actual game systems, not only planning documents. Keep
each stage independently buildable and maintain the last successful runtime.

1. Connect properties to in-world owner/manager conversations. Add contextual
   residents beyond Kakariko using compatible actor visuals and safe placement,
   respecting cultural and story conditions in the population plan.
2. Implement staged visible reconstruction: laborers, repair props, reopening
   activity and paid regional projects. Castle Town should become safe for free
   after Ganon, remain ruined initially, and rebuild through player investment.
   Water Temple completion alone must not claim to thaw Zora's Domain geometry.
3. Add persistent rapport, small favors, gifts and fair/greedy rental choices
   with concrete dialogue/access consequences.
4. Expand property coverage, regional stewardship/treasury and coherent economic
   tuning; preserve existing quest access and NPC roles.
5. Implement safe persistent postgame life, Zelda conversations and eventual
   castle purchase without replacing the original ending or evicting Zelda.
6. Optional gear/cosmetics and deliberate combat improvements; preserve required
   story equipment and avoid changing shared assets globally.

Do not treat these as already implemented. All other world populations remain
planned until actors are actually integrated. No proprietary files belong in
Git. Do not restart toolchain setup. Use Build.ps1 and Test.ps1 in tools/living-hyrule.
