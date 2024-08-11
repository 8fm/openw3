/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "mesh.h"

/// Mesh data helper - allows you to access and modify the content of the mesh easily
/// Basically when instanced on mesh it
class CMeshData
{
public:
	CMeshData( const CMesh* mesh );
	~CMeshData();

	/// Triangle filter interface, note: it's offline mostly, we will survive virtual function here
	class IMergeGeometryModifier
	{
	public:
		virtual ~IMergeGeometryModifier() {};

		/// Process triangle geometry, returns true if we should we merge given triangle or false if we should discard it
		/// It's legal to change triangle contents
		virtual Bool ProcessTriangle( const Matrix& referenceMatrix, SMeshVertex& a, SMeshVertex& b, SMeshVertex& c ) const = 0; 
	};

#ifndef NO_RESOURCE_IMPORT

	// Clear mesh data - remove all of of the chunks and reset LODs
	void Clear();

	// Add LOD information
	Bool AddLOD( Float distance, Int32 insertAtIndex );	// Return false on failure

	// Add new chunk data
	SMeshChunk* AddChunkToLOD( Uint32 lodIndex, Uint32 materialId, EMeshVertexType vertexType, Uint32& newChunkIndex ); // Returns nullptr on failure

	// Remove LOD 
	void RemoveLOD( const Uint32 lodIndex );

	// Remove chunk
	void RemoveChunk( const Uint32 chunkIndex );

	// Find LOD index for given chunk index (-1 if out of range)
	Int32 FindLODForChunk( Int32 index ) const;

	// Merge specified chunks
	Int32 MergeChunks( const TDynArray< Uint32 >& chunkIndices, Uint32 materialIndex, String* outErrorStr = nullptr );

	// Merge all chunks in LOD - Return number of chunks merged for the LOD
	Int32 MergeChunks( Uint32 lodIndex, Bool countOnly = false, String* outErrorStr = nullptr );

	// Merge all chunks in mesh - Returns number of chunks that were (or could be) removed.
	Int32 MergeChunks( Bool countOnly = false, String* outErrorStr = nullptr );

	// Compare if materials with given indices are equal (in source mesh)
	Bool CompareMaterials( Uint32 matIdx1, Uint32 matIdx2 ) const;

	// store data back into mesh, will checkout the mesh and regenerate the data
	Bool FlushChanges( const Bool optimizeMesh = true ) const;

	// generate LODs with Simpligon
	Bool GenerateLODWithSimplygon( Int32 lodIndex, const SLODPresetDefinition& lodDefinition, Uint32& numOfRemovedChunks, String& message, Bool showProgress );

	// add content of another mesh, will try to reuse existing materials
	Bool AppendChunk( const SMeshChunk& otherChunk, const Uint32 lodIndex, const Matrix& referenceMatrix, const IMergeGeometryModifier* geometryModifier, const Int32 renderMaskOverride = -1, const Int32 materialIndexOverride = -1, const Int32 vertexTypeOverride = -1 );

#endif

	// get raw mesh data
	RED_INLINE const TDynArray< SMeshChunk >& GetChunks() const { return *m_chunks; }
	RED_INLINE TDynArray< SMeshChunk >& GetChunks() { return *m_chunks; }

	RED_INLINE CMesh::TLODLevelArray& GetLODs() { return m_lodLevelInfo; }
	RED_INLINE const CMesh::TLODLevelArray& GetLODs() const { return m_lodLevelInfo; }

	// compute stats
	const Uint32 GetNumTriangles() const;
	const Uint32 GetNumVertices() const;

private:
	const CMesh*					m_mesh;

	// extracted chunks
	MeshUnpackedDataHandle			m_chunks;

	// extracted LODs
	CMesh::TLODLevelArray	m_lodLevelInfo;
};
