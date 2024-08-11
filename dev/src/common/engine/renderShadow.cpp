/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "renderShadow.h"
#include "renderObject.h"
#include "renderFrame.h"

CRenderShadow::CRenderShadow( CRenderFrame* parentFrame, CRenderFrame* shadowFrame, const Box& casterBox, Uint32 resolution )
	: m_parentFrame( parentFrame )
	, m_shadowFrame( shadowFrame )
	, m_casterBox( casterBox )
	, m_resolution( resolution )
{	
}

CRenderShadow::~CRenderShadow()
{
	// Release shadow frame
	if ( m_shadowFrame )
	{
		m_shadowFrame->Release();
		m_shadowFrame = NULL;
	}
}
