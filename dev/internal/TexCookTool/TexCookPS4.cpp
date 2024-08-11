/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "common.h"
#include <gnm.h>
#include <gnmx.h>

#include "../../src/common/gpuApiGnm/gpuApiMapping.h"


#pragma comment( lib, "libSceGnm.lib" )
#pragma comment( lib, "libSceGnmx.lib" )
#pragma comment( lib, "libSceGpuAddress.lib" )


Bool DoProcess( ProcessingData& data )
{
	// Create texture, same as GpuApi Gnm, except we can assume that this isn't going to be a render target or anything like that.
	sce::Gnm::Texture tempTex;
	sce::Gnm::DataFormat format = GpuApi::Map( data.texFormat );
	sce::GpuAddress::SurfaceType surfaceType = sce::GpuAddress::kSurfaceTypeTextureFlat;

	if ( data.texType == GpuApi::TEXTYPE_CUBE )
	{
		surfaceType = sce::GpuAddress::kSurfaceTypeTextureCubemap;
	}

	sce::Gnm::TileMode tileMode;
	if ( sce::GpuAddress::computeSurfaceTileMode( &tileMode, surfaceType, format, 1 ) != sce::GpuAddress::kStatusSuccess )
	{
		GOutput << "Cannot compute the tile mode for the texture." << std::endl;
		tileMode = sce::Gnm::kTileModeDisplay_LinearAligned;
	}

	sce::Gnm::SizeAlign sizeAlign;

	switch ( data.texType )
	{
	case GpuApi::TEXTYPE_2D:
		sizeAlign = tempTex.initAs2d( data.baseWidth, data.baseHeight, data.mipCount, format, tileMode, sce::Gnm::kNumFragments1 );
		break;
	case GpuApi::TEXTYPE_ARRAY:
		sizeAlign = tempTex.initAs2dArray( data.baseWidth, data.baseHeight, data.texCount, data.mipCount, format, tileMode, sce::Gnm::kNumFragments1, false );
		break;
	case GpuApi::TEXTYPE_CUBE:
		sizeAlign = tempTex.initAs2dArray( data.baseWidth, data.baseHeight, data.texCount / 6, data.mipCount, format, tileMode, sce::Gnm::kNumFragments1, true);
		break;
	default:
		GOutput << "Invalid texture type: " << (int)data.texType << std::endl;
		return false;
	}

	Int32 gnmRes;

	data.AllocateOutput( sizeAlign.m_size, sizeAlign.m_align );
	if ( data.output == nullptr )
	{
		GOutput << "Failed to allocate temporary memory: " << sizeAlign.m_size << ", alignment: " << sizeAlign.m_align << std::endl;
		return false;
	}
	ZeroMemory( data.output, sizeAlign.m_size );

	for ( Uint16 tex_i = 0; tex_i < data.texCount; ++tex_i )
	{
		TexData& texData = data.texs[ tex_i ];

		for ( Uint16 mip_i = 0; mip_i < data.mipCount; ++mip_i )
		{
			MipData& mipData = texData.mips[ mip_i ];

			uint64_t tiledOffset;
			uint64_t tiledSize;
			gnmRes = sce::GpuAddress::computeTextureSurfaceOffsetAndSize( &tiledOffset, &tiledSize, &tempTex, mip_i, tex_i );
			if ( gnmRes != sce::GpuAddress::kStatusSuccess )
			{
				GOutput << "Failed to get surface offset/size for mip " << mip_i << " slice " << tex_i << std::endl;
				return false;
			}

			sce::GpuAddress::TilingParameters tilingParams;
			gnmRes = tilingParams.initFromTexture( &tempTex, mip_i, tex_i );
			if ( gnmRes != sce::GpuAddress::kStatusSuccess )
			{
				GOutput << "Failed to initialize tiling params for mip " << mip_i << " slice " << tex_i << std::endl;
				return false;
			}

			uint64_t untiledSize;
			sce::Gnm::AlignmentType untiledAlign;
			gnmRes = sce::GpuAddress::computeUntiledSurfaceSize( &untiledSize, &untiledAlign, &tilingParams );
			if ( gnmRes != sce::GpuAddress::kStatusSuccess )
			{
				GOutput << "Failed to compute untiled surface size for mip " << mip_i << " slice " << tex_i << std::endl;
				return false;
			}

			void* tiledMipData = (Uint8*)data.output + tiledOffset;

			Uint32 mipDataSize = mipData.size;
			Uint32 mipDataPitch = mipData.pitch;

			const void* sourceMipData = mipData.data;
			if ( sourceMipData == nullptr )
			{
				GOutput << "No texture data for mip " << mip_i << " slice " << tex_i << std::endl;
				return false;
			}

			if ( untiledSize == mipDataSize )
			{
				gnmRes = sce::GpuAddress::tileSurface( tiledMipData, sourceMipData, &tilingParams );
			}
			else
			{
				// In some cases the size in the source can be different from what we're expecting. In particular, pitch is
				// clamped to 4 bytes, so a 2x2 8-bit texture for example will be larger. In these cases, we have to work around
				// it a bit so we don't end up with the wrong data in the tiled surface.
				sce::GpuAddress::SurfaceRegion destRegion;
				destRegion.m_left = 0;
				destRegion.m_right = tilingParams.m_linearWidth;
				destRegion.m_top = 0;
				destRegion.m_bottom = tilingParams.m_linearHeight;
				destRegion.m_front = 0;
				destRegion.m_back = 1;

				Uint32 bitsPerElement = format.getBitsPerElement();

				Uint32 srcPitchElems = mipDataPitch * 8 / bitsPerElement;
				Uint32 srcSizeElems = mipDataSize * 8 / bitsPerElement;
				gnmRes = sce::GpuAddress::tileSurfaceRegion( tiledMipData, sourceMipData, &tilingParams, &destRegion, srcPitchElems, srcSizeElems );
			}

			if ( gnmRes != sce::GpuAddress::kStatusSuccess )
			{
				GOutput << "Failed to tile mip " << mip_i << " slice " << tex_i << std::endl;
				return false;
			}
		}
	}

	for ( Uint16 mip_i = 0; mip_i < data.mipCount; ++mip_i )
	{
		uint64_t tiledOffset;
		uint64_t tiledSize;
		gnmRes = sce::GpuAddress::computeTextureSurfaceOffsetAndSize( &tiledOffset, &tiledSize, &tempTex, mip_i, 0 );
		if ( gnmRes != sce::GpuAddress::kStatusSuccess )
		{
			GOutput << "Failed to get surface offset/size for mip " << mip_i << std::endl;
			return false;
		}

		data.mipOffsets[ mip_i ] = static_cast< Uint32 >( tiledOffset );
	}

	return true;
}
