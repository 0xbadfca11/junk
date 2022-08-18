#include <windows.h>
#include <winternl.h>
#pragma comment(lib, "ntdll")

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCopyFileChunk(
	_In_ HANDLE SourceHandle,
	_In_ HANDLE DestHandle,
	_In_opt_ HANDLE Event,
	_Out_ PIO_STATUS_BLOCK IoStatusBlock,
	_In_ ULONG Length,
	_In_ PLARGE_INTEGER SourceOffset,
	_In_ PLARGE_INTEGER DestOffset,
	_In_opt_ PULONG SourceKey,
	_In_opt_ PULONG DestKey,
	_In_ ULONG Flags
);

// Bug 1: NtCopyFileChunk() does update file pointer of file handle. Linux's does not when off_in/out is not NULL.
// Bug 2: Does not handle STATUS_PENDING.
INT64 copy_file_range(
	HANDLE fd_in,
	INT64* off_in,
	HANDLE fd_out,
	INT64* off_out,
	ULONG len,
	ULONG flags
)
{
	LARGE_INTEGER src_offset, dst_offset;
	if (off_in)
		src_offset.QuadPart = *off_in;
	else
		if (!SetFilePointerEx(fd_in, { .QuadPart = 0 }, &src_offset, FILE_CURRENT))
			return -1;
	if (off_out)
		dst_offset.QuadPart = *off_out;
	else
		if (!SetFilePointerEx(fd_out, { .QuadPart = 0 }, &dst_offset, FILE_CURRENT))
			return -1;

	IO_STATUS_BLOCK iob;
	NTSTATUS status = NtCopyFileChunk(fd_in, fd_out, nullptr, &iob, len, &src_offset, &dst_offset, nullptr, nullptr, flags);
	if (NT_ERROR(status))
	{
		SetLastError(RtlNtStatusToDosError(status));
		return -1;
	}
	if (off_in)
		*off_in += len;
	if (off_out)
		*off_out += len;
	return iob.Information;
}
