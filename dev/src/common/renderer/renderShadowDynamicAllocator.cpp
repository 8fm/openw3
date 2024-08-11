/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderShadowDynamicAllocator.h"
#include "renderTextureAllocator.h"
#include "renderShadowManager.h"
#include "renderShadowRegions.h"

//-----------------------------------------------------------------------------

CRenderShadowDynamicRegion::CRenderShadowDynamicRegion()
	: m_size( 0 )
	, m_offsetX( 0 )
	, m_offsetY( 0 )
	, m_slice( 0 )
{
}

CRenderShadowDynamicAllocator::CRenderShadowDynamicAllocator( Uint16 textureSize, Uint16 numSlices )
{
	// Create the texture
	GpuApi::TextureDesc desc;
	desc.width = textureSize;
	desc.height = textureSize;
	desc.sliceNum = numSlices;
	desc.type = GpuApi::TEXTYPE_ARRAY;
	desc.initLevels = 1;
	desc.format = GpuApi::TEXFMT_Float_R16;
	desc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_GenMip | GpuApi::TEXUSAGE_NoDepthCompression;
	m_texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Shadow );
	GpuApi::SetTextureDebugPath( m_texture, "dynamicShadowTex" );

	// Create allocators
	m_allocators.Resize( numSlices );
	for ( Uint32 i=0; i<numSlices; ++i )
	{
		m_allocators[i] = new CRenderTextureAllocator( textureSize, textureSize );
	}
}

//-----------------------------------------------------------------------------

CRenderShadowDynamicAllocator::~CRenderShadowDynamicAllocator()
{
	// Release textures
	GpuApi::SafeRelease( m_texture );

	// Delete allocators
	m_allocators.ClearPtr();
}

void CRenderShadowDynamicAllocator::Reset()
{
	// Release all texture space allocators
	for ( Uint32 i=0; i<m_allocators.Size(); ++i )
	{
		m_allocators[i]->Reset();
	}
}

CRenderShadowDynamicRegion* CRenderShadowDynamicAllocator::Allocate( Uint16 size )
{
	// Try to allocate texture space
	for ( Uint32 i=0; i<m_allocators.Size(); ++i )
	{
		const Uint16 sizeWithBorder = size + (SHADOW_BORDER_SIZE*2);

		// Always allocate size with border
		Uint16 offsetX, offsetY;
		if ( m_allocators[i]->AllocateSpace( sizeWithBorder, sizeWithBorder, &offsetX, &offsetY ) )
		{
			// Create the description
			CRenderShadowDynamicRegion* ret = new CRenderShadowDynamicRegion();
			ret->m_slice = (Uint16) i;
			ret->m_offsetX = offsetX + SHADOW_BORDER_SIZE;
			ret->m_offsetY = offsetY + SHADOW_BORDER_SIZE;
			ret->m_size = size;
			return ret;
		}
	}

	// Not allocated
	return NULL;
}
