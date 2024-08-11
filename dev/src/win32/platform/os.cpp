/**
* Copyright © 2007 CDProjekt Red, Inc. All Rights Reserved.
*
* This file contains operating system functions
*/
#include "build.h"
#include "../../common/core/clipboardBase.h"
#include "clipboardWin32.h"
#include <process.h>
#include <shlobj.h>

#include "../../common/redSystem/crt.h"
#include "../../common/redIO/redIO.h"

#include "../../common/core/explatformos.h"
#include "../../common/core/gameConfiguration.h"
#include "../../common/core/version.h"
#include "../../common/core/asyncIO.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/scriptStackFrame.h"
#include "../../common/core/configFileManager.h"
#include "../../common/core/configVarSystem.h"
#include "../../common/core/configVarStorage.h"
#include "../../common/core/scriptingSystem.h"
#include "../../common/core/feedback.h"
#include "../../common/core/tokenizer.h"

#include "../../common/engine/inGameConfig.h"
#include "../../common/engine/inputDeviceManager.h"
#include "../../common/engine/gameResource.h"

#include "inputDeviceManagerWin32.h"

#include "../../common/engine/inGameConfig.h"
#include "../../common/core/depotBundles.h"
#include "../../common/core/contentManager.h"

#include "userProfileManagerWindows.h"

# pragma comment (lib, "Winmm.lib")

RED_DECLARE_DEBUG_CALLBACK_TAG( ScriptCallstack );

/////////////////////////////////////////////////////////////////////
// User config loader saver
class CPcUserConfigLoaderSaver : public Config::IUserConfigLoaderSaver
{
public:
	virtual Bool LoadToStorage(Config::CConfigVarStorage* storage)
	{
		StringAnsi stringContent;
		if( GUserProfileManager->LoadUserSettings( stringContent ) == true )
		{
			return storage->LoadFromString( stringContent );
		}

		return false;
	}

	virtual Bool SaveFromStorage(const Config::CConfigVarStorage* storage)
	{
		StringAnsi stringContent;
		if( storage->SaveToString( stringContent ) == true )
		{
			return GUserProfileManager->SaveUserSettings( stringContent );
		}

		return false;
	}
};

namespace
{
	CPcUserConfigLoaderSaver* GPcUserConfigLoaderSaver;
}

/////////////////////////////////////////////////////////////////////

void SInitializePlatform( const wchar_t* commandLine )
{
	// Initialize the clipboard
	SInitializeWin32Clipboard();

	// Update splash screen
	GSplash->UpdateProgress( TXT("Initializing system...") );

	// Get ID of main thread
	extern Red::System::Internal::ThreadId GMainThreadID;
	GMainThreadID.InitWithCurrentThread();

	// Setup error system
#ifndef NO_ASSERTS
	extern Red::System::Error::EAssertAction AssertMessageImp( const Char* cppFile, Uint32 line, const Char* expression, const Char* message, const Char* details );
	Red::System::Error::Handler::GetInstance()->RegisterAssertHook( &AssertMessageImp );
	Red::System::Error::Handler::GetInstance()->SetVersion( APP_VERSION_NUMBER );
#endif

	// Default values are all we need for the editor
	Core::CommandLineArguments coreArguments;

	extern String* GCommandLine;
	GCommandLine = new String( commandLine );

	String rootOverride;
	CTokenizer tok( *GCommandLine, TXT(" ") );
	for ( Uint32 i=0; i<tok.GetNumTokens(); ++i )
	{
		String token = tok.GetToken( i );
		if ( token == TXT("-root") )
		{
			rootOverride = tok.GetToken( i + 1 );
			rootOverride.ReplaceAll(TXT("/"),TXT("\\"));
			if (!rootOverride.EndsWith(TXT("\\")))
			{
				rootOverride += TXT("\\");
			}
		}
	}

#ifndef RED_PLATFORM_CONSOLE
	if ( !rootOverride.Empty() && !GSystemIO.FileExist( rootOverride.AsChar() ) )
	{
		ERR_CORE(TXT("Root override path %s not found"), rootOverride.AsChar() );
		GFeedback->ShowError( TXT("Root override path %s not found"), rootOverride.AsChar() );
	}
#endif

	// Use the path from the EXE file to setup current directory
	String partPath, rootPath, workingPath, bundlePath, dataPath, scriptPath, configPath, userPath;
	if ( !rootOverride.Empty() )
	{
		ASSERT( rootOverride.GetLength() > 1 );
		String safeOverridePath = rootOverride;
		if ( safeOverridePath.EndsWith(TXT("\\")) )
		{
			safeOverridePath.Replace(TXT("\\"), TXT(""), true );
		}

		dataPath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetDataPathSuffix().AsChar() );
		bundlePath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetBundlePathSuffix().AsChar() );
		scriptPath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetScriptsPathSuffix().AsChar() );
		workingPath = String::Printf( TXT( "%s\\bin\\" ), safeOverridePath.AsChar() );
		configPath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetConfigDirName().AsChar() );
		rootPath = rootOverride;

		SetCurrentDirectory( workingPath.AsChar() );

		// Set user data path
		Char myDocumentsPath[MAX_PATH];
		HRESULT userPathResult = SHGetFolderPath( NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, myDocumentsPath );
		if ( userPathResult == S_OK )
		{
			userPath = String::Printf( TXT( "%s\\%s\\" ), myDocumentsPath, GGameConfig::GetInstance().GetUserPathSuffix().AsChar() );
		}
		else
		{
			userPath = dataPath;
		}
	}
	else
	{
		Char moduleFileName[MAX_PATH];
		Char moduleFullPath[MAX_PATH];
		GetModuleFileName( NULL, moduleFileName, MAX_PATH );
		GetFullPathName( moduleFileName, MAX_PATH, moduleFullPath, NULL);

		partPath = moduleFileName;

		// Strip EXE name
		Char* pEnd = Red::System::StringSearchLast( moduleFullPath, '\\' );
		if ( pEnd )
		{
			*pEnd = '\0';
		}
		// Strip the binary subfolder - x86/x64/etc...
		pEnd = Red::System::StringSearchLast( moduleFullPath, '\\' );
		if ( pEnd )
		{
			*pEnd = '\0';
		}

		workingPath = String::Printf( TXT( "%s\\" ), moduleFullPath );

		// Strip the binary folder and get the root.
		pEnd = Red::System::StringSearchLast( moduleFullPath, '\\' );
		if ( pEnd )
		{
			*pEnd = '\0';
		}

		rootPath = moduleFullPath;

		// Set new current directory to the directory of binary file
		SetCurrentDirectory( workingPath.AsChar() );

		// Set user data path
		Char myDocumentsPath[MAX_PATH];
		HRESULT userPathResult = SHGetFolderPath( NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, myDocumentsPath );
		if ( userPathResult == S_OK )
		{
			userPath = String::Printf( TXT( "%s\\%s\\" ), myDocumentsPath, GGameConfig::GetInstance().GetUserPathSuffix().AsChar() );
		}
		else
		{
			userPath = dataPath;
		}

		// Set config path
#if defined( RED_FINAL_BUILD )
		if ( userPathResult == S_OK )
		{
			configPath = String::Printf( TXT( "%s\\%s\\" ), userPath.AsChar(), GGameConfig::GetInstance().GetConfigDirName().AsChar() );
		}
		else
		{
			configPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetConfigDirName().AsChar() );
		}
#else
		configPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetConfigDirName().AsChar() );
#endif
		
		// Fix paths with \ on the end
		dataPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetDataPathSuffix().AsChar() );

		// Set script path
		scriptPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetScriptsPathSuffix().AsChar() );

		bundlePath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetBundlePathSuffix().AsChar() );
	}

	// Grab command line


	const Bool ioInit = Red::IO::Initialize();
	if ( ! ioInit )
	{
		RED_FATAL( "Failed to initialize redIO" );
	}

	String rootPathWithTrailingBackslash = rootPath;
	if ( !rootPathWithTrailingBackslash.EndsWith(TXT("\\")))
	{
		rootPathWithTrailingBackslash += TXT("\\");
	}

	// Initialize file system
	GFileManager = new CFileManager( rootPathWithTrailingBackslash.AsChar(), workingPath.AsChar(), dataPath.AsChar(), bundlePath.AsChar(), /* read only */ false );
	GFileManager->SetUserDirectory( userPath.AsChar() );
	GFileManager->SetApplicationFilename( partPath.AsChar() );

	// Initialize async I/O
	GDeprecatedIO = new CDeprecatedIO;
	if ( ! GDeprecatedIO->Init() )
	{
		delete GDeprecatedIO;
		GDeprecatedIO = nullptr;
	}
	RED_FATAL_ASSERT( GDeprecatedIO, "Could not create async file manager" );
 	
	GContentManager = new CContentManager;
	GContentManager->Init();
	GContentManager->ScanFoldersForContent( eContentSource_LooseFiles );

	// Initialize configuration system
	GSplash->UpdateProgress( TXT("Loading legact configuration...") );
	GConfig = new CConfigManager( configPath );
	GUserProfileManager = new CUserProfileManagerWindows();

	GPcUserConfigLoaderSaver = new CPcUserConfigLoaderSaver();

	// Initialize configuration system
	GSplash->UpdateProgress( TXT("Loading configuration...") );
	SConfig::GetInstance().Load( workingPath + TXT("config\\"), GPcUserConfigLoaderSaver );	// When all configs will be in new path - change configPath and apply here
	GInGameConfig::GetInstance().GenerateConfigs( workingPath + TXT("config\\") );	// When all configs will be in new path - change configPath and apply here

	extern IInputDeviceManager* GInputDeviceManager;
	GInputDeviceManager = new CInputDeviceManagerWin32;

	// Initialize version control
	GSplash->UpdateProgress( TXT("Initializing P4SCC...") );
	SInitializeVersionControl();

	// Initialize depot + resource monitoring
#ifdef ENABLE_RESOURCE_MONITORING
	ResourceMonitorStats::EnableMonitoring( true );
#endif
	GSplash->UpdateProgress( TXT("Scanning depot...") );
	GDepot = new CDepot( dataPath );
	{
		CTimeCounter timer; 
		GDepot->Initialize( coreArguments );
		RED_LOG( Core, TXT("Initializing depot took %1.1f sec"), timer.GetTimePeriod() );
	}

	// Initialize scripting system
	GSplash->UpdateProgress( TXT("Initializing scripts...") );
	GScriptingSystem = new CScriptingSystem( scriptPath.AsChar() );

#ifndef NO_SCRIPT_DEBUG
	RED_REGISTER_DEBUG_CALLBACK( ScriptCallstack, &CScriptStackFrame::DumpScriptStack );
#endif
}

void SShutdownPlatform()
{
	RED_UNREGISTER_DEBUG_CALLBACK( ScriptCallstack );

	SConfig::GetInstance().Shutdown();

	delete GPcUserConfigLoaderSaver;

	delete GUserProfileManager;

	delete GConfig;

	delete GScriptingSystem;

	delete GDepot;

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	delete GVersionControl;
#endif

	GDeprecatedIO->Shutdown();
	delete GDeprecatedIO;

	delete GFileManager;

	Red::IO::Shutdown();

	extern String* GCommandLine;
	delete GCommandLine;

	delete GClipboard;
}

Bool SPumpMessages()
{
	MSG Msg;
	while ( ::PeekMessageW( &Msg,NULL,0,0,PM_REMOVE ) )
	{
		if ( Msg.message == WM_QUIT )
		{
			return false;
		}

		::TranslateMessage(&Msg);
		::DispatchMessageW(&Msg);
	}

	return true;
}

