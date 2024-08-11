/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderDynamicResource.h"
#include "../engine/gpuDataBuffer.h"
#include "../core/deferredDataBuffer.h"
#include "renderResourceIterator.h"
#include "../engine/meshPacking.h"

class IRenderElement;
struct SMeshCookedData;
class CBuildClustersTask;

// These stream indices should match what is specified for the mesh vertex layouts in gpuApiVertexDeclarations.h
enum ERenderMeshStreams
{
	RMS_PositionSkinning		= FLAG( 0 ),
	RMS_TexCoords				= FLAG( 1 ),
	RMS_TangentFrame			= FLAG( 2 ),
	RMS_Extended				= FLAG( 3 ),

	// For custom rendering requirements...
	RMS_Custom0					= FLAG( 4 ),

	RMS_BindAll					= 0xFF,				//!< Bind all used streams. Maximum of 8 are allowed by GpuApi, so 0xFF is enough.
};

// Keep up to date with any additions to ERenderMeshStreams!
#define RENDER_MESH_NUM_STREAMS		5

RED_FORCE_INLINE Uint32 RenderMeshStreamToIndex( ERenderMeshStreams stream )
{
	Uint32 index = MLog2( stream );
	RED_ASSERT( stream == FLAG( index ), TXT("Invalid stream '%d'"), stream );
	return index;
}

/// Mesh ( static or skinned )
class CRenderMesh : public IDynamicRenderResource, public TRenderResourceListWithCache<CRenderMesh>
{
	DECLARE_RENDER_RESOURCE_ITERATOR;

	friend class CRenderMorphedMesh;
	friend class CRenderDecalMesh;
	friend class CRenderDestructionMesh;
public:
	// Description of the mesh geometry to use when binding the mesh. Does not maintain references to any of the buffers.
	struct ChunkGeometry
	{
		GpuApi::VertexLayoutRef		m_vertexLayout;
		GpuApi::BufferRef			m_vertexBuffers[ RENDER_MESH_NUM_STREAMS ];
		Uint32						m_vertexOffsets[ RENDER_MESH_NUM_STREAMS ];

		GpuApi::BufferRef			m_indexBuffer;
		Uint32						m_indexOffset;

		ChunkGeometry()
			: m_indexOffset( 0 )
		{
			Red::System::MemoryZero( m_vertexOffsets, sizeof( m_vertexOffsets ) );
		}
	};


	struct VertexBufferChunk
	{
		VertexBufferChunk()
			: type( GpuApi::BCT_Max )
		{
			Red::System::MemoryZero( byteOffsets, sizeof( byteOffsets ) );
		}
		VertexBufferChunk( const VertexBufferChunk& other )
			: type( other.type )
		{
			Red::System::MemoryCopy( byteOffsets, other.byteOffsets, sizeof( byteOffsets ) );
		}

		~VertexBufferChunk()
		{
		}

		VertexBufferChunk& operator =( const VertexBufferChunk& other )
		{
			if ( this != &other )
			{
				type = other.type;
				Red::System::MemoryCopy( byteOffsets, other.byteOffsets, sizeof( byteOffsets ) );
			}

			return *this;
		}

		GpuApi::eBufferChunkType type;						//!< The default chunk type

		Uint32 byteOffsets[RENDER_MESH_NUM_STREAMS];		//!< Offset into each vertex buffer where this chunk starts.
	};

	struct IndexBufferChunk
	{
		IndexBufferChunk()
			: type( GpuApi::BCT_Max )
			, byteOffset( 0 )
		{
		}

		GpuApi::eBufferChunkType type;
		Uint32 byteOffset;
	};

	//! Rendering mesh chunk
	struct Chunk
	{
		VertexBufferChunk	m_chunkVertices;				//!< Buffer data with vertices
		IndexBufferChunk	m_chunkIndices;					//!< Buffer data with indices
		Uint16				m_numVertices;					//!< Number of vertices in this chunk
		Uint32				m_numIndices;					//!< Number of indices in this chunk
		Uint8				m_materialId;					//!< Material index
		Uint8				m_vertexFactory;				//!< Base vertex factory to use for this chunk
		Uint8				m_baseRenderMask;				//!< Render mask of this chunk - telling us where should we render this chunk (scene, cascades, etc)
		Uint8				m_mergedRenderMask;				//!< In case this is part of a merged content here we have the merged render mask
		Uint8				m_lodMask;						//!< Chunk LOD mask - in which LODs do we exist

		// Serialize buffer type
		static void SerializeVertexBufferChunk( IFile& ar, VertexBufferChunk& b )
		{
			// layout override is not set for any cooked mesh data, just when creating additional meshes at runtime, so
			// we don't need to try to save that.

			// Buffer typed offset
			for ( Uint32 i = 0; i < RENDER_MESH_NUM_STREAMS; ++i )
			{
				ar << b.byteOffsets[i];
			}

			// Buffer type
			Uint8 val = (Uint8) b.type;
			ar << val;
			b.type = ( GpuApi::eBufferChunkType )val;
		}

		static void SerializeIndexBufferChunk( IFile& ar, IndexBufferChunk& b )
		{
			// Buffer offset
			ar << b.byteOffset;

			// Buffer type
			Uint8 val = (Uint8) b.type;
			ar << val;
			b.type = ( GpuApi::eBufferChunkType )val;
		}

		// Serialize vertex factory
		static void SerializeMaterialVertexFactory( IFile& ar, Uint8& vt )
		{
			Uint8 val = ( Uint32 ) vt;
			ar << val;
			vt = val;
		}

		// Friendly serialization operator
		friend IFile& operator<<( IFile& ar, Chunk& chunk )
		{
			SerializeMaterialVertexFactory( ar, chunk.m_vertexFactory );
			SerializeVertexBufferChunk( ar, chunk.m_chunkVertices );
			SerializeIndexBufferChunk( ar, chunk.m_chunkIndices  );
			ar << chunk.m_numVertices;
			ar << chunk.m_numIndices;
			ar << chunk.m_materialId;
			ar << chunk.m_baseRenderMask;
			ar << chunk.m_mergedRenderMask;
			ar << chunk.m_lodMask;
			return ar;
		}
	};

	///! Rendering mesh LOD
	struct LODLevel
	{
		Float			m_distance;
	};

public:
	//! Get number of chunks in this mesh
	RED_INLINE const Uint32 GetNumChunks() const { return m_chunks.Size(); }

	//! Get chunks
	RED_INLINE const TDynArray< Chunk, MC_RenderMeshChunks >& GetChunks() const { return m_chunks; }

	//! Get number of LODs in this mesh
	RED_INLINE const Uint32 GetNumLODs() const { return m_lods.Size(); }

	//! Get LODs
	RED_INLINE const TDynArray< LODLevel, MC_RenderMeshChunks >& GetLODs() const { return m_lods; }

public:
	RED_INLINE Uint32 GetModelId() const { return m_modelId; }
	
	RED_INLINE Bool IsTwoSided() const { return m_isTwoSided; }

	RED_INLINE Bool IsClusterDataReady() { return m_cullingBuffers.Size() && FinishClusterBuildingTaskOnAccess(); }

	virtual Bool IsFullyLoaded() const override final { return m_buffersPendingLoad.GetValue() == 0; }

	RED_INLINE void* GetInPlaceVertexRegionPtr() { return m_vertexMemRegion.IsValid() ? m_vertexMemRegion.GetRawPtr() : nullptr; }
	RED_INLINE void* GetInPlaceIndexRegionPtr() { return m_indexMemRegion.IsValid() ? m_indexMemRegion.GetRawPtr() : nullptr; }

protected:
	CRenderMesh();

public:
	~CRenderMesh();

	//special usage in morphing
	CRenderMesh( const CRenderMesh& toCopy );

	//! Set up bone position texture, if it hasn't already been created. If the texture is already created, this does nothing.
	//! Safe to call from non render thread (shouldn't be called from render thread, really... since it needs a CMesh...)
	void InitBonePositionTexture( const CMesh* engineMesh );

	//! Get the special texture with bone positions
	GpuApi::TextureRef GetBonePositionTexture() const { return m_bonePositions; }

	//! Bind mesh buffers ( vertex and index buffer ) for given chunk. Caller is responsible for ensuring chunkIndex is valid. Bind
	//! will not do any validation. In addition, Bind must not be called when the mesh is not fully loaded (IsFullyLoaded).
	//! CsVs is a compute shader based culling of geometry in clip-space.
	void BindCsVsBuffersCS( Uint32 chunkIndex, Vector& qb, Vector& qs, Uint32& indexOffset, Uint32& vertexOffset );

	//! Bind mesh vertex buffer for given chunk.  Bind will not do any validation and assumes it's called after invocation of BindCsVsBuffersUAV.
	//! CsVs is a compute shader based culling of geometry in clip-space.
	void BindBuffersVS( Uint32 chunkIndex );
	void BindCsVsBuffersVS( Uint32 chunkIndex );

	//! Bind mesh buffers ( vertex and index buffer ) for given chunk. Caller is responsible for ensuring chunkIndex is valid. Bind
	//! will not do any validation. In addition, Bind must not be called when the mesh is not fully loaded (IsFullyLoaded).
	//! streamsToBind is combination of ERenderMeshStreams.
	void Bind( Uint32 chunkIndex, Uint32 streamsToBind = RMS_BindAll );

	// Draw chunk, no data binding, just a DIP
	void DrawChunkNoBind( Uint32 chunkIndex, Uint32 fragCount, class MeshDrawingStats& outMeshStats );

	// Draw chunk, no data binding, just a DIP
	void DrawChunkNoBindInstanced( Uint32 chunkIndex, Uint32 fragCount, Uint32 instanceCount, class MeshDrawingStats& outMeshStats );

	// Describe resource
	virtual CName GetCategory() const;

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

	// Device lost
	virtual void OnDeviceLost();

	// Device reset
	virtual void OnDeviceReset();

	void GetQuantization( Vector* scale, Vector* offset ) const;

public:
	// Compile from static mesh
	static CRenderMesh* Create( const CMesh* mesh, Uint64 partialRegistrationHash );

	// Compile from static mesh uncached ( does not use cooked data )
	static CRenderMesh* CreateRaw( const CMesh* mesh, Uint64 partialRegistrationHash );

	// Compile from cooked data
	static CRenderMesh* CreateCooked( const CMesh* mesh, Uint64 partialRegistrationHash );

	// Setup buffers
	void SetupRenderData( BufferHandle cookedRenderData, const Uint32 vertexDataSize, const Uint32 indexDataSize, const Uint32 indexDataOffset, const String& debugName );

	struct SClusterData
	{
		Vector Centroid; //(Pos, R)
		Uint32 VEnd;
		Uint32 IStart;
		Uint32 IEnd;
	};

	// Setup Culling cluster data
	void SetupCullingClusterDataVB( const void* vb, const CMeshPackingSource& packer );
	void SetupCullingClusterDataVB( const void* vb );
	void SetupCullingClusterDataIB( const void* ib, const CMeshPackingSource& packer );
	void SetupCullingClusterDataIB( const void* ib );

	// Setup indirect and culled indices buffers
	void SetupCsVsBuffers();

	// Finalize cluster builder task
	Bool FinishClusterBuildingTaskOnAccess();
	Bool FinishClusterBuildingTaskOnDestruction();

	RED_INLINE TDynArray<SClusterData>& GetCullingClusterData( Uint32 chunkIdx ){ return m_cullingBuffers[chunkIdx].m_clusters; }

	// Inplace data allocation
	void AllocateInplaceRenderData( Uint32 totalDataSize, Uint32 indexDataSize, Uint32 vertexDataSize, Uint32 indexDataOffset, Uint32 alignment );
	void DeallocateInplaceRenderData();
	void FlushInplaceDataCaches();

#ifndef RED_FINAL_BUILD
	String GetMeshDepotPath() const { return ANSI_TO_UNICODE(m_depotPath.AsChar()); }
	Uint32 GetMeshSize() const { return m_dataSize; }
#endif


protected:

	//! Get the geometry to bind for the given chunk. Caller is responsible for making sure chunkIndex is valid. GetChunkGeometry
	//! is not obliged to do any range checking.
	virtual void GetChunkGeometry( Uint8 chunkIndex, ChunkGeometry& outGeometry ) const;

	virtual GpuApi::VertexLayoutRef GetChunkVertexLayout( Uint8 chunkIndex ) const;

public:
	CRenderMesh*		m_batchNext;	// Next in batch
	IRenderElement*		m_batchList;	// Per mesh batch list

protected:
	struct SCullingBuffers
	{
		GpuApi::BufferRef m_indirectArgs;	//!< Mesh indirect drawcall args
		GpuApi::BufferRef m_culledIndices;	//!< Mesh culled indices
		TDynArray<SClusterData> m_clusters;
	};

	typedef GpuApi::BufferRef									TVertexStreams;
	typedef GpuApi::BufferRef									TIndexStreams;
	typedef TDynArray< SCullingBuffers >						TCullingArgs;
	typedef TDynArray< Chunk, MC_RenderMeshChunks >				TChunks;
	typedef TDynArray< LODLevel, MC_RenderMeshChunks >			TLODs;

	TVertexStreams					m_vertices;					//!< Mesh vertices
	TIndexStreams					m_indices;					//!< Mesh indices
	TCullingArgs					m_cullingBuffers;			//!< Mesh indirect drawcall args
	TChunks							m_chunks;					//!< Mesh chunks
	TLODs							m_lods;						//!< Mesh LOD level data

	Red::Threads::CAtomic< Int32 >	m_buffersPendingLoad;		//!< Number of internal buffers that are still not yet loaded (if 0 - mesh is ready for rendering)

	Vector							m_quantizationScale;		//!< Mesh quantization scale
	Vector							m_quantizationOffset;		//!< Mesh quantization offset
	Bool							m_isTwoSided;				//!< Is mesh two-sided
	CBuildClustersTask*				m_clusterBuilderTask;
	Uint32							m_modelId;

	GpuApi::TextureRef				m_bonePositions;			//!< Texel (i,0) has model-space position of i'th bone. Used for vertex collapse.

	BufferAsyncDataHandle			m_bufferLoading;			//!< Loading handle for the buffer data
	Red::Threads::CSpinLock			m_bufferLoadingFence;

	Red::MemoryFramework::MemoryRegionHandle	m_vertexMemRegion;	//!< In place memory region storing all vertex data
	Red::MemoryFramework::MemoryRegionHandle	m_indexMemRegion;	//!< In place memory region storing all index data

#ifndef RED_FINAL_BUILD
	StringAnsi						m_depotPath;
	Uint32							m_dataSize;
#endif
};

class CBuildClustersTask : public CTask
{
public:
	CBuildClustersTask(){};
	CBuildClustersTask( const void* vb, const void* ib, CRenderMesh* renderMesh );

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT("CBuildClustersTask"); }
	virtual Uint32 GetDebugColor() const override { return COLOR_UINT32( 0, 233, 199 ); }
#endif

	virtual void Run() override;

	const void* m_vb;
	const void* m_ib;

	CRenderMesh* m_renderMesh;
};