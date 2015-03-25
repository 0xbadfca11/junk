#include <windows.h>
#include <winioctl.h>

int __cdecl wmain( int argc, PWSTR argv[] )
{
	HANDLE f = CreateFileW( argv[1], GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr );
	struct
	{
		WOF_EXTERNAL_INFO wof;
		FILE_PROVIDER_EXTERNAL_INFO provider;
	} compress{
		{ WOF_CURRENT_VERSION, WOF_PROVIDER_FILE },
		{ FILE_PROVIDER_CURRENT_VERSION, FILE_PROVIDER_COMPRESSION_XPRESS4K, 0 }
	};
	static_assert( sizeof compress.wof == offsetof( decltype( compress ), provider ), "" );
	ULONG dummy;
	return !DeviceIoControl( f, FSCTL_SET_EXTERNAL_BACKING, &compress, sizeof compress, nullptr, 0, &dummy, nullptr );
}