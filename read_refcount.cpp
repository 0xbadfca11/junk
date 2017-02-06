#define WIN32_LEAN_AND_MEAN
#define STRICT
#define STRICT_GS_ENABLED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#include <atlbase.h>
#include <windows.h>
#include <winioctl.h>
#include <clocale>
#include <cstdio>
#include <crtdbg.h>

#ifndef FSCTL_GET_RETRIEVAL_POINTERS_AND_REFCOUNT
const ULONG FSCTL_GET_RETRIEVAL_POINTERS_AND_REFCOUNT = CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 244, METHOD_NEITHER, FILE_ANY_ACCESS);
typedef struct RETRIEVAL_POINTERS_AND_REFCOUNT_BUFFER {

	ULONG ExtentCount;
	LARGE_INTEGER StartingVcn;
	struct {
		LARGE_INTEGER NextVcn;
		LARGE_INTEGER Lcn;
		ULONG ReferenceCount;
	} Extents[ANYSIZE_ARRAY];

} RETRIEVAL_POINTERS_AND_REFCOUNT_BUFFER, *PRETRIEVAL_POINTERS_AND_REFCOUNT_BUFFER;
#endif

void pwerror()
{
	PWSTR msg;
	ATLENSURE(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), 0, reinterpret_cast<PWSTR>(&msg), 0, nullptr));
	fputws(msg, stderr);
	_CrtDbgBreak();
	LocalFree(msg);
}
int __cdecl wmain(int argc, PWSTR argv[])
{
	setlocale(LC_ALL, "");
	for (int i = 1; i < argc; i++)
	{
		_putws(argv[i]);
		ATL::CHandle h(CreateFileW(argv[i], 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
		if (h == INVALID_HANDLE_VALUE)
		{
			pwerror();
			h.Detach();
			continue;
		}
		ULONG fs_flags;
		ATLENSURE(GetVolumeInformationByHandleW(h, nullptr, 0, nullptr, nullptr, &fs_flags, nullptr, 0));
		if (!(fs_flags & FILE_SUPPORTS_BLOCK_REFCOUNTING))
		{
			SetLastError(ERROR_NOT_CAPABLE);
			pwerror();
			continue;
		}
		puts("                 VCN           Contiguous   RefCount                  LCN");
		STARTING_VCN_INPUT_BUFFER Vcn = {};
		union
		{
			RETRIEVAL_POINTERS_AND_REFCOUNT_BUFFER refcount_buf;
			BYTE buf[offsetof(RETRIEVAL_POINTERS_AND_REFCOUNT_BUFFER, Extents) + sizeof(RETRIEVAL_POINTERS_AND_REFCOUNT_BUFFER::Extents) * 128];
		};
		ULONG junk;
		for (;;)
		{
			BOOL ret = DeviceIoControl(h, FSCTL_GET_RETRIEVAL_POINTERS_AND_REFCOUNT, &Vcn, sizeof Vcn, &refcount_buf, sizeof buf, &junk, nullptr);
			ULONG err = GetLastError();
			if (ret || err == ERROR_MORE_DATA)
			{
				printf(
					"% 20llu % 20llu % 10lu % 20llu\n",
					refcount_buf.StartingVcn.QuadPart,
					refcount_buf.Extents[0].NextVcn.QuadPart - refcount_buf.StartingVcn.QuadPart,
					refcount_buf.Extents[0].ReferenceCount,
					refcount_buf.Extents[0].Lcn.QuadPart
				);
				if (refcount_buf.ExtentCount > 1)
				{
					for (ULONG j = 1; j < refcount_buf.ExtentCount; j++)
					{
						printf(
							"% 20llu % 20llu % 10lu % 20llu\n",
							refcount_buf.Extents[j - 1].NextVcn.QuadPart,
							refcount_buf.Extents[j].NextVcn.QuadPart - refcount_buf.Extents[j - 1].NextVcn.QuadPart,
							refcount_buf.Extents[j].ReferenceCount,
							refcount_buf.Extents[j].Lcn.QuadPart
						);
					}
				}
				Vcn.StartingVcn = refcount_buf.Extents[refcount_buf.ExtentCount - 1].NextVcn;
			}
			else
			{
				if (err != ERROR_HANDLE_EOF)
				{
					pwerror();
				}
				break;
			}
		}
	}
}