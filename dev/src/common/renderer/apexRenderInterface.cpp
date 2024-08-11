#include "build.h"
#include "apexRenderInterface.h"
#include "renderProxyApex.h"
#include "renderElementApex.h"
#include "renderInterface.h"
#include "renderMaterial.h"
#include "renderShaderPair.h"
#include "..\engine\renderFragment.h"

#ifdef USE_APEX

#include "..\engine\apexMaterialMapping.h"

#include "NxUserRenderResourceDesc.h"
#include "NxUserRenderVertexBufferDesc.h"

#define APEX_BONE_BUFFERS_MAX_BONES					128
#define APEX_BONE_BUFFERS_BYTES_PER_MATRIX			sizeof(Matrix)

#ifndef RED_FINAL_BUILD
extern SceneRenderingStats GRenderingStats;
#endif
using namespace physx;
using namespace physx::apex;

namespace
{
	GpuApi::ePrimitiveType MapApexPrimitiveToGpuApi( NxRenderPrimitiveType::Enum primitiveType )
	{
		switch ( primitiveType )
		{
		case NxRenderPrimitiveType::TRIANGLES:		return GpuApi::PRIMTYPE_TriangleList;
		case NxRenderPrimitiveType::TRIANGLE_STRIP:	return GpuApi::PRIMTYPE_TriangleStrip;
		case NxRenderPrimitiveType::LINES:			return GpuApi::PRIMTYPE_LineList;
		case NxRenderPrimitiveType::LINE_STRIP:		return GpuApi::PRIMTYPE_LineStrip;
		case NxRenderPrimitiveType::POINTS:			return GpuApi::PRIMTYPE_PointList;

		case NxRenderPrimitiveType::POINT_SPRITES:	// Fall through, Point Sprites not supported.
		default:
			RED_HALT( "Apex primitive type unsupported: %d.", primitiveType );
			return GpuApi::PRIMTYPE_Invalid;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// CApexVertexBuffer
//////////////////////////////////////////////////////////////////////////

static GpuApi::VertexPacking::ePackingType MapApexFormatToPackingType( NxRenderDataFormat::Enum format )
{
	switch(format)
	{
	case NxRenderDataFormat::FLOAT1:		return GpuApi::VertexPacking::PT_Float1;
	case NxRenderDataFormat::FLOAT2:		return GpuApi::VertexPacking::PT_Float2;
	case NxRenderDataFormat::FLOAT3:		return GpuApi::VertexPacking::PT_Float3;
	case NxRenderDataFormat::FLOAT4:		return GpuApi::VertexPacking::PT_Float4;
	case NxRenderDataFormat::UBYTE4:		return GpuApi::VertexPacking::PT_UByte4;
	case NxRenderDataFormat::USHORT1:		return GpuApi::VertexPacking::PT_UShort1;
	case NxRenderDataFormat::USHORT2:		return GpuApi::VertexPacking::PT_UShort2;
	case NxRenderDataFormat::USHORT4:		return GpuApi::VertexPacking::PT_UShort4;
	case NxRenderDataFormat::BYTE_SNORM3:	return GpuApi::VertexPacking::PT_Byte4N;
	case NxRenderDataFormat::BYTE_SNORM4:	return GpuApi::VertexPacking::PT_Byte4N;
	case NxRenderDataFormat::B8G8R8A8:		return GpuApi::VertexPacking::PT_Color;
	case NxRenderDataFormat::R8G8B8A8:		return GpuApi::VertexPacking::PT_Color;

	default:
		RED_HALT( "Apex format unsupported: %d. Should perhaps add support for it.", format );
		return GpuApi::VertexPacking::PT_Invalid;
	}
}

static Bool MapApexSemanticToPackingUsage( NxRenderVertexSemantic::Enum semantic, GpuApi::VertexPacking::ePackingUsage& outUsage, Uint8& outUsageIndex )
{
	switch ( semantic )
	{
	case physx::apex::NxRenderVertexSemantic::CUSTOM:		outUsage = GpuApi::VertexPacking::PS_TexCoord;		outUsageIndex = 4;		break;
	case physx::apex::NxRenderVertexSemantic::POSITION:		outUsage = GpuApi::VertexPacking::PS_Position;		outUsageIndex = 0;		break;
	case physx::apex::NxRenderVertexSemantic::NORMAL:		outUsage = GpuApi::VertexPacking::PS_Normal;		outUsageIndex = 0;		break;
	case physx::apex::NxRenderVertexSemantic::TANGENT:		outUsage = GpuApi::VertexPacking::PS_Tangent;		outUsageIndex = 0;		break;
	case physx::apex::NxRenderVertexSemantic::BINORMAL:		outUsage = GpuApi::VertexPacking::PS_Binormal;		outUsageIndex = 0;		break;
	case physx::apex::NxRenderVertexSemantic::COLOR:		outUsage = GpuApi::VertexPacking::PS_Color;			outUsageIndex = 0;		break;

	case physx::apex::NxRenderVertexSemantic::TEXCOORD0:	outUsage = GpuApi::VertexPacking::PS_TexCoord;		outUsageIndex = 0;		break;
	case physx::apex::NxRenderVertexSemantic::TEXCOORD1:	outUsage = GpuApi::VertexPacking::PS_TexCoord;		outUsageIndex = 1;		break;
	case physx::apex::NxRenderVertexSemantic::TEXCOORD2:	outUsage = GpuApi::VertexPacking::PS_TexCoord;		outUsageIndex = 2;		break;
	case physx::apex::NxRenderVertexSemantic::TEXCOORD3:	outUsage = GpuApi::VertexPacking::PS_TexCoord;		outUsageIndex = 3;		break;

	case physx::apex::NxRenderVertexSemantic::BONE_INDEX:	outUsage = GpuApi::VertexPacking::PS_SkinIndices;	outUsageIndex = 0;		break;
	case physx::apex::NxRenderVertexSemantic::BONE_WEIGHT:	outUsage = GpuApi::VertexPacking::PS_SkinWeights;	outUsageIndex = 0;		break;

	case physx::apex::NxRenderVertexSemantic::DISPLACEMENT_TEXCOORD:
	case physx::apex::NxRenderVertexSemantic::DISPLACEMENT_FLAGS:
	default:
		RED_HALT( "Apex vertex semantic unsupported: %d", semantic );
		return false;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////


#ifdef APEX_ENABLE_DEBUG_VISUALIZATION

CApexDebugVisualizerResource::CApexDebugVisualizerResource( const physx::apex::NxUserRenderResourceDesc& rrDesc )
	: CApexDebugVisualizerResourceBase( rrDesc )
{
	for ( Uint32 i = 0; i < rrDesc.numVertexBuffers; ++i )
	{
		CApexVertexBuffer* vb = static_cast< CApexVertexBuffer* >( rrDesc.vertexBuffers[ i ] );
		RED_FATAL_ASSERT( vb->m_debugVisRenderResource == nullptr, "Vertex buffer already has a debug vis resource" );
		vb->m_debugVisRenderResource = this;
	}
}

CApexDebugVisualizerResource::~CApexDebugVisualizerResource()
{
}


#endif // APEX_ENABLE_DEBUG_VISUALIZATION


//////////////////////////////////////////////////////////////////////////


CApexVertexBuffer::CApexVertexBuffer( const NxUserRenderVertexBufferDesc& vbDesc )
	: m_desc( vbDesc )
	, m_lockedBuffer( nullptr )
	, m_hasOneSemantic( false )
	, m_stride( 0 )
#ifndef RED_FINAL_BUILD
	, m_validRangeBegin( 0 )
	, m_validRangeEnd( 0 )
#endif
#ifdef APEX_ENABLE_DEBUG_VISUALIZATION
	, m_debugVisRenderResource( nullptr )
#endif
{
	Uint32 elementIndex = 0;

	for (Uint32 i = 0; i < physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		physx::apex::NxRenderVertexSemantic::Enum	apexSemantic = physx::apex::NxRenderVertexSemantic::Enum(i);
		physx::apex::NxRenderDataFormat::Enum		apexFormat = vbDesc.buffersRequest[i];

		m_semanticElements[i] = -1;

		if ( apexFormat == physx::apex::NxRenderDataFormat::UNSPECIFIED )
		{
			continue;
		}

		GpuApi::VertexPacking::ePackingUsage packUsage;
		Uint8 packUsageIndex;
		GpuApi::VertexPacking::ePackingType packType;
		if ( !MapApexSemanticToPackingUsage( apexSemantic, packUsage, packUsageIndex ) )
		{
			continue;
		}

		packType = MapApexFormatToPackingType( apexFormat );

		// Some semantics need special handling...
		switch ( packUsage )
		{
		case GpuApi::VertexPacking::PS_Tangent:
			if( packType == GpuApi::VertexPacking::PT_Float3 )
			{
				// always use FLOAT4 for tangents!!!
				packType = GpuApi::VertexPacking::PT_Float4;
			}
			break;
		case GpuApi::VertexPacking::PS_SkinIndices:
			// use either USHORT1 for destruction or USHORT4 for everything else. fill with 0 in writeBuffer
			// TODO : since we don't use predefined vertex layouts, we can probably use ushort1 without expanding it.
			packType = GpuApi::VertexPacking::PT_UShort4;
			break;
		case GpuApi::VertexPacking::PS_SkinWeights:
			// use either FLOAT1 for destruction or FLOAT4 for everything else. fill with 0.0 in writeBuffer
			// TODO : since we don't use predefined vertex layouts, we can probably use float1 without expanding it.
			packType = GpuApi::VertexPacking::PT_Float4;
			break;
		}

		if ( packType != GpuApi::VertexPacking::PT_Invalid && packUsage != GpuApi::VertexPacking::PS_Invalid )
		{
			m_layout[elementIndex].m_usage = packUsage;
			m_layout[elementIndex].m_usageIndex = packUsageIndex;
			m_layout[elementIndex].m_type = packType;

			m_semanticElements[i] = elementIndex;

			m_elementOffsets[elementIndex] = m_stride;

			++elementIndex;

			m_stride += GpuApi::VertexPacking::GetPackingTypeSize( packType );
		}
	}

	RED_ASSERT( elementIndex <= GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS );
	// If we didn't fill the entire element buffer, add an empty element to mark the end.
	// If it's full, then the GpuApi will just use the full thing.
	if ( elementIndex < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS )
	{
		m_layout[elementIndex] = GpuApi::VertexPacking::PackingElement::END_OF_ELEMENTS;
	}


	if ( elementIndex == 1 )
	{
		m_hasOneSemantic = true;
	}
}

CApexVertexBuffer::~CApexVertexBuffer()
{
	if ( !m_vb.isNull() )
	{
		GpuApi::Release( m_vb );
	}
}

Bool CApexVertexBuffer::LockBuffer( physx::PxU32 firstVertex, physx::PxU32 numVertices )
{
	if ( nullptr != m_lockedBuffer )
	{
		RED_HALT( "Buffer already locked - make sure lock/unlock are not nested" );
		return true;
	}

	m_lockedBuffer = GpuApi::LockBuffer( m_vb, GpuApi::BLF_Discard, firstVertex * m_stride, numVertices * m_stride );
	RED_ASSERT( m_lockedBuffer, TXT("Unable to lock VertexBuffer!"));

#ifndef RED_FINAL_BUILD
	// Track the section of the buffer that we've locked. Anything outside of this range will be discard, and may contain
	// garbage values, so we want to make sure we aren't trying to use it.
	m_validRangeBegin = firstVertex;
	m_validRangeEnd = firstVertex + numVertices;
#endif

	return nullptr != m_lockedBuffer;
}

void CApexVertexBuffer::UnlockBuffer()
{
	if ( nullptr == m_lockedBuffer )
	{
		RED_HALT( "Buffer already unlocked - make sure lock/unlock are not nested" );
		return;
	}

	GpuApi::UnlockBuffer( m_vb );
	m_lockedBuffer = nullptr;
}

void* CApexVertexBuffer::GetLockedBufferData( NxRenderVertexSemantic::Enum semantic )
{
	void *dataPtr = nullptr;

	RED_ASSERT( nullptr != m_lockedBuffer );
	RED_ASSERT( semantic < NxRenderVertexSemantic::NUM_SEMANTICS, TXT("Invalid VertexBuffer Semantic!") );
	if ( nullptr != m_lockedBuffer && semantic < NxRenderVertexSemantic::NUM_SEMANTICS )
	{
		Int32 idx = m_semanticElements[ semantic ];
		RED_ASSERT( idx != -1, TXT("VertexBuffer does not use this semantic."));
		if ( idx != -1 )
		{
			dataPtr = OffsetPtr( m_lockedBuffer, m_elementOffsets[ idx ] );
		}
	}

	return dataPtr;
}

// special fast path for interleaved vec3 position and normal and optional vec4 tangent (avoids painfully slow strided writes to locked vertex buffer)
Bool CApexVertexBuffer::writeBufferFastPath( const physx::NxApexRenderVertexBufferData& data, physx::PxU32 firstVertex, physx::PxU32 numVertices )
{
	for (Uint32 i = 0; i < physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		physx::apex::NxRenderVertexSemantic::Enum apexSemantic = (physx::apex::NxRenderVertexSemantic::Enum)i;
		switch(apexSemantic)
		{
		case physx::apex::NxRenderVertexSemantic::POSITION:
		case physx::apex::NxRenderVertexSemantic::NORMAL:
		case physx::apex::NxRenderVertexSemantic::TANGENT:
			break;
		default:
			if(data.getSemanticData(apexSemantic).data)
				return false;
		}
	}

	const physx::apex::NxApexRenderSemanticData& positionSemanticData = data.getSemanticData(physx::apex::NxRenderVertexSemantic::POSITION);
	const physx::apex::NxApexRenderSemanticData& normalSemanticData = data.getSemanticData(physx::apex::NxRenderVertexSemantic::NORMAL);
	const physx::apex::NxApexRenderSemanticData& tangentSemanticData = data.getSemanticData(physx::apex::NxRenderVertexSemantic::TANGENT);

	if ( positionSemanticData.stride != 12 || normalSemanticData.stride != 12 )
		return false;

	const physx::PxVec3* PX_RESTRICT positionSrc = (const physx::PxVec3*)positionSemanticData.data;
	const physx::PxVec3* PX_RESTRICT normalSrc = (const physx::PxVec3*)normalSemanticData.data;
	const physx::PxVec4* PX_RESTRICT tangentSrc = (const physx::PxVec4*)tangentSemanticData.data;

	if ( !positionSrc || !normalSrc )
		return false;

	GpuApi::VertexPacking::ePackingType positionFormat = m_layout[ m_semanticElements[ physx::apex::NxRenderVertexSemantic::POSITION ] ].m_type;
	GpuApi::VertexPacking::ePackingType normalFormat = m_layout[ m_semanticElements[ physx::apex::NxRenderVertexSemantic::NORMAL ] ].m_type;
	GpuApi::VertexPacking::ePackingType tangentFormat = tangentSrc ? m_layout[ m_semanticElements[ physx::apex::NxRenderVertexSemantic::TANGENT ] ].m_type : GpuApi::VertexPacking::PT_Invalid;

	if ( GpuApi::VertexPacking::GetPackingTypeSize( positionFormat ) != 12 )
		return false;

	if ( GpuApi::VertexPacking::GetPackingTypeSize( normalFormat ) != 12 )
		return false;

	if ( tangentSrc != nullptr && ( tangentSemanticData.stride != 16 || GpuApi::VertexPacking::GetPackingTypeSize( tangentFormat ) != 16 ) )
		return false;

	if ( !LockBuffer( firstVertex, numVertices ) )
	{
		RED_HALT( "Failed to lock the buffer" );
		return false;
	}

	const Uint32 stride = m_stride;
	void* positionDst	= GetLockedBufferData( physx::apex::NxRenderVertexSemantic::POSITION );
	void* normalDst		= GetLockedBufferData( physx::apex::NxRenderVertexSemantic::NORMAL );
	void* tangentDst	= tangentSrc ? GetLockedBufferData( physx::apex::NxRenderVertexSemantic::TANGENT ) : nullptr;

	Bool useFastPath = ( stride == (tangentSrc ? 40 : 24) );
	useFastPath &= normalDst == (Uint8*)positionDst + 12;
	useFastPath &= !tangentSrc || tangentDst == (Uint8*)positionDst + 24;

	if(useFastPath)
	{
		Uint8* dstIt = (Uint8*)positionDst;
		Uint8* dstEnd = dstIt + stride * numVertices;

		for( ; dstIt < dstEnd; dstIt += stride )
		{
			*(physx::PxVec3* PX_RESTRICT)(dstIt   ) = *positionSrc++;
			*(physx::PxVec3* PX_RESTRICT)(dstIt+12) = *normalSrc++;
			if(tangentSrc)
				*(physx::PxVec4* PX_RESTRICT)(dstIt+24) = *tangentSrc++;
		}
#ifndef RED_FINAL_BUILD
		++GRenderingStats.m_numApexVBUpdatedFastPath;
#endif
	}

	UnlockBuffer();

	return useFastPath;
}

void CApexVertexBuffer::writeBuffer( const physx::NxApexRenderVertexBufferData& data, physx::PxU32 firstVertex, physx::PxU32 numVertices )
{
#ifdef APEX_ENABLE_DEBUG_VISUALIZATION
	// If we have a debugvis render resource, we don't want to create and fill any gpu vertex buffer. Instead we just
	// pass the data to the resource.
	if ( m_debugVisRenderResource != nullptr )
	{
		m_debugVisRenderResource->FillVertices( this, data, firstVertex, numVertices );
		return;
	}
#endif


	// Create buffer if we don't have it.
	// TODO : If this is a static buffer, could create immutable?
	if ( m_vb.isNull() )
	{
		Uint32 bufferSize = m_desc.maxVerts * m_stride;
		m_vb = GpuApi::CreateBuffer( bufferSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );

		RED_ASSERT( !m_vb.isNull(), TXT("Failed to create Apex vertex buffer with %d vertices and stride %d"), m_desc.maxVerts, m_stride );
		if ( !m_vb.isNull() )
		{
			GpuApi::SetBufferDebugPath( m_vb, "APEXVB" );
		}
	}


	//String debugString = String::Printf(TXT("VertexBufferWrite [%0x]\n"), this );
	//OutputDebugString( debugString.AsChar() );

	if ( m_vb.isNull() || numVertices == 0 )
	{
		return;
	}

	// Try to use the "fast path" first. This is used for cloth, where we are updating the buffer every frame, and it always
	// contains Position, Normal, and possibly Tangent packed together, and nothing else. Anything else we'll go through the normal
	// path, which is slower, but those buffers are updated much less frequently.
	if( writeBufferFastPath(data, firstVertex, numVertices) )
	{
		return;
	}


	// Find first semantic to update
	Uint32 semanticIter = 0;
	for ( ; semanticIter < physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS; ++semanticIter )
	{
		physx::apex::NxRenderVertexSemantic::Enum apexSemantic = (physx::apex::NxRenderVertexSemantic::Enum)semanticIter;
		const physx::apex::NxApexRenderSemanticData& semanticData = data.getSemanticData(apexSemantic);
		if ( nullptr != semanticData.data )
		{
			break;
		}
	}

	// Update semantics
	if ( semanticIter < physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS )
	{
		LockBuffer( firstVertex, numVertices );

		for ( ; semanticIter < physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS; ++semanticIter )
		{
			physx::apex::NxRenderVertexSemantic::Enum apexSemantic = (physx::apex::NxRenderVertexSemantic::Enum)semanticIter;
			const physx::apex::NxApexRenderSemanticData& semanticData = data.getSemanticData(apexSemantic);
			if ( !semanticData.data )
			{
				continue;
			}

			const void* srcData = semanticData.data;
			const Uint32 srcStride = semanticData.stride;

			NxRenderDataFormat::Enum srcFormat = semanticData.format;
			Uint32 dstStride = m_stride;

			void* dstData = GetLockedBufferData( apexSemantic );

			RED_ASSERT( dstData && dstStride );

			if ( dstData && dstStride )
			{
				Uint32 formatSize = physx::apex::NxRenderDataFormat::getFormatDataSize( srcFormat );

				// If it's our only semantic, and the strides match, we can do a direct memcpy, rather than item-by-item.
				if ( m_hasOneSemantic && srcStride == dstStride )
				{
					Red::System::MemoryCopy( dstData, srcData, dstStride * numVertices );
				}
				else if (apexSemantic == physx::apex::NxRenderVertexSemantic::TANGENT && srcFormat == physx::apex::NxRenderDataFormat::FLOAT4)
				{
					RED_ASSERT( formatSize == sizeof( physx::PxVec4 ) );
					for (Uint32 j = 0; j < numVertices; j++)
					{
						Red::System::MemoryCopy(dstData, srcData, formatSize);
						dstData = OffsetPtr( dstData, dstStride );
						srcData = OffsetPtr( srcData, srcStride );
					}
				}
				else if (apexSemantic == physx::apex::NxRenderVertexSemantic::TANGENT && srcFormat == physx::apex::NxRenderDataFormat::FLOAT3)
				{
					// we need to increase the data from float3 to float4
					const physx::apex::NxApexRenderSemanticData& bitangentData = data.getSemanticData(physx::apex::NxRenderVertexSemantic::BINORMAL);
					const physx::apex::NxApexRenderSemanticData& normalData = data.getSemanticData(physx::apex::NxRenderVertexSemantic::NORMAL);
					if (bitangentData.format != physx::apex::NxRenderDataFormat::UNSPECIFIED && normalData.format != physx::apex::NxRenderDataFormat::UNSPECIFIED)
					{
						RED_ASSERT(bitangentData.format == physx::apex::NxRenderDataFormat::FLOAT3);
						const physx::PxVec3* srcDataBitangent	= (const physx::PxVec3*)bitangentData.data;
						const Uint32 srcStrideBitangent			= bitangentData.stride;

						RED_ASSERT(normalData.format == physx::apex::NxRenderDataFormat::FLOAT3);
						const physx::PxVec3* srcDataNormal		= (const physx::PxVec3*)normalData.data;
						const Uint32 srcStrideNormal			= normalData.stride;

						for (Uint32 j = 0; j < numVertices; j++)
						{
							const physx::PxVec3& normal		= *srcDataNormal;
							const physx::PxVec3& bitangent	= *srcDataBitangent;
							const physx::PxVec3& tangent	= *(physx::PxVec3*)srcData;
							float tangentw = physx::PxSign(normal.cross(tangent).dot(bitangent));
							*(physx::PxVec4*)dstData = physx::PxVec4(tangent, tangentw);

							dstData				= OffsetPtr( dstData, dstStride );
							srcData				= OffsetPtr( srcData, srcStride );
							srcDataBitangent	= OffsetPtr( srcDataBitangent, srcStrideBitangent );
							srcDataNormal		= OffsetPtr( srcDataNormal, srcStrideNormal );
						}
					}
					else
					{
						// just assume 1.0 as tangent.w if there is no bitangent to calculate this from
						for (Uint32 j = 0; j < numVertices; j++)
						{
							physx::PxVec3 tangent = *(physx::PxVec3*)srcData;
							*(physx::PxVec4*)dstData = physx::PxVec4(tangent, 1.0f);

							dstData = OffsetPtr( dstData, dstStride );
							srcData = OffsetPtr( srcData, srcStride );
						}
					}
				}
				else if (apexSemantic == physx::apex::NxRenderVertexSemantic::BONE_INDEX && ( srcFormat == physx::apex::NxRenderDataFormat::USHORT1 || srcFormat == physx::apex::NxRenderDataFormat::USHORT2 || srcFormat == physx::apex::NxRenderDataFormat::USHORT3))
				{
					Uint32 numIndices = 0;
					switch (srcFormat)
					{
					case physx::apex::NxRenderDataFormat::USHORT1: numIndices = 1; break;
					case physx::apex::NxRenderDataFormat::USHORT2: numIndices = 2; break;
					case physx::apex::NxRenderDataFormat::USHORT3: numIndices = 3; break;
					default:
						RED_HALT( "Unsupported format for BONE_INDEX: %i", srcFormat );
						break;
					}

					for (Uint32 j = 0; j < numVertices; j++)
					{
						Uint16* srcBoneIndices = (Uint16*)srcData;
						Uint16* dstBoneIndices = (Uint16*)dstData;

						for (Uint32 i = 0; i < numIndices; i++)
						{
							dstBoneIndices[i] = srcBoneIndices[i];
						}
						for (Uint32 i = numIndices; i < 4; i++)
						{
							dstBoneIndices[i] = 0;
						}

						dstData = OffsetPtr( dstData, dstStride );
						srcData = OffsetPtr( srcData, srcStride );
					}
				}
				else if (apexSemantic == physx::apex::NxRenderVertexSemantic::BONE_WEIGHT && (semanticData.format == physx::apex::NxRenderDataFormat::FLOAT1 || semanticData.format == physx::apex::NxRenderDataFormat::FLOAT2 || semanticData.format == physx::apex::NxRenderDataFormat::FLOAT3))
				{
					Uint32 numWeights = 0;
					switch (semanticData.format)
					{
					case physx::apex::NxRenderDataFormat::FLOAT1: numWeights = 1; break;
					case physx::apex::NxRenderDataFormat::FLOAT2: numWeights = 2; break;
					case physx::apex::NxRenderDataFormat::FLOAT3: numWeights = 3; break;
					default:
						RED_HALT( "Unsupported format for BONE_WEIGHT: %i", srcFormat );
						break;
					}

					for (Uint32 j = 0; j < numVertices; j++)
					{
						float* boneIndices = (float*)srcData;
						float* dstBoneIndices = (float*)dstData;

						for (Uint32 i = 0; i < numWeights; i++)
						{
							dstBoneIndices[i] = boneIndices[i];
						}
						for (Uint32 i = numWeights; i < 4; i++)
						{
							dstBoneIndices[i] = 0.0f;
						}

						dstData = OffsetPtr( dstData, dstStride );
						srcData = OffsetPtr( srcData, srcStride );
					}
				}
				else if (semanticData.format == physx::apex::NxRenderDataFormat::B8G8R8A8)
				{
					// We map both RGBA and BGRA formats to the same Color engine type (RGBA), so we need to swap R/B to get correct coloring.
					for (Uint32 j = 0; j < numVertices; j++)
					{
						const Uint8* srcDataByte = static_cast< const Uint8* >( srcData );
						Uint8* dstDataByte = static_cast< Uint8* >( dstData );

						dstDataByte[0] = srcDataByte[2];
						dstDataByte[1] = srcDataByte[1];
						dstDataByte[2] = srcDataByte[0];
						dstDataByte[3] = srcDataByte[3];

						dstData = OffsetPtr( dstData, dstStride );
						srcData = OffsetPtr( srcData, srcStride );
					}
				}
				else if (semanticData.format == physx::apex::NxRenderDataFormat::BYTE_SNORM3)
				{
					// normal path (expanding)
					for (Uint32 j = 0; j < numVertices; j++)
					{
						const Int8* srcDataByte = static_cast< const Int8* >( srcData );
						Int8* dstDataByte = static_cast< Int8* >( dstData );

						dstDataByte[0] = srcDataByte[0];
						dstDataByte[1] = srcDataByte[1];
						dstDataByte[2] = srcDataByte[2];
						dstDataByte[3] = 0;

						dstData = OffsetPtr( dstData, dstStride );
						srcData = OffsetPtr( srcData, srcStride );
					}
				}
				else
				{
					for (Uint32 j = 0; j < numVertices; j++)
					{
						Red::System::MemoryCopy(dstData, srcData, formatSize);
						dstData = OffsetPtr( dstData, dstStride );
						srcData = OffsetPtr( srcData, srcStride );
					}
				}
#ifndef RED_FINAL_BUILD
				++GRenderingStats.m_numApexVBSemanticsUpdated;
#endif
			}
		}

		UnlockBuffer();
	}

#ifndef RED_FINAL_BUILD
	++GRenderingStats.m_numApexVBUpdated;
#endif
}


//////////////////////////////////////////////////////////////////////////
// CApexIndexBuffer
//////////////////////////////////////////////////////////////////////////

CApexIndexBuffer::CApexIndexBuffer( const NxUserRenderIndexBufferDesc& ibDesc )
	: m_desc( ibDesc )
#ifndef RED_FINAL_BUILD
	, m_vertexRangeBegin( 0 )
	, m_vertexRangeEnd( 0 )
#endif
{
	// TODO : For static buffers (check m_desc.hint), we could delay creating the buffer until writeBuffer, so we could
	// create it immutable with initial data.

	if (m_desc.format == NxRenderDataFormat::UINT1)
	{
		m_ib = GpuApi::CreateBuffer( ibDesc.maxIndices * 4, GpuApi::BCC_Index32Bit, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	}
	else if (m_desc.format == NxRenderDataFormat::USHORT1)
	{
		m_ib = GpuApi::CreateBuffer( ibDesc.maxIndices * 2, GpuApi::BCC_Index16Bit, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	}
	else
	{
		RED_HALT( "Unsupported IndexBuffer format: %i", m_desc.format );
	}

	RED_ASSERT( !m_ib.isNull(), TXT("Failed to create Apex index buffer with %d indices"), ibDesc.maxIndices );
	if ( !m_ib.isNull() )
	{
		GpuApi::SetBufferDebugPath( m_ib, "APEXIB" );
	}
}

CApexIndexBuffer::~CApexIndexBuffer()
{
	if ( !m_ib.isNull() )
	{
		GpuApi::Release( m_ib );
	}
}

void CApexIndexBuffer::writeBuffer( const void* srcData, physx::PxU32 srcStride, physx::PxU32 firstDestElement, physx::PxU32 numElements )
{
	// TODO : For static buffers (check m_desc.hint), we could create an immutable buffer with initial data here.

	// TODO : Could also maybe take indices in the given order, and switch the culling direction. Might be able to
	// do a straight memcpy, instead of copying index-by-index.

	//LOG_RENDERER( TXT("IndexBufferWrite [%0x]"), this );
	if ( m_ib.isNull() )
	{
		return;
	}

	RED_ASSERT( srcData );

	GpuApi::eBufferChunkType chunkType;
	switch ( srcStride )
	{
	case 2:		chunkType = GpuApi::BCT_IndexUShort;	break;
	case 4:		chunkType = GpuApi::BCT_IndexUInt;		break;
	default:
		RED_HALT(  "Stride not supported: %u", srcStride );
		return;
	}

	Uint32 offset		= srcStride * firstDestElement;
	Uint32 lockedSize	= srcStride * numElements;

#ifndef RED_FINAL_BUILD
	Uint32 minIndex = 0xffffffff;
	Uint32 maxIndex = 0;
#endif

	void* dstData = GpuApi::LockBuffer( m_ib, GpuApi::BLF_Discard, offset, lockedSize );
	RED_ASSERT( dstData, TXT("Failed to lock buffer") );
	if ( dstData )
	{
		if ( m_desc.primitives == physx::apex::NxRenderPrimitiveType::TRIANGLES)
		{
			m_numPrimitives = numElements / 3;

			if ( m_desc.format == NxRenderDataFormat::USHORT1 )
			{
				Uint16*			dst = (Uint16*)dstData;
				const Uint16*	src = (const Uint16*)srcData;
				for ( Uint32 i = 0; i < m_numPrimitives; ++i )
				{
					for ( Uint32 j = 0; j < 3; ++j )
					{
						Uint32 dstIndex = i * 3 + j;
						Uint32 srcIndex = i * 3 + (2 - j);
						dst[ dstIndex ] = src[ srcIndex ];
#ifndef RED_FINAL_BUILD
						minIndex = Min< Uint32 >( minIndex, src[ srcIndex ] );
						maxIndex = Max< Uint32 >( maxIndex, src[ srcIndex ] );
#endif
					}
				}
			}
			else if ( m_desc.format == NxRenderDataFormat::UINT1 )
			{
				Uint32*			dst = (Uint32*)dstData;
				const Uint32*	src = (const Uint32*)srcData;
				for ( Uint32 i = 0; i < m_numPrimitives; ++i )
				{
					for ( Uint32 j = 0; j < 3; ++j )
					{
						Uint32 dstIndex = i * 3 + j;
						Uint32 srcIndex = i * 3 + (2 - j);
						dst[ dstIndex ] = src[ srcIndex ];
#ifndef RED_FINAL_BUILD
						minIndex = Min< Uint32 >( minIndex, src[ srcIndex ] );
						maxIndex = Max< Uint32 >( maxIndex, src[ srcIndex ] );
#endif
					}
				}
			}
			else
			{
				RED_HALT( "Unsupported index format: %i", m_desc.format );
			}
		}
		else
		{
			m_numPrimitives = numElements;
			if ( m_desc.format == NxRenderDataFormat::USHORT1 )
			{
				Uint16*			dst = (Uint16*)dstData;
				const Uint16*	src = (const Uint16*)srcData;
				for ( Uint32 i = 0; i < numElements; ++i )
				{
					*dst = *src;
					++dst;
					src = OffsetPtr( src, srcStride );
				}
			}
			else if ( m_desc.format == NxRenderDataFormat::UINT1 )
			{
				Uint32*			dst = (Uint32*)dstData;
				const Uint32*	src = (const Uint32*)srcData;
				for ( Uint32 i = 0; i < numElements; ++i )
				{
					*dst = *src;
					++dst;
					src = OffsetPtr( src, srcStride );
				}
			}
			else
			{
				RED_HALT( "Unsupported index format: %i", m_desc.format );
			}
		}
	}
	GpuApi::UnlockBuffer( m_ib );

#ifndef RED_FINAL_BUILD
	m_vertexRangeBegin = minIndex;
	m_vertexRangeEnd = maxIndex + 1;
#endif

#ifndef RED_FINAL_BUILD
	++GRenderingStats.m_numApexIBUpdated;
#endif
}

void CApexIndexBuffer::Bind()
{
	GpuApi::BindIndexBuffer( m_ib );
}

//////////////////////////////////////////////////////////////////////////
// CApexBoneBuffer
//////////////////////////////////////////////////////////////////////////

CApexBoneBuffer::CApexBoneBuffer( const NxUserRenderBoneBufferDesc& bbDesc )
	: m_desc( bbDesc )
{
	// Build desc
	Uint32 numElements = APEX_BONE_BUFFERS_MAX_BONES;
	Uint32 elementSize = APEX_BONE_BUFFERS_BYTES_PER_MATRIX;

	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = nullptr;
	bufInitData.m_elementCount = numElements;

	m_buffer = GpuApi::CreateBuffer( numElements * elementSize, GpuApi::BCC_Structured, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitData );
	GpuApi::SetBufferDebugPath( m_buffer, "apexBonesBuffer" );
}

CApexBoneBuffer::~CApexBoneBuffer()
{
	GpuApi::SafeRelease( m_buffer );
}

void CApexBoneBuffer::writeBuffer( const physx::NxApexRenderBoneBufferData& data, physx::PxU32 firstBone, physx::PxU32 numBones )
{
	RED_ASSERT( !m_buffer.isNull(), TXT("Updating apex bone buffer, with no actual buffer") );
	if ( m_buffer.isNull() )
	{
		return;
	}

	const physx::apex::NxApexRenderSemanticData& semanticData = data.getSemanticData(physx::apex::NxRenderBoneSemantic::POSE);
	const physx::apex::NxApexRenderSemanticData& fadeSemanticData = data.getSemanticData(physx::apex::NxRenderBoneSemantic::ALPHA);
	Bool haveFadeData = fadeSemanticData.format == physx::apex::NxRenderDataFormat::FLOAT1;

	Bool isFaded = false;

	if (semanticData.data)
	{
		const Float* srcData = static_cast<const Float*>( semanticData.data );
		const Uint32 srcStride = semanticData.stride;

		const Float* fadeData = static_cast< const Float* >( fadeSemanticData.data );

		void* bufferData = GpuApi::LockBuffer( m_buffer, GpuApi::BLF_Discard, 0, APEX_BONE_BUFFERS_BYTES_PER_MATRIX * APEX_BONE_BUFFERS_MAX_BONES );
		RED_ASSERT( bufferData != nullptr, TXT("Failed to get bone buffer data") );

		Float* skinMatrices = static_cast<Float*>( bufferData );

		for ( Uint32 i = 0; i < numBones; ++i )
		{
			const Float* matrix = &srcData[0];

			skinMatrices[ (firstBone+i) * 16 + 0  ] = matrix[0];
			skinMatrices[ (firstBone+i) * 16 + 1  ] = matrix[3];
			skinMatrices[ (firstBone+i) * 16 + 2  ] = matrix[6];
			skinMatrices[ (firstBone+i) * 16 + 3  ] = 0.0f;

			skinMatrices[ (firstBone+i) * 16 + 4  ] = matrix[1];
			skinMatrices[ (firstBone+i) * 16 + 5  ] = matrix[4];
			skinMatrices[ (firstBone+i) * 16 + 6  ] = matrix[7];
			skinMatrices[ (firstBone+i) * 16 + 7  ] = 0.0f;

			skinMatrices[ (firstBone+i) * 16 + 8  ] = matrix[2];
			skinMatrices[ (firstBone+i) * 16 + 9  ] = matrix[5];
			skinMatrices[ (firstBone+i) * 16 + 10 ] = matrix[8];
			skinMatrices[ (firstBone+i) * 16 + 11 ] = 0.0f;

			skinMatrices[ (firstBone+i) * 16 + 12 ] = matrix[9];
			skinMatrices[ (firstBone+i) * 16 + 13 ] = matrix[10];
			skinMatrices[ (firstBone+i) * 16 + 14 ] = matrix[11];
			skinMatrices[ (firstBone+i) * 16 + 15 ] = haveFadeData ? fadeData[ i ] : 1.0f;

			srcData = OffsetPtr( srcData, srcStride );

			// Track whether we have any fading happening. If there's no fade, we can render a bit faster.
			if ( !isFaded && haveFadeData && fadeData[ i ] < 1.0f )
			{
				isFaded = true;
			}
		}
		GpuApi::UnlockBuffer(m_buffer);

		// Get up-to-date bind info for the skinning data.
		m_bindInfo = Vector(0.f, 1.f, 1.f, 1.f);
	}

	m_isFaded = isFaded;

#ifndef RED_FINAL_BUILD
	++GRenderingStats.m_numApexBBUpdated;
#endif
}

void CApexBoneBuffer::Bind()
{
	RED_ASSERT( !m_buffer.isNull(), TXT("Binding apex bone buffer, with invalid buffer") );
	if ( m_buffer.isNull() )
	{
		return;
	}
	GpuApi::BindBufferSRV( m_buffer, 0, GpuApi::VertexShader );

	GetRenderer()->GetStateManager().SetVertexConst( VSC_SkinningData, m_bindInfo );
}

//////////////////////////////////////////////////////////////////////////
// CApexRenderResource
//////////////////////////////////////////////////////////////////////////

CApexRenderResource::CApexRenderResource( const NxUserRenderResourceDesc& rrDesc )
	: m_numVB( rrDesc.numVertexBuffers )
	, m_indexBuffer( (CApexIndexBuffer*)rrDesc.indexBuffer )
	, m_boneBuffer( (CApexBoneBuffer*)rrDesc.boneBuffer )
	, m_desc( rrDesc )
	, m_firstVertex( rrDesc.firstVertex )
	, m_firstIndex( rrDesc.firstIndex )
	, m_firstBone( rrDesc.firstBone )
	, m_numVertices( rrDesc.numVerts )
	, m_numIndices( rrDesc.numIndices )
	, m_numBones( rrDesc.numBones )
	, m_lastRenderMaterial( nullptr )
	, m_lastRenderMaterialParameters( nullptr )
{
	m_vertexBuffers = static_cast< CApexVertexBuffer** >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_ApexRenderObject, m_numVB * sizeof( CApexVertexBuffer* ), __alignof( CApexVertexBuffer* ) ) );
	Red::System::MemoryCopy( m_vertexBuffers, rrDesc.vertexBuffers, m_numVB * sizeof( CApexVertexBuffer* ) );

	GpuApi::VertexLayoutDesc layoutDesc;
	for ( Uint8 vb = 0; vb < static_cast<Uint8>( m_numVB ); ++vb )
	{
		layoutDesc.AddElements( m_vertexBuffers[vb]->m_layout, vb, GpuApi::VertexPacking::ST_PerVertex );
	}

	m_vertexLayout = GpuApi::CreateVertexLayout( layoutDesc );

	RED_ASSERT( m_vertexLayout, TXT("Failed to create Apex vertex layout.") );

	m_primitiveType = MapApexPrimitiveToGpuApi( m_desc.primitives );
	RED_ASSERT( m_primitiveType != GpuApi::PRIMTYPE_Invalid, TXT("CApexRenderResource was given an invalid primitive type: %d"), m_desc.primitives );
}

CApexRenderResource::~CApexRenderResource()
{
	SAFE_RELEASE( m_lastRenderMaterial );
	SAFE_RELEASE( m_lastRenderMaterialParameters );

	if ( m_vertexBuffers )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_ApexRenderObject, m_vertexBuffers );
	}
}

void CApexRenderResource::setVertexBufferRange(physx::PxU32 firstVertex, physx::PxU32 numVerts)
{ 
	m_firstVertex = firstVertex;
	m_numVertices = numVerts;
	//LOG_RENDERER( TXT("RR [%0x], setVertexBufferRange: firstVertex [%d], numVerts [%d]"), this, firstVertex, numVerts );
}

void CApexRenderResource::setIndexBufferRange(physx::PxU32 firstIndex, physx::PxU32 numIndices)
{ 
	m_firstIndex = firstIndex;
	m_numIndices = numIndices;
	//LOG_RENDERER( TXT("RR [%0x], setIndexBufferRange firstIndex: [%d], numIndices: [%d]"), this, firstIndex, numIndices ); 
}

void CApexRenderResource::setBoneBufferRange(physx::PxU32 firstBone, physx::PxU32 numBones)
{ 
	m_firstBone = firstBone;
	m_numBones = numBones;
	//LOG_RENDERER( TXT("setBoneBufferRange") ); 
	RED_ASSERT(firstBone == 0, TXT("firstBone is not 0"));
}

void CApexRenderResource::setMaterial( void* materialObj )
{
	// We should keep a reference to the material/parameters, in case material registrations get changed while we're working.
	SAFE_RELEASE( m_lastRenderMaterial );
	SAFE_RELEASE( m_lastRenderMaterialParameters );

	if ( materialObj )
	{
		SApexMaterialMapping* mapping = static_cast< SApexMaterialMapping* >( materialObj );

		m_lastRenderMaterial = static_cast< CRenderMaterial* >( mapping->m_material );
		m_lastRenderMaterialParameters = static_cast< CRenderMaterialParameters* >( mapping->m_materialParameters );

		if ( m_lastRenderMaterial ) m_lastRenderMaterial->AddRef();
		if ( m_lastRenderMaterialParameters ) m_lastRenderMaterialParameters->AddRef();
	}
}

NxUserRenderVertexBuffer*	CApexRenderResource::getVertexBuffer(physx::PxU32 index) const
{ 
	return ( index < m_numVB ) ? m_vertexBuffers[ index ] : nullptr;
}


void CApexRenderResource::Render( const SApexRenderContext& context )
{
	if ( m_lastRenderMaterial == nullptr || m_lastRenderMaterialParameters == nullptr )
	{
		ERR_RENDERER( TXT("Can't render an apex render resource with no material.") );
		return;
	}

	CApexIndexBuffer* ib = m_indexBuffer;
	if ( ib )
	{
		ib->Bind();
	}

	CApexBoneBuffer* bb = m_boneBuffer;
	if (bb)
	{
		bb->Bind();
	}

	GetRenderer()->GetStateManager().SetLocalToWorld( &context.m_localToWorld );

	Float wet = 0.f;
	if( context.m_currentProxy )
	{
		wet = context.m_currentProxy->GetWetness();
	}
	CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
	stateManager.SetVertexConst( VSC_Custom_1, Vector( wet, 0.0f, 0.0f, 0.0f ) );
	
	Bool shaderBindSuccess = true;
	{
		// Just use cached distance directly here, rather than calling AdjustCameraDistanceSqForTextures. Apex is generally
		// small, so this should be close enough.
		const Float distanceSquare = context.m_currentProxy->GetCachedDistanceSquared();

		// When rendering something that has an actual material, we expect that there should be an associated render proxy and frame info.
		// Those are only null when rendering the debug geometry, which does not have a material.
		RED_ASSERT( context.m_currentProxy != nullptr && context.m_frameInfo != nullptr );

		Bool isDissolved = (context.m_dissolve || ( bb && bb->m_isFaded )) && !context.m_renderContext->m_forceNoDissolves;

		MaterialRenderingContext materialContext( *context.m_renderContext );
		materialContext.m_materialDebugMode = context.m_renderContext->m_materialDebugMode;
		materialContext.m_selected = context.m_currentProxy->IsSelected() && context.m_frameInfo->IsShowFlagOn( SHOW_Selection );
		materialContext.m_useInstancing = false;
		materialContext.m_vertexFactory = (m_numBones > 0 && m_numVB > 1) ? MVF_ApexWithBones : MVF_ApexWithoutBones;
		materialContext.m_hasExtraStreams = m_numVB > 2;
		materialContext.m_discardingPass = m_lastRenderMaterialParameters->IsMasked() || isDissolved || context.m_currentProxy->HasClippingEllipse();

		if ( materialContext.m_discardingPass )
		{
			// HACK : Should find a better way to do this...
			Vector discardFlags(
				m_lastRenderMaterialParameters->IsMasked() ? 1.0f : 0.0f,
				isDissolved ? 1.0f: 0.0f,
				0.0f,
				context.m_currentProxy->HasClippingEllipse() ? 1.0f : 0.0f
				);
			GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, discardFlags );
		}

		shaderBindSuccess = m_lastRenderMaterial->Bind( materialContext, m_lastRenderMaterialParameters, distanceSquare );
	}

	CGpuApiScopedTwoSidedRender scopedForcedTwoSided;
	{
		// deal with two-sided rendering of Apex resources
		// Enable/disable culling as required. We don't worry about separate CW/CCW culling, Apex is giving the geometry in the order we need so it's not a problem.
		Bool isTwoSided = m_desc.cullMode == NxRenderCullMode::NONE;
		// when doing only the debug rendering (visualizing the physx/apex collision mesh) we do not have the proxy available, so the two-sided setting for that is unavailable
		isTwoSided |= context.m_currentProxy ? context.m_currentProxy->IsTwoSided() : 0;
		isTwoSided |= m_lastRenderMaterial ? m_lastRenderMaterial->IsTwoSided() : 0;
		scopedForcedTwoSided.SetForcedTwoSided( isTwoSided );
	}	

	if ( shaderBindSuccess )
	{
		GpuApi::BufferRef buffers[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS];
		Uint32 strides[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };
		Uint32 offsets[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };

		RED_ASSERT( m_numVB <= GPUAPI_VERTEX_LAYOUT_MAX_SLOTS, TXT("Unsupported number of VBs") );
		if ( m_numVB > GPUAPI_VERTEX_LAYOUT_MAX_SLOTS ) return;

		for ( Uint32 i = 0; i < m_numVB; ++i )
		{
#ifndef RED_FINAL_BUILD
			if ( ib )
			{
				RED_ASSERT( ib->m_vertexRangeBegin >= m_vertexBuffers[i]->m_validRangeBegin && ib->m_vertexRangeEnd <= m_vertexBuffers[i]->m_validRangeEnd, TXT("Index buffer accesses outside of vertex buffer's valid data!") );
			}
#endif

			buffers[i] = m_vertexBuffers[i]->m_vb;
			strides[i] = m_vertexBuffers[i]->m_stride;
			offsets[i] = m_vertexBuffers[i]->m_stride * m_firstVertex;
		}
		GpuApi::SetVertexFormatRaw( m_vertexLayout );
		GpuApi::BindVertexBuffers( 0, m_numVB, buffers, strides, offsets );


		Uint32 numPrims = GpuApi::MapDrawElementCountToPrimitiveCount( m_primitiveType, ib ? m_numIndices : m_numVertices );

		if ( ib )
		{
			GpuApi::DrawIndexedPrimitive( m_primitiveType, 0, 0, m_numVertices, m_firstIndex, numPrims );
		}
		else
		{
			GpuApi::DrawPrimitive( m_primitiveType, 0, numPrims );
		}

#ifndef RED_FINAL_BUILD
		// Update render stats.
		if ( context.m_meshStats )
		{
			if ( context.m_currentProxy->GetApexType() == CRenderProxy_Apex::AT_Cloth )
			{
				context.m_meshStats->m_numChunksSkinned += 1;
				context.m_meshStats->m_numVerticesSkinned += m_numVertices;
				// If we are drawing triangles, add them to the mesh stats.
				if ( m_primitiveType == GpuApi::PRIMTYPE_TriangleList || m_primitiveType == GpuApi::PRIMTYPE_TriangleStrip )
				{
					context.m_meshStats->m_numTrianglesSkinned += numPrims;
				}
			}

			if ( context.m_currentProxy->GetApexType() == CRenderProxy_Apex::AT_Destructible )
			{
				context.m_meshStats->m_numChunksStatic += 1;
				context.m_meshStats->m_numVerticesStatic += m_numVertices;
				// If we are drawing triangles, add them to the mesh stats.
				if ( m_primitiveType == GpuApi::PRIMTYPE_TriangleList || m_primitiveType == GpuApi::PRIMTYPE_TriangleStrip )
				{
					context.m_meshStats->m_numTrianglesStatic += numPrims;
				}
			}

			//count all
			context.m_meshStats->m_numChunks += 1;
			context.m_meshStats->m_numVertices += m_numVertices;
			// If we are drawing triangles, add them to the mesh stats.
			if ( m_primitiveType == GpuApi::PRIMTYPE_TriangleList || m_primitiveType == GpuApi::PRIMTYPE_TriangleStrip )
			{
				context.m_meshStats->m_numTriangles += numPrims;
			}
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////////
// CApexRenderer
//////////////////////////////////////////////////////////////////////////

CApexRenderer::CApexRenderer() 
	: m_frameInfo( nullptr )
	, m_context( nullptr )
	, m_currentMaterial( nullptr )
	, m_currentMaterialParameters( nullptr )
	, m_currentProxy( nullptr )
	, m_currentElement( nullptr )
	, m_meshStats( nullptr )
	, m_dissolve( false )
{

}

void CApexRenderer::renderResource( const physx::apex::NxApexRenderContext& context )
{
	if ( !context.renderResource ) return;

	CApexRenderResource* resource = static_cast< CApexRenderResource* >( context.renderResource );

	// We only want to render this resource if it's using the material we're supposed to be rendering
	if ( ( m_currentMaterial && m_currentMaterial != resource->m_lastRenderMaterial ) || ( m_currentMaterialParameters && m_currentMaterialParameters != resource->m_lastRenderMaterialParameters ) )
	{
		return;
	}

	// If we have a replacement, keep previous material and set it
	CRenderMaterial* previousMaterial = resource->m_lastRenderMaterial;
	CRenderMaterialParameters* previousParameters = resource->m_lastRenderMaterialParameters;
	if ( m_currentMaterial != nullptr && m_currentElement->HasMaterialReplacement() )
	{
		resource->m_lastRenderMaterial = m_currentElement->GetMaterial();
		resource->m_lastRenderMaterialParameters = m_currentElement->GetMaterialParams();
	}

	SApexRenderContext apexRenderContext;
	apexRenderContext.m_apexContext		= &context;
	apexRenderContext.m_frameInfo		= m_frameInfo;
	apexRenderContext.m_renderContext	= m_context;
	apexRenderContext.m_dissolve		= m_dissolve;
#ifndef RED_FINAL_BUILD
	apexRenderContext.m_meshStats		= m_meshStats;
#endif
	apexRenderContext.m_currentProxy	= m_currentProxy;

	Vector c0( context.local2world.column0.x, context.local2world.column0.y, context.local2world.column0.z, context.local2world.column0.w );
	Vector c1( context.local2world.column1.x, context.local2world.column1.y, context.local2world.column1.z, context.local2world.column1.w );
	Vector c2( context.local2world.column2.x, context.local2world.column2.y, context.local2world.column2.z, context.local2world.column2.w );
	Vector c3( context.local2world.column3.x, context.local2world.column3.y, context.local2world.column3.z, context.local2world.column3.w );
	apexRenderContext.m_localToWorld = Matrix(c0, c1, c2, c3);

	resource->Render( apexRenderContext );

	// Restore previous material and parameters if we had a replacement
	if ( m_currentMaterial != nullptr && m_currentElement->HasMaterialReplacement() )
	{
		resource->m_lastRenderMaterial = previousMaterial;
		resource->m_lastRenderMaterialParameters = previousParameters;
	}

#ifndef RED_FINAL_BUILD
	++GRenderingStats.m_numApexResourcesRendered;
#endif
}

//////////////////////////////////////////////////////////////////////////
// CApexRenderResourceManager
//////////////////////////////////////////////////////////////////////////

NxUserRenderVertexBuffer* CApexRenderResourceManager::createVertexBuffer( const NxUserRenderVertexBufferDesc& vertexBufferDesc )
{
	return new CApexVertexBuffer( vertexBufferDesc );
}

NxUserRenderIndexBuffer* CApexRenderResourceManager::createIndexBuffer( const NxUserRenderIndexBufferDesc& indexBufferDesc )
{
	return new CApexIndexBuffer( indexBufferDesc );
}

NxUserRenderBoneBuffer* CApexRenderResourceManager::createBoneBuffer( const NxUserRenderBoneBufferDesc& boneBufferDesc )
{
	return new CApexBoneBuffer( boneBufferDesc );
}

NxUserRenderResource* CApexRenderResourceManager::createResource( const NxUserRenderResourceDesc& resourceDesc )
{
	if ( resourceDesc.userRenderData != nullptr )
	{
		// If user data is non-null, we create a normal resource.
		// Don't actually use it for anything, just need to know if we're doing debug or normal.
		return new CApexRenderResource( resourceDesc );
	}
	else
	{
		// Null, so we're doing debug visualization and need to create a special resource that will support
		// update/dispatch from main thread.
#ifdef APEX_ENABLE_DEBUG_VISUALIZATION
		return new CApexDebugVisualizerResource( resourceDesc );
#else
		RED_HALT( "Not expecting debug resource when it's disabled" );
		return nullptr;
#endif
	}
}

void CApexRenderResourceManager::releaseVertexBuffer( NxUserRenderVertexBuffer& vertexBuffer ) 
{
	delete &vertexBuffer;
}

void CApexRenderResourceManager::releaseIndexBuffer( NxUserRenderIndexBuffer& indexBuffer )
{
	delete &indexBuffer;
}

void CApexRenderResourceManager::releaseBoneBuffer( NxUserRenderBoneBuffer& boneBuffer )
{
	delete &boneBuffer;
}

void CApexRenderResourceManager::releaseResource( NxUserRenderResource& resource )
{
	delete &resource;
}

physx::PxU32 CApexRenderResourceManager::getMaxBonesForMaterial( void* material )
{
	return APEX_BONE_BUFFERS_MAX_BONES;
}

#endif
