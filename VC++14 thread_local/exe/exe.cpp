#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <assert.h>

const WCHAR Dll[] = L"dll";

unsigned __stdcall start_address(void*)
{
	printf("[% 8lx] " __FUNCSIG__ "\n", GetCurrentThreadId());

	HMODULE dll;
	while ((dll = GetModuleHandleW(Dll)) == nullptr)
		Sleep(1);

	auto p = (void(*)())GetProcAddress(dll, "f");
	assert(p);
	p();

	Sleep(1);
	return 0;
}

int __cdecl main()
{
	printf("[% 8x] " __FUNCSIG__ "\n", GetCurrentThreadId());

	WaitForSingleObject((HANDLE)_beginthreadex(nullptr, 0, [](void*)->unsigned {
		const int count = 3;
		HANDLE thread[count * 2];

		for (int i = 0; i < count; ++i)
		{
			unsigned thread_id;
			thread[i * 2] = (HANDLE)_beginthreadex(nullptr, 0, start_address, nullptr, 0, &thread_id);
			printf("[% 8lx] CreateThread [% 8lx]\n", GetCurrentThreadId(), thread_id);
		}

		Sleep(1);
		auto dll = LoadLibraryW(Dll);
		assert(dll);

		for (int i = 0; i < count; ++i)
		{
			unsigned thread_id;
			thread[i * 2 + 1] = (HANDLE)_beginthreadex(nullptr, 0, start_address, nullptr, 0, &thread_id);
			printf("[% 8lx] CreateThread [% 8lx]\n", GetCurrentThreadId(), thread_id);
		}

		auto p = (void(*)())GetProcAddress(dll, "f");
		assert(p);
		p();

		WaitForMultipleObjects(ARRAYSIZE(thread), thread, TRUE, INFINITE);
		return 0;
	}, nullptr, 0, nullptr), INFINITE);
}