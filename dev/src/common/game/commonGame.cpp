/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "commonGame.h"

#include "../core/configVar.h"
#include "../core/deferredDataBufferOOMQueue.h"
#include "../core/deferredDataBufferKickoff.h"
#include "../core/depot.h"
#include "../core/garbageCollector.h"
#include "../core/gatheredResource.h"
#include "../core/resourceDefManager.h"
#include "../core/scriptingSystem.h"
#include "../core/contentManager.h"
#include "../core/taskRunner.h"
#include "../core/taskDispatcher.h"
#include "../core/messagePump.h"
#include "../core/chunkedLZ4File.h"

#include "../engine/registryAccess.h"
#include "../engine/flashPlayer.h"
#include "../engine/freeCamera.h"
#include "../engine/gameSaveManager.h"
#include "../engine/debugPageDynamicAttributes.h"
#include "../physics/physicsWorld.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../engine/redGuiManager.h"
#include "../engine/debugWindowsManager.h"
#include "../engine/renderCommands.h"
#include "../engine/userProfile.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/debugPageManagerTabbed.h"
#include "../engine/localizationManager.h"
#include "../engine/environmentManager.h"
#include "../physics/physicsWorldUtils.h"
#include "../engine/idTagManager.h"
#include "../engine/debugConsole.h"
#include "../engine/gameTimeManager.h"
#include "../engine/animationLogger.h"
#include "../engine/visualDebug.h"
#include "../engine/viewport.h"
#include "../engine/dynamicLayer.h"
#include "../engine/tagManager.h"
#include "../engine/debugServerManager.h"
#include "../engine/renderProxyIterator.h"
#include "../engine/meshTypeComponent.h"
#include "../engine/speedConfig.h"
#include "../engine/inputDeviceManager.h"
#include "../engine/animationManager.h"
#include "../engine/loadingOverlay.h"

#include "actorsManager.h"
#include "attackRange.h"
#include "communitySystem.h"
#include "definitionsManager.h"
#include "gameFastForwardSystem.h"
#include "gameplayStorage.h"
#include "gameWorld.h"
#include "gameWallaInterface.h"
#include "interactionsManager.h"
#include "nodeStorage.h"
#include "storySceneSystem.h"
#include "strayActorManager.h"
#include "questsSystem.h"
#include "player.h"
#include "journalManager.h"
#include "expManager.h"
#include "reactionsManager.h"
#include "explorationScriptSupport.h"
#include "moveGlobalPathPlanner.h"
#include "templateLoadBalancer.h"
#include "behTreeMachine.h"
#include "attitudeManager.h"
#include "minigame.h"
#include "rewards.h"
#include "gameSaver.h"
#include "bgCounter.h"
#include "guiManager.h"
#include "hud.h"
#include "movingPhysicalAgentComponent.h"
#include "factsDB.h"
#include "commonGameResource.h"
#include "storySceneActorMap.h"
#include "behTreeInstance.h"
#include "scriptRegistrationManager.h"
#include "lootDefinitions.h"
#include "behTreeReactionManager.h"
#include "createEntityManager.h"
#include "createEntityHelper.h"
#include "newNpcSensesManager.h"
#include "itemIterator.h"
#include "gameDebuggerPlugin.h"
#include "questDebuggerPlugin.h"
#include "factsDebuggerPlugin.h"
#include "gameSpeedConfigManager.h"
#include "boidSpecies.h"
#include "roadsManager.h"
#include "inGameConfigWrapper.h"
#include "questScenePlayer.h"
#include "dlcManager.h"
#include "dlcDefinition.h"
#include "entityPool.h"
#include "inGameConfigDLC.h"
#include "../engine/inGameConfig.h"
#include "../core/version.h"
#include "../engine/hwCursorManager.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "menu.h"

RED_DEFINE_STATIC_NAME( OnBeforeWorldChange )

namespace Config
{
	extern TConfigVar<Bool> 							cvDBGConsoleOn;
	TConfigVar< Bool >									cvSubtitles( "Audio", "Subtitles", true, eConsoleVarFlag_Save );
	TConfigVar< Int32, Validation::IntRange< 3, 15 > >	cvAutosaveTimeInterval( "Gameplay", "AutosaveTimeInterval", 10, eConsoleVarFlag_Save ); 
	extern TConfigVar< Bool >							cvIsHardwareCursor;
}

CCommonGame* GCommonGame = NULL;

Uint32 SBgCounter::STATIC_COUNTER = 0;
Uint32 SBgCounter::CURRENT_BUCKET_ID = 0;

CGatheredResource resGameDebugFont( TXT("engine\\fonts\\parachute.w2fnt"), RGF_Startup );

#ifndef NO_DEBUG_PAGES
void GDebugPageTogglePhysicalMovement()
{
	CPlayer * player = GCommonGame ? GCommonGame->GetPlayer() : NULL;
	if ( player != NULL )
	{
		CMovingPhysicalAgentComponent * mac = Cast< CMovingPhysicalAgentComponent >( player->GetAgent() );
		if ( mac != NULL )
		{
			mac->SetPhysicalRepresentationRequest( CMovingAgentComponent::Req_Toggle, CMovingAgentComponent::LS_Default );
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////

class CFlushGameplayStorageUpdatesTask : public CTask
{
private:
	CGameplayStorage*				m_gameplayStorage;
	TDynArray< CGameplayEntity* >	m_gameplayStorageUpdateList;
	Bool							m_isGameplayStorageUpdateListOverflow;

	CActorsManager*					m_actorsStorage;
	TDynArray< CActor* >			m_actorsManagerUpdateList;
	Bool							m_isActorsManagerUpdateListOverflow;

	static Uint32					s_lastUpdatePositionEntityCount;
	static Double					s_lastUpdatePositionTimeMS;
	static Double					s_lastUpdatePositionWaitTimeMS;

#ifdef QUAD_TREE_THREAD_SAFETY_ENABLED
	template < typename STORAGE >
	class ScopedThreadOwnerSwitcher
	{
	private:
		Red::System::Internal::ThreadId m_prevOwnerThreadId;
		STORAGE* m_storage;
	public:
		ScopedThreadOwnerSwitcher( STORAGE* storage )
			: m_storage( storage )
			, m_prevOwnerThreadId( storage->GetOwnerThreadId() )
		{
			m_storage->SetOwnerThreadId( Red::System::Internal::ThreadId::CurrentThread() );
		}

		~ScopedThreadOwnerSwitcher()
		{
			m_storage->SetOwnerThreadId( m_prevOwnerThreadId );
		}
	};
#endif

public:
	CFlushGameplayStorageUpdatesTask(
		CGameplayStorage* gameplayStorage, ThreadSafeNodeList< 2048, CGameplayEntity >& gameplayStorageUpdateList,
		CActorsManager* actorsStorage, ThreadSafeNodeList< 1024, CActor >& actorsManagerUpdateList )
		: m_gameplayStorage( gameplayStorage )
		, m_actorsStorage( actorsStorage )
	{
		// Copy entities to TDynArray

		if ( !( m_isGameplayStorageUpdateListOverflow = gameplayStorageUpdateList.IsOverflow() ) )
		{
			m_gameplayStorageUpdateList.Reserve( gameplayStorageUpdateList.Size() );
			for ( Int32 i = 0; i < gameplayStorageUpdateList.Size(); ++i )
			{
				if ( CGameplayEntity* entity = gameplayStorageUpdateList[ i ] )
				{
					m_gameplayStorageUpdateList.PushBack( entity );
				}
			}
		}

		// Copy actors to TDynArray

		if ( !( m_isActorsManagerUpdateListOverflow = actorsManagerUpdateList.IsOverflow() ) )
		{
			m_actorsManagerUpdateList.Reserve( actorsManagerUpdateList.Size() );
			for ( Int32 i = 0; i < actorsManagerUpdateList.Size(); ++i )
			{
				if ( CActor* actor = actorsManagerUpdateList[ i ] )
				{
					m_actorsManagerUpdateList.PushBack( actor );
				}
			}
		}
	}

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT("FlushGameplayStorageUpdatesTask"); }
	virtual Uint32 GetDebugColor() const override { return COLOR_UINT32( 83, 133, 250 ); }
#endif

	void Run() override
	{
		PC_SCOPE_PIX( FlushGameplayStorageUpdatesTask );

		CTimeCounter timer;

		s_lastUpdatePositionEntityCount = m_gameplayStorageUpdateList.Size() + m_actorsManagerUpdateList.Size();

		ProcessUpdatePosition( m_gameplayStorage, m_gameplayStorageUpdateList, m_isGameplayStorageUpdateListOverflow );
		ProcessUpdatePosition( m_actorsStorage, m_actorsManagerUpdateList, m_isActorsManagerUpdateListOverflow );

		s_lastUpdatePositionTimeMS = timer.GetTimePeriodMS();
	}

	void WaitForFinish()
	{
		CTimeCounter timer;

		while ( !IsFinished() )
		{
			Red::Threads::YieldCurrentThread();
		}

		s_lastUpdatePositionWaitTimeMS = timer.GetTimePeriodMS();
	}

	template < typename STORAGE, typename CONTAINER >
	static void ProcessUpdatePosition( STORAGE* storage, CONTAINER& container, Bool isOverflow )
	{
#ifdef QUAD_TREE_THREAD_SAFETY_ENABLED
		ScopedThreadOwnerSwitcher< STORAGE > containerThreadOwnerSwitcher( storage );
#endif

		if ( isOverflow )
		{
			storage->UpdatePositionForAllNodes();
			return;
		}

#ifndef RED_FINAL_BUILD
		Uint32 numBoxUnchanged = 0;
		Uint32 numNodeUnchanged = 0;
		Uint32 numUpdated = 0;
#endif

		// Note: apparently, the main cost in this code is calculating bbox for each processed entity (done in UpdatePosition())
		// For most entities bbox is usually unchanged

		const Uint32 size = container.Size();
		for ( Uint32 i = 0; i < size; ++i )
		{
			auto* entity = container[ i ];

#ifdef RED_FINAL_BUILD
			storage->UpdatePosition( entity );
#else
			const EQuadTreeStorageUpdateResult result = storage->UpdatePosition( entity );
			numBoxUnchanged += result == EQuadTreeStorageUpdateResult_BoxUnchanged ? 1 : 0;
			numNodeUnchanged += result == EQuadTreeStorageUpdateResult_NodeUnchanged ? 1 : 0;
			numUpdated += result == EQuadTreeStorageUpdateResult_Updated ? 1 : 0;
#endif
		}
	}
};

Uint32 CFlushGameplayStorageUpdatesTask::s_lastUpdatePositionEntityCount;
Double CFlushGameplayStorageUpdatesTask::s_lastUpdatePositionTimeMS;
Double CFlushGameplayStorageUpdatesTask::s_lastUpdatePositionWaitTimeMS;

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( OnGiveReward )
RED_DEFINE_STATIC_NAME( OnSaveStarted )

IMPLEMENT_ENGINE_CLASS( CCommonGame );

CCommonGame::CCommonGame()
	: CGame()
	, m_player( NULL )
	, m_definitionsManager( NULL )
	, m_definitionsManagerAccessor( NULL )
	, m_rewardManager( nullptr )
	, m_gameplayStorage( NULL )
	, m_actorsManager( NULL )
	, m_behTreeReactionManager( NULL )
	, m_reactionsManager( NULL )
	, m_pathPlanner( NULL )
	, m_gameSaver( NULL )
	, m_controllerDisconnectMessageNeeded( false )
	, m_startingNewGamePlus( false )
	, m_newGamePlusPlayerLoader( nullptr )
	, m_showedConfigChangedMessage( false )
	, m_guiManager( NULL )
	, m_forceEnableUIAnalog( false )
	, m_boidSpecies (NULL)
	, m_dlcManager (NULL)
	, m_dlcInGameConfigGroup( nullptr )
	, m_createEntityManager( nullptr )
	, m_npcSensesManager( nullptr )
	, m_autoSaveRequestTimer( -1.f )
	, m_autoSaveForced( false )
	, m_flushGameplayStorageUpdatesTask( nullptr )
	, m_HACK_P_1_1_worldChangeFromQuest( false )
{
	m_gameplayStorageUpdate.Reset();
	m_actorsManagerUpdate.Reset();

#ifndef NO_DEBUG_PAGES
	if( !IDebugPageManagerBase::GetInstance() )
	{
		new CDebugPageManagerTabbed( resGameDebugFont );
	}
#endif

	// debug server plugins
	DBGSRV_REG_PLUGIN( new CGameDebuggerPlugin() );
	DBGSRV_REG_PLUGIN( new CQuestDebuggerPlugin() );
	DBGSRV_REG_PLUGIN( new CFactsDebuggerPlugin() );
}

CCommonGame::~CCommonGame()
{
}

void CCommonGame::Init()
{
	TBaseClass::Init();

	GEngine->GetInputDeviceManager()->RegisterListener( &CCommonGame::OnControllerEvent, this );

	// Create item manager
	m_definitionsManager = new CDefinitionsManager();
	m_definitionsManagerAccessor = CreateObject< CDefinitionsManagerAccessor >( this );
	m_definitionsManagerAccessor->SetManager( m_definitionsManager );

	m_rewardManager = new CRewardManager();
	// Actor storage
	m_gameplayStorage = new CGameplayStorage();

	// Create actors manager for LOD system
	m_actorsManager = new CActorsManager( &m_gameplayConfig );

	// Create save keeper
	m_gameSaver = new CGameSaver();

	// Find the debug manager class
	CClass* gameClass = SRTTI::GetInstance().FindClass( CNAME( CDebugAttributesManager ) );
	if ( !gameClass )
	{
		WARN_GAME( TXT("Unable to find Witcher 3 debug attributes class") );
		return;
	}

	// CreateEntity Manager
	m_createEntityManager = InstanciateCreateEntityManager();

	// CNewNpcSensesManager
	m_npcSensesManager = new CNewNpcSensesManager();

	// CDLCManager
	m_dlcManager = new CDLCManager();
	m_dlcManager->SetParent(this);

	m_dlcInGameConfigGroup = CInGameConfigDLCGroup::CreateDLCGroup( m_dlcManager );
	GInGameConfig::GetInstance().RegisterConfigGroup( m_dlcInGameConfigGroup );

	GContentManager->RegisterContentListener( m_dlcManager );

	// CSpeedConfigManager
	CGameSpeedConfigManager *speedConfigManager  = new CGameSpeedConfigManager();
	speedConfigManager->InitParams( );
	m_speedConfigManager = speedConfigManager;

	// Scan initial DLCs
	m_dlcManager->Scan();
	PopulateDlcInGameConfigGroupWithVars();

	//Initialize game sound systems
	CGameWallaInterface::InitClass();

	// Debug pages
#ifndef NO_DEBUG_PAGES
	extern void InitCameraDebugPages();
	InitCameraDebugPages();
	extern void InitTriggeraDebugPages();
	InitTriggeraDebugPages();
	extern void InitGameShortcutsDebugPages();
	InitGameShortcutsDebugPages();
	extern void InitLayerStorageDebugPages();
	InitLayerStorageDebugPages();
	extern void InitSceneDebugPages();
	InitSceneDebugPages();
#endif

#ifndef NO_DEBUG_PAGES
	extern void CreateDebugPageStartPage();
	CreateDebugPageStartPage();
	extern void CreateDebugPageGameFrameBudget();
	CreateDebugPageGameFrameBudget();
	extern void CreateDebugPageQuests();
	CreateDebugPageQuests();
	extern void CreateDebugPageQuests2();
	CreateDebugPageQuests2();
	extern void CreateDebugPageQuestsDebug();
	CreateDebugPageQuestsDebug();
	extern void CreateDebugPageScenes();
	CreateDebugPageScenes();
	extern void CreateDebugPageFactsDB();
	CreateDebugPageFactsDB();
#endif
}


void CCommonGame::ShutDown()
{
	if( m_dlcInGameConfigGroup )
	{
		GInGameConfig::GetInstance().UnregisterConfigGroup( m_dlcInGameConfigGroup );
		m_dlcInGameConfigGroup->Discard();
		delete m_dlcInGameConfigGroup;
		m_dlcInGameConfigGroup = nullptr;
	}

	if ( m_dlcManager )
	{
		GContentManager->UnregisterContentListener( m_dlcManager );

		m_dlcManager->OnGameEnding();
		m_dlcManager->Discard();
		m_dlcManager = nullptr;
	}

	for ( Uint32 i = 0; i < m_gameSystems.Size(); ++i )
	{
		IGameSystem* gameSystem = m_gameSystems[ i ];
		if ( gameSystem != NULL )
		{
			gameSystem->Discard();
		}
		m_gameSystems[ i ] = NULL;
	}

	// Delete item manager
	if( m_definitionsManager )
	{
		delete m_definitionsManager;
		m_definitionsManager = NULL;
	}
	if ( m_definitionsManagerAccessor )
	{
		m_definitionsManagerAccessor->Discard();
		m_definitionsManagerAccessor = NULL;
	}
	if ( m_rewardManager )
	{
		delete m_rewardManager;
		m_rewardManager = nullptr;
	}

	if ( m_gameplayStorage )
	{
		delete m_gameplayStorage;
		m_gameplayStorage = NULL;
	}

	if ( m_actorsManager )
	{
		delete m_actorsManager;
		m_actorsManager = NULL;
	}

	// Delete reaction manager
	if ( m_reactionsManager )
	{
		m_reactionsManager->Discard();
		m_reactionsManager = NULL;
	}

	// Delete beh tree reaction manager
	if( m_behTreeReactionManager )
	{
		m_behTreeReactionManager->Discard();
		m_behTreeReactionManager = NULL;
	}

	// Delete save keeper
	if( m_gameSaver )
	{
		delete m_gameSaver;
		m_gameSaver = NULL;
	}

	if ( m_createEntityManager )
	{
		delete m_createEntityManager;
		m_createEntityManager = nullptr;
	}

	if ( m_npcSensesManager )
	{
		delete m_npcSensesManager;
		m_npcSensesManager = nullptr;
	}

	if ( m_speedConfigManager )
	{
		delete m_speedConfigManager;
		m_speedConfigManager = nullptr;
	}

	// Shutdown localization manager
	SLocalizationManager::GetInstance().Shutdown();	
	CScriptRegistrationManager::ReleaseInstance();

	TBaseClass::ShutDown();
}

void CCommonGame::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( CommonGameTick );

	if ( HandleWorldChange() == true )
	{
		return;
	}

	// Check is save game is requested
	ProcessAutoSaveRequests( timeDelta );

	// Tick game session manager
	SGameSessionManager::GetInstance().UpdateLocks( timeDelta );

	TBaseClass::Tick( timeDelta );

	// Tick magic stuff
#ifndef NO_DEBUG_PAGES
	extern void TickSceneDebugPages();
	TickSceneDebugPages();
#endif

	// Process Controller disconnection events
	if ( m_controllerDisconnectMessageNeeded && !IsLoadingScreenShown() && !m_savegameToLoad.IsValid() )
	{
		m_controllerDisconnectMessageNeeded = false;
		CallEvent( RED_NAME( OnControllerDisconnected ) );
	}

	// Set value for BgCounter
	SBgCounter::UpdateTick( GEngine->GetCurrentEngineTick() );

	if ( !m_activeWorld || !m_viewport )
	{
		// We have no world tick, so we tick game stuff there
		OnTickDuringPostUpdateTransforms( timeDelta, true );
		if ( m_flashPlayer )
		{
			// Capture for render frame sync after GUI update
			m_flashPlayer->Capture();
		}
	}

	// Save game if requested
	m_gameSaver->Update();

	// Update debug windows
#ifndef NO_RED_GUI
	if( GDebugWin::GetInstance().GetVisible() == true )
	{
		PC_SCOPE( RedGui );
		GRedGui::GetInstance().OnTick( timeDelta );
	}
#endif	// NO_RED_GUI

	{
		PC_SCOPE( CreateEntityManager );
		m_createEntityManager->Update();
	}

	// PRECERT HACK: Needs to be done in the right spot at the right time
	if ( !IsActive() )
	{
		if ( m_dlcManager->IsNewContentAvailable() )
		{
			m_dlcManager->Scan();
			PopulateDlcInGameConfigGroupWithVars();
			if (m_guiManager != NULL)
			{
				m_guiManager->RefreshMenu();
			}
		}
	}
}

void CCommonGame::HACK_TickStorySceneVideoElementInstance( Float timeDelta )
{
	GCommonGame->GetSystem< CQuestsSystem >()->Tick( timeDelta ); // for starting next video
	GCommonGame->GetSystem< CStorySceneSystem >()->Tick( timeDelta ); // for ticking current video
	OnTickDuringPostUpdateTransforms( timeDelta, true ); // tick the GUI for subtitles

	if ( m_flashPlayer )
	{
		m_flashPlayer->Capture();
	}
}

void CCommonGame::OnTick( Float timeDelta )
{
	//we just ran the game for 1 frame so pause again and await input
	if ( !IsActivelyPaused() && m_frameAdvance )
	{
		SetActivePause( true );
		m_frameAdvance = false;
	}

	//the game was paused using SHIFT + PAUSE and ENTER (in game.cpp)
	//timeDelta is forced to 1 frame and the game will run for that duration 
	if ( IsActivelyPaused() && m_frameAdvance )
	{
		timeDelta = 0.033f; //1 frame = 1/30 = 0.033
		SetActivePause( false );
	}

	TBaseClass::OnTick( timeDelta );

	if( IsActive() )
	{
		if ( !IsPaused() && !IsActivelyPaused() )
		{
			{
				PC_SCOPE_PIX( GameSystemsTick );

				for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin(); gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
				{
					IGameSystem* gameSystem = *gameSystemIter;
					if ( gameSystem != NULL )
					{
						gameSystem->Tick( timeDelta );
					}
				}
			}


			// Update behaviortree machines
			{
				PC_SCOPE_PIX( TickBehTreeMachines );
				TickBehTreeMachines( timeDelta );
			}

			// Update behaviour tree reactions
			if( m_behTreeReactionManager )
			{
				PC_SCOPE_PIX( BehTreeReactionsManagerTick );
				m_behTreeReactionManager->Update();
			}

			// update reactions
			if ( m_reactionsManager )
			{
				PC_SCOPE_PIX( ReactionsManagerTick );
				m_reactionsManager->Tick( timeDelta );
			}

			// Update the path planner
			if ( m_pathPlanner )
			{
				PC_SCOPE_PIX( PathPlanerTick );
				m_pathPlanner->Tick( timeDelta );
			}

			{
				PC_SCOPE_PIX( ActorsManagerUpdate );
				m_actorsManager->Update( timeDelta );
			}

			if( m_entityPool )
			{
				PC_SCOPE_PIX( EntityPoolUpdate );
				CCommunitySystem* communitSystem = GetSystem< CCommunitySystem >();
				if( !communitSystem->WasDetachmentPerformedThisFrame() )
				{
					m_entityPool->Update();
				}
			}

			// Process npcs and camera collisions
			{
				PC_SCOPE_PIX( NpcCameraCollision );
				ProcessNpcsAndCameraCollisions();
			}
		}
		else
		{
			// paused update
			{
				PC_SCOPE_PIX( GameSystemsTick );

				for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin(); gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
				{
					IGameSystem* gameSystem = *gameSystemIter;
					if ( gameSystem != NULL )
					{
						gameSystem->PausedTick();
					}
				}
			}
		}

		// Fire scripted anim events
		{
			PC_SCOPE_PIX( FireScriptedAnimEvents );
			CGameplayEntity::ScriptAnimEventManager::FireQueuedEvents();
		}

		// Update item entities
		{
			PC_SCOPE_PIX( ItemManagerTick );
			SItemEntityManager::GetInstance().OnTick( timeDelta );
		}
	}
}

void CCommonGame::OnTickDuringPostUpdateTransforms( Float timeDelta, Bool updateGui )
{
	if ( m_guiManager && updateGui )
	{
		// Tick GUI itself with engine time delta, not world
		PC_SCOPE_PIX( GUIManagerTick );
		m_guiManager->Tick( GEngine->GetLastTimeDelta() );
	}

	if ( m_flashPlayer && updateGui )
	{
		// Tick Flash with engine time delta, not world
		PC_SCOPE_PIX( FlashPlayerTick );
		Rect viewport;
		if( Config::cvForcedRendererOverlayResolution.Get() )
		{
			viewport.m_right = Config::cvForcedRendererResolutionWidth.Get();
			viewport.m_bottom = Config::cvForcedRendererResolutionHeight.Get();
			viewport.m_left = 0;
			viewport.m_top = 0;
		}
		else
		{
			viewport.m_left = GGame->GetViewport()->GetX();
			viewport.m_top  = GGame->GetViewport()->GetY();
			viewport.m_right  = viewport.m_left + GGame->GetViewport()->GetWidth();
			viewport.m_bottom = viewport.m_top +  GGame->GetViewport()->GetHeight();
		}		

		m_flashPlayer->Tick( GEngine->GetLastTimeDelta(), viewport );
	}
}

void  CCommonGame::InitReactionManager()
{
	// Create beh tree reaction manager instance
	m_behTreeReactionManager = CreateObject<CBehTreeReactionManager>( this );
	m_behTreeReactionManager->Init();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CCommonGame::OnGameplaySystemsGameStart( const CGameInfo& info )
{
	TBaseClass::OnGameplaySystemsGameStart( info );
	
	// Attach DLC content - before startup of the systems
	m_dlcManager->OnGameStarting();
	
	for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin();
		gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
	{
		IGameSystem* gameSystem = *gameSystemIter;
		if ( gameSystem != NULL )
		{
			gameSystem->OnGameStart( info );
		}
	}
	
	// Create pools for entites
	m_entityPool = CreateEntityPool( GetActiveWorld() );

	if ( !m_templateLoadBalancer )
	{
		m_templateLoadBalancer = new CTemplateLoadBalancer();
	}

	// Create the reactions manager instance
	m_reactionsManager = CreateObject< CReactionsManager >( this );

	InitReactionManager();

	// initialize the global path planner
	m_pathPlanner = new CMoveGlobalPathPlanner();

	if ( m_guiManager )
	{
		m_guiManager->OnGameStart( info );
	}

	m_actorsManager->OnGameStart( m_activeWorld );

	// Initialize cache for localized strings with keys
	SLocalizationManager::GetInstance().InitializeLocStringsWithKeysCache();

	// Inform scripts
	CallEvent( CNAME( OnGameStarting ), info.m_gameLoadStream ? true : false );

	// HACK SWARMS
	// Preload all temppalte and attach to root set so that they do not unload
	m_boidSpecies->PreloadTemplates();

	m_encounerSpawnGroupCounter.ResetConters();

	UnlockMissedAchievements( info );
}

void CCommonGame::OnGameplaySystemsWorldStart( const CGameInfo& info )
{
	TBaseClass::OnGameplaySystemsWorldStart( info );

	for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin();
		gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
	{
		IGameSystem* gameSystem = *gameSystemIter;
		if ( gameSystem != NULL )
		{
			gameSystem->OnWorldStart( info );
		}
	}

	if ( m_guiManager )
	{
		m_guiManager->OnWorldStart( info );
	}

	// Notify env manager
	CGameWorld* world = GetActiveWorld();
	ASSERT( world->GetEnvironmentManager() );
	world->GetEnvironmentManager()->OnGameStart( info );

	m_dlcManager->OnGameStarted();

	// Inform scripts
	CallEvent( CNAME( OnGameStarted ), info.m_gameLoadStream ? true : false );
}

void CCommonGame::OnAfterLoadingScreenGameStart( const CGameInfo& info )
{
	TBaseClass::OnAfterLoadingScreenGameStart( info );

	CGameWorld* world = GetActiveWorld();
	ASSERT( world->GetEnvironmentManager() );
	world->GetEnvironmentManager()->OnAfterLoadingScreenGameStart( info );

	CallEvent( CNAME( OnAfterLoadingScreenGameStart ));
	m_gameSaver->OnWorldStart();
}

void CCommonGame::OnGameplaySystemsWorldEnd( const CGameInfo& info )
{
	for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin();
		gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
	{
		IGameSystem* gameSystem = *gameSystemIter;
		if ( gameSystem != NULL )
		{
			gameSystem->OnWorldEnd( info );
		}
	}

	if ( m_guiManager )
	{
		m_guiManager->OnWorldEnd( info );
	}

	if ( m_createEntityManager )
	{
		m_createEntityManager->OnWorldEnd();
	}

	CGameWorld* world = GetActiveWorld();

	ASSERT( world && world->GetEnvironmentManager() );

	// Notify env manager
	if ( world && world->GetEnvironmentManager() )
	{
		world->GetEnvironmentManager()->OnShutdownAtGameEnd( );
	}

	// Clean the world
	if ( world )
	{
		world->OnShutdownAtGameEnd();
	}

	// Null player
	SetPlayer( NULL );

	// Release all item entities data
	SItemEntityManager::GetInstance().DestroyAll();

	TBaseClass::OnGameplaySystemsWorldEnd( info );
}

void CCommonGame::OnGameplaySystemsGameEnd( const CGameInfo& info )
{
	// Inform scripts
	CallEvent( CNAME( OnGameEnded ) );

	// Unmount DLC content
	m_dlcManager->OnGameEnding();

	for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin();
		gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
	{
		IGameSystem* gameSystem = *gameSystemIter;
		if ( gameSystem != NULL )
		{
			gameSystem->OnGameEnd( info );
		}
	}

	if ( m_guiManager )
	{
		m_guiManager->OnGameEnd( info );
	}

	// Delete reactions manager
	if ( m_reactionsManager )
	{
		m_reactionsManager->Discard();
		m_reactionsManager = NULL;
	}

	// Delete beh tree reactions manager
	if( m_behTreeReactionManager )
	{
		m_behTreeReactionManager->Discard();
		m_behTreeReactionManager = NULL;
	}

	if ( m_pathPlanner )
	{
		// Delete the global path planner
		delete m_pathPlanner;
		m_pathPlanner = NULL;
	}

	if( m_entityPool )
	{
		m_entityPool->Shutdown();
		m_entityPool.Reset();
	}

	if ( m_templateLoadBalancer )
	{
		m_templateLoadBalancer->BreakProcessing();
	}

	// Scripted anim events
	CGameplayEntity::ScriptAnimEventManager::ClearQueuedEvents();

	// HACK SWARMS
	// Preload all temppalte and attach to root set so that they do not unload
	m_boidSpecies->UnloadTemplates();

	TBaseClass::OnGameplaySystemsGameEnd( info );
}

void CCommonGame::OnGameplaySystemsInitialize()
{
	for ( IGameSystem* iter : m_gameSystems )
	{
		if( iter )
		{
			iter->Initialize();
		}
	}
}

void CCommonGame::OnGameplaySystemsShutdown()
{
	for ( IGameSystem* iter : m_gameSystems )
	{
		if( iter )
		{
			iter->Shutdown();
		}		
	}

	if ( m_templateLoadBalancer )
	{
		m_templateLoadBalancer->BreakProcessing();
		delete m_templateLoadBalancer;
		m_templateLoadBalancer = nullptr;
	}
}

void CCommonGame::OnGameDifficultyChanged( Uint32 previousDifficulty, Uint32 currentDifficulty )
{
	for ( ActorIterator actor = ActorIterator(); actor; ++actor )
	{
		( *actor )->CallEvent( CNAME( OnGameDifficultyChanged ), previousDifficulty, currentDifficulty );
	}
}

void CCommonGame::PerformCommunitiesFastForward( const Vector& referencePosition, Float timeLimit, Bool resimulateCommunities )
{
	CGameFastForwardSystem* fastForward = GetSystem< CGameFastForwardSystem >();
	if ( !fastForward )
	{
		return;
	}

	SFastForwardExecutionParameters executionParams( referencePosition );
	executionParams.m_dontSpawnHostilesClose = true;
	executionParams.m_isExternallyTicked = true;
	executionParams.m_fastForwardSpawnTrees = resimulateCommunities;

	fastForward->BeginFastForward( Move( executionParams ) );
	{
		CTimeCounter timer;
		Bool requestedTermination = false;
		while ( fastForward->ExternalTick() )
		{
			if ( !requestedTermination && timer.GetTimePeriod() > timeLimit )
			{
				fastForward->RequestFastForwardShutdown();
				requestedTermination = true;
			}
			SDeferredDataOOMQueue::GetInstance().Reschedule();
			SDeferredDataBufferKickOffList::GetInstance().KickNewJobs();
			PUMP_MESSAGES_DURANGO_CERTHACK();
		}
	}

	fastForward->EndFastForward();

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CEntity* CCommonGame::CreatePlayer( const CGameInfo& info )
{
	CPlayer* player = NULL;

	const Bool isPlayerAllowed = Red::System::StringSearch( SGetCommandLine(), TXT("-noplayer") ) == NULL;
	if ( isPlayerAllowed )
	{
		// By default spawn player at the camera position
		Vector position = info.m_cameraPosition;
		EulerAngles rotation = EulerAngles( 0, 0, info.m_cameraRotation.Yaw );

		String templatePath;

		IGameLoader* loadStream = info.m_playerLoadStream ? info.m_playerLoadStream : info.m_gameLoadStream; 

		// Load player position and template path
		if ( info.m_isChangingWorldsInGame && CTeleportHelper( &info.m_teleport ).GetPlayerStartingPoint( position, rotation ) )
		{
			// Done
		}
		else if ( loadStream )
		{
			if ( loadStream->GetSaveVersion() < SAVE_VERSION_INCLUDE_PLAYER_STATE )
			{
				CGameSaverBlock block( loadStream, CNAME(playerSpawn) );
				loadStream->ReadValue( CNAME(position), position );
				loadStream->ReadValue( CNAME(rotation), rotation );
				loadStream->ReadValue( CNAME(template), templatePath );
			}
			else
			{
				const CActivePlayerStorage& playerStorage = m_universeStorage.GetActivePlayerStorage();
				const Bool isNewGamePlus = info.m_playerLoadStream != nullptr;
				if ( isNewGamePlus )
				{
					GetGameResource()->GetStartingPoint( position, rotation );
				}
				else
				{
					position = playerStorage.GetPosition();
					rotation = playerStorage.GetRotation();
				}
				templatePath = playerStorage.GetTemplatePlath();
			}
		}
		else if ( !info.m_keepExistingLayers )
		{
		 	// But try to find a spawn point (doesn't touch the variables if not filled in game res)
			GetGameResource()->GetStartingPoint( position, rotation );
		}

		// Load player template
		THandle< CEntityTemplate > playerTemplate( NULL );

		// Default?
		if ( templatePath.Empty() )
		{
			playerTemplate = GetGameResource()->GetPlayerTemplate().Get();
		}
		else
		{
			// Load template resource from the given path
			playerTemplate = LoadResource< CEntityTemplate >( templatePath );
		}

		if ( playerTemplate )
		{
			EntitySpawnInfo einfo;
			einfo.m_spawnPosition = position;
			einfo.m_spawnRotation = rotation;
			einfo.m_detachTemplate = false;
			einfo.m_template = playerTemplate;
			einfo.m_idTag = m_universeStorage.GetActivePlayerStorage().GetIdTag();

			// Create player
			player = Cast< CPlayer >( GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo ) );
			if ( player )
			{
				// Add basic tags
				TagList tags = player->GetTags();
				tags.AddTag( CNAME( PLAYER ) );
				player->SetTags( tags );

				// if we are not Geralt, disable player attachments, they will be enabled after we ChangePlayer() to Geralt again
				if ( !IsPrimaryPlayer( player ) )
				{
					m_universeStorage.DisablePlayerAttachments();
				}

				// restore player's managed attachments
				m_universeStorage.RestorePlayerManagedAttachments( player );

				// Register as player
				SetPlayer( player );

				return player;
			}
		}
	}

	// No player created
	return player;
}

CEntity* CCommonGame::ChangePlayer()
{
	ASSERT( !m_playerChangeInfo.m_alias.Empty() );

	EntitySpawnInfo einfo;
	einfo.m_detachTemplate = false;

	if ( m_player )
	{
		einfo.m_spawnPosition = m_player->GetWorldPosition();
		einfo.m_spawnRotation = m_player->GetWorldRotation();
	}
	else 
	{
		einfo.m_spawnPosition = Vector::ZERO_3D_POINT;
		einfo.m_spawnRotation = EulerAngles::ZEROS;
	}

	// Notify that the player is about to be changed
	OnPreChangePlayer( einfo );

	// Make sure that id is valid
	if( !einfo.m_idTag.IsValid() )
	{
		einfo.m_idTag = m_idTagManager->GetReservedId( 0 );
	}

	THandle< CPlayer > tempPlayer = m_player;
		
	SetPlayer( NULL );

	if ( tempPlayer )
	{
		tempPlayer->Destroy();
	}
	
	m_activeWorld->DelayedActions();

	SGarbageCollector::GetInstance().CollectNow();


#ifndef RED_FINAL_BUILD
	// Now we need to test if we can remove the player to prevent a crash
	CObject* ignoredOwner = m_activeWorld->GetDynamicLayer();
	if ( tempPlayer && CObject::TestObjectReachability( tempPlayer.Get(), ignoredOwner ) )
	{
		CObject::PrintDependencies( CNAME( CPlayer ) );

		HALT( "Player can not be replaced because something is holding its reference." );

		// Set the player back
		SetPlayer( tempPlayer.Get() );
		GetSystem< CStorySceneSystem >()->GetActorMap()->RegisterSpeaker( tempPlayer.Get(), tempPlayer->GetVoiceTag() );

		return nullptr;
	}
#endif

	const String aliasFilename(CResourceDefManager::RESDEF_PROTOCOL + m_playerChangeInfo.m_alias);
	einfo.m_template = LoadResource< CEntityTemplate >( aliasFilename );
	ASSERT( einfo.m_template );
	if ( !einfo.m_template )
	{
		HALT( "Given replacer entity template alias ( %s ) is invalid", m_playerChangeInfo.m_alias.AsChar() );
		return nullptr;
	}
	if ( !einfo.m_template->GetEntityObject()->IsA< CPlayer >() )
	{
		HALT( "Given Replacer entity template is not a CPlayer" );
	}

	if( m_playerChangeInfo.m_appearance )
	{
		einfo.m_appearances.PushBack( m_playerChangeInfo.m_appearance );
	}

	m_playerChangeInfo.m_alias.Clear();

	// Create player
	CPlayer* player = Cast< CPlayer >( GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo ) );
	if ( player )
	{
		// Add basic tags
		TagList tags = player->GetTags();
		tags.AddTag( CNAME( PLAYER ) );
		player->SetTags( tags );

		// Register as player
		SetPlayer( player );

		GetActiveWorld()->GetComponentLODManager().ForceUpdateAll();

		// Notify that the player has been changed
		OnPostChangePlayer();

		return player;
	}

	return NULL;
}

CEntity* CCommonGame::CreateCamera() const
{
	// Create game camera
	ASSERT( m_activeWorld && m_activeWorld->GetDynamicLayer() );

	CEntityTemplate* cameraTemplate = NULL;

	if ( GetGameResource() )
	{
		cameraTemplate = GetGameResource()->GetCameraTemplate().Get();
	}

	if ( cameraTemplate )
	{
		EntitySpawnInfo einfo;
		einfo.m_detachTemplate = false;
		einfo.m_template = cameraTemplate;

		CEntity* camera = m_activeWorld->GetDynamicLayer()->CreateEntitySync( einfo );
		if ( camera )
		{
			// Add tag - for behavior debugger
			TagList tags = camera->GetTags();
			tags.AddTag( CNAME( CAMERA ) );
			camera->SetTags(tags);
			return camera;
		}
		else
		{
			WARN_GAME( TXT("World: Couldn't create camera") );
		}
	}
	else
	{
		WARN_GAME( TXT("Game: Couldn't create camera - no %s resource"), GetGameResource() ? GetGameResource()->GetCameraTemplate().GetPath().AsChar() : TXT("game") );
	}

	return NULL;
}

void CCommonGame::OnAttachGameplayEntity( CGameplayEntity* entity )
{
	// inform the actors storage that a new entity was added
	CGameplayStorage* gameplayStorage = GetGameplayStorage();
	if ( gameplayStorage )
	{
		gameplayStorage->Add( entity );
	}

	CActor* actor = Cast< CActor >( entity );
	if ( actor != NULL )
	{
		RegisterActor( actor );
	}
}

void CCommonGame::OnDetachGameplayEntity( CGameplayEntity* entity )
{
	RED_ASSERT( !m_flushGameplayStorageUpdatesTask );

	// inform the actors storage that an entity was removed
	CGameplayStorage* gameplayStorage = GetGameplayStorage();
	if ( gameplayStorage )
	{
		gameplayStorage->Remove( entity );
	}

	// removing entity from "to update" list
	// if there's overflow on the list, it won't be iterated so we don't
	// need to remove dangling references
	if ( !m_gameplayStorageUpdate.IsOverflow() )
	{
		const Uint32 size = m_gameplayStorageUpdate.Size();
		for ( Uint32 i = 0; i < size; ++i )
		{
			if ( m_gameplayStorageUpdate[ i ] == entity )
			{
				m_gameplayStorageUpdate.ResetAt( i );
			}
		}

	}

	CActor* actor = Cast< CActor >( entity );
	if ( actor != NULL )
	{
		UnregisterActor( actor );
	}
}

void CCommonGame::OnUpdateTransformBegin()
{
	TBaseClass::OnUpdateTransformBegin();
}

void CCommonGame::OnUpdateTransformFinished( Bool isMainUpdateTransformPass )
{
	TBaseClass::OnUpdateTransformFinished( isMainUpdateTransformPass );

	FlushGameplayStorageUpdates( isMainUpdateTransformPass );
}

void CCommonGame::RegisterEntityForGameplayStorageUpdate( CGameplayEntity* entity )
{
	RED_ASSERT( !m_flushGameplayStorageUpdatesTask );
	RED_ASSERT( entity );
	if ( entity->IsInGame() )
	{
		m_gameplayStorageUpdate.AddAsync( entity );
	}
}

void CCommonGame::RegisterEntityForActorManagerUpdate( CActor* actor )
{
	RED_ASSERT( !m_flushGameplayStorageUpdatesTask );
	RED_ASSERT( actor );
	if ( actor->IsInGame() )
	{
		m_actorsManagerUpdate.AddAsync( actor );
	}
}

void CCommonGame::UnloadWorld()
{
	TBaseClass::UnloadWorld();

	RED_ASSERT( m_gameplayStorage->Size() == 0 );
	RED_ASSERT( m_actorsManager->Size() == 0 );

	m_gameplayStorage->ClearFast();
	m_actorsManager->ClearFast();
}

void CCommonGame::FlushGameplayStorageUpdates( Bool async )
{
	RED_ASSERT( !m_flushGameplayStorageUpdatesTask );

	if ( async )
	{
		PC_SCOPE_PIX( FlushGameplayStorageUpdates_StartAsync );

		m_flushGameplayStorageUpdatesTask = new ( CTask::Root ) CFlushGameplayStorageUpdatesTask( m_gameplayStorage, m_gameplayStorageUpdate, m_actorsManager, m_actorsManagerUpdate );

		GTaskManager->Issue( *m_flushGameplayStorageUpdatesTask, TSP_Critical );
	}
	else
	{
		PC_SCOPE_PIX( FlushGameplayStorageUpdates_Sync );

		CFlushGameplayStorageUpdatesTask::ProcessUpdatePosition( m_gameplayStorage, m_gameplayStorageUpdate, m_gameplayStorageUpdate.IsOverflow() );
		CFlushGameplayStorageUpdatesTask::ProcessUpdatePosition( m_actorsManager, m_actorsManagerUpdate, m_actorsManagerUpdate.IsOverflow() );
	}

	m_gameplayStorageUpdate.Reset();
	m_actorsManagerUpdate.Reset();
}

void CCommonGame::FinalizeFlushGameplayStorageUpdates()
{
	if ( m_flushGameplayStorageUpdatesTask )
	{
		PC_SCOPE_PIX( FlushGameplayStorageUpdates_WaitAsync );

		// if we reached this point and the task is not running yet then 
		CTaskDispatcher taskDispatcher( *GTaskManager );
		CTaskRunner taskRunner;
		taskRunner.RunTask( *m_flushGameplayStorageUpdatesTask, taskDispatcher );

		m_flushGameplayStorageUpdatesTask->WaitForFinish();
		m_flushGameplayStorageUpdatesTask->Release();
		m_flushGameplayStorageUpdatesTask = nullptr;
	}
}

void CCommonGame::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin();
		gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
	{
		IGameSystem* gameSystem = *gameSystemIter;
		if ( gameSystem != NULL )
		{
			file << gameSystem;
		}
	}

	if ( m_definitionsManagerAccessor )
	{
		file << m_definitionsManagerAccessor;
	}

	if ( m_reactionsManager )
	{
		file << m_reactionsManager;
	}

	if( m_behTreeReactionManager )
	{
		file << m_behTreeReactionManager;
	}

	if ( m_templateLoadBalancer )
	{
		m_templateLoadBalancer->OnSerialize( file );
	}

	if ( m_guiManager )
	{
		file << m_guiManager;
	}

	if( file.IsGarbageCollector() )
	{
		if( m_entityPool )
		{
			m_entityPool->CollectForGC( file );
		}

		if( GAnimationManager )
		{
			GAnimationManager->SanitizePoseProvider();
		}
	}

	// Tell GC what templates are still needed
	SItemEntityManager::GetInstance().OnSerialize( file );
}

Bool CCommonGame::IsPlayingCachetDialog() const
{
	CStorySceneSystem* sceneSystem = GetSystem< CStorySceneSystem >();
	return sceneSystem ? sceneSystem->IsDialogHudShown() : false;
}

void CCommonGame::SetPlayer( CPlayer* player )
{
	// Change player
	m_player = player;

	// Register player in the scripting system
	ASSERT( GScriptingSystem );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_PLAYER, player );
}

CEntity* CCommonGame::GetPlayerEntity() const
{
	return m_player.Get();
}

 void CCommonGame::OnLanguageChange()
 {
	 CGame::OnLanguageChange();

	 if (m_guiManager != NULL)
	 {
		 m_guiManager->RefreshMenu();
	 }
 }

CClass* CCommonGame::GetGameWorldClass() const
{
	return ClassID< CGameWorld >();
}

void CCommonGame::RequestGameSave( ESaveGameType type, Int16 slot, const String& reason )
{
	if ( m_gameSaver->GetRequestedSaveType() != SGT_None )
	{
		ASSERT( ! "Already saving" );
		return;
	}

	CallEvent( CNAME( OnSaveStarted ), type );

	m_gameSaver->RequestSaveGame( type, slot, reason );
}

Bool CCommonGame::IsSavingGame() const
{
	return m_gameSaver->GetRequestedSaveType() != SGT_None;
}

Bool CCommonGame::HandleWorldChange()
{
	if ( m_changeWorldInfo.IsValid() )
	{
		if( GetPlayer() && GetPlayer()->HandleWorldChangeOnBoat()  )
		{
			if( !m_HACK_P_1_1_worldChangeFromQuest )
			{
				CallEvent( CNAME( OnHandleWorldChange ) );
			}
		}

		m_HACK_P_1_1_worldChangeFromQuest = false;

		CWorld* activeWorld = m_activeWorld.Get();
		ASSERT( activeWorld );

		const String worldName = m_changeWorldInfo.m_worldName;
		const String videoToPlay = m_changeWorldInfo.m_videoToPlay;

		m_changeWorldInfo.Reset();

		SGameSessionManager::GetInstance().CreateSession( worldName, true, &m_changeWorldInfo.m_teleport, videoToPlay );
	}
	return false;
}

void CCommonGame::ScheduleWorldChange( SChangeWorldInfo& changeWorldInfo )
{
	CallEvent( CNAME( OnBeforeWorldChange ), changeWorldInfo.m_worldName );

	m_changeWorldInfo = changeWorldInfo;
}

bool CCommonGame::AnalogScaleformEnabled()
{
	if (m_guiManager && m_guiManager->IsAnyMenu())
	{
		return true;
	}

	return m_forceEnableUIAnalog > 0;
}

const CMenu* CCommonGame::GetRootMenu() const
{
	return m_guiManager->GetRootMenu();
}

void CCommonGame::ProcessNpcsAndCameraCollisions()
{
	CWorld* activeWorld = GetActiveWorld();

	const CNode* playerVehicle( nullptr );
	if ( const CEntity* playerEntity = GGame->GetPlayerEntity() )
	{
		if ( const CHardAttachment* parentAttachment = playerEntity->GetTransformParent() )
		{
			playerVehicle = parentAttachment->GetParent();
		}
	}

	// 1. Active camera is not from cutscene
	if ( GetGameplayConfig().m_processNpcsAndCameraCollisions && activeWorld && !IsPlayingCameraCutscene() )
	{
		CEnvironmentManager* envManager = activeWorld->GetEnvironmentManager();
		if ( envManager )
		{
			const Float cameraRadius = Clamp( 1.5f * envManager->GetNearPlane(), 0.4f, 1.f );
			const Vector& cameraPosWS = activeWorld->GetCameraDirector()->GetCameraPosition();
			Sphere cameraShpere( cameraPosWS, cameraRadius );

			static Float offset = 5.f;
			const Vector range( offset + cameraRadius, offset + cameraRadius, offset + cameraRadius );

			m_actorsOutput.ClearFast();
			STATIC_NODE_FILTER( IsNotPlayer, filterNotPlayer );
			static const INodeFilter* filters[] = { &filterNotPlayer };
			GetActorsManager()->GetClosestToPoint( cameraPosWS, m_actorsOutput, Box( -range, range ), INT_MAX, filters, 1 );

			const Uint32 size = m_actorsOutput.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				CActor* actor = m_actorsOutput[ i ].Get();

				// 2. Actor is not in scene ( not gameplay )
				if ( actor && !actor->IsInNonGameplayScene() && actor->GetCurrentStateName() != CNAME( CombatFistStatic ) )
				{
					CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
					CMovingPhysicalAgentComponent* mhac = Cast< CMovingPhysicalAgentComponent >( mac );

					// 3. Actor is not in cutscene
					if ( mhac && !mhac->IsInCutscene() )
					{
						const Bool shouldHideActor = ShouldHideActor( mhac, cameraShpere );
						// 4. Actor's capsule collide with camera sphere
						if ( shouldHideActor )
						{
							HideAllRenderablesTemporarily( actor );

							// 5. Is player's vehicle
							if ( playerVehicle && actor == playerVehicle )
							{
								HideAllRenderablesTemporarily( Cast< CActor >( GGame->GetPlayerEntity() ) );
								playerVehicle = nullptr;
							}
						}
					}
				}
				else
				{
					ASSERT( actor );
				}
			}

			m_actorsOutput.ClearFast();
		}
	}
}

void CCommonGame::HideAllRenderablesTemporarily(CActor* actor)
{
	if( !actor ) return;

	for ( EntityWithItemsComponentIterator< CMeshTypeComponent > it( actor ); it; ++it )
	{
		CMeshTypeComponent* c = *it;

		if ( c->GetRenderProxy() )
		{
			( new CRenderCommand_SetTemporaryFade( c->GetRenderProxy() ) )->Commit();
		}
	}

	// RPT_None == process all render proxies.
	for ( RenderProxyActorWithItemsIterator it( actor, RPT_None ); it; ++it )
	{
		IRenderProxy* proxy = *it;
		( new CRenderCommand_SetTemporaryFade( proxy ) )->Commit();
	}
}

//////////////////////////////////////////////////////////////
Bool CCommonGame::ShouldHideActor( const CMovingPhysicalAgentComponent* const mpac, const Sphere& cameraShpere ) const
{
#ifdef USE_PHYSX
	if ( mpac->IsRagdolled( true ) ) // unfortunately, we use different bounding shapes in different circumstances, can be considered as a HACK
	{
		Box boundingBox;

		if( mpac->GetPhysicalCharacter() && mpac->GetPhysicalCharacter()->GetCharacterController() )
		{
#ifdef USE_PHYSX
			const Float height = mpac->GetPhysicalCharacter()->GetCharacterController()->GetCurrentHeight();
			const Float radius = mpac->GetPhysicalCharacter()->GetCharacterController()->GetCurrentCharacterRadius();

			// calc size of bb, based on some experimental heuristics:
			const Vector span( height/2, height/2, radius/2 );

			boundingBox.Max = mpac->GetWorldPosition() + span;
			boundingBox.Min = mpac->GetWorldPosition() - span;
			mpac->GetLocalToWorld().TransformBox( boundingBox );
#endif
		}

		return boundingBox.IntersectSphere( cameraShpere );
	}
	else
#endif // USE_PHYSX
	{
		FixedCapsule capsule;
		mpac->CreateCharacterCapsuleWS( capsule );

		return capsule.Contains( cameraShpere ) ;
	}
}

void CCommonGame::EnableFreeCamera( Bool enable )
{
	// TODO TOMEK_C MERGE
	TBaseClass::EnableFreeCamera( enable );

	if ( m_freeCameraActive )
	{
		// Reset position
		if ( m_player )
		{
			m_freeCamera->MoveTo( m_player->GetWorldPosition() + Vector( 0.0f, 0.0f, 2.0f ), m_player->GetWorldRotation() );
		}
	}
}

CCommunitySystem* CCommonGame::GetCommunitySystem()
{
	return GetSystem< CCommunitySystem >();
}

void CCommonGame::RegisterNPC( CNewNPC* npc )
{
	TNPCCharacters::iterator iter = Find( m_npcCharacters.Begin(), m_npcCharacters.End(), npc);
	if( iter == m_npcCharacters.End() )
	{
		m_npcCharacters.PushBack( npc );
	}
	else
	{
		ASSERT( 0 , TXT("NPC already present") );
	}
}

void CCommonGame::UnregisterNPC( CNewNPC* npc )
{
	TNPCCharacters::iterator iter = Find( m_npcCharacters.Begin(), m_npcCharacters.End(), npc);
	if( iter != m_npcCharacters.End() )
	{
		m_npcCharacters.Erase( iter );
	}
}

void CCommonGame::RegisterActor( CActor* actor )
{
	TActors::iterator iter = Find( m_actors.Begin(), m_actors.End(), actor );
	if( iter == m_actors.End() )
	{
		m_actors.PushBack( actor );
	}
	else
	{
		ASSERT( 0 , TXT("Actor already present") );
	}

	CNewNPC* newNpc = Cast< CNewNPC >( actor );
	if( newNpc )
	{
		RegisterNPC( newNpc );
	}
}

void CCommonGame::UnregisterActor( CActor* actor )
{
	TActors::iterator iter = Find( m_actors.Begin(), m_actors.End(), actor );
	if( iter != m_actors.End() )
	{
		m_actors.Erase( iter );
	}

	CNewNPC* newNpc = Cast< CNewNPC >( actor );
	if( newNpc )
	{
		UnregisterNPC( newNpc );
	}

	// removing actor from "to update" list
	// if there's overflow on the list, it won't be iterated so we don't
	// need to remove dangling references
	if ( !m_actorsManagerUpdate.IsOverflow() )
	{
		const Uint32 size = m_actorsManagerUpdate.Size();
		for ( Uint32 i = 0; i < size; ++i )
		{
			if ( m_actorsManagerUpdate[ i ] == actor )
			{
				m_actorsManagerUpdate.ResetAt( i );
			}
		}
	}
}

void CCommonGame::RegisterBehTreeMachine( CBehTreeMachine* machine )
{
	TBehTreeMachines::iterator iter = Find( m_behTreeMachines.Begin(), m_behTreeMachines.End(), machine );
	if( iter == m_behTreeMachines.End() )
	{
		m_behTreeMachines.PushBack( machine );
	}
}

void CCommonGame::UnregisterBehTreeMachine( CBehTreeMachine* machine )
{
	m_behTreeMachines.Remove( machine );
}

void CCommonGame::TickBehTreeMachines( Float timeDelta )
{
	TBehTreeMachines::iterator iter = m_behTreeMachines.Begin();
	for( ; iter != m_behTreeMachines.End(); ++iter )
	{
		(*iter)->Tick( timeDelta );
	}
}

void CCommonGame::RegisterStripeComponent( CStripeComponent* stripe )
{
	if ( CRoadsManager* const roadsManager = GetSystem< CRoadsManager >() )
	{
		roadsManager->RegisterRoad( stripe );
	}
	else
	{
		WARN_GAME( TXT("Attempting to register stripe component when there's no road manager") );
	}
}

void CCommonGame::UnregisterStripeComponent( CStripeComponent* stripe )
{
	if ( CRoadsManager* const roadsManager = GetSystem< CRoadsManager >() )
	{
		roadsManager->UnregisterRoad( stripe );
	}
	else
	{
		WARN_GAME( TXT("Attempting to unregister stripe component when there's no road manager") );
	}
}


Bool CCommonGame::AreSubtitlesEnabled() const
{
	return Config::cvSubtitles.Get();
}


String CCommonGame::NumberToLocDifficulty( Int32 difficulty )
{
	switch( difficulty )
	{
	case 0:
		return SLocalizationManager::GetInstance().GetStringByStringKey( TXT( "menuDifficultyEasy" ) );
	case 1:
		return SLocalizationManager::GetInstance().GetStringByStringKey( TXT( "menuDifficultyMedium" ) );
	case 2:
		return SLocalizationManager::GetInstance().GetStringByStringKey( TXT( "menuDifficultyHard" ) );
	case 3:
		return SLocalizationManager::GetInstance().GetStringByStringKey( TXT( "menuDifficultyInsane" ) );
	case 4:
		return SLocalizationManager::GetInstance().GetStringByStringKey( TXT( "menuDifficultyVeryHard" ) );
	default:
		ASSERT( ! "Should not be here!" );
		return String();
	};
}

void CCommonGame::GiveRewardTo( CName rewardName, CName  targetTag  )
{
	TDynArray<CEntity*> entities;
	GetActiveWorld()->GetTagManager()->CollectTaggedEntities( targetTag, entities );
	for ( auto i = entities.Begin(); i != entities.End(); ++i )
	{
		GiveRewardTo( rewardName, *i );
	}
}

void CCommonGame::GiveRewardTo( CName rewardName, CEntity * targetEnt  )
{
	if( targetEnt )
	{
		const SReward* reward = GetReward( rewardName );
		if ( reward )
		{
			CallEvent( CNAME( OnGiveReward ), THandle< CEntity >( targetEnt ), rewardName, *reward );

			if( reward->m_script )
			{
				CFunction* function = SRTTI::GetInstance().FindGlobalFunction( reward->m_script );

				ASSERT( function );
				ASSERT( function->IsReward() );

				if( function )
				{
					// Allocate param space
					const Uint32 stackSize = function->GetStackSize();
					Uint8* newStack = (Uint8*) RED_ALLOCA( stackSize );
					Red::System::MemorySet( newStack, 0, stackSize );

					function->Call( NULL, newStack, NULL );
				}
			}
		}
		else
		{
			RED_ASSERT( false, TXT("There is no reward with name '%s'"), rewardName.AsChar() );
		}
	}
}

#ifdef REWARD_EDITOR
const SReward* CCommonGame::GetReward( CName rewardName )
{
	// Cooked game will handle rewards differently
	// but for a moment it will have to work like this - TODO
	CDirectory* dir = GDepot->FindPath( REWARDS_DIR );
	if( dir )
	{
		for( CRewardDirectoryIterator it( *dir, true ); it; ++it )
		{
			const TDynArray<SReward>& rewards = (*it)->GetRewards();
			TDynArray< SReward >::const_iterator end = rewards.End();
			for( TDynArray< SReward >::const_iterator rit = rewards.Begin(); rit != end; ++rit )
			{
				if( rit->m_name == rewardName )
				{
					return &(*rit);
				}
			}
		}
	}
	return nullptr;
}
#endif

void CCommonGame::RequestMenu( const CName& menuName, const THandle< IScriptable >& initData )
{
	if ( m_guiManager )
	{
		CGuiManager::SGuiEvent guiEvent;
		guiEvent.m_eventName = CNAME( OnRequestMenu );
		guiEvent.m_eventEx = menuName;
		if ( initData.Get() )
		{
			guiEvent.m_args.PushBack( SGuiEventArg(initData) );
		}

		m_guiManager->CallGuiEvent( guiEvent );
	}
}

void CCommonGame::CloseMenu( const CName& menuName )
{
	if ( m_guiManager )
	{
		CGuiManager::SGuiEvent guiEvent;
		guiEvent.m_eventName = CNAME( OnCloseMenu );
		guiEvent.m_eventEx = menuName;

		m_guiManager->CallGuiEvent( guiEvent );
	}
}

void CCommonGame::RequestPopup( const CName& popupName, const THandle< IScriptable >& initData )
{
	if ( m_guiManager )
	{
		CGuiManager::SGuiEvent guiEvent;
		guiEvent.m_eventName = CNAME( OnRequestPopup );
		guiEvent.m_eventEx = popupName;
		if ( initData.Get() )
		{
			guiEvent.m_args.PushBack( SGuiEventArg(initData) );
		}

		m_guiManager->CallGuiEvent( guiEvent );
	}
}

void CCommonGame::ClosePopup( const CName& popupName )
{
	if ( m_guiManager )
	{
		CGuiManager::SGuiEvent guiEvent;
		guiEvent.m_eventName = CNAME( OnClosePopup );
		guiEvent.m_eventEx = popupName;

		m_guiManager->CallGuiEvent( guiEvent );
	}
}

void CCommonGame::OnReloadedConfig()
{
	TBaseClass::OnReloadedConfig();

	if ( m_actorsManager )
	{
		m_actorsManager->OnReloadedConfig( &m_gameplayConfig );
	}
}

CClass* CCommonGame::CBehTreeInstanceClass()
{
	return CBehTreeInstance::GetStaticClass();
}

CLootDefinitions* CCommonGame::CreateLootDefinitions()
{
	return new CLootDefinitions();
}

ILootManager* CCommonGame::GetLootManager()
{
	return nullptr;
}

CInteractionsManager* CCommonGame::GetInteractionsManager()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

Bool CCommonGame::OnViewportTrack( const CMousePacket& packet )
{
	return TBaseClass::OnViewportTrack( packet );
}

Bool CCommonGame::OnViewportMouseMove( const CMousePacket& packet )
{
	return TBaseClass::OnViewportMouseMove( packet );
}

Bool CCommonGame::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
#ifndef NO_RED_GUI
	if(GRedGui::GetInstance().GetEnabled() == true)
	{
		if( GRedGui::GetInstance().OnViewportClick( view, button, state, x, y ) == true )
		{
			return true;
		}
	}
#endif	// NO_RED_GUI

	// Process debug pages stuff
#ifndef NO_DEBUG_PAGES
	if ( IsCheatEnabled( CHEAT_DebugPages ) )
	{
		if( IDebugPageManagerBase::GetInstance()->OnViewportClick( view, button, state, x, y ) )
		{
			return true;
		}
	}
#endif

#ifndef RED_FINAL_BUILD
	if ( button == 1 && state && Cheat_TeleportPlayer( view, x, y ) )
	{
		return true;
	}
#endif

	// Handle by base class
	return TBaseClass::OnViewportClick( view, button, state, x, y );
}

Bool CCommonGame::Cheat_TeleportPlayer( IViewport* view, Int32 x, Int32 y )
{
	if ( IsCheatEnabled( CHEAT_Teleport ) && IsActive() && IsPaused() )
	{
		CGuiManager* guiManager = GetGuiManager();
		if( !guiManager || !guiManager->IsAnyMenu() )
		{
			Vector clickedWorldPos;

			// Try to teleport player to clicked position
			if ( m_player && m_activeWorld->ConvertScreenToWorldCoordinates( view, x, y, clickedWorldPos ) )
			{
				Vector worldCoords = clickedWorldPos + Vector::EZ;
				CPhysicsWorld* physicsWorld = nullptr;
				if( m_activeWorld->GetPhysicsWorld( physicsWorld ) )
				{
					Vector camerPosition = GGame->GetFreeCamera().GetPosition();
					SPhysicsContactInfo cinfo;
					STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( CNAME( Character ) );
					CPhysicsWorld* physicsWorld = nullptr;
					if( m_activeWorld->GetPhysicsWorld( physicsWorld ) && physicsWorld->RayCastWithSingleResult( worldCoords, camerPosition, include, 0, cinfo ) == TRV_Hit )
					{
						// First check if physics trace hits anything
						worldCoords = cinfo.m_position;
					}
				}
				m_player->Teleport( worldCoords, m_player->GetRotation() );
				return true;
			}
		}
	}
	return false;
}

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

Bool CCommonGame::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( m_stopped 
#ifdef USE_ANSEL
		|| isAnselSessionActive
#endif // USE_ANSEL	
		)
	{
		// Pass to base class so can unstop when clicked
		return TBaseClass::OnViewportInput( m_viewport.Get(), key, action, data );
	}

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
	if( key == IK_F8 && action == IACT_Press )
	{
		if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			GRedGui::GetInstance().OnViewportSetDimensions( view, view->GetWidth(), view->GetHeight() );
			GDebugWin::GetInstance().SetVisible(!GRedGui::GetInstance().GetEnabled());

			if( GDebugWin::GetInstance().GetVisible() == true )
			{
				GRedGui::GetInstance().SetBudgetMode( false );
			}
		}
		else
		{
			Bool turningOn = !GDebugWin::GetInstance().IsDebugWindowVisible( DebugWindows::DW_SceneStats );
			GRedGui::GetInstance().OnViewportSetDimensions( view, view->GetWidth(), view->GetHeight() );

			GRedGui::GetInstance().SetBudgetMode( turningOn );

			if (  turningOn )
			{
				GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_SceneStats );
			}
			else
			{
				GDebugWin::GetInstance().HideDebugWindow( DebugWindows::DW_SceneStats );
			}
		}
	}
#endif	// NO_DEBUG_WINDOWS
	if(GRedGui::GetInstance().GetEnabled() == true)
	{
		if( GRedGui::GetInstance().OnViewportInput( view, key, action, data ) == true )
		{
			return true;
		}
	}
#endif	// NO_RED_GUI

	if ( m_inputManager )
	{
		if ( OnViewportInputDebugInGameOnly( view, key, action, data ) )
		{
			// If debug input is processed do not send game events
			m_inputManager->SuppressSendingEvents( true );
			return true;
		}
		else if( IsBlackscreen() )
		{
			m_inputManager->SuppressSendingEvents( true );
		}
		else if ( GUserProfileManager->HaveLockedUserActions() )
		{
			m_inputManager->SuppressSendingEvents( true );
		}
		else
		{
			m_inputManager->SuppressSendingEvents( false );
		}
	}

	{
		EInputKey saveKey = IK_F5;

#ifdef RED_PLATFORM_WINPC
		// it's pretty hard to debug without this
		if ( Red::System::Error::Handler::GetInstance()->IsDebuggerPresent() )
		{
			saveKey = IK_F6;
		}
#endif

		if ( key == saveKey && action == IACT_Press )
		{
			if ( SGameSessionManager::GetInstance().AreGameSavesLocked( SGT_QuickSave ) || IsAnyMenu() )
			{
				CallFunction( GetGuiManager(), CNAME( DisplayLockedSavePopup ) );
				SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( SGT_QuickSave, TXT("Saves were locked."), TXT("hotkey pressed") );
			}
			else
			{
				RequestGameSave( SGT_QuickSave, -1, TXT("hotkey pressed") );
				return true;
			}
		}
	}

#if !defined( RED_FINAL_BUILD )
	if( !GDebugConsole || !GDebugConsole->IsVisible() )
	{
		if ( !IsFreeCameraEnabled() )
		{

			if ( m_flashPlayer && GEngine->GetState() != BES_Constrained )		// When engine is constrained, then we don't want to get input for flash (which is the case when changing window position)
			{
				(void)m_flashPlayer->OnViewportInput( key, action, data );
			}

			if ( !m_stopped )
			{
				if ( m_inputManager )
				{
					BufferedInput input;
					input.PushBack( SBufferedInputEvent( key, action, data ) );
					m_inputManager->ProcessInput( input );
					// Don't return here because we need to allow the the base class
					// input to still get called and supress the inputManager... yes this is a mess!

					if ( IsPaused() )
					{
						//FIXME: Recursing here when paused and need to send the update for key releases etc
						// Not like SoftReset() does anything intuitive...
						m_inputManager->SoftReset(); // Should really be in the input manager when switching contexts...
						m_inputManager->SuppressSendingEvents( true );

						// the return below causes console not to get input when the game is paused, so we need to call it explicitly here
						if ( GDebugConsole )
						{
								GDebugConsole->OnViewportInput( view, key, action, data );
						}

						return true;
					}			
				}
			}
		}
		else if ( DoesFreeCameraProcessInputs() )
		{
			// If debug console or free camera is enabled do not send game events
			if( m_inputManager )
				m_inputManager->SuppressSendingEvents( true );
		}
	}
#else
	if ( m_flashPlayer )
	{
		(void)m_flashPlayer->OnViewportInput( key, action, data );
	}

	if ( !m_stopped )
	{
		if ( m_inputManager )
		{
			BufferedInput input;
			input.PushBack( SBufferedInputEvent( key, action, data ) );
			m_inputManager->ProcessInput( input );
		}
	}
#endif

	// Pass to base class
	return TBaseClass::OnViewportInput( m_viewport.Get(), key, action, data );
}

void CCommonGame::OnViewportKillFocus( IViewport* view )
{
	//FIXME2<<<: Wrong thread when the r4Game and questionable whether we should really do this anyway
// 	if ( m_inputManager )
// 	{
// 		m_inputManager->ResetInputs();
// 	}

#ifndef RED_FINAL_BUILD
	if ( ::SIsMainThread() )
	{
		// Need to do this to allow ForegroundNonexclusive and moving the mouse into another editor window with a viewport!
		// Since the viewport doesn't get the WM_KILLFOCUS at its level, it's faked in the WM_MOUSELEAVE
		Stop();
	}
#endif
}

void CCommonGame::OnViewportSetFocus( IViewport* view )
{
}

Bool CCommonGame::OnViewportInputDebugAlways( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
#ifndef RED_FINAL_BUILD
	if ( action == IACT_Press )
	{
		// Reload shaders
		if ( key == IK_F11 && !GIsEditor )
		{
			GRender->ReloadSimpleShaders();
		}

		#ifndef NO_SAVE_VERBOSITY
			// save while the game is not active calls stats gathering 
			if ( key == IK_F5 && !IsActive() )
			{
				GetGameSaver()->DebugDumpLevelStats();
			}
		#endif
	}
#endif

	// Debug console
#ifdef RED_FINAL_BUILD
	if( Config::cvDBGConsoleOn.Get() )
#endif
	{
		if( GGame && GGame->IsActive() )
		{
			if( GDebugConsole && GDebugConsole->OnViewportInput( view, key, action, data ) )
			{
				return true;
			}
		}
	}

	return false;
}

Bool CCommonGame::OnViewportInputDebugInGameOnly( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( OnViewportInputDebugAlways( view, key, action, data ) )
	{
		return true;
	}

	// HardExit
#ifndef RED_FINAL_BUILD
	if ( key == IK_F10 && action == IACT_Press )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		GDebugWin::GetInstance().HideAllVisibleWindows();
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
		RequestGameEnd();
		EDITOR_DISPATCH_EVENT( CNAME( GameEndRequested ), NULL );
		return true;
	}
#endif

#ifndef RED_FINAL_BUILD
	// Debug time scaling
	if ( IsCheatEnabled( CHEAT_TimeScaling ) )
	{
		if ( key == IK_NumPlus && action == IACT_Release )
		{
			SetOrRemoveTimeScale( ::Min( GetTimeScale() * 2.f, 128.0f ), CNAME( UserInputTimeScale ), 0x7FFFFFFF );
			return true;
		}
		else if ( key == IK_NumMinus && action == IACT_Release )
		{
			SetOrRemoveTimeScale( ::Max( GetTimeScale() * 0.5f, 0.03125f / 16.f ),  CNAME( UserInputTimeScale ), 0x7FFFFFFF );
			return true;
		}
	}
#endif

#ifndef RED_FINAL_BUILD
	// Toggle player physical movement
	if ( IsCheatEnabled( CHEAT_MovementOnPhysics ) )
	{
		if ( key == IK_F12 && action == IACT_Press )
		{
			CPlayer * player = GetPlayer();
			if ( player != NULL )
			{
				CMovingPhysicalAgentComponent * mac = Cast< CMovingPhysicalAgentComponent >( player->GetAgent() );
				if ( mac != NULL )
				{
					mac->SetPhysicalRepresentationRequest( CMovingAgentComponent::Req_Toggle, CMovingAgentComponent::LS_Default );
				}
			}
			return true;
		}
	}
#endif

	// Process debug pages stuff
#ifndef NO_DEBUG_PAGES
	if ( IsCheatEnabled( CHEAT_DebugPages ) )
	{
		if( IDebugPageManagerBase::GetInstance()->OnViewportInput( m_viewport.Get(), key, action, data ) )
		{
			return true;
		}
	}
#endif

	return false;
}

Bool CCommonGame::OnViewportInputDebugInEditor( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( OnViewportInputDebugAlways( view, key, action, data ) )
	{
		return true;
	}
	return false;
}


void CCommonGame::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	TBaseClass::OnViewportGenerateFragments( view, frame );

#ifndef RED_FINAL_BUILD
	for ( TDynArray< IGameSystem* >::iterator gameSystemIter = m_gameSystems.Begin();
		gameSystemIter != m_gameSystems.End(); ++gameSystemIter )
	{
		IGameSystem* gameSystem = *gameSystemIter;
		if ( gameSystem != NULL )
		{
			gameSystem->OnGenerateDebugFragments( frame );
		}
	}
#endif

	// Path planner fragments
	if ( m_pathPlanner )
	{
		m_pathPlanner->GenerateDebugFragments( frame );
	}

	if ( m_guiManager )
	{
		m_guiManager->OnGenerateDebugFragments( frame );
	}

	const CRenderFrameInfo& frameInfo = frame->GetFrameInfo();
#ifndef RED_FINAL_BUILD
	if( m_entityPool )
	{
		m_entityPool->AddDebugScreenText( frame );
	}
	if( frameInfo.IsShowFlagOn( SHOW_TemplateLoadBalancer ) )
	{
		if ( m_templateLoadBalancer )
		{
			m_templateLoadBalancer->OnGenerateDebugFragments( frame );
		}
	}
#endif

	// Can we draw debug shit
#ifndef NO_DEBUG_PAGES
	IDebugPageManagerBase::GetInstance()->OnViewportGenerateFragments( view, frame );
#endif

	// Generate editor fragments
	if ( m_prevHook && (m_prevHook != this) )
	{
		m_prevHook->OnViewportGenerateFragments( view, frame );
	}

	// Actors' debug fragmenst
	if( frameInfo.IsShowFlagOn( SHOW_VisualDebug ) )
	{
		TActors::iterator iter;
		for( iter=m_actors.Begin(); iter!=m_actors.End(); ++iter )
		{
			CActor* actor = *iter;
			actor->GenerateDebugFragments( frame );
		}
	}

	// Camera data
	if ( m_freeCameraActive )
	{
		Vector pos = m_freeCamera->GetPosition();
		frame->AddDebugScreenFormatedText( 10, 10, TXT("Free camera [%.1f, %.1f, %.1f]"), pos.X, pos.Y, pos.Z );
	}

	// Saves
	if ( GGame->IsActive() )
	{
		SGameSessionManager::GetInstance().GenerateDebugFragments( frame );
	}

#ifndef NO_RED_GUI
	if(GDebugWin::GetInstance().GetVisible() == true)
	{
		GRedGui::GetInstance().OnViewportGenerateFragments( view, frame );
	}

	if ( GRedGui::GetInstance().GetEnabled() )
	{
#ifndef RED_PLATFORM_CONSOLE
		frame->AddDebugScreenText( view->GetWidth() - 165, 35, String::Printf( TXT("F8 key closes Debug Windows") ), 0, false, Color::LIGHT_YELLOW );
#endif	// RED_PLATFORM_CONSOLE
	}
	else
	{
#ifndef RED_PLATFORM_CONSOLE
		frame->AddDebugScreenText( view->GetWidth() - 165, 35, String::Printf( TXT("F8 key opens Debug Windows") ), 0, false, Color::LIGHT_YELLOW );
#endif	// RED_PLATFORM_CONSOLE
	}
#endif	// NO_RED_GUI
}

void CCommonGame::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	// Calculate base game camera
	TBaseClass::OnViewportCalculateCamera( view, camera );

#ifndef NO_DEBUG_PAGES
	IDebugPageManagerBase::GetInstance()->OnViewportCalculateCamera( view, camera );
#endif

#ifndef NO_RED_GUI
	if(GRedGui::GetInstance().GetEnabled() == true)
	{
		GRedGui::GetInstance().OnViewportCalculateCamera( view, camera );
	}
#endif	// NO_RED_GUI
}

void CCommonGame::OnViewportSetDimensions ( IViewport* view )
{
	TBaseClass::OnViewportSetDimensions( view );

#ifndef NO_RED_GUI
	if(GRedGui::GetInstance().GetEnabled() == true)
	{
		GRedGui::GetInstance().OnViewportSetDimensions( view, view->GetWidth(), view->GetHeight() );
	}
#endif	// NO_RED_GUI
}

//////////////////////////////////////////////////////////////////////////

void CCommonGame::OnGameTimeChanged()
{
	CFactsDB* factsDB = GetSystem< CFactsDB >();
	if ( factsDB )
	{
		factsDB->RemoveFactsExpired();
	}
}

void CCommonGame::OnCutsceneEvent( const String& csName, const CName& csEvent )
{
	CQuestsSystem* questsSystem = GetSystem< CQuestsSystem >();
	if ( questsSystem )
	{
		questsSystem->OnCutsceneEvent( csName, csEvent );
	}
}

//////////////////////////////////////////////////////////////////////////

String CCommonGame::GetGameReleaseName() const
{
	String releaseId = String::EMPTY;
#ifdef RED_PLATFORM_WINPC
	SRegistryAccess::GetInstance().ReadMachineGameString( TXT( "Release"), releaseId );
#endif
	return releaseId;
}

String CCommonGame::GetCurrentLocale() const
{
	String audioLang;
	String subLang;
	SLocalizationManager::GetInstance().GetGameLanguageName( audioLang, subLang );
	if ( subLang != String::EMPTY )
	{
		return subLang;
	}
	else
	{
		return SLocalizationManager::GetInstance().GetCurrentLocale();
	}
}

//////////////////////////////////////////////////////////////////////////

void CCommonGame::DumpListOfObjects()
{
	// Generate filename
	Red::System::DateTime time;
	Red::System::Clock::GetInstance().GetLocalTime( time );

	String fileName( String::Printf( TXT( "objects_%d%d%d.csv" ), time.GetHour(), time.GetMinute(), time.GetSecond() ) );

	// Create file
	IFile* writer = GFileManager->CreateFileWriter(
		GFileManager->GetBaseDirectory() + fileName, FOF_AbsolutePath );
	if( writer == NULL )
	{
		ERR_GAME( TXT( "Error writing objects log file '%ls'" ), fileName.AsChar() );
		return;
	}

	// Create map
	THashMap< String, Int32 > objects;

	// Iterate every object
	for( BaseObjectIterator it; it; ++it )
	{
		CObject* obj = *it;
		if( obj == NULL )
		{
			continue;
		}

		// Increment object counter
		String objectKey = obj->GetFriendlyName();
		Int32* count = objects.FindPtr( objectKey );
		if ( count )
		{
			// Count same objects
			*count += 1;
		}
		else
		{
			// New object
			VERIFY( objects.Insert( objectKey, 1 ) );
		}
	}

	// Write object names to file
	for( THashMap< String, Int32 >::const_iterator objIter = objects.Begin();
		objIter != objects.End(); ++objIter )
	{
		String str( String::Printf( TXT( "%s ; %d" ), objIter->m_first.AsChar(), objIter->m_second ) );
		StringAnsi buff = UNICODE_TO_ANSI( str.AsChar() );
		writer->Serialize( const_cast< AnsiChar* >( buff.AsChar() ), str.Size() );
		writer->Serialize( const_cast< AnsiChar* >( "\r\n" ), 2 );
	}

	delete writer;

	LOG_GAME( TXT( "Objects list dump complete" ) );
}


void CCommonGame::HandleInGameSavesLoading()
{
	if ( TryLoadGameResource() )
	{
		return;
	}

	if ( TryLoadSavegame() )
	{
		return;
	}

	if ( TryStartDLC() )
	{
		return;
	}
}

Bool CCommonGame::IsNewGamePlus() const
{
	CFactsDB* factsDB = GetSystem< CFactsDB > ();
	Bool retVal = factsDB ? ( 0 != ( factsDB->QuerySum( TXT("NewGamePlus") ) ) ) : false;
	
	// during the startup phase, we need to seach initial facts also
	if ( m_isInStartGame && false == retVal )
	{
		for ( const auto& fact : m_initFacts )
		{
			if ( fact.m_name.EqualsNC( TXT("NewGamePlus") ) && fact.m_value != 0 )
			{
				retVal = true;
				break;
			}
		}
	}

	return retVal;
}


//////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////

CCreateEntityManager *const CCommonGame::InstanciateCreateEntityManager()
{
	return new CCreateEntityManager();
}

//////////////////////////////////////////////////////////////////////////

void CCommonGame::funcEnableSubtitles( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enabled, true );
	FINISH_PARAMETERS;

	Config::cvSubtitles.Set( enabled );
}

void CCommonGame::funcAreSubtitlesEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( AreSubtitlesEnabled() );
}

void CCommonGame::funcGetReactionsMgr( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( m_reactionsManager );
}

void CCommonGame::funcGetIngredientCategoryElements( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_REF( TDynArray< CName >, outNames, TDynArray< CName >() );
	GET_PARAMETER_REF( TDynArray< Int32 >, outPriorites, TDynArray< Int32 >() );
	FINISH_PARAMETERS;

	const SIngredientCategory* ingredientCategory = m_definitionsManager->GetIngredientDefinition( name );

	if(ingredientCategory)
	{
		TDynArray< SIngredientCategoryElement >::const_iterator iter = ingredientCategory->m_elements.Begin();

		for(;iter!= ingredientCategory->m_elements.End();iter++)
		{
			outNames.PushBack(iter->m_name);
			outPriorites.PushBack(iter->m_priority);
		}
	}
}

void CCommonGame::funcIsIngredientCategorySpecified( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	const SIngredientCategory* ingredientCategory = m_definitionsManager->GetIngredientDefinition( name );

	if(ingredientCategory)
	{
		RETURN_BOOL( ingredientCategory->m_specified );
	}
}

void CCommonGame::funcGetIngredientCathegories( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< CName > values;
		m_definitionsManager->GetIngredientCategories(values);

		TDynArray< CName > & retVal = *(TDynArray< CName >*) result;
		retVal = values;
	}
}

void CCommonGame::funcGetSetItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	const SItemSet* itemSet = m_definitionsManager->GetItemSetDefinition( name );
	if ( itemSet )
	{
		TDynArray< CName >& resultArr = *static_cast< TDynArray< CName >* >( result );
		resultArr = itemSet->m_parts;
	}
}

void CCommonGame::funcGetItemSetAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	const SItemSet* itemSet = m_definitionsManager->GetItemSetDefinition( name );
	if ( itemSet )
	{
		TDynArray< CName >& resultArr = *static_cast< TDynArray< CName >* >( result );
		resultArr = itemSet->m_abilities;
	}
}

void CCommonGame::funcGetDefinitionsManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_definitionsManagerAccessor );
}

SExplorationQueryToken CCommonGame::QueryExplorationSync( CEntity* entity, SExplorationQueryContext& queryContext )
{
	SExplorationQueryToken resultToken;
	
	if ( entity )
	{
		SExplorationQueryToken token( entity, queryContext );
		if ( GetActiveWorld()->GetExpManager()->QueryExplorationSync( token, entity ) )
		{
			resultToken = token;
		}
	}
	return resultToken;
}

void CCommonGame::SetGameResourceFilenameToStart( const String& gameResourceFilename )
{
	m_gameResourceFilenameToStart = gameResourceFilename;
}

ELoadGameResult CCommonGame::SetSavegameToLoad( const SSavegameInfo& savegame )
{
	if ( m_savegameToLoad.IsValid() )
	{
		RED_LOG( Save, TXT("CCommonGame::SetSavegameToLoad(): can't initialize loading of '%ls'. Already loading '%ls'."), savegame.m_filename.AsChar(), m_savegameToLoad.m_filename.AsChar() );
		return GUserProfileManager->GetLoadGameProgress();
	}

	ELoadGameResult res = GUserProfileManager->InitGameLoading( savegame );
	if ( LOAD_Initializing == res || LOAD_ReadyToLoad == res )
	{
		RED_LOG( Save, TXT("CCommonGame::SetSavegameToLoad(): Loading save '%ls'"), savegame.m_filename.AsChar() );
		m_savegameToLoad = savegame;
		m_loadingOverlay->ToggleVisible( true, TXT("Loading Save"));
	}
	else
	{
		RED_LOG( Save, TXT("CCommonGame::SetSavegameToLoad(): can't initialize loading of '%ls'. Result status: %ls"), savegame.m_filename.AsChar(), CEnum::ToString< ELoadGameResult > ( res ).AsChar() );
		GUserProfileManager->CancelGameLoading();
	}

	return res;
}

void CCommonGame::SetDLCToStart( CName name, Int32 difficulty )
{
	if ( m_dlcToStart.m_name || false == GetDLCManager()->IsDLCEnabled( name ) )
	{
		return;
	}

	m_dlcToStart.m_name = name;
	m_dlcToStart.m_difficulty = difficulty;
}

Bool CCommonGame::TryLoadSavegame()
{
	if ( m_savegameToLoad.IsValid() )
	{
		ELoadGameResult res = GUserProfileManager->GetLoadGameProgress();
		if ( res == LOAD_NotInitialized )
		{
			HALT("Save not initilized while trying to load! DEBUG please, this should not happen.");
			m_savegameToLoad.Clear();
			return false;
		}

		if ( res == LOAD_Initializing )
		{
			// skip this frame, load is still initializing... need to wait for it.
			return false;
		}

		m_loadingOverlay->ToggleVisible( false, TXT("Loading Save"));

		CallEvent( CNAME( OnGameLoadInitFinished ) );

		if ( res == LOAD_ReadyToLoad && GUserProfileManager->HasActiveUser() )
		{
			CallEvent( CNAME( OnGameLoadInitFinishedSuccess ) );

			if ( IsActive() )
			{
				RequestGameEnd();
			}

			LOG_GAME(TXT("CCommonGame::TryLoadSavegame: loading save '%ls'"), m_savegameToLoad.m_filename.AsChar() );

 			TryEndGame();

			RED_FATAL_ASSERT( !IsActive(), "Cannot load savegame while the game is still active" );

			ESessionRestoreResult sres = m_gameSaver->LoadGame( m_savegameToLoad );
			if ( RESTORE_Success == sres && IsActive() )
			{
				GUserProfileManager->FinalizeGameLoading();
				m_savegameToLoad.Clear();

				// Tick a bit to initialize the parts that initialize themselves on the "next tick" in
				// case that while the loading was happening, an event to enter in constrained mode was
				// received (this avoids having the player t-pose at the background, etc)
				GGame->Tick(0.01f);
				
				// Grab the topmost camera from the camera director
				CWorld* world = GGame->GetActiveWorld();
				ICamera* camera = const_cast<ICamera*>( ( world && world->GetCameraDirector() ) ? world->GetCameraDirector()->GetTopmostCamera() : nullptr );

				// Reset the camera so that we skip the "zoom" effect since it looks weird when the game is
				// paused at the background and suddenly zooms in when you activate it
				if ( camera != nullptr )
				{
					camera->ResetCamera();
				}
	
				return true;
			}
			else
			{
				GUserProfileManager->CancelGameLoading();
				m_savegameToLoad.Clear();
				
				OnLoadingFailed( sres, SGameSessionManager::GetInstance().GetMissingContent() );

				return false;
			}
		}
		else
		{
			GUserProfileManager->CancelGameLoading();
			m_savegameToLoad.Clear();
			return false;
		}
	}

	return false;
}

Bool CCommonGame::TryStartDLC()
{
	if ( !m_dlcToStart.m_name )
	{
		return false;
	}

	// small hack in here, need to update profile maanger so it can process events
	// just to make sure the user haven't signed out, opened the account picker, etc.
	GUserProfileManager->Update();

	// ...and then re-check if we still need to start a dlc.
	if ( !m_dlcToStart.m_name )
	{
		return false;
	}

	ELoadGameResult res = GUserProfileManager->GetLoadGameProgress();
	if ( res != LOAD_NotInitialized )
	{
		HALT("Save loading initilized while trying to start the DLC! DEBUG please, this should not happen.");
		return false;
	}

	IGameLoader* gameLoader = CreateDLCLoader( m_dlcToStart.m_name );

	if ( IsActive() )
	{
		RequestGameEnd();
	}

	TryEndGame();

	if ( nullptr == gameLoader )
	{
		RED_LOG( Save, TXT("Unable to create game loader. Not loading.") );
		m_dlcToStart.m_name = CName::NONE;
		OnLoadingFailed( RESTORE_InternalError, SGameSessionManager::GetInstance().GetMissingContent() );
		return false;
	}

	m_loadingOverlay->ToggleVisible( false, TXT("Loading Save"));

	ESessionRestoreResult sres = SGameSessionManager::GetInstance().CreateExpansionSession( gameLoader, false );
	if ( RESTORE_Success == sres && IsActive() )
	{
		m_gameTime = EngineTime::ZERO;
		SetGameDifficulty( m_dlcToStart.m_difficulty );

		GetSystem<CFactsDB>()->OnDLCGameStarted();

		Tick(0.01f);
		
		// Grab the topmost camera from the camera director
		CWorld* world = GetActiveWorld();
		ICamera* camera = const_cast<ICamera*>( ( world && world->GetCameraDirector() ) ? world->GetCameraDirector()->GetTopmostCamera() : nullptr );

		// Reset the camera so that we skip the "zoom" effect since it looks weird when the game is
		// paused at the background and suddenly zooms in when you activate it
		if ( camera != nullptr )
		{
			camera->ResetCamera();
		}

		m_dlcToStart.m_name = CName::NONE;
		return true;
	}

	OnLoadingFailed( sres, SGameSessionManager::GetInstance().GetMissingContent() );
	m_dlcToStart.m_name = CName::NONE;
	return false;
}

IGameLoader* CCommonGame::CreateDLCLoader( CName nameOfTheDLC )
{
	CDLCDefinition* dlcToStart = nullptr;
	TDynArray< THandle< CDLCDefinition > > dlcs;
	GetDLCManager()->GetEnabledDefinitions( dlcs );
	for ( auto& dlc : dlcs )
	{
		if ( CDLCDefinition* def = dlc.Get() )
		{
			if ( def->GetID() == nameOfTheDLC && def->CanStartInStandaloneMode() )
			{
				dlcToStart = def;
				break;
			}
		}
	}

	IFile* reader = nullptr;
	IFile* fileReaderRaw = dlcToStart->CreateStarterFileReader();
	if ( nullptr == fileReaderRaw )
	{
		RED_LOG( Save, TXT("DLC standalone mode: starting failed, invalid save file provided for DLC '%ls'."), nameOfTheDLC.AsChar() );
		return nullptr;
	}

	Uint32 magicHeader = 0;
	*fileReaderRaw << magicHeader;
	if ( magicHeader == SAVE_NEW_FORMAT_HEADER )
	{
		// The file is new format save-game
		reader = new CChunkedLZ4FileReader( fileReaderRaw );
	}

	if ( nullptr == reader )
	{
		RED_LOG( Save, TXT("DLC standalone mode: starting failed, invalid save file provided for DLC '%ls'."), nameOfTheDLC.AsChar() );
		return nullptr;
	}

	return SGameSaveManager::GetInstance().CreateDebugLoader( reader ); // TODO: rename it, since it's not used ans debug only now
}

Bool CCommonGame::CanStartStandaloneDLC( CName name ) const
{
	TDynArray< THandle< CDLCDefinition > > dlcs;
	GetDLCManager()->GetEnabledDefinitions( dlcs );
	for ( auto& dlc : dlcs )
	{
		if ( CDLCDefinition* def = dlc.Get() )
		{
			if ( def->GetID() == name )
			{
				return def->CanStartInStandaloneMode();
			}
		}
	}
	return false;
}

Bool CCommonGame::TryLoadGameResource()
{
	if ( m_gameResourceFilenameToStart.GetLength() ) // Empty() may not work correctly!
	{
		Bool localNewGameStarting = m_startingNewGamePlus;
		m_startingNewGamePlus = false;
		IGameLoader* playerLoader = m_newGamePlusPlayerLoader;
		m_newGamePlusPlayerLoader = nullptr;
		struct Guard
		{
			IGameLoader* m_loader;
			Guard( IGameLoader* loader ) : m_loader( loader ) {}
			~Guard() 
			{ 
				if ( m_loader ) 
				{ 
					delete m_loader;
				} 

				if ( GUserProfileManager->GetLoadGameProgress() == LOAD_Loading )
				{
					GUserProfileManager->FinalizeGameLoading();
				}
			};
		} guard( playerLoader );

		if ( IsActive() )
		{
			RequestGameEnd();
			TryEndGame();
		}

		RED_FATAL_ASSERT( !IsActive(), "Cannot load game resource while the game is still active" );

		SetupGameResourceFromFile( m_gameResourceFilenameToStart );

		m_gameResourceFilenameToStart.ClearFast();

		CGameResource* gameResource = GetGameResource();
		if ( !gameResource )
		{
			if (localNewGameStarting)
			{
				GUserProfileManager->CancelGameLoading();
			}
			RED_LOG( RED_LOG_CHANNEL( MainMenu ), TXT("No .redgame resource specified") );
			return false;
		}

		const String& worldFilename = gameResource->GetStartWorldPath();
		if ( !worldFilename.EndsWith( TXT(".w2w") ) )
		{
			RED_LOG( RED_LOG_CHANNEL( MainMenu ), TXT("Not a .w2w resource: '%ls'"), worldFilename.AsChar() );
			return false;
		}

		GContentManager->ResetActivatedContent();
		const TDynArray< CName >& toActivate = gameResource->GetPlayGoChunksToActivate();
		for ( CName chunk : toActivate )
		{
			if ( !GContentManager->ActivateContent( chunk ) )
			{
				if (localNewGameStarting)
				{
					GUserProfileManager->CancelGameLoading();
				}
				ERR_GAME( TXT("Failed to activate content %ls from gameResource"), chunk.AsChar() );
				GContentManager->ResetActivatedContent();
				return false;
			}
		}

		if ( toActivate.Empty() )
		{
			GContentManager->ActivateAllContentForDebugQuests();
		}

		const String& loadingVideo = gameResource->GetNewGameLoadingVideo();

		// is it needed? 
		// #JSlama Hopefully not because I need this feature for the simulate_import_ingame fact that is set in main menu when starting a new game
		// It seems CookGameInitialized is currently the only InitialFact in existence from what I can tell. If this DOES need to be cleared, we need
		// an alternate solution for the simulate_import_ingame fact
		//GGame->ClearInitialFacts();

		// Add fake fact, so there will be no "Test chicken phase" in cook, but it will appear in editor
		if ( localNewGameStarting )
		{
			AddInitialFact( TXT("NewGamePlus") );
		}
		AddInitialFact( TXT("CookGameInitialized") );

		SGameSessionManager::GetInstance().CreateSession( worldFilename, false, nullptr, loadingVideo, playerLoader );

		if ( localNewGameStarting )
		{
			GUserProfileManager->FinalizeGameLoading();
		}

		if ( IsActive() )
		{
			return true;
		}
	}

	return false;
}


void CCommonGame::funcQueryExplorationSync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, entityH, NULL );
	GET_PARAMETER( SExplorationQueryContext, queryContext, SExplorationQueryContext() );

	FINISH_PARAMETERS;

	SExplorationQueryToken resultToken = QueryExplorationSync( entityH.Get(), queryContext );	

	RETURN_STRUCT( SExplorationQueryToken, resultToken );
}

void CCommonGame::funcQueryExplorationFromObjectSync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, entityH, NULL );
	GET_PARAMETER( THandle< CEntity >, objectH, NULL );
	GET_PARAMETER( SExplorationQueryContext, queryContext, SExplorationQueryContext() );
	FINISH_PARAMETERS;

	SExplorationQueryToken resultToken;

	CEntity* entity = entityH.Get();
	CEntity* object = objectH.Get();
	if ( entity && object )
	{
		SExplorationQueryToken token( entity, queryContext );
		if ( GetActiveWorld()->GetExpManager()->QueryExplorationFromObjectSync( token, entity, object ) )
		{
			resultToken = token;
		}
	}

	RETURN_STRUCT( SExplorationQueryToken, resultToken );
}

void CCommonGame::funcGetGlobalAttitude( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, srcGroup, CName::NONE );
	GET_PARAMETER( CName, dstGroup, CName::NONE );
	FINISH_PARAMETERS;

	EAIAttitude attitude = CAttitudes::GetDefaultAttitude();
	GetSystem< CAttitudeManager >()->GetGlobalAttitude( srcGroup, dstGroup, attitude );

	RETURN_INT( attitude );
}

void CCommonGame::funcSetGlobalAttitude( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, srcGroup, CName::NONE );
	GET_PARAMETER( CName, dstGroup, CName::NONE );
	GET_PARAMETER( EAIAttitude, attitude, CAttitudes::GetDefaultAttitude() );
	FINISH_PARAMETERS;
	RETURN_BOOL( GetSystem< CAttitudeManager >()->SetGlobalAttitude( srcGroup, dstGroup, attitude ) );
}

void CCommonGame::funcGetReward( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, rewardName, CName::NONE );
	GET_PARAMETER_REF( SReward, reward, SReward::EMPTY );
	FINISH_PARAMETERS;

	Bool ret = false;
	const SReward* foundReward = GetReward( rewardName );
	if ( foundReward )
	{
		reward	= *foundReward;
		ret		= true;
	}
	RETURN_BOOL( ret );
}

void CCommonGame::funcLoadLastGameInit( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, suppressVideo, false );
	FINISH_PARAMETERS;

	SSavegameInfo info;
	if ( GUserProfileManager->GetLatestSaveFile( info ) )
	{
		info.m_suppressVideo = suppressVideo;
		SetSavegameToLoad( info );
	}
}

void CCommonGame::funcLoadGameInit( CScriptStackFrame& stack, void* result )
{
	SSavegameInfo definfo;
	GET_PARAMETER( SSavegameInfo, info, definfo );
	FINISH_PARAMETERS;

	if ( info.IsValid() )
	{
		#ifndef NO_EDITOR
		if ( GIsEditor )
		{
			EDITOR_QUEUE_EVENT( CNAME( LoadGame ), CreateEventData< SSavegameInfo > ( info ) );
			return;
		}
		else
		#endif
		SetSavegameToLoad( info );
	}
}

void CCommonGame::funcCanStartStandaloneDLC( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, dlcName, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_BOOL( CanStartStandaloneDLC( dlcName ) );
}

void CCommonGame::funcInitStandaloneDLCLoading( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, dlcName, CName::NONE );
	GET_PARAMETER( Int32, difficulty, 0 );
	FINISH_PARAMETERS;
	RETURN_ENUM( LOAD_Error );

	if ( CanStartStandaloneDLC( dlcName ) )
	{
		RETURN_ENUM( LOAD_Loading );
		#ifndef NO_EDITOR
		if ( GIsEditor )
		{
			SDLCStartData data;
			data.m_name = dlcName;
			data.m_difficulty = difficulty;
			EDITOR_QUEUE_EVENT( CNAME( SetDLCToStart ), CreateEventData< SDLCStartData > ( data ) );
			return;
		}
		else
		#endif
		SetDLCToStart( dlcName, difficulty );
	}
}

void CCommonGame::funcGetLoadGameProgress( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ELoadGameResult loadResult = GUserProfileManager->GetLoadGameProgress();
	RETURN_INT( loadResult );
}

void CCommonGame::funcListSavedGames( CScriptStackFrame& stack, void* result )
{
	TDynArray< SSavegameInfo > _files;
	GET_PARAMETER_REF( TDynArray< SSavegameInfo >, files, _files );
	FINISH_PARAMETERS;

	GUserProfileManager->GetSaveFiles( files );

	::Sort(files.Begin(), files.End(), SSavegameInfo::ComparePredicate());

	if( files.Empty() )
	{
		RETURN_BOOL( false );
	}

	RETURN_BOOL( true );
}

void CCommonGame::funcImportSave( CScriptStackFrame& stack, void* result )
{
	SSavegameInfo def;
	GET_PARAMETER( SSavegameInfo, info, def );
	FINISH_PARAMETERS;

	#ifndef NO_SAVE_IMPORT
		RETURN_BOOL( GUserProfileManager->ImportSave( info ) );
	#else
		RETURN_BOOL( false );
	#endif
}

void CCommonGame::funcListW2SavedGames( CScriptStackFrame& stack, void* result )
{
	TDynArray< SSavegameInfo > _files;
	GET_PARAMETER_REF( TDynArray< SSavegameInfo >, files, _files );
	FINISH_PARAMETERS;

	#ifndef NO_SAVE_IMPORT
		GUserProfileManager->Import_GetSaveFiles( files );

		if ( files.Empty() )
		{
			RETURN_BOOL( false );
		}
		RETURN_BOOL( true );
	#else
		RETURN_BOOL( false );
	#endif
}

void CCommonGame::funcSaveGame( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ESaveGameType, type, SGT_None );
	GET_PARAMETER( Int32, slot, -1 );
	FINISH_PARAMETERS;

	RequestGameSave( type, Int16( slot ), TXT("called from script") );
}

void CCommonGame::funcGetNumSaveSlots( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ESaveGameType, type, SGT_None );
	FINISH_PARAMETERS;

	RETURN_INT( GUserProfileManager->GetNumSaveSlots( type ) );
}

void CCommonGame::funcGetSaveInSlot( CScriptStackFrame& stack, void* result )
{
	SSavegameInfo definfo;
	GET_PARAMETER( ESaveGameType, type, SGT_None );
	GET_PARAMETER( Int32, slot, -1 );
	GET_PARAMETER_REF( SSavegameInfo, info, definfo );
	FINISH_PARAMETERS;

	RETURN_BOOL( GUserProfileManager->GetSaveInSlot( type, Int16( slot ), info ) );
}

void CCommonGame::funcRequestEndGame( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RequestGameEnd();
}

void CCommonGame::funcRequestExit( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	GEngine->RequestExit();
}

void CCommonGame::funcGetGameResourceList( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	TDynArray< String > gameResourceNames;

	// always add main quest
	gameResourceNames.PushBack( TXT("game\\witcher3.redgame") );

	#if defined( RED_PLATFORM_WIN64 ) 
		GDepot->FindResourcesByExtension( TXT("redgame"), gameResourceNames );
		Sort( gameResourceNames.Begin(), gameResourceNames.End() );
	#else
		THandle< C2dArray > customQuests = LoadResource< C2dArray >( TXT("game\\custom_quests.csv") );
		if ( customQuests )
		{
			const Uint32 count = customQuests->GetNumberOfRows();
			for ( Uint32 i=0; i<count; ++i )
			{
				const String depotPath = customQuests->GetValue(0,i);
				const String name = customQuests->GetValue(1,i);

				// only show existing options
				if ( GDepot->FindFileUseLinks( depotPath, 0 ) != nullptr )
				{
					gameResourceNames.PushBack( depotPath );
				}
			}
		}
	#endif

	if ( false == gameResourceNames.Empty() )
	{
		TDynArray< String > & resultArr = *(TDynArray< String >*) result;
		resultArr = gameResourceNames;
	}
}

void CCommonGame::funcRequestNewGame( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, gameResourceFilename, String::EMPTY );
	FINISH_PARAMETERS;

	RETURN_BOOL( false );

	// Get the actual world name from game configuration
	CGameResource *gameResource = LoadResource< CGameResource >( gameResourceFilename );
	if ( !gameResource )
	{
		RED_LOG( Game, TXT("Not a .redgame resource: '%ls'"), gameResourceFilename.AsChar() );
		RETURN_BOOL( false );
		return;
	}
	String worldFilename = gameResource->GetStartWorldPath();
	if ( !worldFilename.EndsWith( TXT(".w2w") ) )
	{
		RED_LOG( Game, TXT("Not a .w2w resource: '%ls'"), worldFilename.AsChar() );
		RETURN_BOOL( false );
		return;
	}

	if (m_startingNewGamePlus)
	{
		m_startingNewGamePlus = false;

		if (m_newGamePlusPlayerLoader != nullptr)
		{
			delete m_newGamePlusPlayerLoader;
			m_newGamePlusPlayerLoader = nullptr;
		}
	}

	SetGameResourceFilenameToStart( gameResourceFilename );

	RETURN_BOOL( true );
}

void CCommonGame::funcGetGameRelease( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( GetGameReleaseName() );
}

void CCommonGame::funcGetCurrentLocale( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( GetCurrentLocale() );
}

void CCommonGame::funcGetPlayer( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetPlayer() );
}

void CCommonGame::funcGetNPCByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	THandle<CNewNPC> & retVal = *(THandle<CNewNPC>*)result;

	if ( ! IsActive() || tag == CName::NONE )
	{
		retVal = THandle<CNewNPC>();
		return;
	}

	TDynArray< CNode* > nodes;
	GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tag, nodes );

	for ( Uint32 i = 0; i < nodes.Size(); ++i )
	{
		CNewNPC * npc = Cast<CNewNPC>( nodes[i] );
		if ( npc )
		{
			retVal = THandle<CNewNPC>( npc );
			return;
		}
	}

	retVal = THandle<CNewNPC>();
}

void CCommonGame::funcGetNPCsByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_REF( TDynArray< THandle<CNewNPC> >, outNodes, TDynArray< THandle<CNewNPC> >() );
	FINISH_PARAMETERS;

	if ( ! IsActive() || tag == CName::NONE )
		return;

	TDynArray< CNode* > nodes;
	GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tag, nodes );

	if ( outNodes.Empty() )
	{
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
		{
			CNewNPC * npc = Cast<CNewNPC>( nodes[i] );
			if ( npc )
				outNodes.PushBack( npc );
		}
	}
	else
	{
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
		{
			CNewNPC * npc = Cast<CNewNPC>( nodes[i] );
			if ( npc )
				outNodes.PushBackUnique( npc );
		}
	}
}

void CCommonGame::funcGetAllNPCs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< THandle< CNewNPC > >, npcs, TDynArray< THandle< CNewNPC > >() );
	FINISH_PARAMETERS;

	TNPCCharacters::iterator
		currNPC = m_npcCharacters.Begin(),
		lastNPC = m_npcCharacters.End();

	npcs.Reserve( m_npcCharacters.Size() );
	for ( ; currNPC != lastNPC; ++currNPC )
	{
		npcs.PushBack( THandle<CNewNPC>( *currNPC ) );
	}
}

void CCommonGame::funcGetActorByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	THandle<CActor> & retVal = *(THandle<CActor>*)result;

	if ( ! IsActive() || tag == CName::NONE )
	{
		retVal = THandle<CActor>();
		return;
	}

	TDynArray< CNode* > nodes;
	GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tag, nodes );

	for ( Uint32 i = 0; i < nodes.Size(); ++i )
	{
		CActor * actor = Cast<CActor>( nodes[i] );
		if ( actor )
		{
			retVal = THandle<CActor>( actor );
			return;
		}
	}

	retVal = THandle<CActor>();
}

void CCommonGame::funcGetActorsByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_REF( TDynArray< THandle<CActor> >, outNodes, TDynArray< THandle<CActor> >() );
	FINISH_PARAMETERS;

	if ( ! IsActive() || tag == CName::NONE )
		return;

	TDynArray< CNode* > nodes;
	GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tag, nodes );

	if ( outNodes.Empty() )
	{
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
		{
			CActor * actor = Cast<CActor>( nodes[i] );
			if ( actor )
				outNodes.PushBack( actor );
		}
	}
	else
	{
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
		{
			CActor * actor = Cast<CActor>( nodes[i] );
			if ( actor )
				outNodes.PushBackUnique( actor );
		}
	}
}

void CCommonGame::funcGetAPManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetSystem< CCommunitySystem >()->GetActionPointManager().Get() );
}


void CCommonGame::funcGetStorySceneSystem( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetSystem< CStorySceneSystem >() );
}

void CCommonGame::funcAddStateChangeRequest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, entityTag, CName::NONE );
	GET_PARAMETER( THandle< IEntityStateChangeRequest >, modifier, NULL );
	FINISH_PARAMETERS;

	CGameWorld* witcherWorld = Cast< CGameWorld >( GetActiveWorld() );
	if ( witcherWorld )
	{
		witcherWorld->RegisterStateChangeRequest( entityTag, modifier.Get() );
	}
}

void CCommonGame::funcCreateNoSaveLock( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, reason, String() );
	GET_PARAMETER_REF( CGameSaveLock, lock, CGameSessionManager::GAMESAVELOCK_INVALID );
	GET_PARAMETER_OPT( Bool, unique, false );
	GET_PARAMETER_OPT( Bool, allowCheckpoints, true );
	FINISH_PARAMETERS;

	SGameSessionManager::GetInstance().CreateNoSaveLock( reason, lock, unique, allowCheckpoints );
}

void CCommonGame::funcReleaseNoSaveLock( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CGameSaveLock, lock, CGameSessionManager::GAMESAVELOCK_INVALID );
	FINISH_PARAMETERS;

	SGameSessionManager::GetInstance().ReleaseNoSaveLock( lock );
	lock = CGameSessionManager::GAMESAVELOCK_INVALID;
}

void CCommonGame::funcReleaseNoSaveLockByName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( String, lockName, String::EMPTY );
	FINISH_PARAMETERS;

	SGameSessionManager::GetInstance().ReleaseNoSaveLockByName( lockName );
}

void CCommonGame::funcGetGameLanguageId( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( int, audioLang, 0 );
	GET_PARAMETER_REF( int, subtitleLang, 0 );
	FINISH_PARAMETERS;

	SLocalizationManager::GetInstance().GetGameLanguageId( audioLang, subtitleLang );
}

void CCommonGame::funcGetGameLanguageName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( String, audioLang, String::EMPTY );
	GET_PARAMETER_REF( String, subtitleLang, String::EMPTY );
	FINISH_PARAMETERS;

	SLocalizationManager::GetInstance().GetGameLanguageName( audioLang, subtitleLang );
}

void CCommonGame::funcGetGameLanguageIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( int, audioLang, 0 );
	GET_PARAMETER_REF( int, subtitleLang, 0 );
	FINISH_PARAMETERS;

	SLocalizationManager::GetInstance().GetGameLanguageIndex( audioLang, subtitleLang );
}

void CCommonGame::funcGetAllAvailableLanguages( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< String >, textLanguages, TDynArray< String >() );
	GET_PARAMETER_REF( TDynArray< String >, speechLanguages, TDynArray< String >() );
	FINISH_PARAMETERS;

	SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );
}

void CCommonGame::funcSwitchGameLanguageByIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, speechIndex, -1 );
	GET_PARAMETER( Int32, textIndex, -1 );
	FINISH_PARAMETERS;

	SLocalizationManager::GetInstance().SwitchGameLanguageByIndex( speechIndex, textIndex );
}

void CCommonGame::funcReloadLanguage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	SLocalizationManager::GetInstance().ReloadLanguageFromUserSettings();
}

void CCommonGame::funcIsGameTimePaused( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool isTimePaused = false;
	if ( GetTimeManager() != NULL )
	{
		isTimePaused = GetTimeManager()->IsPaused();
	}
	RETURN_BOOL( isTimePaused );
}

void CCommonGame::funcIsInvertCameraX( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( SInputUserConfig::GetIsInvertCameraX() );
}

void CCommonGame::funcIsInvertCameraY( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( SInputUserConfig::GetIsInvertCameraY() );
}

void CCommonGame::funcSetInvertCameraX( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, invert, true );
	FINISH_PARAMETERS;

	SInputUserConfig::SetIsInvertCameraX( invert );
	SInputUserConfig::Save();
}

void CCommonGame::funcSetInvertCameraY( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, invert, true );
	FINISH_PARAMETERS;

	SInputUserConfig::SetIsInvertCameraY( invert );
	SInputUserConfig::Save();
}

void CCommonGame::funcSetInvertCameraXOnMouse( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, invert, true );
	FINISH_PARAMETERS;

	SInputUserConfig::SetIsInvertCameraXOnMouse( invert );
	SInputUserConfig::Save();
}

void CCommonGame::funcSetInvertCameraYOnMouse( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, invert, true );
	FINISH_PARAMETERS;

	SInputUserConfig::SetIsInvertCameraYOnMouse( invert );
	SInputUserConfig::Save();
}

void CCommonGame::funcIsCameraAutoRotX( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( SInputUserConfig::GetIsCameraAutoRotX() );
}

void CCommonGame::funcIsCameraAutoRotY( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( SInputUserConfig::GetIsCameraAutoRotY() );
}

void CCommonGame::funcSetCameraAutoRotX( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, true );
	FINISH_PARAMETERS;

	SInputUserConfig::SetIsCameraAutoRotX( flag );
	SInputUserConfig::Save();
}

void CCommonGame::funcSetCameraAutoRotY( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, true );
	FINISH_PARAMETERS;

	SInputUserConfig::SetIsCameraAutoRotY( flag );
	SInputUserConfig::Save();
}

void CCommonGame::funcConfigSave( CScriptStackFrame& stack, void* result )
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

void CCommonGame::funcAreSavesInitialized( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( GUserProfileManager->AreSavesInitialized() );
}

void CCommonGame::funcChangePlayer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, playerTemplate, NULL );
	GET_PARAMETER_OPT( CName, appearance, CName::NONE );
	FINISH_PARAMETERS;

	m_playerChangeInfo.m_alias = playerTemplate.ToLower();
	m_playerChangeInfo.m_appearance = appearance;
}

void CCommonGame::funcScheduleWorldChangeToMapPin( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, worldPath, NULL );
	GET_PARAMETER( CName, mapPinName, CName::NONE );
	FINISH_PARAMETERS;

	SChangeWorldInfo info;
	info.SetWorldName( worldPath );
	info.m_teleport.SetTarget( STeleportInfo::TargetType_Custom, mapPinName, m_player->GetTags().GetTag( 0 ) );

	ScheduleWorldChange( info );
}

void CCommonGame::funcScheduleWorldChangeToPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, worldPath, NULL );
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( EulerAngles, rotation, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	SChangeWorldInfo info;
	info.SetWorldName( worldPath );
	info.m_teleport.SetTarget( STeleportInfo::TargetType_PositionAndRotation, position, rotation, m_player->GetTags().GetTag( 0 ) );

	ScheduleWorldChange( info );
}

void CCommonGame::funcGetBehTreeReactionManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetBehTreeReactionManager() );
}

void CCommonGame::funcForceUIAnalog( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Bool, enabled, false);
	FINISH_PARAMETERS;

	if (enabled)
	{
		m_forceEnableUIAnalog++;
	}
	else
	{
		m_forceEnableUIAnalog = Max( 0, m_forceEnableUIAnalog - 1);
	}
}

void CCommonGame::funcRequestMenu( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, menuName, CName::NONE );
	GET_PARAMETER_OPT( THandle< IScriptable >, initData, nullptr );
	FINISH_PARAMETERS;

	RequestMenu( menuName, initData );
}

void CCommonGame::funcCloseMenu( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, menuName, CName::NONE );
	FINISH_PARAMETERS;

	CloseMenu( menuName );
}

void CCommonGame::funcRequestPopup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, popupName, CName::NONE );
	GET_PARAMETER_OPT( THandle< IScriptable >, initData, nullptr );
	FINISH_PARAMETERS;

	RequestPopup( popupName, initData );
}

void CCommonGame::funcClosePopup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, popupName, CName::NONE );
	FINISH_PARAMETERS;

	ClosePopup( popupName );
}

void CCommonGame::funcGetHud( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CHud* hud = nullptr;
	if ( m_guiManager )
	{
		CHud* tmpHud = m_guiManager->GetHud();
		if ( tmpHud && tmpHud->IsInitWithFlashSprite() )
		{
			hud = tmpHud;
		}
	}

	RETURN_HANDLE( CHud, hud );
}

void CCommonGame::funcGetInGameConfigWrapper(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( CInGameConfigWrapper, &m_inGameConfigWrapper );
}

void CCommonGame::funcGetCommunitySystem( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( CCommunitySystem, GetCommunitySystem() );
}
void CCommonGame::funcGetAttackRangeForEntity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, sourceHandle, nullptr );
	GET_PARAMETER_OPT( CName, attackRangeName, CName::NONE );
	FINISH_PARAMETERS;

	CEntity* sourceEntity = sourceHandle.Get();

	THandle< CAIAttackRange > attackRange;
	if ( sourceEntity )
	{
		attackRange = CAIAttackRange::Get( sourceEntity, attackRangeName );
	}

	RETURN_HANDLE( CAIAttackRange, attackRange );
}

void CCommonGame::funcGiveReward( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, reward, CName::NONE );
	GET_PARAMETER( THandle< CEntity >, entity, nullptr );
	FINISH_PARAMETERS;

	if( entity.Get() )
	{
		GiveRewardTo( reward, entity.Get() );
	}
}
void CCommonGame::funcConvertToStrayActor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actorHandle, nullptr );
	FINISH_PARAMETERS;
	CActor *const actor = actorHandle.Get();
	Bool resultBool = false;
	if ( actor )
	{
		resultBool = GetSystem< CStrayActorManager >()->ConvertToStrayActor( actor );
	}
	RETURN_BOOL( resultBool );
}


void CCommonGame::funcCreateEntityAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CCreateEntityHelper >, createEntityHelperHandle, nullptr );
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, nullptr );
	GET_PARAMETER( Vector, spawnPosition, Vector::ZEROS );
	GET_PARAMETER_OPT( EulerAngles, spawnRotation, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Bool, useAppearancesFromIncludes, true );
	GET_PARAMETER_OPT( Bool, fake, false );
	GET_PARAMETER_OPT( Bool, doNotAdjustPlacement, false );
	GET_PARAMETER_OPT( EPersistanceMode, persistanceMode, PM_DontPersist );
	GET_PARAMETER_OPT( TDynArray< CName > , tagList, TDynArray< CName >() );
	FINISH_PARAMETERS;

	CEntityTemplate *pEntityTemplate = entityTemplate.Get();
	if ( pEntityTemplate == NULL || !GetActiveWorld())
	{
		RETURN_INT( 0 );
	}

	CGUID guid( CGUID::Create() );
	EntitySpawnInfo einfo;
	einfo.m_guid			= guid;
	einfo.m_template		= pEntityTemplate;
	einfo.m_spawnPosition	= spawnPosition;
	einfo.m_spawnRotation	= spawnRotation;
	einfo.m_entityFlags		= EF_DestroyableFromScript;

	einfo.m_tags.SetTags( tagList );


	switch ( persistanceMode )
	{
		case PM_Persist:
			einfo.m_entityFlags |= EF_ManagedEntity; //note no break after
		case PM_SaveStateOnly:
			einfo.m_idTag = m_idTagManager->Allocate();
			break;
		case PM_DontPersist:
		default:
			break;
	}

	if ( !useAppearancesFromIncludes )
	{
		einfo.m_appearances = pEntityTemplate->GetEnabledAppearancesNames();
	}
	CCreateEntityHelper *const createEntityHelper = createEntityHelperHandle.Get();
	Bool success = false;
	if ( createEntityHelper )
	{
		success = m_createEntityManager->CreateEntityAsync( createEntityHelper, Move( einfo ) );
	}

	RETURN_INT( success ? static_cast< Int32 >( ::GetHash< CGUID >( guid ) ) : 0 );
}

void CCommonGame::funcTestNoCreaturesOnLocation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, testPos, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.f );
	GET_PARAMETER_OPT( THandle< CActor >, ignoreActor, nullptr );
	FINISH_PARAMETERS;

	Bool ret = m_actorsManager->TestLocation( testPos, radius, ignoreActor.Get() );
	RETURN_BOOL( ret );
}

void CCommonGame::funcTestNoCreaturesOnLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, testPos0, Vector::ZEROS );
	GET_PARAMETER( Vector, testPos1, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.f );
	GET_PARAMETER_OPT( THandle< CActor >, ignoreActor0, nullptr );
	GET_PARAMETER_OPT( THandle< CActor >, ignoreActor1, nullptr );
	GET_PARAMETER_OPT( Bool, ignoreGhostCharacters, false ); 
	FINISH_PARAMETERS;

	CActor* ignoreActors[2];
	ignoreActors[ 0 ] = ignoreActor0.Get();
	ignoreActors[ 1 ] = ignoreActor1.Get();
	Bool ret;
	if ( ignoreActors[ 0 ] && ignoreActors[ 1 ] )
	{
		ret = m_actorsManager->TestLine( testPos0, testPos1, radius, ignoreActors, 2, ignoreGhostCharacters );
	}
	else
	{
		ret  = m_actorsManager->TestLine( testPos0, testPos1, radius, ignoreActors[ 0 ] ? ignoreActors[ 0 ] : ignoreActors[ 1 ], ignoreGhostCharacters );
	}
	RETURN_BOOL( ret );
}

Bool CCommonGame::StartGame_AreQuestsStable() const 
{
	CQuestsSystem* questsSystem = GetSystem< CQuestsSystem >();
	if ( !questsSystem )
	{
		return true;
	}
	
	if ( questsSystem->IsStable() )
	{
		return true;
	}

	return false;
}

void CCommonGame::StartGame_ResetQuestStability()
{
	if ( GCommonGame->GetSystem<CQuestsSystem>() )
	{
		GCommonGame->GetSystem<CQuestsSystem>()->ResetStability();
	}
}

void CCommonGame::StartGame_WaitForPlayer()
{
	const Uint32 maxTicks = 10;
	Uint32 i = 0;
	// Attach the item shit... streaming textures after now
	CItemEntityManager& itemEntMgr = SItemEntityManager::GetInstance();
	CLoadingJobManager& jobMgr = SJobManager::GetInstance();
	const Float smallTick = 0.001f;
	while ( itemEntMgr.IsDoingSomething() && i++ < maxTicks )
	{
		jobMgr.FlushPendingJobs();
		itemEntMgr.OnTick( smallTick );
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
	if ( i >= maxTicks )
	{
		LOG_GAME(TXT("Maxed out waiting for items to finish"));
	}
	else
	{
		LOG_GAME(TXT("StartGame_WaitForPlayer: waited %u ticks"), i );
	}
}

void CCommonGame::StartGame_InitHUD()
{
	if ( !m_flashPlayer || !m_guiManager )
	{
		return;
	}

	Rect viewport;
	if( Config::cvForcedRendererOverlayResolution.Get() )
	{
		viewport.m_right = Config::cvForcedRendererResolutionWidth.Get();
		viewport.m_bottom = Config::cvForcedRendererResolutionHeight.Get();
		viewport.m_left = 0;
		viewport.m_top = 0;
	}
	else
	{
		viewport.m_left = GGame->GetViewport()->GetX();
		viewport.m_top  = GGame->GetViewport()->GetY();
		viewport.m_right  = viewport.m_left + GGame->GetViewport()->GetWidth();
		viewport.m_bottom = viewport.m_top +  GGame->GetViewport()->GetHeight();
	}		

	const Float fullUpdateTick = 0.05;

	// Not really waiting, just kick off module loading and start waiting for streaming
	// Tick it enough to get AS to do something
	m_guiManager->Tick( fullUpdateTick ); // process GUI events, like create HUD
	m_flashPlayer->Tick( fullUpdateTick, viewport );
	SJobManager::GetInstance().FlushPendingJobs(); // flush any possible async loading
	
	// Process ActionScript to register HUD components and image load requests
	m_guiManager->Tick( fullUpdateTick );
	m_flashPlayer->Tick( fullUpdateTick, viewport );

	m_guiManager->Tick( fullUpdateTick );
	m_flashPlayer->Tick( fullUpdateTick, viewport );
}

void CCommonGame::StartGame_UpdateActors()
{
	if ( m_actorsManager != nullptr )
	{
		m_actorsManager->Update( 0.0f );
	}
}

Bool CCommonGame::StartGame_UpdateSaver()
{
	m_gameSaver->Update();
	return m_gameSaver->IsSaveInProgress();
}

#ifndef RED_FINAL_BUILD
void CCommonGame::StartGame_DebugDumpUnstableQuestThreads() const 
{
	extern void DebugDumpUnstableQuestThreads();
	DebugDumpUnstableQuestThreads();
}
#endif

void CCommonGame::OnGamePrefetchStart( Bool showLoadingScreen )
{
	// Pause all scenes
	if ( showLoadingScreen )
	{
		if ( CStorySceneSystem* ssSystem = GCommonGame->GetSystem< CStorySceneSystem >() )
		{
			for ( auto& ssPlayer : ssSystem->GetScenePlayers() )
			{
				if ( !ssPlayer.IsValid() )
				{
					continue;
				}

				if ( CStorySceneController* controller = ssPlayer->GetSceneController() )
				{
					controller->Pause( true );
				}
			}
		}
	}
}

void CCommonGame::OnGamePrefetchEnd( Bool showLoadingScreen )
{
	// Unpause all scenes
	if ( showLoadingScreen )
	{
		if ( CStorySceneSystem* ssSystem = GCommonGame->GetSystem< CStorySceneSystem >() )
		{
			for ( auto& ssPlayer : ssSystem->GetScenePlayers() )
			{
				if ( !ssPlayer.IsValid() )
				{
					continue;
				}

				if ( CStorySceneController* controller = ssPlayer->GetSceneController() )
				{
					controller->Pause( false );
				}
			}
		}
	}
}

RED_DEFINE_STATIC_NAME( ep1 )
RED_DEFINE_STATIC_NAME( bob_000_000 )

void CCommonGame::OnLoadingScreenShown()
{
	if ( !m_dlcManager )
	{
		return;
	}

	m_loadingScreen->SetExpansionsAvailable( m_dlcManager->IsDLCEnabled( CNAME( ep1 ) ),
											 m_dlcManager->IsDLCEnabled( CNAME( bob_000_000 ) ) );
}

void CCommonGame::SetStreamingReferencePosition( const Vector& referencePosition )
{
	TBaseClass::SetStreamingReferencePosition( referencePosition );

	if ( m_actorsManager != nullptr )
	{
		m_actorsManager->SetReferencePosition( referencePosition );
	}
}


void CCommonGame::funcGetDisplayNameForSavedGame( CScriptStackFrame& stack, void* result )
{
	SSavegameInfo definfo;
	GET_PARAMETER( SSavegameInfo, info, definfo ); 
	FINISH_PARAMETERS;

	RETURN_STRING( TXT("Error") );
	if ( info.IsValid() )
	{
		RETURN_STRING( GUserProfileManager->BuildFullDisplayNameForSave( info ) );
	}
}

void CCommonGame::funcDeleteSavedGame( CScriptStackFrame& stack, void* result )
{
	SSavegameInfo definfo;
	GET_PARAMETER( SSavegameInfo, info, definfo ); 
	FINISH_PARAMETERS;

	if ( info.IsValid() )
	{
		GUserProfileManager->DeleteSaveGame( info );
	}
}

Bool CCommonGame::OnSuspend()
{
	// save game now, if possible
	if ( IsActive() && GetActiveWorld() != nullptr && !IsLoadingScreenShown() && !IsSavingGame() )
	{
		m_gameSaver->SaveGameSync( SGT_AutoSave, -1, TXT("on suspend") );
	}

	return TBaseClass::OnSuspend();
}

Bool CCommonGame::OnResume()
{
	return TBaseClass::OnResume();
}

void CCommonGame::funcAreSavesLocked( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( SGameSessionManager::GetInstance().AreGameSavesLocked( SGT_Manual ) || m_gameSaver->IsSaveInProgress() );
}

Bool CCommonGame::IsAnyMenu() const
{
	return ( nullptr == m_guiManager ) ? false : m_guiManager->IsAnyMenu();
}

void CCommonGame::funcGetContentRequiredByLastSave(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER_REF( TDynArray< CName >, req, TDynArray< CName >() );
	FINISH_PARAMETERS;

	req = GUserProfileManager->GetContentRequiredByLastSave();
}

void CCommonGame::TryToClearAutoSaveRequests()
{
	if ( false == m_autoSaveForced )
	{
		m_autoSaveRequestTimer = -1.f;
		#ifdef RED_LOGGING_ENABLED
			m_debugAutoSaveRequests.ClearFast();
		#endif
	}
}

void CCommonGame::ProcessAutoSaveRequests( Float timeDelta )
{
	if ( !IsActive() )
	{
		return;
	}

	if ( nullptr == GGame->GetActiveWorld() )
	{
		return;
	}

	if ( IsLoadingScreenShown() )
	{
		return;
	}

	if ( GetGuiManager()->IsAnyMenu() )
	{
		return;
	}

	const Float autosaveTimeInterval = 60.f * Float( Clamp< Int32 > ( Config::cvAutosaveTimeInterval.Get(), 3, 15 ) );
	if ( m_autoSaveRequestTimer < 0.f )
	{
		if ( GetGameSaver()->ForHowLongWeHaventAttemptedToAutoSave() > autosaveTimeInterval )
		{
			RequestAutoSave( TXT("every X minutes" ), false );
		}
	}

	if ( m_autoSaveRequestTimer >= 0.f )
	{
		if ( SGameSessionManager::GetInstance().AreGameSavesLocked( SGT_AutoSave ) )
		{
			TryToClearAutoSaveRequests();
			return;
		}

		if ( IsSavingGame() )
		{
			// no reason to do 2 saves in a row
			RED_LOG( Save, TXT("NOT doing autosave, another save is being done now.") );
			TryToClearAutoSaveRequests();
			return;
		}

		if ( false == m_autoSaveForced && GetGameSaver()->ForHowLongWeHaventAttemptedToAutoSave() < GetGameplayConfig().m_autosaveCooldown )
		{
			// not auto-saving that often, sorry
			RED_LOG( Save, TXT("NOT doing autosave, another save done recently, or level just started.") );
			TryToClearAutoSaveRequests();
			return;
		}

		m_autoSaveRequestTimer += timeDelta;
		if ( m_autoSaveRequestTimer > GetGameplayConfig().m_autosaveDelay )
		{
			#ifdef RED_LOGGING_ENABLED
				RED_LOG( Save, TXT("doing autosave,") );
				for ( const auto& reason : m_debugAutoSaveRequests )
				{
					RED_LOG( Save, TXT("reason: %ls"), reason.AsChar() );
				}

			#endif

			RequestGameSave( SGT_AutoSave, -1, TXT("AutoSave system") );
			m_autoSaveForced = false;
			TryToClearAutoSaveRequests();
		}
	}
}

void CCommonGame::SaveRequiredDLCs( IGameSaver* saver )
{
	RED_LOG( Save, TXT("Saving required DLCs...") );

	CGameSaverBlock block( saver, CNAME( AdditionalContent ) );
	TDynArray< THandle< CDLCDefinition > > dlcs;

	CDLCManager* mgr = GetDLCManager();
	mgr->GetEnabledDefinitions( dlcs );

	RED_LOG( Save, TXT("Enabled definitions count: %d"), dlcs.Size() );

	Uint32 cnt = 0;
	for ( const auto& dlc : dlcs )
	{
		CDLCDefinition* def = dlc.Get();
		cnt += ( ( def && def->IsRequiredByGameSave() ) ? 1 : 0 );
	}

	RED_LOG( Save, TXT("Required by save count: %d"), cnt );

	saver->WriteValue( CNAME( count ), cnt );
	for ( const auto& dlc : dlcs )
	{
		CDLCDefinition* def = dlc.Get();
		if ( def && def->IsRequiredByGameSave() )
		{
			saver->WriteValue( CNAME( name ), def->GetID() );
			RED_LOG( Save, TXT("Required: %ls"), def->GetID().AsChar() );
		}
	}
}

Bool CCommonGame::LoadRequiredDLCs( IGameLoader* loader, TDynArray< CName >& missingContent )
{
	if ( loader->GetSaveVersion() >= SAVE_VERSION_ACTIVATED_DLC_CONTENT_IN_SAVE )
	{
		RED_LOG( Save, TXT("Loading required DLCs...") );

		CGameSaverBlock block( loader, CNAME( AdditionalContent ) );
		TDynArray< CName > additionalContent;

		CDLCManager* mgr = GetDLCManager();

		Uint32 cnt = 0;
		loader->ReadValue( CNAME( count ), cnt );

		RED_LOG( Save, TXT("Required by save count: %d"), cnt );

		for ( Uint32 i = 0; i < cnt; ++i )
		{
			CName dlcName;
			loader->ReadValue( CNAME( name ), dlcName );
			if ( dlcName )
			{
				RED_LOG( Save, TXT("Required: %ls"), dlcName.AsChar() );
				if ( false == mgr->IsDLCEnabled( dlcName ) )
				{
					RED_LOG( Save, TXT("Required DLC '%ls' is not enabled, aborting load."), dlcName.AsChar() );
					missingContent.PushBack( dlcName );
				}
			}
		}
	}
	else
	{
		RED_LOG( Save, TXT("No required DLC information in saved game.") );
	}

	return missingContent.Empty();
}

void CCommonGame::RequestAutoSave( const String& reason, Bool forced )
{
#ifdef RED_LOGGING_ENABLED
	m_debugAutoSaveRequests.PushBack( reason );
	//RED_LOG( Save, TXT("requested autosave for a reason: %ls%ls"), reason.AsChar(), forced ? TXT(" (forced)") : TXT("") );
#else
	RED_UNUSED( reason );
#endif

	m_autoSaveRequestTimer = 0.f;
	if ( forced )
	{
		m_autoSaveForced = true;
	}
}

void CCommonGame::OnUserEvent( const EUserEvent& event )
{
	TBaseClass::OnUserEvent( event );

	if( event == EUserEvent::UE_SignedOut )
	{
		m_dlcToStart.m_name = CName::NONE;
		m_showedConfigChangedMessage = false;
	}
	else if ( event == EUserEvent::UE_GameSaved )
	{
		m_gameSaver->OnSaveCompleted( true );
		CallEvent( CNAME( OnSaveCompleted ), m_gameSaver->GetLastSaveType(), true );
	}
	else if ( event == EUserEvent::UE_GameSaveFailed )
	{
		SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( m_gameSaver->GetLastSaveType(), TXT("Last saving operation was succesful, but the platform API refused to store the data. This might happen due to out of disk space or other filesystem error."), TXT("") );
		m_gameSaver->OnSaveCompleted( false );
		CallEvent( CNAME( OnSaveCompleted ), m_gameSaver->GetLastSaveType(), false );
	}
	else if( event == EUserEvent::UE_LoadSaveReady )
	{
		m_dlcManager->ReloadDLCConfigs();
	}
	else if ( event == EUserEvent::UE_SignInStarted )
	{
		m_dlcToStart.m_name = CName::NONE;
	}
}


void CCommonGame::funcRequestAutoSave( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, reason, String::EMPTY );
	GET_PARAMETER( Bool, forced, false );
	FINISH_PARAMETERS;
	RequestAutoSave( reason.Empty() ? TXT("script") : reason, forced );
}

void CCommonGame::funcCalculateTimePlayed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, GameTime( (Int32)(Double)GetGameTime() ) );
}

Bool CCommonGame::IsSavedRecently( Bool checkForAutoSaveOnly ) const
{
	return ( !checkForAutoSaveOnly || m_gameSaver->GetLastSaveType() == SGT_AutoSave )
		&& ( m_gameSaver->ForHowLongWeHaventSaved() < m_gameplayConfig.m_autosaveCooldown );
}

void CCommonGame::funcGetUIHorizontalPlusFrameScale( CScriptStackFrame& stack, void* result )
{
	Float horizontalPlusFrameScale = 1.0f; 
	FINISH_PARAMETERS;
	//! this extra scale for scaleform hud components is only temporary to the time when UI team fix problem with multi aspect ratio support 
#ifndef RED_VIEWPORT_TURN_ON_CACHETS_16_9
	if( GetViewport() )
	{
		horizontalPlusFrameScale = (Float)GetViewport()->GetWidth()/(Float)GetViewport()->GetHeight();
		horizontalPlusFrameScale /= 1.777777777777778f;
	}
#endif
	RETURN_FLOAT( horizontalPlusFrameScale );
}

void CCommonGame::funcRequestScreenshotData( CScriptStackFrame& stack, void* result )
{
	SSavegameInfo definfo;
	GET_PARAMETER( SSavegameInfo, info, definfo ); 
	FINISH_PARAMETERS;

	GUserProfileManager->RequestScreenshotDataForReading( info );
}

void CCommonGame::funcIsScreenshotDataReady( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( GUserProfileManager->IsScreenshotDataReadyForReading() );
}

void CCommonGame::funcFreeScreenshotData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	GUserProfileManager->DoneReadingScreenshotData();
}

void CCommonGame::funcIsDisplayNameAvailable(CScriptStackFrame& stack, void* result)
{
	SSavegameInfo definfo;
	GET_PARAMETER( SSavegameInfo, info, definfo ); 
	FINISH_PARAMETERS;
	RETURN_BOOL( info.IsDisplayNameAvailable() );
}

void CCommonGame::funcShouldShowSaveCompatibilityWarning( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( CGameSaver::ShouldShowTheCompatibilityWarning() );
}

void CCommonGame::funcIsNewGame( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( SGameSessionManager::GetInstance().IsNewGame() );
}

void CCommonGame::funcIsNewGameInStandaloneDLCMode( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( SGameSessionManager::GetInstance().IsStandaloneDLCStarting() );
}


void CCommonGame::funcIsNewGamePlusEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( GetGameSaver()->IsNewGamePlusEnabled() );
}

namespace Config
{
	extern TConfigVar<Bool> cvIsHardwareCursor;
}

void CCommonGame::funcCenterMouse(CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( Config::cvIsHardwareCursor.Get() )
	{
		GHardwareCursorManager.RequestMouseMove( SCursorPoint(0.5f,0.5f) );
	}
	else if (m_flashPlayer)
	{
		m_flashPlayer->CenterMouse();
	}
}

void CCommonGame::funcMoveMouseTo(CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Float, targetX, 0.0f);
	GET_PARAMETER(Float, targetY, 0.0f);
	FINISH_PARAMETERS;

	if ( Config::cvIsHardwareCursor.Get() )
	{
		GHardwareCursorManager.RequestMouseMove( SCursorPoint(targetX,targetY) );
	}
	else
	{
		MoveMouseTo( targetX, targetY );
	}
}

void CCommonGame::funcGetDLCManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetDLCManager() );
}

void CCommonGame::OnControllerEvent( const EControllerEventType& event )
{
	switch( event )
	{
	case EControllerEventType::CET_Reconnected:

		// #JS If we get a disconnect and reconnect during loading, no need to send messages
		if (m_controllerDisconnectMessageNeeded)
		{
			m_controllerDisconnectMessageNeeded = false; 
		}
		else
		{
			CallEvent( RED_NAME( OnControllerReconnected ) );
		}
		break;

	case EControllerEventType::CET_Disconnected:
		// #JS WS not reliable during loading, should be delayed in this case to avoid empty message box edge cases
		m_controllerDisconnectMessageNeeded = true;
		break;
	}
}

void CCommonGame::PopulateDlcInGameConfigGroupWithVars()
{
	TDynArray<CName> dlcDefinitions;
	m_dlcManager->GetDLCs( dlcDefinitions );
	for( const CName dlcName : dlcDefinitions )
	{
		if( m_dlcManager->IsDLCVisibleInMenu( dlcName ) )
		{
			if( m_dlcInGameConfigGroup->ContainsDLCVar( dlcName ) == false )
			{
				m_dlcInGameConfigGroup->AddDLCVar( dlcName );
			}
		}
	}
}

Bool CCommonGame::AreConfigsResetInThisSession()
{
	return SConfig::GetInstance().AreConfigResetInThisSession();
}

void CCommonGame::funcAreConfigResetInThisSession(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	RETURN_BOOL( AreConfigsResetInThisSession() );
}

void CCommonGame::funcHasShownConfigChangedMessage(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_showedConfigChangedMessage );
}

void CCommonGame::funcSetHasShownConfigChangedMessage(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER(Bool, value, true);
	FINISH_PARAMETERS;

	m_showedConfigChangedMessage = value;
}


void CCommonGame::funcGetApplicationVersion(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	RETURN_STRING( ANSI_TO_UNICODE( GAME_VERSION ) );
} 

#include "../engine/hwCursorManager.h"

void CCommonGame::funcIsSoftwareCursor(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	RETURN_BOOL( !Config::cvIsHardwareCursor.Get() );	
}

void CCommonGame::funcShowHardwareCursor(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	GHardwareCursorManager.Game_RequestHardwareCursor( true );
}

void CCommonGame::funcHideHardwareCursor(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	GHardwareCursorManager.Game_RequestHardwareCursor( false );
}

void CCommonGame::funcGetAchievementsDisabled(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	RETURN_BOOL(false);
}

//////////////////////////////////////////////////////////////////////////

static void funcGetGame( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GCommonGame );
};

void RegisterCommonGameFunctions()
{
	extern void RegisterAgentsWorldScriptFunctions();
	RegisterAgentsWorldScriptFunctions();
	extern void RegisterFactsDBScriptFunctions();
	RegisterFactsDBScriptFunctions();
	extern void RegisterActorsStorageScriptFunctions();
	RegisterActorsStorageScriptFunctions();
	extern void RegisterInventoryItemScriptFunctions();
	RegisterInventoryItemScriptFunctions();
	extern void RegisterItemUniqueIdFunctions();
	RegisterItemUniqueIdFunctions();
	extern void RegisterCommunityScriptFunctions();
	RegisterCommunityScriptFunctions();
	extern void RegisterBehTreeDebugGlobalFunctions();
	RegisterBehTreeDebugGlobalFunctions();
	extern void RegisterLocalizedContentFunctions();
	RegisterLocalizedContentFunctions();
	extern void RegisterSpawnPointManagerFunctions();
	RegisterSpawnPointManagerFunctions();
	extern void RegisterActionPointManagerScriptFunctions();
	RegisterActionPointManagerScriptFunctions();
	extern void RegisterBoidsRelatedScriptFunctions();
	RegisterBoidsRelatedScriptFunctions();
	extern void RegisterAbilitiesFunctions();
	RegisterAbilitiesFunctions();
	extern void RegisterEncounterScriptFunctions();
	RegisterEncounterScriptFunctions();
	extern void RegisterActorManagerScriptFunctions();
	RegisterActorManagerScriptFunctions();
	extern void ExportGameplayFxFunctions();
	ExportGameplayFxFunctions();

	CScriptRegistrationManager::GetInstance()->RegisterScriptFunctions();

	NATIVE_GLOBAL_FUNCTION("GetGame", funcGetGame );
}
