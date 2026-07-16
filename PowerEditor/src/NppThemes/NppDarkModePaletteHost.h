#pragma once

#include "DarkModeThemeAdapter.h"

namespace NppThemesShell {

class NppDarkModePaletteHost final : public DarkModePaletteHost {
public:
    [[nodiscard]] DarkModePaletteState capture() const noexcept override;
    void apply(const DarkModePaletteState& palette) noexcept override;
};

} // namespace NppThemesShell
