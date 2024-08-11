/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../engine/inputDeviceManager.h"

#include "renderViewportWindowRoot.h"
#include "renderViewportWindow.h"
#include "renderViewport.h"
#include "../engine/rawInputManager.h"

//#ifdef RED_PLATFORM_WINPC

static CViewportWindow* const INVALID_UPDATE_WINDOW_PTR = reinterpret_cast< CViewportWindow* >( ~uintptr_t(0) );

CViewportWindowRoot::CViewportWindowRoot()
	: m_activeViewportWindow( nullptr )
	, m_updateViewportWindow( INVALID_UPDATE_WINDOW_PTR )
{
	//FIXME2<<< Called here because created statically....
	RED_VERIFY( Init() );
}

Bool CViewportWindowRoot::Init()
{
	RED_ASSERT( ::SIsMainThread() );

	SRawInputManager::GetInstance().RegisterListener( this );

	return true;
}

CViewportWindowRoot::~CViewportWindowRoot()
{
	SRawInputManager::GetInstance().UnregisterListener( this );

	Cleanup();
}

void CViewportWindowRoot::Cleanup()
{
}

Bool CViewportWindowRoot::ProcessInput( const BufferedInput& input )
{
	CViewportWindow* updateWindow = m_updateViewportWindow.Exchange( INVALID_UPDATE_WINDOW_PTR );
	if ( updateWindow != INVALID_UPDATE_WINDOW_PTR /*&& m_activeViewportWindow != updateWindow*/ )
	{
		m_activeViewportWindow = updateWindow;
	}

	if ( m_activeViewportWindow )
	{
		// Roundabout way to call m_activeViewportWindow->ProcessInput(...);
		CRenderViewport* vp = m_activeViewportWindow->GetViewport();
		if ( vp )
		{
			vp->ProcessInput( input );
		}
	}

	// If *game* viewport active and game running return true !!!always!!!
	return true;
}

void CViewportWindowRoot::Activate( CViewportWindow* viewportWindow )
{
	m_updateViewportWindow.SetValue( viewportWindow );
}

void CViewportWindowRoot::Deactivate( CViewportWindow* viewportWindow )
{
	RED_UNUSED( viewportWindow );
	SRawInputManager::GetInstance().RequestReset();
	// This is technically wrong if can activate/deactivate from different threads, but this is for main vs viewport thread
	// which is retarded in and of itself because the main thread is too bogged down to be responsive enough during
	// loading. Have fun with race conditions to reset input when alt-tabbing out of the r4Launcher game.
	m_updateViewportWindow.SetValue( nullptr );
}

//#endif // RED_PLATFORM_WINPC