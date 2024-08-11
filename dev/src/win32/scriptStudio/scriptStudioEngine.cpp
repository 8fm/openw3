/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

CScriptStudioEngine::CScriptStudioEngine()
{
}

Bool CScriptStudioEngine::Initialize()
{
	// Initialize wxWidgets
	::wxInitialize( 0, NULL );

	// Initialize base engine
	if ( !CBaseEngine::Initialize() )
	{
		return false;
	}

	// Initialize application
	if ( !wxTheApp->CallOnInit() )
	{
		WARN( TXT("Unable to initialize wxWidgets !") );
		return false;
	}

	// Create console instance
	GDebugConsole = new CDebugConsole();
	GDebugConsole->AddToRootSet();

	// Start application
	wxTheApp->OnRun();
	return true;
}

void CScriptStudioEngine::Shutdown()
{
	// Shut down profiler
	GProfilerOutput->RemoveFromRootSet();
	GProfilerOutput = NULL;

	// Shut down console
	GDebugConsole->RemoveFromRootSet();
	GDebugConsole = NULL;

	// Removing all thumbnails attached to the diskfiles
	GDepot->RemoveThumbnails();

	wxTheApp->OnExit();

	// Cleanup 
	wxUninitialize();

	// Shut down base engine
	CBaseEngine::Shutdown();
}

void CScriptStudioEngine::Tick( Float timeDelta )
{
	// Process wxWindows messages
	CSSApp* app = static_cast< CSSApp* >( wxTheApp );
	if ( !app->ProcessWxMessages() )
	{
		// Say bye bye
		RequestExit();
	}

	// Tick base engine
	CBaseEngine::Tick( timeDelta );

	// Flush log
	SLog::GetInstance().FlushRecursive();
}
