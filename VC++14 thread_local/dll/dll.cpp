#include <windows.h>
#include <stdio.h>
#include <type_traits>

struct c
{
	c()
	{
		printf("[% 8lx] " __FUNCSIG__ "\n", GetCurrentThreadId());
	}
	~c()
	{
		printf("[% 8lx] " __FUNCSIG__ "\n", GetCurrentThreadId());
	}
	int i = { 42 };
};
static_assert(!std::is_pod<c>::value, "non-POD");
struct p
{
	int i;
};
static_assert(std::is_pod<p>::value, "POD");

thread_local c c_;
thread_local p p_{ 42 };
thread_local int si = 42;
thread_local int di = [] { return 42; }();
void f()
{
	printf("[% 8lx] " __FUNCSIG__ " non-pod = %d pod = %d dyn init int = %d, static init int = %d\n", GetCurrentThreadId(), c_.i, p_.i, di, si);
}
//* printf might be DllMain rule violation */
BOOL WINAPI DllMain(HINSTANCE, DWORD dwReason, LPVOID)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		printf("[% 8lx] DLL_PROCESS_ATTACH\n", GetCurrentThreadId());
		break;
	case DLL_THREAD_ATTACH:
		printf("[% 8lx] DLL_THREAD_ATTACH\n", GetCurrentThreadId());
		break;
	case DLL_THREAD_DETACH:
		printf("[% 8lx] DLL_THREAD_DETACH\n", GetCurrentThreadId());
		break;
	case DLL_PROCESS_DETACH:
		printf("[% 8lx] DLL_PROCESS_DETACH\n", GetCurrentThreadId());
		break;
	DEFAULT_UNREACHABLE;
	}
	return TRUE;
}