# NppThemes Shell bootstrap contract

Status: active Shell development; not distributable as an NppThemes product.

## Baseline

- Upstream: <https://github.com/notepad-plus-plus/notepad-plus-plus>
- Tag: `v8.9.7`
- Commit: `6634650414ff91220a4c353b7fe5ad741af0f9f9`
- Product branch: `shell/main`
- Upstream mirror branch: `master`
- Shared core: <https://github.com/0langa/npp-themes>
- Shared source commit: `d8a66fdb4b279918617d558426262e10a4694843`
- Imported subtree split: `fc616dac6242a788cbe260c75232daf0ef30d4ca`

## Ownership

NppThemes Shell owns profile persistence, Theme Studio, preview/apply/restore, ThemeService, frame and host-control rendering. It imports the shared profile/token core through a reviewed commit-pinned Git subtree under `PowerEditor/src/NppThemesCore`. It does not load or bundle the stock `NppThemes.dll`.

The imported core is compiled directly in the existing Notepad++ solution using upstream's vendored nlohmann JSON and pugixml. Update it only with `git subtree pull --prefix=PowerEditor/src/NppThemesCore ... --squash`, then record and verify the new split commit through the cross-host conformance suite.

## Current theme foundation

- `PowerEditor/src/NppThemes/ThemeService.*` validates and resolves profiles before mutation.
- Preview/apply/cancel is transactional; rejected profiles preserve active profile, tokens, mode, and generation.
- Windows High Contrast forces native render mode and cannot be overridden by profile changes.
- Typed subscribers receive one generation-stamped snapshot per visible state change.
- Reversible dark-mode adapter maps resolved semantic roles into existing host brushes/pens, snapshots exact native tone/colors, and restores them for High Contrast, native mode, or deactivation.
- Shell-owned tests compile the imported conformance executable and byte-check canonical tokens after checkout line-ending normalization.
- Dedicated Shell CI compiles Release x64, Win32, and ARM64 without uploading unofficial binaries. Physical ARM64 runtime remains a separate release-qualification gate.

ThemeService, startup persistence, and first host palette adapter compile into the application. User profile selection UI and additional surface adapters remain Phase 4 work.

## Startup profile contract

Shell looks for `NppThemes\active-profile.json` below Notepad++'s resolved settings root. Installed, portable, cloud, and `-settingsDir` modes therefore remain isolated. Missing file keeps native rendering and creates no directory or marker.

Startup accepts only a regular JSON file up to 1 MiB. Profile parsing, migration, palette derivation, and contrast validation finish before host mutation. Shell then durably creates `NppThemes\apply.incomplete`, activates ThemeService and host adapter, and removes marker only after success.

Marker found on next launch means previous activation may have crashed. Shell deletes marker, skips custom activation for that launch, and remains native. Following launch may try profile again. Invalid, oversized, unreadable, or unsafe-path input remains native.

Forced Windows High Contrast switches active runtime to native colors immediately and restores custom palette only after High Contrast ends. Deleting or renaming `active-profile.json` disables startup activation on next launch. Current adapter affects controls already using Notepad++ dark-mode renderer; automatic light/dark renderer coordination and selection UI remain upcoming.

## Safety and release rules

- Windows forced High Contrast always wins.
- Startup must fall back to native rendering after an incomplete prior apply.
- No package ships before distinct branding, side-by-side paths, explicit settings import, rollback, GPL source, notices, checksums, SBOM, and provenance exist.
- Preview/beta may be unsigned only when clearly marked and accompanied by provenance/checksums. Shell 1.0 is blocked without project-controlled Authenticode signing.
- Third-party plugin windows, OS file pickers, IME/accessibility overlays, and compositor output are outside complete-window ownership.

Detailed roadmap, surface inventory, and upstream procedure live in the shared repository:

- <https://github.com/0langa/npp-themes/blob/main/FULL_WINDOW_THEMING_PLAN.md>
- <https://github.com/0langa/npp-themes/blob/main/docs/shell/surface-inventory.md>
- <https://github.com/0langa/npp-themes/blob/main/docs/shell/upstream-strategy.md>
