/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "clipMap.h"

#include "game.h"
#include "freeCamera.h"
#include "../physics/physicsDebugger.h"
#include "renderCommands.h"
#include "clipMap.h"
#include "../core/objectGC.h"
#include "../core/scriptingSystem.h"
#include "../core/loadingProfiler.h"
#include "../core/feedback.h"
#include "../core/loadingJobManager.h"
#include "../core/ioTagResolver.h"
#include "physicsDataProviders.h"

#ifdef RED_PLATFORM_ORBIS
#include <perf.h>
#endif

#include "gameResource.h"
#ifndef RED_PLATFORM_CONSOLE
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#endif

#ifndef NO_TEST_FRAMEWORK
#	include "testFramework.h"
#endif
#include "renderFence.h"
#include "viewport.h"
#include "visualDebug.h"
#include "../physics/physicsSettings.h"
#include "pathlibWorld.h"
#include "animationManager.h"
#include "../core/gameSave.h"

#include "gameTimeManager.h"
#include "soundSystem.h"
#include "gameSaveManager.h"
#include "idTagManager.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWrapper.h"
#include "environmentComponentArea.h"
#include "worldIterators.h"
#include "worldTick.h"
#include "persistentEntity.h"
#include "layerGroup.h"
#include "dynamicLayer.h"
#include "layerInfo.h"
#include "baseEngine.h"
#include "inputManager.h"
#include "debugServerManager.h"
#include "tickManager.h"
#include "../core/objectDiscardList.h"
#include "../core/softHandleProcessor.h"
#include "../core/configVar.h"
#include "../physics/physicsWorld.h"
#include "foliageScene.h"
#include "../core/messagePump.h"
#include "containerManager.h"
#include "../core/contentManager.h"

extern Bool GEnablePhysicsDebuggerInGameExe;

RED_DEFINE_STATIC_NAME( TickManager );

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_WORLD_SUPPORT

void CGame::PrepareForPIE( const CGameInfo& info, TDynArray< CLayerInfo* >& layersForFastLoad )
{
	ASSERT( !GIsEditorGame );
	ASSERT( GIsEditor );	

	CTimeCounter timer;

	// Reset visibility flag for layers
	m_activeWorld->GetWorldLayers()->ResetVisiblityFlag( info );

	// Prepare world for PIE
	GIsEditorGame = true;

	// Prepare layers, auto load editor copy of visible layers if fast play was selected
	if ( info.m_keepExistingLayers )
	{
		CTimeCounter timer;

		for ( WorldAttachedLayerIterator it( GetActiveWorld() ); it; ++it )
		{
			CLayerInfo* layerInfo = ( *it )->GetLayerInfo();
			if ( layerInfo )
			{
				layersForFastLoad.PushBack( layerInfo );
			}
		}

		LOG_ENGINE( TXT("Check static time: %1.2fms"), timer.GetTimePeriodMS() );
	}

	// Cache layers
	{
		TDynArray< CLayerInfo* > allLayers;
		m_activeWorld->GetWorldLayers()->GetLayers( allLayers, false );

		// flush the pending jobs
		SJobManager::GetInstance().FlushPendingJobs();

		// finish loading of the layers that were loaded asynchronously
		for ( CLayerInfo* info : allLayers )
		{
			info->FinishAsyncLoading();
		}

		// This won't call any loading or unloading but will create
		// cached copy of layer ( and will detach it from world );
		for ( Uint32 i=0; i<allLayers.Size(); i++ )
		{
			CLayerInfo* info = allLayers[i];
			info->CreateLayerCopyForPIE();
		}

		// Process detachments
		FinishLayerTransitions();
	}

	// Stats
	LOG_ENGINE( TXT("Preparing for PIE took %1.2f seconds"), timer.GetTimePeriod() );
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGame::FinishLayerTransitions()
{
	if ( m_activeWorld )
	{
		// Do it for at most 10 iterations
		Uint32 numIterations = 0;
		while ( m_activeWorld && ++numIterations < 10 )
		{
			// Flush any background jobs, including IO jobs and blow jobs
			GEngine->FlushJobs();
			//SJobManager::GetInstance().FlushPendingJobs();

			// Update loading state - finish any loading or move to final destroy state
			m_activeWorld->GetWorldLayers()->UpdateLoadingState();

			// Check if we can exit
			{
				// Get all layers
				TDynArray< CLayerInfo* > layers;
				m_activeWorld->GetWorldLayers()->GetLayers( layers, false );

				// Flush any loading state
				Bool isAnyLayerLoading = false;
				Bool isAnyLayerDestroying = false;
				for ( Uint32 i=0; i<layers.Size(); i++ )
				{
					CLayerInfo* info = layers[i];
					if ( info->IsLoading() )
					{
						isAnyLayerLoading = true;
						break;
					}

					if ( info->IsUnloading() || info->IsPendingDestroy() )
					{
						isAnyLayerDestroying = true;
						break;
					}
				}

				// Done
				if ( !isAnyLayerLoading && !isAnyLayerDestroying )
				{
					break;
				}
			}
		}

		// Out of iterations
		if ( numIterations >= 10 )
		{
			WARN_ENGINE( TXT("FinishLayerTransitions iteration limit was hit. Please debug.") );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGame::LoadProperties( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(gameProp) );

	m_engineTime = loader->ReadValue< Double >( CNAME( EngineTime ) );

	const Uint32 numProperties = loader->ReadValue< Uint32 >( CNAME( nP ) );

	CClass* theClass = GetClass();
	for ( Uint32 i = 0; i < numProperties; ++i )
	{
		loader->ReadProperty( this, theClass, this );
	}

	m_gameTime = loader->ReadValue< Double >( CNAME( GameTime ) );
	if ( (Float)m_gameTime == 0.0f )
	{
		m_gameTime += SGameSessionManager::GetInstance().CalculateTimePlayed().GetSeconds();
	}
}

void CGame::SaveProperties( IGameSaver* saver )
{
	// Get properties to save
	const Uint32 maxNumProperties( 32 );
	Uint32 numPropertiesToSave( 0 );
	CProperty* propertiesToSave[ maxNumProperties ];

	CGameSaverBlock block( saver, CNAME(gameProp) );

	saver->WriteValue( CNAME( EngineTime ), (Double) m_engineTime );

	numPropertiesToSave = saver->GetSavableProperties( GetClass(), propertiesToSave, maxNumProperties);

	saver->WriteValue( CNAME( nP ), numPropertiesToSave );
	for ( Uint32 i = 0; i < numPropertiesToSave; ++i )
	{
		saver->WriteProperty( this, propertiesToSave[ i ] );
	}

	saver->WriteValue( CNAME( GameTime ), (Double) m_gameTime );
}

Bool CGame::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	if( !m_activeWorld )
	{
		HALT( "Trying to save game while no world is active" );
	}

	// Save world infoHALT(
	{
		CGameSaverBlock block( saver, CNAME(worldInfo) );
		saver->WriteValue( CNAME(world), m_activeWorld->GetDepotPath() );
	}

	// Additional save info
	{
		CGameSaverBlock block( saver, CNAME( AdditionalSaveInfo) );

		// We only serialize whether it was unkosher at the time of saving, whatever the reason, instead of saving the actual flags.
		// When restoring, we'll mark it as unkosher for "game".
		const Bool isKosher = IsKosher();
		saver->WriteValue( CNAME( HonorSystem2 ), isKosher );

		const Bool showBlackscreenAfterRestore = IsBlackscreen() && !IsFadeInProgress() && SGameSessionManager::GetInstance().ShouldBlackscreenBeShownAfterRestore();
		saver->WriteValue( CNAME( SavedBlackscreen ), showBlackscreenAfterRestore );
		SGameSessionManager::GetInstance().ShowBlackscreenAfterRestore( false );

		RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
		const SLoadingScreenParam& defaultParam = m_loadingScreenStack[0];
		saver->WriteValue( CNAME( QuestLoadingVideo ), defaultParam.m_videoToPlay );
		saver->WriteValue( CNAME( LoadingScreenName ), defaultParam.m_contextName );
		// Not saving initString unless absolutely necessary

		const Uint32 taintedFlags = GContentManager->GetTaintedFlags();
		saver->WriteValue( CNAME( ContentTaintedFlags ), taintedFlags );

		const TDynArray< CName >& baseContent = GContentManager->GetBaseContent();
		for ( const CName contentName : baseContent )
		{
			const Bool isActivated = GContentManager->IsContentActivated( contentName );
			saver->WriteValue( contentName, isActivated );
		}
	}

	SaveRequiredDLCs( saver );

	{
		CGameSaverBlock block( saver, CNAME( ContainerManagerSaveInfo ) );
		RED_FATAL_ASSERT( m_containerManager, "Container manager should always exist at this point." );
		m_containerManager->Save( saver );
	}

	// Time scales
	{
		Uint32 numTimescalesToSave = 0;
		Bool hasValidTimescalEntity = false;

		for ( Uint32 i = 0; i < m_timeScaleSets.Size(); ++i )
		{
			const TDynArray< STimeScaleSource >& entries = m_timeScaleSets[ i ].m_entries;
			hasValidTimescalEntity = false;

			for ( Uint32 t = 0; t < entries.Size(); t++ )
			{
				if ( entries[ t ].m_dontSave == false)
				{
					hasValidTimescalEntity = true;
					break;
				}
			}

			if ( hasValidTimescalEntity )
			{
				++numTimescalesToSave;
			}			
		}

		CGameSaverBlock block( saver, CNAME( TimeScale ) );

		saver->WriteValue( CNAME( count ), numTimescalesToSave );

		for ( Uint32 i = 0; i < m_timeScaleSets.Size(); ++i )
		{
			const TDynArray< STimeScaleSource >& entries = m_timeScaleSets[ i ].m_entries;
			hasValidTimescalEntity = false;

			for ( Uint32 t = 0; t < entries.Size(); t++ )
			{
				if ( entries[ t ].m_dontSave == false)
				{
					hasValidTimescalEntity = true;
					break;
				}
			}

			if ( hasValidTimescalEntity )
			{
				CGameSaverBlock block( saver, CNAME( TimeScaleSet ) );
				saver->WriteValue/*< STimeScaleSourceSet >*/( CNAME( TimeScaleSet ), m_timeScaleSets[ i ] );
			}
		}
	}

	CWorld* activeWorld = m_activeWorld.Get();
	ASSERT( activeWorld );

	// Tick manager
	{
		CGameSaverBlock block( saver, CNAME( TickManager ) );
		activeWorld->GetTickManager()->OnSaveGameplayState( saver );
	}

	// World

	CPeristentEntity* playerEntity = static_cast< CPeristentEntity* >( GetPlayerEntity() );

	m_universeStorage.CapturePlayerState( playerEntity );
	m_universeStorage.CapturePlayerManagedAttachments( GetPlayerEntity() );

	TIMER_BLOCK( captureStateEnd )
	m_universeStorage.GetWorldStorage( activeWorld )->CaptureStateStage3(); 
	END_TIMER_BLOCK( captureStateEnd )

	m_universeStorage.Save( saver );

	// Additional world state
	{
		CGameSaverBlock block( saver, CNAME(worldState) );
		m_activeWorld->SaveState( saver );
	}

	SaveProperties( saver );

	END_TIMER_BLOCK( time )
	
	// Done
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern Bool GCanProcessLayerAttachments;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

Bool CGame::StartPreview( const String& worldFileName )
{
	////////////////////////////////////////

	// Remove all streaming textures
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();
	}

	// End current game
	if ( m_isActive )
	{
		WARN_ENGINE( TXT("Cannot start preview if there is an active game") );
		return false;
	}

	////////////////////////////////////////

	// Show loading screen
	ShowLoadingScreen();

	////////////////////////////////////////

	// Reset game crap
	m_requestUnstop = false;
	m_stopped = false;
	m_activePause = CountedBool();
	ResetPauseCounter();

	////////////////////////////////////////

	// Suppress any layer attachments
	GCanProcessLayerAttachments = false;

	////////////////////////////////////////

	// Load world
	CGameInfo info;
	if ( !LoadGameWorld( info, worldFileName ) )
	{
		return false;
	}

	////////////////////////////////////////

	// Activate game physics
	GEngine->SetActiveSubsystem( ES_Physics, true );

	////////////////////////////////////////

	// Attach all layers
	GCanProcessLayerAttachments = true;
	m_activeWorld->GetWorldLayers()->ConditionalAttachToWorld();

	////////////////////////////////////////

	// Create free camera
	m_freeCamera->MoveTo( m_activeWorld->GetStartupCameraPosition(), m_activeWorld->GetStartupCameraRotation() );
	m_freeCameraActive = true;

	////////////////////////////////////////

	// Process world delayed actions - everything will attach here
	m_activeWorld->DelayedActions();

	////////////////////////////////////////

	// Collect crap
	GObjectGC->CollectNow();

	////////////////////////////////////////

	// Clear performance warnings this far
	GScreenLog->ClearPerfWarnings();

	////////////////////////////////////////

	// Install game input hook
	m_prevHook = m_viewport->GetViewportHook();
	m_viewport->SetViewportHook( this );

	////////////////////////////////////////

	// Done
	return true;
}

namespace DebugProfiler
{
	extern void StartDebugProfiler(const Char* name);
	extern void EndDebugProfiler();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Helper
{
	struct SScopedViewportHook : public IViewportHook
	{
		IViewport*		m_viewport;
		IViewportHook*	m_prevHook;
		Bool			m_allowInput;
		Bool			m_allowCameraUpdates;
		Bool			m_playerRequestedExit;
		Bool			m_receivedInput;
		Bool			m_lastUsedPCInput;

		SScopedViewportHook( IViewport* viewport )
			: m_viewport( viewport )
			, m_prevHook( nullptr )
			, m_allowInput( false )
			, m_allowCameraUpdates( false )
			, m_playerRequestedExit( false )
			, m_receivedInput( false )
			, m_lastUsedPCInput( false )
		{
			if ( m_viewport )
			{
				m_prevHook = m_viewport->GetViewportHook();
				m_viewport->SetViewportHook( this );
			}
		}

		~SScopedViewportHook()
		{
			if ( m_viewport )
			{
				RED_FATAL_ASSERT( m_viewport->GetViewportHook() == this, "Viewport hook changed!");
				m_viewport->SetViewportHook( m_prevHook );
			}
		}

		void AllowInput()
		{
			SRawInputManager::GetInstance().RequestReset(); // Reset any pent up input
			m_allowInput = true;
		}

		void AllowCameraUpdates()
		{
			m_allowCameraUpdates = true;
		}

		void Reset()
		{
			if ( m_viewport )
			{
				RED_FATAL_ASSERT( m_viewport->GetViewportHook() == this, "Viewport hook changed!");
				m_viewport->SetViewportHook( m_prevHook );
			}
			m_viewport = nullptr;
			m_prevHook = nullptr;
		}

		virtual Bool OnViewportInput( IViewport* view, EInputKey key, EInputAction action, Float data ) override
		{
			if ( m_allowInput )
			{
				if ( GGame->IsLoadingVideoSkipKey( key ) )
				{
					m_playerRequestedExit = true;
				}
				
				InputUtils::EInputDeviceType deviceType = InputUtils::GetDeviceType( key );
				m_lastUsedPCInput = deviceType == InputUtils::IDT_KEYBOARD || deviceType == InputUtils::IDT_MOUSE;
				m_receivedInput = true;
			}

			return true;
		}

		virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera& camera )
		{
			if ( ! m_allowCameraUpdates )
			{
				return;
			}

			if ( GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetCameraDirector() && !GGame->IsFreeCameraEnabled() )
			{
				GGame->GetActiveWorld()->GetCameraDirector()->OnViewportCalculateCamera( view, camera );
			}
		}

		Bool IsPlayerRequestedExit() const { return m_playerRequestedExit; }
		Bool IsReceivedInput() const { return m_receivedInput; }
		Bool IsLastUsedPCInput() const { return m_lastUsedPCInput; }

		void ResetInput() { m_playerRequestedExit = false; m_receivedInput = false; }
	};
}

namespace Config
{
	extern TConfigVar< Bool > cvLoadingScreenDebugDisableForceLoads;
	
	//
	extern TConfigVar< Float, Validation::FloatRange<0,5,1> > cvLoadingScreenSkipInputDelay;
	extern TConfigVar< Float, Validation::FloatRange<0,5,1> > cvLoadingScreenSkipHideDelay;
	extern TConfigVar< Bool > cvLoadingScreenSkipShowWithAnyKey;
	extern TConfigVar< Bool > cvLoadingScreenSkipAutoshow;
}

#define TOTAL_PROGRESS (26.f)
#define SMALL_PROGRESS (1.f/TOTAL_PROGRESS)
#define BIG_PROGRESS (2*SMALL_PROGRESS)

#define UPDATE_LOADING_PROGRESS(val)\
	do{\
		loadProgress += (val);\
		( new CRenderCommand_UpdateProgress( loadProgress ) )->Commit();\
	} while(false)

Bool CGame::StartGame( const CGameInfo& info )
{
	Float loadProgress = 0.f;

	CoreMemory::CScopedMemoryDump dumpMetrics( TXT( "CGame::StartGame" ) );
	PC_SCOPE( StartGame );

	m_isInStartGame = true;
	Red::System::ScopedFlag< Bool > startGameFlag( m_isInStartGame, false );

	// Prevent the profile manager from processing input (activate account picker on xbox if a user signs out, establish new active user profile)
	GUserProfileManager->ToggleInputProcessing( false, CUserProfileManager::eAPDR_Ingame );

	// Start world loading mode
	GFileSysPriorityResovler.SwitchConext( eIOContext_WorldLoading );

	// New game is starting
	{
		EDITOR_DISPATCH_EVENT( CNAME( GameStarting ), CreateEventData( this ) );
		GLoadingProfiler.FinishStage( TXT("EventDispatch") );
	}

	////////////////////////////////////////

	// Reset input so the new viewport hook doesn't get any previous input events that might
	// cancel video
	SRawInputManager::GetInstance().RequestReset();

	m_inputManager->Reset();


	// Don't rely on the CGame::OnViewportInput to check if loading. Too fragile with overrides.
	Helper::SScopedViewportHook scopedViewportHook( GetViewport() );

	// Remove all streaming textures
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();

		GLoadingProfiler.FinishStage( TXT("RenderFlush") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	// End current game
	if ( m_isActive )
	{
		WARN_ENGINE( TXT("Cannot start new game if there is an active one") );
		return false;
	}

	OnGameStart( info );

	if ( info.m_gameLoadStream )
	{
		m_universeStorage.Load( info.m_gameLoadStream );
		GLoadingProfiler.FinishStage( TXT("SaveGameLoading") );

		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

#ifndef NO_EDITOR_WORLD_SUPPORT
	TDynArray< CLayerInfo* > layersForFastLoad;
	{
		// Copy level for in editor game, detach non static layers
		if ( info.m_inEditorGame )
		{
			PrepareForPIE( info, layersForFastLoad );
			GLoadingProfiler.FinishStage( TXT("PrepareForPIE") );
		}
	}
#endif

	// GAME IS ENABLED FROM NOW ON 
	m_requestUnstop = false;
	m_stopped = false;
	m_isActive = true;
	m_activePause = CountedBool();

	// Clear all streaming related locks
	m_streamingLocks.Clear();
	m_streamingCameraOverrides.Clear();
	UpdateStreamingModeAndDistances();

	// Register in save game crap
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );

	// Suppress any layer attachments
	GCanProcessLayerAttachments = false;

	////////////////////////////////////////

	// Reset time manager
	//
	// this need to be before sound init becouse music depend on deserialized time
	{
		m_timeManager->OnGameStart( info );
		GLoadingProfiler.FinishStage( TXT("TimeManager") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	// Preload animations
	{
		GAnimationManager->PreloadAnimationsOnGameStart();
		GLoadingProfiler.FinishStage( TXT("AnimationSystem") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	// debug server - game start
	{
		if ( !info.m_isChangingWorldsInGame )
		{
			DBGSRV_CALL( GameStarted() );
		}
		GLoadingProfiler.FinishStage( TXT("DebugServer") );
	}

	// CONTAINER MANAGER, should be always loaded before loading world
	RED_FATAL_ASSERT( m_containerManager, "Container manager should exist at this points." );

	if ( !info.IsChangingWorld() )
	{
		m_containerManager->Reset();
		m_gameTime = EngineTime::ZERO;
	}
	if( info.m_gameLoadStream )
	{
		CGameSaverBlock block( info.m_gameLoadStream, CNAME( ContainerManagerSaveInfo ) );
		m_containerManager->Load( info.m_gameLoadStream );
	}

	{
#ifdef RED_PLATFORM_ORBIS
		// We must enter the game without stalling because of Blu-ray.
		// TBD: prefetching before quest ticking
		
		extern Bool GPlayGoFullInstallSpeedOverride;
		Red::System::ScopedFlag<Bool> guard( GPlayGoFullInstallSpeedOverride = true, false );
		const Float discTimeoutSec = 600.f; // W3: give it more than enough if having to stall for a savegame where we need to stall for up to the full launch content. Long, but not infinite.
		CTimeCounter timer;
		for(;;) 
		{
			GContentManager->Update();

			// Check for "stall" vs content available because if you eject the BDD then the game exits
			// If you have a network install, then we should avoid infinite loading. We'll have the launch chunk on a download install,
			// but just to make things explicit.
			if ( GContentManager->GetStallForMoreContent() < eContentStall_Prefetch )
			{
				break;
			}

			// Pump messages only if it looks like we need to wait. For PLM and if ever waiting for disc on Durango
			if (GMessagePump)
			{
				GMessagePump->PumpMessages();
			}

			if ( timer.GetTimePeriod() > discTimeoutSec )
			{
				ERR_ENGINE(TXT(">>>Timeout during WaitForDisc<<<"));
				GContentManager->DumpProgressToLog();
				break;
			}
		}
		GLoadingProfiler.FinishLoading( TXT("WaitForDisc") );
		UPDATE_LOADING_PROGRESS( BIG_PROGRESS );
#endif // RED_PLATFORM_ORBIS
	}

	////////////////////////////////////////////////////////////////////////////////
	// Load world
	////////////////////////////////////////////////////////////////////////////////
	if ( !info.m_inEditorGame || info.m_isChangingWorldsInGame )
	{
		// Loading from save game
		if ( info.m_gameLoadStream )
		{
			// Load the world and partition name
			String worldFile;
			{
				CGameSaverBlock block( info.m_gameLoadStream, CNAME(worldInfo) );
				info.m_gameLoadStream->ReadValue( CNAME(world), worldFile );
			}

			if ( !LoadGameWorld( info, worldFile ) )
			{
				return false;
			}
		}
		else
		{
			// World file to load should be specified
			if ( info.m_worldFileToLoad.Empty() )
			{
				WARN_ENGINE( TXT("No world to load specified. Not able to start game.") );
				return false;
			}

			// Load world
			if ( !LoadGameWorld( info, info.m_worldFileToLoad ) )
			{
				return false;
			}
		}
	}
#ifndef NO_EDITOR_WORLD_SUPPORT
	else 
	{
		// Load fast load layers ( from editor )
		if ( info.m_keepExistingLayers )
		{
			// Must be after DelayedActions -> these layers will be attached as during normal game start during ConditionalAttachToWorld
			for ( Uint32 i=0; i<layersForFastLoad.Size(); ++i )
			{
				// Load
				LayerLoadingContext info;

				CLayerInfo* currentLayer = layersForFastLoad[i];
				currentLayer->ForceVisible( true );
				currentLayer->SyncLoad( info );				
			}
		}
		else
		{	
			m_activeWorld->LoadStaticData();
		}
	}
#endif

	UPDATE_LOADING_PROGRESS( BIG_PROGRESS );

	////////////////////////////////////////////////////////////////////////////////

	// Restore tick manager
	if ( info.m_gameLoadStream && info.m_gameLoadStream->GetSaveVersion() >= SAVE_VERSION_TIMERS_WITH_IDS )
	{
		CGameSaverBlock block( info.m_gameLoadStream, CNAME( TickManager ) );
		m_activeWorld->GetTickManager()->OnLoadGameplayState( info.m_gameLoadStream );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	// Additional world state
	// Gameplay systems depend on engine time. So, properties have to be loaded before them.
	if ( info.m_gameLoadStream )
	{
		CGameSaverBlock block( info.m_gameLoadStream, CNAME(worldState) );
		m_activeWorld->RestoreState( info.m_gameLoadStream );
		GLoadingProfiler.FinishStage( TXT("RestoreState") );

		LoadProperties( info.m_gameLoadStream );
		GLoadingProfiler.FinishStage( TXT("LoadProperties") );
	}

	////////////////////////////////////////////////////////////////////////////////

	// Initialize gameplay systems (some require world created)
	{
		OnGameplaySystemsGameStart( info );
		GLoadingProfiler.FinishStage( TXT("InitGameplay") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////////////////////////////////////////////

	NotifyGameInputModeEnabled();

#ifndef NO_TEST_FRAMEWORK
	SRawInputManager::GetInstance().RegisterListener( &STestFramework::GetInstance() );
#endif

	// Game is starting
	ResetPauseCounter();

	////////////////////////////////////////

	// Reset IdTag manager
	{
		m_idTagManager->OnGameStart( info );
		GLoadingProfiler.FinishStage( TXT("IdTagManager") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	CPathLibWorld* pathlib = m_activeWorld->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->SetGameRunning( true );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}
#endif

	CAreaEnvironmentComponent* environmentComponent = NULL;
	{
		// Find the default environment.  The default environment is
		// stored in one of the root entities with the tag "autoset"
		CLayerGroup* rootLayerGroup = m_activeWorld->GetWorldLayers();
		TDynArray<CLayerInfo*> rootLayers;
		CName autosetTag( TXT("autoset") );
		if ( info.m_keepExistingLayers )
		{
			rootLayerGroup->GetLayers( rootLayers, true, false, false );
			for ( auto it = rootLayers.Begin(); !environmentComponent && it != rootLayers.End(); ++it )
			{
				CLayer* layer = ( *it )->GetLayer();
				if ( layer )
				{
					const LayerEntitiesArray& entities = layer->GetEntities();
					for ( auto it = entities.Begin(); !environmentComponent && it != entities.End(); ++it )
					{
						CEntity* ent = *it;
						if ( ent->GetTags().HasTag( autosetTag ) )
						{
							const TDynArray<CComponent*> components = ent->GetComponents();
							for ( auto it = components.Begin(); !environmentComponent && it != components.End(); ++it )
							{
								environmentComponent = Cast<CAreaEnvironmentComponent>( *it );
							}
						}
					}
				}
			}
		}

		GLoadingProfiler.FinishStage( TXT("EnvManager") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	{
		if ( info.m_playerLoadStream )
		{
			m_universeStorage.LoadPlayerOnly( m_activeWorld, info.m_playerLoadStream ); 
		}

		m_universeStorage.ApplyWorldState( m_activeWorld.Get() );

		GLoadingProfiler.FinishStage( TXT("UniversalStorage") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	CPhysicsWorld* physicsWorld = nullptr;
	m_activeWorld->GetPhysicsWorld( physicsWorld );

	// Physical debugger
#ifndef PHYSICS_RELEASE
	if ( !GIsEditor && GEnablePhysicsDebuggerInGameExe && GPhysicsDebugger )
	{
		GPhysicsDebugger->AttachToWorld( physicsWorld );
	}
#endif

	// Activate game physics
	GEngine->SetActiveSubsystem( ES_Physics, true );

	// Game started
	LOG_ENGINE( TXT("Game started") );

	// Pause IO jobs so we do not create contention with main thread
	SJobManager::GetInstance().Lock();

	// Block all movement on the physical world untill the terrain and static geometry gets loaded
	physicsWorld->ToggleMovementLock( true );

	// Do not stream anything until we are ready
	m_enableStreamingPrefetch = false;

	////////////////////////////////////////

	// Attach all layers
	GCanProcessLayerAttachments = true;

	{
#ifdef RED_PLATFORM_ORBIS
		/*fprintf(stderr,"Capture in....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"3....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"2....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"1....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"0....\n");*/
#endif

		m_activeWorld->PreLayersAttach();
		m_activeWorld->GetWorldLayers()->ConditionalAttachToWorld();
		GLoadingProfiler.FinishStage( TXT("AttachLayers") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );

#ifdef RED_PLATFORM_ORBIS
		/*fprintf(stderr,"End in....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"3....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"2....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"1....\n");
		Red::Threads::SleepOnCurrentThread(1000);
		fprintf(stderr,"0....\n");*/
#endif
	}

	////////////////////////////////////////

	// Initialize player
	
	// Create player
	{
		CEntity* player = CreatePlayer( info );
		GLoadingProfiler.FinishStage( TXT("CreatePlayer") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	// Initialize camera
	{
		// Create camera
		m_camera = CreateCamera();
		GLoadingProfiler.FinishStage( TXT("CreateCamera") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	{
		// Initialize gameplay related systems
		OnGameplaySystemsWorldStart( info );
		GLoadingProfiler.FinishStage( TXT("InitWorldGameplay") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////

	// Inform the world
	EDITOR_QUEUE_EVENT( CNAME( GameStarted ), CreateEventData( this ) );
	m_activeWorld->OnGameStarted( info );
	GLoadingProfiler.FinishStage( TXT("GameStartedEvent") );
	UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );

	////////////////////////////////////////

	// Clear performance warnings this far
	GScreenLog->ClearPerfWarnings();

	////////////////////////////////////////

	// If the environment component is found, set it
	if ( environmentComponent )
	{
		environmentComponent->Activate( true );
		GLoadingProfiler.FinishStage( TXT("EnvironmentActivation") );
		UPDATE_LOADING_PROGRESS( SMALL_PROGRESS );
	}

	////////////////////////////////////////se;f

	const Bool loadingScreenDebugDisableForceLoads = Config::cvLoadingScreenDebugDisableForceLoads.Get();
	LOG_ENGINE(TXT("CLoadingScreen: cvLoadingScreenDebugDisableForceLoads: %d"), loadingScreenDebugDisableForceLoads );

	// disable terrain loading during quest stabilization
	// we will load the terrain fully once we have proper camera position
	m_activeWorld->GetTerrain()->ToggleLoading( false );

	// shitty hack - collect BEFORE stabilizing quests
	// this is needed because we have OOM on the CObject pool ATM
	GObjectGC->CollectNow();

	const Bool wasBlackscreenSet = info.m_isChangingWorldsInGame && HasBlackscreenRequested();
	if ( info.m_setBlackscreen || wasBlackscreenSet )
	{
		if ( !m_blackscreenLockOnReason.Empty() )
		{
			ResetFadeLock( TXT("__StartGame__") );
		}

		const String tempReasonStr = TXT("__StartGame__") + m_blackscreenOnReason;

		SetBlackscreen( true, tempReasonStr );

		if ( !m_blackscreenLockOnReason.Empty() )
		{
			const String tempLockStr = TXT("__StartGame__") + m_blackscreenLockOnReason;

			SetFadeLock( tempLockStr );
		}
		else if ( info.m_setBlackscreen )
		{
			SetFadeLock( TXT("__StartGame__RestoreSession") );
		}
	}

	if ( !loadingScreenDebugDisableForceLoads )
	{

		PC_SCOPE( StabilizeQuests )

		const CGameInfo& gameInfo = SGameSessionManager::GetInstance().GetGameInfo();
		if ( gameInfo.m_allowQuestsToRun )
		{
			// If changing worlds, we can have layers changing and the thread was previous stable.
			// Unstabilizing all threads since they could have been waiting for world change too.
			if ( gameInfo.m_isChangingWorldsInGame )
			{
				StartGame_ResetQuestStability();
			}

			// stabilization ticks
			const Uint32 maxTicks = gameInfo.m_gameLoadStream ? 0 : 30;
			Uint32 questTick = 0;

			Bool pendingLayerChanges = false;
			Bool newLayerChanges = false;
			for ( ; questTick<maxTicks; ++questTick )
			{
				// If the user signs out during loading, we need to break out of this loop
				// or else get stuck in an infinite loop inside Tick()
				if( m_requestEnd )
					break;

				// If pendingLayerChanges us true, ticking this frame will actually create the jobs, then they'll be flushed to completion.
				// We'll loop again after to tick quest system and progress blocks waiting on layer changes.
				pendingLayerChanges = newLayerChanges;
				newLayerChanges = false;

				// tick the game until the quests stabilize :( :( :(
				const Float smallTick = 0.001f;
				Tick( smallTick );
				GPhysicEngine->Update( smallTick );

				// The quest system can try showing/hiding layers for worlds that aren't loaded yet		
				// flush pending IO jobs
				{
					PC_SCOPE( FlushPendingIO )

					SJobManager::GetInstance().Unlock();
					SJobManager::GetInstance().FlushPendingJobs();
					SJobManager::GetInstance().Lock();
				}

				GObjectsDiscardList->ProcessList( true );

				// the quests and layers were stabilized
				if ( !pendingLayerChanges && !newLayerChanges && StartGame_AreQuestsStable() )
				{
					break;
				}			
			}

			if ( questTick >= maxTicks )
			{
#ifndef RED_FINAL_BUILD
				StartGame_DebugDumpUnstableQuestThreads();
#endif

				WARN_ENGINE(TXT("Reached maxTicks waiting for quest system to stabilize!"));
			}
			else
			{
				LOG_ENGINE(TXT("Quests stabilized on questTick #%u"), questTick );
			}

			RED_ASSERT( ! IsPaused(), TXT("Game paused while trying to tick to stabilize loading!") );

		}

		GLoadingProfiler.FinishStage( TXT("LoadGameplay") );
		UPDATE_LOADING_PROGRESS( BIG_PROGRESS );
	}

	////////////////////////////////////////

	SJobManager::GetInstance().Unlock();

	////////////////////////////////////////

#ifdef RED_PLATFORM_CONSOLE
	{
		const Float saveTimeoutSec = 200.f;
		CTimeCounter timer;
		for (;;)
		{
			// Could have had a save request from ticking quests. Update now to try and mitigate TRC timeouts for unmounting
			if ( GMessagePump )
			{
				GMessagePump->PumpMessages();
			}

			if ( !StartGame_UpdateSaver() )
			{
				break;
			}
			
			if ( timer.GetTimePeriod() > saveTimeoutSec )
			{
				ERR_ENGINE(TXT("StartGame_UpdateSaver timed out waiting for saving to finish!"));
				break;
			}
		}

		GLoadingProfiler.FinishStage( TXT("StartGame_UpdateSaver") );
	}
#endif

	{
		// Tick the behtree shit to avoid invisible Geralt. Get the items to start loading.
		const CGameInfo& info = SGameSessionManager::GetInstance().GetGameInfo();
		if ( info.m_gameLoadStream )
		{
			const Float smallTick = 0.001f;
			Tick( smallTick );
			GPhysicEngine->Update( smallTick );		
		}
	}

	// World can be unloaded during the tick
	// It is the case when user is signing out during the loading screen 
	// Game tries to return to the main menu
	if ( nullptr == m_activeWorld )
	{
		return false;
	}

	////////////////////////////////////////

	// All static geometry is about to be loaded - unblock movement on the physical world
	physicsWorld->ToggleMovementLock( false );

	////////////////////////////////////////

	// Unlock the streaming prefetch
	m_enableStreamingPrefetch = true;

	////////////////////////////////////////

	// Prefetch for current camera position
	{
		// determine prefetch source
		if ( UseCameraAsStreamingReference() || !GetPlayerEntity() )
		{
			m_prefetchPosition = m_activeWorld->GetCameraPosition();
			LOG_ENGINE( TXT("Initial prefetch is from camera's position %ls"), ToString( m_prefetchPosition ).AsChar() );
		}
		else
		{
			m_prefetchPosition = GetPlayerEntity()->GetWorldPosition();
			LOG_ENGINE( TXT("Initial prefetch is from player's position %ls"), ToString( m_prefetchPosition ).AsChar() );
		}

		SetStreamingReferencePosition( m_prefetchPosition );

		SPrefetchParamsW3Hack params;
		params.m_fastForwardCommunities = !info.IsSavedGame();
		params.m_waitForPlayer = true;
		params.m_pathlibStreaming = info.IsSavedGame();
		params.m_forceUITextureStreaming = !info.IsNewGame();// HACK: since just extra loading time in a new game
		params.m_isGameStart = true;

		// prefetch data for current location. Do not show additional loading screen (we already have one)
		PerformPrefetch( false, params );

		GLoadingProfiler.FinishStage( TXT("InitialPerfetch") );
		UPDATE_LOADING_PROGRESS( BIG_PROGRESS );
	}

	////////////////////////////////////////

	scopedViewportHook.AllowInput();

	const Bool allowSkipVideo = (GContentManager->GetStallForMoreContent() == eContentStall_None) && m_loadingScreen->IsPlayingVideo();

	Bool isSkipVisible = false;
	if ( allowSkipVideo && Config::cvLoadingScreenSkipAutoshow.Get() )
	{
		m_loadingScreen->ToggleVideoSkip( true );
		isSkipVisible = true;
	}

	Bool cachedLastUsedPCInput = scopedViewportHook.IsLastUsedPCInput();
	CTimeCounter skipVisibleTimer;
	while ( m_loadingScreen->IsPlayingVideo() )
	{
		if (GMessagePump)
		{
			GMessagePump->PumpMessages();
		}

		if (  m_requestEnd || m_requestEndDueToSignout /*okay... why can we still process input here anyway? */ )
		{
			break;
		}

		if ( allowSkipVideo )
		{
			// Process input to allow video cancellation.
			SRawInputManager::GetInstance().ProcessInput();
			const Bool requestedExit = scopedViewportHook.IsPlayerRequestedExit();
			const Bool receivedInput = scopedViewportHook.IsReceivedInput();
			const Bool lastUsedPCInput = scopedViewportHook.IsLastUsedPCInput();
			scopedViewportHook.ResetInput();
			
			if (lastUsedPCInput != cachedLastUsedPCInput)
			{
				cachedLastUsedPCInput = lastUsedPCInput;
				m_loadingScreen->SetPCInput(cachedLastUsedPCInput);
			}

			if ( receivedInput )
			{
				// "Refresh" if any key even if already visible, might want to keep it from fading out in Flash
				if ( (Config::cvLoadingScreenSkipShowWithAnyKey.Get() && !requestedExit) || (!isSkipVisible && requestedExit) )
				{
					if( isSkipVisible == false )	// Don't send render command each time, when there is no difference in skip visibility. Otherwise videos stutter when applying input
					{
						m_loadingScreen->ToggleVideoSkip( true );
					}
					isSkipVisible = true;
					skipVisibleTimer.ResetTimer();
				}
				else if ( isSkipVisible && requestedExit )
				{
					if ( skipVisibleTimer.GetTimePeriod() >= Config::cvLoadingScreenSkipInputDelay.Get() )
					{
						break; // The video will be stopped during the loading screen fadeout
					}
				}
			}

			const Float hideDelay = Config::cvLoadingScreenSkipHideDelay.Get();
			if ( isSkipVisible && skipVisibleTimer.GetTimePeriod() >= hideDelay && hideDelay > 0. )
			{
				m_loadingScreen->ToggleVideoSkip( false );
				isSkipVisible = false;
				skipVisibleTimer.ResetTimer();
			}
		}
	}

	// Just don't send render command if not necessary. Just hiding it when pressed since Flash may want to
	// do some effect of its own. The button is part of the loading screen so would have been hidden anyway.
	if ( isSkipVisible )
	{
		m_loadingScreen->ToggleVideoSkip( false );
	}

	SRawInputManager::GetInstance().RequestReset();
	scopedViewportHook.Reset();

	// Install low level viewport hook
	m_prevHook = m_viewport->GetViewportHook();
	m_viewport->SetViewportHook( this );

	////////////////////////////////////////////////////////////////////////////////

	// Do some final work after loading screen
	{
		OnAfterLoadingScreenGameStart( info );
		GLoadingProfiler.FinishStage( TXT("AfterLoading") );
	}

	////////////////////////////////////////

	GLoadingProfiler.FinishStage( TXT("WaitingForVideo") );

	////////////////////////////////////////////////////////////////////////////////
	//reset clothing on player only
	{
		CEntity* entity = GetPlayerEntity();
		if( CPeristentEntity* persEnt = Cast<CPeristentEntity>( entity ) )
		{
			persEnt->ResetClothAndDangleSimulation();
		}
	}

	////////////////////////////////////////

	// Loading is finished, start world streaming mode
	GFileSysPriorityResovler.SwitchConext( eIOContext_WorldStreaming );

	// Report no errors 
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGame::EndGame()
{
	ASSERT( m_isActive );

	// Detach storage from dynamic layer to prevent modifying savegame state
	m_activeWorld->GetDynamicLayer()->SetStorage( nullptr );

	const CGameInfo& info = SGameSessionManager::GetInstance().GetGameInfo();

	// debug server - game stop
	if ( !info.m_isChangingWorldsInGame )
	{
		DBGSRV_CALL( GameStopped() );
	}

	// Shut down time manager
	m_timeManager->OnGameEnd( info );

	// Shut down id tag manager
	m_idTagManager->OnGameEnd( info );

	// Close any game systems that relates to active world
	OnGameplaySystemsWorldEnd( info );

	// Unregister cutscenes
	UnregisterAllCutscenes();

	// Inform editor listeners
	EDITOR_DISPATCH_EVENT( CNAME( GameEnding ), CreateEventData( this ) );

	// Deactivate game physics
	GEngine->SetActiveSubsystem( ES_Physics, false );

	// Finish any pending background operations
	GEngine->FlushJobs();

	//SJobManager::GetInstance().FlushPendingJobs();

	// Update layer loading state to reflect crap loaded by the async loader
	if ( m_activeWorld )
	{
		m_activeWorld->UpdateLoadingState();

#ifndef NO_EDITOR_PATHLIB_SUPPORT
		CPathLibWorld* pathlib = m_activeWorld->GetPathLibWorld();
		if ( pathlib )
		{
			pathlib->SetGameRunning( true );
		}
#endif


	}

	// Remove cameras
	EnableFreeCamera( false );
	{
		// Destroy main camera
		if ( m_camera )
		{
			DestroyCamera( m_camera );
			m_camera = NULL;
		}
	}

	// Remove player
	{
		// Unregister player from the scripting system
		ASSERT( GScriptingSystem );
		GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_PLAYER, NULL );
	}

	// Remove UI
	{
		// Unregister UI from the scripting system
		ASSERT( GScriptingSystem );
		GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_HUD, NULL );
	}

	// Inform the world
	EDITOR_QUEUE_EVENT( CNAME( GameEnded ), CreateEventData( this ) );

	// Clear global visual debug
	if( m_visualDebug )
	{
		m_visualDebug->RemoveAll();
	}

	// Restore default camera parameters
	if( m_activeWorld->GetCameraDirector() )
	{
		m_activeWorld->GetCameraDirector()->ResetCameraData();
	}

#ifndef NO_EDITOR_WORLD_SUPPORT
	// Release input capture
	if ( info.m_inEditorGame && !info.m_isChangingWorldsInGame )
	{
		ASSERT( GIsEditorGame );
		NotifyGameInputModeDisabled();
	}
#endif

	// Install previous viewport hook
	m_viewport->SetViewportHook( m_prevHook );
	m_prevHook = NULL;

	// Always delete shit from dynamic layer
	{
		m_activeWorld->GetDynamicLayer()->DestroyAllEntities();

		// Finish transitions
		FinishLayerTransitions();
	}

	// Reset streaming. Could possibly have something left if it hasn't expired yet
	m_streamingLocks.ClearFast();
	m_streamingLocksFrameCounter = 0;
	m_streamingLocksModified = false;
	m_requestStreamingPrefetch = false;

	// Unload world
	if ( !info.m_inEditorGame )
	{
		UnloadWorld();
	}

	// Make sure the pause counter is reset so components that need to be ticked for async stuff get ticked
	ResetPauseCounter();

	// Restore world - unload loaded shit, it needs to be bone while the GGame->IsActive() is still true
#ifndef NO_EDITOR_WORLD_SUPPORT
	if ( info.m_inEditorGame && !info.m_isChangingWorldsInGame )
	{
		ASSERT( GIsEditorGame );

		// Get all layers
		TDynArray< CLayerInfo* > allLayers;
		m_activeWorld->GetWorldLayers()->GetLayers( allLayers, false );

		// Unload non static layers
		// MattH: TTP Witcher 3, #4573. Only call garbage collector when all layers have unloaded, rather than one for each layer
		GObjectGC->DisableGC();
		for ( Uint32 i=0; i<allLayers.Size(); i++ )
		{
			CLayerInfo* layer = allLayers[i];
			if ( !layer->IsEnvironment() )
			{
				// Ensure that we reset the streaming mask, as we're unloading.
				layer->SyncUnload();
				layer->ForceVisible( false );
			}
		}

		GObjectGC->EnableGC();
		GObjectGC->CollectNow();	// Force a garbage collect pass when all layers are unloaded

		// Flush world cleanup
		FinishLayerTransitions();

		// All dynamic layers should be unloaded here
		for ( Uint32 i=0; i<allLayers.Size(); i++ )
		{
			CLayerInfo* layer = allLayers[i];
			if ( !layer->IsEnvironment() )
			{
				ASSERT( !layer->IsLoaded() );
			}
		}


		const CClipMap* terrain = m_activeWorld->GetTerrain();
		if ( terrain )
		{
			terrain->InvalidateCollision();
		}
		
		// Uncache layer
		// MattH: TTP Witcher 3, #4573. Only call garbage collector when all layers have unloaded, rather than one for each layer
		GObjectGC->DisableGC();
		for ( Uint32 i=0; i<allLayers.Size(); i++ )
		{
			CLayerInfo* layer = allLayers[i];
			layer->RestoreAfterPIEUnload();
		}
		GObjectGC->EnableGC();
		GObjectGC->CollectNow();	// Force a garbage collect pass when all layers are unloaded

		// Flush world cleanup
		FinishLayerTransitions();
	}
#endif

	// Clear transforms
	if ( m_activeWorld )
	{
		m_activeWorld->GetUpdateTransformManager().ClearSchedule();
	}

	// Close any game systems that relates to active world
	OnGameplaySystemsGameEnd( info );

	// Clear initial facts
	m_initFacts.ClearFast();

	// Unregister from save manager
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );

	// GAME IS NOT NOT ACTIVE
	m_requestEnd = false;
	m_isActive = false;

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	CPathLibWorld* pathlib = m_activeWorld ? m_activeWorld->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		pathlib->SetGameRunning( false );
	}
#endif

	// Restore world - restore shit that should be loaded, it's done after the GGame->IsActive() flag goes to false
#ifndef NO_EDITOR_WORLD_SUPPORT
	if ( info.m_inEditorGame && !info.m_isChangingWorldsInGame )
	{
		ASSERT( GIsEditorGame );

		// Get all layers
		TDynArray< CLayerInfo* > allLayers;
		m_activeWorld->GetWorldLayers()->GetLayers( allLayers, false );

		// Load editor layers
		for ( Uint32 i=0; i<allLayers.Size(); i++ )
		{
			CLayerInfo* info = allLayers[i];
			info->RestoreAfterPIELoad();
		}

		m_activeWorld->OnEditorGameStopped();
	}
#endif

	// Flush world cleanup
	FinishLayerTransitions();

	// End of PIE game
	GIsEditorGame = false;

	// End of game GC
	PUMP_MESSAGES_DURANGO_CERTHACK();
	GObjectGC->CollectNow();
	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Game has ended
	GEngine->OnGameEnded();

	// Remove all streaming textures
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Send the disable all effect command
		( new CRenderCommand_DisableAllGameplayEffects() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();
	}

	// End of game GC
	GObjectGC->CollectNow();

	if ( m_activeWorld )
	{
		m_activeWorld->ForceStreamingUpdate();
	}

	// Allow the profile manager to start processing input again (activate account picker on xbox, establish new active user profile)
	GUserProfileManager->ToggleInputProcessing( true, CUserProfileManager::eAPDR_Ingame );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGame::OnGameplaySystemsGameStart( const CGameInfo& info )
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGame::OnGameplaySystemsWorldStart( const CGameInfo& info )
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGame::OnAfterLoadingScreenGameStart( const CGameInfo& info )
{
	GSoundSystem->OnLoadGame( info.m_gameLoadStream );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGame::OnGameplaySystemsGameEnd( const CGameInfo& info )
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGame::OnGameplaySystemsWorldEnd( const CGameInfo& info )
{

}
