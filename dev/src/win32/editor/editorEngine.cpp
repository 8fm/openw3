/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "inputBoxDlg.h"
#include "scriptCompilationErrorsDialog.h"
#include "editorMemoryBudgets.h"
#include "../../common/redMemoryFramework/redMemorySystemMemoryStats.h"

#include "../../common/engine/TestFramework.h"
#include "../../common/game/actionPoint.h"
#include "../../common/game/dlcManager.h"

#include "../../win32/platform/inputDeviceManagerWin32.h"

#include "../../common/core/depot.h"
#include "../../common/core/thumbnail.h"
#include "../../common/core/tokenizer.h"
#include "../../common/core/feedback.h"

#include "googleanalytics.h"

#if !defined( NO_TELEMETRY )	
#include "../../games/r4/r4Telemetry.h"
#endif // NO_TELEMETRY

#include "..\..\common\engine\debugConsole.h"
#include "..\..\common\engine\renderer.h"
#include "..\..\common\engine\renderSettings.h"

#include "..\..\common\engine\soundSystem.h"

CEditorEngine::CEditorEngine()
{
}

static void CollectMeshFiles( CDirectory* dir, TDynArray< CDiskFile* >& meshFiles )
{
	// Check files
	for ( CDiskFile* file : dir->GetFiles() )
	{
		if ( file->GetFileName().EndsWith( TXT("w2mesh") ) )
		{
			meshFiles.PushBack( file );
		}
	}

	// Recurse
	for ( CDirectory* cur : dir->GetDirectories() )
	{
		CollectMeshFiles( cur, meshFiles );
	}
}

static void CollectEntityTemplateFiles( CDirectory* dir, TDynArray< CDiskFile* >& meshFiles )
{
	// Check files
	for ( CDiskFile* file : dir->GetFiles() )
	{
		if ( file->GetFileName().EndsWith( TXT("w2ent") ) )
		{
			meshFiles.PushBack( file );
		}
	}

	// Recurse
	for ( CDirectory* cur :  dir->GetDirectories() )
	{
		CollectEntityTemplateFiles( cur, meshFiles );
	}
}

static void CollectLevelFiles( CDirectory* dir, TDynArray< CDiskFile* >& allFiles )
{
	// Check files
	for ( CDiskFile* file : dir->GetFiles() )
	{
		allFiles.PushBack( file );
	}

	// Recurse
	for ( CDirectory* cur : dir->GetDirectories() )
	{
		CollectLevelFiles( cur, allFiles );
	}
}

Bool CEditorEngine::Initialize()
{
	// analytics init
	ANALYTICS_INIT();

#ifndef RED_FINAL_BUILD
	LOG_EDITOR( TXT( "******** CEditorEngine::Initialise ********" ) );
	Red::MemoryFramework::SystemMemoryStats::LogTotalProcessMemory();
	RED_MEMORY_DUMP_CLASS_MEMORY_REPORT(  "EngineInit" );
	RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "EngineInit" );
#endif
	// Initialize wxWidgets
	GSplash->UpdateProgress( TXT("Initializing wxWidgets...") );
	wxChar** crap = NULL;
	::wxInitialize( 0, crap );

	// HACK: set locale to C, because using comma as separator breaks shader compilation :)
	wxLocale loc;
	loc.Init( wxLANGUAGE_ENGLISH );

	Red::System::Log::Manager::GetInstance().SetEnabled( true );

#ifdef RELEASE
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, false );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );
#endif

	// load analytics config and start collecting screens/events
	ANALYTICS_CALL( LoadConfig( TXT( "google_analytics_config" ) ) );
	ANALYTICS_CALL( StartCollecting() );

	// launcher screen start
	ANALYTICS_SCREEN( "Editor launcher screen" );

	// Check for flags modifying assert flags before any initialization
	const Char* cmdLine = SGetCommandLine();

	CTokenizer cmdTokens( cmdLine, TXT(" ") ); 
	for ( Uint32 index = 0, end = cmdTokens.GetNumTokens(); index != end; ++index ) 
	{	
		String token = cmdTokens.GetToken( index );
		// Disable asserts in the editor
		if ( token == TXT( "noassert" ) || token == TXT( "-noassert" ) )
		{
			LOG_EDITOR( TXT("EditorEngine: Assertions has been DISABLED") );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );

			ANALYTICS_EVENT( "InitFlags", "noassert" );
		}
		// Enable silent crashes
		else if ( token == TXT( "silentcrash" ) || token == TXT( "-silentcrash" ) )
		{
			LOG_EDITOR( TXT("EditorEngine: Silent crash mode") );

			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_SilentCrashHook, true );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, false );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );

			ANALYTICS_EVENT( "InitFlags", "silentcrash" );
		}
		// QA
		else if ( token == TXT( "qa" ) || token == TXT( "-qa" ) )
		{
			LOG_EDITOR( TXT("QA mode: asserts, log and data asserts turned ON") );
			GDataAsserts = true;
			Red::System::Log::Manager::GetInstance().SetEnabled( true );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, true );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, true );

			ANALYTICS_EVENT( "InitFlags", "qa" );
		}
	}

	InitialiseMemoryBudgets();

#ifndef NO_LOG
	{
		Uint32 logBufferSize = 256;

#define LOG_BUFFER_PARAM TXT( "logbuffer=" )

		const Char* forceScriptsCompilation = Red::System::StringSearch( SGetCommandLine(), LOG_BUFFER_PARAM );

		if( forceScriptsCompilation )
		{
			if( !FromString( forceScriptsCompilation + Red::System::StringLengthCompileTime( LOG_BUFFER_PARAM ), logBufferSize ) )
			{
				logBufferSize = 256;
			}
		}

		GLogOutput = new CLogOutput( logBufferSize );
		GLogOutput->InitThread();
	}
#endif

	// Moved some initialization steps into a separate function
	// So that XRC resources can be accessed earlier
	CEdApp* app = static_cast< CEdApp* >( wxTheApp );
	app->PreInit();

	// Start application
	wxTheApp->OnRun();
	
	// Initialize base engine
	if ( !CBaseEngine::Initialize() )
	{
		return false;
	}


	GSoundSystem->FinalizeLoading();

	ANALYTICS_EVENT_T( "Init", "1. base engine and wx initialized" ); /*, (Uint32)((Double)(EngineTime::GetNow()-timer)*1000.0)*/

	// version
	#ifdef RED_FINAL_BUILD
		ANALYTICS_EVENT( "InitFlags", "Final" );
	#elif RELEASE
		ANALYTICS_EVENT( "InitFlags", "Release" );
	#elif _DEBUG
		ANALYTICS_EVENT( "InitFlags", "Debug" );
	#else
		ANALYTICS_EVENT( "InitFlags", "No opt" );
	#endif

	// bitness
	#ifdef _WIN64
		ANALYTICS_EVENT( "InitFlags", "x64" );
	#else
		ANALYTICS_EVENT( "InitFlags", "x32" );
	#endif

#ifndef RED_FINAL_BUILD
	LOG_EDITOR( TXT( "******** CEditorEngine::Initialise Post-Engine Initialise ********" ) );
	Red::MemoryFramework::SystemMemoryStats::LogTotalProcessMemory();
	RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "PostEngineInit" );
	RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "PostEngineInit" );
#endif

	// read framerate that the movies should be grabbed with
	Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( Config::cvMovieFramerate.Get() );

	m_inputDeviceManager = new CInputDeviceManagerWin32;
	if ( ! m_inputDeviceManager->Init() )
	{
		HALT( "Failed to initialize input device manager" );
		return false;
	}

	RED_ASSERT( GGame );

	//FIXME2<<< Remove before deleting GGame...
	GGame->AddGameInputModeListener( m_inputDeviceManager );

	// Initialize application
	if ( !wxTheApp->CallOnInit() )
	{
		WARN_EDITOR( TXT( "Unable to initialize wxWidgets!" ) );
		return false;
	}

	// Create console instance
	const Bool noDebugConsole = wxTheFrame->HasCommandLineParameter( TXT( "nodebugconsole" ) );
	if ( noDebugConsole )
	{
		ANALYTICS_EVENT( "InitFlags", "nodebugconsole" );
	}
	if ( !noDebugConsole )
	{
		const Bool noLogToConsole = !wxTheFrame->HasCommandLineParameter( TXT( "nologtoconsole" ) );
		if ( noLogToConsole )
		{
			ANALYTICS_EVENT( "InitFlags", "noLogToConsole" );
		}

		GDebugConsole = new CDebugConsole( noLogToConsole );
	}

	// Raise priority of editor thread
	String priority;
	int pri = 0;
	if ( wxTheFrame->GetCommandLineParameter( TXT("priority"), priority ) )
	{
		if( priority == TXT("lowest") )
		{
			::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST ); pri = -2;
		}
		else
		if( priority == TXT("lower") )
		{
			::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL ); pri = -1;
		}
		else
		if( priority == TXT("normal") )
		{
			::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_NORMAL );
		}
		else
		if( priority == TXT("higher") )
		{
			::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL ); pri = 1;
		}
		else
		if( priority == TXT("highest") )
		{
			::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_HIGHEST ); pri = 2;
		}
	}
	ANALYTICS_EVENT_V( "InitFlags", "priority", pri );

	// Disable asserts in the editor
	if ( wxTheFrame->HasCommandLineParameter( TXT("nodataassert") ) || wxTheFrame->HasCommandLineParameter( TXT("-nodataassert") )  )
	{
		LOG_EDITOR( TXT("EditorEngine: Data Assertions has been DISABLED") );
		GDataAsserts = false;

		ANALYTICS_EVENT( "InitFlags", "nodataassert" );
	}

	// Disable logging in the editor if needed, but log everything that came out of initialization of BaseEngine
	if ( wxTheFrame->HasCommandLineParameter( TXT("nolog") ) )
	{
		LOG_EDITOR( TXT("EditorEngine: Logging has been DISABLED") );
		Red::System::Log::Manager::GetInstance().SetEnabled( false );

		ANALYTICS_EVENT( "InitFlags", "nolog" );
	}

	// engine init flagss
	if ( wxTheFrame->HasCommandLineParameter( TXT( "useCookedLocale" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "UseCookedLocale" );
	}
	if ( wxTheFrame->HasCommandLineParameter( TXT( "nostring" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "NoStrings" );
	}
	if ( wxTheFrame->HasCommandLineParameter( TXT( "nosourcecontrol" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "NoSourceControl" );
	}
	if ( wxTheFrame->HasCommandLineParameter( TXT( "forcescriptcompilation" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "ForceScriptCompilation" );
	}
	if ( wxTheFrame->HasCommandLineParameter( TXT( "usebundles" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "UseBundles" );
	}
	if ( wxTheFrame->HasCommandLineParameter( TXT( "nosound" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "NoSound" );
	}
	if ( wxTheFrame->HasCommandLineParameter( TXT( "dumprtti" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "DumpRTTI" );
	}
	if ( wxTheFrame->HasCommandLineParameter( TXT( "compile_nonstrict" ) ) )
	{
		ANALYTICS_EVENT( "InitFlags", "Compile_NoStrict" );
	}

	// Logs
	{
		const Char* const OnOff[] =
		{
			TXT( "OFF" ),
			TXT( "ON" )
		};

		LOG_EDITOR(	TXT( "EditorEngine : logging is %s" ), OnOff[ !!Red::System::Log::Manager::GetInstance().IsEnabled() ] );

		// "Red Error System error flag to int bool"
#define EFLAGTOIB( flag ) ( Red::System::Error::Handler::GetInstance()->HasAssertFlag( Red::System::Error::flag )? 1 : 0 )

		LOG_EDITOR( TXT( "EditorEngine : Error Sytem configuration:" ) );
		LOG_EDITOR( TXT( "Asserts Disabled: %s" ), OnOff[ EFLAGTOIB( AC_ContinueAlways ) ] );
		if( !Red::System::Error::Handler::GetInstance()->HasAssertFlag( Red::System::Error::AC_ContinueAlways ) )
		{
			LOG_EDITOR( TXT( "Show assert popup: %s" ), OnOff[ EFLAGTOIB( AC_PopupHook ) ] );
			LOG_EDITOR( TXT( "Hide Crash Handler: %s" ), OnOff[ EFLAGTOIB( AC_SilentCrashHook ) ] );
			LOG_EDITOR( TXT( "Verbose asserts: %s" ), OnOff[ EFLAGTOIB( AC_Verbose ) ] );
			LOG_EDITOR( TXT( "Log asserts: %s" ), OnOff[ EFLAGTOIB( AC_PrintToLog ) ] );
		}
		LOG_EDITOR( TXT( "Data Asserts Disabled: %s" ), OnOff[ GDataAsserts ? 0 : 1 ] );
	}

#if 0
	// Resave custom shit
	{
		CDirectory* levelDir = GDepot->FindDirectory( TXT("levels\\novigrad\\") );
		LOG_EDITOR( TXT("Resave dir: '%ls'"), levelDir->GetDepotPath().AsChar() );
		
		// Process the meshes
		TDynArray< CDiskFile* > resavedFiles;
		CollectLevelFiles( levelDir, resavedFiles );
		LOG_EDITOR( TXT("Found %i files to resave"), resavedFiles.Size() );

		GFeedback->BeginTask( TXT("Resaving files..."), false );

		for ( Uint32 i=0; i<resavedFiles.Size(); ++i )
		{
			GFeedback->UpdateTaskInfo( TEXT("Resaving '%s'..."), resavedFiles[i]->GetDepotPath().AsChar() );
			GFeedback->UpdateTaskProgress( i, resavedFiles.Size() );

			if ( resavedFiles[i]->Load() )
			{
				CResource* res = resavedFiles[i]->GetResource();
				if ( res )
				{
					res->GetFile()->CheckOut();
					res->GetFile()->Save();
				}
			}

			if ( (i % 50) == 0 )
			{
				SGarbageCollector::GetInstance().CollectNow();
			}
		}

		GFeedback->EndTask();
	}
#endif

#if 0
	// Resave meshes that have the bone count over the limit
	{
		// Process the meshes
		TDynArray< CDiskFile* > meshFiles;
		CollectMeshFiles( GDepot, meshFiles );
		LOG_EDITOR( TXT("Found %i mesh files"), meshFiles.Size() );

		GFeedback->BeginTask( TXT("Scanning meshes..."), false );

		Uint32 numMeshesToResave = 0;
		for ( Uint32 i=0; i<meshFiles.Size(); ++i )
		{
			GFeedback->UpdateTaskInfo( TEXT("Loading '%s'..."), meshFiles[i]->GetDepotPath().AsChar() );
			GFeedback->UpdateTaskProgress( i, meshFiles.Size() );

			if ( meshFiles[i]->Load() )
			{
				CMesh* mesh = Cast< CMesh >( meshFiles[i]->GetResource() );
				if ( mesh && mesh->TestSkinningLimits() )
				{
					LOG_ALWAYS( TXT("[%i] Mesh %s needs resaving"), i, meshFiles[i]->GetDepotPath().AsChar() );
					++numMeshesToResave;

					if ( mesh->GetFile()->CheckOut() )
					{
						mesh->MergeChunks();
						mesh->GetFile()->Save();
					}
				}
			}

			if ( (i % 50) == 0 )
			{
				SGarbageCollector::GetInstance().CollectNow();
			}
		}

		GFeedback->EndTask();


		LOG_EDITOR( TXT("%i meshes needs resaving"), numMeshesToResave );
	}
#endif

	// Fix entity templates
#if 0
	// Check every entity template
	FILE* f = fopen( "D:\\files.txt","w");
	{
		// Process the entity templates
		TDynArray< CDiskFile* > diskFiles;
		CollectEntityTemplateFiles( GDepot, diskFiles );
		LOG_EDITOR( TXT("Found %i entity template files"), diskFiles.Size() );

		GFeedback->BeginTask( TXT("Scanning templates..."), false );

		Uint32 problematicCrap = 0;
		for ( Uint32 i=0; i<diskFiles.Size(); ++i )
		{
			GFeedback->UpdateTaskInfo( TEXT("Loading '%s'..."), diskFiles[i]->GetDepotPath().AsChar() );
			GFeedback->UpdateTaskProgress( i, diskFiles.Size() );

			if ( diskFiles[i]->Load() )
			{
				// Check for waypoints
				CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( diskFiles[i]->GetResource() );
				if ( entityTemplate )
				{
					// Get the entity
					CEntity* entity = entityTemplate->GetEntityObject();
					if ( entity && !entity->IsA< CActionPoint >() )
					{
						const TDynArray< CComponent* >& components = entity->GetComponents();
						for ( Uint32 i=0; i<components.Size(); ++i )
						{
							CWayPointComponent* wc = Cast< CWayPointComponent >( components[i] );
							if ( wc )
							{
								if ( wc->IsUsedByPathEngine() )
								{
									LOG_EDITOR( TXT("!!!!!!! Component '%s' in template '%s' uses pathengine !!!!!!"), wc->GetName().AsChar(), entityTemplate->GetDepotPath().AsChar() );
									++problematicCrap;

									fwprintf( f, TXT("Component '%s' in template '%s' uses pathengine\n"), wc->GetName().AsChar(), entityTemplate->GetDepotPath().AsChar() );
									fflush( f );
								}
							}
						}
					}
				}				
			}
			
			if ( (i % 50) == 0 )
			{
				SGarbageCollector::GetInstance().CollectNow();
			}
		}

		fclose( f );

		LOG_EDITOR( TXT("%i components using path engine"), problematicCrap );

		GFeedback->EndTask();
	}

#endif

#if !defined( NO_TELEMETRY )	
	// here telemetry should be created
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
	{
		telemetrySystem->InitializeDebugWindows();
	}
#endif //NO_TELEMETRY

#ifndef RED_FINAL_BUILD
	LOG_EDITOR( TXT( "******** CEditorEngine::Initialize (complete) ********" ) );
	Red::MemoryFramework::SystemMemoryStats::LogTotalProcessMemory();
	RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "CompleteEngineInit" );
	RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "CompleteEngineInit" );
#endif

	// activate DLCs
	GCommonGame->GetDLCManager()->OnEditorStarted();

	m_postInitialized = true;
	return true;
}

void CEditorEngine::Shutdown()
{
	RED_FATAL_ASSERT( GCommonGame, "CEditorEngine::Shutdown(): GCommonGame must not be nullptr." );

	// deactivate DLCs
	GCommonGame->GetDLCManager()->OnEditorStopped();

#ifndef NO_TEST_FRAMEWORK
	STestFramework::GetInstance().OnFinish();
#endif
	// Shut down console
	if ( GDebugConsole )
	{
		delete GDebugConsole;
		GDebugConsole = NULL;
	}

	// Removing all thumbnails attached to the diskfiles
	GDepot->RemoveThumbnails();

#ifndef NO_LOG
	// Destroy log output
	if ( GLogOutput )
	{
		GLogOutput->RequestExit();
		GLogOutput->JoinThread();
		delete GLogOutput;
		GLogOutput = NULL;
	}
#endif // NO_LOG

	// Discard all thumbnail images
	CThumbnail::DiscardThumbnailImages();

	// Cleanup 
	wxUninitialize();

	// Dump registered listeners (should be none)
	SEvents::GetInstance().DumpRegisteredListeners();

	// Clear listeners
	SEvents::GetInstance().Clear();

	if ( m_inputDeviceManager )
	{
		m_inputDeviceManager->Shutdown();
		m_inputDeviceManager = nullptr;
	}

	// Shut down base engine
	CBaseEngine::Shutdown();
}

// This external method can be used to toggle fullscreen of the editor's main rendering viewport
extern void ToggleFullscreen();

void CEditorEngine::Tick( Float timeDelta )
{
	PC_SCOPE( EditorEngineTick );

#ifndef DISABLE_LOW_MEMORY_WARNINGS
	const Red::System::Uint64 c_warningLevel = 1024 * 1024 * 100;					// If we have < this value, output a warning 
	static Bool s_errorMessageDisplayed = false;
	if( !s_errorMessageDisplayed && Red::MemoryFramework::SystemMemoryStats::WarnOnLowMemory( c_warningLevel ) )
	{
		Red::MemoryFramework::SystemMemoryInfo memInfo = Red::MemoryFramework::SystemMemoryStats::RequestStatistics();
		Red::System::AnsiChar lowMemoryMessage[512] = {'\0'};
		Red::System::SNPrintF( lowMemoryMessage, ARRAY_COUNT( lowMemoryMessage ), "Warning! The system is dangerously low on memory - you may get crashes after this point!\n\n"
																				  "Virtual Memory: %2.1fMB / %2.1fMB remaining", 
																				  (memInfo.m_totalVirtualBytes - memInfo.m_freeVirtualBytes) / (1024.0f * 1024.0f), 
																				  memInfo.m_totalVirtualBytes / (1024.0f * 1024.0f) );
		wxMessageBox( lowMemoryMessage, "Editor is low on memory", wxOK | wxICON_EXCLAMATION, wxTheFrame );
		s_errorMessageDisplayed = true;
	}
#endif

	// Process wxWindows messages
	CEdApp* app = static_cast< CEdApp* >( wxTheApp );
	if ( !app->ProcessWxMessages() )
	{
		// Say bye bye
		RequestExit();
	}

	// Check if the user wishes to toggle the fullscreen mode.
	// It was necessary to inject this code here, because the input
	// is handled in a different way in the editor mode and during the game - and
	// we want a single input checking code that toggles fullscreen on/off.
	if ( RIM_IS_KEY_DOWN( IK_Alt ) && RIM_KEY_JUST_PRESSED( IK_Enter ) )
	{
		ToggleFullscreen();
	}

	// Check if we want to reload shaders.
	if ( RIM_KEY_JUST_PRESSED( IK_F11 ) )
	{
		if ( RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			GRender->ReloadEngineShaders();
		}
		else
		{
			GRender->ReloadSimpleShaders();
		}
	}

	// Tick base engine
	CBaseEngine::Tick( timeDelta );

	// Internal editor tick
	SEvents::GetInstance().DispatchEvent( CNAME( EditorTick ), CreateEventData( timeDelta ) );

	// Flush log
	Red::System::Log::Manager::GetInstance().Flush();

	// Suppress application CPU burning if it's not active
	if ( !app->IsActive() && ( GGame == NULL || !GGame->DoNotPauseWhileStopped() ) )
	{
		Sleep( 100 );
	}
 }

ECompileScriptsReturnValue CEditorEngine::OnCompilationFailed( const CScriptCompilationMessages& errorCollector )
{
	CEdScriptCompilationErrorsDialog* errorDialog = new CEdScriptCompilationErrorsDialog( wxTheFrame, errorCollector );

	// Hack mini loop, since we've not finished initialising the system by this point
	CEdApp* app = static_cast< CEdApp* >( wxTheApp );
	while( errorDialog->IsShown() )
	{
		// Process wxWidgets
		app->ProcessWxMessages();

#ifdef RED_NETWORK_ENABLED
		if( m_scriptHandler.ReloadRequested() )
		{
			m_scriptHandler.RequestReload( false );

			errorDialog->Hide();
			errorDialog->Destroy();

			return CSRV_Recompile;
		}
#endif // RED_NETWORK_ENABLED

	}

	ECompileScriptsReturnValue retval = errorDialog->GetUserChoice();

	errorDialog->Destroy();

	return retval;
}
