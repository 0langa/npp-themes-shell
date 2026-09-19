#include "PopupMenuTheme.h"

#include <commctrl.h>

#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#include "AppSurfaceTheme.h"

namespace NppThemesShell {

namespace {

struct PopupItem {
    HMENU menu{};
    UINT position{};
    UINT type{};
    ULONG_PTR data{};
    UINT state{};
    UINT id{};
    HMENU submenu{};
    HBITMAP bitmap{};
    std::wstring text;
};

struct PopupBackground {
    HMENU menu{};
    HBRUSH brush{};
};

class PopupThemeSession {
public:
    PopupThemeSession(HMENU menu, HWND owner) : _owner(owner) {
        if (activeAppSurfaceTheme() == nullptr || menu == nullptr || owner == nullptr) {
            return;
        }
        collectMenu(menu);
        apply();
    }

    ~PopupThemeSession() { restore(); }

    PopupThemeSession(const PopupThemeSession&) = delete;
    PopupThemeSession& operator=(const PopupThemeSession&) = delete;

    [[nodiscard]] bool active() const noexcept { return _active; }

    [[nodiscard]] PopupItem* find(const ULONG_PTR data) noexcept {
        const auto address = reinterpret_cast<PopupItem*>(data);
        const auto found = std::find_if(_items.begin(), _items.end(),
            [address](PopupItem& item) { return &item == address; });
        return found == _items.end() ? nullptr : &*found;
    }

    void measure(MEASUREITEMSTRUCT& measure) noexcept {
        auto* item = find(measure.itemData);
        if (item == nullptr) {
            return;
        }

        HDC hdc = ::GetDC(_owner);
        if (hdc == nullptr) {
            return;
        }
        auto oldFont = static_cast<HFONT>(::SelectObject(hdc, _font));
        TEXTMETRIC metrics{};
        ::GetTextMetrics(hdc, &metrics);

        const int padding = scale(8);
        const int gutter = scale(28);
        const int arrow = item->submenu == nullptr ? 0 : scale(16);
        if ((item->type & MFT_SEPARATOR) != 0) {
            measure.itemWidth = static_cast<UINT>(scale(48));
            measure.itemHeight = static_cast<UINT>(std::max(scale(7), static_cast<int>(metrics.tmHeight) / 2));
        } else {
            const auto tab = item->text.find(L'\t');
            const std::wstring left = item->text.substr(0, tab);
            const std::wstring right = tab == std::wstring::npos ? std::wstring{} : item->text.substr(tab + 1);
            SIZE leftSize{};
            SIZE rightSize{};
            ::GetTextExtentPoint32(hdc, left.c_str(), static_cast<int>(left.size()), &leftSize);
            ::GetTextExtentPoint32(hdc, right.c_str(), static_cast<int>(right.size()), &rightSize);
            const int shortcutGap = right.empty() ? 0 : scale(24);
            measure.itemWidth = static_cast<UINT>(gutter + padding * 2 + leftSize.cx + shortcutGap + rightSize.cx + arrow);
            measure.itemHeight = static_cast<UINT>(std::max(scale(24), static_cast<int>(metrics.tmHeight) + padding));
        }

        ::SelectObject(hdc, oldFont);
        ::ReleaseDC(_owner, hdc);
    }

    void draw(const DRAWITEMSTRUCT& draw) noexcept {
        auto* item = find(draw.itemData);
        if (item == nullptr) {
            return;
        }

        const bool selected = (draw.itemState & ODS_SELECTED) != 0;
        const bool disabled = (draw.itemState & (ODS_DISABLED | ODS_GRAYED)) != 0;
        const bool checked = (draw.itemState & ODS_CHECKED) != 0 || (item->state & MFS_CHECKED) != 0;
        const auto backgroundRole = selected ? AppSurfaceRole::MenuHotBackground : AppSurfaceRole::MenuBackground;
        ::FillRect(draw.hDC, &draw.rcItem, activeAppSurfaceBrush(backgroundRole));

        const int padding = scale(8);
        const int gutter = scale(28);
        if ((item->type & MFT_SEPARATOR) != 0) {
            RECT line = draw.rcItem;
            line.left += gutter;
            line.right -= padding;
            const int middle = (line.top + line.bottom) / 2;
            line.top = middle;
            line.bottom = middle + 1;
            ::FillRect(draw.hDC, &line, activeAppSurfaceBrush(AppSurfaceRole::Divider));
            return;
        }

        auto oldFont = static_cast<HFONT>(::SelectObject(draw.hDC, _font));
        ::SetBkMode(draw.hDC, TRANSPARENT);
        ::SetTextColor(draw.hDC, activeAppSurfaceColor(disabled
            ? AppSurfaceRole::MenuDisabledForeground
            : AppSurfaceRole::MenuForeground));

        RECT markRect = draw.rcItem;
        markRect.left += scale(4);
        markRect.right = markRect.left + scale(20);
        if (checked) {
            const wchar_t* mark = (item->type & MFT_RADIOCHECK) != 0 ? L"●" : L"✓";
            ::DrawText(draw.hDC, mark, 1, &markRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        } else if (item->bitmap != nullptr && item->bitmap != HBMMENU_CALLBACK &&
                   item->bitmap != HBMMENU_SYSTEM && item->bitmap != HBMMENU_MBAR_RESTORE &&
                   item->bitmap != HBMMENU_MBAR_MINIMIZE && item->bitmap != HBMMENU_MBAR_CLOSE &&
                   item->bitmap != HBMMENU_MBAR_CLOSE_D && item->bitmap != HBMMENU_MBAR_MINIMIZE_D &&
                   item->bitmap != HBMMENU_POPUP_CLOSE && item->bitmap != HBMMENU_POPUP_RESTORE &&
                   item->bitmap != HBMMENU_POPUP_MAXIMIZE && item->bitmap != HBMMENU_POPUP_MINIMIZE) {
            BITMAP bitmap{};
            if (::GetObject(item->bitmap, sizeof(bitmap), &bitmap) != 0) {
                const int x = markRect.left + (markRect.right - markRect.left - bitmap.bmWidth) / 2;
                const int y = markRect.top + (markRect.bottom - markRect.top - bitmap.bmHeight) / 2;
                ::DrawState(draw.hDC, nullptr, nullptr, reinterpret_cast<LPARAM>(item->bitmap), 0,
                            x, y, bitmap.bmWidth, bitmap.bmHeight, DST_BITMAP | (disabled ? DSS_DISABLED : 0));
            }
        }

        RECT textRect = draw.rcItem;
        textRect.left += gutter;
        textRect.right -= padding + (item->submenu == nullptr ? 0 : scale(16));
        const auto tab = item->text.find(L'\t');
        const std::wstring left = item->text.substr(0, tab);
        ::DrawText(draw.hDC, left.c_str(), static_cast<int>(left.size()), &textRect,
                   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (tab != std::wstring::npos) {
            const std::wstring shortcut = item->text.substr(tab + 1);
            ::DrawText(draw.hDC, shortcut.c_str(), static_cast<int>(shortcut.size()), &textRect,
                       DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        if (item->submenu != nullptr) {
            RECT arrowRect = draw.rcItem;
            arrowRect.left = arrowRect.right - scale(18);
            arrowRect.right -= scale(4);
            ::DrawText(draw.hDC, L"›", 1, &arrowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }

        ::SelectObject(draw.hDC, oldFont);
    }

private:
    static LRESULT CALLBACK ownerSubclass(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR subclassId, DWORD_PTR reference) {
        auto* session = reinterpret_cast<PopupThemeSession*>(reference);
        if (message == WM_MEASUREITEM) {
            auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            if (measure != nullptr && measure->CtlType == ODT_MENU && session->find(measure->itemData) != nullptr) {
                session->measure(*measure);
                return TRUE;
            }
        } else if (message == WM_DRAWITEM) {
            auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (draw != nullptr && draw->CtlType == ODT_MENU && session->find(draw->itemData) != nullptr) {
                session->draw(*draw);
                return TRUE;
            }
        } else if (message == WM_NCDESTROY) {
            ::RemoveWindowSubclass(window, ownerSubclass, subclassId);
        }
        return ::DefSubclassProc(window, message, wParam, lParam);
    }

    [[nodiscard]] int scale(const int value) const noexcept {
        HDC hdc = ::GetDC(_owner);
        if (hdc == nullptr) {
            return value;
        }
        const int dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
        ::ReleaseDC(_owner, hdc);
        return ::MulDiv(value, dpi, 96);
    }

    void collectMenu(HMENU menu) {
        MENUINFO menuInfo{sizeof(menuInfo)};
        menuInfo.fMask = MIM_BACKGROUND;
        if (::GetMenuInfo(menu, &menuInfo)) {
            _backgrounds.push_back(PopupBackground{menu, menuInfo.hbrBack});
        }

        const int count = ::GetMenuItemCount(menu);
        for (int position = 0; position < count; ++position) {
            MENUITEMINFO info{sizeof(info)};
            info.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_BITMAP | MIIM_STRING;
            if (!::GetMenuItemInfo(menu, static_cast<UINT>(position), TRUE, &info)) {
                continue;
            }

            std::wstring text(info.cch + 1, L'\0');
            if (info.cch != 0) {
                info.dwTypeData = text.data();
                info.cch = static_cast<UINT>(text.size());
                ::GetMenuItemInfo(menu, static_cast<UINT>(position), TRUE, &info);
                text.resize(wcslen(text.c_str()));
            } else {
                text.clear();
            }

            if ((info.fType & MFT_OWNERDRAW) == 0) {
                _items.push_back(PopupItem{menu, static_cast<UINT>(position), info.fType, info.dwItemData,
                    info.fState, info.wID, info.hSubMenu, info.hbmpItem, std::move(text)});
            }
            if (info.hSubMenu != nullptr) {
                collectMenu(info.hSubMenu);
            }
        }
    }

    void apply() {
        NONCLIENTMETRICS metrics{sizeof(metrics)};
        if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
            _font = ::CreateFontIndirect(&metrics.lfMenuFont);
        }
        if (_font == nullptr) {
            _font = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
            _ownsFont = false;
        }

        for (const auto& background : _backgrounds) {
            MENUINFO menuInfo{sizeof(menuInfo)};
            menuInfo.fMask = MIM_BACKGROUND;
            menuInfo.hbrBack = activeAppSurfaceBrush(AppSurfaceRole::MenuBackground);
            ::SetMenuInfo(background.menu, &menuInfo);
        }
        for (auto& item : _items) {
            MENUITEMINFO info{sizeof(info)};
            info.fMask = MIIM_FTYPE | MIIM_DATA;
            info.fType = item.type | MFT_OWNERDRAW;
            info.dwItemData = reinterpret_cast<ULONG_PTR>(&item);
            ::SetMenuItemInfo(item.menu, item.position, TRUE, &info);
        }

        _subclassId = reinterpret_cast<UINT_PTR>(this);
        _active = ::SetWindowSubclass(_owner, ownerSubclass, _subclassId, reinterpret_cast<DWORD_PTR>(this)) != FALSE;
        if (!_active) {
            restoreItems();
        }
    }

    void restoreItems() noexcept {
        for (auto it = _items.rbegin(); it != _items.rend(); ++it) {
            MENUITEMINFO info{sizeof(info)};
            info.fMask = MIIM_FTYPE | MIIM_DATA;
            info.fType = it->type;
            info.dwItemData = it->data;
            ::SetMenuItemInfo(it->menu, it->position, TRUE, &info);
        }
        for (const auto& background : _backgrounds) {
            MENUINFO menuInfo{sizeof(menuInfo)};
            menuInfo.fMask = MIM_BACKGROUND;
            menuInfo.hbrBack = background.brush;
            ::SetMenuInfo(background.menu, &menuInfo);
        }
    }

    void restore() noexcept {
        if (_active) {
            ::RemoveWindowSubclass(_owner, ownerSubclass, _subclassId);
            restoreItems();
            _active = false;
        }
        if (_ownsFont && _font != nullptr) {
            ::DeleteObject(_font);
        }
        _font = nullptr;
    }

    HWND _owner{};
    std::deque<PopupItem> _items;
    std::vector<PopupBackground> _backgrounds;
    HFONT _font{};
    UINT_PTR _subclassId{};
    bool _active{};
    bool _ownsFont{true};
};

} // namespace

BOOL trackThemedPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved,
                          HWND owner, const RECT* reservedRect) {
    PopupThemeSession session(menu, owner);
    return ::TrackPopupMenu(menu, flags, x, y, reserved, owner, reservedRect);
}

BOOL trackThemedPopupMenuEx(HMENU menu, UINT flags, int x, int y,
                            HWND owner, LPTPMPARAMS params) {
    PopupThemeSession session(menu, owner);
    return ::TrackPopupMenuEx(menu, flags, x, y, owner, params);
}

} // namespace NppThemesShell
