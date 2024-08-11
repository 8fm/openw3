/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "terrainUtils.h"
#include "bitmapTexture.h"


namespace TerrainUtils
{

	struct BC1Block
	{
		union {
			struct
			{
				Uint16 color0;
				Uint16 color1;
				Uint32 indices;
			} block;
			TColorMapRawType raw;
		};


		static TColorMapType Convert16to32( Uint16 c )
		{
			Float rf = ( Float )( ( c >> 11 ) & 31 ) / 31.0f;
			Float gf = ( Float )( ( c >> 5  ) & 63 ) / 63.0f;
			Float bf = ( Float )( ( c >> 0  ) & 31 ) / 31.0f;
			Uint8 ri = ( Uint8 )Clamp( rf * 255.0f, 0.0f, 255.0f );
			Uint8 gi = ( Uint8 )Clamp( gf * 255.0f, 0.0f, 255.0f );
			Uint8 bi = ( Uint8 )Clamp( bf * 255.0f, 0.0f, 255.0f );
			return Color( ri, gi, bi ).ToUint32();
		}
	};


	TColorMapType DecodeColorMap( TColorMapRawType rawValue, Uint32 subX,  Uint32 subY )
	{
		// NOTE : We assume that we aren't dealing with any transparent values

		BC1Block bc1Block;
		bc1Block.raw = rawValue;

		TColorMapType color[4] = {};
		color[0] = BC1Block::Convert16to32( bc1Block.block.color0 );
		color[1] = BC1Block::Convert16to32( bc1Block.block.color1 );

		// Interpolate
		for ( Uint32 i = 0; i < 3; ++i )
		{
			Uint32 c0 = ( color[0] >> (i*8) ) & 0xff;
			Uint32 c1 = ( color[1] >> (i*8) ) & 0xff;

			color[2] |= ( ( ( 2*c0 + 1*c1 ) / 3 ) & 0xff ) << (i*8);
			color[3] |= ( ( ( 1*c0 + 2*c1 ) / 3 ) & 0xff ) << (i*8);
		}
		// Make sure alpha of the interpolated colors is set full.
		color[2] |= 0xff000000;
		color[3] |= 0xff000000;

		Uint32 indices = bc1Block.block.indices;

		Uint32 blockNum = subX + subY*4;
		Uint32 ind = ( indices >> blockNum ) & 3;

		return color[ind];
	}


	void DecompressColor( const TColorMapRawType* compressedData, TColorMapType* rgbaData, Uint32 texWidth, Uint32 texHeight )
	{
		if ( !CBitmapTexture::ConvertBuffer(texWidth, texHeight, CalcColorMapPitch( texWidth ), GpuApi::TEXFMT_BC1, compressedData,
			CalcColorMapSize( texWidth, texHeight ), GpuApi::TEXFMT_R8G8B8A8, rgbaData, GpuApi::CIH_None ) )
		{
			RED_HALT( "Failed to decompress terrain color map. This is only really available in editor builds." );
		}
	}


	void CompressColor( const TColorMapType* rgbaData, TColorMapRawType* compressedData, Uint32 texWidth, Uint32 texHeight )
	{
		// Use alphaThreshold of 0, so that even if the alpha value is 0, it won't be made transparent.
		if ( !CBitmapTexture::ConvertBuffer(texWidth, texHeight, texWidth*4, GpuApi::TEXFMT_R8G8B8A8, rgbaData, texWidth*texHeight*4,
			GpuApi::TEXFMT_BC1, compressedData, GpuApi::CIH_None, 0.0f ) )
		{
			RED_HALT( "Failed to compress terrain color map. This is only really available in editor builds." );
		}
	}

}
