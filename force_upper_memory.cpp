#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <intsafe.h>
#include <map>
#include <crtdbg.h>
#pragma comment(lib, "ntdll")

EXTERN_C NTSYSAPI NTSTATUS NTAPI NtSuspendProcess( _In_ HANDLE ProcessHandle );
EXTERN_C NTSYSAPI NTSTATUS NTAPI NtResumeProcess( _In_ HANDLE ProcessHandle );

uintptr_t ceil( uintptr_t x, size_t y )
{
	return ( x + y - 1 ) / y * y;
}
void AllocLowest4GB( DWORD process_id )
{
	bool is_me = process_id == GetCurrentProcessId();
	HANDLE process = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME | PROCESS_VM_OPERATION, FALSE, process_id );
	if( !process )
	{
		fprintf( stderr, "!OpenProcess\n" );
		return;
	}
	if( !is_me )
		NtSuspendProcess( process );
	SYSTEM_INFO system_info;
	GetSystemInfo( &system_info );
	MEMORY_BASIC_INFORMATION memory_info;
#ifdef _DEBUG
	const std::map<decltype( MEMORY_BASIC_INFORMATION::State ), PCSTR> mem_state{ {
		{ MEM_COMMIT, "MEM_COMMIT" },
		{ MEM_FREE, "MEM_FREE" },
		{ MEM_RESERVE, "MEM_RESERVE" }
	} };
#endif
	_RPT0( _CRT_WARN, " address  | BaseAddress | AllocationBase | RegionSize | State\n" );
	UINT_PTR address = reinterpret_cast<UINT_PTR>( system_info.lpMinimumApplicationAddress );
	do
	{
		if( !VirtualQueryEx( process, reinterpret_cast<PVOID>( address ), &memory_info, sizeof memory_info ) )
			break;
		_RPT5(
			_CRT_WARN,
			" % 8IX | % 8IX    | % 8IX       | % 8IX   | %s\n",
			address,
			memory_info.BaseAddress,
			memory_info.AllocationBase,
			memory_info.RegionSize,
			mem_state.find( memory_info.State )->second
			);
		if( memory_info.State == MEM_FREE )
		{
			SIZE_T alloc_size = reinterpret_cast<UINT_PTR>( memory_info.BaseAddress ) + memory_info.RegionSize <= UINT32_MAX ? memory_info.RegionSize : UINT32_MAX - reinterpret_cast<UINT_PTR>( memory_info.BaseAddress );
			_RPT2( _CRT_WARN, "VirtualAlloc( %p, %IX )\n", memory_info.BaseAddress, alloc_size );
			VirtualAllocEx(
				process,
				memory_info.BaseAddress,
				alloc_size,
				MEM_RESERVE,
				PAGE_NOACCESS
				);
		}
	} while( SUCCEEDED( UIntPtrAdd( address, ceil( memory_info.RegionSize, system_info.dwAllocationGranularity ), &address ) ) && address <= UINT32_MAX );
	if( !is_me )
		NtResumeProcess( process );
	CloseHandle( process );
}
int __cdecl main( int argc, PSTR argv[] )
{
	_CrtSetReportMode( _CRT_WARN, IsDebuggerPresent() ? _CRTDBG_MODE_DEBUG : _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
	if( argc < 2 )
		AllocLowest4GB( GetCurrentProcessId() );
	else
		for( int i = 1; i < argc; ++i )
		{
			printf( "%s\n", argv[i] );
			AllocLowest4GB( strtoul( argv[i], NULL, 10 ) );
		}
}