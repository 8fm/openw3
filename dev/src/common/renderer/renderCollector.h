/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderElement.h"
#include "renderElementMeshChunk.h"
#include "renderElementApex.h"
#include "renderHelpers.h"
#include "shadowCascade.h"
#include "renderOcclusion.h"
#include "renderProxyDecal.h"
#include "renderVisibilityExclusionMap.h"
#include "renderDissolve.h"
#include "meshDrawingStats.h"

#include "../engine/renderFrame.h"
#include "../engine/drawableComponent.h"
#include "../engine/renderFrame.h"

class IRenderProxyBase;
class IRenderElement;
class CRenderProxy_Dimmer;
class IRenderProxyLight;

class CRenderProxy_Flare;
class CRenderProxy_Particles;
class CRenderProxy_TerrainChunk;
class CRenderProxy_Apex;
class CRenderProxy_Decal;
class CRenderProxy_Fur;
class CRenderElement_MeshChunk;
class CRenderElement_Apex;
class CRenderSkinningData;
class CRenderSpotShadowDrawer;
class CRenderEntityGroup;
class CRenderElement_ParticleEmitter;
class CRenderCollectorData;
class CRenderDynamicDecalChunk;
class CRenderLodBudgetSystem;
class CRenderDistantLightBatcher;


// Bitfield of containers, used for selecting which elements to draw
enum ERenderElementContainerGroup : Uint8
{
	RECG_SolidMesh			= FLAG( 0 ),
	RECG_SolidStaticMesh	= FLAG( 1 ),
	RECG_DiscardMesh		= FLAG( 2 ),
	RECG_Swarm				= FLAG( 3 ),
	RECG_Particle			= FLAG( 4 ),
	RECG_Apex				= FLAG( 5 ),

	RECG_ALL				= 0xff
};


struct SSpeedtreeWindParams
{
	Vector params1;
	Vector params2;
	Vector params3;
	Vector params4;
	Vector params5;
	Vector windDir;
	Vector params6;
	Vector params7;
};

/// Collector of elements for rendering
class CRenderCollector
{
protected:
	class CShadowCullingTask : public CTask
	{
	private:
		CRenderCollector* m_collector;

	public:
		CShadowCullingTask( CRenderCollector* collector )
		: m_collector( collector )
		{
		}

		virtual ~CShadowCullingTask();

	protected:
		virtual void Run();

#ifndef NO_DEBUG_PAGES
	public:
		//! Get short debug info
		virtual const Char* GetDebugName() const { return TXT("Shadow culling"); }

		//! Get debug color
		virtual Uint32 GetDebugColor() const override { return Color::BROWN.ToUint32(); }
#endif
	
	};

	friend CShadowCullingTask;


	class CSceneCullingTask : public CTask
	{
	public:
		enum TaskPhase
		{
			PHASE_NotStarted,
			PHASE_OcclusionQuery,
			PHASE_CollectStaticMeshes,
			PHASE_CollectScene,
			PHASE_BuildDynamicShadowElements,
			PHASE_Finished
		};

	private:
		CRenderCollector*				m_collector;
		Red::Threads::CAtomic< Int32 >	m_currentPhase;				// Would be Int8 or something, but it's atomic anyways.

		Red::Threads::CMutex			m_runningMutex;				// Lock task access, so we can run locally if needed.

	public:
		CSceneCullingTask( CRenderCollector* collector )
			: m_collector( collector )
			, m_currentPhase( PHASE_NotStarted )
		{
		}

		virtual ~CSceneCullingTask();

		//! Run up to and including the given phase on the current thread. If this task is already running, this will just wait
		//! until the task reaches the phase. If the task is not running yet, runs to the phase on the local thread, and when the
		//! task actually starts, it will pick up from there.
		void RunToOrWait( TaskPhase phase );

	protected:
		virtual void Run();

#ifndef NO_DEBUG_PAGES
	public:
		//! Get short debug info
		virtual const Char* GetDebugName() const { return TXT("Scene culling"); }

		//! Get debug color
		virtual Uint32 GetDebugColor() const override { return Color::BROWN.ToUint32(); }
#endif

	private:
		//! Run the current phase, and advance to the next. Does not lock m_runningMutex, assumes the caller has it.
		void RunCurrentPhaseAndAdvance();
	};

	friend CSceneCullingTask;

public:
	/// Collector of stuff for hires shadows
	struct HiResShadowsCollector
	{
		TDynArray< CRenderElement_MeshChunk* >		m_receiverSolidElements;		//!< Render elements that are solid ( faster pipe )
		TDynArray< CRenderElement_MeshChunk* >		m_receiverDiscardElements;		//!< Render elements that have discards
		TDynArray< CRenderElement_MeshChunk* >		m_casterSolidElements;			//!< Render elements that are solid ( faster pipe )
		TDynArray< CRenderElement_MeshChunk* >		m_casterDiscardElements;		//!< Render elements that have discards
		
		RED_INLINE HiResShadowsCollector()
		{}

		RED_INLINE void Reset( )
		{
			m_receiverSolidElements.ClearFast();
			m_receiverDiscardElements.ClearFast();
			m_casterSolidElements.ClearFast();
			m_casterDiscardElements.ClearFast();
		}

		//! Has any receiver
		RED_INLINE Bool HasAnyReceiver() const
		{
			return !m_receiverSolidElements.Empty() || !m_receiverDiscardElements.Empty();
		}

		//! Has any caster
		RED_INLINE Bool HasAnyCaster() const
		{
			return !m_casterSolidElements.Empty() || !m_casterDiscardElements.Empty();
		}

		//! Push element
		RED_INLINE void PushMeshChunk( CRenderElement_MeshChunk* element, Bool hasDiscards, Bool isHair )
		{
			Bool isCasterOnly = false;

			if ( RSG_EyeOverlay == element->GetSortGroup() )
			{
				return;
			}	
			else if ( isHair )
			{
				isCasterOnly = true;				
			}
			else
			{
				isCasterOnly = false;				
			}

			Bool isReceiver = !isCasterOnly;
			Bool isCaster = element->GetProxy()->CanCastShadows();

			if ( isCaster )
			{
				if ( hasDiscards )
				{
					m_casterDiscardElements.PushBack( element );
				}
				else
				{
					m_casterSolidElements.PushBack( element );
				}
			}

			if ( isReceiver )
			{
				if ( hasDiscards )
				{
					m_receiverDiscardElements.PushBack( element );
				}
				else
				{
					m_receiverSolidElements.PushBack( element );
				}
			}
		}
	};

	struct SRenderElementContainer
	{
		TDynArray< CRenderElement_MeshChunk* >					m_solidStaticMeshes;
		TDynArray< CRenderElement_MeshChunk* >					m_solidMeshes;
		TDynArray< CRenderElement_MeshChunk* >					m_discardMeshes;
		TDynArray< CRenderElement_MeshChunk* >					m_swarmMeshes;
		TDynArray< CRenderElement_ParticleEmitter* >			m_particles;
		TDynArray< CRenderElement_Apex* >						m_apex;

		SRenderElementContainer()
		{
			m_solidMeshes.Reserve( 1024 );
			m_solidStaticMeshes.Reserve( 1024 );
			m_discardMeshes.Reserve( 1024 );
			m_swarmMeshes.Reserve( 1024 );
			m_particles.Reserve( 1024 );
			m_apex.Reserve( 1024 );
		}

		//! Reset the internal list
		void Reset()
		{
			m_solidMeshes.ClearFast();
			m_solidStaticMeshes.ClearFast();
			m_discardMeshes.ClearFast();
			m_swarmMeshes.ClearFast();
			m_particles.ClearFast();
			m_apex.ClearFast();
		}
		
		//! Is the element container empty ?
		Bool Empty() const
		{
			return m_solidMeshes.Empty() && m_solidStaticMeshes.Empty() && m_discardMeshes.Empty() && m_swarmMeshes.Empty() && m_particles.Empty() && m_apex.Empty();
		}

		//! Draw elements from this container
		//! groups is combination of ERenderElementContainerGroup, saying which element types to draw.
		void Draw( const CRenderCollector &collector, const RenderingContext &context, Uint8 groups ) const;


		//! Push mesh chunk into this container
		void PushMeshChunk( CRenderElement_MeshChunk* chunk );

		//! Push element into this container
		RED_INLINE void Push( IRenderElement* element )
		{
			if ( element->GetType() == RET_MeshChunk )
			{
				PushMeshChunk( (CRenderElement_MeshChunk*)  element );
			}			
			else if ( element->GetType() == RET_ParticlesEmitter )
			{
				m_particles.PushBack( (CRenderElement_ParticleEmitter*) element );
			}
			else if ( element->GetType() == RET_Apex )
			{
				m_apex.PushBack( (CRenderElement_Apex*) element );
			}
			else if ( element->GetType() == RET_Swarm )
			{
				m_swarmMeshes.PushBack( (CRenderElement_MeshChunk*) element );
			}
		}
		
	};

	struct SRenderCollectorElements
	{
		SRenderElementContainer				m_elements[ RSG_Max ];				//!< Elements to draw for each sort group

		void Reset()
		{
			for ( Uint32 i = 0; i < ARRAY_COUNT(m_elements); ++i )
			{
				m_elements[i].Reset();
			}
		}
	};

	/// Data for render collector
	class CRenderCollectorData
	{
	public:
		SRenderCollectorElements						m_elements[RPl_Max];				//!< Elements to draw for each drawing layer
		SRenderElementContainer							m_accumulativeRefractionElements;	//!< Elements for accumulative refractions
		SRenderElementContainer							m_reflectiveMaskedElements;			//!< Elements for reflection mask
		SRenderElementContainer							m_foregroundElements;				//!< Foreground elements (no overlay)
		TDynArray< CRenderProxy_Dimmer* >				m_dimmers;							//!< Dimmers to render
		TDynArray< IRenderProxyLight* >					m_lights;							//!< Lights to render
		TDynArray< IRenderProxyLight* >					m_distantLights;					//!< Distance lights to render
		TDynArray< CRenderElement_ParticleEmitter* >	m_particles[SCREENCAT_Max];			//!< Particles separated into OnScreen & OffScreen categories
		TDynArray< CRenderProxy_Flare* >				m_flaresByCategory[FLARECAT_MAX];	//!< Flare proxies to render
		TDynArray< CRenderProxy_Decal* >				m_decals[EDDI_MAX];					//!< Decals to render
		TDynArray< CRenderProxy_Fur* >					m_furProxies;						//!< Fur
		TDynArray< CRenderProxy_Fur* >					m_furShadowProxies;					//!< Fur
		SShadowCascade									m_cubeFaceFragments;				//!< Static cube face fragments
		TDynArray< IRenderProxyLight* >					m_lightsTemp;						//!< Lights to render
		HiResShadowsCollector							m_hiResShadowsCollector;			//!< Collector for hiResShadows
		TDynArray< CRenderDynamicDecalChunk* >			m_dynamicDecalChunks;				//!< Dynamic decal chunks

	public:
		CRenderCollectorData();
		~CRenderCollectorData();
		void Reset();
	};

public:
	CRenderFrame*								m_frame;								//!< Low level frame with dynamic fragments
	CRenderSceneEx*								m_scene;								//!< Collected scene
	const CRenderFrameInfo*						m_info;									//!< The frame info
	const CRenderFrameOverlayInfo*				m_overlayInfo;							//!< The frame overlay info
	const CRenderCamera*						m_lodCamera;							//!< The camera for calculating LOD
	const CRenderCamera*						m_camera;								//!< The camera
	SSpeedtreeWindParams						m_speedtreeWindData;					//!< Speedtree wind data
	Uint32										m_frameIndex;							//!< Current frame index
	CRenderCollectorData*						m_renderCollectorData;					//!< Shared once allocated render collector data
	Float										m_terrainErrorMetric;					//!< Calculated terrain error metric
	Float										m_terrainHoleDistance;					//!< Distance at which the holes in terrain disappear
	EPostProcessCategoryFlags					m_postProcess;							//!< Post process visibility flags
	Bool										m_renderFoliage;						//!< Yes, we want to collect foliage crap
	Bool										m_renderTerrain;						//!< Yes, we want to collect terrain
	Bool										m_renderSunLighting;					//!< Yes, we want the sun ( and sun shadows, etc )
	Bool										m_renderSky;							//!< Yes, we want the sky
	Int32										m_numberOfMeshLodsToDrop;				//!< Number of lods to drop on gameplay
	Int32										m_numberOfCharacterLodsToDrop;			//!< Number of character lods to drop on gameplay
	CRenderVisibilityExclusionTester			m_visibilityFilter;						//!< Simple visibility filter
	CRenderDissolveSynchronizer					m_dissolveSynchronizer;					//!< Dissolve data synchronizer
	Float										m_cascadeShadowSizeLimit;				//!< Abstract treshold for shadow size culling
	SMergedShadowCascades						m_cascades;								//!< Shadow cascades data
	Bool										m_renderShadows;						//!< We should process and render shadows from lights

	Bool										m_disolveFlushed;						//!< Dissolves were flushed (TEMSHIT FLAG - it's here until we rewrite the rest of the pipeline to be more stateless)

	Uint8										m_usedLightChannels;					//!< Mask all elements that were collected

	// hi res shadows for actor
	CRenderEntityGroup*							m_hiResShadowLists;						//!< List of entity groups that want the hi res shadows

	CShadowCullingTask*							m_shadowCullingTask;
	CSceneCullingTask*							m_sceneCullingTask;

	mutable MeshDrawingStats					m_sceneStats;							//!< Mesh stats for scene

#ifdef USE_UMBRA
	Bool										m_collectWithUmbra;
	CRenderOcclusionData*						m_occlusionData;
#endif // USE_UMBRA

#ifndef RED_FINAL_BUILD
	Uint32										m_collectedMeshes;
#endif // RED_FINAL_BUILD

public:
	//! Get the scene being collected
	RED_INLINE const CRenderSceneEx* GetScene() const { return m_scene; }

	//! Get calculated terrain error metric
	RED_INLINE const Float GetTerrainErrorMetric() const { return m_terrainErrorMetric; }

	//! Get the terrain hole distance limit
	RED_INLINE const Float GetTerrainHoleDistanceLimit() const { return m_terrainHoleDistance; }

	//! Get the shadow size limit ( fade treshold )
	RED_INLINE const Float GetCascadeShadowSizeLimit() const { return m_cascadeShadowSizeLimit; }

	//! Is rendering of the sun lighting enabled ?
	RED_INLINE Bool IsRenderingSunLightingEnabled() const { return m_renderSunLighting; }

	//! Get visibility filter
	RED_INLINE const CRenderVisibilityExclusionTester& GetVisibilityFilter() const { return m_visibilityFilter; }

	//! Get the dissolve effect synchronizer
	RED_INLINE const CRenderDissolveSynchronizer& GetDissolveSynchronizer() const { return m_dissolveSynchronizer; }

#ifdef USE_UMBRA
	const CRenderOcclusionData& GetOcclusionData() const;
#endif

public:
	CRenderCollector();
	~CRenderCollector();

	void Setup( CRenderFrame* frame, CRenderSceneEx* scene, CRenderCollectorData* data, Bool supressSceneRendering = false );
	void Reset();

	//! Grab flares transparency helpers
	void GrabFlaresTransparencyHelpers( const CRenderFrameInfo &info, const GpuApi::TextureRef &texGrabTarget, const GpuApi::TextureRef &texGrabSource );

	//! Update sky flares
	void UpdateSkyFlaresOcclusion( const GpuApi::TextureRef &texTransparencyHelperOpaque, const GpuApi::TextureRef &texTransparencyHelperTransparent );

	//! Update non sky flares
	void UpdateNonSkyFlaresOcclusion();

	//! Draw flares debug occlusion shapes
	void DrawFlaresDebugOcclusionShapes();

	//! Render flares
	void RenderFlares( const class RenderingContext& context, Bool allowCenterFlares, Bool allowLensFlares );

	//! Render elements of given sort group from back plane to front plane
	void RenderElementsAllPlanesBackFirst( enum ERenderingSortGroup sortGroup, const class RenderingContext& context, Uint8 containerGroups );

	//! Render elements of given sort group from front plane to back plane
	void RenderElementsAllPlanesFrontFirst( enum ERenderingSortGroup sortGroup, const class RenderingContext& context, Uint8 containerGroups );

	//! Render elements from given list of sort groups from back plane to front plane
	void RenderElementsAllPlanesBackFirst( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context, Uint8 containerGroups );

	//! Render elements from given list of sort groups from front plane to back plane
	void RenderElementsAllPlanesFrontFirst( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context, Uint8 containerGroups );

	//! Render elements of given sort group
	void RenderElements( enum ERenderingSortGroup sortGroup, const class RenderingContext& context, ERenderingPlane renderingPlane, Uint8 containerGroups );

	//! Render elements of given sort groups
	void RenderElements( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context, ERenderingPlane renderingPlane, Uint8 containerGroups );

	//! Render accumulative refraction elements
	void RenderAccumulativeRefractionElements( const class RenderingContext& context );

	//! Render reflective masked elements
	void RenderReflectiveMaskedElements( const class RenderingContext& context );

	//! Render foreground elements
	void RenderForegroundElements( const class RenderingContext& context );

	//! Render dynamic fragments
	void RenderDynamicFragments( enum ERenderingSortGroup sortGroup, const class RenderingContext& context );

	//! Render dynamic fragments
	void RenderDynamicFragments( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context );
	
	//! Render the entity groups
	void RenderHiResEntityShadows( const GpuApi::RenderTargetSetup& rtMainSetup );

	//dex++: Calculate shadow cascades frustums and other data ( do not collect elements )
	void CalculateCascadesData( const CCascadeShadowResources &cascadeShadowResources );
	//dex--

public:
	//! Is any accumulative refraction element present in collector
	Bool HasAnyAccumulativeRefractionElements() const;

	//! Is any reflective masked element present in collector
	Bool HasAnyReflectiveMaskedElements() const;

	//! Is any foreground element present in collector
	Bool HasAnyForegroundElements() const;

	//! Is any element present for given sort group
	Bool HasAnySortGroupElements( ERenderingSortGroup group ) const;

	//! Is any element present for any given sort group
	Bool HasAnySortGroupElements( Uint32 numGroups, const ERenderingSortGroup *groups ) const;

	//! Is any element present for given sort group at given rendering plane
	Bool HasAnySortGroupElements( ERenderingSortGroup group, ERenderingPlane renderingPlane ) const;

	//! Is any element present for any given sort group at given rendering plane
	Bool HasAnySortGroupElements( Uint32 numGroups, const ERenderingSortGroup *groups, ERenderingPlane renderingPlane ) const;

	//! Is any light element that need to be injected to bloom
	Bool HasAnyDistantLights( ) const;

	//! Is there any decals 
	Bool HasAnyDecals() const;

	//! Is there any decals on requested render type
	Bool HasAnyDecalsType( enum EDynamicDecalRenderIndex type ) const;

	//! Is collector was generated from main world scene ?
	Bool IsWorldScene() const;

	//!
	RED_INLINE Bool HasCollectedLightChannel( Uint8 mask ) const { return ( m_usedLightChannels & mask ) != 0; };

protected:
	//! Collect visible elements
	void RunSceneQuery();

	//! Collect visible elements
	void BuildSceneStatics();
	void BuildSceneElements();

	//! Update shadow casting proxies in this frame (both static and dynamic)
	void BuildDynamicShadowElements();

	void BuildCascadesElements();

	void NotifyProxyOfCollectedElement( IRenderElement* element );
	
public:
	// collects element into appropriate bucket
	void CollectElement( IRenderProxyBase* proxy );

	void ProcessAutoHideProxy( IRenderProxyBase* proxy );

	// Push element.
	Bool PushElement( IRenderElement* element );

	Bool PushDecal( CRenderProxy_Decal* element );

	Bool PushDynamicDecalChunk( CRenderDynamicDecalChunk* decalChunk );

	void PushOnScreenParticle( CRenderProxy_Particles* proxy );

	void PushOffScreenParticle( CRenderProxy_Particles* proxy );

	// Push foreground element. Will notify proxy (OnCollectElement for mesh/apex proxies) if occlusion passes.
	Bool PushForegroundElement( IRenderElement* element );

	void RenderDecals( enum EDynamicDecalRenderIndex type, const class RenderingContext& context );

	void RenderDynamicDecals( const class RenderingContext& context );
	void RenderStripes( const class RenderingContext& context, Bool projected );

	void RenderDistantLights( CRenderDistantLightBatcher& batcher, Float intensityScale );

#ifdef USE_NVIDIA_FUR
	void RenderFur( const class RenderingContext& context );
	void UpdateFurSkinning(); 
#endif

public:
	void FinishShadowCulling();

	void FinishSceneOcclusionQuery();
	void FinishSceneStaticsCulling();
	void FinishSceneCulling();
	void FinishDynamicShadowsCollection();
	void FinishSceneCullingTask();

	const CRenderFrameInfo & GetRenderFrameInfo() const { return *m_info; }
	const CRenderFrameOverlayInfo & GetRenderFrameOverlayInfo() const { return *m_overlayInfo; }
	const CRenderCamera & GetRenderCamera() const { return *m_camera; }
	const CRenderCamera & GetLodRenderCamera() const { return *m_lodCamera; }

	Bool WasLastFrameCameraInvalidated() const;
};
