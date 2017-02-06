#define WIN32_LEAN_AND_MEAN
#define STRICT
#define STRICT_GS_ENABLED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _CRTDBG_MAP_ALLOC
#include <atlalloc.h>
#include <atlbase.h>
#include <windows.h>
#include <bcrypt.h>
#include <pathcch.h>
#include <shlwapi.h>
#include <wincrypt.h>
#include <winioctl.h>
#include <algorithm>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <crtdbg.h>

namespace filesystem = std::experimental::filesystem;

struct Piece
{
	HANDLE file;
	ULONG64 offset;
	Piece(HANDLE file, ULONG64 offset) noexcept
		: file(file)
		, offset(offset)
	{}
};
struct Dedupe
{
	std::once_flag initialized;
	const ULONG BytesPerCluster = 0;
	std::unordered_map<std::string, Piece> dedup_table;
	std::deque<ATL::CHandle> handles;
	void RetrieveAllocationUnitSize(HANDLE file)
	{
		if (BytesPerCluster != 0)
		{
			throw std::logic_error("Twice called");
		}
		REFS_VOLUME_DATA_BUFFER buf;
		ULONG dummy;
		if (!DeviceIoControl(file, FSCTL_GET_REFS_VOLUME_DATA, nullptr, 0, &buf, sizeof buf, &dummy, nullptr))
		{
			throw std::invalid_argument("Not local ReFS volume");
		}
		ULONG fs_flags;
		ATLENSURE(GetVolumeInformationByHandleW(file, nullptr, 0, nullptr, nullptr, &fs_flags, nullptr, 0));
		if (!(fs_flags & FILE_SUPPORTS_BLOCK_REFCOUNTING))
		{
			throw std::invalid_argument("Not local ReFS v2 volume");
		}
		const_cast<ULONG&>(BytesPerCluster) = buf.BytesPerCluster;
	}
	std::string Bin2Hex(BYTE data[], ULONG length)
	{
		ULONG stringize_size = length * 2 + 1;
		ATL::CTempBuffer<CHAR> stringize(stringize_size);
		ATLENSURE(CryptBinaryToStringA(data, length, CRYPT_STRING_HEXRAW | CRYPT_STRING_NOCRLF, stringize, &stringize_size));
		return{ stringize, stringize_size };
	}
	std::string GetHash(BYTE data[], ULONG length)
	{
		const size_t sha512_length = 64;
		BYTE hash[sha512_length];
		ATLENSURE(BCRYPT_SUCCESS(BCryptHash(BCRYPT_SHA512_ALG_HANDLE, nullptr, 0, data, length, hash, sha512_length)));
		return Bin2Hex(hash, sha512_length);
	}
	void ProcessFile(const filesystem::path& path)
	{
		HANDLE file = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			return;
		}
		handles.emplace_back(file);
		std::call_once(initialized, &Dedupe::RetrieveAllocationUnitSize, this, file);

		BY_HANDLE_FILE_INFORMATION file_info;
		if (!GetFileInformationByHandle(file, &file_info))
		{
			return;
		}
		ULONG64 file_size = file_info.nFileSizeHigh * 1ULL << 32 | file_info.nFileSizeLow;
		if (file_size < BytesPerCluster)
		{
			return;
		}

		struct vm_free
		{
			void operator()(void* p) noexcept
			{
				VirtualFree(p, 0, MEM_RELEASE);
			}
		};
		std::unique_ptr<void, vm_free> mm(VirtualAlloc(nullptr, BytesPerCluster, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		PBYTE p = static_cast<PBYTE>(mm.get());
		ATLENSURE(p);
		for (ULONG64 i = 0, n = file_size / BytesPerCluster * BytesPerCluster; i < n && i <= MAXLONG64; i += BytesPerCluster)
		{
			ULONG read;
			if (!ReadFile(file, p, BytesPerCluster, &read, nullptr) || read != BytesPerCluster)
			{
				return;
			}
			auto key = GetHash(p, BytesPerCluster);
			key += Bin2Hex(p, 16);
			key += (file_info.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM) ? '1' : '0';
			key += (file_info.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) ? '1' : '0';
			auto it = dedup_table.find(key);
			if (it == dedup_table.end())
			{
				dedup_table.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(file, i));
			}
			else
			{
				DUPLICATE_EXTENTS_DATA dup_extent;
				dup_extent.FileHandle = it->second.file;
				dup_extent.SourceFileOffset.QuadPart = it->second.offset;
				dup_extent.TargetFileOffset.QuadPart = i;
				dup_extent.ByteCount.QuadPart = BytesPerCluster;
				ULONG dummy;
				if (!DeviceIoControl(file, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &dup_extent, sizeof dup_extent, nullptr, 0, &dummy, nullptr))
				{
					dedup_table.erase(it);
					dedup_table.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(file, i));
				}
			}
		}
	}
	template <typename T>
	bool PickNextEntry(T& p) noexcept
	{
		_ASSERT(p);
		if (p->NextEntryOffset)
		{
			p = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(p) + p->NextEntryOffset);
			return true;
		}
		else
		{
			return false;
		}
	}
	void ProcessDirectory(const filesystem::path& path)
	{
		for (auto dentries = filesystem::recursive_directory_iterator(path); dentries != filesystem::recursive_directory_iterator(); ++dentries)
		{
			ULONG attr = GetFileAttributesW(dentries->path().c_str());
			if (attr == INVALID_FILE_ATTRIBUTES || (attr & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_REPARSE_POINT)))
			{
				fprintf(stderr, "[SKIP]%ls\n", dentries->path().c_str());
				dentries.disable_recursion_pending();
				continue;
			}
			else if (filesystem::is_regular_file(dentries->path()))
			{
				ProcessFile(dentries->path());
			}
		}
	}
	void ProcessArgv(int argc, PCWSTR argv[])
	{
		for (int i = 1; i < argc; i++)
		{
			filesystem::path path = filesystem::system_complete(argv[i]);
			ULONG attr = GetFileAttributesW(path.c_str());
			if (attr == INVALID_FILE_ATTRIBUTES)
			{
				fprintf(stderr, "[SKIP]%ls\n", path.c_str());
				return;
			}
			else if (attr & FILE_ATTRIBUTE_REPARSE_POINT)
			{
				fprintf(stderr, "[SKIP]%ls\n", path.c_str());
				return;
			}
			else if (attr & FILE_ATTRIBUTE_DIRECTORY)
			{
				ProcessDirectory(path);
			}
			else
			{
				ProcessFile(path);
			}
		}
	}
};
int __cdecl wmain(int argc, PCWSTR argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	setlocale(LC_ALL, "");

	try
	{
		Dedupe().ProcessArgv(argc, argv);
	}
	catch (std::exception& e)
	{
		fprintf(stderr, "%s\n", e.what());
	}

	std::quick_exit(EXIT_SUCCESS);
}