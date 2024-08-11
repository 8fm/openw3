/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "platform.h"
#include "../redSystem/crt.h"
#include "../redIO/redIO.h"
#include "../core/explatformos.h"
#include "../core/gameConfiguration.h"
#include "../core/version.h"
#include "../core/loadingProfiler.h"
#include "../core/asyncIO.h"
#include "../core/depot.h"
#include "../core/configVarSystem.h"
#include "../core/configVarStorage.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/scriptingSystem.h"
#include "../core/feedback.h"
#include "../engine/platformViewport.h"
#include "../../orbis/platformOrbis/userProfileOrbis.h"
#include "../../orbis/platformOrbis/inputDeviceManagerOrbis.h"
#include "../../orbis/platformOrbis/streamingInstallerOrbis.h"

// for quick boot
#include "../engine/renderer.h"
#include "../engine/scaleformSystem.h"
#include "../engine/shaderCacheManager.h"
#include "../engine/videoPlayer.h"
#include "../engine/renderCommands.h"
#include "../engine/flashPlayer.h"
#include "../engine/baseEngine.h"
#include "../engine/localizationManager.h"
#include "../engine/soundSystem.h"
#include "../engine/viewportWindowMode.h"
#include "../renderer/renderVideoPlayer.h"
RED_DEFINE_STATIC_NAME( VideoClient_MenuBackground );


#ifndef NO_DEBUG_PAGES
#ifdef RED_NETWORK_ENABLED
#include <libnetctl.h>
#endif // RED_NETWORK_ENABLED
#endif // NO_DEBUG_PAGES
#include "../engine/inGameConfig.h"

// For details see: T1-16-DebuggingTargetManagementAndDistributedBuilds.pdf
// or Debugger-Users_Guide_e.pdf.
#ifndef RED_FINAL_BUILD
# include <dbg_enc.h>
# pragma comment(lib, "libSceDbgEnc.a")
#endif
#include "../core/tokenizer.h"


#include <system_service.h> // for sceSystemServiceHideSplashScreen
#include "../../orbis/platformOrbis/contentManagerOrbis.h"
#include "../../orbis/platformOrbis/dlcInstallerOrbis.h"

namespace Orbis
{
	CDlcInstallerOrbis* GDlcInstaller;
	CStreamingInstallerOrbis* GStreamingInstaller;
}

/////////////////////////////////////////////////////////////////////
// User config loader saver
class COrbisUserConfigLoaderSaver : public Config::IUserConfigLoaderSaver
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
	COrbisUserConfigLoaderSaver* GOrbisUserConfigLoaderSaver;
}

/////////////////////////////////////////////////////////////////////
// Orbis Viewport
//	Just a stub since the renderer can handle everything and there is
//  no real message loop
class ViewportStub : public IPlatformViewport
{
public:
	Bool PumpMessages()
	{ 
#ifndef RED_FINAL_BUILD
		if ( ::sceDbgEnCCheckForPendingChanges() )
		{
			LOG_ENGINE(TXT("Applying edit and continue changes..."));
			::sceDbgEnCApplyChangesNow();
			LOG_ENGINE(TXT("Done applying edit and continue changes"));
		}
#endif // !RED_FINAL_BUILD
		return true;
	}
};

/////////////////////////////////////////////////////////////////////
// CreateViewport
//	Retrieves a viewport interface 
IPlatformViewport* CPlatform::CreateViewport( bool isEditor )
{
	static IPlatformViewport* internalVP = new ViewportStub();
	return internalVP;
}

static String GetOrbisElfName()
{
	/*
	 Get executing module name?
	 https://ps4.scedev.net/forums/thread/5574/

	Leon Hui
	SCEA Developer Support
		Reply to this postMore from this user
		Permalink 2013-10-15 19:38 • 2 months ago
		The first entry of the module list is always the executable.  We will update the documentation by 1.600 to reflect.
	*/

	SceKernelModule modules[512];
	size_t actualNum = 0;
	if ( ::sceKernelGetModuleList( modules, ARRAY_COUNT(modules), &actualNum ) != SCE_OK || actualNum < 1 )
	{
		RED_HALT("Could not get Orbis kernel module list" );
		return String::EMPTY;
	}

	SceKernelModuleInfo moduleInfo;
	moduleInfo.size = sizeof(SceKernelModuleInfo);
	if ( ::sceKernelGetModuleInfo( modules[0], &moduleInfo ) != SCE_OK )
	{
		RED_HALT("Could not get Orbis kernel module info" );
		return String::EMPTY;
	}

	const String moduleName = ANSI_TO_UNICODE(moduleInfo.name);

	if ( ! moduleName.EndsWith(TXT(".elf")) && moduleName != TXT("eboot.bin") )
	{
		RED_HALT("First Orbis module '%ls' was not an ELF?", moduleName.AsChar() );
		return String::EMPTY;
	}

	// Return <name>.elf since the format is r4gameOrbis.config.elf,
	// and scripts knock off the .elf but we don't want them to knock off the .config
	return moduleName;
}

static void SetupPlatformPathsSplitCook( String &rootPath, String &workingPath, String &dataPath, String &bundlePath, String &userPath, String &partPath, String &configPath, String &scriptPath )
{
	workingPath	= TXT( "/app0/bin/" );
	rootPath	= TXT( "/app0/" );
	dataPath	= TXT("/app0/content/content0/");
	bundlePath	= TXT("/app0/content/");
	userPath	= dataPath;
	partPath	= GetOrbisElfName();
	configPath	= workingPath + TXT( "r4config/" );
	scriptPath	= dataPath + TXT( "scripts/" );
}

/////////////////////////////////////////////////////////////////////
// SetupPlatformPaths
//	Sets up and retrieves engine paths
void CPlatform::SetupPlatformPaths( String &rootPath, String &workingPath, String &dataPath, String &bundlePath, String &userPath, String &partPath, String &configPath, String &scriptPath )
{
	CGameConfiguration& gameConfig = GGameConfig::GetInstance();
	
	workingPath	= TXT( "/app0/bin/" );
	rootPath	= TXT( "/app0/" );
	dataPath	= String::Printf(TXT("/app0/%ls/"), gameConfig.GetDataPathSuffix().AsChar());
	bundlePath	= String::Printf(TXT("/app0/%ls/"), gameConfig.GetBundlePathSuffix().AsChar());
	userPath	= dataPath;
	partPath	= GetOrbisElfName();
	configPath	= workingPath + TXT( "r4config/" );
	scriptPath	= String::Printf(TXT("/app0/%ls/"), gameConfig.GetScriptsPathSuffix().AsChar());
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

	// DONT CHECK THIS IN!
	//while (sceDbgIsDebuggerAttached() == 0)
	//{
	//	sleep(1);
	//}


	GSplash->UpdateProgress( TXT("Initializing system...") );

	// Set ID of main thread
	extern Red::System::Internal::ThreadId GMainThreadID;
	GMainThreadID.InitWithCurrentThread();

	// Grab command line
	extern String* GCommandLine;
	GCommandLine = new String( commandLine );

	// FIXME: Why pass in unaltered commandLine as a ansichar?
	// Set up engine paths
	String partPath, rootPath, workingPath, dataPath, bundlePath, scriptPath, configPath, userPath;
	if ( coreArguments.m_splitCook )
	{
		SetupPlatformPathsSplitCook( rootPath, workingPath, dataPath, bundlePath, userPath, partPath, configPath, scriptPath );
	}
	else
	{
		SetupPlatformPaths( rootPath, workingPath, dataPath, bundlePath, userPath, partPath, configPath, scriptPath );
	}

	Bool isReadOnly = true;
#ifndef RED_FINAL_BUILD
	{
		if ( !coreArguments.m_useBundles )
		{
			isReadOnly = false;
		}
	}
#endif

	// Initialize file system
	GFileManager = new CFileManager( TXT("/app0/"), rootPath.AsChar(), dataPath.AsChar(), bundlePath.AsChar(), isReadOnly );
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

	// Initialize configuration system
	GSplash->UpdateProgress( TXT("Loading engine configuration...") );


	GContentManager = new CContentManagerOrbis;
	GContentManager->Init();

	Bool streamInstall = Red::System::StringSearch( SGetCommandLine(), TXT( "-streaminstall" ) ) != nullptr;
	if ( streamInstall )
	{
		Orbis::GStreamingInstaller = new CStreamingInstallerOrbis;
		if ( !Orbis::GStreamingInstaller->Init() )
		{
			delete Orbis::GStreamingInstaller;
			Orbis::GStreamingInstaller = nullptr;
			RED_FATAL( "Could not create CStreamingInstallerOrbis" );
		}

		GContentManager->RegisterContentInstaller( Orbis::GStreamingInstaller );
	}
	else
	{
		EContentSource contentSource = eContentSource_LooseFiles;
		/*if ( GDepot->IsUsingBundles() )*/
		// GDepot might not be initialized yet.
		if ( coreArguments.m_useBundles )
		{
			contentSource = ( coreArguments.m_splitCook ) ? eContentSource_SplitCook : eContentSource_Cook;
		}

		GContentManager->ScanFoldersForContent( contentSource );
	}

	const Bool dlcInstall = Red::System::StringSearch( SGetCommandLine(), TXT( "-dlcinstall" ) ) != nullptr;
	if ( dlcInstall )
	{
		Orbis::GDlcInstaller = new CDlcInstallerOrbis;
		if ( !Orbis::GDlcInstaller->Init() )
		{
			delete Orbis::GDlcInstaller;
			Orbis::GDlcInstaller = nullptr;
			RED_FATAL( "Could not create CDlcInstallerOrbis" );
		}

		GContentManager->RegisterContentInstaller( Orbis::GDlcInstaller );
	}

	GConfig = new CConfigManager( configPath );

	GUserProfileManager = new CUserProfileManagerOrbis;

	GOrbisUserConfigLoaderSaver = new COrbisUserConfigLoaderSaver();

	// Initialize configuration system
	GSplash->UpdateProgress( TXT("Loading configuration...") );
	SConfig::GetInstance().Load( workingPath + TXT("config\\"), GOrbisUserConfigLoaderSaver );	// When all configs will be in new path - change configPath and apply here
	GInGameConfig::GetInstance().GenerateConfigs( workingPath + TXT("config\\") );	// When all configs will be in new path - change configPath and apply here

	extern IInputDeviceManager* GInputDeviceManager;
	GInputDeviceManager = new CInputDeviceManagerOrbis;

	// Preinitialize sound system
	CSoundSystem::Init( workingPath + TXT("initialdata/sound/") );

	// Copy configuration files
#ifdef RED_FINAL_BUILD
	{
		Char configTemplatePath[ Red::IO::REDIO_MAX_PATH_LENGTH ];
		Red::System::StringCopy( configTemplatePath, rootPath.AsChar(), Red::IO::REDIO_MAX_PATH_LENGTH );
		Char* pEnd = Red::System::StringSearchLast( configTemplatePath, '\\' );
		if ( pEnd ) *pEnd = '\0';
		Red::System::StringConcatenate( configTemplatePath, TXT("\\config\\"), Red::IO::REDIO_MAX_PATH_LENGTH );
	}
#endif

	// Account everything to platform stuff
	GLoadingProfiler.FinishStage( TXT("PlatformInit") );

	// Initialize depot + resource monitoring if required
#ifdef ENABLE_RESOURCE_MONITORING
	String resourceMonitorEnabled = TXT("false");
	SConfig::GetInstance().GetValue( "Engine", "EnableResourceMonitor", resourceMonitorEnabled );
	if( resourceMonitorEnabled.ToLower() == TXT("true") )
	{
		ResourceMonitorStats::EnableMonitoring( true );
	}
#endif

	// Initialize scripting system
	GSplash->UpdateProgress( TXT("Initializing scripts...") );
	GScriptingSystem = new CScriptingSystem( scriptPath.AsChar() );

	// StartBumperMovies
	if (1)
	{
		GDeferredInit = new CDeferredInit (dataPath);

		// create Viewport here, need to pass it up the stack when we've finished
		GSplash->UpdateProgress( TXT( "Creating Renderer..." ) );
		IPlatformViewport* vp = CreateViewport(false);
		extern IRender* SCreateRender( IPlatformViewport* );
		GRender = SCreateRender( vp );

		if ( !GShaderCache->Init() )
		{
			ERR_ENGINE(TXT("Failed to initialize shader cache!"));
		}

		// Initialize object /RTTI systems so we can serialize resources
		CObject::InitializeSystem();

		extern void InitializeRTTI();
		InitializeRTTI();

#ifdef USE_SCALEFORM
		// Create GUI
		// Need to be before GRender initialization, because we overwrite Scaleform memory allocator and it needs to be done before first Scaleform allocation
		GSplash->UpdateProgress( TXT("Creating GUI...") );
		RED_VERIFY( CScaleformSystem::StaticInstance() );
#endif

		GSplash->UpdateProgress( TXT("Initializing Renderer...") );
		RED_VERIFY( GRender->Init() );

		// CreateGameViewport kicks off the render thread
		QuickBoot::g_quickInitViewport = GRender->CreateGameViewport( TXT("The Witcher 3"), 1920, 1080, VWM_Fullscreen );

#ifdef USE_SCALEFORM
		GSplash->UpdateProgress( TXT("Initializing GUI...") );
		// Needs to be after render thread init
		RED_VERIFY( CScaleformSystem::StaticInstance()->Init( QuickBoot::g_quickInitViewport.Get() ) );
#endif

		// create Flash Player
		CFlashPlayer* flashPlayer = CFlashPlayer::CreateFlashPlayerInstance();
		QuickBoot::g_quickInitFlashPlayer = flashPlayer;

		// load videoplayer swf
		CVideoPlayer::CreateGlobalVideoPlayer(flashPlayer);

		struct BumperConfig
		{
			const Char* m_fileName;
			Bool		m_loop;
		} bumperConfigs[] = { { TXT( "cutscenes\\gamestart\\bumpers\\bumpers.usm" ), false }, { TXT( "cutscenes\\gamestart\\loading.usm" ), true } };

		IRenderVideo* renderVideo = nullptr;
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(bumperConfigs); ++i )
		{
			if ( renderVideo )
			{
				renderVideo->Release();
				renderVideo = nullptr;
			}
			const BumperConfig& config = bumperConfigs[i];
			const Char* fileName = config.m_fileName;
			const SVideoParams videoParams( fileName, eVideoParamFlag_Bumpers | (config.m_loop ? eVideoParamFlag_PlayLooped : 0), eVideoBuffer_Bumpers );
			renderVideo = GRender->CreateVideo( CNAME( VideoClient_MenuBackground ), videoParams );
			(new CRenderCommand_PlayVideo(renderVideo, eVideoThreadIndex_RenderThread))->Commit();
		}

		QuickBoot::g_lastBumperVideo = renderVideo; // we'll release it in baseEngineInit

		// hide the splash screen because we should be seeing videos any moment!
		::sceSystemServiceHideSplashScreen();
	}

	GSplash->UpdateProgress( TXT("Scanning depot...") );
	GDepot = new CDepot( dataPath );
	{
		CTimeCounter timer; 
		GDepot->Initialize( coreArguments );
		RED_LOG( Core, TXT("Initializing depot took %1.1f sec"), timer.GetTimePeriod() );
		GLoadingProfiler.FinishStage( TXT("Depot") );
	}


#ifndef NO_DEBUG_PAGES
#ifdef RED_NETWORK_ENABLED

	extern Char* GIpAddress;

	GIpAddress = new Char[ SCE_NET_CTL_IPV4_ADDR_STR_LEN ];

	SceNetCtlInfo info;
	Int32 sceErr = sceNetCtlGetInfo( SCE_NET_CTL_INFO_IP_ADDRESS, &info );

	if( sceErr == SCE_OK )
	{
		Red::System::StringConvert( GIpAddress, info.ip_address, SCE_NET_CTL_IPV4_ADDR_STR_LEN );
	}
	else
	{
		Red::System::SNPrintF( GIpAddress, SCE_NET_CTL_IPV4_ADDR_STR_LEN, TXT( "GetIP Err: %i" ), sceErr );
	}

#endif // RED_NETWORK_ENABLED
#endif // NO_DEBUG_PAGES
}

//////////////////////////////////////////////////////////////////////////
// Shutdown
//	Close down the platform-specific bits initialised above
void CPlatform::Shutdown()
{
#ifndef NO_DEBUG_PAGES
#ifdef RED_NETWORK_ENABLED

	extern Char* GIpAddress;

	delete [] GIpAddress;
	GIpAddress = nullptr;

#endif // RED_NETWORK_ENABLED
#endif // NO_DEBUG_PAGES

	SConfig::GetInstance().Shutdown();

	delete GOrbisUserConfigLoaderSaver;

	delete GUserProfileManager;

	delete GConfig;
		delete GScriptingSystem;

	delete GDepot;

	delete GContentManager;
	delete Orbis::GStreamingInstaller;
	delete Orbis::GDlcInstaller;

	GDeprecatedIO->Shutdown();
	delete GDeprecatedIO;

	delete GFileManager;

	extern String* GCommandLine;
	delete GCommandLine;
}

//////////////////////////////////////////////////////////////////////////
// InitializeVersionControl
//	Set up P4 if required
void CPlatform::InitializeVersionControl()
{
}
