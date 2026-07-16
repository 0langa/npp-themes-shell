# Baseline build evidence

The initial `shell/main` product branch contains official Notepad++ v8.9.7 source at commit `6634650414ff91220a4c353b7fe5ad741af0f9f9` plus bootstrap documentation only.

On 2026-07-16, the unmodified application source built successfully for x64 Release with Visual Studio Build Tools 2022, MSBuild 17.14.40, v143, and Windows SDK 10.0.26100.0:

```powershell
MSBuild.exe PowerEditor\visual.net\notepadPlus.sln /m /p:Configuration=Release /p:Platform=x64 /v:minimal
```

Produced `PowerEditor\bin64\Notepad++.exe` reports file/product version `8.9.7.0`. This proves a local clean baseline only. It is not a reproducibility, branding, security, or distribution qualification.

An isolated startup smoke with `-multiInst -noPlugin -nosession` and a temporary settings directory also passed; the process remained healthy and accepted a normal main-window close.

## Shared-core integration

The commit-pinned NppThemesCore subtree is compiled directly by `notepadPlus.vcxproj` with the solution's existing C++20, warnings-as-errors, vendored nlohmann JSON, and vendored pugixml configuration.

The first integration build exposed that upstream defines `PUGIXML_NO_XPATH`. NppThemesCore removed its XPath dependency in canonical source commit `43fe41b27d0b35b1955bbaa2d2eea321888e1edf`; the plugin core tests passed, the corrected split `0f272a5e82f272cf5c7bc57fc070befe5efcafea` was pulled, and the x64 Release Shell build then passed. This is the intended shared-core compatibility loop: fix canonically, test there, update the pinned subtree, then qualify the shell.

Win32 Release also builds with the shared core and passes isolated startup smoke as version 8.9.7.0.

ARM64 cross-build remains unavailable locally because the installed VS 2022 v143 workload has no ARM64 compiler or libraries (`Hostx64\arm64\cl.exe` and `lib\arm64` are absent). NppThemes Shell CI therefore performs compile-only Release qualification on GitHub's Windows runner for x64, Win32, and ARM64. Physical ARM64 startup, rendering, DPI, and accessibility checks remain deferred until hardware is available; compile success does not replace those runtime gates.

[NppThemes Shell CI run 29503534204](https://github.com/0langa/npp-themes-shell/actions/runs/29503534204) passed contract/service tests plus Release x64, Win32, and ARM64 compilation on 2026-07-16. No binaries were uploaded.

## ThemeService foundation

The first fork-owned ThemeService foundation compiles inside both local Release architectures. Its standalone behavior suite covers validated initialization, atomic invalid-profile rejection, preview cancel/commit, subscription lifetime, generation changes, forced High Contrast native fallback, host-palette mapping, and exact native-palette restoration. The cross-host conformance suite compiles directly from the pinned shared subtree and compares canonical output with its golden fixture.

Local verification commands:

```powershell
cmake -S PowerEditor\Test\NppThemes -B out\theme-tests -A x64
cmake --build out\theme-tests --config Release --parallel
ctest --test-dir out\theme-tests -C Release --output-on-failure
```
