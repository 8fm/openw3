/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "viewportHook.h"
#include "renderCommands.h"
#include "viewport.h"
#include "inputBufferedInputEvent.h"

IMPLEMENT_RTTI_ENUM( EInputAction );

CMousePacket::CMousePacket( IViewport* viewport, Int32 x, Int32 y, Int32 dx, Int32 dy, Float pressure )
	: m_viewport( viewport )
	, m_x( x )
	, m_y( y )
	, m_dx( dx )
	, m_dy( dy )
	, m_pressure( pressure )
{
	m_viewport->CalcRay( m_x, m_y, m_rayOrigin, m_rayDirection );
}

CRenderFrame *IViewportHook::OnViewportCreateFrame( IViewport *view )
{
	CRenderFrameInfo frameInfo( view );
	return GRender->CreateFrame( NULL, frameInfo );
}

void IViewportHook::OnViewportRenderFrame( IViewport *view, CRenderFrame *frame )
{
	// Render this crap
	( new CRenderCommand_RenderScene( frame, NULL ) )->Commit();
}
