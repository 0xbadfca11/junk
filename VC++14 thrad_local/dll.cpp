#include <windows.h>
#include <stdio.h>

struct s
{
	s()
	{
		printf( "[% 8lx] " __FUNCSIG__ "\n", GetCurrentThreadId() );
	}
	~s()
	{
		printf( "[% 8lx] " __FUNCSIG__ "\n", GetCurrentThreadId() );
	}
	int i = { 42 };
};
struct pod
{
	int i;
};
thread_local s s_;
thread_local pod p_{ 42 };
thread_local int i = 42;
DWORD FlsIndex;
void f()
{
	printf( "[% 8lx] " __FUNCSIG__ " constructor = %d pod = %d int = %d\n", GetCurrentThreadId(), s_.i, p_.i, i );
	FlsSetValue( FlsIndex, UlongToPtr( 42 ) );
}
//* printf might be DllMain rule violation */
BOOL WINAPI DllMain( HINSTANCE, DWORD dwReason, LPVOID )
{
	switch( dwReason ) {
	case DLL_PROCESS_ATTACH:
		printf( "[% 8lx] DLL_PROCESS_ATTACH\n", GetCurrentThreadId() );
		FlsIndex = FlsAlloc( []( PVOID ){printf( "[% 8lx] PFLS_CALLBACK_FUNCTION\n", GetCurrentThreadId() ); } );
		if( FlsIndex == FLS_OUT_OF_INDEXES )
			return FALSE;
		break;
	case DLL_THREAD_ATTACH:
		printf( "[% 8lx] DLL_THREAD_ATTACH\n", GetCurrentThreadId() );
		break;
	case DLL_THREAD_DETACH:
		printf( "[% 8lx] DLL_THREAD_DETACH\n", GetCurrentThreadId() );
		break;
	case DLL_PROCESS_DETACH:
		printf( "[% 8lx] DLL_PROCESS_DETACH\n", GetCurrentThreadId() );
		FlsFree( FlsIndex );
		break;
	DEFAULT_UNREACHABLE;
	}
	return TRUE;
}