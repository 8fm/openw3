/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyDissolvable.h"
#include "renderDynamicDecalTarget.h"
#include "renderCollector.h" //for the shadow cascades
#include "renderDissolve.h"
#include "renderEntityGroup.h"
#include "renderVisibilityExclusionMap.h"
#include "renderMesh.h"
#include "../engine/meshRenderSettings.h"


// Hacky fix for missing entity proxy mesh shadows in cascade 1. If we could change the data for all proxy meshes, and properly
// set shadow-casting properties, that would be better. But for now this is about all we can do. For future, should be able to
// remove everything wrapped in this define, and set up the assets properly.
#define HACKY_ENTITY_PROXY_SHADOWS_FIX 1



class CRenderElement_MeshChunk;
class CRenderProxyShadowCulling;
class CRenderSkinningData;

enum ERenderMeshProxyFlags
{
	RMPF_DrawOriginalWhileReplacingMaterial		= FLAG( 1 ),		//!< Do we draw original mesh when we replace material?
	RMPF_UsesVertexCollapse						= FLAG( 2 ),		//!< Uses vertex collapse
	RMPF_IsCharacterProxy						= FLAG( 3 ),		//!< True if proxy is for character
	RMPF_ForceHighestLOD						= FLAG( 4 ),		//!< Use Highest LOD on this mesh, regardless of the actual LOD computation settings
	RMPF_CastingShadowsEvenIfNotVisible			= FLAG( 5 ),		//!< Proxy used as base of character shadows
	RMPF_MergedMesh								= FLAG( 6 ),		//!< This is a merged mesh geometry
	RMPF_ForceDynamic							= FLAG( 7 ),		//!< Is forced dynamic mesh (for RenderElementMap) -> attached via HardAttachment, placed on ignored layer, etc.
	RMPF_IsCharacterShadowFallback				= FLAG( 8 ),		//!< Is character shadow fallback
	RMPF_IsEntityProxy							= FLAG( 9 ),		//!< This is an entity proxy
	RMPF_UseShadowDistances						= FLAG( 10 ),		//!< Is using shadow distances
	RMPF_HasAdditionalShadowElements			= FLAG( 11 ),		//!< Does this proxy have any additional elements that should be drawn into the cascades event when the proxy itself is filtered out by the merged shadows ?
	RMPF_IsStaticInRenderElementMap				= FLAG( 12 ),		//!< Is this proxy in the render element map's static proxies list?
	RMPF_IsVolumeMesh							= FLAG( 13 ),
	RMPF_DissolveOverride						= FLAG( 14 ),		//!< If set, forces dissolve (isDissolved flag in all collects will be true)
#if HACKY_ENTITY_PROXY_SHADOWS_FIX
	RMPF_HACK_EntityProxyNeedsShadows			= FLAG( 15 )		//!< Hack, entity proxies need to be drawn to cascade 1
#endif
};


class IRenderMeshChunkMaterialProvider
{
public:
	virtual ~IRenderMeshChunkMaterialProvider() {}

	// Returns render material and parameters. Returned objects hold a reference, so caller is responsible for release.
	virtual Bool GetMaterialForChunk( Uint8 chunkIndex, CRenderMaterial*& outMaterial, CRenderMaterialParameters*& outMaterialParams ) const = 0;
};


class CMeshTypeResourceMaterialProvider : public IRenderMeshChunkMaterialProvider
{
private:
	const CMeshTypeResource* m_mesh;
	const CRenderMesh* m_renderMesh;

public:
	CMeshTypeResourceMaterialProvider( const CMeshTypeResource* mesh, const CRenderMesh* renderMesh );
	virtual Bool GetMaterialForChunk( Uint8 chunkIndex, CRenderMaterial*& outMaterial, CRenderMaterialParameters*& outMaterialParams ) const override;
};


/// Rendering proxy for mesh
class CRenderProxy_Mesh : public IRenderProxyDissolvable, public IDynamicDecalTarget, public IRenderEntityGroupProxy
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderProxyMesh )
protected:
	
	// LOD group for mesh
	class MeshLodGroup
	{
	private:
		CRenderElement_MeshChunk**					m_chunks;				// Mesh chunks	
		Uint8										m_numChunks;			// Number of mesh elements in this LOD group

		void Reallocate( Uint32 count );

	public:
		//! Default
		MeshLodGroup();

		//! Mesh LOD level
		MeshLodGroup( CRenderProxy_Mesh* proxy, CRenderMesh* mesh, const IRenderMeshChunkMaterialProvider& materialProvider, Int32 sourceLodGroupIndex, Int32 lodIndex, const Uint8 renderFilterMask = 0xFF );

		//! Destructor
		~MeshLodGroup();

		//! Create a copy of this LOD Group, replacing materials on any compatible chunks. Chunks with replaced materials will
		//! have the RMCF_MaterialOverride flag set.
		MeshLodGroup* CreateCopyWithMaterialOverride( CRenderMaterial* material, CRenderMaterialParameters* parameters ) const;

		//! Add chunks from a CMesh to this group.
		void Append( CRenderProxy_Mesh* proxy, CRenderMesh* mesh, const IRenderMeshChunkMaterialProvider& materialProvider, Int32 sourceLodGroupIndex, Int32 lodIndex, const Uint8 renderFilterMask = 0xFF );

		//! Add pre-made chunks to this group. The chunks should be totally OK (i.e. non-null, have materials, etc). On non-final builds this is
		//! checked, and the function will fail if bad chunks are found, but ideally this should not be needed.
		void Append( CRenderElement_MeshChunk* const* newChunks, Uint8 numChunks );

		CRenderElement_MeshChunk* GetChunk( Uint32 index ) const;
		CRenderElement_MeshChunk** GetChunks() const;
		void ReleaseChunk( Uint32 index );
		void SetChunk( Uint32 index,  CRenderElement_MeshChunk* chunk );
		RED_INLINE Uint32 GetChunkCount() const { return m_numChunks; }
	};

	// LOD
	TDynArray< MeshLodGroup, MC_RenderData >	m_lodGroups;							//!< LOD groups

	// MergedMesh support
	LocalVisObjectID							m_localVisId;							//!< ID of this object as it's registered in the visibility system
	Uint8										m_renderMask;							//!< Merged rendering mask for object

	// Material replacement
	MeshLodGroup*								m_replacedMaterialLodGroup;				//!< LOD group for mesh with replaced material, LOD = 0

	// Normal blends
	MeshLodGroup*								m_normalBlendLodGroup;					//!< LOD group for normal blending. LOD = 0, may contain NULL chunks
	Float*										m_normalBlendAreas;						//!< Areas for normal blending
	Float*										m_normalBlendWeights;					//!< Weights for normal blending

	CRenderSkinningData*						m_skinningData;							//!< Mesh skinning data
	Uint16										m_meshProxyFlags;						//!< Merged mesh proxy flags

	CRenderMesh*								m_mesh;									//!< Mesh that is rendered

	CRenderProxyObjectGroup*					m_renderGroup;							//!< The render group we are registered in

	Bool										m_skipOcclusionTest : 1;					//!< Hack

#ifndef NO_EDITOR
	// Selection
	Vector										m_selectionColor;						//!< Selection color
#endif

public:
	CRenderMesh* GetMesh() { return m_mesh; }

	//! Get LOD groups
	RED_INLINE const TDynArray< MeshLodGroup, MC_RenderData >& GetLodGroups() const { return m_lodGroups; }

	//! Get LOD count
	RED_INLINE Int32 GetNumLods() const { return (Int32)m_lodGroups.Size(); }

	//! Get skinning data for proxy
	RED_INLINE CRenderSkinningData* GetSkinningData() const { return m_skinningData; }

	//! Uses vertex collapse?
	RED_INLINE Bool UsesVertexCollapse() const { return 0 != ( m_meshProxyFlags & RMPF_UsesVertexCollapse ); }

	//! Returns true if given mesh proxy flag is set
	RED_INLINE Bool CheckMeshProxyFlag( ERenderMeshProxyFlags flag ) const { return 0 != ( m_meshProxyFlags & flag ); }

	//! Is this a forced dynamic proxy ?
	RED_INLINE Bool IsForcedDynamic() const { return CheckMeshProxyFlag( RMPF_ForceDynamic ); }

	//! Was the proxy registered into the render element map's static proxies list?
	RED_INLINE Bool IsStaticInRenderElementMap() const { return CheckMeshProxyFlag( RMPF_IsStaticInRenderElementMap ); }

	//! Set whether it's in the REM's static list. Should _only_ be called by REM when proxy is registered.
	RED_INLINE void SetStaticInRenderElementMap( Bool isStatic ) { SetMeshProxyFlag( RMPF_IsStaticInRenderElementMap, isStatic ); }

	RED_INLINE Bool SkipOcclusionTest() const { return m_skipOcclusionTest; }

#ifndef NO_EDITOR
	void UpdateMeshRenderParams( const SMeshRenderParams& params );
#endif

	virtual CRenderTexture* GetUVDissolveTexture( const CRenderElement_MeshChunk* chunk ) const { return nullptr; }
	virtual Vector GetUVDissolveValues( const CRenderElement_MeshChunk* chunk ) const { return Vector::ZEROS; }
	virtual Bool DoesUVDissolveUseSeparateTexCoord( const CRenderElement_MeshChunk* chunk ) const { return false; }

protected:
	enum INIT_SUBCLASS_TAG { INIT_SUBCLASS };
	// Use this constructor for subclasses, to ensure that the _Mesh base won't do any preemptive initialization.
	CRenderProxy_Mesh( const RenderProxyInitInfo& initInfo, INIT_SUBCLASS_TAG );

public:
	CRenderProxy_Mesh( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_Mesh();

protected:
	//////////////////////////////////////////////////////////////////////////
	// Bunch of stuff called during construction. These probably should not
	// be used outside of constructor. Subclasses can use them, probably along
	// with the "INIT_SUBCLASS_TAG" constructor, to have custom creation.

	void SetMeshProxyFlagsFromComponent( const CComponent* component );
	void SetDefaultEffectParameters( const CComponent* component );
	void SetupUmbra( const CMeshTypeResource* meshTypeResource, const CComponent* component );
	void SetupAutohideDistance( const CMeshTypeResource* meshTypeResource, const RenderProxyInitInfo& initInfo );

	// If non-null, keep reference to provided renderMesh, and create LOD groups from it.
	void InitializeRenderMesh( CRenderMesh* renderMesh, const CMeshTypeResource* meshTypeResource, Int32 forcedLOD, Uint8 allowedRenderMask );
	// Populate m_lodGroups. Requires m_mesh to already be set, as well as m_autoHideDistance
	void CreateLodGroups( const IRenderMeshChunkMaterialProvider& materialProvider, Int32 forcedLOD, Uint8 renderMask );

	// Set m_renderMask. Must come after creating LOD groups.
	void SetupRenderMask( Uint8 allowedRenderMask );

	Uint8 CalculateAllowedRenderMask( const CMeshTypeResource* meshTypeResource ) const;
	Uint8 PatchAllowedRenderMask( Uint8 allowedRenderMask ) const;

	void W3HACK_ApplyAutohideDistanceHack( const CMeshTypeResource* meshResource );
	void W3HACK_ApplyProxyMeshUmbraHack();
	void W3HACK_ApplyVolumeMeshFlag();
	void W3HACK_ApplyUmbraHacks( const CMeshTypeResource* meshTypeResource );
	//////////////////////////////////////////////////////////////////////////

public:

	virtual IDynamicDecalTarget* ToDynamicDecalTarget() { return this; }

	//! Is this a morphed mesh ?
	virtual Bool IsMorphedMesh() const { return false; }

	//! Get proxy rendering mask - helps with visibility filtering (avoids testing proxy in cases when it does not want to be rendered any way)
	virtual const Uint8 GetRenderMask() const override { return m_renderMask; };

	//! Relink proxy - update bounding box, local to world matrix, spatial caching etc
	virtual void Relink( const Box& boundingBox, const Matrix& localToWorld );

	virtual void OnNotVisibleFromAutoHide( CRenderCollector& collector ) override;

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector ) override;

	//! Collect elements for shadow rendering
	virtual void CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults ) override;

	//! Force Collect shadows lods
	void CollectShadowLodsForced( Uint8 lodIndex, CRenderCollector::SRenderElementContainer& elementCollector );

	//! Collect local shadow elements ( for point and spot lights )
	virtual void CollectLocalShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector );

	//! Collect local shadow elements for STATIC shadows ( highest LOD, no dissolve )
	virtual void CollectStaticShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector );

	//! Collect elements for rendering hires shadows
	virtual void CollectHiResShadowsElements( CRenderCollector& collector, CRenderCollector::HiResShadowsCollector& elementCollector, Bool forceHighestLOD );

	//! Is all data for this proxy ready ? Should return true only if proxy can be safely displayed
	//! The only legal transition is false->true, getting "unready" suddenly is not legal
	virtual Bool IsProxyReadyForRendering() const;

	//! Set skinning data
	void SetSkinningData( IRenderObject* skinningData );

#ifndef NO_EDITOR
	//! Get selection color
	virtual void GetSelectionColor( Vector &selectionColor /* out */ ) { selectionColor = m_selectionColor; }

	//! Set selection color
	virtual void SetSelectionColor( const Vector &selectionColor ) { m_selectionColor = selectionColor; }
#endif

	//! Set material replaced by gameplay
	virtual void SetReplacedMaterial( IRenderResource* material, IRenderResource* parameters, Bool drawOriginal );

	//! Has material replaced by gameplay?
	virtual Bool HasReplacedMaterial();

	//! Disable material replacement
	virtual void DisableMaterialReplacement();
	
	//! Set normal-blend material. The nbMaterial will be used to replace chunks using the given base material and/or normals texture.
	//! sourceMaterial and sourceTexture can be NULL, in which case they will not be used to filter the replaced materials.
	void SetNormalBlendMaterial( IRenderResource* nbMaterial, IRenderResource* parameters, IRenderResource* sourceMaterial, IRenderResource* sourceTexture );

	//! Each area is 4 Floats: left, top, right, bottom. firstArea + numAreas <= NUM_MIMIC_AREAS.
	void SetNormalBlendAreas( Uint32 firstArea, Uint32 numAreas, const Float* areas );

	//! Negative weights are clamped to 0.
	void SetNormalBlendWeights( Uint32 firstWeight, Uint32 numWeights, const Float *weights );

	//! Checks if any normal-blend area has a non-zero weight.
	Bool IsAnyNormalBlendAreaActive() const;

	//! Set dissolve override flag
	void SetDissolveOverride( Bool useDissolve );

	//! Set use shadow distances
	virtual void SetUseShadowDistances( Bool enable );

	const Float* GetNormalBlendAreas() const { return m_normalBlendAreas; }
	const Float * GetNormalBlendWeights() const { return m_normalBlendWeights; }
	
	virtual void CreateDynamicDecalChunks( CRenderDynamicDecal* decal, DynamicDecalChunkList& outChunks );

	//! Get mesh proxy for hires shadows
	virtual CRenderProxy_Mesh* AsMeshProxy() override { return this; }

	virtual IRenderProxyBase* AsBaseProxy() { return this; }

	//! Is character shadow fallback
	virtual Bool IsCharacterShadowFallback() const;

public:
	//! Is this drawable allowed to be rendered into envProbes
	virtual Bool IsAllowedForEnvProbes() const;

public:
	//! Attach to scene
	virtual void AttachToScene( CRenderSceneEx* scene );

	//! Detach from scene
	virtual void DetachFromScene( CRenderSceneEx* scene );

protected:
	RED_INLINE Bool IsElementMasked( const CRenderElement_MeshChunk* chunk ) const { return chunk->HasFlag( RMCF_IsMaskedMaterial ) || HasClippingEllipse(); }

	//! Push given lod group elements
	void PushLodGroupElements( CRenderCollector& collector, Int32 lodGroupIndex, const Bool isDissolved, const Bool useMergedGeometry, const Uint8 renderMask );

	//! Collect forced shadows for cascades
	void CollectForcedCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults );
		
public:
	//! Determine whether a mesh chunk should be collected. collector can be NULL in certain shadow map passes.
	virtual Bool ShouldCollectElement( CRenderCollector* collector, const Bool isShadow, const CRenderElement_MeshChunk* chunk ) const;

	//! Element was collected
	virtual void OnCollectElement( CRenderCollector* collector, Bool isShadow, CRenderElement_MeshChunk* chunk );

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	virtual void SetClippingEllipseMatrix( const Matrix& localToEllipse );

	virtual void ClearClippingEllipse();

protected:
	/// Chain for update function that should happen only once per frame
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );

	/// Update internal LOD calculations
	void UpdateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame );

	/// Update internal fade stuff
	void UpdateFades( const CRenderCollector& collector, const Bool wasVisibleLastFrame );
	
	//! Set mesh proxy flag
	void SetMeshProxyFlag( ERenderMeshProxyFlags flag, Bool flagValue = true );

	//! Clear mesh proxy flag
	void ClearMeshProxyFlag( ERenderMeshProxyFlags flag );

	void RefreshInstancingFlag();

protected:
	static const CMeshTypeResource* ExtractMeshTypeResource( const RenderProxyInitInfo& initInfo );
	static Int32 ExtractForceLODLevel( const RenderProxyInitInfo& initInfo );
	static Bool ExtractForceNoAutohide( const RenderProxyInitInfo& initInfo );
	static Float ExtractAutohideDistance( const RenderProxyInitInfo& initInfo );
	static Bool ExtractCastShadowsEvenIfNotVisible( const RenderProxyInitInfo& initInfo );
};
