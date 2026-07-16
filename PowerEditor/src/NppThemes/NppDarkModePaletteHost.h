#pragma once

#include "DarkModeThemeAdapter.h"

namespace NppThemesShell {

class NppDarkModePaletteHost final : public DarkModePaletteHost {
public:
    [[nodiscard]] DarkModePaletteState capture() const noexcept override;
    void apply(const DarkModePaletteState& palette) noexcept override;
    [[nodiscard]] bool rendererDark() const noexcept override;
    void setRendererDark(bool dark) noexcept override;
};

} // namespace NppThemesShell
