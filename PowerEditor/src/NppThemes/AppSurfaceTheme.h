#pragma once

#include <optional>
#include <windows.h>

#include "ThemeService.h"

namespace NppThemesShell {

struct AppSurfaceThemeState {
    nppthemes::ThemeProfile profile;
    nppthemes::ShellPalette palette;

    bool operator==(const AppSurfaceThemeState&) const = default;
};

enum class AppSurfaceRole {
    WindowBackground,
    SurfacePrimary,
    SurfaceSecondary,
    SurfaceRaised,
    Border,
    Divider,
    FocusRing,
    MenuBackground,
    MenuForeground,
    MenuHotBackground,
    MenuDisabledForeground,
    ToolbarBackground,
    ToolbarHover,
    ToolbarPressed,
    ToolbarSeparator,
    ControlBackground,
    ControlForeground,
    ControlBorder,
    ControlHover,
    ControlPressed,
    ControlDisabledForeground,
    DialogBackground,
    DialogSurface,
    DialogForeground,
    DialogMuted,
    IconForeground,
    IconMuted,
    IconAccent,
    Count,
};

[[nodiscard]] nppthemes::Color appSurfaceRoleColor(const nppthemes::ShellPalette& palette,
                                                   AppSurfaceRole role) noexcept;

class AppSurfaceThemeHost {
public:
    virtual ~AppSurfaceThemeHost() = default;
    [[nodiscard]] virtual std::optional<AppSurfaceThemeState> capture() const = 0;
    virtual void apply(std::optional<AppSurfaceThemeState> state) = 0;
};

class AppSurfaceThemeAdapter final : public ThemeSubscriber {
public:
    AppSurfaceThemeAdapter(ThemeService& service, AppSurfaceThemeHost& host) noexcept;
    ~AppSurfaceThemeAdapter() override;

    [[nodiscard]] bool activate();
    void deactivate() noexcept;
    void onThemeChanged(const ThemeSnapshot& snapshot) noexcept override;

    [[nodiscard]] bool isActive() const noexcept { return _active; }

private:
    void restore() noexcept;

    ThemeService& _service;
    AppSurfaceThemeHost& _host;
    std::optional<std::optional<AppSurfaceThemeState>> _original;
    bool _active{};
};

class NppAppSurfaceThemeHost final : public AppSurfaceThemeHost {
public:
    [[nodiscard]] std::optional<AppSurfaceThemeState> capture() const override;
    void apply(std::optional<AppSurfaceThemeState> state) override;
};

[[nodiscard]] AppSurfaceThemeHost& appSurfaceThemeHost() noexcept;
[[nodiscard]] const AppSurfaceThemeState* activeAppSurfaceTheme() noexcept;
[[nodiscard]] COLORREF activeAppSurfaceColor(AppSurfaceRole role) noexcept;
[[nodiscard]] COLORREF activeAppSurfaceColorOr(AppSurfaceRole role, COLORREF fallback) noexcept;
[[nodiscard]] HBRUSH activeAppSurfaceBrush(AppSurfaceRole role) noexcept;
[[nodiscard]] HPEN activeAppSurfacePen(AppSurfaceRole role) noexcept;

} // namespace NppThemesShell
