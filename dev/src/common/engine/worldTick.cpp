/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "world.h"
#include "worldTick.h"
#include "pathlibWorld.h"
#include "globalWater.h"
#include "dynamicCollisionCollector.h"
#include "cameraDirector.h"
#include "entityMotion.h"
#include "renderCommands.h"
#include "foliageScene.h"
#include "clipMap.h"
#include "../core/scriptingSystem.h"
#include "viewport.h"
#include "viewportHook.h"
#include "../physics/physicsWorld.h"
#include "animationManager.h"
#include "debugConsole.h"
#include "environmentManager.h"
#include "weatherManager.h"
#include "scaleable.h"
#include "tickManager.h"
#include "tagManager.h"
#include "streamingSectorData.h"
#include "sectorDataStreaming.h"
#include "areaComponent.h"
#include "umbraScene.h"
#include "baseEngine.h"
#include "extAnimEvent.h"
#include "gameTimeManager.h"
#include "../physics/physicsWrapper.h"
#include "physicsTileWrapper.h"
#include "collisionCache.h"

// required if we want to recreate all render proxies for Ansel
//#include "layerInfo.h"
//#include "layerGroup.h"
//#include "meshTypeComponent.h"

#define USE_UPDATE_TRANSFORM_MULTITHREADED
Bool	GGlobalScriptFunctionToExecuteOnGameStated( false );
String	GGlobalScriptFunctionToExecuteOnGameStatedWithArguments( String::EMPTY );

Uint32 pr_time = 0;
Uint32 pr_time2 = 0;

Bool GIsUpdatingTransforms = false;

#ifdef USE_ANSEL
extern Bool isAnselCaptureActive;
extern Bool isAnselSessionActive;
extern Bool isAnselTurningOn;
extern Bool isAnselTurningOff;
extern Matrix anselCameraTransform;
#endif // USE_ANSEL

#ifdef USE_CAM_ASAP
namespace
{
	void UpdateCameraASAP( CCameraDirector* camDir, Float timeDelta, Bool& updateCameraAsap, SCameraVisibilityData& cameraVisibilityData )
	{
		// A lot of fatal asserts because we do a lot of assumptions how we update this game/gameplay

		PC_SCOPE_PIX( CameraAsap );

		CEntity* player = GGame->GetPlayerEntity();
		CEntity* playersParent( nullptr );
		CEntity* gameCamera = GGame->GetGameplayCameraEntity();
		CEntity* camera = Cast< CEntity >( camDir->GetTopmostCameraObject().Get() );

		//RED_FATAL_ASSERT( player, "Update camera ASAP" );
		//RED_FATAL_ASSERT( gameCamera, "Update camera ASAP" );
		//RED_FATAL_ASSERT( camera, "Update camera ASAP" );
		if ( !player || !gameCamera || !camera )
		{
			return;
		}

		updateCameraAsap = true;

		// Check if we can do it
		if ( CHardAttachment* ha = player->GetTransformParent() )
		{
			CNode* playersParentNode = ha->GetParent();
			if ( playersParentNode->GetTransformParent() )
			{
				RED_FATAL_ASSERT( !playersParentNode->GetTransformParent(), "Update camera ASAP" );
				updateCameraAsap = false;
			}
			playersParent = Cast< CEntity >( playersParentNode );
			if ( !playersParent )
			{
				RED_FATAL_ASSERT( playersParent, "Update camera ASAP" );
				updateCameraAsap = false;
			}
		}

		RED_FATAL_ASSERT( updateCameraAsap, "Update camera ASAP" );

		if ( updateCameraAsap )
		{
			if ( playersParent )
			{
				PC_SCOPE_PIX( CameraAsap_playersParent );

				playersParent->HACK_UpdateLocalToWorld();
				player->HACK_UpdateLocalToWorld();
			}
			else
			{
				PC_SCOPE_PIX( CameraAsap_player );

				player->HACK_UpdateLocalToWorld();
			}

			{
				PC_SCOPE_PIX( CameraAsap_cameraDirector );

				camDir->Update( timeDelta );
			}

			if ( camera == gameCamera )
			{
				RED_ASSERT( camera->HasScheduledUpdateTransform() );
			}
			//if ( camera->HasScheduledUpdateTransform() )
			{
				PC_SCOPE_PIX( CameraAsap_camera );

				camera->ForceUpdateTransformNodeAndCommitChanges();
			}

			//RED_FATAL_ASSERT( !m_updateTransformManager.HasEntityScheduled( camera ), "Update camera ASAP" );

			camDir->CacheCameraData();

			camDir->CalcVisibilityDataAndCacheRenderCamera( cameraVisibilityData );
		}
	}
}
#endif

void CWorld::Tick( const CWorldTickInfo& tickInfo, const CRenderFrameInfo* previousFrameInfo, CRenderFrame** frame, Bool updateCurrentCamera )
{
	WorldUpdateStateContext context( this, tickInfo.m_phase );

#ifdef USE_ANSEL
	const Bool isAnselTurnedOn = isAnselSessionActive || isAnselTurningOn || isAnselTurningOff;
#endif // USE_ANSEL

	PC_SCOPE_LV0( WorldTick, PBC_CPU );

	// Advance timers
	{
		PC_SCOPE_PIX( TickManagerAdvanceTime );
		m_tickManager->BeginFrame();
		m_tickManager->AdvanceTime( tickInfo.m_timeDelta );

		CExtAnimEvent::StatsCollector::GetInstance().Reset();
	}

	// Update layers state
	if ( m_worldLayers )
	{
		PC_SCOPE_PIX( UpdateLoadingState );
		UpdateLoadingState();
	}

#ifdef USE_ANSEL
	{
		/*
		// recreate drawables with autohide distance turned off -> kind of useless, because the streaming will destroy them over distance
		if ( isAnselTurningOn || isAnselTurningOff )
		{
			Bool forceNoAutohide = false;
			if ( isAnselTurningOn )
			{
				forceNoAutohide = true;
			}
			else
			{
				RED_FATAL_ASSERT( isAnselTurningOff, "" );
			}

			TDynArray< CLayerInfo* > layerInfos;
			m_worldLayers->GetLayers( layerInfos, true );
			for ( auto layerInfo : layerInfos )
			{
				if ( CLayer* layer = layerInfo->GetLayer() )
				{
					for ( auto entity : layer->GetEntities() )
					{
						for ( auto component : entity->GetComponents() )
						{
							if ( CDrawableComponent* drawable = SafeCast< CDrawableComponent >( component ) )
							{
								drawable->SetForceNoAutohide( forceNoAutohide );
							}
						}
					}
				}
			}
		}
		//*/

		m_tickManager->TickNPCLODs();
	}
#endif // USE_ANSEL

#ifdef USE_ANSEL
	if ( !isAnselCaptureActive )
#endif // USE_ANSEL
	{
		// Update tick LODs for components and effects
		{
			PC_SCOPE_PIX( TickLODUpdate );	
			m_componentLODManager.UpdateLODs();
			m_effectTickLODManager.UpdateLODs();
		}

		// Update LODs manually streamed objects
		{
			PC_SCOPE_PIX( TickStreamingHelper );	
			m_manualStreamingHelper.Tick();
		}
	}

	// Update transform on dirty nodes
#ifdef USE_ANSEL
	if ( !isAnselTurnedOn )
#endif // USE_ANSEL
	{
		PC_SCOPE_PIX( UpdateTransforms_Pre );

		UpdateTransformContext context;
		context.m_canUseThreads = tickInfo.m_fromTickThread;
		context.m_relatedTickGroup = TICK_PrePhysics;
		m_updateTransformManager.UpdateTransforms_StartAndWait( context );
	}

	// Update the area shapes
	// TODO: may be we need some kind of generalized system for dirty objects ?
	// RebuildDirtyAreas it's inside
	CAreaComponent::ProcessPendingAreaUpdates();

	// Update threads (only in main world)
	if( tickInfo.m_updateScripts && this == GGame->GetActiveWorld() )
	{
		PC_CHECKER_SCOPE( 0.010f, TXT("AI"), TXT("Scripts are slow!!!, %i threads"), GScriptingSystem->GetThreads().Size() );
		PC_SCOPE_PIX( AdvanceThreads );
		GScriptingSystem->AdvanceThreads( tickInfo.m_timeDelta );
	}

	if( this == GGame->GetActiveWorld() )
	{
		PC_SCOPE_PIX( CollisionCacheTick );
		GCollisionCache->Tick();
	}

	// Pre physics tick
	{
		PC_SCOPE_PIX( PrePxFrameComponentsTick1 );
		STickManagerContext context( m_tickManagerNarrowScopeTaskBatch, TICK_PrePhysics, tickInfo.m_timeDelta, false );
		m_tickManager->Tick( context );
	}

	// Pre physics tick post
	static Bool USE_ANIM_OPT = false;
	STickManagerContext context_PrePhysicsPost( m_tickManagerBroadScopeTaskBatch, TICK_PrePhysicsPost, tickInfo.m_timeDelta, USE_ANIM_OPT );
	{
		if ( m_physicsWorld && tickInfo.m_updatePhysics )
		{
			m_physicsWorld->SetWhileSceneProcessFlag( true );
			m_physicsWorld->FetchCurrentFrameSimulation( true );
		}

		{
			PC_SCOPE_PIX( PrePxFrameComponentsTick2 );
			{
				PC_SCOPE_PIX( TickImmediateJobs );
				m_tickManager->TickImmediateJobs( context_PrePhysicsPost, m_taskBatch );
			}

			if ( context_PrePhysicsPost.m_group != TICK_PrePhysics )
			{
				PC_SCOPE_PIX( ImmediateJobsIssue );
				m_taskBatch.IssueWithoutInlining( TSP_VeryHigh );
			}

			{
				PC_SCOPE_PIX( MainThreadWhileAnimationJobs );

				// pathlib update
				if( tickInfo.m_updatePathLib )
				{
					if ( m_pathLib )
					{
						PC_SCOPE_PIX( PathLibTick );
						m_pathLib->Tick();
					}
				}

#ifdef USE_UMBRA
				// update occlusion system
				if ( m_umbraScene )
				{
					PC_SCOPE_PIX( UmbraSceneTick );
					m_umbraScene->Tick( GetRenderSceneEx() );
				}
#endif // USE_UMBRA
			}

			if ( context_PrePhysicsPost.m_group != TICK_PrePhysics )
			{
				PC_SCOPE_PIX( ImmediateJobsWait );
				m_taskBatch.FinishTasksAndWait();		
				context_PrePhysicsPost.Issue( TSP_VeryHigh );
			}
			{
				PC_SCOPE_PIX( TickSingleThreaded );
				m_tickManager->TickSingleThreaded( context_PrePhysicsPost, m_taskBatch );
			}
		}
	}

	// If there is a physical world and its not stealed from anyone
	if ( m_physicsWorld )
	{
		if ( tickInfo.m_updatePhysics )
		{
			{
				PC_SCOPE_PIX( PX_CompleteCurrentFrameSimulation );
				m_physicsWorld->CompleteCurrentFrameSimulation();
				m_physicsWorld->SetWhileSceneProcessFlag( false );
				m_physicsWorld->SendReceivedScriptEvents();
			}
			
			{
				// PC_SCOPE_PHYSICS( FinalizeMovement ) it's inside
				FinalizeMovement( tickInfo.m_timeDelta );
			}
		}

		{
			PC_SCOPE_PIX( PrePhysicsTickPost_Wait );
			context_PrePhysicsPost.Wait();
		}

		if( tickInfo.m_updatePhysics )
		{
			if ( previousFrameInfo )
			{
				m_physicsWorld->SetRenderingView( previousFrameInfo->m_camera.GetViewToWorld() );
			}

			if ( m_physicsWorld )
			{
				m_physicsWorld->SetWhileSceneProcessFlag( true );
			}

			CPhysicsTileWrapper::PrepereStaticBodyCreationQue( m_physicsWorld );

			{
				PC_SCOPE_PIX( PhysicsStartNextFrameSim_Main )
				m_physicsWorld->StartNextFrameSimulation( tickInfo.m_timeDelta, GGame->GetTimeScale(), GGame->IsBlackscreen() );
			}
			{
				PC_SCOPE( PhysicsSetWhileSceneProcessFlag_Main )
				m_physicsWorld->SetWhileSceneProcessFlag( false );
			}

		}

		if( m_physicsBatchQueryManager )
		{
			PC_SCOPE_PIX( PhysicsBatchQueryManagerTick );
			m_physicsBatchQueryManager->Tick( tickInfo.m_timeDelta );
		}
	}


	// Main tick (physics simulation is done in background)
	{
		PC_SCOPE_PIX( MainTick );
		STickManagerContext context( m_tickManagerNarrowScopeTaskBatch, TICK_Main, tickInfo.m_timeDelta, false );
		m_tickManager->Tick( context );
	}

	SCameraVisibilityData cameraVisibilityData;
	Bool updateCameraAsap( false );
#ifdef USE_CAM_ASAP
	if ( GGame->IsActive() && GGame->GetActiveWorld() == this )
	{
#ifdef USE_ANSEL
		if ( !isAnselTurnedOn )
#endif // USE_ANSEL
		{
			UpdateCameraASAP( m_cameraDirector, tickInfo.m_timeDelta, updateCameraAsap, cameraVisibilityData );
		}
	}
#endif

	// Property animations
	{
		PC_SCOPE_PIX( PropertyAnimationsTick );
		m_tickManager->TickPropertyAnimations( tickInfo.m_timeDelta );
	}

	// Post physics tick
	{
		PC_SCOPE_PIX( PostPxFrameComponentsTick1 );
		STickManagerContext context( m_tickManagerNarrowScopeTaskBatch, TICK_PostPhysics, tickInfo.m_timeDelta, false );
		m_tickManager->Tick( context );
	}

	// Finalize movement is moved to pre simulation

	// Post physics post tick
	{
		PC_SCOPE_PIX( PostPxFrameComponentsTick2 );
		STickManagerContext context( m_tickManagerNarrowScopeTaskBatch, TICK_PostPhysicsPost, tickInfo.m_timeDelta, false );
		m_tickManager->Tick( context );
	}

	// Update transform on dirty nodes
	Vector envManagerMainTickPosition = Vector::ZERO_3D_POINT;
	{
		UpdateTransformContext context;
#ifdef USE_UPDATE_TRANSFORM_MULTITHREADED
		context.m_canUseThreads = tickInfo.m_fromTickThread;
#else
		context.m_canUseThreads = false;
#endif
		context.m_relatedTickGroup = TICK_PostPhysicsPost;
		context.m_cameraVisibilityData = cameraVisibilityData;

		static Bool USE_OPT_UT = true;
		const Bool useUpdateTransformParallelUpdate = USE_OPT_UT && tickInfo.m_fromTickThread;

#ifdef USE_ANSEL
		if ( !isAnselTurnedOn )
#endif // USE_ANSEL
		{
			if ( useUpdateTransformParallelUpdate )
			{
				PC_SCOPE_PIX( UpdateTransforms_Post_Start );

				m_updateTransformManager.UpdateTransforms_Start( context );
			}
			else
			{
				PC_SCOPE_PIX( UpdateTransforms_Post );

				m_updateTransformManager.UpdateTransforms_StartAndWait( context );
			}
		}		

		if ( GGame )
		{
			PC_SCOPE_PIX( GGame_OnTickDuringPostUpdateTransforms );

			// Don't update GUI if preview world or else it'll recursively tick the gui, flash, the menu, and the world.
			// ( could have a recursion check in each, but that seems easier to break. )
			GGame->OnTickDuringPostUpdateTransforms( tickInfo.m_timeDelta, ! GetPreviewWorldFlag() );
		}

		// Terrain update
		if ( m_terrainClipMap )
		{
			PC_SCOPE_PIX( TerrainClipMapUpdate );

			// Use GEngine's time instead of tickInfo.m_timeDelta. The time is used for tracking data lifetimes, which
			// shouldn't be affected by possible time scaling.
			SClipMapUpdateInfo updateInfo;
			updateInfo.m_viewerPosition = 
#ifdef USE_ANSEL
				isAnselSessionActive ? anselCameraTransform.GetTranslation() : 
#endif // USE_ANSEL
				m_cameraPosition;
			updateInfo.m_timeDelta = GEngine->GetLastTimeDelta();
			m_terrainClipMap->Update( updateInfo );
		}

		// Foliage streaming update
		if ( m_foliageScene )
		{
			PC_SCOPE_PIX( FoliageManagerTick );
			m_foliageScene->Tick();
		}

		// Tick environment manager
		if ( m_environmentManager )
		{
			PC_SCOPE_PIX( EnvironmentTick_Parallel );
			envManagerMainTickPosition = m_environmentManager->GetCurrentAreaEnvUpdateReferencePos();
			m_environmentManager->Tick_Parallel( tickInfo.m_timeDelta );
		}

#ifdef USE_ANSEL
		if ( !isAnselTurnedOn )
#endif // USE_ANSEL
		{
			if ( useUpdateTransformParallelUpdate )
			{
				PC_SCOPE_PIX( UpdateTransforms_Post_End );

				m_updateTransformManager.UpdateTransforms_WaitForFinish( context );
			}
		}

		// Tick environment manager
		if ( m_environmentManager )
		{
			PC_SCOPE_PIX( EnvironmentTick_Single );
			m_environmentManager->Tick_Single( tickInfo.m_timeDelta );
		}

		// Tick character clothing
		if ( m_physicsWorldSecondary && tickInfo.m_updatePhysics )
		{
			if ( previousFrameInfo )
			{
				m_physicsWorldSecondary->SetRenderingView( previousFrameInfo->m_camera.GetViewToWorld() );
			}

			PC_SCOPE_PIX( PhysicsStartNextFrameSim_Secondary );
			m_physicsWorldSecondary->StartNextFrameSimulation( tickInfo.m_timeDelta, GGame->GetTimeScale(), GGame->IsBlackscreen() );
		}
	}

	// Tick effects
	if( tickInfo.m_updateEffects )
	{
		PC_SCOPE_PIX( TickEffects );

#ifdef USE_ANSEL
		if ( isAnselTurnedOn )
		{
			static Uint32 frameCounter = 0;
			if ( ++frameCounter > 15 )
			{
				m_tickManager->TickEffects( 0.01f );
				frameCounter = 0;
			}
		}
		else
#endif
		{
			m_tickManager->TickEffects( tickInfo.m_timeDelta );
		}
	}

	// Finish updating gameplay storage
	{
		GGame->FinalizeFlushGameplayStorageUpdates();
	}

	// Post update transform tick
	{
		PC_SCOPE_PIX( TickPostUpdateTransform );
		STickManagerContext context( m_tickManagerNarrowScopeTaskBatch, TICK_PostUpdateTransform, tickInfo.m_timeDelta, false );
		m_tickManager->Tick( context );
	}

	if ( m_cameraDirector && !updateCameraAsap )
	{
		PC_SCOPE_PIX( CameraDirectorUpdate );
		m_cameraDirector->Update( tickInfo.m_timeDelta );
	}

	if ( m_entityMotionManager )
	{
		PC_SCOPE_PIX( EntityMotionManager );
		m_entityMotionManager->Tick( tickInfo.m_timeDelta );
	}

#ifdef USE_ANSEL
	if ( !isAnselTurnedOn )
#endif // USE_ANSEL
	{
		PC_SCOPE_PIX( UpdateTransforms_PostUpdate );

		UpdateTransformContext context;
		context.m_canUseThreads = tickInfo.m_fromTickThread;
		context.m_relatedTickGroup = TICK_PostUpdateTransform;
		context.m_cameraVisibilityData = cameraVisibilityData;
		m_updateTransformManager.UpdateTransforms_StartAndWait( context );
	}

	if ( m_cameraDirector && !updateCameraAsap )
	{
		m_cameraDirector->CacheCameraData();
	}

	// Entity streaming update start, we should do it as soon as we have valid camera position
#ifndef NO_EDITOR
	if ( m_enableStreaming )
#endif
	{
	}

	// Update current camera
	if ( updateCurrentCamera )
	{
		PC_SCOPE_PIX( ProcessCameraMovement );

		Vector camPos;
		Vector camForward;
		Vector camUp;

#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			camPos		= anselCameraTransform.GetTranslation();
			camForward	= anselCameraTransform.GetAxisY();
			camUp		= anselCameraTransform.GetAxisZ();
		}
		else
#endif // USE_ANSEL
		{
			camPos		= GetCameraDirector()->GetCameraPosition();
			camForward	= GetCameraDirector()->GetCameraForward();
			camUp		= GetCameraDirector()->GetCameraUp();
		}

		{
			PC_SCOPE_PIX( WorldUpdateCameraPosition );
			UpdateCameraPosition( camPos );
			UpdateCameraForward( camForward );
			UpdateCameraUp( camUp );
		}
	}

	// Update trigger system
	if ( tickInfo.m_updateTriggers && NULL != m_triggerManager )
	{
		// PC_SCOPE(TriggerManagerUpdate) it's inside
		m_triggerManager->Update();
	}

	// Update environment manager post trigger system and camera director stuff
	if ( m_environmentManager )
	{
		const Vector currentEnvMgrTickPosition = m_environmentManager->GetCurrentAreaEnvUpdateReferencePos();
		
		// Teleport fixup
		// It needs to be done after trigger system update, so that we would have up to date area environments set.
		// Main Tick is done in parallell with update transforms so we want to keep it there and we're doing a 'fixup'
		// only when camera position changes significantly (it's not perfect but for this moment seems like the best solution).
		{
			PC_SCOPE_PIX( EnvironmentTick_TeleportFixup );

			Float cameraDeltaSq = 0;			
			{
				cameraDeltaSq = Max( cameraDeltaSq, envManagerMainTickPosition.DistanceSquaredTo( currentEnvMgrTickPosition ) );
				cameraDeltaSq = Max( cameraDeltaSq, envManagerMainTickPosition.DistanceSquaredTo( m_lastEnvironmentManagerUpdatePos ) );
				cameraDeltaSq = Max( cameraDeltaSq, m_lastEnvironmentManagerUpdatePos.DistanceSquaredTo( currentEnvMgrTickPosition ) );
			}

			const Float teleportDistThreshold = 5.f;
			if ( cameraDeltaSq > teleportDistThreshold * teleportDistThreshold )
			{
				m_environmentManager->Tick_TeleportFixup();
			}
		}

		// Update camera params
		m_environmentManager->UpdateCameraParams();

		// Save last update position
		m_lastEnvironmentManagerUpdatePos = currentEnvMgrTickPosition;
	}

	if ( m_tagManager != nullptr )
	{
		PC_SCOPE_PIX( TagManagerUpdate );
		m_tagManager->Update();
	}

	if( tickInfo.m_updateGame && GGame->ShouldChangePlayer() )
	{
		GGame->ChangePlayer();
	}

	// Animation reporter
	{
		PC_SCOPE_PIX( AnimationManager_Pre );
		GAnimationManager->BeginUpdate( tickInfo.m_timeDelta );
	}

	// Delayed stuff
	{
		PC_SCOPE_PIX( DelayedStuff );
		DelayedActions();
	}

	// Entity streaming update end
#ifndef NO_EDITOR
	if ( m_enableStreaming )
#endif
	{
		PC_SCOPE_PIX( EntityStreaming );

		if ( m_sectorStreaming )
		{
			PC_SCOPE_LV0( SectorStreaming, PBC_CPU );
			m_sectorStreaming->FinishStreaming(); // finish streaming from previous frame
			m_sectorStreaming->BeginStreaming(); // start streaming for current frame
		}

		// OLD STREAMING CODE:
		{
			PC_SCOPE_LV0( StreamingSectorData, PBC_CPU );
#ifndef NO_EDITOR
			if ( GGame->IsContignousModeEnabled() )
			{
				// if we are recording, then force streaming to complete
				m_streamingSectorData->UpdateStreaming( /*force=*/ true, /*flush=*/ true );
			}
			else
#endif
			{
				m_streamingSectorData->UpdateStreaming();
			}
		}

	}

	CPhysicsTileWrapper::FlushStaticBodyCreationQue();
		
	// Global water update
	if( tickInfo.m_updateWater )
	{
		if ( m_globalWater )
		{
			PC_SCOPE_PIX( GlobalWaterUpdate );
			m_globalWater->Update( tickInfo.m_timeDelta  );
		}
	}

	// Update dynamic collisions - spt ATM
	if( m_dynamicCollisions )
	{
		// PC_SCOPE(DynamicCollisionCollector_Tick); it's inside
		m_dynamicCollisions->Tick( this, tickInfo.m_timeDelta );
	}

	// Tick debug console (use raw engine time)
	if ( GDebugConsole )
	{
		PC_SCOPE_PIX( DebugConsoleUpdate );
		GDebugConsole->Tick( GEngine->GetLastTimeDelta() );
		if ( GGlobalScriptFunctionToExecuteOnGameStated && !GGlobalScriptFunctionToExecuteOnGameStatedWithArguments.Empty() 
			&& GScriptingSystem && GScriptingSystem->IsValid() && GGame->IsActive() && GGame->GetPlayerEntity() )
		{
			GGlobalScriptFunctionToExecuteOnGameStated = false;
			GScriptingSystem->CallGlobalExecFunction( GGlobalScriptFunctionToExecuteOnGameStatedWithArguments, true );
			GGlobalScriptFunctionToExecuteOnGameStatedWithArguments.Clear();
		}
	}

	if ( m_physicsWorldSecondary && tickInfo.m_updatePhysics )
	{
		m_physicsWorldSecondary->FetchCurrentFrameSimulation( false );
		m_physicsWorldSecondary->CompleteCurrentFrameSimulation();
	}

}

CRenderFrame* CWorld::GenerateFrame( IViewport *viewport, CRenderFrameInfo& info )
{
	WorldUpdateStateContext context( this, WUP_GenerateFrame );
	PC_SCOPE_PIX( GenerateFrame );

	// Pop environment triggers
	Bool performInstantAdaptation = !info.m_camera.GetLastFrameData().m_isValid;
	Bool performInstantDissolve = !info.m_camera.GetLastFrameData().m_isValid;
	if ( GetEnvironmentManager() )
	{
		performInstantAdaptation |= GetEnvironmentManager()->GetInstantAdaptationTrigger();
		performInstantDissolve |= GetEnvironmentManager()->GetInstantDissolveTrigger();

#ifdef USE_ANSEL
		const Bool instantAdaptationTrigger = isAnselSessionActive;
		const Bool instantDissolveTrigger = isAnselSessionActive;
#else
		const Bool instantAdaptationTrigger = false;
		const Bool instantDissolveTrigger = false;
#endif // USE_ANSEL
		
		GetEnvironmentManager()->SetInstantAdaptationTrigger( instantAdaptationTrigger );
		GetEnvironmentManager()->SetInstantDissolveTrigger( instantDissolveTrigger );
	}

	// Update camera parameters to take into account changes
	{
		PC_SCOPE_PIX( RefreshCameraParams );
		info.RefreshCameraParams();
	}

	// Atlas render params update
	if ( m_atlasRegionsRegenerateRequested )
	{
		//m_terrainManager->RegenerateAtlasRegions( m_atlasRegionsRegenerateForceNext );
		m_atlasRegionsRegenerateRequested = false;
		m_atlasRegionsRegenerateForceNext = false;
	}

	// Copy environment settings to frame info
	const Uint32 gameDays = GGame ? GGame->GetTimeManager()->GetTime().Days() : 0;
	if ( m_environmentManager )
	{
		PC_SCOPE_PIX( UpdateEnvironment );
											 
		// Set params
		Bool instantAdaptation = performInstantAdaptation || m_environmentManager->GetGameEnvironmentParams().m_displaySettings.m_enableInstantAdaptation;
		info.SetAreaEnvParams( CAreaEnvironmentParamsAtPoint( m_environmentManager->GetCurrentAreaEnvironmentParams() ) );

		// Get blended env params
		CAreaEnvironmentParams* firstMostImportantParams = NULL;
		CAreaEnvironmentParams* secondMostImportantParams = NULL;
		Float mostImportantBlendingFactor = 0.0f;
		m_environmentManager->GetMostImportantEnvironments( &firstMostImportantParams, &secondMostImportantParams, mostImportantBlendingFactor );
		if ( firstMostImportantParams && secondMostImportantParams && mostImportantBlendingFactor > 0.f )
		{
			info.m_envParametersAreaBlend1 = *firstMostImportantParams;
			info.m_envParametersAreaBlend2 = *secondMostImportantParams;
			info.m_envBlendingFactor = mostImportantBlendingFactor;
		}
		else
		{
			info.m_envBlendingFactor = 0.0f;
		}

		//// GI_213_DEMO_HACK: don't ask ;)
		//const Float bloomIntensity = GGame->GetControlledBloomIntensity();
		//info.m_envParametersArea.m_bloomAnamorphic.m_colorFlare1.SetValue( Color( Vector( bloomIntensity, 0.f, 0.f ) ), 1.f );  

		// Set world render settings
		{
			info.SetWorldRenderSettings( m_environmentParameters.m_renderSettings );

			// Override some render settings
			{
				Float& distanceStart	= info.m_worldRenderSettings.m_distantLightStartDistance;
				Float& distanceFade		= info.m_worldRenderSettings.m_distantLightFadeDistance;

				// well, last minute hack due the performance reasons
#ifdef RED_PLATFORM_CONSOLE
				distanceStart	= Max( 20.0f , m_environmentManager->GetDistantLightOverride() );
				distanceFade	= 10.0f;
#else
				distanceStart	= Max( distanceStart , m_environmentManager->GetDistantLightOverride() );
				// distanceFade, not changed so far
#endif
			}
		}


		// Set camera lights modifiers
		info.SetCameraLightModifiers( m_environmentManager->GetCameraLightsModifers() );

		// this will set up also wind / weather conditions parameters
		info.SetDayPointEnvParams( m_environmentManager->GetDayPointEnvironmentParams() );

		info.SetGameEnvParams( m_environmentManager->GetGameEnvironmentParams() );		
		info.SetBaseLightingParams( m_environmentManager->GetBaseLightingParams() );
		info.SetInstantAdaptation( instantAdaptation );
		
		// Update camera params (needs to be after worldRenderSettings because it's where the near/far planes are being setup)
		info.UpdateEnvCameraParams();
		
		// Set speed tree parameters
		{
			SGlobalSpeedTreeParameters speedTreeParams = m_environmentParameters.m_speedTreeParameters;
			speedTreeParams.ImportGlobalOverrides();			
			info.SetSpeedTreeParams( speedTreeParams );
		}

		// Use environment manager time
		Float envAnimTime = m_environmentManager->GetCurrentEnvAnimationTime();
		info.SetFrameTime( envAnimTime );

		// Update game time
		info.SetGameTime( m_environmentManager->GetCurrentGameTime().ToFloat(), gameDays );
	}
	else
	{
		// Use engine time
		const Float gameTime = GGame->GetEngineTime();
		info.SetFrameTime( gameTime );
		info.SetGameTime( gameTime, gameDays );
	}

	// Update engine time
	info.SetEngineTime( GGame->GetCleanEngineTime() );
	info.SetCleanEngineTime( GGame->GetCleanEngineTime() );

	// Setup reversed projection
	info.m_camera.SetReversedProjection( info.IsShowFlagOn( SHOW_ReversedProjection ) );
	info.m_occlusionCamera.SetReversedProjection( info.IsShowFlagOn( SHOW_ReversedProjection ) );
	info.m_globalWaterLevelAtCameraPos = GetWaterLevel( info.m_camera.GetPosition(), 1 );

	// Remember if the scene is the main world one
	info.m_isWorldScene = !m_isPreviewWorld;
	info.m_isGamePaused = info.m_isWorldScene && ( GGame && GGame->IsPaused() );
	info.m_allowSkinningUpdate = !info.m_isGamePaused || !info.m_isWorldScene;

	// Setup gameplay camera lights factor
	info.m_gameplayCameraLightsFactor = 1.f;
	if ( GGame && GetEnvironmentManager() && !m_isPreviewWorld )
	{	
		const CWeatherManager *weatherMgr = GetEnvironmentManager()->GetWeatherManager();
		const Bool isInterior = weatherMgr && weatherMgr->GetPlayerIsInInterior();		
		const Float currTime = GGame->GetCleanEngineTime();
		GetEnvironmentManager()->UpdateGameplayCameraLightsFactor( isInterior, currTime, info.m_camera.GetPosition(), false );
		info.m_gameplayCameraLightsFactor = GetEnvironmentManager()->GetGameplayCameraLightsFactor( currTime ); //< set factor
		info.m_gameplayCameraLightsFactor = Max( info.m_gameplayCameraLightsFactor, Min( 1.f, info.m_cameraLightsModifiersSetup.m_scenesSystemActiveFactor ) ); //< disable factor in scenes (we shouldn't have any gameplay lights there, but just in case)
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Last frame invalidation
	static CRenderCamera prevCamera;
	if( info.m_cameraLightsModifiersSetup.m_scenesSystemActiveFactor > 0.0f ) // If we are in cutscene
	{
		SCameraChangeTreshold cameraTreshold;
		// hackyCameraTreshold.m_positionTreshold = 1.0f;
		// hackyCameraTreshold.m_rotationTreshold = 20.0f;

		// Check if camera changed
		performInstantDissolve |= cameraTreshold.DoesCameraChanged( info.m_camera, prevCamera );
	}
	prevCamera = info.m_camera;

	// Force fades to finish - this is global so we send it to the scene
	// TODO: this is bound to camera (info.m_camera.GetLastFrameData()) and maybe should be moved to cameraManager ?
	if ( performInstantDissolve )
	{
#ifdef USE_ANSEL
		if ( !isAnselCaptureActive )
		{
			FinishRenderingFades();
		}
#endif // USE_ANSEL
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	info.m_hiResShadowMaxExtents = m_environmentManager->GetHiResShadowMapExtents();

	// Create rendering frame
	CRenderFrame* newFrame = GRender->CreateFrame( NULL, info );
	if ( !newFrame )
	{
		return nullptr;
	}

	if ( viewport )
	{
		PC_SCOPE_LV0( GeneratingDebugFragments, PBC_CPU );

#ifndef NO_EDITOR_FRAGMENTS
		// Collect visibility for new frame
		GenerateEditorFragments( newFrame );

		// Generate editor rendering fragments 
		IViewportHook* hook = viewport->GetViewportHook();
		if ( hook )
		{
			hook->OnViewportGenerateFragments( viewport, newFrame );
		}
#endif

		// Generate console fragments
		if ( GDebugConsole )
		{
			PC_SCOPE( DebugConsoleUpdate );
			GDebugConsole->OnGenerateFragments( newFrame );
		}
	}

	// Return created frame
	return newFrame;
}

void CWorld::RenderWorld( CRenderFrame* frame )
{
	// Schedule rendering
	if ( GEngine->IsActiveSubsystem( ES_Rendering ) && m_renderSceneEx )
	{
		( new CRenderCommand_RenderScene( frame, m_renderSceneEx ) )->Commit();
	}
	else
	{
		( new CRenderCommand_RenderScene( frame, NULL ) )->Commit();
	}
}

#ifndef NO_EDITOR

void CWorld::RenderWorld( CRenderFrame* frame, Bool forcePrefetch )
{
	// Schedule rendering
	if ( GEngine->IsActiveSubsystem( ES_Rendering ) && m_renderSceneEx )
	{
		( new CRenderCommand_RenderScene( frame, m_renderSceneEx, forcePrefetch ) )->Commit();
	}
	else
	{
		( new CRenderCommand_RenderScene( frame, NULL, forcePrefetch ) )->Commit();
	}
}

#endif
