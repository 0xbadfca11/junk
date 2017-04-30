#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <clocale>
#include <cstdio>
#include <memory>
#include <crtdbg.h>

void PrintfWindowsError(ULONG error_code = GetLastError())
{
	thread_local auto msg = std::make_unique<WCHAR[]>(USHRT_MAX);
	if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code, 0, msg.get(), USHRT_MAX, nullptr))
	{
		fprintf(stderr, "%lx\n", error_code);
	}
	fprintf(stderr, "%ls\n", msg.get());
}
int __cdecl wmain(int argc, PWSTR argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	setlocale(LC_ALL, "");

	WCHAR mount_point[MAX_PATH];
	WCHAR guid_path[50];
	for (int i = 1; i < argc; ++i)
	{
		_RPT2(_CRT_WARN, "argv[%d] == %ls\n", i, argv[i]);

		if (!GetVolumePathNameW(argv[i], mount_point, ARRAYSIZE(mount_point)))
		{
			PrintfWindowsError();
			continue;
		}
		_RPT1(_CRT_WARN, "GetVolumePathName() == %ls\n", mount_point);

		if (!GetVolumeNameForVolumeMountPointW(mount_point, guid_path, ARRAYSIZE(guid_path)))
		{
			PrintfWindowsError();
			continue;
		}
		*wcsrchr(guid_path, L'\\') = L'\0';
		_RPT1(_CRT_WARN, "GetVolumeNameForVolumeMountPoint() == %ls\n", guid_path);

		auto f = CreateFileW(guid_path, FILE_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (f == INVALID_HANDLE_VALUE)
		{
			PrintfWindowsError();
			continue;
		}
		ULONG dummy;
		REFS_VOLUME_DATA_BUFFER refs_buffer;
		struct
		{
			NTFS_VOLUME_DATA_BUFFER basic;
			NTFS_EXTENDED_VOLUME_DATA extended;
		} ntfs_buffer;
		static_assert(offsetof(decltype(ntfs_buffer), extended) == sizeof(NTFS_VOLUME_DATA_BUFFER), "padding");
		FILE_QUERY_ON_DISK_VOL_INFO_BUFFER udf_buffer;
		WCHAR fs_name_buffer[MAX_PATH + 1];
		if (DeviceIoControl(f, FSCTL_GET_REFS_VOLUME_DATA, nullptr, 0, &refs_buffer, sizeof refs_buffer, &dummy, nullptr))
		{
			printf("%ls == ReFS %lu.%lu\n", mount_point, refs_buffer.MajorVersion, refs_buffer.MinorVersion);
			continue;
		}
		else if (DeviceIoControl(f, FSCTL_GET_NTFS_VOLUME_DATA, nullptr, 0, &ntfs_buffer, sizeof ntfs_buffer, &dummy, nullptr))
		{
			if (dummy >= (RTL_SIZEOF_THROUGH_FIELD(decltype(ntfs_buffer), extended.MinorVersion)))
			{
				printf("%ls == NTFS %u.%u\n", mount_point, ntfs_buffer.extended.MajorVersion, ntfs_buffer.extended.MinorVersion);
			}
			else
			{
				printf("%ls == NTFS unknown version\n", mount_point);
			}
			continue;
		}
		else if (DeviceIoControl(f, FSCTL_QUERY_ON_DISK_VOLUME_INFO, nullptr, 0, &udf_buffer, sizeof udf_buffer, &dummy, nullptr))
		{
			printf("%ls == %ls %u.%02u\n", mount_point, udf_buffer.FsFormatName, udf_buffer.FsFormatMajVersion, udf_buffer.FsFormatMinVersion);
			continue;
		}
		else if (GetVolumeInformationByHandleW(f, nullptr, 0, nullptr, nullptr, nullptr, fs_name_buffer, ARRAYSIZE(fs_name_buffer)))
		{
			printf("%ls == %ls\n", mount_point, fs_name_buffer);
			continue;
		}
		PrintfWindowsError();
	}
}