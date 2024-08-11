/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "terrainTypes.h"

namespace TerrainUtils
{

	// Get byte pitch of a color map.
	RED_INLINE Uint32 CalcColorMapPitch( Uint32 texelWidth )
	{
		return texelWidth * 2;
	}
	// Get total byte size of a color map.
	RED_INLINE Uint32 CalcColorMapSize( Uint32 texelWidth, Uint32 texelHeight )
	{
		return texelWidth * texelHeight / 2;
	}

	// Convert a texel coordinate to compression block, rounding down.
	RED_INLINE Uint32 ColorMapTexelToBlock( Uint32 texel )
	{
		return texel / 4;
	}
	// Convert a texel coordinate to compression block, rounding up.
	RED_INLINE Uint32 ColorMapTexelToBlockCeil( Uint32 texel )
	{
		return ( texel + 3 ) / 4;
	}

	// Convert compression block to texel coordinate.
	RED_INLINE Uint32 ColorMapBlockToTexel( Uint32 block )
	{
		return block * 4;
	}

	// Convert texel coordinate to the subtexel within a compression block.
	RED_INLINE Uint32 ColorMapTexelToSubTexel( Uint32 texel )
	{
		return texel & 3;
	}

	// Check if the given texel size is exactly a multiple of block size (or, check if coordinate is at the start of a block).
	RED_INLINE Bool IsExactlyColorMapBlock( Uint32 texel )
	{
		return ( texel & 3 ) == 0;
	}
	// Adjust a texel coordinate so that it is at the start of its compression block.
	RED_INLINE Uint32 MakeExactlyColorMapBlock( Uint32 texel )
	{
		return texel &= ~3;
	}

	// Given a compression block and subtexel coordinates within that block, extract the RGBA color.
	TColorMapType DecodeColorMap( TColorMapRawType rawValue, Uint32 subX,  Uint32 subY );



	// Decompress colormap data. texWidth and texHeight should be proper block-aligned sizes. rgbaData should already be allocated,
	// with at least texWidth*texHeight*4 bytes.
	void DecompressColor( const TColorMapRawType* compressedData, TColorMapType* rgbaData, Uint32 texWidth, Uint32 texHeight );

	// Compress colormap data. texWidth and texHeight should be proper block-aligned sizes. compressedData should already be allocated,
	// with at least CalcColorMapSize( texWidth, texHeight ) bytes.
	void CompressColor( const TColorMapType* rgbaData, TColorMapRawType* compressedData, Uint32 texWidth, Uint32 texHeight );



	struct StampMixHeightReplace
	{
		RED_INLINE Uint16 operator()( Uint16 current, Uint16 stamp, Float scale, Float offset, Float falloff )
		{
			return ( Uint16 )Lerp< Float >( falloff, current, Clamp( stamp * scale + offset, 0.0f, 65535.0f ) );
		}
	};

	struct StampMixHeightAdd
	{
		RED_INLINE Uint16 operator()( Uint16 current, Uint16 stamp, Float scale, Float offset, Float falloff )
		{
			return ( Uint16 )Clamp< Float >( current + ( stamp * scale + offset ) * falloff, 0, 65535 );
		}
	};

	struct StampMixControl
	{
		RED_INLINE TControlMapType operator()( TControlMapType /*current*/, TControlMapType stamp, Float /*scale*/, Float /*offset*/, Float /*falloff*/ )
		{
			return stamp;
		}
	};

	struct StampMixColor
	{
		RED_INLINE TColorMapType operator()( TColorMapType current, TColorMapType stamp, Float /*scale*/, Float /*offset*/, Float falloff )
		{
			// In general, a bad way to deal with uint32 colors, but we aren't concerned about specific
			// channel ordering, so it's fine here.
			Uint8* dst = (Uint8*)&current;
			Uint8* src = (Uint8*)&stamp;

			Uint8 blended[4];
			for ( Uint32 i = 0; i < 4; ++i )
			{
				blended[i] = ( Uint8 )MRound( Lerp( falloff, ( Float )dst[i], ( Float )src[i] ) );
			}

			return *(TColorMapType*)blended;
		}
	};



	// Basic interpolation of heightmap values.
	struct FilterHeightMap
	{
		RED_INLINE Uint16 operator()( Uint16 uv00, Uint16 uv10, Uint16 uv01, Uint16 uv11, Float u, Float v )
		{
			return ( Uint16 )( uv00*(1-u)*(1-v) + uv01*(1-u)*v + uv10*u*(1-v) + uv11*u*v );
		}
	};

	// Filter for control map. Since it can't be interpolated, just selects the one with greatest weight.
	struct FilterControlMap
	{
		RED_INLINE TControlMapType operator()( TControlMapType uv00, TControlMapType uv10, TControlMapType uv01, TControlMapType uv11, Float u, Float v )
		{
			if ( u < 0.5f )
			{
				if ( v < 0.5f )	return uv00;
				else			return uv01;
			}
			else
			{
				if ( v < 0.5f )	return uv10;
				else			return uv11;
			}
		}
	};

	// Interpolation of color values.
	struct FilterColorMap
	{
		RED_INLINE TColorMapType operator()( TColorMapType uv00, TColorMapType uv10, TColorMapType uv01, TColorMapType uv11, Float u, Float v )
		{
			Float w0 = (1-u)*(1-v);
			Float w1 = (u)*(1-v);
			Float w2 = (1-u)*(v);
			Float w3 = (u)*(v);

			// In general, a bad way to deal with uint32 colors, but we aren't concerned about specific
			// channel ordering, so it's fine here.
			Uint8* c0 = (Uint8*)&uv00;
			Uint8* c1 = (Uint8*)&uv10;
			Uint8* c2 = (Uint8*)&uv01;
			Uint8* c3 = (Uint8*)&uv11;

			Uint8 blended[4];
			for ( Uint32 i = 0; i < 4; ++i )
			{
				blended[i] = ( Uint8 )MRound( c0[i]*w0 + c1[i]*w1 + c2[i]*w2 + c3[i]*w3 );
			}

			return *(Uint32*)blended;
		}
	};


	template< typename T, typename _Filter >
	T Sample( const T* buffer, Float x, Float y, Uint32 bufferWidth, Uint32 bufferHeight )
	{
		Int32 srcXbase = ( Int32 )x;
		Int32 srcYbase = ( Int32 )y;
		Int32 srcXbase_1 = Min< Int32 >( srcXbase + 1, bufferWidth - 1 );
		Int32 srcYbase_1 = Min< Int32 >( srcYbase + 1, bufferHeight - 1 );

		const T& v0 = buffer[ srcYbase * bufferWidth + srcXbase ];
		const T& v1 = buffer[ srcYbase * bufferWidth + srcXbase_1 ];
		const T& v2 = buffer[ srcYbase_1 * bufferWidth + srcXbase ];
		const T& v3 = buffer[ srcYbase_1 * bufferWidth + srcXbase_1 ];

		Float srcXfrac = x - ( Float )srcXbase;
		Float srcYfrac = y - ( Float )srcYbase;

		return _Filter()( v0, v1, v2, v3, srcXfrac, srcYfrac );
	}



	template< typename TYPE >
	void CopySubBuffer( TYPE* dstBuffer, const Rect& dstRect, const Uint32 dstWidth, const TYPE* srcBuffer, const Rect& srcRect, const Uint32 srcWidth )
	{
		ASSERT( dstBuffer );
		ASSERT( srcBuffer );
		ASSERT( dstRect.Width() == srcRect.Width() );
		ASSERT( dstRect.Height() == srcRect.Height() );

		Int32 dstRow = dstRect.m_top;
		Int32 srcRow = srcRect.m_top;

		Uint32 dataSizeToCopy = dstRect.Width() * sizeof( TYPE );

		for ( Int32 row = 0; row < srcRect.Height(); ++row )
		{
			Uint32 dstIndex = dstRow * dstWidth + dstRect.m_left;
			Uint32 srcIndex = srcRow * srcWidth + srcRect.m_left;

			void* dst = (void*)(dstBuffer + dstIndex);
			const void* src = (const void*)(srcBuffer + srcIndex);

			Red::System::MemoryCopy( dst, src, dataSizeToCopy );

			++dstRow;
			++srcRow;
		}
	}

}
