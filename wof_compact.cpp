#include <windows.h>
#include <winioctl.h>
#include <stddef.h>

struct WOF
{
	WOF_EXTERNAL_INFO wof_header = { WOF_CURRENT_VERSION, WOF_PROVIDER_FILE };
	FILE_PROVIDER_EXTERNAL_INFO provider = { FILE_PROVIDER_CURRENT_VERSION, FILE_PROVIDER_COMPRESSION_XPRESS4K };
};
static_assert( sizeof WOF::wof_header == offsetof( WOF, provider ), "" );

int __cdecl wmain( int, PWSTR argv[] )
{
	HANDLE f = CreateFileW( argv[1], GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr );
	WOF compress;
	ULONG dummy;
	return !DeviceIoControl( f, FSCTL_SET_EXTERNAL_BACKING, &compress, sizeof compress, nullptr, 0, &dummy, nullptr );
}