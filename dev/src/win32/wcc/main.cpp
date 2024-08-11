/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#ifdef WCC_LITE
#include <richedit.h>
#include <commctrl.h>
#include <shellapi.h>
#endif
#include "wccDummyGame.h"
#include "wccTTYWriter.h"

CWccEngine* GEngineWCC = nullptr;

// One directory up to ensure it goes in bin/ and not bin/x64/
#ifdef RED_LOGGING_ENABLED
	Red::System::Log::File fileLogger( TXT( "..\\wcc.log" ), true );
	static CWccTTYWriter wccTTYWriter;
#endif

#include "../../common/redSystem/stringWriter.h"
#include "../../common/core/gameConfiguration.h"
#include "../../common/game/configParser.h"
#include "wccMemory.h"

static ERedGame GGameProject = RG_R4;

CGame* CreateGame() 
{ 
	// Create game - it will be added to the root set by the caller
	GCommonGame = new CWccDummyGame( GGameProject );
	return GCommonGame;
}

Red::System::Error::EAssertAction AssertMessageImp( const Char* cppFile, Uint32 line, const Char* expression, const Char* message, const Char* details )
{
	if ( GEngineWCC )
	{
		ERR_WCC( message );
		bool remember = GEngineWCC->GetRememberErrors();
		GEngineWCC->SetRememberErrors( false );
		ERR_WCC( details );
		GEngineWCC->SetRememberErrors( remember );
	}
	return Red::System::Error::AA_Continue;
}

void SInitializeVersionControl()
{
}

void RegisterRendererNames() {}
void RegisterGameNames() {}

#ifdef WCC_LITE

#pragma comment( lib, "comctl32.lib" )
#pragma comment( linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )

HKEY RegOpenOrCreate( const Char* path )
{
	HKEY key;
	if ( ERROR_SUCCESS != RegOpenKeyW( HKEY_CURRENT_USER, path, &key ) )
	{
		RegCreateKey( HKEY_CURRENT_USER, path, &key );
	}
	return key;
}

static const Char* STORAGE_PATH = L"Software\\CD Projekt RED\\Mod Tools\\1.0";
static const Char* KEY_NAME = L"DoNotShowEula";

Bool ShouldShowEula()
{
	HKEY key = RegOpenOrCreate( STORAGE_PATH );
	DWORD val = 0, type, size;
	if ( ERROR_SUCCESS != RegQueryValueExW( key, KEY_NAME, 0, &type, (BYTE*)&val, &size ) )
	{
		RegSetValueExW( key, KEY_NAME, 0, REG_DWORD, (const BYTE*)&val, sizeof(val) );
	}
	RegCloseKey( key );
	return val == 0;
}

void DisableEula()
{
	HKEY key = RegOpenOrCreate( STORAGE_PATH );
	const DWORD val = 1;
	RegSetValueExW( key, KEY_NAME, 0, REG_DWORD, (const BYTE*)&val, sizeof(val) );
}

DWORD CALLBACK EditStreamCallback( DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, PLONG pcb )
{
	HANDLE hFile = (HANDLE)dwCookie;

	if ( ReadFile(hFile, lpBuff, cb, (DWORD *)pcb, NULL) )
	{
		return 0;
	}

	return -1;
}

INT_PTR CALLBACK DialogProc( _In_ HWND hwndDlg,  _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_INITDIALOG:
		{
			LoadLibraryW( L"Msftedit.dll" );
			RECT r;
			GetClientRect( hwndDlg, &r );
			LONG w = r.right - r.left, h = r.bottom - r.top;
			HMODULE inst = GetModuleHandle( NULL );
			HWND okBtn     = CreateWindowW( L"Button", L"Agree",   BS_DEFPUSHBUTTON|WS_VISIBLE|WS_TABSTOP|WS_CHILD, w-220, h-35, 100, 25, hwndDlg, (HMENU)0, inst, NULL );
			HWND cancelBtn = CreateWindowW( L"Button", L"Decline", BS_PUSHBUTTON|WS_VISIBLE|WS_TABSTOP|WS_CHILD, w-110, h-35, 100, 25, hwndDlg, (HMENU)1, inst, NULL );
			HWND textCtrl  = CreateWindowW( MSFTEDIT_CLASS, L"", ES_MULTILINE|ES_READONLY|ES_SUNKEN|WS_VISIBLE|WS_CHILD|WS_VSCROLL, 5, 5, w-10, h-45, hwndDlg, NULL, inst, NULL );
			SendMessage( okBtn, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE );
			SendMessage( cancelBtn, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE );
			SendMessage( textCtrl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)FALSE );
			SendMessage( textCtrl, EM_SETEDITSTYLE, (WPARAM)SES_HIDEGRIDLINES, (LPARAM)SES_HIDEGRIDLINES );
			SendMessage( textCtrl, EM_SETEVENTMASK, 0, (LPARAM)ENM_LINK ); // enables sending EN_LINK notifications
			// load RTF content 
			HANDLE hFile = CreateFileW( L"wcc_eula.rtf", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
			Bool err = false;
			if ( hFile != INVALID_HANDLE_VALUE )
			{
				EDITSTREAM es = { 0 };
				es.pfnCallback = EditStreamCallback;
				es.dwCookie    = (DWORD_PTR)hFile;
				SendMessage( textCtrl, EM_STREAMIN, SF_RTF, (LPARAM)&es );
				err = es.dwError != 0;
				CloseHandle( hFile );
			}
			else
			{
				err = true;
			}

			if ( err )
			{
				SendMessage( textCtrl, WM_SETTEXT, NULL, (LPARAM)L"<Error loading EULA text>" );
			}
			return TRUE;
		}
	case WM_CLOSE:
		{
			EndDialog( hwndDlg, 0 );
		}
	case WM_COMMAND:
		{
			WORD id = LOWORD( wParam );
			WORD cmd = HIWORD( wParam );
			switch ( cmd )
			{
			case BN_CLICKED:
				{
					EndDialog( hwndDlg, id == 0 );
					return TRUE;
				}
			}
			return FALSE;
		}
	case WM_NOTIFY:
		{
			NMHDR* nmHdr = (NMHDR*)lParam;
			switch ( nmHdr->code )
			{
			case EN_LINK:
				{
					ENLINK* pLink = (ENLINK*)lParam;
					if ( pLink->msg == WM_LBUTTONDOWN )
					{
						LONG numChars = pLink->chrg.cpMax - pLink->chrg.cpMin;
						if ( numChars > 0 )
						{
							if ( LPWSTR url = (LPWSTR)GlobalAlloc( 0, (numChars+1)*sizeof(WCHAR) ) )
							{
								TEXTRANGEW range = { pLink->chrg, url };
								SendMessage( pLink->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM)&range );
								ShellExecuteW( NULL, L"open", url, NULL, NULL, SW_SHOW );
								GlobalFree( url );
							}
							break;
						}
					}
				}
			}
			return TRUE;
		}
	}

	return FALSE;
}

struct DialogData
{
	DialogData( const Char* aTitle, DWORD style, DWORD exStyle, int x, int y, int cx, int cy )
		: dlgVer( 1 )
		, signature( 0xFFFF )
		, helpID( 0 )
		, exStyle( exStyle )
		, style( style )
		, cDlgItems( 0 )
		, x( x ), y( y ), cx( cx ), cy( cy )
		, pointsize( 11 )
		, weight( FW_NORMAL )
		, italic( FALSE )
		, charset( ANSI_CHARSET )
	{
		menu[0] = 0; // no menu
		windowClass[0] = 0; // standard class
		lstrcpy( typeface, L"MS Shell Dlg" );
		lstrcpyn( title, aTitle, 256 );
	}

  WORD      dlgVer;
  WORD      signature;
  DWORD     helpID;
  DWORD     exStyle;
  DWORD     style;
  WORD      cDlgItems;
  short     x;
  short     y;
  short     cx;
  short     cy;
  WORD      menu[1];
  WORD      windowClass[1];
  WCHAR     title[256];
  WORD      pointsize;
  WORD      weight;
  BYTE      italic;
  BYTE      charset;
  WCHAR     typeface[13]; // "MS Shell Dlg"
};

DialogData __declspec(align(32)) data( 
	L"EULA", WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_CENTER | DS_SETFONT, WS_EX_TOPMOST, 0, 0, 320, 200 );

Bool ShowEula()
{
	//TDynArray< Uint8 > data( sizeof( DLGTEMPLATE ) + 6 );
	
	if ( ShouldShowEula() )
	{
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_STANDARD_CLASSES;
		InitCommonControlsEx(&icex);

		if ( DialogBoxIndirect( NULL, (DLGTEMPLATE*)&data, NULL, DialogProc ) != 0 )
		{
			DisableEula();
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}

#endif

int wmain( int argc, const wchar_t* argv[] )
{
#ifdef WCC_LITE
	if ( !ShowEula() )
	{
		return 0;
	}
#endif

	RED_MEMORY_INITIALIZE( WccMemoryParameters );

	// Parse arguments
	Bool disableAsserts = false;
	Bool dumpMemoryUsage = false;
	TDynArray< String > args;
	String commandLine;
	for( int i = 0; i < argc; i++ )
	{
		commandLine += argv[i];
		commandLine += TXT( " " );

		// verbose mode
		if ( 0 == wcsicmp( argv[i], L"-verbose" ) )
		{
			wccTTYWriter.SetVerbose(true);
		}

		// silent mode
		if ( 0 == wcsicmp( argv[i], L"-silent" ) )
		{
			wccTTYWriter.SetSilent(true);
		}

		// memory dump mode
		if ( 0 == wcsicmp( argv[i], L"-memdump" ) )
		{
			dumpMemoryUsage = true;
			continue;
		}

		// disable asserts
		if ( 0 == wcsicmp( argv[i], L"-noassert" ) )
		{
			disableAsserts = true;
			continue;
		}

		args.PushBack( argv[i] );
	}

	String gameName = TXT( "r4" );

	if( args.Size() > 1 && args[1] == TXT( "game" ) )
	{
		if( args.Size() < 2 )
		{
			ERR_WCC( TXT( "Not enough arguments. Wcc was called with the 'game' argument which requires the game name." ) );
			return 4;
		}

		gameName = args[2];
		args.RemoveAt( 1 );
		args.RemoveAt( 1 ); // the game arguments ware handled and we want the rest of the code work as before.
	}

	if( gameName == TXT( "r6" ) )
	{
		GGameProject = RG_R6;
	}

	if( !GameConfig::LoadConfig( gameName ) )
	{		
		return 3;
	}

	int errorCode = 0;

	// This is a cooker
	GIsCooker = true;

	// Pass pool parameters to gpu memory system
	auto poolParameterFn = []( Red::MemoryFramework::PoolLabel l, const Red::MemoryFramework::IAllocatorCreationParameters* p ) 
	{ 
		GpuApi::SetPoolParameters( l, p ); 
	};
	WccGpuMemoryParameters gpuMemoryPoolParameters;
	gpuMemoryPoolParameters.ForEachPoolParameter( poolParameterFn );

	// Enable memory dumping for the whole engine
	if ( dumpMemoryUsage )
	{
		Red::System::DateTime dt;
		Red::System::Clock::GetInstance().GetLocalTime( dt );

		Red::System::StackStringWriter< Char, 256 > txt;
		txt.Append( TXT("memdump_wcc_") );
		txt.Appendf( TXT("%04d_%02d_%02d_[%02d_%02d_%02d].dmp"),
			dt.GetYear(), dt.GetMonth(), dt.GetDay(),
			dt.GetHour(), dt.GetMinute(), dt.GetSecond() );

		LOG_ENGINE( TXT("Enabled memory dump to file '%ls'"), txt.AsChar() );
	}

	// Initialize platform
	SInitializePlatform( commandLine.AsChar() );
	
	// Start cooking engine
	GEngine = GEngineWCC = new CWccEngine();	
	
	Red::System::Log::Manager::GetInstance().SetEnabled( true );
#if defined( RELEASE )
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_SilentCrashHook, true );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, false );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
#else
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, true );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );
#endif

	// Disable asserts
	if ( disableAsserts )
	{
		Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
		ASSERT( !"Assertions should be disabled!" );
	}

	// Disable editor event backbone
	SEvents::GetInstance().Enable( false );

	//////////////////////////////////////

	// Create external reporting interface
	GExternalReporter = new CookerExternalReporter();
	GExternalReporter->SendStatus( TXT("Initializing...") );

	//////////////////////////////////////

	// Initialize engine
	if ( GEngine->Initialize() )
	{
		// Process
		if ( !GEngineWCC->Main( args ) )
		{
			errorCode = 1;
			ERR_WCC( TXT( "Wcc operation failed" ) );
		}

		// Unregister output device
		SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), 7 );
		
		// Close the engine
		GEngine->Shutdown();
		delete GEngine;

		// Reset engine
		GEngineWCC = nullptr;
		GEngine = nullptr;
	}
	else
	{
		errorCode = 2;
		ERR_WCC( TXT( "Failed to initialize the engine!" ) );
	}

	//////////////////////////////////////

	// Update status
	GExternalReporter->SetCookedResource( TXT("") );
	GExternalReporter->SendStatus( TXT("Done!") );
	GExternalReporter->SendProgress( 1,1 );
	delete GExternalReporter;

	//////////////////////////////////////

	// Shutdown local platform
	SShutdownPlatform();

	//////////////////////////////////////

	// Return error code
	return errorCode;
}
 