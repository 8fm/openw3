/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "flareParameters.h"
#include "globalLightingTrajectory.h"
#include "envParameters.h"
#include "triggerManager.h"
#include "updateTransformManager.h"
#include "worldShowFlags.h"
#include "scaleable.h"
#include "physicsBatchQueryManager.h"
#include "fxState.h"
#include "persistentEntity.h"
#include "gameSession.h"
#include "manualStreamingHelper.h"
#include "mergedWorldGeometry.h"
#include "../core/resource.h"
#include "../core/events.h"
#include "../core/grid.h"
#include "../core/uniquePtr.h"
#include "../core/sharedPtr.h"
#include "../core/queue.h"
#include "../core/hashset.h"
#include "../core/atomicSharedPtr.h"
#include "characterControllerManager.h"
#include "../core/taskBatch.h"
#include "cubeTexture.h"

class CPhysicsWorld;
class CParticlePool;
class CComponent;
class CWorldTickThread;
class CWorldTickInfo;
class CPathFinder;
class CTickManager;
class CTriggerSystem;
class CTagManager;
class CTerrain;
class CPathEngineWorld;
class CStreamingSystem;
class CSectorDataStreaming;
class CDynamicLayer;
class CEnvironmentManager;
class IUpdateTransformStatCollector;
class WorldAttachedLayerIterator;
class WorldAttachedEntitiesIterator;
class AttachedComponentIterator;
class IDynamicObstaclesFactory;
class CClipMap;
class CPathLibWorld;
class CGlobalWater;
struct TomeData;
class CUmbraScene;
class CWorldBurstLoader;
class CStreamingSectorData;
class CDynamicCollisionCollector;
class CCameraDirector;
class ITriggerManager;
class ITriggerActivator;
class CWorldLookupRegistry;
struct STomeDataGenerationContext;
class CFoliageScene;
class CFoliageEditionController;
class CFoliageDynamicInstanceService;
class IRenderScene;
class CRenderFrameInfo;
class IViewport;
class CMesh;
class CSelectionManager;
class CHitProxyMap;
class EntitySpawnInfo;
class IGameSaver;
class IGameLoader;
class CNodeTransformManager;
class CSectorDataStreamingGCFlusher;
class CSectorPrefetchRuntimeCache;

namespace RedGui
{
	class CRedGuiResourceSystemHeatMapControl;
}

/// Internal world update phase
enum EWorldUpdatePhase
{
	WUP_None,					//!< None, safe to issue most of the functions
	WUP_PartitionTransition,	//!< Transition to other world partition
	WUP_Load,					//!< World is being loaded
	WUP_Unload,					//!< World is being unloaded
	WUP_Tick,					//!< Tick, beware
	WUP_GenerateFrame,			//!< Generating rendering shit
	WUP_DelayedActions,			//!< All delayed actions
	WUP_DelayedAttach,			//!< Delayed attach actions
	WUP_DelayedDetach,			//!< Delayed attach actions
	WUP_DelayedDestroy,			//!< Delayed destruction of entitites	
};

/// World transition mode
enum EWorldTransitionMode : CEnum::TValueType
{
	WTM_Async,							//!< Background transition,
	WTM_LoadingScreen,					//!< Transit with loading screen
	WTM_LoadingScreenWithWaitForKey,	//!< Transit with loading screen and wait for user input
	WTM_LoadingScreenWithWaitForFadeIn,	//!< Transit with loading screen and wait for fade in command
	WTM_SilentBlocking,					//!< Block game but do not display anything
};

BEGIN_ENUM_RTTI( EWorldTransitionMode );
	ENUM_OPTION_DESC( TXT("Stream"), WTM_Async );
	ENUM_OPTION_DESC( TXT("Loading screen"), WTM_LoadingScreen );
	ENUM_OPTION_DESC( TXT("Loading screen till key is pressed"), WTM_LoadingScreenWithWaitForKey );
	ENUM_OPTION_DESC( TXT("Loading screen till fade in"), WTM_LoadingScreenWithWaitForFadeIn );
	ENUM_OPTION_DESC( TXT("Block game but do not display anything"), WTM_SilentBlocking );
END_ENUM_RTTI();

class CWorldLoadProgressInfo
{
public:
	String	m_message;
	Int32		m_progress;
	CWorldLoadProgressInfo( String msg, Int32 progress )
		: m_message( msg )
		, m_progress( progress )
	{

	}
};

/// Filtering of layers to load by terrain
class IWorldLoadingFilter
{
public:
	virtual ~IWorldLoadingFilter() {};
	virtual Bool ShouldLoadLayer( CLayerInfo* layerInfo ) const=0;
};

/// World loading context
class WorldLoadingContext
{
public:
	IWorldLoadingFilter*		m_layerFilter;			//!< Filter for loading layers
	Bool						m_dumpStats;			//!< Dump world loading stats
	Bool						m_useDependencies;		//!< Use dependency mapping for loading
	String						m_partitionToLoad;		//!< World partition to auto load

public:
	WorldLoadingContext();
};

/// Layer group loading context
class LayerGroupLoadingContext
{
public:
	IWorldLoadingFilter*		m_layerFilter;			//!< Filter for loading layers
	Bool						m_dumpStats;			//!< Dump world loading stats
	Bool						m_loadHidden;			//!< Load layers even if they are hidden

public:
	LayerGroupLoadingContext();
};

/// World loading progress
class IWorldLoadingProgress
{
public:
	virtual ~IWorldLoadingProgress() {};
	virtual void AfterCollectingDependencies()=0;
	virtual void AfterPrecachedResource( CResource* res )=0;
	virtual void AfterLoadedLayer( CLayerInfo* info )=0;
	virtual void AfterFinalFlush()=0;
	virtual void AfterAttach()=0;
};

/// World preview data
struct WorldPreviewData
{
	Vector					m_cameraPosition;
	EulerAngles				m_cameraRotation;
	TDynArray< String >		m_layers;

	friend IFile& operator<<( IFile& ar, WorldPreviewData& data )
	{
		ar << data.m_cameraPosition;
		ar << data.m_cameraRotation;
		ar << data.m_layers;
		return ar;
	}
};

// Global world streaming area HACKS (W3)
class CWorldGlobalStreamingArea : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS( CWorldGlobalStreamingArea );

public:
	CWorldGlobalStreamingArea();

private:
	CName						m_areaName;
	TDynArray< String >			m_layerGroupNames;
	Bool						m_isInitiallyVisible;
};

BEGIN_CLASS_RTTI( CWorldGlobalStreamingArea );
	PARENT_CLASS( ISerializable );
	PROPERTY_EDIT( m_areaName, TXT("Name of the streaming area") );
	PROPERTY_CUSTOM_EDIT( m_layerGroupNames, TXT("Affected layer groups"), TXT("") );
	PROPERTY_EDIT( m_isInitiallyVisible, TXT("Are the layer groups initially visible ?") );
END_CLASS_RTTI();

// Global world shadow config ( for cascades )
class CWorldShadowConfig
{
	DECLARE_RTTI_STRUCT( CWorldShadowConfig );

public:
	static Float DefaultHiResShadowBiasOffsetSlopeMul();
	static Float DefaultHiResShadowBiasOffsetConst();
	static Float DefaultHiResShadowTexelRadius();

public:
	Int32			m_numCascades;								//!< Number of cascades
	Float			m_cascadeRange1;							//!< Range at which first cascade ends
	Float			m_cascadeRange2;							//!< Range at which second cascade ends
	Float			m_cascadeRange3;							//!< Range at which third cascade ends
	Float			m_cascadeRange4;							//!< Range at which fourth (last) cascade ends
	Float			m_cascadeFilterSize1;						//!< Filtering size for cascades
	Float			m_cascadeFilterSize2;						//!< Filtering size for cascades
	Float			m_cascadeFilterSize3;						//!< Filtering size for cascades
	Float			m_cascadeFilterSize4;						//!< Filtering size for cascades
	Float			m_shadowEdgeFade1;							//!< Size of the cascade edge fade [m]
	Float			m_shadowEdgeFade2;							//!< Size of the cascade edge fade [m]
	Float			m_shadowEdgeFade3;							//!< Size of the cascade edge fade [m]
	Float			m_shadowEdgeFade4;							//!< Size of the cascade edge fade [m]
	Float			m_shadowBiasOffsetSlopeMul;					//!< Shadow slope offset multiplier
	Float			m_shadowBiasOffsetConst;					//!< Shadow const bias
	Float			m_shadowBiasOffsetConstCascade1;			//!< Shadow const bias cascade 1
	Float			m_shadowBiasOffsetConstCascade2;			//!< Shadow const bias cascade 2
	Float			m_shadowBiasOffsetConstCascade3;			//!< Shadow const bias cascade 3
	Float			m_shadowBiasOffsetConstCascade4;			//!< Shadow const bias cascade 4
	Float			m_shadowBiasCascadeMultiplier;				//!< Shadow bias multiplier (per cascade)
	Float			m_speedTreeShadowFilterSize1;				//!< Speedtree shadow filter size
	Float			m_speedTreeShadowFilterSize2;				//!< Speedtree shadow filter size
	Float			m_speedTreeShadowFilterSize3;				//!< Speedtree shadow filter size
	Float			m_speedTreeShadowFilterSize4;				//!< Speedtree shadow filter size
	Float			m_speedTreeShadowGradient;					//!< Speedtree shadow filter gradient
	Float			m_hiResShadowBiasOffsetSlopeMul;			//!< Hi Res Shadow slope offset multiplier
	Float			m_hiResShadowBiasOffsetConst;				//!< Hi Res Shadow const bias
	Float			m_hiResShadowTexelRadius;					//!< Hi Res Shadow texel radius
	Bool			m_useTerrainShadows;						//!< Enable global terrain shadows
	Float			m_terrainShadowsDistance;					//!< Rendering distance for terrain shadows
	Float			m_terrainShadowsFadeRange;					//!< Range over which the terrain shadows fade away
	Float			m_terrainShadowsBaseSmoothing;				//!< Terrain shadows base smoothing
	Float			m_terrainShadowsTerrainDistanceSoftness;	//!< Controlls how much the terrain shadows got smoothed over distance
	Float			m_terrainShadowsMeshDistanceSoftness;		//!< Controlls how much the mesh shadows got smoothed over distance
	Float			m_terrainMeshShadowDistance;				//!< Distance at which the terrain shadows casted from meshes are drawn
	Float			m_terrainMeshShadowFadeRange;				//!< Fade range for the mesh shadows

public:
	CWorldShadowConfig();
};

BEGIN_CLASS_RTTI( CWorldShadowConfig );
	PROPERTY_EDIT_RANGE( m_numCascades, TXT("Number of cascades 1-4" ), 1, 4 );
	PROPERTY_EDIT( m_cascadeRange1, TXT("Range at which first cascade end") );
	PROPERTY_EDIT( m_cascadeRange2, TXT("Range at which second cascade ends") );
	PROPERTY_EDIT( m_cascadeRange3, TXT("Range at which third cascade ends") );
	PROPERTY_EDIT( m_cascadeRange4, TXT("Range at which fourth cascade ends") );
	PROPERTY_EDIT( m_cascadeFilterSize1, TXT("Filtering size for first cascade") );
	PROPERTY_EDIT( m_cascadeFilterSize2, TXT("Filtering size for second cascade") );
	PROPERTY_EDIT( m_cascadeFilterSize3, TXT("Filtering size for third cascade") );
	PROPERTY_EDIT( m_cascadeFilterSize4, TXT("Filtering size for fourth cascade") );
	PROPERTY_EDIT( m_shadowEdgeFade1, TXT("Size of the cascade edge fade [m]") );
	PROPERTY_EDIT( m_shadowEdgeFade2, TXT("Size of the cascade edge fade [m]") );
	PROPERTY_EDIT( m_shadowEdgeFade3, TXT("Size of the cascade edge fade [m]") );
	PROPERTY_EDIT( m_shadowEdgeFade4, TXT("Size of the cascade edge fade [m]") );
	PROPERTY_EDIT( m_shadowBiasOffsetSlopeMul, TXT("Shadow slope offset multiplier") );
	PROPERTY_EDIT( m_shadowBiasOffsetConst, TXT("Shadow const bias") );
	PROPERTY_EDIT( m_shadowBiasOffsetConstCascade1, TXT("Shadow const bias for first cascade") );
	PROPERTY_EDIT( m_shadowBiasOffsetConstCascade2, TXT("Shadow const bias for second cascade") );
	PROPERTY_EDIT( m_shadowBiasOffsetConstCascade3, TXT("Shadow const bias for third cascade") );
	PROPERTY_EDIT( m_shadowBiasOffsetConstCascade4, TXT("Shadow const bias for fourth cascade") );	
	PROPERTY_EDIT( m_shadowBiasCascadeMultiplier, TXT("Shadow bias multiplier (per cascade)") );
	PROPERTY_EDIT( m_speedTreeShadowFilterSize1, TXT("Speedtree shadow filter size for first cascade") );
	PROPERTY_EDIT( m_speedTreeShadowFilterSize2, TXT("Speedtree shadow filter size for second cascade") );
	PROPERTY_EDIT( m_speedTreeShadowFilterSize3, TXT("Speedtree shadow filter size for third cascade") );
	PROPERTY_EDIT( m_speedTreeShadowFilterSize4, TXT("Speedtree shadow filter size for fourth cascade") );
	PROPERTY_EDIT( m_speedTreeShadowGradient, TXT("Speedtree shadow gradient") );
	PROPERTY_EDIT( m_hiResShadowBiasOffsetSlopeMul, TXT("HiRes Shadow slope offset multiplier") );
	PROPERTY_EDIT( m_hiResShadowBiasOffsetConst, TXT("HiRes Shadow const bias") );
	PROPERTY_EDIT( m_hiResShadowTexelRadius, TXT("HiRes Shadow texel radius") );
	PROPERTY_EDIT( m_useTerrainShadows, TXT("Enable global terrain shadows") );
	PROPERTY_EDIT( m_terrainShadowsDistance, TXT("Rendering distance for terrain shadows") );
	PROPERTY_EDIT( m_terrainShadowsFadeRange, TXT("Range over which the terrain shadows fade away") );
	PROPERTY_EDIT( m_terrainShadowsBaseSmoothing, TXT("Terrain shadows base smoothing") );
	PROPERTY_EDIT( m_terrainShadowsTerrainDistanceSoftness, TXT("Controlls how much the terrain shadows got smoothed over distance") );
	PROPERTY_EDIT( m_terrainShadowsMeshDistanceSoftness, TXT("Controlls how much the mesh shadows got smoothed over distance") );
	PROPERTY_EDIT( m_terrainMeshShadowDistance, TXT("Distance at which the terrain shadows casted from meshes are drawn") );
	PROPERTY_EDIT( m_terrainMeshShadowFadeRange, TXT("Fade range for the mesh shadows") );
END_CLASS_RTTI();

struct SWorldSkyboxParameters
{
	DECLARE_RTTI_STRUCT( SWorldSkyboxParameters )

	THandle< CMesh >				m_sunMesh;
	THandle< CMaterialInstance >	m_sunMaterial;
	THandle< CMesh >				m_moonMesh;
	THandle< CMaterialInstance >	m_moonMaterial;
	THandle< CMesh > 				m_skyboxMesh;
	THandle< CMaterialInstance >	m_skyboxMaterial;
	THandle< CMesh >				m_cloudsMesh;
	THandle< CMaterialInstance >	m_cloudsMaterial;

	SWorldSkyboxParameters ();

	// ctremblay: Because of the way the THandle is made, msvc generate default copy operator at this point. I have to force it out of the .h if I want to forward declare CMesh...
	//SWorldSkyboxParameters & operator=( const SWorldSkyboxParameters & value );  
};

BEGIN_CLASS_RTTI( SWorldSkyboxParameters );        
PROPERTY_EDIT(        m_sunMesh,									TXT("Sun mesh.") );
PROPERTY_INLINED(     m_sunMaterial,								TXT("Sun material.") );
PROPERTY_EDIT(        m_moonMesh,									TXT("Moon mesh.") );
PROPERTY_INLINED(     m_moonMaterial,								TXT("Moon material.") );
PROPERTY_EDIT(        m_skyboxMesh,									TXT("Skybox mesh.") );
PROPERTY_INLINED(     m_skyboxMaterial,								TXT("Skybox material.") );
PROPERTY_EDIT(        m_cloudsMesh,									TXT("Clouds mesh.") );
PROPERTY_INLINED(     m_cloudsMaterial,								TXT("Clouds material.") );
END_CLASS_RTTI();


/// Lens flare group parameters
struct SLensFlareGroupsParameters
{
	DECLARE_RTTI_STRUCT( SLensFlareGroupsParameters );

public:
	SLensFlareParameters	m_default;
	SLensFlareParameters	m_sun;
	SLensFlareParameters	m_moon;
	SLensFlareParameters	m_custom0;
	SLensFlareParameters	m_custom1;
	SLensFlareParameters	m_custom2;
	SLensFlareParameters	m_custom3;
	SLensFlareParameters	m_custom4;
	SLensFlareParameters	m_custom5;

public:
	const SLensFlareParameters& GetGroupParams( ELensFlareGroup group ) const
	{
		switch ( group )
		{
		case LFG_Default:		return m_default;
		case LFG_Sun:			return m_sun;
		case LFG_Moon:			return m_moon;
		case LFG_Custom0:		return m_custom0;
		case LFG_Custom1:		return m_custom1;
		case LFG_Custom2:		return m_custom2;
		case LFG_Custom3:		return m_custom3;
		case LFG_Custom4:		return m_custom4;
		case LFG_Custom5:		return m_custom5;
		default:				ASSERT( !"invalid" ); return m_default;
		}
	}
};

BEGIN_CLASS_RTTI( SLensFlareGroupsParameters )
	PROPERTY_EDIT( m_default,		TXT("m_default") );
	PROPERTY_EDIT( m_sun,			TXT("m_sun") );
	PROPERTY_EDIT( m_moon,			TXT("m_moon") );
	PROPERTY_EDIT( m_custom0,		TXT("m_custom0") );
	PROPERTY_EDIT( m_custom1,		TXT("m_custom1") );
	PROPERTY_EDIT( m_custom2,		TXT("m_custom2") );
	PROPERTY_EDIT( m_custom3,		TXT("m_custom3") );
	PROPERTY_EDIT( m_custom4,		TXT("m_custom4") );
	PROPERTY_EDIT( m_custom5,		TXT("m_custom5") );
END_CLASS_RTTI()


/// World environment parameters
struct SWorldEnvironmentParameters
{
	DECLARE_RTTI_STRUCT( SWorldEnvironmentParameters )
	
	THandle< CBitmapTexture >			m_vignetteTexture;
	THandle< CBitmapTexture >			m_cameraDirtTexture;
	THandle< CCubeTexture >				m_interiorFallbackAmbientTexture;
	THandle< CCubeTexture >				m_interiorFallbackReflectionTexture;
	Float								m_cameraDirtNumVerticalTiles;
	CGlobalLightingTrajectory			m_globalLightingTrajectory;
	Float								m_toneMappingAdaptationSpeedUp;
	Float								m_toneMappingAdaptationSpeedDown;
	THandle< CEnvironmentDefinition >	m_environmentDefinition;	
	THandle< CEnvironmentDefinition >	m_scenesEnvironmentDefinition;
	SGlobalSpeedTreeParameters			m_speedTreeParameters;
	THandle< C2dArray >					m_weatherTemplate;
	Bool								m_disableWaterShaders;
	SWorldSkyboxParameters				m_skybox;
	SLensFlareGroupsParameters			m_lensFlare;
	SWorldRenderSettings				m_renderSettings;
	THandle< CResourceSimplexTree >		m_localWindDampers;
	THandle< CResourceSimplexTree >		m_localWaterVisibility;

	SWorldEnvironmentParameters();
};

BEGIN_CLASS_RTTI( SWorldEnvironmentParameters );        
    PROPERTY_EDIT(        m_vignetteTexture,                            TXT("Vignette texture") );
    PROPERTY_EDIT(        m_cameraDirtTexture,                          TXT("Camera dirt texture") );
	PROPERTY_EDIT(        m_interiorFallbackAmbientTexture,				TXT("Interior fallback ambient texture") );
	PROPERTY_EDIT(        m_interiorFallbackReflectionTexture,			TXT("Interior fallback reflection texture") );
	PROPERTY_EDIT(        m_cameraDirtNumVerticalTiles,					TXT("Camera dirt num vertical tiles") );
    PROPERTY_INLINED(     m_globalLightingTrajectory,                   TXT("Global lighting trajectory") );
    PROPERTY_EDIT(        m_toneMappingAdaptationSpeedUp,				TXT("Tone mapping adaptation speed up") );    
	PROPERTY_EDIT(        m_toneMappingAdaptationSpeedDown,				TXT("Tone mapping adaptation speed down") );    
	PROPERTY_EDIT(        m_environmentDefinition,						TXT("Environment definition resource") );		
	PROPERTY_EDIT(        m_scenesEnvironmentDefinition,				TXT("Scenes environment definition") );
	PROPERTY_EDIT(        m_speedTreeParameters,						TXT("Speed Tree parameters") );
	PROPERTY_EDIT(        m_weatherTemplate,							TXT("Default weather template parameters") );
	PROPERTY_EDIT(        m_disableWaterShaders,						TXT("Set this to disable all global water rendering on this level") );
	PROPERTY_EDIT(        m_skybox,										TXT("Skybox parameters.") );
	PROPERTY_EDIT(        m_lensFlare,									TXT("Lens flare.") );
	PROPERTY_EDIT(        m_renderSettings,								TXT("Render settings") );
	PROPERTY_EDIT(        m_localWindDampers,							TXT("Simplex Wind Areas.") );
	PROPERTY_EDIT(        m_localWaterVisibility,						TXT("Simplex Water Visibility Areas.") );
END_CLASS_RTTI();

/// Structure describing how to initialize the world
class WorldInitInfo
{
public:
	Bool		m_previewWorld;				//!< This is an in editor preview world (EDITOR ONLY!)

	Bool		m_initializePhysics;		//!< Initialize physical representation for this world, always enabled by default - disable only to save memory
	Bool		m_initializePathLib;		//!< Create the PathLib representation (optional, should be there only for the game worlds) - disabled by default
	Bool		m_initializeOcclusion;		//!< Initialize the occlusion system - disabled by default
	CWorld*		m_sharePhysicalWorld;		//!< Share physical world with this specified world

public:
	WorldInitInfo();
};

#define USE_BALANCE_STREAM_DISPATCH_E3_HACK
/// World class
class CWorld
:	public CResource
#ifndef NO_EDITOR_EVENT_SYSTEM
,	public IEdEventListener
#endif
,	public ITriggerSystemTerrainQuery
{
	DECLARE_ENGINE_RESOURCE_CLASS( CWorld, CResource, "w2w", "World" );

	friend class CGame;
	friend class CLayer;
	friend class CComponent;
	friend class CEdPreviewPanel;
	friend class CEdWorldEditPanel;
	friend class WorldAttachedLayerIterator;
	friend class WorldAttachedComponentsIterator;

	// For debugging purposes
	friend class RedGui::CRedGuiResourceSystemHeatMapControl;

private:
	class WorldUpdateStateContext
	{
	private:
		EWorldUpdatePhase	m_prevUpdatePhase;
		CWorld*				m_world;

	public:
		WorldUpdateStateContext( CWorld* world, EWorldUpdatePhase newPhase );
		~WorldUpdateStateContext();
	};

	struct CEntityByGUIDHashFunc
	{
		static RED_FORCE_INLINE Uint32 GetHash( const CEntity* entity ) { return entity->GetGUID().CalcHash(); }
		static RED_FORCE_INLINE Uint32 GetHash( const CGUID& guid ) { return guid.CalcHash(); }
	};
	struct CEntityByGUIDEqualFunc
	{
		static RED_INLINE Bool Equal( const CEntity* a, const CEntity* b ) { return a == b; }
		static RED_INLINE Bool Equal( const CEntity* entity, const CGUID& guid ) { return entity->GetGUID() == guid; }
	};
	typedef THashSet< CEntity*, CEntityByGUIDHashFunc, CEntityByGUIDEqualFunc > TEntityByGUIDSet;

	struct CPersistentEntityByIdTagHashFunc
	{
		static RED_FORCE_INLINE Uint32 GetHash( const CPeristentEntity* entity ) { return entity->GetIdTag().CalcHash(); }
		static RED_FORCE_INLINE Uint32 GetHash( const IdTag& tag ) { return tag.CalcHash(); }
	};
	struct CPersistentEntityByIdTagEqualFunc
	{
		static RED_INLINE Bool Equal( const CPeristentEntity* a, const CPeristentEntity* b ) { return a == b; }
		static RED_INLINE Bool Equal( const CPeristentEntity* entity, const IdTag& tag ) { return entity->GetIdTag() == tag; }
	};
	typedef THashSet< CPeristentEntity*, CPersistentEntityByIdTagHashFunc, CPersistentEntityByIdTagEqualFunc > TPersistentEntityByIdTagSet;

protected:
	typedef THashMap< Uint64, CLayerGroup* > LayerGroupsMap;

	CDynamicLayer*								m_dynamicLayer;										// Dynamic layer for spawning dynamic objects
	CLayerGroup*								m_worldLayers;										// Hierarchy of world layers
	LayerGroupsMap								m_layerGroupsMap;
	TEntityByGUIDSet							m_attachedEntitiesByGUID;							// Attached entities (hashed by GUID, not by CEntity* pointer)
	TPersistentEntityByIdTagSet					m_attachedPersistentEntitiesByIdTag;				// Attached persistent entities (hashed by IdTag, not by CPersistentEntity* pointer)
						
	CStreamingSectorData*						m_streamingSectorData;								// Streaming of entities
	CSectorDataStreaming*						m_sectorStreaming;									// Streaming of sector data (packed layer data)
	CSectorDataStreamingGCFlusher*				m_sectorStreamingGCFlusher;							// Flushing of streaming before every GC
	CSectorPrefetchRuntimeCache*				m_sectorPrefetch;									// Streaming data prefetcher
	THashSet< CLayer* >							m_attachedLayers;									// List of attached layers
	CUpdateTransformManager						m_updateTransformManager;							// Manager for update transform functionality
	CSelectionManager*							m_selectionManager;									// Get selection manager
	CWorldTickThread*							m_tickThread;										// Tick thread
	CPhysicsWorld*								m_physicsWorld;										// DA physics world
	CPhysicsWorld*								m_physicsWorldSecondary;							// DA physics world (secondary)
	CWorld*										m_sharedPhyscalWorldOwner;							// World that we share physics with
	CTickManager*								m_tickManager;										// Tick manager
	CComponent*									m_allAttachedComponents;							// All attached components
	ITriggerManager*							m_triggerManager;									// Trigger system
	CTagManager*								m_tagManager;										// Tagged nodes manager
	CEnvironmentManager*						m_environmentManager;								// Environment manager
	Vector										m_lastEnvironmentManagerUpdatePos;					// Last environment manager update position
	SWorldEnvironmentParameters					m_environmentParameters;							// World (global) environment parameters
	String										m_depotPath;										// Depot path
	IRenderScene*								m_renderSceneEx;									// New rendering scene object
	CWorldEditorFragmentsFilter					m_editorFragmentsFilter;							// Categories filter for editor fragments
	Vector										m_cameraPosition;									// MGol: Supposed to be last camera position, read with care
	Vector										m_cameraForward;									// MGol: Supposed to be last camera position, read with care
	Vector										m_cameraUp;											// MGol: Supposed to be last camera position, read with care
	CCameraDirector*							m_cameraDirector;									// The camera director
	class CEntityMotionManager*					m_entityMotionManager;								// Entity motion manager, mostly used to prototype entity motions for quests
	CWorldLookupRegistry*						m_lookupRegistry;									// World lookup registry 
	CLODableManager								m_componentLODManager;							// Manages tick LODs for non-actor components
	CEffectManager								m_effectTickLODManager;								// Manages tick LODs for effects
	CManualStreamingHelper						m_manualStreamingHelper;
	THashMap< CGUID, CLayerInfo* >				m_guidLayerInfoCache;

	Bool										m_gameStartInProgress;
	THashSet< CNode* >							m_onGameStartedQueue;								// Set of entities to inform of the game start

#ifndef NO_EDITOR
	Bool										m_enableStreaming;									// Enable streaming while moving around the world
#endif
	Bool										m_atlasRegionsRegenerateRequested;					// Atlas textures render params needs to be updated
	Bool										m_atlasRegionsRegenerateForceNext;					// Should next atlas texture regeneration be forced
	EWorldUpdatePhase							m_updatePhase;										// On which phase of update we are in
	CPathLibWorld*								m_pathLib;											// PathLib world data & functionality interface
	THandle< CUmbraScene >						m_umbraScene;										// Umbra scene, responsible for generating occlusion data and occlusion queries
	TQueue< VectorI >							m_tilesToGenerate;
	struct STomeDataGenerationContext*			m_tomeGenerationContext;

	IUpdateTransformStatCollector*				m_updateTransformStatCollector;						// Stat collector for update transform crap
	CName										m_startupWaypointTag;								// Tag of the startup waypoint
	Vector										m_startupCameraPosition;							// Startup position for preview camera
	EulerAngles									m_startupCameraRotation;							// Startup rotation for preview camera


	static const Float							DEFAULT_WORLD_DIMENSIONS;
	Float										m_worldDimensions;
	
	TDynArray< CLayerInfo* >					m_queueUpdating;

	TDynArray< CLayerInfo* >					m_needsUpdating;									// Layers that need updating

	CClipMap*									m_terrainClipMap;									// Terrain on this world
		
	CGlobalWater*								m_globalWater;										// Global water object - for cpu height map calculations
	CDynamicCollisionCollector*					m_dynamicCollisions;								// Pseudo-physics collision shapes collector for grass/water reaction

	CWorldShadowConfig							m_shadowConfig;										// Config for global shadows ( terrain + cascades )

	Bool										m_isPreviewWorld;									// Is this a world created in editor just for preview

	CFoliageScene*								m_foliageScene;
	Red::TUniquePtr< CFoliageEditionController > m_foliageEditionController;

	CEntity*									m_skyboxEntity;										// At init, we automatically create the skybox entity.

	ITriggerActivator*							m_cameraActivator;									//!< Trigger system activator for the camera

	TDynArray< CName >							m_soundBanksDependency;

	TDynArray< StringAnsi >						m_soundEventsOnAttach;
	TDynArray< StringAnsi >						m_soundEventsOnDetach;

	Bool										m_newLayerGroupFormat;								//!< World was saved in new layer group format
	THandle< C2dArray >							m_initialyHidenLayerGroups;							//!< Layer groups that are makred as hidden

	Bool										m_hasEmbeddedLayerInfos;							//!< We have cooked world data (pulled layer groups and layer infos)
	Int32										m_forcedGraphicalLOD;

	CPhysicsBatchQueryManager*					m_physicsBatchQueryManager;							//!< Manager for doing asynchronous traces all at once

	Red::TAtomicSharedPtr< CCharacterControllersManager > m_controllerManager;	
	TDynArray< CName >							m_playGoChunks;

	String										m_minimapsPath;										//!< Path to minimap textures for this level (not cooked)
	String										m_hubmapsPath;										//!< Path to hubmap textures for this level (not cooked)

	THandle< CObject >							m_preloadedData;									//!< Preloaded world startup data

	CMergedWorldGeometry*						m_mergedGeometry;									//!< Container for merged geometry

	CTaskBatch									m_taskBatch;

	// Recycled per frame mainly because of PS4 and to avoid crashes *here* if something else takes up way too much internal pthread memory
	CTaskBatch									m_tickManagerNarrowScopeTaskBatch;
	CTaskBatch									m_tickManagerBroadScopeTaskBatch;

	Bool										m_debugLastStreamingCameraValid;
	Vector										m_debugLastStreamingCamera;

public:
	CWorld();
	~CWorld();

	void ForceFinishAsyncResourceLoads();

	// get forced graphical lod
	RED_INLINE Int32 GetForcedGraphicalLOD() const { return m_forcedGraphicalLOD; }

    // TODO: REMOVE and place that stuff somewhere else
	RED_INLINE class CStreamingSectorData* GetStreamingSectorData() const { return m_streamingSectorData; }
	RED_INLINE class CSectorDataStreaming* GetSectorDataStreaming() const { return m_sectorStreaming; }

	// Get streaming prefetch system
	RED_INLINE CSectorPrefetchRuntimeCache* GetStreamingDataPrefetch() const { return m_sectorPrefetch; }

	// Get dynamic layer used for placing dynamic objects
	RED_INLINE CDynamicLayer* GetDynamicLayer() const { return m_dynamicLayer; }

	// Get world layer hierarchy
	RED_INLINE CLayerGroup* GetWorldLayers() const { return m_worldLayers; }

	// Get world depot path
	RED_INLINE const String& DepotPath() const { return m_depotPath; }

	// Get update transform manager
	RED_INLINE CUpdateTransformManager& GetUpdateTransformManager() { return m_updateTransformManager; }

	// Get selection manager
	RED_INLINE CSelectionManager* GetSelectionManager() const { return m_selectionManager; }

	template< typename T >
	Bool GetPhysicsWorld( T*& physicsWorld ) { physicsWorld = static_cast< T* >( m_physicsWorld ); return physicsWorld != nullptr; }

	template< typename T >
	Bool GetPhysicsWorldSecondary( T*& physicsWorld ) { physicsWorld = static_cast< T* >( m_physicsWorldSecondary ); return physicsWorld != nullptr; }

	// Checks if the physical world is crerated by this world (isn't stealed)
	RED_INLINE Bool IsPhysicalWorldOwned() const { return !m_sharedPhyscalWorldOwner; }

	// Get the batch trace manager
	RED_INLINE CPhysicsBatchQueryManager* GetPhysicsBatchQueryManager() const { return m_physicsBatchQueryManager; }

	RED_INLINE Red::TAtomicSharedPtr< CCharacterControllersManager > GetCharacterControllerManager() const { return m_controllerManager; }

	// Get tick manager
	RED_INLINE CTickManager* GetTickManager() const { return m_tickManager; }

	// Get trigger manager
	RED_INLINE ITriggerManager* GetTriggerManager() const { return m_triggerManager; }

	// Get tag manager
	RED_INLINE CTagManager* GetTagManager() const { return m_tagManager; }

	// Get environment manager
	RED_INLINE CEnvironmentManager* GetEnvironmentManager() const { return m_environmentManager; }

	// Get the tick thread
	RED_INLINE CWorldTickThread* GetTickThread() const { return m_tickThread; }

	// Get new render scene interface
	RED_INLINE IRenderScene* GetRenderSceneEx() const { return m_renderSceneEx; }

	// Get editor fragments filter
	RED_INLINE CWorldEditorFragmentsFilter& GetEditorFragmentsFilter() { return m_editorFragmentsFilter; }

	// Get update phase we are in
	RED_INLINE EWorldUpdatePhase GetUpdatePhase() const { return m_updatePhase; }

	RED_INLINE const Vector& GetCameraPosition() const { return m_cameraPosition; }

	// Get position of camera ( editor mostly )
	RED_INLINE const Vector& GetCameraForward() const { return m_cameraForward; }

	// Get position of camera ( editor mostly )
	RED_INLINE const Vector& GetCameraUp() const { return m_cameraUp; }

	// Get camera manager
	RED_INLINE CCameraDirector* GetCameraDirector() const { return m_cameraDirector; }

	// Get entity motion manager
	RED_INLINE class CEntityMotionManager* GetEntityMotionManager() const { return m_entityMotionManager; }

	RED_INLINE CPathLibWorld* GetPathLibWorld() { return m_pathLib; }
	RED_INLINE const CPathLibWorld* GetPathLibWorld() const { return m_pathLib; }

	//! Get assigned PlayGO chunks
	RED_INLINE const TDynArray< CName >& GetPlayGoChunks() const { return m_playGoChunks; }

	//! Is this the preview world ?
	RED_INLINE Bool GetPreviewWorldFlag() const { return m_isPreviewWorld; }

	RED_INLINE CUmbraScene* GetUmbraScene() 
	{ 
#ifdef USE_UMBRA 
		return m_umbraScene.Get(); 
#else 
		return nullptr; 
#endif 
	}
	RED_INLINE const CUmbraScene* GetUmbraScene() const 
	{ 
#ifdef USE_UMBRA 
		return m_umbraScene.Get(); 
#else 
		return nullptr; 
#endif 
	}

	RED_INLINE CFoliageScene* GetFoliageScene() const { return m_foliageScene; }

	RED_INLINE CLODableManager& GetComponentLODManager() { return m_componentLODManager; } 
	RED_INLINE CEffectManager& GetEffectManager() { return m_effectTickLODManager; } 
	RED_INLINE CManualStreamingHelper& GetManualStreamingHelper() { return m_manualStreamingHelper; } 

	RED_INLINE CMergedWorldGeometry* GetMergedGeometryContainer() const { return m_mergedGeometry; }

#ifndef NO_EDITOR
	void EnableStreaming( Bool enable, Bool forceLoad = true );
	RED_INLINE Bool IsStreamingEnabled() const { return m_enableStreaming; }
#endif

	// Returns the world lookup registry
	RED_INLINE CWorldLookupRegistry* GetLookupRegistry() const { return m_lookupRegistry; }

	// Get world shadow configuration
	RED_INLINE const CWorldShadowConfig& GetShadowConfig() const { return m_shadowConfig; }

	// Set world shadow configuration
	RED_INLINE void SetShadowConfig( const CWorldShadowConfig& shadowConfig ) { m_shadowConfig = shadowConfig; }

	RED_INLINE const SWorldEnvironmentParameters& GetEnvironmentParameters() const { return m_environmentParameters; }
	void SetEnvironmentParameters( const SWorldEnvironmentParameters& params );
	void SetupSceneSkyboxParams( const SWorldSkyboxParameters &params );
	void RefreshSceneSkyboxParams();
	void RefreshLensFlareParams();

	CFoliageEditionController & GetFoliageEditionController();
	const CFoliageEditionController & GetFoliageEditionController() const;
	CFoliageDynamicInstanceService CreateFoliageDynamicInstanceService();

	// ------------------------------------------------------------------------
	// Layers visibility changes
	// ------------------------------------------------------------------------
	// Perform layers visibility change
	void ChangeLayersVisibility( const TDynArray< String >& groupsToHide, const TDynArray< String >& groupsToShow, Bool purgeStorages, class CWorldLayerStreamingFence* fence );

	// Is the world saved in the new layer group/layer info format ?
	RED_INLINE const Bool IsNewLayerGroupFormat() const { return m_newLayerGroupFormat; }

	// Get startup camera position
	RED_INLINE const Vector& GetStartupCameraPosition() const { return m_startupCameraPosition; }

	// Get startup camera rotation
	RED_INLINE const EulerAngles& GetStartupCameraRotation() const { return m_startupCameraRotation; }

	// Register new layer group
	void RegisterLayerGroup( CLayerGroup* layerGroup );

	// Unregister layer group
	void UnregisterLayerGroup( CLayerGroup* layerGroup );

	// Query layer group
	CLayerGroup* GetLayerGroupById( Uint64 layerGroupId );

public:
	// Serialize object
	virtual void OnSerialize( IFile& file );

#ifndef NO_RESOURCE_COOKING
	// World cooking
	virtual void OnCook( class ICookerFramework& cooker );
#endif

	// Should we prevent GC from collecting this resource
	virtual Bool PreventCollectingResource() const;
	
#ifndef NO_DATA_VALIDATION
	//! Check data
	virtual void OnCheckDataErrors() const;
#endif

	virtual void OnPropertyPostChange( IProperty* property );

	// Initialize dynamic world shit
	virtual void Init( const WorldInitInfo& initInfo );

	// Shut down dynamic world shit
	virtual void Shutdown();

#ifndef NO_EDITOR_WORLD_SUPPORT
	// When game is ended in the editor, the world is not shutdown, but a lot of it is cleaned up. Some extra processing can be done here.
	void OnEditorGameStopped();
#endif

#ifndef NO_EDITOR_RESOURCE_SAVE
	//! Called just before this resource is saved in editor
	virtual void OnResourceSavedInEditor() override;
#endif

	// Update layers internal state loading
	void UpdateLayersLoadingState( Bool sync = false );

	// Update internal state loading
	void UpdateLoadingState( Bool sync = false );

	RED_INLINE void AddToUpdateList( CLayerInfo* layer )
	{
#ifdef USE_WORLD_STREAMING_THROTTLE
		m_queueUpdating.PushBackUnique( layer );
#else
		m_needsUpdating.PushBackUnique( layer );
#endif
	}

	RED_INLINE void RemoveFromUpdateList( CLayerInfo* layer )
	{
#ifdef USE_WORLD_STREAMING_THROTTLE
		m_queueUpdating.Remove( layer );
#endif
		m_needsUpdating.Remove( layer );
	}

	RED_INLINE Bool HasPendingStreamingTasks() const { return !m_needsUpdating.Empty(); }

	// Enumerate layers
	void EnumerateLayersStructure();
	
	//**************************************************************************************************
	//** Please do not remove these following 3 functions. I left them for a reason:				  **
	//** I want to avoid creating such things in future, so if anyone needs such a functionality,     **
	//** he should find it here and read this comment.											      **
	//** 																						      **
	//** Instead of filling temporary table just to iterate over it, one should use an iterator.	  **
	//** We have a pretty decent ones, made some time ago by Tomsin:								  **
	//** WorldAttachedComponentsIterator, WorldAttachedEntitiesIterator, WorldAttachedLayersIterator  **
	//** Please make use of them.																	  **
	//**************************************************************************************************

	// Get list of attached layers. Use WorldAttachedLayerIterator
	void GetAttachedLayers_DeprecatedDoNotUse( TDynArray< CLayer* >& layers );

	// Get list of all attached entities. Use WorldAttachedEntitiesIterator
	void GetAttachedEntities_DeprecatedDoNotUse( TDynArray< CEntity* >& entities );

	// Get list of all attached components. Use WorldAttachedComponentIterator
	void GetAttachedComponents_DeprecatedDoNotUse( TDynArray< CComponent* >& components );

	//**************************************************************************************************

	// Get list of all attached components
	template< class T >
	void GetAttachedComponentsOfClass( TDynArray< T* >& components ) const;

	// Tick, should be called from tick thread
	void Tick( const CWorldTickInfo& tickInfo, const CRenderFrameInfo* previousFrameInfo = NULL, CRenderFrame** frame = NULL, Bool updateCurrentCamera = false );

	// Generate rendering frame
	CRenderFrame* GenerateFrame( IViewport *viewport, CRenderFrameInfo& info );

	//! Render this world using given frame
	void RenderWorld( CRenderFrame* frame );

#ifndef NO_EDITOR
	//! Render this world using given frame with optional forced prefetch
	void RenderWorld( CRenderFrame* frame, Bool forcePrefetch );
#endif

#ifndef NO_EDITOR_FRAGMENTS
	// Generate editor hit proxies
	void GenerateEditorHitProxies( CHitProxyMap& hitProxyMap );

	// Generate editor fragments
	virtual void GenerateEditorFragments( CRenderFrame* frame );
#endif

	// Get world name (directory name, works only for saved worlds, returns empty string for preview worlds)
	String GetWorldDirectoryName() const;

	// Get world size
	const Float GetWorldDimensionsWithTerrain() const;

	// Find layer using layer GUID
	CLayer* FindLayer( const CGUID &layerGuid );

    // and by tag
    CLayer* FindLayerByTag( const CName& tag );

	// Finds entity by GUID
	CEntity* FindEntity( const CGUID& entityGUID );
	// Registers entity
	void RegisterEntity( CEntity* entity );
	// Unregisters entity
	void UnregisterEntity( CEntity* entity );
	// Invoked after entity GUID changes
	void OnEntityGUIDChanged( CEntity* entity, const CGUID& oldGuid );

	// Finds persistent entity by IdTag
	CPeristentEntity* FindPersistentEntity( const IdTag& tag );
	// Registers persistent entity
	void RegisterPersistentEntity( CPeristentEntity* entity );
	// Unregisters persistent entity
	void UnregisterPersistentEntity( CPeristentEntity* entity );
	// Invoked after persistent entity IdTag changes
	void OnPersistentEntityIdTagChanged( CPeristentEntity* entity, const IdTag& oldTag );

	// Checks for line segment intersections (for tool purposes only, it will try to use physics world if available and approximate heightmap checks otherwise)
	Bool CheckForSegmentIntersection( const Vector& a, const Vector& b, Vector& worldCoords, Vector& worldNormal, Bool snapOnlyToHeightmap = false );

	// Convert coordinates, used for clicking :)
	Bool ConvertScreenToWorldCoordinates( IViewport* view, Int32 screenX, Int32 screenY, Vector & worldCoords, Vector * worldNormal = NULL, Bool snapOnlyToHeightmap = false );

	// Request atlas regions regeneration
	void RequestAtlasRegionsRegeneration( bool forceRegenerate );

	// Method that load synchronously the initial set of static data for the world
	// This data can remain loaded when we are loading a savegame for the same world
	void LoadStaticData();

	// Flush tags - make sure all nodes and entities has valid tags registered
	void FlushTags();

	// Change the initial visibility flag for a layer group, requires checkout of the world file
	Bool SetLayerGroupInitialVisibility( const CLayerGroup* layerGroup, const Bool flag );

	// Get the initial visibility flag for layer group
	Bool GetLayerGroupInitialVisibility( const CLayerGroup* layerGroup ) const;

	// Pull layer groups and layer infos into the CWorld resource (makes the loading faster and saves the layer enumeration step)
	void PullLayerGroupsAndLayerInfos();

	// Get streaming stats
	void GetStreamingStats( struct SSectorStreamingDebugData& outSectorStreamingStats ) const;

	// Hack? CONSULT ON REVIEW!!
	virtual void OnLayerAttached( CLayer* layer ) {} // implemented in CGameWorld
	virtual void OnLayerDetached( CLayer* layer ) {} // implemented in CGameWorld

public:

#ifndef NO_EDITOR_WORLD_SUPPORT

	// Add new layers after sync
	void SynchronizeLayersAdd(CLayerGroup* group);

	// Remove layers after sync
	void SynchronizeLayersRemove(CLayerGroup* group);

	// Save all layers, returns false if any of the saving fails
	Bool SaveLayers();

#endif

public:

	Float GetWorldDimensions() const { return m_worldDimensions; }



public:
	// Load world
	static THandle< CWorld > LoadWorld( const String& depotPath, WorldLoadingContext& context );

	// Unload world
	static void UnloadWorld( CWorld* world );

public:
	//! Perform delayed world actions ( attaching, detaching, etc )
	void DelayedActions() {}

	virtual void PreLayersAttach();
	virtual void OnGameStarted( const CGameInfo& info );
	void RegisterForOnGameStarted( CNode* node );

public:

#ifndef NO_EDITOR
	CEntity* CreateEntity( const EntitySpawnInfo& spawnInfo );
#if 0
	void ChangeEntityLOD( CEntity* entity, CLayerGroup* newLODGroup );
#endif
	Bool PasteEntities( CLayer* layer, const TDynArray< Uint8 >& data, const TDynArray< Uint8 >& infoData, TDynArray< CEntity* >& pastedEntities, Bool relativeSpawn, const Vector& spawnPosition, const EulerAngles& spawnRotation );

	struct STileLayerWithBBox
	{
		CLayer* layer;
		Box		box;
	};

	void GetOrCreateTileLayersRectWithBBox( const Box& worldRect, const String& layerName, TDynArray< STileLayerWithBBox >& tileLayers );
#endif

	void FindTileLayersAroundPositionRect( const Vector& position, Float radius, const String& layerName, TDynArray< CLayer* >& tileLayers, Bool allowLoading );

	// Returns a "nice" LOD for the given box
	static Int8 SuggestStreamingLODFromBox( const Box& box );

	// Add sector data to the world streaming
	Uint32 RegisterSectorData( const Uint64 contentHash, class CSectorData* sectorData, const Bool isVisible );

	// Unregister sector data from the streaming
	void UnregisterSectorData( const Uint32 sectorDataId );

	// Toggle sector data visibility
	void ToggleSectorDataVisibility( const Uint32 sectorDataId, const Bool isVisible );

	// Forces a full streaming update (sync, slow, but ensures everything is loaded)
	void ForceStreamingUpdate();

	// Request streaming update on next tick (needed when new layers are loaded/removed)
	void RequestStreamingUpdate();

	// Force stream stuff in given area, NOTE: entities may be removed by normal streaming next frame
	void ForceStreamingForArea( const Box& box );

	// Stream all stuff visible for given point, NOTE: entities may be removed by normal streaming next frame
	void ForceStreamingForPoint( const Vector& point );

	// Add entity to the streaming ignore list
	void IgnoreEntityStreaming( CEntity* entity );

	// Remove entity from the streaming ignore list
	void UnignoreEntityStreaming( CEntity* entity );

	// Clear entity ignore list
	void ClearStreamingIgnoreList();

	// Update camera position (used by rendering panel, world edit panel, and game)
	void UpdateCameraPosition( const Vector& position );

	// Update camera position (used by rendering panel, world edit panel, and game)
	void UpdateCameraForward( const Vector& position );

	// Update camera position (used by rendering panel, world edit panel, and game)
	void UpdateCameraUp( const Vector& position );

	// Update streaming reference position (does not have to be camera's position)
	void SetStreamingReferencePosition( const Vector& position );

	// Setup locked area for streaming (will clear previous locked area), NOTE: this will NOT stream anything right away
	void SetStreamingLockedArea( const Box* worldSpaceBounds, const Uint32 numAreas );

	// Finsh fading hapending in the renderer
	void FinishRenderingFades();

	// Terrain system notification
	void OnTerrainTilesDestruction();
	void OnTerrainTilesCreation();
	void OnTerrainTileUpdate( Uint32 x, Uint32 y );

#ifdef USE_ANSEL
	Bool EnsureTerrainCollisionGenerated( const Vector& position );
#endif // USE_ANSEL

	virtual void OnTerrainCollisionDataBoundingUpdated( const Box& bbox );
	// Wind
	virtual Vector GetWindAtPoint( const Vector& point ) const { return Vector::ZEROS; }
	virtual Vector GetWindAtPointForVisuals( const Vector& point, Bool withTurbulence, Bool withForcefield = true ) const { return Vector::ZEROS; }
	virtual void GetWindAtPoint( Uint32 elementsCount, void* inputPos, void* outputPos, size_t stride ) const {}

	virtual void FinalizeMovement( Float timeDelta ) {}

	// Merged geometry
#ifndef NO_RESOURCE_IMPORT
	const Bool RebuildMergedGeometry( const Vector& worldCenter, const Float worldRadius );
#endif

	// ------------------------------------------------------------------------
	// Save game
	// ------------------------------------------------------------------------
	// Saves the world state
	virtual void SaveState( IGameSaver* saver ) {}

	// Restores the world state
	virtual void RestoreState( IGameLoader* loader ) {}

public:
	virtual void RefreshAllWaypoints( const Box& bbox ) {}

protected:
#ifndef NO_EDITOR_EVENT_SYSTEM
	//! Dispatch editor related event
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

public:
	CClipMap* CreateTerrain();

	// writing getters the Scott Meyers way (Effective C++, 3d ed)
	const CClipMap*	GetTerrain() const	{ return m_terrainClipMap; }
	CClipMap*		GetTerrain()		{ return const_cast< CClipMap* >( static_cast< const CWorld& >( *this ).GetTerrain() ); }

	void	  SetTerrain(CClipMap* terrain) { m_terrainClipMap = terrain; }
	
	void							SetWaterVisible( Bool isVisible, Bool surpassRendering = false );
	void							UpdateWaterProxy();
	Float							GetWaterLevel( const Vector &point, Uint32 lodApproximation = 3, Float* heightDepth = 0 ) const; //<! lodApproximation = 0..3, 0 is perf heaviest -> 3 is perf cheapest
	Bool							GetWaterLevelBurst( Uint32 elementsCount, void* inputPos, void* outputPos, void* outputHeightDepth, size_t stride, Vector referencePosition, Bool useApproximation = true ) const;
	Bool							IsWaterShaderEnabled() const;	
	CGlobalWater*					GetGlobalWater() const { return m_globalWater; }
	CDynamicCollisionCollector*		GetDynamicCollisionsCollector() const { return m_dynamicCollisions; }

	TDynArray< StringAnsi >&		GetSoundEventsOnAttach() { return m_soundEventsOnAttach; }

protected:
	// ITriggerSystemTerrainQuery interface implementation
	virtual bool IsPositionAboveTerrain( const Vector& position ) const;

public:
#ifndef NO_EDITOR
#ifdef USE_UMBRA
	Bool GenerateDuplicatesList( struct SDuplicatesFinderContext& context );
	Bool GenerateOcclusionForTileSync( struct STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData );
	Bool GenerateOcclusionForAllTilesSync( const VectorI& bounds, struct STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData, Bool dumpExistingTomeData );
	Bool CalculateRuntimeOcclusionMemoryUsage( const Float density );

	void NotifyOcclusionSystem( CEntity * entity );
#endif // USE_UMBRA
#endif // NO_EDITOR

public:

	// Returns true if there are no obstacles ('Static' and 'Terrain') between source and target position
	Bool TestLineOfSight( const Vector& sourcePos, const CNode* target, Bool adjustTargetPos = false, const TDynArray< const CEntity* > * ignoreEntities = nullptr ) const;
	Bool TestLineOfSight( const Vector& sourcePos, const Vector& targetPos, const TDynArray< const CEntity* > * ignoreEntities = nullptr ) const;
	Bool TestLineOfSight( const Vector& sourcePos, const Vector& targetPos ) const;

private:
	// Initialize streaming system
	void InitializeStreaming();

	// Initialize and enumerate the layers
	void InitializeLayersStructure();

#ifdef USE_UMBRA
	// initialization of Umbra occlusion culling system
	void InitializeOcclusionCulling();
#endif

	// Intialize physical representation
	void InitializePhysicalRepresentation( const WorldInitInfo& worldInitInfo );

	// Initialize pathlib
	void InitializePathLib();

public:
	void funcShowLayerGroup( CScriptStackFrame& stack, void* result );
	void funcHideLayerGroup( CScriptStackFrame& stack, void* result );
	void funcPointProjectionTest( CScriptStackFrame& stack, void* result );
	void funcStaticTrace( CScriptStackFrame& stack, void* result );	
	void funcStaticTraceWithAdditionalInfo( CScriptStackFrame& stack, void* result );
	void funcGetWaterLevel( CScriptStackFrame& stack, void* result );
	void funcGetWaterTangent( CScriptStackFrame& stack, void* result );
	void funcGetWaterDepth( CScriptStackFrame& stack, void* result );
	void funcSweepTest( CScriptStackFrame& stack, void* result );
	void funcSphereOverlapTest( CScriptStackFrame& stack, void* result );
	void funcNavigationLineTest( CScriptStackFrame& stack, void* result );
	void funcNavigationCircleTest( CScriptStackFrame& stack, void* result );
	void funcNavigationClosestObstacleToLine( CScriptStackFrame& stack, void* result );
	void funcNavigationClosestObstacleToCircle( CScriptStackFrame& stack, void* result );
	void funcNavigationClearLineInDirection( CScriptStackFrame& stack, void* result );
	void funcNavigationFindSafeSpot( CScriptStackFrame& stack, void* result );
	void funcNavigationComputeZ( CScriptStackFrame& stack, void* result );
	void funcPhysicsCorrectZ( CScriptStackFrame& stack, void* result );
	void funcGetDepotPath( CScriptStackFrame& stack, void* result );
	void funcForceGraphicalLOD( CScriptStackFrame& stack, void* result );
	void funcGetTerrainParameters( CScriptStackFrame& stack, void* result );
	void funcGetTraceManager( CScriptStackFrame& stack, void* result );
};

template <> RED_INLINE Bool CWorld::GetPhysicsWorld( CPhysicsWorld*& physicsWorld ) { physicsWorld = m_physicsWorld; return physicsWorld != nullptr; }
template <> RED_INLINE Bool CWorld::GetPhysicsWorldSecondary( CPhysicsWorld*& physicsWorld ) { physicsWorld = m_physicsWorldSecondary; return physicsWorld != nullptr; }

BEGIN_CLASS_RTTI( CWorld );
	PARENT_CLASS( CResource );
	PROPERTY( m_startupCameraPosition );
	PROPERTY( m_startupCameraRotation );
	PROPERTY( m_terrainClipMap );
	PROPERTY( m_newLayerGroupFormat );		
	PROPERTY( m_hasEmbeddedLayerInfos );
	PROPERTY_RO( m_initialyHidenLayerGroups, TXT("List of layer groups that are initialy hidden") );
#ifdef USE_UMBRA
	PROPERTY_INLINED( m_umbraScene, TXT( "Definition of occlusion data for current level" ) );
#endif
	PROPERTY( m_globalWater );
	PROPERTY_INLINED( m_pathLib, TXT("PathLib") );
	PROPERTY_EDIT( m_worldDimensions, TXT( "The size of the world" ) );
	PROPERTY_EDIT( m_shadowConfig, TXT("Global shadow configuration") );
	PROPERTY_INLINED( m_environmentParameters, TXT("World environment parameters") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundBanksDependency, TXT( "" ), TXT( "SoundBankEditor" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnAttach, TXT( "" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnDetach, TXT( "" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_INLINED( m_foliageScene, TXT("Foliage Scene") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_playGoChunks, TXT("PlayGo chunks associated with the world"), TXT("PlayGoChunkSelector") );
	PROPERTY_EDIT_NOT_COOKED( m_minimapsPath, TXT("Minimaps path (for cooking)") );
	PROPERTY_EDIT_NOT_COOKED( m_hubmapsPath, TXT("Hubmaps path (for cooking)") );
	PROPERTY_INLINED( m_mergedGeometry, TXT("Merged geometry holder") );
	NATIVE_FUNCTION( "ShowLayerGroup", funcShowLayerGroup );
	NATIVE_FUNCTION( "HideLayerGroup", funcHideLayerGroup );
	NATIVE_FUNCTION( "PointProjectionTest", funcPointProjectionTest );
	NATIVE_FUNCTION( "StaticTrace", funcStaticTrace );	
	NATIVE_FUNCTION( "StaticTraceWithAdditionalInfo", funcStaticTraceWithAdditionalInfo );	
	NATIVE_FUNCTION( "GetWaterLevel", funcGetWaterLevel );
	NATIVE_FUNCTION( "GetWaterDepth", funcGetWaterDepth );
	NATIVE_FUNCTION( "GetWaterTangent", funcGetWaterTangent );
	NATIVE_FUNCTION( "SweepTest", funcSweepTest );
	NATIVE_FUNCTION( "SphereOverlapTest", funcSphereOverlapTest );
	NATIVE_FUNCTION( "NavigationLineTest", funcNavigationLineTest );
	NATIVE_FUNCTION( "NavigationCircleTest", funcNavigationCircleTest );
	NATIVE_FUNCTION( "NavigationClosestObstacleToLine", funcNavigationClosestObstacleToLine );
	NATIVE_FUNCTION( "NavigationClosestObstacleToCircle", funcNavigationClosestObstacleToCircle );
	NATIVE_FUNCTION( "NavigationClearLineInDirection", funcNavigationClearLineInDirection );
	NATIVE_FUNCTION( "NavigationFindSafeSpot", funcNavigationFindSafeSpot );
	NATIVE_FUNCTION( "NavigationComputeZ", funcNavigationComputeZ );
	NATIVE_FUNCTION( "PhysicsCorrectZ", funcPhysicsCorrectZ );
	NATIVE_FUNCTION( "GetDepotPath", funcGetDepotPath );
	NATIVE_FUNCTION( "ForceGraphicalLOD", funcForceGraphicalLOD );
	NATIVE_FUNCTION( "GetTerrainParameters", funcGetTerrainParameters );
	NATIVE_FUNCTION( "GetTraceManager", funcGetTraceManager );
END_CLASS_RTTI();
