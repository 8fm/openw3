/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugPage.h"
#include "testFramework.h"
#include "renderFrame.h"

#ifndef NO_DEBUG_PAGES

IDebugPage::IDebugPage( const String& name )
	: m_pageName( name )
{
}

IDebugPage::~IDebugPage()
{
}

void IDebugPage::OnPageShown()
{
}

void IDebugPage::OnPageHidden()
{
}

void IDebugPage::OnTick( Float timeDelta )
{
}

Bool IDebugPage::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	return false;
}

Bool IDebugPage::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	return false;
}

void IDebugPage::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
}

Bool IDebugPage::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	return false;
}


void IDebugPage::DrawProgressBar( CRenderFrame *frame, Int32 x, Int32 y, Int32 width, Int32 height, Float prc, const Color& color, const Color& colorFrame )
{
	// Draw background bar
	Int32 barWidth = Clamp< Int32 >( (Int32)( prc * ( width - 2 ) ), 0, width - 2 );
	frame->AddDebugRect( x + 1, y + 1, barWidth, height - 1, color );

	// Draw frame
	frame->AddDebugFrame( x, y, width, height, colorFrame );
}

Bool IDebugPage::FullScreen() const
{
    return false;
}

#endif
