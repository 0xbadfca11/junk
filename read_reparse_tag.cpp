#include <windows.h>
#include <VersionHelpers.h>
#include <stdexcept>
#include <map>
#include <cstdio>
#include <boost/optional.hpp>
#include <boost/scope_exit.hpp>
#pragma comment(lib, "advapi32")

typedef decltype( FILE_ATTRIBUTE_TAG_INFO::ReparseTag ) REPARSE_TAG;

PCSTR ReparseTagToString( REPARSE_TAG tab_value )
{
	static const std::map<REPARSE_TAG, PCSTR> tag{ {
			{ IO_REPARSE_TAG_MOUNT_POINT, "IO_REPARSE_TAG_MOUNT_POINT" },
			{ IO_REPARSE_TAG_HSM, "IO_REPARSE_TAG_HSM" },
			{ IO_REPARSE_TAG_HSM2, "IO_REPARSE_TAG_HSM2" },
			{ IO_REPARSE_TAG_SIS, "IO_REPARSE_TAG_SIS" },
			{ IO_REPARSE_TAG_WIM, "IO_REPARSE_TAG_WIM" },
			{ IO_REPARSE_TAG_CSV, "IO_REPARSE_TAG_CSV" },
			{ IO_REPARSE_TAG_DFS, "IO_REPARSE_TAG_DFS" },
			{ IO_REPARSE_TAG_SYMLINK, "IO_REPARSE_TAG_SYMLINK" },
			{ IO_REPARSE_TAG_DFSR, "IO_REPARSE_TAG_DFSR" },
			{ IO_REPARSE_TAG_DEDUP, "IO_REPARSE_TAG_DEDUP" },
			{ IO_REPARSE_TAG_NFS, "IO_REPARSE_TAG_NFS" },
			{ IO_REPARSE_TAG_FILE_PLACEHOLDER, "IO_REPARSE_TAG_FILE_PLACEHOLDER" },
			{ IO_REPARSE_TAG_WOF, "IO_REPARSE_TAG_WOF" },
			} };
	try
	{
		return tag.at( tab_value );
	}
	catch( std::out_of_range& )
	{
		return nullptr;
	}
}
boost::optional<REPARSE_TAG> ReadReparseTagByFindFile( _In_z_ PCWSTR filename )
{
	WIN32_FIND_DATA find_data;
	HANDLE find = FindFirstFileExW(
		filename,
		IsWindows7OrGreater() ? FindExInfoBasic : FindExInfoStandard,
		&find_data,
		FindExSearchNameMatch,
		nullptr,
		0
		);
	if( find == INVALID_HANDLE_VALUE )
		throw std::runtime_error( "FindFirstFileEx" );
	BOOST_SCOPE_EXIT_ALL( find )
	{
		FindClose( find );
	};
	if( find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
		return  find_data.dwReserved0;
	else
		return boost::none;

}
boost::optional<REPARSE_TAG> ReadReparseTagByHandle( _In_z_ PCWSTR filename )
{
	HANDLE file = CreateFileW(
		filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
		nullptr
		);
	if( file == INVALID_HANDLE_VALUE )
		throw std::runtime_error( "CreateFile" );
	BOOST_SCOPE_EXIT_ALL( file )
	{
		CloseHandle( file );
	};
	FILE_ATTRIBUTE_TAG_INFO file_tag_info;
	if( !GetFileInformationByHandleEx( file, FileAttributeTagInfo, &file_tag_info, sizeof file_tag_info ) )
		throw std::runtime_error( "GetFileInformationByHandleEx" );
	if( file_tag_info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
		return  file_tag_info.ReparseTag;
	else
		return boost::none;
}

int __cdecl wmain( int argc, PWSTR argv[] )
{
	if( argc < 2 )
		return EXIT_FAILURE;

	HANDLE process_token;
	TOKEN_PRIVILEGES token_privilege;
	OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &process_token );
	LookupPrivilegeValue( NULL, SE_BACKUP_NAME, &token_privilege.Privileges[0].Luid );
	token_privilege.PrivilegeCount = 1;
	token_privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges( process_token, FALSE, &token_privilege, sizeof token_privilege, NULL, NULL );
	CloseHandle( process_token );

	for( int i = 1; i < argc; ++i )
	{
		_putws( argv[i] );
		fputs( "\tGetFileInformationByHandleEx: ", stdout );
		try
		{
			if( auto reparse_tag = ReadReparseTagByHandle( argv[i] ) )
			{
				if( auto tag_name = ReparseTagToString( *reparse_tag ) )
					printf( "%s(%lx)\n", tag_name, *reparse_tag );
				else
					printf( "Unknown Tag(%lx)\n", *reparse_tag );
			}
			else
				puts( "is not Reparse Point" );
		}
		catch( const std::runtime_error& e )
		{
			fputs( e.what(), stderr );
			fputs( " failed\n", stderr );
		}
		fputs( "\tFindFirstFileEx: ", stdout );
		try
		{
			if( auto reparse_tag = ReadReparseTagByFindFile( argv[i] ) )
			{
				if( auto tag_name = ReparseTagToString( *reparse_tag ) )
					printf( "%s(%lx)\n", tag_name, *reparse_tag );
				else
					printf( "Unknown Tag(%lx)\n", *reparse_tag );
			}
			else
				puts( "is not Reparse Point" );
		}
		catch( const std::runtime_error& e )
		{
			fputs( e.what(), stderr );
			fputs( " failed\n", stderr );
		}

	}
}