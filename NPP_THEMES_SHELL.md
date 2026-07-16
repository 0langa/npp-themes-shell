# NppThemes Shell bootstrap contract

Status: unmodified upstream baseline; not distributable as an NppThemes product.

## Baseline

- Upstream: <https://github.com/notepad-plus-plus/notepad-plus-plus>
- Tag: `v8.9.7`
- Commit: `6634650414ff91220a4c353b7fe5ad741af0f9f9`
- Product branch: `shell/main`
- Upstream mirror branch: `master`
- Shared core: <https://github.com/0langa/npp-themes>
- Shared source commit: `cda602cf9b48c55f7041f7c58f5d04c4d493d86a`
- Imported subtree split: `9b54344a0c4c6fa847ec76215abbce00a05e22ee`

## Ownership

NppThemes Shell owns profile persistence, Theme Studio, preview/apply/restore, ThemeService, frame and host-control rendering. It imports the shared profile/token core through a reviewed commit-pinned Git subtree under `PowerEditor/src/NppThemesCore`. It does not load or bundle the stock `NppThemes.dll`.

The imported core is compiled directly in the existing Notepad++ solution using upstream's vendored nlohmann JSON and pugixml. Update it only with `git subtree pull --prefix=PowerEditor/src/NppThemesCore ... --squash`, then record and verify the new split commit through the cross-host conformance suite.

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
