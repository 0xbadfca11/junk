#include <atlstr.h>
#include <windows.h>
#include <shlwapi.h>
#include <cstdio>
#include <cstdlib>
#include <crtdbg.h>

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
	ATL::CStringW format(system32);
	format += L"\\format.com";
	ATL::CStringW format_args(format);
	format_args += L" /FS:ReFS ";
	format_args += PathGetArgsW(GetCommandLineW());
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
	if (!CreateProcessW(format, format_args.GetBuffer(), nullptr, nullptr, FALSE, 0, nullptr, system32, &si, &pi))
	{
		pwerror();
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	ULONG exit_code;
	GetExitCodeProcess(pi.hProcess, &exit_code);
	std::quick_exit(exit_code);
}