/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/uniqueBuffer.h"
#include "../../common/core/cooker.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/meshPacking.h"
#include "../../common/engine/meshDataBuilder.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/memoryFileWriter.h"

// Rather than trying to keep chunk definitions etc synced between render mesh and and cooker, just include the rendermesh header.
// Note that this only works for using functions defined in the header, because wcc does not link with renderer (so nothing from
// rendermesh source file will be used).
#include "../../common/renderer/renderMesh.h"


/// Mesh cooker
class CMeshCooker : public ICooker
{
	DECLARE_RTTI_SIMPLE_CLASS( CMeshCooker );

	GpuApi::VertexPacking::PackingParams		m_packingParams;
	GpuApi::VertexPacking::PackingVertexLayout	m_sourceVertexLayout;

public:

	CMeshCooker();
	virtual Bool DoCook( class ICookerFramework& cooker, const CookingOptions& options ) override;

	Bool PackData( const CMeshData& data, const Bool canUseExtraStreams, const TDynArray< Uint16 >& chunksToPack, GpuApi::VertexPacking::IBufferWriter& vbWriter, GpuApi::VertexPacking::IBufferWriter& ibWriter, Uint32* vbOffsets = nullptr, Uint32* vbSizes = nullptr, Uint32* ibOffsets = nullptr, Uint32* ibSizes = nullptr, Uint32* fullVbSize = nullptr, Uint32* fullIbSize = nullptr, Uint32* ibOffset = nullptr );
};

DEFINE_SIMPLE_RTTI_CLASS( CMeshCooker, ICooker );
IMPLEMENT_ENGINE_CLASS( CMeshCooker );



CMeshCooker::CMeshCooker()
{
	m_resourceClass = ClassID< CMesh >();

	// Add supported platforms
	m_platforms.PushBack( PLATFORM_PC );
#ifndef WCC_LITE
	m_platforms.PushBack( PLATFORM_PS4 );
	m_platforms.PushBack( PLATFORM_XboxOne );
#endif

	// Setup chunk packing streams
	m_sourceVertexLayout.m_stride = sizeof( SMeshVertex );

	// Standard streams
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_position ),		GpuApi::VertexPacking::PS_Position,		0, GpuApi::VertexPacking::PT_Float3 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_normal ),		GpuApi::VertexPacking::PS_Normal,		0, GpuApi::VertexPacking::PT_Float3 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_tangent ),		GpuApi::VertexPacking::PS_Tangent,		0, GpuApi::VertexPacking::PT_Float3 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_binormal ),		GpuApi::VertexPacking::PS_Binormal,		0, GpuApi::VertexPacking::PT_Float3 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_uv0 ),			GpuApi::VertexPacking::PS_TexCoord,		0, GpuApi::VertexPacking::PT_Float2 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_uv1 ),			GpuApi::VertexPacking::PS_TexCoord,		1, GpuApi::VertexPacking::PT_Float2 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_color),			GpuApi::VertexPacking::PS_Color,		0, GpuApi::VertexPacking::PT_Color  );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_indices ),		GpuApi::VertexPacking::PS_SkinIndices,	0, GpuApi::VertexPacking::PT_UByte4 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_weights ),		GpuApi::VertexPacking::PS_SkinWeights,	0, GpuApi::VertexPacking::PT_Float4 );

	// Special streams
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_extraData0 ),	GpuApi::VertexPacking::PS_ExtraData,	0, GpuApi::VertexPacking::PT_Float4 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_extraData1 ),	GpuApi::VertexPacking::PS_ExtraData,	1, GpuApi::VertexPacking::PT_Float4 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_extraData2 ),	GpuApi::VertexPacking::PS_ExtraData,	2, GpuApi::VertexPacking::PT_Float4 );
	m_sourceVertexLayout.Init( offsetof( SMeshVertex, m_extraData3 ),	GpuApi::VertexPacking::PS_ExtraData,	3, GpuApi::VertexPacking::PT_Float4 );

}

//////////////////////////////////////////////////////////////////////////

Bool CMeshCooker::PackData( const CMeshData& data, const Bool canUseExtraStreams, const TDynArray< Uint16 >& chunksToPack, GpuApi::VertexPacking::IBufferWriter& vbWriter, GpuApi::VertexPacking::IBufferWriter& ibWriter, Uint32* vbOffsets, Uint32* vbSizes, Uint32* ibOffsets, Uint32* ibSizes, Uint32* fullVbSize, Uint32* fullIbSize, Uint32* ibOffset )
{
	const auto& meshChunks = data.GetChunks();
	const Uint32 numChunksToPack = chunksToPack.Size();

	for ( Uint32 i = 0; i < numChunksToPack; ++i )
	{
		const SMeshChunk& sourceChunk = meshChunks[ chunksToPack[ i ] ];

		const GpuApi::eBufferChunkType vertexType = MeshUtilities::GetVertexTypeForMeshChunk( canUseExtraStreams, sourceChunk );
		const GpuApi::VertexPacking::PackingElement* packing = GpuApi::GetPackingForFormat( vertexType );


		const void* vertexData = sourceChunk.m_vertices.Data();
		const Uint32 numVertices = sourceChunk.m_vertices.Size();

		// Alignment
		// TODO : Might need to adjust per-platform?
		const Uint32 vertexAlignment = 32;

		// Count each buffer in sequence.
		Uint32 numStreams = RENDER_MESH_NUM_STREAMS;//GpuApi::VertexPacking::GetStreamCount( packing );
		for ( Uint32 streamIndex = 0; streamIndex < numStreams; ++streamIndex )
		{
			vbWriter.Align( vertexAlignment );

			const Uint32 offset = vbWriter.GetOffset();
			GpuApi::VertexPacking::PackVertices( &vbWriter, packing, m_packingParams, m_sourceVertexLayout, vertexData, numVertices, streamIndex );
			const Uint32 size = vbWriter.GetOffset() - offset;

			if ( vbOffsets != nullptr )
			{
				*vbOffsets = offset;
				++vbOffsets;
			}
			if ( vbSizes != nullptr )
			{
				*vbSizes = size;
				++vbSizes;
			}
		}
	}

	const Uint32 vbSize = vbWriter.GetOffset();

	// Reset alignment, we're starting a new buffer.
	ibWriter.Align( sizeof( Uint32 ) );

	const Uint32 ibStart = ibWriter.GetOffset();

	// Pack indices
	for ( Uint32 i = 0; i < numChunksToPack; ++i )
	{
		// Get chunk to pack
		const SMeshChunk& sourceChunk = meshChunks[ chunksToPack[ i ] ];

		// Pack indices
		{
			// Align
			const Uint32 indexSize = sizeof( Uint16 );
			ibWriter.Align( indexSize );

			// Pack vertices, will auto align
			const void* indexData = sourceChunk.m_indices.Data();
			const Uint32 numIndices = sourceChunk.m_numIndices;

			const Uint32 offset = ibWriter.GetOffset();

			GpuApi::VertexPacking::PackIndices( &ibWriter, GpuApi::VertexPacking::PT_Index16, indexData, numIndices );

			const Uint32 size = ibWriter.GetOffset() - offset;

			if ( ibOffsets != nullptr )
			{
				*ibOffsets = offset - ibStart;
				++ibOffsets;
			}
			if ( ibSizes != nullptr )
			{
				*ibSizes = size;
				++ibSizes;
			}

		}
	}

	const Uint32 ibSize = ibWriter.GetOffset() - ibStart;


	if ( fullVbSize != nullptr )
	{
		*fullVbSize = vbSize;
	}
	if ( fullIbSize != nullptr )
	{
		*fullIbSize = ibSize;
	}
	if ( ibOffset != nullptr )
	{
		*ibOffset = ibStart;
	}


	return true;
}

Bool CMeshCooker::DoCook( class ICookerFramework& cooker, const CookingOptions& options )
{
	// Trying to cook resource that was already cooked
	if ( options.m_resource->IsCooked() )
	{
		WARN_WCC( TXT("Resource '%s' was already cooked. Not cooking."), options.m_resource->GetFriendlyName().AsChar() );
		return true;
	}

	// Get the mesh to cook
	CMesh* mesh = SafeCast< CMesh >( options.m_resource );
	const Bool canUseExtraStreams = mesh->CanUseExtraStreams();
	ASSERT( mesh );
	
	// Get current mesh data
	const CMesh::TLODLevelArray& meshLods = mesh->GetMeshLODLevels();
	const Uint32 numMeshLods = meshLods.Size();
	if ( numMeshLods == 0 )
	{
		WARN_WCC( TXT("Mesh '%s' has no LODs."), mesh->GetFriendlyName().AsChar() );
		return false;
	}

	// Lock mesh data
	const CMeshData meshData( mesh );
	const auto& meshChunks = meshData.GetChunks();

	// Validate mesh data
	Bool hasValidData = true;
	for ( Uint32 i=0; i<meshChunks.Size() && hasValidData; ++i )
	{
		const SMeshChunk& sourceChunk = meshChunks[ i ];

		// test data		
		const Uint16 maxIndex = sourceChunk.m_numVertices-1;
		for ( Uint32 j=0; j<sourceChunk.m_indices.Size(); ++j )
		{
			if ( sourceChunk.m_indices[j] > maxIndex )
			{
				hasValidData = false;
				break;
			}
		}
	}

	// No valid data
	if ( !hasValidData )
	{
		ERR_WCC( TXT("!!! CORRUPTED MESH DATA !!!") );
		ERR_WCC( TXT("Mesh %ls contains corrupted data and will not be cooked"), mesh->GetDepotPath().AsChar() );
		return false;
	}

	// Pack mesh data
	CMeshPackingSource packer;
	if ( !packer.Pack( mesh ) )
		return false;

	// Get total mesh data
	Uint32 vertexDataSize=0, indexDataSize=0;
	packer.ComputeLayout( 16, 2, vertexDataSize, indexDataSize );
	if ( !vertexDataSize || !indexDataSize )
		return false;

	// Build mesh cooked data
	SMeshCookedData cookedData;
	cookedData.m_collisionInitPositionOffset = mesh->GetInitPositionOffset();
	cookedData.m_quantizationScale= mesh->GetBoundingBox().CalcSize();
	cookedData.m_quantizationOffset	= mesh->GetBoundingBox().Min;
	cookedData.m_vertexBufferSize = vertexDataSize;
	cookedData.m_indexBufferSize = indexDataSize;
	cookedData.m_indexBufferOffset = (Uint32) AlignOffset( cookedData.m_vertexBufferSize, 1024 );

	// Prepare merged buffer
	const Uint32 totalDataSize = cookedData.m_indexBufferOffset + cookedData.m_indexBufferSize;
	Red::UniqueBuffer deviceBufferData = Red::CreateUniqueBuffer( totalDataSize, 8 );
	packer.CopyVertexData( OffsetPtr( deviceBufferData.Get(), 0 ), vertexDataSize );
	packer.CopyIndexData( OffsetPtr( deviceBufferData.Get(), cookedData.m_indexBufferOffset ), indexDataSize );

	// Copy merged data to mesh buffer
	cookedData.m_renderBuffer.SetBufferContent( deviceBufferData.Get(), deviceBufferData.GetSize() );

	// Get cached bone positions
	{
		TDynArray< Vector > bonePositions;
		mesh->GetBonePositions(bonePositions);
		cookedData.m_bonePositions = bonePositions;
	}

	// Setup LODs
	cookedData.m_renderLODs.Reserve( packer.m_lods.Size() );
	for ( const CMeshPackingSource::PackedLod& lod : packer.m_lods )
	{
		cookedData.m_renderLODs.PushBack( lod.m_distance );
	}

	// Setup chunks
	TDynArray< CRenderMesh::Chunk > cookedChunks;
	cookedChunks.Reserve( packer.m_chunks.Size() );
	for ( const CMeshPackingSource::PackedChunk& chunk : packer.m_chunks )
	{
		CRenderMesh::Chunk newChunk;
		newChunk.m_lodMask = chunk.m_lodMask;
		newChunk.m_baseRenderMask = chunk.m_baseRenderMask;
		newChunk.m_mergedRenderMask = chunk.m_mergedRenderMask;
		newChunk.m_vertexFactory = chunk.m_vertexFactory;
		newChunk.m_materialId = chunk.m_materialId;
		newChunk.m_numVertices = chunk.m_numVertices;
		newChunk.m_numIndices = chunk.m_numIndices;

		// setup index stream
		newChunk.m_chunkIndices.type = GpuApi::BCT_IndexUShort;
		newChunk.m_chunkIndices.byteOffset = chunk.m_streams.m_indices->m_placement; // where did the packed indices ended up ?

		// setup vertex stream
		newChunk.m_chunkVertices.type = chunk.m_vertexType;
		for ( Uint32 i=0; i<ARRAY_COUNT(chunk.m_streams.m_vertices); ++i )
		{
			if ( chunk.m_streams.m_vertices[i] != nullptr )
			{
				newChunk.m_chunkVertices.byteOffsets[i] = chunk.m_streams.m_vertices[i]->m_placement;
			}
		}
		cookedChunks.PushBack( newChunk );
	}

	// Compile system data ( header + chunks )
	TDynArray< Uint8 > cookedSystemData;
	CMemoryFileWriter writer( cookedSystemData );
	writer << cookedChunks;
	cookedData.m_renderChunks = cookedSystemData;

	// Compile system data
	mesh->SetCookedData( cookedData );

	// Mesh was cooked
	return true;
}
