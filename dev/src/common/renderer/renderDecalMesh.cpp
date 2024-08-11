/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderDecalMesh.h"
#include "renderSkinningData.h"
#include "renderDynamicDecal.h"
#include "renderInterface.h"
#include "renderShaderPair.h"
#include "../engine/meshEnum.h"


// Generate a new vertex layout for a given chunk, with the decal's texcoords added in.
static GpuApi::VertexLayoutRef CreateModifiedVertexLayout( GpuApi::VertexLayoutRef sourceLayout )
{
	const GpuApi::VertexLayoutDesc* originalLayoutDesc = GpuApi::GetVertexLayoutDesc( sourceLayout );
	RED_ASSERT( originalLayoutDesc );

	// Mostly use the original vertex layout.
	GpuApi::VertexLayoutDesc modifiedLayoutDesc;
	for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
	{
		if ( originalLayoutDesc->m_elements[i].IsEmpty() )
		{
			break;
		}

		modifiedLayoutDesc.m_elements[i] = originalLayoutDesc->m_elements[i];

		// Replace texcoord0 with texcoord4.
		if ( originalLayoutDesc->m_elements[i].m_usage == GpuApi::VertexPacking::PS_TexCoord && originalLayoutDesc->m_elements[i].m_usageIndex == 0 )
		{
			modifiedLayoutDesc.m_elements[i].m_usageIndex = 4;
			modifiedLayoutDesc.m_elements[i].m_type = GpuApi::VertexPacking::PT_Float16_4;
		}
	}

	modifiedLayoutDesc.UpdateStrides();

	return GpuApi::CreateVertexLayout( modifiedLayoutDesc );
}


//////////////////////////////////////////////////////////////////////////


CRenderDecalMesh::CRenderDecalMesh()
	: m_sourceMesh( nullptr )
{
	m_buffersPendingLoad.SetValue( 0 );
}

CRenderDecalMesh::~CRenderDecalMesh()
{
	SAFE_RELEASE( m_sourceMesh );
	for ( auto& layout : m_chunkVertexLayouts )
	{
		GpuApi::SafeRelease( layout );
	}
}


CRenderDecalMesh* CRenderDecalMesh::Create( const CRenderDynamicDecal* decal, const CRenderElement_MeshChunk* const* meshChunks, Uint8 numChunks, const Matrix& meshLocalToWorld, CRenderSkinningData* skinningData, Uint64 partialRegistrationHash )
{
	// TODO : Could we issue a GS-SO query, and when it's ready remake the buffer with only the triangles that have something useful?
	// Could even get crazy and generate new index buffer, instead of full triangle list...


	PC_SCOPE_PIX( CRenderDecalMesh_Create );

	RED_FATAL_ASSERT( numChunks == 0 || meshChunks != nullptr, "meshChunks can't be null if numChunks != 0" );

	TScopedRenderResourceCreationObject< CRenderDecalMesh > newChunkMesh ( partialRegistrationHash );

	if ( numChunks == 0 )
	{
		return nullptr;
	}

#ifndef RED_FINAL_BUILD
	for ( Uint8 i = 0; i < numChunks; ++i )
	{
		RED_FATAL_ASSERT( meshChunks[ i ] != nullptr, "Null mesh chunk element" );
	}
#endif

	// We assume here that we have a single CRenderMesh for all chunk elements. If this weren't the case, then we'd need to
	// have per-chunk quantization settings and index buffers, or something. With our current rendering, this should always
	// be the case though.
	CRenderMesh* sourceMesh = meshChunks[ 0 ]->GetMesh();
	if ( sourceMesh == nullptr )
	{
		RED_HALT( "Mesh chunk element has null render mesh" );
		return nullptr;
	}


	// Do not create decals on meshes that are not fully loaded
	if ( !sourceMesh->IsFullyLoaded() )
	{
		return nullptr;
	}
	
	newChunkMesh.InitResource( new CRenderDecalMesh() );
	
	SAFE_COPY( newChunkMesh->m_sourceMesh, sourceMesh );

	newChunkMesh->m_quantizationScale		= sourceMesh->m_quantizationScale;
	newChunkMesh->m_quantizationOffset		= sourceMesh->m_quantizationOffset;
	GpuApi::SafeRefCountAssign( newChunkMesh->m_indices, sourceMesh->m_indices );

	// Just have one LOD.
	{
		LODLevel singleLod;
		singleLod.m_distance = 0.0f;
		newChunkMesh->m_lods.PushBack( singleLod );
	}


	// Output from decal generation is the same for any chunk.
	const Uint32 decalGenOutStride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexDynamicDecalGenOut );


	TDynArray< Uint8 > chunksInSourceMesh;
	chunksInSourceMesh.Reserve( numChunks );

	for ( Uint32 i = 0; i < numChunks; ++i )
	{
		RED_ASSERT( meshChunks[ i ]->GetMesh() == sourceMesh, TXT("Mesh proxy with more than one render mesh found!") );

		// Don't include any morph target chunks. Since the decal doesn't have to worry about different materials, we can just draw
		// for the source (or blended) chunks.
		if ( meshChunks[ i ]->HasFlag( RMCF_MorphTarget ) )
		{
			continue;
		}

		const Uint8 chunkIndex = meshChunks[ i ]->GetChunkIndex();
		RED_ASSERT( !chunksInSourceMesh.Exist( chunkIndex ), TXT("Two mesh chunk elements have the same chunk index") );
		chunksInSourceMesh.PushBack( chunkIndex );
	}


	const Uint32 texStreamIndex = RenderMeshStreamToIndex( RMS_TexCoords );

	// We're going to use a single vertex buffer for all generated vertex data. Figure out how big it needs to be.
	// Also set up the chunks in the created decal mesh.
	Uint32 vertexBufferSize = 0;
	const auto& origMeshChunks = sourceMesh->GetChunks();
	newChunkMesh->m_chunks.Reserve( chunksInSourceMesh.Size() );
	newChunkMesh->m_sourceMeshChunkMapping.Reserve( chunksInSourceMesh.Size() );

	for ( Uint32 i = 0; i < chunksInSourceMesh.Size(); ++i )
	{
		const Uint8 sourceChunkIndex = chunksInSourceMesh[ i ];
		if ( sourceChunkIndex >= origMeshChunks.Size() )
		{
			RED_HALT( "Chunk index is out of bounds! %u >= %u", sourceChunkIndex, origMeshChunks.Size() );
			continue;
		}

		const CRenderMesh::Chunk& sourceChunk = origMeshChunks[ sourceChunkIndex ];

		// Only for normal scene-rendered chunks.
		if ( ( sourceChunk.m_baseRenderMask & MCR_Scene ) == 0 )
		{
			continue;
		}


		// Create vertex layout with the proper texcoord info.
		GpuApi::VertexLayoutRef newLayout = CreateModifiedVertexLayout( sourceMesh->GetChunkVertexLayout( sourceChunkIndex ) );
		if ( !newLayout )
		{
			RED_HALT( "Failed to create vertex layout for decal chunk" );
			continue;
		}

		// Chunk in the new mesh is based on the original chunk, but with appropriate texcoord offset and layout and whatever.
		CRenderMesh::Chunk newChunk = sourceChunk;
		newChunk.m_lodMask = 1;
		newChunk.m_chunkVertices.byteOffsets[ texStreamIndex ] = vertexBufferSize;


		newChunkMesh->m_chunkVertexLayouts.PushBack( newLayout );
		newChunkMesh->m_chunks.PushBack( newChunk );
		newChunkMesh->m_sourceMeshChunkMapping.PushBack( sourceChunkIndex );


		vertexBufferSize += newChunk.m_numVertices * decalGenOutStride;
	}

	if ( vertexBufferSize == 0 )
	{
		WARN_RENDERER( TXT("No vertices in decal mesh?") );
		return nullptr;
	}


	newChunkMesh->m_vertices = GpuApi::CreateBuffer( vertexBufferSize, GpuApi::BCC_Vertex, GpuApi::BUT_StreamOut, 0 );

	Matrix worldToDecal;
	if ( decal->ApplyInLocalSpace() )
	{
		GetRenderer()->m_shaderDynamicDecalGen->Bind();
		GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

		worldToDecal = decal->GetParentToDecal();
	}
	else
	{
		if ( skinningData != nullptr )
		{
			GetRenderer()->m_shaderDynamicDecalSkinnedGen->Bind();
			skinningData->Bind();
		}
		else
		{
			GetRenderer()->m_shaderDynamicDecalGen->Bind();
		}
		GetRenderer()->GetStateManager().SetLocalToWorld( &meshLocalToWorld );

		worldToDecal = decal->GetWorldToDecalParent() * decal->GetParentToDecal();
	}
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );


	decal->BindGenerateParameters( worldToDecal );

	{
		PC_SCOPE_RENDER_LVL1( CRenderDecalMesh_Create_Generate );

		// For each chunk, generate new vertex data.
		const Uint32 numNewChunks = newChunkMesh->m_chunks.Size();
		for ( Uint32 i = 0; i < numNewChunks; ++i )
		{
			const Uint8 sourceChunkIndex = newChunkMesh->m_sourceMeshChunkMapping[ i ];
			const CRenderMesh::Chunk& sourceChunk = origMeshChunks[ sourceChunkIndex ];
			const CRenderMesh::Chunk& newChunk = newChunkMesh->m_chunks[ i ];

			// newChunk was originally a copy of sourceChunk. How could they be different?
			RED_FATAL_ASSERT( sourceChunk.m_numVertices == newChunk.m_numVertices, "# Vertices doesn't match?" );

			sourceMesh->Bind( sourceChunkIndex, RMS_PositionSkinning | RMS_TangentFrame );

			GpuApi::BindStreamOutBuffers( 1, &newChunkMesh->m_vertices, &newChunk.m_chunkVertices.byteOffsets[ texStreamIndex ] );
			GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_PointList, 0, sourceChunk.m_numVertices );
		}
	}

	GpuApi::BindStreamOutBuffers( 0, nullptr, nullptr );
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );

	return newChunkMesh.RetrieveSuccessfullyCreated();
}


void CRenderDecalMesh::GetChunkGeometry( Uint8 chunkIndex, ChunkGeometry& outGeometry ) const
{
	RED_FATAL_ASSERT( chunkIndex < m_chunks.Size(), "Chunk index out of bounds! Caller is responsible for this!" );

	// For the most part, we just use the source mesh directly.
	m_sourceMesh->GetChunkGeometry( m_sourceMeshChunkMapping[ chunkIndex ], outGeometry );

	// Just need to swap in our own texcoords stream (buffer and offset)
	const Uint32 texStreamIndex = RenderMeshStreamToIndex( RMS_TexCoords );
	outGeometry.m_vertexBuffers[ texStreamIndex ] = m_vertices;
	outGeometry.m_vertexOffsets[ texStreamIndex ] = m_chunks[ chunkIndex ].m_chunkVertices.byteOffsets[ texStreamIndex ];

	// And use proper layout for the decal geometry
	outGeometry.m_vertexLayout = m_chunkVertexLayouts[ chunkIndex ];
}

GpuApi::VertexLayoutRef CRenderDecalMesh::GetChunkVertexLayout( Uint8 chunkIndex ) const
{
	if ( chunkIndex >= m_chunks.Size() )
	{
		RED_HALT( "Mesh chunk index out of range. %d >= %d", chunkIndex, m_chunks.Size() );
		return GpuApi::VertexLayoutRef::Null();
	}

	return m_chunkVertexLayouts[ chunkIndex ];
}
