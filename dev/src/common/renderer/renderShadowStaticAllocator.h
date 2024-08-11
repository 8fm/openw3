/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderShadowStaticCube;

//-----------------------------------------------------------------------------

/// A cache (texture array) for point light
class CRenderShadowStaticAllocator
{
	friend class CRenderShadowStaticCube;

private:
	//! Texture array or CUBES ( for ESM shadows, R16F )
	GpuApi::TextureRef			m_texture;

	//! Free cube indices
	TDynArray< Uint16 >			m_freeIndices;

	//! Allocated cube
	TDynArray< CRenderShadowStaticCube* >		m_allocatedCubes;

public:
	//! Get the shadowmap texture
	RED_INLINE GpuApi::TextureRef GetTexture() const { return m_texture; }

public:
	CRenderShadowStaticAllocator( Uint32 resolution, Uint16 numSides );
	~CRenderShadowStaticAllocator();

	//! Allocate shadow cube, can return NULL if limit was reached
	CRenderShadowStaticCube* AllocateCube( Uint32 currentFrameIndex, const Box& lightBounds );

	//! Invalidate cubes touching given bounds
	void InvalidateBounds( const Box& box );

protected:
	//! Called when allcated cube is released ( no longer needed or invalid )
	void ReleaseCube( CRenderShadowStaticCube& cube );
};