/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "InitializationState.h"
#include "states.h"
#include "../../common/redMemoryFramework/redMemoryCrt.h"
#include "../../common/core/loadingProfiler.h"
#include "../../games/r4/gameEngine.h"

// hack, to allow us to change the game launcher icon on the renderer viewport window.
#ifdef RED_PLATFORM_WINPC
#include "resource.h"
#include "../../common/engine/inputBufferedInputEvent.h"
#include "../../common/renderer/renderViewportWindow.h"
#include "../../common/renderer/renderViewport.h"

HWND FindMyTopMostWindow()
{
	DWORD dwProcID = GetCurrentProcessId();
	HWND hWnd = GetTopWindow( GetDesktopWindow() );
	while( hWnd )
	{
		DWORD dwWndProcID = 0;
		GetWindowThreadProcessId( hWnd, &dwWndProcID );
		if( dwWndProcID == dwProcID )
		{
			TCHAR className[ MAX_PATH ];
			GetClassName( hWnd, className, MAX_PATH );
			if( lstrcmp( TXT( "W2ViewportClass" ), className ) == 0 )
			{
				return hWnd;
			}  
		}
		hWnd = GetNextWindow( hWnd, GW_HWNDNEXT );
	}
	return NULL;
}
#endif

Bool CInitializationSate::OnTick( IGameApplication& state )
{
	GLoadingProfiler.FinishStage( TXT("PreInitialization") );
	if( GEngine->Initialize() )
	{
		GLoadingProfiler.FinishLoading( TXT("EngineInit") );
#ifndef RED_FINAL_BUILD
		RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "EngineInit" );
		RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "EngineInit" );
#endif

// hack, to allow us to change the game launcher icon on the renderer viewport window.
#ifdef RED_PLATFORM_WINPC
		HWND hWnd = FindMyTopMostWindow();
		const HICON hicon = ::LoadIcon( ::GetModuleHandle( 0 ), MAKEINTRESOURCE( IDI_ICON1 ) );
		SendMessage( hWnd, WM_SETICON, ICON_BIG, reinterpret_cast< LPARAM >( hicon ) );
		SendMessage( hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast< LPARAM >( hicon ) );
#endif // RED_PLATFORM_WINPC

		state.RequestState( GameRunning );
		return true;
	}

	state.RequestState( GameShutdown );
	return false;
}
