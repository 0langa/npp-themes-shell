#include "DarkModeThemeAdapter.h"

namespace NppThemesShell {

namespace {

constexpr int customizedTone = 32;

} // namespace

DarkModePaletteState mapDarkModePalette(const nppthemes::ShellPalette& palette) noexcept {
    DarkModePaletteState mapped;
    mapped.tone = customizedTone;
    mapped.background = palette.windowBackground;
    mapped.controlBackground = palette.controlBackground;
    mapped.hotBackground = palette.controlHover;
    mapped.dialogBackground = palette.dialogBackground;
    mapped.errorBackground = nppthemes::blendColors(palette.dialogBackground, palette.iconError, 0.24);
    mapped.text = palette.controlForeground;
    mapped.mutedText = palette.dialogMuted;
    mapped.disabledText = palette.controlDisabledForeground;
    mapped.linkText = palette.iconAccent;
    mapped.edge = palette.controlBorder;
    mapped.hotEdge = palette.focusRing;
    mapped.disabledEdge = palette.divider;
    return mapped;
}

DarkModeThemeAdapter::DarkModeThemeAdapter(ThemeService& service, DarkModePaletteHost& host) noexcept
    : _service(service), _host(host) {}

DarkModeThemeAdapter::~DarkModeThemeAdapter() {
    deactivate();
}

bool DarkModeThemeAdapter::activate() {
    if (_active || !_service.subscribe(*this)) {
        return false;
    }
    _active = true;
    if (const auto* current = _service.snapshot()) {
        onThemeChanged(*current);
    }
    return true;
}

void DarkModeThemeAdapter::deactivate() noexcept {
    if (!_active) {
        return;
    }
    _service.unsubscribe(*this);
    restore();
    _active = false;
}

void DarkModeThemeAdapter::onThemeChanged(const ThemeSnapshot& snapshot) noexcept {
    if (!_active) {
        return;
    }
    if (snapshot.renderMode == ThemeRenderMode::Native) {
        restore();
        return;
    }
    if (!_original) {
        _original = _host.capture();
    }
    _host.apply(mapDarkModePalette(snapshot.palette));
}

void DarkModeThemeAdapter::restore() noexcept {
    if (!_original) {
        return;
    }
    _host.apply(*_original);
    _original.reset();
}

} // namespace NppThemesShell
