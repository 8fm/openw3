/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/rawInputManager.h"

//#ifdef RED_PLATFORM_WINPC

class CViewportWindow;

/// WinAPI viewport window manager
class CViewportWindowRoot : public IRawInputListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	CViewportWindow*							m_activeViewportWindow;
	Red::Threads::CAtomic< CViewportWindow* >	m_updateViewportWindow;
	
public:
											CViewportWindowRoot();
	Bool									Init();
	virtual									~CViewportWindowRoot();

public:
	virtual Bool							ProcessInput( const BufferedInput& input ) override;

public:
	void									Activate( CViewportWindow* viewportWindow );
	void									Deactivate( CViewportWindow* viewportWindow );
	Bool									IsViewportActive( const CViewportWindow* viewportWindow ) 
											{ return ( m_activeViewportWindow != nullptr && viewportWindow == m_activeViewportWindow ) || viewportWindow == m_updateViewportWindow.GetValue(); }

private:
	void									Cleanup();
};

//#endif // RED_PLATFORM_WINPC