#include <windows.h>
#include <map>
#include <cstdio>
#pragma comment(lib, "advapi32")

int __cdecl wmain( int argc, PWSTR argv[] )
{
	if( argc < 2 )
		return EXIT_FAILURE;

	const std::map<decltype( FILE_ATTRIBUTE_TAG_INFO::ReparseTag ), PCSTR> tag{ {
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

	HANDLE process_token;
	TOKEN_PRIVILEGES token_privilege;
	OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &process_token );
	LookupPrivilegeValue( NULL, SE_BACKUP_NAME, &token_privilege.Privileges[0].Luid );
	token_privilege.PrivilegeCount = 1;
	token_privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges( process_token, FALSE, &token_privilege, sizeof( TOKEN_PRIVILEGES ), NULL, NULL );
	CloseHandle( process_token );

	for( int i = 1; i < argc; ++i )
	{
		PCWSTR filename = argv[i];
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
		{
			fprintf( stderr, "!CreateFile(%ws) fail %08lx\n", filename, GetLastError() );
			continue;
		}
		FILE_ATTRIBUTE_TAG_INFO file_tag_info;
		if( !GetFileInformationByHandleEx( file, FileAttributeTagInfo, &file_tag_info, sizeof file_tag_info ) )
		{
			fprintf( stderr, "!GetFileInformationByHandleEx: %08lx\n", GetLastError() );
		}
		else
		{
			printf( "%ws -> ", filename );
			if( file_tag_info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
			{
				auto tag_name = tag.find( file_tag_info.ReparseTag );
				if( tag_name != tag.end() )
					printf( "%s(%lX)\n", tag_name->second, file_tag_info.ReparseTag );
				else
					printf( "Unknown Tag %lX\n", file_tag_info.ReparseTag );
			}
			else
				puts( "is not Reparse" );
		}
		CloseHandle( file );
	}
}