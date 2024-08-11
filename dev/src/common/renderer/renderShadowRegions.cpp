/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderShadowRegions.h"
#include "renderShadowStaticAllocator.h"
#include "renderShadowManager.h"

CRenderShadowStaticCube::CRenderShadowStaticCube()
	: m_allocator( NULL )
	, m_index( 0 )
	, m_lastFrame( 0 )
{
}

CRenderShadowStaticCube::~CRenderShadowStaticCube()
{
	if ( m_allocator != NULL )
	{
		m_allocator->ReleaseCube( *this );
		m_allocator = NULL;
		m_index = 0;
	}
}

Bool CRenderShadowStaticCube::IsValid() const
{
	return m_allocator != NULL;
}

void CRenderShadowStaticCube::UpdateFrameIndex( Uint32 frame )
{
	m_lastFrame = frame;
}

