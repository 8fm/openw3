/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifdef USE_SCALEFORM


#include "scaleformTextureCacheImage.h"
#include "textureCache.h"
#include "texture.h"
#include "../core/fileLatentLoadingToken.h"

#include "scaleformTextureCacheQuee.h"

// static helper so we don't have to call a virtual function from construct for assert check.
static SF::Render::ImageFormat MapFormat( GpuApi::eTextureFormat gpuFormat )
{
	switch ( gpuFormat )
	{
	case GpuApi::TEXFMT_R8G8B8A8:	return SF::Render::Image_R8G8B8A8;
	case GpuApi::TEXFMT_R8G8B8X8:	return SF::Render::Image_R8G8B8;
	case GpuApi::TEXFMT_A8:			return SF::Render::Image_A8;
	case GpuApi::TEXFMT_BC1:		return SF::Render::Image_BC1;
	case GpuApi::TEXFMT_BC2:		return SF::Render::Image_BC2;
	case GpuApi::TEXFMT_BC3:		return SF::Render::Image_BC3;
	case GpuApi::TEXFMT_BC7:		return SF::Render::Image_BC7;

	default:
		RED_HALT( "Unsupported texture format: %" RED_PRIWas, GpuApi::GetTextureFormatName( gpuFormat ) );
		return SF::Render::Image_None;
	};
}



CScaleformTextureCacheImage::CScaleformTextureCacheImage( const CTextureCacheQuery& cacheQuery )
	: m_cacheQuery( cacheQuery )
{
	RED_ASSERT( m_cacheQuery );
	RED_ASSERT( !m_cacheQuery.GetEntry().m_isCube && m_cacheQuery.GetEntry().m_info.m_sliceCount <= 1 );
	RED_ASSERT( MapFormat( GetGpuTextureFormat() ) != SF::Render::Image_None );

}

CScaleformTextureCacheImage::~CScaleformTextureCacheImage()
{
}

SF::Render::ImageFormat CScaleformTextureCacheImage::GetFormat() const
{
	if ( !m_cacheQuery )
	{
		return SF::Render::Image_None;
	}

	return MapFormat( GetGpuTextureFormat() );
}

SF::Render::ImageSize CScaleformTextureCacheImage::GetSize() const
{
	if ( !m_cacheQuery )
	{
		return SF::Render::ImageSize( 0 );
	}

	return SF::Render::ImageSize( m_cacheQuery.GetEntry().m_info.m_baseWidth, m_cacheQuery.GetEntry().m_info.m_baseHeight );
}

unsigned CScaleformTextureCacheImage::GetMipmapCount() const
{
	if ( !m_cacheQuery )
	{
		return 0;
	}

	return m_cacheQuery.GetEntry().m_info.m_mipCount;
}

GpuApi::eTextureFormat CScaleformTextureCacheImage::GetGpuTextureFormat() const
{
	if ( !m_cacheQuery )
	{
		return GpuApi::TEXFMT_Max;
	}

	return ITexture::DecodeTextureFormat( m_cacheQuery.GetEntry().m_encodedFormat );
}

#endif
