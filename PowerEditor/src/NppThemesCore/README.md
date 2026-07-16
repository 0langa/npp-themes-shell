# NppThemesCore

Canonical host-independent profile, shell-token, contrast, migration, and Notepad++ XML compiler library shared by the stock plugin and NppThemes Shell.

The directory is intentionally self-contained for commit-pinned Git subtree export. Its CMake target expects consumers to provide `nlohmann_json::nlohmann_json` and `pugixml::pugixml`; direct Notepad++ builds use the compatible vendored `json/json.hpp` and `pugixml/pugixml.hpp` through compile-time include detection. It has no Win32, Notepad++ plugin ABI, network, telemetry, or executable-asset dependency.

`tools/shell-conformance.cpp` and `tests/fixtures` are part of the exported contract. Both hosts compile that same executable and byte-compare its canonical token output with the checked-in golden result. Run it as:

```text
NppThemesConformance --check <profile.json> <expected.json>
```

Source repository: <https://github.com/0langa/npp-themes>

License: GPL-3.0, governed by the source repository `LICENSE` and the GPL-compatible license of the consuming Notepad++ shell.
