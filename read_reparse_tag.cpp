#include <windows.h>
#include <atlbase.h>
#include <versionhelpers.h>
#include <map>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <cstdio>
#pragma comment(lib, "advapi32")

using REPARSE_TAG = decltype(FILE_ATTRIBUTE_TAG_INFO::ReparseTag);
#define TAG_MAP(x) { x, #x }

static const std::map<REPARSE_TAG, PCSTR> tag{ {
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
		TAG_MAP(IO_REPARSE_TAG_WCI_1),
		TAG_MAP(IO_REPARSE_TAG_GLOBAL_REPARSE),
		TAG_MAP(IO_REPARSE_TAG_CLOUD),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_1),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_2),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_3),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_4),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_5),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_6),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_7),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_8),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_9),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_A),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_B),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_C),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_D),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_E),
		TAG_MAP(IO_REPARSE_TAG_CLOUD_F),
		TAG_MAP(IO_REPARSE_TAG_APPEXECLINK),
		TAG_MAP(IO_REPARSE_TAG_PROJFS),
		TAG_MAP(IO_REPARSE_TAG_STORAGE_SYNC),
		TAG_MAP(IO_REPARSE_TAG_WCI_TOMBSTONE),
		TAG_MAP(IO_REPARSE_TAG_UNHANDLED),
		TAG_MAP(IO_REPARSE_TAG_ONEDRIVE),
		TAG_MAP(IO_REPARSE_TAG_PROJFS_TOMBSTONE),
		TAG_MAP(IO_REPARSE_TAG_AF_UNIX),
		{ 0x80000005, "IO_REPARSE_TAG_DRIVER_EXTENDER" },
		{ 0x8000000B, "IO_REPARSE_TAG_FILTER_MANAGER" },
		{ 0xA0000010, "IO_REPARSE_TAG_IIS_CACHE" },
		{ 0xC0000014, "IO_REPARSE_TAG_APPXSTRM" },
		{ 0x80000016, "IO_REPARSE_TAG_DFM" },
		{ 0xA000001D, "IO_REPARSE_TAG_LX_SYMLINK" },
		{ 0x80000024, "IO_REPARSE_TAG_LX_FIFO" },
		{ 0x80000025, "IO_REPARSE_TAG_LX_CHR" },
		{ 0x80000026, "IO_REPARSE_TAG_LX_BLK" },
		{ 0xA0000027, "IO_REPARSE_TAG_WCI_LINK" },
		{ 0xA0001027, "IO_REPARSE_TAG_WCI_LINK_1" },
		} };
PCSTR ReparseTagToString(_In_ REPARSE_TAG tab_value)
{
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
std::optional<REPARSE_TAG> ReadReparseTagByFindFile(_In_z_ PCWSTR filename)
{
	WIN32_FIND_DATAW find_data;
	HANDLE find = FindFirstFileExW(
		filename,
		IsWindows7OrGreater() ? FindExInfoBasic : FindExInfoStandard,
		&find_data,
		FindExSearchNameMatch,
		nullptr,
		FIND_FIRST_EX_CASE_SENSITIVE
	);
	if (find == INVALID_HANDLE_VALUE)
		throw std::runtime_error("FindFirstFileEx");
	FindClose(find);
	if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		return find_data.dwReserved0;
	else
		return std::nullopt;
}
std::optional<REPARSE_TAG> ReadReparseTagByHandle(_In_z_ PCWSTR filename)
{
	ATL::CHandle file(CreateFileW(
		filename,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
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
		return file_tag_info.ReparseTag;
	else
		return std::nullopt;
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