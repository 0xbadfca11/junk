#include <windows.h>

#pragma intrinsic(strcmp)
FARPROC WINAPI GetProcAddressImpl( _In_ HMODULE hModule, _In_z_ LPCSTR lpProcName )
{
	PCHAR base_address = reinterpret_cast<PCHAR>( hModule );
	PIMAGE_NT_HEADERS nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>( base_address + reinterpret_cast<PIMAGE_DOS_HEADER>( base_address )->e_lfanew );
	PIMAGE_EXPORT_DIRECTORY export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>( base_address + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress );
	PULONG AddressOfFunctions = reinterpret_cast<PULONG>( base_address + export_directory->AddressOfFunctions );
	PULONG AddressOfNames = reinterpret_cast<PULONG>( base_address + export_directory->AddressOfNames );
	PUSHORT AddressOfNameOrdinals = reinterpret_cast<PUSHORT>( base_address + export_directory->AddressOfNameOrdinals );
	if( reinterpret_cast<uintptr_t>( lpProcName ) > USHRT_MAX )
	{
		for( unsigned i = 0; i < export_directory->NumberOfNames; ++i )
			if( strcmp( base_address + AddressOfNames[i], lpProcName ) == 0 )
				return reinterpret_cast<FARPROC>( base_address + AddressOfFunctions[AddressOfNameOrdinals[i]] );
	}
	else
	{
		unsigned ordinals = PtrToUshort( lpProcName ) - export_directory->Base;
		if( ordinals < export_directory->NumberOfFunctions )
			return reinterpret_cast<FARPROC>( base_address + AddressOfFunctions[ordinals] );
	}
	return NULL;
}