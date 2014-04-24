#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#pragma comment(lib, "shlwapi")

int __cdecl main()
{
	SYSTEM_INFO system_info;
	GetNativeSystemInfo( &system_info );
	char buffer[16];
	printf(
		"%p (%s)",
		system_info.lpMaximumApplicationAddress,
		StrFormatByteSize64A( reinterpret_cast<uintptr_t>( system_info.lpMaximumApplicationAddress ), buffer, ARRAYSIZE( buffer ) )
		);
}