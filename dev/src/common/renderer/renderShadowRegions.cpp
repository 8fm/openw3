/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderShadowRegions.h"
#include "renderShadowStaticAllocator.h"
#include "renderShadowManager.h"

CRenderShadowStaticCube::CRenderShadowStaticCube()
	: m_allocator( nullptr )
	, m_index( 0 )
	, m_lastFrame( 0 )
{
}

CRenderShadowStaticCube::~CRenderShadowStaticCube()
{
	if ( m_allocator != nullptr )
	{
		m_allocator->ReleaseCube( *this );
		m_allocator = nullptr;
		m_index = 0;
	}
}

Bool CRenderShadowStaticCube::IsValid() const
{
	return m_allocator != nullptr;
}

void CRenderShadowStaticCube::UpdateFrameIndex( Uint32 frame )
{
	m_lastFrame = frame;
}

