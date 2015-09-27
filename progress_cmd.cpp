#define WIN32_LEAN_AND_MEAN
#define STRICT
#define STRICT_GS_ENABLED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#include <windows.h>
#include <atlbase.h>
#include <shobjidl.h>
#include <cstdlib>

struct ProgressTaskBar
{
	ATL::CComPtr<ITaskbarList3> TaskbarList;
	HWND hwnd;
	ULONGLONG Total;
	ProgressTaskBar( __RPC__in HWND hwnd, ULONGLONG Total )
		: hwnd( hwnd )
		, Total( Total )
	{
		if( FAILED( CoInitialize( nullptr ) ) )
			ATL::AtlThrowLastWin32();
		if( FAILED( TaskbarList.CoCreateInstance( CLSID_TaskbarList ) ) )
			ATL::AtlThrowLastWin32();
	}
	~ProgressTaskBar()
	{
		TaskbarList->SetProgressState( hwnd, TBPF_NOPROGRESS );
	}
	void SetProgressValue( ULONGLONG Completed )
	{
		TaskbarList->SetProgressValue( hwnd, Completed, Total );
	}
};

int main()
{
	const ULONGLONG count = 1000;
	ProgressTaskBar cmd_bar{ GetConsoleWindow(), count };
	for( int i = 0; i <= count; ++i )
	{
		cmd_bar.SetProgressValue( i );
		Sleep( 1 );
	}
}