#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dbghelp.h>
#include <shlwapi.h>
#include <cstdio>
#include <cstring>
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "shlwapi")

int __cdecl wmain( int argc, PWSTR argv[] )
{
	while( --argc )
	{
		auto base_address = reinterpret_cast<PCHAR>( LoadLibraryExW( argv[argc], nullptr, LOAD_LIBRARY_AS_DATAFILE ) );
		if( !base_address )
			continue;
		ULONG Size;
		auto export_directory = static_cast<PIMAGE_EXPORT_DIRECTORY>( ImageDirectoryEntryToDataEx( base_address, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &Size, NULL ) );
		if( !export_directory )
			continue;
		auto AddressOfNames = reinterpret_cast<PULONG>( base_address + export_directory->AddressOfNames );
		if( !AddressOfNames )
			continue;
		auto libfilename = PathFindFileNameW( argv[argc] );
		WCHAR deffilename[MAX_PATH];
		if( wcscpy_s( deffilename, libfilename ) )
			continue;
		PathRenameExtensionW( deffilename, L".def" );
		auto deffile = _wfopen( deffilename, L"wt" );
		if( !deffile )
			continue;
		fputs( "EXPORTS\n", deffile );
		for( decltype( IMAGE_EXPORT_DIRECTORY::NumberOfNames ) i = 0; i < export_directory->NumberOfNames; ++i )
			fprintf( deffile, "\t%s\n", base_address + AddressOfNames[i] );
		fclose( deffile );
	}
}