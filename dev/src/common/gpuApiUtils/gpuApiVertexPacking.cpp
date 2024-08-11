/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApiUtils.h"


namespace GpuApi
{
	Float Dot3( Float x1, Float y1, Float z1, Float x2, Float y2, Float z2 )
	{
		return x1*x2 + y1*y2 + z1*z2;
	}

	void Cross( Float x1, Float y1, Float z1, Float x2, Float y2, Float z2, Float& x3, Float& y3, Float& z3 )
	{
		x3 = y1 * z2 - y2 * z1;
		y3 = x2 * z1 - x1 * z2;
		z3 = x1 * y2 - x2 * y1;
	}

	namespace VertexPacking
	{

		const PackingElement PackingElement::END_OF_ELEMENTS = { VertexPacking::PT_Invalid, VertexPacking::PS_Invalid, 0, 0, VertexPacking::ST_Invalid };


		//! Get stream float
		const Float* GetVertexStreamFloat( ePackingUsage usage, Uint8 usageIndex, const PackingVertexLayout& vertexLayout, const void* vertexData )
		{
			Uint32 i = usage * PackingVertexLayout::USAGE_INDEX_COUNT + usageIndex;
			ePackingType format = vertexLayout.m_streams[ i ].m_format;
#ifdef NO_GPU_ASSERTS
			RED_UNUSED( format );
#endif
			GPUAPI_ASSERT( format == PT_Float1 || format == PT_Float2 || format == PT_Float3 || format == PT_Float4 );
			const Uint8* offset = (const Uint8*) vertexData + vertexLayout.m_streams[ i ].m_offset;
			return (const Float*) offset;
		}

		//! Get stream float
		Float* GetVertexStreamFloatForWriting( ePackingUsage usage, Uint8 usageIndex, const PackingVertexLayout& vertexLayout, void* vertexData )
		{
			Uint32 i = usage * PackingVertexLayout::USAGE_INDEX_COUNT + usageIndex;
			ePackingType format = vertexLayout.m_streams[ i ].m_format;
#ifdef NO_GPU_ASSERTS
			RED_UNUSED( format );
#endif
			GPUAPI_ASSERT( format == PT_Float1 || format == PT_Float2 || format == PT_Float3 || format == PT_Float4 );
			const Uint8* offset = (const Uint8*) vertexData + vertexLayout.m_streams[ i ].m_offset;
			return (Float*) offset;
		}

		//! Get stream byte
		const Uint8* GetVertexStreamByte( ePackingUsage usage, Uint8 usageIndex, const PackingVertexLayout& vertexLayout, const void* vertexData )
		{
			Uint32 i = usage * PackingVertexLayout::USAGE_INDEX_COUNT + usageIndex;
			ePackingType format = vertexLayout.m_streams[ i ].m_format;
#ifdef NO_GPU_ASSERTS
			RED_UNUSED( format );
#endif
			GPUAPI_ASSERT( format == PT_UByte4 || format == PT_UByte4N || format == PT_Color );
			const Uint8* offset = (const Uint8*) vertexData + vertexLayout.m_streams[ i ].m_offset;
			return offset;
		}

		//! Get stream byte
		Uint8* GetVertexStreamByteForWriting( ePackingUsage usage, Uint8 usageIndex, const PackingVertexLayout& vertexLayout, void* vertexData )
		{
			Uint32 i = usage * PackingVertexLayout::USAGE_INDEX_COUNT + usageIndex;
			ePackingType format = vertexLayout.m_streams[ i ].m_format;
#ifdef NO_GPU_ASSERTS
			RED_UNUSED( format );
#endif
			GPUAPI_ASSERT( format == PT_UByte4 || format == PT_UByte4N || format == PT_Color );
			Uint8* offset = (Uint8*) vertexData + vertexLayout.m_streams[ i ].m_offset;
			return offset;
		}

		//! Pack position X
		Float PackPositionX( const PackingParams& params, Float val )
		{
			Float v = (val - params.m_positionPackOffsetX) * params.m_positionPackScaleX;
			if ( v < 0.0f ) v = 0.0f;
			if ( v > 1.0f ) v = 1.0f;
			return v;
		}

		//! Pack position Y
		Float PackPositionY( const PackingParams& params, Float val )
		{
			Float v = (val - params.m_positionPackOffsetY) * params.m_positionPackScaleY;
			if ( v < 0.0f ) v = 0.0f;
			if ( v > 1.0f ) v = 1.0f;
			return v;
		}

		//! Pack position Z
		Float PackPositionZ( const PackingParams& params, Float val )
		{
			Float v = (val - params.m_positionPackOffsetZ) * params.m_positionPackScaleZ;
			if ( v < 0.0f ) v = 0.0f;
			if ( v > 1.0f ) v = 1.0f;
			return v;
		}

		//! Unpack position X
		Float UnpackPositionX( const PackingParams& params, Float val )
		{
			return val / params.m_positionPackScaleX + params.m_positionPackOffsetX;
		}


		//! Unpack position Y
		Float UnpackPositionY( const PackingParams& params, Float val )
		{
			return val / params.m_positionPackScaleY + params.m_positionPackOffsetY;
		}

		//! Unpack position Z
		Float UnpackPositionZ( const PackingParams& params, Float val )
		{
			return val / params.m_positionPackScaleZ + params.m_positionPackOffsetZ;
		}

		//! Initialize packing params from range of values
		void InitPackingParams( PackingParams& packParams, Float minX, Float minY, Float minZ, Float maxX, Float maxY, Float maxZ )
		{
			const Float rangeX = maxX - minX;
			const Float rangeY = maxY - minY;
			const Float rangeZ = maxZ - minZ;

			packParams.m_positionPackOffsetX = minX;
			packParams.m_positionPackOffsetY = minY;
			packParams.m_positionPackOffsetZ = minZ;
			packParams.m_positionPackScaleX = (rangeX > 0.0001f) ? ( 1.0f / rangeX ) : 0.0f;
			packParams.m_positionPackScaleY = (rangeY > 0.0001f) ? ( 1.0f / rangeY ) : 0.0f;
			packParams.m_positionPackScaleZ = (rangeZ > 0.0001f) ? ( 1.0f / rangeZ ) : 0.0f;
		}


		Uint32 GetPackingTypeSize( ePackingType type )
		{
			switch ( type )
			{
			case PT_Float1:		return 4;
			case PT_Float2:		return 8;
			case PT_Float3:		return 12;
			case PT_Float4:		return 16;
			case PT_Float16_2:	return 4;
			case PT_Float16_4:	return 8;
			case PT_Color:		return 4;
			case PT_UByte1:		return 1;
			case PT_UByte4:		return 4;
			case PT_UByte4N:	return 4;
			case PT_Byte4N:		return 4;
			case PT_Dec4:		return 4;
			case PT_Short1:		return 2;
			case PT_Short2:		return 4;
			case PT_Short4:		return 8;
			case PT_Short4N:	return 8;
			case PT_UShort1:	return 2;
			case PT_UShort2:	return 4;
			case PT_UShort4:	return 8;
			case PT_UShort4N:	return 8;
			case PT_UInt1:		return 4;
			case PT_UInt2:		return 8;
			case PT_UInt3:		return 12;
			case PT_UInt4:		return 16;
			case PT_Int1:		return 4;
			case PT_Int2:		return 8;
			case PT_Int3:		return 12;
			case PT_Int4:		return 16;
			case PT_Index16:	return 2;
			case PT_Index32:	return 4;
			default:			GPUAPI_HALT( "Invalid element type" );
			}

			return 0;
		}


		//! Calculate size of element list
		Uint32 CalcElementListSize( const PackingElement* elements )
		{
			Uint32 totalSize = 0;

			while ( elements && elements->m_type != PT_Invalid )
			{
				totalSize += GetPackingTypeSize( elements->m_type );
				++elements;
			}

			return totalSize;
		}

		//! How many streams are used by these elements
		Uint32 GetStreamCount( const PackingElement* elements )
		{
			Int32 highestStream = -1;
			while ( elements && elements->m_type != PT_Invalid )
			{
				if ( elements->m_slot > highestStream )
				{
					highestStream = elements->m_slot;
				}
				++elements;
			}
			return highestStream + 1;
		}

		//! Calculate sizes of element list
		void CalcElementListSizes( const PackingElement* elements, Uint32* sizes, Uint32 numSizes )
		{
			while ( elements && elements->m_type != PT_Invalid )
			{
				// Make sure this element's slot isn't out of range. If it is, we won't write anything, but the caller probably needs to
				// provide a bigger buffer...
				RED_ASSERT( elements->m_slot < numSizes, TXT("PackingElement found with out-of-range slot index: %d >= %d"), elements->m_slot, numSizes );
				if ( elements->m_slot < numSizes )
				{
					sizes[elements->m_slot] += GetPackingTypeSize( elements->m_type );
				}
				++elements;
			}
		}

		//! Pack vertex data from vertex stream
		void PackElement( IBufferWriter* writer, const PackingElement& element, const PackingParams& params, const PackingVertexLayout& vertexLayout, const void* vertexData )
		{
			// Don't pack per-instance data...?
			if ( element.m_slotType == ST_PerInstance )
				return;


			if ( element.m_usage == PS_Position )
			{
				const Float* streamData = GetVertexStreamFloat( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float3 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
					writer->WriteFloat( streamData[2] );
				}
				else if ( element.m_type == PT_UShort4N )
				{
					// Pack position X
					writer->WriteUnsignedShortN( PackPositionX( params, streamData[0] ) );
					writer->WriteUnsignedShortN( PackPositionY( params, streamData[1] ) );
					writer->WriteUnsignedShortN( PackPositionZ( params, streamData[2] ) );
					writer->WriteUnsignedShortN( 1.0f );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for POSITION" );
				}
			}
			else if ( element.m_usage == PS_Normal || element.m_usage == PS_Tangent || element.m_usage == PS_Binormal )
			{
				const Float* streamData = GetVertexStreamFloat( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float3 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
					writer->WriteFloat( streamData[2] );
				}
				else if ( element.m_type == PT_UByte4N )
				{
					writer->WriteSignedByteN( streamData[0] );
					writer->WriteSignedByteN( streamData[1] );
					writer->WriteSignedByteN( streamData[2] );
					writer->WriteSignedByteN( 1.0f );
				}
				else if ( element.m_type == PT_Dec4 )
				{
					if ( element.m_usage == PS_Tangent )
					{
						Float sign = 1.0f;
						const Float* streamDataNormal = GetVertexStreamFloat( PS_Normal, 0, vertexLayout, vertexData );
						const Float* streamDataBinormal = GetVertexStreamFloat( PS_Binormal, 0, vertexLayout, vertexData );

						Float cbx=1.0f, cby=0.0f, cbz=0.0f;

						Cross( streamDataNormal[0], streamDataNormal[1], streamDataNormal[2], streamData[0], streamData[1], streamData[2], cbx, cby, cbz );

						if ( Dot3( cbx, cby, cbz, streamDataBinormal[0], streamDataBinormal[1], streamDataBinormal[2] ) < 0.0f )
						{
							sign = -1.0f;
						}

						writer->WriteDec4N( streamData[0]*.5f+.5f, streamData[1]*.5f+.5f, streamData[2]*.5f+.5f, sign*.5f+.5f );
					}
					else
					{
						writer->WriteDec3N( streamData[0]*.5f+.5f, streamData[1]*.5f+.5f, streamData[2]*.5f+.5f );
					}
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for NORMAL/TANGENT/BINORMAL" );
				}
			}
			else if ( element.m_usage == PS_TexCoord )
			{
				const Float* streamData = GetVertexStreamFloat( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float1 )
				{
					writer->WriteFloat( streamData[0] );
				}
				else if ( element.m_type == PT_Float2 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
				}
				else if ( element.m_type == PT_Float3 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
					writer->WriteFloat( streamData[2] );
				}
				else if ( element.m_type == PT_Float4 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
					writer->WriteFloat( streamData[2] );
					writer->WriteFloat( streamData[3] );
				}
				else if ( element.m_type == PT_Float16_2 )
				{
					writer->WriteFloat16( streamData[0] );
					writer->WriteFloat16( streamData[1] );
				}
				else if ( element.m_type == PT_Float16_4 )
				{
					writer->WriteFloat16( streamData[0] );
					writer->WriteFloat16( streamData[1] );
					writer->WriteFloat16( streamData[2] );
					writer->WriteFloat16( streamData[3] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for TEXCOORD" );
				}
			}
			else if ( element.m_usage == PS_Color )
			{
				const Uint8* streamData = GetVertexStreamByte( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float4 )
				{
					writer->WriteFloat( streamData[0] / 255.0f );
					writer->WriteFloat( streamData[1] / 255.0f );
					writer->WriteFloat( streamData[2] / 255.0f );
					writer->WriteFloat( streamData[3] / 255.0f );
				}
				else if ( element.m_type == PT_Color || element.m_type == PT_UByte4 || element.m_type == PT_UByte4N )
				{
					writer->WriteColor( *(const Uint32*)streamData );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for COLOR" );
				}
			}
			else if ( element.m_usage == PS_SkinIndices )
			{
				const Uint8* streamData = GetVertexStreamByte( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_UByte4 )
				{
					writer->WriteUnsignedByte( streamData[0] );
					writer->WriteUnsignedByte( streamData[1] );
					writer->WriteUnsignedByte( streamData[2] );
					writer->WriteUnsignedByte( streamData[3] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for BLENDINDICES" );
				}
			}
			else if ( element.m_usage == PS_SkinWeights )
			{
				const Float* streamData = GetVertexStreamFloat( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_UByte4 || element.m_type == PT_UByte4N )
				{
					writer->WriteUnsignedByteN( streamData[0] );
					writer->WriteUnsignedByteN( streamData[1] );
					writer->WriteUnsignedByteN( streamData[2] );
					writer->WriteUnsignedByteN( streamData[3] );
				}
				else if ( element.m_type == PT_Float4 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
					writer->WriteFloat( streamData[2] );
					writer->WriteFloat( streamData[3] );
				}
				else if ( element.m_type == PT_Float16_4 )
				{
					writer->WriteFloat16( streamData[0] );
					writer->WriteFloat16( streamData[1] );
					writer->WriteFloat16( streamData[2] );
					writer->WriteFloat16( streamData[3] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for BLENDWEIGHTS" );
				}
			}
			else if ( element.m_usage == PS_ExtraData )
			{
				const Float* streamData = GetVertexStreamFloat( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float4 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
					writer->WriteFloat( streamData[2] );
					writer->WriteFloat( streamData[3] );
				}
				else if ( element.m_type == PT_Float16_4 )
				{
					writer->WriteFloat16( streamData[0] );
					writer->WriteFloat16( streamData[1] );
					writer->WriteFloat16( streamData[2] );
					writer->WriteFloat16( streamData[3] );
				}
				else if ( element.m_type == PT_Float2 )
				{
					writer->WriteFloat( streamData[0] );
					writer->WriteFloat( streamData[1] );
				}
				else if ( element.m_type == PT_Float16_2 )
				{
					writer->WriteFloat16( streamData[0] );
					writer->WriteFloat16( streamData[1] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for EXTRA DATA" );
				}
			}
			else
			{
				GPUAPI_HALT( "Invalid packing element" );
			}
		}

		void UnpackElement( IBufferReader* reader, const PackingElement& element, const PackingParams& params, const PackingVertexLayout& vertexLayout, void* vertexData )
		{
			// Don't pack per-instance data...?
			if ( element.m_slotType == ST_PerInstance )
				return;


			if ( element.m_usage == PS_Position )
			{
				Float* streamData = GetVertexStreamFloatForWriting( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float3 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
					reader->ReadFloat( streamData[2] );
				}
				else if ( element.m_type == PT_UShort4N )
				{
					Float x, y, z, w;
					reader->ReadUnsignedShortN( x );
					reader->ReadUnsignedShortN( y );
					reader->ReadUnsignedShortN( z );
					reader->ReadUnsignedShortN( w ); // ignored
					streamData[0] = UnpackPositionX( params, x );
					streamData[1] = UnpackPositionY( params, y );
					streamData[2] = UnpackPositionZ( params, z );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for POSITION" );
				}
			}
			else if ( element.m_usage == PS_Normal || element.m_usage == PS_Tangent || element.m_usage == PS_Binormal )
			{
				Float* streamData = GetVertexStreamFloatForWriting( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float3 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
					reader->ReadFloat( streamData[2] );
				}
				else if ( element.m_type == PT_UByte4N )
				{
					Float dummy;
					reader->ReadSignedByteN( streamData[0] );
					reader->ReadSignedByteN( streamData[1] );
					reader->ReadSignedByteN( streamData[2] );
					reader->ReadSignedByteN( dummy );
				}
				else if ( element.m_type == PT_Dec4 )
				{
					if ( element.m_usage == PS_Tangent )
					{
						Float x, y, z, w;
 						reader->ReadDec4N( x, y, z, w );
						streamData[0] = ( x - .5f ) * 2.f;
						streamData[1] = ( y - .5f ) * 2.f;
						streamData[2] = ( z - .5f ) * 2.f;
					}
					else
					{
						Float x, y, z;
						reader->ReadDec3N( x, y, z );
						streamData[0] = ( x - .5f ) * 2.f;
						streamData[1] = ( y - .5f ) * 2.f;
						streamData[2] = ( z - .5f ) * 2.f;
					}
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for NORMAL/TANGENT/BINORMAL" );
				}
			}
			else if ( element.m_usage == PS_TexCoord )
			{
				Float* streamData = GetVertexStreamFloatForWriting( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float1 )
				{
					reader->ReadFloat( streamData[0] );
				}
				else if ( element.m_type == PT_Float2 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
				}
				else if ( element.m_type == PT_Float3 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
					reader->ReadFloat( streamData[2] );
				}
				else if ( element.m_type == PT_Float4 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
					reader->ReadFloat( streamData[2] );
					reader->ReadFloat( streamData[3] );
				}
				else if ( element.m_type == PT_Float16_2 )
				{
					reader->ReadFloat16( streamData[0] );
					reader->ReadFloat16( streamData[1] );
				}
				else if ( element.m_type == PT_Float16_4 )
				{
					reader->ReadFloat16( streamData[0] );
					reader->ReadFloat16( streamData[1] );
					reader->ReadFloat16( streamData[2] );
					reader->ReadFloat16( streamData[3] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for TEXCOORD" );
				}
			}
			else if ( element.m_usage == PS_Color )
			{
				Uint8* streamData = GetVertexStreamByteForWriting( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float4 )
				{
					Float r, g, b, a;
					reader->ReadFloat( r );
					reader->ReadFloat( g );
					reader->ReadFloat( b );
					reader->ReadFloat( a );
					streamData[0] = (Uint8)( r * 255.f );
					streamData[1] = (Uint8)( g * 255.f );
					streamData[2] = (Uint8)( b * 255.f );
					streamData[3] = (Uint8)( a * 255.f );
				}
				else if ( element.m_type == PT_Color || element.m_type == PT_UByte4 || element.m_type == PT_UByte4N )
				{
					reader->ReadColor( *(Uint32*)streamData );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for COLOR" );
				}
			}
			else if ( element.m_usage == PS_SkinIndices )
			{
				Uint8* streamData = GetVertexStreamByteForWriting( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_UByte4 )
				{
					reader->ReadUnsignedByte( streamData[0] );
					reader->ReadUnsignedByte( streamData[1] );
					reader->ReadUnsignedByte( streamData[2] );
					reader->ReadUnsignedByte( streamData[3] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for BLENDINDICES" );
				}
			}
			else if ( element.m_usage == PS_SkinWeights )
			{
				Float* streamData = GetVertexStreamFloatForWriting( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_UByte4 || element.m_type == PT_UByte4N )
				{
					reader->ReadUnsignedByteN( streamData[0] );
					reader->ReadUnsignedByteN( streamData[1] );
					reader->ReadUnsignedByteN( streamData[2] );
					reader->ReadUnsignedByteN( streamData[3] );
				}
				else if ( element.m_type == PT_Float4 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
					reader->ReadFloat( streamData[2] );
					reader->ReadFloat( streamData[3] );
				}
				else if ( element.m_type == PT_Float16_4 )
				{
					reader->ReadFloat16( streamData[0] );
					reader->ReadFloat16( streamData[1] );
					reader->ReadFloat16( streamData[2] );
					reader->ReadFloat16( streamData[3] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for BLENDWEIGHTS" );
				}
			}
			else if ( element.m_usage == PS_ExtraData )
			{
				Float* streamData = GetVertexStreamFloatForWriting( element.m_usage, element.m_usageIndex, vertexLayout, vertexData );
				if ( element.m_type == PT_Float4 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
					reader->ReadFloat( streamData[2] );
					reader->ReadFloat( streamData[3] );
				}
				else if ( element.m_type == PT_Float16_4 )
				{
					reader->ReadFloat16( streamData[0] );
					reader->ReadFloat16( streamData[1] );
					reader->ReadFloat16( streamData[2] );
					reader->ReadFloat16( streamData[3] );
				}
				else if ( element.m_type == PT_Float2 )
				{
					reader->ReadFloat( streamData[0] );
					reader->ReadFloat( streamData[1] );
				}
				else if ( element.m_type == PT_Float16_2 )
				{
					reader->ReadFloat16( streamData[0] );
					reader->ReadFloat16( streamData[1] );
				}
				else
				{
					GPUAPI_HALT( "Invalid packing format for EXTRA DATA" );
				}
			}
			else
			{
				GPUAPI_HALT( "Invalid packing element" );
			}
		}

		//! Pack vertices into buffer
		void PackVertices( IBufferWriter* writer, const PackingElement* elements, const PackingParams& packParams, const PackingVertexLayout& vertexLayout, const void* vertexData, Uint32 numVertices, Uint32 slotIndex )
		{
			GPUAPI_ASSERT( vertexLayout.m_stride != 0 );

			// Get data ptr
			const Uint8* vertexPtr = ( const Uint8* ) vertexData;

			// Pack vertices
			for ( Uint32 i=0; i<numVertices; ++i )
			{
				// Pack all elements
				const PackingElement* packElement = elements;
				while ( packElement && packElement->m_type != PT_Invalid )
				{
					if ( packElement->m_slot == slotIndex)
					{
						PackElement( writer, *packElement, packParams, vertexLayout, vertexPtr );
					}
					++packElement;
				}

				// Move to next vertex
				vertexPtr += vertexLayout.m_stride;
			}
		}

		void UnpackVertices( IBufferReader* reader,  const PackingElement* packingElement, const PackingParams& packParams, const PackingVertexLayout& vertexLayout, void* vertexData, Uint32 numVertices, Uint32 slotIndex )
		{
			GPUAPI_ASSERT( vertexLayout.m_stride != 0 );

			Uint8* vertexPtr = ( Uint8* ) vertexData;

			// Unpack vertices
			for ( Uint32 i=0; i<numVertices; ++i )
			{
				for ( auto elem = packingElement; elem && elem->m_type != GpuApi::VertexPacking::PT_Invalid; ++elem )
				{
					if ( elem->m_slot == slotIndex )
					{
						UnpackElement( reader, *elem, packParams, vertexLayout, vertexPtr );
					}
				}

				// Move to next vertex
				vertexPtr += vertexLayout.m_stride;
			}
		}
	
		//! Pack indices into buffer
		void PackIndices( IBufferWriter* writer, ePackingType type, const void* indexData, Uint32 numIndices, const Uint32 baseVertexIndex )
		{
			if ( type == PT_Index16 )
			{
				writer->Align( sizeof( Uint16 ) );

				const Uint16* indexPtr = ( const Uint16* ) indexData;
				for ( Uint32 i=0; i<numIndices; ++i )
				{
					RED_FATAL_ASSERT( (Uint32)*indexPtr + baseVertexIndex < 65536, "Index buffer indices were wrapper" );
					writer->WriteUnsignedShort( *indexPtr + baseVertexIndex );
					++indexPtr;
				}
			}
			else if ( type == PT_Index32 )
			{
				writer->Align( sizeof( Uint32 ) );

				const Uint32* indexPtr = ( const Uint32* ) indexData;
				for ( Uint32 i=0; i<numIndices; ++i )
				{
					RED_FATAL_ASSERT( (Uint32)*indexPtr + baseVertexIndex < 65536, "Index buffer indices were wrapper" );
					writer->WriteUnsignedInt( *indexPtr + baseVertexIndex );
					++indexPtr;
				}
			}
			else
			{
				GPUAPI_HALT( "Invalid index pack format" );
			}
		}
	}
}
