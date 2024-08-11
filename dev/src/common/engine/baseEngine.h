/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "scriptCompilationHelper.h"
#include "networkedLogOutputDevice.h"
#include "../redNetwork/manager.h"
#include "../redNetwork/ping.h"
#include "../core/engineTime.h"
#include "../core/resourcepaths.h"
#include "../core/diskBundleContent.h"
#include "../core/softHandleProcessor.h"
#include "viewport.h"
#include "inGameConfigRefreshEvent.h"

// System flags
extern Bool GDataAsserts;

// The engine
extern class CBaseEngine* GEngine;

class CRenderFrame;
class IDebugPage;
class IDebugCounter;
class IPlatformViewport;
class CDrawableComponent;
class IInputDeviceManager;
class CMarkersSystem;
class CFlashPlayer;
class IRenderVideo;

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			class CBundleMetadataStore;
		}
	}
}

namespace Config
{
	extern TConfigVar< Int32, Validation::IntRange< 0, INT_MAX > >			cvLimitFPS;

	extern TConfigVar< Bool >												cvDebugDumpNameList;
}

// Active subsystems flags ( simulated )
enum EActiveSubsystems
{
	ES_Physics				= FLAG( 0 ),
//	ES_Audio				= FLAG( 1 ),
	ES_Particles			= FLAG( 2 ),
	ES_Animation			= FLAG( 3 ),
	ES_Scripts				= FLAG( 4 ),
	ES_Rendering			= FLAG( 5 ),
};

///////////////////////////////////////////////////////////////////////////////////////////////

/// Mesh coloring scheme
class IMeshColoringScheme
{
protected:
	Bool							m_refreshOnMove;		//!< Refresh the mesh coloring every time the camera moves

public:
	IMeshColoringScheme() : m_refreshOnMove( false ) {}
	virtual ~IMeshColoringScheme() {};
	
	//! Get mesh Selection color
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const = 0;
	
	//! Generate editor fragments
	virtual void GenerateEditorFragments( CRenderFrame* frame ) = 0;

	//! Returns true if the mesh coloring should be refreshed every time the camera moves
	RED_INLINE Bool GetRefreshOnMove() const { return m_refreshOnMove; }
};

enum EMaterialDebugMode : Int32
{
	MDM_None,
	MDM_UVDensity,
	MDM_Holes,
	MDM_Mask,
	MDM_Overlay,
	MDM_Heightmap,
	MDM_WaterMode,
	MDM_FullGBuffer,

	MDM_Max,
};

enum EBaseEngineState : Uint32
{
	BES_Initializing,
	BES_Running,
	BES_Suspended,
	BES_Constrained,
	BES_Unknown
};

class CBaseEngineRefreshListener : public InGameConfig::IRefreshEventListener
{
public:
	CBaseEngineRefreshListener();

	virtual void OnRequestRefresh(const CName& eventName);
	Bool GetAndClearRefreshGraphics();
	Bool GetAndClearRefreshViewport();

private:
	Red::Threads::CAtomic<Bool> m_refreshGraphicsRequest;
	Red::Threads::CAtomic<Bool> m_refreshViewportRequest;

};

/// Base engine implementation
class CBaseEngine
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	//! Get the state of the engine
	RED_INLINE EBaseEngineState GetState() const { return m_state; }

	//! Get raw engine time so far
	RED_INLINE const EngineTime& GetRawEngineTime() const { return m_rawEngineTime; }

	//! Get last time delta
	RED_INLINE Float GetLastTimeDelta() const { return m_lastTimeDelta; }

	//! Get last time delta
	RED_INLINE Float GetLastTimeDeltaUnclamped() const { return m_lastTimeDeltaUnclamped; }

	//! Get last FPS
	RED_INLINE Float GetLastTickRate() const { return m_lastTickRate; }

	//! Get minimum FPS
	RED_INLINE Float GetMinTickRate() const { return m_minTickRateToShow; }

	//! Get current engine tick
	RED_INLINE Uint64 GetCurrentEngineTick() const { return m_currentEngineTick; }

#ifndef NO_MARKER_SYSTEMS
	//! Get marker systems manager
	RED_INLINE CMarkersSystem* GetMarkerSystems() const { return m_markerSystemsManager; }
#endif	// NO_MARKER_SYSTEMS

	//! Is the subsystem active
	RED_INLINE Bool IsActiveSubsystem( Uint32 flag ) const { return ( m_activeSubsystems & flag ) != 0; }

	//! Get current mesh coloring scheme
	RED_INLINE IMeshColoringScheme* GetMeshColoringScheme() const { return m_meshColoringScheme; }

	//! Returns true if the current mesh coloring scheme requires refreshing the coloring at every camera move
	RED_INLINE Bool GetMeshColoringRefreshOnCameraMove() const { return m_meshColoringScheme ? m_meshColoringScheme->GetRefreshOnMove() : false; }

	RED_INLINE Int32 GetReturnValue() { return m_returnValue; }

	virtual Bool IsPostInitialized() = 0;

	virtual void PostInitialize( float /*timeDelta*/ ) {}

	RED_INLINE CStandardRand& GetRandomNumberGenerator() { return m_randomNumberGenerator; }

#ifndef NO_DEBUG_PAGES
	//! Get active debug page
	RED_INLINE IDebugPage* GetActiveDebugPage() const { return m_activeDebugPage; }

	//! Get debug counters
	RED_INLINE const TDynArray< IDebugCounter* >& GetDebugCounters() const { return m_debugCounter; }

	//! Return array of debug pages
	RED_INLINE TDynArray< IDebugPage* >& GetDebugPages() { return m_debugPages; }
#endif

public:
	CBaseEngine();
	virtual ~CBaseEngine();

	// Active subsystems
	void SetActiveSubsystem( Uint32 flag, Bool active = true );

	/////////////////////////////////////////////////////////////////////
	// Process lifetime states

	// Suspend / Resume states are used on the consoles when the app is no longer running / returning from suspend
	virtual void OnSuspend( Bool constrained );
	virtual void OnResume( Bool constrained );

	// Normal / Constrained states are used to determine when the app has full access to the cpu / gpu and input
	virtual void OnEnterNormalRunningMode();
	virtual void OnEnterConstrainedRunningMode();
	virtual void OnExitConstrainedRunningMode();

	// Request exit from the game (This is the return value of the executable to the operating system, non-zero indicates failure)
	void RequestExit( Int32 programReturnValue = 0 );

	// Request reloading of scripts
	RED_INLINE void RequestScriptsReload()
	{ 
#ifdef RED_NETWORK_ENABLED
		m_scriptHandler.RequestReload();
#endif
	}

	IInputDeviceManager* GetInputDeviceManager() const;

	// Flush all loading
	void FlushAllLoading();
	
	void FlushJobs();

	// Initialize engine, create platform dependant systems
	virtual Bool Initialize();

	// Shutdown engine, called when main loop exits
	virtual void Shutdown();

	// Engine main loop
	virtual void MainLoop();

	// Engine tick method, called from within main loop
	virtual void Tick( Float timeDelta );

	// Single tick of the main loop. If false returned, exit the game
	virtual Bool MainLoopSingleTick();

	// Generate debug HUD
	virtual void GenerateDebugFragments( CRenderFrame* frame );

	// Request viewport refresh
	void RequestRefreshViewport();

	// Hosted game has ended
	virtual void OnGameEnded();

	// User event callback (sign in, sign out, etc.) executed on main thread
	virtual void OnUserEvent( const EUserEvent& event );

	// Use this to figure out where certain streaming resources reside on disk
	RED_INLINE Red::Core::ResourceManagement::CResourcePaths& GetPathManager() { return m_resourcePaths; }
	RED_INLINE const Red::Core::ResourceManagement::CResourcePaths& GetPathManager() const { return m_resourcePaths; }

public:
#ifndef NO_DEBUG_PAGES
	//! Register debug page
	void RegisterDebugPage( IDebugPage* page );

	//! Unregister debug page
	void UnregisterDebugPage( IDebugPage* page );

	//! Register debug counter
	void RegisterDebugCounter( IDebugCounter* counter );

	//! Register debug counter
	void UnregisterDebugCounter( IDebugCounter* counter );

	//! Show next debug page
	void ShowNextDebugPage();

	//! Show previous debug page
	void ShowPrevDebugPage();

	//! Select custom debug page
	void SelectDebugPage( IDebugPage* page );
#endif

public:
	//! Set mesh coloring scheme, remove the previous one if existed
	void SetMeshColoringScheme( IMeshColoringScheme* newColoringScheme );

	//! Get material debug mode
	EMaterialDebugMode GetRenderDebugMode() const { return m_materialDebugMode; }

	//! Set material debug mode
	void SetRenderDebugMode( EMaterialDebugMode mdm ){ m_materialDebugMode = mdm; }

	//! Silent compilation feedback means no popups (useful for automated execution)
	void SetSilentCompilationFeedback( bool silent ){ m_silentScripts = silent; }

	// Engine is used as a bridge between the high-level platform stuff and the renderer
	virtual IPlatformViewport* GetPlatformViewport() = 0;	

	// Get custom script compilation error handling interface
	virtual IScriptCompilationFeedback* QueryScriptCompilationFeedback() { return nullptr; }

	virtual Bool IsFPSDisplayEnabled() const = 0;

private:
	// Handles gracefull exit from Initialize(), returns false (so it can be nicely chained)
	Bool GracefulExit( const Char* failMessage );

	// System initialization
	Bool InitializeContentManager();
	Bool InitializeTaskManager();
	Bool InitializeBundles();
	Bool InitializeNetwork();
	Bool InitializeDebugNetwork();
	Bool CreateRenderer();
	Bool InitializeRenderer();
	Bool InitializeSoundSystem();
	Bool InitializeSoundSystemObject();
	Bool InitializeGUI();
	Bool InitializeRTTI();
	Bool InitializePhysicsDebugger();
	Bool InitializeStartupPackage();
	Bool InitializeScripts( Bool& outIsKosherScripts );
	Bool InitializeUserProfileManager();
	Bool InitializeInputDeviceManager();
	Bool CreateGame();
	Bool InitializeGame();
	Bool InitializePhysics();
	Bool InitializeMarkerSystem();
	Bool InitializeAnimationSystem();
	Bool InitializeDebugWindows();
	Bool InitializeDebugServer();
	Bool InitializeProfilers();
#ifndef RED_FINAL_BUILD
	Bool CheckMemoryUsage();
#endif

	Bool ShutdownInputDeviceManager();
	Bool ShutdownUserProfileManager();

protected:
	// Frame (technically Tick()) start / end calls
	void OnFrameStart();
	void OnFrameEnd();

	// Refresh engine (drawable components, textures, etc.)
	virtual void RefreshEngine();

protected:
	// Engine state
	EngineTime										m_rawEngineTime;		//!< Raw engine time, counter from the start of the game
	Float											m_lastTimeDelta;		//!< Last time delta used to tick the engine
	Float											m_lastTimeDeltaUnclamped;
	Float											m_lastTickRate;			//!< Tick rate ( FPS )
	Float											m_minTickRateToShow;	//!< Minimum tick rate 
	Float											m_minTickRateOver1Sec;	//!< Minimum tick rate over the last sec
	Uint64											m_currentEngineTick;	//!< Current engine tick
	Bool											m_requestExit;			//!< Request an exit from the application
	Uint32											m_activeSubsystems;		//!< Active subsystems flags ( simulated )
	IMeshColoringScheme*							m_meshColoringScheme;	//!< Mesh coloring scheme
	EMaterialDebugMode								m_materialDebugMode;	//!< Material debug mode
	Bool											m_postInitialized;
	Bool											m_silentScripts;
	Int32											m_returnValue;
	CStandardRand									m_randomNumberGenerator;

	// Engine refresh flag
	CBaseEngineRefreshListener						m_refreshListener;

#ifdef RED_NETWORK_ENABLED
	Red::Network::Manager							m_network;

#ifdef RED_LOG_VIA_NETWORK
	CNetworkedLogOutputDevice						m_networkedLogger;
#endif

	CScriptNetworkHandler							m_scriptHandler;
	Red::Network::Ping								m_pingUtil;
#endif // RED_NETWORK_ENABLED

	IInputDeviceManager*							m_inputDeviceManager;

	Red::Core::ResourceManagement::CResourcePaths	m_resourcePaths;

#ifndef NO_DEBUG_PAGES
	TDynArray< IDebugPage* >						m_debugPages;			//!< Debug pages
	TDynArray< IDebugCounter* >						m_debugCounter;			//!< Debug counters
	IDebugPage*										m_activeDebugPage;		//!< Active debug page
#endif

#ifndef NO_MARKER_SYSTEMS
	CMarkersSystem*									m_markerSystemsManager;	//!< Manager for all system based on markers
#endif

	// Main loop tick counters
	Double											m_oldTickTime;
	Double											m_prevTickRateTime;
	Int32											m_numTicks;
	Bool											m_usingBundles;
	Bool											m_noRuntimeGC;

	// Initialization flags (for gracefull exits)
	Uint32											m_hasDebugNetworkInitialized:1;
	Uint32											m_hasSoundInitialized:1;

	// Startup content
	THandle< CDiskBundleContent >					m_startupContent;

	EBaseEngineState								m_state;
};

// variables for playing movies before all systems are initialised
namespace QuickBoot
{
	extern ViewportHandle	g_quickInitViewport;
	extern CFlashPlayer*	g_quickInitFlashPlayer;
	extern IRenderVideo*	g_lastBumperVideo;
}
