/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "globalVisibilityId.h"
#include "meshDataBuilder.h"

//------------------------------------------------------------

#ifndef NO_RESOURCE_IMPORT

/// Merged mesh source data
class MergedMeshSourceData
{
public:
	MergedMeshSourceData( const Uint32 renderMask );

	// Get number of object in set
	RED_INLINE const Uint32 GetNumObjects() const { return m_objects.Size(); }

	// Get number of triangles in set
	RED_INLINE const Uint32 GetNumTriangles() const { return m_numTriangles; }

	// Get number of vertices in set
	RED_INLINE const Uint32 GetNumVertices() const { return m_numVertices; }

	// Get world space bounding box
	RED_INLINE const Box& GetWorldBox() const { return m_worldBox; }

	// Extract stuff, NOTE: entity must be streamed in
	void AddComponent( const CMeshComponent* component, const TDynArray< Uint32 >& chunkIndices );

	// Insert data into mesh data
	void InsertObjects( CMeshData& outMeshData, const CMeshData::IMergeGeometryModifier* geometryModifier, const EMeshVertexType vertexType, TDynArray<String>& outCorruptedMeshPaths ) const;

private:
	struct ObjectInfo
	{
		GlobalVisID				m_id;					//!< Assigned object ID
		const CMesh*			m_mesh;					//!< Source mesh
		Matrix					m_localToWorld;			//!< Object's location
		TDynArray< Uint32 >		m_chunksToUse;			//!< Chunk selection
		Uint8					m_renderMask;			//!< Render chunk selection
	};

	Uint8						m_mergeMask;

	TDynArray< ObjectInfo >		m_objects;
	Uint32						m_numVertices;
	Uint32						m_numTriangles;

	Box							m_worldBox;

	friend class MergedMeshBuilder;
};

/// Mesh builder
class MergedMeshBuilder
{
public:
	MergedMeshBuilder( THandle< CMesh > mesh, EMeshVertexType vertexType );

	/// Flush changes, returns true if we have any data
	Bool Flush();

	/// Build mesh from source data with optional root offset, mesh should exist, it will be updated
	void AddData( const MergedMeshSourceData& source, const CMeshData::IMergeGeometryModifier* geometryModifier, TDynArray<String> &outCorruptedMeshDepotPaths, Vector& outOrigin );

	/// Set render mask for chunks in the data
	void SetRenderMaskForAllChunks( const Uint8 renderMask );

private:
	THandle< CMesh >				m_mesh;
	EMeshVertexType					m_vertexType;
	CMeshData						m_data;
};

#endif