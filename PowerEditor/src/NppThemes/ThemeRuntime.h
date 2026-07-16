#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "DarkModeThemeAdapter.h"
#include "StartupProfileStore.h"

namespace NppThemesShell {

struct ThemeRuntimeResult {
    StartupProfileStatus status{StartupProfileStatus::Disabled};
    std::string diagnostic;
};

class ThemeRuntime {
public:
    explicit ThemeRuntime(DarkModePaletteHost& host) noexcept;
    ~ThemeRuntime();

    [[nodiscard]] ThemeRuntimeResult initialize(const std::filesystem::path& settingsRoot,
                                                bool highContrastActive);
    void setHighContrastActive(bool active) noexcept;
    void shutdown() noexcept;

    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] ThemeService& service() noexcept { return _service; }

private:
    DarkModePaletteHost& _host;
    ThemeService _service;
    std::unique_ptr<DarkModeThemeAdapter> _adapter;
};

[[nodiscard]] ThemeRuntime& themeRuntime() noexcept;

} // namespace NppThemesShell
