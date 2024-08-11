/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "world.h"

#include "globalWater.h"
#include "clipMap.h"
#include "pathlibWorld.h"
#include "cameraDirector.h"
#include "entityMotion.h"
#include "triggerManager.h"
#include "renderCommands.h"
#include "dynamicCollisionCollector.h"
#include "lensFlareSetupParameters.h"
#include "foliageScene.h"
#include "foliageEditionController.h"
#include "foliageDynamicInstanceService.h"
#include "worldLookup.h"
#include "mesh.h"
#include "sectorDataStreaming.h"
#include "renderVertices.h"

#include "mergedWorldGeometry.h"

#ifdef USE_UMBRA
#include "umbraScene.h"
#endif
#include "renderFence.h"
#include "streamingSectorData.h"
#include "viewport.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsSettings.h"
#include "../physics/physicsWrapper.h"
#include "traceTool.h"
#include "umbraTile.h"
#include "animationManager.h"
#include "soundFileLoader.h"
#include "soundSystem.h"
#include "environmentManager.h"
#include "weatherManager.h"
#include "componentIterator.h"
#include "renderProxy.h"
#include "worldIterators.h"
#include "dynamicLayer.h"
#include "selectionManager.h"
#include "tickManager.h"
#include "tagManager.h"
#include "layerInfo.h"
#include "layerGroup.h"
#include "materialInstance.h"
#include "environmentDefinition.h"
#include "sectorPrefetchRuntimeCache.h"

#include "../core/cooker.h"
#include "../core/scriptStackFrame.h"
#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/garbageCollector.h"
#include "../core/memoryFileReader.h"
#include "../core/feedback.h"
#include "../core/2darray.h"
#include "../core/gameConfiguration.h"
#include "../core/resourcepaths.h"
#include "../core/gatheredResource.h"
#include "../core/dependencyLoader.h"
#include "../core/versionControl.h"
#include "../core/depot.h"
#include "../core/depotBundles.h"
#include "../core/diskBundle.h"
#include "../core/dataError.h"
#include "../core/configVar.h"
#include "../redSystem/numericalLimits.h"
#include "baseEngine.h"
#include "foliageCollisionHandler.h"
#include "foliageResourceHandler.h"
#include "globalWaterUpdateParams.h"
#include "animationBufferBitwiseCompressed.h"
#include "physicsDataProviders.h"
#include "soundSettings.h"
#include "soundContactListener.h"
#include "../core/fileSystemProfilerWrapper.h"

#include "terrainTile.h" // for STerrainTileMipMap::HACK_BumpTerrainVersionCounter

IMPLEMENT_ENGINE_CLASS( SLensFlareGroupsParameters );
IMPLEMENT_RTTI_ENUM( ELensFlareGroup );
IMPLEMENT_RTTI_ENUM( EFlareCategory );

IMPLEMENT_ENGINE_CLASS( SWorldSkyboxParameters );
IMPLEMENT_ENGINE_CLASS( SWorldEnvironmentParameters );
IMPLEMENT_ENGINE_CLASS( CWorld );
IMPLEMENT_RTTI_ENUM( EWorldTransitionMode );


CGatheredResource resSkyboxTemplate( TXT("engine\\templates\\default_skybox.w2ent"), 0 );

namespace Config
{
	TConfigVar< Bool >		cvPreloadWorldData( "Streaming/World", "PreloadWorldData", true );
}

//////////////////////////////////////////////////////////////

WorldLoadingContext::WorldLoadingContext()
	: m_layerFilter( NULL )
	, m_dumpStats( false )
	, m_useDependencies( false )
{};

LayerGroupLoadingContext::LayerGroupLoadingContext()
	: m_layerFilter( NULL )
	, m_dumpStats( false )
	, m_loadHidden( false )
{};

//////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CWorldShadowConfig );

CWorldShadowConfig::CWorldShadowConfig()
	: m_numCascades( 3 )
	, m_cascadeRange1( 4.0f )
	, m_cascadeRange2( 18.0f )
	, m_cascadeRange3( 50.0f )
	, m_cascadeRange4( 120.0f )
	, m_cascadeFilterSize1( 0.1f )
	, m_cascadeFilterSize2( 0.1f )
	, m_cascadeFilterSize3( 0.2f )
	, m_cascadeFilterSize4( 0.3f )
	, m_shadowEdgeFade1( 1.0f )
	, m_shadowEdgeFade2( 4.0f )
	, m_shadowEdgeFade3( 16.0f )
	, m_shadowEdgeFade4( 32.0f )
	, m_shadowBiasOffsetSlopeMul( 1 )
	, m_shadowBiasOffsetConst( 0.02f )
	, m_shadowBiasOffsetConstCascade1( 0.f )
	, m_shadowBiasOffsetConstCascade2( 0.f )
	, m_shadowBiasOffsetConstCascade3( 0.f )
	, m_shadowBiasOffsetConstCascade4( 0.f )
	, m_shadowBiasCascadeMultiplier( 0.1f )
	, m_speedTreeShadowFilterSize1( -1 )
	, m_speedTreeShadowFilterSize2( -1 )
	, m_speedTreeShadowFilterSize3( -1 )
	, m_speedTreeShadowFilterSize4( -1 )
	, m_speedTreeShadowGradient( 0 )
	, m_hiResShadowBiasOffsetSlopeMul( DefaultHiResShadowBiasOffsetSlopeMul() )
	, m_hiResShadowBiasOffsetConst( DefaultHiResShadowBiasOffsetConst() )
	, m_hiResShadowTexelRadius( DefaultHiResShadowTexelRadius() )
	, m_useTerrainShadows( true )
	, m_terrainShadowsDistance( 1500.0f )
	, m_terrainShadowsBaseSmoothing( 5.0f )
	, m_terrainShadowsFadeRange( 200.0f )
	, m_terrainMeshShadowDistance( 80.0f )
	, m_terrainMeshShadowFadeRange( 20.0f )
	, m_terrainShadowsTerrainDistanceSoftness( 80.0f )
	, m_terrainShadowsMeshDistanceSoftness( 80.0f )
{
}

Float CWorldShadowConfig::DefaultHiResShadowBiasOffsetSlopeMul()
{
	return 1.5f;
}

Float CWorldShadowConfig::DefaultHiResShadowBiasOffsetConst()
{
	return 0.01f;
}

Float CWorldShadowConfig::DefaultHiResShadowTexelRadius()
{
	return 2;
}

SWorldEnvironmentParameters::SWorldEnvironmentParameters()
	: m_vignetteTexture( NULL )
	, m_cameraDirtTexture( NULL )
	, m_interiorFallbackAmbientTexture( NULL )
	, m_interiorFallbackReflectionTexture( NULL )
	, m_cameraDirtNumVerticalTiles( 1 )
	, m_toneMappingAdaptationSpeedUp( 15.f )	
	, m_toneMappingAdaptationSpeedDown( 15.f )
	, m_environmentDefinition( NULL )	
	, m_scenesEnvironmentDefinition( NULL )
	, m_disableWaterShaders( true )
{
}

SWorldSkyboxParameters::SWorldSkyboxParameters ()
	: m_sunMesh( nullptr )
	, m_sunMaterial( nullptr )
	, m_moonMesh( nullptr )
	, m_moonMaterial( nullptr )
	, m_skyboxMesh( nullptr )
	, m_skyboxMaterial( nullptr )
	, m_cloudsMesh( nullptr )
	, m_cloudsMaterial( nullptr )
{}

WorldInitInfo::WorldInitInfo()
	: m_previewWorld( false )
	, m_initializePhysics( true ) // always enabled by default - disable only to save memory
	, m_initializePathLib( false ) // not enabled by default
	, m_initializeOcclusion( false ) // not enabled by default
	, m_sharePhysicalWorld( nullptr )
{
}

//////////////////////////////////////////////////////////////

CWorld::WorldUpdateStateContext::WorldUpdateStateContext( CWorld* world, EWorldUpdatePhase newPhase )
	: m_world( world )
	, m_prevUpdatePhase( world->m_updatePhase )
{
	ASSERT( newPhase > WUP_None );

	// Check phase validity
	if ( newPhase <= world->m_updatePhase )
	{
		HALT( "World phase update error. DEBUG.");
		return;
	}

	// Set new phase
	world->m_updatePhase = newPhase;
}

CWorld::WorldUpdateStateContext::~WorldUpdateStateContext()
{
	m_world->m_updatePhase = m_prevUpdatePhase;
}

//////////////////////////////////////////////////////////////////////////

class CSectorDataStreamingGCFlusher : public IObjectGCHelper
{
public:
	CSectorDataStreamingGCFlusher( CSectorDataStreaming* streamingData )
		: m_streamingData( streamingData ) 
	{
	}

	virtual void OnGCStarting() override
	{
		CTimeCounter timer;

		LOG_ENGINE( TXT("Forcing streaming to finish because GC is about to be performed...") );
		m_streamingData->FinishStreaming();
		LOG_ENGINE( TXT("Streaming finished in %1.2fms"), timer.GetTimePeriodMS() );
	}

	virtual void OnGCFinished() override
	{
	}

private:
	CSectorDataStreaming*		m_streamingData;
};

//////////////////////////////////////////////////////////////////////////
const Float CWorld::DEFAULT_WORLD_DIMENSIONS = 8192;

//////////////////////////////////////////////////////////////////////////
CWorld::CWorld()
	: m_streamingSectorData( nullptr )
	, m_sectorStreaming( nullptr )
	, m_sectorStreamingGCFlusher( nullptr )
	, m_pathLib( NULL )
#ifdef USE_UMBRA
	, m_umbraScene( nullptr )
#endif
	, m_terrainClipMap( NULL )
	, m_globalWater( nullptr )
	, m_dynamicCollisions( nullptr )
#ifndef NO_EDITOR
	, m_enableStreaming( true )
#endif
	, m_worldDimensions( DEFAULT_WORLD_DIMENSIONS )
	, m_isPreviewWorld( false )
	, m_cameraPosition( Vector::ZERO_3D_POINT )
	, m_entityMotionManager( NULL )
	, m_lookupRegistry( NULL )
	, m_newLayerGroupFormat( false )
	, m_hasEmbeddedLayerInfos( false )
	, m_forcedGraphicalLOD( -1 )
	, m_gameStartInProgress( false )
	, m_sharedPhyscalWorldOwner( nullptr )
	, m_physicsWorld( nullptr )
	, m_physicsWorldSecondary( nullptr )
	, m_physicsBatchQueryManager( nullptr )
	, m_sectorPrefetch( nullptr )
	, m_lastEnvironmentManagerUpdatePos( 0, 0, 0, 1 )
	, m_taskBatch( 256 )
	, m_tickManagerNarrowScopeTaskBatch( 16 )
	, m_tickManagerBroadScopeTaskBatch( 16 )
{
}

CWorld::~CWorld()
{}

extern String GW3StreamingCacheAbsoluteFilePath;
extern String GEP2StreamingCacheAbsoluteFilePath;

void CWorld::Init( const WorldInitInfo& initInfo )
{
	// Setup preview world flag
	m_isPreviewWorld = initInfo.m_previewWorld;

	// Register listener
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( DetailMaterialRenderParamsChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( PostReloadBaseTree ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEndRequested ), this );
#endif

	// Initialize old path mapping
	if( !m_isPreviewWorld )
	{
		CDiskFile* worldFile = this->GetFile();
		RED_ASSERT( worldFile != nullptr, TXT( "World has no file associated with it. World streaming manager requires a world associated file") );

		// Ensure that we don't try and setup a streaming manager for a world which doesn't actually have an associated file.
		if( worldFile != nullptr )
		{
			Red::Core::ResourceManagement::CResourcePaths& pathManager = GEngine->GetPathManager();
			pathManager.Initialize( worldFile->GetDirectory(), worldFile->GetFileName() );

			const Bool isNovigrad = worldFile->GetFileName().ContainsSubstring( TXT("novigrad") );
			const Bool isBob = worldFile->GetFileName().ContainsSubstring( TXT("bob") );

			// Novigrad or BOB ? :) HARDCORE W3 HACK
			if ( !m_sectorPrefetch && ( isNovigrad || isBob ) ) 
			{
				// assemble path to the streaming cache
				// HARDCORE W3 HACK
				String streamingCacheFilePath = String::EMPTY;
				if( isNovigrad )
				{
					streamingCacheFilePath = GW3StreamingCacheAbsoluteFilePath;
					LOG_ENGINE( TXT("Using GW3StreamingCacheAbsoluteFilePath %ls"), streamingCacheFilePath.AsChar() );
				}
				if( isBob )
				{
					streamingCacheFilePath = GEP2StreamingCacheAbsoluteFilePath;
					LOG_ENGINE( TXT("Using GEP2StreamingCacheAbsoluteFilePath %ls"), streamingCacheFilePath.AsChar() );
				}		

				// Another W3 HACK: use the global memory buffer for prefetch
				extern class CSectorPrefetchMemoryBuffer* GetGlobalPrefetchBuffer();
				m_sectorPrefetch = new CSectorPrefetchRuntimeCache( *GetGlobalPrefetchBuffer() );
				if ( !m_sectorPrefetch->Initialize( streamingCacheFilePath ) )
				{
					delete m_sectorPrefetch;
					m_sectorPrefetch = nullptr;
				}
			}
		}
	}
	// Enumerate layers in the world
	InitializeLayersStructure();	

	// Initialize path lib stuff
	if ( m_pathLib )
	{
		m_pathLib->Initialize();
	}

	// Initialize selection manager
	m_selectionManager = new CSelectionManager( this );

	// Create tick manager
	m_tickManager = new CTickManager( this );

	// Create trigger manager
	m_triggerManager = ITriggerManager::CreateInstance();
	m_triggerManager->SetTerrainIntegration( static_cast< ITriggerSystemTerrainQuery* >( this ) );

	// Create trigger system activator for camera
	CTriggerActivatorInfo triggerInitInfo;
	triggerInitInfo.m_channels = TC_Camera;
	triggerInitInfo.m_component = NULL;
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	triggerInitInfo.m_debugName = TXT("Camera");
#endif
	triggerInitInfo.m_extents = Vector::ZEROS;
	triggerInitInfo.m_localToWorld = Matrix::IDENTITY;
	m_cameraActivator = m_triggerManager->CreateActivator(triggerInitInfo);

	// Tag manager
	m_tagManager = new CTagManager();
	if( !m_isPreviewWorld )
	{
		m_tagManager->ReserveBucket( 9 * 1024 );
	}

	// Create dynamic layer
	m_dynamicLayer = CreateObject< CDynamicLayer >( this );
	m_dynamicLayer->SetFlag( OF_Transient );
	m_dynamicLayer->AttachToWorld( this );

	// Create terrain manager
	m_atlasRegionsRegenerateRequested = false;
	m_atlasRegionsRegenerateForceNext = false;

	// Create environment manager
	m_environmentManager = new CEnvironmentManager( this );
	
	// Create rendering scene
	if ( GRender )
	{
		const Bool isWorldScene = !m_isPreviewWorld;
		m_renderSceneEx = GRender->CreateScene( isWorldScene );
	}

	// Create physical representation
	InitializePhysicalRepresentation( initInfo );

	// Initialize stremaing managers for this world
	InitializeStreaming();

	// Init skybox
	RefreshSceneSkyboxParams();

	// Init lens flares
	RefreshLensFlareParams();

	if( !m_foliageScene )
	{
		m_foliageScene = CreateObject< CFoliageScene >( this );
		m_foliageScene->SetWorldDimensions( Vector2( m_worldDimensions, m_worldDimensions ) );
	}

	m_foliageScene->Initialize( m_renderSceneEx );
	m_foliageEditionController = m_foliageScene->CreateEditionController();

	{ // find and instantiate Camera Director class
		CClass* cdClass = SRTTI::GetInstance().FindClass( CName( GGameConfig::GetInstance().GetCameraDirectorClassName() ) );
		RED_ASSERT( cdClass, TXT( "Camera Director class not found, check IGameConfiguration." ) );
		m_cameraDirector = CreateObject< CCameraDirector >( cdClass, this );
		RED_ASSERT( m_cameraDirector, TXT( "Camera Director could not be created" ) );
	}

	// Create entity motion manager
	m_entityMotionManager = new CEntityMotionManager( this );
	
	// Create dynamic collision manager
	m_dynamicCollisions = new CDynamicCollisionCollector( this );

	// Initialize this world water system		
	SetWaterVisible( m_environmentManager->GetGameEnvironmentParams().m_displaySettings.m_allowWaterShader );

	// Initialize LODding systems

	const SGameplayLODConfig& lodConfig = GGame->GetGameplayConfig().m_LOD;

	m_componentLODManager.SetTickManager( m_tickManager );
	m_componentLODManager.SetMaxUpdateTime( lodConfig.m_componentsTickLODUpdateTime );
	m_componentLODManager.SetDistances( lodConfig.m_componentsBudgetableTickDistance, lodConfig.m_componentsDisableTickDistance );

	m_effectTickLODManager.SetTickManager( m_tickManager );
	m_effectTickLODManager.SetMaxUpdateTime( lodConfig.m_effectsTickLODUpdateTime );
	m_effectTickLODManager.SetDistances( lodConfig.m_effectsBudgetableTickDistance, FLT_MAX /* unused */ );

	// Initialize new PathLib instance
	if ( initInfo.m_initializePathLib )
	{
		InitializePathLib();
	}

#ifdef USE_UMBRA
	// Initialize occlusion
	if ( initInfo.m_initializeOcclusion )
	{
		InitializeOcclusionCulling();
	}
#endif // USE_UMBRA
}

void CWorld::Shutdown()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( this );
#endif

	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Finish pending streaming
	if ( m_sectorStreaming )
	{
		m_sectorStreaming->FinishStreaming();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Cleanup pathlib stuff
	if ( m_pathLib )
	{
		m_pathLib->Shutdown();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

#ifdef USE_UMBRA
	if ( m_umbraScene )
	{
		m_umbraScene->Shutdown();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
#endif

	if ( m_sectorStreamingGCFlusher )
	{
		GObjectGC->UnregisterHelper( m_sectorStreamingGCFlusher );
		delete m_sectorStreamingGCFlusher;
		m_sectorStreamingGCFlusher = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Deleting the streaming system early unstreams all data
	if ( m_streamingSectorData )
	{
		delete m_streamingSectorData;
		m_streamingSectorData = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	if ( m_sectorStreaming )
	{
		delete m_sectorStreaming;
		m_sectorStreaming = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Detach dynamic layer
	if ( m_dynamicLayer )
	{
		m_dynamicLayer->DetachFromWorld();
		m_dynamicLayer->Discard();
		m_dynamicLayer = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Delete environment manager
	m_lastEnvironmentManagerUpdatePos = Vector ( 0, 0, 0, 1 );
	if ( m_environmentManager )
	{
		delete m_environmentManager;
		m_environmentManager = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Cleanup
	DelayedActions();
	PUMP_MESSAGES_DURANGO_CERTHACK();

	if( m_terrainClipMap )
	{
		m_terrainClipMap->InvalidateCollision();
		m_terrainClipMap->ClearTerrainProxy();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
	SetWaterVisible(false);
	PUMP_MESSAGES_DURANGO_CERTHACK();

	if( m_dynamicCollisions ) 
	{
		m_dynamicCollisions->Shutdown();
		delete m_dynamicCollisions;
		m_dynamicCollisions = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	if ( m_physicsBatchQueryManager )
	{
		delete m_physicsBatchQueryManager;
		m_physicsBatchQueryManager = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	m_controllerManager.Reset();

	// HACK APEX the apex stuff must be removed from the renderer before going to the physx world destroy
	GRender->Flush();
	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Delete tag manager
	if ( m_tagManager )
	{
		delete m_tagManager;
		m_tagManager = nullptr;
	}

	// Delete trigger system activator
	if ( m_cameraActivator )
	{
		m_cameraActivator->Remove();
		m_cameraActivator->Release();
		m_cameraActivator = nullptr;
	}

	PUMP_MESSAGES_DURANGO_CERTHACK();
	GSoundSystem->FlushListener();
	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Shut down trigger system
	if ( m_triggerManager )
	{
		delete m_triggerManager;
		m_triggerManager = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Shut down selection manager
	if ( m_selectionManager )
	{
		delete m_selectionManager;
		m_selectionManager = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Destroy foliage
	m_foliageEditionController.Reset();
	PUMP_MESSAGES_DURANGO_CERTHACK();

	if( m_foliageScene )
	{
		m_foliageScene->Shutdown();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Shut down physics
	if ( m_physicsWorld )
	{
		// Only destroy the world if we actually created it
		if ( IsPhysicalWorldOwned() )
		{
			GPhysicEngine->DestroyWorld( m_physicsWorld );
		}

		m_physicsWorld = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	if ( m_physicsWorldSecondary )
	{
		// Only destroy the world if we actually created it
		if ( IsPhysicalWorldOwned() )
		{
			GPhysicEngine->DestroyWorld( m_physicsWorldSecondary );
		}

		m_physicsWorldSecondary = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Delete tick manager
	if ( m_tickManager )
	{
		delete m_tickManager;
		m_tickManager = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Destroy camera manager
	if( m_cameraDirector )
	{
		m_cameraDirector->Discard();
		m_cameraDirector = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Destroy entity motion manager
	if ( m_entityMotionManager )
	{
		delete m_entityMotionManager;
		m_entityMotionManager = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Release rendering scene
	if ( m_renderSceneEx )
	{
		// Flush rendering thread before releasing last reference to render scene
		if ( GRender->GetRenderThread() )
		{
			PUMP_MESSAGES_DURANGO_CERTHACK();
			IRenderFence* fence = GRender->CreateFence();
			( new CRenderCommand_Fence( fence ) )->Commit();
			fence->FlushFence();
			fence->Release();
		}

		// Release reference
		m_renderSceneEx->Release();
		m_renderSceneEx = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	if ( m_worldLayers )
	{
		delete m_worldLayers;
		m_worldLayers = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Delete data prefetcher
	if ( m_sectorPrefetch )
	{
		delete m_sectorPrefetch;
		m_sectorPrefetch = nullptr;
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	if( !m_isPreviewWorld ) GSoundSystem->Reset();
	PUMP_MESSAGES_DURANGO_CERTHACK();

	FlushAllAnimationBuffer();
	PUMP_MESSAGES_DURANGO_CERTHACK();
}

#ifndef NO_EDITOR_WORLD_SUPPORT
void CWorld::OnEditorGameStopped()
{	
	// empty
}
#endif

#ifndef NO_EDITOR_RESOURCE_SAVE
void CWorld::OnResourceSavedInEditor()
{
	EDITOR_DISPATCH_EVENT( CNAME( ActiveWorldSaving ), NULL );
}
#endif

void CWorld::RefreshSceneSkyboxParams()
{
	SetupSceneSkyboxParams( m_environmentParameters.m_skybox );
}

void CWorld::GetStreamingStats( struct SSectorStreamingDebugData& outSectorStreamingStats ) const
{
	if ( m_sectorStreaming )
	{
		m_sectorStreaming->GetDebugInfo( outSectorStreamingStats );
	}
	else
	{
		Red::MemoryZero( &outSectorStreamingStats, sizeof(outSectorStreamingStats) );
	}
}

void CWorld::PullLayerGroupsAndLayerInfos()
{
	if ( m_worldLayers )
	{
		m_worldLayers->PullLayerGroupsAndLayerInfos();
	}

	m_hasEmbeddedLayerInfos = true;
}

Bool CWorld::GetLayerGroupInitialVisibility( const CLayerGroup* layerGroup ) const
{
	String layerGroupPath;
	layerGroup->GetLayerGroupPath( layerGroupPath );
	layerGroupPath.MakeLower();

	C2dArray* csv = m_initialyHidenLayerGroups.Get();

	// Use the DLC-specific CSV file for layergroups under DLC
	if ( layerGroup->GetParentGroup() != nullptr && layerGroup->GetParentGroup()->GetParentGroup() != nullptr && layerGroup->GetParentGroup()->GetParentGroup()->IsDLC() )
	{
		// Find the layer group right below DLC 
		const CLayerGroup* dlcGroup = layerGroup;
		while ( dlcGroup->GetParentGroup()->GetParentGroup()->IsDLC() )
		{
			dlcGroup = dlcGroup->GetParentGroup();
		}
		// Load/find the resource (here the assumption is that there will not be many layer groups in DLCs that add new groups to existing worlds)
		csv = Cast< C2dArray >( GDepot->LoadResource( String::Printf( TXT("dlc\\%ls\\levels\\%ls_lg.csv"), dlcGroup->GetName().AsChar(), GetFile()->GetDirectory()->GetName().AsChar() ) ).Get() );
		if ( csv == nullptr )
		{
			csv = m_initialyHidenLayerGroups.Get();
		}
	}

	Bool isHidden = false;
	if ( csv != nullptr )
	{
		const Uint32 numRows = csv->GetNumberOfRows();
		for ( Uint32 i=0; i<numRows; ++i )
		{
			const String& row = csv->GetValueRef(0,i);
			if ( row == layerGroupPath )
			{
				isHidden = true;
				break;
			}
		}
	}

	return !isHidden;
}

Bool CWorld::SetLayerGroupInitialVisibility( const CLayerGroup* layerGroup, const Bool flag )
{
	String layerGroupPath;
	layerGroup->GetLayerGroupPath( layerGroupPath );
	layerGroupPath.MakeLower();
	
	C2dArray* csv = m_initialyHidenLayerGroups.Get();

	// Use the DLC-specific CSV file for layergroups under DLC
	if ( layerGroup->GetParentGroup() != nullptr && layerGroup->GetParentGroup()->GetParentGroup() != nullptr && layerGroup->GetParentGroup()->GetParentGroup()->IsDLC() )
	{
		// Find the layer group right below DLC 
		const CLayerGroup* dlcGroup = layerGroup;
		while ( dlcGroup->GetParentGroup()->GetParentGroup()->IsDLC() )
		{
			dlcGroup = dlcGroup->GetParentGroup();
		}
		csv = Cast< C2dArray >( GDepot->LoadResource( String::Printf( TXT("dlc\\%ls\\levels\\%ls_lg.csv"), dlcGroup->GetName().AsChar(), GetFile()->GetDirectory()->GetName().AsChar() ) ).Get() );
		if ( csv == nullptr )
		{
#ifndef NO_EDITOR
			// Failed to load the csv, try to create it
			csv = new C2dArray();
			csv->AddColumn( TXT("LayerGroupPath"), TXT("") );
			if ( !csv->SaveAs( GDepot->FindPath( String::Printf( TXT("dlc\\%ls\\levels\\"), dlcGroup->GetName().AsChar() ).AsChar() ), String::Printf( TXT("%ls_lg.csv"), GetFile()->GetDirectory()->GetName().AsChar() ) ) )
			{
				GFeedback->ShowError( TXT("Failed to create the layer visibility CSV file for the DLC") );
				return false;
			}
#else
			// Nothing will be saved, so use the world csv
			csv = m_initialyHidenLayerGroups.Get();
#endif
		}
	}

	if ( csv != nullptr )
	{
#ifndef NO_EDITOR
		// get neweset content and reload it
		if ( csv->GetFile() )
		{
			if ( csv->GetFile()->Sync() )
			{
				// reload data
				C2dArray* tempArray = C2dArray::CreateFromString( csv->GetFile()->GetAbsolutePath() );
				if ( tempArray != nullptr )
				{
					csv->SetData( tempArray->GetData() );
					tempArray->Discard();
				}
			}
		}
#endif

		// modify the data
		Bool wasModified = false;
		if ( flag )
		{
			const Uint32 numRows = csv->GetNumberOfRows();
			for ( Uint32 i=0; i<numRows; ++i )
			{
				const String& row = csv->GetValue(0,i);
				if ( row == layerGroupPath )
				{
					// we want to go visible - remove this row
					if ( !csv->MarkModified() )
					{
						return false;
					}

					// remove the row
					csv->DeleteRow( i );
					wasModified = true;
				}
			}
		}
		else // we want to go hidden
		{
			// make sure it does not exist already
			Bool isHiden = false;
			const Uint32 numRows = csv->GetNumberOfRows();
			for ( Uint32 i=0; i<numRows; ++i )
			{
				const String& row = csv->GetValue(0,i);
				if ( row == layerGroupPath )
				{
					isHiden = true;
					break;
				}
			}

			// add to list
			if ( !isHiden )
			{
				// checkout the file
				if ( !csv->MarkModified() )
				{
					return false;
				}

				// find the best place to insert the row (so the list is kind of sorted)
				// this allows easy merging of the CSV file
				Uint32 insertPosition = 0;
				while ( insertPosition < numRows )
				{
					const String& row = csv->GetValue(0,insertPosition);
					if ( Red::StringCompare( row.AsChar(), layerGroupPath.AsChar() ) > 0 )
					{
						break;
					}

					insertPosition++;
				}

				// insert string in the position
				csv->InsertRow( insertPosition, layerGroupPath );
				wasModified = true;
			}
		}

		// save and check in
#ifndef NO_EDITOR
		if ( wasModified )
		{
			// save the file
			if ( csv->Save() )
			{
				// auto check in
				if ( m_newLayerGroupFormat )
				{
					if ( csv->GetFile() )
					{
						csv->GetFile()->Submit( 
							String::Printf( TXT("LayerGroup initial visibility change for group '%ls'"), layerGroupPath.AsChar() ) );
					}
				}
			}
		}
#endif
	}

	// Done
	return true;
}

void CWorld::RefreshLensFlareParams()
{
	const SLensFlareGroupsParameters &groups = m_environmentParameters.m_lensFlare;

	SLensFlareGroupsSetupParameters setupParams;

	for ( Uint32 group_i=0; group_i<LFG_MAX; ++group_i )
	{
		SLensFlareSetupParameters &setup = setupParams.m_groups[group_i];
		const SLensFlareParameters &flareParams = groups.GetGroupParams( (ELensFlareGroup)group_i );

		setup.m_nearDistance	= Max( 0.f, flareParams.m_nearDistance );
		setup.m_nearInvRange	= 1.f / Max( 0.001f, flareParams.m_nearRange );
		setup.m_farDistance		= Max( 0.f, flareParams.m_farDistance );
		setup.m_farInvRange		= 1.f / Max( 0.001f, flareParams.m_farRange );

		setup.m_elements.Reserve( flareParams.m_elements.Size() );
		for ( Uint32 elem_i=0; elem_i<flareParams.m_elements.Size(); ++elem_i )
		{
			const SLensFlareElementParameters &elementParams = flareParams.m_elements[elem_i];
			
			IRenderResource *currMaterialResource = elementParams.m_material && elementParams.m_material->GetBaseMaterial() ? elementParams.m_material->GetBaseMaterial()->GetRenderResource() : nullptr;
			IRenderResource *currMaterialParamsResource = elementParams.m_material ? elementParams.m_material->GetRenderResource() : nullptr;
			if ( nullptr == currMaterialResource || nullptr == currMaterialParamsResource || elementParams.m_size <= 0 )
			{
				continue;
			}

			setup.m_elements.PushBack( SLensFlareElementSetupParameters () );
			SLensFlareElementSetupParameters &elementSetup = setup.m_elements.Back();

			elementSetup.m_materialResource = currMaterialResource;
			elementSetup.m_materialParamsResource = currMaterialParamsResource;
			elementSetup.m_isConstRadius = elementParams.m_isConstRadius;
			elementSetup.m_isAligned = elementParams.m_isAligned;
			elementSetup.m_centerFadeStart = elementParams.m_centerFadeStart;
			elementSetup.m_centerFadeRange = elementParams.m_centerFadeRange;
			elementSetup.m_colorGroupParamsIndex = elementParams.m_colorGroupParamsIndex;
			elementSetup.m_alpha = elementParams.m_alpha;
			elementSetup.m_size = elementParams.m_size;
			elementSetup.m_aspect = elementParams.m_aspect;
			elementSetup.m_shift = elementParams.m_shift;
			elementSetup.m_pivot = elementParams.m_pivot;
			elementSetup.m_colorLinear = elementParams.m_color.ToVectorLinear();

			elementSetup.AddRefAll();
		}
	}

	( new CRenderCommand_LensFlareSetup( m_renderSceneEx, setupParams ) )->Commit();
}

void CWorld::SetupSceneSkyboxParams( const SWorldSkyboxParameters &params )
{
	struct Local
	{
		static void Extract( CMesh *mesh, CMaterialInstance *materialInstance, IRenderResource *&outMeshResource, IRenderResource *&outMaterialResource, IRenderResource *&outMaterialParamsResource )
		{
			outMeshResource = mesh ? mesh->GetRenderResource() : nullptr;
			outMaterialResource = materialInstance && materialInstance->GetBaseMaterial() ? materialInstance->GetBaseMaterial()->GetRenderResource() : nullptr;
			outMaterialParamsResource = materialInstance ? materialInstance->GetRenderResource() : nullptr;
		}
	};

	if ( !m_renderSceneEx )
	{
		return;
	}

	SSkyboxSetupParameters setupParams;
	Local::Extract( params.m_sunMesh.Get(), params.m_sunMaterial.Get(), setupParams.m_sunMeshResource, setupParams.m_sunMaterialResource, setupParams.m_sunMaterialParamsResource );
	Local::Extract( params.m_moonMesh.Get(), params.m_moonMaterial.Get(), setupParams.m_moonMeshResource, setupParams.m_moonMaterialResource, setupParams.m_moonMaterialParamsResource );
	Local::Extract( params.m_skyboxMesh.Get(), params.m_skyboxMaterial.Get(), setupParams.m_skyboxMeshResource, setupParams.m_skyboxMaterialResource, setupParams.m_skyboxMaterialParamsResource );
	Local::Extract( params.m_cloudsMesh.Get(), params.m_cloudsMaterial.Get(), setupParams.m_cloudsMeshResource, setupParams.m_cloudsMaterialResource, setupParams.m_cloudsMaterialParamsResource );
	setupParams.AddRefAll();

	( new CRenderCommand_SkyboxSetup( m_renderSceneEx, setupParams ) )->Commit();
}

void CWorld::SetEnvironmentParameters( const SWorldEnvironmentParameters& params )
{
	m_environmentParameters = params;

	if ( m_environmentManager )
	{
		// Force new settings
		if ( params.m_environmentDefinition )
		{
			m_environmentManager->EnableForcedAreaEnvironment( params.m_environmentDefinition->GetAreaEnvironmentParams() );
		}
		else
		{
			CAreaEnvironmentParams defaults( EnvResetMode_CurvesDefault );
			m_environmentManager->EnableForcedAreaEnvironment( defaults );
		}
		m_environmentManager->DisableForceAreaEnvironment();
	}

	RefreshSceneSkyboxParams();
	RefreshLensFlareParams();
}

bool CWorld::IsPositionAboveTerrain( const Vector& position ) const
{
	if ( NULL != m_terrainClipMap )
	{
		// Get the height at whatever clipmap level is loaded
		Float height;
		m_terrainClipMap->GetHeightForWorldPosition( position, height );
		if ( position.Z < height )
		{
			// below terrain
			return false;
		}
	}

	// assume above terrain
	return true;
}

void CWorld::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// Serialize dynamic layer for GC
	if ( file.IsGarbageCollector() )
	{
		if ( m_worldLayers != nullptr )
		{
			m_worldLayers->OnSerialize( file );
		}

		file << m_dynamicLayer;
		file << m_cameraDirector;
		file << m_preloadedData;

		for ( TDynArray<CLayerInfo*>::iterator it=m_needsUpdating.Begin(); it != m_needsUpdating.End(); ++it )
		{
			file << *it;
		}

		// Ask the environment manager to serialize its objects
		if ( m_environmentManager )
		{
			m_environmentManager->SerializeForGC( file );
		}

		// Ask the entity motion manager to serialize its objects
		if ( m_entityMotionManager )
		{
			m_entityMotionManager->SerializeForGC( file );
		}

		return;
	}

	// Layer structure, saved only for cooked builds
	if ( m_hasEmbeddedLayerInfos )
	{
		file << m_worldLayers;
	}
}

#ifndef NO_RESOURCE_COOKING
void CWorld::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	// enumerate layer group/layer info structure
	if ( !m_worldLayers )
		EnumerateLayersStructure();

	// pull existing layer infos and save them inside the CWorld
	PullLayerGroupsAndLayerInfos();

	// report all layers as soft dependencies of the world
	if ( m_worldLayers )
	{
		// get all of the layers
		TDynArray< CLayerInfo* > allLayers;
		allLayers.Reserve( 10000 );
		m_worldLayers->GetLayers( allLayers, false, true );

		// report them as dependencies
		// always visible layers are reported as HARD dependencies
		// streamable or quest layers are reported as SOFT dependencies
		for ( CLayerInfo* layer : allLayers )
		{
			if ( layer->IsEnvironment() )
			{
				LOG_ENGINE( TXT("Reporting layer '%ls' as hard dependency"), layer->GetDepotPath().AsChar() );
				cooker.ReportHardDependency( layer->GetDepotPath() );
			}
			else
			{
				LOG_ENGINE( TXT("Reporting layer '%ls' as soft dependency"), layer->GetDepotPath().AsChar() );
				cooker.ReportSoftDependency( layer->GetDepotPath() );
			}
		}
	}

	// report all minimap and hubmap textures as soft dependencies
	TDynArray< String > mapPaths;

	if( m_minimapsPath.Empty() == false )
	{
		GFileManager->FindFilesRelative( GFileManager->GetDataDirectory(), m_minimapsPath, TXT( "*.jpg" ), mapPaths, true );
		GFileManager->FindFilesRelative( GFileManager->GetDataDirectory(), m_minimapsPath, TXT( "*.png" ), mapPaths, true );
	}

	if( m_hubmapsPath.Empty() == false )
	{
		GFileManager->FindFilesRelative( GFileManager->GetDataDirectory(), m_hubmapsPath, TXT( "*.jpg" ), mapPaths, true );
		GFileManager->FindFilesRelative( GFileManager->GetDataDirectory(), m_hubmapsPath, TXT( "*.png" ), mapPaths, true );
	}

	for( String& path : mapPaths )
	{
		cooker.ReportSoftDependency( path );
	}
}
#endif

#ifndef NO_DATA_VALIDATION
void CWorld::OnCheckDataErrors() const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors();

	if ( m_worldLayers )
	{
		const CLayerGroup::TLayerList& layers = m_worldLayers->GetLayers();
		for ( auto it = layers.Begin(); it != layers.End(); ++it )
		{
			if ( (*it)->GetLayer() )
			{
				(*it)->GetLayer()->OnCheckDataErrors();
			}
		}

		const CLayerGroup::TGroupList& groups = m_worldLayers->GetSubGroups();
		GFeedback->BeginTask( TXT( "Checking data errors" ), true );
		Uint32 counter = 0;
		for ( auto it = groups.Begin(); it != groups.End(); ++it, ++counter )
		{
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			GFeedback->UpdateTaskProgress( counter, groups.Size() );
			(*it)->OnCheckDataErrors();
		}
		GFeedback->EndTask();
	}
	else
	{
		DATA_HALT( DES_Minor, this, TXT("World"), TXT( "World has no layers" ) );
	}
}
#endif

void CWorld::EnumerateLayersStructure()
{
	ASSERT( !m_worldLayers );

	// Dynamic generation of layer map is possible only when we have direct mapping to a file
	CDiskFile* file = GetFile();
	if ( !file )
	{
		return;
	}

	// Hacky stuff in order to prevent the modification of the W2W files
	// Do not modify the world resource, instead try to open/create side-by-side CVS file 
	Bool layerDataImported = false;
	if ( !m_initialyHidenLayerGroups )
	{
		// format the file name
		CFilePath filePath( file->GetDepotPath() );
		filePath.SetFileName( filePath.GetFileName() + TXT("_lg") );
		filePath.SetExtension( ResourceExtension< C2dArray >() );
		const String layerGroupFile = filePath.ToString();

		// load existing file
		m_initialyHidenLayerGroups = LoadResource< C2dArray >( layerGroupFile );

		// create new CVS file
		if ( !m_initialyHidenLayerGroups.IsValid() )
		{
			m_initialyHidenLayerGroups = new C2dArray();
			m_initialyHidenLayerGroups->AddColumn( TXT("LayerGroupPath"), TXT("") );

#ifndef NO_RESOURCE_IMPORT
			const String layerGroupShortName = filePath.GetFileNameWithExt();
			m_initialyHidenLayerGroups->SaveAs( file->GetDirectory(), layerGroupShortName ); // this will create the file
#endif
			layerDataImported = true;
		}
		else
		{
			// file loaded, conversion must have happened before
			m_newLayerGroupFormat = true;
		}
	}
	else
	{
		// file is linked (cooked data)
		m_newLayerGroupFormat = true;
	}

	// Initialize layer map
	const String& depotPath = file->GetDirectory()->GetDepotPath();
	const String& absolutePath = file->GetDirectory()->GetAbsolutePath();
	m_worldLayers = new CLayerGroup( this, NULL, TXT("World"), depotPath, absolutePath );
	{
		CTimeCounter timer;
		m_worldLayers->Populate();
		LOG_ENGINE( TXT("Repopulating world layers took: %1.2fms"), timer.GetTimePeriodMS() );
	}

	// Conversion has happened
	m_newLayerGroupFormat = true;

	// Check in
#ifndef NO_EDITOR
	if ( m_initialyHidenLayerGroups.IsValid() && layerDataImported )
	{
		if ( m_initialyHidenLayerGroups->GetFile() )
		{
			m_initialyHidenLayerGroups->GetFile()->Submit( String::Printf( TXT("Automatic LayerGroup visibility conversion for world '%ls'"), file->GetDepotPath().AsChar() ) );
		}
	}
#endif
}

Bool CWorld::PreventCollectingResource() const
{
	return false;
}

#ifndef NO_EDITOR_WORLD_SUPPORT

Bool CWorld::SaveLayers()
{
	// Save layers
	m_worldLayers->Save( true, false );
	return true;
}

#endif

#ifndef NO_EDITOR

void CWorld::EnableStreaming( Bool enable, Bool forceLoad )
{
	m_enableStreaming = enable;
}

Bool ContainsLogic( const EntitySpawnInfo& spawnInfo )
{
	//return spawnInfo.
	return true;
}

CEntity* CWorld::CreateEntity( const EntitySpawnInfo& spawnInfo )
{
	RED_HALT( "This should NEVER be called, i will remove it later");
	return nullptr;
}

Bool CWorld::PasteEntities( CLayer* layer, const TDynArray< Uint8 >& data, const TDynArray< Uint8 >& infoData, TDynArray< CEntity* >& pastedEntities, Bool relativeSpawn, const Vector& spawnPosition, const EulerAngles& spawnRotation )
{
	// Deserialize
	TDynArray< CObject* > objects;
	CMemoryFileReader reader( data, 0 );
	CDependencyLoader loader( reader, NULL );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = NULL;
	if ( !loader.LoadObjects( loadingContext ) )
	{
		// No objects loaded
		return false;
	}

	// Get spawned entities
	TDynArray< CEntity* > entities;
	for ( Uint32 i=0; i<loadingContext.m_loadedRootObjects.Size(); i++ )
	{
		CEntity* object = Cast< CEntity >( loadingContext.m_loadedRootObjects[i] );
		if ( object )
		{
			entities.PushBack( object );
		}
	}

	// Call post load of spawned objects
	loader.PostLoad();

	// Offset root components by specified spawn point
	if ( relativeSpawn )
	{
		// No explicit pivot, try using root transform component
		CEntity* localPivot = entities[0];
		ASSERT( localPivot );
		Vector localPivotPos = localPivot->GetWorldPosition();

		// Transform components
		for ( Uint32 i=0; i<entities.Size(); i++ )
		{
			// Set initial position
			CEntity* entity = entities[i];
			entity->SetPosition( spawnPosition + entity->GetWorldPosition() - localPivotPos );

			// Force transform update
			entity->ForceUpdateTransformNodeAndCommitChanges();
			entity->ForceUpdateBoundsNode();
		}
	}

	for ( Uint32 entityIndex=0; entityIndex<entities.Size(); entityIndex++ )
	{
		CEntity* entity = entities[entityIndex];
		entity->SetName( layer->GenerateUniqueEntityName( entity->GetName() ) );
		layer->AddEntity( entity );
		entity->OnPasted( layer );
	}

	pastedEntities = entities;
	return true;
}

#endif

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

void CWorld::GetAttachedLayers_DeprecatedDoNotUse( TDynArray< CLayer* >& layers )
{
	RED_FATAL_ASSERT( 0, "stop immediately on any attempt of calling such a stupid function" );

	// DO NO, DO NOT, DO NOT!!!!
	// use iterators instead!
	//layers = m_attachedLayers;
}

void CWorld::GetAttachedEntities_DeprecatedDoNotUse( TDynArray< CEntity* >& entities )
{	  
	// crash immediately on any attempt of calling such a stupid function 
	RED_FATAL_ASSERT( 0, "stop immediately on any attempt of calling such a stupid function" );

	// DO NO, DO NOT, DO NOT!!!!
	// DON'T YOU EVER, NEVER, COMMIT SUCH A CRIME!!!
	/*
	TDynArray< CLayer* > layers;
	layers.Reserve( 512 );

	GetAttachedLayers( layers );

	// For each layer
	for ( Uint32 i=0; i<layers.Size(); i++ )
	{
		TDynArray< CEntity* > localEntities;
		localEntities.Reserve( 512 );

		layers[i]->GetEntities( localEntities );

		// Merge
		entities.PushBack( localEntities );
	}
	*/
}

void CWorld::GetAttachedComponents_DeprecatedDoNotUse( TDynArray< CComponent* >& components )
{
	RED_FATAL_ASSERT( 0, "stop immediately on any attempt of calling such a stupid function" );

	// DO NO, DO NOT, DO NOT!!!!
	// DON'T YOU EVER, NEVER, COMMIT SUCH A CRIME!!!
	/*for ( CComponent* cur = m_allAttachedComponents; cur; cur=cur->GetNextAttachedComponent() )
	{
		components.PushBack( cur );
	}*/
}

//**************************************************************************************************

#ifndef NO_EDITOR_FRAGMENTS

void CWorld::GenerateEditorHitProxies( CHitProxyMap& hitProxyMap )
{
	// Generate editor hit proxies
	for ( WorldAttachedComponentsIterator it( this ); it; ++it )
	{
		( *it )->OnGenerateEditorHitProxies( hitProxyMap );
	}
}

void CWorld::GenerateEditorFragments( CRenderFrame* frame )
{
	PC_SCOPE_PIX( GenerateEditorFragments );

	// Generate custom fragments
	m_editorFragmentsFilter.GenerateEditorFragments( frame );

#ifdef FIT_TO_CASCADES
	//dex++: cascade debug
	extern void DrawCascadeFrustums( CRenderFrame* frame );
	DrawCascadeFrustums( frame );
	//dex--
#endif
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_DynamicCollector ) )
	{
		m_dynamicCollisions->GenerateEditorFragments( frame );
	}
	// 2D editor grid
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Grid ) )
	{
		// Line buffer
		static TDynArray< DebugVertex > lines;
		lines.ClearFast();

		// Color
		const Color gridColor( 0, 0, 80 );
		const Color axisColor( 0, 0, 127 );

		// Generate lines
		const float extents = 200.0f;
		for ( Float y=-extents; y<=extents; y+=10.0f )
		{
			for ( Float x=-extents; x<=extents; x+=10.0f )
			{
				lines.PushBack( DebugVertex( Vector( x, -extents, 0.0f ), gridColor ) );
				lines.PushBack( DebugVertex( Vector( x, extents, 0.0f ), gridColor ) );
				lines.PushBack( DebugVertex( Vector( -extents, y, 0.0f ), gridColor ) );
				lines.PushBack( DebugVertex( Vector( extents, y, 0.0f ), gridColor ) );
			}
		}

		// Center lines
		lines.PushBack( DebugVertex( Vector( 0.0f, -extents, 0.0f ), axisColor ) );
		lines.PushBack( DebugVertex( Vector( 0.0f, extents, 0.0f ), axisColor ) );
		lines.PushBack( DebugVertex( Vector( -extents, 0.0f, 0.0f ), axisColor ) );
		lines.PushBack( DebugVertex( Vector( extents, 0.0f, 0.0f ), axisColor ) );

		// Draw
		frame->AddDebugLines( &lines[0], lines.Size(), false );
	}
	if ( NULL != m_triggerManager )
	{
		m_triggerManager->Render( frame );
	}

	if ( m_pathLib )
	{
		m_pathLib->GenerateEditorFragments( frame );
	}

#ifdef USE_UMBRA
	if ( m_umbraScene )
	{
		m_umbraScene->GenerateEditorFragments( frame );
	}
#endif

	// Display world time scale if it is other than 1.f
	Float timeScale = GGame->GetTimeScale();
	if ( timeScale != 1.f )
	{
		const Int32 x = frame->GetFrameOverlayInfo().m_width - 100;
		frame->AddDebugScreenFormatedText( x, 20, Color::RED, TXT("Time scale: %1.2fx"), timeScale );
	}

	// Generate environment fragments
	if ( m_environmentManager )
	{
		m_environmentManager->GenerateEditorFragments( frame );
	}

	GAnimationManager->GenerateEditorFragments( frame );

	if( m_cameraDirector )
	{
		m_cameraDirector->GenerateEditorFragments( frame );
	}

	// Show the entity visibility statistics
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_EntityVisibility ) )
	{
		Uint32 numEntitiesVisibile = 0;
		Uint32 numEntitiesPartialyVisibile = 0;
		Uint32 numEntitiesNotVisible = 0;
		Uint32 numEntitiesNotTested = 0;

		for ( CLayer* attachedLayer : m_attachedLayers )
		{
			for ( CEntity* entity : attachedLayer->GetEntities() )
			{
				const ERenderVisibilityResult visibility = entity->GetLastFrameVisibility();
				if ( visibility == RVR_Visible )
				{
					numEntitiesVisibile += 1;
				}
				else if ( visibility == RVR_PartialyVisible )
				{
					numEntitiesPartialyVisibile += 1;
				}
				else if ( visibility == RVR_NotVisible )
				{
					numEntitiesNotVisible += 1;
				}
				else if ( visibility == RVR_NotTested )
				{
					numEntitiesNotTested += 1;
				}
			}
		}

		// legend + count
		const Int32 spaceWidth = 14;
		const Int32 leftMargin = 30;
		const Int32 upMargin = 7;
		const Int32 topWindow = 110;
		frame->AddDebugRect( leftMargin-2, topWindow - 4, 120, 2*upMargin + (4 * spaceWidth), Color( 0, 0, 0 )  );
		frame->AddDebugScreenText( leftMargin+7, topWindow + 1*spaceWidth, String::Printf( TXT("Visible: %d"), numEntitiesVisibile ), Color::RED );
		frame->AddDebugScreenText( leftMargin+7, topWindow + 2*spaceWidth, String::Printf( TXT("ShadowsOnly: %d"), numEntitiesPartialyVisibile ), Color::YELLOW );
		frame->AddDebugScreenText( leftMargin+7, topWindow + 3*spaceWidth, String::Printf( TXT("NotVisible: %d"), numEntitiesNotVisible ), Color::GRAY );
		frame->AddDebugScreenText( leftMargin+7, topWindow + 4*spaceWidth, String::Printf( TXT("NotTested: %d"), numEntitiesNotTested ), Color::DARK_CYAN );
	}

	// Allow engine to create some debug fragments
	GEngine->GenerateDebugFragments( frame );
	PhysicsGenerateEditorFragments( frame,this );
}

#endif

void CWorld::RequestAtlasRegionsRegeneration( bool forceRegenerate )
{
	m_atlasRegionsRegenerateRequested = true;
	m_atlasRegionsRegenerateForceNext = (m_atlasRegionsRegenerateForceNext || forceRegenerate);
}

#ifndef NO_EDITOR_EVENT_SYSTEM
void CWorld::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( DetailMaterialRenderParamsChanged ) )
	{
		RequestAtlasRegionsRegeneration( false );
	}
	else if ( name == CNAME( PostReloadBaseTree ) )
	{
		if ( GetTerrain() )
		{
			GetTerrain()->UpdateGrassRendering();
		}
	}
	else if ( name == CNAME( GameEndRequested ) )
	{
		CWeatherManager *weatherMgr = GetEnvironmentManager() ? GetEnvironmentManager()->GetWeatherManager() : nullptr;
		RED_ASSERT( weatherMgr );
		if ( weatherMgr )
		{
			weatherMgr->ResetPlayerIsInInterior();
		}
	}
}
#endif

Bool CWorld::CheckForSegmentIntersection( const Vector& a, const Vector& b, Vector& worldCoords, Vector& worldNormal, Bool snapOnlyToHeightmap /* = false */ )
{
	SPhysicsContactInfo cinfo;

	// Trace geometry
	Bool collided = false;
	if ( !snapOnlyToHeightmap )
	{
		STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) );
		collided = m_physicsWorld->RayCastWithSingleResult( a, b, include, 0, cinfo ) == TRV_Hit;
		if ( collided )
		{
			// First check if physics trace hits anything
			worldCoords = cinfo.m_position;
			worldNormal = cinfo.m_normal;
		}
	}

#if 0 // Hey tomasz, i commented this out because it was breaking entity
	// placement, quickjumping (via J) and rotating around a point in
	// the world (via Z + mouse drag).  When you fix it, please try to
	// make sure these work properly                        -badsector
	if ( ( dir.Z < 0 ) && ( -origin.Z / dir.Z < RAY_DISTANCE ) )
	{
		// Check if trace hits XY plane
		Float dist = -origin.Z / dir.Z;

		worldCoords = origin + ( dir * dist );
		if ( worldNormal != NULL )
		{
			*worldNormal = Vector::EZ;
		}
		return true;
	}
#endif

	if ( m_terrainClipMap )
	{
		Vector terrainWorldCoords;
		Vector dir = ( b - a ).Normalized3();
		Bool terrainCollided = m_terrainClipMap->Intersect( a, dir, terrainWorldCoords );
		if ( terrainCollided )
		{
			if ( !collided || ( collided && a.DistanceSquaredTo( terrainWorldCoords ) < a.DistanceSquaredTo( worldCoords ) ) )
			{
				worldCoords = terrainWorldCoords;

				Vector p[3];
				p[0] = Vector( worldCoords.X, worldCoords.Y - 0.1f, worldCoords.Z );
				p[1] = Vector( worldCoords.X + 0.1f, worldCoords.Y + 0.1f, worldCoords.Z );
				p[2] = Vector( worldCoords.X - 0.1f, worldCoords.Y + 0.1f, worldCoords.Z );
				for ( Uint32 i=0; i<3; ++i )
				{
					m_terrainClipMap->GetHeightForWorldPosition( Vector( p[i].X, p[i].Y, p[i].Z ), p[i].Z );
				}
				Vector ta = p[1] - p[0];
				Vector tb = p[2] - p[0];
				worldNormal = worldNormal.Cross( ta, tb ).Normalized3();
			}
		}
		collided = collided || terrainCollided;
	}

	return collided;
}

Bool CWorld::ConvertScreenToWorldCoordinates( IViewport * view, Int32 screenX, Int32 screenY, Vector & worldCoords, Vector * worldNormal /*= NULL*/, Bool snapOnlyToHeightmap /*= false*/ )
{
	// Calculate ray
	Vector origin, dir, normal;
	view->CalcRay( screenX, screenY, origin, dir );

	// Perform collision check
	const Float RAY_DISTANCE = 1000.f;
	Bool r = CheckForSegmentIntersection( origin, origin + dir * RAY_DISTANCE, worldCoords, normal, snapOnlyToHeightmap );

	// Give back the world normal, if needed
	if ( r && worldNormal )
	{
		*worldNormal = normal;
	}

	// Return result
	return r;
}

THandle< CWorld > CWorld::LoadWorld( const String& depotPath, WorldLoadingContext& context )
{
	CTimeCounter loadingTimer;

	// Cooked game - load the initial package first
	THandle< CDiskBundleContent > worldStartupContent;
	if ( GFileManager->IsReadOnly() && Config::cvPreloadWorldData.Get() )
	{
		const CFilePath worldDepotPath( depotPath );
		const StringAnsi worldShortName( UNICODE_TO_ANSI( worldDepotPath.GetDirectories().Back().AsChar() ) );

		// enable the bundles for selected world
		GDepot->GetBundles()->SwitchWorldBundles( worldShortName );

		// find startup package
		CDiskBundle* startupBundle = GDepot->GetBundles()->GetStartupBundle( worldShortName );
		if ( !startupBundle )
		{
			WARN_ENGINE( TXT("No startup bundle for world '%hs'"), worldShortName.AsChar() );
		}
		else
		{
			// preload the startup data for the world
			worldStartupContent = startupBundle->Preload();
			if ( worldStartupContent )
			{
				worldStartupContent->AddToRootSet();
			}
		}
	}

	// Load new world
	THandle< CWorld > newWorld = LoadResource< CWorld >( depotPath );
	if ( !newWorld )
	{
		WARN_ENGINE( TXT("Unable to load world from '%ls'"), depotPath.AsChar() );
		return nullptr;
	}

	// Set the world path
	newWorld->m_depotPath = depotPath;
	newWorld->m_preloadedData = worldStartupContent;

	// Keep worlds referenced
	newWorld->AddToRootSet();

	// Initialize normal world with PathLib support
	WorldInitInfo initInfo;
	initInfo.m_initializePhysics = true;
	initInfo.m_initializePathLib = true;
	initInfo.m_initializeOcclusion = true;
	initInfo.m_previewWorld = false;
	newWorld->Init( initInfo );

	// initialize pathlib - this function was separated to stop CWorld dummies in eg. entity editor from initializing pathlib
	newWorld->InitializePathLib();

	Uint32 banksCount = newWorld->m_soundBanksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( newWorld->m_soundBanksDependency[ i ] );
		if( !soundBank ) continue;
		soundBank->QueueLoading();
	}
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( newWorld->m_soundBanksDependency[ i ] );
		if( !soundBank ) continue;
		while( !soundBank->IsLoadingFinished() ) {}
		RED_ASSERT( soundBank->IsLoaded(), TXT("CWorld::LoadWorld - sound bank didn't load properly - bank: [%ls] - result [%ls]"), 
			soundBank->GetFileName().AsChar(), soundBank->GetLoadingResultString().AsChar() );
	}

	for( auto i = newWorld->m_soundEventsOnAttach.Begin(); i != newWorld->m_soundEventsOnAttach.End(); ++i )
	{
		GSoundSystem->GetAmbientManager().SoundEvent( i->AsChar() );
	}

	// Output loading time
	LOG_ENGINE( TXT("World loaded in %1.2fs"), loadingTimer.GetTimePeriod() );

	// Show info
	EDITOR_QUEUE_EVENT( CNAME( WorldLoaded ), NULL );

	// Emit loaded world
	return newWorld;
}

void CWorld::UnloadWorld( CWorld* world )
{
	STerrainTileMipMap::HACK_BumpTerrainVersionCounter();

	{ // THIS SCOPE IS NECESSARY. WorldUpdateStateContext will access world in destructor. However without the scope, world is destroyed before resulting in a MemoryStomp

		WorldUpdateStateContext context( world, WUP_Unload );

		ASSERT( world );

		for( auto i = world->m_soundEventsOnDetach.Begin(); i != world->m_soundEventsOnDetach.End(); ++i )
		{
			GSoundSystem->GetAmbientManager().SoundEvent( i->AsChar() );
		}

		// Unload world layers
		if ( world->m_worldLayers )
		{
			TDynArray< CLayerInfo * > layers;
			world->m_worldLayers->GetLayers( layers, true );

			// Disable caching in layers so we can really unload whem
			for ( Uint32 i = 0; i < layers.Size(); i++ )
			{
				layers[i]->GetLayer()->DisableCaching();
			}

			// Request unload
			world->m_worldLayers->SyncUnload();

			// Delayed destruction
			world->DelayedActions();

			// Flush loading and reset IO state
			GEngine->FlushJobs();
			//SJobManager::GetInstance().FlushPendingJobs();
			world->UpdateLoadingState();
		}

		// Destroy dynamic layer entities
		if ( world->m_dynamicLayer )
		{
			// Destroy all entities from dynamic layers
			LayerEntitiesArray entities = world->m_dynamicLayer->GetEntities();
			for ( Uint32 i=0; i<entities.Size(); ++i )
			{
				CEntity* entity = entities[i];
				if ( entity )
				{
					entity->Destroy();
				}
			}
		}

		// Delayed destruction
		world->DelayedActions();

		// Finish any pending jobs
		GEngine->FlushJobs();
		//SJobManager::GetInstance().FlushPendingJobs();

		// Destroy dynamic layer
		if ( world->m_dynamicLayer )
		{
			world->m_dynamicLayer->Discard();
			world->m_dynamicLayer = NULL;
		}

		// Dispose preloaded content
		if ( world->m_preloadedData )
		{
			world->m_preloadedData->RemoveFromRootSet();
			world->m_preloadedData->Discard();
			world->m_preloadedData = nullptr;
		}

		// Dispose world
		world->RemoveFromRootSet();
		world->Shutdown();

		GSoundSystem->GetAmbientManager().SoundStop(0.0f);

		Uint32 banksCount = world->m_soundBanksDependency.Size();
		for( Uint32 i = 0; i != banksCount; ++i )
		{
			CSoundBank* soundBank = CSoundBank::FindSoundBank( world->m_soundBanksDependency[ i ] );
			if( !soundBank ) continue;

			soundBank->Unload();
		}
	}

	world->Discard();

	// Force garbage collecting on world change
	SGarbageCollector::GetInstance().CollectNow();

	// Inform others
	EDITOR_QUEUE_EVENT( CNAME( WorldUnloaded ), NULL );
}

#ifndef NO_EDITOR_WORLD_SUPPORT

void CWorld::SynchronizeLayersAdd(CLayerGroup* group)
{
	CLayerGroup::TGroupList subGroups = group->GetSubGroups();

	TDynArray< String > subDirs;
	GFileManager->FindDirectories( group->GetAbsolutePath(), subDirs );

	for ( Uint32 i = 0; i < subGroups.Size(); i++ )
	{
		SynchronizeLayersAdd(subGroups[i]);
	}

	for ( Uint32 i=0; i<subDirs.Size(); i++ )
	{
		if (!group->FindGroup(subDirs[i]))
		{
			CLayerGroup* newGroup = group->CreateGroup(subDirs[i], true);
			SynchronizeLayersAdd(newGroup);
		}
	}

	// Scan for layer files
	TDynArray< String > layers;
	String pattern = String::Printf( TXT("*.%s"), ResourceExtension< CLayer >() );
	GFileManager->FindFiles( group->GetAbsolutePath(), pattern, layers, false );

	CLayerGroup::TLayerList group_layers = group->GetLayers();

	for ( Uint32 i = 0; i < group_layers.Size(); i++ )
	{
		String absolutePath = group->GetAbsolutePath() + group_layers[i]->GetShortName() + TXT(".w2l");
		if ( layers.Exist( absolutePath ) )
		{
			layers.Remove( absolutePath );
		}
	}

	// Add layers, that aren't already in the group
	for ( Uint32 i=0; i<layers.Size(); i++)
	{
		CLayerInfo* info = group->GrabDynamicLayerInfo( layers[i] );
		if ( info )
		{
			group->AddLayer(info);
		}
	}
}

void CWorld::SynchronizeLayersRemove(CLayerGroup* group)
{
	CLayerGroup::TGroupList subGroups = group->GetSubGroups();

	for ( Int32 i = (Int32)subGroups.Size()-1; i>=0; i-- )
	{
		SynchronizeLayersRemove(subGroups[i]);
	}

	// Scan for layer files
	TDynArray< String > layers;
	String pattern = String::Printf( TXT("*.%s"), ResourceExtension< CLayer >() );
	GFileManager->FindFiles( group->GetAbsolutePath(), pattern, layers, false );
	TDynArray< CLayerInfo * > removed;

	CLayerGroup::TLayerList group_layers = group->GetLayers();

	for ( Uint32 i = 0; i < group_layers.Size(); i++ )
	{
		String absolutePath = group->GetAbsolutePath() + group_layers[i]->GetShortName() + TXT(".w2l");
		if ( !layers.Exist( absolutePath ) )
		{
			removed.PushBack( group_layers[i] );
		}
	}

	// Delete layer infos that don't exist
	for ( Int32 i = removed.Size()-1; i>=0; i-- )
	{
		if ( removed[i]->IsLoaded() )
		{
			removed[i]->SyncUnload();
		}
		removed[i]->Remove();
	}

	// Delete empty group
	for ( Int32 i = (Int32)subGroups.Size()-1; i>=0; i-- )
	{
		CLayerGroup::TGroupList subgroupGroups = subGroups[i]->GetSubGroups();
		TDynArray<CLayerInfo*> subgroupLayers;
		subGroups[i]->GetLayers(subgroupLayers, false, false);

		if (subgroupLayers.Size()==0 && subgroupGroups.Size()==0)
		{
			subGroups[i]->Remove();
		}
	}
}

#endif

String CWorld::GetWorldDirectoryName() const
{
	// world does not come from a file
	if ( !GetFile() )
		return String::EMPTY;

	// get world directory
	CDirectory* dir = GetFile()->GetDirectory();
	if ( !dir )
		return String::EMPTY;

	// return world directory name
	return dir->GetName();
}

CLayer* CWorld::FindLayer( const CGUID &layerGuid )
{
	CLayerInfo* layerInfo = nullptr;

	// Check if we have the layer's layerinfo cached
	if ( m_guidLayerInfoCache.Find( layerGuid, layerInfo ) )
	{
		if ( layerInfo->IsLoaded() )
		{
			return layerInfo->GetLayer();
		}
	}

	// No cache, find it the hard way
	for ( auto it=m_attachedLayers.Begin(); it != m_attachedLayers.End(); ++it )
	{
		CLayer* layer = *it;

		if ( layer->GetGUID() == layerGuid )
		{
			// Put it in the cache
			if ( layer->GetLayerInfo() != nullptr )
			{
				m_guidLayerInfoCache[layer->GetGUID()] = layer->GetLayerInfo();
			}

			return layer;
		}
	}

	// No dice
	return nullptr;
}

CLayer* CWorld::FindLayerByTag( const CName& tag )
{
    // No cache, find it the hard way
    for ( auto it=m_attachedLayers.Begin(); it != m_attachedLayers.End(); ++it )
    {
        CLayer* layer = *it;
        CLayerInfo* info = layer->GetLayerInfo();

        if( info!= nullptr && info->m_tags.HasTag( tag ) )
            return layer;
    }

    // No dice
    return nullptr;
}


CEntity* CWorld::FindEntity( const CGUID& guid )
{
	ASSERT( SIsMainThread() );

	auto it = m_attachedEntitiesByGUID.Find( guid );
	return it != m_attachedEntitiesByGUID.End() ? *it : nullptr;
}

void CWorld::RegisterEntity( CEntity* entity )
{
	RED_ASSERT( SIsMainThread() );
	if ( !entity->GetGUID().IsZero() )
	{
		RED_VERIFY( m_attachedEntitiesByGUID.Insert( entity ) );
	}
}

void CWorld::UnregisterEntity( CEntity* entity )
{
	RED_ASSERT( SIsMainThread() );
	if ( !entity->GetGUID().IsZero() )
	{
		RED_VERIFY( m_attachedEntitiesByGUID.Erase( entity ) );
	}
}

void CWorld::OnEntityGUIDChanged( CEntity* entity, const CGUID& oldGuid )
{
	RED_ASSERT( SIsMainThread() );
	if ( !oldGuid.IsZero() )
	{
		RED_VERIFY( m_attachedEntitiesByGUID.EraseOldKey( oldGuid, entity ) );
	}
	if ( !entity->GetGUID().IsZero() )
	{
		RED_VERIFY( m_attachedEntitiesByGUID.Insert( entity ) );
	}
}

CPeristentEntity* CWorld::FindPersistentEntity( const IdTag& tag )
{
	ASSERT( SIsMainThread() );

	auto it = m_attachedPersistentEntitiesByIdTag.Find( tag );
	return it != m_attachedPersistentEntitiesByIdTag.End() ? *it : nullptr;
}

void CWorld::RegisterPersistentEntity( CPeristentEntity* entity )
{
	RED_ASSERT( SIsMainThread() );
	if ( entity->GetIdTag().IsValid() )
	{
		RED_VERIFY( m_attachedPersistentEntitiesByIdTag.Insert( entity ) );
	}
}

void CWorld::UnregisterPersistentEntity( CPeristentEntity* entity )
{
	RED_ASSERT( SIsMainThread() );
	if ( entity->GetIdTag().IsValid() )
	{
		RED_VERIFY( m_attachedPersistentEntitiesByIdTag.Erase( entity ) );
	}
}

void CWorld::OnPersistentEntityIdTagChanged( CPeristentEntity* entity, const IdTag& oldTag )
{
	RED_ASSERT( SIsMainThread() );
	if ( oldTag.IsValid() )
	{
		RED_VERIFY( m_attachedPersistentEntitiesByIdTag.EraseOldKey( oldTag, entity ) );
	}
	if ( entity->GetIdTag().IsValid() )
	{
		RED_VERIFY( m_attachedPersistentEntitiesByIdTag.Insert( entity ) );
	}
}

Uint32 CWorld::RegisterSectorData( const Uint64 contentHash, class CSectorData* sectorData, const Bool isVisible )
{
	if ( m_sectorStreaming )
		return m_sectorStreaming->AttachSectorData( contentHash, sectorData, isVisible );
	else
		return 0;
}

void CWorld::UnregisterSectorData( const Uint32 sectorDataId )
{
	if ( m_sectorStreaming )
	{
		m_sectorStreaming->RemoveSectorData( sectorDataId );
	}
}

void CWorld::ToggleSectorDataVisibility( const Uint32 sectorDataId, const Bool isVisible )
{
	if ( m_sectorStreaming )
	{
		m_sectorStreaming->ToggleSectorDataVisibility( sectorDataId, isVisible );
	}
}

void CWorld::ForceStreamingUpdate()
{
#ifndef NO_EDITOR
	if( m_enableStreaming )
#endif
	{
		m_streamingSectorData->UpdateStreaming( true, true );
		//m_sectorStreaming->UpdateStreaming( true );
	}
}

void CWorld::RequestStreamingUpdate()
{
	m_streamingSectorData->RequestFullUpdate();
}

void CWorld::ForceStreamingForArea( const Box& box )
{
	m_streamingSectorData->ForceStreamForArea( box );
}

void CWorld::ForceStreamingForPoint( const Vector& point )
{
	m_streamingSectorData->ForceStreamForPoint( point );

	if ( m_sectorStreaming )
	{
		m_sectorStreaming->ForceStreamForPoint( point );
	}
}

void CWorld::IgnoreEntityStreaming( CEntity* entity )
{
#ifndef NO_EDITOR
	m_streamingSectorData->GetIgnoreList().IgnoreEntity( entity );
#endif
}

void CWorld::UnignoreEntityStreaming( CEntity* entity )
{
#ifndef NO_EDITOR
	m_streamingSectorData->GetIgnoreList().UnignoreEntity( entity );
	m_streamingSectorData->RequestFullUpdate();
#endif
}

void CWorld::ClearStreamingIgnoreList()
{
#ifndef NO_EDITOR
	m_streamingSectorData->GetIgnoreList().ClearIgnoreList();
	m_streamingSectorData->RequestFullUpdate();
#endif
}

void CWorld::UpdateCameraPosition( const Vector& position )
{
	// Update camera position
	m_cameraPosition = position;

	// update the things triggered by the camera
	if ( NULL != m_cameraActivator )
	{
		PC_SCOPE_PIX( CameraActivatorMove );
		const IntegerVector4 newCamPosI( position );
		m_cameraActivator->Move( newCamPosI, true );
	}
}

void CWorld::SetStreamingReferencePosition( const Vector& position )
{
	// update resource prefetcher position
	if ( m_sectorPrefetch )
	{
		m_sectorPrefetch->PrecacheCells( position );
	}

	// Camera
#ifdef RED_PROFILE_FILE_SYSTEM	
	if ( m_debugLastStreamingCameraValid )
	{
		const Float dist = position.DistanceTo( m_debugLastStreamingCamera );
		RedIOProfiler::ProfileStreamingCameraDist( dist );
	}
	RedIOProfiler::ProfileStreamingCameraPos( position.X, position.Y, position.Z );
#endif

	// Update streaming camera position for debug
	m_debugLastStreamingCameraValid = true;
	m_debugLastStreamingCamera = position;

	// update reference position for the entity streaming
	// NOTE: this is NOT doing any streaming, streaming is done in tick
	m_streamingSectorData->SetReferencePosition( position );

	// update reference position for the sector streaming
	if ( m_sectorStreaming )
	{
		m_sectorStreaming->SetReferencePosition( position );
	}

	// update reference position for the physical scene
	// NOTE: we do not update the reference position for physical worlds that we do not own
	if ( m_physicsWorld && IsPhysicalWorldOwned() )
	{
		m_physicsWorld->SetReferencePosition( position );

		if ( m_physicsWorldSecondary )
		{
			m_physicsWorldSecondary->SetReferencePosition( position );
		}
	}

	m_componentLODManager.SetReferencePosition( position );
	m_effectTickLODManager.SetReferencePosition( position );
	m_manualStreamingHelper.SetReferencePosition( position );

	if ( m_tickManager )
	{
		m_tickManager->SetReferencePosition( position );
	}

#ifdef USE_UMBRA
	if ( m_umbraScene )
	{
#ifndef NO_UMBRA_DATA_GENERATION
		if ( !m_umbraScene->IsDuringSyncRecreation() )
#endif // NO_UMBRA_DATA_GENERATION
		{
			m_umbraScene->SetReferencePosition( position );
		}
	}
#endif // USE_UMBRA

	// Foliage
	if ( m_foliageScene && !m_isPreviewWorld )
	{
		PC_SCOPE_PIX( FoliageSceneUpdate );

		m_foliageScene->Update( position );
	}

	if ( m_pathLib )
	{
		m_pathLib->SetReferencePosition( position );
	}
}

void CWorld::FinishRenderingFades()
{
	if ( m_renderSceneEx )
	{
		(new CRenderCommand_FinishFades(m_renderSceneEx))->Commit();
	}
}

void CWorld::SetStreamingLockedArea( const Box* worldSpaceBounds, const Uint32 numAreas )
{
	// entities (incremental)
	m_streamingSectorData->SetStreamingLock( worldSpaceBounds, numAreas );

	// sector data
	if ( m_sectorStreaming )
		m_sectorStreaming->SetStreamingLock( worldSpaceBounds, numAreas );

	if ( m_foliageScene )
	{
		m_foliageScene->PrefetchArea( worldSpaceBounds, numAreas );
	}

	if ( m_terrainClipMap )
	{
		//todo
	}
}

Int8 CWorld::SuggestStreamingLODFromBox( const Box& box )
{
	Int8 lod = 0;
	Vector sizes(5, 20, FLT_MAX);
	
	Float diagonal = box.CalcSize().Mag3();
	for (Int8 i = 0; i < 3; ++i)
	{
		if ( sizes.A[(Uint32)i] > diagonal )
		{
			lod = i;
			break;
		}
	}

	return lod;
}

void CWorld::UpdateCameraForward( const Vector& vec )
{
	m_cameraForward = vec;
}

void CWorld::UpdateCameraUp( const Vector& vec )
{
	m_cameraUp = vec;
}

void CWorld::InitializeStreaming()
{
	// Determine world size
	Float worldSize = 128.0f; // world without the terrain
	if ( m_terrainClipMap )
	{
		worldSize = m_terrainClipMap->GetTerrainSize();
	}

	// Make sure we have CStreamingSectorData
	if ( m_streamingSectorData == nullptr )
	{
		m_streamingSectorData = new CStreamingSectorData( this );
	}

	// Make sure we have the sector streaming manager
	if ( m_sectorStreaming == nullptr )
	{
		 // W3 hack: we need this structure only for big enough worlds
		if ( worldSize > 256.0f )
		{
			const Bool hasPrefetchData = (m_sectorPrefetch != nullptr);

			RED_ASSERT( m_dynamicLayer != nullptr, TXT("Dynamic layer is required for streaming") );
			m_sectorStreaming = new CSectorDataStreaming( worldSize, m_renderSceneEx, m_physicsWorld, m_dynamicLayer, hasPrefetchData, m_terrainClipMap );

			// create a helper class that will flush streaming every time GC occurs
			m_sectorStreamingGCFlusher = new CSectorDataStreamingGCFlusher( m_sectorStreaming );
			GObjectGC->RegisterHelper( m_sectorStreamingGCFlusher );
		}
	}
}

void CWorld::InitializeLayersStructure()
{
	if ( m_hasEmbeddedLayerInfos )
	{
		if ( !m_worldLayers )
		{
			ERR_ENGINE( TXT("World was not cooked with new cooker. Failed to load.") );
			EnumerateLayersStructure();
		}
		else
		{
			// Link the layer group structure to the world
			m_worldLayers->LinkToWorld( this );
		}
	}
	else
	{
		// We cannot have any precooked data in non cooked builds
		ASSERT( !m_worldLayers );
		EnumerateLayersStructure();
	}

	//! For DLC content we have to always populate layers manual 
	//! Layers for DLC are cooked manual in CR4DLCAnalyzer
	if( m_worldLayers )
	{
		CTimeCounter dtimer;
		m_worldLayers->PopulateFromDLC();
		LOG_ENGINE( TXT("Repopulating DLC layers took: %1.2fms"), dtimer.GetTimePeriodMS() );
	}

}

void CWorld::InitializePhysicalRepresentation( const WorldInitInfo& initInfo )
{
	if ( initInfo.m_sharePhysicalWorld )
	{
		// share physical representation with other world
		m_sharedPhyscalWorldOwner	= initInfo.m_sharePhysicalWorld;
		m_physicsWorld				= initInfo.m_sharePhysicalWorld->m_physicsWorld;
		m_physicsWorldSecondary		= initInfo.m_sharePhysicalWorld->m_physicsWorldSecondary;
	}
	else if ( initInfo.m_initializePhysics )
	{
		const Uint32 numTilesPerEdge	= m_terrainClipMap ? m_terrainClipMap->GetNumTilesPerEdge() : 0;
		const Float terrainTile			= m_terrainClipMap ? m_terrainClipMap->GetTerrainSize() : 0.0f;

		Vector2 areaCornerPosition		= m_terrainClipMap ? m_terrainClipMap->GetTerrainCorner() : Vector::ZERO_3D_POINT;
		Uint32 clipMapSize				= m_terrainClipMap ? m_terrainClipMap->GetClipmapSize() : 0;
		Uint32 tileRes					= m_terrainClipMap ? m_terrainClipMap->GetTilesMaxResolution() : 0;

		// we should create the same world in every configuration for easy debug
		m_physicsWorld				= GPhysicEngine->CreateWorld( new CPhysicsWorldParentEngineWorldProvider( this ), numTilesPerEdge, terrainTile, areaCornerPosition, clipMapSize, tileRes, true, false, true );
		if( SSoundSettings::m_contactSoundsEnabled )
		{
			m_physicsWorld->PushContactListener( new CSoundContactListener() );
		}
#ifndef RED_FINAL_BUILD
		if( !SPhysicsSettings::m_dontCreateClothSecondaryWorld )
#endif
		{
			m_physicsWorldSecondary		= GPhysicEngine->CreateWorld( new CPhysicsWorldParentEngineWorldProvider( this ), 0, 0.0f, areaCornerPosition, 0, 0, false, true, false );
		}
		m_sharedPhyscalWorldOwner	= nullptr;

#ifdef USE_PHYSX
		CPhysicsWorldPhysXImpl* world = nullptr;
		if( GetPhysicsWorld( world ) )
		{
			if( world->GetPxScene() )
			{
				m_controllerManager.Reset( new CCharacterControllersManager( world->GetPxScene() ) );
			}
		}
#endif

		// Create physics batch trace manager
		m_physicsBatchQueryManager = new CPhysicsBatchQueryManager( m_physicsWorld );
	}
}

void CWorld::InitializePathLib()
{
	// create new pathlib instance
	if ( !m_pathLib )
	{
		m_pathLib = ::CreateObject< CPathLibWorld >( this );
		m_pathLib->InitializeTerrain();
	}
}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
void GetChangelist( const String& worldName, SChangelist& changelist )
{
	static Bool sentinel = false;
	// Create a changelist only if the automatic changelists are enabled
	if ( !sentinel && GVersionControl->AutomaticChangelistsEnabled() )
	{
		String tags;
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("SourceControl"), TXT("DepartmentTags"), tags );
		String name = tags + TXT(" Occlusion data for ") + worldName;

		Bool changelistCreated = GVersionControl->CreateChangelist( name, changelist );
		ASSERT( changelistCreated );
		sentinel = true;
	}
}
#endif

#ifdef USE_UMBRA
void CWorld::InitializeOcclusionCulling()
{
	// if the pointer is valid, that means the occlusion data was serialized,
	// otherwise create the empty occlusion scene
	if ( !m_umbraScene )
	{
		Float worldDimensions = GetWorldDimensions();
		const Float tileSize = 256.0f;

		Int32 iWorldDimensions = (Int32)worldDimensions;
		Int32 iTileSize = 256;
		ASSERT( iWorldDimensions % iTileSize == 0 );

		Int32 tilesCount = iWorldDimensions / iTileSize;

		m_umbraScene = ::CreateObject< CUmbraScene >( this );

		CDirectory* dir = GetFile()->GetDirectory();
		CFilePath filePath( GetFile()->GetAbsolutePath() );

		String umbraSceneFileName = String::Printf( TXT( "%s.%s" ), filePath.GetFileName().AsChar(), CUmbraScene::GetFileExtension() );
		String potentiallyExistingFileName = String::Printf( TXT("%s%s"), dir->GetDepotPath().AsChar() , umbraSceneFileName.AsChar() );
		
		CDiskFile* occlusionDefinitionFile = GDepot->FindFile( potentiallyExistingFileName );
		if ( occlusionDefinitionFile )
		{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
			occlusionDefinitionFile->Rebind( m_umbraScene.Get() );
#endif
		}
		else
		{
			occlusionDefinitionFile = new CDiskFile( dir, umbraSceneFileName, m_umbraScene.Get() );
			dir->AddFile( occlusionDefinitionFile );
		}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		occlusionDefinitionFile->GetStatus();
		SChangelist changelist = SChangelist::DEFAULT;
		GetChangelist( GetFriendlyName(), changelist );
		occlusionDefinitionFile->SetChangelist( changelist );
#endif

		m_umbraScene->Initialize( this, tilesCount, tileSize );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		occlusionDefinitionFile->Save();
		if ( !occlusionDefinitionFile->IsEdited() )
		{
			occlusionDefinitionFile->Add();
		}
#endif
	}
	
#if 0
	// ifdef-ed out piece of code for quick fixing the occlusion tiles if the UmbraScene got corrupted,
	// might get in handy some time
	else
	{
		CDirectory* dir = GetFile()->GetDirectory();
		CFilePath filePath( GetFile()->GetAbsolutePath() );

		String umbraSceneFileName = String::Printf( TXT( "%s.%s" ), filePath.GetFileName().AsChar(), CUmbraScene::GetFileExtension() );
		String potentiallyExistingFileName = String::Printf( TXT("%s%s"), dir->GetDepotPath().AsChar() , umbraSceneFileName.AsChar() );

		CDiskFile* occlusionDefinitionFile = GDepot->FindFile( potentiallyExistingFileName );
		if ( occlusionDefinitionFile )
		{
			occlusionDefinitionFile->Bind( m_umbraScene.Get() );
		}
		else
		{
			occlusionDefinitionFile = new CDiskFile( dir, umbraSceneFileName );
			occlusionDefinitionFile->BindResource( m_umbraScene.Get() );
			dir->AddFile( occlusionDefinitionFile );
		}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		occlusionDefinitionFile->GetStatus();
		SChangelist changelist = SChangelist::DEFAULT;
		GetChangelist( GetFriendlyName(), changelist );
		occlusionDefinitionFile->SetChangelist( changelist );
#endif  // NO_FILE_SOURCE_CONTROL_SUPPORT

		const Float tileSize = 256.0f;
		Int32 tilesCount = 32; // specific for the level
		m_umbraScene->Initialize( this, tilesCount, tileSize );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		occlusionDefinitionFile->Save();
#endif // NO_FILE_SOURCE_CONTROL_SUPPORT
	}
#endif // 0

#ifdef USE_UMBRA_COOKING
#ifndef NO_UMBRA_DATA_GENERATION
	// generate intermediate results files if necessary
	{
		ASSERT( m_umbraScene );
		Uint32 numTiles = m_umbraScene->GetTilesCount();

		String dirPath = GetFile()->GetDirectory()->GetAbsolutePath();
		String intermediateDirPath = String::Printf( TXT( "%s\\occlusion_tiles\\intermediate_results" ), dirPath.AsChar() );

		Uint32 numberOfEntries = 0;

		for ( Uint32 i = 0; i < numTiles; ++i )
		for ( Uint32 j = 0; j < numTiles; ++j )
		{
			String intermediateFileName = String::Printf( TXT("%s\\%dx%d.intermediateResult"), intermediateDirPath.AsChar(), i, j );
			if ( GFileManager->FileExist( intermediateFileName ) )
			{
				continue;
			}

			IFile* f = GFileManager->CreateFileWriter( intermediateFileName, FOF_AbsolutePath );
			if ( f )
			{
				f->Serialize( &numberOfEntries, sizeof( numberOfEntries ) );
				delete f;
				f = nullptr;
			}
		}
	}
#endif // NO_UMBRA_DATA_GENERATION
#endif // USE_UMBRA_COOKING

	( new CRenderCommand_UploadObjectCache( m_renderSceneEx, m_umbraScene->GetObjectCache() ) )->Commit();
}
#endif // USE_UMBRA

void CWorld::OnTerrainTilesDestruction()
{
	if ( m_pathLib )
	{
		m_pathLib->DestroyTerrainData();
	}
}
void CWorld::OnTerrainTilesCreation()
{
	if ( m_pathLib )
	{
		m_pathLib->InitializeTerrain();
	}
}
void CWorld::OnTerrainTileUpdate( Uint32 x, Uint32 y )
{
	if ( m_pathLib )
	{
		m_pathLib->MarkTileSurfaceModified( x, y );
	}
}

void CWorld::OnTerrainCollisionDataBoundingUpdated( const Box& bbox )
{

}

#ifdef USE_ANSEL
Bool CWorld::EnsureTerrainCollisionGenerated( const Vector& position )
{
	return m_terrainClipMap->EnsureCollisionIsGenerated( position );
}
#endif // USE_ANSEL

CClipMap* CWorld::CreateTerrain()
{
	// Create terrain clipmap
	if ( !m_terrainClipMap )
	{
		m_terrainClipMap = CreateObject< CClipMap >( this );
	}
	return m_terrainClipMap;
}

void  CWorld::SetWaterVisible( Bool isVisible, Bool surpassRendering )
{
	if( isVisible && !m_environmentParameters.m_disableWaterShaders )
	{
		if( !m_globalWater )
		{				
			m_globalWater = new CGlobalWater( this );
			
			m_globalWater->Setup( GetRenderSceneEx() );			
		}

		m_globalWater->SetWaterRenderSurpass( surpassRendering );
	}
	else
	{
		if( m_globalWater )
		{
			m_globalWater->DestroyProxy( GetRenderSceneEx() );
			m_globalWater->Discard();			
			m_globalWater = NULL;
		}
	}
}

void CWorld::UpdateWaterProxy()
{
	if ( m_globalWater ) 
	{ 
		m_globalWater->Setup( GetRenderSceneEx() ); 
	}
}

RED_DEFINE_STATIC_NAME( environmentParameters );
RED_DEFINE_STATIC_NAME( weatherTemplate );

void CWorld::OnPropertyPostChange( IProperty* property )
{
	// We can't get finer notifications of individual properties within environmentParameters, but if any parameters have changed,
	// make sure the skybox entity is correctly updated.
	if ( property->GetName() == CNAME( environmentParameters ) )
	{
		RefreshSceneSkyboxParams();
		RefreshLensFlareParams();

		// Update weather parameters from world's template
		if ( m_environmentManager )
		{
			m_environmentManager->UpdateWeatherParametersFromWorld();
			
			// Updating the water shader visibility
			const Bool shouldDisplayWater = m_environmentManager->GetGameEnvironmentParams().m_displaySettings.m_allowWaterShader;
			if( IsWaterShaderEnabled() != shouldDisplayWater ) SetWaterVisible( shouldDisplayWater );
		}		
	}
}
//////////////////////////////////////////////////////////////////////////////////////////

Bool CWorld::TestLineOfSight( const Vector& sourcePos, const CNode* target, Bool adjustTargetPos /* = false */, const TDynArray< const CEntity* > * ignoreEntities /* = nullptr */ ) const
{
	if ( m_physicsWorld == nullptr )
	{
		ASSERT( false && TXT( "No physics world for line of sight test. Assuming full visibility." ) );
		return true;
	}

	Vector targetPos = target->GetWorldPosition();

	// move target position towards source (to avoid collisions with target entity)
	if ( adjustTargetPos )
	{
		Vector offset = sourcePos - targetPos;
		offset.Normalize3();
		Float dist = 0.0f;
		if ( target->IsA< CEntity >() )
		{
			const CEntity* entity = static_cast< const CEntity* >( target );
			Vector extents = entity->CalcBoundingBox().CalcExtents();
			// arithmetic mean value of BB extents is a rough approximation
			// of bounding sphere radius - it's not precise but fits our needs
			dist = ( ( extents.X + extents.Y + extents.Z ) * 0.34f );
		}
		dist = Max( dist, 0.3f );
		targetPos += offset * dist;
	}

	return TestLineOfSight( sourcePos, targetPos, ignoreEntities );
}

Bool CWorld::TestLineOfSight( const Vector& sourcePos, const Vector& targetPos, const TDynArray< const CEntity* > * ignoreEntities /* = nullptr */ ) const
{
	if ( m_physicsWorld == nullptr )
	{
		ASSERT( false && TXT( "No physics world for line of sight test. Assuming full visibility." ) );
		return true;
	}

	static CPhysicsEngine::CollisionMask collisionMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) );

	SPhysicsContactInfo contactInfo;
	Bool isHit = m_physicsWorld->RayCastWithSingleResult( sourcePos, targetPos, collisionMask, 0, contactInfo ) == TRV_Hit;
	if ( isHit && ignoreEntities != nullptr )
	{
		Int16 actorIndex = contactInfo.m_rigidBodyIndexA.m_actorIndex;
		RED_ASSERT( contactInfo.m_userDataA != nullptr , TXT( "Hit occured, but no user data received!" ) );
		if ( actorIndex >= 0 && contactInfo.m_userDataA != nullptr )
		{
			CComponent* hitComponent = nullptr; ;
			if ( contactInfo.m_userDataA->GetParent( hitComponent, static_cast< Uint32 >( actorIndex ) ) )
			{
				if ( ignoreEntities->Exist( hitComponent->GetEntity() ) )
				{
					// Hitting itself, treat as no hit
					isHit = false;
				}
			}
		}
	}

	return !isHit;
}

Bool CWorld::TestLineOfSight( const Vector& sourcePos, const Vector& targetPos ) const
{
	if ( m_physicsWorld == nullptr )
	{
		ASSERT( false && TXT( "No physics world for line of sight test. Assuming full visibility." ) );
		return true;
	}

	static CPhysicsEngine::CollisionMask collisionMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) |
														 GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) );

	SPhysicsContactInfo contactInfo;
	return m_physicsWorld->RayCastWithSingleResult( sourcePos, targetPos, collisionMask, 0, contactInfo ) != TRV_Hit;
}

//////////////////////////////////////////////////////////////////////////////////////////

void CWorld::funcShowLayerGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, layerGroupName, String::EMPTY );
	FINISH_PARAMETERS;

	TDynArray< String > layers;
	TDynArray< String > layersEmpty;
	layers.PushBack( layerGroupName );

	GGame->ScheduleLayersVisibilityChange( GetDepotPath(), layersEmpty, layers, false );
}

void CWorld::funcHideLayerGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, layerGroupName, String::EMPTY );
	FINISH_PARAMETERS;

	TDynArray< String > layers;
	TDynArray< String > layersEmpty;
	layers.PushBack( layerGroupName );
	GGame->ScheduleLayersVisibilityChange( GetDepotPath(), layers, layersEmpty, false );
}

void CWorld::funcPointProjectionTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, point, Vector::ZEROS );
	GET_PARAMETER_REF( EulerAngles, normal, EulerAngles::ZEROS );
	GET_PARAMETER( Float, range, 1.f );
	FINISH_PARAMETERS;
	RETURN_BOOL( CTraceTool::StaticPointProjectionTestTwoWay( this, point, normal, range ) );
}


Bool CWorld::IsWaterShaderEnabled() const
{
	return m_globalWater != nullptr;
}

Float CWorld::GetWaterLevel( const Vector &point, Uint32 lodApproximation, Float* heightDepth ) const
{
	Float result = WATER_DEFAULT_NON_INIT_LEVEL;
	if( m_globalWater )
	{
		if( lodApproximation < 1 )
		{
			result = m_globalWater->GetWaterLevelAccurate( point.X, point.Y, heightDepth );
		}
		else if( lodApproximation < 2 )
		{
			result = m_globalWater->GetWaterLevelApproximate( point.X, point.Y, heightDepth );
		}
		else if( lodApproximation < 3 )
		{
			result = m_globalWater->GetWaterLevelBasic( point.X, point.Y );
		}
		else
		{
			result = m_globalWater->GetWaterLevelReference();
		}
	}
	return result;
}

Bool CWorld::GetWaterLevelBurst( Uint32 elementsCount, void* inputPos, void* outputPos, void* outputHeightDepth, size_t stride, Vector referencePosition, Bool useApproximation ) const
{
	if( !m_globalWater ) return false;
	return m_globalWater->GetWaterLevelBurst( elementsCount, inputPos, outputPos, outputHeightDepth, stride, referencePosition, useApproximation );
}

#ifdef USE_UMBRA
#ifndef NO_EDITOR
Bool CWorld::GenerateOcclusionForTileSync( STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData )
{
#ifdef NO_UMBRA_DATA_GENERATION
	return false;
#else

#ifndef USE_UMBRA_COOKING
	return false;
#else
	Bool generateTome = m_umbraScene->ShouldGenerateData( context.tileId );
	if ( generateTome )
	{
		CClipMap* terrain = GetTerrain();
		m_umbraScene->AddTerrain( context.tileId, terrain );
		terrain->UnloadAll();

		Float tileSize = m_umbraScene->GetTileSize();
		Vector minCorner(	( -( m_umbraScene->GetTilesCount() / 2 ) + context.tileId.X ) * tileSize,
			( -( m_umbraScene->GetTilesCount() / 2 ) + context.tileId.Y ) * tileSize,
			-1000.f
			);

		context.tileCorner = minCorner;
		context.umbraTileSize = context.computationParameters.m_umbraTileSize;

		return m_umbraScene->GenerateTomeForTileSync( context, dumpScene, dumpRawTomeData );
	}
	else
	{
		CUmbraTile* tile = m_umbraScene->GetTile( context.tileId );
		ASSERT( tile );
		tile->Clear();
		tile->SetStatus( EUTDS_NoData );
		tile->SaveGeneratedData();
		context.tile = tile;
	}
	return true;
#endif //USE_UMBRA_COOKING
#endif //NO_UMBRA_DATA_GENERATION
}

namespace 
{
	void Cleanup()
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		Int64 beforeGC = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_BufferMesh );
#endif
		SGarbageCollector::GetInstance().CollectNow();
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		Int64 afterGC = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_BufferMesh );
		Int64 delta = afterGC - beforeGC;
		String sBeforeGC	= String::FormatByteNumber( beforeGC, TString<Char>::BNF_Precise );
		String sAfterGC		= String::FormatByteNumber( afterGC, TString<Char>::BNF_Precise );
		String sDelta		= String::Printf( TXT("%s%s"), delta < 0 ? TXT("-") : TXT(""), String::FormatByteNumber( delta < 0 ? -delta : delta, TString<Char>::BNF_Precise ).AsChar() );
		LOG_ENGINE( TXT("BeforeGC: %s; After GC: %s; Delta: %s"), sBeforeGC.AsChar(), sAfterGC.AsChar(), sDelta.AsChar() );
#endif
	}
}

Bool CWorld::GenerateDuplicatesList( SDuplicatesFinderContext& context )
{
	TPositionMap positionMap;

	TDynArray< CLayerInfo* > layersToLoad;
	m_worldLayers->GetLayers( layersToLoad, false, true, true );
	for ( Uint32 i = 0; i < layersToLoad.Size(); ++i )
	{
		LayerLoadingContext layerLoadingContext;
		layerLoadingContext.m_loadHidden = true;
		CLayerInfo* layerInfo = layersToLoad[i];
		if ( !layerInfo )
		{
			continue;
		}

		const String& layerName = layerInfo->GetDepotPath();

		// cache original value
		Bool isLayerVisibilityForced = layerInfo->IsVisibilityForced();
		layerInfo->ForceVisible( true );

		layerInfo->SyncLoad( layerLoadingContext );
		if ( !layerInfo->GetLayer() )
		{
			continue;
		}

		const LayerEntitiesArray& entities = layerInfo->GetLayer()->GetEntities();
		for ( auto& entity : entities )
		{
			if ( !entity )
			{
				continue;
			}

			const String& entityName = entity->GetName();

			entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
			const TDynArray<CComponent*>& components = entity->GetComponents();
			for ( auto& component : components )
			{
				if ( !component->ShouldBeCookedAsOcclusionData() || !CUmbraScene::ShouldAddComponent( component ) )
				{
					// consider only those valid for occlusion data
					continue;
				}

				component->ForceUpdateTransformNodeAndCommitChanges();

				SPositionInfo positionInfo;
				positionInfo.position = component->GetLocalToWorld().GetTranslationRef();
				positionInfo.layerName = layerName;
				positionInfo.entityName = entityName;
				positionInfo.componentName = component->GetName();

				Uint32 modelId = component->GetOcclusionId();
				Uint32 transformHash = UmbraHelpers::CalculateTransformHash( component->GetLocalToWorld() );
				TObjectCacheKeyType objectCacheKey = UmbraHelpers::CompressToKeyType( modelId, transformHash );
				
				if ( positionMap.KeyExist( objectCacheKey ) )
				{
					positionMap[ objectCacheKey ].PushBack( positionInfo );
				}
				else
				{
					TDynArray< SPositionInfo > arr;
					arr.PushBack( positionInfo );
					positionMap.Insert( objectCacheKey, arr );
				}
			}
			entity->DestroyStreamedComponents( SWN_DoNotNotifyWorld );
		}

		// restore cached value
		layerInfo->ForceVisible( isLayerVisibilityForced );
		layerInfo->SyncUnload();

		if ( i % 20 == 0 )
		{
			// perform garbage collection every 20 layers. This will free mesh data (vertices, indices), freeing even up to 1GB of memory per run
			Cleanup();
		}
	}

	FILE* fp = nullptr;
	if ( context.useOutputFile )
	{
		fp = _wfopen( context.outputFilePath.AsChar(), TXT("w") );
		if ( !fp )
		{
			ERR_ENGINE( TXT("Could not create output file '%s'"), context.outputFilePath.AsChar() );
		}
	}

	if ( fp )
	{
		fwprintf( fp, TXT("LP;Position;Layer;Entity;Component\n") );
	}
	else
	{
		LOG_ENGINE( TXT("LP;Position;Layer;Entity;Component") );
	}

	
	for ( const auto& positionMapEntry : positionMap )
	{
		if ( positionMapEntry.m_second.Size() > 1 )
		{
			// we have a duplicate
			for ( Uint32 i = 0; i < positionMapEntry.m_second.Size(); ++i )
			{
				const auto& dup = positionMapEntry.m_second[ i ];

				if ( fp )
				{
					fwprintf( fp, TXT("%d;%s;%s;%s;%s\n"), i, ToString( dup.position ).AsChar(), dup.layerName.AsChar(), dup.entityName.AsChar(), dup.componentName.AsChar() );
				}
				else
				{
					LOG_ENGINE( TXT("%d;%s;%s;%s;%s"), i, ToString( dup.position ).AsChar(), dup.layerName.AsChar(), dup.entityName.AsChar(), dup.componentName.AsChar() );
				}
			}

			// empty line
			if ( fp )
			{
				fwprintf( fp, TXT("\n") );
			}
			else
			{
				LOG_ENGINE( TXT("") );
			}
		}
	}

	if ( fp )
	{
		fclose( fp );
		fp = nullptr;
	}

	return true;
}

Bool CWorld::GenerateOcclusionForAllTilesSync( const VectorI& bounds, struct STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData, Bool dumpExistingTomeData )
{
#ifndef NO_UMBRA_DATA_GENERATION
	Int32 minI = 0;
	Int32 minJ = 0;
	Int32 maxI = m_umbraScene->GetGrid().GetCellCounts().X - 1;
	Int32 maxJ = m_umbraScene->GetGrid().GetCellCounts().Y - 1;

	static const VectorI InvalidBounds( -1, -1, -1, -1 );
	Bool isInvalid = bounds == InvalidBounds;
	if ( !isInvalid )
	{
		minI = Max< Int32 >( minI, bounds.X );
		minJ = Max< Int32 >( minJ, bounds.Y );
		maxI = Min< Int32 >( maxI, bounds.Z );
		maxJ = Min< Int32 >( maxJ, bounds.W );
	}
	const VectorI adjustedBounds( minI, maxI, minJ, maxJ );

	// fill in fileName and intermediate directory once
	CFilePath path( GetFile()->GetAbsolutePath() );
	CFilePath::PathString worldFileName = path.GetFileName();
	CFilePath::PathString worldPath = path.GetPathString();
	String filepathWithoutExtension = String::Printf( TXT("%s\\occlusion_tiles\\%s"), worldPath.AsChar(), worldFileName.AsChar() );
	String intermediateDirectory = String::Printf( TXT("%s\\occlusion_tiles\\intermediate_results"), worldPath.AsChar() );
	context.fileName = Move( filepathWithoutExtension );
	context.intermediateDirectory = Move( intermediateDirectory );

	if ( dumpExistingTomeData )
	{
		Bool res = true;
		for ( Int32 j = minJ; j <= maxJ; ++j )
		for ( Int32 i = minI; i <= maxI; ++i )
		{
			context.tileId = VectorI( i, j, 0, 0 );
			res &= m_umbraScene->DumpTomeDataForTile( context );
		}
		return res;
	}

	// clear data for all generated tiles
	for ( Int32 j = 0; j < m_umbraScene->GetGrid().GetCellCounts().Y; ++j )
	for ( Int32 i = 0; i < m_umbraScene->GetGrid().GetCellCounts().X; ++i )
	{
		VectorI id( i, j, 0, 0 );
		m_umbraScene->ClearTileData( id );
		ASSERT( m_umbraScene->GetTile( id )->GetObjectCache().Empty() );
		if ( !m_umbraScene->GetTile( id )->GetObjectCache().Empty() )
		{
			RED_LOG_WARNING( UmbraWarning, TXT("ObjectCache in tile [%d; %d] failed to Clear() properly"), i, j );
		}
	}
	ASSERT( m_umbraScene->GetObjectCache().Empty() );
	if ( !m_umbraScene->GetObjectCache().Empty() )
	{
		RED_LOG_WARNING( UmbraWarning, TXT("ObjectCache in UmbraScene failed to Clear() properly") );
	}

	m_umbraScene->ClearTileDependencies();

	Int32 total = ( maxJ - minJ + 1 ) * ( maxI - minI + 1 );

	m_umbraScene->SetIsDuringSyncRecreation( true );	

	Uint32 prevProgress = 0;

	TDynArray< CLayerInfo* > layersToLoad;
	m_worldLayers->GetLayers( layersToLoad, false, true, true );
	for ( Uint32 i = 0; i < layersToLoad.Size(); ++i )
	{
		LayerLoadingContext layerLoadingContext;
		layerLoadingContext.m_loadHidden = true;
		CLayerInfo* layerInfo = layersToLoad[i];
		if ( !layerInfo )
		{
			continue;
		}

		// cache original value
		Bool isLayerVisibilityForced = layerInfo->IsVisibilityForced();
		layerInfo->ForceVisible( true );

		layerInfo->SyncLoad( layerLoadingContext );
		if ( !layerInfo->GetLayer() )
		{
			continue;
		}

		const LayerEntitiesArray& entities = layerInfo->GetLayer()->GetEntities();
		for ( auto& entity : entities )
		{
			if ( !entity )
			{
				continue;
			}

			entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
			const TDynArray<CComponent*>& components = entity->GetComponents();
			for ( auto& component : components )
			{
				component->ForceUpdateTransformNodeAndCommitChanges();
				component->OnAttachToUmbraScene( m_umbraScene.Get(), adjustedBounds );
			}
			entity->DestroyStreamedComponents( SWN_DoNotNotifyWorld );
		}

		// restore cached value
		layerInfo->ForceVisible( isLayerVisibilityForced );
		layerInfo->SyncUnload();

		if ( i % 20 == 0 )
		{
			// perform garbage collection every 20 layers. This will free mesh data (vertices, indices), freeing even up to 1GB of memory per run
			Cleanup();
		}

		Uint32 progress = (Uint32)( ( (Float)(i + 1) / (Float)layersToLoad.Size() ) * 100.0f );
		if ( progress != prevProgress )
		{
			RED_LOG( UmbraInfo, TXT("Collecting mesh data. Layers %d of %d. Progress: %d%%"), i+1, layersToLoad.Size(), progress );
			prevProgress = progress;
		}
	}

	// perform final cleanup after loading all meshes
	Cleanup();

	// process dependencies
	for ( Int32 j = 0; j < m_umbraScene->GetGrid().GetCellCounts().Y; ++j )
	for ( Int32 i = 0; i < m_umbraScene->GetGrid().GetCellCounts().X; ++i )
	{
		m_umbraScene->ProcessDependencies( VectorI( i, j, 0, 0 ) );
	}
	
	struct TileDensityInfo
	{
		Uint32	numberOfObjects;
		VectorI coordinates;
	};

	TDynArray< TileDensityInfo > densityOfTiles;
	for ( Int32 j = minJ; j <= maxJ; ++j )
	for ( Int32 i = minI; i <= maxI; ++i )
	{
		TileDensityInfo info;
		info.coordinates = VectorI( i, j, 0, 0 );
#ifdef USE_UMBRA_COOKING
		info.numberOfObjects = m_umbraScene->GetTileDensity( info.coordinates );
#else
		info.numberOfObjects = 0;
#endif //USE_UMBRA_COOKING
		densityOfTiles.PushBack( info );
	}

	struct TileDensitySorter
	{
		RED_INLINE Bool operator()( TileDensityInfo& td1, TileDensityInfo& td2 )  const { return td1.numberOfObjects > td2.numberOfObjects; }
	};
	Sort( densityOfTiles.Begin(), densityOfTiles.End(), TileDensitySorter() );

	for ( TileDensityInfo& info : densityOfTiles )
	{
		RED_LOG( UmbraInfo, TXT( "Calculating occlusion for tile [%d; %d]" ), info.coordinates.X, info.coordinates.Y );

		context.tileId = info.coordinates;
		Bool status = GenerateOcclusionForTileSync( context, dumpScene, dumpRawTomeData );

		RED_LOG( UmbraInfo, TXT( "Finished occlusion calculation data for tile [%d; %d] - %s" ), info.coordinates.X, info.coordinates.Y, status ? TXT("SUCCEEDED") : TXT("FAILED") );
		if ( !status )
		{
			// early exit
			return false;
		}
	}

	m_umbraScene->SetIsDuringSyncRecreation( false );
	return true;
#endif

	return false;
}

String GetMemDesc( size_t dataSize, Bool precise = false )
{
	return precise ? String::FormatByteNumber( dataSize, String::BNF_Precise ) : ToString( dataSize );
}

Bool CWorld::CalculateRuntimeOcclusionMemoryUsage( const Float density )
{
	if ( !m_umbraScene )
	{
		return false;
	}
	if ( density < 0.5f )
	{
		return false;
	}

	// get min and max dimentions along each axis
	Int32 mx = Red::NumericLimits< Int32 >::Max();
	Int32 mn = Red::NumericLimits< Int32 >::Min();
	Rect iBounds( mx, mn, mn, mx );
	TOcclusionGrid::ElementList elements;
	m_umbraScene->GetGrid().GetAllElements( elements );
	for ( auto element : elements )
	{
		CUmbraTile* tile = element.m_Data;
		RED_FATAL_ASSERT( tile, "" );
		if ( tile->GetStatus() == EUTDS_Valid )
		{
			const VectorI& coords = tile->GetCoordinates();
			// X coordinate, [min, max]
			iBounds.m_left = Min< Int32 >( iBounds.m_left, coords.X );
			iBounds.m_right = Max< Int32 >( iBounds.m_right, coords.X );

			// Y coordinate, [min, max]
			iBounds.m_bottom = Min< Int32 >( iBounds.m_bottom, coords.Y );
			iBounds.m_top = Max< Int32 >( iBounds.m_top, coords.Y );
		}
	}

	const Box minBox = m_umbraScene->GetBoundingBoxOfTile( VectorI( iBounds.m_left, iBounds.m_bottom ) );
	const Box maxBox = m_umbraScene->GetBoundingBoxOfTile( VectorI( iBounds.m_right, iBounds.m_top ) );

	RectF bounds( minBox.Min.X, maxBox.Max.X, maxBox.Max.Y, minBox.Min.Y );

	Float x = bounds.m_left;
	Float y = bounds.m_bottom;

	UmbraMemoryStats statsMaxUmbraPoolAllocatedMemory;
	UmbraMemoryStats statsMaxTomeCollectionSize;
	UmbraMemoryStats statsMaxTomeCollectionScratchAllocationPeak;
	UmbraMemoryStats statsMaxNumberOfLoadedTiles;
	while ( true )
	{
		Vector position( x, y, 0.0f );
		UmbraMemoryStats memoryStats;
		memoryStats.m_position = position;

		Bool updateStats = m_umbraScene->GetMemoryStatsForPosition( position, memoryStats );

		x += density;
		if ( x >= bounds.m_right )
		{
			// next line
			y += density;
			if ( y >= bounds.m_top )
			{
				break;
			}
			x = bounds.m_left;
		}

		if ( !updateStats )
		{
			continue;
		}

		LOG_ENGINE( TXT("pos;%1.2f;%1.2f;loadedTiles;%d;memory;%s;tomeCollection;%s;tempPeak;%s"), position.X, position.Y, memoryStats.m_numberOfLoadedTiles,
																						GetMemDesc( memoryStats.m_umbraPoolAllocatedMemory ).AsChar(),
																						GetMemDesc( memoryStats.m_tomeCollectionSize ).AsChar(),
																						GetMemDesc( memoryStats.m_tempBufferAllocatedMemoryPeak ).AsChar()
																						);

		if ( memoryStats.m_numberOfLoadedTiles > statsMaxNumberOfLoadedTiles.m_numberOfLoadedTiles )
		{
			statsMaxNumberOfLoadedTiles = memoryStats;
		}
		if ( memoryStats.m_tomeCollectionScratchAllocationPeak > statsMaxTomeCollectionScratchAllocationPeak.m_tomeCollectionScratchAllocationPeak )
		{
			statsMaxTomeCollectionScratchAllocationPeak = memoryStats;
		}
		if ( memoryStats.m_tomeCollectionSize > statsMaxTomeCollectionSize.m_tomeCollectionSize )
		{
			statsMaxTomeCollectionSize = memoryStats;
		}
		if ( memoryStats.m_umbraPoolAllocatedMemory > statsMaxUmbraPoolAllocatedMemory.m_umbraPoolAllocatedMemory )
		{
			statsMaxUmbraPoolAllocatedMemory = memoryStats;
		}
	}

	// log stats
	LOG_ENGINE( TXT("World: %s"), GetDepotPath().AsChar() );
	LOG_ENGINE( TXT("MaxUmbraPoolAllocatedMemory: %s, [%s]"), GetMemDesc( statsMaxUmbraPoolAllocatedMemory.m_umbraPoolAllocatedMemory ).AsChar(), ToString( statsMaxUmbraPoolAllocatedMemory.m_position ).AsChar() );
	LOG_ENGINE( TXT("MaxTomeCollectionSize: %s, [%s]"), GetMemDesc( statsMaxTomeCollectionSize.m_tomeCollectionSize).AsChar(), ToString( statsMaxTomeCollectionSize.m_position ).AsChar() );
	LOG_ENGINE( TXT("MaxTomeCollectionScratchAllocationPeak: %s, [%s]"), GetMemDesc( statsMaxTomeCollectionScratchAllocationPeak.m_tomeCollectionScratchAllocationPeak ).AsChar(), ToString( statsMaxTomeCollectionScratchAllocationPeak.m_position ).AsChar() );
	LOG_ENGINE( TXT("MaxNumberOfLoadedTiles: %d, [%s]"), statsMaxNumberOfLoadedTiles.m_numberOfLoadedTiles, ToString( statsMaxNumberOfLoadedTiles.m_position ).AsChar() );

	return true;
}

void CWorld::NotifyOcclusionSystem( CEntity* entity )
{
}

#endif // NO_EDITOR
#endif // USE_UMBRA

CFoliageEditionController & CWorld::GetFoliageEditionController() 
{ 
	return *m_foliageEditionController; 

}

const CFoliageEditionController & CWorld::GetFoliageEditionController() const 
{ 
	return *m_foliageEditionController; 
}

CFoliageDynamicInstanceService CWorld::CreateFoliageDynamicInstanceService()
{
	return m_foliageScene->CreateDynamicInstanceController();
}

void CWorld::funcGetWaterLevel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, dontUseApproximation, false );
	FINISH_PARAMETERS;

	const Float rv = GetWaterLevel( point, dontUseApproximation ? 0 : 1 );

	RETURN_FLOAT( rv );
}

void CWorld::funcGetWaterDepth( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, dontUseApproximation, false );
	FINISH_PARAMETERS;
	
	Float res = 0;
	if ( m_terrainClipMap )
	{
		const Float waterLevel = GetWaterLevel( point, dontUseApproximation ? 0 : 1 );
		Float height;
		m_terrainClipMap->GetHeightForWorldPosition( point, height );

		res = waterLevel-height;
		if ( res < 0.0f || point.Z < height )
		{
			res = 10000.0f;
			if ( waterLevel > -10000.0f && m_physicsWorld )
			{
				SPhysicsContactInfo ci;
				CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) );
				point.Z += 0.5f;
				Bool rv = m_physicsWorld->RayCastWithSingleResult( point, point-Vector::EZ*10000.0f, include, 0, ci ) == TRV_Hit;
				if ( rv )
				{
					res = waterLevel-ci.m_position.Z;
				}
			}
		}
	}

	RETURN_FLOAT( res );
}

void CWorld::funcGetWaterTangent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::EY );
	GET_PARAMETER( Float, resolution, 0.25f );
	FINISH_PARAMETERS;

	direction.Normalize2();
	if ( resolution < 0.001f )
		resolution = 0.001f;

	Vector pt = point;
	Vector front = point+direction*resolution;
	pt.Z = GetWaterLevel( pt, 0 );
	front.Z = GetWaterLevel( front, 0 );
	Vector res = front-pt;
	res.Normalize3();

	RETURN_STRUCT( Vector, res );
}

void CWorld::funcStaticTrace( CScriptStackFrame& stack, void* result )
{
	Vector v = Vector::ZEROS;

	GET_PARAMETER( Vector, pointA, v );
	GET_PARAMETER( Vector, pointB, v );
	GET_PARAMETER_REF( Vector, position, v );
	GET_PARAMETER_REF( Vector, normal, v );
	GET_PARAMETER_OPT( TDynArray<CName>, collisionTypeNames, TDynArray<CName>() );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask include = 0;
	if ( collisionTypeNames.Empty() )
	{
		include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	}
	else
	{
		for( Uint32 i = 0; i < collisionTypeNames.Size(); ++i )
		{
			include |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	if ( m_physicsWorld == nullptr )
	{
		ASSERT( false && TXT("No physics world while tracing!") );
		RETURN_BOOL( false );
	}
	else
	{
		SPhysicsContactInfo ci;
		Bool rv = m_physicsWorld->RayCastWithSingleResult( pointA, pointB, include, 0, ci ) == TRV_Hit;
		position = ci.m_position;
		normal = ci.m_normal;

		RETURN_BOOL( rv );
	}
}

void CWorld::funcStaticTraceWithAdditionalInfo( CScriptStackFrame& stack, void* result )
{
	Vector v = Vector::ZEROS;
	CName mat;
	THandle<CComponent> cComp = nullptr;
	GET_PARAMETER( Vector, pointA, v );
	GET_PARAMETER( Vector, pointB, v );
	GET_PARAMETER_REF( Vector, position, v );
	GET_PARAMETER_REF( Vector, normal, v );
	GET_PARAMETER_REF( CName, material, mat );
	GET_PARAMETER_REF( THandle<CComponent>, parent, cComp );
	GET_PARAMETER_OPT( TDynArray<CName>, collisionTypeNames, TDynArray<CName>() );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask include = 0;
	if ( collisionTypeNames.Empty() )
	{
		include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	}
	else
	{
		for( Uint32 i = 0; i < collisionTypeNames.Size(); ++i )
		{
			include |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	if ( m_physicsWorld == nullptr )
	{
		ASSERT( false && TXT("No physics world while tracing!") );
		RETURN_BOOL( false );
	}
	else
	{
		SPhysicsContactInfo ci;
		Bool rv = m_physicsWorld->RayCastWithSingleResult( pointA, pointB, include, 0, ci ) == TRV_Hit;
		position = ci.m_position;
		CPhysicsWrapperInterface* userData = nullptr;
		if(ci.m_userDataA)
		{
			userData = reinterpret_cast< CPhysicsWrapperInterface* >( ci.m_userDataA );
		}
		if(userData)
		{
			SPhysicalMaterial * mat = userData->GetMaterial(ci.m_rigidBodyIndexA.m_actorIndex, ci.m_rigidBodyIndexA.m_shapeIndex);
			material = mat ? mat->m_name : CNAME( default );

			CComponent* component = nullptr;
			userData->GetParent( component, static_cast< Uint32 >( ci.m_rigidBodyIndexA.m_actorIndex ) );
			parent = component;
		}

		normal = ci.m_normal;

		RETURN_BOOL( rv );
	}
}


void CWorld::funcSweepTest( CScriptStackFrame& stack, void* result )
{
	const Vector v = Vector::ZEROS;
	static TDynArray< CName > fallbackArray;

	GET_PARAMETER( Vector, pointA, v );
	GET_PARAMETER( Vector, pointB, v );
	GET_PARAMETER( Float, radius, 0.2f );
	GET_PARAMETER_REF( Vector, position, v );
	GET_PARAMETER_REF( Vector, normal, v );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, fallbackArray );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask include;
	if ( !collisionTypeNames.Empty() )
	{
		include = 0;
		for ( auto it = collisionTypeNames.Begin(), end = collisionTypeNames.End(); it != end; ++it )
		{
			include |= GPhysicEngine->GetCollisionTypeBit( *it );
		}
	}
	else
	{
		include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	}

	if ( m_physicsWorld == nullptr )
	{
		ASSERT( false && TXT("No physics world while tracing!") );
		RETURN_BOOL( false );
	}
	else
	{
		SPhysicsContactInfo ci;
		Bool rv = m_physicsWorld->SweepTestWithSingleResult( pointA, pointB, radius, include, 0, ci ) == TRV_Hit;
		position = ci.m_position;
		normal = ci.m_normal;
		RETURN_BOOL( rv );
	}
}

void CWorld::funcSphereOverlapTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< THandle< CEntity > >, output, TDynArray< THandle< CEntity > > () );
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0 );
	GET_PARAMETER_OPT( TDynArray<CName>, collisionGroupsNames, TDynArray<CName>() );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask collisionMask = 0;

	if ( collisionGroupsNames.Empty() )
	{
		// since we're interested in entities only we don't use 'Terrain' group as default
		collisionMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) );
	}
	else
	{
		collisionMask = GPhysicEngine->GetCollisionTypeBit( collisionGroupsNames );
	}

	if ( m_physicsWorld == nullptr )
	{
		ASSERT( false && TXT("No physics world for sphere overlap test!") );
	}
	else
	{
		const Uint32 SPHERE_OVERLAP_MAX_RESULTS = 20;
		SPhysicsOverlapInfo infos[ SPHERE_OVERLAP_MAX_RESULTS ];
		Uint32 hits = 0;
		const ETraceReturnValue retVal = m_physicsWorld->SphereOverlapWithMultipleResults( position, radius, collisionMask, 0, infos, hits, SPHERE_OVERLAP_MAX_RESULTS );
		for ( Uint32 i = 0; i < hits; ++i )
		{
			if ( infos[i].m_userData == nullptr )
			{
				continue;
			}

			CComponent* actorComponent = nullptr;
			if ( !infos[i].m_userData->GetParent( actorComponent, infos[i].m_actorNshapeIndex.m_actorIndex ) )
			{
				continue;
			}

			CEntity* entity = actorComponent->GetEntity();
			if ( entity != nullptr )
			{
				output.PushBackUnique( entity );
			}
		}
	}
	RETURN_INT( output.Size() );
}

void CWorld::funcNavigationLineTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos1, Vector::ZEROS );
	GET_PARAMETER( Vector, pos2, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_OPT( Bool, ignoreObstacles, false );
	GET_PARAMETER_OPT( Bool, noEndpointZ, false );
	FINISH_PARAMETERS;

	Bool ret = false;
	if ( m_pathLib )
	{
		Uint32 flags = PathLib::CT_DEFAULT;
		if ( ignoreObstacles )
		{
			flags |= PathLib::CT_NO_OBSTACLES;
		}
		if ( noEndpointZ )
		{
			flags |= PathLib::CT_NO_ENDPOINT_TEST;
		}

		if ( radius > 0.f )
		{
			ret = m_pathLib->TestLine( pos1.AsVector3(), pos2.AsVector3(), radius, flags );
		}
		else
		{
			ret = m_pathLib->TestLine( pos1.AsVector3(), pos2.AsVector3(), flags );
		}
	}
	RETURN_BOOL( ret );
}
void CWorld::funcNavigationCircleTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_OPT( Bool, ignoreObstacles, false );
	FINISH_PARAMETERS;

	Bool ret = false;
	if ( m_pathLib )
	{
		Uint32 flags = PathLib::CT_DEFAULT;
		if ( ignoreObstacles )
		{
			flags |= PathLib::CT_NO_OBSTACLES;
		}

		ret = m_pathLib->TestLocation( position.AsVector3(), radius, flags );
	}
	RETURN_BOOL( ret );
}

void CWorld::funcNavigationClosestObstacleToLine( CScriptStackFrame& stack, void* result )
{
	Vector v = Vector::ZEROS;

	GET_PARAMETER( Vector, pos1, Vector::ZEROS );
	GET_PARAMETER( Vector, pos2, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_REF( Vector, closestPointOnLine, v );
	GET_PARAMETER_REF( Vector, closestPointOnGeometry, v );
	GET_PARAMETER_OPT( Bool, ignoreObstacles, false );
	FINISH_PARAMETERS;

	Float ret = FLT_MAX;
	if ( m_pathLib )
	{
		Uint32 flags = PathLib::CT_DEFAULT;
		if ( ignoreObstacles )
		{
			flags |= PathLib::CT_NO_OBSTACLES;
		}
		ret = m_pathLib->GetClosestObstacle( pos1.AsVector3(), pos2.AsVector3(), radius, closestPointOnGeometry.AsVector3(), closestPointOnLine.AsVector3(), flags );
	}
	RETURN_FLOAT( ret );
}
void CWorld::funcNavigationClosestObstacleToCircle( CScriptStackFrame& stack, void* result )
{
	Vector v = Vector::ZEROS;

	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_REF( Vector, closestPointOnGeometry, v );
	GET_PARAMETER_OPT( Bool, ignoreObstacles, false );
	FINISH_PARAMETERS;

	Float ret = FLT_MAX;
	if ( m_pathLib )
	{
		Uint32 flags = PathLib::CT_DEFAULT;
		if ( ignoreObstacles )
		{
			flags |= PathLib::CT_NO_OBSTACLES;
		}
		ret = m_pathLib->GetClosestObstacle( position.AsVector3(), radius, closestPointOnGeometry.AsVector3(), flags );
	}
	RETURN_FLOAT( ret );
}

void CWorld::funcNavigationClearLineInDirection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos1, Vector::ZEROS );
	GET_PARAMETER( Vector, pos2, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_REF( Vector, outPos, Vector::ZEROS );
	FINISH_PARAMETERS;

	Bool ret = false;
	if ( m_pathLib )
	{
		PathLib::AreaId areaId = PathLib::INVALID_AREA_ID;
		Uint32 flags = PathLib::CT_DEFAULT;
		ret = m_pathLib->GetClearLineInDirection( areaId, pos1.AsVector3(), pos2.AsVector3(), radius, outPos.AsVector3() ) != PathLib::CLEARLINE_INVALID_START_POINT;
	}

	RETURN_BOOL( ret );
}

void CWorld::funcNavigationFindSafeSpot( CScriptStackFrame& stack, void* result )
{
	Vector v = Vector::ZEROS;

	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Float, personalSpace, 0.1f );
	GET_PARAMETER( Float, searchRadius, 1.0f );
	GET_PARAMETER_REF( Vector, walkableSpot, v );
	FINISH_PARAMETERS;

	v = position;

	Bool ret = false;
	if ( m_pathLib )
	{
		PathLib::AreaId areaId = PathLib::INVALID_AREA_ID;
		ret = m_pathLib->FindSafeSpot( areaId, position.AsVector3(), searchRadius, personalSpace, walkableSpot.AsVector3() );
	}
	RETURN_BOOL( ret );
}

void CWorld::funcNavigationComputeZ( CScriptStackFrame& stack, void* result )
{
	Float v = 0.f;

	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Float, zMin, v );
	GET_PARAMETER( Float, zMax, v );
	GET_PARAMETER_REF( Float, zOut, v );
	FINISH_PARAMETERS;

	Bool ret = false;
	if ( m_pathLib )
	{
		PathLib::AreaId tmpShit = PathLib::INVALID_AREA_ID;
		ret = m_pathLib->ComputeHeightTop( position.AsVector2(), zMin, zMax, zOut, tmpShit );
	}

	RETURN_BOOL( ret );
}

void CWorld::funcPhysicsCorrectZ( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER_REF( Float, zOut, 0.f );
	FINISH_PARAMETERS;

	CPhysicsWorld* physcisWorld = nullptr;

	Bool m_didZCorrection = false;
	zOut = position.Z;

	if ( GetPhysicsWorld( physcisWorld ) && physcisWorld->IsAvaible( position ) ) // make sure that physics data is streamed-in
	{
		SPhysicsContactInfo outInfo;
		Vector from( position.X, position.Y, position.Z + 1.1f );
		Vector to  ( position.X, position.Y, position.Z - 1.1f );

		const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) )
			| GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) 
			| GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) ) 
			| GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

		if ( m_physicsWorld->RayCastWithSingleResult( from, to, includeMask, 0, outInfo ) == TRV_Hit )
		{
			zOut = outInfo.m_position.Z;
			m_didZCorrection = true;
		}
	}

	RETURN_BOOL( m_didZCorrection );
}

void CWorld::funcGetDepotPath( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
    RETURN_STRING( DepotPath() );
}
void CWorld::funcForceGraphicalLOD( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, lodLevel, -1 );
	FINISH_PARAMETERS;
	( new CRenderCommand_ChangeSceneForcedLOD( this->GetRenderSceneEx(), lodLevel ) )->Commit();
	m_forcedGraphicalLOD = lodLevel;
	RETURN_VOID();
}

void CWorld::funcGetTerrainParameters( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Float, terrainSize, 0 );
	GET_PARAMETER_REF( Uint32, tilesCount, 0 );
	FINISH_PARAMETERS;

	SClipmapParameters params;

	if ( !m_terrainClipMap )
	{
		RETURN_BOOL( false );
		return;
	}
	m_terrainClipMap->GetClipmapParameters( &params );
	terrainSize = params.terrainSize;
	tilesCount = params.clipmapSize / params.tileRes;

	RETURN_BOOL( true );
}

void CWorld::funcGetTraceManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_physicsBatchQueryManager->GetScriptAccessor() );
}

void CWorld::ForceFinishAsyncResourceLoads()
{
	PC_SCOPE( ForceFinishAsyncResourceLoads );

	for ( CLayer* layer : m_attachedLayers )
	{
		for ( CEntity* entity : layer->GetEntities() )
		{
			if ( entity->IsAttached() && !entity->IsDetaching() )
			{
				entity->ForceFinishAsyncResourceLoads();
			}
		}
	}
}

void CWorld::PreLayersAttach()
{
	m_gameStartInProgress = true;
}

void CWorld::RegisterForOnGameStarted( CNode* node )
{
	if ( m_gameStartInProgress )
	{
		m_onGameStartedQueue.Insert( node );
	}
}

void CWorld::OnGameStarted( const CGameInfo& info )
{
	for ( CNode* node : m_onGameStartedQueue )
	{
		node->OnGameStarted();
	}

	m_onGameStartedQueue.Clear();
	m_gameStartInProgress = false;
}

#ifndef NO_RESOURCE_IMPORT

namespace Helper
{
	class CMergedWorldGeometrySupplierPersistentLayersOnly : public IMergedWorldGeometrySupplier
	{
	public:
		CMergedWorldGeometrySupplierPersistentLayersOnly( THandle< CWorld > world )
			: m_world( world )
		{
			// get all layers that are loaded right now
			world->GetWorldLayers()->GetLayers( m_initialLoadedSet, true, true );

			// get all PERSISTENT layers in the world
			TDynArray< CLayerInfo* > presistentLayers;
			world->GetWorldLayers()->GetPersistentLayers( presistentLayers, false, true );

			// global task description
			GFeedback->UpdateTaskName( TXT("Loading world data") );

			// load all of the persistent layers
			for ( Uint32 i=0; i<presistentLayers.Size(); ++i )
			{
				CLayerInfo* layer = presistentLayers[i];

				GFeedback->UpdateTaskInfo( TXT("Loading layer '%ls'"), layer->GetDepotPath().AsChar() );
				GFeedback->UpdateTaskProgress( i, presistentLayers.Size() );

				LayerLoadingContext loadingContext;
				const Bool wasLoaded = layer->IsLoaded();
				if ( layer->SyncLoad( loadingContext ) )
				{
					// we loaded this layer
					if ( !wasLoaded )
					{
						m_loadedLayers.PushBack( layer );
					}

					// add to list
					m_interestingLayers.PushBack( layer );

					// get all of the entities in that layer
					if ( layer->GetLayer() )
					{
						TDynArray< CEntity* > allEntities;
						layer->GetLayer()->GetEntities( allEntities );

						for ( CEntity* entity : allEntities )
						{
							m_interestingEntities.PushBack( entity );
						}
					}
				}
			}

			// stats
			LOG_ENGINE( TXT("Found %d intersting entities in %d layers"), 
				m_interestingEntities.Size(), m_interestingLayers.Size() );
		}

		~CMergedWorldGeometrySupplierPersistentLayersOnly()
		{
			CleanupStreamedEntities();
			CleanupLoadedLayers();
		}

		virtual const Vector ProjectOnTerrain( const Vector& pos ) const override
		{
			// use terrain height if avaiable
			Float height = 0.0f;
			if ( m_world->GetTerrain()->GetHeightForWorldPosition( pos, height ) )
			{
				return Vector( pos.X, pos.Y, height );
			}

			// default to something deterministic
			return Vector( pos.X, pos.Y, 0.0f );
		}

		virtual const Float GetWorldSize() const override
		{
			return m_world->GetWorldDimensionsWithTerrain();
		}

		virtual void GetEntitiesFromArea( const Box& worldSpaceBox, TDynArray< THandle< CEntity > >& outEntities ) const override
		{
			// Unload entities that we artificially loaded
			CleanupStreamedEntities();

			// gather new entities
			for ( CEntity* entity : m_interestingEntities )
			{
				if ( entity && worldSpaceBox.Contains( entity->GetWorldPositionRef() ) )
				{
					const Bool isStreamed = entity->IsStreamedIn();
					if ( entity->CheckStaticFlag( ESF_Streamed ) )
					{
						entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );

						// unload on next batch
						if ( isStreamed )
						{
							m_streamedEntities.PushBack( entity );
						}
					}

					outEntities.PushBack( entity );
				}
			}
		}

	private:
		THandle< CWorld >							m_world;
		TDynArray< CLayerInfo* >					m_interestingLayers;
		TDynArray< THandle< CEntity > >				m_interestingEntities;

		mutable TDynArray< THandle< CEntity > >		m_streamedEntities;
		TDynArray< CLayerInfo* >					m_loadedLayers;

		TDynArray< CLayerInfo* >					m_initialLoadedSet;

		void CleanupStreamedEntities() const
		{
			// unload previous entities
			for ( CEntity* entity : m_streamedEntities )
			{
				if ( entity )
				{
					entity->DestroyStreamedComponents( SWN_DoNotNotifyWorld );
				}
			}
			m_streamedEntities.ClearFast();
		}

		void CleanupLoadedLayers()
		{
			// get the merged content layer
			CLayerInfo* mergedContentLayer = nullptr;
			if ( m_world->GetMergedGeometryContainer() )
			{
				auto contentLayer = m_world->GetMergedGeometryContainer()->GetContentLayer( m_world );
				if ( contentLayer )
				{
					mergedContentLayer = contentLayer->GetLayerInfo();
				}
			}

			// unload all layers from the world just to make sure everything is refreshed
			GFeedback->UpdateTaskInfo( TXT( "Refreshing content...") );
			GFeedback->UpdateTaskProgress( 0, 0 );
			m_world->GetWorldLayers()->SyncUnload();

			// restore all of the initial layers set
			for ( Uint32 i=0; i<m_initialLoadedSet.Size(); ++i )
			{
				CLayerInfo* layer = m_initialLoadedSet[i];
				GFeedback->UpdateTaskProgress( i, m_initialLoadedSet.Size() );

				LayerLoadingContext loadingContext;
				layer->SyncLoad( loadingContext );
			}
			m_initialLoadedSet.ClearFast();

			// make sure the merged content layer is loaded
			if ( mergedContentLayer )
			{
				LayerLoadingContext loadingContext;
				mergedContentLayer->SyncLoad( loadingContext );
			}
		}
	};
}

/// TODO: this function is here and not in the editor only because we may use it from the WCC :(

const Bool CWorld::RebuildMergedGeometry( const Vector& worldCenter, const Float worldRadius )
{
	// no merged geometry container
	if ( !m_mergedGeometry )
		return false;

	// prepare merged layer - it will be used to place merged content in the world
	THandle< CLayer > mergedLayer = m_mergedGeometry->GetContentLayer( this );
	if ( !mergedLayer )
	{
		ERR_ENGINE( TXT("Failed to create merged content layer for current world. Check P4 error messages.") );
		return false;
	}

	// prepare content directory - it will be used to store automaticaly generated assets
	CDirectory* contentDir = m_mergedGeometry->GetContentDirectory( this );
	if ( !contentDir )
	{
		ERR_ENGINE( TXT("Failed to create content directory for current world. Check P4 error messages.") );
		return false;
	}

	// build the merged geometry from the suitable set of data from this world (only persistent layers, no gameplay layers so far)
	{
		Helper::CMergedWorldGeometrySupplierPersistentLayersOnly layerAccess( this );
		if ( !m_mergedGeometry->Build( contentDir, mergedLayer, &layerAccess, worldCenter, worldRadius ) )
		{
			ERR_ENGINE( TXT("Failed to build merged geometry. Check P4 error messages.") );
			return false;
		}

		// save the final layer
		mergedLayer->Save();
	}

	// TODO: any more P4 bullshit in here ?

	// done
	return true;
}

#endif

// W3 hack to glue streaming prefetch with rendering stuff WITHOUT shitloads of additional classes/headers
void* GetPrefetchedStreamingData( const Uint64 hash, Uint32 expectedDataSize, std::function< void* ( Uint32 dataSize, Uint16 alignment ) > allocator, std::function< void ( void* memory ) > deallocator )
{
	if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetStreamingDataPrefetch() )
		return GGame->GetActiveWorld()->GetStreamingDataPrefetch()->GetCachedData( hash, expectedDataSize, allocator, deallocator );

	return nullptr;
}