/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "mesh.h"
#include "meshPacking.h"
#include "material.h"
#include "materialGraph.h"
#include "materialDefinition.h"
#include "materialCompiler.h"
#include "meshDataBuilder.h"

#include "..\renderer\renderMeshBufferWriter.h"

#include "..\gpuApiUtils\gpuApiVertexFormats.h"
#include "..\gpuApiUtils\gpuApiVertexPacking.h"
#include "..\gpuApiUtils\gpuApiMemory.h"

namespace Helper
{
	static Uint8 GetChunkLODMask( const CMesh* mesh, const Uint32 chunkIndex )
	{
		// LEGACY: each chunk can be only in a single LOD

		const auto& lods = mesh->GetMeshLODLevels();
		for ( Uint32 i=0; i<lods.Size(); ++i )
		{
			const auto& lod = lods[i];

			if ( lod.m_chunks.Exist( chunkIndex ) )
			{
				return ( 1 << i );
			}
		}

		return 0;
	}

	static Bool CanMergeChunksShadows( const CMesh* mesh, const Uint32 chunkIndex )
	{
		const auto& sourceChunk = mesh->GetChunks()[ chunkIndex ];
		return (sourceChunk.m_vertexType == MVT_StaticMesh); // only static meshes can be merged like this
	}

	static Uint64 ComputeBufferHash64( const CPagedMemoryBuffer& buffer )
	{
		Uint64 hash = RED_FNV_OFFSET_BASIS64;

		const Uint32 bufferSize = buffer.GetTotalSize();
		Uint32 pos = 0;
		while ( pos < bufferSize )
		{
			Uint8 tempBuffer[ 8192 ];

			// get part of the buffer
			Uint32 blockSizeRead = 0;
			const Uint32 blockSize = Min< Uint32 >( ARRAY_COUNT(tempBuffer), bufferSize - pos );
			buffer.Read( pos, tempBuffer, blockSize, blockSizeRead );
			RED_FATAL_ASSERT( blockSizeRead == blockSize, "Error reading paged buffer" );

			// compute hash
			hash = Red::CalculateHash64( tempBuffer, blockSize, hash );
			pos += blockSize;
		}

		return hash;
	}

	static Bool IsSolidShadowMaterial( IMaterial* material )
	{
		if ( material && !material->IsMasked() )
		{
			CMaterialGraph* baseMaterial = Cast< CMaterialGraph >( material->GetMaterialDefinition() );
			if ( baseMaterial && baseMaterial->GetRenderingBlendMode() == RBM_None )
			{
				return true;
			}
		}

		return false;
	}

	static Bool IsSolidShadowMaterial( const CMesh* mesh, const Int32 materialIndex )
	{
		if ( !mesh || materialIndex < 0 || materialIndex >= mesh->GetMaterials().SizeInt() )
			return false;

#ifndef NO_RESOURCE_IMPORT
		// HACK for interior volumes: thanks KK...
		const String& materialName = mesh->GetMaterialNames()[ materialIndex ];
		if ( materialName == TXT("volume") )
			return false;
#endif

		// generic material case
		return IsSolidShadowMaterial( mesh->GetMaterials()[ materialIndex ] );
	}
}

//------------------

CMeshPackingSource::PackedStreams::PackedStreams()
{
	Red::MemoryZero( &m_vertices, sizeof(m_vertices) );
	m_indices = nullptr;
}

CMeshPackingSource::PackedStreams::~PackedStreams()
{
}

//------------------

CMeshPackingSource::CMeshPackingSource()
{
}

CMeshPackingSource::~CMeshPackingSource()
{
	m_vertices.ClearPtrFast();
	m_indices.ClearPtrFast();

	m_chunks.ClearFast();
	m_lods.ClearFast();
}

CMeshPackingSource::PackedBuffer* CMeshPackingSource::CreateVertexBuffer( CPagedMemoryBuffer*& memory )
{
	// compute CRC
	const Uint64 hash = Helper::ComputeBufferHash64( *memory );

	// already exists ?
	PackedBuffer* cachedBuffer = nullptr;
	for ( Uint32 i=0; i<m_vertices.Size(); ++i )
	{
		if ( m_vertices[i]->m_hash == hash )
		{
			// delete temporary shit
			delete memory;
			memory = nullptr;

			// return shared buffer
			return m_vertices[i];
		}
	}

	// create new buffer entry
	PackedBuffer* buffer = new PackedBuffer();
	buffer->m_hash = hash;
	buffer->m_placement = 0;
	buffer->m_data = memory; // keep alive

	// insert
	m_vertices.PushBack( buffer );
	return buffer;
}

CMeshPackingSource::PackedBuffer* CMeshPackingSource::CreateIndexBuffer( CPagedMemoryBuffer*& memory )
{
	// compute CRC
	const Uint64 hash = Helper::ComputeBufferHash64( *memory );

	// already exists ?
	PackedBuffer* cachedBuffer = nullptr;
	for ( Uint32 i=0; i<m_indices.Size(); ++i )
	{
		if ( m_indices[i]->m_hash == hash )
		{
			// delete temporary shit
			delete memory;
			memory = nullptr;

			// return shared buffer
			return m_indices[i];
		}
	}

	// create new buffer entry
	PackedBuffer* buffer = new PackedBuffer();
	buffer->m_hash = hash;
	buffer->m_placement = 0;
	buffer->m_data = memory; // keep alive

	// insert
	m_indices.PushBack( buffer );
	return buffer;
}

Bool CMeshPackingSource::Pack( const CMesh* mesh )
{
	// unpack mesh data
	CMeshData meshData( mesh );
	if ( meshData.GetChunks().Empty() )
		return false;

	const Uint32 numLODs = mesh->GetMeshLODLevels().Size();
	const Uint32 numChunks = mesh->GetChunks().Size();
	const Uint32 numMaterials = mesh->GetMaterials().Size();

	// prepare memory
	m_lods.Reserve( numLODs );
	m_chunks.Reserve( numChunks*2 ); // we may have shadow chunks

	// Prepare LOD stuff
	for ( const auto& lod : mesh->GetMeshLODLevels() )
	{
		PackedLod newLod;
		newLod.m_distance = lod.GetDistance();
		m_lods.PushBack( newLod );
	}

	// Which render masks channels can be merged ?
	const auto mergableChunks = MCR_Cascade1 | MCR_Cascade2 | MCR_Cascade3 | MCR_Cascade4;

	// Pack data into render mesh chunks with gpuapi vertex and index buffers
	const auto& sourceChunks = meshData.GetChunks();
	TDynArray< Uint32 > shadowChunksToMerge;
	for ( Uint32 chunkIndex = 0; chunkIndex<sourceChunks.Size(); ++chunkIndex )
	{
		const SMeshChunk& sourceChunk = sourceChunks[chunkIndex];

		// Is this chunk going to be packed into the shadow chunk ?
		// NOTE: masked materials cannot be in packed into the shadow chunks
		// NOTE: this is how the filtering happens: the normal mesh chunks that will be merged into big shadow chunk are masked out here, so we are NOT rendering them into the shadow maps
		Uint8 renderMask = sourceChunk.m_renderMask;
		if ( sourceChunk.m_materialID < mesh->GetMaterials().Size() )
		{
			IMaterial* material = mesh->GetMaterials()[ sourceChunk.m_materialID ];
			const Bool usesSolidMaterial = Helper::IsSolidShadowMaterial( mesh, sourceChunk.m_materialID );
			const Bool willBeInOtherShadowChunk = ((sourceChunk.m_renderMask & mergableChunks) != 0) && usesSolidMaterial && Helper::CanMergeChunksShadows( mesh, chunkIndex );

			// Add chunk to merge list
			if ( willBeInOtherShadowChunk )
			{
				shadowChunksToMerge.PushBack( chunkIndex ); // source chunk index
				renderMask &= ~mergableChunks;
			}
		}
		else
		{
			ERR_ENGINE( TXT("Chunk %d uses non existing material %d in mesh '%ls'"), 
				chunkIndex, sourceChunk.m_materialID, mesh->GetDepotPath().AsChar() );

			// do not pack chunks with invalid material
			continue;
		}

		// Compute new render mask
		if ( renderMask == 0 )
			continue; // chunk is not going to be renderer on it's own, it's always merged

		// Copy obvious data to chunk
		PackedChunk* newChunk = new ( m_chunks ) PackedChunk();
		newChunk->m_materialId = (Uint8)sourceChunk.m_materialID;
		newChunk->m_baseRenderMask = sourceChunk.m_renderMask; // always use the orignal
		newChunk->m_mergedRenderMask = renderMask;
		newChunk->m_lodMask = Helper::GetChunkLODMask( mesh, chunkIndex );
		newChunk->m_numIndices = sourceChunk.m_numIndices;
		newChunk->m_numVertices = sourceChunk.m_numVertices;

		// Setup vertex packing
		newChunk->m_vertexFactory = (Uint8)MeshUtilities::GetVertexFactoryForMeshChunk( sourceChunk );
		newChunk->m_vertexType = MeshUtilities::GetVertexTypeForMeshChunk( mesh->CanUseExtraStreams(), sourceChunk );
		newChunk->m_vertexPacking = GpuApi::GetPackingForFormat( newChunk->m_vertexType );

		// Only one chunk for normal geometry
		SourceChunk sourceChunkInfo;
		sourceChunkInfo.m_baseVertexIndex = 0;
		sourceChunkInfo.m_chunkIndex = chunkIndex;
		newChunk->m_sourceChunks.PushBack( sourceChunkInfo );
	}

	// Find first non-masked material
	Int32 nonMaskedMaterial = -1;
	for ( Uint32 i=0; i<numMaterials; ++i )
	{
		if ( Helper::IsSolidShadowMaterial( mesh, i ) )
		{
			nonMaskedMaterial = i;
			break;
		}
	}

	// We can only build merged shadow mesh if we have non masked material in the mesh
	if ( nonMaskedMaterial != -1 )
	{
		// Pack shadow mesh for each LOD
		for ( Uint32 i=0; i<numLODs; ++i )
		{
			// Process each group - cascades + local shadows separately
			const Uint8 shadowMasks[] = { MCR_Cascade1, MCR_Cascade2, MCR_Cascade3, MCR_Cascade4 };
			for ( Uint32 smIndex=0; smIndex<ARRAY_COUNT(shadowMasks); smIndex++ )
			{
				Uint32 chunkToMerge = 0;
				while ( chunkToMerge < shadowChunksToMerge.Size() )
				{
					PackedChunk shadowChunk;
					shadowChunk.m_materialId = nonMaskedMaterial; // use one common material
					shadowChunk.m_lodMask = (1 << i);
					shadowChunk.m_baseRenderMask = 0; // this chunk is not a part of original mesh
					shadowChunk.m_mergedRenderMask = shadowMasks[smIndex];

					// try to merge as much as possible
					// there's a limit though of 64K vertices we can address
					Uint32 baseVertexIndex = 0 ;
					while ( chunkToMerge < shadowChunksToMerge.Size() )
					{
						const auto sourceChunkIndex = shadowChunksToMerge[chunkToMerge];
						const SMeshChunk& sourceChunk = sourceChunks[ sourceChunkIndex ];

						// will it fit ?
						if ( baseVertexIndex + sourceChunk.m_numVertices >= 65536 )
							break;

						// is it for this LOD?
						const Uint8 chunkLODMask = Helper::GetChunkLODMask( mesh, sourceChunkIndex );
						if ( chunkLODMask & shadowChunk.m_lodMask )
						{
							// should we merge it for this mask ?
							if ( (sourceChunk.m_renderMask & shadowMasks[smIndex]) != 0 )
							{
								SourceChunk sourceChunkInfo;
								sourceChunkInfo.m_baseVertexIndex = baseVertexIndex;
								sourceChunkInfo.m_chunkIndex = sourceChunkIndex;

								shadowChunk.m_sourceChunks.PushBack( sourceChunkInfo );
								shadowChunk.m_numIndices += sourceChunk.m_numIndices;
								shadowChunk.m_numVertices += sourceChunk.m_numVertices;

								baseVertexIndex += sourceChunk.m_numVertices;
							}
						}

						// advance
						chunkToMerge += 1;
					}

					// push it if it's worth it
					if ( !shadowChunk.m_sourceChunks.Empty() )
					{
						// Setup vertex packing for the shadow mesh
						shadowChunk.m_vertexFactory = MVF_MeshStatic; // always use this shit
						shadowChunk.m_vertexType = GpuApi::BCT_VertexMeshStaticPositionOnly; // only positions are needed for shadow meshes
						shadowChunk.m_vertexPacking = GpuApi::GetPackingForFormat( shadowChunk.m_vertexType );
						m_chunks.PushBack( shadowChunk );
					}
				}
			}
		}
	}

	// stats
	LOG_ENGINE( TXT("Build %d candidate chunks, %d LODs from %d chunks in mesh %ls"), 
		m_chunks.Size(), numLODs, numChunks, mesh->GetFriendlyName().AsChar() );

	// initialize GLOBAL packing parameters for the whole mesh packing
	GpuApi::VertexPacking::PackingVertexLayout vertexLayout = MeshUtilities::GetVertexLayout();

	// Determine packing params ( quantization extents )
	GpuApi::VertexPacking::PackingParams packingParams;
	const Box& meshBounds = mesh->GetBoundingBox();
	GpuApi::VertexPacking::InitPackingParams( 
		packingParams,
		meshBounds.Min.X, meshBounds.Min.Y, meshBounds.Min.Z,
		meshBounds.Max.X, meshBounds.Max.Y, meshBounds.Max.Z );

	// Buffer map
	TSortedMap< Uint64, CPagedMemoryBuffer* > vertexBuffers;
	TSortedMap< Uint64, CPagedMemoryBuffer* > indexBuffers;

	// start packing, try not to pack something that we already have
	Uint32 initialPackedVertexSize = 0;
	Uint32 initialPackedIndexSize = 0;
	for ( PackedChunk& chunk : m_chunks )
	{
		// Get number of streams in the packer
		const Uint32 numStreams = GpuApi::VertexPacking::GetStreamCount( chunk.m_vertexPacking );

		// Calculate packing sizes
		Uint32 vertexSizes[ NUM_STREAMS ] = {0};
		GpuApi::VertexPacking::CalcElementListSizes( chunk.m_vertexPacking, vertexSizes, NUM_STREAMS );

		// Create packing data
		CPagedMemoryBuffer* packedVertexData[ NUM_STREAMS ] = { nullptr };
		CPagedMemoryBuffer* packedIndexData = new CPagedMemoryBuffer();
		
		// Pack chunks (each of the source chunks)
		for ( Uint32 i=0; i<chunk.m_sourceChunks.Size(); ++i )
		{
			// Get first source chunk
			const Uint32 sourceChunkIndex = chunk.m_sourceChunks[i].m_chunkIndex;
			const Uint32 sourceChunkVertexOffset = chunk.m_sourceChunks[i].m_baseVertexIndex;
			const auto& sourceChunk = meshData.GetChunks()[ sourceChunkIndex ];

			// Pack vertices
			{
				const Uint32 vertexCount = sourceChunk.m_vertices.Size();
				const void* vertexData = sourceChunk.m_vertices.TypedData();

				for ( Uint32 streamIndex = 0; streamIndex < numStreams; ++streamIndex )
				{
					if ( !packedVertexData[ streamIndex ] )
					{
						packedVertexData[ streamIndex ] = new CPagedMemoryBuffer();
					}

					const Uint32 verifyoffset = packedVertexData[ streamIndex ]->GetTotalSize() / vertexSizes[ streamIndex ];
					RED_FATAL_ASSERT( verifyoffset == sourceChunkVertexOffset, "Vertex packing offset mismatch: %d != %d", verifyoffset, sourceChunkVertexOffset );

					CRenderMeshBufferWriterPaged writer( *packedVertexData[ streamIndex ] );
					GpuApi::VertexPacking::PackVertices( &writer, chunk.m_vertexPacking, packingParams, vertexLayout, vertexData, vertexCount, streamIndex );

					initialPackedVertexSize += writer.GetOffset();
				}
			}

			// Pack indices
			{
				const void* indexData = sourceChunk.m_indices.TypedData();
				const Uint32 indexCount = sourceChunk.m_indices.Size();

				CRenderMeshBufferWriterPaged writer( *packedIndexData );
				GpuApi::VertexPacking::PackIndices( &writer, GpuApi::VertexPacking::PT_Index16, indexData, indexCount, sourceChunkVertexOffset );
				initialPackedIndexSize += writer.GetOffset();
				
			}
		}

		// Create final buffers
		for ( Uint32 streamIndex = 0; streamIndex < numStreams; ++streamIndex )
		{
			if ( packedVertexData[ streamIndex ] )
			{
				chunk.m_streams.m_vertices[ streamIndex ] = CreateVertexBuffer( packedVertexData[ streamIndex ] );
			}
			else
			{
				chunk.m_streams.m_vertices[ streamIndex ] = nullptr;
			}
		}

		// Create final index buffer
		chunk.m_streams.m_indices = CreateIndexBuffer( packedIndexData );
	}

	// compute data layout
	Uint32 mergedPackedVertexSize = 0;
	Uint32 mergedPackedIndexSize = 0;
	ComputeLayout( 16, 2, mergedPackedVertexSize, mergedPackedIndexSize );

	// final post merge stats
	LOG_ENGINE( TXT("Initial data size: %1.2fKB vertex, %1.2fKB index"), 
		initialPackedVertexSize / 1024.0f, initialPackedIndexSize / 1024.0f );

	LOG_ENGINE( TXT("Final data size: %1.2fKB vertex, %1.2fKB index (%d vertex buffers, %d index buffers)"), 
		mergedPackedVertexSize / 1024.0f, mergedPackedIndexSize / 1024.0f, 
		m_vertices.Size(), m_indices.Size() );

	// packed
	return true;
}

void CMeshPackingSource::ComputeLayout( const Uint32 vertexDataAlignment, const Uint32 indexDataAlignment, Uint32& outVertexDataSize, Uint32& outIndexDataSize )
{
	outVertexDataSize = 0;
	outIndexDataSize = 0;

	for ( auto* buffer : m_vertices )
	{
		buffer->m_placement = (Uint32) AlignOffset( outVertexDataSize, vertexDataAlignment );
		outVertexDataSize = buffer->m_placement + buffer->m_data->GetTotalSize();
	}
	
	for ( auto* buffer : m_indices )
	{
		buffer->m_placement = (Uint32) AlignOffset( outIndexDataSize, indexDataAlignment );
		outIndexDataSize = buffer->m_placement + buffer->m_data->GetTotalSize();
	}
}

void CMeshPackingSource::CopyVertexData( void* outputMemory, const Uint32 bufferSize ) const
{
	for ( auto* buffer : m_vertices )
	{
		Uint8* targetData = (Uint8*) outputMemory + buffer->m_placement;
		const Uint32 sizeLeft = bufferSize - buffer->m_placement;
		buffer->m_data->CopyData( targetData, sizeLeft );
	}
}

void CMeshPackingSource::CopyIndexData( void* outputMemory, const Uint32 bufferSize  ) const
{
	for ( auto* buffer : m_indices )
	{
		Uint8* targetData = (Uint8*) outputMemory + buffer->m_placement;
		const Uint32 sizeLeft = bufferSize - buffer->m_placement;
		buffer->m_data->CopyData( targetData, sizeLeft );
	}
}
