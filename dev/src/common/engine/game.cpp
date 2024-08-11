/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "game.h"
#include "freeCamera.h"
#include "gameSession.h"
#include "soundStartData.h"
#include "staticCamera.h"
#include "cameraDirector.h"
#include "gameResource.h"
#include "flashPlayer.h"
#include "../physics/physicsDebugger.h"
#include "timerKeyword.h"
#include "reviewSystem.h"
#include "renderCommands.h"
#include "viewport.h"
#include "globalEventsManager.h"
#include "renderFence.h"
#include "visualDebug.h"
#include "cutsceneInstance.h"
#include "animationManager.h"
#include "gameTimeManager.h"
#include "idTagManager.h"
#include "soundSystem.h"
#include "debugConsole.h"
#include "worldTick.h"
#include "layerGroup.h"
#include "world.h"
#include "dynamicLayer.h"
#include "tagManager.h"
#include "baseEngine.h"
#include "inputManager.h"
#include "inputGameInputModeListener.h"
#include "foliageEditionController.h"
#include "debugServerManager.h"
#include "speedConfig.h"
#include "loadingScreen.h"
#include "loadingOverlay.h"
#include "videoPlayer.h"
#include "renderSettings.h"
#include "containerManager.h"
#include "anselIntegration.h"

#include "../core/2darray.h"
#include "../core/tagList.h"
#include "../core/gatheredResource.h"
#include "../core/loadingJobManager.h"
#include "../core/loadingProfiler.h"
#include "../core/depot.h"
#include "../core/depotBundles.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptingSystem.h"
#include "../core/bundleMetadataStore.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/messagePump.h"

#include "../engine/scaleformSystem.h"
#include "../core/contentManager.h"
#include "inGameConfigListingFunction.h"

//Ansel integration
#ifdef USE_ANSEL
#include "../physics/physicsWorld.h"
#include "../physics/physicsWorldUtils.h"
#include "../engine/umbraScene.h"
#include "../engine/anselIncludes.h"
#include "../renderer/renderViewport.h"
#include "../renderer/renderViewportWindow.h"

#include "renderer.h"
#include "renderThread.h"
#endif // USE_ANSEL

#ifdef USE_ANSEL
Bool isAnselTurningOn			= false;
Bool isAnselTurningOff			= false;
Bool isAnselSessionActive		= false;
Bool isAnselCaptureActive		= false;
Bool isAnselParticleAttach		= false;
Float anselParticlesPrewarmTime = 15.0f;
Float anselFadeTime				= 0.01f;
EAnselState anselState			= EAS_Off;
Float anselLastFov				= 60.f;
static const Uint32 DEFAULT_TICKS_TO_PAUSE					= 2;
static const Float ANSEL_CAMERA_DEFAULT_DISTANCE_THRESHOLD	= 110.0f;
static const Float ANSEL_CAMERA_WYZIMA_DISTANCE_THRESHOLD	= 12.0f;

Matrix anselCameraTransform;
#endif // USE_ANSEL

#ifdef USE_ANSEL
namespace
{
	static CRenderCamera 				cwCachedCamera;
	static SRenderCameraLastFrameData	cwCachedLastFreeCameraData;
	ansel::Camera						cwCamera;
	static Bool							wasUmbraTurnedOnBeforeAnselToggle;
}
#endif // USE_ANSEL

namespace Config
{
	TConfigVar<Bool> cvDoNotPauseWhileStopped( "Game", "DoNotPauseWhileStopped", false, eConsoleVarFlag_Save );
	TConfigVar<Int32> cvCheatLevel( "Cheat", "CheatLevel", 0, eConsoleVarFlag_Developer );
	TConfigVar<Bool>  cvCheatAllowFreeCamera( "Cheat", "AllowFreeCamera", true, eConsoleVarFlag_Developer );
	TConfigVar<Int32> cvDiffivulty( "Gameplay", "Difficulty", 0, eConsoleVarFlag_Save );
	TConfigVar<Bool> cvCameraBasedStreamingOverride( "Streaming", "UseCameraForStreamingOverride", false );
	TConfigVar<Bool> cvDBGConsoleOn( "General", "DBGConsoleOn", false, eConsoleVarFlag_ReadOnly );
}

CGatheredResource resCutsceneArrayDef( TXT("gameplay\\globals\\cutscene.csv"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CGame )
IMPLEMENT_ENGINE_CLASS( STimeScaleSource );
IMPLEMENT_ENGINE_CLASS( STimeScaleSourceSet );
IMPLEMENT_RTTI_ENUM( ECheats )
IMPLEMENT_RTTI_ENUM( EPersistanceMode )
RED_DECLARE_DEBUG_CALLBACK_TAG( Game );

CGame* GGame;

#ifndef NO_DEBUG_PAGES
extern volatile Float GLastRenderFenceTime;
#endif // !NO_DEBUG_PAGES

RED_DEFINE_STATIC_NAME( npc );
RED_DEFINE_STATIC_NAME( player );

//////////////////////////////////////////////////////////////////////////

Bool CGame::CRumbleLogic::SRumble::IsEqual( Float lowFreq, Float highFreq ) const
{
	return ( ( Abs( m_lowFreqMotorSpeed - lowFreq ) < 0.01f ) && ( Abs( m_highFreqMotorSpeed - highFreq ) < 0.01f ) );
}


void CGame::CRumbleLogic::AddRumble( Float lowFreqSpeed, Float highFreqSpeed, Float duration )
{
	Bool vibrationEnabled = false;
	if ( !SInputUserConfig::GetIsVibrationEnabled( vibrationEnabled ) || vibrationEnabled )
	{
		ASSERT( duration < 10.0f );	// Arbitrary value, sanity check
		m_currentRumbles.PushBack( SRumble( lowFreqSpeed, highFreqSpeed, duration ) );
	}
}

Bool CGame::CRumbleLogic::UpdateRumbles( Float timeDelta, Float& lowFreqSpeed, Float& highFreqSpeed )
{
	if ( m_currentRumbles.Empty() )
	{
		// No rumbles
		if ( m_lowFreqMotorSpeed > 0.0f || m_highFreqMotorSpeed > 0.0f )
		{
			LOG_ENGINE( TXT("==== Stopping rumbles.") );
			m_lowFreqMotorSpeed = 0.0f;
			m_highFreqMotorSpeed = 0.0f;
			// Need to update motors
			return true;
		}

		// No need to update motors
		return false;
	}

	// Update rumbles
	Float tempLowFreqSpeed = 0.0f;
	Float tempHighFreqSpeed = 0.0f;
	ASSERT( !m_currentRumbles.Empty() );
	TDynArray< SRumble >::iterator	rumbleIter = m_currentRumbles.End() - 1;
	for ( ; rumbleIter >= m_currentRumbles.Begin(); --rumbleIter )
	{
		if ( rumbleIter->m_timeLeft <= 0.0f )
		{
			// Rumble ended, remove it
			m_currentRumbles.Erase( rumbleIter );
			continue;
		}
		tempLowFreqSpeed += rumbleIter->m_lowFreqMotorSpeed;
		tempHighFreqSpeed += rumbleIter->m_highFreqMotorSpeed;
		rumbleIter->m_timeLeft -= timeDelta;
	}

	// Update return values
	lowFreqSpeed = Clamp< Float >( tempLowFreqSpeed, 0.0f, 1.0f );
	highFreqSpeed = Clamp< Float >( tempHighFreqSpeed, 0.0f, 1.0f );

	// Affect motors?
	if ( m_lowFreqMotorSpeed != tempLowFreqSpeed || m_highFreqMotorSpeed != tempHighFreqSpeed )
	{
		m_lowFreqMotorSpeed = tempLowFreqSpeed;
		m_highFreqMotorSpeed = tempHighFreqSpeed;

		LOG_ENGINE( TXT("==== Motors speed changed.") );
		// Motors speed changed, update
		return true;
	}

	// No need to update motors
	return false;
}

Bool CGame::CRumbleLogic::Pause()
{
	Bool retVal = false;

	if ( m_lowFreqMotorSpeed > 0.0f || m_highFreqMotorSpeed >0.0f )
	{
		// Should stop motors
		retVal = true;
	}
	m_lowFreqMotorSpeed = 0.0f;
	m_highFreqMotorSpeed = 0.0f;
	return retVal;
}

Bool CGame::CRumbleLogic::Reset()
{
	m_currentRumbles.Clear();
	if ( m_lowFreqMotorSpeed > 0.0f || m_highFreqMotorSpeed > 0.0f )
	{
		m_lowFreqMotorSpeed = 0.0f;
		m_highFreqMotorSpeed = 0.0f;

		// Need to stop motors
		return true;
	}
	
	// Motors idle anyway
	return false;
}

void CGame::CRumbleLogic::RemoveRumble( Float lowFreqSpeed, Float highFreqSpeed )
{
	for( auto it = m_currentRumbles.Begin(); it != m_currentRumbles.End(); ++it )
	{
		if( it->IsEqual( lowFreqSpeed, highFreqSpeed ) )
		{
			m_currentRumbles.EraseFast( it );
			return;
		}
	}
}

Bool CGame::CRumbleLogic::IsRumbleActive( Float lowFreqSpeed, Float highFreqSpeed ) const
{
	for( auto it = m_currentRumbles.Begin(); it != m_currentRumbles.End(); ++it )
	{
		if( it->IsEqual( lowFreqSpeed, highFreqSpeed ) )
		{
			return true;
		}
	}

	return false;
}


void CGame::CRumbleLogic::StopRumbles()
{
	m_currentRumbles.ClearFast();
}


void CGame::CRumbleLogic::GetCurrentFreq( Float& lowFreq, Float& highFreq ) const
{
	lowFreq = m_lowFreqMotorSpeed;
	highFreq = m_highFreqMotorSpeed;
}

void CGame::CRumbleLogic::OverrideDuration( Float lowFreq, Float highFreq, Float newDuration )
{
	for( auto it = m_currentRumbles.Begin(); it != m_currentRumbles.End(); ++it )
	{
		if( it->IsEqual( lowFreq, highFreq ) )
		{
			it->m_timeLeft = newDuration;
			return;
		}
	}

	AddRumble( lowFreq, highFreq, newDuration );
}



//////////////////////////////////////////////////////////////////////////

const Uint8 CGame::CHEAT_LEVELS[] = { 
	1,	 // CHEAT_Console,
	1,	 // CHEAT_FreeCamera,
	1,	 // CHEAT_DebugPages,
	2,	 // CHEAT_InstantKill,
	2,	 // CHEAT_Teleport,
	2,	 // CHEAT_MovementOnPhysics,
	2,	 // CHEAT_TimeScaling,

	0	 // CHEAT_MAX (for safety reasons)
}; 

CGame::CGame()
	: m_activeWorld( NULL )
	, m_engineTime( 0.0f )
	, m_cleanEngineTime( 0.0f )
	, m_gameTime( 0.0f )
	, m_isActive( false )
	, m_isLoadingWorld( false )
	, m_requestEnd( false )
	, m_requestEndDueToSignout( false )
	, m_isInStartGame( false )
	, m_stopped( false )
	, m_requestUnstop( false )
	, m_isKeyHandled( false )
	, m_blockingLoadingCooldown( 0.0f )
#if defined RED_PLATFORM_WINPC && defined( NO_EDITOR )
	, m_isUsingPad( false )
#else
	, m_isUsingPad( true )
#endif
	, m_inputManager( NULL )
	, m_videoPlayer( NULL )
	, m_flashPlayer( NULL )
	, m_loadingScreen( NULL )
	, m_loadingOverlay( NULL )
	, m_camera( NULL )
	, m_timeManager( NULL )
	, m_speedConfigManager ( nullptr )
	, m_gameInputEnabled( true )
	, m_renderFence( NULL )
	, m_blackscreenColor( Color::BLACK )
	, m_shiftState( IACT_None )
	, m_isContignousCapture( false )
	, m_doNotPauseWhileStopped( false )
	, m_disablePausingOnApplicationIdle( 0 )
	, m_difficulty( 2 ) // 0 = EDM_NotSet, 1 == EDM_Easy, 2 == EDM_Medium, 3 == EDM_Hard, 4 == EDM_Hardcore
	, m_cheatLevel( 0 )	// 0 == no cheats, 1 == only level 1 cheats, 2 == all cheats
	, m_nonGameplaySceneActive( false )
	, m_kosherTaintFlags(0)
	, m_requestEndReason(ENDGAMEREASON_Default)
	, m_timerScriptKeyword( NULL )
	, m_gameResource( NULL )
	, m_aiObjectLooseTime( 15.0f )
	, m_globalEventsManager( nullptr )
	, m_saveLockForBlackscreens( CGameSessionManager::GAMESAVELOCK_INVALID )
	, m_prefetchPosition( Vector::ZEROS )
	, m_requestStreamingPrefetch( false )
	, m_streamingLocksModified( false )
	, m_enableStreamingPrefetch( true )
	, m_blackscreenWasSetPrevFrame( false )
	, m_containerManager( nullptr )
	, m_didRenderLastFrame( false )
{
	RED_REGISTER_DEBUG_CALLBACK( Game, &CGame::DebugCallback );

	m_doNotPauseWhileStopped = Config::cvDoNotPauseWhileStopped.Get();
	m_cheatLevel = Config::cvCheatLevel.Get();
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("Gameplay"), TXT("UsePad"), m_isUsingPad );

	//////////////////////////////////////////////////////////////////////////
	// For debug

	m_cheatLevel = 2;

	//////////////////////////////////////////////////////////////////////////

	ReloadConfig();

	// update initial streaming stuff
	UpdateStreamingModeAndDistances();

	m_freeCamera = new CGameFreeCamera();

	m_loadingScreenStack.PushBack( SLoadingScreenParam::DEFAULT );
}

CGame::~CGame()
{
	if ( m_freeCamera )
	{
		delete m_freeCamera;
	}

	RED_UNREGISTER_DEBUG_CALLBACK( Game );
}

void CGame::ReloadConfig()
{
	m_gameplayConfig.Load();
	m_gameplayConfig.Validate();
	OnReloadedConfig();
}

extern class CSectorPrefetchMemoryBuffer* GetGlobalPrefetchBuffer();

void CGame::Init()
{
	// Create global streaming prefetch buffer BEFORE we fragment memory to much
	GetGlobalPrefetchBuffer();

	// Create input manager
	m_inputManager = CreateObject< CInputManager >( this );
	m_inputManager->GetGestureSystem();

	m_containerManager = new CContainerManager;

	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_INPUT, m_inputManager );

	GUserProfileManager->RegisterListener( &CGame::OnUserEvent, this, Events::P_Low );

	// Create global events manager
	m_globalEventsManager = new CGlobalEventsManager();

	// Create video player
	m_videoPlayer = new CVideoPlayer;

	m_flashPlayer = QuickBoot::g_quickInitFlashPlayer ? QuickBoot::g_quickInitFlashPlayer : CFlashPlayer::CreateFlashPlayerInstance();

	m_loadingScreen = new CLoadingScreen;
	if ( ! m_loadingScreen->Init() )
	{
		WARN_ENGINE(TXT("Failed to initialize loading screen"));
	}

	m_loadingOverlay = new CLoadingOverlay;
	// Delayed init on loading overlay!

	// Spawn time machine
	m_timeManager = new CTimeManager();

	// Spawn IdTag manager
	m_idTagManager = new CIdTagManager();

	// Reset the universe
	m_universeStorage.Reset();

	// Create visual debugger, CObject because it's exposed to scripts
	m_visualDebug = CreateObject< CVisualDebug >( this );

	// Initialise Script time information
	m_timerScriptKeyword = CreateObject< CTimerScriptKeyword >( this );
	ASSERT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_TIMER ) == NULL, TXT( "About to replace an existing CTimerScriptKeyword in script globals" ) );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_TIMER, m_timerScriptKeyword );
}

void CGame::ShutDown()
{
	LOG_ENGINE( TXT("Shutting down game engine") );

	if( m_timerScriptKeyword )
	{
		ASSERT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_TIMER ) == m_timerScriptKeyword, TXT( "About to NULL a CTimerScriptKeyword which isn't the same as in script globals" ) );
		GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_TIMER, NULL );

		m_timerScriptKeyword->Discard();
		m_timerScriptKeyword = NULL;
	}

	// Destroy visual debuffer
	if ( m_visualDebug )
	{
		m_visualDebug->Discard();
		m_visualDebug = NULL;
	}

	// Unregister cutscenes
	UnregisterAllCutscenes();

	// Delete time machine
	if ( m_timeManager )
	{
		delete m_timeManager;
		m_timeManager = NULL;
	}

	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_INPUT, NULL );
	if( m_inputManager )
	{
		m_inputManager->Discard();
		m_inputManager = NULL;
	}	

	// Delete IdTag manager
	if ( m_idTagManager )
	{
		delete m_idTagManager;
		m_idTagManager = NULL;
	}

	// Delete loading screen.
	// Moved under UnloadWorld() because CDrawableComponent checks the loading screen status even during shutdown
	delete m_loadingScreen;
	m_loadingScreen = NULL;

	// Delete video player
	delete m_videoPlayer;
	m_videoPlayer = NULL;

	delete m_loadingOverlay;
	m_loadingOverlay = NULL;

	// Delete flash player
	delete m_flashPlayer;
	m_flashPlayer = NULL;

	if ( m_globalEventsManager != nullptr )
	{
		delete m_globalEventsManager;
		m_globalEventsManager = nullptr;
	}

	delete m_containerManager;
	m_containerManager = nullptr;
}

namespace Hacks
{
	extern void RenderAudioWhileBlockingGame();
	extern void RenderAudioForLoadingScreen();
	extern void MuteAudioForLoadingScreen();
	extern void UnmuteAudioAfterLoadingScreen();
}

Bool CGame::ShowLoadingScreen()
{
	LOG_ENGINE( TXT("ShowLoadingScreen") );

	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	const SLoadingScreenParam& activeParam = m_loadingScreenStack.Back();

	if ( activeParam.m_supressGameAudio )
	{
		LOG_ENGINE(TXT("Loading screen suppressing game audio (switching to 'game_state' 'movie'"));
		GSoundSystem->SoundSwitch( "game_state", "movie" );
	}

//	Hacks::MuteAudioForLoadingScreen();

	const Bool retval = m_loadingScreen->Show( activeParam );

	if ( retval )
	{
		OnLoadingScreenShown();
	}

	// Must always keep the first loading screen as a default unless replaced
	if ( m_loadingScreenStack.Size() > 1 )
	{
		m_loadingScreenStack.PopBack();
	}
	return retval;
}

void CGame::OnUserEvent( const EUserEvent& event )
{
	if( event == EUserEvent::UE_LoadSaveReady )
	{
		SetGameDifficulty( Config::cvDiffivulty.Get() );

		if ( m_viewport )
		{
			m_viewport->NotifyGammaChanged();
		}

		//make sure it's reset for the new user
		m_requestEndDueToSignout = false;

		m_inputManager->ReloadSettings();

		CallEvent( RED_NAME( OnUserSignedIn ) );

		LOG_ENGINE( TXT( "User signed in: %ls" ), GUserProfileManager->GetActiveUserDisplayName( 50 ).AsChar() );
	}
	else if( event == EUserEvent::UE_SignedOut )
	{
		if ( IsBlackscreen() )
		{
			SetBlackscreen( false, TXT( "RequestGameEnd" ), Color::BLACK );
		}

		m_requestEndDueToSignout = true;

		CallEvent( RED_NAME( OnUserSignedOut ) );

		LOG_ENGINE( TXT( "User signed out" ) );
	}
	else if( event == EUserEvent::UE_AccountPickerOpened )
	{
	}
	else if( event == EUserEvent::UE_AccountPickerClosed )
	{
	}
	else if( event == EUserEvent::UE_SignInStarted )
	{
		CallEvent( RED_NAME( OnSignInStarted ) );
	}
	else if( event == EUserEvent::UE_SignInCancelled )
	{
		CallEvent( RED_NAME( OnSignInCancelled ) );
	}
}

//#define NO_RUMBLE

void CGame::VibrateController( Float lowFreqSpeed, Float highFreqSpeed, Float duration )
{
#ifndef NO_RUMBLE
	if( !m_inputManager || !m_inputManager->LastUsedGamepad() )
	{
		return;
	}

	Bool vibrationEnabled = false;
	if ( !SInputUserConfig::GetIsVibrationEnabled( vibrationEnabled ) || vibrationEnabled )
	{
		m_rumbleLogic.AddRumble( lowFreqSpeed, highFreqSpeed, duration );
		//LOG_ENGINE( TXT("==== Adding rumble  %f / %f / %f"), lowFreqSpeed, highFreqSpeed, duration );
	}
#endif
}

void CGame::UpdateForceFeedback( Float timeDelta )
{
	static Bool controllerBusyLastTime = false;
	Float lowFreqSpeed = 0.0f;
	Float highFreqSpeed = 0.0f;

#ifdef NO_EDITOR
	if ( !IsActive() )
	{
		// Reset rumble state
		if ( m_rumbleLogic.Reset() || controllerBusyLastTime )
		{
			controllerBusyLastTime = !SRawInputManager::GetInstance().SetPadVibrate( 0.0f, 0.0f );
			LOG_ENGINE( TXT("==== Resetting rumbles.") );
		}
		return;
	}
#endif

	if ( !IsPaused() && !IsActivelyPaused() )
	{
		if ( m_rumbleLogic.UpdateRumbles( timeDelta, lowFreqSpeed, highFreqSpeed ) || controllerBusyLastTime )
		{
			// Desired motor speed changed, or couldn't update previously. Update low level
			controllerBusyLastTime = !SRawInputManager::GetInstance().SetPadVibrate( lowFreqSpeed, highFreqSpeed );
		}

		if ( controllerBusyLastTime )
		{
			LOG_ENGINE( TXT("==== Controller is busy") );
		}
	}
	else
	{
		if ( m_rumbleLogic.Pause() || controllerBusyLastTime )
		{
			controllerBusyLastTime = !SRawInputManager::GetInstance().SetPadVibrate( 0.0f, 0.0f );
		}

		if ( controllerBusyLastTime )
		{
			LOG_ENGINE( TXT("==== Controller is busy") );
		}
	}
}

void CGame::OnLanguageChange()
{
	// TBD: Verify language on startup/default console language!!!
#ifdef USE_SCALEFORM
	CScaleformSystem* system = CScaleformSystem::StaticInstance();
	if ( system )
	{
		system->OnLanguageChange();
	}
#endif
}

Bool GPumpingMessagesOutsideMainLoop;

#ifdef RED_PLATFORM_WINPC
void Hack_PumpWindowsMessagesForDXGI()
{
	// Not bothering to filter as experimentally makes no difference and supported by technical reference. Quit message is posted, so won't miss processing 
	// it here. Doubtful PM_NOYIELD makes any practical difference here if some other process is waiting for the launcher to be initialized with WaitForInputIdle,
	// but using it as we're not processing all messages in the main loop.

	// See "GetMessage and PeekMessage Internals" by Bob Gunderson.
	/*
	Applications that do not desire window-handle filtering may pass a NULL value to GetMessageand PeekMessage in the hwnd parameter.
	Similarly, passing NULL values in bothuMsgFilterMin and uMsgFilterMax parameters disables message-range filtering.

	It is important to realize that only mouse and keyboard hardware messages, posted messages, WM_PAINT messages, and timer messages can be filtered.
	Most messages a window procedure receives are sent directly to the window using SendMessage.
	*/

	// mainthread check mainly for the sake of not making GPumpingMessagesOutsideMainLoop TLS, since PeekMessage would be a no-op anyway
	if ( ::SIsMainThread() )
	{
		Red::System::ScopedFlag<Bool> flag( GPumpingMessagesOutsideMainLoop = true, false );
		MSG msg;
		PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE | PM_NOYIELD );
	}
}
#endif // RED_PLATFORM_WINPC

Bool CGame::HideLoadingScreen()
{
	// Hide the loading screen
	LOG_ENGINE( TXT("HideLoadingScreen") );

	m_loadingScreen->StartFadeOut();

//	Hacks::UnmuteAudioAfterLoadingScreen();

	//TBD: Mash button to hide loading screen NOW!!!
	while ( m_loadingScreen->IsShown() )
	{
#if defined( RED_PLATFORM_DURANGO ) || defined ( RED_PLATFORM_ORBIS )
		RED_FATAL_ASSERT( SIsMainThread(), "Incorrect assumptions made about which threads can call this function" );
		if ( GMessagePump )
		{
			GMessagePump->PumpMessages();
		}
#elif defined( RED_PLATFORM_WINPC )
		void Hack_PumpWindowsMessagesForDXGI();
#endif

		Hacks::RenderAudioWhileBlockingGame();

#ifdef RED_PLATFORM_WINPC
		::YieldProcessor();
#endif
	}

	// TBD: Restore music and sounds
	return true;
}

Uint32 GNumFramesToSkip = 0;
Bool GForceTimeFreezeForOneFrame = false;

Bool GHackStorySceneVideoElementInstance;

void CGame::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( GameTick );

	const Float worldTickTimeDelta = IsPaused() || IsActivelyPaused() ? 0.0f : timeDelta;

#ifdef USE_ANSEL

	if ( anselState == EAS_FirstFrameAfterDeactivation )
	{
		GGame->StartFade( true, TXT("AnselFadeInEvent"), anselFadeTime, Color::BLACK, false );
	}	

	// deal with Ansel state
	static Bool paused = false;
	static Uint32 ticksToPause = 0;
	switch ( anselState )
	{
	case EAS_FirstFrameOfActivation:		anselState = EAS_Activating;	break;
	case EAS_FirstFrameAfterDeactivation:	anselState = EAS_Off;			break;
	case EAS_FirstFrameOfDeactivation:		anselState = EAS_Deactivating;	break;
	}

	if ( ticksToPause > 0 )
	{
		--ticksToPause;
		GRender->Flush();
	}
	else if ( isAnselTurningOn )
	{
		ticksToPause = DEFAULT_TICKS_TO_PAUSE;
	}
	else if ( isAnselTurningOff )
	{
		anselState = EAS_FirstFrameOfDeactivation;
		ticksToPause = DEFAULT_TICKS_TO_PAUSE;
	}

	if ( ticksToPause == 0 )
	{
		if ( isAnselTurningOn )
		{
			isAnselTurningOn = false;
			isAnselSessionActive = true;
			anselState = EAS_Active;
		}
		else if ( isAnselTurningOff )
		{
			isAnselTurningOff = false;
			isAnselSessionActive = false;
			anselState = EAS_FirstFrameAfterDeactivation;
		}
	}
#endif // USE_ANSEL

	m_blackscreenWasSetPrevFrame = HasBlackscreenRequested();

	// Previous frame was not rederer - recover it by rendering it twice
	// NOTE: we should use timeDelta of 0.0 but we can't
	if ( GForceTimeFreezeForOneFrame )
	{
		GForceTimeFreezeForOneFrame = false;
		timeDelta = 0.0002f;
	}

	// Update script information with tick delta
	m_timerScriptKeyword->SetTimer( worldTickTimeDelta, GEngine->GetLastTimeDelta() );

	// Update free camera
	m_freeCamera->Tick( timeDelta );

	// Calculate engine time 
	m_engineTime += worldTickTimeDelta;
	m_cleanEngineTime += worldTickTimeDelta;
	
	// End current game
	if( TryEndGame() )
	{
		return;
	}
	else
	{
		// Check for exit due to sign out
		if ( m_isActive && !m_isInStartGame && ( m_requestEndDueToSignout ) )
		{
			SGameSessionManager::GetInstance().EndSession();
			m_requestEndDueToSignout = false;
			m_requestEnd = false;
			
			return;
		}
	}

	// Process the world partition changes
	{
		PC_SCOPE_PIX( ProcessScheduledLayerVisibilityChanges );
		ProcessScheduledLayerVisibilityChanges();
	}

	// Restore after pause

#ifndef RED_FINAL_BUILD
#ifdef RED_PLATFORM_CONSOLE // this trick is only needed on consoles and it may cause problems with editor
	if ( RIM_KEY_JUST_PRESSED( IK_Escape ) && IsPaused( TXT("CGame") ) )
	{
		m_requestUnstop = true;
	}
#endif
#endif
	
	if ( m_requestUnstop )
	{
		Unstop();
	}

	// Update inputs
	{
		PC_SCOPE_PIX( UpdateInputManager );
		m_inputManager->UpdateInput( timeDelta );
	}

	// Update time manager
	{
		PC_SCOPE_PIX( UpdateTimeManager );
		m_timeManager->Tick( timeDelta );
	}

	// Step Visual Debugger
#ifndef PHYSICS_RELEASE
	{
		PC_SCOPE_PIX( PhysicsDebuggerTick );
		GPhysicsDebugger->Tick();
	}
#endif

#ifndef RED_FINAL_BUILD
	/*
	if ( RIM_KEY_JUST_PRESSED( IK_F2 ) 
#ifdef USE_ANSEL
		&& !isAnselSessionActive 
#endif // USE_ANSEL
		)
	{
		SScreenshotParameters params;
		Uint32 saveFlags = SCF_SaveToDisk;
		params.m_saveFormat = SF_BMP;
		params.m_superSamplingSize = 4;

		// modify FOV
		if ( m_viewport && m_viewport->GetViewportHook() )
		{
			CRenderCamera camera;
			m_viewport->GetViewportHook()->OnViewportCalculateCamera( m_viewport.Get(), camera );
			params.m_fov = camera.GetFOV();
		}

		Uint32 renderFlags = SRF_PlainScreenshot;
		if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
		{
			renderFlags |= SRF_Debug;
		}

		struct ScreenshotCallback
		{
			void operator() ( const String& path, const void* buffer, Bool status, const String& msg )
			{
				// log the results
			}

			Bool operator== ( const ScreenshotCallback& other ) const { return true; }
		};

		ScreenshotCallback callbackObject;
		Functor4< void, const String&, const void*, Bool, const String& > callback( callbackObject );

		SScreenshotSystem::GetInstance().RequestScreenshot( params, saveFlags, renderFlags, callback );
	}
	*/
#endif

	// Tick active world
	if ( m_activeWorld && m_viewport && !GHackStorySceneVideoElementInstance )
	{
		PC_SCOPE_PIX( TickBlock );

		UpdateGameTime( timeDelta );

		if ( CCameraDirector* camDirector = m_activeWorld->GetCameraDirector() )
		{
			camDirector->MarkCachedRenderCameraInvalid();
		}

		// Prepare tick info
		CWorldTickInfo tickInfo( m_activeWorld.Get(), worldTickTimeDelta );
		tickInfo.m_updatePhysics = !IsPaused();
		tickInfo.m_fromTickThread = GGame->GetGameplayConfig().m_useMultiTick;


		// If we rendered the previous frame, clear out any camera invalidation. If we didn't render, then we want
		// the invalidation to carry through until we do.
		if ( m_didRenderLastFrame )
		{
			m_activeWorld->GetCameraDirector()->ClearLastFrameDataInvalidation();
		}

		// Tick event
		{
			PC_SCOPE_LV0( GameOnTick, PBC_CPU );
			OnTick( timeDelta );
		}

		CRenderFrameInfo frameInfo( m_viewport.Get() );

		if ( GEngine->IsFPSDisplayEnabled() )
		{
			frameInfo.m_enableFPSDisplay = true;
		}

		// Main frameinfo should allow sequential capture
		frameInfo.m_allowSequentialCapture = true;

		// Force free camera
		if ( m_freeCameraActive )
		{
			m_freeCamera->CalculateCamera( frameInfo.m_camera );		
		}

		if ( m_activeWorld )
		{
			frameInfo.SetShadowConfig( m_activeWorld->GetShadowConfig() );
		}

		{
			PC_SCOPE_PIX( SoundSystemListener );

			const Vector* controllerPosition = NULL;
			if ( IsActive() && GetPlayerEntity() != NULL && !IsFreeCameraEnabled() )
			{
				controllerPosition = &GetPlayerEntity()->GetWorldPositionRef();
			}

			// update listener position
			const CRenderCamera& camera = GetCachedFrameInfo().m_camera;
			const Vector cameraPosition = camera.GetPosition();
			GSoundSystem->SetListenerVectorsFromCameraAndCharacter( cameraPosition, camera.GetCameraUp(), camera.GetCameraForward(), controllerPosition );
		}

		// Update camera position and inform trigger system
		// Additionally store camera position info at the active world so it can be queried from there
		const Bool shouldUpdateCamera = IsActive();

		// Tick the world
		m_activeWorld->Tick( tickInfo, &m_cachedFrameInfo, nullptr, shouldUpdateCamera );

		// Temp
		{
			PC_SCOPE_PIX( AnimationManager_Post );
			GAnimationManager->EndUpdate( timeDelta );
		}

		// PreTick sound system
		{
			GSoundSystem->IssuePreTick( timeDelta );
		}

#ifdef USE_ANSEL
		// ANSEL integration
		{
			using namespace ansel;

			static bool isCameraworksInitialized = false;

			auto startAnselSessionCallback = []( ansel::SessionConfiguration& conf, void* userPointer ) -> ansel::StartSessionStatus
			{
				isAnselSessionActive = false;
				anselState = EAS_Off;

				if ( !GGame )
				{
					return ansel::kDisallowed;
				}
				
				// list all the possible reasons why we can't start Ansel mode
				Bool blockAnsel = false;
				if ( GGame->IsInIngameMenu() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsInIngameMenu()") );
					blockAnsel = true;
				}
				if ( GGame->IsInMainMenu() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsInMainMenu()") );
					blockAnsel = true;
				}
				if ( GGame->IsPaused() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsPaused()") );
					blockAnsel = true;
				}
				if ( GGame->CERT_HACK_IsInStartGame() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: CERT_HACK_IsInStartGame()") );
					blockAnsel = true;
				}
				if ( GGame->IsBlackscreen() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsBlackscreen()") );
					blockAnsel = true;
				}
				if ( GGame->IsAnyMenu() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsAnyMenu()") );
					blockAnsel = true;
				}
				if ( GGame->EndGameRequested() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: EndGameRequested()") );
					blockAnsel = true;
				}
				if ( GGame->HasBlackscreenRequested() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: HasBlackscreenRequested()") );
					blockAnsel = true;
				}
				if ( !GGame->IsActive() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: !IsActive()") );
					blockAnsel = true;
				}
				if ( GGame->IsFadeInProgress() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsFadeInProgress()") );
					blockAnsel = true;
				}
				if ( GGame->IsLoadingScreenShown() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsLoadingScreenShown()") );
					blockAnsel = true;
				}
				if ( GGame->IsPlayingCameraCutscene() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsPlayingCameraCutscene()") );
					blockAnsel = true;
				}
				if ( GGame->IsCurrentlyPlayingNonGameplayScene() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsCurrentlyPlayingNonGameplayScene()") );
					blockAnsel = true;
				}
				if ( GRender && GRender->GetRenderThread() && GRender->GetRenderThread()->IsPlayingVideo() )
				{
					LOG_ENGINE( TXT("Ansel cannot start: IsPlayingVideo()") );
					blockAnsel = true;
				}

				if ( blockAnsel )
				{
					return ansel::kDisallowed;
				}

				if ( userPointer )
				{
					cwCachedCamera = static_cast< CGame* >( userPointer )->GetCachedFrameInfo().m_camera;
				}
				else
				{
					cwCachedCamera = static_cast< CGame* >( GGame )->GetCachedFrameInfo().m_camera;
				}
				
				cwCachedLastFreeCameraData.m_position		= cwCachedCamera.GetPosition();
				cwCachedLastFreeCameraData.m_rotation		= cwCachedCamera.GetRotation();
				cwCachedLastFreeCameraData.m_fov			= cwCachedCamera.GetFOV();
				cwCachedLastFreeCameraData.m_engineTime		= GGame->GetEngineTime();
				cwCachedLastFreeCameraData.m_viewToScreen	= cwCachedCamera.GetViewToScreen();

				cwCamera.position.x = cwCachedCamera.GetPosition().X;
				cwCamera.position.y = cwCachedCamera.GetPosition().Y;
				cwCamera.position.z = cwCachedCamera.GetPosition().Z;

				const Vector quatVecTemp = cwCachedCamera.GetRotation().ToQuat();

				cwCamera.rotation.x = quatVecTemp.X;
				cwCamera.rotation.y = quatVecTemp.Y;
				cwCamera.rotation.z = quatVecTemp.Z;
				cwCamera.rotation.w = quatVecTemp.W;

				cwCamera.fov = cwCachedCamera.GetFOV();

				anselLastFov = cwCamera.fov;

				Vector quatVec( cwCamera.rotation.x, cwCamera.rotation.y, cwCamera.rotation.z, cwCamera.rotation.w );
				anselCameraTransform.BuildFromQuaternion( quatVec ).Transpose();
				anselCameraTransform.SetTranslation( cwCamera.position.x, cwCamera.position.y, cwCamera.position.z );

				wasUmbraTurnedOnBeforeAnselToggle = CUmbraScene::IsUsingOcclusionCulling();
				CUmbraScene::UseOcclusionCulling( false );

				//isAnselSessionActive = true;
				isAnselTurningOn = true;
				anselState = EAS_FirstFrameOfActivation;

				return ansel::kAllowed;
			};

			auto stopAnselSessionCallback = [](void* userPointer)
			{
				CUmbraScene::UseOcclusionCulling( wasUmbraTurnedOnBeforeAnselToggle );
				isAnselTurningOff = true;

				GGame->StartFade( false, TXT("AnselFadeOutEvent"), anselFadeTime, Color::BLACK, false );
				//isAnselSessionActive = false;
			};

			auto startCaptureCallback = [](void* userPointer)
			{
				isAnselCaptureActive = true;
			};

			auto stopCaptureCallback = [](void* userPointer)
			{
				isAnselCaptureActive = false;
			};

			if ( !isCameraworksInitialized )
			{
				nv::Vec3 right, up, forward;
				right.x = 1.0f; right.y = 0.0f; right.z = 0.0f;
				up.x = 0.0f; up.y = 0.0f; up.z = 1.0f;
				forward.x = 0.0f; forward.y = 1.0f; forward.z = 0.0f;

				ansel::Configuration cfg;
				cfg.right = right;
				cfg.up = up;
				cfg.forward = forward;
				cfg.translationSpeedInWorldUnitsPerSecond = 5.0f;
				cfg.rotationalSpeedInDegreesPerSecond = 180.0f / 4.0f;
				cfg.captureLatency = 1;
				cfg.captureSettleLatency = 0;
				cfg.metersInWorldUnit = 1.0f;
				cfg.titleName = "The Witcher 3";
				cfg.fovType = ansel::kVerticalFov;
				cfg.startSessionCallback = startAnselSessionCallback;
				cfg.stopSessionCallback = stopAnselSessionCallback;
				cfg.startCaptureCallback = startCaptureCallback;
				cfg.stopCaptureCallback = stopCaptureCallback;

				cfg.isCameraFovSupported = true;
				cfg.isCameraOffcenteredProjectionSupported = true;
				cfg.isCameraRotationSupported = true;
				cfg.isCameraTranslationSupported = true;

				cfg.isFilterOutsideSessionAllowed = false;

				HWND windowHandle = static_cast< CRenderViewport* >( GGame->GetViewport() )->GetWindow()->GetWindowHandle();
				cfg.gameWindowHandle = windowHandle;

				cfg.userPointer = this;
				ansel::setConfiguration( cfg );
				isCameraworksInitialized = true;
			}
		}
#endif // USE_ANSEL

		// Sanity check for a risky changelist
		RED_FATAL_ASSERT( shouldUpdateCamera == IsActive(), "Active state is expected to not change" );

		CRenderFrame* frame = NULL;
		if( !m_viewport->IsMinimized() )
		{
			// PC_SCOPE_PIX( GenerateFrame ); it's inside
			frame = m_activeWorld->GenerateFrame( m_viewport.Get(), frameInfo );

#ifdef USE_ANSEL
			if ( isAnselSessionActive )
			{
				frameInfo.SetSubpixelOffset( cwCamera.screenOriginXOffset, -cwCamera.screenOriginYOffset, 2, 2 );
			}
#endif // USE_ANSEL

			m_cachedFrameInfo = frameInfo;
		}

		if ( frame && m_freeCamera && m_freeCameraActive )
		{
			m_freeCamera->ConfigureFrameInfo( frame->GetFrameInfo() );
		}

#ifndef NO_EDITOR_FRAGMENTS
		if ( frame )
		{
			PC_SCOPE( GenerateDebugFragments );
			GEngine->GenerateDebugFragments( frame );
		}
#endif

		// Create fence
		if ( !m_renderFence )
		{
			m_renderFence = GRender->CreateFence();
		}
		else
		{
			PC_SCOPE_LV0( RenderFence, PBC_CPU );

			// Measure the time needed to render scene
			RED_PROFILING_TIMER( renderFenceTimer );

			// Flush previous fence
			m_renderFence->FlushFence();

#ifndef NO_DEBUG_PAGES
			// Get the results
			RED_PROFILING_TIMER_GET_DELTA( GLastRenderFenceTime, (Float)renderFenceTimer );
#endif
		}

#ifdef USE_ANSEL
		{
			// Helper utilities
			auto VectorToNvVec3 = []( const Vector& in )
			{
				nv::Vec3 vec;
				vec.x = in.X;	vec.y = in.Y;	vec.z = in.Z;
				return vec;
			};

			auto NvVec3ToVector = []( const nv::Vec3& in )
			{
				Vector vec;
				vec.X = in.x;	vec.Y = in.y;	vec.Z = in.z;	vec.W = 1.0f;
				return vec;
			};

			// The actual stuff
			if ( isAnselSessionActive )
			{
				const CRenderCamera& camera = GetCachedFrameInfo().m_camera;
				const Float aspectRatio = Float( GetViewport()->GetWidth() ) / Float( GetViewport()->GetHeight() );
				cwCamera.position = VectorToNvVec3( camera.GetPosition() );
				RedEulerAngles angles( camera.GetRotation().Roll, camera.GetRotation().Pitch, camera.GetRotation().Yaw );
				RedQuaternion quat = angles.ToQuaternion();
				nv::Quat q;
				q.x = quat.Quat.X;
				q.y = quat.Quat.Y;
				q.z = quat.Quat.Z;
				q.w = quat.Quat.W;
				cwCamera.rotation = q;
				cwCamera.fov = camera.GetFOV();

				ansel::Camera cachedCamera;
				cachedCamera = cwCamera;
				ansel::updateCamera( cwCamera );

				// enable collision with geometry to avoid entering empty buildings
				CPhysicsWorld* physicsWorld = nullptr;
				if ( m_activeWorld->GetPhysicsWorld( physicsWorld ) )
				{
					CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) );
					CPhysicsEngine::CollisionMask exclude = 0;
					SPhysicsOverlapInfo hitContactInfo;
					const Float cameraCollisionSphereRadius = 0.3f;
					if ( physicsWorld->SphereOverlapWithAnyResult( NvVec3ToVector( cwCamera.position ), cameraCollisionSphereRadius, include, exclude, hitContactInfo ) == TRV_Hit )
					{
						cwCamera.position = cachedCamera.position;
					}
				}

#ifdef USE_ANSEL_CAMERA_LIMIT
				if ( GetPlayerEntity() )
				{
					const Vector& playerPos = GetPlayerEntity()->GetLocalToWorld().GetTranslationRef();
					const Float distance = playerPos.DistanceTo( NvVec3ToVector( cwCamera.position ) );
					const Bool isWyzima = m_activeWorld && m_activeWorld->GetDepotPath().ContainsSubstring( TXT("wyzima_castle") );
					const Float distanceThreshold = isWyzima ? ANSEL_CAMERA_WYZIMA_DISTANCE_THRESHOLD : ANSEL_CAMERA_DEFAULT_DISTANCE_THRESHOLD;
					const Bool blockCameraMovement = distance >= distanceThreshold;

					// check if camera is not too far from the player
					if ( blockCameraMovement )
					{
						// camera too far, block it
						cwCamera.position = cachedCamera.position;
					}
					else
					{
						// always test for wyzima
						if ( isWyzima && blockCameraMovement )
						{
							// camera too far, block it
							cwCamera.position = cachedCamera.position;
						}
					}
				}
#endif // USE_ANSEL_CAMERA_LIMIT

				Vector quatVec( cwCamera.rotation.x, cwCamera.rotation.y, cwCamera.rotation.z, cwCamera.rotation.w );
				anselCameraTransform.BuildFromQuaternion( quatVec ).Transpose();
				anselCameraTransform.SetTranslation( cwCamera.position.x, cwCamera.position.y, cwCamera.position.z );

				frameInfo.SetSubpixelOffset( cwCamera.screenOriginXOffset, -cwCamera.screenOriginYOffset, 2, 2 );
				
				if ( !paused )
				{
					Pause( TXT( "Ansel" ) );
					ToggleHud();
					paused = true;
				}
			}
			else if ( paused )
			{
				Unpause( TXT( "Ansel" ) );
				ToggleHud();
				paused = false;
			}
		}
#endif // USE_ANSEL

		// Capture for rendering after flushing the render fence to ensure Scaleform snapshot sync
		if ( m_flashPlayer )
		{
			m_flashPlayer->Capture();
		}

		////// 
		// Streaming prefetch - an unholy hack to get this shit working...
		// Idea:
		//  - we need access to proper camera position
		//  - we need to be outside rendering (after fence)
		//  - we need to be able to CANCEL the frame rendering (if there's crap there's no point of rendering it)
		/////

		Bool allowedToRenderFrame = true;
		{
			Vector referencePosition; 
			if ( frame 
#ifdef USE_ANSEL
				&& isAnselSessionActive 
#endif // USE_ANSEL
				)
			{
				// use directly the camera used for rendering
				referencePosition = frame->GetFrameInfo().m_occlusionCamera.GetPosition();
			}
			else if ( !UseCameraAsStreamingReference() && GetPlayerEntity() && !Config::cvCameraBasedStreamingOverride.Get() )
			{
				// use player position
				referencePosition = GetPlayerEntity()->GetWorldPosition();
			}
			else if ( frame )
			{
				// use directly the camera used for rendering
				referencePosition = frame->GetFrameInfo().m_occlusionCamera.GetPosition();
			}
			else
			{
				// use some bullshit cached camera
				referencePosition = m_activeWorld->GetCameraPosition();
			}

			UpdatePrefetch( referencePosition, allowedToRenderFrame );

#ifdef USE_ANSEL
			if ( anselState == EAS_FirstFrameOfDeactivation )
			{
				CEntity* player = GetPlayerEntity();

				CTimeCounter timer;
				Uint32 numCollisionTicks = 0;
				while ( !m_activeWorld->EnsureTerrainCollisionGenerated( referencePosition ) )
				{
					++numCollisionTicks;
				}
				LOG_ENGINE( TXT("Ansel, Took %d iterations [%1.3f ms] to ensure terrain collision"), numCollisionTicks, timer.GetTimePeriodMS() );
			}

			if ( isAnselSessionActive && m_streamingLocks.Empty() )
			{
				PC_SCOPE( AnselPhysicsTick );
				const Float miniTick = 0.0f; //otherwise cloth and ropes will still simulate
				
				CPhysicsWorld* physicsWorld = nullptr;
				m_activeWorld->GetPhysicsWorld( physicsWorld );
				physicsWorld->SetReferencePosition( anselCameraTransform.GetTranslation() );
				physicsWorld->MarkSectorAsStuffAdded();
				// Wait for the terrain to appear
				while ( !physicsWorld->IsAllPendingCollisionLoadedAndCreated() )
				{
					// Finish current frame
					physicsWorld->FetchCurrentFrameSimulation( false );
					physicsWorld->CompleteCurrentFrameSimulation();

					// Schedule and execute ONE frame of physics
					physicsWorld->StartNextFrameSimulation( miniTick, 1.0f, true );
					physicsWorld->FetchCurrentFrameSimulation( false );
					physicsWorld->CompleteCurrentFrameSimulation();

					// Update master physics engine
					GPhysicEngine->Update( miniTick );
				}
				allowedToRenderFrame = true;
			}
#endif // USE_ANSEL

			// we did not allow this frame to be rendered - make sure the next time delta will be minimal
			if ( !allowedToRenderFrame )
			{
				GForceTimeFreezeForOneFrame = true;
			}
		}

		if ( frame )
		{
			PC_SCOPE_PIX( WorldRender );

			if ( allowedToRenderFrame )
				m_activeWorld->RenderWorld( 
					frame
#ifndef NO_EDITOR
					, IsForcedPrefetchEnabled()
#endif
				);

			frame->Release();
		}

		m_didRenderLastFrame = frame != nullptr && allowedToRenderFrame;


		// Insert fence
		( new CRenderCommand_Fence( m_renderFence ) )->Commit();
	}
	else
	{
		PC_SCOPE_PIX( EmptyFrame );

		// Tick event
		if ( GHackStorySceneVideoElementInstance )
		{
			HACK_TickStorySceneVideoElementInstance( timeDelta );
		}

#ifndef NO_EDITOR
		// PreTick sound system
		{
			GSoundSystem->IssuePreTick( timeDelta );
		}
#endif // !NO_EDITOR


		// Create fence
		if ( !m_renderFence )
		{
			m_renderFence = GRender->CreateFence();
		}
		else
		{
			PC_SCOPE_PIX( RenderFence );

			// Flush previous fence
			CTimeCounter flushTime;
			m_renderFence->FlushFence();
		}


		{
			PC_SCOPE_PIX( AnimationManagerPreview );
			GAnimationManager->UpateForInactiveWorld( timeDelta );
		}

		// Insert fence
		( new CRenderCommand_Fence( m_renderFence ) )->Commit();

		// Draw empty frame
		CRenderFrameInfo frameInfo( m_viewport.Get() );
		frameInfo.m_clearColor = GIsEditor ? Color::WHITE : Color::BLACK;
		frameInfo.SetShowFlag( SHOW_PostProcess, false );
		CRenderFrame* compiledFrame = GRender->CreateFrame( NULL, frameInfo );
		if ( compiledFrame )
		{
			// Generate editor rendering fragments 
			IViewportHook* hook = m_viewport->GetViewportHook();
			if ( hook ) hook->OnViewportGenerateFragments( m_viewport.Get(), compiledFrame );

			// Engine shit
			GEngine->GenerateDebugFragments( compiledFrame );

			// Submit
			( new CRenderCommand_RenderScene( compiledFrame, NULL ) )->Commit();
			compiledFrame->Release();

			// Flush fence
			IRenderFence* renderFence = GRender->CreateFence();
			( new CRenderCommand_Fence( renderFence ) )->Commit();
			renderFence->FlushFence();
			renderFence->Release();
		}
	}

	// debug server update
	DBGSRV_CALL( Tick() );

	// Update force feedback no matter what
	UpdateForceFeedback( timeDelta );
}

void CGame::UpdateGameTime( Float timeDelta )
{
	if ( GEngine->GetState() == BES_Constrained )
	{
		return;
	}

	if ( IsInIngameMenu() )
	{
		return;
	}

	m_gameTime += timeDelta;
}

void CGame::SetViewport( ViewportHandle viewport )
{
	// Flush pending frame
	if ( m_renderFence )
	{
		m_renderFence->FlushFence();
		m_renderFence->Release();
		m_renderFence = NULL;
	}

	// Set new viewport
	m_viewport = viewport;

	if ( m_viewport )
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!" );
		RED_VERIFY( CScaleformSystem::StaticInstance()->Init( m_viewport.Get() ) );

		if ( m_videoPlayer )
		{
			RED_VERIFY( m_videoPlayer->DelayedInit() );
		}

		if ( m_loadingOverlay )
		{
			RED_VERIFY( m_loadingOverlay->DelayedInit() );
		}
	}
}

#ifndef NO_EDITOR_WORLD_SUPPORT

Bool CGame::CreateWorld( const String& localDepotPath )
{
	ASSERT( GIsEditor );

	// Unload current active world
	UnloadWorld();

	// Find depot directory
	CFilePath path( localDepotPath );
	CDirectory* worldDir = GDepot->CreatePath( localDepotPath );

	// Create world
	CClass* worldClass = GetGameWorldClass();
	CWorld* newWorld = CreateObject< CWorld >( worldClass );
	if ( !newWorld )
	{
		WARN_ENGINE( TXT("Unable to create world: not able to create world object") );
		return false;
	}

	// Set depot path
	newWorld->m_depotPath = localDepotPath;

#ifndef NO_RESOURCE_IMPORT
	if ( newWorld->SaveAs( worldDir, path.GetFileName() ) )
	{
		// Activate new one, add it to root set so it will hold references to other objects and resources
		m_activeWorld = newWorld;
		m_activeWorld->AddToRootSet();

		WorldInitInfo initInfo;
		initInfo.m_initializeOcclusion = true;
		initInfo.m_initializePhysics = true;
		initInfo.m_initializePathLib = true;
		initInfo.m_previewWorld = false;
		m_activeWorld->Init( initInfo );

		// Emit editor event
		EDITOR_QUEUE_EVENT( CNAME( ActiveWorldChanged ), CreateEventData( m_activeWorld ) );
	}
	else
#endif
	{
		// Show error
		WARN_ENGINE( TXT("Unable to create world: not able to save world index file") );
	}

	// Load data for marker systems
	#ifndef NO_MARKER_SYSTEMS
		GEngine->GetMarkerSystems()->SendRequest( MSRT_LoadData );
	#endif

	if( m_activeWorld )
	{
		// Register camera director in the scripting system
		ASSERT( GScriptingSystem );
		GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_CAMERA, m_activeWorld->GetCameraDirector() );

		// attach debug server
		DBGSRV_CALL( AttachToWorld() );
	}

	// Return success flag :)
	return m_activeWorld.IsValid();
}

Bool CGame::SaveWorld()
{
	ASSERT( GIsEditor );

	if ( m_activeWorld )
	{
		// Show info
		LOG_ENGINE( TXT("Saving world") );

		// Save world object
#ifndef NO_RESOURCE_IMPORT
		if ( m_activeWorld->Save() )
		{
			m_activeWorld->GetFoliageEditionController().Save();

			// Save all layers
			if ( m_activeWorld->SaveLayers() )
			{
				// Saved
				return true;
			}
		}
#endif
	}

	// No shit !
	return false;
}

#endif


void CGame::NonGameplaySceneStarted()
{
	m_nonGameplaySceneActive = true;
}


void CGame::GameplaySceneStarted()
{
}


Bool CGame::LoadWorld( const String& localDepotPath, WorldLoadingContext& context )
{
	// We are loading world right now
	if ( m_isLoadingWorld )
	{
		WARN_ENGINE( TXT("Cannot load world when already loading.") );
		return false;
	}

	// We are loading world
	m_isLoadingWorld = true;

	// Unload current world
	UnloadWorld();
	
	// Load new world
	m_activeWorld = CWorld::LoadWorld( localDepotPath, context );

	// World loaded
	m_isLoadingWorld = false;

	// Inform listeners
	EDITOR_QUEUE_EVENT( CNAME( ActiveWorldChanged ), CreateEventData( m_activeWorld ) );

// Load data for marker systems
#ifndef NO_MARKER_SYSTEMS
	GEngine->GetMarkerSystems()->SendRequest( MSRT_LoadData );
#endif

	if ( m_activeWorld )
	{
		// Register camera director in the scripting system
		ASSERT( GScriptingSystem );
		GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_CAMERA, m_activeWorld->GetCameraDirector() );

		// attach debug server
		DBGSRV_CALL( AttachToWorld() );
	}

	return m_activeWorld.IsValid();
}

Bool CGame::LoadGameWorld( const CGameInfo& info, const String& localDepotPath )
{
	CTimeCounter totalLoading;

	// Load new world
	WorldLoadingContext worldLoadingContext;
	worldLoadingContext.m_dumpStats = true;
	worldLoadingContext.m_useDependencies = true;
	if ( !LoadWorld( localDepotPath, worldLoadingContext ) )
	{
		RED_LOG( Loading, TXT("World root file not loaded. Not loading.") );
		return false;
	}

	// World loaded
	GLoadingProfiler.FinishStage( TXT("LoadingWorld") );

	// Reset initial visibility
	m_activeWorld->GetWorldLayers()->ResetVisiblityFlag( info );

	// Prestream layers
	m_activeWorld->LoadStaticData();

	// Remember crap
	m_worldChanges.ClearPtr();

	// Done
	return true;
}

void CGame::UnloadWorld()
{
	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Reset layer visibility changes
	m_worldChanges.ClearPtr();

	// Unload world
	if ( m_activeWorld )
	{
		// Release marker systems data
		#ifndef NO_MARKER_SYSTEMS
			GEngine->GetMarkerSystems()->SendRequest( MSRT_ReleaseData );
		#endif

		// Flush rendering
		GRender->Flush();
		PUMP_MESSAGES_DURANGO_CERTHACK();

#ifndef PHYSICS_RELEASE
		// Detach physics world
		if( GPhysicsDebugger )
		{
			GPhysicsDebugger->DetachFromWorld();
		}
#endif

		// detach debug server
		DBGSRV_CALL( DetachFromWorld() );

		// Emit editor event
		EDITOR_DISPATCH_EVENT( CNAME( ActiveWorldChanging ), CreateEventData( m_activeWorld.Get() ) );

		// Unload world
		CWorld::UnloadWorld( m_activeWorld.Get() );
		m_activeWorld = NULL;

		// Unregister camera director in the scripting system
		ASSERT( GScriptingSystem );
		GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_CAMERA, NULL );

		// Emit editor event
		EDITOR_QUEUE_EVENT( CNAME( ActiveWorldChanged ), NULL );

		// Reset sound system after unload the world
		GSoundSystem->Reset();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
}

Float CGame::GetTimeScale( Bool forCamera /*= false*/ ) const
{
	// if there is no time scale set, return defualt 1.0f
	if ( m_timeScaleSets.Empty() )
	{
		return 1.0f;
	}

	Int32 lastAddedIndex = -1;

	if ( forCamera )
	{
		// just get the first element of time scale sets (highest priority) and find final time scale
		const TDynArray< STimeScaleSource >& entries = m_timeScaleSets[ 0 ].m_entries;
		for ( Uint32 t = 0; t < entries.Size(); t++ )
		{
			// take into account only sources that affect camera
			if ( entries[ t ].m_affectCamera )
			{
				// look for most recently added source
				if ( lastAddedIndex == -1 || entries[ lastAddedIndex ].m_priorityIndex < entries[ t ].m_priorityIndex )
				{
					lastAddedIndex = t;
				}
			}
		}
		if ( lastAddedIndex != -1 )
		{
			// return most recently added source that affects camera
			return entries[ lastAddedIndex ].m_timeScale;
		}
		// seems like no source affects camera
	}
	else
	{
		// just get the first element of time scale sets (highest priority) and find final time scale
		Uint32 lesserCount = 0,
			   greaterCount = 0;
		Int32 lesserIndex = -1,
			  greaterIndex = -1;
		const TDynArray< STimeScaleSource >& entries = m_timeScaleSets[ 0 ].m_entries;
		for ( Uint32 t = 0; t < entries.Size(); t++ )
		{
			// look for < 1 and > 1 sources count and lowest and biggest among them
			if ( entries[ t ].m_timeScale < 1 )
			{
				lesserCount++;
				if ( lesserIndex == -1 || entries[ lesserIndex ].m_timeScale > entries[ t ].m_timeScale )
				{
					lesserIndex = t;
				}
			}
			else if ( entries[ t ].m_timeScale > 1 )
			{
				greaterCount++;
				if ( greaterIndex == -1 || entries[ greaterIndex ].m_timeScale < entries[ t ].m_timeScale )
				{
					greaterIndex = t;
				}
			}

			// look for most recently added source
			if ( lastAddedIndex == -1 || entries[ lastAddedIndex ].m_priorityIndex < entries[ t ].m_priorityIndex )
			{
				lastAddedIndex = t;
			}
		}
		if ( lesserCount > 0 && greaterCount > 0 )
		{
			// there are both < 1 and > 1 sources, we take the only most recently added
			ASSERT( lastAddedIndex != -1 );
			return entries[ lastAddedIndex ].m_timeScale;
		}
		else if ( lesserCount > 0 )
		{
			// there are only < 1 sources, we take the lowest one
			ASSERT( lesserIndex != -1 );
			return entries[ lesserIndex ].m_timeScale;
		}
		else if ( greaterCount > 0 )
		{
			// there are only > 1 sources, we take the biggest one
			ASSERT( greaterIndex != -1 );
			return entries[ greaterIndex ].m_timeScale;
		}
	}

	// return default value 1.0f if no appropriate time scale was found
	return 1.0f;
}

void CGame::SetTimeScale( Float timeScale, const CName& sourceName, Int32 priority, Bool affectCamera /*= false*/, Bool dontSave /*=false*/ )
{
	RED_LOG( TimeScale, TXT("SetTimeScale %f '%ls' %d %d"), timeScale, sourceName.AsString().AsChar(), priority, affectCamera );

	// find time scale set with desired priority
	for ( Uint32 s = 0; s < m_timeScaleSets.Size(); s++ )
	{
		if ( m_timeScaleSets[ s ].m_priority == priority )
		{
			// find matching source name and update parameters
			TDynArray< STimeScaleSource >& entries = m_timeScaleSets[ s ].m_entries;
			for ( Uint32 t = 0; t < entries.Size(); t++ )
			{
				if ( entries[ t ].m_name == sourceName )
				{
					entries[ t ].m_timeScale       = timeScale;
					entries[ t ].m_affectCamera    = affectCamera;
					entries[ t ].m_dontSave		   = dontSave;
					entries[ t ].m_priorityIndex   = m_timeManager->GenerateNewTimeScalePriorityIndex();
					return;
				}
			}

			// if desired source was not found, add it
			STimeScaleSource entry;
			entry.m_timeScale		= timeScale;
			entry.m_name			= sourceName;
			entry.m_affectCamera	= affectCamera;
			entry.m_dontSave		= dontSave;
			entry.m_priorityIndex	= m_timeManager->GenerateNewTimeScalePriorityIndex();

			entries.PushBack( entry );
			return;
		}
	}

	// no time scale set with desired priority were found, add a new one
	STimeScaleSource entry;
	entry.m_timeScale		= timeScale;
	entry.m_name			= sourceName;
	entry.m_affectCamera	= affectCamera;
	entry.m_dontSave		= dontSave;
	entry.m_priorityIndex	= m_timeManager->GenerateNewTimeScalePriorityIndex();

	TDynArray< STimeScaleSource > entries;
	entries.PushBack( entry );

	STimeScaleSourceSet set;
	set.m_priority = priority;
	set.m_entries  = entries;

	m_timeScaleSets.Insert( set );
}

void CGame::RemoveTimeScale( const CName& sourceName )
{
	RED_LOG( TimeScale, TXT("RemoveTimeScale '%ls'"), sourceName.AsString().AsChar() );

	// go through all time scale sets
	for ( Uint32 s = 0; s < m_timeScaleSets.Size(); )
	{
		// go through all source names
		TDynArray< STimeScaleSource >& entries = m_timeScaleSets[ s ].m_entries;
		for ( Uint32 t = 0; t < entries.Size(); )
		{
			if ( entries[ t ].m_name == sourceName )
			{
				// erase if time scale with esired priority and source was found
				entries.Erase( entries.Begin() + t );
			}
			else
			{
				t++;
			}
		}
		// if there are no more entries in time scale set with current priority, erase entry
		if ( entries.Size() == 0 )
		{
			m_timeScaleSets.Erase( m_timeScaleSets.Begin() + s );
		}
		else
		{
			s++;
		}
	}
}

void CGame::RemoveAllTimeScales()
{
	RED_LOG( TimeScale, TXT("RemoveAllTimeScales") );

	m_timeScaleSets.Clear();
}

void CGame::SetOrRemoveTimeScale( Float timeScale, const CName& sourceName, Int32 priority, Bool affectCamera /*= false*/ )
{
	// if time scale is close to 1.0, delete it, otherwise set
	if ( MAbs( timeScale - 1 ) < 0.01f )
	{
		RemoveTimeScale( sourceName );
	}
	else
	{
		SetTimeScale( timeScale, sourceName, priority, affectCamera );
	}
}

void CGame::LogTimeScales()
{
	RED_LOG( TimeScale, TXT("--------- TIME SCALES START ---------") );
	if ( m_timeScaleSets.Empty() )
	{
		RED_LOG( TimeScale, TXT("No timescales") );
	}
	else
	{
		for ( Uint32 s = 0; s < m_timeScaleSets.Size(); s++ )
		{
			RED_LOG( TimeScale, TXT("PRIORITY %d"), m_timeScaleSets[ s ].m_priority );
			TDynArray< STimeScaleSource >& entries = m_timeScaleSets[ s ].m_entries;
			for ( Uint32 t = 0; t < entries.Size(); t++ )
			{
				RED_LOG( TimeScale, TXT("         %4.2f '%ls' %d %u %d"), entries[ t ].m_timeScale, entries[ t ].m_name.AsString().AsChar(), entries[ t ].m_affectCamera, entries[ t ].m_priorityIndex, entries[ t ].m_dontSave );
			}
		}
		RED_LOG( TimeScale, TXT("GetTimeScale( false ) = %f"), GetTimeScale( false ) );
		RED_LOG( TimeScale, TXT("GetTimeScale( true )  = %f"), GetTimeScale( true ) );
	}
	RED_LOG( TimeScale, TXT("---------- TIME SCALES END ----------") );
}

void CGame::TogglePad( Bool isUsingPad )
{
	// Only if changes
	if ( isUsingPad != m_isUsingPad )
	{
		if( isUsingPad )
		{
			// Info
			LOG_ENGINE( TXT("X360 Pad is Enabled") );
		}
		else
		{
			// Info
			LOG_ENGINE( TXT("X360 Pad is Disabled") );
		}
	}

	// Set flag
	m_isUsingPad = isUsingPad;

}

Bool CGame::IsPadConnected() const
{
	return SRawInputManager::GetInstance().IsPadAvailable();
}

void CGame::RequestGameEnd()
{
	// Request end of the game
	m_requestEnd = true;
	m_requestEndReason = ENDGAMEREASON_Default;

	SetBlackscreen( false, TXT( "RequestGameEnd" ), Color::BLACK );
}

void CGame::ResetPauseCounter()
{
	m_pauseCount.Clear();
}

void CGame::Pause( const String& reason )
{
	if( m_pauseCount.PushBackUnique( reason ) )
	{
		if( m_pauseCount.Size() == 1 )
		{
			PauseCutscenes();

			EDITOR_DISPATCH_EVENT( CNAME( GamePause ), NULL );
		}
	}
}

void CGame::Unpause( const String& reason )
{
	if( m_pauseCount.Remove( reason ) )
	{
		if( m_pauseCount.Empty() )
		{
			UnpauseCutscenes();

			EDITOR_DISPATCH_EVENT( CNAME( GamePause ), NULL );
		}
	}
}

void CGame::UnpauseAll()
{
	for(Uint32 i=0; i<m_pauseCount.Size(); ++i)
	{
		Unpause(m_pauseCount[i]);
	}
}

void CGame::SetActivePause( Bool flag )
{
	m_activePause.Update( flag );

	if ( flag )
	{
		PauseCutscenes();
	}
	else
	{
		UnpauseCutscenes();
	}
}

// Buffered Event Structure
struct BufferedEvent
{
	EInputKey	 m_key;
	EInputAction m_action;
	Float		 m_data;	

	BufferedEvent() {};
	BufferedEvent(  EInputKey key, EInputAction action, Float data  ): m_key(key), m_action(action), m_data( data ) {};
};

void CGame::Unstop()
{
	if( !m_isActive || !m_stopped || !m_requestUnstop )
		return;

	NotifyGameInputModeEnabled();
	m_inputManager->SoftReset();

	if( IsPaused() )
	{
		Unpause( TXT( "CGame" ) );
	}

	m_stopped = false;
	m_requestUnstop = false;
}

void CGame::Stop()
{
	// Don't stop if the loading screen is up, since the editor can call GGame->Stop() when the frame loses
	// focus and then we don't have input to Unstop() it because the viewport hook has been changed.
	if ( !m_isActive || m_loadingScreen->IsShown() || ( m_stopped && !m_requestUnstop ) )
	{
		return;
	}

	if( !IsPaused() && !DoNotPauseWhileStopped() )
	{
		Pause( TXT( "CGame" ) );
	}


	SRawInputManager::GetInstance().RequestReset();

	// Release full input capture
	NotifyGameInputModeDisabled();
	m_inputManager->SoftReset();

	// Update state
	m_requestUnstop = false;
	m_stopped = true;
}

Bool CGame::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Bool inputProcessed = false;

	// Ignore alt+tab
	if ( key == IK_Tab && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		inputProcessed = true;
	}

	// Request unpause when game is paused and something is pressed
	if ( !inputProcessed && m_stopped && !m_requestUnstop && action == IACT_Press )
	{
		m_requestUnstop = true;
		inputProcessed = true;
	}

#ifndef RED_FINAL_BUILD
	// Enable screenshot capture mode
	if ( !inputProcessed && m_shiftState == IACT_Press && action == IACT_Press )
	{
		if ( key == IK_F2 
#ifdef USE_ANSEL
			&& isAnselSessionActive 
#endif // USE_ANSEL
			)

		{
			Bool movieUbersampling = Config::cvMovieUbersampling.Get();
			if ( RIM_IS_KEY_DOWN( IK_Alt ) )
			{
				ToggleContignous( FCSF_PNG, movieUbersampling );
				inputProcessed = true;
			}
			else if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
			{
				ToggleContignous( FCSF_DDS, movieUbersampling );
				inputProcessed = true;
			}
			else
			{
				ToggleContignous( FCSF_BMP, movieUbersampling );
				inputProcessed = true;
			}
		}
	}
#endif

	// Save shift state
	if( !inputProcessed && ( key == IK_LShift || key == IK_RShift ) )
	{
		m_shiftState = action;
	}

	// Debug console
#ifdef RED_FINAL_BUILD
	if( Config::cvDBGConsoleOn.Get() )
#endif
	{
		if( !inputProcessed && GDebugConsole && GDebugConsole->OnViewportInput( view, key, action, data ) )
		{
			inputProcessed = true;
		}
	}

#ifndef RED_FINAL_BUILD
	// Game pause
	if ( !inputProcessed && key == IK_Pause && action == IACT_Release )
	{
		if( m_shiftState == IACT_Press )
		{
			SetActivePause( !IsActivelyPaused() );
		}
		else
		{
			Stop();
			inputProcessed = true;
		}
	}
#endif

	//Frame-by-frame advancement: when the game is actively paused, ENTER will advance gametime by 1 frame (1/30 secs)
#ifndef RED_FINAL_BUILD
	if ( !inputProcessed && key == IK_Enter && action == IACT_Press && IsActivelyPaused() )
	{
		m_frameAdvance = true;
	}
#endif

#ifndef RED_FINAL_BUILD
	// Quick feature: toggle HUD
	if ( !inputProcessed && key == IK_U && action == IACT_Press )
	{
		ToggleHud();
		inputProcessed = true;
	}
#endif

#ifndef NO_FREE_CAMERA
	// Process free camera
	if( !inputProcessed && !IsPaused() )
	{
		if ( ProcessFreeCameraInput( key, action, data ) )
		{
			inputProcessed = true;
		}
	}
#endif

	if ( inputProcessed )
	{
		m_inputManager->SoftReset(); // Because the input it got shouldn't be used
		m_inputManager->SuppressSendingEvents( true );
	}

	// Not handled
	return inputProcessed;
}

void CGame::OnViewportActivated( IViewport* view )
{
	if ( view == GetViewport() && !GIsEditor )
	{
		GEngine->OnExitConstrainedRunningMode();

		// TEMP - Super paranoid ifdef.
#ifdef RED_PLATFORM_WINPC
		GEngine->OnEnterNormalRunningMode();
#endif
	}
}

void CGame::OnViewportDeactivated( IViewport* view )
{
	if ( view == GetViewport() && !GIsEditor && !IsFreeCameraEnabled() )
	{
		GEngine->OnEnterConstrainedRunningMode();
	}
}

void CGame::RegisterCutscene( CCutsceneInstance* instance )
{
	TDynArray< CEntity* > actors;
	instance->GetActors( actors );

	if ( instance->HasCamera() )
	{
		for ( Uint32 i=0; i<m_csInstances.Size(); ++i )
		{
			CCutsceneInstance* cs = m_csInstances[i];

			if ( cs->HasCamera() )
			{
				CS_WARN( TXT("Cutscene: Two cutscenes want to have active camera - prev '%ls', new '%ls'."), m_csInstances[i]->GetName().AsChar(), instance->GetName().AsChar() );
			}
		}
	}

	for ( Uint32 i=0; i<m_csInstances.Size(); ++i )
	{
		CCutsceneInstance* cs = m_csInstances[i];

		if ( !cs->CheckActors( actors ) )
		{
			CS_ERROR( TXT("Cutscene: Cutscene '%ls' steals actors from cutscene '%ls'"), instance->GetName().AsChar(), cs->GetName().AsChar() );
			RED_HALT( "Cutscene actor is used in two cutscene. See log file!!!" );
		}
	}

	m_csInstances.PushBack( instance );
}

void CGame::UnregisterCutscene( CCutsceneInstance* instance )
{
	m_csInstances.Remove( instance );	
}

CCutsceneInstance* CGame::GetCutscene( const String& name ) const
{
	for ( Uint32 i=0; i<m_csInstances.Size(); ++i )
	{
		if ( m_csInstances[i]->GetName() == name )
		{
			return m_csInstances[i];
		}
	}

	return NULL;
}

void CGame::UnregisterAllCutscenes()
{
	m_csInstances.Clear();
}

void CGame::PauseCutscenes()
{
	for ( Uint32 i=0; i<m_csInstances.Size(); ++i )
	{
		m_csInstances[i]->Pause();
	}
}

void CGame::UnpauseCutscenes()
{
	for ( Uint32 i=0; i<m_csInstances.Size(); ++i )
	{
		m_csInstances[i]->Unpause();
	}
}


Bool CGame::IsPlayingCameraCutscene() const
{
	for ( Uint32 i=0; i<m_csInstances.Size(); ++i )
	{
		if ( m_csInstances[i]->HasActiveCamera() )
		{
			return true;
		}
	}

	return false;
}

void CGame::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) 
{
	const CRenderFrameInfo& frameInfo = frame->GetFrameInfo();

	// Allow camera to generate some fragments
	if( frameInfo.IsShowFlagOn( SHOW_Camera ) )
	{
		if( m_camera )
		{
			m_camera->GenerateDebugFragments( frame );
		}
	}

	if( frameInfo.IsShowFlagOn( SHOW_OnScreenMessages ) )
	{
		if( ! m_blackscreenOnReason.Empty() )
		{
			frame->AddDebugScreenText( 10, 10, String::Printf( TXT("Blackscreen reason: %ls"), m_blackscreenOnReason.AsChar() ), Color::BLUE );
		}

		Int32 x = 10;
		Int32 y = 300;

		if( IsPaused() )
		{
			frame->AddDebugScreenText( x, y, TXT( "Game Paused By:" ), Color::WHITE, NULL, true, Color::BLACK );

			for( Uint32 i = 0; i < m_pauseCount.Size(); ++i )
			{
				y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT( "%2u: %ls" ), i, m_pauseCount[ i ].AsChar() ), Color::WHITE, NULL, true, Color::BLACK );
			}
		}

		// Show streaming locks
		if ( !m_streamingLocks.Empty() )
		{
			y += 10;

			frame->AddDebugScreenText( x, y, TXT( "STREAMING LOCKS:" ), Color::RED, NULL, true, Color::BLACK );
			y += 20;

			for ( const auto& it : m_streamingLocks )
			{
				frame->AddDebugScreenText( x, y, it.m_name, Color::RED, NULL, true, Color::BLACK );
				y += 20;
			}
		}

		// Show streaming camera overrides
		if ( !m_streamingCameraOverrides.Empty() )
		{
			y += 10;

			frame->AddDebugScreenText( x, y, TXT( "STREAMING CAMERA MODE:" ), Color::YELLOW, NULL, true, Color::BLACK );
			y += 20;

			for ( const auto& it : m_streamingCameraOverrides )
			{
				const String txt = String::Printf( TXT("(%1.2fH %1.2fS) %ls"), it.m_hardDistance, it.m_softDistance, it.m_name.AsChar() );
				frame->AddDebugScreenText( x, y, txt, Color::YELLOW, NULL, true, Color::BLACK );
				y += 20;
			}
		}
	}

	if ( frameInfo.IsShowFlagOn( SHOW_VisualDebug ) )
	{
		if( m_visualDebug )
		{
			m_visualDebug->Render( frame, Matrix::IDENTITY );
		}
	}
}

void CGame::OnViewportCalculateCamera( IViewport* view, CRenderCamera& camera )
{
	// Force free camera
	if ( m_freeCameraActive )
	{
		m_freeCamera->CalculateCamera( camera );
	}
#ifdef USE_ANSEL
	else if ( isAnselSessionActive && IsPaused() ) // ANSEL Integration
	{
		camera.Set( anselCameraTransform.GetTranslation(),
					anselCameraTransform.ToEulerAnglesFull(),
					cwCamera.fov, 
					cwCachedCamera.GetAspect(), 
					cwCachedCamera.GetNearPlane(), 
					cwCachedCamera.GetFarPlane(),
					cwCachedCamera.GetZoom(),
					cwCachedCamera.IsReversedProjection() );
		// setup off-screen projection
		camera.SetSubpixelOffset( cwCamera.screenOriginXOffset, -cwCamera.screenOriginYOffset, 2, 2 );
		camera.SetLastFrameData( cwCachedLastFreeCameraData );
		cwCachedLastFreeCameraData.Init( GGame->GetEngineTime(), camera);
		m_activeWorld->GetCameraDirector()->SetCachedRenderCamera( camera );
	}
#endif // USE_ANSEL
	else if ( m_activeWorld )
	{
		m_activeWorld->GetCameraDirector()->OnViewportCalculateCamera( view, camera );
	}
}

CEntity* CGame::CreatePlayer( const CGameInfo& info )
{
	// No player in default game
	return NULL;
}

CEntity* CGame::CreateCamera() const
{
	return NULL;
}

void CGame::GetFreeCameraWorldPosition( Vector* outPosition, EulerAngles* outRotation, Vector* outDir ) const
{
	if ( outPosition )
	{
		*outPosition = m_freeCamera->GetPosition();
	}

	if ( outRotation )
	{
		*outRotation = m_freeCamera->GetRotation();
	}

	if ( outDir )
	{
		*outDir = m_freeCamera->GetDirection();
	}
}

Float CGame::GetFreeCameraFovDistanceMultiplier() const
{
	return m_freeCamera->GetFovDistanceMultiplier();
}

void CGame::SetFreeCameraWorldPosition( const Vector& camWorldPos )
{
	m_freeCamera->MoveTo( camWorldPos, m_freeCamera->GetRotation() );
}

void CGame::SetFreeCameraWorldRotation( const EulerAngles& camWorldRot )
{
	m_freeCamera->MoveTo( m_freeCamera->GetPosition(), camWorldRot );
}

void CGame::EnableFreeCamera( Bool enable )
{
	// Add/Remove the streaming override for camera mode
	if ( m_freeCameraActive && !enable )
	{
		DisableCameraBasedStreaming( TXT("FreeCamera") );
	}
	else if ( !m_freeCameraActive && enable )
	{
		EnableCameraBasedStreaming( TXT("FreeCamera"), 40.0f, 40.0f );
	}

	// Enable the free  camera
	m_freeCameraActive = enable;

	// If enable and we have player entity move the camera to it
	if ( enable )
	{
		m_freeCamera->Reset();
		CEntity* player = GetPlayerEntity();
		if ( player )
		{
			m_freeCamera->MoveTo( player->GetWorldPosition() + Vector( 0.0f, 0.0f, 2.0f ), player->GetWorldRotation() );
		}
	}
}

Bool CGame::ProcessFreeCameraInput( enum EInputKey key, enum EInputAction action, Float data )
{
	// Free camera is a cheat so make sure it's allowed
	if ( Config::cvCheatAllowFreeCamera.Get() )
	{
		// Toggle free camera
		if ( key == IK_F1 && action == IACT_Press )
		{	
			EnableFreeCamera( !IsFreeCameraEnabled() );
			return true;
		}

		if ( key == IK_Pad_LeftShoulder )
		{
			if( RIM_IS_KEY_DOWN( IK_Pad_RightShoulder ) && RIM_GET_AXIS_VALUE( IK_Pad_LeftTrigger ) && RIM_GET_AXIS_VALUE( IK_Pad_RightTrigger ) )
			{
				EnableFreeCamera( true );
				return true;
			}
		}

		// Process camera input
		if ( IsFreeCameraEnabled() )
		{
			return m_freeCamera->ProcessInput( key, action, data );
		}
	}

	// Input not processed
	return false;
}

Bool CGame::DoesFreeCameraProcessInputs() const
{
	return m_freeCamera->CanProcessedInputs();
}

void CGame::DestroyCamera( CEntity* camera )
{
	ASSERT( m_activeWorld && m_activeWorld->GetDynamicLayer() );

	if ( camera )
	{
		camera->Destroy();
		camera = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Fades
//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_BLACKSCREEN
#pragma optimize("",off)
TDynArray< String > g_fadeHistory;
#endif

// Override the fade to white
Uint32 GW3Hack_WhiteFadeOverride;

// Check after "reason end" after setting actualColor so we still have a white-fadein
#define UPDATE_WHITESCREEN_FADE_HACK()\
	Color actualColor = color;\
	do{\
		if ( GW3Hack_WhiteFadeOverride > 0 )\
			actualColor = Color::WHITE;\
		if ( GW3Hack_WhiteFadeOverride == 1 )\
			GW3Hack_WhiteFadeOverride = 0;\
	}while(0)

void CGame::UpdateBlackscreenColor( const Color& color )
{
	m_blackscreenColor = color;

	// For now, let this get updated from storySceneVideo.cpp and we ignore the blackscreen colors set
	//	( new CRenderCommand_W3HackSetVideoClearRGB( color ) )->Commit();
}

void CGame::StartFade( Bool fadeIn, const String& reason, Float fadeDuration /*=1.f*/, const Color& color /*=Color::BLACK*/, const Bool informSoundSystem /*=true*/ )
{
	if ( m_blackscreenLockOnReason.Empty() )
	{
		if ( fadeIn == true )
		{
			#ifdef DEBUG_BLACKSCREEN
			const String tmpStr = String::Printf( TXT("StartFade_fadeIn = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
			g_fadeHistory.PushBack( tmpStr );
			const Bool isAleadySet = !m_blackscreenOffReason.Empty();
			#endif

			m_blackscreenOnReason = String::EMPTY;
			m_blackscreenOffReason = reason;

			SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLockForBlackscreens );
			m_saveLockForBlackscreens = CGameSessionManager::GAMESAVELOCK_INVALID;

			( new CRenderCommand_ScreenFadeIn( fadeDuration ) )->Commit();

			if ( informSoundSystem )
			{
				GSoundSystem->OnBlackscreenEnd();
			}

			// Invalidate last frame's camera when fade in starts. We don't really want things to dissolve in immediately after,
			// they should be properly visible from the start.
			if ( m_activeWorld && m_activeWorld->GetCameraDirector() )
			{
				m_activeWorld->GetCameraDirector()->InvalidateLastFrameCamera();
			}

			#ifdef DEBUG_BLACKSCREEN
			if ( isAleadySet )
			{
				m_blackscreenOnReason = String::EMPTY;
			}
			#endif

			RED_LOG_SPAM( RED_LOG_CHANNEL( Blackscreens ), TXT("StartFade_fadeIn = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
		}
		else
		{
			UPDATE_WHITESCREEN_FADE_HACK();

			UpdateBlackscreenColor( actualColor );

			#ifdef DEBUG_BLACKSCREEN
			const String tmpStr = String::Printf( TXT("StartFade_fadeOut = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
			g_fadeHistory.PushBack( tmpStr );
			const Bool isAleadySet = !m_blackscreenOnReason.Empty();
			#endif

			/* 
			We should not check if the loading screen is shown. If the loading screen is shown, the blackscreen (or fade-out)
			was not triggered, therefore the fade-in was triggered with no blackscreen on. That is why there were "shots" of gameplay
			at the begining of scenes - fade-in was triggered without blackscreen on, so it had no effect.
			*/

			m_blackscreenOnReason = reason;
			m_blackscreenOffReason = String::EMPTY;

			SGameSessionManager::GetInstance().CreateNoSaveLock( reason, m_saveLockForBlackscreens, false, true );

			( new CRenderCommand_ScreenFadeOut( actualColor, fadeDuration ) )->Commit();

			if ( informSoundSystem )
			{
				GSoundSystem->OnBlackscreenStart();
			}

			#ifdef DEBUG_BLACKSCREEN
			if ( isAleadySet )
			{
				m_blackscreenOffReason = String::EMPTY;
			}
			#endif

			RED_LOG_SPAM( RED_LOG_CHANNEL( Blackscreens ), TXT("StartFade_fadeOut = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
		}
	}
	else
	{
		const String fadeDesc = fadeIn ? TXT("FadeIn") : TXT("FadeOut");
		LOG_ENGINE( TXT("Game StartFade func '%ls' is blocked because of '%ls' lock"), fadeDesc.AsChar(), m_blackscreenLockOnReason.AsChar() );
	}
}

void CGame::SetFade( Bool fadeIn, const String& reason, Float progress, const Color& color )
{
	if ( m_blackscreenLockOnReason.Empty() )
	{
		if ( fadeIn == true )
		{
			#ifdef DEBUG_BLACKSCREEN
			const String tmpStr = String::Printf( TXT("SetFade_fadeIn = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
			g_fadeHistory.PushBack( tmpStr );
			#endif

			m_blackscreenOnReason = String::EMPTY;
			m_blackscreenOffReason = reason;

			SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLockForBlackscreens );
			m_saveLockForBlackscreens = CGameSessionManager::GAMESAVELOCK_INVALID;

			( new CRenderCommand_SetScreenFade( fadeIn, progress, color ) )->Commit();

			GSoundSystem->OnBlackscreenEnd();

			RED_LOG_SPAM( RED_LOG_CHANNEL( Blackscreens ), TXT("SetFade_fadeIn = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
		}
		else
		{
			UPDATE_WHITESCREEN_FADE_HACK();

			UpdateBlackscreenColor( actualColor );

			#ifdef DEBUG_BLACKSCREEN
			const String tmpStr = String::Printf( TXT("SetFade_fadeOut = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
			g_fadeHistory.PushBack( tmpStr );
			#endif

			/* 
			We should not check if the loading screen is shown. If the loading screen is shown, the blackscreen (or fade-out)
			was not triggered, therefore the fade-in was triggered with no blackscreen on. That is why there were "shots" of gameplay
			at the begining of scenes - fade-in was triggered without blackscreen on, so it had no effect.
			*/

			m_blackscreenOnReason = reason;
			m_blackscreenOffReason = String::EMPTY;

			SGameSessionManager::GetInstance().CreateNoSaveLock( reason, m_saveLockForBlackscreens, false, true );

			( new CRenderCommand_SetScreenFade( fadeIn, progress, actualColor ) )->Commit();

			GSoundSystem->OnBlackscreenStart();

			RED_LOG_SPAM( RED_LOG_CHANNEL( Blackscreens ), TXT("SetFade_fadeOut = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
		}
	}
	else
	{
		const String fadeDesc = fadeIn ? TXT("FadeIn") : TXT("FadeOut");
		LOG_ENGINE( TXT("Game SetFade func '%ls' is blocked because of '%ls' lock"), fadeDesc.AsChar(), m_blackscreenLockOnReason.AsChar() );
	}
}

void CGame::SetBlackscreen( Bool enable, const String& reason, const Color& color )
{
	if ( m_blackscreenLockOnReason.Empty() )
	{
		if ( enable == true )
		{
			UPDATE_WHITESCREEN_FADE_HACK();

			UpdateBlackscreenColor( actualColor );

			#ifdef DEBUG_BLACKSCREEN
			const String tmpStr = String::Printf( TXT("SetBlackscreen_on = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
			g_fadeHistory.PushBack( tmpStr );
			#endif

			m_blackscreenOnReason = reason;
			m_blackscreenOffReason = String::EMPTY;

			SGameSessionManager::GetInstance().CreateNoSaveLock( reason, m_saveLockForBlackscreens, false, true );

			( new CRenderCommand_ScreenFadeOut( actualColor, 0.0f ) )->Commit();

			GSoundSystem->OnBlackscreenStart();

			RED_LOG_SPAM( RED_LOG_CHANNEL( Blackscreens ), TXT("SetBlackscreen_on = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
		}
		else
		{
			#ifdef DEBUG_BLACKSCREEN
			const String tmpStr = String::Printf( TXT("SetBlackscreen_off = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
			g_fadeHistory.PushBack( tmpStr );
			#endif

			m_blackscreenOnReason = String::EMPTY;
			m_blackscreenOffReason = reason;

			SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLockForBlackscreens );
			m_saveLockForBlackscreens = CGameSessionManager::GAMESAVELOCK_INVALID;

			( new CRenderCommand_ScreenFadeIn( 0.0f ) )->Commit();

			GSoundSystem->OnBlackscreenEnd();

			// Invalidate last frame's camera after a blackscreen. We don't really want things to dissolve in immediately after,
			// they should be properly visible from the start.
			if ( m_activeWorld && m_activeWorld->GetCameraDirector() )
			{
				m_activeWorld->GetCameraDirector()->InvalidateLastFrameCamera();
			}

			RED_LOG_SPAM( RED_LOG_CHANNEL( Blackscreens ), TXT("SetBlackscreen_off = %d = %ls"), GEngine->GetCurrentEngineTick(), reason.AsChar() );
		}
	}
	else
	{
		const String desc = enable ? TXT("On") : TXT("Off");
		LOG_ENGINE( TXT("Game SetBlackscreen func '%ls' is blocked because of '%ls' lock"), desc.AsChar(), m_blackscreenLockOnReason.AsChar() );
	}
}

#ifdef DEBUG_BLACKSCREEN
#pragma optimize("",on)
#endif

Bool CGame::HasBlackscreenRequested() const
{
	return m_blackscreenOnReason != String::EMPTY;
}

Bool CGame::HasBlackscreenLockRequested() const
{
	return m_blackscreenLockOnReason != String::EMPTY;
}

void CGame::SetFadeLock( const String& reason )
{
	m_blackscreenLockOnReason = reason;
	m_blackscreenLockOffReason = String::EMPTY;
}

void CGame::ResetFadeLock( const String& reason )
{
	m_blackscreenLockOnReason = String::EMPTY;
	m_blackscreenLockOffReason = reason;
}

Bool CGame::IsFadeInProgress() const
{
	return GRender->IsFading();
}

Bool CGame::IsBlackscreen() const
{
	Bool ret = false;

	if (m_loadingScreen)
	{
		ret |= m_loadingScreen->IsShown();
	}

	if (GRender)
	{
		ret |= GRender->IsBlackscreen();
	}

	return ret;
}

void CGame::NotifyDeviceLost()
{

}

void CGame::NotifyDeviceRestore()
{

}

void CGame::ShowMainMenuFirstTime( Bool showStartVideos )
{
}

const String CGame::GetGameResourcePath() const
{
	CGameResource* gameResource = GetGameResource();
	if ( gameResource == NULL )
	{
		return String::EMPTY;
	}
	CDiskFile* gameResourceFile = gameResource->GetFile();
	if ( gameResourceFile == NULL )
	{
		return String::EMPTY;
	}
	return gameResourceFile->GetDepotPath();
}

void CGame::SetGameDifficulty( Uint32 level )
{
	if ( level == m_difficulty )
	{
		return;
	}

	const Uint32 previousDifficulty = m_difficulty;
	m_difficulty = level; 
	OnGameDifficultyChanged( previousDifficulty, m_difficulty ); 
}

//////////////////////////////////////////////////////////////////////////
// Script
//////////////////////////////////////////////////////////////////////////


void CGame::funcIsActive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsActive() );
}

void CGame::funcIsPaused( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsPaused() );
}

void CGame::funcIsPausedForReason( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, reason, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_BOOL( IsPaused(reason) );
}

void CGame::funcIsStopped( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_stopped );
}

void CGame::funcIsActivelyPaused( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsActivelyPaused() );
}

void CGame::funcIsLoadingScreenVideoPlaying( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsLoadingScreenVideoPlaying() );
}

void CGame::funcGetEngineTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, GetEngineTime() );
}

void CGame::funcGetEngineTimeAsSeconds( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( ( Float ) GetEngineTime() );
}

void CGame::funcGetTimeScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, forCamera, false );
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetTimeScale( forCamera ) );
}

void CGame::funcSetTimeScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, timeScale, 1.0f );
	GET_PARAMETER( CName, source, CName::NONE );
	GET_PARAMETER( Int32, priority, 0 );
	GET_PARAMETER_OPT( Bool, affectCamera, false );
	GET_PARAMETER_OPT( Bool, dontSave, false );
	FINISH_PARAMETERS;
	SetTimeScale( ::Max( timeScale, 0.0f ), source, priority, affectCamera, dontSave );
}

void CGame::funcRemoveTimeScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, source, CName::NONE );
	FINISH_PARAMETERS;
	RemoveTimeScale( source );
}

void CGame::funcRemoveAllTimeScales( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RemoveAllTimeScales();
}

void CGame::funcSetOrRemoveTimeScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, timeScale, 1.0f );
	GET_PARAMETER( CName, source, CName::NONE );
	GET_PARAMETER( Int32, priority, 0 );
	GET_PARAMETER_OPT( Bool, affectCamera, false );
	FINISH_PARAMETERS;
	SetOrRemoveTimeScale( ::Max( timeScale, 0.0f ), source, priority, affectCamera );
}

void CGame::funcLogTimeScales( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	LogTimeScales();
}

void CGame::funcGetGameTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, GetTimeManager()->GetTime() );
}

void CGame::funcSetGameTime( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, time, GameTime() );
	GET_PARAMETER( Bool, callEvents, false );
	GetTimeManager()->SetTime( time, callEvents );
	FINISH_PARAMETERS
}

void CGame::funcSetHoursPerMinute( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, number, 0 );
	FINISH_PARAMETERS;
	GetTimeManager()->SetHoursPerMinute( number );
}

void CGame::funcGetHoursPerMinute( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetTimeManager()->GetHoursPerMinute() );
}

void CGame::funcPause( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, reason, String::EMPTY );
	FINISH_PARAMETERS;

	Pause( reason );
}

void CGame::funcUnpause( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, reason, String::EMPTY );
	FINISH_PARAMETERS;

	Unpause( reason );
}

void CGame::funcExitGame( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RequestGameEnd();
}

void CGame::funcSetActivePause( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;
	SetActivePause( flag );	
}

void CGame::funcCreateEntity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, NULL );
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
		RETURN_OBJECT( NULL );
		return;
	}
	EntitySpawnInfo einfo;
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
	CEntity* entity = GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
	RETURN_OBJECT( entity );
}


void CGame::funcCreateEntityByPath( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String , filename , String() );
	GET_PARAMETER( Vector, spawnPosition, Vector::ZEROS );
	GET_PARAMETER_OPT( EulerAngles, spawnRotation, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Bool, useAppearancesFromIncludes, true );
	GET_PARAMETER_OPT( Bool, fake, false );
	GET_PARAMETER_OPT( Bool, doNotAdjustPlacement, false );
	GET_PARAMETER_OPT( EPersistanceMode, persistanceMode, PM_DontPersist );
	GET_PARAMETER_OPT( TDynArray< CName > , tagList, TDynArray< CName >() );
	FINISH_PARAMETERS;
	
	THandle< CResource > res;
	res = GDepot->LoadResource( filename );
	CEntityTemplate *pEntityTemplate = Cast< CEntityTemplate >( res.Get() );

	if ( pEntityTemplate == NULL || !GetActiveWorld())
	{		
		RETURN_OBJECT( NULL );
		return;
	}
	EntitySpawnInfo einfo;
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
	CEntity* entity = GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
	RETURN_OBJECT( entity );
}

void CGame::funcGetNodeByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	if( !result )
	{
		return;
	}

	THandle<CNode>& retVal = *(THandle<CNode>*)result;

	if ( ! IsActive() || tag == CName::NONE || GetActiveWorld().Get() == nullptr )
	{
		retVal = THandle<CNode>();
		return;
	}
	
	CNode* node = GetActiveWorld()->GetTagManager()->GetTaggedNode( tag );
	retVal = THandle<CNode>( node );
}

void CGame::funcGetEntityByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	if( !result )
	{
		return;
	}
	 
	THandle< CEntity >& retVal = *( THandle< CEntity >* )result;	

	if ( ! IsActive() || tag == CName::NONE || GetActiveWorld().Get() == nullptr )
	{
		retVal = THandle< CEntity >();
		return;
	}
	
	CEntity* entity = GetActiveWorld()->GetTagManager()->GetTaggedEntity( tag );
	retVal = THandle<CEntity>( entity );
}

void CGame::funcGetNodesByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_REF( TDynArray< THandle<CNode> >, outNodes, TDynArray< THandle<CNode> >() );
	FINISH_PARAMETERS;

	if ( ! IsActive() || tag == CName::NONE || GetActiveWorld().Get() == NULL )
		return;

	static TDynArray< CNode* > nodes;
	nodes.ClearFast();
	nodes.Reserve( 16 );
	GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tag, nodes );

	if ( outNodes.Empty() )
	{
		outNodes.Reserve( nodes.Size() );
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
			outNodes.PushBack( nodes[i] );
	}
	else
	{
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
			outNodes.PushBackUnique( nodes[i] );
	}
}

void CGame::funcGetNodesByTags( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName > , tagsList, TDynArray< CName >() );
	GET_PARAMETER_REF( TDynArray< THandle<CNode> >, outNodes, TDynArray< THandle<CNode> >() );
	GET_PARAMETER_OPT( Bool, matchAll, false );
	FINISH_PARAMETERS;

	if ( ! IsActive() || tagsList.Empty() || GetActiveWorld().Get() == nullptr )
		return;

	static TDynArray< CNode* > nodes;
	nodes.ClearFast();
	nodes.Reserve( 16 );
	GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tagsList, nodes, matchAll ? BCTO_MatchAll : BCTO_MatchAny );

	if ( outNodes.Empty() )
	{
		outNodes.Reserve( nodes.Size() );
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
			outNodes.PushBack( nodes[i] );
	}
	else
	{
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
			outNodes.PushBackUnique( nodes[i] );
	}
}

void CGame::funcGetEntitiesByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_REF( TDynArray< THandle<CEntity> >, outEntities, TDynArray< THandle<CEntity> >() );
	FINISH_PARAMETERS;

	if ( ! IsActive() || tag == CName::NONE || GetActiveWorld().Get() == nullptr )
		return;

	TDynArray< CEntity* > entities;
	GetActiveWorld()->GetTagManager()->CollectTaggedEntities( tag, entities );

	if ( outEntities.Empty() )
	{
		for ( Uint32 i = 0; i < entities.Size(); ++i )
			outEntities.PushBack( entities[i] );
	}
	else
	{
		for ( Uint32 i = 0; i < entities.Size(); ++i )
			outEntities.PushBackUnique( entities[i] );
	}
}

void CGame::funcGetWorld( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( m_activeWorld.Get() );
}

void CGame::funcIsFreeCameraEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;	
	RETURN_BOOL( IsFreeCameraEnabled() );
}

void CGame::funcEnableFreeCamera( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	if ( IsCheatEnabled( CHEAT_FreeCamera ) )
	{
		EnableFreeCamera( flag );
	}
}

void CGame::funcIsShowFlagEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EShowFlags, showFlag, EShowFlags(0) );
	FINISH_PARAMETERS;
	const Bool* renderingMask = GetViewport()->GetRenderingMask();
	RETURN_BOOL( renderingMask[ showFlag ] );
}

void CGame::funcSetShowFlag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EShowFlags, showFlag, EShowFlags(0) );
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;
	if( flag )
	{
		GetViewport()->SetRenderingMask( showFlag );
	}
	else
	{
		GetViewport()->ClearRenderingMask( showFlag );
	}
}

void CGame::funcPlayCutsceneAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, csName, String::EMPTY );
	GET_PARAMETER( TDynArray< String >, actorNames, TDynArray< String >() );
	GET_PARAMETER( TDynArray< THandle<CEntity> >, actorEntities, TDynArray< THandle<CEntity> >() );
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	GET_PARAMETER( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Int32, camNum, 0 );
	FINISH_PARAMETERS;

	if ( actorNames.Size() != actorEntities.Size() )
	{
		WARN_ENGINE( TXT("PlayCutscene: Actor name list and actor entities list have diffrent sizes. Fail.") );
		RETURN_BOOL( false );
		return;	
	}

	// Get cutscene resource from cs array
	C2dArray* csArray = resCutsceneArrayDef.LoadAndGet< C2dArray >();
	ASSERT( csArray );

	if ( !csArray )
	{
		WARN_ENGINE( TXT("PlayCutscene: Couldn't find 2d array with cs definitions. Fail.") );
		RETURN_BOOL( false );
		return;
	}

	String fileParh = csArray->GetValue( TXT("Name"), csName, TXT("Resource") );
	if ( fileParh.Empty() )
	{
		WARN_ENGINE( TXT("PlayCutscene: Couldn't find file path for '%ls' cutscene. Fail."), csName.AsChar() );
		RETURN_BOOL( false );
		return;
	}

	// Get cutscene resource template
	CCutsceneTemplate* csTempl = Cast< CCutsceneTemplate >( GDepot->LoadResource( fileParh ) );
	if ( !csTempl )
	{
		WARN_ENGINE( TXT("PlayCutscene: Couldn't find resource '%ls' for '%ls' cutscene. Fail."), fileParh.AsChar(), csName.AsChar() );
		RETURN_BOOL( false );
		return;
	}

	// Destination point
	Matrix dest = rot.ToMatrix();
	dest.SetTranslation( pos );

	// Get dynamic layer
	CLayer* layer = GetActiveWorld()->GetDynamicLayer();
	ASSERT( layer );

	// Create actors map
	THashMap< String, CEntity* > actors;
	for ( Uint32 i=0; i<actorNames.Size(); ++i )
	{
		VERIFY( actors.Insert( actorNames[i], actorEntities[i].Get() ) );
	}

	String errors;

	// Create cutscene instance
	CCutsceneInstance* csInstance = csTempl->CreateInstance( layer, dest, errors, actors );
	if ( !csInstance )
	{
		errors = String::Printf( TXT("PlayCutscene: Couldn't find resource '%ls' for '%ls' cutscene. Fail.\n"), fileParh.AsChar(), csName.AsChar() ) + errors;
		WARN_ENGINE( errors.AsChar() );
		RETURN_BOOL( false );
		return;
	}

	// Start cutscene playing. Auto-update is true. Auto-destroy is true.
	csInstance->Play( true, true, false, camNum );

	// Done
	RETURN_BOOL( true );
}

void CGame::funcIsUsingPad( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsUsingPad() );
}


//////////////////////////////////////////////////////////////////////////
// Latent functions

extern Bool GLatentFunctionStart;

void CGame::funcPlayCutscene( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, csName, String::EMPTY );
	GET_PARAMETER( TDynArray< String >, actorNames, TDynArray< String >() );
	GET_PARAMETER( TDynArray< THandle<CEntity> >, actorEntities, TDynArray< THandle<CEntity> >() );
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	GET_PARAMETER( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Int32, camNum, 0 );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// Starting
	if ( GLatentFunctionStart )
	{
		if ( actorNames.Size() != actorEntities.Size() )
		{
			WARN_ENGINE( TXT("PlayCutscene: Actor name list and actor entities list have diffrent sizes. Fail.") );
			RETURN_BOOL( false );
			return;	
		}

		// Get cutscene resource from cs array
		C2dArray* csArray = resCutsceneArrayDef.LoadAndGet< C2dArray >();
		ASSERT( csArray );

		if ( !csArray )
		{
			WARN_ENGINE( TXT("PlayCutscene: Couldn't find 2d array with cs definitions. Fail.") );
			RETURN_BOOL( false );
			return;
		}

		String fileParh = csArray->GetValue( TXT("Name"), csName, TXT("Resource") );
		if ( fileParh.Empty() )
		{
			WARN_ENGINE( TXT("PlayCutscene: Couldn't find file path for '%ls' cutscene. Fail."), csName.AsChar() );
			RETURN_BOOL( false );
			return;
		}

		// Get cutscene resource template
		CCutsceneTemplate* csTempl = Cast< CCutsceneTemplate >( GDepot->LoadResource( fileParh ) );
		if ( !csTempl )
		{
			WARN_ENGINE( TXT("PlayCutscene: Couldn't find resource '%ls' for '%ls' cutscene. Fail."), fileParh.AsChar(), csName.AsChar() );
			RETURN_BOOL( false );
			return;
		}

		// Destination point
		Matrix dest = rot.ToMatrix();
		dest.SetTranslation( pos );

		// Get dynamic layer
		CLayer* layer = GetActiveWorld()->GetDynamicLayer();
		ASSERT( layer );

		// Create actors map
		THashMap< String, CEntity* > actors;
		for ( Uint32 i=0; i<actorNames.Size(); ++i )
		{
			VERIFY( actors.Insert( actorNames[i], actorEntities[i].Get() ) );
		}

		String errors;

		// Create cutscene instance
		CCutsceneInstance* csInstance = csTempl->CreateInstance( layer, dest, errors, actors );
		if ( !csInstance )
		{
			errors = String::Printf( TXT("PlayCutscene: Couldn't find resource '%ls' for '%ls' cutscene. Fail.\n"), fileParh.AsChar(), csName.AsChar() ) + errors;
			WARN_ENGINE( errors.AsChar() );
			RETURN_BOOL( false );
			return;
		}

		// Set script name
		csInstance->SetName( csName );

		// Start cutscene playing. Auto-update is true. Auto-destroy is false.
		csInstance->Play( true, false, false, camNum );
	}

	// Is still running
	CCutsceneInstance* csInstance = GetCutscene( csName );
	ASSERT( csInstance );

	if ( csInstance && !csInstance->IsFinished() )
	{
		stack.m_thread->ForceYield();
		return;
	}

	// Destroy
	if ( csInstance )
	{
		csInstance->DestroyCs();
	}

	// Done
	RETURN_BOOL( true );
}

void CGame::funcFadeOut( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeTime, 1.0f );
	GET_PARAMETER_OPT( Color, fadeColor, Color::BLACK );
	FINISH_PARAMETERS;

	// Send fade out
	StartFade( false, stack.m_function->GetName().AsString(), fadeTime, fadeColor );
 
	// Yield if not waited long enough
	if ( fadeTime > 0.0f )
	{	
		ASSERT( stack.m_thread );
		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime <= fadeTime )
		{
			stack.m_thread->ForceYield();
		}
	}
}

void CGame::funcFadeIn( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeTime, 1.0f );
	FINISH_PARAMETERS;

	// Send fade in
	StartFade( true, stack.m_function->GetName().AsString(), fadeTime );

	// Yield if not waited long enough
	if ( fadeTime > 0.0f )
	{
		ASSERT( stack.m_thread );
		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime <= fadeTime )
		{
			stack.m_thread->ForceYield();
		}
	}
}

void CGame::funcFadeOutAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeTime, 1.0f );
	GET_PARAMETER_OPT( Color, fadeColor, Color::BLACK );
	FINISH_PARAMETERS;

	// Send fade out
	StartFade( false, stack.m_function->GetName().AsString(), fadeTime, fadeColor );
}

void CGame::funcFadeInAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeTime, 1.0f );
	FINISH_PARAMETERS;

	// Send fade in
	StartFade( true, stack.m_function->GetName().AsString(), fadeTime );
}

void CGame::funcSetFadeLock( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, lockName, TXT("Error param") );
	FINISH_PARAMETERS;

	SetFadeLock( lockName );
}

void CGame::funcResetFadeLock( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, lockName, TXT("Error param") );
	FINISH_PARAMETERS;

	ResetFadeLock( lockName );
}

void CGame::funcIsStreaming( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const Bool isStreaming = false; // we don't have this metric yet
	RETURN_BOOL( isStreaming );
}

void CGame::funcUnlockAchievement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, achName, CName::NONE );
	FINISH_PARAMETERS;

	LOG_ENGINE(TXT("UnlockAchievement '%ls'. IsKosher=%d, KosherFlags=0x%08X"), achName.AsString().AsChar(), IsKosher(), m_kosherTaintFlags );

	if( GUserProfileManager /* && IsKosher() */ )
	{
		LOG_ENGINE( TXT("Unlocking achievement '%ls'..."), achName.AsString().AsChar() );

		GUserProfileManager->UnlockAchievement( achName );

		RETURN_BOOL( true );
	}
	else
	{
		LOG_ENGINE( TXT("NOT Unlocking achievement '%ls'"), achName.AsString().AsChar() );

		RETURN_BOOL( false );
	}
}

void CGame::funcLockAchievement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, achName, CName::NONE );
	FINISH_PARAMETERS;
	
	if( GUserProfileManager )
	{
		GUserProfileManager->LockAchievement( achName );

		RETURN_BOOL( true );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CGame::funcGetUnlockedAchievements( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, array, TDynArray< CName >() );
	FINISH_PARAMETERS;

	TDynArray< CName > allAchievments;
	array.ClearFast();
	if( GUserProfileManager )
	{
		GUserProfileManager->GetAllAchievements( allAchievments );
		for( Uint32 i = 0; i < allAchievments.Size(); ++i )
		{
			if( !GUserProfileManager->IsAchievementLocked( allAchievments[ i ] ) )
			{
				array.PushBack( allAchievments[ i ] );
			}
		}
	}
}

void CGame::funcGetAllAchievements( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, array, TDynArray< CName >() );
	FINISH_PARAMETERS;

	array.ClearFast();
	if ( GUserProfileManager )
	{
		GUserProfileManager->GetAllAchievements( array );
	}
}

void CGame::funcIsAchievementUnlocked( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( GUserProfileManager ? !GUserProfileManager->IsAchievementLocked( name ) : false );
}

void CGame::funcToggleUserProfileManagerInputProcessing( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enabled, true );
	FINISH_PARAMETERS;

	if( GUserProfileManager )
	{
		GUserProfileManager->ToggleInputProcessing( enabled, CUserProfileManager::eAPDR_User );
	}

	RETURN_VOID();
}

void CGame::funcGetDifficulty( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( GetGameDifficulty() );
}

void CGame::funcSetDifficulty( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, difficulty, 0 );
	FINISH_PARAMETERS;

	Config::cvDiffivulty.Set( difficulty );
	SetGameDifficulty( difficulty );
}

void CGame::funcIsCheatEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ECheats, cheatFeature, CHEAT_MAX );
	FINISH_PARAMETERS;
	RETURN_BOOL( IsCheatEnabled( cheatFeature ) );
}

void CGame::funcReloadGameplayConfig( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ReloadConfig();
}

void CGame::ToggleHud()
{
	if( !m_viewport )
	{
		return;
	}

	if ( m_viewport->GetRenderingMask()[ SHOW_GUI ] )
	{
		m_viewport->ClearRenderingMask( SHOW_GUI );
	}
	else
	{
		m_viewport->SetRenderingMask( SHOW_GUI );
	}
}

Bool GIsGrabbingCutsceneMovie = false;

void CGame::ToggleContignous( EFrameCaptureSaveFormat saveFormat /*= FCSF_BMP*/, Bool ubersample /*= false*/ )
{
	m_isContignousCapture = !m_isContignousCapture;
	GIsGrabbingCutsceneMovie = m_isContignousCapture;
	( new CRenderCommand_ToggleContinuousScreenshot( m_isContignousCapture, saveFormat, ubersample ) )->Commit();

#ifdef CONTINUOUS_SCREENSHOT_HACK
	if ( m_isContignousCapture )
	{
		Red::System::Clock::GetInstance().GetTimer().EnableGameTimeHack();
	}
	else 
	{
		Red::System::Clock::GetInstance().GetTimer().DisableGameTimeHack();
	}
#endif
}

#ifndef NO_DEBUG_PAGES

void CGame::CollectCutsceneInstancesName( TDynArray< String >& names ) const
{
	names.Reserve( m_csInstances.Size() );

	for ( Uint32 i=0; i<m_csInstances.Size(); ++i )
	{
		names.PushBack( m_csInstances[ i ]->GetCsTemplate()->GetDepotPath() );
	}
}

#endif

void CGame::funcIsFading( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsFadeInProgress() );
}

void CGame::funcIsBlackscreen( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsBlackscreen() );
}

void CGame::funcGetGameplayChoice( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( true );
}

void CGame::funcIsCurrentlyPlayingNonGameplayScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsCurrentlyPlayingNonGameplayScene() );
}

void CGame::funcIsFinalBuild( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

#ifdef RED_FINAL_BUILD
	RETURN_BOOL( true );
#else
	RETURN_BOOL( false );
#endif
}

void CGame::funcPauseCutscenes( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	PauseCutscenes();
}

void CGame::funcUnpauseCutscenes( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	UnpauseCutscenes();
}

void CGame::funcTogglePad( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, usePad, false );
	FINISH_PARAMETERS;

	TogglePad( usePad );
}

void CGame::funcIsPadConnected( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsPadConnected() );
}


void CGame::funcGetFreeCameraPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, m_freeCamera->GetPosition() );
}

void CGame::funcIsVibrationEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool enabled = false;
	SInputUserConfig::GetIsVibrationEnabled( enabled );
	RETURN_BOOL( enabled );
}

void CGame::funcSetVibrationEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enabled, false );
	FINISH_PARAMETERS;

	SInputUserConfig::SetIsVibrationEnabled( enabled );
}

void CGame::funcVibrateController( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, lowFreq, 0.0f );
	GET_PARAMETER( Float, highFreq, 0.0f );
	GET_PARAMETER( Float, duration, 1.0f );
	FINISH_PARAMETERS;

	VibrateController( lowFreq, highFreq, duration );
}

void CGame::funcStopVibrateController( CScriptStackFrame& stack, void* resul )
{
	FINISH_PARAMETERS;
	m_rumbleLogic.StopRumbles();
}


void CGame::funcGetCurrentVibrationFreq( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Float, lowFreq, 0.0f );
	GET_PARAMETER_REF( Float, highFreq, 0.0f );
	FINISH_PARAMETERS;
	m_rumbleLogic.GetCurrentFreq( lowFreq, highFreq );
}


void CGame::funcRemoveSpecificRumble( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, lowFreq, 0.0f );
	GET_PARAMETER( Float, highFreq, 0.0f );
	FINISH_PARAMETERS;
	m_rumbleLogic.RemoveRumble( lowFreq, highFreq );
}

void CGame::funcIsSpecificRumbleActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, lowFreq, 0.0f );
	GET_PARAMETER( Float, highFreq, 0.0f );
	FINISH_PARAMETERS;

	RETURN_BOOL( m_rumbleLogic.IsRumbleActive( lowFreq, highFreq ) );
}

void CGame::funcOverrideRumbleDuration( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, lowFreq, 0.0f );
	GET_PARAMETER( Float, highFreq, 0.0f );
	GET_PARAMETER( Float, newDuration, 0.0f );
	FINISH_PARAMETERS;

	if( !m_inputManager || !m_inputManager->LastUsedGamepad() )
	{
		return;
	}

	m_rumbleLogic.OverrideDuration( lowFreq, highFreq, newDuration );
}

void CGame::funcGetGameplayConfigFloatValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, propName, CName::NONE );
	FINISH_PARAMETERS;

	Float val = 0.f;

	CProperty* prop = m_gameplayConfig.GetClass()->FindProperty( propName );
	if ( prop )
	{
		const IRTTIType* propType = prop->GetType();
		
		const CName propTypeName = propType->GetName();
		if ( propType->GetType() == RT_Fundamental && propTypeName == CNAME( Float ) )
		{
			prop->Get( &m_gameplayConfig, &val );
		}
	}

	RETURN_FLOAT( val );
}

void CGame::funcGetGameplayConfigBoolValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, propName, CName::NONE );
	FINISH_PARAMETERS;

	Bool val = false;

	CProperty* prop = m_gameplayConfig.GetClass()->FindProperty( propName );
	if ( prop )
	{
		const IRTTIType* propType = prop->GetType();

		const CName propTypeName = propType->GetName();
		if ( propType->GetType() == RT_Fundamental && propTypeName == CNAME( Bool ) )
		{
			prop->Get( &m_gameplayConfig, &val );
		}
	}

	RETURN_BOOL( val );
}

void CGame::funcGetGameplayConfigIntValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, propName, CName::NONE );
	FINISH_PARAMETERS;

	Int32 val = 0;

	CProperty* prop = m_gameplayConfig.GetClass()->FindProperty( propName );
	if ( prop )
	{
		const IRTTIType* propType = prop->GetType();

		const CName propTypeName = propType->GetName();
		if ( propType->GetType() == RT_Fundamental && propTypeName == CNAME( Int32 ) )
		{
			prop->Get( &m_gameplayConfig, &val );
		}
	}

	RETURN_INT( val );
}

void CGame::funcGetGameplayConfigEnumValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, propName, CName::NONE );
	FINISH_PARAMETERS;

	Int32 val = 0;

	CProperty* prop = m_gameplayConfig.GetClass()->FindProperty( propName );
	if ( prop )
	{
		const IRTTIType* propType = prop->GetType();

		const CName propTypeName = propType->GetName();
		if ( propType->GetType() == RT_Enum )
		{
			const CEnum* enumType = static_cast< const CEnum* >( propType );
			val = enumType->GetAsInt( &m_gameplayConfig );
		}
		else if ( propType->GetType() == RT_Fundamental && propTypeName == CNAME( Int32 ) )
		{
			prop->Get( &m_gameplayConfig, &val );
		}
	}

	RETURN_INT( val );
}

void CGame::funcSetAIObjectsLooseTime( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, time, 15.0f );
	FINISH_PARAMETERS;
	m_aiObjectLooseTime = time;
}

Bool CGame::DoesInitialFactExist( const String& factName ) const
{
	for ( auto& fact : m_initFacts )
	{
		if ( fact.m_name == factName )
		{
			return true;
		}
	}
	return false;
}

void CGame::funcAddInitialFact( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, factName, String::EMPTY );
	FINISH_PARAMETERS;

	if ( !DoesInitialFactExist( factName ) )
	{
		new ( m_initFacts ) SInitialFact( factName, 1 );
	}
}

void CGame::funcRemoveInitialFact( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, factName, String::EMPTY );
	FINISH_PARAMETERS;

	for ( auto it = m_initFacts.Begin(); it != m_initFacts.End(); ++it )
	{
		if ( it->m_name == factName )
		{
			m_initFacts.EraseFast( it );
			return;
		}
	}
}

void CGame::funcClearInitialFacts( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ClearInitialFacts();
}

// !!! Don't expose replacing the default loading screen to scripts !!!
void CGame::funcSetSingleShotLoadingScreen( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, contextName, CName::NONE );
	GET_PARAMETER_OPT( String, initString, String::EMPTY );
	GET_PARAMETER_OPT( String, videoToPlay, String::EMPTY );
	FINISH_PARAMETERS;

	ReplaceLastLoadingScreenParam( SLoadingScreenParam( contextName, initString, videoToPlay ) );
}

void CGame::funcDebugActivateContent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, contentToActivate, CName::NONE );
	FINISH_PARAMETERS;

	if ( contentToActivate && GContentManager->ActivateContent( contentToActivate ) )
	{
		LOG_ENGINE(TXT("DebugActivateContent: Activating content '%ls'"), contentToActivate.AsChar() );
		GContentManager->AddTaintedFlags( eContentTaintedFlag_ContentActivated );
	}
	else
	{
		ERR_ENGINE(TXT("DebugActivateContent: Failed to activate content '%ls'"), contentToActivate.AsChar() );
	}
}

void CGame::funcHasBlackscreenRequested( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const Bool req = HasBlackscreenRequested();
	const Bool isBlackscreen = IsBlackscreen(); // get loading screen conditions...
	const Bool retval = req || isBlackscreen;

	RETURN_BOOL( retval );
}

Uint32 CGame::DebugCallback( Char* buffer, Uint32 bufferSize )
{
	Uint32 bufferUsed = 0;

	// last opened world
	if( GGame )
	{
		if ( CWorld* activeWorld = GGame->GetActiveWorld() )
		{
			if ( CDiskFile* worldFile = activeWorld->GetFile() )
			{
				RED_APPEND_ERROR_STRING( buffer, bufferSize, bufferUsed, TXT( "Last opened world: %ls\n" ), worldFile->GetFileName().AsChar() );
			}
		}
	}

	return bufferUsed;
}

void CGame::NotifyGameInputModeEnabled()
{
	// Activate viewport manually, since the other way is to mouse click on it and we lose the viewport when mousing out in the editor
	GetViewport()->Activate();

	if( !m_isUsingPad )
	{
		GetViewport()->SetMouseMode( MM_ClipAndCapture );
	}

	ForEach( m_gameInputModeListeners, [](IGameInputModeListener* listener){ listener->OnGameInputMode(true); } );
}

void CGame::NotifyGameInputModeDisabled()
{
	GetViewport()->Deactivate();

	ForEach( m_gameInputModeListeners, [](IGameInputModeListener* listener){ listener->OnGameInputMode(false); } );

	if( !m_isUsingPad )
	{
		GetViewport()->SetMouseMode( MM_Normal );
	}
}

CClass* CGame::GetGameWorldClass() const 
{ 
	return ClassID< CWorld >(); 
}

Bool CGame::TryEndGame()
{
	if ( m_isActive && m_requestEnd && !m_isInStartGame )
	{
		m_rumbleLogic.StopRumbles();
		SGameSessionManager::GetInstance().EndSession();
		return true;
	}

	return false;
}

Bool CGame::IsLoadingVideoSkipKey( EInputKey key ) const
{
	return m_loadingScreen->IsVideoSkipKey( key );
}

const SLoadingScreenParam& CGame::GetActiveLoadingScreenParam() const
{
	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	return m_loadingScreenStack.Back();
}

SLoadingScreenParam& CGame::GetActiveLoadingScreenParam()
{
	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	return m_loadingScreenStack.Back();
}

void CGame::PushLoadingScreenParam( const SLoadingScreenParam& param )
{
	m_loadingScreenStack.PushBack( param );
}

void CGame::ReplaceDefaultLoadingScreenParam( const SLoadingScreenParam& param )
{
	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	LOG_ENGINE(TXT("ReplaceDefaultLoadingScreenParam: video from '%ls' to '%ls', name from '%ls' to '%ls'"), 
		 m_loadingScreenStack[0].m_videoToPlay.AsChar(), param.m_videoToPlay.AsChar(),
		 m_loadingScreenStack[0].m_contextName.AsChar(), param.m_contextName.AsChar());
	m_loadingScreenStack[0] = param;
}

void CGame::ReplaceDefaultLoadingScreenVideo( const String& videoToPlay )
{
	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	LOG_ENGINE(TXT("ReplaceDefaultLoadingScreenVideo from '%ls' to '%ls'"), m_loadingScreenStack[0].m_videoToPlay.AsChar(), videoToPlay.AsChar());
	m_loadingScreenStack[0].m_videoToPlay = videoToPlay;
}

void CGame::ReplaceDefaultLoadingScreenName( CName name )
{
	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	LOG_ENGINE(TXT("ReplaceDefaultLoadingScreenName from '%ls' to '%ls'"), m_loadingScreenStack[0].m_contextName.AsChar(), name.AsChar());
	m_loadingScreenStack[0].m_contextName = name;
}

void CGame::ClearLoadingScreenParams( const SLoadingScreenParam& newDefaultParam )
{
	m_loadingScreenStack.Clear();
	m_loadingScreenStack.PushBack( newDefaultParam );
}

void CGame::ClearLoadingScreenParams()
{
	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	m_loadingScreenStack.ResizeFast(1);
}

void CGame::ReplaceLastLoadingScreenParam( const SLoadingScreenParam& param )
{
	RED_FATAL_ASSERT( !m_loadingScreenStack.Empty(), "Loading screen stack is empty! Should have default left" );
	if ( m_loadingScreenStack.Size() > 1 )
	{
		m_loadingScreenStack.Back() = param;
	}
	else
	{
		// Don't replace the default
		m_loadingScreenStack.PushBack( param );
	}
}

void CGame::funcGetCurrentViewportResolution(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER_REF( Uint32, width, 1920 );
	GET_PARAMETER_REF( Uint32, height, 1080 );
	FINISH_PARAMETERS;

	width = GetViewport()->GetWidth();
	height = GetViewport()->GetHeight();

	RETURN_VOID();
}

void CGame::MoveMouseTo( Float targetX, Float targetY )
{
	if ( m_flashPlayer )
	{
		m_flashPlayer->SetMousePosition( targetX, targetY );
	}
}
