#include <windows.h>
#pragma comment(lib, "OneCore")

int main()
{
	auto p = reinterpret_cast<char*>(VirtualAlloc2(nullptr, nullptr, 0x10000, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0));
	VirtualFree(p + 0x1000, 0x1000, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
	VirtualFree(p + 0x2000, 0x1000, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
	VirtualAlloc2(nullptr, p, 0x1000, MEM_RESERVE | MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	VirtualAlloc2(nullptr, p, 0x1000, MEM_COMMIT, PAGE_READWRITE, nullptr, 0);
	VirtualAlloc2(nullptr, p + 0x2000, 0x1000, MEM_RESERVE | MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	VirtualAlloc2(nullptr, p + 0x2000, 0x1000, MEM_COMMIT, PAGE_READWRITE, nullptr, 0);
	auto m = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE | SEC_COMMIT, 0, 0x1000, nullptr);
	MapViewOfFile3(m, nullptr, p + 0x1000, 0, 0x1000, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	SecureZeroMemory(p, 0x3000);
}