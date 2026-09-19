#include "ThemeRuntime.h"

#include "NppDarkModePaletteHost.h"

namespace NppThemesShell {

ThemeRuntime& themeRuntime() noexcept {
    static NppDarkModePaletteHost host;
    static ThemeRuntime runtime(host, appSurfaceThemeHost());
    return runtime;
}

} // namespace NppThemesShell
