/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "platform.h"

#include "../redSystem/stringWriter.h"
#include "../redSystem/crt.h"
#include "../redIO/redIO.h"

#include "../core/clipboardBase.h"
#include "../core/explatformos.h"
#include "../core/gameConfiguration.h"
#include "../core/version.h"
#include "../core/asyncIO.h"
#include "../core/versionControl.h"
#include "../core/depot.h"
#include "../core/scriptStackFrame.h"
#include "../core/configFileManager.h"
#include "../core/configVarSystem.h"
#include "../core/configVarStorage.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/configVarRegistry.h"
#include "../core/scriptingSystem.h"
#include "../core/feedback.h"
#include "../core/contentManager.h"
#include "../core/xmlFileReader.h"
#include "../core/tokenizer.h"
#include "../core/configVar.h"

#include "../engine/inGameConfig.h"
#include "../engine/soundSystem.h"
#include "../engine/baseEngine.h"

#include "clipboardWin32.h"
#include "win32Viewport.h"
#include <shlobj.h>

#include "../../win32/platform/userProfileManagerWindows.h"
#include "../../win32/platform/inputDeviceManagerWin32.h"

# pragma comment (lib, "Winmm.lib")

RED_DECLARE_DEBUG_CALLBACK_TAG( ScriptCallstack );

RED_DECLARE_NAME( graphics );
RED_DEFINE_NAME( graphics );

/////////////////////////////////////////////////////////////////////
// Externs

extern CUserProfileManager* GUserProfileManager;

/////////////////////////////////////////////////////////////////////
// CreateViewport
//	Retrieves a viewport interface 
IPlatformViewport* CPlatform::CreateViewport( bool isEditor )
{
	if( isEditor )
	{
		return new CWin32EditorViewport;
	}
	else
	{
		return new CWin32GameViewport;
	}
}

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
// SetupPlatformPathsWithOverride
//	Sets up and retrieves engine paths from override directory
static void SetupPlatformPathsWithOverride( const String& overridePath, String &rootPath, String &workingPath, String &dataPath, String &bundlePath, String &userPath, String &partPath, String &configPath, String &scriptPath )
{
	// Do not calculate basePath and dataPath using path to executable.
	// Use rootOverride to set both paths (this is for external applications using the engine).
	ASSERT( overridePath.GetLength() > 1 );

	String safeOverridePath = overridePath;
	if ( safeOverridePath.EndsWith(TXT("\\")) )
	{
		safeOverridePath.Replace(TXT("\\"), TXT(""), true );
	}
	
	dataPath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetDataPathSuffix().AsChar() );
	bundlePath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetBundlePathSuffix().AsChar() );
	scriptPath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetScriptsPathSuffix().AsChar() );
	workingPath = String::Printf( TXT( "%s\\bin\\" ), safeOverridePath.AsChar() );
	configPath = String::Printf( TXT( "%s\\%s\\" ), safeOverridePath.AsChar(), GGameConfig::GetInstance().GetConfigDirName().AsChar() );
	rootPath = overridePath;

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

/////////////////////////////////////////////////////////////////////
// SetupPlatformPaths
//	Sets up and retrieves engine paths
void CPlatform::SetupPlatformPaths( String &rootPath, String &workingPath, String &dataPath, String &bundlePath, String &userPath, String &partPath, String &configPath, String &scriptPath )
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
		configPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(),GGameConfig::GetInstance().GetConfigDirName().AsChar() );
	}
#else
	configPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetConfigDirName().AsChar() );
#endif

	// Fix paths with \ on the end
	dataPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetDataPathSuffix().AsChar() );
	bundlePath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetBundlePathSuffix().AsChar() );

	// Set script path
	scriptPath = String::Printf( TXT( "%s\\%s\\" ), rootPath.AsChar(), GGameConfig::GetInstance().GetScriptsPathSuffix().AsChar() );
}

namespace GpuApi
{
	extern Bool GetDeviceId( Uint32& vendorId, Uint32& deviceId );
}

class CPerformanceConfigReader
{
public:
	CPerformanceConfigReader()
	:	DEVICEID_ATTR( TXT( "did" ) )
	,	VENDORID_ATTR( TXT( "vid" ) )
	,	PRESET_ATTR( TXT( "preset" ) )
	,	FPSLIMIT_ATTR( TXT( "fpslimit" ) )
	,	m_preset( 0 )
	,	m_fpsLimit( 0 )
	{

	}

	Bool Read( const String& path )
	{
		IFile* performanceFile = GFileManager->CreateFileReader( path, FOF_Buffered | FOF_AbsolutePath );
		Bool success = false;

		if( performanceFile )
		{
			String performanceFileContents;
			GFileManager->LoadFileToString( performanceFile, performanceFileContents );
			CXMLReader* performanceXML = new CXMLReader( performanceFileContents );

			//CXMLReader* performanceXML = new CXMLFileReader( *performanceFile );

			if( FetchDeviceId() )
			{
				success = ReadRoot( performanceXML );
			}

			delete performanceXML;
			delete performanceFile;
		}

		return success;
	}

	Uint32 GetPreset() const { return m_preset; }
	Uint32 GetFPSLimit() const { return m_fpsLimit; }

private:

	Bool FetchDeviceId()
	{
		return GpuApi::GetDeviceId( m_vendorId, m_deviceId );
	}

	Bool ReadRoot( CXMLReader* xml )
	{
		const String ROOT_NODE( TXT( "performance" ) );

		if( xml->BeginNode( ROOT_NODE ) )
		{
			Bool success = ReadDevices( xml );

			xml->EndNode();

			return success;
		}

		return false;
	}

	Bool ReadDevices( CXMLReader* xml )
	{
		const String DEVICE_NODE( TXT( "device" ) );

		Bool success = false;
		while( xml->BeginNode( DEVICE_NODE ) )
		{
			success = ReadDevice( xml );

			xml->EndNode();

			if( success )
			{
				break;
			}
		}

		return success;
	}

	Bool ReadDevice( CXMLReader* xml )
	{
		Uint32 vendorId = ReadAttribute( xml, VENDORID_ATTR );
		Uint32 deviceId = ReadAttribute( xml, DEVICEID_ATTR );

		if( vendorId == m_vendorId && deviceId == m_deviceId )
		{
			m_preset = ReadAttribute( xml, PRESET_ATTR );
			m_fpsLimit = ReadAttribute( xml, FPSLIMIT_ATTR, Red::System::BaseTen );

			return true;
		}

		return false;
	}

	Uint32 ReadAttribute( CXMLReader* xml, const String& name, Red::System::Base base = Red::System::BaseSixteen )
	{
		Uint32 val = 0;
		if( xml->Attribute( name, m_attribute ) )
		{
			Red::System::StringToInt( val, m_attribute.AsChar(), nullptr, base );
		}

		return val;
	}

	const String DEVICEID_ATTR;
	const String VENDORID_ATTR;
	const String PRESET_ATTR;
	const String FPSLIMIT_ATTR;
	
	String m_attribute;

	Uint32 m_vendorId;
	Uint32 m_deviceId;
	Uint32 m_preset;
	Uint32 m_fpsLimit;
};

namespace Helper
{
	Uint32 GetConfigValueUint( const AnsiChar* section, const AnsiChar* key )
	{
		String stringValue;
		SConfig::GetInstance().GetValue( section, key, stringValue );

		Uint32 val;
		if ( FromString< Uint32 >( stringValue, val ) )
			return val;

		return (Uint32) 0xFFFFFFFF;
	}

	void SetConfigValueUint( const AnsiChar* section, const AnsiChar* key, Uint32 value )
	{
		Char stringValue[ 64 ];
		Red::SNPrintF( stringValue, ARRAY_COUNT(stringValue), TXT("%d"), value );

		SConfig::GetInstance().SetValue( section, key, stringValue );
	}


} // Helper

static AnsiChar* GetCvarTypeAsChar( Config::IConfigVar* cvar )
{
	switch( cvar->GetType() )
	{
	case Config::eConsoleVarType_Bool:		return "bool";
	case Config::eConsoleVarType_Int:		return "int";
	case Config::eConsoleVarType_Float:		return "float";
	case Config::eConsoleVarType_String:	return "string";
	default:
		return "<unknown>";
	}
}

static AnsiChar* GetCvarFlagAsChar( Config::EConfigVarFlags flag )
{
	switch ( flag )
	{
	case Config::eConsoleVarFlag_Developer: return "developer";
	case Config::eConsoleVarFlag_ReadOnly:	return "readonly";
	case Config::eConsoleVarFlag_Save:		return "save";
	default:
		return "<unknown>";
	}
}

static StringAnsi FormatCvarAsString( Config::IConfigVar* cvar )
{
	Red::System::StackStringWriter<AnsiChar, 2048> writer;

	String value;
	String defaultValue;
	cvar->GetText(value);
	cvar->GetTextDefault(defaultValue);

	writer.Appendf("%hs %hs Value='%hs'", cvar->GetGroup(), cvar->GetName(), UNICODE_TO_ANSI(value.AsChar()));
	if ( value != defaultValue )
	{
		writer.Appendf(" DefaultValue='%hs'", UNICODE_TO_ANSI(defaultValue.AsChar()));
	}
	writer.Appendf(" Type='%hs'", GetCvarTypeAsChar(cvar));

	writer.Appendf(" Flags='");
	static const Config::EConfigVarFlags flags[] = { Config::eConsoleVarFlag_Developer, Config::eConsoleVarFlag_ReadOnly, Config::eConsoleVarFlag_Save, };
	Uint32 numFlags = 0;
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(flags); ++i )
	{
		const Config::EConfigVarFlags flag = flags[i];
		if ( cvar->HasFlag( flag ) )
		{
			if ( numFlags++ > 0 )
				writer.Append(" ");
			writer.Appendf("%hs", GetCvarFlagAsChar(flag));
		}
	}
	writer.Appendf("'");
	return writer.AsChar();
}

static void DumpCvarsToFile( const String& absolutePath )
{
	Red::TScopedPtr<IFile> writer( GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath|FOF_Buffered ) );
	if ( !writer )
	{
		return;
	}

	TDynArray< Config::IConfigVar* > cvars;
	SConfig::GetInstance().GetRegistry().EnumVars( cvars );
	Sort( cvars.Begin(), cvars.End(), [](const Config::IConfigVar* a, const Config::IConfigVar* b )
		{
			const Int32 cmp = Red::System::StringCompare( a->GetGroup(), b->GetGroup() );
			if ( cmp == 0 )
			{
				return Red::System::StringCompare( a->GetName(), b->GetName() ) < 0;
			}

			return cmp < 0;
		});

	for ( Config::IConfigVar* cvar : cvars )
	{
		StringAnsi fmtCvar = FormatCvarAsString(cvar);
		writer->Serialize( const_cast<AnsiChar*>(fmtCvar.AsChar()), fmtCvar.GetLength() );
		writer->Serialize( const_cast<AnsiChar*>("\r\n"), 2 );
	}
}

/////////////////////////////////////////////////////////////////////
// Initialise
//	Initialise the platform-specific bits of the engine that are common to all windows executables
void CPlatform::Initialize( const String& commandLine, const Core::CommandLineArguments& coreArguments )
{
	// Initialise the error / assert systems as soon as possible
#ifndef NO_ASSERTS
	Red::System::Error::Handler::GetInstance()->RegisterAssertHook( m_assertHandlerFn );
	Red::System::Error::Handler::GetInstance()->SetVersion( APP_VERSION_NUMBER );
#endif

	// Because of our loading screens. :(
	// This or we create the HWND on the render thread and pump messages there again. But we need to make
	// the updates thread safe, which they weren't.
	::DisableProcessWindowsGhosting();

	GSplash->UpdateProgress( TXT("Initializing system...") );

	// Initialize the clipboard
	SInitializeWin32Clipboard();

	// Update splash screen
	GSplash->UpdateProgress( TXT("Initializing system...") );

	// Set ID of main thread
	extern Red::System::Internal::ThreadId GMainThreadID;
	GMainThreadID.InitWithCurrentThread();

	// Grab command line
	extern String* GCommandLine;
	GCommandLine = new String( commandLine );

	String rootOverride;
	Bool	dumpCvars = false;
	Bool    dumpScripts = false;
	String	dumpCvarsPath;

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
		else if (token.BeginsWith( TXT("-dumpscripts") ) )
		{
			dumpScripts = true;
		}
		else if (token == TXT("-cvarsdump") )
		{
			dumpCvars = true;
			dumpCvarsPath = tok.GetToken( i + 1 );
			dumpCvarsPath.ReplaceAll(TXT("/"),TXT("\\"));
		}
	}

#ifndef RED_PLATFORM_CONSOLE
	if ( !rootOverride.Empty() && !GSystemIO.FileExist( rootOverride.AsChar() ) )
	{
		ERR_CORE(TXT("Root override path %s not found"), rootOverride.AsChar() );
	}
#endif

	if ( coreArguments.m_splitCook )
	{
		SGameConfigurationParameter params = GGameConfig::GetInstance().GetConfigParameters();

		const String newDataPathSuffix = TXT("content\\content0");
		LOG_CORE(TXT("Overriding data path for split cook to %s"), newDataPathSuffix.AsChar() );
		params.dataPathSuffix = newDataPathSuffix;

		// If you need to recompile scripts
		const String newScriptsPathSuffix = TXT("content\\content0\\scripts");
		LOG_CORE(TXT("Overriding scripts path for split cook to %s"), newScriptsPathSuffix.AsChar() );
		params.scriptsPathSuffix = newScriptsPathSuffix;

		const String newBundlesPathSuffix = TXT("content");
		LOG_CORE(TXT("Overriding bundles path for split cook to %s"), newBundlesPathSuffix.AsChar() );
		params.bundlePathSuffix = newBundlesPathSuffix;

		GGameConfig::GetInstance().Initialize( params );
	}

	// Set up engine paths
	String partPath, workingPath, rootPath, dataPath, bundlePath, scriptPath, configPath, userPath;
	if( !rootOverride.Empty() )
	{
		SetupPlatformPathsWithOverride( rootOverride, rootPath, workingPath, dataPath, bundlePath, userPath, partPath, configPath, scriptPath );
	}
	else
	{
		SetupPlatformPaths( rootPath, workingPath, dataPath, bundlePath, userPath, partPath, configPath, scriptPath );
	}

	// On windows we can run in non-read only mode only if we are not running from bundles
	Bool isReadOnly = true;
#ifndef RED_FINAL_BUILD
	{
		if ( !coreArguments.m_useBundles )
		{
			isReadOnly = false;
		}
	}
#endif

	String rootPathWithTrailingBackslash = rootPath;
	if ( !rootPathWithTrailingBackslash.EndsWith(TXT("\\")))
	{
		rootPathWithTrailingBackslash += TXT("\\");
	}

	// Initialize file system
	GFileManager = new CFileManager( rootPathWithTrailingBackslash.AsChar(), workingPath.AsChar(), dataPath.AsChar(), bundlePath.AsChar(), isReadOnly );
	GFileManager->SetUserDirectory( userPath.AsChar() );
	GFileManager->SetApplicationFilename( partPath.AsChar() );

	// Initialize async I/O
	GDeprecatedIO = new CDeprecatedIO;
	if ( !GDeprecatedIO->Init() )
	{
		delete GDeprecatedIO;
		GDeprecatedIO = nullptr;
	}
	RED_FATAL_ASSERT( GDeprecatedIO, "Could not create async file manager" );

	// Initialize configuration system
	GSplash->UpdateProgress( TXT("Loading engine configuration...") );
	GConfig = new CConfigManager( configPath );

	GUserProfileManager = new CUserProfileManagerWindows();
	GUserProfileManager->Initialize();

	// Tell the content manager what language the platform is set to
	String textLang, speechLang;
	GUserProfileManager->GetSystemLanguageStrings( textLang );
	GUserProfileManager->GetSystemLanguageSpeech( speechLang );

	GPcUserConfigLoaderSaver = new CPcUserConfigLoaderSaver();

	// Initialize configuration system
	GSplash->UpdateProgress( TXT("Loading configuration...") );
	SConfig::GetInstance().Load( workingPath + TXT("config\\"), GPcUserConfigLoaderSaver );	// When all configs will be in new path - change configPath and apply here
	GInGameConfig::GetInstance().GenerateConfigs( workingPath + TXT("config\\") );	// When all configs will be in new path - change configPath and apply here

	{
		CPerformanceConfigReader performanceConfigReader;
		const Uint32 currentHardwarePreset = performanceConfigReader.GetPreset();

		// apply only once
		const Uint32 savedHardwarePreset = Helper::GetConfigValueUint( "System", "HWPreset" );
		if ( savedHardwarePreset != currentHardwarePreset )
		{
			String path = GFileManager->GetRootDirectory() + TXT( "bin\\config\\performance.xml" );
			if( performanceConfigReader.Read( path ) )
			{
				TDynArray< InGameConfig::IConfigGroup* > groups;
				GInGameConfig::GetInstance().ListAllConfigGroups( groups );

				for( InGameConfig::IConfigGroup* group : groups )
				{
					if( group->HasTag( RED_NAME( graphics ) ) )
					{
						group->ApplyPreset( performanceConfigReader.GetPreset(), InGameConfig::eConfigVarAccessType_Loaded );
					}
				}

				if( performanceConfigReader.GetFPSLimit() > 0 )
				{
					Config::cvLimitFPS.Set( performanceConfigReader.GetFPSLimit() );
				}

				// make sure the performance values are saved only once
				Helper::SetConfigValueUint( "System", "HWPreset", currentHardwarePreset );

				// flush config after hardware change
				SConfig::GetInstance().Save();
			}
		}
	}

	if ( dumpCvars )
	{
		CFilePath fp(dumpCvarsPath);

		// Well, CFilePath::Relative is broken
// 		if ( fp.IsRelative() )
// 		{
// 			fp = CFilePath(GFileManager->GetUserDirectory() + dumpCvarsPath);
// 		}
		// Some lame check
		if ( !fp.GetPathString().ContainsCharacter(TXT(':')) )
		{
			fp = CFilePath(GFileManager->GetUserDirectory() + dumpCvarsPath);
		}

		if ( !fp.HasFilename() )
		{
			fp.SetFileName( TXT("cvarsdump.txt") );
		}

		dumpCvarsPath = fp.ToString();

		// don't let somebody maliciously clobber a more important file type
		if ( !dumpCvarsPath.ToLower().EndsWith(TXT(".txt")) ) // goddammit cfilepath
		{
			dumpCvarsPath += TXT(".txt");
		}

		DumpCvarsToFile( dumpCvarsPath );
	}

	GContentManager = new CContentManager;
	GContentManager->Init();

	EContentSource contentSource = eContentSource_LooseFiles;
	/*if ( GDepot->IsUsingBundles() )*/
	// GDepot might not be initialized yet.
	if ( coreArguments.m_useBundles )
	{
		contentSource = ( coreArguments.m_splitCook ) ? eContentSource_SplitCook : eContentSource_Cook;
	}

	GContentManager->ScanFoldersForContent( contentSource );
	GContentManager->SetSystemLanguages( textLang, speechLang );

	extern IInputDeviceManager* GInputDeviceManager;
	GInputDeviceManager = new CInputDeviceManagerWin32;

	// If no sound initialization is done here, then we have initialization later in baseEngineInit.
#ifdef NO_EDITOR
	// FIXME: there is no need to initialize sound system when dumping scripts and it can crash when no cooking data  
	// is available.
	if( !dumpScripts )
	{
		// Preinitialize sound system
		CSoundSystem::Init( workingPath + TXT("initialdata\\sound\\") );
	}
#endif

	// Initialize version control
	GSplash->UpdateProgress( TXT("Initializing P4SCC...") );
	InitializeVersionControl();

	// Initialize depot + resource monitoring if required
#ifdef ENABLE_RESOURCE_MONITORING
	String resourceMonitorEnabled = TXT("false");
	SConfig::GetInstance().GetValue( "Engine", "EnableResourceMonitor", resourceMonitorEnabled );
	if( resourceMonitorEnabled.ToLower() == TXT("true") )
	{
		ResourceMonitorStats::EnableMonitoring( true );
	}
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

//////////////////////////////////////////////////////////////////////////
// Shutdown
//	Close down the platform-specific bits initialised above
void CPlatform::Shutdown()
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

	extern String* GCommandLine;
	delete GCommandLine;

	delete GClipboard;
}

//////////////////////////////////////////////////////////////////////////
// InitializeVersionControl
//	Set up P4 if required
void CPlatform::InitializeVersionControl()
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if( m_versionControlFn != nullptr )
	{
		m_versionControlFn();
	}
#endif
}
