#pragma once

#include <optional>

#include "ThemeService.h"

namespace NppThemesShell {

struct DarkModePaletteState {
    int tone{};
    nppthemes::Color background{};
    nppthemes::Color controlBackground{};
    nppthemes::Color hotBackground{};
    nppthemes::Color dialogBackground{};
    nppthemes::Color errorBackground{};
    nppthemes::Color text{};
    nppthemes::Color mutedText{};
    nppthemes::Color disabledText{};
    nppthemes::Color linkText{};
    nppthemes::Color edge{};
    nppthemes::Color hotEdge{};
    nppthemes::Color disabledEdge{};

    bool operator==(const DarkModePaletteState&) const = default;
};

class DarkModePaletteHost {
public:
    virtual ~DarkModePaletteHost() = default;
    [[nodiscard]] virtual DarkModePaletteState capture() const noexcept = 0;
    virtual void apply(const DarkModePaletteState& palette) noexcept = 0;
};

[[nodiscard]] DarkModePaletteState mapDarkModePalette(const nppthemes::ShellPalette& palette) noexcept;

class DarkModeThemeAdapter final : public ThemeSubscriber {
public:
    DarkModeThemeAdapter(ThemeService& service, DarkModePaletteHost& host) noexcept;
    ~DarkModeThemeAdapter() override;

    DarkModeThemeAdapter(const DarkModeThemeAdapter&) = delete;
    DarkModeThemeAdapter& operator=(const DarkModeThemeAdapter&) = delete;

    [[nodiscard]] bool activate();
    void deactivate() noexcept;
    void onThemeChanged(const ThemeSnapshot& snapshot) noexcept override;

    [[nodiscard]] bool isActive() const noexcept { return _active; }

private:
    void restore() noexcept;

    ThemeService& _service;
    DarkModePaletteHost& _host;
    std::optional<DarkModePaletteState> _original;
    bool _active{};
};

} // namespace NppThemesShell
