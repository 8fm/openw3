/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "viewportHook.h"
#include "gameSession.h"
#include "gameUniverseStorage.h"
#include "../core/countedBool.h"
#include "gameplayConfig.h"
#include "renderFrameInfo.h"
#include "loadingScreen.h"
#include "viewport.h"

class CCameraComponent;
class ISteerAgent;
class CCutsceneSetup;
class CTimeManager;
class CPlayer;
class CVideoPlayer;
class CFlashPlayer;
class CIdTagManager;
class CGameSaver;
class CGameFreeCamera;
class CTimerScriptKeyword;
class CGlobalEventsManager;
class IRenderFence;
class WorldLoadingContext;
class IGameInputModeListener;
class CSpeedConfigManager;
class CLoadingScreen;
class CLoadingOverlay;
class CContainerManager;
class CGuiManager;
class CMenu;
class IMovingAgentsStorage;
enum EWorldTransitionMode : CEnum::TValueType;
enum EInputKey : CEnum::TValueType;

enum class EUserEvent;
enum class EControllerEventType;

//#define DEBUG_BLACKSCREEN

///////////////////////////////////////////////////////////////////////////////

enum ECheats
{
    CHEAT_Console = 0,
    CHEAT_FreeCamera,
    CHEAT_DebugPages,
    CHEAT_InstantKill, // script-side
    CHEAT_Teleport,	   
    CHEAT_MovementOnPhysics,
    CHEAT_TimeScaling, // ?? side

    CHEAT_MAX
};

BEGIN_ENUM_RTTI( ECheats );
    ENUM_OPTION( CHEAT_Console )
    ENUM_OPTION( CHEAT_FreeCamera )
    ENUM_OPTION( CHEAT_DebugPages )
    ENUM_OPTION( CHEAT_InstantKill )
    ENUM_OPTION( CHEAT_Teleport )
    ENUM_OPTION( CHEAT_MovementOnPhysics )
    ENUM_OPTION( CHEAT_TimeScaling )
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum EEndGameReason
{
	ENDGAMEREASON_Default,
	ENDGAMEREASON_UserSignOut
};

enum EPersistanceMode
{
    PM_DontPersist,
    PM_SaveStateOnly,
    PM_Persist
};

BEGIN_ENUM_RTTI( EPersistanceMode );
    ENUM_OPTION( PM_DontPersist )
    ENUM_OPTION( PM_SaveStateOnly )
    ENUM_OPTION( PM_Persist )
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum EFrameCaptureSaveFormat
{
	FCSF_PNG,
	FCSF_BMP,
	FCSF_DDS
};

///////////////////////////////////////////////////////////////////////////////

struct SPlayerChangeInfo
{
	Bool				isRequested;
	CEntityTemplate*	playerTemplate;
	Bool				successfullyDetached;

	SPlayerChangeInfo()
		: isRequested( false )
		, playerTemplate( NULL )
		, successfullyDetached( true ) {};
};

///////////////////////////////////////////////////////////////////////////////

struct STimeScaleSource
{
	DECLARE_RTTI_STRUCT( STimeScaleSource );

	Float	m_timeScale;
	CName	m_name;
	Bool    m_affectCamera;
	Bool    m_dontSave;
	Uint32	m_priorityIndex;
};

BEGIN_CLASS_RTTI( STimeScaleSource );
	PROPERTY_RO( m_timeScale, TXT("Time scale") );
	PROPERTY_RO( m_name, TXT("Source name") );
	PROPERTY_RO( m_affectCamera, TXT("Affects camera") );
	PROPERTY_RO( m_dontSave, TXT("Dont save") );
	PROPERTY_RO( m_priorityIndex, TXT("Priority: the higher vaule the higher priority") );
END_CLASS_RTTI();

struct STimeScaleSourceSet
{
	DECLARE_RTTI_STRUCT( STimeScaleSourceSet );

	Int32							m_priority;
    TDynArray< STimeScaleSource >	m_entries;

	struct CompareFunc
	{
		static RED_INLINE Bool Less( const STimeScaleSourceSet& key1, const STimeScaleSourceSet& key2 )
		{
			return key1.m_priority > key2.m_priority;
		}	
	};
};

BEGIN_CLASS_RTTI( STimeScaleSourceSet );
	PROPERTY_RO( m_priority, TXT("Priority") );
	PROPERTY_RO( m_entries, TXT("Time scale entries") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

namespace Helper
{
	struct SScopedViewportHook;
}

/// The So Awesome Game
class CGame : public CObject, public IViewportHook, public IGameSaveSection
{
    DECLARE_ENGINE_ABSTRACT_CLASS_WITH_ALLOCATOR( CGame, CObject, MC_Engine )

    friend class CWorld;
    friend class CBaseEngine;
    friend class CEditorEngine;
    friend class CGameSessionManager;
	friend struct SScopedLoadingScreen;
	friend struct Helper::SScopedViewportHook;
	friend class CGameScopedPrefetch;

    static const Uint8 CHEAT_LEVELS[];

	struct SInitialFact
	{
		String m_name;
		Int32 m_value;

		SInitialFact() : m_value( 0 )
		{
		}
		SInitialFact( const String& name, Int32 value ) : m_name ( name ), m_value( value )
		{
		}

		RED_INLINE Bool operator==( const SInitialFact& other ) const { return m_name.EqualsNC( other.m_name ) && m_value == other.m_value; }
	};


    // High level rumble controller
    // It tells if and how the controller motors speed should be altered
    class CRumbleLogic
    {
    protected:
        struct SRumble
        {
            Float		m_lowFreqMotorSpeed;
            Float		m_highFreqMotorSpeed;
            Float		m_timeLeft;

            SRumble( Float lowFreqMotorSpeed, Float highFreqMotorSpeed, Float duration )
                : m_lowFreqMotorSpeed( lowFreqMotorSpeed )
                , m_highFreqMotorSpeed( highFreqMotorSpeed )
                , m_timeLeft( duration )
            {}

            SRumble()
                : m_lowFreqMotorSpeed( 0.0f )
                , m_highFreqMotorSpeed( 0.0f )
                , m_timeLeft( 0.0f)
            {}

			Bool IsEqual( Float lowFreq, Float highFreq ) const;
        };

        TDynArray< SRumble >	m_currentRumbles;		// Active rumbles
        Float					m_lowFreqMotorSpeed;	// Unnormalized value of low freq motor speed
        Float					m_highFreqMotorSpeed;	// Unnormalized value of high freq motor speed
    public:
        CRumbleLogic() : m_lowFreqMotorSpeed( 0.0f ), m_highFreqMotorSpeed( 0.0f ) {}

        // Update state and return normalized motor speed values. Returns true if motor speed should change.
        Bool UpdateRumbles( Float timeDelta, Float& lowFreqSpeed, Float& highFreqSpeed );

        // Set current motor speed values to 0.0. Returns true if motor speed should change. Designed for anything that pauses gameplay.
        Bool Pause();

        // Clear all rumble data, return true if motor speed should change. Basically for start/end game.
        Bool Reset();

        // Start new rumble
        void AddRumble( Float lowFreqSpeed, Float highFreqSpeed, Float duration );
		void RemoveRumble( Float lowFreqSpeed, Float highFreqSpeed );
		Bool IsRumbleActive( Float lowFreqSpeed, Float highFreqSpeed ) const;

		void StopRumbles();
		void GetCurrentFreq( Float& lowFreq, Float& highFreq ) const;

		void OverrideDuration( Float lowFreq, Float highFreq, Float newDuration );
    };
    
private:
	TDynArray< SLoadingScreenParam >	m_loadingScreenStack;

protected:
	CUniverseStorage					m_universeStorage;							//!< Savegame storage of the state of all worlds
    THandle< CWorld >					m_activeWorld;								//!< Active world
    ViewportHandle						m_viewport;									//!< Active game viewport
    IViewportHook*						m_prevHook;									//!< Previous hook of game viewport
    Bool								m_isActive;									//!< Is game active?
    Bool								m_isLoadingWorld;							//!< We are loading game world right now
    Bool								m_requestEnd;								//!< Request game end
	Bool								m_requestEndDueToSignout;					//!< Player signed out
	Bool								m_isInStartGame;							//!< Currently executing StartGame()
    Bool								m_gameEnded;								//!< True if EndGame() has been executed
	Bool								m_frameAdvance;								//!< Used for advancing the game frame-by-frame when paused
	Bool								m_stopped;									//!< Cursor back to editor
	Bool								m_requestUnstop;							//!< We want to unpause the game
	Bool								m_isKeyHandled;								//!< Internal key event was handles
	Bool								m_isUsingPad;								//!< Is gamepad controller active
	Bool								m_gameInputEnabled;							//!< Is game input processing enabled?
	Bool								m_isContignousCapture;						//!< Movie like screenshot capture mode
	Bool								m_doNotPauseWhileStopped;					//!< Do not pause the game while stopped
	Bool								m_nonGameplaySceneActive;					//!< Indicates whether currently is playing non-gameplay scene (cutscene, etc)
	Bool								m_freeCameraActive;							//!< Is the free camera active
	Bool								m_didRenderLastFrame;						//!< were we able to render the previous frame?
	Uint32								m_kosherTaintFlags;							//!< Is the game kosher? Eg., expected scripts or non-modified?
	Uint8								m_cheatLevel;								//!< Cheat level
	Uint16								m_disablePausingOnApplicationIdle;			//!< Dynamic bitmask that blocks pausing the game while its being stopped
	Uint32								m_difficulty;								//!< game difficulty level
	EInputAction						m_shiftState;								//!< State of shift key
	EEndGameReason						m_requestEndReason;							//!< Reason Request game end true
	Float								m_blockingLoadingCooldown;					//!< Cooldown factor for loading screen showed when blocking loading is enabled
	Float								m_aiObjectLooseTime;
    EngineTime							m_engineTime;								//!< Real time counted when game is not paused
	EngineTime							m_cleanEngineTime;							//!< Real time counted when game is not paused (it's not saved, so it's reset every time the game session is restarted)
	EngineTime							m_gameTime;									//!< Game time without time spent in main menu and ingame menu
    TDynArray< String >					m_pauseCount;								//!< Pause counter
    CountedBool							m_activePause;								//!< Active pause
	CContainerManager*					m_containerManager;							//!< Manager of all containers in the game

#ifndef NO_EDITOR
	Bool								m_forcePrefetch;							//!< Force texture prefetching before rendering every frame
#endif

protected:
    TDynArray< CCutsceneInstance* >		m_csInstances;								//!< Active cutscenes in game ( in scripts )
    CInputManager*						m_inputManager;								//!< Input manager
    CVideoPlayer*						m_videoPlayer;								//!< Video player
	CFlashPlayer*						m_flashPlayer;								//!< Flash player
	CLoadingScreen*						m_loadingScreen;							//!< Loading screen
	CLoadingOverlay*					m_loadingOverlay;							//!< Loading overlay
    CEntity*							m_camera;									//!< Game dynamic camera
    CTimeManager*						m_timeManager;								//!< Time flow management
    CIdTagManager*						m_idTagManager;								//!< Dynamic ID tag manager
	CSpeedConfigManager*				m_speedConfigManager;						//!< Container for speed data for skeletons
    IRenderFence*						m_renderFence;								//!< Fence for rendering flushing
    
	TSortedArray< STimeScaleSourceSet, STimeScaleSourceSet::CompareFunc >	m_timeScaleSets;	//!< Engine time scale
 
    
    CVisualDebug*						m_visualDebug;								//!< Global visual debug
	TDynArray< IGameInputModeListener* > m_gameInputModeListeners;					//!< Listeners that handle the game needing input or not
    
    TDynArray< SInitialFact >			m_initFacts;								//!< Initial facts db on game start (temporary HACK)

    Color								m_blackscreenColor;							//!< Last blackscreen color set through the game
    String								m_blackscreenOnReason;						//!< Debug information for black screen
    String								m_blackscreenOffReason;						//!< Debug information for black screen
	String								m_blackscreenLockOnReason;
	String								m_blackscreenLockOffReason;
	Bool								m_blackscreenWasSetPrevFrame;
    SGameplayConfig						m_gameplayConfig;							//!< Gameplay configuration
    
    CGameFreeCamera*					m_freeCamera;								//!< Free camera (for debug purposes)
    CRumbleLogic						m_rumbleLogic;								//!< High level state of the controller motors
	CRenderFrameInfo					m_cachedFrameInfo;							//!< Cached frame info - can be from last frame or current!!!!

	struct 
	{
		String	m_alias;			//!< player template alias name to change to
		CName	m_appearance;		//!< appearance
	}									m_playerChangeInfo;							//!< Info for changing player
	CTimerScriptKeyword*				m_timerScriptKeyword;						//!< Utility to allow scripters easy access to time data
	THandle< CGameResource >			m_gameResource;	
	CGlobalEventsManager*				m_globalEventsManager;						//!< Global gameplay events manager
	CGameSaveLock						m_saveLockForBlackscreens;					//!< One lock for all the blackscreens

	struct SWorldLayersStreaming
	{
		String						m_world;
		TDynArray< String >			m_groupsToHide;
		TDynArray< String >			m_groupsToShow;
		CWorldLayerStreamingFence*	m_fence; // custom fence to update
		Bool						m_purgeStorages;

		SWorldLayersStreaming( CWorldLayerStreamingFence* fence );
		~SWorldLayersStreaming();
	};

	typedef TDynArray< SWorldLayersStreaming* >	TWorldChangesList;
	TWorldChangesList					m_worldChanges;	

	struct SCameraStreamingOverride
	{
		String		m_name;
		Float		m_softDistance;
		Float		m_hardDistance;
	};

	struct SStreamingLock
	{
		String				m_name;
		CName				m_areaTag;
		Uint32				m_toRemoveOnFrame; // remove it on next frame unless we want it back
		TDynArray< Box >	m_areas; // collected
	};

	// the last valid position that was used for prefetch check - can be player position, camera position or manual position ;]
	Vector								m_prefetchPosition;
	Float								m_prefetchDistanceSoft;
	Float								m_prefetchDistanceHard;

	// prefetch suppressors - prefetch will
	TDynArray< SStreamingLock >			m_streamingLocks;
	Bool								m_streamingLocksModified;
	Uint32								m_streamingLocksFrameCounter;

	// streaming reference point override - camera mode
	TDynArray< SCameraStreamingOverride >	m_streamingCameraOverrides;

	// request manual streaming prefetch
	Bool								m_requestStreamingPrefetch;

	// manual disabling of streaming prefetch
	Bool								m_enableStreamingPrefetch;

public:
	enum EDisableAutopauseFlags
	{
		DISABLE_AUTOPAUSE_PathLib				 = FLAG( 0 ),
		DISABLE_AUTOPAUSE_Cutscene				 = FLAG( 1 )
	};

	enum EKosherTaintFlags
	{
		eKosherTaintFlag_Scripts				= FLAG(0),
		eKosherTaintFlag_Session				= FLAG(31),

		eKosherClearMask						= eKosherTaintFlag_Scripts, 
	};

    CGame();

    virtual ~CGame();

	const SLoadingScreenParam& GetActiveLoadingScreenParam() const;
	SLoadingScreenParam& GetActiveLoadingScreenParam();

	void PushLoadingScreenParam( const SLoadingScreenParam& param );
	void ReplaceLastLoadingScreenParam( const SLoadingScreenParam& param );
	void ClearLoadingScreenParams( const SLoadingScreenParam& newDefaultParam );
	void ClearLoadingScreenParams();

	void ReplaceDefaultLoadingScreenParam( const SLoadingScreenParam& param );
	void ReplaceDefaultLoadingScreenVideo( const String& videoToPlay );
	void ReplaceDefaultLoadingScreenName( CName name );

    // Are we in game ?
    RED_INLINE Bool IsActive() const { return m_isActive; }

	// Shitty hack for the profile manager to avoid changing too much. Need to delay signin if not signed out yet.
	RED_INLINE Bool CERT_HACK_IsInStartGame() const { return m_isInStartGame; }

	// Is game paused
	RED_INLINE Bool IsPaused( const String& reason ) const { return m_pauseCount.Exist( reason ); }
    RED_INLINE Bool IsPaused() const { return !m_pauseCount.Empty(); }

    // Is game actively paused
    RED_INLINE Bool IsActivelyPaused() const { return m_activePause; }

	RED_INLINE Bool IsKosher() const { return m_kosherTaintFlags == 0; }
	RED_INLINE void SetKosherTaintFlags( Uint32 flagsToSet ) { m_kosherTaintFlags |= flagsToSet; }
	RED_INLINE void ClearKosherTaintFlags( Uint32 flagsToClear ) { m_kosherTaintFlags &= ~(flagsToClear & ~eKosherClearMask); }

    // Get active game world
    RED_INLINE const THandle< CWorld >& GetActiveWorld() { return m_activeWorld; }

    // Get game viewport
    RED_INLINE IViewport* GetViewport() const { return m_viewport.Get(); }

    // Get engine time (real time counted when game is not paused)
    RED_INLINE const EngineTime &GetEngineTime() const { return m_engineTime; }

	// Get ingame time (exluding main menu and ingame menu and including time spent in inventories etc.)
	RED_INLINE const EngineTime &GetGameTime() const { return m_gameTime; }
	
	// Get clean engine time (real time counted when game is not paused, but not saved/loaded, so it's reset every time the game session is restarted)
	RED_INLINE const EngineTime& GetCleanEngineTime() const { return m_cleanEngineTime; }

    // Global time manager
    RED_INLINE CTimeManager* GetTimeManager() const { return m_timeManager; }

    // Enable/disable game input
    RED_INLINE void EnableGameInput( Bool enable ) { m_gameInputEnabled = enable; }

    RED_INLINE Bool IsGameInputEnabled() { return m_gameInputEnabled; }

    // Is the continuous mode enabled
    RED_INLINE Bool IsContignousModeEnabled() const { return m_isContignousCapture; }

    // Get global visual debug
    RED_INLINE CVisualDebug* GetVisualDebug() const { return m_visualDebug; }

	RED_INLINE EEndGameReason GetEndGameReason() const { return m_requestEndReason; }

	const String& GetBlackscreenOnReason() const { return m_blackscreenOnReason; }
	const String& GetBlackscreenOffReason() const { return m_blackscreenOffReason; }

    // Do not pause the game while the app is stopped (needed for debug)
    RED_INLINE Bool DoNotPauseWhileStopped() const { return m_doNotPauseWhileStopped || m_disablePausingOnApplicationIdle; }

	// Blocks pausing while the app is stopped throught bit flag
	RED_INLINE void DisablePausingOnApplicationIdle( EDisableAutopauseFlags flags ) { m_disablePausingOnApplicationIdle |= flags; }

	// Remove bit flag that blocks pausing while the app is stopped
	RED_INLINE void EnablePausingOnApplicationIdle( EDisableAutopauseFlags flags ) { m_disablePausingOnApplicationIdle &= ~flags; }

    // Get the input manager (required by editor)
    RED_INLINE CInputManager* GetInputManager() { return m_inputManager; }

    // Get the video player
    RED_INLINE CVideoPlayer* GetVideoPlayer() const { return m_videoPlayer; }

	// Get the flash player
	RED_INLINE CFlashPlayer* GetFlashPlayer() const { return m_flashPlayer; }

    // Get the IdTag manager
    RED_INLINE CIdTagManager* GetIdTagManager() const { return m_idTagManager; }

	//! Get speed config manager
	RED_INLINE CSpeedConfigManager* GetSpeedConfigManager()const { return m_speedConfigManager; }

	// Last frame info
	RED_INLINE const CRenderFrameInfo& GetCachedFrameInfo() const { return m_cachedFrameInfo; }

	RED_INLINE CGlobalEventsManager*  GetGlobalEventsManager() { return m_globalEventsManager; }

    // Is cheat feature enabled
#ifdef W2_PLATFORM_WIN32
    RED_INLINE Bool IsCheatEnabled( ECheats cheatFeature ) const { return CHEAT_LEVELS[ cheatFeature ] <= m_cheatLevel; }
#else
    RED_INLINE Bool IsCheatEnabled( ECheats cheatFeature ) const { return true; }
#endif

    //! Get free camera
    RED_INLINE const CGameFreeCamera& GetFreeCamera() const { return *m_freeCamera; }

    //! Is the game free camera active ?
    RED_INLINE Bool IsFreeCameraEnabled() const { return m_freeCameraActive; }

	//! Get time scale sets
	RED_INLINE TSortedArray< STimeScaleSourceSet, STimeScaleSourceSet::CompareFunc >& GetTimeScaleSets() { return m_timeScaleSets; }

	RED_INLINE Float GetAIObjectLoseTime(){ return m_aiObjectLooseTime; }

#ifndef NO_EDITOR
	RED_INLINE void EnableForcedPrefetch( Bool enable ) { m_forcePrefetch = enable; }

	RED_INLINE Bool IsForcedPrefetchEnabled() const { return m_forcePrefetch; }
#endif

public:
    // Initialize game
    virtual void Init();

    // Tick game, updates physics, world, rendering etc
    virtual void Tick( Float timeDelta );

	// check if there was an end-game request and if so, end the game
	Bool TryEndGame();

    // Called before world tick
    virtual void OnTick( Float timeDelta ) {}

	// Tick during post update transform
	virtual void OnTickDuringPostUpdateTransforms( Float timeDelta, Bool updateGui ){}

    // Shut down game, unload world
    virtual void ShutDown();

    // Set game viewport - a viewport that is used to render game scene and grab input
    virtual void SetViewport( ViewportHandle viewport );

public:
	// Activate game camera
	virtual Bool ActivateGameCamera( Float blendTime, Bool useFocusTarget = false, Bool force = true, Bool reset = true ) const { return false; }
	//! Resets gameplay camera
	virtual void ResetGameplayCamera() {}
	//! Gets gameplay camera data
	virtual Bool GetGameplayCameraData( ICamera::Data& data ) { return false; }
	//! Starts gameplay camera blend from given camera data
	virtual void StartGameplayCameraBlendFrom( const ICamera::Data& data, Float blendTime ) {}
	//! Gets gameplay camera entity
	virtual CEntity* GetGameplayCameraEntity() const { return nullptr; }

	//  Get's the gui manager for the game
	virtual CGuiManager* GetGuiManager() const { return nullptr; }

	// Get the root active menu of the game, returns nullptr if no menu is active
	virtual const CMenu* GetRootMenu() const { return nullptr; }

	//Returns true if the game is in the main menu
	virtual Bool IsInMainMenu() const { return false; }

	//Returns true if the game is in the ingame menu
	virtual Bool IsInIngameMenu() const { return false; }

public:
    //! Global update transform is about to begin
    virtual void OnUpdateTransformBegin() {};

    //! Global update transform has finished
    virtual void OnUpdateTransformFinished( Bool isMainUpdateTransformPass ) {};

	//! Waits until gameplay storage update task is finished
	virtual void FinalizeFlushGameplayStorageUpdates() {}

public:
	// Handle engine input
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	// Handle viewport deactivation
	virtual void OnViewportActivated( IViewport* view ) override;

	// Handle viewport deactivation
	virtual void OnViewportDeactivated( IViewport* view ) override;

	// Is pad connected
	Bool IsPadConnected() const;

    //! Is using pad
    RED_INLINE Bool IsUsingPad() const { return m_isUsingPad; }

    //! Toggle pad on/off - resets some mouse/pad related input events
    void TogglePad( Bool isUsingPad );

    //! Called when the game time is arbitrarily changed.
    virtual void OnGameTimeChanged() {}

    //! Vibrate Eggs Box Pad, false means no pleasure
    void VibrateController( Float lowFreqSpeed, Float highFreqSpeed, Float duration );

    //! Update force feedback state
    void UpdateForceFeedback( Float timeDelta );
public:

	virtual void OnUserEvent( const EUserEvent& event );
	void OnControllerEvent( const EControllerEventType& event );

public:
    //! Generate debug viewport fragments
    virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

    //! Generate debug pages fragments
#ifndef NO_DEBUG_PAGES
    virtual void GenerateDebugPageFragmets( IViewport* view, CRenderFrame* frame ) {} // empty impl.
#endif

    //! Override camera
    virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

public:
    //! Get player
    virtual CEntity* GetPlayerEntity() const { return NULL; }

    //! Get world class
    virtual CClass* GetGameWorldClass() const;

	RED_INLINE Bool ShouldChangePlayer() const { return !m_playerChangeInfo.m_alias.Empty(); }

	virtual CEntity* ChangePlayer() { return NULL; }

	//! Collect stripe component (road, path etc.) (once it's attached to the world)
	virtual void RegisterStripeComponent( CStripeComponent* ) {}

	//! Get rid  stripe component (once it's detached from the world)
	virtual void UnregisterStripeComponent( CStripeComponent* ) {}

public:
    //! Cutscene event
    virtual void OnCutsceneEvent( const String& csName, const CName& csEvent ) {}

    //! Register cutscene
    void RegisterCutscene( CCutsceneInstance* instance );

    //! Unregister cutscene
    void UnregisterCutscene( CCutsceneInstance* instance );

    //! Unregister all cutscenes
    void UnregisterAllCutscenes();

    //! Get cutscene
    CCutsceneInstance* GetCutscene( const String& name ) const;

    //! Pause
    void PauseCutscenes();

    //! Unpause
    void UnpauseCutscenes();

    //! Is playing cutscene with active camera
    Bool IsPlayingCameraCutscene() const;

    //! Is playing cachet dialog
    virtual Bool IsPlayingCachetDialog() const { return false; }

public:

	CContainerManager& GetContainerManager() { return *m_containerManager; }

public:
	//! Enable camera streaming override with specified soft (try prefetch) and hard (must prefetch) distances
	//! This is disable player based streaming
	//! New camera override will not be added if there's one with the same name
	void EnableCameraBasedStreaming( const String& idName, const Float softDistance, const Float hardDistance );

	//! Enable camera streaming override with specified soft (try prefetch) and hard (must prefetch) distances
	//! This will get back to player based streaming if there are no other overrides
	//! The idName that is passed must be the same as in idName passed to EnableCameraBasedStreaming
	void DisableCameraBasedStreaming( const String& idName );

	//! Enable streaming lockdown - there will be no streaming prefetches for the duration of the scene
	//! New streaming lock will not be added if there's one with the same areaTag
	void EnableStreamingLockdown( const String& idName, const CName areaTag );

	//! Disable streaming lockdown
	//! The areaTag that is passed must be the same as in areaTag passed to EnableStreamingLockdown
	void DisableStreamingLockdown( const CName areaTag );

	//! Is streaming lock enabled?
	const Bool IsStreamingLockdownEnabled( const CName areaTag ) const;

public:
    RED_INLINE Bool IsCurrentlyPlayingNonGameplayScene() const { return m_nonGameplaySceneActive; }
    virtual Bool IsPlayingMiniGame() { return false; }
    void NonGameplaySceneStarted();
    RED_INLINE void NonGameplaySceneEnded() { m_nonGameplaySceneActive = false; }

	void GameplaySceneStarted();

public:
    // Load world, low-level, blocking
    Bool LoadWorld( const String &localDepotPath, WorldLoadingContext& context );

    // Load world and preload given partition, blocking
    Bool LoadGameWorld( const CGameInfo& info, const String &localDepotPath );

    // Unload world
    virtual void UnloadWorld();

#ifndef NO_EDITOR_WORLD_SUPPORT
    // (EDITOR) Create new world to play on, editor only
    Bool CreateWorld( const String &localDepotPath );
#endif

#ifndef NO_EDITOR_WORLD_SUPPORT
    // (EDITOR) Save active world and all layers
    Bool SaveWorld();
#endif

	virtual void LoadGame( const SSavegameInfo& saveFileNoPath ) {}

public:
    // Get time scale
    Float GetTimeScale( Bool forCamera = false ) const;

    // Change game time scale
    void SetTimeScale( Float timeScale, const CName& sourceName, Int32 priority, Bool affectCamera = false, Bool dontSave = false );

	// Remove time scale
	void RemoveTimeScale( const CName& sourceName );

	// Remove all time scales
	void RemoveAllTimeScales();

	// Set or remove time scale depending on argument
	void SetOrRemoveTimeScale( Float timeScale, const CName& sourceName, Int32 priority, Bool affectCamera = false );

	// Print current time scales
	void LogTimeScales();

public:
    //! Gameplay config for debug and tests
    RED_INLINE SGameplayConfig& GetGameplayConfig() { return m_gameplayConfig; }

protected:
	void ResetPauseCounter();

protected:
	struct SPrefetchParamsW3Hack
	{
		SPrefetchParamsW3Hack()
			: m_fastForwardCommunities( false )
			, m_waitForPlayer( false )
			, m_forceUITextureStreaming( false )
			, m_pathlibStreaming( false )
			, m_isGameStart( false )
			, m_didMoveFar( true )
		{}

		Bool m_fastForwardCommunities:1;
		Bool m_waitForPlayer:1;
		Bool m_forceUITextureStreaming:1;
		Bool m_pathlibStreaming:1;
		Bool m_isGameStart:1;
		Bool m_didMoveFar:1;
	};

	virtual void SetStreamingReferencePosition( const Vector& referencePosition );

	void PerformPrefetch( const Bool showLoadingScreen, const SPrefetchParamsW3Hack& params );
	void UpdatePrefetch( const Vector& referencePosition, Bool& allowCurrentFrameToRender );
	void UpdateStreamingModeAndDistances();
	void RetireOldStreamingLocks();
	const Bool UseCameraAsStreamingReference() const;
	const Bool IsStreamingLocked() const;
	void UpdateStreamingLocks();

	virtual void PerformCommunitiesFastForward( const Vector& referencePosition, Float timeLimit, Bool resimulateCommunities );

public:
	Bool AddGameInputModeListener( IGameInputModeListener* listener ) { RED_ASSERT( listener); if ( listener ) { return m_gameInputModeListeners.PushBackUnique( listener ); } return false; }
	Bool RemoveGameInputModeListener( IGameInputModeListener* listener ) { RED_ASSERT( listener); if ( listener ) { return m_gameInputModeListeners.Remove( listener ); } return false; }

public:
    // Pauses the game, rerouting the input back to Windows
    virtual void Stop();

    // Resumes the paused game, locking the Input
    virtual void Unstop();

    // Pause game
    virtual void Pause( const String& reason );

    // Unpause game
    virtual void Unpause( const String& reason );

	// Force unpause game
	virtual void UnpauseAll();

	virtual void EnteredConstrainedMode() {}
	virtual void ExitedConstrainedMode() {}

    // Set active pause state
    void SetActivePause( Bool flag );

    // Request game end
    void RequestGameEnd();

	Bool EndGameRequested() const { return m_requestEnd; }

    // Change layers visibility, optionally you can specify a loading fence to be applied to the loading
    Bool ScheduleLayersVisibilityChange( const String& world, const TDynArray< String >& groupsToHide, const TDynArray< String >& groupsToShow, Bool purgeStorages, CWorldLayerStreamingFence* optionalFence = nullptr );
public:
	//Check if any Menu is Opened
	virtual Bool IsAnyMenu() const { return false; }

public:
    // Gets the world position of free camera
    void GetFreeCameraWorldPosition( Vector* outPosition, EulerAngles* outRotation, Vector* outDir ) const;
	
	// Gets free camera's fov distance multiplier
	Float GetFreeCameraFovDistanceMultiplier() const;

    // Sets the world position of free camera
    void SetFreeCameraWorldPosition( const Vector &camWorldPos );

	// Sets the world rotation of free camera
	void SetFreeCameraWorldRotation( const EulerAngles& camWorldRot );

    // Enable free camera
    void EnableFreeCamera( Bool enable );

public:
    //! HACK: Clears initial facts
    void ClearInitialFacts() { m_initFacts.Clear(); }

    //! HACK: Adds a new initial fact
    void AddInitialFact( const String &factName, Int32 value = 1 ) { new ( m_initFacts ) SInitialFact( factName, value ); }

	Bool DoesInitialFactExist( const String& factName ) const;

    //! HACK: Returns initial facts
    RED_INLINE const TDynArray< SInitialFact > &GetInitialFacts() const { return m_initFacts; }


public:
    // Start fade in/out
    void StartFade( Bool fadeIn, const String& reason, Float fadeDuration = 1.f, const Color & color = Color::BLACK, const Bool informSoundSystem = true );
	// Set fade in/out state
	void SetFade( Bool fadeIn, const String& reason, Float fadeProgress = 0.f, const Color & color = Color::BLACK );

    // Set blackscreen
    void SetBlackscreen( Bool enable, const String& reason, const Color & color = Color::BLACK );

    // Is screen fade in progress ?
    Bool IsFadeInProgress() const;

    // Is the blackscreen visible ?
    Bool IsBlackscreen() const;
	Bool HasBlackscreenRequested() const;
	Bool HasBlackscreenLockRequested() const;

	void SetFadeLock( const String& reason );
	void ResetFadeLock( const String& reason );
	// Bool IsFadeLocked( String& reason ) const; Should not be needed!

	RED_INLINE Bool IsLoadingScreenShown() const { return ( nullptr == m_loadingScreen ) ? false : m_loadingScreen->IsShown(); }

	RED_INLINE Bool IsLoadingScreenVideoPlaying() const { return m_loadingScreen ? m_loadingScreen->IsPlayingVideo() : false; }

public:
    void SetGameDifficulty( Uint32 level );
    RED_INLINE Uint32 GetGameDifficulty() const { return m_difficulty; }

protected:
	virtual void OnGameDifficultyChanged( Uint32 previousDifficulty, Uint32 currentDifficulty ) {}

    //! Start true game on loaded world
    //! THIS METHOD SHOULD NOT BE VIRTUAL
    Bool StartGame( const CGameInfo& info );

	// Hacks for the loading screen and layers
	virtual Bool StartGame_AreQuestsStable() const { return true; }
	virtual void StartGame_ResetQuestStability() {}
	virtual void StartGame_WaitForPlayer() {}
	virtual void StartGame_InitHUD() {}
	virtual void StartGame_UpdateActors() {}

#ifndef RED_FINAL_BUILD
	virtual void StartGame_DebugDumpUnstableQuestThreads() const {}
#endif

	virtual Bool StartGame_UpdateSaver() { return false; }

protected:
	virtual void HACK_TickStorySceneVideoElementInstance( Float timeDelta ) {}

public:
    //! End game, do not call directly
    //! THIS METHOD SHOULD NOT BE VIRTUAL
    void EndGame();

    //! Start preview on given world
    Bool StartPreview( const String& worldFileName );

	//! Gets universe storage
	CUniverseStorage* GetUniverseStorage() { return &m_universeStorage; }

protected:
    //! Initialize gameplay only sub system, called BEFORE game world has been loaded
    virtual void OnGameplaySystemsGameStart( const CGameInfo& info );

    //! Initialize gameplay only sub system, called AFTER game world has been loaded
    virtual void OnGameplaySystemsWorldStart( const CGameInfo& info );

	//! Initialize anything left, called AFTER game world has been loaded and loading screen video ended
	virtual void OnAfterLoadingScreenGameStart( const CGameInfo& info );

    //! Close gameplay only sub system, called BEFORE game world has been unloaded
    virtual void OnGameplaySystemsWorldEnd( const CGameInfo& info );

    //! Close gameplay only sub system, called AFTER game world has been unloaded
    virtual void OnGameplaySystemsGameEnd( const CGameInfo& info );

    //!
    virtual void OnGameStart( const CGameInfo& gameInfo ) {}

	virtual void OnGamePrefetchStart( Bool showLoadingScreen ) {}
	virtual void OnGamePrefetchEnd( Bool showLoadingScreen ) {}

	virtual void OnLoadingScreenShown() {}

private:
	// Keep show/hide private.
    Bool ShowLoadingScreen();
    Bool HideLoadingScreen();

	Bool IsLoadingVideoSkipKey( EInputKey key ) const;

private:
	void UpdateBlackscreenColor( const Color& color );

protected:
    virtual CEntity* CreateCamera() const;
    virtual CEntity* CreatePlayer( const CGameInfo& info );
    void DestroyCamera( CEntity* camera );	
    Bool ProcessFreeCameraInput( enum EInputKey key, enum EInputAction action, Float data );
    void ToggleHud();

public:
    void ToggleContignous( EFrameCaptureSaveFormat saveFormat = FCSF_BMP, Bool ubersample = false );

protected:
    void FinishLayerTransitions();
    void ProcessScheduledLayerVisibilityChanges();
    virtual Bool OnSaveGame( IGameSaver* saver );
	virtual void SaveRequiredDLCs( IGameSaver* saver ) {}
	virtual Bool LoadRequiredDLCs( IGameLoader* loader, TDynArray< CName >& missingContent ) { RED_UNUSED( loader ); RED_UNUSED( missingContent ); return true; }
	virtual void SaveProperties( IGameSaver* saver );
	virtual void LoadProperties( IGameLoader* loader );
	Bool DoesFreeCameraProcessInputs() const;

public:
	virtual void HandleInGameSavesLoading() {}

public:
	virtual Bool TickMainMenu( float timeDelta ) { return false; }
    virtual void ShowMainMenuFirstTime( Bool showStartVideos );

	virtual void OnSavedGamesInfoChanged() {} // implemented in CWitcherGame

	//! Called when external overlay UI is toggled
	virtual void OnExternalUI( bool isShown ) {}

	virtual bool AnalogScaleformEnabled() { return false; }

public:
    // Get game saver
    virtual CGameSaver* GetGameSaver() = 0;

    // Gets game difficulty
    RED_INLINE Uint32 GetDifficulty() { return m_difficulty; }

public:
    // Received when device is lost
    virtual void NotifyDeviceLost();

    // Received when device is restored
    virtual void NotifyDeviceRestore();

public:
	// Notifies language dependent systems
	virtual void OnLanguageChange();

private:
	void NotifyGameInputModeEnabled();
	void NotifyGameInputModeDisabled();

	void ReloadConfig();
	void UpdateGameTime( Float timeDelta );
protected:
	virtual void OnReloadedConfig() {}

public:
	
	virtual CGameResource* GetGameResource() const { return m_gameResource.Get(); }

	// Loads proper (game specific) global game definition resource.
	// Intended for overriding in derived game classes.
	virtual Bool SetupGameResourceFromFile( const String& filePath ) { return false; }

	const String GetGameResourcePath() const;

	static Uint32 DebugCallback( Char* buffer, Uint32 bufferSize );

#ifndef NO_EDITOR_WORLD_SUPPORT
protected:
    void PrepareForPIE( const CGameInfo& info, TDynArray< CLayerInfo* >& layersForFastLoad );
#endif

#ifndef NO_DEBUG_PAGES
public:
	void CollectCutsceneInstancesName( TDynArray< String >& names ) const;
#endif

public:
	virtual void FinalizeMovement( Float timeDelta ) {}
	
public:
	virtual Bool OnSuspend() { return true; }
	virtual Bool OnResume() { return true; }

	void MoveMouseTo( Float targetX, Float targetY );

private:
    void funcIsActive( CScriptStackFrame& stack, void* result );
    void funcIsPaused( CScriptStackFrame& stack, void* result );
	void funcIsPausedForReason( CScriptStackFrame& stack, void* result );
	void funcIsStopped( CScriptStackFrame& stack, void* result );
	void funcIsLoadingScreenVideoPlaying( CScriptStackFrame& stack, void* result );
    void funcIsActivelyPaused( CScriptStackFrame& stack, void* result );
    void funcGetEngineTime( CScriptStackFrame& stack, void* result );
	void funcGetEngineTimeAsSeconds( CScriptStackFrame& stack, void* result );
    void funcGetTimeScale( CScriptStackFrame& stack, void* result );
    void funcSetTimeScale( CScriptStackFrame& stack, void* result );
	void funcRemoveTimeScale( CScriptStackFrame& stack, void* result );
	void funcRemoveAllTimeScales( CScriptStackFrame& stack, void* result );
	void funcSetOrRemoveTimeScale( CScriptStackFrame& stack, void* result );
	void funcLogTimeScales( CScriptStackFrame& stack, void* result );
    void funcGetGameTime( CScriptStackFrame& stack, void* result );
    void funcSetGameTime( CScriptStackFrame& stack, void* result );
    void funcSetHoursPerMinute( CScriptStackFrame& stack, void* result );
	void funcGetHoursPerMinute( CScriptStackFrame& stack, void* result );
    void funcPause( CScriptStackFrame& stack, void* result );
    void funcUnpause( CScriptStackFrame& stack, void* result );
    void funcExitGame( CScriptStackFrame& stack, void* result );
    void funcSetActivePause( CScriptStackFrame& stack, void* result );
    void funcCreateEntity( CScriptStackFrame& stack, void* result );
    void funcGetNodeByTag( CScriptStackFrame& stack, void* result );
    void funcGetNodesByTag( CScriptStackFrame& stack, void* result );
    void funcGetNodesByTags( CScriptStackFrame& stack, void* result );
    void funcGetWorld( CScriptStackFrame& stack, void* result );
    void funcIsFreeCameraEnabled( CScriptStackFrame& stack, void* result );
    void funcEnableFreeCamera( CScriptStackFrame& stack, void* result );
    void funcGetFreeCameraPosition( CScriptStackFrame& stack, void* result );
    void funcIsShowFlagEnabled( CScriptStackFrame& stack, void* result );
    void funcSetShowFlag( CScriptStackFrame& stack, void* result );	
    void funcPlayCutsceneAsync( CScriptStackFrame& stack, void* result );
    void funcGetEntityByTag( CScriptStackFrame& stack, void* result );
    void funcGetEntitiesByTag( CScriptStackFrame& stack, void* result );
    void funcIsUsingPad( CScriptStackFrame& stack, void* result );
    void funcIsStreaming( CScriptStackFrame& stack, void* result );
    // Achievement functions
    void funcUnlockAchievement( CScriptStackFrame& stack, void* result );
    void funcLockAchievement( CScriptStackFrame& stack, void* result );
    void funcGetUnlockedAchievements( CScriptStackFrame& stack, void* result );
    void funcGetAllAchievements( CScriptStackFrame& stack, void* result );
	void funcIsAchievementUnlocked( CScriptStackFrame& stack, void* result );
	void funcToggleUserProfileManagerInputProcessing( CScriptStackFrame& stack, void* result );
    //void funcAddStatF( CScriptStackFrame& stack, void* result );
    //void funcAddStatI( CScriptStackFrame& stack, void* result );
    void funcGetDifficulty( CScriptStackFrame& stack, void* result );
    void funcSetDifficulty( CScriptStackFrame& stack, void* result );
    void funcIsCheatEnabled( CScriptStackFrame& stack, void* result );
    void funcReloadGameplayConfig( CScriptStackFrame& stack, void* result );
    void funcGetGameplayChoice( CScriptStackFrame& stack, void* result );
    void funcIsCurrentlyPlayingNonGameplayScene( CScriptStackFrame& stack, void* result );
    void funcIsFinalBuild( CScriptStackFrame& stack, void* result );
    void funcPauseCutscenes( CScriptStackFrame& stack, void* result );
    void funcUnpauseCutscenes( CScriptStackFrame& stack, void* result );
    void funcTogglePad( CScriptStackFrame& stack, void* result );
    void funcIsPadConnected( CScriptStackFrame& stack, void* result );
    void funcIsBlackscreen( CScriptStackFrame& stack, void* result );
    void funcGetGameInputMappings( CScriptStackFrame& stack, void* result );
    void funcIsFading( CScriptStackFrame& stack, void* result );
	void funcGetSignoutMessagePending( CScriptStackFrame& stack, void* result );
    void funcIsVibrationEnabled( CScriptStackFrame& stack, void* result );
	void funcSetVibrationEnabled( CScriptStackFrame& stack, void* result );
	void funcGetGameplayConfigFloatValue( CScriptStackFrame& stack, void* result );
	void funcGetGameplayConfigBoolValue( CScriptStackFrame& stack, void* result );
	void funcGetGameplayConfigIntValue( CScriptStackFrame& stack, void* result );
	void funcGetGameplayConfigEnumValue( CScriptStackFrame& stack, void* result );
	void funcSetAIObjectsLooseTime( CScriptStackFrame& stack, void* result );

	void funcAddInitialFact( CScriptStackFrame& stack, void* result );
	void funcRemoveInitialFact( CScriptStackFrame& stack, void* result );
	void funcClearInitialFacts( CScriptStackFrame& stack, void* result );
	
	void funcGetCurrentViewportResolution( CScriptStackFrame& stack, void* result );

    // Latent functions
    void funcPlayCutscene( CScriptStackFrame& stack, void* result );
    void funcFadeOut( CScriptStackFrame& stack, void* result );
    void funcFadeIn( CScriptStackFrame& stack, void* result );
    void funcFadeOutAsync( CScriptStackFrame& stack, void* result );
    void funcFadeInAsync( CScriptStackFrame& stack, void* result );
	void funcSetFadeLock( CScriptStackFrame& stack, void* result );
	void funcResetFadeLock( CScriptStackFrame& stack, void* result );
	void funcVibrateController( CScriptStackFrame& stack, void* result );
	void funcStopVibrateController( CScriptStackFrame& stack, void* result );
	void funcGetCurrentVibrationFreq( CScriptStackFrame& stack, void* result );
	void funcRemoveSpecificRumble( CScriptStackFrame& stack, void* result );
	void funcIsSpecificRumbleActive( CScriptStackFrame& stack, void* result );
	void funcOverrideRumbleDuration( CScriptStackFrame& stack, void* result );
	void funcCreateEntityByPath( CScriptStackFrame& stack, void* result );

	// Loading screen functions
	void funcSetSingleShotLoadingScreen( CScriptStackFrame& stack, void* result );

	void funcDebugActivateContent( CScriptStackFrame& stack, void* result );
	void funcHasBlackscreenRequested( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( CGame );	
    PARENT_CLASS( CObject );
    PROPERTY_NOSERIALIZE( m_activeWorld );
	PROPERTY( m_visualDebug );
	PROPERTY( m_inputManager );
	PROPERTY_NOSERIALIZE( m_timerScriptKeyword );
	PROPERTY( m_gameResource );
    NATIVE_FUNCTION( "IsActive", funcIsActive);
    NATIVE_FUNCTION( "IsPaused", funcIsPaused);
	NATIVE_FUNCTION( "IsPausedForReason", funcIsPausedForReason);
	NATIVE_FUNCTION( "IsStopped", funcIsStopped);
    NATIVE_FUNCTION( "IsActivelyPaused", funcIsActivelyPaused);
	NATIVE_FUNCTION( "IsLoadingScreenVideoPlaying", funcIsLoadingScreenVideoPlaying);
    NATIVE_FUNCTION( "GetEngineTime", funcGetEngineTime);
	NATIVE_FUNCTION( "GetEngineTimeAsSeconds", funcGetEngineTimeAsSeconds);
    NATIVE_FUNCTION( "GetTimeScale", funcGetTimeScale );
    NATIVE_FUNCTION( "SetTimeScale", funcSetTimeScale );	
    NATIVE_FUNCTION( "RemoveTimeScale", funcRemoveTimeScale );	
    NATIVE_FUNCTION( "RemoveAllTimeScales", funcRemoveAllTimeScales );	
    NATIVE_FUNCTION( "SetOrRemoveTimeScale", funcSetOrRemoveTimeScale );	
    NATIVE_FUNCTION( "LogTimeScales", funcLogTimeScales );	
    NATIVE_FUNCTION( "GetGameTime", funcGetGameTime );
    NATIVE_FUNCTION( "SetGameTime", funcSetGameTime );
    NATIVE_FUNCTION( "SetHoursPerMinute", funcSetHoursPerMinute );
	NATIVE_FUNCTION( "GetHoursPerMinute", funcGetHoursPerMinute );
    NATIVE_FUNCTION( "Pause", funcPause );
    NATIVE_FUNCTION( "Unpause", funcUnpause );
    NATIVE_FUNCTION( "ExitGame", funcExitGame );
    NATIVE_FUNCTION( "SetActivePause", funcSetActivePause );
    NATIVE_FUNCTION( "CreateEntity", funcCreateEntity );
	NATIVE_FUNCTION( "CreateEntityByPath", funcCreateEntityByPath);
    NATIVE_FUNCTION( "GetNodeByTag", funcGetNodeByTag );
    NATIVE_FUNCTION( "GetEntityByTag", funcGetEntityByTag );
    NATIVE_FUNCTION( "GetNodesByTag", funcGetNodesByTag );
	NATIVE_FUNCTION( "GetNodesByTags", funcGetNodesByTags );
    NATIVE_FUNCTION( "GetWorld", funcGetWorld );
    NATIVE_FUNCTION( "IsFreeCameraEnabled", funcIsFreeCameraEnabled );
    NATIVE_FUNCTION( "EnableFreeCamera", funcEnableFreeCamera );
    NATIVE_FUNCTION( "GetFreeCameraPosition", funcGetFreeCameraPosition );
    NATIVE_FUNCTION( "IsShowFlagEnabled", funcIsShowFlagEnabled );
    NATIVE_FUNCTION( "SetShowFlag" , funcSetShowFlag );
	NATIVE_FUNCTION( "AddInitialFact" , funcAddInitialFact );
	NATIVE_FUNCTION( "RemoveInitialFact" , funcRemoveInitialFact );
	NATIVE_FUNCTION( "ClearInitialFacts", funcClearInitialFacts );
    NATIVE_FUNCTION( "PlayCutscene", funcPlayCutscene);
    NATIVE_FUNCTION( "PlayCutsceneAsync", funcPlayCutsceneAsync);
    NATIVE_FUNCTION( "IsStreaming", funcIsStreaming );
    NATIVE_FUNCTION( "GetEntityByTag", funcGetEntityByTag );
    NATIVE_FUNCTION( "GetEntitiesByTag", funcGetEntitiesByTag );
    NATIVE_FUNCTION( "IsUsingPad", funcIsUsingPad );
    NATIVE_FUNCTION( "FadeIn", funcFadeIn );
    NATIVE_FUNCTION( "FadeOut", funcFadeOut );
    NATIVE_FUNCTION( "FadeInAsync", funcFadeInAsync );
    NATIVE_FUNCTION( "FadeOutAsync", funcFadeOutAsync );
    NATIVE_FUNCTION( "IsFading", funcIsFading );
    NATIVE_FUNCTION( "IsBlackscreen", funcIsBlackscreen );
	NATIVE_FUNCTION( "SetFadeLock", funcSetFadeLock );
	NATIVE_FUNCTION( "ResetFadeLock", funcResetFadeLock );
    NATIVE_FUNCTION( "UnlockAchievement", funcUnlockAchievement );
    NATIVE_FUNCTION( "LockAchievement", funcLockAchievement );
    NATIVE_FUNCTION( "GetUnlockedAchievements", funcGetUnlockedAchievements );
    NATIVE_FUNCTION( "GetAllAchievements", funcGetAllAchievements );
    NATIVE_FUNCTION( "IsAchievementUnlocked", funcIsAchievementUnlocked );
    NATIVE_FUNCTION( "GetDifficultyLevel", funcGetDifficulty );
	NATIVE_FUNCTION( "SetDifficultyLevel", funcSetDifficulty );
	NATIVE_FUNCTION( "ToggleUserProfileManagerInputProcessing", funcToggleUserProfileManagerInputProcessing );
    NATIVE_FUNCTION( "IsCheatEnabled", funcIsCheatEnabled );
    NATIVE_FUNCTION( "ReloadGameplayConfig", funcReloadGameplayConfig );
    NATIVE_FUNCTION( "GetGameplayChoice", funcGetGameplayChoice );
    NATIVE_FUNCTION( "IsCurrentlyPlayingNonGameplayScene", funcIsCurrentlyPlayingNonGameplayScene );
    NATIVE_FUNCTION( "IsFinalBuild", funcIsFinalBuild );
    NATIVE_FUNCTION( "PauseCutscenes", funcPauseCutscenes );
    NATIVE_FUNCTION( "UnpauseCutscenes", funcUnpauseCutscenes );
    NATIVE_FUNCTION( "TogglePad", funcTogglePad );
    NATIVE_FUNCTION( "IsPadConnected", funcIsPadConnected );
	NATIVE_FUNCTION( "IsVibrationEnabled", funcIsVibrationEnabled );
	NATIVE_FUNCTION( "SetVibrationEnabled", funcSetVibrationEnabled );
	NATIVE_FUNCTION( "VibrateController", funcVibrateController );
	NATIVE_FUNCTION( "StopVibrateController", funcStopVibrateController );
	NATIVE_FUNCTION( "GetCurrentVibrationFreq", funcGetCurrentVibrationFreq );
	NATIVE_FUNCTION( "RemoveSpecificRumble", funcRemoveSpecificRumble );
	NATIVE_FUNCTION( "IsSpecificRumbleActive", funcIsSpecificRumbleActive );
	NATIVE_FUNCTION( "OverrideRumbleDuration", funcOverrideRumbleDuration );
	NATIVE_FUNCTION( "GetGameplayConfigFloatValue", funcGetGameplayConfigFloatValue );
	NATIVE_FUNCTION( "GetGameplayConfigBoolValue", funcGetGameplayConfigBoolValue );
	NATIVE_FUNCTION( "GetGameplayConfigIntValue", funcGetGameplayConfigIntValue );
	NATIVE_FUNCTION( "GetGameplayConfigEnumValue", funcGetGameplayConfigEnumValue );
	NATIVE_FUNCTION( "SetAIObjectsLooseTime", funcSetAIObjectsLooseTime );
	NATIVE_FUNCTION( "SetSingleShotLoadingScreen", funcSetSingleShotLoadingScreen );
	NATIVE_FUNCTION( "GetCurrentViewportResolution", funcGetCurrentViewportResolution );
	NATIVE_FUNCTION( "DebugActivateContent", funcDebugActivateContent );
	NATIVE_FUNCTION( "HasBlackscreenRequested", funcHasBlackscreenRequested );
END_CLASS_RTTI();

// The game instance
extern CGame* GGame;
