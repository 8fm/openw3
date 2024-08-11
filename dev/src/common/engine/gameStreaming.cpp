/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "game.h"
#include "world.h"
#include "worldStreaming.h"
#include "layerInfo.h"
#include "gameUniverseStorage.h"
#include "renderCommands.h"
#include "viewport.h"
#include "../physics/physicsWorld.h"
#include "foliageScene.h"
#include "clipMap.h"
#include "../physics/physicsWrapper.h"
#include "streamingAreaComponent.h"
#include "tagManager.h"
#include "umbraScene.h"

#include "../../common/core/profiler.h"
#include "../../common/core/loadingProfiler.h"
#include "../../common/core/configVar.h"
#include "../../common/core/deferredDataBufferKickoff.h"
#include "../../common/core/messagePump.h"

#include "entityTickManager.h"
#include "tickManager.h"
#include "baseEngine.h"
#include "pathlibWorld.h"
#include "pathlibStreamingManager.h"
#include "../core/objectGC.h"
#include "areaComponent.h"
#include "../core/fileSystemProfilerWrapper.h"

#include "renderFramePrefetch.h"
#include "renderSettings.h"

//---

namespace Config
{
		// one frame movement distance for which the prefetch is always peformed
		TConfigVar< Float >		cvStreamingPrefetchHardDistance( "Streaming/Pefetch", "HardDistance", 25.0f );

		// one frame movement distance for which the prefetch is considered
		TConfigVar< Float >		cvStreamingPrefetchSoftDistance( "Streaming/Pefetch", "SoftDistance", 10.0f );

		// maximum time we can wait for pathlib
		TConfigVar< Float >		cvStreamingPrefetchMaxPathLibWaitTime( "Streaming/Pefetch", "MaxPathLibWaitTime", 10.0f );

		// maximum time we can wait for the textures
		TConfigVar< Float >		cvStreamingPrefetchMaxTexturesWaitTime( "Streaming/Pefetch", "MaxTexturesWaitTime", 15.0f );

		// maximum time we can wait for the the community fast forward
		TConfigVar< Float >		cvStreamingPrefetchMaxCommunitiesWaitTime( "Streaming/Pefetch", "MaxCommunityWaitTime", 15.0f );

		// maximum time we can wait for the envprobes
		TConfigVar< Float >		cvStreamingPrefetchMaxEnvProbesWaitTime( "Streaming/Pefetch", "MaxEnvProbesWaitTime", 30.0f );

		// maximum time we can wait for Umbra
		TConfigVar< Float >		cvStreamingPrefetchMaxUmbraWaitTime( "Streaming/Pefetch", "MaxUmbraWaitTime", 15.0f );

		// minimum difference in tile count that causes the whole Umbra structure to reload
		TConfigVar< Int32 >		cvStreamingPrefetchMinUmbraTileDifference( "Streaming/Pefetch", "MinUmbraTileDifference", 10 );
}
//---

CGame::SWorldLayersStreaming::SWorldLayersStreaming( CWorldLayerStreamingFence* fence )
	: m_fence( fence )
{
	if ( m_fence )
		m_fence->AddRef();
}

CGame::SWorldLayersStreaming::~SWorldLayersStreaming()
{
	if ( m_fence )
	{
		m_fence->Release();
		m_fence = nullptr;
	}
}

//---

Bool CGame::ScheduleLayersVisibilityChange( const String& world, const TDynArray< String >& groupsToHide, const TDynArray< String >& groupsToShow, Bool purgeStorages, CWorldLayerStreamingFence* optionalFence /*= nullptr*/ )
{
	// Verify world path
	String worldPath = world;
	if ( worldPath.Empty() && nullptr != GGame->GetActiveWorld() )
	{
		worldPath = GGame->GetActiveWorld()->GetDepotPath();	
	}

	// Well, no path
	if ( worldPath.Empty() )
	{
		RED_LOG_ERROR( Engine, TXT("ScheduleLayersVisibilityChange() failed: wrong or empty world path. Please DEBUG!") );
		return false;
	}

	// Add to list 
	SWorldLayersStreaming* change = new SWorldLayersStreaming( optionalFence );
	change->m_world = worldPath;
	change->m_groupsToHide = groupsToHide;
	change->m_groupsToShow = groupsToShow;
	change->m_purgeStorages = purgeStorages;
	m_worldChanges.PushBack( change );
	return true;
}

void CGame::ProcessScheduledLayerVisibilityChanges()
{
	PC_SCOPE( ProcessScheduledLayerVisibilityChanges );

	// get current list
	TDynArray< SWorldLayersStreaming* > loadingTasks;
	::Swap( loadingTasks, m_worldChanges );

	// Process the loading tasks
	for ( SWorldLayersStreaming* task : loadingTasks )
	{
		// only tasks for our world
		if ( m_activeWorld && task->m_world.EqualsNC( m_activeWorld->GetDepotPath() ) )
		{
			m_activeWorld->ChangeLayersVisibility( task->m_groupsToHide, task->m_groupsToShow, task->m_purgeStorages, task->m_fence );
		}

		// update the storage system
		m_universeStorage.OnLayersVisibilityChanged( task->m_world, task->m_groupsToHide, task->m_groupsToShow );

		// delete the task - no longer needed
		delete task;
	}
}

//---

void CGame::EnableCameraBasedStreaming( const String& idName, const Float softDistance, const Float hardDistance )
{
	// already added ?
	for ( const auto it : m_streamingCameraOverrides )
	{
		if ( it.m_name == idName )
		{
			ERR_ENGINE( TXT("Streaming camera override '%ls' already exists"), idName.AsChar() );
			return;
		}
	}

	// add new override
	SCameraStreamingOverride* ovr = new ( m_streamingCameraOverrides ) SCameraStreamingOverride;
	ovr->m_name = idName;
	ovr->m_softDistance = softDistance;
	ovr->m_hardDistance = hardDistance;

	// recompute the soft/hard distance
	UpdateStreamingModeAndDistances();
}

void CGame::DisableCameraBasedStreaming( const String& idName )
{
	for ( Uint32 i=0; i<m_streamingCameraOverrides.Size(); ++i )
	{
		if ( m_streamingCameraOverrides[i].m_name == idName )
		{
			m_streamingCameraOverrides.RemoveAt( i );
			return;
		}
	}

	ERR_ENGINE( TXT("Streaming camera override '%ls' not found and cannot be deleted"), idName.AsChar() );
}

static const Uint32 STREAMING_LOCKS_FRAME_MAX = 0xFFFFFFFF;

void CGame::EnableStreamingLockdown( const String& idName, const CName areaTag )
{
	// no active world
	if ( !m_activeWorld )
	{
		ERR_ENGINE( TXT("Unable to add streaming lock for '%ls' area '%ls' because there's no world"), idName.AsChar(), areaTag.AsChar() );
		return;
	}

	// already added ?
	for ( auto& it : m_streamingLocks )
	{
		if ( it.m_areaTag == areaTag )
		{
			if ( it.m_toRemoveOnFrame != STREAMING_LOCKS_FRAME_MAX )
			{
				LOG_ENGINE( TXT("Streaming lock for area '%ls' was ressurected with no penalty"), areaTag.AsChar() );
				it.m_toRemoveOnFrame = STREAMING_LOCKS_FRAME_MAX;
			}
			else
			{
				ERR_ENGINE( TXT("Streaming lock for area '%ls' already exists"), areaTag.AsChar() );
			}

			return;
		}
	}

	// collect entities
	TDynArray< CEntity* > gameEntities;
	m_activeWorld->GetTagManager()->CollectTaggedEntities( areaTag, gameEntities );
	if ( gameEntities.Empty() )
	{
		ERR_ENGINE( TXT("Streaming lock for '%ls' cannot be applied because there are no entities tagged '%ls'"), idName.AsChar(), areaTag.AsChar() );
		return;
	}

	// collect areas
	TDynArray< Box > areas;
	for ( CEntity* entity : gameEntities )
	{
		for ( CComponent* comp : entity->GetComponents() )
		{
			// We should use CStreamingAreaComponent here but we can not change the content now (patch1.1)
			// Please change it back after patch1.1 to CStreamingAreaComponent
			if ( comp->IsA< CAreaComponent >() )
			{
				CAreaComponent* area = static_cast< CAreaComponent* >( comp );
				areas.PushBack( area->GetBoundingBox() );
			}
		}
	}

	// no streaming areas
	if ( areas.Empty() )
	{
		ERR_ENGINE( TXT("Streaming lock for '%ls' cannot be applied because there are no streaming areas in entities tagged '%ls'"), idName.AsChar(), areaTag.AsChar() );
		return;
	}

	// add new streaming lock
	SStreamingLock* ovr = new ( m_streamingLocks ) SStreamingLock;
	ovr->m_name = idName;
	ovr->m_areaTag = areaTag;
	ovr->m_areas = areas;
	ovr->m_toRemoveOnFrame = STREAMING_LOCKS_FRAME_MAX;

	// force a refresh
	ERR_ENGINE( TXT("Streaming lock '%ls' added"), areaTag.AsChar() );
	m_streamingLocksModified = true;

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileStreamingLockAdded( areaTag.AsChar() );
#endif
}

void CGame::DisableStreamingLockdown( const CName areaTag )
{
	for ( Uint32 i=0; i<m_streamingLocks.Size(); ++i )
	{
		if ( m_streamingLocks[i].m_areaTag == areaTag )
		{
			m_streamingLocks[i].m_toRemoveOnFrame = m_streamingLocksFrameCounter + 2;
			LOG_ENGINE( TXT("Streaming lock '%ls' marked for removing"), areaTag.AsChar() );

#ifdef RED_PROFILE_FILE_SYSTEM
			RedIOProfiler::ProfileStreamingLockRemoved( areaTag.AsChar() );
#endif
			return;
		}
	}

	// lock not found
	ERR_ENGINE( TXT("Streaming lock '%ls' not found and not removed"), areaTag.AsChar() );
}

const Bool CGame::IsStreamingLockdownEnabled( const CName areaTag ) const
{
	for ( Uint32 i=0; i<m_streamingLocks.Size(); ++i )
	{
		if ( m_streamingLocks[i].m_areaTag == areaTag )
		{
			return true;
		}
	}

	return false;
}

class CGameScopedPrefetch : Red::System::NonCopyable
{
	Bool	m_showLoadingScreen;

public:
	CGameScopedPrefetch( Bool showLoadingScreen )
		: m_showLoadingScreen( showLoadingScreen )
	{
		GGame->OnGamePrefetchStart( showLoadingScreen );
	}

	~CGameScopedPrefetch()
	{
		GGame->OnGamePrefetchEnd( m_showLoadingScreen );
	}
};

void CGame::PerformPrefetch( const Bool parentShowLoadingScreen, const SPrefetchParamsW3Hack& params )
{
	SCOPED_ENABLE_PUMP_MESSAGES_DURANGO_CERTHACK();

	PUMP_MESSAGES_DURANGO_CERTHACK();

	CTimeCounter timer;
	PC_SCOPE( PrefetchLocation );

	////////////////////////////////////////
	
	Bool usedBlur = false;
	// NOTE : IsActive check is to avoid switching to a black screen when moving the camera fast in the editor. But it means
	// also that there won't be any loading screen in some other situations too, like during cutscene preview.
	const Bool showLoadingScreen = IsActive() && parentShowLoadingScreen && !IsLoadingScreenShown();
	Bool HACK_isFastTravel = false;

	CGameScopedPrefetch scopedPrefetch( showLoadingScreen );

	if ( showLoadingScreen )
	{
		// More cosmetic, but try to avoid blurring a blackscreen or fading out, which doesn't work so well
		// and looks really wrong when it's at the beginning of a cutscene after a loading blackscreen screen
		// Take m_blackscreenWasSetPrevFrame into account because previous frame (under black screen) can be corrupted
		const Bool hasBlackscreen = HasBlackscreenRequested() || HasBlackscreenLockRequested() || m_blackscreenWasSetPrevFrame;

		// Do we have any streaming locks active ?
		const Bool hasStreamingLocks = !m_streamingLocks.Empty();

		if ( UseCameraAsStreamingReference() && !hasBlackscreen && !hasStreamingLocks )
		{
			(new CRenderCommand_ShowLoadingScreenBlur(32.0f, 0.5f))->Commit();
			usedBlur = true;
		}
		else if ( m_loadingScreenStack.Size() > 1 ) // Fast traveling - FIXME: what if keeping the same? For now requires pushing anyway. Kind of hacky resetting the camera here based on a local loading screen being set or not!
		{
			// for marking as FT in the same world
			HACK_isFastTravel = true;

			// FIXME: Reset all the camera shit so it's at the prefetch position
			// instead of having it blend into position and cause the later texture
			// prefetching to fetch from the wrong position.
 			if ( m_activeWorld && m_activeWorld->GetCameraDirector() )
 			{
				CCameraDirector* cameraDirector = m_activeWorld->GetCameraDirector();
				cameraDirector->InvalidateLastFrameCamera();
				ResetGameplayCamera();
				ActivateGameCamera( 0.f );
				cameraDirector->Update(0.f);
				cameraDirector->CacheCameraData();

				CRenderCamera unused;
				cameraDirector->OnViewportCalculateCamera( m_viewport.Get(), unused );

				const Vector& camPos = cameraDirector->GetCameraPosition();
				const Vector camForward = cameraDirector->GetCameraForward();
				const Vector camUp = cameraDirector->GetCameraUp();
				{
					PC_SCOPE_PIX( WorldUpdateCameraPositionPrefetch );
					m_activeWorld->UpdateCameraPosition( camPos );
					m_activeWorld->UpdateCameraForward( camForward );
					m_activeWorld->UpdateCameraUp( camUp );
				}
 			}

			ShowLoadingScreen();
		}
		else
		{
			PushLoadingScreenParam( SLoadingScreenParam::BLACKSCREEN );
			const Uint32 color = (m_blackscreenColor.R << 16) | (m_blackscreenColor.G << 8 ) | m_blackscreenColor.B;
			const String hexString = String::Printf(TXT("0x%06X"), color );
			LOG_ENGINE(TXT("Prefetch blackscreen color %ls (%u)"), hexString.AsChar(), color );
			GetActiveLoadingScreenParam().m_initString = hexString;
			ShowLoadingScreen();
		}
	}

	PUMP_MESSAGES_DURANGO_CERTHACK();

	////////////////////////////////////////

	// start envprobe prefetch
	{
		RED_ASSERT( m_activeWorld->GetRenderSceneEx() != nullptr );

		// Start streaming
		( new CRenderCommand_SetupEnvProbesPrefetch( m_activeWorld->GetRenderSceneEx(), m_prefetchPosition ) )->Commit();

		// make sure the initial prefetch happens before we continue
		GRender->Flush();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

#ifdef USE_UMBRA
	{
		PC_SCOPE( PrefetchUmbraStart )
		RED_ASSERT( m_activeWorld->GetUmbraScene() != nullptr );
		RED_ASSERT( m_activeWorld->GetRenderSceneEx() != nullptr );

		CUmbraScene* scene = m_activeWorld->GetUmbraScene();

		Int32 diff = scene->GetDifferenceInTileCount( m_prefetchPosition );
		if ( diff >= Config::cvStreamingPrefetchMinUmbraTileDifference.Get() )
		{
			scene->CleanupActiveTomes( m_activeWorld->GetRenderSceneEx() );
			GRender->Flush();
		}
		scene->UpdateTomeCollection( m_prefetchPosition, m_activeWorld->GetRenderSceneEx() );

		scene->TickUntilStateOrIdle( m_activeWorld->GetRenderSceneEx(), CUmbraScene::TS_LoadTiles, Config::cvStreamingPrefetchMaxUmbraWaitTime.Get() );

		GLoadingProfiler.FinishStage( TXT("PrefetchUmbraStart") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
#endif // USE_UMBRA

	////////////////////////////////////////

	// load initial terrain. Only worry about this is the prefetch is happening as a result of the camera moving sufficiently far.
	// If it's only because there are new streaming locks or something, then we don't really need to do this expensive update, it
	// can be done with the normal per-frame thing.
	if ( params.m_didMoveFar )
	{
		PC_SCOPE( LoadTerrain )

		SClipMapUpdateInfo clipmapUpdateInfo;
		clipmapUpdateInfo.m_viewerPosition = m_prefetchPosition;
		clipmapUpdateInfo.m_timeDelta = 0.0f;
		clipmapUpdateInfo.m_forceSync = true;
		clipmapUpdateInfo.m_aggressiveEviction = true;

		m_activeWorld->GetTerrain()->ToggleLoading( true );
		m_activeWorld->GetTerrain()->TouchAndEvict( clipmapUpdateInfo );
		m_activeWorld->GetTerrain()->Update( clipmapUpdateInfo );
		
		GLoadingProfiler.FinishStage( TXT("PrefetchTerrain") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

#ifdef USE_UMBRA
	// wait for Umbra to finish loading from disk. Loading happens in a couple stages, so we'll try to split it up with
	// other meaningful work in between.
	{
		PC_SCOPE( PrefetchUmbraMid );

		// Tick until umbra is done with all loading
		m_activeWorld->GetUmbraScene()->TickUntilStateOrIdle( m_activeWorld->GetRenderSceneEx(), CUmbraScene::TS_GenerateTomeCollection, Config::cvStreamingPrefetchMaxUmbraWaitTime.Get() );

		GLoadingProfiler.FinishStage( TXT("PrefetchUmbraMid") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
#endif // USE_UMBRA

	////////////////////////////////////////

	// tick physics once to get the terrain wrappers created
	{
		PC_SCOPE( LoadTerrainCollision );

		CPhysicsWorld* physicalWorld = nullptr;
		m_activeWorld->GetPhysicsWorld( physicalWorld );
		physicalWorld->SetReferencePosition( m_prefetchPosition );

		const Float miniTick = 0.001f;

		physicalWorld->MarkSectorAsStuffAdded();
		// Wait for the terrain to appear
		while ( !physicalWorld->IsAllPendingCollisionLoadedAndCreated() )
		{
			// Finish current frame
			physicalWorld->FetchCurrentFrameSimulation( false );
			physicalWorld->CompleteCurrentFrameSimulation();

			// Schedule and execute ONE frame of physics
			physicalWorld->StartNextFrameSimulation( miniTick, 1.0f, true );
			physicalWorld->FetchCurrentFrameSimulation( false );
			physicalWorld->CompleteCurrentFrameSimulation();

			// Update master physics engine
			GPhysicEngine->Update( miniTick );
			PUMP_MESSAGES_DURANGO_CERTHACK();
		}
		GLoadingProfiler.FinishStage( TXT("PrefetchCollision") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

#ifdef USE_UMBRA
	// wait for Umbra to update the data
	// should happen before streaming entities, because we should have full info about TomeCollection before attaching
	// new render proxies to scene (indexing reasons etc.)
	{
		PC_SCOPE( PrefetchUmbraEnd );

		// Tick until umbra is done with all loading
		m_activeWorld->GetUmbraScene()->TickUntilStateOrIdle( m_activeWorld->GetRenderSceneEx(), CUmbraScene::TS_Idle, Config::cvStreamingPrefetchMaxUmbraWaitTime.Get() );

		GLoadingProfiler.FinishStage( TXT("PrefetchUmbraEnd") );
	}
#endif // USE_UMBRA

	////////////////////////////////////////

	// load initial entities and sectors
	{
		PC_SCOPE( LoadObjects );
		m_activeWorld->ForceStreamingForPoint( m_prefetchPosition );
		GLoadingProfiler.FinishStage( TXT("PrefetchObjects") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////
	// Get communities ready. Comes before updating lods and waiting for textures/meshes.
	{
		PC_SCOPE( FastForwardCommunities )
		PerformCommunitiesFastForward( m_prefetchPosition, Config::cvStreamingPrefetchMaxCommunitiesWaitTime.Get(), params.m_fastForwardCommunities );
		GLoadingProfiler.FinishStage( TXT("FastForwardCommunities") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	{
		// At this point, everything we don't need anymore is known. Get rid of it now!
		PC_SCOPE( PrefecthGC );
		GObjectGC->CollectNow();
		GLoadingProfiler.FinishStage( TXT("PrefetchGC") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
	
	////////////////////////////////////////

	m_activeWorld->FinishRenderingFades();
	PUMP_MESSAGES_DURANGO_CERTHACK();

	////////////////////////////////////////

	// Update LODs
	{
		// Delayed add/removes?
		CEntityTickManager& entTickMgr = m_activeWorld->GetTickManager()->GetEntityTickManager();
		entTickMgr.Tick( 0.001 );
		PUMP_MESSAGES_DURANGO_CERTHACK();

		m_activeWorld->GetComponentLODManager().ForceUpdateAll();
		m_activeWorld->GetEffectManager().ForceUpdateAll();
		m_activeWorld->GetManualStreamingHelper().ForceUpdateAll();
		PUMP_MESSAGES_DURANGO_CERTHACK();

		m_activeWorld->GetComponentLODManager().UpdateLODs();
		m_activeWorld->GetEffectManager().UpdateLODs();
		m_activeWorld->GetManualStreamingHelper().Tick();
		PUMP_MESSAGES_DURANGO_CERTHACK();

		SJobManager::GetInstance().FlushPendingJobs();

		// TBD: UpdXform or good enough?
		GLoadingProfiler.FinishStage( TXT("TickLODs"));
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	{
		PC_SCOPE( UpdateActors );
		StartGame_UpdateActors();
		GLoadingProfiler.FinishStage( TXT("UpdateActors"));
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Pathlib streaming
	if ( params.m_pathlibStreaming || HACK_isFastTravel )
	{
		CPathLibWorld* pathLib = m_activeWorld->GetPathLibWorld();
		pathLib->SetReferencePosition( m_prefetchPosition );

		pathLib->Tick(); // kick off streaming with the reference position

		CTimeCounter timer;
		do
		{
			SJobManager::GetInstance().FlushPendingJobs();
			pathLib->Tick();
			Red::Threads::SleepOnCurrentThread(1); // try to defer to lower priority PathLibThread

			if ( timer.GetTimePeriod() >= Config::cvStreamingPrefetchMaxPathLibWaitTime.Get() )
			{
				WARN_ENGINE(TXT("Timed out streaming pathlib. WaitTime=%.2f sec"), Config::cvStreamingPrefetchMaxPathLibWaitTime.Get());
				break;
			}
			PUMP_MESSAGES_DURANGO_CERTHACK();
		} while ( pathLib->HasJobOrTasks() );

		GLoadingProfiler.FinishStage( TXT("PrefetchPathLib"));

		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	if ( params.m_waitForPlayer )
	{
		PC_SCOPE( WaitForPlayer );

		StartGame_WaitForPlayer();

		GLoadingProfiler.FinishStage( TXT("WaitForPlayer") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Wait for meshes to stream in
	{
		PC_SCOPE( LoadMeshes );

		// Start asynchronous loading
		const EngineTime maximumTimeout = EngineTime::GetNow() + (Float)30.0f;
		while ( GRender->IsStreamingMeshes() )
		{
			if ( EngineTime::GetNow() > maximumTimeout )
				break;

			// wait a little bit
			//Red::Threads::SleepOnCurrentThread(50);

			// make sure the kick off jobs are triggered
			SDeferredDataBufferKickOffList::GetInstance().KickNewJobs();
			PUMP_MESSAGES_DURANGO_CERTHACK();
		}

		if ( GRender->IsStreamingMeshes() )
		{
			LOG_ENGINE( TXT("Meshes still streaming in. Probably timed out.") );
		}

		GLoadingProfiler.FinishStage( TXT("PrefetchMeshes") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Load foliage
	{
		PC_SCOPE( LoadFoliage );

		// Start asynchronous loading
		CFoliageScene* foliageScene = m_activeWorld->GetFoliageScene();
		foliageScene->PrefetchPositionSync( m_prefetchPosition );

		GLoadingProfiler.FinishStage( TXT("PrefetchFoliage") );
	}

	////////////////////////////////////////

	// Tick physics once again, to force our newly streamed objects on scene	
	{ 	
		PC_SCOPE( LoadFlushObjectCollision );

		CPhysicsWorld* physicalWorld = nullptr;
		m_activeWorld->GetPhysicsWorld( physicalWorld );
		physicalWorld->SetReferencePosition( m_prefetchPosition );

		const Float miniTick = 0.001f;

		physicalWorld->MarkSectorAsStuffAdded();
		// Wait for the terrain to appear
		while ( !physicalWorld->IsAllPendingCollisionLoadedAndCreated() )
		{
			// Schedule and execute ONE frame of physics
			physicalWorld->StartNextFrameSimulation( miniTick, 1.0f, true );
			physicalWorld->FetchCurrentFrameSimulation( false );
			physicalWorld->CompleteCurrentFrameSimulation();

			// Update master physics engine
			GPhysicEngine->Update( miniTick );
			PUMP_MESSAGES_DURANGO_CERTHACK();
		}
		GLoadingProfiler.FinishStage( TXT("PrefetchCollision") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// TBD update environmentmanager?

	// Tick effects before starting texture prefetch, to ensure stuff is up.
	{
		PC_SCOPE(TickEffects);
		if ( m_activeWorld->GetTickManager() )
		{
			m_activeWorld->GetTickManager()->TickAllEffects( 0.001 );
		}
		PUMP_MESSAGES_DURANGO_CERTHACK();

		SJobManager::GetInstance().FlushPendingJobs();
		GLoadingProfiler.FinishStage(TXT("PrefetchEffects"));

		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Render a single frame, to give a change for things like particles to get proper bounding boxes.
	// We don't present the frame, since we've got a loading screen or something showing.
	{
		PC_SCOPE( RenderOneFrame );

		// Set viewport hook, so we can set the proper starting camera.
		IViewportHook* oldHook = m_viewport->GetViewportHook();
		m_viewport->SetViewportHook( this );

		// Get streaming started. This will start loading textures for any proxies that have been created already.
		CRenderFrameInfo frameInfo( m_viewport.Get() );
		frameInfo.m_present = false;

		// Don't give a viewport, it's only there to generate debug fragments.
		CRenderFrame* frame = m_activeWorld->GenerateFrame( nullptr, frameInfo );

		// Restore old hook. We're going to set it again later, but we'll behave, in case something else expects to be there.
		m_viewport->SetViewportHook( oldHook );

		( new CRenderCommand_RenderScene( frame, m_activeWorld->GetRenderSceneEx() ) )->Commit();
		SAFE_RELEASE( frame );

		GLoadingProfiler.FinishStage( TXT("RenderOneFrame") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Start texture prefetch - must be after objects AND FOLIAGE are loaded, so we know what textures to load
	// We'll fire off one or more render frame prefetches, so that we can wait on them later.
	TDynArray< IRenderFramePrefetch* > renderFramePrefetches;
	{
		PC_SCOPE( PrefetchTexturesStart );

		RED_ASSERT( m_activeWorld->GetRenderSceneEx() != nullptr );

		// Set viewport hook, so we can set the proper starting camera.
		IViewportHook* oldHook = m_viewport->GetViewportHook();
		m_viewport->SetViewportHook( this );

		// Get streaming started. This will start loading textures for any proxies that have been created already.
		CRenderFrameInfo frameInfo( m_viewport.Get() );

		// Don't give a viewport, it's only there to generate debug fragments.
		CRenderFrame* frame = m_activeWorld->GenerateFrame( nullptr, frameInfo );

		// Restore old hook. We're going to set it again later, but we'll behave, in case something else expects to be there.
		m_viewport->SetViewportHook( oldHook );

		IRenderScene* scene = m_activeWorld->GetRenderSceneEx();

		// Check if this prefetch is coming from a new/load game or a fast travel ("gameplay"), or if it's from a camera cut
		// in a scene or from streaming locks or something (not "gameplay").
		const Bool isGameplay = ( params.m_isGameStart || HACK_isFastTravel );

		// Prepare for prefetch. Only reset distances if we're coming from a new game or fast travel. Otherwise, we probably aren't
		// moving much, and don't need to completely reset everything else.
		( new CRenderCommand_PrepareInitialTextureStreaming( scene, isGameplay ) )->Commit();


		// If we're teleporting to some new location, we'll do multiple prefetches in directions all around. This way if the player
		// spins the camera, the stuff behind us will be there. Don't need it if the prefetch is triggered for a scene, because
		// the camera will be generally fixed and cuts handled by the scene player.
		if ( isGameplay )
		{
			const CRenderCamera& origCamera = frame->GetFrameInfo().m_camera;

			// Limit far plane to max streaming distance. No point collected stuff farther than that.
			const Float farPlane	= Min( origCamera.GetFarPlane(), MSqrt( Config::cvTextureStreamingDistanceLimitSq.Get() ) );

			EulerAngles rotation	= origCamera.GetRotation();
			const Float vFov		= origCamera.GetFOV();

			const Float vSize = MTan( DEG2RAD( vFov * 0.5f ) );
			const Float hSize = vSize * origCamera.GetAspect();
			const Float hFov = RAD2DEG( MATan2( hSize, 1.0f ) * 2.0f );

			const Uint32 numPrefetches = (Uint32)( 360.0f / hFov );

			renderFramePrefetches.Reserve( numPrefetches );
			for ( Uint32 i = 0; i < numPrefetches; ++i )
			{
				CRenderFrameInfo frameInfo = frame->GetFrameInfo();
				frameInfo.m_camera.SetRotation( rotation );
				frameInfo.m_camera.SetFarPlane( farPlane );
				frameInfo.m_occlusionCamera = frameInfo.m_camera;
				CRenderFrame* newFrame = GRender->CreateFrame( nullptr, frameInfo );

				// Create prefetch, without occlusion.
				IRenderFramePrefetch* prefetch = GRender->CreateRenderFramePrefetch( newFrame, scene, false );
				( new CRenderCommand_StartFramePrefetch( prefetch ) )->Commit();

				renderFramePrefetches.PushBack( prefetch );

				newFrame->Release();

				rotation.Yaw += 360.0f / numPrefetches;
			}
		}
		else
		{
			// Use occlusion test here, so we just get stuff immediately visible
			IRenderFramePrefetch* prefetch = GRender->CreateRenderFramePrefetch( frame, scene, true );
			( new CRenderCommand_StartFramePrefetch( prefetch ) )->Commit();

			renderFramePrefetches.PushBack( prefetch );
		}

		SAFE_RELEASE( frame );

		GLoadingProfiler.FinishStage( TXT("PrefetchTexturesStart") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Starts GUI texture streaming. We'll wait for it later
	if ( params.m_forceUITextureStreaming || HACK_isFastTravel )
	{
		PC_SCOPE( UITextureStreaming );

		StartGame_InitHUD();
		( new CRenderCommand_ForceUITextureStreaming(true) )->Commit();

		GLoadingProfiler.FinishStage( TXT("UITextureStreamingStart") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Tick textures and flush renderer so we know things are started.
	{
		( new CRenderCommand_TickTextureStreaming() )->Commit();
		GRender->Flush();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// wait until envprobe prefetch ends
	{
		PC_SCOPE( LoadEnvProbes );

		CTimeCounter timer;
		while ( timer.GetTimePeriod() < Config::cvStreamingPrefetchMaxEnvProbesWaitTime.Get() )
		{
			if ( !GRender->IsDuringEnvProbesPrefetch() )
			{
				break;
			}
			Red::Threads::SleepOnCurrentThread( 5 );
			// make sure the kick off jobs are triggered
			SDeferredDataBufferKickOffList::GetInstance().KickNewJobs();
			PUMP_MESSAGES_DURANGO_CERTHACK();
		}
		GLoadingProfiler.FinishStage( TXT("PrefetchEnvProbes") );
	}

	////////////////////////////////////////

	{
		PC_SCOPE( UpdateTriggers );
		if ( m_activeWorld->GetTriggerManager() )
		{
			// activate any triggered CAreaEnvironmentComponent, which can cause seconds worth of loading
			// from things like filmic_tonemaping_balancemap_daylight.xbm
			m_activeWorld->GetTriggerManager()->Update();
		}
		GLoadingProfiler.FinishStage( TXT("PrefetchTriggers") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	// Wait for texture streaming to finish
	{
		PC_SCOPE( LoadTexturesEnd );

		( new CRenderCommand_TickTextureStreaming() )->Commit();
		GRender->Flush();
		PUMP_MESSAGES_DURANGO_CERTHACK();

		// Now wait for our prefetches to finish.
		CTimeCounter timer;
		CTimeCounter timeSinceLastForcedTick;
		for ( IRenderFramePrefetch* prefetch : renderFramePrefetches )
		{
			Bool didTimeout = false;

			while ( !prefetch->IsFinished() )
			{
				// If there's no loading screen, and we didn't enable the loading screen blur, then the prefetches will
				// not be processed by normal non-interactive rendering. So we need to manually tick the texture
				// streaming. To avoid spamming these, just fire one off occasionally.
				static Float tickDelay = 50.0f;
				if ( !IsLoadingScreenShown() && !usedBlur && timeSinceLastForcedTick.GetTimePeriodMS() > tickDelay )
				{
					( new CRenderCommand_TickTextureStreaming( true ) )->Commit();
					timeSinceLastForcedTick.ResetTimer();
				}


				// Flush, to give render thread a chance to do some work before hammering IsFinished.
				GRender->Flush();
				PUMP_MESSAGES_DURANGO_CERTHACK();


				if ( timer.GetTimePeriod() > Config::cvStreamingPrefetchMaxTexturesWaitTime.Get() )
				{
					LOG_ENGINE( TXT("Wait For Textures timed out after %.3fs"), timer.GetTimePeriod() );
					didTimeout = true;
					break;
				}
			}

			if ( didTimeout )
			{
				break;
			}
		}

		for ( IRenderFramePrefetch* prefetch : renderFramePrefetches )
		{
			prefetch->Release();
		}
		renderFramePrefetches.ClearFast();

		( new CRenderCommand_FinishInitialTextureStreaming() )->Commit();


		// Also make sure GUI textures are done streaming in.
		while ( GRender->IsStreamingGuiTextures() )
		{
			// Flush, to give render thread a chance to do some work before hammering IsFinished.
			GRender->Flush();
			PUMP_MESSAGES_DURANGO_CERTHACK();

			if ( timer.GetTimePeriod() > Config::cvStreamingPrefetchMaxTexturesWaitTime.Get() )
			{
				LOG_ENGINE( TXT("Wait For Textures timed out after %.3fs"), timer.GetTimePeriod() );
				break;
			}
		}


		GLoadingProfiler.FinishStage( TXT("PrefetchTexturesEnd") );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	////////////////////////////////////////

	m_activeWorld->FinishRenderingFades();

	////////////////////////////////////////

	LOG_ENGINE( TXT("Streaming prefetching took %1.2fs"), timer.GetTimePeriod() );

	////////////////////////////////////////

	if ( showLoadingScreen )
	{
		if ( usedBlur )
		{
			// no fadeout time
			(new CRenderCommand_HideLoadingScreenBlur(0.0f))->Commit();
		}
		else
		{
			HideLoadingScreen();
		}
	}


	if( HACK_isFastTravel )
	{
		CEntity* entity = GetPlayerEntity();
		if( CPeristentEntity* persEnt = Cast<CPeristentEntity>( entity ) )
		{
			PUMP_MESSAGES_DURANGO_CERTHACK();
			persEnt->ResetClothAndDangleSimulation();
			PUMP_MESSAGES_DURANGO_CERTHACK();
		}
	}

	// Unlock UI texture streaming
	if ( params.m_forceUITextureStreaming || HACK_isFastTravel )
	{
		( new CRenderCommand_ForceUITextureStreaming(false) )->Commit();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
}

const Bool CGame::IsStreamingLocked() const
{
	return !m_streamingLocks.Empty();
}

const Bool CGame::UseCameraAsStreamingReference() const
{
	return !m_streamingCameraOverrides.Empty();
}

void CGame::RetireOldStreamingLocks()
{
	Bool stuffRemoved = false;

	// remove old locks
	for ( Int32 i=m_streamingLocks.SizeInt()-1; i >= 0; --i )
	{
		if ( m_streamingLocksFrameCounter >= m_streamingLocks[i].m_toRemoveOnFrame )
		{
			LOG_ENGINE( TXT("Streaming lock '%ls' was retired"), m_streamingLocks[i].m_areaTag.AsChar() );

			m_streamingLocks.RemoveAtFast( i );
			stuffRemoved = true;
		}
	}

	// request update of the streaming 
	if ( stuffRemoved )
	{
		m_streamingLocksModified = true;
	}
}

void CGame::UpdateStreamingModeAndDistances()
{
	// reset to default case from config
	m_prefetchDistanceSoft = Config::cvStreamingPrefetchSoftDistance.Get();
	m_prefetchDistanceHard = Config::cvStreamingPrefetchHardDistance.Get();

	// use the values from the overrides
	for ( const auto& data : m_streamingCameraOverrides )
	{
		m_prefetchDistanceSoft = Max< Float >( m_prefetchDistanceSoft, data.m_softDistance );
		m_prefetchDistanceHard = Max< Float >( m_prefetchDistanceSoft, data.m_hardDistance );
	}

	// make sure soft range is bigger
	m_prefetchDistanceSoft = Max< Float >( m_prefetchDistanceHard + 1.0f, m_prefetchDistanceSoft );	
}

void CGame::UpdateStreamingLocks()
{
	PC_SCOPE( UpdateStreamingLocks );
	
	// increase frame counted (time advancemenet)
	m_streamingLocksFrameCounter += 1;

	// get rid of the old locks first
	RetireOldStreamingLocks();

	// rebuild areas
	if ( m_streamingLocksModified )
	{
		m_streamingLocksModified = false;

		// extract the bounding boxes from the areas
		TStaticArray< Box, 32 > streamingBoxes;
		for ( const auto& data : m_streamingLocks )
		{
			// do we have the entities ?
			for ( const auto& box : data.m_areas )
			{
				if ( streamingBoxes.Full() )
				{
					ERR_ENGINE( TXT("To many active streaming locks, failed to collect lock for '%ls', it will not be used"), 
						data.m_name.AsChar() );
				}
				else
				{
					streamingBoxes.PushBack( box );
				}
			}
		}

		LOG_ENGINE( TXT("Collected %d streaming lock areas from %d locks"), 
			streamingBoxes.Size(), m_streamingLocks.Size() );

		// lock streaming on world in given areas
		m_activeWorld->SetStreamingLockedArea( streamingBoxes.TypedData(), streamingBoxes.Size() ); 

		// if we have some locked areas make sure the content is there by forcing a prefetch on next frame
		if ( !streamingBoxes.Empty() )
		{
			m_requestStreamingPrefetch = true;
		}
	}
}

void CGame::PerformCommunitiesFastForward( const Vector& referencePosition, Float timeLimit, Bool resimulateCommunities )
{

}

void CGame::UpdatePrefetch( const Vector& referencePosition, Bool& allowCurrentFrameToRender )
{
	PC_SCOPE( UpdateStreaming );

	// in general we can render
	allowCurrentFrameToRender = true;

	// do not stream when disabled
	if ( !m_enableStreamingPrefetch )
		return;

	// Update streaming locks
	UpdateStreamingLocks();

	// calculate distance to the reference position from last frame
	const Float dist = m_prefetchPosition.DistanceTo( referencePosition );
	m_prefetchPosition = referencePosition;

	// update the reference position for streaming
	SetStreamingReferencePosition( referencePosition );

	// perform prefetch if moved more than the specified distance in one frame
	// do not perform distance based prefetch if we have streaming locks applied
	// NOTE: the first prefetch AFTER applying the locks still happen
	const Bool hasStreamingLocksActive = !m_streamingLocks.Empty();
	if ( m_requestStreamingPrefetch || ( dist > m_prefetchDistanceHard && !hasStreamingLocksActive ) )
	{
		// prefetch data for new position
		SPrefetchParamsW3Hack params;
		params.m_fastForwardCommunities = true;
		// Only update terrain if we've moved sufficiently far
		params.m_didMoveFar = ( dist > m_prefetchDistanceHard );
		PerformPrefetch( true, params );

		// do not render this frame - it contains unfinished scene any way
		allowCurrentFrameToRender = false;
		m_requestStreamingPrefetch = false;
	}
}

void CGame::SetStreamingReferencePosition( const Vector& referencePosition )
{
	m_activeWorld->SetStreamingReferencePosition( referencePosition );
}


//---

