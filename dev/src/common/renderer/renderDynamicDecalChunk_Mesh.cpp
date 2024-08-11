/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderDynamicDecalChunk_Mesh.h"
#include "renderDynamicDecal.h"
#include "renderDecalMesh.h"
#include "renderProxyMesh.h"


CRenderDynamicDecalChunk_Mesh::CRenderDynamicDecalChunk_Mesh( CRenderDynamicDecal* ownerDecal, CRenderProxy_Mesh* target, CRenderDecalMesh* decalMesh, Uint8 chunkIndex )
	: CRenderDynamicDecalChunk( ownerDecal, target )
	, m_mesh( decalMesh )
	, m_chunkIndex( chunkIndex )
{
	RED_ASSERT( m_mesh != nullptr );
	m_mesh->AddRef();
}

CRenderDynamicDecalChunk_Mesh::~CRenderDynamicDecalChunk_Mesh()
{
	SAFE_RELEASE( m_mesh );
}


void CRenderDynamicDecalChunk_Mesh::Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo )
{
	PC_SCOPE_RENDER_LVL1(RenderDynamicDecalChunk_Mesh);

	if ( m_ownerDecal == nullptr || m_targetProxy == nullptr )
	{
		return;
	}

	GetRenderer()->GetStateManager().SetLocalToWorld( &GetLocalToWorld() );

	m_mesh->Bind( m_chunkIndex, RMS_PositionSkinning | RMS_TexCoords | RMS_TangentFrame );

	MeshDrawingStats dummyStats;
	m_mesh->DrawChunkNoBind( m_chunkIndex, 1, dummyStats );
}


CRenderSkinningData* CRenderDynamicDecalChunk_Mesh::GetSkinningData() const
{
	return static_cast< CRenderProxy_Mesh* >( m_targetProxy )->GetSkinningData();
}

GpuApi::VertexLayoutRef CRenderDynamicDecalChunk_Mesh::GetVertexLayout() const
{
	return m_mesh->GetChunkVertexLayout( m_chunkIndex );
}


Uint32 CRenderDynamicDecalChunk_Mesh::GetUsedVideoMemory() const
{
	// Just find out how much space our chunk's texcoords take...
	const CRenderMesh::Chunk& chunk = m_mesh->GetChunks()[m_chunkIndex];
	GpuApi::VertexLayoutRef layout = m_mesh->GetChunkVertexLayout( m_chunkIndex );
	return chunk.m_numVertices * GpuApi::GetVertexLayoutStride( layout, RenderMeshStreamToIndex( RMS_TexCoords ) );
}
