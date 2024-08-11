/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderOcclusionData;
class CRenderElement_MeshChunk;
class CRenderProxy_Mesh;
class IRenderProxyDrawable;
struct SSpeedtreeWindParams;
struct Chunk;
class CRenderMesh;

#define MAX_INDIRECT_ARGS 65

struct SBatcherInfo
{

	enum EBatcherDissolveType
	{
		EBDT_None = 0,
		EBDT_Regular,
		EBDT_Shadowed
	};

	enum EBatcherFunctionType
	{
		// General flags
		EBFT_HitProxyBatch	= FLAG(0),
		EBFT_Selection		= FLAG(1),
		EBFT_Skinning		= FLAG(2),
		EBFT_Eye			= FLAG(3),
		EBFT_Skin			= FLAG(4),
		// Unused,				// 5
		// Unused,				// 6
		// Unused,				// 7

		// Fragment related
		EBFT_ColorShift		= FLAG(8),
		EBFT_NormalBlend	= FLAG(9),
		EBFT_EffectParam0	= FLAG(10),
		EBFT_EffectParam1	= FLAG(11),
		EBFT_MaterialOveride= FLAG(12),
		EBFT_DissolveUV		= FLAG(13),
		EBFT_Morph			= FLAG(14),
		// Unused,				// 15

		// Proxy related
		EBFT_ClippingElipse	= FLAG(16),

		// Some stuff
		EBFT_None			=	0,
		EBFT_All			=	EBFT_HitProxyBatch | EBFT_Selection | EBFT_Skinning | EBFT_Eye | EBFT_Skin |
								EBFT_ColorShift | EBFT_NormalBlend | EBFT_EffectParam0 | EBFT_EffectParam1 | EBFT_MaterialOveride | EBFT_DissolveUV | EBFT_Morph | 
								EBFT_ClippingElipse

	};

	CRenderMeshBatcher*			m_batcher;
	CRenderMesh*				m_mesh;
	CRenderElement_MeshChunk*	m_batchList;
	IRenderProxyDrawable*		m_proxy;
	CRenderElement_MeshChunk*	m_frag;
	CRenderStateManager*		m_stateManager;

	const CRenderFrameInfo*		m_info;
	const RenderingContext*		m_context;
	const Batch*				m_batch;
	class MeshDrawingStats*		m_outMeshStats;

	Uint32						m_chunkIndex;
	Uint32						m_renderFlags;

	Uint64						m_batcherMask;
	Uint64						m_batcherInclude;

	Vector						m_skinningData;

	Vector						m_dissolveData;
	EBatcherDissolveType		m_dissolveType : 8;

	Bool						m_needsSeparateDraw;

	SBatcherInfo()
		: m_batcher( nullptr )
		, m_info( nullptr )
		, m_context( nullptr )
		, m_batch( nullptr )
		, m_mesh( nullptr )
		, m_chunkIndex( 0 )
		, m_renderFlags( 0 )
		, m_batchList( nullptr )
		, m_proxy( nullptr )
		, m_frag( nullptr )
		, m_stateManager( nullptr )
		, m_outMeshStats( nullptr )
		, m_needsSeparateDraw( false )
		, m_batcherMask( EBFT_All )
		, m_batcherInclude( EBFT_None )
		, m_dissolveType( EBDT_None )
	{
#ifdef NO_COMPONENT_GRAPH
		m_batcherMask &= ~EBFT_HitProxyBatch;
#endif
#ifdef NO_EDITOR
		m_batcherMask &= ~EBFT_Selection;
#endif
	}

	RED_INLINE static Uint64 TransformFlag( Uint64 mask, Uint64 srcFlag , Uint64 dstFlag )
	{
		return !!( mask & srcFlag ) * dstFlag;
	}

};

// Drawable batch
struct Batch
{
	Uint32						m_numFragments;
	CRenderMaterial*			m_material;
	CRenderMaterialParameters*	m_parameters;
	CRenderMesh*				m_mesh;
	Uint32						m_chunkIndex;
	EMaterialVertexFactory		m_vertexFactory;
	CRenderElement_MeshChunk*	m_elements;
	Bool						m_isSkinned;
	Bool						m_isSelected;
	Bool						m_hasExtraStreams;
	Bool						m_hasVertexCollapse;
	Uint32						m_lightChannel;
	Bool						m_clippingEllipse;
	Bool						m_isTwoSided;
	Float						m_closestDistance;
};	

/// Mesh rendering flags
enum ERenderMeshBatcherFlags
{
	RMBF_Shadowmap				= FLAG( 0 ),		//!< Rendering things into a shadow map
	RMBF_CascadeShadows			= FLAG( 1 ),		//!< Rendering cascaded shadows
	RMBF_Transparent			= FLAG( 2 ),		//!< Rendering transparent objects
};

struct SMeshInstanceDescriptor
{
	Float	m_localToWorld[12];
	Vector	m_detailLevelParams;
};

struct SSkinnedMeshInstanceDescriptor
{
	Float	m_localToWorld[12];
	Vector	m_detailLevelParams;
	Vector	m_skinningData;
};

/// Rendering for meshes
class CRenderMeshBatcher : public IDynamicRenderResource
{
	friend class SBatcherMacroNode;

protected:
	Uint32							m_maxFragmentsPerBatch;			//!< Number of mesh chunks drawable in one batch

protected:
	CRenderMaterial*				m_lastMaterial;					//!< Last bound material
	CRenderMaterialParameters*		m_lastParameters;				//!< Last bound parameters
	const IRenderProxyDrawable*		m_lastProxy;					//!< Last bound proxy
	const IRenderProxyDrawable*		m_lastProxy_Instanced;			//!< Last proxy during instanced batch generation
	Int32							m_lastCharactersLightingBoost;	//!< Last needs characters lighting boost

	TDynArray< SMeshInstanceDescriptor >	m_instances;				//!< Array of instances generated from a batch
	TDynArray< SSkinnedMeshInstanceDescriptor >	m_skinnedInstances;				//!< Array of instances generated from a batch
	GpuApi::BufferRef						m_instancesBufferRef;		//!< Patch instancing buffer
	Uint32									m_instancesDataElemOffset;	//!< Current instances write position
	
	Matrix									m_lastLocalToWorld;				//!< ...of the last proxy processed
	Matrix									m_lastInstancedLocalToWorld;	//!< ...of the last proxy processed during instanced batch

public:
	CRenderMeshBatcher( Uint32 maxFragmentsPerBatch );
	~CRenderMeshBatcher();

	//! Render batch list of static mesh
	void RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, CRenderElement_MeshChunk* batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats );
	
	//! Render batch of merged meshes
	void RenderMergedMeshes( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats );

	//! Render batch of merged meshes with CsVs culling
	void RenderMergedMeshesCsVs( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats );

	//! Render batch of static mesh
	void RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats );

	//! Render batch of static mesh
	void RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, Uint32 numChunks, CRenderElement_MeshChunk * const *chunks, Uint32 renderFlags, class MeshDrawingStats& outMeshStats );
	
protected:
	//! Draw a single batch of meshes, all states are set, just DRAW DRAW DRAW
	static void DrawBatchOfMeshes( SBatcherInfo& batcherInfo );

	//! Set render states needed for batch
	bool SelectBatchRenderStates( const Batch& batch, const RenderingContext& context, Uint32 renderFlags );

	//! Build skinning matrices cache for given frame
	void BuildSkinningMatricesCache( CRenderElement_MeshChunk* batchList );

	//! Compute local to world based on given proxy, element, and context
	void ComputeLocalToWorld( const RenderingContext& context, const IRenderProxyDrawable* proxy, const CRenderElement_MeshChunk* fragment, Matrix& outMatrix ) const;

	//! Make an instanced drawcall for a given mesh
	void DrawInstances( CRenderMesh* mesh, Uint32 chunkIndex, void* bufferData, Uint32 numInstances, Uint32 streamStride, MeshDrawingStats& outMeshStats );

protected:
	// Device Reset/Lost handling
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
	virtual CName GetCategory() const;
	virtual Uint32 GetUsedVideoMemory() const;

private:

	struct SCsCullCB
	{
		// generic
		Matrix m_localToWorld;
		Matrix m_worldToScreen;
		Vector m_qs;
		Vector m_qb;
		Vector m_count;
	};

	GpuApi::BufferRef m_customCsCB;

	void InitCsCullingBuffers();
	void ReleaseCsCullingBuffers();

	void DispatchCsVs( const Chunk& chunk );
};
