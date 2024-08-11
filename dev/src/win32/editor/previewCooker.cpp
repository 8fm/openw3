#include "build.h"
#include "previewCooker.h"
#include "../../common/core/dependencyMapper.h"

#include <TlHelp32.h>

//#define COOK_FOR_XBOX

#ifdef COOK_FOR_XBOX
#include "xbdm.h"
#pragma comment ( lib, "..\\..\\..\\external\\xdk\\lib\\xbdm.lib" )
#endif
#include "../../common/core/diskFile.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"

BEGIN_EVENT_TABLE( CEdPreviewCooker, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID("btnCook"), CEdPreviewCooker::OnStartCooking )
	EVT_BUTTON( XRCID("btnDeploy"), CEdPreviewCooker::OnDeployCook )
	EVT_BUTTON( XRCID("btnRun"), CEdPreviewCooker::OnRunCook )
	EVT_BUTTON( XRCID("btnKill"), CEdPreviewCooker::OnKillCooking )
	EVT_BUTTON( XRCID("btnReset"), CEdPreviewCooker::OnResetDevkit )
	EVT_CLOSE( CEdPreviewCooker::OnCloseWindow )
	EVT_TIMER( 12345, CEdPreviewCooker::OnTaskEnded )
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////

/// Cooking task
struct CookingTask
{
	String					m_info;
	String					m_directory;
	String					m_executable;
	String					m_commandLine;
	bool					m_canFail;
};

/// Cooking settings
struct CookingSettings
{
	ECookingPlatform			m_platform;
	TDynArray< CookingTask >	m_tasks;
};

//////////////////////////////////////////////////////////////////////////////

/// Cooking thread
class CEdCookingThread : public Red::Threads::CThread
{
private:
	Bool				m_requestExit;
	Bool				m_hasEnded;
	Bool				m_hadErrors;
	CookingSettings		m_settings;
	wxTextCtrl*			m_output;
	HANDLE				m_stdOutRead;
	HANDLE				m_stdOutWrite;

public:
	//! Has the thread ended?
	RED_INLINE Bool HasEnded() const { return m_hasEnded; }

	//! Did we have any errors running commands ?
	RED_INLINE Bool HadErrors() const { return m_hadErrors; }

public:
	CEdCookingThread( wxTextCtrl* log, const CookingSettings& settings )
		: Red::Threads::CThread( "CookingThread" )
		, m_settings( settings )
		, m_requestExit( false )
		, m_hasEnded( false )
		, m_hadErrors( false )
		, m_output( log )
		, m_stdOutRead( NULL )
		, m_stdOutWrite( NULL )
	{
	}

	~CEdCookingThread()
	{
		// Close write handle as we won't use it anymore
		if ( m_stdOutWrite )
		{
			CloseHandle( m_stdOutWrite );
			m_stdOutWrite = NULL;
		}

		// Close the read handle as we won't use it anymore
		if ( m_stdOutRead )
		{
			CloseHandle( m_stdOutRead );
			m_stdOutRead = NULL;
		}
	}

	void PrintLog( const Char* message, ... )
	{
		// Format
		va_list arglist;
		va_start(arglist, message);
		Char formattedMessage[4096];
		Red::System::VSNPrintF(formattedMessage,  ARRAY_COUNT(formattedMessage), message, arglist); 

		// Print
		m_output->AppendText( formattedMessage );
	}

	virtual void ThreadFunc()
	{
		// Run tasks
		for ( Uint32 i=0; i<m_settings.m_tasks.Size(); ++i )
		{
			const CookingTask& task = m_settings.m_tasks[i];

			// Stats
			PrintLog( TXT("Starting task '%s'...\n"), task.m_info.AsChar() );

			// Run process
			const Bool status = RunProcess( task.m_executable, task.m_directory, task.m_commandLine );
			if ( !status && !task.m_canFail )
			{
				PrintLog( TXT("Cooking task '%s' failed !\n"), task.m_info.AsChar() );
				break;
			}
		}

		// Inform owner that we have finished
		m_hasEnded = true;
	}

	bool RunProcess( const String& processName, const String& processDirectory, const String& commandLine )
	{
		// Reset handles
		m_stdOutWrite = NULL;
		m_stdOutRead = NULL;

		// Setup security attributes needed by the pipe
		SECURITY_ATTRIBUTES securityAttribs; 

		// Set the bInheritHandle flag so pipe handles are inherited. 
		securityAttribs.nLength = sizeof(SECURITY_ATTRIBUTES); 
		securityAttribs.bInheritHandle = TRUE; 
		securityAttribs.lpSecurityDescriptor = NULL; 

		// Create a pipe for the fxc process's STDOUT. 
		if ( !CreatePipe( &m_stdOutRead, &m_stdOutWrite, &securityAttribs, 0 ) ) 
		{
			WARN_EDITOR( TXT( "Unable to create fxc's process STDOUT pipe" ) );
		}

		// Ensure the read handle to the pipe for STDOUT is not inherited.
		if ( !SetHandleInformation( m_stdOutRead, HANDLE_FLAG_INHERIT, 0 ) )
		{
			WARN_EDITOR( TXT( "Read handle of the fxc process's STDOUT is inherited!" ) );
		}

		// Initialize process startup info
		STARTUPINFOW startupInfo;
		ZeroMemory( &startupInfo, sizeof( STARTUPINFO ) );
		startupInfo.cb = sizeof( STARTUPINFO );

		// Specify redirection handles
		startupInfo.hStdError = m_stdOutWrite;
		startupInfo.hStdOutput = m_stdOutWrite;
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;

		// Process information
		PROCESS_INFORMATION processInformation;    
		ZeroMemory( &processInformation, sizeof( PROCESS_INFORMATION ) );

		// Assemble full commandline
		String processCrap = processName;
		processCrap += TXT(" ");
		processCrap += commandLine;

		// Prepare commandline
		WCHAR processCommandline[ 4096 ];
		wcscpy_s( processCommandline, ARRAYSIZE(processCommandline), processCrap.AsChar() );

		// Spawn shader compiler
		DWORD exitCode = ~0;
		PrintLog( TXT("Starting '%s' in '%s'\n"), processCrap.AsChar(), processDirectory.AsChar() );
		if ( CreateProcessW( NULL, processCommandline, NULL, NULL, true, CREATE_NO_WINDOW, NULL, processDirectory.AsChar(), &startupInfo, &processInformation ) )
		{
			// Wait for compilation
			do
			{
				ProcessOutput();
			}
			while ( WAIT_OBJECT_0 != WaitForSingleObject( processInformation.hProcess, 100 ) );

			// Get exit code
			GetExitCodeProcess( processInformation.hProcess, &exitCode );
			PrintLog( TXT("Process exit code: %i\n"), exitCode );
		}

		// Close write handle as we won't use it anymore
		if ( m_stdOutWrite )
		{
			CloseHandle( m_stdOutWrite );
			m_stdOutWrite = NULL;
		}

		// Close the read handle as we won't use it anymore
		if ( m_stdOutRead )
		{
			CloseHandle( m_stdOutRead );
			m_stdOutRead = NULL;
		}

		// Done, returns true if process ended with success
		return exitCode == 0;
	}

	void ProcessOutput()
	{
		// Read the output from process
		if ( m_stdOutRead )
		{
			DWORD numBytesRead;
			Bool success;
			AnsiChar logBuf[4096];

			// Check data
			for ( ;; )
			{
				// Are there any data to read ?
				DWORD totalBytes = 0;
				PeekNamedPipe( m_stdOutRead, NULL, NULL, NULL, &totalBytes, NULL );
				if ( !totalBytes ) break;

				// Read data
				Red::System::MemorySet( logBuf, 0, sizeof(logBuf) );
				success = ( 0 != ReadFile( m_stdOutRead, logBuf, 4096, &numBytesRead, NULL ) );
				if ( !success || numBytesRead == 0 ) break;
				wxString str = ANSI_TO_UNICODE( logBuf );
				m_output->AppendText( str );
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////////

static wxString ExtractImage2( const wxString& imageName )
{
	wxBitmap image = SEdResources::GetInstance().LoadBitmap( imageName );
	if ( image.IsOk() )
	{
		// Format temp path
		Char buf[256], buf2[256];
		GetTempPath( 256, buf );
		GetTempFileName( buf, TXT("icon"), 0, buf2 );

		// Save image
		wxString fileName( buf2 );
		if ( image.SaveFile( fileName, wxBITMAP_TYPE_PNG ) )
		{
			// Return the file name
			return fileName;
		}
	}

	// Invalid path
	return wxEmptyString;
}

//////////////////////////////////////////////////////////////////////////////

static CEdPreviewCooker* GPreviewCooker = NULL;

CEdPreviewCooker::CEdPreviewCooker( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("CookFrame"), true )
	, m_log( NULL )
	, m_cookingThread( NULL )
	, m_tickTimer( this, 12345 )
	, m_xenonDeployDir( wxT("xe:\\LavaEngine") ) // TODO: change preview cooker
	, m_pcDeployDir( wxT("C:\\LavaEngine") )
{
	// Set the preview cooker
	GPreviewCooker = this;

	// Get the log window
	m_log = XRCCTRL( *this, "LogWindow", wxTextCtrl );

	// Get stat windows
	m_statStatus = XRCCTRL( *this, "CookOperation", wxStaticText );
	m_statProgress = XRCCTRL( *this, "CookProgress", wxGauge );
	m_statNumFiles = XRCCTRL( *this, "statNumFiles", wxStaticText );
	m_statNumErrors = XRCCTRL( *this, "statNumErrors", wxStaticText );
	m_statNumWarnings = XRCCTRL( *this, "statNumWarnings", wxStaticText );
	m_cookReport = XRCCTRL( *this, "CookReport", wxHtmlWindow );

	// Extract images
	m_imageFileError = ExtractImage2( wxT("IMG_FUCKUP1") );
	m_imageFileWarning = ExtractImage2( wxT("IMG_FUCKUP2") );
	m_imageFileKitten = ExtractImage2( wxT("IMG_COOKING_KITTEN") );

	// Get cooking settings
	{
		// The cooker file
		WCHAR processFileName[ 1024 ];
		GetModuleFileNameW( GetModuleHandle( NULL ), processFileName, ARRAYSIZE(processFileName) );
		CFilePath processFilePath( processFileName );

		// Get the path to cooker file
		processFilePath.SetFileName( TXT("wcc") );
		m_cookerFile = processFilePath.ToString();
		LOG_EDITOR( TXT("Cooker file path: '%s'"), m_cookerFile.AsChar() );

		// Get the path to game cooker file
		processFilePath.SetFileName( TXT("wcc-game") );
		m_cookerFileGame = processFilePath.ToString();
		LOG_EDITOR( TXT("Cooker game file path: '%s'"), m_cookerFileGame.AsChar() );

		// Get cooker directory
		processFilePath.SetExtension( String::EMPTY );
		processFilePath.SetFileName( String::EMPTY );
		m_cookerDirectory = processFilePath.ToString();
		LOG_EDITOR( TXT("Cooker directory: '%s'"), m_cookerDirectory.AsChar() );
	}

	// Reset interface
	ResetCookingInterface();

	// Reset report
	m_cookReport->SetPage( wxT("<HTML><BODY>Please start cooking process</BODY></HTML>") );

	// Connect the events
	m_tickTimer.Start( 500, false );

	// Show the dialog
	Show();
	Layout();
}

CEdPreviewCooker::~CEdPreviewCooker()
{
	// Reset preview cooker
	ASSERT( GPreviewCooker == this );
	GPreviewCooker = NULL;
}

void CEdPreviewCooker::ShowFrame()
{
	// Create window
	if ( !GPreviewCooker )
	{
		GPreviewCooker = new CEdPreviewCooker( wxTheFrame );
	}

	// Show window
	GPreviewCooker->Show();
	GPreviewCooker->SetFocus();	
}

void CEdPreviewCooker::ResetCookingInterface()
{
	// Clear log
	m_log->Clear();

	// Reset cooking stats
	m_statNumFiles->SetLabelText( TXT("0") );
	m_statNumErrors->SetLabelText( TXT("0") );
	m_statNumWarnings->SetLabelText( TXT("0") );
	m_statStatus->SetLabelText( TXT("Cooking not started") );
	m_statProgress->SetValue( 0 );

	// Reset report
	m_cookReport->SetPage( wxT("<HTML><BODY>Cooking is in progress...</BODY></HTML>") );

	// Reset values
	m_numFiles = 0;
	m_cookerErrors.Clear();
	m_cookerWarnings.Clear();

	// Enable normal cooking interface
	EnableCookingUI( true );
}

void CEdPreviewCooker::EnableCookingUI( Bool state )
{
	XRCCTRL( *this, "boxPlatform", wxChoice )->Enable( state );
	XRCCTRL( *this, "btnCook", wxButton )->Enable( state );
	XRCCTRL( *this, "btnDeploy", wxButton )->Enable( state );
	XRCCTRL( *this, "btnReset", wxButton )->Enable( state );
	XRCCTRL( *this, "btnRun", wxButton )->Enable( state );
	XRCCTRL( *this, "btnKill", wxButton )->Enable( !state );
	XRCCTRL( *this, "cookingMode", wxRadioBox )->Enable( state );
	XRCCTRL( *this, "cleanBefore", wxCheckBox )->Enable( state );
	XRCCTRL( *this, "cookDeploy", wxCheckBox )->Enable( state );
	XRCCTRL( *this, "cookRun", wxCheckBox )->Enable( state );
}

ECookingPlatform CEdPreviewCooker::GetCookingPlatform()
{
	// Get cooking platform
	wxChoice* platformChoice = XRCCTRL( *this, "boxPlatform", wxChoice );
	return (platformChoice->GetSelection() == 0) ? PLATFORM_XboxOne : PLATFORM_PC; // TODO: support PS4
}

void CEdPreviewCooker::GenerateReport()
{
	wxString code;

	code += wxT("<HTML><BODY>");

	// Summary
	{
		code += wxT("<p>");

		// Header
		code += wxT("<font size=20>");
		code += wxT("Cooking report");
		code += wxT("</font>");
		code += wxT("<hr height=5/>");
		code += wxT("<br/>");

		// Fuckups table
		code += wxT("<table>");	

		// Errors
		{
			code += wxT("<td valign=top align=center width=30>");
			code += wxT("<img src=\"");
			code += m_imageFileError;
			code += wxT("\">");
			code += wxT("</td>");
			code += wxT("<td valign=top>");
			code += String::Printf( TXT("%i Errors(s)<br>"), m_cookerErrors.Size() ).AsChar();
			code += wxT("</td>");
		}

		// Warnings
		{
			code += wxT("<td valign=top align=center width=30>");
			code += wxT("<img src=\"");
			code += m_imageFileWarning;
			code += wxT("\">");
			code += wxT("</td>");
			code += wxT("<td valign=top>");
			code += String::Printf( TXT("%i Warnings(s)<br>"), m_cookerWarnings.Size() ).AsChar();
			code += wxT("</td>");
		}

		code += wxT("</table>");
		code += wxT("</p>");

		// Errors
		if ( m_cookerErrors.Size() )
		{
			code += wxT("<p>");

			// Header
			code += wxT("<font size=20>");
			code += wxT("Errors");
			code += wxT("</font>");
			code += wxT("<hr height=5/>");
			code += wxT("<br/>");

			// Fuckups
			code += wxT("<table border=1 bgcolor=#ff0000>");	
			code += wxT("<tr><td width=500>Problem</td><td width=500>Resource</td></tr>");
			for ( Uint32 i=0; i<m_cookerErrors.Size(); ++i )
			{
				const CookerMessage& msg = m_cookerErrors[i];

				code += wxT("<tr>");

				code += wxT("<td>");
				code += msg.m_message.AsChar();
				code += wxT("</td>");

				code += wxT("<td>");
				if ( !msg.m_resource.Empty() )
				{
					code += String::Printf( TXT("<a href=\"#reserr:%i\">"), i ).AsChar();
					code += msg.m_resource.AsChar();
					code += wxT("</a>");
				}
				else
				{
					code += wxT("Undefined");
				}
				code += wxT("</td>");
	
				code += wxT("</tr>");
			}

			code += wxT("</table>");
			code += wxT("</p>");
		}

		// Warnings
		if ( m_cookerWarnings.Size() )
		{
			code += wxT("<p>");

			// Header
			code += wxT("<font size=20>");
			code += wxT("Warnings");
			code += wxT("</font>");
			code += wxT("<hr height=5/>");
			code += wxT("<br/>");

			// Fuckups
			code += wxT("<table border=1 bgcolor=#ffff00>");	
			code += wxT("<tr><td width=500>Problem</td><td width=500>Resource</td></tr>");
			for ( Uint32 i=0; i<m_cookerWarnings.Size(); ++i )
			{
				const CookerMessage& msg = m_cookerWarnings[i];

				code += wxT("<tr>");

				code += wxT("<td>");
				code += msg.m_message.AsChar();
				code += wxT("</td>");

				code += wxT("<td>");
				if ( !msg.m_resource.Empty() )
				{
					code += String::Printf( TXT("<a href=\"#reswar:%i\">"), i ).AsChar();
					code += msg.m_resource.AsChar();
					code += wxT("</a>");
				}
				else
				{
					code += wxT("Undefined");
				}
				code += wxT("</td>");

				code += wxT("</tr>");
			}

			code += wxT("</table>");
			code += wxT("</p>");
		}
	}

	code += wxT("</HTML></BODY>");

	// Reset report
	m_cookReport->SetPage( code );
}

void CEdPreviewCooker::ClearLog()
{
	// Remove all text from log
	m_log->Clear();
}

void CEdPreviewCooker::PrintLog( const Char* message, ... )
{
	// Format
	va_list arglist;
	va_start(arglist, message);
	Char formattedMessage[1024];
	Red::System::VSNPrintF(formattedMessage,  ARRAY_COUNT(formattedMessage), message, arglist); 

	// Print
	m_log->AppendText( formattedMessage );
}

static void GatherPreviewLayers( CWorld* world, CLayerGroup* group, Bool skipHidden, TDynArray< String >& previewLayerPaths )
{
	// Do not process hidden groups
	if ( skipHidden && group->GetParentGroup() && !group->IsVisibleOnStart() )
	{
		return;
	}

	// Process layers
	const CLayerGroup::TLayerList& layers = group->GetLayers();
	for ( Uint32 i=0; i<layers.Size(); ++i )	
	{
		CLayerInfo* layer = layers[i];
		if ( layer->IsLoaded() && ( layer->IsVisible() || !skipHidden ) )
		{
			// Get layer path
			String groupPath;
			layer->GetHierarchyPath( groupPath, true );

			// Add to the preview layer lists
			previewLayerPaths.PushBackUnique( groupPath );
		}
	}

	// Process sub groups
	const CLayerGroup::TGroupList& subGroup = group->GetSubGroups();
	for ( Uint32 i=0; i<subGroup.Size(); ++i )
	{
		GatherPreviewLayers( world, subGroup[i], skipHidden, previewLayerPaths );
	}
}

void CEdPreviewCooker::OnKillCooking( wxCommandEvent& event )
{
	// Make sure user knows what he's doing
	if ( wxNO == wxMessageBox( wxT("Killing a cook in progress is very unsafe. Are you sure ?"), wxT("Kill cooking"), wxICON_EXCLAMATION | wxYES_NO | wxNO_DEFAULT ) )
	{
		return;
	}

	HANDLE hndl = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	DWORD dwsma = GetLastError();
	DWORD dwExitCode = -6666;
	PROCESSENTRY32  procEntry={0};
	procEntry.dwSize = sizeof( PROCESSENTRY32 );
	Process32First(hndl,&procEntry);
	do
	{
		if(!Red::System::StringCompare(procEntry.szExeFile,L"wcc.exe"))
		{
			HANDLE hHandle;
			hHandle = ::OpenProcess(PROCESS_ALL_ACCESS,0,procEntry.th32ProcessID);

			::GetExitCodeProcess(hHandle,&dwExitCode);
			::TerminateProcess(hHandle,dwExitCode); 

			break;
		}
	}while(Process32Next(hndl,&procEntry));


}

void CEdPreviewCooker::OnDeployCook( wxCommandEvent& event )
{
	// Cooking is in progress
	if ( m_cookingThread )
	{
		wxMessageBox( wxT("Cooking is already in progress"), wxT("Start cooking"), wxOK | wxICON_ERROR );
		return;
	}

	// Reset interface
	// ResetCookingInterface();

	// Setup options
	CookingSettings settings;

	// Get cooking platform
	settings.m_platform = GetCookingPlatform();

	// Cooking platform string
	String cookingPlatformString = TXT("targetpc ");
	if ( settings.m_platform == PLATFORM_XboxOne )
	{
		cookingPlatformString = TXT("targetdurango ");
	}
	else if ( settings.m_platform == PLATFORM_PS4 )
	{
		cookingPlatformString = TXT("targetorbis ");
	}

#ifdef COOK_FOR_XBOX
	// Exit title before deploying
	if ( settings.m_platform == PLATFORM_Xbox360 )
	{
		DmSetTitleEx( NULL, NULL, NULL, DMTITLE_UNPERSIST );
		DmReboot( DMBOOT_TITLE );
	}
#endif
	// Generate deploy task
	{
		CookingTask* task = new ( settings.m_tasks ) CookingTask();
		task->m_canFail = false;
		task->m_executable = m_cookerFile;
		task->m_directory = m_cookerDirectory;
		task->m_info = TXT("Deploying to target");

		// Format commandline
		task->m_commandLine = TXT("deploy ");
		task->m_commandLine += cookingPlatformString;

		// Target
		if ( settings.m_platform == PLATFORM_PC )
		{
			task->m_commandLine += m_pcDeployDir.c_str().AsWChar();
		}
		else if ( settings.m_platform == PLATFORM_XboxOne )
		{
			task->m_commandLine += m_xenonDeployDir.c_str().AsWChar();
		}
		else if ( settings.m_platform == PLATFORM_PS4 )
		{
			// TODO: support preview cooker for consoles
			//task->m_commandLine += m_xenonDeployDir.c_str().AsWChar();
		}
	}

	// Has ended
	PrintLog( TXT("Starting cooking task...\n") );
	m_statStatus->SetLabelText( TXT("Starting cooker...") );

	// Disable cooking interface
	EnableCookingUI( false );

	// Show the status window
	wxNotebook* tabs = XRCCTRL( *this, "m_notebook22", wxNotebook );
	tabs->SetSelection( 0 );

	// Create cooking task
	m_cookType = wxT("Deploy");
	m_cookingThread = new CEdCookingThread( m_log, settings );
	m_cookingThread->InitThread();
}

void CEdPreviewCooker::OnRunCook( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		const ECookingPlatform platform = GetCookingPlatform();
#ifdef COOK_FOR_XBOX
		if ( platform == PLATFORM_Xbox360 )
		{
			char titleDir[ 512 ];
			char titleName[ 512 ];
			char titleCommand[ 512 ];

			// Get type of console
			DWORD consoleFeatures = 0;
			DmGetConsoleFeatures( &consoleFeatures );

			// Run different xex based on the console type
			Red::System::StringCopy( titleName, "Witcher2.xex", ARRAY_COUNT(titleName) );

			// Prepare launch data
			Red::System::StringCopy( titleDir, m_xenonDeployDir.c_str().AsChar(), ARRAY_COUNT(titleDir) );
			titleCommand[0] = 0;
			//Red::System::StringCopy( titleCommand, UNICODE_TO_ANSI(world->GetDepotPath().AsChar()), ARRAY_COUNT(titleCommand) );

			// Run Xbox game
			DmSetTitleEx( titleDir, titleName, titleCommand, DMTITLE_UNPERSIST );
			DmReboot( DMBOOT_TITLE );
		}
#endif
	}
}

void CEdPreviewCooker::OnResetDevkit( wxCommandEvent& event )
{
	const ECookingPlatform platform = GetCookingPlatform();
#ifdef COOK_FOR_XBOX
	if ( platform == PLATFORM_Xbox360 )
	{
		DmSetTitleEx( NULL, NULL, NULL, DMTITLE_UNPERSIST );
		DmReboot( DMBOOT_COLD );
	}
#endif
}

void CEdPreviewCooker::OnStartCooking( wxCommandEvent& event )
{
	// Cooking is in progress
	if ( m_cookingThread )
	{
		wxMessageBox( wxT("Cooking is already in progress"), wxT("Start cooking"), wxOK | wxICON_ERROR );
		return;
	}

	// Save world
	if ( !GGame->GetActiveWorld() )
	{
		wxMessageBox( wxT("Please open world before cooking preview build"), wxT("Start cooking"), wxOK | wxICON_ERROR );
		Close();
		return;
	}

	// World is not saved
	if ( !GGame->GetActiveWorld()->GetFile() )
	{
		wxMessageBox( wxT("Please save this world at least once before cooking"), wxT("Start cooking"), wxOK | wxICON_ERROR );
		Close();
		return;
	}

	// Get world file
	String worldDepotPath = GGame->GetActiveWorld()->GetDepotPath();

	// Save layers
	CWorld* world = GGame->GetActiveWorld();
	if ( !world->GetWorldLayers()->Save( true, true ) )
	{
		wxMessageBox( wxT("Failed to save some of the layers. Cannot cook preview world."), wxT("Start cooking"), wxOK | wxICON_ERROR );
		Close();
		return;
	}

	// Gather preview layers
	const Bool isPreviewCook = (XRCCTRL( *this, "cookingMode", wxRadioBox )->GetSelection() == 1);
	const Bool isWorldPreviewCook = (XRCCTRL( *this, "cookingMode", wxRadioBox )->GetSelection() == 2);
	if ( isPreviewCook )
	{
		// Gather list
		TDynArray< String > previewLayerPaths;
		const Bool skipHidden = true;
		GatherPreviewLayers( world, world->GetWorldLayers(), skipHidden, previewLayerPaths );
		if ( previewLayerPaths.Empty() )
		{
			// In preview mode we don't allow cooking of no layers
			wxMessageBox( wxT("There are no preview layers to cook."), wxT("Start cooking"), wxOK | wxICON_ERROR );
			Close();
			return;
		}

		// Save the list of layers
		{
			// Get world file absolute path
			String worldFileAbsolutePath = world->GetFile()->GetAbsolutePath();
			worldFileAbsolutePath += TXT(".preview");
			IFile* previewLayers = GFileManager->CreateFileWriter( worldFileAbsolutePath, FOF_AbsolutePath );
			if ( !previewLayers )
			{
				wxMessageBox( wxT("Unable to create list of preview layers."), wxT("Start cooking"), wxOK | wxICON_ERROR );
				Close();
				return;
			}

			// Prepare preview data
			WorldPreviewData previewData;
			previewData.m_cameraPosition = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();
			previewData.m_cameraRotation = wxTheFrame->GetWorldEditPanel()->GetCameraRotation();
			previewData.m_layers = previewLayerPaths;

			// Save layers
			*previewLayers << previewData;
			delete previewLayers;
		}
	}

	// Reset interface
	ResetCookingInterface();

	// Setup options
	CookingSettings settings;

	// Get cooking platform
	wxChoice* platformChoice = XRCCTRL( *this, "boxPlatform", wxChoice );
	settings.m_platform = (platformChoice->GetSelection() == 0) ? PLATFORM_XboxOne : PLATFORM_PC; // TODO: support PS4

	// Cooking platform string
	String cookingPlatformString = TXT("targetpc ");
	if ( settings.m_platform == PLATFORM_XboxOne )
	{
		cookingPlatformString = TXT("targetdurango ");
	}
	else if ( settings.m_platform == PLATFORM_PS4 )
	{
		cookingPlatformString = TXT("targetorbis ");
	}

	// Generate cleanup task
	const Bool cleanBeforeCooking = XRCCTRL( *this, "cleanBefore", wxCheckBox )->IsChecked();
	if ( cleanBeforeCooking )
	{
		CookingTask* task = new ( settings.m_tasks ) CookingTask();
		task->m_canFail = false;
		task->m_executable = m_cookerFile;
		task->m_commandLine = String( TXT("clean ") ) + cookingPlatformString;
		task->m_directory = m_cookerDirectory;
		task->m_info = TXT("Cleaning up cooked folder");
	}

	// Generate script task
	{
		CookingTask* task = new ( settings.m_tasks ) CookingTask();
		task->m_canFail = false;
		task->m_executable = m_cookerFileGame;
		task->m_commandLine = String( TXT("scripts ") ) + cookingPlatformString;
		task->m_directory = m_cookerDirectory;
		task->m_info = TXT("Compiling scripts");
	}

	// Generate cooking task
	{
		CookingTask* task = new ( settings.m_tasks ) CookingTask();
		task->m_canFail = false;
		task->m_executable = m_cookerFile;
		task->m_directory = m_cookerDirectory;
		task->m_info = TXT("Cooking level");

		// Format commandline
		task->m_commandLine = TXT("cook2 ");
		task->m_commandLine += isPreviewCook ? TXT("preview ") : ( isWorldPreviewCook ? TXT("fullpreview ") : TXT("all ") );
		task->m_commandLine += cookingPlatformString;
		task->m_commandLine += worldDepotPath;
	}
	
	// Has ended
	PrintLog( TXT("Starting cooking task...\n") );
	m_statStatus->SetLabelText( TXT("Starting cooker...") );

	// Disable cooking interface
	EnableCookingUI( false );

	// Show the status window
	wxNotebook* tabs = XRCCTRL( *this, "m_notebook22", wxNotebook );
	tabs->SetSelection( 0 );

	// Create cooking task
	m_cookType = wxT("Cook");
	m_cookingThread = new CEdCookingThread( m_log, settings );
	m_cookingThread->InitThread();
}

void CEdPreviewCooker::OnCloseWindow( wxCloseEvent& event )
{
	// Cooking is in progress
	if ( m_cookingThread )
	{
		wxMessageBox( wxT("Cooking is still in progress."), wxT("Start cooking"), wxOK | wxICON_ERROR );
		return;
	}

	// We are done
	event.Skip();
}

void CEdPreviewCooker::OnTaskEnded( wxTimerEvent& event )
{
	// Has the thread ended ?
	if ( m_cookingThread && m_cookingThread->HasEnded() )
	{
		// Errors ?
		const Bool hadErrors = m_cookingThread->HadErrors();

		// Delete cooking thread
		m_cookingThread->JoinThread();
		delete m_cookingThread;
		m_cookingThread = NULL;

		// Reenable normal interface
		EnableCookingUI( true );

		// Has ended
		PrintLog( TXT("Cooking task has ended\n") );

		// Errors?
		Bool canAutoRun = true;
		if ( hadErrors )
		{
			canAutoRun = false;
			wxMessageBox( wxT("WCC crashed during cooking :(\n.Please see the report."), wxT("Cooking errors"), wxOK | wxICON_ERROR );
		}

		// Generate raport
		GenerateReport();

		// Show the report window
		wxNotebook* tabs = XRCCTRL( *this, "m_notebook22", wxNotebook );
		tabs->SetSelection( 1 );

		// Next step
		if ( m_cookType == wxT("Cook") )
		{
			// Deploy
			const Bool isDeploying = XRCCTRL( *this, "cookDeploy", wxCheckBox )->IsChecked();
			if ( isDeploying )
			{
				wxCommandEvent fakeEvent;
				OnDeployCook( fakeEvent );
			}
		}
		else if ( m_cookType == wxT("Deploy") )
		{
			// Run
			const Bool isRunning = XRCCTRL( *this, "cookRun", wxCheckBox )->IsChecked();
			if ( isRunning )
			{
				wxCommandEvent fakeEvent;
				OnRunCook( fakeEvent );
			}
		}
	}
}

// Bool CEdPreviewCooker::ProcessDebugRequest( CName requestType, const CNetworkPacket& packet, CNetworkPacket& response )
// {
// 	// File
// 	if ( requestType == TXT("CookerFile") )
// 	{
// 		m_numFiles += 1;
// 		m_statNumFiles->SetLabelText( String::Printf( TXT("%i"), m_numFiles ).AsChar() );
// 		return true;
// 	}
// 
// 	// Error
// 	if ( requestType == TXT("CookerError") )
// 	{
// 		String resource = packet.ReadString();
// 		String msg = packet.ReadString();
// 		
// 		Bool found = false;
// 
// 		for ( Uint32 i = 0; i < m_cookerErrors.Size(); ++i )
// 		{
// 			if ( m_cookerErrors[i].m_message == msg )
// 			{
// 				found = true;
// 				break;
// 			}
// 		}
// 
// 		if ( !found )
// 		{
// 			// Read message
// 			CookerMessage* message = new ( m_cookerErrors ) CookerMessage;
// 			message->m_resource = resource;
// 			message->m_message = msg;
// 
// 			// Update stats
// 			m_statNumErrors->SetLabelText( String::Printf( TXT("%i"), m_cookerErrors.Size() ).AsChar() );
// 		}
// 
// 		return true;
// 	}
// 
// 	// Warning
// 	if ( requestType == TXT("CookerWarning") )
// 	{
// 		String resource = packet.ReadString();
// 		String msg = packet.ReadString();
// 
// 		Bool found = false;
// 
// 		for ( Uint32 i = 0; i < m_cookerWarnings.Size(); ++i )
// 		{
// 			if ( m_cookerWarnings[i].m_message == msg )
// 			{
// 				found = true;
// 				break;
// 			}
// 		}
// 
// 		if ( !found )
// 		{
// 			// Read message
// 			CookerMessage* message = new ( m_cookerWarnings ) CookerMessage;
// 			message->m_resource = resource;
// 			message->m_message = msg;
// 
// 			// Update stats
// 			m_statNumWarnings->SetLabelText( String::Printf( TXT("%i"), m_cookerWarnings.Size() ).AsChar() );
// 		}
// 
// 		return true;
// 	}
// 
// 	// Progress
// 	if ( requestType == TXT("CookerProgress") )
// 	{
// 		// Read data
// 		Uint32 count = packet.ReadDWord();
// 		Uint32 max = packet.ReadDWord();
// 
// 		// Update progress
// 		const Uint32 prc = max ? ((Uint32)(1000.0f * count / (Float)max )) : 0;
// 		m_statProgress->SetValue( prc );
// 		return true;
// 	}
// 
// 	// Status
// 	if ( requestType == TXT("CookerStatus") )
// 	{
// 		// Read data
// 		String status = packet.ReadString();
// 
// 		// Update status
// 		m_statStatus->SetLabelText( status.AsChar() );
// 		return true;
// 	}
// 
// 	// Not handed
// 	return false;
// }

//////////////////////////////////////////////////////////////////////////////
