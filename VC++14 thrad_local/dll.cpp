#include <windows.h>
#include <stdio.h>
#include <type_traits>

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
static_assert( std::is_pod<pod>::value, "pod must POD" );

thread_local s s_;
thread_local pod p_{ 42 };
thread_local int i = 42;
void f()
{
	printf( "[% 8lx] " __FUNCSIG__ " constructor = %d pod = %d int = %d\n", GetCurrentThreadId(), s_.i, p_.i, i );
}
//* printf might be DllMain rule violation */
BOOL WINAPI DllMain( HINSTANCE, DWORD dwReason, LPVOID )
{
	switch( dwReason ) {
	case DLL_PROCESS_ATTACH:
		printf( "[% 8lx] DLL_PROCESS_ATTACH\n", GetCurrentThreadId() );
		break;
	case DLL_THREAD_ATTACH:
		printf( "[% 8lx] DLL_THREAD_ATTACH\n", GetCurrentThreadId() );
		break;
	case DLL_THREAD_DETACH:
		printf( "[% 8lx] DLL_THREAD_DETACH\n", GetCurrentThreadId() );
		break;
	case DLL_PROCESS_DETACH:
		printf( "[% 8lx] DLL_PROCESS_DETACH\n", GetCurrentThreadId() );
		break;
	DEFAULT_UNREACHABLE;
	}
	return TRUE;
}