/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "hwCursorManager.h"

CHardwareCursorManager GHardwareCursorManager;

namespace Config
{
	TConfigVar<Bool> cvIsHardwareCursor( "Rendering", "HardwareCursor", false, eConsoleVarFlag_Save );
}

CHardwareCursorManager::CHardwareCursorManager()
	: m_stateRequestedByGame( false )
	, m_stateRequestedByViewport( true )
	, m_updateCursorVisibilityPending(false)
	, m_lastSentMouseX(-1000)
	, m_lastSentMouseY(-1000)
{
}

void CHardwareCursorManager::Viewport_RequestHardwareCursor( Bool r )
{
	m_stateRequestedByViewport = r;
	m_updateCursorVisibilityPending = true;
	
	if ( GIsEditor )
	{
		// if we are in the editor update the cursor visibility immediately
		// so we don't end up with missing cursor on pausing the game etc
		Update();
	}
}

void CHardwareCursorManager::Update()
{
	if ( !m_updateCursorVisibilityPending )
	{
		return;
	}
	m_updateCursorVisibilityPending = false;

	Bool stateToSet = m_stateRequestedByGame || m_stateRequestedByViewport;

	// *********** temp comment:
	// Below is just a copy-paste from viewport
	// ***********


	// For future code dwellers: the reason this code doesn't use GetCursorInfo to
	// check the current cursor visibility state is that GetCursorInfo returns the 
	// *global* state while ShowCursor controls the *thread-local* state.  This means
	// that if for some reason the cursor becomes visible because of another thread
	// or application, the ShowCursor's counter may still be below 0 but GetCursorInfo
	// will tell us that the cursor is visible.  To avoid that, we use ShowCursor itself
	// to force the counter to 0 or -1.

#ifndef RED_PLATFORM_CONSOLE
	if ( stateToSet )
	{
		int r = ::ShowCursor( true );
		if ( r > 0 ) // The cursor was already visible, try to reduce the counter to 1
		{
			while ( ::ShowCursor( false ) > 0 );
		}
		else if ( r < 0 ) // Despite asking to be shown, the cursor is not really visible
		{
			while ( ::ShowCursor( true ) < 0 );
		}	

		if ( !GIsEditor )
		{
			::SetCursor( ::LoadCursor( ::GetModuleHandle( 0 ), MAKEINTRESOURCE( 102 ) ) ); // safe to call ::LoadCursor() each time, it does nothing if it's loaded already	
		}
	}
	else
	{
		int r = ::ShowCursor( false );
		if ( r > -1 ) // The cursor is still visible, try to reduce the counter until it isn't
		{
			while ( ::ShowCursor( false ) > -1 );
		}
		else if ( r < -1 ) // The cursor was already invisible, increase the counter to -1
		{
			while ( ::ShowCursor( true ) < -1 );
		}	
	}
#endif
}

void CHardwareCursorManager::OnMouseMove( Float x, Float y )
{
	if ( Config::cvIsHardwareCursor.Get() && (m_lastSentMouseX != x || m_lastSentMouseY != y) )
	{
		// This is necessary to avoid flooding scaleform with mouse move events that are redundant
		m_lastSentMouseX = x;
		m_lastSentMouseY = y;
		GGame->MoveMouseTo( x, y );
	}
}
