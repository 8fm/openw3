/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderMesh.h"
#include "renderSkinningData.h"
#include "renderMeshBufferWriter.h"

#include "..\gpuApiUtils\gpuApiVertexFormats.h"
#include "..\gpuApiUtils\gpuApiVertexPacking.h"
#include "..\gpuApiUtils\gpuApiMemory.h"
#include <stddef.h> // for offsetof
#include "../engine/mesh.h"
#include "../engine/material.h"
#include "../engine/meshDataBuilder.h"
#include "../core/memoryFileReader.h"
#include "../core/sortedmap.h"
#include "../core/configVar.h"
#include "../core/ioTags.h"

#ifndef RED_FINAL_BUILD
#include "renderResourceIterator.h"
#endif

namespace Config
{
	TConfigVar< Bool >		cvForceSyncMeshLoading( "Rendering/MeshLoading", "ForceSyncMeshLoading", false, eConsoleVarFlag_Developer );
	TConfigVar< Float >		cvAutoRefreshTime( "Rendering/MeshLoading", "AutoRefreshTime", 0.1f, eConsoleVarFlag_Developer );
	TConfigVar< Int32 >		cvLoadingDelay( "Rendering/MeshLoading", "LoadingDelay", 0, eConsoleVarFlag_Developer );
}

//------------------------

Red::Threads::CAtomic< Int32 > GNumMeshes(0);
Red::Threads::CAtomic< Int32 > GNumMeshesDataSize(0);

//------------------------

IMPLEMENT_RENDER_RESOURCE_ITERATOR_WITH_CACHE( CRenderMesh );

//------------------------

CRenderMesh::CRenderMesh()
	: m_batchList( nullptr )
	, m_batchNext( nullptr )
	, m_quantizationScale( 1,1,1,1 )
	, m_quantizationOffset( 0,0,0,0 )
	, m_buffersPendingLoad( 2 )
	, m_clusterBuilderTask( nullptr )
{}

CRenderMesh::CRenderMesh( const CRenderMesh& toCopy )
	: m_batchList( nullptr )
	, m_batchNext( nullptr )
	, m_quantizationScale( toCopy.m_quantizationScale )
	, m_quantizationOffset( toCopy.m_quantizationOffset )
	, m_vertices( toCopy.m_vertices )
	, m_indices( toCopy.m_indices )
	, m_chunks( toCopy.m_chunks ) 
	, m_lods( toCopy.m_lods )
	, m_bonePositions( toCopy.m_bonePositions )
	, m_buffersPendingLoad( 0 )
	, m_clusterBuilderTask( nullptr )
{
	// We can only copy fully loaded meshes
	RED_FATAL_ASSERT( toCopy.IsFullyLoaded(), "Trying to copy mesh that is not fully loaded" );

	GpuApi::AddRef( m_indices );
	GpuApi::AddRef( m_vertices );

	GpuApi::AddRefIfValid( m_bonePositions );
}

CRenderMesh::~CRenderMesh()
{
	GpuApi::SafeRelease( m_bonePositions );

	// Remove indices
	GpuApi::SafeRelease( m_indices );

	if( m_cullingBuffers.Size() > 0 )
	{
		FinishClusterBuildingTaskOnDestruction();

		for( Uint32 i = 0; i < m_cullingBuffers.Size(); ++i )
		{
			GpuApi::SafeRelease( m_cullingBuffers[i].m_indirectArgs );
			GpuApi::SafeRelease(m_cullingBuffers[i].m_culledIndices );
			m_cullingBuffers[i].m_clusters.Clear();
		}
		m_cullingBuffers.Clear();
	}

	// Remove vertices
	GpuApi::SafeRelease( m_vertices );

	// Stats
#ifndef RED_FINAL_BUILD
	if ( m_dataSize )
	{
		GNumMeshes.Decrement();
		GNumMeshesDataSize.ExchangeAdd( -(Int32)m_dataSize );
		LOG_RENDERER( TXT("--Mesh %d: %hs, %1.2fKB (%1.2fKB TOTAL)"), 
			GNumMeshes.GetValue(),
			m_depotPath.AsChar(),
			m_dataSize / 1024.0f,
			GNumMeshesDataSize.GetValue() / 1024.0f );
	}
#endif
}

void CRenderMesh::GetQuantization( Vector* scale, Vector* offset ) const
{
	if ( scale != nullptr )
	{
		*scale = m_quantizationScale;
	}
	if ( offset != nullptr )
	{
		*offset = m_quantizationOffset;
	}
}

void CRenderMesh::BindCsVsBuffersCS( Uint32 chunkIndex, Vector& qs, Vector& qb, Uint32& indexOffset, Uint32& vertexOffset )
{
	RED_FATAL_ASSERT( m_buffersPendingLoad.GetValue() == 0, "Binding a mesh that isn't done loading!" );
	RED_FATAL_ASSERT( chunkIndex < m_chunks.Size(), "Mesh chunk index out of range. %d >= %d", chunkIndex, m_chunks.Size() );

	qs = m_quantizationScale;
	qb = m_quantizationOffset;

	ChunkGeometry geometry;
	GetChunkGeometry( chunkIndex, geometry );

	indexOffset = geometry.m_indexOffset;
	vertexOffset = geometry.m_vertexOffsets[0];

	GpuApi::BindBufferUAV( m_cullingBuffers[ chunkIndex ].m_indirectArgs, 0 );
	GpuApi::BindBufferUAV( m_cullingBuffers[ chunkIndex ].m_culledIndices, 1 );
	GpuApi::BindBufferSRV(geometry.m_vertexBuffers[0], 0, GpuApi::eShaderType::ComputeShader );
	GpuApi::BindBufferSRV(geometry.m_indexBuffer, 1, GpuApi::eShaderType::ComputeShader );
}

void CRenderMesh::BindBuffersVS( Uint32 chunkIndex )
{
	ChunkGeometry geometry;
	GetChunkGeometry( chunkIndex, geometry );

	GetRenderer()->GetStateManager().SetVertexShaderConstRaw( VSC_QS, (const Float*)&m_quantizationScale, 2 );

	const GpuApi::VertexLayoutDesc* layoutDesc = GpuApi::GetVertexLayoutDesc( geometry.m_vertexLayout );
	const Uint32 stride = layoutDesc->GetSlotStride( 0 );

	GpuApi::BindVertexBuffers( 0, 1, &geometry.m_vertexBuffers[0], &stride, &geometry.m_vertexOffsets[0] );
	GpuApi::BindIndexBuffer( geometry.m_indexBuffer, geometry.m_indexOffset );
}

void CRenderMesh::BindCsVsBuffersVS( Uint32 chunkIndex )
{
	ChunkGeometry geometry;
	GetChunkGeometry( chunkIndex, geometry );

	GetRenderer()->GetStateManager().SetVertexShaderConstRaw( VSC_QS, (const Float*)&m_quantizationScale, 2 );

	const GpuApi::VertexLayoutDesc* layoutDesc = GpuApi::GetVertexLayoutDesc( geometry.m_vertexLayout );
	const Uint32 stride = layoutDesc->GetSlotStride( 0 );

	GpuApi::BindVertexBuffers( 0, 1, &geometry.m_vertexBuffers[0], &stride, &geometry.m_vertexOffsets[0] );
	//GpuApi::BindIndexBuffer( geometry.m_indexBuffer, geometry.m_indexOffset );
	GpuApi::BindIndexBuffer( m_cullingBuffers[ chunkIndex ].m_culledIndices );
	GpuApi::BindIndirectArgs( m_cullingBuffers[ chunkIndex ].m_indirectArgs );
}

void CRenderMesh::Bind( Uint32 chunkIndex, Uint32 streamsToBind )
{
	RED_FATAL_ASSERT( m_buffersPendingLoad.GetValue() == 0, "Binding a mesh that isn't done loading!" );
	RED_FATAL_ASSERT( chunkIndex < m_chunks.Size(), "Mesh chunk index out of range. %d >= %d", chunkIndex, m_chunks.Size() );

	// Bind quantization data
	static_assert( offsetof( CRenderMesh, m_quantizationOffset ) == offsetof( CRenderMesh, m_quantizationScale ) + sizeof( Vector ), "m_quantizationOffset must follow immediately after m_quantizationScale" );
	static_assert( VSC_QB == VSC_QS + 1, "VSC_QB should be VSC_QS + 1. This needs to be adjusted" );
	GetRenderer()->GetStateManager().SetVertexShaderConstRaw( VSC_QS, (const Float*)&m_quantizationScale, 2 );

	ChunkGeometry geometry;
	GetChunkGeometry( chunkIndex, geometry );

	GpuApi::SetVertexFormatRaw( geometry.m_vertexLayout, false );
	GpuApi::BindIndexBuffer( geometry.m_indexBuffer, geometry.m_indexOffset );


	// Already have offsets and buffers, just need to extract strides from the vertex layout. While we're at it we can
	// filter out the unused streams.
	const GpuApi::VertexLayoutDesc* layoutDesc = GpuApi::GetVertexLayoutDesc( geometry.m_vertexLayout );

	// The streams we want to bind are exactly those that were requested and that have data in the layout.
	const Uint32 usedStreams = streamsToBind & layoutDesc->m_slotMask;

	Uint32 vertexStrides[RENDER_MESH_NUM_STREAMS];
	for ( Uint32 i = 0; i < RENDER_MESH_NUM_STREAMS; ++i )
	{
		if ( ( usedStreams & ( 1 << i ) ) != 0 )
		{
			const Uint32 stride = layoutDesc->GetSlotStride( i );
			RED_ASSERT( stride != 0, TXT("Slot %u was included in layout mask, but has 0 stride"), i );
			vertexStrides[i] = stride;
		}
		else
		{
			geometry.m_vertexBuffers[i] = GpuApi::BufferRef();
			vertexStrides[i] = 0;
		}
	}

	GpuApi::BindVertexBuffers( 0, RENDER_MESH_NUM_STREAMS, geometry.m_vertexBuffers, vertexStrides, geometry.m_vertexOffsets );
}


void CRenderMesh::GetChunkGeometry( Uint8 chunkIndex, ChunkGeometry& outGeometry ) const
{
	RED_FATAL_ASSERT( chunkIndex < m_chunks.Size(), "Chunk index out of bounds! Caller is responsible for this!" );

	const Chunk& chunk = m_chunks[ chunkIndex ];

	outGeometry.m_vertexLayout = GpuApi::GetVertexLayoutForChunkType( chunk.m_chunkVertices.type );

	GpuApi::BufferRef vertexBuffer = m_vertices;
	for ( Uint32 i = 0; i < RENDER_MESH_NUM_STREAMS; ++i )
	{
		outGeometry.m_vertexBuffers[ i ] = vertexBuffer;
	}

	Red::System::MemoryCopy( outGeometry.m_vertexOffsets, chunk.m_chunkVertices.byteOffsets, sizeof( outGeometry.m_vertexOffsets ) );

	outGeometry.m_indexBuffer = m_indices;
	outGeometry.m_indexOffset = chunk.m_chunkIndices.byteOffset;
}

GpuApi::VertexLayoutRef CRenderMesh::GetChunkVertexLayout( Uint8 chunkIndex ) const
{
	if ( chunkIndex >= m_chunks.Size() )
	{
		RED_HALT( "Mesh chunk index out of range. %d >= %d", chunkIndex, m_chunks.Size() );
		return GpuApi::VertexLayoutRef::Null();
	}

	const Chunk& chunk = m_chunks[ chunkIndex ];
	return GpuApi::GetVertexLayoutForChunkType( chunk.m_chunkVertices.type );
}


void CRenderMesh::DrawChunkNoBind( Uint32 chunkIndex, Uint32 fragCount, MeshDrawingStats& outMeshStats )
{
	if ( !m_indices || !m_vertices )
	{
		return;
	}

	RED_ASSERT( chunkIndex < m_chunks.Size(), TXT("Mesh chunk index out of range. %d >= %d"), chunkIndex, m_chunks.Size() );

	// Draw chunk
	const Chunk& chunk = m_chunks[ chunkIndex ];
	GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, chunk.m_numVertices, 0, chunk.m_numIndices / 3 );

#ifndef RED_FINAL_BUILD
	// Stats
	outMeshStats.Append( chunk.m_vertexFactory, chunk.m_numVertices, chunk.m_numIndices, fragCount, 1 );
#endif
}

void CRenderMesh::DrawChunkNoBindInstanced( Uint32 chunkIndex, Uint32 fragCount, Uint32 instanceCount, MeshDrawingStats& outMeshStats )
{
	if ( !m_indices || !m_vertices )
	{
		return;
	}

	RED_ASSERT( chunkIndex < m_chunks.Size(), TXT("Mesh chunk index out of range. %d >= %d"), chunkIndex, m_chunks.Size() );

	const Chunk& chunk = m_chunks[ chunkIndex ];
	GpuApi::DrawInstancedIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, chunk.m_numVertices, 0, chunk.m_numIndices / 3, instanceCount );

#ifndef RED_FINAL_BUILD
	// Stats
	outMeshStats.Append( chunk.m_vertexFactory, chunk.m_numVertices, chunk.m_numIndices, fragCount, instanceCount );	
#endif
}

CName CRenderMesh::GetCategory() const
{
	return CNAME( RenderMesh );
}

Uint32 CRenderMesh::GetUsedVideoMemory() const
{
	Uint32 size = sizeof( *this );
	size += (m_indices ? GpuApi::GetBufferDesc(m_indices).size  : 0 );
	size += (m_vertices ? GpuApi::GetBufferDesc(m_vertices).size  : 0 );
	return size;
}


//////////////////////////////////////////////////////////////////////////

static TDynArray< BufferAsyncDataHandle > GAliveMTF;

Uint32 GNumMeshesLoadedFromStreamingCache = 0;
Uint64 GNumMeshBytesLoadedFromStreamingCache = 0;

// W3 hack: direct access to resource prefetcher
extern void* GetPrefetchedStreamingData( const Uint64 hash, Uint32 expectedDataSize, std::function< void* ( Uint32 dataSize, Uint16 alignment ) > allocator, std::function< void ( void* memory ) > deallocator );

CRenderMesh* CRenderMesh::CreateCooked( const CMesh* mesh, Uint64 partialRegistrationHash )
{
	const SMeshCookedData& data = mesh->GetCookedData();

	TScopedRenderResourceCreationObject< CRenderMesh > createdRenderMesh ( partialRegistrationHash );

	// no data
	if ( !data.m_renderBuffer.GetSize() )
	{
		return nullptr;
	}

	createdRenderMesh.InitResource( new CRenderMesh() );
	
	// stats
#ifndef RED_FINAL_BUILD
	GNumMeshes.Increment();
	GNumMeshesDataSize.ExchangeAdd( data.m_renderBuffer.GetSize() );
	createdRenderMesh->m_depotPath = UNICODE_TO_ANSI( mesh->GetDepotPath().AsChar() );
	createdRenderMesh->m_dataSize = data.m_renderBuffer.GetSize();
	LOG_RENDERER( TXT("++Mesh %d: %hs, %1.2fKB (%1.2fKB TOTAL)"), 
		GNumMeshes.GetValue(),
		createdRenderMesh->m_depotPath.AsChar(),
		createdRenderMesh->m_dataSize / 1024.0f,
		GNumMeshesDataSize.GetValue() / 1024.0f );
#endif

	// Load cooked render chunks
	CMemoryFileReaderExternalBuffer systemDataReader( data.m_renderChunks.Data(), (Uint32)data.m_renderChunks.DataSize() );
	systemDataReader << createdRenderMesh->m_chunks;

	// copy basic stuff
	createdRenderMesh->m_quantizationOffset	= data.m_quantizationOffset;
	createdRenderMesh->m_quantizationScale	= data.m_quantizationScale;	

	// copy LODs
	createdRenderMesh->m_lods.Resize( data.m_renderLODs.Size() );
	for ( Uint32 i=0; i<data.m_renderLODs.Size(); ++i )
	{
		createdRenderMesh->m_lods[i].m_distance = data.m_renderLODs[i];
	}

	// copy buffer sizes
	const Uint32 vertexBufferSize = data.m_vertexBufferSize;
	const Uint32 indexBufferSize = data.m_indexBufferSize;
	const Uint32 indexBufferOffset = data.m_indexBufferOffset;
	const Uint32 totalDataSize = data.m_renderBuffer.GetSize();

	String depotPath = mesh->GetDepotPath();

	// get data
	if ( Config::cvForceSyncMeshLoading.Get() )
	{
		BufferHandle renderData = data.m_renderBuffer.AcquireBufferHandleSync();		
		if ( !renderData )
		{
			return nullptr;
		}

		// Upload buffers
		createdRenderMesh->SetupRenderData( renderData, vertexBufferSize, indexBufferSize, indexBufferOffset, depotPath );
	}
	else
	{
		// get resource depot path and hash
		const String depotPath = mesh->GetDepotPath();
		const Uint64 depotHash = Red::CalculatePathHash64( depotPath.AsChar() );

		// should we add mesh priorities or types ? 
		const bool isShadowMesh = depotPath.ContainsSubstring( TXT("shadow") );
		const auto tag = isShadowMesh ? eIOTag_MeshesImmediate : eIOTag_MeshesNormal;

		// start async loading of vertex data
		TRenderObjectPtr< CRenderMesh > renderMeshPtr;
		renderMeshPtr.ResetFromExternal( createdRenderMesh.GetResource() );

#ifdef RED_PLATFORM_CONSOLE
		{
			// function to allocate memory
			auto allocFunc = [renderMeshPtr, vertexBufferSize, indexBufferSize, indexBufferOffset](Uint32 size, Uint16 alignment) -> void*
			{
				renderMeshPtr->AllocateInplaceRenderData( size, indexBufferSize, vertexBufferSize, indexBufferOffset, alignment );

				return renderMeshPtr->GetInPlaceVertexRegionPtr();
			};

			// function to deallocate memory
			auto deallocFunc = [renderMeshPtr](void*)
			{
				renderMeshPtr->DeallocateInplaceRenderData();
			};

			// function to process loaded data
			auto callbackFunc = [renderMeshPtr, vertexBufferSize, indexBufferSize, indexBufferOffset, depotPath](BufferHandle renderData)
			{				
				// buffer loaded
				GetRenderer()->GetMeshStreamingStats().ReportBufferLoaded( renderData->GetSize() );

				// process data
				renderMeshPtr->SetupRenderData( renderData, vertexBufferSize, indexBufferSize, indexBufferOffset, depotPath );
				Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( renderMeshPtr->m_bufferLoadingFence );
				renderMeshPtr->m_bufferLoading.Reset();
			};

			// update mesh streaming stats
			GetRenderer()->GetMeshStreamingStats().ReportBufferPending( data.m_renderBuffer.GetSize() );

			// W3 hack: try to get the mesh data from the streaming cache
			void* allocatedMem = GetPrefetchedStreamingData( depotHash, data.m_renderBuffer.GetSize(), allocFunc, deallocFunc );
			if ( allocatedMem )
			{
				// create fake wrapper over the just loaded data and call the callback
				BufferHandle buf( new BufferProxy( allocatedMem, data.m_renderBuffer.GetSize(), deallocFunc ) );
				callbackFunc( buf );

				// stats
				GNumMeshesLoadedFromStreamingCache += 1;
				GNumMeshBytesLoadedFromStreamingCache += data.m_renderBuffer.GetSize();
			}
			else
			{
				// load asynchronously from disk (old, slower pipeline)
				// The mesh buffers will be used in-place, to avoid extra copies and ensure we have ownership of the memory
				Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( renderMeshPtr->m_bufferLoadingFence );
				createdRenderMesh->m_bufferLoading = data.m_renderBuffer.AcquireBufferHandleAsync( tag, callbackFunc, allocFunc, deallocFunc );
			};
		}
	#else
		{
			auto callbackFunc = [renderMeshPtr, vertexBufferSize, indexBufferSize, indexBufferOffset, depotPath](BufferHandle renderData)
			{
				// buffer loaded
				GetRenderer()->GetMeshStreamingStats().ReportBufferLoaded( renderData->GetSize() );

				// process data
				renderMeshPtr->SetupRenderData(renderData, vertexBufferSize, indexBufferSize, indexBufferOffset, depotPath );
				Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( renderMeshPtr->m_bufferLoadingFence );
				renderMeshPtr->m_bufferLoading.Reset();
			};

			Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( createdRenderMesh->m_bufferLoadingFence );
			createdRenderMesh->m_bufferLoading = data.m_renderBuffer.AcquireBufferHandleAsync( tag, callbackFunc );
		}
	#endif
	}

	// Return created mesh
	return createdRenderMesh.RetrieveSuccessfullyCreated();
}

void CRenderMesh::SetupRenderData( BufferHandle cookedRenderData, const Uint32 vertexDataSize, const Uint32 indexDataSize, const Uint32 indexDataOffset, const String& debugName )
{
#ifndef RED_FINAL_BUILD
	// fake wait
	if ( Config::cvLoadingDelay.Get() )
	{
		Red::Threads::SleepOnCurrentThread( Config::cvLoadingDelay.Get() );
	}
#endif

	// Create buffers
	const void* deviceBufferData = cookedRenderData->GetData();
	if ( deviceBufferData )
	{
		// Flush CPU Cache for the full combined buffer
		GpuApi::FlushCpuCache( deviceBufferData, cookedRenderData->GetSize() );

		const Bool isMerged = debugName.ContainsSubstring(TXT("shadow_mesh"));

		// vertex buffer
		{
			GpuApi::BufferInitData bufInitData;

			// determine the memory for vertex buffer
			if ( m_vertexMemRegion.IsValid() )
			{
				// use in place memory if provided
				bufInitData.m_memRegionHandle = m_vertexMemRegion;
			}
			else
			{
				// allocate new memory and copy content from passed data
				bufInitData.m_buffer = OffsetPtr( deviceBufferData, 0 );
			}

			// create vertex buffer
			GpuApi::eBufferChunkCategory category;
#ifdef RED_PLATFORM_ORBIS
			category = isMerged ? GpuApi::BCC_VertexSRV : GpuApi::BCC_Vertex;
#else
			category = GpuApi::BCC_Vertex;
#endif
			GpuApi::eBufferUsageType usage = GpuApi::BUT_ImmutableInPlace;
			bufInitData.m_elementCount = vertexDataSize / 8;
			GpuApi::BufferRef vb = GpuApi::CreateBuffer( vertexDataSize, category, usage, 0, &bufInitData );
			if ( !vb.isNull() )
			{
#if !defined(RED_FINAL_BUILD) || defined( RED_PROFILE_BUILD ) 
				// name the buffers
				GpuApi::SetBufferDebugPath( vb, UNICODE_TO_ANSI(debugName.AsChar()) );
#endif
				// setup binding
				m_vertices = vb;

				// make more ready for rendering
				m_buffersPendingLoad.Decrement();
			}
		}

		// index buffer
		{
			GpuApi::BufferInitData bufInitData;

			// determine the memory for vertex buffer
			if ( m_indexMemRegion.IsValid() )
			{
				// use in place memory if provided
				bufInitData.m_memRegionHandle = m_indexMemRegion;
			}
			else
			{
				// allocate new memory and copy content from passed data
				bufInitData.m_buffer = OffsetPtr( deviceBufferData, indexDataOffset );
			}

			// create index buffer
			GpuApi::eBufferChunkCategory category;
#ifdef RED_PLATFORM_ORBIS
			category = isMerged ? GpuApi::BCC_Index16BitUAV : GpuApi::BCC_Index16Bit;
#else
			category = GpuApi::BCC_Index16Bit;
#endif
			GpuApi::eBufferUsageType usage = GpuApi::BUT_ImmutableInPlace;
			bufInitData.m_elementCount = indexDataSize / 2;
			GpuApi::BufferRef ib = GpuApi::CreateBuffer( indexDataSize, category, usage, 0, &bufInitData );
			if ( !ib.isNull() )
			{
#if !defined(RED_FINAL_BUILD) || defined( RED_PROFILE_BUILD ) 
				// name the buffers
				GpuApi::SetBufferDebugPath( ib, UNICODE_TO_ANSI(debugName.AsChar()) );
#endif
				// setup bindings
				m_indices = ib;

				if( isMerged )
				{
#ifdef RED_PLATFORM_ORBIS
					m_cullingBuffers.Resize( m_chunks.Size() );

					// Setup CsVs buffers if it's merged mesh
					SetupCsVsBuffers();
#endif
				}
				
				// make more ready for rendering
				m_buffersPendingLoad.Decrement();
			}
		}
	}

	// The memory regions can now be unlocked so they are moveable
	GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Buffer, m_indexMemRegion );
	GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Buffer, m_vertexMemRegion );

	// We can release the memory region handles now - if we use them they are now owned by the buffer objects
	m_indexMemRegion = nullptr;
	m_vertexMemRegion = nullptr;
}

void CRenderMesh::AllocateInplaceRenderData( Uint32 totalDataSize, Uint32 indexDataSize, Uint32 vertexDataSize, Uint32 indexDataOffset, Uint32 alignment )
{
	// Round up to multiple of 4.
	totalDataSize = ( totalDataSize + 3 ) & (~3);

	// allocate big memory block - it will be split into the vertex/index parts
	// we assume that it's ok to SPLIT memory blocks after allocation (should not pose problems)
	Red::MemoryFramework::MemoryRegionHandle bigMemRegion = GpuApi::AllocateInPlaceMemoryRegion( GpuApi::INPLACE_Buffer, totalDataSize, GpuApi::MC_InPlaceBuffer, alignment ); 

	if ( bigMemRegion.IsValid() )
	{
		// cut off the vertex data
		m_indexMemRegion = GpuApi::SplitInPlaceMemoryRegion( bigMemRegion, indexDataOffset, 16 );		// Index buffers are aligned to 16 bytes internally
		m_vertexMemRegion = bigMemRegion;

#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
		m_indexMemRegion.SetDebugString( (m_depotPath + ":IB").AsChar() );
		m_vertexMemRegion.SetDebugString( (m_depotPath + ":VB").AsChar() );
#endif
	}
	else
	{
		m_indexMemRegion = nullptr;
		m_vertexMemRegion = nullptr;
	}
}

void CRenderMesh::FlushInplaceDataCaches()
{
	if( m_vertexMemRegion.IsValid() )
	{
		GpuApi::FlushCpuCache( m_vertexMemRegion.GetRawPtr(), (Uint32)m_vertexMemRegion.GetSize() );
	}

	if( m_indexMemRegion.IsValid() )
	{
		GpuApi::FlushCpuCache( m_indexMemRegion.GetRawPtr(), (Uint32)m_indexMemRegion.GetSize() );
	}
}

void CRenderMesh::DeallocateInplaceRenderData()
{
	if ( m_vertexMemRegion.IsValid() )
	{
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Buffer, m_vertexMemRegion );
		m_vertexMemRegion = nullptr;
	}

	if ( m_indexMemRegion.IsValid() )
	{
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Buffer, m_indexMemRegion );
		m_indexMemRegion = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Helper
{
	static Bool CreateBuffersFromPackedData( const CMesh* mesh, CRenderMesh* renderMesh, CMeshPackingSource& packer, GpuApi::BufferRef& outVertexBuffer, GpuApi::BufferRef& outIndexBuffer )
	{
		// Base for debug names. Cached so we don't have to convert U->A for every buffer.
		const StringAnsi debugNameBase = UNICODE_TO_ANSI( mesh->GetDepotPath().AsChar() );
		const Bool isMerged = debugNameBase.ContainsSubstring("shadow_mesh");

		// Calculate buffer placement
		// TODO: those alignments are pulled from ass
		Uint32 vertexDataSize = 0, indexDataSize = 0;
		packer.ComputeLayout( 16, 2, vertexDataSize, indexDataSize );
		RED_FATAL_ASSERT( vertexDataSize > 0, "Mesh with no vertex data" );
		RED_FATAL_ASSERT( indexDataSize > 0, "Mesh with no vertex data" );

		// Create vertex buffer
		{
#if 1
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, vertexDataSize );
			packer.CopyVertexData( (void*) bufInitData.m_buffer, vertexDataSize );
			bufInitData.m_elementCount = vertexDataSize / 8;

			GpuApi::eBufferUsageType usage = GpuApi::BUT_Immutable;
			GpuApi::eBufferChunkCategory category = GpuApi::BCC_Vertex;

			outVertexBuffer = GpuApi::CreateBuffer( vertexDataSize, category, usage, 0, &bufInitData );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, (void*)bufInitData.m_buffer );

			if ( outVertexBuffer.isNull() )
				return false;
#else
			GpuApi::BufferInitData bufInitData;
			outVertexBuffer = GpuApi::CreateBuffer( vertexDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, 0, &bufInitData );
			if ( outVertexBuffer.isNull() )
				return false;

			// lock the data
			void* mem = GpuApi::LockBuffer( outVertexBuffer, GpuApi::BLF_NoOverwrite, 0, vertexDataSize );
			if ( !mem )
			{
				GpuApi::SafeRelease( outVertexBuffer );
				return false;
			}

			// copy the data in place
			packer.CopyVertexData( (void*) mem, vertexDataSize );

			// unlock
			GpuApi::UnlockBuffer( outVertexBuffer );
#endif

			// Setup debug name
			const StringAnsi debugNameVB = debugNameBase + StringAnsi::Printf( "[VB]" );
			GpuApi::SetBufferDebugPath(outVertexBuffer, debugNameVB.AsChar());
		}
		
		// Create index buffer
		{
#if 1
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, indexDataSize );
			packer.CopyIndexData( (void*) bufInitData.m_buffer, indexDataSize );
			bufInitData.m_elementCount = indexDataSize / 2;

			GpuApi::eBufferChunkCategory category = GpuApi::BCC_Index16Bit;

			outIndexBuffer = GpuApi::CreateBuffer( indexDataSize, category, GpuApi::BUT_Default, 0, &bufInitData );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, (void*)bufInitData.m_buffer );

			if ( outIndexBuffer.isNull() )
			{
				GpuApi::SafeRelease( outVertexBuffer );
				return false;
			}
#else
			GpuApi::BufferInitData bufInitData;
			outIndexBuffer = GpuApi::CreateBuffer( indexDataSize, GpuApi::BCC_Index16Bit, GpuApi::BUT_Dynamic, 0, &bufInitData );
			if ( outIndexBuffer.isNull() )
			{
				GpuApi::SafeRelease( outVertexBuffer );
				return false;
			}

			// lock the data
			void* mem = GpuApi::LockBuffer( outIndexBuffer, GpuApi::BLF_NoOverwrite, 0, indexDataSize );
			if ( !mem )
			{
				GpuApi::SafeRelease( outIndexBuffer );
				GpuApi::SafeRelease( outVertexBuffer );
				return false;
			}

			// copy the data in place
			packer.CopyIndexData( (void*) mem, indexDataSize );

			// unlock
			GpuApi::UnlockBuffer( outIndexBuffer );
#endif

			// Setup debug name
			const StringAnsi debugNameIB = debugNameBase + StringAnsi::Printf( "[IB]" );
			GpuApi::SetBufferDebugPath(outIndexBuffer, debugNameIB.AsChar());
		}

		// Data copied
		return true;
	}
}

CRenderMesh* CRenderMesh::CreateRaw( const CMesh* mesh, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderMesh > createdRenderMesh ( partialRegistrationHash );

	// Create output mesh
	createdRenderMesh.InitResource( new CRenderMesh() );

	// Pack mesh data
	CMeshPackingSource packer;
	if ( !packer.Pack( mesh ) )
	{
		return nullptr;
	}

	// Empty geometry
	if ( packer.m_vertices.Empty() || packer.m_indices.Empty() )
	{
		return nullptr;
	}
	// Extract quantization params for use during Clusters building
	createdRenderMesh->m_quantizationScale	= mesh->GetBoundingBox().CalcSize();
	createdRenderMesh->m_quantizationOffset	= mesh->GetBoundingBox().Min;

	// Upload packed data into buffers
	GpuApi::BufferRef indexBuffer, vertexBuffer;
	if ( !Helper::CreateBuffersFromPackedData( mesh, createdRenderMesh.GetResource(), packer, vertexBuffer, indexBuffer ) )
	{
		ERR_RENDERER( TXT("Failed to create rendering buffers for mesh '%ls'"), mesh->GetDepotPath().AsChar() );
		return nullptr;
	}

#ifndef NO_RESOURCE_IMPORT
	createdRenderMesh->m_depotPath = UNICODE_TO_ANSI( mesh->GetDepotPath().AsChar() );
	Uint32 lodLevels = mesh->GetNumLODLevels();
	for ( Uint32 i = 0; i< lodLevels; ++i )
	{
		createdRenderMesh->m_dataSize += mesh->EstimateMemoryUsageGPU(i);
	}
#endif
	//
	createdRenderMesh->m_indices = indexBuffer;
	createdRenderMesh->m_vertices = vertexBuffer;
	createdRenderMesh->m_buffersPendingLoad.SetValue(0);

	// Setup LODs
	createdRenderMesh->m_lods.Reserve( packer.m_lods.Size() );
	for ( const CMeshPackingSource::PackedLod& lod : packer.m_lods )
	{
		CRenderMesh::LODLevel renderLod;
		renderLod.m_distance = lod.m_distance;
		createdRenderMesh->m_lods.PushBack( renderLod );
	}

	// Setup chunks
	createdRenderMesh->m_chunks.Reserve( packer.m_chunks.Size() );
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
		createdRenderMesh->m_chunks.PushBack( newChunk );
	}

	// Return created mesh
	return createdRenderMesh.RetrieveSuccessfullyCreated();
}

CRenderMesh* CRenderMesh::Create( const CMesh* mesh, Uint64 partialRegistrationHash )
{
	RED_ASSERT( mesh );
	CRenderMesh* renderMesh = nullptr;

	if ( mesh->IsCooked() )
	{
		// Cooked mesh - create from cooked data
		renderMesh = CreateCooked( mesh, partialRegistrationHash );
	}
	else
	{
		// If there is not cooked data compile mesh from the chunk list
		renderMesh = CreateRaw( mesh, partialRegistrationHash );
	}

	if ( renderMesh )
	{
		renderMesh->m_isTwoSided = mesh->IsTwoSided();
		Uint32 occlusionModelId = GetHash( mesh->GetDepotPath() );
		renderMesh->m_modelId = occlusionModelId;
	}

	return renderMesh;
}

IRenderResource* CRenderInterface::UploadMesh( const CMesh* mesh )
{
	RED_ASSERT( !IsDeviceLost() && "Unable to create new render resources when device is lost" );

	if ( IsDeviceLost() )
	{
		return nullptr;
	}

	CRenderMesh* renderMesh = nullptr;

	if ( CanUseResourceCache() )
	{
		const Uint64 hash = CRenderMesh::CalcResourceHash( mesh );
		if ( CRenderMesh::ResourceCacheRequestPartialCreate( hash, renderMesh ) )
		{
			renderMesh = CRenderMesh::Create( mesh, hash );
		}
	}
	else
	{
		renderMesh = CRenderMesh::Create( mesh, 0 );
	}

	return renderMesh;
}

void CRenderMesh::OnDeviceLost()
{
	GpuApi::SafeRelease( m_indices );
	GpuApi::SafeRelease( m_vertices );
}

void CRenderMesh::OnDeviceReset()
{
	// Nothing, resources NEED to be recreated on engine side
}


// Protect against creating m_bonePositions while simultaneously binding it. Also against potentially trying to create twice.
static Red::Threads::CMutex s_bonePositionsMutex;

void CRenderMesh::InitBonePositionTexture( const CMesh* engineMesh )
{
	if ( m_bonePositions.isNull() )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_bonePositionsMutex );
		// Check again, in case it was set while we were getting the lock.
		if ( !m_bonePositions.isNull() )
		{
			return;
		}

		GpuApi::TextureDesc desc;
		desc.format = GpuApi::TEXFMT_Float_R32G32B32A32;
		desc.height = 1;
		desc.initLevels = 1;
		desc.type = GpuApi::TEXTYPE_2D;
		desc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;

		GpuApi::TextureLevelInitData initMipData;
		initMipData.m_isCooked = false;

		TDynArray< Vector > meshBonePositions;
		if ( engineMesh->GetCookedData().m_bonePositions.Empty() )
		{
			const Uint32 numBones = engineMesh->GetBoneCount();
			engineMesh->GetBonePositions( meshBonePositions );

			desc.width = meshBonePositions.Size();
			initMipData.m_data = meshBonePositions.Data();
		}
		else
		{
			desc.width = engineMesh->GetCookedData().m_bonePositions.Size();
			initMipData.m_data = engineMesh->GetCookedData().m_bonePositions.Data();
		}

		GpuApi::TextureInitData initData;
		initData.m_isCooked = false;
		initData.m_mipsInitData = &initMipData;

		m_bonePositions = GpuApi::CreateTexture( desc, GpuApi::TEXG_Generic, &initData );
	}
}

void CRenderMesh::SetupCsVsBuffers()
{
	{
		struct SIndirectArgs
		{
			Uint32 initArgs[5];
		};
		GpuApi::BufferInitData bufInitDataIndirectArgs;
		Uint32 byteSize = sizeof( SIndirectArgs );
		SIndirectArgs args;
		args.initArgs[0] = 0;
		args.initArgs[1] = 1;
		args.initArgs[2] = 0;
		args.initArgs[3] = 0;
		args.initArgs[4] = 0;
		bufInitDataIndirectArgs.m_elementCount = byteSize / 4;
		bufInitDataIndirectArgs.m_buffer = &args;

		for( Uint32 i = 0; i < m_chunks.Size(); ++ i )
		{
			m_cullingBuffers[i].m_indirectArgs = GpuApi::CreateBuffer( byteSize, GpuApi::BCC_IndirectUAV, GpuApi::BUT_Default, 0, &bufInitDataIndirectArgs );
			RED_FATAL_ASSERT( m_cullingBuffers[i].m_indirectArgs, "Couldn't create indirect args for merged meshes rendering!" );
		}
	}
	{
		GpuApi::BufferInitData bufInitDataIndex;
		for( Uint32 i = 0; i < m_chunks.Size(); ++ i )
		{
			Uint32 byteSize = m_chunks[i].m_numIndices * 2;
			bufInitDataIndex.m_elementCount = m_chunks[i].m_numIndices;

			m_cullingBuffers[i].m_culledIndices = GpuApi::CreateBuffer( byteSize, GpuApi::BCC_Index16BitUAV, GpuApi::BUT_Default, 0, &bufInitDataIndex );
			RED_FATAL_ASSERT( m_cullingBuffers[i].m_culledIndices, "Couldn't create culled indices append buffer for merged meshes rendering!" );
		}
	}
}

namespace Config
{
	TConfigVar< Int32 >	cvMinClusterDistance( "Rendering/MergedShadows", "cvMinClusterDistance", 40, eConsoleVarFlag_Developer );
}

void CRenderMesh::SetupCullingClusterDataVB( const void* vb, const CMeshPackingSource& packer )
{
	m_cullingBuffers.Resize( packer.m_chunks.Size() );

	const Float eps = (Float)Config::cvMinClusterDistance.Get();
	Vector minB(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector maxB(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for(Uint32 i = 0; i < packer.m_chunks.Size(); ++i)
	{
		const CMeshPackingSource::PackedChunk& chunk = packer.m_chunks[i];

		Uint16* vertexData = (Uint16*)((Uint8*)vb + chunk.m_streams.m_vertices[0]->m_placement);
		Vector prevV(vertexData[0], vertexData[1], vertexData[2], 0.0f);
		prevV /= 65535.0f;
		prevV = prevV * m_quantizationScale + m_quantizationOffset;
		minB = maxB = prevV;
		vertexData += 4;

		for( Uint32 j = 1; j < chunk.m_numVertices; ++j )
		{
			Vector v(vertexData[0], vertexData[1], vertexData[2], 0.0f);
			v /= 65535.0f;
			v = v * m_quantizationScale + m_quantizationOffset;

			Float dist = v.DistanceTo(prevV);
			if( dist > eps )
			{
				
				Vector centroid( (minB + maxB)/2.0f );
				centroid.W = (maxB - centroid).Mag3();

				SClusterData cd;
				cd.Centroid = centroid;
				cd.VEnd = j-1;
				m_cullingBuffers[i].m_clusters.PushBack(cd);

				minB = Vector(FLT_MAX, FLT_MAX, FLT_MAX);
				maxB = Vector(-FLT_MAX,-FLT_MAX,-FLT_MAX);
			}

			minB = Vector::Min4( minB, v );
			maxB = Vector::Max4( maxB, v );

			prevV = v;
			vertexData += 4;
		}

		Vector centroid( (minB + maxB)/2.0f );
		centroid.W = (maxB - centroid).Mag3();

		SClusterData cd;
		cd.Centroid = centroid;
		cd.VEnd = chunk.m_numVertices-1;
		m_cullingBuffers[i].m_clusters.PushBack(cd);
	}
}

void CRenderMesh::SetupCullingClusterDataVB( const void* vb )
{
	const Float eps = (Float)Config::cvMinClusterDistance.Get();
	Vector minB(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector maxB(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for(Uint32 i = 0; i < m_chunks.Size(); ++i)
	{
		const CRenderMesh::Chunk& chunk = m_chunks[i];

		Uint16* vertexData = (Uint16*)((Uint8*)vb + chunk.m_chunkVertices.byteOffsets[0]);
		Vector prevV(vertexData[0], vertexData[1], vertexData[2], 0.0f);
		prevV /= 65535.0f;
		prevV = prevV * m_quantizationScale + m_quantizationOffset;
		minB = maxB = prevV;
		vertexData += 4;

		for( Uint32 j = 1; j < chunk.m_numVertices; ++j )
		{
			Vector v(vertexData[0], vertexData[1], vertexData[2], 0.0f);
			v /= 65535.0f;
			v = v * m_quantizationScale + m_quantizationOffset;

			Float dist = v.DistanceTo(prevV);
			if( dist > eps )
			{
				Vector centroid( (minB + maxB)/2.0f );
				centroid.W = (maxB - centroid).Mag3();

				SClusterData cd;
				cd.Centroid = centroid;
				cd.VEnd = j-1;
				m_cullingBuffers[i].m_clusters.PushBack(cd);

				minB = Vector(FLT_MAX, FLT_MAX, FLT_MAX);
				maxB = Vector(-FLT_MAX,-FLT_MAX,-FLT_MAX);
			}

			minB = Vector::Min4( minB, v );
			maxB = Vector::Max4( maxB, v );

			prevV = v;
			vertexData += 4;
		}

		Vector centroid( (minB + maxB)/2.0f );
		centroid.W = (maxB - centroid).Mag3();

		SClusterData cd;
		cd.Centroid = centroid;
		cd.VEnd = chunk.m_numVertices-1;
		m_cullingBuffers[i].m_clusters.PushBack(cd);
	}
}

void CRenderMesh::SetupCullingClusterDataIB( const void* ib, const CMeshPackingSource& packer )
{
	for(Uint32 i = 0; i < packer.m_chunks.Size(); ++i)
	{
		const CMeshPackingSource::PackedChunk& chunk = packer.m_chunks[i];
		CRenderMesh::SCullingBuffers& cullingBuffer = m_cullingBuffers[i];

		Uint32 startI = 0;
		Uint32 chunkCluster = 0;
		Uint16* vertexData = (Uint16*)((Uint8*)ib + chunk.m_streams.m_indices->m_placement);
		for(Uint32 j = 0; j < packer.m_chunks[i].m_numIndices; ++j)
		{
			Uint16 ind = *vertexData;
			if( ind == cullingBuffer.m_clusters[chunkCluster].VEnd )
			{
				cullingBuffer.m_clusters[chunkCluster].IStart = startI;
				if( chunkCluster == cullingBuffer.m_clusters.Size()-1 )
				{
					cullingBuffer.m_clusters[chunkCluster].IEnd = chunk.m_numIndices-1;
					break;
				}
				else
				{
					cullingBuffer.m_clusters[chunkCluster].IEnd = j - j % 3 - 1;
				}

				startI = cullingBuffer.m_clusters[chunkCluster].IEnd+1;

				chunkCluster++;
			}

			vertexData++;
		}
	}
}

void CRenderMesh::SetupCullingClusterDataIB( const void* ib )
{
	for(Uint32 i = 0; i < m_chunks.Size(); ++i)
	{
		const CRenderMesh::Chunk& chunk = m_chunks[i];
		CRenderMesh::SCullingBuffers& cullingBuffer = m_cullingBuffers[i];

		Uint32 startI = 0;
		Uint32 chunkCluster = 0;
		Uint16* vertexData = (Uint16*)((Uint8*)ib + chunk.m_chunkIndices.byteOffset);
		for(Uint32 j = 0; j < chunk.m_numIndices; ++j)
		{
			Uint16 ind = *vertexData;
			if( ind == cullingBuffer.m_clusters[chunkCluster].VEnd )
			{
				cullingBuffer.m_clusters[chunkCluster].IStart = startI;
				if( chunkCluster == cullingBuffer.m_clusters.Size()-1 )
				{
					cullingBuffer.m_clusters[chunkCluster].IEnd = chunk.m_numIndices-1;
					break;
				}
				else
				{
					cullingBuffer.m_clusters[chunkCluster].IEnd = j - j % 3 - 1;
				}

				startI = cullingBuffer.m_clusters[chunkCluster].IEnd+1;

				chunkCluster++;
			}

			vertexData++;
		}
	}
}

Bool CRenderMesh::FinishClusterBuildingTaskOnDestruction()
{
	if( m_clusterBuilderTask != nullptr )
	{
		if ( m_clusterBuilderTask->MarkRunning() )
		{
			m_clusterBuilderTask->MarkFinished();
		}

		while ( !m_clusterBuilderTask->IsFinished() ){}

		m_clusterBuilderTask->Release();
		m_clusterBuilderTask = nullptr;
	}

	return true;
}

Bool CRenderMesh::FinishClusterBuildingTaskOnAccess()
{
	if( m_clusterBuilderTask != nullptr )
	{
		if( !m_clusterBuilderTask->IsFinished() )
		{
			return false;
		}

		m_clusterBuilderTask->Release();
		m_clusterBuilderTask = nullptr;
	}

	return true;
}

#ifndef RED_FINAL_BUILD
GeneralStats CRenderInterface::GetGeneralMeshStats( GeneralStats& st )
{
	for ( TRenderResourceIterator< CRenderMesh > it; it; ++it )
	{
		CRenderMesh* currentMesh = *it;
		if (currentMesh->GetMeshDepotPath().ContainsSubstring(TXT("characters")) || currentMesh->GetMeshDepotPath().ContainsSubstring(TXT("items")) )
		{
			st.m_characterMemory += currentMesh->GetMeshSize();
		}
		else if (currentMesh->GetMeshDepotPath().ContainsSubstring(TXT("environment")))
		{
			st.m_environmentMemory += currentMesh->GetMeshSize();
		}
	}
	return st;
}
#endif

// ------------------- BUILD CLUSTERS TASK -----------------------------

CBuildClustersTask::CBuildClustersTask(const void* vb, const void* ib, CRenderMesh* renderMesh)
	: m_vb( vb )
	, m_ib( ib )
	, m_renderMesh( renderMesh )
{
}

void CBuildClustersTask::Run()
{
	m_renderMesh->SetupCullingClusterDataVB( m_vb );
	m_renderMesh->SetupCullingClusterDataIB( m_ib );
}

