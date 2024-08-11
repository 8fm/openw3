/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderTextureAllocator;
class CRenderShadowDynamicAllocator;
class CRenderShadowDynamicRegion;

/// Allocator of shadow space
class CRenderShadowDynamicAllocator
{
public:
	//! Texture array itself ( for ESM shadows, R32F or R16F )
	GpuApi::TextureRef			m_texture;

	//! Region allocators (one for each slice of texture)
	TDynArray< CRenderTextureAllocator*	 >	m_allocators;

public:
	//! Get the shadowmap texture
	RED_INLINE GpuApi::TextureRef GetTexture() const { return m_texture; }

public:
	CRenderShadowDynamicAllocator( Uint16 textureSize, Uint16 numSlices );
	~CRenderShadowDynamicAllocator();

	//! Reset (for a new frame)
	void Reset();

	//! Request a page to be allocated, can return NULL
	CRenderShadowDynamicRegion* Allocate( Uint16 size );
};