#include <windows.h>
#include <versionhelpers.h>
#include <winternl.h>
#include <cstdio>
#include <cstdlib>
#pragma comment(lib, "ntdll")

EXTERN_C NTSYSAPI NTSTATUS NTAPI RtlGetVersion(_Inout_ POSVERSIONINFOW);

int __cdecl wmain()
{
	OSVERSIONINFOW Os = { sizeof Os };
#pragma warning( suppress : 4996 )
	if (!GetVersionExW(&Os))
	{
		abort();
	}
	printf("GetVersionEx()  == %2lu,%2lu\n", Os.dwMajorVersion, Os.dwMinorVersion);
	if (NT_ERROR(RtlGetVersion(&Os)))
	{
		abort();
	}
	printf("RtlGetVersion() == %2lu,%2lu\n", Os.dwMajorVersion, Os.dwMinorVersion);
	printf(
		"IsWindowsVersionOrGreater( %lu, %lu, 0 ) == %s",
		Os.dwMajorVersion,
		Os.dwMinorVersion,
#pragma warning( suppress : 4244 )
		IsWindowsVersionOrGreater(Os.dwMajorVersion, Os.dwMinorVersion, 0)
		? "true"
		: "false"
	);
}