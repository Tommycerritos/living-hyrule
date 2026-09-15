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

Branch: `feature/living-hyrule-regional-properties`, based on integration commit
5598595a5. Four automated test suites pass. Full configuration/build is currently
running; inspect `C:\ZeldaDev\logs\overnight-properties-configure.log` and
`overnight-properties-build.log`, and active compiler processes before starting
another build. Current executable is still the earlier economy/resident build
until this checkpoint is updated with staging success.

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

## Required finish for this increment

1. Finish full build; resolve errors if any. Run Stage only after success and
   while the development executable is closed. Never invoke Run.
2. Verify output/runtime hashes and update the local latest-build receipt.
3. Update README/status/changelog to accurately describe this increment.
4. Review the diff, commit source only through repository guards, push feature
   branch and integrate into living-hyrule when the build is successful.

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
