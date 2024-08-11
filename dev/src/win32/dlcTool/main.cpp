#include "build.h"
#include <psapi.h>
#include <Tlhelp32.h>
#include <shlobj.h>
#include <memory>
#include "resource.h"


#ifdef _DEBUG
	#ifdef _WIN64
		#pragma comment ( lib, "../../../external/dexzip/dzip64d.lib" )
	#elif WIN32
		#pragma comment ( lib, "../../../external/dexzip/dzipd.lib" )
	#endif
#else
	#ifdef _WIN64
		#pragma comment ( lib, "../../../external/dexzip/dzip64.lib" )
	#elif WIN32
		#pragma comment ( lib, "../../../external/dexzip/dzip.lib" )
	#endif
#endif
#pragma comment( lib, "Psapi.lib" ) 
#define DPSAPI_VERSION (1)

namespace DLCTool
{
	void StopLauncher()
	{
		HANDLE hProcessSnap;
		HANDLE hProcess;
		PROCESSENTRY32 pe32;

		// Take a snapshot of all processes in the system.
		hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
		if( hProcessSnap != INVALID_HANDLE_VALUE )
		{
			// Set the size of the structure before using it.
			pe32.dwSize = sizeof( PROCESSENTRY32 );

			// Retrieve information about the first process,
			// and exit if unsuccessful
			if( Process32First( hProcessSnap, &pe32 ) )
			{
				DWORD currentId = GetCurrentProcessId();
				do 
				{
					if ( pe32.th32ProcessID == currentId )
					{
						hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ParentProcessID );
						if ( hProcess != NULL )
						{
							TCHAR processName[ 512 ];
							DWORD size = 512;
							GetModuleFileNameEx(hProcess, NULL, processName, size );

							const TCHAR *procNameAlone = wcsrchr( processName, '\\' );
							if ( procNameAlone == NULL )
							{
								// '\\' not found
								procNameAlone = processName;
							}
							else
							{
								procNameAlone = procNameAlone + 1;
							}

							if ( _wcsicmp( procNameAlone, TEXT( "launcher.exe" ) ) == 0 )
							{
								TerminateProcess( hProcess, 0 );
							}
							CloseHandle( hProcess );
						}
						break;
					}
				} while ( Process32Next( hProcessSnap, &pe32 ) );

			}
			CloseHandle( hProcessSnap );          // clean the snapshot object
		}
	}

	void RestartLauncher()
	{
		HKEY localMachineKey;
		if( ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\CD Projekt RED\\The Witcher 3"), 0, KEY_READ, &localMachineKey) )
		{
			return;
		}

		DWORD dwSize = 2048;
		DWORD dwType = REG_SZ;
		std::auto_ptr<TCHAR> resultBuffer ( new TCHAR [ dwSize ] );
		long queryresult = RegQueryValueEx( localMachineKey, TEXT("InstallFolder"), 0, &dwType, (LPBYTE)resultBuffer.get(), &dwSize);
		if (ERROR_SUCCESS != queryresult )
		{
			return;
		}

		TCHAR launcherPath[MAX_PATH] = {'\0'};
		wcscpy_s( launcherPath, resultBuffer.get() );
		wcscat_s( launcherPath, TEXT( "\\launcher.exe" ) );

		SHELLEXECUTEINFO executeInfo = {0};
		executeInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		executeInfo.lpVerb = NULL;
		executeInfo.lpFile = launcherPath;
		executeInfo.lpParameters = NULL;
		executeInfo.nShow = SW_SHOW;

		ShellExecuteEx( &executeInfo );
	}

	void DoImportDLC()
	{
		// Stats
		LOG( TEXT("Starting DLC import") );

		// Check privileges
		if ( HasAdminPrivledges() )
		{
			LOG( TEXT("Process is running with admin privledges") );
		}
		else
		{
			ERR( String_Error_NoPrivledges );
			return;
		}

		// Get game installation directory
		WCHAR gameDirectory[ 1024 ];
		if ( GetGameDirectory( gameDirectory, ARRAYSIZE(gameDirectory) ) )
		{
			LOG( TEXT("Game installation directory: '%s'"), gameDirectory );
		}
		else
		{	
			ERR( String_Error_NoGameInstalled );
			return;
		}

		// Stop Launcher
		StopLauncher();

		// Import the DLC
		{
			ExtractArchive( gameDirectory );
			ShowMessage( String_Info_DLCInstalled );
		}
	}

	
}

static void* __stdcall _my_dzip_allocate( size_t size )
{
	return malloc( size );
}

static void __stdcall _my_dzip_free( void* ptr )
{
	free( ptr );
}

static void* __stdcall _my_dzip_realloc( void* ptr, size_t size )
{
	return realloc( ptr, size );
}

INT __stdcall WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd )
{
	// MessageBox( NULL, TEXT("Debug"), TEXT("Debug"), MB_OK ); 
#ifndef 0
	// Initialize DZIP allocator
	dzip_setallocator( _my_dzip_allocate, _my_dzip_free, _my_dzip_realloc );
#endif
	DLCTool::ShowProgressWindow();
	DLCTool::UpdateTaskInfo( DLCTool::String_State_Initializing );

	if ( strstr( lpCmdLine, "-c" ) != NULL )
	{
		// create an archive
		std::string args( lpCmdLine );
		std::size_t pos = args.find( "-c" );
		args = args.substr( pos + 3 );
		char iniFileName[255]; 
		sscanf_s( args.c_str(), "%s", &iniFileName );

		std::string outputFileName( iniFileName );
		outputFileName = outputFileName.substr( 0, outputFileName.length() - 4 );
		outputFileName += ".exe";

		DLCTool::CreateArchive( iniFileName, outputFileName.c_str() );
	}
	else
	{
		// unpack the archive

		// Try to detect language
		Char language[ 20 ];
		if ( DLCTool::DetectLanguage( language ) )
		{
			DLCTool::SetLanguage( language );
		}

		// Initialize controls
		InitCommonControls();

		// Start importing the DLC
		try
		{
			DLCTool::DoImportDLC();
		}

		// Catch any errors
		catch ( ... )
		{
			// Display any error message
			DLCTool::HideProgressWindow();
			DLCTool::DisplayError();
			return -1;
		}
	}

	// No error
	return 0;
}
