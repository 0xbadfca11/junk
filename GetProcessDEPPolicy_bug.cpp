#ifndef _M_IX86
#error x86 inline asm required.
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

void PrintDEPPolicy(HANDLE process)
{
	DWORD flags = 0;
	BOOL permanent = FALSE;
	BOOL result = FALSE;
	__asm {
		lea edx, permanent
		push edx
		lea eax, flags
		push eax
		push process
		mov cl, 42h
		call GetProcessDEPPolicy
		mov result, eax
	}
	if (result == TRUE)
		printf("%02x\n", permanent);
	else
		puts("!GetProcessDEPPolicy");
}
int __cdecl main(int argc, PSTR argv[])
{
	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, argc < 2 ? GetCurrentProcessId() : strtoul(argv[1], NULL, 10));

	PrintDEPPolicy(process);

	const BYTE mov_edi_edi[2] = { 0x8B, 0xFF };
	const BYTE xor_ecx_ecx[2] = { 0x33, 0xC9 };
	if (memcmp(GetProcessDEPPolicy, mov_edi_edi, sizeof(mov_edi_edi)) == 0)
		WriteProcessMemory(GetCurrentProcess(), GetProcessDEPPolicy, xor_ecx_ecx, sizeof(xor_ecx_ecx), NULL);
	else
		fputs("GetProcessDEPPolicy() entry point is not \"MOV EDI, EDI\"\n", stderr);

	PrintDEPPolicy(process);
}