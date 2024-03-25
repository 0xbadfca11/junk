#include <windows.h>
#include <unordered_map>
#include <stdio.h>
#pragma comment(lib, "OneCore")

#pragma warning(disable:6333 28160)
struct P
{
	char p1[0x2000];
	char p2[0x2000];
	char p3[0x2000];
	char p4[0x2000];
	char p5[0x2000];
	char p6[0x2000];
	char p7[0x2000];
};
void r(const P* p)
{
#define A(X) { X, #X }
	const static std::unordered_map<ULONG, PCSTR> Protect = {
		{ 0, "#N/A" },
		A(PAGE_NOACCESS),
		A(PAGE_READWRITE)
	};
	const static std::unordered_map<ULONG, PCSTR> State = {
		A(MEM_COMMIT),
		A(MEM_FREE),
		A(MEM_RESERVE)
	};
	const static std::unordered_map<ULONG, PCSTR> Type = {
		{ 0, "#N/A" },
		A(MEM_IMAGE),
		A(MEM_MAPPED),
		A(MEM_PRIVATE)
	};
	for (const void* a : { p->p1, p->p2, p->p3, p->p4, p->p5, p->p6, p->p7 })
	{
		MEMORY_BASIC_INFORMATION i;
		VirtualQuery(a, &i, sizeof i);
		printf(
			"%p  %p  %-16s % 6zx  %-12s  %-16s %s\n",
			i.BaseAddress,
			i.AllocationBase,
			Protect.at(i.AllocationProtect),
			i.RegionSize,
			State.at(i.State),
			Protect.at(i.Protect),
			Type.at(i.Type)
		);
	}
	SetLastError(NO_ERROR);
}
int main()
{
	auto p = static_cast<P*>(VirtualAlloc2(nullptr, nullptr, sizeof P, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0));
	r(p);
	VirtualFree(p->p2, sizeof p->p2, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
	VirtualFree(p->p4, sizeof p->p4, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
	VirtualFree(p->p6, sizeof p->p6, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
	puts("Split");
	r(p);
	VirtualAlloc2(nullptr, p->p2, sizeof p->p2, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	VirtualAlloc2(nullptr, p->p4, sizeof p->p4, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	VirtualAlloc2(nullptr, p->p6, sizeof p->p6, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	puts("Commit");
	r(p);
	auto m = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE | SEC_COMMIT, 0, sizeof P, nullptr);
	MapViewOfFile3(m, nullptr, p->p3, offsetof(P, p3), sizeof p->p3, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	MapViewOfFile3(m, nullptr, p->p5, offsetof(P, p5), sizeof p->p5, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
	puts("Mapped");
	r(p);
	SecureZeroMemory(&p->p2, offsetof(P, p7) - offsetof(P, p2));
	UnmapViewOfFile2(GetCurrentProcess(), p->p3, MEM_PRESERVE_PLACEHOLDER);
	UnmapViewOfFile(p->p5);
	puts("Unmapped");
	r(p);
	VirtualFree(p->p1, 0, MEM_RELEASE);
	VirtualFree(p->p2, 0, MEM_RELEASE);
	VirtualFree(p->p3, 0, MEM_RELEASE);
	VirtualFree(p->p4, 0, MEM_RELEASE);
	VirtualFree(p->p6, 0, MEM_RELEASE);
	VirtualFree(p->p7, 0, MEM_RELEASE);
	puts("Freed");
	r(p);
}
