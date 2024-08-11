/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "common.h"
#include <d3d11.h>
#include <xg.h>

#include "../../src/common/gpuApiDX10/gpuApiMapping.h"

#pragma comment( lib, "xg.lib" )


Bool DoProcess( ProcessingData& data )
{
	const Uint16 mipCount = data.mipCount;;
	const Uint16 texCount = data.texCount;
	const Uint16 baseWidth = data.baseWidth;
	const Uint16 baseHeight = data.baseHeight;

	// Always use 1D tiling for resource textures. When we get optimal from XG, it can change depending on mips and size,
	// which causes problems if we need to stream in a subset of the mips.
	XG_TILE_MODE tileMode = XG_TILE_MODE_1D_THIN;

	XG_TEXTURE2D_DESC texDesc;
	texDesc.Width = baseWidth;
	texDesc.Height = baseHeight;
	texDesc.MipLevels = mipCount;
	texDesc.ArraySize = texCount;
	texDesc.Format = (XG_FORMAT)GpuApi::Map( data.texFormat );
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = XG_USAGE_IMMUTABLE;
	texDesc.BindFlags = XG_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	if ( data.texType == GpuApi::TEXTYPE_CUBE )
	{
		texDesc.MiscFlags |= XG_RESOURCE_MISC_TEXTURECUBE;
	}
	texDesc.TileMode = tileMode;
	texDesc.Pitch = 0;

	XGTextureAddressComputer* computer = nullptr;
	HRESULT hr = XGCreateTexture2DComputer( &texDesc, &computer );
	if ( FAILED( hr ) )
	{
		GOutput << "Failed to create texture computer" << std::endl;
		return false;
	}


	XG_RESOURCE_LAYOUT layout;
	hr = computer->GetResourceLayout( &layout );
	if ( FAILED( hr ) )
	{
		GOutput << "Failed to get resource layout" << std::endl;
		SAFE_RELEASE( computer );
		return false;
	}


	data.AllocateOutput( layout.SizeBytes, layout.BaseAlignmentBytes );
	if ( data.output == nullptr )
	{
		GOutput << "Failed to allocate temporary memory: " << layout.SizeBytes << ", alignment: " << layout.BaseAlignmentBytes << std::endl;
		SAFE_RELEASE( computer );
		return false;
	}
	ZeroMemory( data.output, layout.SizeBytes );

	for ( Uint16 tex_i = 0; tex_i < texCount; ++tex_i )
	{
		TexData& texData = data.texs[ tex_i ];

		for ( Uint16 mip_i = 0; mip_i < mipCount; ++mip_i )
		{
			MipData& mipData = texData.mips[ mip_i ];
			const void* sourceMipData = mipData.data;
			Uint32 sourcePitch = mipData.pitch;

			if ( sourceMipData == nullptr )
			{
				GOutput << "No texture data for mip " << mip_i << " slice " << tex_i << std::endl;
				SAFE_RELEASE( computer );
				return false;
			}

			hr = computer->CopyIntoSubresource( data.output, 0, D3D11CalcSubresource( mip_i, tex_i, mipCount ), sourceMipData, sourcePitch, mipData.size );
			if ( FAILED( hr ) )
			{
				GOutput << "Failed to copy mip " << mip_i << " slice " << tex_i << std::endl;
				SAFE_RELEASE( computer );
				return false;
			}
		}
	}

	for ( Uint16 mip_i = 0; mip_i < mipCount; ++mip_i )
	{
		data.mipOffsets[ mip_i ] = static_cast< Uint32 >( computer->GetMipLevelOffsetBytes( 0, mip_i ) );
	}

	SAFE_RELEASE( computer );

	return true;
}
