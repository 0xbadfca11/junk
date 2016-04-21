#include <windows.h>
#include <stdio.h>
#include <intrin.h>

struct c
{
	c()
	{
		printf("[%p] " __FUNCSIG__ "\n", this);
	}
	~c()
	{
		printf("[%p] " __FUNCSIG__ "\n", this);
	}
};
thread_local c c1;
ULONG fls_index;
BOOL WINAPI DllMain(HINSTANCE, DWORD dwReason, LPVOID)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		fls_index = FlsAlloc([](PVOID p) { delete static_cast<c*>(p); });
		__fallthrough;
	case DLL_THREAD_ATTACH:
		FlsSetValue(fls_index, new c);
		break;
	case DLL_THREAD_DETACH:
		FlsFree(fls_index);
		break;
	case DLL_PROCESS_DETACH:
		delete static_cast<c*>(FlsGetValue(fls_index));
		break;
	DEFAULT_UNREACHABLE;
	}
	return TRUE;
}