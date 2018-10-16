#if defined(_UNICODE) && !defined(DBGHELP_TRANSLATE_TCHAR)
#define DBGHELP_TRANSLATE_TCHAR
#endif
#if defined(DBGHELP_TRANSLATE_TCHAR) && !defined(_UNICODE)
#define _UNICODE
#endif
#define PSAPI_VERSION 1
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _CRTDBG_MAP_ALLOC
#include <windows.h>
#include <atlbase.h>
#include <dbghelp.h>
#include <psapi.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <cstdio>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <tchar.h>
#include <crtdbg.h>
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "psapi")
#pragma comment(lib, "shlwapi")

#include <pshpack4.h>
struct EXCEPTION_REGISTRATION_RECORD32
{
	EXCEPTION_REGISTRATION_RECORD32* UPOINTER_32 Next;
	EXCEPTION_ROUTINE* UPOINTER_32 Handler;
};
#include <poppack.h>
static_assert(sizeof(EXCEPTION_REGISTRATION_RECORD32) == 8);
struct _TEB
{
	NT_TIB Tib;
};

struct SEH_Walk
{
	SEH_Walk(DWORD process_id, DWORD thread_id)
		: process_id(process_id)
		, process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, process_id))
		, thread_id(thread_id)
		, thread(OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, FALSE, thread_id))
	{
		if (!thread || !process)
			throw std::runtime_error("OpenProcess|OpenThread");

		BOOL run_under_wow64;
#ifdef _WIN64
		if (!IsWow64Process(process, &run_under_wow64))
			throw std::runtime_error("IsWow64Process");
		else
			if (!run_under_wow64)
				throw std::invalid_argument("Is not WOW64 process");
#else
		if (IsOS(OS_WOW6432))
			if (!IsWow64Process(process, &run_under_wow64))
				throw std::runtime_error("IsWow64Process");
			else
				if (!run_under_wow64)
					throw std::invalid_argument("Is not WOW64 process");
#endif

		SymSetOptions(SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_FAVOR_COMPRESSED | SymGetOptions());
		if (!SymInitialize(process, nullptr, TRUE))
			throw std::runtime_error("SymInitialize");
		if (!GetModuleHandleW(L"symsrv"))
			puts("symsrv.dll is not loaded. Result may incorrect.");
	}
	~SEH_Walk()
	{
		SymCleanup(process);
	}
	void SEH_Chain_Walk() const
	{
#ifdef _M_IX86
		if (thread_id == GetCurrentThreadId())
			for (auto address = NtCurrentTeb()->Tib.ExceptionList; PtrToUlong(address) != ULONG_MAX; address = address->Next)
			{
				auto module_file_name = GetModuleFileNameFromAddress(address->Handler);
				auto symbol = GetSymbolFromAddress(address->Handler);
				_tprintf(
					_T("Handler->%p(%ls!%s):Next->%p\n"),
					address->Handler,
					module_file_name ? PathFindFileNameW(module_file_name.get()) : nullptr,
					symbol ? symbol->si.Name : nullptr,
					address->Next
				);
			}
		else
#endif
		{
#ifdef _WIN64
			if (Wow64SuspendThread(thread) == -1)
#else
			if (SuspendThread(thread) == -1)
#endif
				throw std::runtime_error("SuspendThread");
			const std::shared_ptr<std::remove_pointer<HANDLE>::type> thread_wake(HANDLE(thread), ResumeThread);
#ifdef _WIN64
			WOW64_CONTEXT Context;
			Context.ContextFlags = CONTEXT_SEGMENTS;
			if (!Wow64GetThreadContext(thread, &Context))
#else
			CONTEXT Context;
			Context.ContextFlags = CONTEXT_SEGMENTS;
			if (!GetThreadContext(thread, &Context))
#endif
				throw std::runtime_error("GetThreadContext");
#ifdef _WIN64
			WOW64_LDT_ENTRY ldtSel;
			if (!Wow64GetThreadSelectorEntry(thread, Context.SegFs, &ldtSel))
#else
			LDT_ENTRY ldtSel;
			if (!GetThreadSelectorEntry(thread, Context.SegFs, &ldtSel))
#endif
				throw std::runtime_error("GetThreadSelectorEntry");
			DWORD FS0 = (ldtSel.HighWord.Bits.BaseHi << 24) | (ldtSel.HighWord.Bits.BaseMid << 16) | (ldtSel.BaseLow);
			NT_TIB32 Tib;
			if (!ReadProcessMemory(process, ULongToPtr(FS0), &Tib, sizeof Tib, nullptr))
				throw std::runtime_error("ReadProcessMemory");
			EXCEPTION_REGISTRATION_RECORD32 exception_record;
			for (ULONG record_address = Tib.ExceptionList; record_address != ULONG_MAX; record_address = PtrToUlong(exception_record.Next))
			{
				if (!ReadProcessMemory(process, ULongToPtr(record_address), &exception_record, sizeof exception_record, nullptr))
					throw std::runtime_error("ReadProcessMemory");
				auto module_file_name = GetModuleFileNameFromAddress(exception_record.Handler);
				auto symbol = GetSymbolFromAddress(exception_record.Handler);
				_tprintf(
					_T("Handler->%08lX(%ls!%s):Next->%08lX\n"),
					PtrToUlong(exception_record.Handler),
					module_file_name ? PathFindFileNameW(module_file_name.get()) : nullptr,
					symbol ? symbol->si.Name : nullptr,
					PtrToUlong(exception_record.Next)
				);
			}
		}
	}
protected:
	const DWORD process_id;
	const ATL::CHandle process;
	const DWORD thread_id;
	const ATL::CHandle thread;
	std::unique_ptr<WCHAR[]> GetModuleFileNameFromAddress(_In_ PVOID address) const
	{
		auto module_file_name = std::make_unique<WCHAR[]>(MAX_PATH);
		if (GetMappedFileNameW(process, address, module_file_name.get(), MAX_PATH))
			return module_file_name;
		else
			return nullptr;
	}
	std::unique_ptr<SYMBOL_INFO_PACKAGE> GetSymbolFromAddress(_In_ PVOID address) const
	{
		auto symbol = std::make_unique<SYMBOL_INFO_PACKAGE>();
		symbol->si.SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->si.MaxNameLen = MAX_SYM_NAME;
		if (SymFromAddr(process, PtrToUlong(address), nullptr, &symbol->si))
			return symbol;
		else
			return nullptr;
	}
private:
	SEH_Walk(const SEH_Walk&) = delete;
	SEH_Walk& operator=(const SEH_Walk&) = delete;
};
int __cdecl main(int argc, PSTR argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, IsDebuggerPresent() ? _CRTDBG_MODE_DEBUG : _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

	if (GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", nullptr, 0) == 0)
		SetEnvironmentVariableW(L"_NT_SYMBOL_PATH", L"srv**http://msdl.microsoft.com/download/symbols");
	if (argc < 2)
	{
		printf("%s PID(-1 if self)\n", argv[0]);
		return EXIT_SUCCESS;
	}
	DWORD process_id = strtoul(argv[1], NULL, 10);
	if (process_id == -1)
		process_id = GetCurrentProcessId();
	printf("ProcessId: %lu\n", process_id);
	HANDLE th32 = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (th32 == INVALID_HANDLE_VALUE)
	{
		return EXIT_FAILURE;
	}
	const ATL::CHandle th32_close(th32);
	THREADENTRY32 thread_entry;
	thread_entry.dwSize = sizeof(thread_entry);
	if (!Thread32First(th32, &thread_entry))
	{
		return EXIT_FAILURE;
	}
	try
	{
		do
		{
			if (thread_entry.dwSize < RTL_SIZEOF_THROUGH_FIELD(THREADENTRY32, th32OwnerProcessID))
			{
				continue;
			}
			if (thread_entry.th32OwnerProcessID == process_id)
			{
				try
				{
					printf("ThreadId:  %lu\n", thread_entry.th32ThreadID);
					SEH_Walk(process_id, thread_entry.th32ThreadID).SEH_Chain_Walk();
				}
				catch (const std::runtime_error& e)
				{
					fputs(e.what(), stderr);
					fputs("\n", stderr);
				}
			}
		} while (thread_entry.dwSize = sizeof(thread_entry), Thread32Next(th32, &thread_entry));
	}
	catch (const std::invalid_argument& e)
	{
		fputs(e.what(), stderr);
	}
}