/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "platform.h"

#include "../redSystem/crt.h"
#include "../redSystem/errorDurango.h"

#include "../core/explatformos.h"
#include "../core/gameConfiguration.h"
#include "../core/version.h"
#include "../core/asyncIO.h"
#include "../core/depot.h"
#include "../core/configVarSystem.h"
#include "../core/configVarStorage.h"

#include "../engine/inGameConfig.h"

#include "../../durango/platformDurango/userProfileDurango.h"
#include "../../durango/platformDurango/inputDeviceManagerDurango.h"
#include "../../common/core/configFileManager.h"
#include "../../common/core/scriptingSystem.h"
#include "../../common/core/feedback.h"
#include "../engine/platformViewport.h"

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
#include "../renderer/renderVideoPlayer.h"
#include "../engine/renderSettings.h"
RED_DEFINE_STATIC_NAME( VideoClient_MenuBackground );

#ifndef NO_TEST_FRAMEWORK
#include "../engine/testFramework.h"
#include "../engine/userProfileMock.h"
#endif

#if defined( RED_KINECT )
#include "../../durango/platformDurango/kinectSpeechRecognizer.h"
#endif
#include "../redIO/redIO.h"
#include "../../durango/platformDurango/streamingInstallerDurango.h"
#include "../../durango/platformDurango/dlcInstallerDurango.h"
#include "../core/contentManager.h"

namespace Durango
{
	CDlcInstallerDurango*		GDlcInstaller;
	CStreamingInstallerDurango* GStreamingInstaller;
}

/////////////////////////////////////////////////////////////////////
// Durango viewport stub. Since the renderer really should not be firing messages on xbox, 
// we leave this empty
class CDurangoViewportStub : public IPlatformViewport
{
public:
	CDurangoViewportStub()	{ }
	virtual ~CDurangoViewportStub() { }
	Bool PumpMessages() { return true; }
};

/////////////////////////////////////////////////////////////////////
// User config loader saver
class CDurangoUserConfigLoaderSaver : public Config::IUserConfigLoaderSaver
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
	CDurangoUserConfigLoaderSaver* GDurangoUserConfigLoaderSaver;
}

/////////////////////////////////////////////////////////////////////
// CreateViewport
//
IPlatformViewport* CPlatform::CreateViewport( bool isEditor )
{
	return new CDurangoViewportStub;
}

static void SetupPlatformPathsSplitCook( String &rootPath, String &workingPath, String &dataPath, String &bundlePath, String &userPath, String &partPath, String &configPath, String &scriptPath )
{				// Pull deployment. 
	Char moduleFileName[MAX_PATH];
	GetModuleFileName( NULL, moduleFileName, MAX_PATH );

	workingPath	= TXT( "g:/bin/" );
	rootPath	= TXT( "g:/" );
	dataPath	= TXT( "g:/content/content0/" );
	bundlePath	= TXT( "g:/content/" );
	userPath	= dataPath;
	partPath	= moduleFileName;
	configPath	= workingPath + TXT( "r4config/" );
	scriptPath	= dataPath + TXT( "scripts/" );
}

/////////////////////////////////////////////////////////////////////
// SetupPlatformPaths
//	These should return the various paths used to initialise the engine systems
void CPlatform::SetupPlatformPaths( String &rootPath, String &workingPath, String &dataPath, String &bundlePath, String &userPath, String &partPath, String &configPath, String &scriptPath )
{				// Pull deployment. 
	Char moduleFileName[MAX_PATH];
	GetModuleFileName( NULL, moduleFileName, MAX_PATH );

	workingPath	= TXT( "g:/bin/" );
	rootPath	= TXT( "g:/" );
	dataPath	= TXT( "g:/r4data/" );
	bundlePath	= TXT( "g:/bundles/" );
	userPath	= dataPath;
	partPath	= moduleFileName;
	configPath	= workingPath + TXT( "r4config/" );
	scriptPath	= dataPath + TXT( "scripts/" );
}

/////////////////////////////////////////////////////////////////////
// Initialise
//	Initialise the platform-specific bits of the engine that are common to all Durango executables
void CPlatform::Initialize( const String& commandLine, const Core::CommandLineArguments& coreArguments )
{
	// Initialise the error / assert systems as soon as possible
#ifndef NO_ASSERTS
	Red::System::Error::Handler::GetInstance()->RegisterAssertHook( m_assertHandlerFn );
	Red::System::Error::Handler::GetInstance()->SetVersion( APP_VERSION_NUMBER );
#endif

	GSplash->UpdateProgress( TXT("Initializing system...") );

	// Get ID of main thread
	extern Red::System::Internal::ThreadId GMainThreadID;
	GMainThreadID.InitWithCurrentThread();

	// Grab command line
	extern String* GCommandLine;
	GCommandLine = new String( commandLine );

	// Set up paths
	String partPath, workingPath, dataPath, bundlePath, scriptPath, configPath, userPath, rootPath;

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
	GFileManager = new CFileManager( TXT("g:/"), workingPath.AsChar(), dataPath.AsChar(), bundlePath.AsChar(), isReadOnly );
	GFileManager->SetUserDirectory( userPath.AsChar() );
	GFileManager->SetApplicationFilename( partPath.AsChar() );

	SYSTEMTIME time;
	::GetSystemTime( &time );

	const String timestamp = String::Printf( TXT("%02d%02d%04d-%02d%02d%02d"),
		(Int32)time.wMonth, (Int32)time.wDay, (Int32)time.wYear,
		(Int32)time.wHour, (Int32)time.wMinute, (Int32)time.wSecond );

	const String miniDumpFileName =  String::Printf( TXT("d:\\%s-%s.dmp"), 
		GFileManager->GetApplicationFilename()->GetFileName().AsChar(),
		timestamp.AsChar() );

	Red::System::Error::DurangoHandler::SetMiniDumpFileName( miniDumpFileName.AsChar() );

#ifdef RED_LOGGING_ENABLED
	const String crashLogFileName = String::Printf( TXT("d:\\%s-%s.log"),
		GFileManager->GetApplicationFilename()->GetFileName().AsChar(),
		timestamp.AsChar() );

	Red::System::Error::DurangoHandler::SetCrashLogFileName( crashLogFileName.AsChar() );
#endif

	// Log the minidump filename for later to see if it matches up to the provided log file. Nice if our
	// logger wasn't static so we could have a dynamic log name.
	LOG_CORE( TXT("Minidump file name for this session: %s"), miniDumpFileName.AsChar() );

	// Initialize async I/O
	GDeprecatedIO = new CDeprecatedIO;
	if ( ! GDeprecatedIO->Init() )
	{
		delete GDeprecatedIO;
		GDeprecatedIO = nullptr;
	}
	RED_FATAL_ASSERT( GDeprecatedIO, "Could not create async file manager" );

	// If you subclass the content manager, have a good reason. Soon to remove being subclasses per platform.
 	GContentManager = new CContentManager;
 	GContentManager->Init();

	const Bool streamInstall = Red::System::StringSearch( SGetCommandLine(), TXT( "-streaminstall" ) ) != nullptr;
	if ( streamInstall )
	{
		Durango::GStreamingInstaller = new CStreamingInstallerDurango;
		if ( !Durango::GStreamingInstaller->Init() )
		{
			delete Durango::GStreamingInstaller;
			Durango::GStreamingInstaller = nullptr;
			RED_FATAL( "Could not create CStreamingInstallerDurango" );
		}
		GContentManager->RegisterContentInstaller( Durango::GStreamingInstaller );
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
		Durango::GDlcInstaller = new CDlcInstallerDurango;
		if ( !Durango::GDlcInstaller->Init() )
		{
			delete Durango::GDlcInstaller;
			Durango::GDlcInstaller = nullptr;
			RED_FATAL( "Could not create CDlcInstallerDurango" );
		}

		GContentManager->RegisterContentInstaller( Durango::GDlcInstaller );
	}

	// Initialize legacy configuration system
	GSplash->UpdateProgress( TXT("Loading legacy configuration...") );
	GConfig = new CConfigManager( configPath );

	// Initialize configuration system
	GSplash->UpdateProgress( TXT("Loading configuration...") );

	extern IInputDeviceManager* GInputDeviceManager;
	CInputDeviceManagerDurango* durangoInputDeviceManager = new CInputDeviceManagerDurango;
	GInputDeviceManager = durangoInputDeviceManager;
#ifndef NO_TEST_FRAMEWORK
	if ( STestFramework::GetInstance().CommandlineStartsTestFramework( SGetCommandLine() ) ) 
	{
		GUserProfileManager = new CUserProfileManagerMock;
	}
	else
#endif
	{
		GUserProfileManager = new CUserProfileManagerDurango;
		GUserProfileManager->Initialize();

		static_cast< CUserProfileManagerDurango* >( GUserProfileManager )->AddGamepadListener( durangoInputDeviceManager );
	}

	GDurangoUserConfigLoaderSaver = new CDurangoUserConfigLoaderSaver();

	SConfig::GetInstance().Load( workingPath + TXT("config\\"), GDurangoUserConfigLoaderSaver );	// When all configs will be in new path - change configPath and apply here
	GInGameConfig::GetInstance().GenerateConfigs( workingPath + TXT("config\\") );	// When all configs will be in new path - change configPath and apply here

	// Preinitialize sound system
	CSoundSystem::Init( workingPath + TXT("initialdata\\sound\\") );

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
		Int32 width;
		Int32 height;
		Config::Helper::ExtractDisplayModeValues( Config::cvResolution.Get(), width, height );
		QuickBoot::g_quickInitViewport = GRender->CreateGameViewport( TXT("The Witcher 3"), width, height, VWM_Fullscreen );

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

		// NOTE: The main reason on Durango is to avoid the userprofile manager from allowing you to sign in before the 
		// start screen is even visible because the bumpers are still playing while we tick the engine.
		QuickBoot::g_lastBumperVideo = renderVideo; // we'll release it in baseEngineInit
	}

	GSplash->UpdateProgress( TXT("Scanning depot...") );
	GDepot = new CDepot( dataPath );
	{
		CTimeCounter timer; 
		GDepot->Initialize( coreArguments );
		RED_LOG( Core, TXT("Initializing depot took %1.1f sec"), timer.GetTimePeriod() );
	}

#ifdef RED_NETWORK_ENABLED

	//to allow wwise authoring tool to connect on durango
	auto secureDeviceAssociationTemplate1 = Windows::Xbox::Networking::SecureDeviceAssociationTemplate::GetTemplateByName( "WwiseDiscovery" );

	//to allow red networking enabled applications to connect
	auto secureDeviceAssociationTemplate2 = Windows::Xbox::Networking::SecureDeviceAssociationTemplate::GetTemplateByName( "RedToolDiscovery" );
	auto secureDeviceAssociationTemplate3 = Windows::Xbox::Networking::SecureDeviceAssociationTemplate::GetTemplateByName( "RedToolSleep" );

	auto secureDeviceAssociationTemplate4 = Windows::Xbox::Networking::SecureDeviceAssociationTemplate::GetTemplateByName( "RedDebugHTMLPages" );

#ifndef NO_DEBUG_PAGES
	try
	{
		auto hostNames = Windows::Networking::Connectivity::NetworkInformation::GetHostNames();

		Uint32 numHostnames = hostNames->Size;
		for( Uint32 i = 0; i < numHostnames; ++i )
		{
			auto hostname = hostNames->GetAt( i );
			RED_LOG( Net, TXT( "Host %u: CanonicalName: %ls, Display: %ls, Raw:" ), i, hostname->CanonicalName->Data(), hostname->DisplayName->Data(), hostname->RawName->Data() );
		}

		// This index is based on the results of printing out the results of the hostname array (above)
#define IP_ADDRESS_INDEX 2

		if( numHostnames > IP_ADDRESS_INDEX )
		{
			auto hostname = hostNames->GetAt( IP_ADDRESS_INDEX );
			Platform::String^ systemIP = hostname->CanonicalName;

			Uint32 size = systemIP->Length() + 1;
			extern Char* GIpAddress;
			GIpAddress = new Char[ size ];

			Red::System::StringCopy( GIpAddress, systemIP->Data(), size );
		}
	}
	catch( Platform::Exception^ ex )
	{
		RED_LOG_WARNING( Net, TXT( "Exception getting hostnames (debug information): error code = 0x%x, message = %s" ), ex->HResult, ex->Message );
	}
#endif // NO_DEBUG_PAGES
#endif // RED_NETWORK_ENABLED

#if defined( RED_KINECT )
	CKinectSpeechRecognizer* kinectSpeechRecognizer = new CKinectSpeechRecognizer();
	SPlatformFeatureManager::GetInstance().SetFeature( PF_Kinect, (CPlatformFeature*) kinectSpeechRecognizer );
#endif
}

//////////////////////////////////////////////////////////////////////////
// Shutdown
//	Close down the platform-specific bits initialised above
void CPlatform::Shutdown()
{
#if defined( RED_KINECT )
	CKinectSpeechRecognizer* kinect =  (CKinectSpeechRecognizer*)SPlatformFeatureManager::GetInstance().GetFeature( PF_Kinect );
	if( kinect != NULL )
	{
		SPlatformFeatureManager::GetInstance().RemFeature( PF_Kinect );
		delete kinect;
	}
#endif

	SConfig::GetInstance().Shutdown();

	delete GDurangoUserConfigLoaderSaver;

	delete GUserProfileManager;
	GUserProfileManager = nullptr;

	delete GConfig;

	delete GScriptingSystem;

	delete GDepot;

	GDeprecatedIO->Shutdown();
	delete GDeprecatedIO;

	delete GFileManager;

	extern String* GCommandLine;
	delete GCommandLine;
}

//////////////////////////////////////////////////////////////////////////
// InitializeVersionControl
//	Does nothing on consoles
void CPlatform::InitializeVersionControl()
{

}
