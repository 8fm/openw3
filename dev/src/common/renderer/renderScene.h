/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderTerrainLogicMap.h"
#include "renderOcclusion.h"
#include "renderProxyDecal.h"
#include "renderDissolve.h"
#include "renderCollectorTickList.h"
#include "renderProxyFadeable.h"
#include "meshDrawingStats.h"
#include "../engine/lensFlareSetupParameters.h"
#include "../engine/umbraQuery.h"
#include "../engine/renderProxy.h"
#include "../engine/umbraScene.h"

class CRenderElementMap;
class IRenderProxyLight;
class IRenderVisibilityExclusion;
class CRenderProxy_Dimmer;
class IRenderProxyDrawable;
class CRenderShadowCollector;
class CRenderProxy_Flare;
class CRenderProxy_Particles;
class CRenderProxy_SpeedTree;
class CRenderProxy_Terrain;
class CRenderProxy_Water;
class CRenderProxy_Apex;
class CRenderProxy_Decal;
class CRenderProxy_Stripe;
class CRenderProxy_Fur;
class CRenderProxy_Mesh;
class CRenderDynamicDecal;
class CRenderVisibilityQueryManager;
class CRenderSkybox;
class CRenderTerrainShadows;
class CRenderVisibilityExclusionMap;
class CRenderProxyObjectGroup;
struct SShadowCascade;

///////////////////////////////////////////////////////////////////////////////

/// Collected envprobe light
struct SCollectedEnvProbeLight
{
	Float m_sortKey;
	IRenderProxyLight *m_light;
};

/// Envprobe light info
struct SSceneEnvProbeLightInfo
{
	SSceneEnvProbeLightInfo ()
		: m_posAndRadius( 0, 0, 0, 0 )
		, m_light( 0 )
	{}

	SSceneEnvProbeLightInfo ( const Vector &pos, Float radius, IRenderProxyLight *light )
		: m_light( light )
	{
		SetPosAndRadius( pos, radius );
	}

	RED_INLINE SSceneEnvProbeLightInfo& SetPosAndRadius( const Vector &pos, Float radius )
	{
		m_posAndRadius.Set4( pos.X, pos.Y, pos.Z, radius );
		return *this;
	}

	Vector m_posAndRadius;
	IRenderProxyLight* m_light;
};


struct SQueuedDecalSpawn
{
	CRenderDynamicDecal*				m_decal;
	TDynArray< IRenderProxyDrawable* >	m_targets;
	Bool								m_staticOnly;

	SQueuedDecalSpawn() : m_decal( nullptr ) {}

	// SQueuedDecalSpawn doesn't do automatic reference tracking, but this is a little helper to just release
	// the decal and all targets.
	void Reset();
};


/// Rendering scene
class CRenderSceneEx : public IRenderScene
{
	friend class CRenderInterface;

	enum ECoherentFrameType
	{
		ECFT_UpdateSkinningData = 0,
		ECFT_Max
	};

protected:
	TDynArray< SSceneEnvProbeLightInfo >	m_envProbeLights;					//!< Registered envprobe lights
	TDynArray< IRenderProxyDrawable* >		m_backgroundDrawables;				//!< Overlay drawables
	TDynArray< IRenderProxyFadeable* >		m_fadeablesToRemove;				//!< Drawables waiting for removal
	TDynArray< CRenderProxy_Flare* >		m_activeFlares;						//!< Active flares
	CRenderProxy_Terrain*					m_terrain;							//!< Terrain proxy
	CRenderProxy_Water*						m_water;							//!< Water proxy
	CRenderSkybox*							m_skybox;							//!< Skybox
	SLensFlareGroupsSetupParameters			m_lensFlareGroupsParams;			//!< Lens flare groups
	TDynArray< GpuApi::TextureRef >			m_cameraInteriorStagingTextures;	//!< Camera interior value staging textures
	Uint32									m_cameraInteriorNumValuesCopied;	//!< Number of camera interior values copied
	Float									m_cameraInteriorFactor;				//!< Currect camera interior factor
	CRenderDissolveSynchronizer				m_dissolveSynchronizer;				//!< Dissolve synchronizer

#ifdef USE_SPEED_TREE	
	CRenderProxy_SpeedTree*					m_speedTree;						//!< Foliage definitions implemented through speed tree
#endif
	TDynArray< IRenderProxyBase* >			m_proxiesWithVisibilityQueries;		//!< Visibility queries
	TDynArray< CRenderProxy_Stripe* >		m_stripeProxies;					//!< Stripes
	TDynArray< CRenderProxy_Fur* >			m_furProxies;						//!< Simulated fur
	Int32									m_frameCounter;						//!< Scene frame counter for marking cache, used for stuff that should be done once per multiple renders of the frame (e.g. dissolve update etc)
	Int32									m_repeatedFrameCounter;				//!< Scene frame counter for marking cache, used for stuff that are meant to be done for each of multiple render of the frame (e.g. hires shadows collection must be done for each of ubersample subframes)
	Bool									m_updateTerrainLOD;					//!< Reinitialize terrain LOD settings
	CRenderTerrainLogicMap					m_terrainLogicMap;					//!< Terrain logic grid map
	Int32									m_forcedLOD;						//!< Force LOD for meshes
	SceneRenderingStats						m_renderingStats;					//!< Rendering stats
	CRenderTerrainShadows*					m_terrainShadows;					//!< terrain shadows system

#ifdef USE_UMBRA
	CRenderOcclusionData*					m_occlusionData;
	Bool									m_isTomeCollectionValid;
	Uint8*									m_occlusionQueryAdditionalMemory;
	MemSize									m_occlusionQueryAdditionalMemorySize;
#endif // USE_UMBRA

	CRenderElementMap*						m_renderElementMap;
	IRenderFramePrefetch*					m_currentFramePrefetch;

	TDynArray< CRenderDynamicDecal* >		m_dynamicDecals;					//!< Dynamic decals
	TDynArray< Uint8 >						m_dynamicDecalsChunkUsage;			//!< How much of the chunk budget each decal is using.
	TDynArray< CRenderDynamicDecal* >		m_removedDynamicDecals;				//!< Dynamic decals that were removed during rendering.
	TDynArray< CRenderDynamicDecal* >		m_removedDynamicDecalChunks;		//!< Dynamic decals that had chunks removed during rendering.
	TDynArray< SQueuedDecalSpawn >			m_dynamicDecalsToSpawn;				//!< Dynamic decals queued to be spawned after simulation of the particles
	Red::Threads::CAtomic< Int32 >			m_dynamicDecalChunkBudgetUsage;		//!< Track how much of the dynamic decal (with chunks) budget is currently used.
	Red::Threads::CAtomic< Int32 >			m_dynamicDecalBudgetUsage;			//!< Track how much of the dynamic decal budget is currently used.

	Bool									m_isWorldScene;

	CRenderVisibilityExclusionMap*			m_visibilityExclusionMap;			//!< Visibility exclusion map
	CRenderVisibilityQueryManager*			m_visibilityQueryManager;			//!< Visibility query manager

	Bool									m_isRendering;						//!< Is this scene currently being rendered?

	Bool									m_fadeFinishRequested;				//!< We requested all of the fade

	THashMap< Uint64, CRenderProxyObjectGroup* >	m_renderingGroups;					//!< Render groups

	CRenderCollectorTickList				m_tickList;

	SCoherentFrameTracker					m_coherentFrameTrackers[ ECFT_Max ];

public:
	//! Get terrain logic map
	RED_INLINE CRenderTerrainLogicMap& GetTerrainLogicMap() { return m_terrainLogicMap; }

	//! Get force LOD level
	RED_INLINE Int32 GetForcedLOD() const { return m_forcedLOD; }

	//! Get current render stats
	RED_INLINE SceneRenderingStats GetRenderStats() const { return m_renderingStats; }

	//! Is this a world scene
	RED_INLINE Bool IsWorldScene() const { return m_isWorldScene; }

#ifdef USE_UMBRA
	RED_INLINE void SetTomeCollectionValid( Bool isValid ) { m_isTomeCollectionValid = isValid; }
	RED_INLINE Bool IsTomeCollectionValid() const { return m_isTomeCollectionValid; }
	RED_INLINE CRenderOcclusionData* GetOcclusionData() const		{ return m_occlusionData; }
	RED_INLINE Uint8* GetQueryAdditionalMemory() const				{ return m_occlusionQueryAdditionalMemory; }
	RED_INLINE MemSize GetQueryAdditionalMemorySize() const			{ return m_occlusionQueryAdditionalMemorySize; }
	
	RED_INLINE Bool ShouldCollectWithUmbra() const { return CUmbraScene::IsUsingOcclusionCulling() && m_occlusionData != nullptr && IsTomeCollectionValid(); }
#else
	RED_INLINE Bool ShouldCollectWithUmbra() const { return false; }
#endif // USE_UMBRA

	RED_INLINE void SetCurrentFramePrefetch( IRenderFramePrefetch* currentFramePrefetch ) { m_currentFramePrefetch = currentFramePrefetch; }

	CRenderProxy_Terrain* GetTerrain() const { return m_terrain; }
	CRenderProxy_SpeedTree* GetSpeedTree() const { return m_speedTree; }

	RED_INLINE CRenderElementMap* GetRenderElementMap() const { return m_renderElementMap; }

	RED_FORCE_INLINE CRenderVisibilityQueryManager* GetVisibilityQueryManager() const { return m_visibilityQueryManager; }

	RED_FORCE_INLINE CRenderTerrainShadows* GetTerrainShadows() const { return m_terrainShadows; }

	RED_INLINE const TDynArray< CRenderProxy_Stripe* >& GetStripeProxies() const { return m_stripeProxies; }

	RED_INLINE const CRenderDissolveSynchronizer GetDissolveSynchronizer() const { return m_dissolveSynchronizer; }

	/// Get visibility exclusion map
	RED_INLINE CRenderVisibilityExclusionMap* GetVisibilityExclusionMap() const { return m_visibilityExclusionMap; }

public:
	CRenderSceneEx( Bool isWorldScene );
	~CRenderSceneEx();

	void BeginRendering();
	void EndRendering();
	RED_INLINE Bool IsRendering() const { return m_isRendering; }
	RED_INLINE Uint32 GetRepeatedFrameCounter() const { return m_repeatedFrameCounter; }

	//! Allocate frame for collecting, invalidates things cached for previous frame
	void AllocateFrame();

	//! Add proxy to scene
	void AddProxy( IRenderProxyBase* proxy );

	//! Remove proxy from scene
	void RemoveProxy( IRenderProxyBase* proxy );

	//! Add stripe to scene
	void AddStripe( IRenderProxy* proxy );

	//! Remove stripe from scene
	void RemoveStripe( IRenderProxy* proxy );

	//! Add fur proxy
	void AddFur( IRenderProxy* proxy );

	//! Remove screen space decal proxy from scene
	void RemoveFur( IRenderProxy* proxy );

	//! Add speed tree proxy to scene
	void AddSpeedTreeProxy( IRenderObject* proxy );

	//! Remove speed tree proxy from scene
	void RemoveSpeedTreeProxy( IRenderObject* proxy );

	//! Set terrain proxy to scene
	void SetTerrainProxy( IRenderProxy* proxy );

	//! Remove terrain proxy from scene
	void RemoveTerrainProxy( IRenderProxy* proxy );

	//! Set water proxy to scene
	void SetWaterProxy( IRenderProxy* proxy );

	//! Remove water proxy from scene
	void RemoveWaterProxy( IRenderProxy* proxy );

	//! Add visibility exclusion list
	void AddVisibilityExclusionObject( IRenderVisibilityExclusion* object );

	//! Remove visibility exclusion object
	void RemoveVisibilityExclusionObject( IRenderVisibilityExclusion* object );

	//! Toggle visibility list
	void RefreshVisibilityExclusionObject( IRenderVisibilityExclusion* object );

	//! Retrieve water proxy from scene
	RED_INLINE CRenderProxy_Water* GetWaterProxy(){ return m_water; }

	//! Get skybox
	RED_INLINE CRenderSkybox* GetSkybox() const { return m_skybox; }

	//! Get lensflare parameters
	void SetLensFlareGroupsParameters( const SLensFlareGroupsSetupParameters &params );
	const SLensFlareGroupsSetupParameters& GetLensFlareGroupsParameters() const { return m_lensFlareGroupsParams; }

	//! Enqueue a dynamic decal for spawning in the scene. Screen-space decal will be created for the decal, and optionally
	//! all dynamic objects near the decal can generate chunks. Adds reference to the decal, caller can Release if it wants.
	void QueueDynamicDecalSpawn( IRenderResource* decal, Bool projectOnlyOnStatic = false );

	//! Enqueue a dynamic decal for spawning. Decal will be applied to only the given set of render proxies. Adds reference
	//! to the decal and all proxies, so caller can Release as appropriate.
	void QueueDynamicDecalSpawn( IRenderResource* decal, const TDynArray< IRenderProxy* >& targetProxies );
	
	//! Spawn queued dynamic decals
	void SpawnQueuedDynamicDecals();

	//! Remove dynamic decal from scene. This may cause the decal to be destroyed immediately, so the caller should assume
	//! it has been destroyed, unless an extra reference was added prior to removing. Any budget consumed by the decal will
	//! be returned immediately.
	void RemoveDynamicDecal( IRenderResource* decal );

	//! Remove dynamic decal chunks from scene. Only has any effect if the decal has been fully spawned (for pending spawns
	//! nothing will be done). The decal itself may remain alive if it also has a screenspace decal. Chunks may be destroyed
	//! immediately, or it may be deferred to a later time. Any budget consumed by the chunks will be returned immediately.
	void RemoveDynamicDecalChunks( IRenderResource* decal );

	//! Get rendering group for given ID, new group is created on demand
	CRenderProxyObjectGroup* GetRenderingGroup( const Uint64 hash );

protected:
#ifndef RED_FINAL_BUILD
	//! Check the decal spawn list and current decals, make sure the given decal isn't there. Halt if it's found and return false.
	Bool EnsureDynamicDecalNotInScene( IRenderResource* decal ) const;
#endif

	//! If a decal is removed during rendering, it's added to a removal list. This is called when rendering ends to destroy
	//! any such decals.
	void FlushRemovedDynamicDecals();

	//! Destroy decals until we are under budget.
	Bool KeepUnderDynamicDecalBudget();
	//! Consume some amount of decal budget, generally when a decal is added.
	void ConsumeDynamicDecalBudget( Int32 chunksAmount, Int32 decalsAmount );
	//! Return some amount of decal budget, generally when a decal is destroyed.
	void ReturnDynamicDecalBudget( Int32 chunksAmount, Int32 decalsAmount );

public:
	//! Setup active flare
	void SetupActiveFlare( CRenderProxy_Flare *flare, Bool setActive );

	//! Remove fading out drawable
	void RegisterFadeOutRemoval( IRenderProxyFadeable* proxy );

	//! Update bounding box/location
	void RelinkProxy( IRenderProxyBase* proxy );

	//! Performs pending fadeout removals
	void RemovePendingFadeOutRemovals( bool forceRemoveAll );

	//! Collect elements in camera to render
	void CollectStaticDrawables( CRenderCollector& collector );
	void CollectDrawables( CRenderCollector& collector );

	//! Collect active flares
	void CollectActiveFlares( CRenderCollector& collector );

	//! Collect background elements
	void CollectBackgroundDrawables( CRenderCollector& collector );

	//! Collect envprobe lights
	void CollectEnvProbeLights( const Vector &collectOrigin, Float collectRadius, TDynArray< SCollectedEnvProbeLight >& lights ) const;

	//! Update envprobe light parameters
	void UpdateEnvProbeLightParameters( IRenderProxyLight &light );

	//! Enable forced LOD
	void SetForcedLOD( Int32 forcedLODLevel );

	//! Update scene stats
	void UpdateSceneStats( const SceneRenderingStats& stats );

	//! Render speed tree stuff
	void RenderSpeedTree( const class RenderingContext& context, const CRenderFrameInfo& frameInfo );

	//! Build quad tree for terrain
	void BuildTerrainQuadTree( const CRenderSceneEx* scene, const CRenderFrameInfo& frameInfo );

	//! Render terrain
	void RenderTerrain( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, MeshDrawingStats& stats );

	//! Render water
	Bool ShouldRenderWater( const CRenderFrameInfo &frameInfo ) const;
	Bool ShouldRenderUnderwater( const CRenderFrameInfo& frameInfo ) const;
	void SimulateWater(const CRenderFrameInfo& frameInfo);
	void RenderWater( const class RenderingContext& context, const CRenderFrameInfo& frameInfo );

	// Prepare shadow data for SpeedTree ( per-cascade culling )
	void PrepareSpeedTreeCascadeShadows( const CRenderCollector& collector );

	// Render SpeedTree into the shadow cascades
	void RenderSpeedTreeCascadeShadows( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, const SShadowCascade* cascade );

	// Finalize dissolves
	void FinishDissolves();

	// Latch the state of the fade finish flag
	const Bool LatchFadeFinishRequested();

public:
	Bool EnsureCameraInteriorHelpersCreated();
	void DestroyCameraInteriorHelpers();
	void UpdateCameraInteriorFactor( GpuApi::TextureRef texValueSource, Bool forceImmediate );
	Float GetDelayedCameraInteriorFactor() const;

public:
	//! Create visibility query wrapper, may return NULL. Note - this is no longer returning an abstract interface
	virtual TRenderVisibilityQueryID CreateVisibilityQuery() override;

	//! Release visibility query wrapper
	virtual void ReleaseVisibilityQuery( const TRenderVisibilityQueryID ) override;

	//! Get the result of a query
	virtual enum ERenderVisibilityResult GetVisibilityQueryResult( const TRenderVisibilityQueryID queryId ) const override;

	virtual Bool GetVisibilityQueryResult( Uint32 elementsCount, Int16* indexes, void* inputPos, void* outputPos, size_t stride ) const override;

public:
	//! Ticks all proxies on tickables list
	void TickProxies( Float timeDelta );

	//! Update active flares
	void UpdateActiveFlares( const CRenderCamera &camera, const CFrustum &frustum, Float timeDelta );

	//! Request a "collected tick" this frame. This must only be called during scene collection (in a proxy's CollectElements() or similar function),
	//! and must be called each time the collected tick is needed (likely every time it's collected). Calling this multiple times in a single frame
	//! (for example, calling in CollectElements() and also CollectCascadeShadowElements()) will not cause multiple collected ticks to occur on a
	//! proxy, so it is safe to call at every collection.
	RED_INLINE void RequestCollectedTickAsync( IRenderProxyBase* proxy ) { m_tickList.PushProxy( proxy ); }

	//! Go through collected tick list, call CollectedTick() on each proxy. The collected tick list will be empty after this call.
	RED_INLINE void TickCollectedProxies( ) { m_tickList.TickCollectedProxies( this ); }

	RED_INLINE void SetTickCollectionEnabled( Bool collectionEnabled ) { m_tickList.SetCollectionEnabled( collectionEnabled ); } 

	RED_INLINE SCoherentFrameTracker& GetCoherentTracker( const ECoherentFrameType type ) { return m_coherentFrameTrackers[type]; }

public:
	//! Get last allocated frame index
	Int32 GetLastAllocatedFrame() const { return m_frameCounter; }

	void RenderStripes( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, CRenderCollector* renderCollector, Bool projected );
#ifdef USE_NVIDIA_FUR
	void RenderFur( const CRenderCollector &collector, const class RenderingContext& context );
	void UpdateFurSkinning(); 
#endif

#ifdef USE_UMBRA
	void UploadOcclusionData( CRenderOcclusionData* occlusionData );
	void UpdateQueryThresholdParameter( Float val );
	void PerformOcclusionQuery( CRenderFrame* frame );
	void PerformVisibilityQueriesBatch( const UmbraQueryBatch& queries );
	void SetDoorState( TObjectIdType objectId, Bool opened );
	void SetCutsceneModeForGates( Bool isCutscene );

#ifndef NO_EDITOR
	void DumpVisibleMeshes( const TLoadedComponentsMap& componentsMap, const String& path );
#endif // NO_EDITOR
#endif // USE_UMBRA
};
