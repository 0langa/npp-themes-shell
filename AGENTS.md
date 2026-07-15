# NppThemes Shell agent rules

- Treat `shell/main` as the product branch and `master` as an untouched upstream mirror.
- Read `NPP_THEMES_SHELL.md` and the shared full-window roadmap before product changes.
- Keep upstream changes separable from NppThemes changes; record the upstream base on every sync.
- Do not bundle or load the stock `NppThemes.dll`. Reuse only the reviewed, commit-pinned shared core.
- Preserve official build behavior before adding theme behavior. Keep native and High Contrast fallback paths usable.
- Do not distribute binaries under official Notepad++ branding. Branding/coexistence work precedes any public binary.
- Preserve GPL and third-party license/attribution files.
