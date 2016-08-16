#include <windows.h>
#include <atlbase.h>
#include <versionhelpers.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <cstdio>
#pragma comment(lib, "advapi32")

typedef decltype(FILE_ATTRIBUTE_TAG_INFO::ReparseTag) REPARSE_TAG;
#define TAG_MAP(x) { x, #x }

PCSTR ReparseTagToString(REPARSE_TAG tab_value)
{
	static const std::map<REPARSE_TAG, PCSTR> tag{ {
			TAG_MAP(IO_REPARSE_TAG_RESERVED_ZERO),
			TAG_MAP(IO_REPARSE_TAG_RESERVED_ONE),
			TAG_MAP(IO_REPARSE_TAG_RESERVED_TWO),
			TAG_MAP(IO_REPARSE_TAG_MOUNT_POINT),
			TAG_MAP(IO_REPARSE_TAG_HSM),
			TAG_MAP(IO_REPARSE_TAG_HSM2),
			TAG_MAP(IO_REPARSE_TAG_SIS),
			TAG_MAP(IO_REPARSE_TAG_WIM),
			TAG_MAP(IO_REPARSE_TAG_CSV),
			TAG_MAP(IO_REPARSE_TAG_DFS),
			TAG_MAP(IO_REPARSE_TAG_SYMLINK),
			TAG_MAP(IO_REPARSE_TAG_DFSR),
			TAG_MAP(IO_REPARSE_TAG_DEDUP),
			TAG_MAP(IO_REPARSE_TAG_NFS),
			TAG_MAP(IO_REPARSE_TAG_FILE_PLACEHOLDER),
			TAG_MAP(IO_REPARSE_TAG_WOF),
			TAG_MAP(IO_REPARSE_TAG_WCI),
			TAG_MAP(IO_REPARSE_TAG_GLOBAL_REPARSE),
			TAG_MAP(IO_REPARSE_TAG_CLOUD),
			TAG_MAP(IO_REPARSE_TAG_APPEXECLINK),
			TAG_MAP(IO_REPARSE_TAG_GVFS),
			{ 0x80000005, "IO_REPARSE_TAG_DRIVER_EXTENDER" },
			{ 0x8000000B, "IO_REPARSE_TAG_FILTER_MANAGER" },
			{ 0xA0000010, "IO_REPARSE_TAG_IIS_CACHE" },
			{ 0xC0000014, "IO_REPARSE_TAG_APPXSTRM" },
			{ 0x80000016, "IO_REPARSE_TAG_DFM" },
			{ 0xA000001D, "IO_REPARSE_TAG_LX_SYMLINK" },
			} };
	auto it = tag.find(tab_value);
	if (it != tag.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}
std::unique_ptr<REPARSE_TAG> ReadReparseTagByFindFile(_In_z_ PCWSTR filename)
{
	WIN32_FIND_DATAW find_data;
	HANDLE find = FindFirstFileExW(
		filename,
		IsWindows7OrGreater() ? FindExInfoBasic : FindExInfoStandard,
		&find_data,
		FindExSearchNameMatch,
		nullptr,
		0
	);
	if (find == INVALID_HANDLE_VALUE)
		throw std::runtime_error("FindFirstFileEx");
	FindClose(find);
	if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		return std::make_unique<REPARSE_TAG>(find_data.dwReserved0);
	else
		return nullptr;
}
std::unique_ptr<REPARSE_TAG> ReadReparseTagByHandle(_In_z_ PCWSTR filename)
{
	ATL::CHandle file(CreateFileW(
		filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
		nullptr
	));
	if (file == INVALID_HANDLE_VALUE)
	{
		file.Detach();
		throw std::runtime_error("CreateFile");
	}
	FILE_ATTRIBUTE_TAG_INFO file_tag_info;
	if (!GetFileInformationByHandleEx(file, FileAttributeTagInfo, &file_tag_info, sizeof file_tag_info))
		throw std::runtime_error("GetFileInformationByHandleEx");
	if (file_tag_info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		return std::make_unique<REPARSE_TAG>(file_tag_info.ReparseTag);
	else
		return nullptr;
}

int __cdecl wmain(int argc, PWSTR argv[])
{
	if (argc < 2)
		return EXIT_FAILURE;

	HANDLE process_token;
	TOKEN_PRIVILEGES token_privilege;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &process_token);
	LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &token_privilege.Privileges[0].Luid);
	token_privilege.PrivilegeCount = 1;
	token_privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(process_token, FALSE, &token_privilege, sizeof token_privilege, NULL, NULL);
	CloseHandle(process_token);

	for (int i = 1; i < argc; ++i)
	{
		_putws(argv[i]);
		fputs("\tGetFileInformationByHandleEx: ", stdout);
		try
		{
			if (auto reparse_tag = ReadReparseTagByHandle(argv[i]))
			{
				if (auto tag_name = ReparseTagToString(*reparse_tag))
					printf("%s(%lx)\n", tag_name, *reparse_tag);
				else
					printf("Unknown Tag(%lx)\n", *reparse_tag);
			}
			else
				puts("is not Reparse Point");
		}
		catch (const std::runtime_error& e)
		{
			fputs(e.what(), stderr);
			fputs(" failed\n", stderr);
		}
		fputs("\tFindFirstFileEx: ", stdout);
		try
		{
			if (auto reparse_tag = ReadReparseTagByFindFile(argv[i]))
			{
				if (auto tag_name = ReparseTagToString(*reparse_tag))
					printf("%s(%lx)\n", tag_name, *reparse_tag);
				else
					printf("Unknown Tag(%lx)\n", *reparse_tag);
			}
			else
				puts("is not Reparse Point");
		}
		catch (const std::runtime_error& e)
		{
			fputs(e.what(), stderr);
			fputs(" failed\n", stderr);
		}

	}
}