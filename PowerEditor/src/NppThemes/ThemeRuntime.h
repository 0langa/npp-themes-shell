#pragma once

#include <filesystem>
#include <memory>
#include <optional>
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
    [[nodiscard]] bool selectProfile(const nppthemes::ThemeProfile& profile, std::string& error);
    [[nodiscard]] bool disable(std::string& error);
    void setHighContrastActive(bool active) noexcept;
    void shutdown() noexcept;

    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] const nppthemes::ThemeProfile* activeProfile() const noexcept;
    [[nodiscard]] ThemeService& service() noexcept { return _service; }

private:
    DarkModePaletteHost& _host;
    ThemeService _service;
    std::unique_ptr<DarkModeThemeAdapter> _adapter;
    std::unique_ptr<StartupProfileStore> _store;
    std::optional<nppthemes::ThemeProfile> _activeProfile;
    std::optional<bool> _originalRendererDark;
    bool _highContrastActive{};
};

[[nodiscard]] ThemeRuntime& themeRuntime() noexcept;

} // namespace NppThemesShell
