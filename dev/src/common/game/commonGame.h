/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CCommonGame;

#include "gameSystem.h"
#include "gameSystemOrder.h"
#include "game.h"
#include "teleportHelper.h"
#include "rewardManager.h"
#include "inGameConfigWrapper.h"
#include "binaryStorage.h"
#include "attitude.h"
#include "encounterSpawnGroupCounter.h"
#include "../engine/gameSaveManager.h"
#include "../core/iterator.h"
#include "../core/threadSafeNodeList.h"

class CNodesBinaryStorage;
class CGameplayStorage;
class CActorsManager;
class CActor;
class CJournalManager;
class CDefinitionsManager;
class CDefinitionsManagerAccessor;
class CRewardManager;
class CPlayer;
class CNewNPC;
class ExpManager;
class CReactionsManager;
class CMoveGlobalPathPlanner;
class CBehTreeMachine;
class CMinigame;
class CGameplayEntity;
class CGameSaver;
class CGuiGameObjectManager;
class CGuiManager;
class CBoidSpecies;
class CLootDefinitions;
class ILootManager;
class CEntityPool;
class CTemplateLoadBalancer;
class CCreateEntityManager;
class CNewNpcSensesManager;
class CDLCManager;
class CFlushGameplayStorageUpdatesTask;

/// Common game pointer
extern CCommonGame* GCommonGame;

class CGameWorld;

class CInGameConfigDLCGroup;

struct SChangeWorldInfo
{
	String			m_worldName;
	String			m_videoToPlay;
	STeleportInfo	m_teleport;

	SChangeWorldInfo()
	{}

	Bool IsValid() const { return m_worldName.GetLength() > 0; }
	void Reset() { m_worldName.Clear(); m_videoToPlay.Clear(); }
	void SetWorldName( const String& worldName ) { m_worldName = worldName; }
	void SetVideoToPlay( const String& videoToPlay ) { m_videoToPlay = videoToPlay; }
};

struct SDLCStartData
{
	SDLCStartData() : m_name( CName::NONE ), m_difficulty( 0 ) {}
	CName m_name;
	Int32 m_difficulty;
};

class CCommonGame : public CGame
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CCommonGame, CGame )

public:
	typedef TList< CNewNPC* >			TNPCCharacters;
	typedef TList< CActor* >			TActors;
	typedef TList< CBehTreeMachine* >	TBehTreeMachines;

protected:
	enum ESystemErrorMessageFlags
	{
		SEMF_Off = 0,
		SEMF_Paused,
		SEMF_Unpause,
		SEMF_ErrorMessageBoxShowing,		// Message box is showing
		SEMF_ShowOKButton,					// User has reinserted the offending item
		SEMF_CanDismissPopup,				// OK Button prompt is now visible
		SEMF_ExitShowPrompt,				// User has opted to quit the game
		SEMF_ExitMessage,
		SEMF_ExitBackOut,					// User changed their mind about quitting
		SEMF_ExitGame,						// User has confirmed choice to exit
		SEMF_CloseErrorMessageBox,			// Request to close the error box (user has pressed a button)
		SEMF_ControllerDisconnected,		// Controller is currently disconnected
		SEMF_ControllerInform,				// Popup message box at appropriate time
		SEMF_ProfileStorageRemoved,			// Profile Storage is currently disconnected
		SEMF_ProfileStorageInform,
		SEMF_Max
	};

protected:
	THandle< CPlayer >								m_player;								//!< The player
	CDefinitionsManager*							m_definitionsManager;					//!< Item manager
	CDefinitionsManagerAccessor*					m_definitionsManagerAccessor;			//!< Item manager Accessor
	CRewardManager*									m_rewardManager;						//!< Reward manager
	CReactionsManager*								m_reactionsManager;						//!< Manager for reaction events broadcast. TODO: Consider making it a game system
	CMoveGlobalPathPlanner*							m_pathPlanner;							//!< Global path planner instance
	CGuiManager*									m_guiManager;							//!< GUI Manager
	int 											m_forceEnableUIAnalog;					//!< 
	CGameplayStorage*								m_gameplayStorage;						//!< Storage with fast queries containing all active gameplay entities
	CActorsManager*									m_actorsManager;						//!< Manager of actor logic LOD
	CBehTreeReactionManager*						m_behTreeReactionManager;				//!< Manager for reactions handled by behaviour trees
	TDynArray< IGameSystem* >						m_gameSystems;							//!< Stores references for game systems
	TNPCCharacters									m_npcCharacters;						//!< List of all currently attached NPC characters
	TActors											m_actors;								//!< List of all currently attached actors
	TBehTreeMachines								m_behTreeMachines;						//!< List of behavior tree machine

	ThreadSafeNodeList< 2048,	CGameplayEntity >	m_gameplayStorageUpdate;				//!< Update list of gameplay entity storage ( needed because of OnUpdateTransform that runs multithreaded )
	ThreadSafeNodeList< 1024,	CActor >			m_actorsManagerUpdate;					//!< Update list of actor manager ( needed because of OnUpdateTransform that runs multithreaded )
	CFlushGameplayStorageUpdatesTask*				m_flushGameplayStorageUpdatesTask;

	SChangeWorldInfo								m_changeWorldInfo;						//!< Info about scheduled world change
	Bool											m_controllerDisconnectMessageNeeded;	//!< If this happens during loading, the flag is set and consumed once shown

	CBoidSpecies*									m_boidSpecies;							//!< Container for swarm parameters

	Red::TUniquePtr< CEntityPool >					m_entityPool;
	CTemplateLoadBalancer*							m_templateLoadBalancer;

	CCreateEntityManager *							m_createEntityManager;
	CNewNpcSensesManager*							m_npcSensesManager;						//!< NPC senses manager

	CDLCManager*									m_dlcManager;							//!< Game related DLC manager
	CInGameConfigDLCGroup*							m_dlcInGameConfigGroup;					//!< In-game config group for DLCs

	Bool											m_startingNewGamePlus;
	IGameLoader*									m_newGamePlusPlayerLoader;

	Bool											m_showedConfigChangedMessage;

private:
	CInGameConfigWrapper							m_inGameConfigWrapper;					//!< Allows to access engine and game configuration in game

private:
	String											m_gameResourceFilenameToStart;

	CGameSaver*										m_gameSaver;							//!< game saver
	SSavegameInfo									m_savegameToLoad;						//!< set if game load is requested
	SDLCStartData									m_dlcToStart;
#ifdef RED_LOGGING_ENABLED
	TDynArray< String >								m_debugAutoSaveRequests;
#endif
	Float											m_autoSaveRequestTimer;
	Bool											m_autoSaveForced;

	TDynArray< TPointerWrapper< CActor > >			m_actorsOutput;

	CEncounterSpawnGroupCounter						m_encounerSpawnGroupCounter;

	Bool											m_HACK_P_1_1_worldChangeFromQuest;

public:
	CCommonGame();
	virtual ~CCommonGame();

public:
	// Initialize game
	virtual void Init();

	// Shut down game, unload world
	virtual void ShutDown();

	// Overloaded game tick performing additional actions - needs refactor
	virtual void Tick( Float timeDelta );

	// Called before world tick
	virtual void OnTick( Float timeDelta );

	// Tick during post update transform
	virtual void OnTickDuringPostUpdateTransforms( Float timeDelta, Bool updateGui );

	virtual void OnAttachGameplayEntity( CGameplayEntity* entity );

	virtual void OnDetachGameplayEntity( CGameplayEntity* entity );

	//! Global update transform is about to begin
	virtual void OnUpdateTransformBegin();

	//! Global update transform has finished
	virtual void OnUpdateTransformFinished( Bool isMainUpdateTransformPass );

	virtual void OnSerialize( IFile& file );

	void OnControllerEvent( const EControllerEventType& event );

	//! Get world class
	virtual CClass* GetGameWorldClass() const;

	void UnloadWorld() override;

	virtual void RequireLocalizationChange() {}

protected:
	virtual void InitReactionManager();

	virtual void OnGameplaySystemsGameStart( const CGameInfo& info );
	virtual void OnGameplaySystemsGameEnd( const CGameInfo& info );

	virtual void OnGameplaySystemsWorldStart( const CGameInfo& info );
	virtual void OnGameplaySystemsWorldEnd( const CGameInfo& info );

	virtual void OnAfterLoadingScreenGameStart( const CGameInfo& info );

	virtual void OnGameplaySystemsInitialize();
	virtual void OnGameplaySystemsShutdown();

	virtual void OnGameDifficultyChanged( Uint32 previousDifficulty, Uint32 currentDifficulty );

	virtual void PerformCommunitiesFastForward( const Vector& referencePosition, Float timeLimit, Bool resimulateCommunities ) override;
	
	//! Create player entity
	virtual CEntity* CreatePlayer( const CGameInfo& info );

	//! instantiate custom CCreateEntityManager ( called by Init )
	virtual CCreateEntityManager *const InstanciateCreateEntityManager();

	//! Change the player entity
	virtual CEntity* ChangePlayer();

	//! Called before the player is going to be changed
	virtual void OnPreChangePlayer( EntitySpawnInfo& einfo ) {};

	//! Called after the player has been changed
	virtual void OnPostChangePlayer() {};

	//! Create game camera
	virtual CEntity* CreateCamera() const;

	virtual void UnlockMissedAchievements( const CGameInfo& info ) { }

public:
	virtual Bool IsPrimaryPlayer( CPlayer* player ) const { return true; }

	//! Register actor in actor storage update list
	void RegisterEntityForGameplayStorageUpdate( CGameplayEntity* actor );

	//! Register actor for actors managers update
	void RegisterEntityForActorManagerUpdate( CActor* actor );

	//! Flush updates to the gameplay storage
	void FlushGameplayStorageUpdates( Bool async );
	void FinalizeFlushGameplayStorageUpdates() override;

	// Is playing cachet dialog
	virtual Bool IsPlayingCachetDialog() const override;

	//! Get storage for gameplay entities
	RED_INLINE CGameplayStorage* GetGameplayStorage() const { return m_gameplayStorage; }

	//! Get storage for actors
	RED_INLINE CActorsManager* GetActorsManager() const { return m_actorsManager; }

	//! Get the game player
	RED_INLINE CPlayer* GetPlayer() const { return m_player.Get(); }

	//! Get player
	virtual CEntity* GetPlayerEntity() const;

	virtual void OnLanguageChange();

	//! Set player entity
	void SetPlayer( CPlayer* player );

	//! Get active game world
	RED_INLINE CGameWorld* GetActiveWorld() { return (CGameWorld*) m_activeWorld.Get(); }

	//! Get behaviour tree reaction manager
	RED_INLINE CBehTreeReactionManager* GetBehTreeReactionManager() const { return m_behTreeReactionManager; }

	//! Get the entity pool
	RED_INLINE CEntityPool* GetEntityPool() const { return m_entityPool.Get(); }

	RED_INLINE CTemplateLoadBalancer* GetTemplateLoadBalancer() const { return m_templateLoadBalancer; }

	RED_INLINE CEncounterSpawnGroupCounter& GetEncounterSpawnGroup(){ return m_encounerSpawnGroupCounter; }

	virtual void SaveRequiredDLCs( IGameSaver* saver ) override;

	virtual Bool LoadRequiredDLCs( IGameLoader* loader, TDynArray< CName >& missingContent ) override;

	void RequestAutoSave( const String& reason, Bool forced );
	
	Bool IsSavedRecently( Bool checkForAutoSaveOnly ) const;

	// Get save keeper
	virtual CGameSaver* GetGameSaver() { return m_gameSaver; }

	// Makes request for save game
	void RequestGameSave( ESaveGameType type, Int16 slot, const String& reason );

	// Checks if a game save is being made
	Bool IsSavingGame() const;

	virtual void OnLoadingFailed( ESessionRestoreResult sres, const TDynArray< CName >& missingContent ) { RED_UNUSED( sres ); RED_UNUSED( missingContent ); }

	//! Schedule world change
	void ScheduleWorldChange( SChangeWorldInfo& changeWorldInfo );

public:
	virtual bool AnalogScaleformEnabled();

	//  Get's the gui manager for the game
	virtual CGuiManager* GetGuiManager() const { return m_guiManager; }

	// Get the root active menu of the game, returns nullptr if no menu is active
	virtual const CMenu* GetRootMenu() const;

	//Returns true if the game is in the main menu
	virtual Bool IsInMainMenu() const { return false; }

	//Returns true if the game is in the ingame menu
	virtual Bool IsInIngameMenu() const { return false; }

	//! Get item manager
	RED_INLINE CDefinitionsManager* GetDefinitionsManager() const { return m_definitionsManager; }

	//! Get item manager accessor (for script)
	RED_INLINE CDefinitionsManagerAccessor* GetDefinitionsManagerAccessor() const { return m_definitionsManagerAccessor; }

	RED_INLINE CRewardManager* GetRewardManager() const { return m_rewardManager; }

	//! Get community system
	CCommunitySystem* GetCommunitySystem();

	//! Get game reaction manager
	RED_INLINE CReactionsManager* GetReactionsManager() const { return m_reactionsManager; }

	//! Returns an active instance of the global path planner
	RED_INLINE CMoveGlobalPathPlanner* GetPathPlanner() const { return m_pathPlanner; }

	//! Get list of NPC characters
	RED_INLINE const TNPCCharacters& GetNPCCharacters() const { return m_npcCharacters; }

	//! Get the swarm species parameters
	RED_INLINE const CBoidSpecies* GetBoidSpecies() const { return m_boidSpecies; }

	//! Get the In Game Config Wrapper
	RED_INLINE const CInGameConfigWrapper& GetInGameConfigWrapper() const { return m_inGameConfigWrapper; }

	//! Get npc senses manager
	RED_FORCE_INLINE CNewNpcSensesManager* GetNpcSensesManager() const { return m_npcSensesManager; }

	void RegisterNPC( CNewNPC* npc );

	void UnregisterNPC( CNewNPC* npc );

	//Returns EMonsterCategory for the specified npc or -1 if none can be found
	//Should be overridden in game the game-specific child
	virtual Int32 GetMonsterCategoryForNpc(const CNewNPC* npc) const { return -1; }

	void RegisterActor( CActor* actor );

	void UnregisterActor( CActor* actor );

	//! Register behavior tree machine
	void RegisterBehTreeMachine( CBehTreeMachine* machine );

	//! Unregister behavior tree machine
	void UnregisterBehTreeMachine( CBehTreeMachine* machine );

	//! Are subtitles enabled?
	Bool AreSubtitlesEnabled() const;

	// Returns string describing game difficulty
	String NumberToLocDifficulty( Int32 difficulty );

	virtual Bool IsAnyMenu() const override;

	//! Collect stripe component (road, path etc.) (once it's attached to the world)
	virtual void RegisterStripeComponent( CStripeComponent* stripe ) override;

	//! Get rid  stripe component (once it's detached from the world)
	virtual void UnregisterStripeComponent( CStripeComponent* stripe ) override;

	//! Get DLC manager
	RED_INLINE CDLCManager* GetDLCManager() const { return m_dlcManager; }

	//! Populate dlc in-game config group with vars
	void PopulateDlcInGameConfigGroupWithVars();

protected:
	//! Tick behavior tree machines
	void TickBehTreeMachines( Float timeDelta );

	Bool HandleWorldChange();

	void ProcessNpcsAndCameraCollisions();
	void HideAllRenderablesTemporarily( CActor* actor );
	Bool ShouldHideActor( const CMovingPhysicalAgentComponent* const mpac, const Sphere& cameraShpere ) const;

	//! Enable free camera
	void EnableFreeCamera( Bool enable );

	//! Dumb list of all objects to text file
	void DumpListOfObjects();

	void ProcessAutoSaveRequests( Float timeDelta );

	void TryToClearAutoSaveRequests();

public:
	virtual void HandleInGameSavesLoading();

private:
	String GetGameReleaseName() const;

	String GetCurrentLocale() const;

public:
	virtual const Char* GetGamePrefix() const { return TXT("CO"); } // it's a common game

	virtual Bool GetCustomPlayerStartingPoint( const CName& tag, Vector& position, EulerAngles& rotation ) { return false; }

public:
	// For GUI for the moment. Need the to change how the viewport sends us input, deltas are useless if we want a proper hardware mouse
	// since the raw input isn't accounting for it in all cases when we don't have a fullscreen viewport.
	virtual Bool OnViewportTrack( const CMousePacket& packet );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );

	//! Generalized mouse click, windows only
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );

protected:
	virtual Bool Cheat_TeleportPlayer( IViewport* view, Int32 x, Int32 y );

public:
	// Handle engine input
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	virtual void OnViewportKillFocus( IViewport* view );

	virtual void OnViewportSetFocus( IViewport* view );

	// Handle common debug keys
	virtual Bool OnViewportInputDebugAlways( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	// Handle game-time only debug keys (includes common debug keys too)
	virtual Bool OnViewportInputDebugInGameOnly( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	// Handle editor-time only debug keys (includes common debug keys too)
	virtual Bool OnViewportInputDebugInEditor( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! Override camera
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

	//! Set new dimensions of a viewport
	virtual void OnViewportSetDimensions ( IViewport* view );

	//! Called when the game time is arbitrarily changed.
	virtual void OnGameTimeChanged();

	//! Cutscene event
	virtual void OnCutsceneEvent( const String& csName, const CName& csEvent );

	//! Focus mode visibility was changed for the entity
	virtual void OnFocusModeVisibilityChanged( CGameplayEntity* entity, Bool persistent ) { }

	virtual void GiveRewardTo( CName rewardName, CEntity * targetEnt );
	virtual void GiveRewardTo( CName rewardName, CName targetTag );
#ifdef REWARD_EDITOR
	virtual const SReward* GetReward( CName rewardName );
#else
	RED_INLINE const SReward* GetReward( CName rewardName ) { return m_rewardManager ? m_rewardManager->GetReward( rewardName ) : nullptr; }
#endif

	// GUI
	virtual void RequestMenu( const CName& menuName, const THandle< IScriptable >& initData );
	virtual void CloseMenu( const CName& menuName );
	virtual void RequestPopup( const CName& popupName, const THandle< IScriptable >& initData );
	virtual void ClosePopup( const CName& popupName );

	// R4-R6 object spawning
	// TODO: Rethink this attitude to object spawning
	virtual CClass* CBehTreeInstanceClass();

	virtual CLootDefinitions*	CreateLootDefinitions();
	virtual ILootManager*		GetLootManager();
	virtual CInteractionsManager*	GetInteractionsManager();

	SExplorationQueryToken QueryExplorationSync( CEntity* entity, SExplorationQueryContext& queryContext );

	void OnReloadedConfig() override;
	virtual Bool IsPositionInInterior( const Vector& position ) { return false; }



// Iterators
public:
	class ActorIterator : public CollectionIterator< CActor, TList >
	{
	public:
		ActorIterator() : CollectionIterator( GCommonGame->m_actors ) {}
	};

public:
	void SetGameResourceFilenameToStart( const String& gameResourceFilename );
	ELoadGameResult SetSavegameToLoad( const SSavegameInfo& info );
	void SetDLCToStart( CName name, Int32 difficulty );

	Bool TryLoadSavegame();
	Bool TryStartDLC();

	IGameLoader* CreateDLCLoader( CName nameOfTheDLC );

	Bool CanStartStandaloneDLC( CName name ) const;
	Bool TryLoadGameResource();

	virtual void OnUserEvent( const EUserEvent& event ) override;

	Bool AreConfigsResetInThisSession();

// GameSystems support
public:
	RED_INLINE IGameSystem* GetSystem( Int32 gameSystemId ) const
	{
		ASSERT( GIsCooker || gameSystemId < m_gameSystems.SizeInt() );
		return ( m_gameSystems.SizeInt() > gameSystemId ) ? m_gameSystems[ gameSystemId ] : nullptr;
	}

	template< class T >
	T* GetSystem() const { return static_cast< T* > ( GetSystem( T::GetSystemId() ) ); }	

	friend void InitializeR4GameSystems( CCommonGame *game );
	friend void InitializeR6GameSystems( CCommonGame *game );
	friend void ShutdownR4GameSystems( CCommonGame *game );
	friend void ShutdownR6GameSystems( CCommonGame *game );

private:
	virtual Bool StartGame_AreQuestsStable() const override;
	virtual void StartGame_ResetQuestStability() override;
	virtual void StartGame_WaitForPlayer() override;
	virtual void StartGame_InitHUD() override;
	virtual void StartGame_UpdateActors() override;
#ifndef RED_FINAL_BUILD
	virtual void StartGame_DebugDumpUnstableQuestThreads() const override;
#endif

	virtual Bool StartGame_UpdateSaver() override;

	virtual void OnGamePrefetchStart( Bool showLoadingScreen ) override;
	virtual void OnGamePrefetchEnd( Bool showLoadingScreen ) override;

	virtual void OnLoadingScreenShown() override;

protected:
	virtual void SetStreamingReferencePosition( const Vector& referencePosition ) override;


	virtual void HACK_TickStorySceneVideoElementInstance( Float timeDelta ) override;

	template< class T >
	void AddSystem( T* gameSystem )
	{
		IGameSystem* typeCheck = static_cast< IGameSystem* >( gameSystem );	// unused casted variable put here to ensure type consistency
		RED_UNUSED( typeCheck );
		m_gameSystems[ T::GetSystemId() ] = gameSystem;
	}

public:

	RED_INLINE void HACK_P_1_1_SetWorldChangeFromQuest( Bool val ) { m_HACK_P_1_1_worldChangeFromQuest = val; }

	Bool IsNewGamePlus() const;

public:
	virtual Bool OnSuspend() override;
	virtual Bool OnResume() override;


// Script functions
	void funcEnableSubtitles( CScriptStackFrame& stack, void* result );
	void funcAreSubtitlesEnabled( CScriptStackFrame& stack, void* result );
	void funcGetReactionsMgr( CScriptStackFrame& stack, void* result );
	void funcGetIngredientCategoryElements( CScriptStackFrame& stack, void* result );
	void funcIsIngredientCategorySpecified( CScriptStackFrame& stack, void* result );
	void funcGetIngredientCathegories( CScriptStackFrame& stack, void* result );
	void funcGetSetItems( CScriptStackFrame& stack, void* result );
	void funcGetItemSetAbilities( CScriptStackFrame& stack, void* result );
	void funcGetDefinitionsManager( CScriptStackFrame& stack, void* result );
	void funcQueryExplorationSync( CScriptStackFrame& stack, void* result );
	void funcQueryExplorationFromObjectSync( CScriptStackFrame& stack, void* result );
	void funcGetGlobalAttitude( CScriptStackFrame& stack, void* result );
	void funcSetGlobalAttitude( CScriptStackFrame& stack, void* result );
	void funcGetReward( CScriptStackFrame& stack, void* result );

	void funcLoadLastGameInit( CScriptStackFrame& stack, void* result );
	void funcLoadGameInit( CScriptStackFrame& stack, void* result );
	void funcCanStartStandaloneDLC( CScriptStackFrame& stack, void* result );
	void funcInitStandaloneDLCLoading( CScriptStackFrame& stack, void* result );
	void funcGetLoadGameProgress( CScriptStackFrame& stack, void* result );
	void funcListSavedGames( CScriptStackFrame& stack, void* result );
	void funcImportSave( CScriptStackFrame& stack, void* result );
	void funcGetDisplayNameForSavedGame( CScriptStackFrame& stack, void* result );
	void funcListW2SavedGames( CScriptStackFrame& stack, void* result );
	void funcSaveGame( CScriptStackFrame& stack, void* result );
	void funcDeleteSavedGame( CScriptStackFrame& stack, void* result );
	void funcGetSaveInSlot( CScriptStackFrame& stack, void* result );
	void funcGetNumSaveSlots( CScriptStackFrame& stack, void* result );
	void funcGetContentRequiredByLastSave( CScriptStackFrame& stack, void* result );
	void funcRequestScreenshotData( CScriptStackFrame& stack, void* result );
	void funcIsScreenshotDataReady( CScriptStackFrame& stack, void* result );
	void funcFreeScreenshotData( CScriptStackFrame& stack, void* result );
	void funcIsDisplayNameAvailable( CScriptStackFrame& stack, void* result );
	void funcShouldShowSaveCompatibilityWarning( CScriptStackFrame& stack, void* result );
	void funcIsNewGame( CScriptStackFrame& stack, void* result );
	void funcIsNewGameInStandaloneDLCMode( CScriptStackFrame& stack, void* result );
	void funcIsNewGamePlusEnabled( CScriptStackFrame& stack, void* result ); 

	void funcCenterMouse(CScriptStackFrame& stack, void* result );
	void funcMoveMouseTo(CScriptStackFrame& stack, void* result );

	void funcRequestNewGame( CScriptStackFrame& stack, void* result );
	void funcRequestEndGame( CScriptStackFrame& stack, void* result );
	void funcRequestExit( CScriptStackFrame& stack, void* result );
	void funcGetGameResourceList( CScriptStackFrame& stack, void* result );
	void funcGetGameRelease( CScriptStackFrame& stack, void* result );
	void funcGetCurrentLocale( CScriptStackFrame& stack, void* result );
	void funcGetPlayer( CScriptStackFrame& stack, void* result );
	void funcGetNPCByName( CScriptStackFrame& stack, void* result );
	void funcGetNPCByTag( CScriptStackFrame& stack, void* result );
	void funcGetNPCsByTag( CScriptStackFrame& stack, void* result );
	void funcGetAllNPCs( CScriptStackFrame& stack, void* result );
	void funcGetActorByTag( CScriptStackFrame& stack, void* result );
	void funcGetActorsByTag( CScriptStackFrame& stack, void* result );
	void funcGetAPManager( CScriptStackFrame& stack, void* result );
	void funcGetStorySceneSystem( CScriptStackFrame& stack, void* result );	
	void funcAddStateChangeRequest( CScriptStackFrame& stack, void* result );
	void funcCreateNoSaveLock( CScriptStackFrame& stack, void* result );
	void funcReleaseNoSaveLock( CScriptStackFrame& stack, void* result );
	void funcReleaseNoSaveLockByName( CScriptStackFrame& stack, void* result );
	void funcAreSavesLocked( CScriptStackFrame& stack, void* result );
	void funcGetGameLanguageId( CScriptStackFrame& stack, void* result );
	void funcGetGameLanguageName( CScriptStackFrame& stack, void* result );
	void funcGetGameLanguageIndex( CScriptStackFrame& stack, void* result );
	void funcGetAllAvailableLanguages( CScriptStackFrame& stack, void* result );
	void funcSwitchGameLanguageByIndex( CScriptStackFrame& stack, void* result );
	void funcReloadLanguage( CScriptStackFrame& stack, void* result );
	void funcIsGameTimePaused( CScriptStackFrame& stack, void* result );
	void funcIsInvertCameraX( CScriptStackFrame& stack, void* result );
	void funcIsInvertCameraY( CScriptStackFrame& stack, void* result );
	void funcSetInvertCameraX( CScriptStackFrame& stack, void* result );
	void funcSetInvertCameraY( CScriptStackFrame& stack, void* result );
	void funcSetInvertCameraXOnMouse( CScriptStackFrame& stack, void* result );
	void funcSetInvertCameraYOnMouse( CScriptStackFrame& stack, void* result );
	void funcIsCameraAutoRotX( CScriptStackFrame& stack, void* result );
	void funcIsCameraAutoRotY( CScriptStackFrame& stack, void* result );
	void funcSetCameraAutoRotX( CScriptStackFrame& stack, void* result );
	void funcSetCameraAutoRotY( CScriptStackFrame& stack, void* result );
	void funcAreSavesInitialized( CScriptStackFrame& stack, void* result );
	void funcChangePlayer( CScriptStackFrame& stack, void* result );
	void funcConfigSave( CScriptStackFrame& stack, void* result );
	void funcScheduleWorldChangeToMapPin( CScriptStackFrame& stack, void* result );
	void funcScheduleWorldChangeToPosition( CScriptStackFrame& stack, void* result );
	void funcGetBehTreeReactionManager( CScriptStackFrame& stack, void* result );
	void funcForceUIAnalog( CScriptStackFrame& stack, void* result );
	void funcRequestMenu( CScriptStackFrame& stack, void* result );
	void funcCloseMenu( CScriptStackFrame& stack, void* result );
	void funcRequestPopup( CScriptStackFrame& stack, void* result );
	void funcClosePopup( CScriptStackFrame& stack, void* result );
	void funcGetHud( CScriptStackFrame& stack, void* result );
	void funcGetInGameConfigWrapper( CScriptStackFrame& stack, void* result );
	void funcGetCommunitySystem( CScriptStackFrame& stack, void* result );
	void funcGetAttackRangeForEntity( CScriptStackFrame& stack, void* result );
	void funcGiveReward( CScriptStackFrame& stack, void* result );
	void funcConvertToStrayActor( CScriptStackFrame& stack, void* result );
	void funcCreateEntityAsync( CScriptStackFrame& stack, void* result );
	void funcTestNoCreaturesOnLocation( CScriptStackFrame& stack, void* result );
	void funcTestNoCreaturesOnLine( CScriptStackFrame& stack, void* result );
	void funcRequestAutoSave( CScriptStackFrame& stack, void* result );
	void funcCalculateTimePlayed( CScriptStackFrame& stack, void* result );
	void funcGetUIHorizontalPlusFrameScale( CScriptStackFrame& stack, void* result );
	void funcGetDLCManager( CScriptStackFrame& stack, void* result );
	void funcAreConfigResetInThisSession( CScriptStackFrame& stack, void* result );
	void funcHasShownConfigChangedMessage( CScriptStackFrame& stack, void* result );
	void funcSetHasShownConfigChangedMessage( CScriptStackFrame& stack, void* result );
	void funcGetApplicationVersion( CScriptStackFrame& stack, void* result );

	void funcIsSoftwareCursor( CScriptStackFrame& stack, void* result );
	void funcShowHardwareCursor( CScriptStackFrame& stack, void* result );
	void funcHideHardwareCursor( CScriptStackFrame& stack, void* result );

	void funcGetAchievementsDisabled( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( CCommonGame )
	PARENT_CLASS( CGame )
	PROPERTY( m_player );
	PROPERTY( m_dlcManager );
	NATIVE_FUNCTION( "ScheduleWorldChangeToMapPin", funcScheduleWorldChangeToMapPin );
	NATIVE_FUNCTION( "ScheduleWorldChangeToPosition", funcScheduleWorldChangeToPosition )
	NATIVE_FUNCTION( "EnableSubtitles", funcEnableSubtitles );
	NATIVE_FUNCTION( "AreSubtitlesEnabled", funcAreSubtitlesEnabled );
	NATIVE_FUNCTION( "GetReactionsMgr", funcGetReactionsMgr );
	NATIVE_FUNCTION( "GetIngredientCategoryElements", funcGetIngredientCategoryElements );
	NATIVE_FUNCTION( "IsIngredientCategorySpecified", funcIsIngredientCategorySpecified );
	NATIVE_FUNCTION( "GetIngredientCathegories", funcGetIngredientCathegories );
	NATIVE_FUNCTION( "GetSetItems", funcGetSetItems );
	NATIVE_FUNCTION( "GetItemSetAbilities", funcGetItemSetAbilities );
	NATIVE_FUNCTION( "GetDefinitionsManager", funcGetDefinitionsManager );
	NATIVE_FUNCTION( "QueryExplorationSync", funcQueryExplorationSync );
	NATIVE_FUNCTION( "QueryExplorationFromObjectSync", funcQueryExplorationFromObjectSync );
	NATIVE_FUNCTION( "GetGlobalAttitude", funcGetGlobalAttitude );
	NATIVE_FUNCTION( "SetGlobalAttitude", funcSetGlobalAttitude );
	NATIVE_FUNCTION( "GetReward", funcGetReward );

	NATIVE_FUNCTION( "LoadLastGameInit", funcLoadLastGameInit );
	NATIVE_FUNCTION( "LoadGameInit", funcLoadGameInit );
	NATIVE_FUNCTION( "CanStartStandaloneDLC", funcCanStartStandaloneDLC );
	NATIVE_FUNCTION( "InitStandaloneDLCLoading", funcInitStandaloneDLCLoading );
	NATIVE_FUNCTION( "GetLoadGameProgress", funcGetLoadGameProgress );
	NATIVE_FUNCTION( "ListSavedGames", funcListSavedGames );
	NATIVE_FUNCTION( "ImportSave", funcImportSave );
	NATIVE_FUNCTION( "GetDisplayNameForSavedGame", funcGetDisplayNameForSavedGame );
	NATIVE_FUNCTION( "ListW2SavedGames", funcListW2SavedGames );
	NATIVE_FUNCTION( "SaveGame", funcSaveGame );
	NATIVE_FUNCTION( "DeleteSavedGame", funcDeleteSavedGame );
	NATIVE_FUNCTION( "GetNumSaveSlots", funcGetNumSaveSlots );
	NATIVE_FUNCTION( "GetSaveInSlot", funcGetSaveInSlot );
	NATIVE_FUNCTION( "GetContentRequiredByLastSave", funcGetContentRequiredByLastSave );
	NATIVE_FUNCTION( "RequestScreenshotData", funcRequestScreenshotData );
	NATIVE_FUNCTION( "IsScreenshotDataReady", funcIsScreenshotDataReady );
	NATIVE_FUNCTION( "FreeScreenshotData", funcFreeScreenshotData );
	NATIVE_FUNCTION( "IsDisplayNameAvailable", funcIsDisplayNameAvailable );
	NATIVE_FUNCTION( "ShouldShowSaveCompatibilityWarning", funcShouldShowSaveCompatibilityWarning );
	NATIVE_FUNCTION( "IsNewGame", funcIsNewGame ); 
	NATIVE_FUNCTION( "IsNewGameInStandaloneDLCMode", funcIsNewGameInStandaloneDLCMode ); 
	NATIVE_FUNCTION( "IsNewGamePlusEnabled", funcIsNewGamePlusEnabled ); 

	NATIVE_FUNCTION( "CenterMouse", funcCenterMouse );
	NATIVE_FUNCTION( "MoveMouseTo", funcMoveMouseTo );

	NATIVE_FUNCTION( "RequestNewGame", funcRequestNewGame );
	NATIVE_FUNCTION( "RequestEndGame", funcRequestEndGame );
	NATIVE_FUNCTION( "RequestExit", funcRequestExit );
	NATIVE_FUNCTION( "GetGameResourceList", funcGetGameResourceList );
	NATIVE_FUNCTION( "GetGameRelease", funcGetGameRelease );
	NATIVE_FUNCTION( "GetCurrentLocale", funcGetCurrentLocale );
	NATIVE_FUNCTION( "GetPlayer", funcGetPlayer );
	NATIVE_FUNCTION( "GetNPCByTag", funcGetNPCByTag );
	NATIVE_FUNCTION( "GetNPCsByTag", funcGetNPCsByTag );
	NATIVE_FUNCTION( "GetAllNPCs", funcGetAllNPCs );
	NATIVE_FUNCTION( "GetActorByTag", funcGetActorByTag);
	NATIVE_FUNCTION( "GetActorsByTag", funcGetActorsByTag);
	NATIVE_FUNCTION( "GetAPManager", funcGetAPManager );
	NATIVE_FUNCTION( "GetStorySceneSystem", funcGetStorySceneSystem );	
	NATIVE_FUNCTION( "AddStateChangeRequest", funcAddStateChangeRequest );
	NATIVE_FUNCTION( "CreateNoSaveLock", funcCreateNoSaveLock );
	NATIVE_FUNCTION( "ReleaseNoSaveLock", funcReleaseNoSaveLock );
	NATIVE_FUNCTION( "ReleaseNoSaveLockByName", funcReleaseNoSaveLockByName );
	NATIVE_FUNCTION( "AreSavesLocked", funcAreSavesLocked );
	NATIVE_FUNCTION( "GetGameLanguageId", funcGetGameLanguageId );
	NATIVE_FUNCTION( "GetGameLanguageName", funcGetGameLanguageName );
	NATIVE_FUNCTION( "GetGameLanguageIndex", funcGetGameLanguageIndex );
	NATIVE_FUNCTION( "GetAllAvailableLanguages",funcGetAllAvailableLanguages );
	NATIVE_FUNCTION( "SwitchGameLanguageByIndex", funcSwitchGameLanguageByIndex );
	NATIVE_FUNCTION( "ReloadLanguage", funcReloadLanguage );
	NATIVE_FUNCTION( "IsGameTimePaused", funcIsGameTimePaused );
	NATIVE_FUNCTION( "IsInvertCameraX", funcIsInvertCameraX );
	NATIVE_FUNCTION( "IsInvertCameraY", funcIsInvertCameraY );
	NATIVE_FUNCTION( "SetInvertCameraX", funcSetInvertCameraX );
	NATIVE_FUNCTION( "SetInvertCameraY", funcSetInvertCameraY );
	NATIVE_FUNCTION( "SetInvertCameraXOnMouse", funcSetInvertCameraXOnMouse );
	NATIVE_FUNCTION( "SetInvertCameraYOnMouse", funcSetInvertCameraYOnMouse );
	NATIVE_FUNCTION( "IsCameraAutoRotX", funcIsCameraAutoRotX );
	NATIVE_FUNCTION( "IsCameraAutoRotY", funcIsCameraAutoRotY );
	NATIVE_FUNCTION( "SetCameraAutoRotX", funcSetCameraAutoRotX );
	NATIVE_FUNCTION( "SetCameraAutoRotY", funcSetCameraAutoRotY );
	NATIVE_FUNCTION( "ConfigSave", funcConfigSave );
	NATIVE_FUNCTION( "AreSavesInitialized", funcAreSavesInitialized );
	NATIVE_FUNCTION( "ChangePlayer", funcChangePlayer );
	NATIVE_FUNCTION( "GetBehTreeReactionManager", funcGetBehTreeReactionManager );
	NATIVE_FUNCTION( "ForceUIAnalog", funcForceUIAnalog );
	NATIVE_FUNCTION( "RequestMenu", funcRequestMenu );
	NATIVE_FUNCTION( "CloseMenu", funcCloseMenu );
	NATIVE_FUNCTION( "RequestPopup", funcRequestPopup );
	NATIVE_FUNCTION( "ClosePopup", funcClosePopup );
	NATIVE_FUNCTION( "GetHud", funcGetHud );
	NATIVE_FUNCTION( "GetInGameConfigWrapper", funcGetInGameConfigWrapper );
	NATIVE_FUNCTION( "GetCommunitySystem", funcGetCommunitySystem );
	NATIVE_FUNCTION( "GetAttackRangeForEntity", funcGetAttackRangeForEntity );
	NATIVE_FUNCTION( "GiveReward", funcGiveReward );
	NATIVE_FUNCTION( "ConvertToStrayActor", funcConvertToStrayActor );
	NATIVE_FUNCTION( "CreateEntityAsync", funcCreateEntityAsync )
	NATIVE_FUNCTION( "TestNoCreaturesOnLocation", funcTestNoCreaturesOnLocation );
	NATIVE_FUNCTION( "TestNoCreaturesOnLine", funcTestNoCreaturesOnLine );
	NATIVE_FUNCTION( "RequestAutoSave", funcRequestAutoSave );
	NATIVE_FUNCTION( "CalculateTimePlayed", funcCalculateTimePlayed );
	NATIVE_FUNCTION( "GetUIHorizontalPlusFrameScale", funcGetUIHorizontalPlusFrameScale );
	NATIVE_FUNCTION( "GetDLCManager", funcGetDLCManager );
	NATIVE_FUNCTION( "AreConfigResetInThisSession", funcAreConfigResetInThisSession );
	NATIVE_FUNCTION( "HasShownConfigChangedMessage", funcHasShownConfigChangedMessage );
	NATIVE_FUNCTION( "SetHasShownConfigChangedMessage", funcSetHasShownConfigChangedMessage );
	NATIVE_FUNCTION( "GetApplicationVersion", funcGetApplicationVersion );

	NATIVE_FUNCTION( "IsSoftwareCursor", funcIsSoftwareCursor );
	NATIVE_FUNCTION( "ShowHardwareCursor", funcShowHardwareCursor );
	NATIVE_FUNCTION( "HideHardwareCursor", funcHideHardwareCursor );

	NATIVE_FUNCTION( "GetAchievementsDisabled", funcGetAchievementsDisabled );
END_CLASS_RTTI()
