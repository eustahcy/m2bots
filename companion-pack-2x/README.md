# Companion pack 1.0.0 — 2.x line

For the current custom **Metin2 2.0.14 / r41023** server. Windows PowerShell 5.1 and Docker Desktop are required. Extract `companion-pack-2x` directly inside the server folder, beside `VERSION`. Log out and close the launcher/updater while installing or removing the pack.

1. Run **Check.bat**. Unknown versions, edited files, or changed build dependencies stop the operation.
2. Run **Install.bat**. It verifies companion-table access with the game database account (root is needed only for initialization), backs up all nine managed files, installs the captured source, synchronizes the manager `.cpp`/`.h` and combat header overlays, then builds and restarts only the `game` service with two compiler jobs. Docker may compile dependencies required by that image. Database and other services are not rebuilt.
3. Check in game: spawn, follow, trade, friend/party, pack combat, skill 31 for Mija, loot, death/respawn and follow/combat again. Test owner buffs with a Shaman companion.

For another folder location:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\companion-pack-2x\tools\manage.ps1 -Mode Check -ServerRoot 'F:\Metin2 Singleplayer\Serwer'
```

Use `Install`, `Rollback` or `Uninstall` in place of `Check`. `-SourceOnly` is for testing: it skips SQL, building and deployment; the running game is unchanged.

## Recovery and removal

**Rollback.bat** restores the exact files present before the first installation, then rebuilds game. **Uninstall.bat** restores the package's captured pre-mod baseline, including when the custom code was already present before installation. Both retain the backups and refuse to overwrite later edits. This distinction matters on an already-modified server.

Backups and the operation journal live in `SERVER/.companion-pack-2x`. Keep this folder and the matching package. Rerun an interrupted operation to resume. A build failure leaves the source staged and does not start deployment; use Rollback for recovery. A failure during game recreation can leave game stopped until a rebuild/restart succeeds. Source rollback is not an instant Docker image rollback.

Characters, items and companion relationships are preserved. The installer creates no character links and does not copy accounts or databases. Uninstall leaves the companion table and its rows intact.

## Preserved implementation

Includes the captured companion registry, login/follow/recall and death handling, bot trade acceptance, existing pack combat, normal weapon/skill/loot helpers, and the exact final Shaman owner-buff restore from the referenced chat. Existing friend/party behavior remains supplied by the supported base. **The PID 2505 (Mija) dagger-skill correction remains hardcoded**, exactly as implemented; this is a user-specific build, not a generalized character configuration tool. No queued features were added.

The disk snapshot had the owner-buff call but lacked its definition. The package adds the exact final chat restore to both source and overlay. Therefore rollback on that particular pre-install snapshot also restores that missing-helper condition; uninstall restores the pre-mod manager without that call. See `manifest.json` for baseline provenance and `review/` for diffs.

## Updates and build lines

Checks are deliberately strict: full-file payloads are written only over captured baseline or installed hashes. There is no fuzzy patching or automatic updater hook. Run Check before any rebuild after an upstream update. If it fails, retain backups and port the reviewed differences to the new base; do not copy this payload over newer source. Uninstall/rollback also refuse a changed version or dependency structure because rebuilding old source against it is unsafe.

Keep this directory on a future `companion/2.x` branch. The existing `custom-mods-1332` package belongs on `companion/1.x`; it is not upgraded or overwritten here. They have separate metadata and state directories. Do not install both onto one server tree. No GitHub branches have been created or published.

## Validation

Isolated installer tests cover byte-exact install, repeat install, rollback, uninstall, already-modded adoption, conflicts, changed versions/dependencies, damaged backups, partial application and locking. Runtime command tests use a fake Docker command. **A fresh real Docker build and in-game acceptance test of this packaged snapshot have not been run.** The referenced chat reports working gameplay; that does not substitute for testing this archive.
