#include "AppSurfaceTheme.h"

#include <array>
#include <cstddef>
#include <utility>

namespace NppThemesShell {

namespace {

std::optional<AppSurfaceThemeState> currentSurfaceTheme;

constexpr auto roleCount = static_cast<std::size_t>(AppSurfaceRole::Count);

COLORREF toColorRef(const nppthemes::Color color) noexcept {
    return RGB((color >> 16) & 0xFFU, (color >> 8) & 0xFFU, color & 0xFFU);
}

class SurfaceGdiCache {
public:
    ~SurfaceGdiCache() { clear(); }

    void rebuild(const nppthemes::ShellPalette& palette) noexcept {
        clear();
        for (std::size_t index = 0; index < roleCount; ++index) {
            const auto role = static_cast<AppSurfaceRole>(index);
            const auto color = toColorRef(appSurfaceRoleColor(palette, role));
            _colors[index] = color;
            _brushes[index] = ::CreateSolidBrush(color);
            _pens[index] = ::CreatePen(PS_SOLID, 1, color);
        }
    }

    void clear() noexcept {
        for (auto& brush : _brushes) {
            if (brush) {
                ::DeleteObject(brush);
                brush = nullptr;
            }
        }
        for (auto& pen : _pens) {
            if (pen) {
                ::DeleteObject(pen);
                pen = nullptr;
            }
        }
        _colors.fill(CLR_INVALID);
    }

    [[nodiscard]] COLORREF color(const AppSurfaceRole role) const noexcept {
        return _colors[static_cast<std::size_t>(role)];
    }

    [[nodiscard]] HBRUSH brush(const AppSurfaceRole role) const noexcept {
        return _brushes[static_cast<std::size_t>(role)];
    }

    [[nodiscard]] HPEN pen(const AppSurfaceRole role) const noexcept {
        return _pens[static_cast<std::size_t>(role)];
    }

private:
    std::array<COLORREF, roleCount> _colors{};
    std::array<HBRUSH, roleCount> _brushes{};
    std::array<HPEN, roleCount> _pens{};
};

SurfaceGdiCache surfaceGdiCache;

} // namespace

nppthemes::Color appSurfaceRoleColor(const nppthemes::ShellPalette& palette,
                                     const AppSurfaceRole role) noexcept {
    switch (role) {
        case AppSurfaceRole::WindowBackground: return palette.windowBackground;
        case AppSurfaceRole::SurfacePrimary: return palette.surfacePrimary;
        case AppSurfaceRole::SurfaceSecondary: return palette.surfaceSecondary;
        case AppSurfaceRole::SurfaceRaised: return palette.surfaceRaised;
        case AppSurfaceRole::Border: return palette.border;
        case AppSurfaceRole::Divider: return palette.divider;
        case AppSurfaceRole::FocusRing: return palette.focusRing;
        case AppSurfaceRole::MenuBackground: return palette.menuBackground;
        case AppSurfaceRole::MenuForeground: return palette.menuForeground;
        case AppSurfaceRole::MenuHotBackground: return palette.menuHotBackground;
        case AppSurfaceRole::MenuDisabledForeground: return palette.menuDisabledForeground;
        case AppSurfaceRole::ToolbarBackground: return palette.toolbarBackground;
        case AppSurfaceRole::ToolbarHover: return palette.toolbarHover;
        case AppSurfaceRole::ToolbarPressed: return palette.toolbarPressed;
        case AppSurfaceRole::ToolbarSeparator: return palette.toolbarSeparator;
        case AppSurfaceRole::ControlBackground: return palette.controlBackground;
        case AppSurfaceRole::ControlForeground: return palette.controlForeground;
        case AppSurfaceRole::ControlBorder: return palette.controlBorder;
        case AppSurfaceRole::ControlHover: return palette.controlHover;
        case AppSurfaceRole::ControlPressed: return palette.controlPressed;
        case AppSurfaceRole::ControlDisabledForeground: return palette.controlDisabledForeground;
        case AppSurfaceRole::DialogBackground: return palette.dialogBackground;
        case AppSurfaceRole::DialogSurface: return palette.dialogSurface;
        case AppSurfaceRole::DialogForeground: return palette.dialogForeground;
        case AppSurfaceRole::DialogMuted: return palette.dialogMuted;
        case AppSurfaceRole::IconForeground: return palette.iconForeground;
        case AppSurfaceRole::IconMuted: return palette.iconMuted;
        case AppSurfaceRole::IconAccent: return palette.iconAccent;
        case AppSurfaceRole::Count: break;
    }
    return palette.windowBackground;
}

AppSurfaceThemeAdapter::AppSurfaceThemeAdapter(ThemeService& service, AppSurfaceThemeHost& host) noexcept
    : _service(service), _host(host) {}

AppSurfaceThemeAdapter::~AppSurfaceThemeAdapter() {
    deactivate();
}

bool AppSurfaceThemeAdapter::activate() {
    if (_active || !_service.subscribe(*this)) {
        return false;
    }
    _active = true;
    if (const auto* current = _service.snapshot()) {
        onThemeChanged(*current);
    }
    return true;
}

void AppSurfaceThemeAdapter::deactivate() noexcept {
    if (!_active) {
        return;
    }
    _service.unsubscribe(*this);
    restore();
    _active = false;
}

void AppSurfaceThemeAdapter::onThemeChanged(const ThemeSnapshot& snapshot) noexcept {
    if (!_active) {
        return;
    }
    if (snapshot.renderMode == ThemeRenderMode::Native) {
        restore();
        return;
    }
    try {
        if (!_original) {
            _original = _host.capture();
        }
        _host.apply(AppSurfaceThemeState{snapshot.profile, snapshot.palette});
    } catch (...) {
        restore();
    }
}

void AppSurfaceThemeAdapter::restore() noexcept {
    if (!_original) {
        return;
    }
    _host.apply(std::move(*_original));
    _original.reset();
}

std::optional<AppSurfaceThemeState> NppAppSurfaceThemeHost::capture() const {
    return currentSurfaceTheme;
}

void NppAppSurfaceThemeHost::apply(std::optional<AppSurfaceThemeState> state) {
    currentSurfaceTheme = std::move(state);
    if (currentSurfaceTheme) {
        surfaceGdiCache.rebuild(currentSurfaceTheme->palette);
    } else {
        surfaceGdiCache.clear();
    }
}

AppSurfaceThemeHost& appSurfaceThemeHost() noexcept {
    static NppAppSurfaceThemeHost host;
    return host;
}

const AppSurfaceThemeState* activeAppSurfaceTheme() noexcept {
    return currentSurfaceTheme ? &*currentSurfaceTheme : nullptr;
}

COLORREF activeAppSurfaceColor(const AppSurfaceRole role) noexcept {
    return currentSurfaceTheme ? surfaceGdiCache.color(role) : CLR_INVALID;
}

COLORREF activeAppSurfaceColorOr(const AppSurfaceRole role, const COLORREF fallback) noexcept {
    const auto color = activeAppSurfaceColor(role);
    return color == CLR_INVALID ? fallback : color;
}

HBRUSH activeAppSurfaceBrush(const AppSurfaceRole role) noexcept {
    return currentSurfaceTheme ? surfaceGdiCache.brush(role) : nullptr;
}

HPEN activeAppSurfacePen(const AppSurfaceRole role) noexcept {
    return currentSurfaceTheme ? surfaceGdiCache.pen(role) : nullptr;
}

} // namespace NppThemesShell
