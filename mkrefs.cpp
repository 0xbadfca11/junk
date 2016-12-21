#define WIN32_LEAN_AND_MEAN
#define STRICT_GS_ENABLED
#include <atlbase.h>
#include <windows.h>
#include <pathcch.h>
#include <shlwapi.h>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <crtdbg.h>
#include <string>
#pragma comment(lib, "pathcch")

[[noreturn]]
void pwerror()
{
	PWSTR msg;
	ATLENSURE(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), 0, reinterpret_cast<PWSTR>(&msg), 0, nullptr));
	_putws(msg);
	_CrtDbgBreak();
	std::quick_exit(EXIT_FAILURE);
}
bool IF_REG(ULONG retval)
{
	SetLastError(retval);
	return retval == ERROR_SUCCESS;
}
int __cdecl main()
{
	ATLENSURE(SetEnvironmentVariableW(L"PATH", nullptr));
	ATLENSURE(SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32));
	WCHAR system32[MAX_PATH];
	ATLENSURE(GetSystemDirectoryW(system32, ARRAYSIZE(system32)));
	ATLENSURE(SetCurrentDirectoryW(system32));
	setlocale(LC_ALL, "");
	WCHAR format[MAX_PATH];
	ATLENSURE_SUCCEEDED(PathCchCombine(format, ARRAYSIZE(format), system32, L"format.com"));
	std::wstring format_args = std::wstring(L"\"") + format + L"\" /FS:ReFS " + PathGetArgsW(GetCommandLineW());
	HKEY MiniNT;
	if (!IF_REG(RegCreateKeyExW(HKEY_LOCAL_MACHINE, LR"(SYSTEM\CurrentControlSet\Control\MiniNT)", 0, nullptr, REG_OPTION_VOLATILE, KEY_READ, nullptr, &MiniNT, nullptr)))
	{
		pwerror();
	}
	std::at_quick_exit([]
	{
		if (!LoadLibraryExW(L"wpeutil", nullptr, LOAD_LIBRARY_AS_DATAFILE))
		{
			RegDeleteTreeW(HKEY_LOCAL_MACHINE, LR"(SYSTEM\CurrentControlSet\Control\MiniNT)");
		}
	});
	ULONG AllowRefsFormatOverNonmirrorVolume = 1;
	if (!IF_REG(RegSetKeyValueW(HKEY_LOCAL_MACHINE, LR"(SYSTEM\CurrentControlSet\Control\MiniNT)", L"AllowRefsFormatOverNonmirrorVolume", REG_DWORD, &AllowRefsFormatOverNonmirrorVolume, sizeof AllowRefsFormatOverNonmirrorVolume)))
	{
		pwerror();
	}
	ULONG RefsFormatVersion = 1;
	if (!IF_REG(RegSetKeyValueW(HKEY_LOCAL_MACHINE, LR"(SYSTEM\CurrentControlSet\Control\FileSystemUtilities)", L"RefsFormatVersion", REG_DWORD, &RefsFormatVersion, sizeof RefsFormatVersion)))
	{
		pwerror();
	}
	std::at_quick_exit([]
	{
		RegDeleteKeyValueW(HKEY_LOCAL_MACHINE, LR"(SYSTEM\CurrentControlSet\Control\FileSystemUtilities)", L"RefsFormatVersion");
	});
	STARTUPINFOW si = { sizeof si };
	PROCESS_INFORMATION pi;
	if (!CreateProcessW(format, &format_args[0], nullptr, nullptr, FALSE, 0, nullptr, system32, &si, &pi))
	{
		pwerror();
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	ULONG exit_code;
	GetExitCodeProcess(pi.hProcess, &exit_code);
	std::quick_exit(exit_code);
}