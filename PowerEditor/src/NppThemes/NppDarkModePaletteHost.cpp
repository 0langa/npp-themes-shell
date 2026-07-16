#include "NppDarkModePaletteHost.h"

#include <windows.h>

#include "NppDarkMode.h"

namespace NppThemesShell {

namespace {

[[nodiscard]] nppthemes::Color fromColorRef(const COLORREF color) noexcept {
    return (static_cast<nppthemes::Color>(GetRValue(color)) << 16U) |
           (static_cast<nppthemes::Color>(GetGValue(color)) << 8U) |
           static_cast<nppthemes::Color>(GetBValue(color));
}

[[nodiscard]] COLORREF toColorRef(const nppthemes::Color color) noexcept {
    return RGB((color >> 16U) & 0xFFU, (color >> 8U) & 0xFFU, color & 0xFFU);
}

} // namespace

DarkModePaletteState NppDarkModePaletteHost::capture() const noexcept {
    DarkModePaletteState captured;
    captured.tone = static_cast<int>(NppDarkMode::getDarkTone());
    captured.background = fromColorRef(NppDarkMode::getBackgroundColor());
    captured.controlBackground = fromColorRef(NppDarkMode::getCtrlBackgroundColor());
    captured.hotBackground = fromColorRef(NppDarkMode::getHotBackgroundColor());
    captured.dialogBackground = fromColorRef(NppDarkMode::getDlgBackgroundColor());
    captured.errorBackground = fromColorRef(NppDarkMode::getErrorBackgroundColor());
    captured.text = fromColorRef(NppDarkMode::getTextColor());
    captured.mutedText = fromColorRef(NppDarkMode::getDarkerTextColor());
    captured.disabledText = fromColorRef(NppDarkMode::getDisabledTextColor());
    captured.linkText = fromColorRef(NppDarkMode::getLinkTextColor());
    captured.edge = fromColorRef(NppDarkMode::getEdgeColor());
    captured.hotEdge = fromColorRef(NppDarkMode::getHotEdgeColor());
    captured.disabledEdge = fromColorRef(NppDarkMode::getDisabledEdgeColor());
    return captured;
}

void NppDarkModePaletteHost::apply(const DarkModePaletteState& palette) noexcept {
    const auto tone = static_cast<NppDarkMode::ColorTone>(palette.tone);
    if (tone == NppDarkMode::ColorTone::customizedTone) {
        NppDarkMode::Colors colors;
        colors.background = toColorRef(palette.background);
        colors.softerBackground = toColorRef(palette.controlBackground);
        colors.hotBackground = toColorRef(palette.hotBackground);
        colors.pureBackground = toColorRef(palette.dialogBackground);
        colors.errorBackground = toColorRef(palette.errorBackground);
        colors.text = toColorRef(palette.text);
        colors.darkerText = toColorRef(palette.mutedText);
        colors.disabledText = toColorRef(palette.disabledText);
        colors.linkText = toColorRef(palette.linkText);
        colors.edge = toColorRef(palette.edge);
        colors.hotEdge = toColorRef(palette.hotEdge);
        colors.disabledEdge = toColorRef(palette.disabledEdge);
        NppDarkMode::changeCustomTheme(colors);
    }
    NppDarkMode::setDarkTone(tone);
}

bool NppDarkModePaletteHost::rendererDark() const noexcept {
    return NppDarkMode::isEnabled();
}

void NppDarkModePaletteHost::setRendererDark(const bool dark) noexcept {
    NppDarkMode::setRuntimeEnabled(dark);
}

} // namespace NppThemesShell
