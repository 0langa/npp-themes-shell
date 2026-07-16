#include "nppthemes/ShellPalette.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#else
#include <json/json.hpp>
#endif

namespace nppthemes {
namespace {

struct RoleBinding {
    std::string_view name;
    Color ShellPalette::*member;
};

constexpr std::array<RoleBinding, 45> shellColorRoles = {
    RoleBinding{"shell.window.background", &ShellPalette::windowBackground},
    RoleBinding{"shell.surface.primary", &ShellPalette::surfacePrimary},
    RoleBinding{"shell.surface.secondary", &ShellPalette::surfaceSecondary},
    RoleBinding{"shell.surface.raised", &ShellPalette::surfaceRaised},
    RoleBinding{"shell.border", &ShellPalette::border},
    RoleBinding{"shell.divider", &ShellPalette::divider},
    RoleBinding{"shell.focusRing", &ShellPalette::focusRing},
    RoleBinding{"shell.caption.background", &ShellPalette::captionBackground},
    RoleBinding{"shell.caption.foreground", &ShellPalette::captionForeground},
    RoleBinding{"shell.caption.inactiveForeground", &ShellPalette::captionInactiveForeground},
    RoleBinding{"shell.menu.background", &ShellPalette::menuBackground},
    RoleBinding{"shell.menu.foreground", &ShellPalette::menuForeground},
    RoleBinding{"shell.menu.hotBackground", &ShellPalette::menuHotBackground},
    RoleBinding{"shell.menu.disabledForeground", &ShellPalette::menuDisabledForeground},
    RoleBinding{"shell.toolbar.background", &ShellPalette::toolbarBackground},
    RoleBinding{"shell.toolbar.hover", &ShellPalette::toolbarHover},
    RoleBinding{"shell.toolbar.pressed", &ShellPalette::toolbarPressed},
    RoleBinding{"shell.toolbar.separator", &ShellPalette::toolbarSeparator},
    RoleBinding{"shell.tab.active", &ShellPalette::tabActive},
    RoleBinding{"shell.tab.inactive", &ShellPalette::tabInactive},
    RoleBinding{"shell.tab.hover", &ShellPalette::tabHover},
    RoleBinding{"shell.tab.foreground", &ShellPalette::tabForeground},
    RoleBinding{"shell.tab.dirty", &ShellPalette::tabDirty},
    RoleBinding{"shell.tab.closeHover", &ShellPalette::tabCloseHover},
    RoleBinding{"shell.status.background", &ShellPalette::statusBackground},
    RoleBinding{"shell.status.foreground", &ShellPalette::statusForeground},
    RoleBinding{"shell.status.separator", &ShellPalette::statusSeparator},
    RoleBinding{"shell.scrollbar.track", &ShellPalette::scrollbarTrack},
    RoleBinding{"shell.scrollbar.thumb", &ShellPalette::scrollbarThumb},
    RoleBinding{"shell.scrollbar.thumbHover", &ShellPalette::scrollbarThumbHover},
    RoleBinding{"shell.control.background", &ShellPalette::controlBackground},
    RoleBinding{"shell.control.foreground", &ShellPalette::controlForeground},
    RoleBinding{"shell.control.border", &ShellPalette::controlBorder},
    RoleBinding{"shell.control.hover", &ShellPalette::controlHover},
    RoleBinding{"shell.control.pressed", &ShellPalette::controlPressed},
    RoleBinding{"shell.control.disabledForeground", &ShellPalette::controlDisabledForeground},
    RoleBinding{"shell.dialog.background", &ShellPalette::dialogBackground},
    RoleBinding{"shell.dialog.surface", &ShellPalette::dialogSurface},
    RoleBinding{"shell.dialog.foreground", &ShellPalette::dialogForeground},
    RoleBinding{"shell.dialog.muted", &ShellPalette::dialogMuted},
    RoleBinding{"shell.icon.foreground", &ShellPalette::iconForeground},
    RoleBinding{"shell.icon.muted", &ShellPalette::iconMuted},
    RoleBinding{"shell.icon.accent", &ShellPalette::iconAccent},
    RoleBinding{"shell.icon.warning", &ShellPalette::iconWarning},
    RoleBinding{"shell.icon.error", &ShellPalette::iconError},
};

[[nodiscard]] std::uint8_t channel(Color color, unsigned int shift) noexcept {
    return static_cast<std::uint8_t>((color >> shift) & 0xFFU);
}

[[nodiscard]] Color compose(std::uint8_t red, std::uint8_t green, std::uint8_t blue) noexcept {
    return (static_cast<Color>(red) << 16U) | (static_cast<Color>(green) << 8U) | static_cast<Color>(blue);
}

} // namespace

Color blendColors(Color first, Color second, double secondWeight) noexcept {
    const double weight = std::clamp(secondWeight, 0.0, 1.0);
    const auto mix = [weight](std::uint8_t left, std::uint8_t right) {
        const double value = static_cast<double>(left) * (1.0 - weight) + static_cast<double>(right) * weight;
        return static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
    };
    return compose(mix(channel(first, 16U), channel(second, 16U)), mix(channel(first, 8U), channel(second, 8U)),
                   mix(channel(first, 0U), channel(second, 0U)));
}

ShellPalette deriveShellPalette(const ThemeProfile& profile) noexcept {
    const auto& editor = profile.palette;
    const Color surfaceSecondary = blendColors(editor.surface, editor.background, 0.48);
    const Color surfaceRaised = blendColors(editor.surface, editor.foreground, profile.dark ? 0.08 : 0.03);
    const Color border = blendColors(editor.surface, editor.foreground, profile.dark ? 0.25 : 0.18);
    const Color divider = blendColors(editor.surface, editor.foreground, profile.dark ? 0.16 : 0.12);
    const Color hover = blendColors(editor.surface, editor.accent, profile.dark ? 0.22 : 0.12);
    const Color pressed = blendColors(editor.surface, editor.accent, profile.dark ? 0.34 : 0.22);

    ShellPalette palette{
        editor.background,
        editor.surface,
        surfaceSecondary,
        surfaceRaised,
        border,
        divider,
        editor.accent,

        editor.surface,
        editor.foreground,
        editor.muted,

        editor.surface,
        editor.foreground,
        hover,
        editor.muted,

        editor.surface,
        hover,
        pressed,
        divider,

        editor.background,
        surfaceSecondary,
        hover,
        editor.foreground,
        editor.accent,
        editor.string,

        editor.surface,
        editor.foreground,
        divider,

        editor.background,
        editor.muted,
        editor.accent,

        editor.background,
        editor.foreground,
        border,
        hover,
        pressed,
        editor.muted,

        editor.background,
        editor.surface,
        editor.foreground,
        editor.muted,

        editor.foreground,
        editor.muted,
        editor.accent,
        editor.number,
        editor.string,
    };
    if (profile.shell) {
        for (const auto& binding : shellColorRoles) {
            if (const auto overrideValue = profile.shell->colorOverrides.find(binding.name);
                overrideValue != profile.shell->colorOverrides.end()) {
                palette.*(binding.member) = overrideValue->second;
            }
        }
    }
    return palette;
}

std::vector<std::string> validateShellPalette(const ShellPalette& palette) {
    std::vector<std::string> errors;
    const std::array<std::pair<const char*, double>, 6> checks = {
        std::pair{"caption foreground", contrastRatio(palette.captionForeground, palette.captionBackground)},
        std::pair{"menu foreground", contrastRatio(palette.menuForeground, palette.menuBackground)},
        std::pair{"tab foreground", contrastRatio(palette.tabForeground, palette.tabActive)},
        std::pair{"status foreground", contrastRatio(palette.statusForeground, palette.statusBackground)},
        std::pair{"control foreground", contrastRatio(palette.controlForeground, palette.controlBackground)},
        std::pair{"dialog foreground", contrastRatio(palette.dialogForeground, palette.dialogBackground)},
    };
    for (const auto& [name, ratio] : checks) {
        if (ratio < 4.5) {
            errors.emplace_back(std::string(name) + " contrast must be at least 4.5:1");
        }
    }
    if (contrastRatio(palette.focusRing, palette.windowBackground) < 3.0) {
        errors.emplace_back("focus ring contrast must be at least 3.0:1");
    }
    return errors;
}

bool isShellColorRole(std::string_view role) noexcept {
    return std::any_of(shellColorRoles.begin(), shellColorRoles.end(),
                       [role](const RoleBinding& binding) { return binding.name == role; });
}

std::string serializeShellPalette(const ShellPalette& palette) {
    nlohmann::ordered_json tokens = nlohmann::ordered_json::object();
    for (const auto& binding : shellColorRoles) {
        tokens[std::string(binding.name)] = formatHexColor(palette.*(binding.member));
    }
    const nlohmann::ordered_json document = {
        {"contractVersion", 1},
        {"tokens", std::move(tokens)},
    };
    return document.dump(2) + '\n';
}

} // namespace nppthemes
