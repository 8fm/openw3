/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/objectGC.h"
#include "../../common/game/commonGame.h"
#include "ticketGlobalSource.h"
#include "carryableItemsRegistry.h"
#include "gameParams.h"

class CSwarmSpecies;
class CR4TelemetryScriptProxy;
class CR4CreateEntityManager;
class CR4SecondScreenManagerScriptProxy;
class CR4KinectSpeechRecognizerListener;
class CR4KinectSpeechRecognizerListenerScriptProxy;
class CR4WorldDLCExtender;

struct SUsedFastTravelEvent;

#if !defined( NO_TELEMETRY )
#if !defined( NO_DEBUG_DATA_SERVICE )
#include "r4DebugDataServiceLogOutputDevice.h"
#endif
#endif
#include "profilerConsoleCommands.h"

#define RESERVED_TAG_ID_INDEX_GERALT			0
#define RESERVED_TAG_ID_INDEX_CIRI				1
#define RESERVED_TAG_ID_INDEX_GERALT_HORSE		2
#define RESERVED_TAG_ID_INDEX_CIRI_HORSE		3
#define RESERVED_TAG_ID_INDEX_GERALT_BOAT		4

enum ENewGamePlusStatus
{
	NGP_Success,				// when everything went fine
	NGP_Invalid,				// when passed SSavegameInfo is not a valid one
	NGP_CantLoad,				// when save data cannot be loaded (like: corrupted save data or missing DLC)
	NGP_TooOld,					// when save data is made on unpatched game version and needs to be resaved
	NGP_RequirementsNotMet,		// when player didn't finish the game or have too low level
	NGP_InternalError,			// shouldn't happen at all, means that we have internal problems with the game
	NGP_ContentRequired,		// when the game is not fully installed
	NGP_WrongGameVersion,		// when tried to start NGP on unsupported game version
};

BEGIN_ENUM_RTTI( ENewGamePlusStatus )
	ENUM_OPTION( NGP_Success )
	ENUM_OPTION( NGP_Invalid )
	ENUM_OPTION( NGP_CantLoad )
	ENUM_OPTION( NGP_TooOld )
	ENUM_OPTION( NGP_RequirementsNotMet )
	ENUM_OPTION( NGP_InternalError )
	ENUM_OPTION( NGP_ContentRequired )
	ENUM_OPTION( NGP_WrongGameVersion )
END_ENUM_RTTI()

class CR4GCOverlayHelper : public IObjectGCHelper
{
private:
	Bool m_isBlurred;

public:
	virtual void OnGCStarting() override;
	virtual void OnGCFinished() override;
};

/// R4 game pointer
extern CR4Game* GR4Game;

class CR4Game : public CCommonGame
{
	DECLARE_ENGINE_CLASS( CR4Game, CCommonGame, 0 );

	static void InitializeGameSystems( CCommonGame *game );
	static void ShutdownGameSystems( CCommonGame *game );

protected:
	CTicketsDefaultConfiguration*					m_ticketsDefaultConfiguration;
	THandle< CGlabalTicketSourceProvider >			m_globalTicketSource;
	THandle< CCarryableItemsRegistry >				m_carryableItemsRegistry;

	THandle<CCustomCamera>	m_horseCamera;
	CR4TelemetryScriptProxy*						m_telemetryScriptProxy;				//!< This is wrapper for calling telemetry logging methods from the scripts
	CR4SecondScreenManagerScriptProxy*				m_secondScreenScriptProxy;			//!< This is wrapper for calling second screen manager methods from the scripts
	CR4GwintManager*								m_gwintManager;
	class CDynamicTagsContainer*					m_dynamicTagsContainer;
	Bool											m_constrainedModeBecauseOfASystemMessage : 1;

#if !defined( NO_TELEMETRY )
	Float											m_accumulatedTime;
	#if !defined( NO_DEBUG_DATA_SERVICE )
		CR4DebugDataServiceLogOutputDevice			m_debugDataLogger;
	#endif
#endif // NO_TELEMETRY

#if defined(RED_KINECT)
	CR4KinectSpeechRecognizerListener*				m_kinectSpeechRecognizerListener;
#endif //NO_KINECT

	CR4KinectSpeechRecognizerListenerScriptProxy*	m_kinectSpeechRecognizerListenerScriptProxy;

	static CR4ProfilerScriptRegistration			s_scriptProfilerCommands;	
	static const String								CONSTRAINED_MODE_PAUSE_REASON;

	class CR4GlobalEventsScriptsDispatcher*			m_globalEventsScriptsDispatcher;

	class CMainMenuController*						m_mainMenuController;

	THandle< W3GameParams >							m_params;						//!< Game params managed from scripts

	CR4GCOverlayHelper*								m_gcOverlayHelper;

	CR4WorldDLCExtender*							m_worldDLCExtender;
	
public:
	CR4Game();

	// Initialize game
	virtual void Init();

	// Shut down game, unload world
	virtual void ShutDown();

	void OnAttachGameplayEntity( CGameplayEntity* entity );
	void OnDetachGameplayEntity( CGameplayEntity* entity );

	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

	virtual void Tick(  Float timeDelta );

	virtual void OnSerialize( IFile& file );

	virtual void OnUserEvent( const EUserEvent& event );

	//! EG. Constrained mode is trigged when xbox home button is pressed
	virtual void EnteredConstrainedMode();
	virtual void ExitedConstrainedMode();

	virtual void RequireLocalizationChange() override;

	//Returns true if the main menu is active
	virtual Bool IsInMainMenu() const;

	//Returns true if the game is in common ingame menu
	virtual Bool IsInIngameMenu() const;
	
	RED_FORCE_INLINE CR4WorldDLCExtender* GetWorldDLCExtender() const { return m_worldDLCExtender; }

protected:
	void PauseIfNeeded();

	void InitReactionManager() override;
	//! Initialize gameplay only sub system, called AFTER game world has been loaded
	virtual void OnGameplaySystemsWorldStart( const CGameInfo& info );

	//! Close gameplay only sub system, called BEFORE game world has been unloaded
	virtual void OnGameplaySystemsWorldEnd( const CGameInfo& info );

	virtual void OnGameplaySystemsGameStart( const CGameInfo& info ) override;

	virtual void OnGameStart( const CGameInfo& gameInfo );

	//! Create player entity
	virtual CEntity* CreatePlayer( const CGameInfo& info );

	//! Called before the player is going to be changed
	virtual void OnPreChangePlayer( EntitySpawnInfo& einfo );

	//! Called after the player has been changed
	virtual void OnPostChangePlayer();

	//! Create game camera
	virtual CEntity* CreateCamera() const;

	virtual Bool ActivateGameCamera( Float blendTime, Bool useFocusTarget = false, Bool force = true, Bool reset = true ) const override;
	virtual void ResetGameplayCamera() override;
	virtual Bool GetGameplayCameraData( ICamera::Data& data ) override;
	virtual void StartGameplayCameraBlendFrom( const ICamera::Data& data, Float blendTime ) override;
	virtual CEntity* GetGameplayCameraEntity() const override;

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! Focus mode visibility was changed for the entity
	virtual void OnFocusModeVisibilityChanged( CGameplayEntity* entity, Bool persistent ) override;

#ifdef ENABLE_REVIEW_WORKAROUNDS
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
#endif

public:

	//! Get game camera
	CCustomCamera* GetCamera() const;

	virtual Bool SetupGameResourceFromFile( const String& filePath );

	void SetupGameResource();
	ENewGamePlusStatus StartNewGamePlus( const SSavegameInfo& info );
	virtual Bool OnSaveGame( IGameSaver* saver );

	virtual void OnLoadingFailed( ESessionRestoreResult sres, const TDynArray< CName >& missingContent );

	RED_INLINE CTicketsDefaultConfiguration*	GetDefaultTicketsConfiguration() const	{ return m_ticketsDefaultConfiguration; }
	RED_INLINE CGlabalTicketSourceProvider*		GetGlabalTicketSourceProvider()  const	{ return m_globalTicketSource.Get(); }
	RED_INLINE CCarryableItemsRegistry*			GetCarryableItemsRegistry()	const		{ return m_carryableItemsRegistry.Get(); }
	RED_INLINE W3GameParams*					GetGameParams() const					{ return m_params.Get(); }
   CR4CreateEntityManager *const GetR4CreateEntityManager() const;

	Bool GetCustomPlayerStartingPoint( const CName& tag, Vector& position, EulerAngles& rotation ) override;

	Bool IsPositionInInterior( const Vector& position ) override;
	
	//Returns EMonsterCategory for the specified npc or -1 if none can be found
	virtual Int32 GetMonsterCategoryForNpc(const CNewNPC* npc) const;
	
	const CDynamicTagsContainer* const GetDynamicTagsContainer() const			{ return m_dynamicTagsContainer; }

public:
	//! World map event
	virtual void OnUsedFastTravelEvent( const SUsedFastTravelEvent& event );

	CClass* CBehTreeInstanceClass() override;
	CLootDefinitions*	CreateLootDefinitions() override;
	ILootManager*		GetLootManager() override;
	CInteractionsManager*	GetInteractionsManager() override;

	virtual const Char* GetGamePrefix() const { return TXT("R4"); }
public:
	// Pause game
	void Pause( const String& reason );

	// Unpause game
	void Unpause( const String& reason );

public:
	virtual Bool TickMainMenu( float timeDelta );
	void CloseMainMenu();

	Bool IsPrimaryPlayer( CPlayer* player ) const override;

public:
	// Notifies language dependent systems
	virtual void OnLanguageChange();

protected:
	void UnlockMissedAchievements( const CGameInfo& info ) override;
private:
	void TryUnlockAchievement( Bool shouldUnlock, CName achievementName );

protected :
	CCreateEntityManager *const InstanciateCreateEntityManager() override;
	
private:
	void funcActivateHorseCamera( CScriptStackFrame& stack, void* result );
	void funcGetFocusModeController( CScriptStackFrame& stack, void* result );
	void funcGetSurfacePostFX( CScriptStackFrame& stack, void* result );	
	void funcGetCommonMapManager( CScriptStackFrame& stack, void* result );
	void funcGetJournalManager( CScriptStackFrame& stack, void* result );
	void funcGetLootManager( CScriptStackFrame& stack, void* result );
	void funcGetCityLightManager( CScriptStackFrame& stack, void* result );
	void funcGetInteractionsManager( CScriptStackFrame& stack, void* result );
	void funcGetGuiManager( CScriptStackFrame& stack, void* result );
	void funcGetGlobalEventsScriptsDispatcher( CScriptStackFrame& stack, void* result );
	void funcGetFastForwardSystem( CScriptStackFrame& stack, void* result );
	void funcStartSepiaEffect( CScriptStackFrame& stack, void* result );
	void funcStopSepiaEffect( CScriptStackFrame& stack, void* result );
	void funcGetWindAtPoint( CScriptStackFrame& stack, void* result );
	void funcGetWindAtPointForVisuals( CScriptStackFrame& stack, void* result );
	void funcGetGameCamera( CScriptStackFrame& stack, void* result );
	void funcGetBuffImmunitiesForActor( CScriptStackFrame& stack, void* result );
	void funcGetMonsterParamsForActor( CScriptStackFrame& stack, void* result );
	void funcGetMonsterParamForActor( CScriptStackFrame& stack, void* result );
	void funcGetVolumePathManager( CScriptStackFrame& stack, void* result );
	void funcSummonPlayerHorse( CScriptStackFrame& stack, void* result );
	void funcGetSecondScreenManager( CScriptStackFrame& stack, void* result );
	void funcGetKinectSpeechRecognizer( CScriptStackFrame& stack, void* result );

	void funcToggleMenus( CScriptStackFrame& stack, void* result );
	void funcToggleInput( CScriptStackFrame& stack, void* result );

	void funcNotifyOpeningJournalEntry( CScriptStackFrame& stack, void* result );

	void funcGetResourceAliases( CScriptStackFrame& stack, void* result );
	void funcGetTutorialSystem( CScriptStackFrame& stack, void* result );
	void funcOnUserDialogCallback( CScriptStackFrame& stack, void* result );

	void funcSetActiveUserPromiscuous( CScriptStackFrame& stack, void* result );
	void funcChangeActiveUser(CScriptStackFrame& stack, void* result );
	void funcGetActiveUserDisplayName(CScriptStackFrame& stack, void* result );
	void funcGetPlatform(CScriptStackFrame& stack, void* result );
	void funcIsContentAvailable(CScriptStackFrame& stack, void* result );
	void funcProgressToContentAvailable(CScriptStackFrame& stack, void* result );
	void funcShouldForceInstallVideo(CScriptStackFrame& stack, void* result );

	void funcDisplaySystemHelp( CScriptStackFrame& stack, void* result );
	void funcDisplayStore( CScriptStackFrame& stack, void* result );
	void funcDisplayUserProfileSystemDialog( CScriptStackFrame& stack, void* result );
	void funcSetRichPresence( CScriptStackFrame& stack, void* result );

	void funcSaveUserSettings( CScriptStackFrame& stack, void* result );

	void funcGetGwintManager( CScriptStackFrame& stack, void* result );

	void funcIsDebugQuestMenuEnabled( CScriptStackFrame& stack, void* result );

	void funcEnableNewGamePlus( CScriptStackFrame& stack, void* result );
	void funcStartNewGamePlus( CScriptStackFrame& stack, void* result );

	void funcWorldDLCExtender( CScriptStackFrame& stack, void* result );
	void funcShowSteamControllerBindingPanel( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CR4Game );
	PARENT_CLASS( CCommonGame );
	PROPERTY_NOSERIALIZE( m_horseCamera );
	PROPERTY_NOSERIALIZE( m_telemetryScriptProxy );
	PROPERTY_NOSERIALIZE( m_secondScreenScriptProxy );
	PROPERTY_NOSERIALIZE( m_kinectSpeechRecognizerListenerScriptProxy );
	PROPERTY_NOSERIALIZE( m_ticketsDefaultConfiguration );
	PROPERTY_NOSERIALIZE( m_globalEventsScriptsDispatcher );
	PROPERTY_NOSERIALIZE( m_worldDLCExtender );
	PROPERTY( m_globalTicketSource );
	PROPERTY( m_carryableItemsRegistry );
	PROPERTY( m_params );
	NATIVE_FUNCTION( "ActivateHorseCamera", funcActivateHorseCamera );
	NATIVE_FUNCTION( "GetFocusModeController", funcGetFocusModeController );
	NATIVE_FUNCTION( "GetSurfacePostFX", funcGetSurfacePostFX );
	NATIVE_FUNCTION( "GetCommonMapManager", funcGetCommonMapManager );
	NATIVE_FUNCTION( "GetJournalManager", funcGetJournalManager );
	NATIVE_FUNCTION( "GetLootManager", funcGetLootManager );
	NATIVE_FUNCTION( "GetCityLightManager", funcGetCityLightManager );
	NATIVE_FUNCTION( "GetInteractionsManager", funcGetInteractionsManager );
	NATIVE_FUNCTION( "GetGuiManager", funcGetGuiManager );
	NATIVE_FUNCTION( "GetGlobalEventsScriptsDispatcher", funcGetGlobalEventsScriptsDispatcher );
	NATIVE_FUNCTION( "GetFastForwardSystem", funcGetFastForwardSystem );
	NATIVE_FUNCTION( "StartSepiaEffect", funcStartSepiaEffect );
	NATIVE_FUNCTION( "StopSepiaEffect", funcStopSepiaEffect );
	NATIVE_FUNCTION( "GetWindAtPoint", funcGetWindAtPoint );
	NATIVE_FUNCTION( "GetWindAtPointForVisuals", funcGetWindAtPointForVisuals );
	NATIVE_FUNCTION( "GetGameCamera", funcGetGameCamera );
	NATIVE_FUNCTION( "GetBuffImmunitiesForActor",   funcGetBuffImmunitiesForActor );
    NATIVE_FUNCTION( "GetMonsterParamsForActor", funcGetMonsterParamsForActor );
	NATIVE_FUNCTION( "GetMonsterParamForActor", funcGetMonsterParamForActor );
	NATIVE_FUNCTION( "SummonPlayerHorse", funcSummonPlayerHorse );
	NATIVE_FUNCTION( "GetVolumePathManager", funcGetVolumePathManager );
	NATIVE_FUNCTION( "ToggleMenus", funcToggleMenus );
	NATIVE_FUNCTION( "ToggleInput", funcToggleInput );
	NATIVE_FUNCTION( "NotifyOpeningJournalEntry", funcNotifyOpeningJournalEntry );
	NATIVE_FUNCTION( "GetSecondScreenManager", funcGetSecondScreenManager );
	NATIVE_FUNCTION( "GetKinectSpeechRecognizer", funcGetKinectSpeechRecognizer );
	NATIVE_FUNCTION( "GetResourceAliases", funcGetResourceAliases );
	NATIVE_FUNCTION( "GetTutorialSystem", funcGetTutorialSystem );
	NATIVE_FUNCTION( "DisplaySystemHelp", funcDisplaySystemHelp );
	NATIVE_FUNCTION( "DisplayStore", funcDisplayStore );
	NATIVE_FUNCTION( "DisplayUserProfileSystemDialog", funcDisplayUserProfileSystemDialog );
	NATIVE_FUNCTION( "SetRichPresence", funcSetRichPresence );
	NATIVE_FUNCTION( "SetActiveUserPromiscuous", funcSetActiveUserPromiscuous );
	NATIVE_FUNCTION( "ChangeActiveUser", funcChangeActiveUser );
	NATIVE_FUNCTION( "GetActiveUserDisplayName", funcGetActiveUserDisplayName );
	NATIVE_FUNCTION( "GetPlatform", funcGetPlatform );
	NATIVE_FUNCTION( "IsContentAvailable", funcIsContentAvailable );
	NATIVE_FUNCTION( "ProgressToContentAvailable", funcProgressToContentAvailable );
	NATIVE_FUNCTION( "ShouldForceInstallVideo", funcShouldForceInstallVideo );
	NATIVE_FUNCTION( "OnUserDialogCallback", funcOnUserDialogCallback );
	NATIVE_FUNCTION( "SaveUserSettings", funcSaveUserSettings );
	NATIVE_FUNCTION( "GetGwintManager", funcGetGwintManager );
	NATIVE_FUNCTION( "IsDebugQuestMenuEnabled", funcIsDebugQuestMenuEnabled );
	NATIVE_FUNCTION( "EnableNewGamePlus", funcEnableNewGamePlus );
	NATIVE_FUNCTION( "StartNewGamePlus", funcStartNewGamePlus );
	NATIVE_FUNCTION( "GetWorldDLCExtender", funcWorldDLCExtender );
	NATIVE_FUNCTION( "ShowSteamControllerBindingPanel", funcShowSteamControllerBindingPanel );
END_CLASS_RTTI();
