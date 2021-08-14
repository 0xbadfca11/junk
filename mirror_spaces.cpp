#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atlbase.h>
#include <pathcch.h>
#include <winioctl.h>
#include <algorithm>
#include <memory>
#include <cstdlib>
#pragma comment(lib, "pathcch")

struct StorageResiliency
{
	ATL::CHandle file_handle;
	using CopiesType = decltype(STORAGE_DEVICE_RESILIENCY_DESCRIPTOR::NumberOfPhysicalCopies);
	CopiesType CopiesCount = 0;
	StorageResiliency(
		PCWSTR lpFileName,
		ULONG dwDesiredAccess,
		ULONG dwShareMode,
		ULONG dwCreationDisposition,
		ULONG dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr,
		HANDLE hTemplateFile = nullptr
	)
	{
		file_handle.Attach(Create(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, dwFlagsAndAttributes | FILE_FLAG_NO_BUFFERING, lpSecurityAttributes, hTemplateFile));
		ATLENSURE(file_handle);
		auto path = std::make_unique<WCHAR[]>(PATHCCH_MAX_CCH);
		ATLENSURE(GetVolumePathNameW(lpFileName, path.get(), PATHCCH_MAX_CCH));
		WCHAR volume_GUID[50];
		ATLENSURE(GetVolumeNameForVolumeMountPointW(path.get(), volume_GUID, ARRAYSIZE(volume_GUID)));
		*wcsrchr(volume_GUID, L'\\') = L'\0';
		ATL::CHandle volume_handle;
		volume_handle.Attach(Create(volume_GUID, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, nullptr, nullptr));
		ATLENSURE(volume_handle);
		STORAGE_PROPERTY_QUERY storage_query = { StorageDeviceResiliencyProperty, PropertyStandardQuery };
		STORAGE_DEVICE_RESILIENCY_DESCRIPTOR storage_resiliency;
		ULONG junk;
		if (DeviceIoControl(volume_handle, IOCTL_STORAGE_QUERY_PROPERTY, &storage_query, sizeof storage_query, &storage_resiliency, sizeof storage_resiliency, &junk, nullptr))
			CopiesCount = storage_resiliency.NumberOfPhysicalCopies;
	}
	static HANDLE Create(
		PCWSTR lpFileName,
		ULONG dwDesiredAccess,
		ULONG dwShareMode,
		ULONG dwCreationDisposition,
		ULONG dwFlagsAndAttributes,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		HANDLE hTemplateFile
	)
	{
		HANDLE h = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		if (h == INVALID_HANDLE_VALUE)
			return nullptr;
		else
			return h;
	}
	operator HANDLE() const noexcept
	{
		return file_handle.m_h;
	}
	bool SwitchCopy(ULONG copy) const noexcept
	{
		MARK_HANDLE_INFO handle_info = { copy, nullptr, MARK_HANDLE_READ_COPY };
		ULONG junk;
		return DeviceIoControl(file_handle, FSCTL_MARK_HANDLE, &handle_info, sizeof handle_info, nullptr, 0, &junk, nullptr) != 0;
	}
	bool ResetCopy() const noexcept
	{
		MARK_HANDLE_INFO handle_info = { 0, nullptr, MARK_HANDLE_NOT_READ_COPY };
		ULONG junk;
		return DeviceIoControl(file_handle, FSCTL_MARK_HANDLE, &handle_info, sizeof handle_info, nullptr, 0, &junk, nullptr) != 0;
	}
	bool Repair(const LARGE_INTEGER& FileOffset, ULONG Length, ULONG SourceCopy, ULONG RepairCopies_0) const noexcept
	{
		REPAIR_COPIES_INPUT input = { sizeof(REPAIR_COPIES_INPUT), 0, FileOffset, Length, SourceCopy, 1, RepairCopies_0 };
		REPAIR_COPIES_OUTPUT output;
		ULONG junk;
		return DeviceIoControl(file_handle, FSCTL_REPAIR_COPIES, &input, sizeof input, &output, sizeof output, &junk, nullptr) != 0;
	}
};

#include <bcrypt.h>
#include <wincrypt.h>
#include <winternl.h>
#include <clocale>
#include <cstdio>
#pragma comment(lib, "bcrypt")
#pragma comment(lib, "crypt32")

int wmain(int argc, PWSTR argv[])
{
	if (argc < 2)
		std::quick_exit(EXIT_FAILURE);
	setlocale(LC_ALL, "");

	const StorageResiliency stor_res{ argv[1], FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_DATA, FILE_SHARE_READ, OPEN_EXISTING };
	LARGE_INTEGER fsize;
	ATLENSURE(GetFileSizeEx(stor_res, &fsize));
	ULONG msize = (ULONG)std::min<ULONG64>(fsize.QuadPart, 128 * 1024 * 1024) / 4096 * 4096;
	if (!msize)
		std::quick_exit(EXIT_FAILURE);
	PVOID buf = VirtualAlloc(nullptr, msize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	ATLENSURE(buf);
	ULONG result;
	union {
		ULONG hash_size;
		BYTE hash_size_bytes[ANYSIZE_ARRAY];
	};
	const BCRYPT_ALG_HANDLE Algorithm = BCRYPT_SHA384_ALG_HANDLE;
	ATLENSURE(NT_SUCCESS(BCryptGetProperty(Algorithm, BCRYPT_HASH_LENGTH, hash_size_bytes, sizeof hash_size, &result, 0)));
	for (ULONG i = 0, end = stor_res.CopiesCount; i < end; i++)
	{
		ATLENSURE(stor_res.SwitchCopy(i));
		ULONG read;
		OVERLAPPED o = {};
		ATLENSURE(ReadFile(stor_res, buf, msize, &read, &o));
		ATL::CTempBuffer<BYTE> hash{ hash_size };
		ATLENSURE(NT_SUCCESS(BCryptHash(Algorithm, nullptr, 0, (PBYTE)buf, msize, hash, hash_size)));
		ULONG hash_stringize_size = hash_size * 2 + 1;
		ATL::CTempBuffer<char> hash_stringize{ hash_stringize_size };
		ATLENSURE(CryptBinaryToStringA(hash, hash_size, CRYPT_STRING_HEXRAW | CRYPT_STRING_NOCRLF, hash_stringize, &hash_stringize_size));
		puts(hash_stringize);
	}
	ATLENSURE(stor_res.CopiesCount == 0 || stor_res.ResetCopy());
	std::quick_exit(EXIT_SUCCESS);
}
