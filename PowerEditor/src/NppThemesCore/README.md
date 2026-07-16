# NppThemesCore

Canonical host-independent profile, shell-token, contrast, migration, and Notepad++ XML compiler library shared by the stock plugin and NppThemes Shell.

The directory is intentionally self-contained for commit-pinned Git subtree export. Its CMake target expects consumers to provide `nlohmann_json::nlohmann_json` and `pugixml::pugixml`. It has no Win32, Notepad++ plugin ABI, network, telemetry, or executable-asset dependency.

Source repository: <https://github.com/0langa/npp-themes>

License: GPL-3.0, governed by the source repository `LICENSE` and the GPL-compatible license of the consuming Notepad++ shell.
