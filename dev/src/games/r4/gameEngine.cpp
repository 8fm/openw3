/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

// HACK. GIVE US BACK OUR PLATFORM ENGINE NOT THIS HYBRID "GAME ENGINE"!!!
#ifdef RED_PLATFORM_ORBIS
# include <system_service.h> // TMP: for sceSystemServiceHideSplashScreen
#endif

#include "../../common/engine/inputDeviceManager.h"

#include "../../common/engine/TestFramework.h"
#include "../../common/engine/gameResource.h"
#include "../../common/renderer/renderViewportWindow.h"
#include "../../common/core/tokenizer.h"
#include "../../common/core/depot.h"
#include "../../common/core/configVar.h"
#include "../../common/core/configVarLegacyWrapper.h"
#include "../../common/core/loadingProfiler.h"

#include "gameDebugMenu.h"
#include "gameEngine.h"
#include "r4MemoryBudgets.h"

// alpha hack
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/platformViewport.h"
#include "../../common/engine/debugConsole.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderSettings.h"

#if !defined( NO_TELEMETRY )	
#include "r4Telemetry.h"
#endif // NO_TELEMETRY
#include "../../common/engine/soundSystem.h"

/////////////////////////////////////////////////////////////////////////////////
// Globals, externs, etc, etc
//
#define STARTUP_PACKAGE		TXT("startup.w2pkg")
extern EShowFlags	GShowGameMask[];
extern Bool			GGlobalScriptFunctionToExecuteOnGameStated;
extern String		GGlobalScriptFunctionToExecuteOnGameStatedWithArguments;
extern Bool			GEnablePhysicsDebuggerInGameExe;
extern Bool			GEnableKinectInGameExe;

/////////////////////////////////////////////////////////////////////////////////
// Params default CTor
//
CR4GameEngine::Parameters::Parameters()
	: m_desktopWidth( -1 )
	, m_desktopHeight( -1 )
	, m_desiredWidth( -1 )				
	, m_desiredHeight( -1 )
	, m_resolutionOverride( false )
	, m_forceWindowed( false )
	, m_textureResolutionClamp( 2 )
	, m_enableLogging( true )
	, m_enablePhysicsDebugger( true )
	, m_enableKinect( false )
	, m_enableFPSDisplay( false )
	, m_borderlessMode( false )
{
}

/////////////////////////////////////////////////////////////////////////////////
// CTor
//
CR4GameEngine::CR4GameEngine( const Parameters& params, const Core::CommandLineArguments& coreParams )
	: m_debugMenu( NULL )
	, m_initialParameters( params )
	, m_initialCoreParameters( coreParams )
{
//#ifdef RED_PLATFORM_ORBIS
//	m_initialParameters.m_worldToRun = TXT("qa/drey/ps4_test_level_february_14/ps4test.redgame");
//#endif
}

/////////////////////////////////////////////////////////////////////////////////
// GetViewportSettings
//	Returns settings for the viewport based on params and render settings
void CR4GameEngine::GetViewportSettings( Int32& width, Int32& height, EViewportWindowMode &windowMode )
{
	if( m_initialParameters.m_resolutionOverride )
	{
		RED_ASSERT( m_initialParameters.m_desiredWidth > 0, TXT( "Bad desired width" ) );
		RED_ASSERT( m_initialParameters.m_desiredHeight > 0, TXT( "Bad desired height" ) );
		width = m_initialParameters.m_desiredWidth;
		height = m_initialParameters.m_desiredHeight;

		Int32 bestWidth, bestHeight;
		Config::Helper::GetBestMatchingResolution( width, height, bestWidth, bestHeight );
	}
	else
	{
		Config::Helper::ExtractDisplayModeValues( Config::cvResolution.Get(), width, height );
	}

#ifndef RED_PLATFORM_CONSOLE
	if ( m_initialParameters.m_borderlessMode )
	{
		windowMode = VWM_Borderless;
	}
	else 
#endif
	if ( m_initialParameters.m_forceWindowed )
	{
		windowMode = VWM_Windowed;
	}
	else
	{
		windowMode = (EViewportWindowMode)Clamp<Int32>( Config::cvFullScreenMode.Get(), VWM_Windowed, VWM_Fullscreen );
	}
}

/////////////////////////////////////////////////////////////////////////////////
// ProcessViewportFilterConfigs
//	Parses the configs for render filter flags
void CR4GameEngine::ProcessViewportFilterConfigs()
{
	Config::Legacy::CConfigLegacyFile* filterConfig = SConfig::GetInstance().GetLegacy().GetFile( TXT( "filters" ) );
	EShowFlags* filterFlags = GShowGameMask;
	if( filterConfig )
	{
		TDynArray< EShowFlags > customFilterFlags;
		auto& sections = filterConfig->GetSections();
		CEnum* showFlagEnum = SRTTI::GetInstance().FindEnum( CNAME( EShowFlags ) );

		for( auto sectionIter = sections.Begin(); sectionIter != sections.End(); ++sectionIter )
		{
			if( sectionIter->m_first == TXT( "Show" ) )
			{
				Config::Legacy::CConfigLegacySection* section = sectionIter->m_second;

				const auto& keyValueMap = section->GetItems();
				for( auto keyIter = keyValueMap.Begin(); keyIter != keyValueMap.End(); ++keyIter )
				{
					Int32 enumValue = SHOW_MAX_INDEX;
					if( showFlagEnum->FindValue( CName( keyIter->m_first ), enumValue ) )
					{
						Bool isSet = false;

						FromString( keyIter->m_second[ 0 ], isSet );

						if( isSet )
						{
							customFilterFlags.PushBack( static_cast< EShowFlags >( enumValue ) );
						}
					}
				}
			}
		}

		if( !customFilterFlags.Empty() )
		{
			customFilterFlags.PushBack( SHOW_MAX_INDEX );
			filterFlags = customFilterFlags.TypedData();
		}
	}

	m_viewport->SetRenderingMask( filterFlags );
}

/////////////////////////////////////////////////////////////////////////////////
// Initialize
//	Start up the engine ready for running
Bool CR4GameEngine::Initialize()
{
	// Initialize base engine
	if ( !CBaseEngine::Initialize() )
	{
		return false;
	}

	// Set up memory budgets
	InitialiseMemoryBudgets();

	// Set up asserts
	GDataAsserts = false;
	Red::System::Log::Manager::GetInstance().SetEnabled( true );

	// Enable/disable logging
	if ( !m_initialParameters.m_enableLogging )
	{
		LOG_R4( TXT("Logging has been DISABLED") );
		Red::System::Log::Manager::GetInstance().SetEnabled( false );
	}

	// Set up scripts to run on startup
	if( m_initialParameters.m_scriptToExecuteOnStartup.GetLength() > 0 )
	{
		GGlobalScriptFunctionToExecuteOnGameStatedWithArguments = m_initialParameters.m_scriptToExecuteOnStartup;
		size_t openBracketIndex;
		size_t closeBracketIndex;
		GGlobalScriptFunctionToExecuteOnGameStated = ( m_initialParameters.m_scriptToExecuteOnStartup.FindCharacter( ')', closeBracketIndex ) && closeBracketIndex > 0 &&
													   m_initialParameters.m_scriptToExecuteOnStartup.FindCharacter( '(', openBracketIndex ) && openBracketIndex > 0 );
	}

	// Set up physics debugger
	GEnablePhysicsDebuggerInGameExe = m_initialParameters.m_enablePhysicsDebugger;

#ifndef NO_TEST_FRAMEWORK
	// feed TestFramework with commandline parameters
	if(! STestFramework::GetInstance().ParseCommandline( SGetCommandLine() ) )
	{
		ERR_R4( TXT( "Failed to initialize test framework." ) );
		return false;
	}
#endif

	if( m_initialParameters.m_worldToRun.GetLength() > 0 )
	{
		LOG_R4( TXT("World to run: '%ls'"), m_initialParameters.m_worldToRun.AsChar() );
	}
	
	LOG_R4
	(
		TXT( "win32 GameEngine : logging turned %ls, assertions turned %ls." ),
		Red::System::Log::Manager::GetInstance().IsEnabled() ? TXT("ON") : TXT("OFF"),
		Red::System::Error::Handler::GetInstance()->HasAssertFlag( Red::System::Error::AC_Break ) ? TXT("ON") : TXT("OFF")
	);
	
	// Initialise the viewport
	Int32 screenWidth = 0, screenHeight = 0;
	EViewportWindowMode windowMode = VWM_Windowed;
	GetViewportSettings( screenWidth, screenHeight, windowMode );

	// Reset resolution and fullscreen mode configs in cases like commandline overrides or first launch
	Config::cvResolution.Set( Config::Helper::PackDisplayModeValues( screenWidth, screenHeight ) );
	Config::cvFullScreenMode.Set( (Int32)windowMode );

	if( windowMode == VWM_Fullscreen )
	{
		GInGameConfig::GetInstance().ActivateTag(CName(TXT("fullscreen")));
	}

	m_viewport = GRender->CreateGameViewport( TXT("The Witcher 3"), screenWidth, screenHeight, windowMode );
	if ( !m_viewport )
	{ 
		LOG_R4( TXT("Unable to create main viewport") );
		return false;
	}

	m_viewport->RequestOutputMonitor( Config::Helper::GetCurrentOutputMonitorConfig() );

	GLoadingProfiler.FinishStage( TXT("CreateViewport") );

	// Adjust internal size	
	if( GIsGame == false )
	{
#ifdef RED_VIEWPORT_TURN_ON_CACHETS_16_9
		m_viewport->AdjustSizeWithCachets( IViewport::EAspectRatio::FR_16_9 );
#else
		m_viewport->AdjustSizeWithCachets( IViewport::EAspectRatio::FR_NONE );
#endif
	}
	else
	{
		m_viewport->AdjustSizeWithCachets( IViewport::EAspectRatio::FR_NONE );
	}

	// Process filters from config
	ProcessViewportFilterConfigs();

	// Set game viewport
	m_viewport->SetRenderingMode( RM_Shaded );
	GGame->SetViewport( m_viewport );
	
	// Create console instance
	GDebugConsole = new CDebugConsole();

	RED_ASSERT( GGame );

	//FIXME2<<< Remove before deleting GGame...
	GGame->AddGameInputModeListener( m_inputDeviceManager );

	m_viewport->Activate();
	m_inputDeviceManager->OnGameInputMode( true );
	GLoadingProfiler.FinishStage( TXT("ActivateViewport") );

#ifndef NO_TEST_FRAMEWORK
	STestFramework::GetInstance().OnStart( m_viewport.Get() );
#endif

	// Sound system finish loading
	GSoundSystem->FinalizeLoading();
	GLoadingProfiler.FinishStage( TXT("SoundFinalize") );

	// Preload resources
	PostInitialize();
	GLoadingProfiler.FinishStage( TXT("GamePostInitialize") );

#ifndef RED_FULL_DETERMINISM		// Not sure if this condition needs to be here any more
	// Initialize GUI
	GGame->ShowMainMenuFirstTime( true );
#endif

	m_viewport->SetViewportHook( GGame );

	// Start from specific game definition
	if( m_initialParameters.m_worldToRun.GetLength() > 0 )
	{
		if ( m_initialParameters.m_worldToRun.EndsWith( TXT(".redgame") ) )
		{
			GGame->SetupGameResourceFromFile( m_initialParameters.m_worldToRun );
			CGameResource *res = GGame->GetGameResource();
			if ( res )
			{
				m_initialParameters.m_worldToRun = res->GetStartWorldPath();
				m_initialParameters.m_videoToPlay = res->GetNewGameLoadingVideo();
			}
			else
			{
				LOG_R4( TXT("wrong .redgame resource: '%ls'"), m_initialParameters.m_worldToRun.AsChar() );
				m_initialParameters.m_worldToRun.Clear();
				return false;
			}
		}
	}
	// Start with m menu
	else
	{
#			if !defined( NO_DEBUG_PAGES )
					extern void CreateDebugPageGameStartup( Bool scanDepot, Bool makeVisible );
#			if defined( RED_PLATFORM_WINPC )
					const Bool scanDepot = true;
#			else
					const Bool scanDepot = m_initialCoreParameters.m_useBundles;
#			endif // defined( RED_PLATFORM_WINPC )
					CreateDebugPageGameStartup( scanDepot, false );
#			endif // !defined( NO_DEBUG_PAGES )
}

#if !defined( NO_TELEMETRY )	
	// here telemetry should be created
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
	{
		telemetrySystem->InitializeDebugWindows();
	}
#endif //NO_TELEMETRY

	// Turn on/off Kinect
	GEnableKinectInGameExe = m_initialParameters.m_enableKinect;

	// Initialized
	return true;
}

/////////////////////////////////////////////////////////////////////////////////
// Shutdown
//	Cleanup any systems
void CR4GameEngine::Shutdown()
{
#ifndef NO_TEST_FRAMEWORK
	STestFramework::GetInstance().OnFinish();
#endif

	// Shut down console
	GDebugConsole = NULL;

	if ( m_viewport )
	{
		GGame->SetViewport( ViewportHandle() );
		m_viewport.Reset();
	}

	// Shut down base engine
	CBaseEngine::Shutdown();
}

/////////////////////////////////////////////////////////////////////////////////
// Tick
//	Run the next frame of logic
void CR4GameEngine::Tick( Float timeDelta )
{
	// Tick message pump
	if( !GetPlatformViewport()->PumpMessages() )
	{
		m_requestExit = true;
	}

	// Tick base engine
	CBaseEngine::Tick( timeDelta );

	// Start from specific game definition (after the first tick)
	if( m_initialParameters.m_worldToRun.GetLength() > 0 )
	{
		SGameSessionManager::GetInstance().CreateSession( m_initialParameters.m_worldToRun, false, nullptr, m_initialParameters.m_videoToPlay );
		m_initialParameters.m_worldToRun.Clear();
		m_initialParameters.m_videoToPlay.Clear();
	}

	// Tick main menu
	GGame->TickMainMenu( timeDelta );
}

/////////////////////////////////////////////////////////////////////////////////
// OnGameEnded
//
void CR4GameEngine::OnGameEnded()
{
}

/////////////////////////////////////////////////////////////////////////////////
// PostInitialize
//	Do stuff after the engine is initialized
void CR4GameEngine::PostInitialize( float timeDelta )
{
	// HACK. GIVE US BACK OUR PLATFORM ENGINE NOT THIS HYBRID "GAME ENGINE"!!!
#ifdef RED_PLATFORM_ORBIS
	// TBD: Move somewhere appropriate.
	const Int32 err = ::sceSystemServiceHideSplashScreen();
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("sceSystemServiceHideSplashScreen() returned error code 0x%08X"), err );
	}
#endif

	m_postInitialized = true;
}
