/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/TestFramework.h"
#include "../../common/engine/gameResource.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/core/depot.h"
#include "../../common/core/configFileManager.h"

#include "gameEngine.h"

#define STARTUP_PACKAGE		TXT("startup.w2pkg")

extern EShowFlags	GShowGameMask[];
extern Bool			GGlobalScriptFunctionToExecuteOnGameStated;
extern String		GGlobalScriptFunctionToExecuteOnGameStatedWithArguments;
extern Bool			GEnablePhysicsDebuggerInGameExe;

//===========================================================================
// Dummy implementation of the editor side class that's being used in scripts

#include "../../common/game/storyScenePlayer.h"
#include "../../common/engine/debugConsole.h"
#include "../../common/engine/viewport.h"
#include "../../common/core/tokenizer.h"
#include "../../common/engine/renderSettings.h"
class CStoryScenePreviewPlayer : public CStoryScenePlayer
{
	DECLARE_ENGINE_CLASS( CStoryScenePreviewPlayer, CStoryScenePlayer, 0 );
};

BEGIN_CLASS_RTTI( CStoryScenePreviewPlayer );
PARENT_CLASS( CStoryScenePlayer );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CStoryScenePreviewPlayer );

void HACK_InitializeEditorClasses()
{
	touchClassCStoryScenePreviewPlayer();
}

//===========================================================================

CGameEngine::CGameEngine()
	: m_debugMenu( NULL )
	, m_viewport( NULL )
{
}

Bool CGameEngine::Initialize()
{
	// Initialize base engine
	if ( !CBaseEngine::Initialize() )
	{
		return false;
	}

	HACK_InitializeEditorClasses();
	

	GGame->GetInputManager()->Reset();

	// Use desktop size
	Int32 desktopX = ::GetSystemMetrics( SM_CXSCREEN );
	Int32 desktopY = ::GetSystemMetrics( SM_CYSCREEN );
	Int32 singleScreenWidth = desktopX;
	Int32 singleScreenHeight = desktopY;
	Bool fullScreen = Config::cvIsFullScreen.Get();

	Config::Helper::ExtractDisplayModeValues( Config::cvResolution.Get(), width, height );
	
	Bool isCommandlineResolutionSet = false;

#if !defined( RED_FINAL_BUILD )
	String worldToRun;
#endif

	Red::System::Log::Manager::GetInstance().SetEnabled( true );
#ifdef RELEASE
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );
#endif 

	GDataAsserts = false;
	Uint32 texRes = 2;
	{
		// Tokenize commandline
		CTokenizer tok( SGetCommandLine(), TXT(" ") );
		for ( Uint32 i=0; i<tok.GetNumTokens(); ++i )
		{
			String token = tok.GetToken( i );
			// Texture resolution clamp
			if ( token == TXT("-texres") )
			{
				const String param = tok.GetToken( i + 1 );
				FromString< Int32 >( param, (Int32&)texRes );
				texRes = ::Clamp< Int32 >( texRes, 0, 4 );
			}

			// Screen resolution ( width )
			if ( token == TXT( "-width" ) )
			{
				const String param = tok.GetToken( i + 1 );
				FromString< Int32 >( param, (Int32&)desktopX );
				isCommandlineResolutionSet = true;
			}

			// Screen resolution ( height )
			if ( token == TXT( "-height" ) )
			{
				const String param = tok.GetToken( i + 1 );
				FromString< Int32 >( param, (Int32&)desktopY );
				isCommandlineResolutionSet = true;
			}

			// Play in window
			if ( token == TXT( "-window" ) )
			{
				fullScreen = false;
			}

			// Enable logging
			if ( token == TXT( "nolog" ) || token == TXT( "-nolog" ) )
			{
				LOG_R6( TXT("win32 GameEngine: Logging has been DISABLED") );
				Red::System::Log::Manager::GetInstance().SetEnabled( false );
			}

			// Execute script on start
			if ( token == TXT( "-script" ) )
			{
				GGlobalScriptFunctionToExecuteOnGameStatedWithArguments = tok.GetToken( i + 1 );
				size_t openBracketIndex;
				size_t closeBracketIndex;
				GGlobalScriptFunctionToExecuteOnGameStated =
				(
					GGlobalScriptFunctionToExecuteOnGameStatedWithArguments.FindCharacter( ')', closeBracketIndex ) &&
					closeBracketIndex > 0 &&
					GGlobalScriptFunctionToExecuteOnGameStatedWithArguments.FindCharacter( '(', openBracketIndex ) &&
					openBracketIndex > 0
				);	
			}

			// Visual debugger - enable here on xbox by default (it's still disabled by default in gameSettings.h)
			if ( token == TXT( "-vdb" ) )
			{
				GEnablePhysicsDebuggerInGameExe = true;
			}

#if !defined( RED_FINAL_BUILD )
			// World to run
			if ( token.EndsWith( TXT(".w2w") ) || token.EndsWith( TXT(".redgame") ) )
			{
				LOG_R6( TXT("World to run: '%s'"), token.AsChar() );
				worldToRun = token;
			}
#endif
		}

#ifndef NO_TEST_FRAMEWORK
		// feed TestFramework with commandline parameters
		STestFramework::GetInstance().ParseCommandline( SGetCommandLine() );
#endif

		LOG_R6
		(
			TXT( "win32 GameEngine : logging turned %ls, assertions turned %ls." ),
			Red::System::Log::Manager::GetInstance().IsEnabled() ? TXT("ON") : TXT("OFF"),
			Red::System::Error::Handler::GetInstance()->HasAssertFlag( Red::System::Error::AC_Break ) ? TXT("ON") : TXT("OFF")
		);
	}
	
	m_viewport = GRender->CreateGameViewport( TXT("Cyberpunk 2077"), desktopX, desktopY, fullScreen );

	if ( !m_viewport )
	{ 
		LOG_R6( TXT("Unable to create main viewport") );
		return false;
	}

	// Adjust internal size	
#ifdef RED_VIEWPORT_TURN_ON_CACHETS_16_9
	m_viewport->AdjustSizeWithCachets( IViewport::EAspectRatio::FR_16_9 );
#else
	m_viewport->AdjustSizeWithCachets( IViewport::EAspectRatio::FR_NONE );
#endif

	CConfigFile* filterConfig = SConfig::GetInstance().GetLegacy().GetFile( TXT( "filters" ) );

	EShowFlags* filterFlags = GShowGameMask;
	TDynArray< EShowFlags > customFilterFlags;
	if( filterConfig )
	{
		auto& sections = filterConfig->GetSections();
		CEnum* showFlagEnum = SRTTI::GetInstance().FindEnum( CNAME( EShowFlags ) );

		for( auto sectionIter = sections.Begin(); sectionIter != sections.End(); ++sectionIter )
		{
			if( sectionIter->m_first == TXT( "Show" ) )
			{
				CConfigSection* section = sectionIter->m_second;

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

	// Set game viewport
	m_viewport->SetRenderingMode( RM_Shaded );
	m_viewport->SetRenderingMask( filterFlags );
	GGame->SetViewport( m_viewport );

	// Create console instance
	GDebugConsole = new CDebugConsole();

#if !defined( RED_FINAL_BUILD )
	if( worldToRun.Size() )
	{
		if ( worldToRun.EndsWith( TXT(".redgame") ) )
		{
			GGame->SetupGameResourceFromFile( worldToRun );
			CGameResource *res = GGame->GetGameResource();
			if ( res )
			{
				worldToRun = res->GetStartWorldPath();		
			}
			else
			{
				LOG_R6( TXT("wrong .redgame resource: '%s'"), worldToRun.AsChar() );
				return false;
			}
		}

		SGameSessionManager::GetInstance().CreateSession( worldToRun );
	}
#endif

#ifndef NO_TEST_FRAMEWORK
	STestFramework::GetInstance().OnStart( m_viewport );
#endif

	// Preload resources
	PostInitialize();

	m_viewport->SetViewportHook( GGame );
	//FIXME<<<
//	m_viewport->CaptureInput( ICM_Full );
	
	
#if !defined( RED_FINAL_BUILD )
	// Show debug menu
	if ( !worldToRun.Size() )
	{
#ifndef NO_DEBUG_PAGES
		extern void CreateDebugPageGameStartup();
		CreateDebugPageGameStartup();
#endif
	}
#endif

	// Initialized
	return true;
}

void CGameEngine::Shutdown()
{
#ifndef NO_TEST_FRAMEWORK
	STestFramework::GetInstance().OnFinish();
#endif

	// Shut down console
	GDebugConsole = NULL;

	if ( m_viewport )
	{
		GGame->SetViewport( NULL );
		delete m_viewport;
		m_viewport = NULL;
	}

	// Shut down base engine
	CBaseEngine::Shutdown();
}

void CGameEngine::Tick( Float timeDelta )
{
	// Process messages
	if ( !SPumpMessages() )
	{
		m_requestExit = true;
	}

	// Tick base engine
	CBaseEngine::Tick( timeDelta );

	// Tick main menu
	GGame->TickMainMenu( timeDelta );
}

void CGameEngine::OnGameEnded()
{
}

void CGameEngine::PostInitialize( Float timeDelta )
{
	m_postInitialized = true;
}
