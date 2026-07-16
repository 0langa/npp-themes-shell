#include "ThemeRuntime.h"

#include "NppDarkModePaletteHost.h"

namespace NppThemesShell {

ThemeRuntime& themeRuntime() noexcept {
    static NppDarkModePaletteHost host;
    static ThemeRuntime runtime(host);
    return runtime;
}

} // namespace NppThemesShell
