/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "googleanalytics.h"

#include "../../common/core/feedback.h"

// include compiled XRC resources
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editorXRC.hpp)
namespace EditorQuestsXRC
{
	#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_questsXRC.hpp)
}
namespace EditorJournalXRC
{
	#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_journalXRC.hpp)
}
namespace EditorCharacterDbXRC
{
	#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_character_dbXRC.hpp)
}
namespace EditorAIXRC
{
	#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_aiXRC.hpp)
};
namespace EditorBehXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_behXRC.hpp)
};
namespace EditorCutSceneXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_cutsceneeditorXRC.hpp)
};
namespace EditorParticleEditorXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_particleeditorXRC.hpp)
};
namespace EditorToolsXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_toolsXRC.hpp)
};
namespace EditorJobXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_jobXRC.hpp)
};
namespace EditorGUIResourceXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_guiresourceXRC.hpp)
};
namespace EditorFoliageGeneratorXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_foliage_generatorXRC.hpp)
};
namespace EditorDialogsXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_dialogsXRC.hpp)
};
namespace EditorErrorDialogsXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_errorDialogsXRC.hpp)
};

// terrain tool V2.0
namespace EditorTerrainToolXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_terrainXRC.hpp)
};
namespace EditorVegetationToolXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_vegetationXRC.hpp)
}
namespace EditorTextureArraysViewerXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_tarraysXRC.hpp)
};
namespace EditorPathlibXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_pathlibXRC.hpp)
};

namespace EditorAssetBrowserXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_assetbrowserXRC.hpp)
};

namespace EditorEntityXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_entityXRC.hpp)
}

namespace EditorSceneImporterXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_scene_importerXRC.hpp)
};

namespace EditorMaterialEditorXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_materialeditorXRC.hpp)
};

namespace EditorTextureViewerXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_textureviewerXRC.hpp)
};

namespace EditorSwfViewerXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_swfviewerXRC.hpp)
};

namespace EditorMeshEditorXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_mesheditorXRC.hpp)
};

namespace EditorEnvironmentsXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_environmentsXRC.hpp)
};

namespace EditorScreenshotXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_screenshotXRC.hpp)
};

namespace EditorEncountersXRC
{
#include RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_encountersXRC.hpp)
}

namespace EditorLootXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_lootXRC.hpp)
}

#ifndef NO_MARKER_SYSTEMS
namespace EditorReviewSystemXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_review_systemXRC.hpp)
};
#endif // NO_MARKER_SYSTEMS

#ifndef NO_DATA_VALIDATION
namespace EditorDataErrorReporterXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_dataErrorReporterXRC.hpp)
};

#include "dataError.h"

#endif // NO_DATA_VALIDATION

namespace EditorMaterialMapping
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_materialMappingXRC.hpp)
};

namespace EditorWorldSceneDebugger
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_worldSceneDebuggerXRC.hpp)
};

namespace EditorUmbraXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_umbraXRC.hpp)
}

namespace EditorObjectInspectorXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_objectInspectorXRC.hpp)
}

namespace EditorStripesXRC
{
#include  RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_stripesXRC.hpp)
}

namespace EditorEntityMeshGenXRC
{
#include RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_entityMeshGenXRC.hpp)
}

namespace EditorEntityProxySetupXRC
{
#include RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\editor_entityProxySetupXRC.hpp)
}

#include "textCtrlExXmlHandler.h"

Red::Threads::CMutex CEdApp::m_runLatersMutex;
Red::Threads::CMutex CEdApp::m_parallelMutex;

IMPLEMENT_APP_NO_MAIN( CEdApp )

BEGIN_EVENT_TABLE(CEdApp, wxApp)
	EVT_IDLE(CEdApp::OnIdle)
    EVT_ACTIVATE_APP(CEdApp::OnActivateApp)
END_EVENT_TABLE()

CEdApp::~CEdApp()
{
	Gdiplus::GdiplusShutdown( m_gdiToken );
}

void CEdApp::PreInit()
{
	GSplash->UpdateProgress( TXT("Preinitializing application...") );

	SetVendorName( TXT("CD Projekt Red") );
	SetAppName( TXT("LavaEditor") );

	// Init XRC stuff
	wxInitAllImageHandlers();

	// Add wxTextCtrlEx Handler (overrides wxTextCtrlXmlHandler)
	wxXmlResource::Get()->AddHandler(new wxTextCtrlExXmlHandler);

	wxXmlResource::Get()->InitAllHandlers();

	// Add wxTreeListCtrl Handler
	wxXmlResource::Get()->AddHandler( new wxTreeListCtrl_XmlHandler() ); 

	// Add wxGradientCtrl Handler
	wxXmlResource::Get()->AddHandler( new wxGradientCtrl_XmlHandler() ); 

	InitXmlResource();
	EditorAIXRC::InitXmlResource();
	EditorQuestsXRC::InitXmlResource();
	EditorJournalXRC::InitXmlResource();
	EditorCharacterDbXRC::InitXmlResource();
	EditorBehXRC::InitXmlResource();
	EditorCutSceneXRC::InitXmlResource();
	EditorParticleEditorXRC::InitXmlResource();
	EditorToolsXRC::InitXmlResource();
	EditorJobXRC::InitXmlResource();
	EditorGUIResourceXRC::InitXmlResource();
	EditorFoliageGeneratorXRC::InitXmlResource();
	EditorDialogsXRC::InitXmlResource();
	EditorErrorDialogsXRC::InitXmlResource();
	EditorTerrainToolXRC::InitXmlResource();
	EditorVegetationToolXRC::InitXmlResource();
	EditorTextureArraysViewerXRC::InitXmlResource();
	EditorPathlibXRC::InitXmlResource();
	EditorAssetBrowserXRC::InitXmlResource();
	EditorMaterialEditorXRC::InitXmlResource();
	EditorTextureViewerXRC::InitXmlResource();
	EditorSwfViewerXRC::InitXmlResource();
	EditorMeshEditorXRC::InitXmlResource();
	EditorEntityXRC::InitXmlResource();
	EditorSceneImporterXRC::InitXmlResource();
	EditorUmbraXRC::InitXmlResource();
	EditorEnvironmentsXRC::InitXmlResource();
	EditorScreenshotXRC::InitXmlResource();
	EditorEncountersXRC::InitXmlResource();
	EditorLootXRC::InitXmlResource();
	#ifndef NO_MARKER_SYSTEMS
		EditorReviewSystemXRC::InitXmlResource();
	#endif
#ifndef NO_DATA_VALIDATION
	EditorDataErrorReporterXRC::InitXmlResource();
#endif // NO_DATA_VALIDATION
	EditorMaterialMapping::InitXmlResource();
	EditorWorldSceneDebugger::InitXmlResource();
	EditorObjectInspectorXRC::InitXmlResource();
	EditorStripesXRC::InitXmlResource();
	EditorEntityMeshGenXRC::InitXmlResource();
	EditorEntityProxySetupXRC::InitXmlResource();

	// Initialize feedback system
	extern void InitWin32FeedbackSystem();
	InitWin32FeedbackSystem();

#ifndef NO_DATA_VALIDATION
	GDataError = new CDataErrorSystem();
#endif
}

bool CEdApp::OnInit()
{
	// single instance check
	GSplash->UpdateProgress( TXT( "Checking for multiple instances..." ) );

	m_processMutex = new wxSingleInstanceChecker();

	if( m_processMutex->IsAnotherRunning() )
	{
		if( wxMessageBox( TXT( "Would you like to force it to shut down?\nWarning, any unsaved work will be lost!" ), TXT( "The Editor is already running" ), wxYES_NO ) == wxYES )
		{
			GSplash->UpdateProgress( TXT( "Attempting to close previous instance..." ) );

			DWORD processId = FindAssociatedProcessWithName( TXT( "editor" ) );

			if( processId != -1 )
			{
				// Get a handle to the process.
				HANDLE processHandle = OpenProcess( PROCESS_TERMINATE, FALSE, processId );

				if( processHandle != NULL )
				{
					TerminateProcess( processHandle, 0 );

					while( WaitForSingleObject( processHandle, 10000 ) == WAIT_TIMEOUT )
					{
						if( wxMessageBox( TXT( "The existing instance is taking a long time to shut down, keep waiting?" ), TXT( "Timeout occured" ), wxYES_NO ) == wxNO )
						{
							break;
						}
					}

					CloseHandle( processHandle );
				}
				else
				{
					wxMessageBox( TXT( "Could not close existing editor" ), TXT( "Error" ), wxOK );
					return false;
				}
			}
		}
	}
	ANALYTICS_EVENT_T( "Init", "2. single instance checked" );

	
	// Touch importer classes
	GSplash->UpdateProgress( TXT("Initializing application...") );
	extern void RegisterImportClasses();
	RegisterImportClasses();
	ANALYTICS_EVENT_T( "Init", "3. application initialized" );


	// Initialize GDI plus
	GSplash->UpdateProgress( TXT("Initializing GDI+...") );
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup( &m_gdiToken, &gdiplusStartupInput, NULL );
	ANALYTICS_EVENT_T( "Init", "4. GDI initialized" );


	// Create main frame
	GSplash->UpdateProgress( TXT("Loading main frame...") );
	wxTheFrame = new CEdFrame();
	SetTopWindow( wxTheFrame );
	wxTheFrame->SetIcon( wxICON( amain ) );
	wxTheFrame->Show();
	ANALYTICS_EVENT_T( "Init", "6. main frame loaded" );
	

	// loading finished - let's start edit mode
	GSplash->UpdateProgress( TXT("HideSplash") );
	ANALYTICS_SCREEN( "Editor edit mode" );

	// Initialized
	return true;
}

int CEdApp::OnExit()
{
#ifndef NO_DATA_VALIDATION
	if( GDataError != nullptr )
	{
		delete GDataError;
		GDataError = nullptr;
	}
#endif

	m_mainLoop->Exit();
	delete m_mainLoop;

	int retCode = wxApp::OnExit();
	return retCode;
}

/// Custom message processor
class CFakeEventLoop : public wxEventLoop
{
	DWORD			m_lastWheelTime;
	HWND			m_lastWheelHWND;

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
public:	
	// start the event loop, return the exit code when it is finished
	virtual int Run()
	{ 
		// Activate this event loop
		wxEventLoop::SetActive( this );

		// This is not used as the main loop is implemented on the engine side
		return 0;
	}

	// exit from the loop with the given exit code
	virtual void Exit( int rc = 0 )
	{
		// Post closing message
		::PostQuitMessage( 0 );
	}

	// Called on next iteration
	virtual void OnNextIteration()
	{
		PC_SCOPE( OnNextIteration );
		wxEventLoop::OnNextIteration();
	};

    virtual void ProcessMessage(WXMSG *msg)
	{
		// redirect the mouse wheel message to the window under the mouse
		// cursor (unless a mouse wheel message happened recently, in which
		// case redirect the message to the last wheel message target window)
		if ( msg->message == WM_MOUSEWHEEL )
		{
			POINT cursorPos;

			// find the window that is under the mouse cursor
 			::GetCursorPos( &cursorPos );
			HWND targetWnd = ::WindowFromPoint( cursorPos );

			// check if the last message was sent 0.3 seconds ago and if so
			// change the target to the previous wheel message target.  This is
			// necessary to avoid sending wheel messages to widgets that "scroll by"
			// the mouse when scrolling a panel
			DWORD wheelTime = ::GetTickCount();
			if ( m_lastWheelHWND && wheelTime - m_lastWheelTime < 300 )
			{
				targetWnd = m_lastWheelHWND;
			}
			
			// save time and target window for the next wheel event
			m_lastWheelTime = wheelTime;
			m_lastWheelHWND = targetWnd;

			// make sure the window belongs to the editor's process
			// and if not, invalidate it
			DWORD winPID = 0;
			GetWindowThreadProcessId( targetWnd, &winPID);
			if ( winPID != GetCurrentProcessId() )
			{
				targetWnd = 0;
			}

			// check if we have a valid window
			if ( targetWnd )
			{
				// modify the message with the final target
				msg->hwnd = targetWnd;
			}
			else
			{
				// if we don't have a valid window handle, cancel the
				// message by replacing it with WM_NULL
				msg->message = WM_NULL;
			}
		}

		// forward the message to wxWidgets
		wxEventLoop::ProcessMessage( msg );
	}
};

int CEdApp::OnRun()
{
	// Create fake message processor
	m_mainLoop = new CFakeEventLoop;
	m_mainLoop->Run();

	// Continue
	return 0;
}

void CEdApp::OnActivateApp( wxActivateEvent& event )
{
/*	if ( m_isAlwaysActive )
		GGame->SetApplicationActive( true );
	else
		GGame->SetApplicationActive( event.GetActive() );*/

    event.Skip();
}

Bool CEdApp::ProcessIdle()
{
	PC_SCOPE( ProcessIdle );
	TDynArray< CEdRunnable* > runnables = m_runLaters;
	Double timeNow = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

	// Check for finished parallel tasks
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( m_parallelMutex );
		for ( Int32 i=m_parallelTasks.SizeInt() - 1; i >= 0; --i )
		{
			SRunParallelInfo* info = m_parallelTasks[i];
			if ( info->finished )
			{
				RunLater( info->afterFinishRunnable );
				delete info;
				m_parallelTasks.RemoveAt( i );
			}
		}
	}

	// Run "run laters"
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_runLatersMutex );
		for ( auto it=runnables.Begin(); it != runnables.End(); ++it )
		{
			CEdRunnable* runnable = *it;

			// Run the runnable only if we're at or past the trigger time
			if ( runnable->GetTriggerTime() <= timeNow )
			{
				// Remove the runnable before running it so it can re-run itself
				m_runLaters.Remove( *it );

				// Run it
				(*it)->Run();
			}
		}
	}

	// Delete them
	for ( auto it=runnables.Begin(); it != runnables.End(); ++it )
	{
		if ( (*it)->GetTriggerTime() <= timeNow )
		{
			delete *it;
		}
	}

	return wxApp::ProcessIdle();
}

bool CEdApp::ProcessWxMessages()
{
	PC_SCOPE( ProcessWxMessages );

    // give them the possibility to do whatever they want
    ((CFakeEventLoop*)m_mainLoop)->OnNextIteration();
	
    // Do idle job
    {
		PC_SCOPE( DoIdleJob );
        while ( !Pending() && ProcessIdle() ) {};
    }

    // a message came or no more idle processing to do, sit in
    // Dispatch() waiting for the next message
	{
		PC_SCOPE( ProcessAndWait );
		Int32 counter = 0;
		const Int32 MAX_TO_PROCESS = 50;
	    while ( Pending() && counter < MAX_TO_PROCESS )
	    {
	        // Dispatch message
	        if ( !Dispatch() )
	        {
	            // we got WM_QUIT
	            return false;
	        }
	
			++counter;
	    }
	}

	// Needed to handle destroyed objects
	{
		PC_SCOPE( ProcessPendingEvents );
		ProcessPendingEvents();
	}

    // Process idle, one time
    {
		PC_SCOPE( ProcessIdle );
        ProcessIdle();
    }

    // Continue
    return true;
}

void CEdApp::RunLater( CEdRunnable* runnable )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_runLatersMutex );
	m_runLaters.PushBackUnique( runnable );
}

void CEdApp::RunLaterOnce( CEdRunnable* runnable )
{
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_runLatersMutex );

		// Make sure there is not already an instance of the runnable's class
		for ( auto it=m_runLaters.Begin(); it != m_runLaters.End(); ++it )
		{
			if ( (*it)->IsDuplicate( runnable ) )
			{
				delete runnable;
				return;
			}
		}
	}

	RunLater( runnable );
}

DWORD WINAPI CEdApp::RunParallelThreadProc( LPVOID lpParameter )
{
	SRunParallelInfo* runParallelInfo = reinterpret_cast< SRunParallelInfo* >( lpParameter );

	// Sleep the specified trigger time (if any)
	if ( runParallelInfo->taskRunnable->GetTriggerTime() > 0.0 )
	{
		::Sleep( static_cast< DWORD >( 1000.0*runParallelInfo->taskRunnable->GetTriggerTime() ) );
	}

	// Run the task
	runParallelInfo->taskRunnable->Run();

	// Mark the entry as finished
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( m_parallelMutex );
		runParallelInfo->finished = true;
	}

	return 0;
}

void CEdApp::RunParallel( CEdRunnable* taskRunnable, CEdRunnable* afterFinishRunnable )
{
	// Allocate the parallel task structure
	SRunParallelInfo* runParallelInfo = new SRunParallelInfo;
	runParallelInfo->taskRunnable = taskRunnable;
	runParallelInfo->afterFinishRunnable = afterFinishRunnable;
	runParallelInfo->finished = false;

	// Add entry in parallel tasks array
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( m_parallelMutex );
		m_parallelTasks.PushBack( runParallelInfo );
	}

	// Spawn the thread for the task
	::CloseHandle( ::CreateThread( NULL, 0, RunParallelThreadProc, runParallelInfo, 0, NULL ) );
}

void CEdApp::SetAlwaysActive( Bool active /*= true */ )
{

}

#ifndef NO_ASSERTS
void CEdApp::OnAssertFailure( const wxChar* file, int line, const wxChar* func, const wxChar* cond, const wxChar* msg )
{
	// It looks like wxWidgets doesn't let you account for different asserts, so once a user hits continue always, that's what'll always happen
	static Red::System::Error::EAssertAction lastAction = Red::System::Error::AA_Break;
	if ( lastAction != Red::System::Error::AA_ContinueAlways )
	{
		lastAction = Red::System::Error::HandleAssertion( file, line, cond, TXT( "[wxWidgets]: %s" ), msg );
		if ( lastAction == Red::System::Error::AA_Break )
		{
			RED_BREAKPOINT();
		}
	}
}
#endif
