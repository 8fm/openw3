/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugCommandBox.h"
#include "renderFrame.h"

#ifndef NO_DEBUG_PAGES

void IDebugCommandBox::OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
{
	IDebugCheckBox::OnRender( frame, x, y, counter, options );

	if ( m_timer > 0.f )
	{
		const Float p = m_timer / m_timeMax;

		Color color;
		color.R = (Uint8)( p * 255 );
		color.G = (Uint8)( p * 255 );
		color.B = (Uint8)( p * 255 );

		y -= LINE_HEIGHT;

		frame->AddDebugScreenText( x + 80, y, GetComment(), color );

		y += LINE_HEIGHT;
	}
}

#endif
