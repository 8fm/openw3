/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "r4Game.h"

#include "../../common/core/configVarSystem.h"
#include "../../common/core/dataError.h"
#include "../../common/core/depot.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/redTelemetryServiceInterface.h"
#include "../../common/core/resourceDefManager.h"
#include "../../common/core/scriptingSystem.h"


#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/debugPageManagerBase.h"
#include "../../common/engine/debugServerManager.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/engine/flashPlayer.h"
#include "../../common/engine/foliageScene.h"
#include "../../common/engine/idTagManager.h"
#include "../../common/engine/occlusionSystem.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/staticCamera.h"


#include "../../common/game/actorsManager.h"
#include "../../common/game/aiPositioning.h"
#include "../../common/game/attitudeManager.h"
#include "../../common/game/communitySystem.h"
#include "../../common/game/createEntityHelper.h"
#include "../../common/game/entitiesDetector.h"
#include "../../common/game/factsDB.h"
#include "../../common/game/gameWorld.h"
#include "../../common/game/gameSaver.h"
#include "../../common/game/gameFastForwardSystem.h"
#include "../../common/game/gameWorld.h"
#include "../../common/game/interactionsManager.h"
#include "../../common/game/factsDB.h"
#include "../../common/game/roadsManager.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/strayActorManager.h"
#include "../../common/game/menu.h"


#include "alchemy.h"
#include "behTreeNodeVitalSpotActive.h"
#include "buffImmunity.h"
#include "cityLightManager.h"
#include "commonMapManager.h"
#include "customCamera.h"
#include "focusModeController.h"
#include "gameplayFXSurfacePost.h"
#include "mainMenuController.h"
#include "mapFastTravel.h"
#include "monsterParam.h"
#include "r4BoidSpecies.h"
#include "r4CreateEntityManager.h"
#include "r4LootDefinitions.h"
#include "r4LootManager.h"
#include "r4GuiManager.h"
#include "r4Kinect.h"
#include "r4QuestSystem.h"
#include "r4JournalManager.h"
#include "r4GameResource.h"
#include "r4GameDebuggerPlugin.h"
#include "r4ReactionManager.h"
#include "r4GlobalEventsScriptsDispatcher.h"
#include "r4BehTreeInstance.h"
#include "r4SwarmUtils.h"
#include "r4Telemetry.h"
#include "r4TelemetryScriptProxy.h"
#include "r4SecondScreenManagerScriptProxy.h"
#include "r4DebugDataService.h"
#include "r4GwintManager.h"
#include "ticketSystemConfiguration.h"
#include "tutorial.h"
#include "volumePathManager.h"
#include "vitalSpot.h"
#include "w2saveImporter.h"
#include "dynamicTagsContainer.h"
#include "r4DLCWorldMounter.h"
#include "r4Enums.h"

#if !defined( NO_SECOND_SCREEN )
#	include "r4SecondScreenManager.h"
#	include "../../common/platformCommon/secondScreenManager.h"
#endif
#include "../../common/core/contentManager.h"

RED_DEFINE_STATIC_NAME( EA_FoundYennefer );
RED_DEFINE_STATIC_NAME( EA_FreedDandelion );
RED_DEFINE_STATIC_NAME( EA_YenGetInfoAboutCiri );
RED_DEFINE_STATIC_NAME( EA_FindBaronsFamily );
RED_DEFINE_STATIC_NAME( EA_FindCiri );
RED_DEFINE_STATIC_NAME( EA_ConvinceGeelsToBetrayEredin );
RED_DEFINE_STATIC_NAME( EA_DefeatEredin );
RED_DEFINE_STATIC_NAME( EA_CompleteWitcherContracts );
RED_DEFINE_STATIC_NAME( EA_CompleteSkelligeRaceForCrown );
RED_DEFINE_STATIC_NAME( EA_CompleteWar );
RED_DEFINE_STATIC_NAME( EA_CompleteKeiraMetz );
RED_DEFINE_STATIC_NAME( EA_GetAllForKaerMorhenBattle );
RED_DEFINE_STATIC_NAME( EA_Brawler );
RED_DEFINE_STATIC_NAME( EA_BrawlMaster );
RED_DEFINE_STATIC_NAME( EA_NeedForSpeed );	
RED_DEFINE_STATIC_NAME( EA_MonsterHuntFogling );	
RED_DEFINE_STATIC_NAME( EA_MonsterHuntEkimma );	
RED_DEFINE_STATIC_NAME( EA_MonsterHuntLamia );	
RED_DEFINE_STATIC_NAME( EA_MonsterHuntFiend );
RED_DEFINE_STATIC_NAME( EA_MonsterHuntDao );
RED_DEFINE_STATIC_NAME( EA_MonsterHuntDoppler );
RED_DEFINE_STATIC_NAME( EA_GwintCollector );
RED_DEFINE_STATIC_NAME( EA_GwintMaster );

IMPLEMENT_RTTI_ENUM( ENewGamePlusStatus )

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

Bool GEnableKinectInGameExe;

CGatheredResource resHorseCameraEnitity( TXT("gameplay\\camera\\horse_camera.w2ent"), RGF_Startup );
CGatheredResource resR4DefaultGameDef( TXT("engine\\default.redgame"), RGF_Startup );

CR4Game* GR4Game = NULL;

CR4ProfilerScriptRegistration CR4Game::s_scriptProfilerCommands;
const String CR4Game::CONSTRAINED_MODE_PAUSE_REASON( TXT( "Constrained" ) );

void CR4GCOverlayHelper::OnGCStarting()
{
	if ( !GGame->IsLoadingScreenShown() && !GGame->IsBlackscreen() && !GGame->IsAnyMenu() )
	{
		// pop up the blur/spinner from here
		m_isBlurred = true;
		( new CRenderCommand_ShowLoadingScreenBlur( 4.f, 10.f ) )->Commit();
	}
}

void CR4GCOverlayHelper::OnGCFinished()
{
	if ( m_isBlurred )
	{
		// take everything off as fast as we can, we are done
		( new CRenderCommand_HideLoadingScreenBlur( 0.f ) )->Commit();
		m_isBlurred = false;
	}
}

void InitializeR4GameSystems( CCommonGame *game )
{
	// FIXME: this should be done through the R4 game somehow
	game->m_gameSystems.Resize( GSR4_MAX );
	
	game->AddSystem( CreateObject< CInteractionsManager >( game ) );
	game->AddSystem( CreateObject< CR4QuestSystem >( game ) );
	game->AddSystem( CreateObject< CStorySceneSystem >( game ) );
	game->AddSystem( CreateObject< CFactsDB >( game ) );
	game->AddSystem( CreateObject< CCommunitySystem >( game ) );
	game->AddSystem( CreateObject< CAttitudeManager >( game ) );
	game->AddSystem( CreateObject< CWitcherJournalManager >( game ) );
	game->AddSystem( CreateObject< CEntitiesDetector >( game ) );
	game->AddSystem( CreateObject< CFocusModeController >( game ) );
	game->AddSystem( CreateObject< CCommonMapManager >( game ) );
	game->AddSystem( CreateObject< CVolumePathManager >( game ) );
	game->AddSystem( CreateObject< CR4LootManager >( game ) );
	game->AddSystem( CreateObject< CCityLightManager >( game ) );
	game->AddSystem( CreateObject< CR4GwintManager >( game ) );
	game->AddSystem( CreateObject< CRoadsManager >( game ) );
	game->AddSystem( CreateObject< CGameFastForwardSystem >( game ) );
	game->AddSystem( CreateObject< CStrayActorManager >( game ) );
#if !defined( NO_TELEMETRY )
	game->AddSystem( CreateObject< CR4Telemetry >( game ) );
#endif
#if !defined( NO_SECOND_SCREEN )
	game->AddSystem( CreateObject< CR4SecondScreenManager >( game ) );
#endif
	game->AddSystem( CreateObject< CGameplayFXSurfacePost >( game ) );
	game->AddSystem( CreateObject< CR4TutorialSystem >( game ) );
	game->OnGameplaySystemsInitialize();

	// debug server plugins
	DBGSRV_REG_PLUGIN( new CR4GameDebuggerPlugin() );
}

void ShutdownR4GameSystems( CCommonGame *game )
{
	game->OnGameplaySystemsShutdown();
}

IMPLEMENT_ENGINE_CLASS( CR4Game );

RED_DEFINE_STATIC_NAME( FilterVitalSpots );
RED_DEFINE_STATIC_NAME( CommonIngameMenu )

CR4Game::CR4Game()
	: CCommonGame()
	, m_ticketsDefaultConfiguration( NULL )
	, m_globalTicketSource( NULL )
	, m_horseCamera( NULL )
	, m_telemetryScriptProxy( NULL )
	, m_dynamicTagsContainer( nullptr )
#if !defined( NO_TELEMETRY )
	, m_accumulatedTime( 0.0f )
	, m_globalEventsScriptsDispatcher( nullptr )
#endif // NO_TELEMETRY

#if defined(RED_KINECT)
	, m_kinectSpeechRecognizerListener( nullptr )
#endif
	, m_kinectSpeechRecognizerListenerScriptProxy( nullptr )
	, m_mainMenuController( nullptr )
	, m_constrainedModeBecauseOfASystemMessage( false )
	, m_gcOverlayHelper( nullptr )
	, m_worldDLCExtender( nullptr )
{
}

void CR4Game::Init()
{
	TBaseClass::Init();

	// the global keyword for scripts must be present, despite disabling the telemetry
	m_telemetryScriptProxy = CreateObject< CR4TelemetryScriptProxy >( this );
	ASSERT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_TELEMETRY ) == NULL, TXT(  "About to replace an existing CTelemetry in script globals. Data might be lost!" ) );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_TELEMETRY, m_telemetryScriptProxy );

	m_secondScreenScriptProxy = CreateObject< CR4SecondScreenManagerScriptProxy >( this );

#if !defined( NO_SECOND_SCREEN )	
	SCSecondScreenManager::GetInstance().SetDelegate( m_secondScreenScriptProxy );
#endif //! NO_SECOND_SCREEN

	m_globalEventsScriptsDispatcher = CreateObject< CR4GlobalEventsScriptsDispatcher >( this );

	m_worldDLCExtender = CreateObject< CR4WorldDLCExtender >( this );

	// Register game in scripting system
	ASSERT( GScriptingSystem );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_GAME, this );

	InitializeR4GameSystems( this );

#if !defined( NO_TELEMETRY )	
	// here telemetry should be created
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
	{
		telemetrySystem->LoadConfigs();
		telemetrySystem->StartCollecting();
		telemetrySystem->Log( TE_SYS_GAME_LAUNCHED );
	}
#if !defined( NO_DEBUG_DATA_SERVICE )
	SDebugDataService::GetInstance().LoadConfig( TXT( "debug_data_service_config" ) );
	m_debugDataLogger.Initialise( &SDebugDataService::GetInstance() );
	SDebugDataService::GetInstance().StartCollecting();
	SDebugDataService::GetInstance().StartSession( TXT( "" ) );
#endif	// NO_DEBUG_DATA_SERVICE 	
#endif // NO_TELEMETRY

	SetupGameResource();

	// Create boid species definition Manager
	m_boidSpecies = new CR4BoidSpecies();
	m_boidSpecies->InitParams(m_definitionsManager);

	CR4SwarmScriptRegistration::s_r4SwarmScriptRegistration.PreventOptimisedAway();

	m_guiManager = CreateObject< CR4GuiManager >( this ); // NOTE: GC is handled in CCommonGame OnSerialize	
	m_guiManager->Initialize();

	m_ticketsDefaultConfiguration = CreateObject< CTicketsDefaultConfiguration >( this );
	m_ticketsDefaultConfiguration->Initialize();

	m_globalTicketSource = CreateObject< CGlabalTicketSourceProvider >( this );
	m_globalTicketSource.Get()->Initialize();

	m_carryableItemsRegistry = CreateObject< CCarryableItemsRegistry >( this );
	m_carryableItemsRegistry.Get()->Initialize();

	m_mainMenuController = new CMainMenuController();
	m_dynamicTagsContainer = new CDynamicTagsContainer();
	m_dynamicTagsContainer->LoadDynamicTags();

	m_params = CreateObject< W3GameParams >( this );

	#ifndef NO_SAVE_IMPORT
		GUserProfileManager->SetImporter( &createSaveImporter );
	#endif
	m_kinectSpeechRecognizerListenerScriptProxy = CreateObject< CR4KinectSpeechRecognizerListenerScriptProxy >( this );
#if defined(RED_KINECT)
	m_kinectSpeechRecognizerListener = new CR4KinectSpeechRecognizerListener( m_kinectSpeechRecognizerListenerScriptProxy );
#endif

	m_gcOverlayHelper = new CR4GCOverlayHelper();
	GObjectGC->RegisterHelper( m_gcOverlayHelper );

#ifdef NO_EDITOR
	SRTTI::GetInstance().CreateDefaultObjects();
#endif
}

void CR4Game::InitReactionManager()
{
	m_behTreeReactionManager = CreateObject< CR4ReactionManager >( this );
	m_behTreeReactionManager->Init();
}

CCreateEntityManager *const CR4Game::InstanciateCreateEntityManager()
{
	return new CR4CreateEntityManager();
}

void CR4Game::ShutDown()
{
	if ( m_activeWorld )
	{
		UnloadWorld();
	}

	if ( m_gcOverlayHelper )
	{
		GObjectGC->UnregisterHelper(m_gcOverlayHelper);
		delete m_gcOverlayHelper;
		m_gcOverlayHelper = nullptr;
	}

#if defined(RED_KINECT)
	delete m_kinectSpeechRecognizerListener;
	m_kinectSpeechRecognizerListener = nullptr;
#endif
	if( m_kinectSpeechRecognizerListenerScriptProxy )
	{
		m_kinectSpeechRecognizerListenerScriptProxy->Discard();
		m_kinectSpeechRecognizerListenerScriptProxy = NULL;
	}

	m_ticketsDefaultConfiguration = NULL;
	if( m_globalTicketSource.Get() )
	{
		m_globalTicketSource.Get()->ClearTickets();
	}

	delete m_mainMenuController;
	m_mainMenuController = nullptr;

	delete m_dynamicTagsContainer;
	m_dynamicTagsContainer = nullptr;

	if ( m_guiManager )
	{
		m_guiManager->Deinitialize();
		m_guiManager->Discard();
		m_guiManager = NULL;
	}

	// Delete boid parameters
	if( m_boidSpecies )
	{
		delete m_boidSpecies;
		m_boidSpecies = NULL;
	}

	// Unregister game in scripting system
	ASSERT( GScriptingSystem );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_GAME, NULL );

	ShutdownR4GameSystems( this );


	if( m_worldDLCExtender )
	{
		m_worldDLCExtender->Discard();
		m_worldDLCExtender = NULL;
	}

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->StopCollecting();
	#if !defined( NO_DEBUG_DATA_SERVICE )
		SDebugDataService::GetInstance().StopSession();
		SDebugDataService::GetInstance().StopCollecting();
	#endif
#endif
	m_globalEventsScriptsDispatcher = nullptr;

	if( m_secondScreenScriptProxy )
	{
		m_secondScreenScriptProxy->Discard();
		m_secondScreenScriptProxy = NULL;
	}

	if( m_telemetryScriptProxy )
	{
		ASSERT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_TELEMETRY ) == m_telemetryScriptProxy, TXT( "About to NULL a CTelemetry which isn't the same as in script globals" ) );
		GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_TELEMETRY, NULL );

		m_telemetryScriptProxy->Discard();
		m_telemetryScriptProxy = NULL;
	}

	TBaseClass::ShutDown();
}

void CR4Game::OnAttachGameplayEntity( CGameplayEntity* entity )
{
	TBaseClass::OnAttachGameplayEntity( entity );
}

void CR4Game::OnDetachGameplayEntity( CGameplayEntity* entity )
{
	TBaseClass::OnDetachGameplayEntity( entity );
}

void CR4Game::Tick( Float deltaTime )
{
	TBaseClass::Tick( deltaTime );

	{
		PC_SCOPE( R4GameCallTick )
		CallFunction( this, CNAME( OnTick ) );
	}

	PauseIfNeeded();

#if !defined( NO_TELEMETRY )
	{
		PC_SCOPE( Telemetry );
		m_accumulatedTime += deltaTime;
		CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
		if ( telemetrySystem )
			telemetrySystem->SetStatValue( CS_GAME_TIME, m_accumulatedTime );
		SRedTelemetryServicesManager::GetInstance().Update();
	}
#endif // NO_TELEMETRY
#if !defined(NO_SECOND_SCREEN) 
	{
		PC_SCOPE( SecondScreen );
		if( SCSecondScreenManager::GetInstance().GetState() == CSecondScreenManager::GS_NONE )
		{			
			if( m_mainMenuController && m_mainMenuController->GetCurrentMenuState() != GS_None )
			{
				SCSecondScreenManager::GetInstance().SendState( CSecondScreenManager::GS_MAIN_MENU );
			}		
		}
		SCSecondScreenManager::GetInstance().Update( deltaTime );
	}
#endif
#if defined(RED_KINECT)
	m_kinectSpeechRecognizerListener->Update();
#endif
}

void CR4Game::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		file << m_gameSystems;
	}
}

void CR4Game::OnUserEvent( const EUserEvent& event )
{
	TBaseClass::OnUserEvent(event);

	switch( event )
	{
	case EUserEvent::UE_LoadSaveReady:
		{
			if( m_mainMenuController )
			{
				m_mainMenuController->OnUserProfileReady();
			}

#if defined(RED_KINECT)
			m_kinectSpeechRecognizerListener->RefreshSettings();
#endif
		}
		break;

	case EUserEvent::UE_SignedOut:
		if( m_mainMenuController )
		{
			m_mainMenuController->OnProfileSignedOut();
		}

		if( !m_isInStartGame && m_isActive && !IsLoadingScreenShown() )
		{
			if( GGame->GetActiveWorld() )
			{
				CGameSaver* gameSaver = GetGameSaver();
				if( gameSaver )
				{
					gameSaver->SaveGameSync( SGT_AutoSave, -1, TXT("EUserEvent::UE_SignedOut") );
				}
			}
		}

		break;
	}
}

void CR4Game::PauseIfNeeded()
{
	if ( GEngine->GetState() != BES_Constrained )
	{
		return;
	}

	/*struct BoolTo { static const Char* Text( Bool b ) { return b ? TXT("true") : TXT("false"); } };
	RED_LOG( Constrained, TXT("anymenu: %ls, isactive: %ls, isblackscreen: %ls, isloadingscreen: %ls, blackscreenreq: %ls, cachet: %ls, cc: %ls, mmc: %d, sysmgs: %ls\n"), 
		BoolTo::Text( IsAnyMenu() ),
		BoolTo::Text( IsActive() ),
		BoolTo::Text( IsBlackscreen() ),
		BoolTo::Text( IsLoadingScreenShown() ),
		BoolTo::Text( HasBlackscreenRequested() ), // vs fade because we shouldn't wait for fade-in
		BoolTo::Text( IsPlayingCachetDialog() ),
		BoolTo::Text( IsPlayingCameraCutscene() ),
		Uint32( ( nullptr == m_mainMenuController ) ? GS_None : m_mainMenuController->GetCurrentMenuState() ),
		BoolTo::Text( m_constrainedModeBecauseOfASystemMessage ) );	   */

	const Bool shouldRequestPauseMenu =
			!m_constrainedModeBecauseOfASystemMessage
		&&	!IsAnyMenu()
		&&	IsActive()
		&&	!IsBlackscreen()
		&&  !HasBlackscreenRequested()
		&&	!IsLoadingScreenShown()
		&&	( !IsPlayingCachetDialog() 
					&&	!IsPlayingCameraCutscene() 
					&&	m_mainMenuController != nullptr 
					&&	m_mainMenuController->GetCurrentMenuState() == GS_Playing );

	if ( shouldRequestPauseMenu )
	{
		THandle< IScriptable > initData;
		RequestMenu( CNAME( CommonIngameMenu ), initData );
	}
	else if ( IsActive() && !IsPaused() && !IsLoadingScreenShown() )
	{
		// Might have been a loading screen right up to a cutscene, so we should pause the cutscene when coming out of it
		Pause( CONSTRAINED_MODE_PAUSE_REASON );
		//RED_LOG( Constrained, TXT("Pausing game.") );
	}
}

void CR4Game::EnteredConstrainedMode()
{
	TBaseClass::EnteredConstrainedMode();
	//RED_LOG( Constrained, TXT("Entering constrained mode, loading screen is: %ls."), IsLoadingScreenShown() ? TXT("shown") : TXT("hidden") );

	m_constrainedModeBecauseOfASystemMessage = GUserProfileManager ? GUserProfileManager->IsDisplayingSystemMessage() : false;
	PauseIfNeeded();

	if ( GetGuiManager() )
	{
		GetGuiManager()->OnEnteredConstrainedState();
	}
}


void CR4Game::ExitedConstrainedMode()
{
	//RED_LOG( Constrained, TXT("Exiting constrained mode, loading screen is: %ls."), IsLoadingScreenShown() ? TXT("shown") : TXT("hidden") );

	if ( IsPaused( CONSTRAINED_MODE_PAUSE_REASON ) )
	{
		Unpause( CONSTRAINED_MODE_PAUSE_REASON );
		//RED_LOG( Constrained, TXT("Unpausing game.") );
	}

	if ( GetGuiManager() )
	{
		GetGuiManager()->OnExitedConstrainedState();
	}

	m_constrainedModeBecauseOfASystemMessage = false;

	TBaseClass::ExitedConstrainedMode();
}


void CR4Game::RequireLocalizationChange()
{
	if ( m_mainMenuController )
	{
		m_mainMenuController->RequireLocalizationChange();
	}
}

RED_DEFINE_STATIC_NAME( CR4CommonMenu )

Bool CR4Game::IsInMainMenu() const
{
	return m_guiManager && m_guiManager->GetRootMenu() && m_guiManager->GetRootMenu()->GetClass()->GetName() == CNAME( CR4CommonMenu );
}

Bool CR4Game::IsInIngameMenu() const
{
	return m_guiManager && m_guiManager->GetRootMenu() && m_guiManager->GetRootMenu()->GetMenuName() == CNAME( CommonIngameMenu );
}

Bool CR4Game::SetupGameResourceFromFile( const String& filePath )
{
	if ( filePath.Empty() == false )
	{
		LOG_R4( TXT( "Setup Game Resource: %" ) RED_PRIWs , filePath.AsChar() );
		THandle< CWitcherGameResource > newResource = LoadResource< CWitcherGameResource >( filePath );
		if ( newResource )
		{
			m_gameResource = Cast< CGameResource >( newResource );
			return true;
		}
	}

	ASSERT( m_gameResource.IsValid() );
	return false;
}

void CR4Game::SetupGameResource()
{
	ASSERT( IsActive() == false );

#if 0 // this is criminally slow
	TDynArray< String > mainResourcePaths;
	GDepot->FindResourcesByExtension( CWitcherGameResource::GetFileExtension(), mainResourcePaths );

	if ( mainResourcePaths.Empty() == true )
	{
		ERR_R4( TXT( "Game main definitions resource was not found!!!" ) );
	}
	if ( mainResourcePaths.Size() > 1 && GIsEditor == false )
	{
		WARN_R4( TXT( "More than one game definition resource found!! Using the first one" ) );
	}
	SetupGameResourceFromFile( mainResourcePaths[ 0 ] );
#endif

	SetupGameResourceFromFile( resR4DefaultGameDef.GetPath().ToString() );
}
																										 
void CR4Game::OnLoadingFailed( ESessionRestoreResult sres, const TDynArray< CName >& missingContent )
{
	if ( m_mainMenuController )
	{
		m_mainMenuController->OnLoadingFailed();
	}

	if ( m_guiManager )
	{
		CallFunction( m_guiManager, CNAME( OnLoadingFailed ), sres, missingContent );
	}
}

ENewGamePlusStatus CR4Game::StartNewGamePlus( const SSavegameInfo& info )
{
	// disable NGP in patch 1.07, enable it from 1.08
	if ( SAVE_GAME_VERSION <= GAME_VERSION_WITCHER_3_PATCH_1_07 )
	{
		return NGP_WrongGameVersion;
	}
	else
	{
		if ( m_startingNewGamePlus )
		{
			m_startingNewGamePlus = false;
			delete m_newGamePlusPlayerLoader;
			m_newGamePlusPlayerLoader = nullptr;
		}

		if ( false == info.IsValid() ) 				    
		{
			RED_LOG( Save, TXT("New game plus: '%ls' is invalid."), info.GetFileName().AsChar() );
			return NGP_Invalid;
		}

		ELoadGameResult ret = GUserProfileManager->InitGameLoading( info );
		if ( ret != ELoadGameResult::LOAD_Initializing && ret != ELoadGameResult::LOAD_ReadyToLoad )
		{
			GUserProfileManager->CancelGameLoading();
			RED_LOG( Save, TXT("New game plus: cannot initialize loading of '%ls'"), info.GetFileName().AsChar() );
			return NGP_CantLoad;
		}

		Int32 timeout = 200;
		while ( timeout > 0 )
		{
			GUserProfileManager->Update();

			ret = GUserProfileManager->GetLoadGameProgress();
			if ( ret != ELoadGameResult::LOAD_Initializing )
			{
				break;
			}

			Red::Threads::SleepOnCurrentThread( 100 );
			--timeout;
		}

		if ( timeout == 0 || ret != ELoadGameResult::LOAD_ReadyToLoad )
		{
			GUserProfileManager->CancelGameLoading();
			RED_LOG( Save, TXT("New game plus: cannot initialize loading of '%ls'"), info.GetFileName().AsChar() );
			return NGP_CantLoad;
		}

		ELoaderCreationResult res;
		IGameLoader* playerLoader = SGameSaveManager::GetInstance().CreateLoader( info, res );
		if ( !playerLoader )
		{
			GUserProfileManager->CancelGameLoading();
			RED_LOG( Save, TXT("New game plus: cannot create loader for '%ls'"), info.GetFileName().AsChar() );
			return NGP_CantLoad;
		}

		struct Guard
		{
			IGameLoader* m_loader;
			Guard( IGameLoader* loader ) : m_loader( loader ) {}
			~Guard() { if (m_loader) { delete m_loader; } };
		} guard( playerLoader );

		if ( playerLoader->GetSaveVersion() < SAVE_VERSION_NEW_GAME_PLUS )
		{
			GUserProfileManager->CancelGameLoading();
			RED_LOG( Save, TXT("New game plus: save '%ls' is too old, please resave"), info.GetFileName().AsChar() );
			return NGP_TooOld;
		}

		TDynArray< CName > missingContent;
		if ( false == LoadRequiredDLCs( playerLoader, missingContent ) )
		{
			GUserProfileManager->CancelGameLoading();
			OnLoadingFailed( RESTORE_DLCRequired, missingContent );

			RED_LOG( Save, TXT("New game plus: save '%ls' is requiring some dlcs."), info.GetFileName().AsChar() );
			return NGP_CantLoad;
		}

		{
			CGameSaverBlock block( playerLoader, CNAME( NewGamePlus ) );

			Bool enable = false, active = false;
			playerLoader->ReadValue( CNAME( Enable ), enable );
			playerLoader->ReadValue( CNAME( active ), active );

			if ( false == enable || true == active )
			{
				GUserProfileManager->CancelGameLoading();
				RED_LOG( Save, TXT("New game plus: save '%ls' is made before the player met the requirements for ng+ (finish game, etc...)"), info.GetFileName().AsChar() );
				return NGP_RequirementsNotMet;
			}
		}

		m_startingNewGamePlus = true;
		m_newGamePlusPlayerLoader = playerLoader;
		guard.m_loader = nullptr; // Need to clear the guard or else the loader will be destroyed when we no longer want it to be
		m_gameTime = EngineTime::ZERO;

		SetGameResourceFilenameToStart( TXT("game/witcher3.redgame") );

		// can't start new game plus from ng+ session
		GetGameSaver()->EnableNewGamePlus( false );

		return NGP_Success;
	}
}

Bool CR4Game::OnSaveGame( IGameSaver* saver )
{
	Bool isNewGamePlus = IsNewGamePlus();

	// added for extra security
	if ( isNewGamePlus )
	{
		// can't start new game plus from ng+ session
		GetGameSaver()->EnableNewGamePlus( false );
	}

	{
		CGameSaverBlock block( saver, CNAME( NewGamePlus ) );
		saver->WriteValue( CNAME( Enable ), GetGameSaver()->IsNewGamePlusEnabled() );
		saver->WriteValue( CNAME( active ), isNewGamePlus );
	}

	return TBaseClass::OnSaveGame( saver );
}

void CR4Game::OnGameplaySystemsWorldStart( const CGameInfo& info )
{
	// Pass to base class
	TBaseClass::OnGameplaySystemsWorldStart( info );

	CEntityTemplate* horseCamTempl = resHorseCameraEnitity.LoadAndGet<CEntityTemplate>();
	if ( horseCamTempl )
	{
		EntitySpawnInfo einfo;
		einfo.m_detachTemplate = false;
		einfo.m_template = horseCamTempl;

		CCustomCamera* horseCamera = SafeCast<CCustomCamera>( m_activeWorld->GetDynamicLayer()->CreateEntitySync( einfo ) );
		if ( horseCamera )
		{
			horseCamera->SetResetScheduled( false );

			// Add tag - for behavior debugger
			TagList tags = horseCamera->GetTags();
			tags.AddTag( CNAME( CAMERA ) );
			horseCamera->SetTags(tags);

			m_horseCamera = horseCamera;
		}
		else
		{
			WARN_R4( TXT("World: Couldn't create horse camera") );
		}
	}
	else
	{
		WARN_R4( TXT("Game: Couldn't create horse camera - no %s resource"), resHorseCameraEnitity.GetPath().ToString().AsChar() );
	}

	// Set up tree hiding...
	// TODO : I've put this here just to illustrate how to do it. I leave it up to the gameplay folks to do it as they see best.
	{
		( new CRenderCommand_SetupTreeFading( true ) )->Commit();
	}

#if defined(RED_KINECT)
	//! can be removed after add option Kinect on/off to game settings
	if( GEnableKinectInGameExe )
	{
		m_kinectSpeechRecognizerListener->SetEnabled( true );
	}	
	m_kinectSpeechRecognizerListener->Start();
#endif
}

void CR4Game::OnGameplaySystemsWorldEnd( const CGameInfo& info )
{
#if defined(RED_KINECT)
	m_kinectSpeechRecognizerListener->Stop();
#endif

	// Turn off tree hiding
	// TODO : I've put this here just to illustrate how to do it. I leave it up to the gameplay folks to do it as they see best.
	( new CRenderCommand_SetupTreeFading( false ) )->Commit();

	TBaseClass::OnGameplaySystemsWorldEnd( info );
}

void CR4Game::OnGameplaySystemsGameStart( const CGameInfo& info )
{
	TBaseClass::OnGameplaySystemsGameStart( info );
}

void CR4Game::OnGameStart( const CGameInfo& info )
{
	if ( false == info.m_isChangingWorldsInGame )
	{
		Bool newGamePlusActive = false;
		if ( info.m_gameLoadStream )
		{
			Bool ngp = false;
			if ( info.m_gameLoadStream->GetSaveVersion() >= SAVE_VERSION_NEW_GAME_PLUS )
			{
				CGameSaverBlock block( info.m_gameLoadStream, CNAME( NewGamePlus ) );
				ngp = info.m_gameLoadStream->ReadValue( CNAME( Enable ), false );
				newGamePlusActive = info.m_gameLoadStream->ReadValue( CNAME( active ), false );
			}

			GetGameSaver()->EnableNewGamePlus( ngp );

			if ( newGamePlusActive )
			{
				// make sure we have the initial fact, it's needed for checking ngp before we load facts db
				m_initFacts.PushBackUnique( SInitialFact( TXT("NewGamePlus"), 1 ) );
			}
		}
		else
		{
			// starting new game, initial fact should be set if this is ngp
			for ( const auto& fact : m_initFacts )
			{
				if ( fact.m_name.EqualsNC( TXT("NewGamePlus") ) && fact.m_value != 0 )
				{
					newGamePlusActive = true;
					break;
				}
			}
		}

		GetDefinitionsManager()->SetAlternativePathsMode( newGamePlusActive );
	}

	TBaseClass::OnGameStart( info );
}

void CR4Game::UnlockMissedAchievements( const CGameInfo& info )
{

#ifdef RED_PLATFORM_CONSOLE
	return;
#endif // RED_PLATFORM_CONSOLE

	if ( info.m_isChangingWorldsInGame )
		return;

	if ( !info.m_gameLoadStream )
		return;

	if ( ! ( SGameSessionManager::GetInstance().ContainsHistoryEventFromVersion( SAVE_VERSION_KOSHER_CHECK ) || SGameSessionManager::GetInstance().ContainsHistoryEventFromVersion( SAVE_VERSION_KOSHER_CHECK_PART_DEUX ) ) )

		return;

	CFactsDB* factsDB = GetSystem< CFactsDB >();
	if ( !factsDB )
		return;

	if ( factsDB->DoesExist( TXT( "achievement_fix_executed" ) ) && factsDB->DoesExist( TXT( "achievement_fix_executed2" ) ) && factsDB->DoesExist( TXT( "achievement_fix_executed3" ) ) )
		return;

	if ( !GUserProfileManager )
		return;

	TryUnlockAchievement( factsDB->DoesExist( TXT( "q002_talked_to_yennefer" ) ), CNAME( EA_FoundYennefer ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q305_completed" ) ), CNAME( EA_FreedDandelion ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q205_completed" ) ), CNAME( EA_YenGetInfoAboutCiri ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q103_nml_part1_ciri" ) ), CNAME( EA_FindBaronsFamily ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q402_found_ciri" ) ), CNAME( EA_FindCiri ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q311_completed" ) ), CNAME( EA_ConvinceGeelsToBetrayEredin ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q501_eredin_died" ) ), CNAME( EA_DefeatEredin ) );

	TryUnlockAchievement( factsDB->DoesExist( TXT( "mh101_completed") ) &&
						  factsDB->DoesExist( TXT( "mh102_completed") ) &&
						  factsDB->DoesExist( TXT( "mh103_completed") ) &&
						  factsDB->DoesExist( TXT( "mh104_completed") ) &&
						  factsDB->DoesExist( TXT( "mh105_done") ) &&
						  factsDB->DoesExist( TXT( "mh106_completed") ) &&
						  factsDB->DoesExist( TXT( "mh107_completed") ) &&
						  factsDB->DoesExist( TXT( "mh108_done") ) &&
						  factsDB->DoesExist( TXT( "mh202_done") ) &&
						  factsDB->DoesExist( TXT( "mh203_done") ) &&
						  factsDB->DoesExist( TXT( "mh206_completed") ) &&
						  factsDB->DoesExist( TXT( "mh207_done") ) &&
						  factsDB->DoesExist( TXT( "mh208_done") ) &&
						  factsDB->DoesExist( TXT( "mh210_done") ) &&
						  factsDB->DoesExist( TXT( "mh301_completed") ) &&
						  factsDB->DoesExist( TXT( "mh302_completed") ) &&
						  factsDB->DoesExist( TXT( "mh303_done") ) &&
						  factsDB->DoesExist( TXT( "mh304_done") ) &&
						  factsDB->DoesExist( TXT( "mh305_done") ) &&
						  factsDB->DoesExist( TXT( "mh306_done") ) &&
						  factsDB->DoesExist( TXT( "mh307_completed") ) &&
						  factsDB->DoesExist( TXT( "mh308_done") ) &&
						  factsDB->DoesExist( TXT( "mq1051_fact_completed_mh") ), CNAME( EA_CompleteWitcherContracts ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q208_completed" ) ), CNAME( EA_CompleteSkelligeRaceForCrown ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "mq3035_radovid_dead" ) ), CNAME( EA_CompleteWar ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q109_keira_to_km" ) ) || 
						  factsDB->DoesExist( TXT( "q109_keira_to_radovid" ) ) || 
						  factsDB->DoesExist( TXT( "q109_keira_defeated" ) ), CNAME( EA_CompleteKeiraMetz ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "q402_allies_gathered" ) ), CNAME( EA_GetAllForKaerMorhenBattle ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "ff204_won" ) ), CNAME( EA_Brawler ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "ff_troll_beaten" ) ), CNAME( EA_BrawlMaster ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "hr101_1_won" ) ) &&
						  factsDB->DoesExist( TXT( "hr101_2_won" ) ) &&
						  factsDB->DoesExist( TXT( "hr101_3_won" ) ) &&
						  factsDB->DoesExist( TXT( "hr201_won" ) ) &&
						  factsDB->DoesExist( TXT( "hr202_won" ) ) &&
						  factsDB->DoesExist( TXT( "hr203_won" ) ) &&
						  factsDB->DoesExist( TXT( "hr204_won" ) ) &&
						  factsDB->DoesExist( TXT( "mq3026_won_night_race" ) ), CNAME( EA_NeedForSpeed ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "mh101_completed" ) ), CNAME( EA_MonsterHuntFogling ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "mh104_achievement_given" ) ), CNAME( EA_MonsterHuntEkimma ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "sq204_leshy_fight_started" ) ) && 
						  factsDB->DoesExist( TXT( "sq204_completed" ) ), CNAME( EA_MonsterHuntLamia ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "mh206_completed" ) ), CNAME( EA_MonsterHuntFiend ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "mh306_done" ) ), CNAME( EA_MonsterHuntDao ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "mh305_done" ) ), CNAME( EA_MonsterHuntDoppler ) );
	TryUnlockAchievement( factsDB->DoesExist( TXT( "gwint_all_cards_collected" ) ), CNAME( EA_GwintCollector ) );	

	Bool basicGwintMasterCondition = factsDB->DoesExist( TXT( "cg_nml_done" ) ) && 
									 factsDB->DoesExist( TXT( "cg_innkeepers_done" ) ) && 
									 factsDB->DoesExist( TXT( "cg_novigrad_done" ) ) &&
									 factsDB->DoesExist( TXT( "cg_skellige_done" ) ) &&
									 factsDB->DoesExist( TXT( "sq306_defeated_tybalt" ) );
	Bool basicGwintMasterCondition2 = factsDB->DoesExist( TXT( "cg_old_friends_done" ) ) && 
									 !factsDB->DoesExist( TXT( "cg300_talar_omitted" ) );
	Bool basicGwintMasterCondition3 = factsDB->DoesExist( TXT( "cg300_talar_omitted" ) ) &&
									  factsDB->DoesExist( TXT( "cg_old_friends_done" ) ) &&
									  factsDB->DoesExist( TXT( "cg_talar_standalone_done" ) );
	TryUnlockAchievement( basicGwintMasterCondition && ( basicGwintMasterCondition2 || basicGwintMasterCondition3 ), CNAME( EA_GwintMaster ) );

	// Inform scripts
	CallFunction( this, CNAME( UnlockMissedAchievements ) );

	factsDB->AddFact( TXT( "achievement_fix_executed" ), 1, GGame->GetEngineTime(), CFactsDB::EXP_NEVER_SEC );
	factsDB->AddFact( TXT( "achievement_fix_executed2" ), 1, GGame->GetEngineTime(), CFactsDB::EXP_NEVER_SEC );
	factsDB->AddFact( TXT( "achievement_fix_executed3" ), 1, GGame->GetEngineTime(), CFactsDB::EXP_NEVER_SEC );
}

void CR4Game::TryUnlockAchievement( Bool shouldUnlock, CName achievementName )
{
	if ( !shouldUnlock )
		return;

	GUserProfileManager->UnlockAchievement( achievementName );
}

void CR4Game::OnUsedFastTravelEvent( const SUsedFastTravelEvent& event )
{
	CR4QuestSystem* questSystem = GetSystem< CR4QuestSystem >();
	if ( questSystem )
	{
#if !defined( NO_TELEMETRY )
		CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
		if ( telemetrySystem )
			telemetrySystem->Log( TE_HERO_FAST_TRAVEL );
#endif
		questSystem->OnUsedFastTravelEvent( event );
	}
}

CClass* CR4Game::CBehTreeInstanceClass()
{
	return CR4BehTreeInstance::GetStaticClass();
}

CLootDefinitions* CR4Game::CreateLootDefinitions()
{
	return new CR4LootDefinitions();
}

ILootManager* CR4Game::GetLootManager()
{
	return GetSystem< CR4LootManager >();
}

CInteractionsManager* CR4Game::GetInteractionsManager()
{
	return GetSystem< CInteractionsManager >();
}

CEntity* CR4Game::CreatePlayer( const CGameInfo& info )
{
	CEntity* player = TBaseClass::CreatePlayer( info );
	if ( player != NULL )
	{
		TagList playerTags = player->GetTags();
		playerTags.AddTag( CNAME( GERALT ) );
		player->SetTags( playerTags );
	}
	return player;
}

Bool CR4Game::IsPrimaryPlayer( CPlayer* player ) const
{
	return player->GetIdTag() == GCommonGame->GetIdTagManager()->GetReservedId( RESERVED_TAG_ID_INDEX_GERALT );
}

void CR4Game::OnPreChangePlayer( EntitySpawnInfo& einfo )
{
	// Ciri is our only replacer so hardcoding is good enough... right?
	if( m_playerChangeInfo.m_alias == TXT("ciri") )
	{
		einfo.m_idTag = m_idTagManager->GetReservedId( RESERVED_TAG_ID_INDEX_CIRI );

		// capture Geralt's managed attachments (for example: horse_manager)
		m_universeStorage.CapturePlayerManagedAttachments( GGame->GetPlayerEntity() );
		m_universeStorage.DisablePlayerAttachments();
	}
}

void CR4Game::OnPostChangePlayer()
{
	CallFunction( this, CNAME( OnPlayerChanged ) );
	if ( IsPrimaryPlayer( m_player ) )
	{
		m_universeStorage.EnablePlayerAttachments();
		// restore Geralt's managed attachments (for example: horse_manager)
		m_universeStorage.RestorePlayerManagedAttachments( m_player );
	}

	CCommonMapManager* manager = GetSystem< CCommonMapManager >();
	if ( manager )
	{
		manager->OnPlayerChanged();
	}
}

CEntity* CR4Game::CreateCamera() const
{
	CEntity* camera = TBaseClass::CreateCamera();

	// Activate game camera
	CCustomCamera* customCam = SafeCast< CCustomCamera >( camera );
	if( customCam )
	{
		m_activeWorld->GetCameraDirector()->ActivateCamera( customCam, customCam );
	}
	else
	{
		WARN_R4( TXT("Failed to activate game camera. Check gamedef to make sure using CCustomCamera and not CCamera entity") );
	}

	return camera;
}

Bool CR4Game::ActivateGameCamera( Float blendTime, Bool useFocusTarget, Bool force, Bool reset ) const
{
	CCustomCamera* gameCam = GetCamera();
	if ( !gameCam || gameCam->IsDestroyed() )
	{
		return true;
	}

	if ( m_activeWorld )
	{
		if ( !force )
		{
			// Don't activate game camera if it's already active; this prevents undesired e.g. horse camera switching to default camera and vice versa

			const ICamera* currentCamera = m_activeWorld->GetCameraDirector()->GetTopmostCamera();
			if ( currentCamera == gameCam || currentCamera == m_horseCamera.Get() )
			{
				return true;
			}
		}

		m_activeWorld->GetCameraDirector()->ActivateCamera( gameCam, gameCam, blendTime, useFocusTarget, reset );
		return true;
	}

	return true;
}

void CR4Game::ResetGameplayCamera()
{
	CCustomCamera* gameCam = GetCamera();
	if ( !gameCam || gameCam->IsDestroyed() )
	{
		return;
	}

	if ( m_player )
	{
		// Finalize potential player teleport to its destination

		// We don't need to tick components because we always call SnapPlayerEntityToTeleportedTransform before this func
		//if ( CAnimatedComponent* animatedComponent = m_player->GetRootAnimatedComponent() )
		//{
			// TODO - this is super hacky!
			//const Float smallDeltaTime = 0.0001f;
			//animatedComponent->OnTickPrePhysics( smallDeltaTime );
			//animatedComponent->OnTickPostPhysics( smallDeltaTime );
		//}
		m_player->HACK_UpdateLocalToWorld();
	}

	// Get the camera into default position

	gameCam->ResetCamera();
}

Bool CR4Game::GetGameplayCameraData( ICamera::Data& data )
{
	if ( CCustomCamera* camera = GetCamera() )
	{
		camera->UpdateWithoutInput();
		return camera->GetData( data );
	}

	return false;
}

CEntity* CR4Game::GetGameplayCameraEntity() const
{
	return GetCamera();
}

void CR4Game::StartGameplayCameraBlendFrom( const ICamera::Data& data, Float blendTime )
{
	if ( CCustomCamera* camera = GetCamera() )
	{
		camera->StartBlendFrom( data, blendTime );
	}
}

void CR4Game::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	TBaseClass::OnViewportCalculateCamera( view, camera );

	//////////////////////////////////////////////////////////////////////////
	// Calculate reference points for tree fading. These three points form two vertical planes.
	// Any trees that are behind both planes and within some distance of the camera (the actual
	// distance used is the maximum distance from the camera of the three reference points) will
	// be faded out (as long as tree fading is turned on)
	//
	// In this implementation, rays are cast in the bottom left and right corners of the screen,
	// and the points where they intersect the terrain are chosen for left/right reference points.
	// The player's position is the center reference. The left/right points are limited to be no
	// further from the camera than the player, and if a ray does not intersect the terrain the
	// point is chosen on that ray that is the same distance as the player.
	if ( m_player != nullptr && m_activeWorld->GetFoliageScene() != nullptr )
	{
		if ( IsFreeCameraEnabled() || GGame->IsPlayingCameraCutscene() )
		{
			return;
		}


		const Vector& camPos = camera.GetPosition();

		const Vector playerPos = m_player->GetWorldPosition();
		const Float distToPlayer = playerPos.DistanceTo( camPos );

		Vector corners[4];
		camera.GetFrustumCorners( 0.0f, corners, false );

		const Vector& bottomLeft = corners[0];
		const Vector& bottomRight = corners[3];

		// Can adjust this to move the rays towards the center of the screen. 0.5 puts both rays in the middle, and nothing
		// will be faded. So keep it under that :)
		static Float TIGHTENING = 0.0f;

		const Vector cornerDiff = bottomRight - bottomLeft;
		const Vector frustumPoints[2] = {
			bottomLeft + cornerDiff * TIGHTENING,
			bottomRight - cornerDiff * TIGHTENING
		};

		Vector refPoints[2];

		for ( Uint32 i = 0; i < 2; ++i )
		{
			// Is casting just to the terrain enough? Maybe it should cast into physics scene, against everything solid (except trees).
			// The distance cap tends to make it not quite reach the terrain anyways...
			Vector dir = frustumPoints[i] - camPos;
			if ( m_activeWorld->GetTerrain() != nullptr && m_activeWorld->GetTerrain()->Intersect( camPos, dir, refPoints[i] ) )
			{
				// Limit the selected point to be no further away than the player. This way, we're not likely to hide trees that are actually in
				// front of the player.
				Float dist = refPoints[i].DistanceTo( camPos );
				if ( dist > distToPlayer )
				{
					refPoints[i] = camPos + ( refPoints[i] - camPos ) * distToPlayer / dist;
				}
			}
			else
			{
				// No terrain intersection, so we just extend the frustum corner out to the player's distance. Seems to work reasonably well.
				refPoints[i] = camPos + dir.Normalized3() * distToPlayer;
			}
		}

#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			m_activeWorld->GetFoliageScene()->SetTreeFadingReferencePoints( refPoints[0], refPoints[1], camPos );
		}
		else
#endif // USE_ANSEL
		{
			m_activeWorld->GetFoliageScene()->SetTreeFadingReferencePoints( refPoints[0], refPoints[1], playerPos );
		}
	}
}


void CR4Game::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	TBaseClass::OnViewportGenerateFragments( view, frame );

#if defined(RED_KINECT)
	m_kinectSpeechRecognizerListener->OnViewportGenerateFragments( view, frame );
#endif

}

void CR4Game::OnFocusModeVisibilityChanged( CGameplayEntity* entity, Bool persistent )
{
	RED_ASSERT( entity != nullptr );
	CFocusModeController* focusModeController = GetSystem< CFocusModeController >();
	if ( focusModeController != nullptr )
	{
		if ( focusModeController->IsActive() )
		{
			focusModeController->RegisterForUpdateFocusModeVisibility( entity );
		}
		focusModeController->StoreFocusModeVisibility( entity, persistent );
	}
}

#ifdef ENABLE_REVIEW_WORKAROUNDS
Bool CR4Game::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if( key == IK_Pad_Back_Select && action == IACT_Press && !RIM_IS_KEY_DOWN( IK_Pad_LeftShoulder ) && !RIM_IS_KEY_DOWN( IK_Pad_RightShoulder ) )
	{
		if( IsPaused() )
		{
			Unpause( TXT("Player Request") );
		}
		else
		{
			Pause( TXT("Player Request") );
		}

		return true;
	}
	//else
	//{
	//	if( key == IK_Pad_DigitDown && action == IACT_Press )
	//	{
	//		Cheat_TeleportPlayer( view, view->GetWidth() / 2, view->GetHeight() / 2 );
	//	}
	//}

	return TBaseClass::OnViewportInput( view, key, action, data );
};
#endif

CCustomCamera* CR4Game::GetCamera() const
{
	return Cast< CCustomCamera >( m_camera );
}

Bool CR4Game::IsPositionInInterior( const Vector& position )
{
	if ( CCommonMapManager* commonMapManager = GetSystem< CCommonMapManager >() )
	{
		return commonMapManager->GetInteriorFromPosition( position ) != nullptr;
	}
	return false;
}

Int32 CR4Game::GetMonsterCategoryForNpc(const CNewNPC* npc) const
{
	if(npc->GetCachedMonsterCategory() >= 0)
	{
		return npc->GetCachedMonsterCategory();
	}

	CEntityTemplate* temp = npc->GetEntityTemplate();
	if( temp )
	{
		THandle<CMonsterParam>  monsterParam = nullptr;
		TDynArray<CGameplayEntityParam*> params;
		temp->CollectGameplayParams( params, CMonsterParam::GetStaticClass() ) ;

		if ( params.Size() > 0 )
		{
#ifndef NO_EDITOR
			if ( params.Size() > 1 )
			{
				DATA_HALT( DES_Tiny, temp, TXT("Entity"), TXT( "Multiple monster params, only one is applicable" ) );
			}
#endif

			monsterParam = static_cast< CMonsterParam* >( params[0] );

			return monsterParam->GetMonsterCategory();
		}
	}

	return -1;
}

void CR4Game::funcActivateHorseCamera( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, activate, true );
	GET_PARAMETER( Float, blend, 0.f );
	GET_PARAMETER_OPT( Bool, instantMount, false );
	FINISH_PARAMETERS;

	if( activate )
	{
		CCustomCamera* horseCam = m_horseCamera.Get();
		if( horseCam )
		{
			m_activeWorld->GetCameraDirector()->ActivateCamera( horseCam, horseCam, blend );

			if( instantMount )
			{
				horseCam->ResetCamera();
			}
		}
	}
	else
	{
		ActivateGameCamera( blend );
	}
}

void CR4Game::funcGetFocusModeController( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetSystem< CFocusModeController>() );
}

void CR4Game::funcGetSurfacePostFX( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetSystem< CGameplayFXSurfacePost >() );
}

void CR4Game::funcGetCommonMapManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetSystem< CCommonMapManager >() );
}

void CR4Game::funcGetJournalManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetSystem< CWitcherJournalManager >() );
}

void CR4Game::funcGetLootManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetSystem< CR4LootManager >() );
}

void CR4Game::funcGetCityLightManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetSystem< CCityLightManager >() );
}

void CR4Game::funcGetInteractionsManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetSystem< CInteractionsManager >() );
}

void CR4Game::funcGetGuiManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetGuiManager() );
}

void CR4Game::funcGetGlobalEventsScriptsDispatcher( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_globalEventsScriptsDispatcher );
}

void CR4Game::funcGetFastForwardSystem( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetSystem< CGameFastForwardSystem >() );
}

void CR4Game::funcStartSepiaEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, fadeTime, 0.000001f );
	FINISH_PARAMETERS;

	ASSERT( GetActiveWorld() && GetActiveWorld()->GetEnvironmentManager() );

	GetActiveWorld()->GetEnvironmentManager()->EnableSepiaEffect( fadeTime );

	RETURN_BOOL( true );
}

void CR4Game::funcStopSepiaEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, fadeTime, 0.000001f );
	FINISH_PARAMETERS;

	ASSERT( GetActiveWorld() && GetActiveWorld()->GetEnvironmentManager() );

	GetActiveWorld()->GetEnvironmentManager()->DisableSepiaEffect( fadeTime );

	RETURN_BOOL( true );
}

void CR4Game::funcGetWindAtPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	FINISH_PARAMETERS;

	Vector wind( Vector::ZEROS );
	if ( m_activeWorld )
	{
		wind = m_activeWorld->GetWindAtPoint( point );
	}

	RETURN_STRUCT( Vector, wind );
}

void CR4Game::funcGetWindAtPointForVisuals( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	FINISH_PARAMETERS;

	Vector wind( Vector::ZEROS );
	if ( m_activeWorld )
	{
		wind = m_activeWorld->GetWindAtPointForVisuals( point, true, false );
	}

	RETURN_STRUCT( Vector, wind );
}

void CR4Game::funcGetGameCamera( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( Cast< CCustomCamera >( m_camera ) );		// msl> SafeCast -> Cast because m_camera is possibly null
}

void CR4Game::funcGetBuffImmunitiesForActor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, NULL );
	FINISH_PARAMETERS;

	CBuffImmunity  val;

	CEntityTemplate* temp = actor.Get() ? actor.Get()->GetEntityTemplate() : NULL;
	if(temp)
	{
		TDynArray<CGameplayEntityParam*> params;
		temp->CollectGameplayParams( params , CBuffImmunityParam::GetStaticClass() ) ;
		for( auto i = params.Begin() ; i!= params.End(); ++i  )
		{
			val.m_immunityTo.PushBackUnique( Cast<CBuffImmunityParam>(*i)->GetImmunities() );
			Int32 flags = Cast<CBuffImmunityParam>(*i)->GetFlags();
			val.m_potion	 = val.m_potion		|| ((flags & IF_Potion)    );
			val.m_positive   = val.m_positive	|| ((flags & IF_Positive)  );
			val.m_negative   = val.m_negative	|| ((flags & IF_Negative)  );
			val.m_neutral    = val.m_neutral	|| ((flags & IF_Neutral)   );
			val.m_immobilize = val.m_immobilize || ((flags & IF_Immobilize));
			val.m_confuse	 = val.m_confuse	|| ((flags & IF_Confuse)   );
			val.m_damage	 = val.m_damage		|| ((flags & IF_Damage)    );
		}
	}

	RETURN_STRUCT( CBuffImmunity , val )
}

void CR4Game::funcGetMonsterParamsForActor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, NULL );
	GET_PARAMETER_REF( Int8, monsterCategory, 0 );
	GET_PARAMETER_REF( CName, soundMonsterName, CName::NONE );
	GET_PARAMETER_REF( Bool, isTeleporting, false );
	GET_PARAMETER_REF( Bool, canBeTargeted, false );
	GET_PARAMETER_REF( Bool, canBeHitByFists, false );
	FINISH_PARAMETERS;

	Bool res = false;

	CEntityTemplate* temp = actor.Get() ? actor.Get()->GetEntityTemplate() : NULL;
	if( temp )
	{
		TDynArray<CGameplayEntityParam*> params;
		temp->CollectGameplayParams( params, CMonsterParam::GetStaticClass() ) ;

		if ( params.Size() > 0 )
		{
#ifndef NO_EDITOR
			if ( params.Size() > 1 )
			{
				DATA_HALT( DES_Tiny, temp, TXT("Entity"), TXT( "Multiple monster params, only one is applicable" ) );
			}
#endif
			CMonsterParam* monsterParam = static_cast< CMonsterParam* >( params[0] );

			monsterCategory  = (Int8)monsterParam->GetMonsterCategory();
			
			soundMonsterName = monsterParam->GetSoundMonsterName();
			isTeleporting    = monsterParam->IsMonsterTeleporting();
			canBeTargeted    = monsterParam->CanMonsterBeTargeted();
			canBeHitByFists  = monsterParam->CanMonsterBeHitByFists();
			res = true;
		} // Having no monster params probalby means we are hostile but not a monster ( ie animals )
	}

	RETURN_BOOL( res );
}

void CR4Game::funcGetMonsterParamForActor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, NULL );
	GET_PARAMETER_REF( THandle<CMonsterParam>, monsterParam, nullptr );
	FINISH_PARAMETERS;

	Bool res = false;
	CEntityTemplate* temp = actor.Get() ? actor.Get()->GetEntityTemplate() : NULL;
	if( temp )
	{
		TDynArray<CGameplayEntityParam*> params;
		temp->CollectGameplayParams( params, CMonsterParam::GetStaticClass() ) ;

		if ( params.Size() > 0 )
		{
#ifndef NO_EDITOR
			if ( params.Size() > 1 )
			{
				DATA_HALT( DES_Tiny, temp, TXT("Entity"), TXT( "Multiple monster params, only one is applicable" ) );
			}
#endif

			monsterParam = static_cast< CMonsterParam* >( params[0] );
			if(actor->IsA<CNewNPC>())
			{
				Cast<CNewNPC>(actor.Get())->SetCachedMonsterCategory(monsterParam->GetMonsterCategory());
			}
			res = true;
		} // Having no monster params probalby means we are hostile but not a monster ( ie animals )
	}

	RETURN_BOOL( res );
}

void CR4Game::funcGetVolumePathManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CVolumePathManager, GetSystem< CVolumePathManager >() );
}

void CR4Game::funcGetSecondScreenManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CR4SecondScreenManagerScriptProxy, m_secondScreenScriptProxy );
}

void CR4Game::funcGetKinectSpeechRecognizer( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CR4KinectSpeechRecognizerListenerScriptProxy, m_kinectSpeechRecognizerListenerScriptProxy );
}

void CR4Game::funcToggleMenus( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CR4GuiManager* guiManager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( guiManager )
	{
		guiManager->ToggleMenus();
	}
}

void CR4Game::funcToggleInput( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CR4GuiManager* guiManager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( guiManager )
	{
		guiManager->ToggleInput();
	}
}

void CR4Game::funcNotifyOpeningJournalEntry( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	CR4GuiManager* guiManager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( guiManager )
	{
		guiManager->OnOpenedJournalEntryEvent( entryHandle );
	}
}

void CR4Game::Pause( const String& reason )
{
	TBaseClass::Pause( reason );

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->LogV( TE_SYS_GAME_PAUSE, reason );
#endif
}

void CR4Game::Unpause( const String& reason )
{
	TBaseClass::Unpause( reason );

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->LogV( TE_SYS_GAME_UNPAUSE, reason );
#endif
}

Bool CR4Game::TickMainMenu( float timeDelta )
{
#ifndef NO_DEBUG_PAGES
	if ( IDebugPageManagerBase::GetInstance()->IsDebugPageActive( TXT( "Game Startup" ) ) )
	{
		return false;
	}
#endif //NO_DEBUG_PAGES

	if ( m_mainMenuController )
	{
		m_mainMenuController->Tick( timeDelta );
	}

	return true;
}

void CR4Game::OnLanguageChange()
{
	TBaseClass::OnLanguageChange();
}





Bool CR4Game::GetCustomPlayerStartingPoint( const CName& tag, Vector& position, EulerAngles& rotation )
{
	CCommonMapManager* manager = GetSystem< CCommonMapManager >();

	if ( const TDynArray< SEntityMapPinInfo >* entityMapPins = manager->GetEntityMapPins( m_activeWorld->GetDepotPath() ) )
	{
		for ( auto it = entityMapPins->Begin(); it != entityMapPins->End(); ++it )
		{
			const SEntityMapPinInfo& pin = *it;

			if ( ( pin.m_entityType == CName( TXT("RoadSign") ) || pin.m_entityType == CName( TXT("Harbor") ) ) && pin.m_entityName == tag )
			{
				position = pin.m_fastTravelTeleportWayPointPosition;
				rotation = pin.m_fastTravelTeleportWayPointRotation;
				return true;
			}
		}
	}
	return false;
}

CR4CreateEntityManager *const CR4Game::GetR4CreateEntityManager()const
{
	return static_cast< CR4CreateEntityManager * >( m_createEntityManager );
}

void CR4Game::funcSummonPlayerHorse( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, teleportToSafeSpot, true );
	GET_PARAMETER( THandle< CR4CreateEntityHelper >, createEntityHelperHandle, nullptr );
	FINISH_PARAMETERS;

	CR4CreateEntityHelper *const createEntityHelper = createEntityHelperHandle.Get();
	if ( createEntityHelper )
	{
		GetR4CreateEntityManager()->SummonPlayerHorse( createEntityHelper, teleportToSafeSpot );
	}
}

void CR4Game::funcGetResourceAliases( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< String >, aliases, TDynArray< String >() );
	FINISH_PARAMETERS;

	aliases.ClearFast();

	const CResourceDefManager::TResourceMap& resourceMap = SResourceDefManager::GetInstance().GetResourceMap();
	if ( !resourceMap.Empty() )
	{
		aliases.Reserve( resourceMap.Size() );
		for ( CResourceDefManager::TResourceMap::const_iterator it = resourceMap.Begin(); it != resourceMap.End(); ++it )
		{
			aliases.PushBackUnique( (*it).m_first );
		}
	}
}

void CR4Game::CloseMainMenu()
{
	if ( m_mainMenuController )
	{
		m_mainMenuController->CloseMenu();
	}
}

void CR4Game::funcGetTutorialSystem( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetSystem< CR4TutorialSystem > () );
}

void CR4Game::funcDisplayUserProfileSystemDialog( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	GUserProfileManager->DisplayUserProfileSystemDialog();

	RETURN_VOID();
}

void CR4Game::funcSetRichPresence( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, presence, CName::NONE );
	FINISH_PARAMETERS;

	GUserProfileManager->SetUserPresence( presence );

	RETURN_VOID();
}

void CR4Game::funcSetActiveUserPromiscuous( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	GUserProfileManager->SetActiveUserPromiscuous();

	RETURN_VOID();
}

void CR4Game::funcChangeActiveUser( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	GUserProfileManager->ChangeActiveUser();

	RETURN_VOID();
}

void CR4Game::funcGetActiveUserDisplayName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( GUserProfileManager->GetActiveUserDisplayName( 30 ) );
}

void CR4Game::funcGetPlatform( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( (int)(Config::GetPlatform()) );
}

void CR4Game::funcIsContentAvailable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, contentTag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( GContentManager->IsContentAvailable(contentTag) );
}

void CR4Game::funcProgressToContentAvailable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, contentTag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_INT( (int)(GContentManager->GetPercentCompleted(contentTag)) );
}

void CR4Game::funcShouldForceInstallVideo( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	// Don't force the install video if just for languages - that's annoying and ultimately unnecessary given where else
	// we can block
	const Bool retval = GContentManager->GetStallForMoreContent() >= eContentStall_DiscLaunch;
	
	RETURN_BOOL( retval );
}

void CR4Game::funcOnUserDialogCallback(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Int32, messageId, 0 );
	GET_PARAMETER( Int32, actionId, 0 );
	FINISH_PARAMETERS;

	GUserProfileManager->OnUserDialogCallback( messageId, actionId );
}

void CR4Game::funcSaveUserSettings( CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	SConfig::GetInstance().Save();
	if( m_inputManager != nullptr )
	{
		m_inputManager->SaveUserMappings();
	}
	else
	{
		ERR_GAME( TXT("input manager does not exist while trying to save configs. No input configs will be saved.") );
	}
}

void CR4Game::funcDisplaySystemHelp( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	GUserProfileManager->DisplayHelp();

	RETURN_VOID();
}

void CR4Game::funcDisplayStore( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	GUserProfileManager->DisplayStore();
}

void CR4Game::funcGetGwintManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetSystem< CR4GwintManager >() );
}

void CR4Game::funcIsDebugQuestMenuEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

#if defined( RED_FINAL_BUILD ) && !defined( RED_PROFILE_BUILD ) 
	const Bool isMenuEnabled = false; // MUST RETURN FALSE FOR CERTIFICATION. DO NOT FORGET.
#else
	const Bool isMenuEnabled = true;
#endif

	RETURN_BOOL( isMenuEnabled );
}

void CR4Game::funcEnableNewGamePlus( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	if ( IsNewGamePlus() )
	{
		// can't start new game plus from ng+ session
		GetGameSaver()->EnableNewGamePlus( false );
	}
	else
	{
		GetGameSaver()->EnableNewGamePlus( enable );
	}
}

void CR4Game::funcStartNewGamePlus( CScriptStackFrame& stack, void* result )
{
	SSavegameInfo definfo;
	GET_PARAMETER( SSavegameInfo, info, definfo );
	FINISH_PARAMETERS;

	RETURN_ENUM( StartNewGamePlus( info ) );
}

void CR4Game::funcWorldDLCExtender( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetWorldDLCExtender() );
}

void CR4Game::funcShowSteamControllerBindingPanel( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool retval = false;
	if ( GUserProfileManager )
		retval = GUserProfileManager->HACK_ShowSteamControllerBindingPanel();

	RETURN_BOOL( retval );
}
