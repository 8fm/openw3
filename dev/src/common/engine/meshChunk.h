/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_MESH_CHUNK_H_
#define _ENGINE_MESH_CHUNK_H_

#include "meshVertex.h"
#include "meshEnum.h"

//---------------------------------------------------------------

/// Source mesh chunk - used to initialize mesh
struct SMeshChunk
{
	EMeshVertexType							m_vertexType;			// Mesh vertex type
	Uint32									m_materialID;			// Material index
	Uint8									m_numBonesPerVertex;	// Number of bones used per vertex
	Uint32									m_numVertices;			// Number of vertices
	Uint32									m_numIndices;			// Number of indices
	TDynArray< SMeshVertex, MC_BufferMesh >	m_vertices;				// Vertices
	TDynArray< Uint16, MC_BufferMesh >		m_indices;				// Indices
	Uint8									m_renderMask;			// Where should we render this chunk ?

	SMeshChunk()
		: m_vertexType( MVT_StaticMesh )
		, m_materialID( 0 )
		, m_numBonesPerVertex( 0 )
		, m_numVertices( 0 )
		, m_numIndices( 0 )
		, m_renderMask( MCR_Scene | MCR_Cascade1 | MCR_Cascade2 | MCR_LocalShadows )
	{
	}
};

extern IFile& operator<<( class IFile& file, SMeshChunk& c );

//---------------------------------------------------------------

/// Light weight mesh chunk - does not hold the heavy data
struct SMeshChunkPacked
{
	DECLARE_RTTI_STRUCT( SMeshChunkPacked );

public:
	EMeshVertexType							m_vertexType;			// Mesh vertex type
	Uint32									m_materialID;			// Material index
	Uint8									m_numBonesPerVertex;	// Number of bones used per vertex
	Uint32									m_numVertices;			// Number of vertices
	Uint32									m_numIndices;			// Number of indices
	Uint32									m_firstVertex;			// First vertex in the chunk array
	Uint32									m_firstIndex;			// First vertex in the chunk array
	Uint8									m_renderMask;			// Where should we render this chunk ?
	Bool									m_useForShadowmesh;		// Use this chunk for shadowmesh

	SMeshChunkPacked()
		: m_vertexType( MVT_StaticMesh )
		, m_materialID( 0 )
		, m_numBonesPerVertex( 0 )
		, m_numVertices( 0 )
		, m_numIndices( 0 )
		, m_renderMask( MCR_Scene | MCR_Cascade1 | MCR_Cascade2 | MCR_LocalShadows )
		, m_useForShadowmesh( false )
	{
	}
};

BEGIN_CLASS_RTTI( SMeshChunkPacked );
	PROPERTY( m_vertexType );
	PROPERTY( m_materialID );
	PROPERTY( m_numBonesPerVertex );
	PROPERTY( m_numVertices );
	PROPERTY( m_numIndices );
	PROPERTY( m_firstVertex );
	PROPERTY( m_firstIndex );
	PROPERTY_BITFIELD( m_renderMask, EMeshChunkRenderMask );
	PROPERTY( m_useForShadowmesh );
END_CLASS_RTTI();

//---------------------------------------------------------------



#endif
