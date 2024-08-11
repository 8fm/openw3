#include "build.h"

namespace DLCTool
{
	// We we inside error state
	bool GIsInError = false;

	// We are in silent mode
	bool GIsSilent = false;

	// Error state
	WCHAR GErrorMsg[ 4096 ];

	extern HWND GMainWindow;

	// Enable silent mode - not user output
	void EnableSilentMode()
	{
		GIsSilent = true;
		LOG( TEXT("DLCTool is running in silent mode") );
	}

	// Debug message
	void DebugMessage( const WCHAR* txt, ... )
	{
		// Format error msg
		va_list arglist;
		va_start( arglist, txt );
		WCHAR formattedBuf[ 4096 ];
		vswprintf_s( formattedBuf, ARRAYSIZE(formattedBuf), txt, arglist ); 

		// Show debug message
		if ( !GIsSilent )
		{
			OutputDebugStringW( formattedBuf );
			OutputDebugStringW( TEXT("\n") );
		}
	}

	// Log
	void Log( const WCHAR* txt, ... )
	{
		// Format error msg
		va_list arglist;
		va_start( arglist, txt );
		WCHAR formattedBuf[ 4096 ];
		vswprintf_s( formattedBuf, ARRAYSIZE(formattedBuf), txt, arglist ); 

		// Emit to debug output
		OutputDebugStringW( formattedBuf );
		OutputDebugStringW( TEXT("\n") );
	}

	// Log
	void Log( const CHAR* txt, ... )
	{
		// Format error msg
		va_list arglist;
		va_start( arglist, txt );
		CHAR formattedBuf[ 4096 ];
		vsprintf_s( formattedBuf, ARRAYSIZE(formattedBuf), txt, arglist ); 

		// Emit to debug output
		OutputDebugStringA( formattedBuf );
		OutputDebugStringA( "\n" );
	}

	// Display generic message
	void ShowMessage( StringID messageStringID )
	{
		if ( !GIsSilent )
		{
			const WCHAR* title = GetStringById( String_ToolTitle );
			const WCHAR* msg = GetStringById( messageStringID );
			MessageBoxW( GMainWindow, msg, title, MB_ICONINFORMATION | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND );
		}
	}

	// Throw error
	void ThrowError( StringID errorStringID, ... )
	{
		// Get argument list
		va_list arglist;
		va_start( arglist, errorStringID );

		// Format full error msg
		WCHAR formattedBuf[ 4096 ];
		const WCHAR* errorString = DLCTool::GetStringById( errorStringID );
		vswprintf_s( formattedBuf, ARRAYSIZE(formattedBuf), errorString, arglist ); 

		// Output to debug
		OutputDebugStringW( TEXT("Application error has occured:\n") );
		OutputDebugStringW( formattedBuf );
		OutputDebugStringW( TEXT("\n") );

		// Throw exception to be catched up
		if ( !GIsInError )
		{
			wcscpy_s( GErrorMsg, ARRAYSIZE(GErrorMsg), formattedBuf );
			GIsInError = true;
			throw 666;
		}
	}

	// Display error message
	void DisplayError()
	{
		if ( GIsInError && !GIsSilent )
		{
			const WCHAR* title = GetStringById( String_ErrorTilte );
			MessageBoxW( GMainWindow, GErrorMsg, title, MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND );
		}
	}
}