/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderDebugMesh.h"
#include "renderShaderPair.h"

CRenderDebugMesh::CRenderDebugMesh( const GpuApi::BufferRef &vertices, const GpuApi::BufferRef &indices, Uint32 numVertices, Uint32 numIndices )
	: m_vertices( vertices )
	, m_indices( indices )
	, m_numVertices( numVertices )
	, m_numIndices( numIndices )
{
}

CRenderDebugMesh::~CRenderDebugMesh()
{
	// Dispose vertices
	GpuApi::SafeRelease( m_vertices );
	
	// Dispose indices
	GpuApi::SafeRelease( m_indices );	
}

void CRenderDebugMesh::Draw( Bool drawAsWireframe, Bool drawSingleColor )
{
	if ( !m_indices || !m_vertices )
	{
		return;
	}

	// Bind shader
	if ( drawSingleColor )
	{
		GetRenderer()->GetDebugDrawer().GetShaderSingleColor()->Bind();
	}
	else
	{
		GetRenderer()->GetDebugDrawer().GetShaderPlain()->Bind();
	}

	// Bind buffers
	Uint32 vbSstride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPosColor );
	Uint32 vbOffset = 0;
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor, true );
	GpuApi::BindVertexBuffers(0, 1, &m_vertices, &vbSstride, &vbOffset);

	GpuApi::BindIndexBuffer( m_indices );
	
	// Draw chunk
	if ( drawAsWireframe )
	{
		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_LineList, 0, 0, m_numVertices, 0, m_numIndices / 2 );
	}
	else
	{
		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_numVertices, 0, m_numIndices / 3 );
	}
}

CName CRenderDebugMesh::GetCategory() const
{
	return CNAME( RenderDebugMesh );
}

Uint32 CRenderDebugMesh::GetUsedVideoMemory() const
{
	Uint32 size = sizeof( *this );
	size += m_indices  ? GpuApi::GetBufferDesc(m_indices).CalcUsedVideoMemory()  : 0;
	size += m_vertices ? GpuApi::GetBufferDesc(m_vertices).CalcUsedVideoMemory() : 0;
	return size;
}

CRenderDebugMesh* CRenderDebugMesh::Create( const TDynArray< DebugVertex >& vertices, const TDynArray< Uint32 >& indices )
{
	// No vertices or indices, no mesh to create	
	if ( vertices.Empty() || indices.Empty() )
	{
		return NULL;
	}

	// Create vertex buffer
	GpuApi::BufferInitData bufInitData;

	bufInitData.m_buffer = vertices.TypedData();
	GpuApi::BufferRef vertexBuffer = GpuApi::CreateBuffer( sizeof(DebugVertex) * vertices.Size(), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
	if ( !vertexBuffer )
	{
		return NULL;
	}
	GpuApi::SetBufferDebugPath( vertexBuffer, "debugMeshVB" );

	// Create index buffer
	bufInitData.m_buffer = indices.TypedData();
	GpuApi::BufferRef indexBuffer = GpuApi::CreateBuffer( indices.Size() * 4, GpuApi::BCC_Index32Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
	if ( !indexBuffer )
	{
		GpuApi::SafeRelease( vertexBuffer );
		return NULL;
	}
	GpuApi::SetBufferDebugPath( indexBuffer, "debugMeshIB" );

	// Create debug mesh
	return new CRenderDebugMesh( vertexBuffer, indexBuffer, vertices.Size(), indices.Size() );
}

void CRenderDebugMesh::OnDeviceLost()
{
#ifdef W2_PLATFORM_WIN32
	// Dispose vertices
	GpuApi::SafeRelease( m_vertices );

	// Dispose indices
	GpuApi::SafeRelease( m_indices );	
#endif
}

void CRenderDebugMesh::OnDeviceReset()
{
	//Nothing, resources NEED to be recreated on engine side
}

const GpuApi::MeshStats* CRenderInterface::GetMeshStats()
{
	return GpuApi::GetMeshStats();
}

const GpuApi::TextureStats* CRenderInterface::GetTextureStats()
{
	return GpuApi::GetTextureStats();
}

IRenderResource* CRenderInterface::UploadDebugMesh( const TDynArray< DebugVertex >& vertices, const TDynArray< Uint32 >& indices )
{
	ASSERT( !IsDeviceLost() && "Unable to create new render resources when device is lost" );

	if ( IsDeviceLost() )
	{
		return NULL;
	}

	return CRenderDebugMesh::Create( vertices, indices );
}
