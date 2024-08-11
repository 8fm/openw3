/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/pagedMemoryWriter.h"

/// This file contains code to pack the engine mesh CMesh into the buffers suitable for rendering
/// It handles both the runtime (in editor) and cooking cases using the same code

typedef const GpuApi::VertexPacking::PackingElement* GpuPacking;
typedef GpuApi::eBufferChunkType GpuChunkType;

/// Chunk lister
class CMeshPackingSource
{
public:
	static const Uint32 NUM_STREAMS = 5; // RENDER_MESH_NUM_STREAMS, keep up to date with ERenderMeshStreams

	struct PackedLod
	{
		Float							m_distance;
	};

	struct PackedBuffer
	{
		CPagedMemoryBuffer*		m_data;			// raw data
		Uint32					m_placement;	// placement offset
		Uint64					m_hash;

		PackedBuffer()
			: m_placement(0)
			, m_hash(0)
			, m_data(nullptr)
		{}

		~PackedBuffer()
		{
			delete m_data;
		}
	};

	struct PackedStreams
	{
		PackedBuffer*			m_vertices[ NUM_STREAMS ];
		PackedBuffer*			m_indices;

		PackedStreams(); // zero
		~PackedStreams(); // no free
	};

	struct SourceChunk
	{
		Uint32					m_chunkIndex;
		Uint32					m_baseVertexIndex;
	};

	struct PackedChunk
	{
		TDynArray< SourceChunk >	m_sourceChunks;
		Uint32						m_materialId;

		Uint8						m_vertexFactory;
		GpuChunkType				m_vertexType;
		GpuPacking					m_vertexPacking;

		Uint32						m_numVertices;
		Uint32						m_numIndices;

		Uint8						m_baseRenderMask;				//!< Render mask of this chunk - telling us where should we render this chunk (scene, cascades, etc)
		Uint8						m_mergedRenderMask;				//!< In case this is part of a merged content here we have the merged render mask
		Uint8						m_lodMask;
		PackedStreams				m_streams;

		PackedChunk()
			: m_numVertices( 0 )
			, m_numIndices( 0 )
			, m_baseRenderMask( 0 )
			, m_mergedRenderMask( 0 )
			, m_lodMask( 1 )
			, m_materialId( 0 )
			, m_vertexPacking( nullptr )
			, m_vertexFactory( 0 )
		{}
	};

	TDynArray< PackedBuffer* >	m_vertices; // all packed vertices
	TDynArray< PackedBuffer* >	m_indices; // all packed indices

	TDynArray< PackedChunk >	m_chunks; // final packed chunks (may be aliased)
	TDynArray< PackedLod >		m_lods; // final packed LODs

	CMeshPackingSource();
	~CMeshPackingSource();
	
	// pack mesh 
	Bool Pack( const CMesh* mesh );

	// compute final buffer layout
	void ComputeLayout( const Uint32 vertexDataAlignment, const Uint32 indexDataAlignment, Uint32& outVertexDataSize, Uint32& outIndexDataSize );

	// copy ALL vertex data into specified memory, requires layout to be computed first
	void CopyVertexData( void* outputMemory, const Uint32 bufferSize ) const;

	// copy ALL index data into specified memory, requires layout to be computed first
	void CopyIndexData( void* outputMemory, const Uint32 bufferSize ) const;

private:
	// Create vertex buffer wrapper from given data
	PackedBuffer* CreateVertexBuffer( CPagedMemoryBuffer*& memory );

	// Create inex buffer wrapper from given data
	PackedBuffer* CreateIndexBuffer( CPagedMemoryBuffer*& memory );
};
