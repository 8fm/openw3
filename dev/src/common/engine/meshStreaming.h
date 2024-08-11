/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifdef USE_RED_RESOURCEMANAGER

// CH: This was in mesh during the integration from streaming to Main.Lava - didn't seem to need to be so I moved it here.
// Not sure that this stuff is useful, but I want to pull it up, so I can strip it back correctly later.
#define MAX_STREAMED_MESH_VERTEX_STREAMS 5
// Mesh chunk data wrapper
// Used to stream particular mesh parts without requiring the entire thing to be loaded
// These are never kept around in memory very long and are purposely non-const so the internal arrays
// can be *moved* not copied! into the chunks themselves
// The VB and IB are already packed for the mesh streams (platform specific), and so can be quickly patched
class CMeshChunkPart
{
public:
	static const Uint16 c_invalidChunkIndex = (Uint16)-1;

	CMeshChunkPart( Uint16 chunkIndex, TDynArray< Uint8, MC_BufferMesh >(& vertices)[ MAX_STREAMED_MESH_VERTEX_STREAMS ], TDynArray< Uint8, MC_BufferMesh >& indices )
		: m_chunkIndex(chunkIndex)
		, m_packedIndices( std::move(indices) )
	{
		for( Int32 s = 0; s < MAX_STREAMED_MESH_VERTEX_STREAMS; ++s )
		{
			m_packedVertices[s] = std::move( vertices[s] );
		}
	}

	CMeshChunkPart()
		: m_chunkIndex( c_invalidChunkIndex )
	{
	}

	CMeshChunkPart( CMeshChunkPart&& other )
	{
		*this = other;
	}

	CMeshChunkPart& operator=( CMeshChunkPart&& other )
	{
		m_chunkIndex = other.m_chunkIndex;
		other.m_chunkIndex = c_invalidChunkIndex;
		for( Int32 s = 0; s < MAX_STREAMED_MESH_VERTEX_STREAMS; ++s )
		{
			m_packedVertices[s] = std::move( other.m_packedVertices[s] );
		}
		m_packedIndices = std::move( other.m_packedIndices );
		return *this;
	}

	inline Uint16 GetChunkIndex() const { return m_chunkIndex; }
	inline const TDynArray< Uint8, MC_BufferMesh >& GetVertexStream( Int32 stream )								{ return m_packedVertices[stream]; }
	inline const TDynArray< Uint8, MC_BufferMesh >(& GetVertexStreams())[ MAX_STREAMED_MESH_VERTEX_STREAMS ]	{ return m_packedVertices; }
	inline const TDynArray< Uint8, MC_BufferMesh >& GetIndexBuffer()											{ return m_packedIndices; }

	// Serialisation 
	void Serialize( IFile& file );

private:
	CMeshChunkPart( const CMeshChunkPart& );
	CMeshChunkPart& operator=( const CMeshChunkPart& );

	Uint16 m_chunkIndex;																				// which CMesh::Chunk this data maps to
	TDynArray< Uint8, MC_BufferMesh >	m_packedVertices[ MAX_STREAMED_MESH_VERTEX_STREAMS ];		// Vertex streams
	TDynArray< Uint8, MC_BufferMesh >	m_packedIndices;												// Index data
};

// This class represents a collection of mesh chunk data. It may be a single chunk, the chunks for a LOD, or the chunks for an entire mesh
// They are NOT kept around, we only need them during the load (data is passed directly to the owner mesh)
class CStreamedMeshChunks
{
public:
	CStreamedMeshChunks();
	~CStreamedMeshChunks();

	// They cannot be copied but they CAN be moved
	CStreamedMeshChunks( CStreamedMeshChunks&& other );
	CStreamedMeshChunks& operator=(CStreamedMeshChunks&& other);

	// Add a vertex buffer and index buffer for a particular chunk. These should be kept strictly as one set of data per chunk. We won't handle resizing VBs!
	// The vb and ib are *moved* into this class - don't touch them after calling this!
	void AddChunk( Uint16 chunkIndex, TDynArray< Uint8, MC_BufferMesh >(& vertices)[MAX_STREAMED_MESH_VERTEX_STREAMS], TDynArray< Uint8, MC_BufferMesh >& indexData );
	
	CMeshChunkPart& GetChunkByIndex( Uint32 index );

	RED_INLINE Uint32 GetChunkCount() const { return m_chunkArray.Size(); }
	RED_INLINE void ClearCpuSideData() { m_chunkArray.Resize( 0 ); }

	RED_INLINE void SetMeshResource( const Red::Core::ResourceManagement::CResourceId& mesh ) { m_ownerMesh = mesh; }
	RED_INLINE const Red::Core::ResourceManagement::CResourceId& GetMeshResource() const { return m_ownerMesh; }

	// Serialisation 
	void Serialize( IFile& file );

private:
	CStreamedMeshChunks( const CStreamedMeshChunks& );
	CStreamedMeshChunks& operator=( const CStreamedMeshChunks& );

	Red::Core::ResourceManagement::CResourceId m_ownerMesh;		// Hash of the mesh we will pass the chunk to

	TDynArray< CMeshChunkPart, MC_BufferMesh > m_chunkArray;
};

// This is the resource manager factory for CStreamedMeshChunks objects
class CStreamedMeshChunksFactory : public Red::Core::ResourceManagement::IResourceFactoryBase
{
public:
	CStreamedMeshChunksFactory();
	~CStreamedMeshChunksFactory();

	// This factory needs access to the resource manager
	void SetResourceManager( Red::Core::ResourceManagement::CResourceManager* man )	{ m_resourceManager = man; }

	// Allocate memory for a new resource buffer. Return the size allocated
	virtual MemSize CreateResourceBuffer( Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData );

	// Serialize data from sourceBuffer to a new resource buffer. Returns the size of the new buffer
	virtual Bool Serialize( const void* srcBuffer, Uint32 sourceSize, Red::Core::ResourceManagement::CResourceDataBuffer& dstBuffer, void* userData );			

	// Destroy a resource
	virtual void DestroyResource( Red::Core::ResourceManagement::CResourceDataBuffer& buffer, void* userData );

private:
	Red::Core::ResourceManagement::CResourceManager* m_resourceManager;
};

namespace MeshStreamingUtils
{
	// Stuff used by either the tools (bundle def building) or the game
	void GenerateChunksFromMesh( CMesh& sourceMesh, TDynArray< CStreamedMeshChunks >& resultingChunks, Int32 lodIndex=-1 );

	// Get the number of chunk resources expected for a particular mesh
	Int32 GetChunkResourceCount( CMesh& sourceMesh, Int32 lodIndex=-1 );

	// Save mesh chunks to a file on disk. Returns full path to saved file
	String SaveMeshChunksToDisk( CDirectory& directory, const String& meshName, Int32 chunksIndex, CStreamedMeshChunks& chunks );

	// Get the path to a mesh's chunk resource with a particular index
	String GetMeshChunkFilename( const String& meshName, Int32 chunksIndex );

	// Save a stripped mesh to disk. Returns path to the saved file
	String SaveStrippedMeshToDisk( CDirectory& directory, CMesh& theMesh );
}

#endif