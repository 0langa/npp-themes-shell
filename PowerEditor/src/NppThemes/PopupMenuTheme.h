#pragma once

#include <windows.h>

namespace NppThemesShell {

BOOL trackThemedPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved,
                          HWND owner, const RECT* reservedRect = nullptr);
BOOL trackThemedPopupMenuEx(HMENU menu, UINT flags, int x, int y,
                            HWND owner, LPTPMPARAMS params = nullptr);

} // namespace NppThemesShell
