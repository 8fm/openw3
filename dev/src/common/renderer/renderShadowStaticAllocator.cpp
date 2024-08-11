/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderShadowStaticAllocator.h"
#include "renderShadowManager.h"
#include "renderShadowRegions.h"

//-----------------------------------------------------------------------------

CRenderShadowStaticAllocator::CRenderShadowStaticAllocator( Uint32 resolution, Uint16 numSides )
{
	// Create the texture
	GpuApi::TextureDesc desc;
	desc.width = resolution;
	desc.height = resolution;
	desc.sliceNum = numSides;
	desc.type = GpuApi::TEXTYPE_CUBE;
	desc.initLevels = 1;
	desc.format = GpuApi::TEXFMT_Float_R16;
	desc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_NoDepthCompression;
	m_texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Shadow );
	GpuApi::SetTextureDebugPath( m_texture, "staticShadowTex" );

	// Generate initial list of free indices
	for ( Uint16 i=0; i<numSides; ++i )
	{
		m_freeIndices.PushBack( i );
	}
}

CRenderShadowStaticAllocator::~CRenderShadowStaticAllocator()
{
	// Release the cubemap cache
	GpuApi::SafeRelease( m_texture );

	// Release local references
	for ( Uint32 i=0; i<m_allocatedCubes.Size(); ++i )
	{
		m_allocatedCubes[i]->m_allocator = NULL;
		m_allocatedCubes[i]->Release();
	}
}

CRenderShadowStaticCube* CRenderShadowStaticAllocator::AllocateCube( Uint32 currentFrameIndex, const Box& lightBounds )
{
	// Try to get one from free list
	if ( !m_freeIndices.Empty() )
	{
		// Get the cube index
		const Uint16 cubeIndex = m_freeIndices.PopBack();

		// Create wrapper
		CRenderShadowStaticCube* cube = new CRenderShadowStaticCube();
		cube->m_allocator = this;
		cube->m_lastFrame = currentFrameIndex;
		cube->m_index = cubeIndex;
		cube->m_boundingBox = lightBounds;

		// Remember the object
		m_allocatedCubes.PushBack( cube );
		return cube;
	}

	// Find cube that is old enough ( at least 2 frames old )
	CRenderShadowStaticCube* oldestCube = NULL;
	for ( Uint32 i=0; i<m_allocatedCubes.Size(); ++i )
	{
		CRenderShadowStaticCube* cube = m_allocatedCubes[i];
		if ( cube->m_lastFrame < ( currentFrameIndex-2 ) )
		{
			// try to reuse oldest cubes
			if ( !oldestCube || (cube->m_lastFrame < oldestCube->m_lastFrame) )
			{
				oldestCube = cube;
			}
		}
	}

	// We have a cube we can liberate
	if ( oldestCube != NULL )
	{
		// Get the actual cube index
		const Uint16 cubeIndex = oldestCube->m_index;
		ASSERT( oldestCube->m_allocator != NULL );

		// Liberate the cube
		oldestCube->m_allocator = NULL;
		oldestCube->m_index = 0;

		// Remove from list of allocated cubes
		m_allocatedCubes.Remove( oldestCube );

		// Create new wrapper
		CRenderShadowStaticCube* cube = new CRenderShadowStaticCube();
		cube->m_allocator = this;
		cube->m_lastFrame = currentFrameIndex;
		cube->m_index = cubeIndex;
		cube->m_boundingBox = lightBounds;

		// Remember the object
		m_allocatedCubes.PushBack( cube );
		return cube;
	}

	// No cube allocated
	return NULL;
}

void CRenderShadowStaticAllocator::ReleaseCube( CRenderShadowStaticCube& cube )
{
	ASSERT( cube.m_allocator == this );
	ASSERT( m_allocatedCubes.GetIndex( &cube ) != -1 );
	ASSERT( m_freeIndices.GetIndex( cube.GetIndex() ) == -1 );

	// give back the cube to the cache
	m_allocatedCubes.Remove( &cube );
	m_freeIndices.PushBackUnique( cube.GetIndex() );
	cube.m_allocator = NULL;
}

void CRenderShadowStaticAllocator::InvalidateBounds( const Box& box )
{
	for ( Int32 i=(Int32)m_allocatedCubes.Size()-1; i >= 0; --i )
	{
		CRenderShadowStaticCube* cube = m_allocatedCubes[i];
		if ( cube->GetBoundingBox().Touches( box ) )
		{
			// Release cube index
			m_freeIndices.PushBack( cube->GetIndex() );

			// Reset cube
			cube->m_allocator = NULL;
			cube->m_index = 0;

			// Remove from list of allocated cubes
			m_allocatedCubes.RemoveAtFast( i );
		}
	}
}