/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "app.h"
#include "fileStubs.h"
#include "frame.h"

// include compiled XRC resources
#include RED_EXPAND_AND_STRINGIFY(res\PROJECT_PLATFORM\PROJECT_CONFIGURATION\scriptStudioXRC.hpp)

#define SOLUTION_IMAGE_SIZE 16
#define LOG_FILE TXT( "scriptstudio.log" )

#ifdef _DEBUG
#	define RED_VERSION_CONTROL_DLL	"redscm_d.dll"
#else
#	define RED_VERSION_CONTROL_DLL	"redscm.dll"
#endif

// windows specific
#include <ShlObj.h>

#include "solution/slnContainer.h"

LPCWSTR GetDesktopPath()
{
	static wxString path;
	PWSTR ppath;

	if ( FAILED( SHGetKnownFolderPath( FOLDERID_Desktop, 0, NULL, &ppath ) ) )
	{
		return NULL;
	}

	path = ppath;

	CoTaskMemFree( ppath );

	return path.wc_str();
}

HRESULT CreateShortcut( LPCWSTR target, LPCWSTR arguments, LPCWSTR linkPath, LPCWSTR description )
{
	IShellLink* link;
	HRESULT hres = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&link );
	if ( FAILED(hres) )
	{
		return hres;
	}

	IPersistFile* pf;
	link->SetPath( target );
	link->SetDescription( description );
	link->SetArguments( arguments );
	hres = link->QueryInterface( IID_IPersistFile, (LPVOID*)&pf );
	if ( FAILED(hres) )
	{
		link->Release();
		return hres;
	}

	hres = pf->Save( linkPath, TRUE );

	pf->Release();
	link->Release();

	return hres;
}

CSSApp::CSSApp()
	: wxApp()
	, m_versionControl( nullptr )
	, m_XResourcesPanel( NULL )
	, m_networkPort( Red::Network::Manager::DEFAULT_PORT )
	, m_fileLogDevice( LOG_FILE, true )
	, m_solution( nullptr )
{
	Red::System::MemoryZero( &m_versionControlLibraryHandle, sizeof( HMODULE ));
}

CSSApp::~CSSApp()
{
	GStubSystem.StopThread();

	// Shutdown network
	m_network.ImmediateShutdown();

	// Close perforce connection
	ShutdownVersionControl();

	delete m_solution;
	m_solution = nullptr;
}

bool CSSApp::OnInit()
{
	SetVendorName( wxT( "CD Projekt Red" ) );
	SetAppName( wxT( "Blade" ) ); 

	wxLog::EnableLogging( false );

	// Prevent the system from breakpointing on asserts
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, false );

	// 
	m_network.Initialize( &realloc, &free );

	wxCmdLineParser cmdLineParser( argc, argv );

	cmdLineParser.AddOption( wxT( "scriptpath" ),( "S" ), wxT( "Open solution that points to specified directory" ) );
	cmdLineParser.AddOption( wxT( "openfile" ), wxT( "o" ), wxT( "Open and select file" ) );
	cmdLineParser.AddOption( wxT( "goto" ), wxT( "g" ), wxT( "Goto line No. Requires a file to have been specified" ), wxCMD_LINE_VAL_NUMBER );
	cmdLineParser.AddOption( wxT( "port" ), wxT( "p" ), wxT( "Network connection port override" ), wxCMD_LINE_VAL_NUMBER );
	cmdLineParser.AddSwitch( wxT( "v" ), wxT( "novcs" ), wxT( "Disables VCS system connection" ) );
	cmdLineParser.AddSwitch( wxT( "game" ), wxEmptyString, wxT( "Connect to game by default" ) );
	cmdLineParser.AddParam( wxT( "Solution file" ), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL ); 

	if( cmdLineParser.Parse() != 0 )
	{
		return false;
	}

	// Initialize autocomplete manager
	GStubSystem.StartThread();

	// Initialize source control
	{
		bool noVcs = cmdLineParser.Found( wxT( "v" ) );
		InitializeVersionControl( noVcs );
	}

	Solution* solution = nullptr;
	if( cmdLineParser.GetParamCount() > 0 )
	{
		wxString solutionFilePath = cmdLineParser.GetParam();

		if( !solutionFilePath.EndsWith( wxT( '\\' ) ) )
		{
			solutionFilePath.Append( wxT( '\\' ) );
		}

		solution = new Solution( solutionFilePath.wc_str() );
	}
	else
	{
		wxString scriptsRootPath;

		if( cmdLineParser.Found( wxT("scriptpath"), &scriptsRootPath ) )
		{
			if( !scriptsRootPath.EndsWith( wxT( '\\' ) ) )
			{
				scriptsRootPath.Append( wxT( '\\' ) );
			}

			solution = new Solution( scriptsRootPath.wc_str() );
		}
		else
		{
			solution = new Solution();
		}
	}

	// Init XRC stuff
	wxInitAllImageHandlers();
	
	wxXmlResource::Get()->InitAllHandlers();

	InitXmlResource();

	// Create main frame
	wxTheFrame = new CSSFrame( solution );
	SetTopWindow( wxTheFrame );
	wxTheFrame->SetIcon( wxICON( amain ) );
	wxTheFrame->SetSize( 800, 600 );
	wxTheFrame->Maximize( true );

	if( cmdLineParser.Found( wxT( "game" ) ) )
	{
		wxTheFrame->SetClientToGame();
	}

	wxTheFrame->Show();

	wxString filepath = wxEmptyString;
	if( cmdLineParser.Found( wxT( "openfile" ), &filepath ) )
	{
		long line = 0;
		cmdLineParser.Found( wxT( "goto" ), &line );
		wxTheFrame->OpenFileAndGotoLine( filepath, line );
	}

	long port = 0;
	if ( cmdLineParser.Found( wxT( "port" ), &port ) )
	{
		m_networkPort = uint16_t( port );
	}

	// Start processing of files - it's important to start it after creating main window 
	// as it displays file parsing status in status bar
	GStubSystem.StartProcessing();

	// Initialized
	return true;
}

bool CSSApp::InitializeVersionControl( bool noVersionControl )
{
	m_versionControlLibraryHandle = noVersionControl ? nullptr : LoadLibraryA( RED_VERSION_CONTROL_DLL );

	if( m_versionControlLibraryHandle )
	{
		CreateInterfaceFunc create = (CreateInterfaceFunc) GetProcAddress( m_versionControlLibraryHandle, "CreateInterface" );

		if( create )
		{
			m_versionControl = create();

			if( m_versionControl->Initialize( "ScriptStudio" ) )
			{
				map< wxString, wxString > credentials;
				
				if( LoadVersionControlCredentials( credentials ) )
				{
					for( map< wxString, wxString >::iterator iter = credentials.begin(); iter != credentials.end(); ++iter )
					{
						wxTheSSApp->GetVersionControl()->SetCredential( iter->first.mb_str(), iter->second.mb_str() );
					}
				}

				return true;
			}
		}

		FreeLibrary( m_versionControlLibraryHandle );
		Red::System::MemoryZero( &m_versionControlLibraryHandle, sizeof( HMODULE ));
	}

	m_versionControl = &m_dummyVersionControl;

	return false;
}

void CSSApp::ShutdownVersionControl()
{
	if( m_versionControlLibraryHandle )
	{
		DestroyInterfaceFunc destroy = (DestroyInterfaceFunc) GetProcAddress( m_versionControlLibraryHandle, "DestroyInterface" );

		destroy( m_versionControl );
		m_versionControl = nullptr;

		FreeLibrary( m_versionControlLibraryHandle );
		Red::System::MemoryZero( &m_versionControlLibraryHandle, sizeof( HMODULE ));
	}
}

wxString CSSApp::GetVersionControlCredentialsConfigPath() const
{
	wxFileName filePath( wxStandardPaths::Get().GetExecutablePath() );
	filePath.SetFullName( wxT( "credentials.ini" ) );

	return filePath.GetFullPath();
}

bool CSSApp::SaveVersionControlCredentials( const map< wxString, wxString >& credentials )
{
	// Transfer the data from the map to a configuration file
	wxFileConfig config;

	map< wxString, wxString >::const_iterator iter;
	for( iter = credentials.begin(); iter != credentials.end(); ++iter )
	{
		if( !iter->second.IsEmpty() )
		{
			config.Write( iter->first.wx_str(), iter->second.wx_str() );
		}
	}

	// Save the file
	wxFileOutputStream fileStream( GetVersionControlCredentialsConfigPath() );

	if( fileStream.IsOk() )
	{
		config.Save( fileStream );

		return true;
	}

	return false;
}

bool CSSApp::LoadVersionControlCredentials( map< wxString, wxString >& credentials ) const
{
	wxString path = GetVersionControlCredentialsConfigPath();

	if( wxFileName::Exists( path ) )
	{
		wxFileInputStream fileStream( path );

		if( fileStream.IsOk() )
		{
			wxFileConfig config( fileStream );

			wxString entry;
			long index;

			if( config.GetFirstEntry( entry, index ) )
			{
				do 
				{
					wxString value;
					config.Read( entry, &value );

					credentials[ entry ] = value;

				} while( config.GetNextEntry( entry, index ) );
			}

			return true;
		}
	}

	return false;
}

wxBitmap CSSApp::LoadBitmap( const wxString& name )
{
	// Load images
	if ( !m_XResourcesPanel )
	{
		m_XResourcesPanel = new wxPanel();
		wxXmlResource::Get()->LoadPanel( m_XResourcesPanel, wxTheFrame, wxT("XResources") );
		m_XResourcesPanel->Hide();
	}

	wxStaticBitmap* bitmap = wxStaticCast( m_XResourcesPanel->FindWindow( wxXmlResource::GetXRCID( name ) ), wxStaticBitmap );
	return bitmap->GetBitmap();
}

wxImageList* CSSApp::CreateSolutionImageList()
{
	// Create image list
	wxImageList* images = new wxImageList( SOLUTION_IMAGE_SIZE, SOLUTION_IMAGE_SIZE, true, SOLIMG_Max );

	// 	SOLIMG_Classes
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_CLASSES" ) ) );
	// 	SOLIMG_DirOpened
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_DIR_OPENED" ) ) );
	// 	SOLIMG_DirClosed
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_DIR_CLOSED" ) ) );
	// 	SOLIMG_NotInDepot
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_LOCAL" ) ) );
	//	SOLIMG_NotInDepotModified
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_LOCAL_MODIFIED" ) ) );
	// 	SOLIMG_FileNormal
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_NORMAL" ) ) );
	// 	SOLIMG_FileNormalOutOfDate
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_OUTOFDATE" ) ) );
	// 	SOLIMG_FileLocked
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_LOCKED" ) ) );
	// 	SOLIMG_FileLockedOutOfDate
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_LOCKED_OUTOFDATE" ) ) );
	// 	SOLIMG_FileModified
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_MODIFIED" ) ) );
	// 	SOLIMG_FileModifiedOutOfDate
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_MODIFIED_OUTOFDATE" ) ) );
	// 	SOLIMG_FileAdd
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_ADD" ) ) );
	// 	SOLIMG_FileAddModified
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_ADD_MODIFIED" ) ) );
	// 	SOLIMG_Deleted
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_FILE_DELETED" ) ) );

	return images;
}

void CSSApp::OnAssertFailure( const wxChar* file, int line, const wxChar* func, const wxChar* cond, const wxChar* msg )
{
	// It looks like wxWidgets doesn't let you account for different asserts, so once a user hits continue always, that's what'll always happen
	static Red::System::Error::EAssertAction lastAction = Red::System::Error::AA_Break;
	if ( lastAction != Red::System::Error::AA_ContinueAlways )
	{
		lastAction = Red::System::Error::HandleAssertion( file, line, cond, TXT( "[wxWidgets]: %" ) RED_PRIWs, msg );
		if ( lastAction == Red::System::Error::AA_Break )
		{
			RED_BREAKPOINT();
		}
	}
}
