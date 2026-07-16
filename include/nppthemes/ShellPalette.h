#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "nppthemes/ThemeProfile.h"

namespace nppthemes {

struct ShellPalette {
    Color windowBackground{};
    Color surfacePrimary{};
    Color surfaceSecondary{};
    Color surfaceRaised{};
    Color border{};
    Color divider{};
    Color focusRing{};

    Color captionBackground{};
    Color captionForeground{};
    Color captionInactiveForeground{};

    Color menuBackground{};
    Color menuForeground{};
    Color menuHotBackground{};
    Color menuDisabledForeground{};

    Color toolbarBackground{};
    Color toolbarHover{};
    Color toolbarPressed{};
    Color toolbarSeparator{};

    Color tabActive{};
    Color tabInactive{};
    Color tabHover{};
    Color tabForeground{};
    Color tabDirty{};
    Color tabCloseHover{};

    Color statusBackground{};
    Color statusForeground{};
    Color statusSeparator{};

    Color scrollbarTrack{};
    Color scrollbarThumb{};
    Color scrollbarThumbHover{};

    Color controlBackground{};
    Color controlForeground{};
    Color controlBorder{};
    Color controlHover{};
    Color controlPressed{};
    Color controlDisabledForeground{};

    Color dialogBackground{};
    Color dialogSurface{};
    Color dialogForeground{};
    Color dialogMuted{};

    Color iconForeground{};
    Color iconMuted{};
    Color iconAccent{};
    Color iconWarning{};
    Color iconError{};

    bool operator==(const ShellPalette&) const = default;
};

[[nodiscard]] Color blendColors(Color first, Color second, double secondWeight) noexcept;
[[nodiscard]] ShellPalette deriveShellPalette(const ThemeProfile& profile) noexcept;
[[nodiscard]] std::vector<std::string> validateShellPalette(const ShellPalette& palette);
[[nodiscard]] bool isShellColorRole(std::string_view role) noexcept;
[[nodiscard]] std::string serializeShellPalette(const ShellPalette& palette);

} // namespace nppthemes
