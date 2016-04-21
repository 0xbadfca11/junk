#include <windows.h>
#include <stdio.h>
#include <process.h>

const WCHAR Dll[] = L"dll";

int __cdecl main()
{
	HMODULE dll = LoadLibraryW(Dll);

	const int count = 2;
	for (int i = 0; i < count; ++i)
	{
		CloseHandle(CreateThread(nullptr, 0, [](PVOID)->ULONG { Sleep(INFINITE); return 0; }, nullptr, 0, nullptr));
		Sleep(1);
	}
	for (int i = 0; i < count; ++i)
	{
		CloseHandle(CreateThread(nullptr, 0, [](PVOID)->ULONG { return 0; }, nullptr, 0, nullptr));
		Sleep(1);
	}
	Sleep(1);
	FreeLibrary(dll);
}