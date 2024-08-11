/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/gameConfiguration.h"
#include "../../common/core/codeParser.h"
#include "../../common/game/configParser.h"
#include "editorMemory.h"
#include <wx/ffile.h>
#include <WindowsX.h>

#include "googleanalytics.h"

// Splash screen
#include "win32splash.h"

class CEditorGameConfiguration
{
	String	m_splashPath;

	static LRESULT CALLBACK GameSelectorWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		PAINTSTRUCT ps;
		HDC hdc;

		switch (message)
		{
		case WM_COMMAND:
			if ( HIWORD( wParam ) == BN_CLICKED )
			{
				PostQuitMessage( 0 );
			}
			return DefWindowProc( hWnd, message, wParam, lParam );
		case WM_PAINT:
			{
				hdc = BeginPaint( hWnd, &ps );
				HGDIOBJ prevFont = SelectObject( hdc, GetStockFont( DEFAULT_GUI_FONT ) );
				SetBkMode( hdc, TRANSPARENT );
				TextOut( hdc, 8, 11, _T("Select the game to edit:"), 24 );
				SelectObject( hdc, prevFont );
				EndPaint( hWnd, &ps );
			}
			break;
		case WM_CLOSE:
			PostQuitMessage( 0 );
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
		}
		return 0;
	}

	void SplitCmdLine( const String & cmdLine, TDynArray< String > & parts )
	{
		CCodeParser parser( cmdLine );
		parser.SetParseStrings();
		while ( parser.HasMore() )
		{
			parts.PushBack( parser.ScanToken() );
		}
	}

public:
	CEditorGameConfiguration()
	{}

	void Initialize()
	{
		SGameConfigurationParameter configParam = 
		{
			TXT( "unknown" ),
			TXT( "data" ),
			TXT( "RedKit Unknown" ),
			TXT( "unknown" ),
			TXT( "unknown" ),
			TXT( "config" ),
			TXT( "unknown" ),
			TXT( "unknown" ),
			TXT( "Telemetry unknown" ),
			TXT( "CR4CameraDirector" )
		};
	

		// Find location of the editor
		Char partPath[ MAX_PATH ];
		Char basePath[ MAX_PATH ];

		GetModuleFileName( NULL, partPath, MAX_PATH );
		GetFullPathName( partPath, MAX_PATH, basePath, NULL );

		// Strip EXE name
		Char* pEnd = Red::System::StringSearchLast( basePath, '\\' );
		if ( pEnd ) 
		{
			*pEnd = '\0';
		}
		// Strip the binary folder - x86/x64/etc...
		pEnd = Red::System::StringSearchLast( basePath, '\\' );
		if ( pEnd ) 
		{
			*pEnd = '\0';
		}

		// Set new current directory to the directory of binary file
		SetCurrentDirectory( basePath );

		// Load game configurations file
		FILE* f = fopen( "gameconf.cfg", "rb" );
		String cfgContents;
		if ( f )
		{
			fseek( f, 0, SEEK_END );
			size_t flen = ftell( f );
			fseek( f, 0, SEEK_SET );
			char* buffer = (char*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, flen + 1 );
			fread( buffer, 1, flen, f );
			buffer[ flen ] = '\0';
			cfgContents = ANSI_TO_UNICODE( buffer );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, buffer );
		}
		else
		{
			MessageBoxA( NULL, "Missing gameconf.cfg file! Aborting!", "Fatal Error", MB_OK|MB_ICONERROR );
			exit( 1 );
		}

		THashMap< String, THashMap< String, String > > cfgParams;
		TDynArray< String > configs;
		GameConfig::ParseConfig( cfgContents, cfgParams );
		cfgParams.GetKeys( configs );

		String lastGameName;
		bool cmdParamUsed = false;

		// Check if the game is defined by the command line
		TDynArray< String > cmdParts;
		SplitCmdLine( GetCommandLine( ), cmdParts );
		ptrdiff_t gameParamIdx = cmdParts.GetIndex( TXT("-game") );
		if ( gameParamIdx != -1 && cmdParts.Size( ) > static_cast< Uint32 >( gameParamIdx ) + 1 )
		{
			lastGameName = cmdParts[ gameParamIdx + 1 ];
			cmdParamUsed = true;
		}

		if ( !cmdParamUsed )
		{
			// Load last chosen game
			f = fopen( "lasteditorgame.txt", "rt" );
			if ( f )
			{
				char name[ 256 ];
				fgets( name, 256, f );
				fclose( f );
				lastGameName = ANSI_TO_UNICODE( name );
				lastGameName.Trim();
			}
		}

		// If there is no game defined (either because the lasteditorgame.txt file is missing
		// or the game isn't defined in the gameconf.cfg file), show a popup asking the user
		// for the game's name
		if ( lastGameName.Empty() || !configs.Exist( lastGameName ) )
		{
			// Register window class
			WNDCLASSEX wcex;
			Red::System::MemorySet( &wcex, 0, sizeof(wcex) );
			wcex.cbSize			= sizeof(WNDCLASSEX);
			wcex.style			= CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc	= GameSelectorWindowProc;
			wcex.hInstance		= GetModuleHandle( NULL );
			wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
			wcex.hbrBackground	= (HBRUSH)( COLOR_3DFACE + 1 );
			wcex.lpszClassName	= _T("EDITORGAMESELECTORDLG");
			RegisterClassEx(&wcex);

			// Create window
			const DWORD style = WS_BORDER|WS_CAPTION|WS_OVERLAPPED|WS_CLIPCHILDREN|WS_DLGFRAME;
			RECT screenRect, windowRect;
			GetClientRect( GetDesktopWindow(), &screenRect );
			windowRect.left = windowRect.top = 0;
			windowRect.right = 298;
			windowRect.bottom = 78;
			AdjustWindowRect( &windowRect, style, FALSE );
			windowRect.right -= windowRect.left;
			windowRect.bottom -= windowRect.top;
			HWND dlg = CreateWindow( wcex.lpszClassName, _T("Select Game"), style,
				( screenRect.right - windowRect.right ) / 2, ( screenRect.bottom - windowRect.bottom ) / 2, 
				windowRect.right, windowRect.bottom, NULL, NULL, wcex.hInstance, NULL );

			// Create the controls
			HWND combo = CreateWindow( _T("COMBOBOX"), _T(""),
				WS_CHILD|WS_VISIBLE|WS_OVERLAPPED|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_HASSTRINGS, 
				141, 8, 149, 123, dlg, NULL, wcex.hInstance, NULL  );
			HWND button = CreateWindow( _T("BUTTON"), _T("&Continue"),
				WS_CHILD|WS_VISIBLE|WS_OVERLAPPED|WS_TABSTOP|BS_DEFPUSHBUTTON,
				215, 47, 75, 25, dlg, NULL, wcex.hInstance, NULL  );

			// Set proper font
			HFONT font = GetStockFont( DEFAULT_GUI_FONT );
			SetWindowFont( combo, font, TRUE );
			SetWindowFont( button, font, TRUE );

			// Fill game entries
			for ( Uint32 i=0; i<configs.Size(); ++i )
			{
				ComboBox_AddString( combo, cfgParams[configs[i]][TXT("title")].AsChar() );
			}
			int currentIndex = ComboBox_FindStringExact( combo, 0, lastGameName.AsChar() );
			if ( currentIndex == -1 )
			{
				currentIndex = 0;
			}
			ComboBox_SetCurSel( combo, currentIndex );

			// Show the window
			ShowWindow( dlg, SW_SHOW );
			UpdateWindow( dlg );
			SetForegroundWindow( dlg );

			// Dialog message loop
			MSG msg;
			while ( GetMessage( &msg, NULL, 0, 0 ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}

			// Flush message queue
			while ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}

			// Obtain the selected game
			lastGameName = configs[ComboBox_GetCurSel( combo )];

			// Destroy the window
			DestroyWindow( dlg );

			// Unregister window class
			UnregisterClass( wcex.lpszClassName, wcex.hInstance );

			// If the game name is still undefined or not found, just use the first one
			if ( lastGameName.Empty() || !configs.Exist( lastGameName ) )
			{
				lastGameName = configs[0];
			}
		}

		// Save the game's name to the lasteditorgame.txt file
		if ( !cmdParamUsed )
		{
			f = fopen( "lasteditorgame.txt", "wt" );
			if ( f )
			{
				fprintf( f, "%ls", lastGameName.AsChar() );
				fclose( f );
			}
		}

		// Set the game configuration parameters using the parameters from the selected game
		configParam.name				= lastGameName;
		configParam.dataPathSuffix		= cfgParams[ lastGameName ][ TXT("data") ];
		configParam.userPathSuffix		= cfgParams[ lastGameName ][ TXT("title") ];
		configParam.bundlePathSuffix	= cfgParams[ lastGameName ][ TXT("bundle") ];
		configParam.scriptsPathSuffix	= cfgParams[ lastGameName ][ TXT("scripts") ];
		configParam.configDirectoryName	= cfgParams[ lastGameName ][ TXT("config") ];
		m_splashPath					= String( basePath ) + TXT("\\..\\") + configParam.dataPathSuffix + TXT("\\") + cfgParams[ lastGameName ][ TXT("splash") ];

		configParam.gameClassName = cfgParams[ lastGameName ][ TXT("gameClass") ];
		configParam.playerClassName = cfgParams[ lastGameName ][ TXT("playerClass") ];
		configParam.telemetryClassName = cfgParams[ lastGameName ][ TXT("telemetryClass") ];
		configParam.cameraDirectorClassName = cfgParams[ lastGameName ][ TXT("cameraDirClassName") ];

		GGameConfig::GetInstance().Initialize( configParam );

		// Rename existing generic INI files to game-specific INI
#define COPYOLDINIFILE(INIFILE) \
		if ( ::wxFileExists( wxString( basePath ) + wxT("\\") + wxString( INIFILE ) ) ) \
		{ \
			::wxRenameFile( wxString( basePath ) + wxT("\\") + wxString( INIFILE ), wxString( basePath ) + wxT("\\") + wxString( configParam.name.AsChar() ) + wxString( INIFILE ) ); \
		}
		COPYOLDINIFILE(wxT("LavaEditor2.common.ini"));
		COPYOLDINIFILE(wxT("LavaEditor2.sessions.ini"));
		COPYOLDINIFILE(wxT("LavaEditor2.presets.ini"));
		COPYOLDINIFILE(wxT("LavaEditor2.ini"));
#undef COPYOLDINIFILE
	}

	const String& GetSplashPath() const
	{
		return m_splashPath;
	}
};

Int32 RunEngine()
{
	GEngine = new CEditorEngine;	
	if ( GEngine->Initialize() )
	{
		GEngine->MainLoop();

		// editor stopped event
		ANALYTICS_SCREEN( "Editor stopped" );
		ANALYTICS_CALL( ManualShutDownService() );

		GEngine->Shutdown();
	}
	auto returnValue = GEngine->GetReturnValue();
	delete GEngine;
	GEngine = NULL;
	return returnValue;
}

class SomeStaticClass
{
public:
	Win32Splash splash;
	CEditorGameConfiguration gameConfig;

	SomeStaticClass()
	{
		gameConfig.Initialize();
		splash.ShowSplash( gameConfig.GetSplashPath().AsChar() );
	}
};

// Place the logger here to ensure that it gets created before we set the current working directory
// (Which is done by TheSpashLauncher below)
// One directory up to ensure it goes in bin/ and not bin/x64/
#ifdef RED_LOGGING_ENABLED
	Red::System::Log::File fileLogger( TXT( "..\\editor.log" ), true );
#endif

static SomeStaticClass TheSplashLauncher;

INT WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, wchar_t* cmdLine, INT cmdShow)
{
	// Start as editor
	GSplash = &TheSplashLauncher.splash;
	GIsEditor = true;

#if defined(W2_PLATFORM_WIN32) && defined(_DEBUG)
	// Enable memory debug
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	RED_MEMORY_INITIALIZE( EditorMemoryParameters );

	// Pass pool parameters to gpu memory system
	auto poolParameterFn = []( Red::MemoryFramework::PoolLabel l, const Red::MemoryFramework::IAllocatorCreationParameters* p ) 
	{ 
		GpuApi::SetPoolParameters( l, p ); 
	};
	EditorGpuMemoryParameters gpuMemoryPoolParameters;
	gpuMemoryPoolParameters.ForEachPoolParameter( poolParameterFn );

	// Initialize platform
	SInitializePlatform( cmdLine );

	// Set global HINSTANCE for WX
	wxSetInstance( instance );

	// Create and run engine
	auto returnValue = RunEngine();

	// Close platform shit
	SShutdownPlatform();

	return returnValue;
}