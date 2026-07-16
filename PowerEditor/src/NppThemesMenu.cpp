#include "Notepad_plus.h"

#include <windows.h>

#include <string>

#include "Common.h"
#include "Notepad_plus_Window.h"
#include "Notepad_plus_msgs.h"
#include "NppThemes/ThemeRuntime.h"
#include "menuCmdID.h"

void Notepad_plus::initNppThemesMenu(HWND hwnd)
{
	_nppThemesMenuHandle = ::CreatePopupMenu();
	if (!_nppThemesMenuHandle)
		return;

	::AppendMenuW(_nppThemesMenuHandle, MF_STRING | MF_DISABLED | MF_GRAYED, IDM_NPPTHEMES_STATUS, L"Active: Native");
	::AppendMenuW(_nppThemesMenuHandle, MF_STRING, IDM_NPPTHEMES_DISABLE, L"&Disable custom theme");
	::AppendMenuW(_nppThemesMenuHandle, MF_SEPARATOR, 0, nullptr);

	const auto profiles = nppthemes::builtInProfiles();
	for (size_t index = 0; index < profiles.size() && index < 32; ++index)
	{
		const auto label = string2wstring(profiles[index].name, CP_UTF8);
		::AppendMenuW(_nppThemesMenuHandle, MF_STRING,
			IDM_NPPTHEMES_PROFILE_FIRST + static_cast<UINT>(index), label.c_str());
	}

	const int menuCount = ::GetMenuItemCount(_mainMenuHandle);
	const UINT position = menuCount > 0 ? static_cast<UINT>(menuCount - 1) : 0;
	::InsertMenuW(_mainMenuHandle, position, MF_BYPOSITION | MF_POPUP,
		reinterpret_cast<UINT_PTR>(_nppThemesMenuHandle), L"&NppThemes");
	updateNppThemesMenu();
	::DrawMenuBar(hwnd);
}

void Notepad_plus::updateNppThemesMenu()
{
	if (!_nppThemesMenuHandle)
		return;

	const auto* active = NppThemesShell::themeRuntime().activeProfile();
	std::wstring status = L"Active: Native";
	if (active)
	{
		status = L"Active: ";
		status += string2wstring(active->name, CP_UTF8);
	}

	MENUITEMINFOW statusInfo{};
	statusInfo.cbSize = sizeof(statusInfo);
	statusInfo.fMask = MIIM_STRING | MIIM_STATE;
	statusInfo.fState = MFS_DISABLED | MFS_GRAYED;
	statusInfo.dwTypeData = status.data();
	::SetMenuItemInfoW(_nppThemesMenuHandle, IDM_NPPTHEMES_STATUS, FALSE, &statusInfo);

	::CheckMenuItem(_nppThemesMenuHandle, IDM_NPPTHEMES_DISABLE,
		MF_BYCOMMAND | (active ? MF_UNCHECKED : MF_CHECKED));
	const auto profiles = nppthemes::builtInProfiles();
	for (size_t index = 0; index < profiles.size() && index < 32; ++index)
	{
		const bool selected = active && active->id == profiles[index].id;
		::CheckMenuItem(_nppThemesMenuHandle, IDM_NPPTHEMES_PROFILE_FIRST + static_cast<UINT>(index),
			MF_BYCOMMAND | (selected ? MF_CHECKED : MF_UNCHECKED));
	}
}

void Notepad_plus::commandNppThemes(const int id)
{
	std::string error;
	bool succeeded = false;
	if (id == IDM_NPPTHEMES_DISABLE)
	{
		succeeded = NppThemesShell::themeRuntime().disable(error);
	}
	else
	{
		const auto profiles = nppthemes::builtInProfiles();
		const auto index = static_cast<size_t>(id - IDM_NPPTHEMES_PROFILE_FIRST);
		if (index < profiles.size())
			succeeded = NppThemesShell::themeRuntime().selectProfile(profiles[index], error);
		else
			error = "unknown built-in profile command";
	}

	if (!succeeded)
	{
		const auto message = string2wstring(error, CP_UTF8);
		::MessageBoxW(_pPublicInterface->getHSelf(), message.c_str(), L"NppThemes profile change failed",
			MB_OK | MB_ICONERROR);
		return;
	}

	updateNppThemesMenu();
	::DrawMenuBar(_pPublicInterface->getHSelf());
	::SendMessage(_pPublicInterface->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, TRUE, 0);
}
