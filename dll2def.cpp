#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dbghelp.h>
#include <shlwapi.h>
#include <cstdio>
#include <cstring>
#include <memory>
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "shlwapi")

int __cdecl wmain(int argc, PWSTR argv[])
{
	while (--argc)
	{
		HANDLE h = CreateFileW(argv[argc], FILE_GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			continue;
		HANDLE m = CreateFileMappingW(h, nullptr, PAGE_READONLY | SEC_IMAGE_NO_EXECUTE, 0, 0, nullptr);
		CloseHandle(h);
		if (!m)
			continue;
		std::shared_ptr<char> base_address(reinterpret_cast<char*>(MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0)), UnmapViewOfFile);
		CloseHandle(m);
		if (!base_address)
			continue;
		ULONG Size;
		auto export_directory = static_cast<PIMAGE_EXPORT_DIRECTORY>(ImageDirectoryEntryToDataEx(base_address.get(), TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &Size, NULL));
		if (!export_directory)
			continue;
		if (!export_directory->AddressOfNames)
			continue;
		auto AddressOfNames = reinterpret_cast<PULONG>(base_address.get() + export_directory->AddressOfNames);
		auto libfilename = PathFindFileNameW(argv[argc]);
		WCHAR deffilename[MAX_PATH];
		if (wcscpy_s(deffilename, libfilename))
			continue;
		PathRenameExtensionW(deffilename, L".def");
		auto deffile = _wfopen(deffilename, L"wt");
		if (!deffile)
			continue;
		fputs("EXPORTS\n", deffile);
		for (decltype(IMAGE_EXPORT_DIRECTORY::NumberOfNames) i = 0; i < export_directory->NumberOfNames; ++i)
			fprintf(deffile, "\t%s\n", base_address.get() + AddressOfNames[i]);
		fclose(deffile);
	}
}