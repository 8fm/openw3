/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "baseEngine.h"

#include "../redSystem/cpuid.h"
#include "../redMemoryFramework/redMemorySystemMemoryStats.h"
#include "../core/loadingJobManager.h"
#include "../core/taskManager.h"
#include "../core/dependencyLoader2dArray.h"
#include "../core/dependencySaver2dArray.h"
#include "../core/asyncIO.h"
#include "../core/depot.h"
#include "../core/configFileManager.h"
#include "../core/scriptingSystem.h"
#include "../core/resourceDefManager.h"
#include "../core/feedback.h"
#include "../core/rttiSerializer.h"
#include "../core/objectDiscardList.h"
#include "../core/objectGC.h"
#include "../core/debugPageServer.h"
#include "../core/gatheredResource.h"
#include "../core/gameConfiguration.h"
#include "../core/loadingProfiler.h"
#include "../core/tokenizer.h"
#include "../core/contentManager.h"
#include "../core/remoteConfig.h"
#include "../core/diskBundle.h"
#include "../core/messagePump.h"
#include "../core/sha256.h"
#include "../core/version.h"
#include "dependencyLoaderSwf.h"

#include "../gpuApiUtils/gpuApiMemory.h"

#include "renderCommands.h"
#include "remoteConnection.h"
#include "scaleformSystem.h"
#include "soundSystem.h"
#include "animationCache2.h"
#include "dependencyLoaderSRT.h"
#include "../physics/physicsDebugger.h"
#include "debugServerManager.h"
#include "debugPageManagerTabbed.h"
#include "videoPlayer.h"
#include "scriptsHash.h"
#include "apexWrapperBase.h"

#ifndef NO_RED_GUI
  #include "redGuiManager.h"
  #ifndef NO_DEBUG_WINDOWS
    #include "debugWindowsManager.h"
  #endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

#ifdef USE_PHYSX
#include "../physics/physXEngine.h"
#endif // USE_PHYSX 

#ifndef NO_MARKER_SYSTEMS
  #include "markersSystem.h"
#endif	// NO_MARKER_SYSTEMS

#include "debugPageManagerBase.h"
#include "collisionCache.h"
#include "particlePool.h"
#include "shaderCacheManager.h"
#include "renderSettings.h"
#include "animationManager.h"
#include "textureCache.h"
#include "occlusionSystem.h"
#include "inputDeviceManager.h"
#include "cookedbinStringDBDataAccess.h"
#include "localizationManager.h"

#include "../core/fileSys.h"
#include "../core/depotBundles.h"
#include "../core/cacheFilenames.h"
#include "../core/xmlFileReader.h"
#include "../core/commandLineParser.h"
#include "sectorDataRawData.h"
#include "sectorDataGlobals.h"
#include "clipMap.h"

Bool GDebugSaves = false;
Bool GUseSaveHack = false;

static const MemSize PS4_MAX_DIRECT_MEMORY_FREE = 2 * 1024 * 1024;		// If there is > this, then more memory should be given to the pools
static const Uint16 EDITOR_PORT = 37000;
static const Uint16 GAME_PORT = 37001;

CGatheredResource resEngineFontResource( TXT("engine\\fonts\\parachute.w2fnt"), RGF_Startup );

namespace Config
{
	TConfigVar< Int32 > cvSectorDataPreallocatedBufferSizeInMB( "BaseEngine", "SectorDataPreallocatedBufferSizeInMB", 34, eConsoleVarFlag_ReadOnly );
}

Bool CBaseEngine::InitializeContentManager()
{
	if ( GDepot->IsUsingBundles() )
	{
		const AnsiChar* filters[] = { "*.store", "*.bundle", "*.cache" };
		GContentManager->RegisterContentListener( GDepot, filters, 3 );
	}

	GContentManager->RegisterContentListener( GCollisionCache, "collision.cache" );
	GContentManager->RegisterContentListener( GTextureCache, "texture.cache" );
	GContentManager->RegisterContentListener( &GSoundCacheResolver::GetInstance(), "sounds*.cache" );
	if ( !GShaderCache->Init() )
	{
		ERR_ENGINE(TXT("Failed to initialize shader cache!"));
		return false;
	}

	{
		const AnsiChar* filters[] = { UNICODE_TO_ANSI( SHADER_CACHE_FILENAME ), "furshader.cache" };
		GContentManager->RegisterContentListener( GShaderCache, filters, 2 );
	}

	if ( nullptr != Red::System::StringSearch( SGetCommandLine(), TXT("-useCookedLocale") ) )
	{
		const StringAnsi w3strings = StringAnsi::Printf("*%ls", STRINGS_FILE_PATH_POSTFIX.AsChar());
		const StringAnsi w3speech = StringAnsi::Printf("*%ls", CURRENT_SPEECH_FILE_PATH_POSTFIX.AsChar());
		const AnsiChar* fileFilterWildcards[] = { w3strings.AsChar(), w3speech.AsChar() };
		GContentManager->RegisterContentListener( GCookedStringsDB, fileFilterWildcards, ARRAY_COUNT_U32(fileFilterWildcards) );
	}

	{
		CTimeCounter timer;

		// Note this includes content *attaching* as well, so don't be too harsh on the timeout
		// This is just to give some indication of what's going on
		const Float timeout = 60.f;
#ifdef RED_LOGGING_ENABLED
		Bool warned = false; // what's with WARN_ONCE now?
#endif
		do 
		{
			GContentManager->Update();
#ifdef RED_PLATFORM_WINPC
			Red::Threads::SleepOnCurrentThread( 5 );
#endif
#ifdef RED_LOGGING_ENABLED
			if ( !warned && timer.GetTimePeriod() >= timeout )
			{
				warned = true;
				WARN_ENGINE(TXT("InitializeContentManager: Waiting for content is taking too long!"));
			}
#endif
		}
		while ( !GContentManager->IsReady( eContentMask_BootUp ) );

		LOG_ENGINE(TXT("Waiting for content manager took %1.2f sec"), timer.GetTimePeriod() );
	}

	//FIXME: Need an init function. Creating here so GCookedStringsDB gets set up with a language to test with
	// Instantiate here after the content providers initialized, since this is where we set the user default language
	// NOTE: And before GDeferredInit->OnPostContentManagerInit() or else there won't be a delegate for init.
	SLocalizationManager::GetInstance();

	if (GDeferredInit)
	{
		GDeferredInit->OnPostContentManagerInit();
	}

	return true;
}

#ifdef RED_NETWORK_ENABLED
namespace NetworkHelper
{
	void* NetworkRealloc( void* ptr, size_t size )
	{
		return RED_MEMORY_REALLOCATE( MemoryPool_Default, ptr, MC_Network, size );
	}

	void NetworkFree( void* ptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Network, ptr );
	}
} // NetworkHelper
#endif // RED_NETWORK_ENABLED


void InitializeRTTI()
{
	static Bool rttiInitialised = false;
	if (rttiInitialised)
		return;
	rttiInitialised = true;

	GSplash->UpdateProgress( TXT("Initializing RTTI...") );

	{
		extern void RegisterCoreClasses();
		RegisterCoreClasses();		
	}

	// Initialize RTTI classes
	{
		// Engine
		extern void RegisterEngineClasses();
		RegisterEngineClasses();
	}

	{
		// Register material compiler classes
		extern void RegisterMatClasses();
		RegisterMatClasses();
	}
	{
		// Common game
		extern void RegisterCommonGameClasses();
		RegisterCommonGameClasses();
	}

	{
		// Register native game classes
		extern void RegisterGameClasses();
		RegisterGameClasses();
	}

	{
		// Register core names
		extern void RegisterCoreNames();
		RegisterCoreNames();
	}

	{
		// Register engine names
		extern void RegisterEngineNames();
		RegisterEngineNames();
	}

	{
		// Register common game names
		extern void RegisterCommonGameNames();
		RegisterCommonGameNames();
	}

	{
#ifndef RED_PLATFORM_ORBIS
		extern void	RegisterRendererNames();
		RegisterRendererNames();
#endif
	}

	{
		// Register material compiler names
		extern void RegisterMatNames();
		RegisterMatNames();
	}

	{
#ifndef RED_PLATFORM_CONSOLE
		// Register game names
		extern void RegisterGameNames();
		RegisterGameNames();
#endif
	}

	// Initialize base RTTI
	SRTTI::GetInstance().Init();
}


Bool CBaseEngine::InitializeRTTI()
{
	::InitializeRTTI();

#if !defined(RED_FINAL_BUILD)
	// Validate RTTI layout - just a sanity check
	if ( !SRTTI::GetInstance().ValidateLayout() )
	{
		ERR_ENGINE( TXT("RTTI layout corrupted!") );
		return false;
	}
#endif

	// Initialize native opcodes
	extern void ExportCoreOperators();
	ExportCoreOperators();
	extern void ExportPhysicsNatives();
	ExportPhysicsNatives();
	extern void ExportSoundNatives();
	ExportSoundNatives();
	extern void ExportCoreNatives();
	ExportCoreNatives();
	extern void ExportEngineGameTimeNatives();
	ExportEngineGameTimeNatives();
	extern void ExportEngineGameTimeIntervalNatives();
	ExportEngineGameTimeIntervalNatives();
	extern void ExportEnvironmentManagerNatives();
	ExportEnvironmentManagerNatives();
	extern void RegisterNodeFunctions();
	RegisterNodeFunctions();
	extern void RegisterCommonGameFunctions();
	RegisterCommonGameFunctions();
	extern void RegisterCSVFunctions();
	RegisterCSVFunctions();
	extern void RegisterRendererFunctions();
	RegisterRendererFunctions();
	extern void RegisterEngineScriptFunctions();
	RegisterEngineScriptFunctions();
	extern void RegisterEntityTemplateFunctions();
	RegisterEntityTemplateFunctions();
	extern void RegisterFlashFunctions();
	RegisterFlashFunctions();

#ifndef RED_FINAL_BUILD
	extern void ExportShowFlagsNatives();
	ExportShowFlagsNatives();
#endif


	// Custom resource loaders and savers
	new CDependencyLoader2dArrayFactory();
	new CDependencyLoaderSwfFactory();
	new CDependencySaver2dArrayFactory();
	new CDependencyLoaderSRTFactory();
	new CDependencySaverSRTFactory();

	// Dump current state of ___NATIVE___ RTTI
	const Bool dumpRTTI = Red::System::StringSearch( SGetCommandLine(), TXT("-dumprtti") ) != NULL;
	if ( dumpRTTI )
	{
		const String dumpFilePath = GFileManager->GetDataDirectory() + TXT("rttidump.xml");
		SRTTI::GetInstance().DumpRTTILayout( dumpFilePath.AsChar() );

		return false; // DO NOT CONTINUE
	}

	// done
	return true;
}

namespace ScriptCompilationHelpers
{
#ifndef RED_PLATFORM_CONSOLE
	typedef ECompileScriptsReturnValue (*SCRIPT_FAILURE_DLG_FUNC)( const CScriptCompilationMessages& );
	typedef void (*SCRIPT_SHOWHIDE_SPLASH_FUNC)(Bool show);
	
	static void DefaultScriptShowHideSplashFunc(Bool)
	{
		// Leave whatever GSplash is already set to
	}
	
	static ECompileScriptsReturnValue DefaultScriptFailureDlgFunc( const CScriptCompilationMessages& errorCollector )
	{
		ECompileScriptsReturnValue retVal = CSRV_Quit;
		const String message = TXT( "There were errors compiling scripts. Try again?" );
		if( MessageBox( NULL, message.AsChar(), TXT("Script compilation error"), MB_ICONHAND | MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND ) == IDYES )
		{
			// Try again!
			retVal = CSRV_Recompile;
		}
		else
		{
			const Char* unableToLoad = TXT( "Unable to load scripts" );
			if ( MessageBox( NULL, TXT( "Do you want to launch the editor either way?" ), unableToLoad, MB_ICONHAND | MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND ) == IDYES )
			{
				retVal = CSRV_Skip;
			}
		}

		return retVal;
	}
	
	SCRIPT_FAILURE_DLG_FUNC ScriptFailureDlgFunc = &DefaultScriptFailureDlgFunc;
	SCRIPT_SHOWHIDE_SPLASH_FUNC ScriptSplashFunc = &DefaultScriptShowHideSplashFunc;

	struct ScopedSplashScreen
	{
		ScopedSplashScreen()
		{
			ScriptSplashFunc(true);
		}

		~ScopedSplashScreen()
		{
			ScriptSplashFunc(false);
		}
	};
	
#endif // RED_PLATFORM_CONSOLE

	class ScriptPopupWindow : public IScriptCompilationFeedback
	{
	public:
		ScriptPopupWindow()
		{}

		static IScriptCompilationFeedback& GetDefaultInstance()
		{
			static ScriptPopupWindow theInstance;
			return theInstance;
		}

	protected:
		virtual ECompileScriptsReturnValue OnCompilationFailed( const CScriptCompilationMessages& errorCollector )
		{
			ECompileScriptsReturnValue retval = CSRV_Quit;

#ifndef RED_PLATFORM_CONSOLE
			retval = ScriptFailureDlgFunc( errorCollector );
#endif

			return retval;
		}
	};

	Bool CalculateScriptFilesCrc( const TDynArray<String>& scriptFiles, Uint64& outCrc )
	{
		const Uint32 BUFSZ= 4096;
		Uint8 buf[BUFSZ];

		CTimeCounter crcTimer;

		// If no script files or error then try loading the scripts anyway with a "no check" zero CRC
		outCrc = 0;

		if ( scriptFiles.Empty() )
		{
			ERR_CORE(TXT("GetScriptFilesCrc: No script files!"));
			return false;
		}

		Uint64 totalCrc = RED_FNV_OFFSET_BASIS64;
		for ( Uint32 i = 0; i < scriptFiles.Size(); ++i )
		{
			GSplash->UpdateProgress( TXT("Calculating scripts CRC (reading file %u/%u)..."), i, scriptFiles.Size() );

			const String& path = scriptFiles[i];
			Red::TScopedPtr<IFile> reader( GFileManager->CreateFileReader( path, FOF_AbsolutePath ) );
			if ( !reader )
			{
				ERR_CORE(TXT("GetScriptFilesCrc: failed to open '%ls'"), path.AsChar());
				return false;
			}

			Uint64 toRead = reader->GetSize();
			while ( toRead > 0 )
			{
				const Uint64 size = toRead > BUFSZ ? BUFSZ : toRead;
				toRead -= size;
				reader->Serialize( buf, size );
				totalCrc = Red::System::CalculateHash64( buf, size, totalCrc );
			}
		}

		LOG_ENGINE(TXT("Calculated CRC 0x%016llX from %u script files in %.2f sec"), totalCrc, scriptFiles.Size(), crcTimer.GetTimePeriod());

		outCrc = totalCrc;

		return true;
	}

	class ScriptCollector
	{
	public:
		static const Uint64 NO_CHECK_CRC = 0;

	public:
		ScriptCollector( Bool noCrcCheck = false )
			: m_crc(NO_CHECK_CRC)
			, m_noCrcCheck( noCrcCheck )
		{}

	public:
		const TDynArray<String>& GetScriptFiles()
		{ 
			RefreshScriptsIfNeeded();
			return m_scriptFiles;
		}

		Uint64 GetCrc() 
		{ 
			if ( m_noCrcCheck )
			{
				return NO_CHECK_CRC;
			}

			RefreshScriptsIfNeeded();
			return m_crc;
		}

		void Clear()
		{
			m_scriptFiles.ClearFast();
			m_crc = NO_CHECK_CRC;
		}

	private:
		void RefreshScriptsIfNeeded()
		{
			if ( !m_scriptFiles.Empty() )
			{
				return;
			}

			m_crc = NO_CHECK_CRC;
			GContentManager->EnumScriptFiles( m_scriptFiles );

			if ( !m_noCrcCheck && !CalculateScriptFilesCrc( m_scriptFiles, m_crc ) )
			{
				WARN_ENGINE(TXT("Failed to get script files CRC!"));
			}
		}

	private:
		TDynArray<String>	m_scriptFiles;
		Uint64				m_crc;
		Bool				m_noCrcCheck;
	};
}

#if defined( RED_PLATFORM_WINPC ) && defined( RED_USE_CRYPTO )
# define USE_SCRIPTS_HASH
#endif

#if defined( USE_SCRIPTS_HASH ) && defined( RED_FINAL_BUILD )
# define VALIDATE_KOSHER_SCRIPTS
#endif


#ifdef USE_SCRIPTS_HASH
namespace ScriptCompilationHelpers
{
	Bool GetScriptsSha256( const String& filePath, Ssha256& outHash )
	{
		Red::TScopedPtr<IFile> reader( GFileManager->CreateFileReader(filePath,FOF_AbsolutePath));
		if ( !reader )
		{
			ERR_ENGINE(TXT("Failed to open scripts '%ls' for reading!"), filePath.AsChar());
#ifdef RED_PLATFORM_WINPC
			OutputDebugString(TXT("ns\n"));
#endif
			return false;
		}

		// Skip past the header. Not worth the bother to verify with builds getting hand-modified.
		CRTTISerializer::RTTIHeader scriptsHeader;
		if ( reader->GetSize() <= sizeof(CRTTISerializer::RTTIHeader) )
		{
			ERR_ENGINE(TXT("Scripts '%ls' are empty!"), filePath.AsChar());
#ifdef RED_PLATFORM_WINPC
			OutputDebugString(TXT("zs\n"));
#endif
			return false;
		}
		scriptsHeader.Serialize( *reader);
		CalculateSha256( *reader, outHash );

		return true;
	}

	StringAnsi GetSHA256AsString( const Ssha256& hash )
	{
		const Uint32 HASHSZ = 32;
		AnsiChar hashstr[HASHSZ*2+1];
		AnsiChar* buf = hashstr;
		for ( Uint32 i = 0; i < HASHSZ; ++i )
		{
			const Uint8 val = hash.m_value[i];
			Red::System::SNPrintF( buf, 3, "%02x", val );
			buf += 2;
		}
		return StringAnsi(hashstr);
	}

	StringAnsi GetSHA256AsString( unsigned const char* bytes )
	{
		const Uint32 HASHSZ = 32;
		AnsiChar hashstr[HASHSZ*2+1];
		AnsiChar* buf = hashstr;
		unsigned const char* pch = bytes;
		if ( !*pch )
		{
			return StringAnsi::EMPTY;
		}

		for ( Uint32 i = 0; i < HASHSZ; ++i )
		{
			const Uint8 val = *pch++;
			Red::System::SNPrintF( buf, 3, "%02x", val );
			buf += 2;
		}

		return StringAnsi(hashstr);
	}

	Bool ValidateKosherScripts( const String& filePath )
	{
		String scriptsPath;
		GDepot->GetAbsolutePath(scriptsPath);
		scriptsPath += filePath;

		Ssha256 hash;
		if ( !GetScriptsSha256( scriptsPath, hash ) )
		{
			return false;
		}

		LOG_ENGINE(TXT("ValidateKosherScripts '%ls' SHA-256: %hs"), filePath.AsChar(), GetSHA256AsString(hash).AsChar());
		LOG_ENGINE(TXT("ValidateKosherScripts expected SHA-256: %hs"), GetSHA256AsString(SCRIPTS_HASH).AsChar());

#ifdef RED_PLATFORM_WINPC
		OutputDebugString( TXT("a") ); OutputDebugString( ANSI_TO_UNICODE(GetSHA256AsString(hash).AsChar()) ); OutputDebugString( TXT("\n") );
		OutputDebugString( TXT("e") ); OutputDebugString( ANSI_TO_UNICODE(GetSHA256AsString(SCRIPTS_HASH).AsChar()) ); OutputDebugString( TXT("\n") );
#endif

		// Easy way to bypass the kosher check, but let people still test their achievements... again so be it.
		const Uint8 ZERO_HASH[32] = {0};
		const Bool isZeroHash = Red::System::MemoryCompare( ZERO_HASH, SCRIPTS_HASH, sizeof(ZERO_HASH) ) == 0;
		if ( isZeroHash )
		{
			LOG_ENGINE(TXT("ValidateKosherScripts: expected hash is zero. Trivially validating as true."));
#ifdef RED_PLATFORM_WINPC
			OutputDebugString(TXT("z\n"));
#endif
			return true;
		}

		const Bool isMatch = Red::System::MemoryCompare( hash.m_value, SCRIPTS_HASH, sizeof(ZERO_HASH) ) == 0;
		LOG_ENGINE(TXT("ValidateKosherScripts isMatch: %d"), isMatch );
#ifdef RED_PLATFORM_WINPC
		OutputDebugString(TXT("m"));
		OutputDebugString( isMatch ? TXT("1") : TXT("0") );
		OutputDebugString(TXT("\n"));
#endif

		return isMatch;
	}
} // namespace ScriptCompilationHelpers
#endif // USE_SCRIPTS_HASH

#ifdef RED_MOD_SUPPORT
namespace Config
{
	// Maybe one day can change in the debug console
	TConfigVar< Bool > cvLogDebugScripts( "Scripts", "LogDebugScripts", true, eConsoleVarFlag_Developer );
	TConfigVar< String > cvDebugScriptsLogDir( "Scripts", "DebugScriptsLogDir", String::EMPTY, eConsoleVarFlag_Save );
	TConfigVar< Bool > cvDebugScriptsForceFlush( "Scripts", "DebugScriptsForceFlush", true, eConsoleVarFlag_Save );
}

namespace ModHelpers
{
	typedef void (*LOG_FUNC)(CName, const String&);
	extern LOG_FUNC LogFn;

	static FILE* s_logFile;
		
	Bool OpenLogFile( const String& logDir )
	{
		::fwide( stdout, 1 );
		::fwide( stderr, 1 );

		if ( logDir.EqualsNC(TXT("stdout")) )
		{
			s_logFile = stdout;
		}
		else if ( logDir.EqualsNC(TXT("stderr")) )
		{
			s_logFile = stderr;
		}
		else
		{
			String absoluteFilePath = logDir;
			absoluteFilePath.ReplaceAll(TXT("/"),TXT("\\"));
			if (!absoluteFilePath.EndsWith(TXT("\\")))
			{
				absoluteFilePath += TXT("\\");
			}
			absoluteFilePath += TXT("scriptslog.txt");
			s_logFile = ::_wfopen( absoluteFilePath.AsChar(), TXT("w") );
		}
	
		return s_logFile != nullptr;
	}

	void CloseLogFile()
	{
		if ( s_logFile && s_logFile != stdout && s_logFile != stderr )
			::fclose( s_logFile );
		s_logFile = nullptr;
	}

	static void LogToFile( CName channel, const String& text )
	{
		if ( s_logFile )
		{
			::fputws( TXT("["), s_logFile );
			::fputws( channel.AsString().AsChar(), s_logFile );
			::fputws( TXT("] "), s_logFile );
			::fputws( text.AsChar(), s_logFile );
			::fputws( TXT("\n"), s_logFile );
			if ( Config::cvDebugScriptsForceFlush.Get() )
				::fflush(s_logFile);
		}
	}

	void InitScriptLog()
	{
		// Yes, script logs *in* final
#ifndef RED_FINAL_BUILD
		return;
#endif
		if (!GScriptingSystem->IsFinalRelease())
		{
			ModHelpers::LogFn = &ModHelpers::LogToFile;
			const String& logDir = !Config::cvDebugScriptsLogDir.Get().Empty() ? Config::cvDebugScriptsLogDir.Get() : GFileManager->GetUserDirectory();
			OpenLogFile( logDir );
			LogFn( CNAME(Script), String(TXT("Debugging scripts for ") + String(APP_VERSION_NUMBER)) );
		}
	}
}
#endif // RED_MOD_SUPPORT

#ifdef VALIDATE_KOSHER_SCRIPTS
struct SScopedKosherVerififyAndExit
{
	SScopedKosherVerififyAndExit( Bool doVerifyAndExit, const Bool* pIsKosher )
		: m_doVerifyAndExit( doVerifyAndExit )
		, m_pIsKosher( pIsKosher )
	{
	}

	~SScopedKosherVerififyAndExit()
	{
		if ( m_doVerifyAndExit )
		{
			const Int32 EXIT_KOSHER = 2;
			const Int32 EXIT_NOT_KOSHER = 3;

			// Can't really rely on 0/1 in our engine... so use special return codes
			const Bool isKosher = m_pIsKosher ? *m_pIsKosher : false;
			const Int32 exitCode = isKosher ?  EXIT_KOSHER : EXIT_NOT_KOSHER;
			GEngine->RequestExit( exitCode );
		}
	}

	Bool		m_doVerifyAndExit;
	const Bool*	m_pIsKosher;
};
#endif

Bool CBaseEngine::InitializeScripts( Bool& outIsKosherScripts )
{
#ifdef VALIDATE_KOSHER_SCRIPTS
	outIsKosherScripts = false;
#else
	outIsKosherScripts = true;
#endif

	// Load scripts
	LOG_ENGINE( TXT( "Loading scripts" ) );
	GSplash->UpdateProgress( TXT( "Loading scripts..." ) );

#ifdef RED_NETWORK_ENABLED
	m_scriptHandler.Initialize();
#endif

#ifdef RED_MOD_SUPPORT
	ModHelpers::InitScriptLog();
#endif

	// config 
	const Bool dumpScripts = Red::System::StringSearch( SGetCommandLine(), TXT("-dumpscripts") ) != nullptr;
	const Bool forceCompilation = Red::System::StringSearch( SGetCommandLine(), TXT("-forcescriptcompilation") ) != nullptr;
	const Bool noScriptCRC = Red::System::StringSearch( SGetCommandLine(), TXT("-noscriptcrc") ) != nullptr; // use at your own risk. Mainly for players if too slow or whatever reason.
	Bool verifyScriptsAndExit = Red::System::StringSearch( SGetCommandLine(), TXT("-verifyscriptsandexit") ) != nullptr;

	Bool forceUncookedScripts = false;

	const Bool noCrcCheck = noScriptCRC || GIsCooker || GIsEditor; // reevaluate for cooker and editor. On SSD takes < 1 sec
	ScriptCompilationHelpers::ScriptCollector scriptCollector( noCrcCheck );

	Uint64 expectedCrc = 0;
	if ( GIsCooker || GContentManager->RequiresUncookedScripts() )
	{
		forceUncookedScripts = true;
	}

#ifdef VALIDATE_KOSHER_SCRIPTS
	SScopedKosherVerififyAndExit scopedVerifyAndExit( verifyScriptsAndExit, &outIsKosherScripts );
#endif

	// do not load existing scripts if we were asked to 
	if ( !forceCompilation && !dumpScripts )
	{
		Bool validateHeader = true;
		String filePath;
		CRTTISerializer loader;

		// Try cooked scripts first
		if( !forceUncookedScripts )
		{
			// This code path is going to execute even in development builds. It won't hurt unless somebody has the cooked.redscripts folder in his development r4data folder.
			// The intention behind this approach is to not introduce a zillion switches to the executable and still maintain a possibility to mix cooked and uncooked data.
#ifdef RED_FINAL_BUILD
			filePath = TXT("cookedfinal.redscripts");
#else
			filePath = TXT("cooked.redscripts");
#endif
			LOG_ENGINE( TXT("Attempting to load cooked scripts from '%ls'..."), filePath.AsChar() );

			const Uint64 noCheckCrc = ScriptCompilationHelpers::ScriptCollector::NO_CHECK_CRC;
			if ( loader.LoadScriptData( filePath, noCheckCrc, CRTTISerializer::eHeaderValidateNone ) )
			{
#ifdef VALIDATE_KOSHER_SCRIPTS
				outIsKosherScripts = ScriptCompilationHelpers::ValidateKosherScripts( filePath );
#endif

				LOG_ENGINE( TXT("Engine is using cooked precompiled scripts from '%ls'"), filePath.AsChar() );
				return !verifyScriptsAndExit;
			}
		}

		// Use a normal, development stage path
		{
			expectedCrc = scriptCollector.GetCrc();
			filePath = CRTTISerializer::GetPlatformFileName();
			LOG_ENGINE( TXT("Trying to load scripts from '%ls'..."), filePath.AsChar() );

			Uint32 headerValidationMask = CRTTISerializer::eHeaderValidateNone;
			if ( !GFileManager->IsReadOnly() )
			{
				headerValidationMask |= CRTTISerializer::eHeaderValidateAll;
			}
#ifndef RED_PLATFORM_CONSOLE
			else if ( GContentManager->RequiresUncookedScripts() )
			{
				// Force a recompile vs potential crash loading scripts. The cached scripts may be incompatible.
				headerValidationMask |= CRTTISerializer::eHeaderValidateAppVersion|CRTTISerializer::eHeaderValidateScriptVersion;
			}
#endif
			if ( loader.LoadScriptData( filePath, expectedCrc, headerValidationMask ) )
			{
				LOG_ENGINE( TXT("Engine is using precompiled scripts from '%ls'"), filePath.AsChar() );
				return true;
			}
		}
	}

	// allow some kind of feedback window in case the scripts fail to compile
	// NOTE: the feedback window is only allowed in interactive configurations, it's disabled in any kind of automatic process
	IScriptCompilationFeedback* feedback = nullptr;
	if ( !forceCompilation && !m_silentScripts && !dumpScripts )
	{
		// allow Editor to override the feedback window
		feedback = QueryScriptCompilationFeedback();
		if ( !feedback )
		{
			// use the default platform window
			feedback = &ScriptCompilationHelpers::ScriptPopupWindow::GetDefaultInstance();
		}
	}

#ifndef RED_PLATFORM_CONSOLE
	ScriptCompilationHelpers::ScopedSplashScreen splash;
#endif

	// compile 
	for (;;)
	{
		const TDynArray<String>& scriptFiles = scriptCollector.GetScriptFiles();
		expectedCrc = scriptCollector.GetCrc();

		const Bool fullContext = GContentManager->RequiresUncookedScripts();
		CScriptCompilationMessages errorCollector;
		if ( !GScriptingSystem->LoadScripts( &errorCollector, scriptFiles, fullContext ) )
		{
#ifdef RED_PLATFORM_CONSOLE
			ERR_CORE(TXT("]------------------------["));
			ERR_CORE(TXT("Scripts failed to compile!"));
			ERR_CORE(TXT("]------------------------["));
			for ( const auto& ctxt : errorCollector.m_errors )
			{
				const String errMsg = String::Printf(TXT("%ls:%d: %ls"), ctxt.file.AsChar(), ctxt.line, ctxt.text.AsChar());
				ERR_CORE(TXT("\t%ls"), errMsg.AsChar() );
			}
#endif

			if ( feedback )
			{
				switch( feedback->OnCompilationFailed( errorCollector ) )
				{
				case CSRV_Recompile:
					{
						// can refresh existing files and added/removed base files
						scriptCollector.Clear();
					}
					// loop again
					break;

				case CSRV_Skip:
					return true;

				case CSRV_Quit:
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			// compiled
			break;
		}
	}

#if !defined(RED_FINAL_BUILD)
	// Validate RTTI layout - just a sanity check
	if ( !SRTTI::GetInstance().ValidateLayout() )
	{
		ERR_ENGINE( TXT("RTTI layout corrupted!") );
		return false;
	}
#endif

	Bool useCustomScriptFileName = false;
	String scriptFileName;
	const Char* customScriptFileName = Red::System::StringSearch( SGetCommandLine(), TXT("-dumpscripts=") );
	if ( customScriptFileName )
	{
		const String cmdToTheEnd = customScriptFileName + 13; // skip the -dumpscripts=
		const String cmdToFirstSpace = cmdToTheEnd.StringBefore( TXT(" ") ); // in case we have more args
		scriptFileName = cmdToFirstSpace.Empty() ? cmdToTheEnd : cmdToFirstSpace;
		useCustomScriptFileName = true;
		LOG_ENGINE( TXT("Custom script file for dump: '%ls'"), scriptFileName.AsChar() );
	}

	if ( !useCustomScriptFileName )
	{
		scriptFileName = GFileManager->GetDataDirectory()  + CRTTISerializer::GetPlatformFileName();
	}

	CRTTISerializer saver;
	saver.SaveScriptData( scriptFileName, expectedCrc );

	return !dumpScripts;
}

Bool CBaseEngine::InitializeTaskManager()
{
	GSplash->UpdateProgress( TXT( "Initializing Task Manager..." ) );

	{
#if defined( RED_PLATFORM_ORBIS )
		// On the PS4 the O/S interrupts cores starting at zero. So start at the highest on the first core. TBD: give render thread core 3 instead?
		const Red::Threads::TAffinityMask mainThreadAffinityMask = 1 << 3;
#elif defined( RED_PLATFORM_DURANGO )
		const Red::Threads::TAffinityMask mainThreadAffinityMask = 1 << 0;
#else
		const Red::Threads::TAffinityMask mainThreadAffinityMask = 0;
#endif

		Red::Threads::InitializeFramework( mainThreadAffinityMask );
	}

	extern CTaskManager* SCreateTaskManager();
	GTaskManager = SCreateTaskManager();
	if ( !GTaskManager )
	{
		ERR_ENGINE( TXT("Unable to create task manager") );
		return false;
	}

	Uint32 numTaskThreads = 0;
	Red::Threads::TAffinityMask* affinityMasks = nullptr;

#if defined( RED_PLATFORM_ORBIS )
	// On the PS4 the O/S interrupts cores starting at zero. Main on core 3 and renderer on core 2.
	// TBD: or use more, but so far: { main, render, 2x task} , { 2x io/misc, 2x used by OS }
	numTaskThreads = 4;
	Red::Threads::TAffinityMask platformAffinityMasks[] = { 1 << 0 | 1 << 1,
															1 << 0 | 1 << 1,
															1 << 4 | 1 << 5,
															1 << 4 | 1 << 5 };
	affinityMasks = platformAffinityMasks;
#elif defined( RED_PLATFORM_DURANGO )
	// On Xbox we will use 4 work threads with affinities set to run on any of the cores
	// The O/S is good at scheduling the core effectively
	numTaskThreads = 4;
	Red::Threads::TAffinityMask platformAffinityMasks[] = { 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5,
															1 << 2 | 1 << 3 | 1 << 4 | 1 << 5,
															1 << 2 | 1 << 3 | 1 << 4 | 1 << 5,
															1 << 2 | 1 << 3 | 1 << 4 | 1 << 5 };
	affinityMasks = platformAffinityMasks;
#else
	numTaskThreads = Min<Uint32>(Max< Uint32 >( 1, Red::System::CpuId::GetInstance().GetNumberOfLogicalCores() - 4 ), 4) ; // Renderer, Game, AsyncLoader, FMod (or Wwise)
	affinityMasks = nullptr;
#endif

	STaskGroupInitParams taskGroupInitParams;
	auto& tgNormalParam = taskGroupInitParams.m_threadParams[ TSG_Normal ];
	tgNormalParam.m_affinityMasks = affinityMasks;
	tgNormalParam.m_numTaskThreads = numTaskThreads;

	tgNormalParam.m_priority = Red::Threads::TP_Highest;

#ifdef RED_PLATFORM_ORBIS
	// Xbox has ConCRT. PS4 has this... TBD: grow/shrink the pool?
	Uint32 numServiceThreads = 4;
	Red::Threads::TAffinityMask serviceAffinityMasks[] = {	1 << 0 | 1 << 1 | 1 << 4 | 1 << 5,
															1 << 0 | 1 << 1 | 1 << 4 | 1 << 5,
															1 << 0 | 1 << 1 | 1 << 4 | 1 << 5,
															1 << 0 | 1 << 1 | 1 << 4 | 1 << 5 };

	auto& tgServiceParam = taskGroupInitParams.m_threadParams[ TSG_Service ];
	tgServiceParam.m_affinityMasks = serviceAffinityMasks;
	tgServiceParam.m_numTaskThreads = numServiceThreads;
	tgServiceParam.m_priority = Red::Threads::TP_Highest;
# ifdef RED_LOGGING_ENABLED
	// Give at least 1 MB if you want to see logs... not enabling this by default, sorry.
	tgServiceParam.m_memParams = Red::Threads::SThreadMemParams( 2*PTHREAD_STACK_MIN );
# else
	tgServiceParam.m_memParams = Red::Threads::SThreadMemParams( 2*PTHREAD_STACK_MIN );
# endif
#endif // RED_PLATFORM_ORBIS

	if ( !GTaskManager->Init( taskGroupInitParams ) )
	{
		ERR_ENGINE( TXT("Unable to initialize task manager") );
		return false;
	}

	return true;
}



Bool CBaseEngine::InitializeNetwork()
{
	// Initialise Network
	GSplash->UpdateProgress( TXT( "Initializing Network..." ) );

#ifdef RED_NETWORK_ENABLED

#if defined(RED_FINAL_BUILD) || defined(RED_PROFILE_BUILD)
	// If networking is enabled in final, make sure it's only activated by a command line switch
	Bool enabled = Red::System::StringSearch( SGetCommandLine(), TXT( "-net" ) ) != nullptr;
	if( !enabled )
		return true;
#endif

	m_network.Initialize( &NetworkHelper::NetworkRealloc, &NetworkHelper::NetworkFree );

#ifdef RED_LOG_VIA_NETWORK
	m_networkedLogger.Initialise( &m_network );
#endif

	// Initialize remote config
	Config::CRemoteConfig::Initialize();

	GScriptingSystem->RegisterNetworkChannelListener();
#endif

	return true;
}

Bool CBaseEngine::CreateRenderer()
{
	// Create renderer
	GSplash->UpdateProgress( TXT( "Initializing Renderer..." ) );
	extern IRender* SCreateRender( IPlatformViewport* );
	if (!GRender)
	{
		GRender = SCreateRender( GetPlatformViewport() );
		if ( !GRender )
		{
			ERR_ENGINE( TXT("Unable to create renderer") );
			return false;
		}
	}
	// Create renderer memory
	if (!GRenderMemory)
	{
		GRenderMemory = new CRenderMemoryPages( 1 << 16, 5 );
		if ( !GRenderMemory )
		{
			ERR_ENGINE( TXT("Unable to create renderer memory buffer") );
			return false;
		}
	}
	return true;
}

Bool CBaseEngine::InitializeRenderer()
{
	if (QuickBoot::g_quickInitViewport == nullptr)
	{
		// Initialize renderer. Needs to be after RTTI is up, so resource loading works proper (e.g. CGameplayEffects needs to load some CSV files)
		GSplash->UpdateProgress( TXT("Initializing Renderer...") );
		if ( !GRender->Init() )
		{
			ERR_ENGINE( TXT("Unable to initialize renderer") );
			return false;
		}
	}

	// Create particle memory pool
	GParticlePool = new CParticlePool();
	return true;
}

Bool CBaseEngine::InitializeSoundSystem()
{
	// Initialize sound system
	if( GDepot->IsUsingBundles() )
	{
		CSoundSystem::Init( GFileManager->GetRootDirectory() + TXT("bin\\initialdata\\sound\\") );
	}
	else
	{
		CSoundSystem::Init( GFileManager->GetRootDirectory() + TXT("r4data\\soundbanks\\pc\\") );		// for editor and wcc, game sound initialization is done in platform code
	}
	m_hasSoundInitialized = true;

	return true;
}

Bool CBaseEngine::InitializeSoundSystemObject()
{
	RED_ASSERT( GSoundSystem == nullptr, TXT("Sound system already initialized") );
	GSoundSystem = new CSoundSystem();

	return true;
}

Bool CBaseEngine::InitializeGUI()
{
#ifdef USE_SCALEFORM
	// Create GUI
	// Need to be before GRender initialization, because we overwrite Scaleform memory allocator and it needs to be done before first Scaleform allocation
	GSplash->UpdateProgress( TXT("Initializing GUI...") );
	RED_VERIFY( CScaleformSystem::StaticInstance() );
#endif

	return true;
}

Bool CBaseEngine::InitializeDebugNetwork()
{
#ifdef RED_NETWORK_ENABLED

#if defined(RED_FINAL_BUILD) || defined(RED_PROFILE_BUILD)
	// If networking is enabled in final, make sure it's only activated by a command line switch
	Bool enabled = Red::System::StringSearch( SGetCommandLine(), TXT( "-net" ) ) != nullptr;
	if( !enabled )
		return true;
#endif

	// Enable listening to incoming connections
	GSplash->UpdateProgress( TXT("Listen for incoming connections...") );
	while( !m_network.IsInitialized() )
	{
		Red::Threads::SleepOnCurrentThread( 10 );
	}

	if ( GIsGame )
		m_network.ListenForIncomingConnections( GAME_PORT );
	else
		m_network.ListenForIncomingConnections( EDITOR_PORT );

	m_pingUtil.Initialize();
	m_hasDebugNetworkInitialized = true;

	GRemoteConnection = new CRemoteConnection();

	CDebugPageServer::GetInstance().Initialize();
#endif

	return true;
}

Bool CBaseEngine::InitializePhysicsDebugger()
{
#ifndef PHYSICS_RELEASE
	GSplash->UpdateProgress( TXT("Initializing Havok Debugger...") );
	GPhysicsDebugger = new CPhysicsDebugger;
	if ( !GPhysicsDebugger )
	{
		ERR_ENGINE( TXT("Unable to create Havok Debugger") );
		return false;
	}
	else if ( !GPhysicsDebugger->Init() )
	{
		ERR_ENGINE( TXT("Unable to initialize Havok Debugger") );
		return false;
	}
#endif

	return true;
}

Bool CBaseEngine::InitializeStartupPackage()
{
	// no bundles
	if ( !GFileManager->IsReadOnly() )
	{
		WARN_ENGINE( TXT("Engine not run in read-only mode, startup package will not be loaded") );
		return true;
	}

	// find startup package
	CDiskBundle* startupBundle = GDepot->GetBundles()->GetStartupBundle( StringAnsi::EMPTY );
	if ( !startupBundle )
	{
#ifdef RED_PLATFORM_WINPC
		WARN_ENGINE( TXT("No engine startup bundle found - no will be loaded. Expect reduced loading times.") );
		return true;
#else
		ERR_ENGINE( TXT("No engine startup bundle found - cook is incomplete") );
		return false;
#endif
	}

	// preload the startup content
	m_startupContent = startupBundle->Preload();
	if ( m_startupContent )
	{
		m_startupContent->AddToRootSet();
	}
	return true;
}

Bool CBaseEngine::InitializeMarkerSystem()
{
#ifndef NO_MARKER_SYSTEMS
	GSplash->UpdateProgress( TXT("Creating markers system") );
	m_markerSystemsManager = new CMarkersSystem();
	m_markerSystemsManager->Initialize();
#endif	// NO_MARKER_SYSTEMS

	return true;
}

Bool CBaseEngine::InitializeDebugWindows()
{
#ifndef NO_DEBUG_PAGES
	GSplash->UpdateProgress( TXT("Creating debug pages...") );

	if( !IDebugPageManagerBase::GetInstance() )
	{
		new CDebugPageManagerTabbed( resEngineFontResource );
	}

	extern void CreateDebugPageCommunityLog();
	CreateDebugPageCommunityLog();
	extern void CreateDebugPageLanguagePacks();
	CreateDebugPageLanguagePacks();
#endif

#ifndef NO_RED_GUI
	GSplash->UpdateProgress( TXT("Creating red gui manager") );
	GRedGui::GetInstance().Init();
#ifndef NO_DEBUG_WINDOWS
	GSplash->UpdateProgress( TXT("Creating debug windows") );
	::InitializeAndRegisterDebugWindows();

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

#ifndef NO_DEBUG_PAGES
	extern void InitDepotDebugPages();
	InitDepotDebugPages();
	extern void InitObjectDebugPages();
	InitObjectDebugPages();
	extern void InitResourceDebugPages();
	InitResourceDebugPages();
	extern void InitBundlesDebugPages();
	InitBundlesDebugPages();
	extern void InitMemoryDebugPages();
	InitMemoryDebugPages();
	extern void InitFileHandleCacheDebugPages();
	InitFileHandleCacheDebugPages();
	extern void InitFios2DebugPages();
	InitFios2DebugPages();
	extern void InitLayerDebugPages();
	InitLayerDebugPages();
	extern void InitVideoDebugPages();
	InitVideoDebugPages();
#endif

	return true;
}

Bool CBaseEngine::InitializeUserProfileManager()
{
#ifdef RED_PLATFORM_ORBIS
	// PC and Durango are initialised in platform init
	GUserProfileManager->Initialize();

	String path = GFileManager->GetRootDirectory() + TXT( "bin/orbis_content_restriction.xml" );
	IFile* contentRestrictionFile = GFileManager->CreateFileReader( path, FOF_Buffered | FOF_AbsolutePath );

	if( contentRestrictionFile )
	{
		CXMLReader* ageContentRestrictionXML = new CXMLFileReader( *contentRestrictionFile );
		GUserProfileManager->LoadContentRestrictionXML( ageContentRestrictionXML );

		delete ageContentRestrictionXML;
		delete contentRestrictionFile;
	}
#endif

	GUserProfileManager->RegisterListener( &CBaseEngine::OnUserEvent, this, Events::P_High );

	String achievementMapFilepath = TXT( "game/achievements.xml" );
	CXMLReader* achievementMap = GDepot->LoadXML(achievementMapFilepath);

	if( achievementMap )
	{
		GUserProfileManager->LoadMap( *achievementMap, achievementMapFilepath );

		delete achievementMap;
	}

	String presenceMapFilepath = TXT( "game/presence.xml" );
	CXMLReader* presenceMap = GDepot->LoadXML(presenceMapFilepath);

	if( presenceMap )
	{
		GUserProfileManager->LoadMap( *presenceMap, presenceMapFilepath );

		delete presenceMap;
	}

	return true;
}

Bool CBaseEngine::ShutdownUserProfileManager()
{
	return GUserProfileManager->Shutdown();
}

Bool CBaseEngine::InitializeInputDeviceManager()
{
	// Initialize the input system
	extern IInputDeviceManager* GInputDeviceManager;

	if( !GInputDeviceManager )
	{
		return false;
	}

	m_inputDeviceManager = GInputDeviceManager;
	return m_inputDeviceManager->Init();
}

Bool CBaseEngine::ShutdownInputDeviceManager()
{
	extern IInputDeviceManager* GInputDeviceManager;

	if( !GInputDeviceManager )
	{
		return false;
	}

	m_inputDeviceManager = nullptr;
	GInputDeviceManager->Shutdown();

	return true;
}

Bool CBaseEngine::CreateGame()
{
	GSplash->UpdateProgress( TXT("Creating Game...") );

	// Create game instance
	extern CGame* CreateGame();
	GGame = CreateGame();
	if ( !GGame )
	{
		ERR_ENGINE( TXT("Unable to create game") );
		return false;
	}

	// Make sure game stays alive
	GGame->AddToRootSet();
	return true;
}

Bool CBaseEngine::InitializeGame()
{
	GSplash->UpdateProgress( TXT("Initializing game systems...") );
	GGame->Init();
	return true;
}

Bool CBaseEngine::InitializePhysics()
{
#ifdef USE_PHYSX
	GSplash->UpdateProgress( TXT("Initializing PhysX...") );
	GPhysicEngine = GPhysXEngine = new CPhysXEngine;
	if ( !GPhysXEngine )
	{
		ERR_ENGINE( TXT("Unable to create PhysX") );
		return false;
	}
#else
	GPhysicEngine = new CPhysicsEngine;
#endif

	if ( !GPhysicEngine->Init() )
	{
		ERR_ENGINE( TXT("Unable to initialize physics") );	
		return false;
	}

	return true;
}

Bool CBaseEngine::InitializeDebugServer()
{
#ifndef NO_DEBUG_SERVER
	// debug server
	GSplash->UpdateProgress( TXT("Initializing Debugger Server...") );
	DBGSRV_CALL( Init() );
#endif

	return true;
}

Bool CBaseEngine::InitializeAnimationSystem()
{
	// Create animation manager
	GSplash->UpdateProgress( TXT("Creating animation manager...") );
	GAnimationManager = new AnimationManager();
	return true;
}

#if !defined( RED_FINAL_BUILD ) && defined( RED_PLATFORM_ORBIS )
static Uint8 GBlinkenLichten;
#define DEBUG_BLINK() do { ::sceKernelSetGPO( ++GBlinkenLichten ); } while( false )
#define DEBUG_BLINKOFF() do { ::sceKernelSetGPO( 0 ); } while( false )
#define DEBUG_BLINKALL() do { ::sceKernelSetGPO( 0xFF ); } while( false )
#else
#define DEBUG_BLINK()
#define DEBUG_BLINKOFF()
#define DEBUG_BLINKALL()
#endif

#define SAFE_INIT( func, txt ) 	if ( !(func) )	{ return GracefulExit( TXT(txt) ); } else { DEBUG_BLINK(); GLoadingProfiler.FinishStage( TXT(txt) ); }

namespace OutOfMemoryCallbacks
{
	Bool OnCoreOutOfMemory( Red::MemoryFramework::PoolLabel poolId )
	{
		// Since small-block allocator may be using additional memory in the default pool, we kick it on OOM
		// to release any unused memory. 
		if( poolId == Memory::GetPoolLabel< MemoryPool_Default >() )
		{
			Red::System::MemSize memoryFreed = Memory::ReleaseFreeMemoryToSystem();
			LOG_ENGINE( TXT( "OnCoreOutOfMemory Released %d bytes from small object allocator" ), memoryFreed );
			return memoryFreed > 0;
		}
		else
		{
			return false;
		}

	}
}

Bool CBaseEngine::Initialize()
{
	DEBUG_BLINKOFF();

	RED_MEMORY_REGISTER_OOM_CALLBACK( OutOfMemoryCallbacks::OnCoreOutOfMemory );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Create here, since apparently the only thing currently saving it from a race to be created on another thread
	// is having asserts defined. Should really even be created earlier, but here you don't have to worry about doing it per platform.
	(void)Red::System::Error::Handler::GetInstance();

	// Report command line to the log
	if ( SGetCommandLine() )
	{
		LOG_ENGINE( TXT( "Engine running with command line parameters '%ls'" ), SGetCommandLine() );
	}

#ifndef RED_FINAL_BUILD
	// Make sure memory is ok
	SAFE_INIT( CheckMemoryUsage(), "CheckMemoryUsage" );
	GLoadingProfiler.FinishStage( TXT("CheckMemoryUsage") );
#endif

	//////////////////////////////////////////////////////////////////////////
	// Do not move anything requiring attached content above this
	//////////////////////////////////////////////////////////////////////////
	SAFE_INIT( InitializeContentManager(), "ContentManager" );
	
	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// W3 Hack: have only one buffer that is global
	// Allocating it early to try to pack memory as much as possible before loading first world.
	// Use only by sectorDataMerged. DO NOT USE OR MODIFY
	extern TDynArray< Uint8, MC_SectorData > GSectorDataPreallocatedBuffer;
	Uint32 sectorDataPreallocatedBuffer = Config::cvSectorDataPreallocatedBufferSizeInMB.Get()*1024*1024;
	GSectorDataPreallocatedBuffer.Reserve( sectorDataPreallocatedBuffer );
	
	extern TDynArray< SectorData::PackedObject, MC_SectorData > GSectorDataPreallocatedObjectsBuffer;
	GSectorDataPreallocatedObjectsBuffer.Reserve( SectorData::MAX_OBJECTS  );
	
	CClipMapCookedData::Hack_InitializeStorage();

	// Task manager
	SAFE_INIT( InitializeTaskManager(), "TaskManager" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Network
	SAFE_INIT( InitializeNetwork(), "Network" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Rendering - create
	SAFE_INIT( CreateRenderer(), "Renderer" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Sound system
	SAFE_INIT( InitializeSoundSystem(), "SoundSystem" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// GUI
	SAFE_INIT( InitializeGUI(), "GUI" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Magic global options, TODO: remove both
#ifndef FINAL
	GDebugSaves = ( nullptr != Red::System::StringSearch( SGetCommandLine(), TXT("-debugsaves") ) );
	GUseSaveHack = ( nullptr != Red::System::StringSearch( SGetCommandLine(), TXT("-savehack") ) );
#endif

	// Initialize RTTI
	SAFE_INIT( InitializeRTTI(), "RTTI" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Create sound system
	SAFE_INIT( InitializeSoundSystemObject(), "SoundSystem" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Sound system requires a kick at this point
	GSoundSystem->PostInit();
	GLoadingProfiler.FinishStage( TXT("SoundPostInit") );
	
	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Initialize rendering
	SAFE_INIT( InitializeRenderer(), "RendererInit" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Debug network
	SAFE_INIT( InitializeDebugNetwork(), "DebugNetwork" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	Bool isKosherScripts = false;
	// Compile or load already compiled scripts
	SAFE_INIT( InitializeScripts( isKosherScripts ), "Scripts" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Physics visualizer
	SAFE_INIT( InitializePhysicsDebugger(), "PhysicsDebugger" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Initialize animation system
	SAFE_INIT( InitializeAnimationSystem(), "AnimationSystem" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Initialize game object
	SAFE_INIT( CreateGame(), "Game" );

#ifdef RED_PLATFORM_WINPC
	RED_FATAL_ASSERT( GGame, "Game not initialized!" );
	if ( !isKosherScripts )
	{
		WARN_ENGINE(TXT("Scripts not kosher"));
		GGame->SetKosherTaintFlags( CGame::eKosherTaintFlag_Scripts );
	}
#endif

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Initialize physics
	SAFE_INIT( InitializePhysics(), "Physics" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Initialize old job manager
	SJobManager::GetInstance().InitializeThreads();
	GLoadingProfiler.FinishStage( TXT("JobManager") );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Load resource definitions
	GSplash->UpdateProgress( TXT("Loading resource definitions...") );
	SResourceDefManager::GetInstance().LoadAllDefinitions();
	GLoadingProfiler.FinishStage( TXT("LoadResourceDefs") );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// User profile manager
	SAFE_INIT( InitializeUserProfileManager(), "GUserProfileManager" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Input device manager (must come after the user profile manager and before the game)
	SAFE_INIT( InitializeInputDeviceManager(), "InputDeviceManager" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Startup package
	SAFE_INIT( InitializeStartupPackage(), "StartupPackage" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Initialize the Game
	SAFE_INIT( InitializeGame(), "GameInit" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Initialize debug server
	SAFE_INIT( InitializeDebugServer(), "DebugServer" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Load rendering settings
	GSplash->UpdateProgress( TXT("Loading render settings...") );
	
	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	SRenderSettingsManager::GetInstance().LoadTextureGroups();

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	GLoadingProfiler.FinishStage( TXT("LoadRenderSettings") );

	// Create marker systems manager and all subordinate marker systems
	SAFE_INIT( InitializeMarkerSystem(), "MarkerSystem" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Create the debug window/debug pages
	SAFE_INIT( InitializeDebugWindows(), "DebugWindows" );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	// Load the fallback material immediately. Done here so we don't randomly load + create the objects
	// during the game (it can play havok with the garbage collector!)
	GRender->GetFallbackShader();
	GLoadingProfiler.FinishStage( TXT("FallbackShader") );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

#ifdef USE_UMBRA
	// Initialize occlusion system
	SOcclusionSystem::GetInstance().Init();
	GLoadingProfiler.FinishStage( TXT("OcclusionInit") );
#endif // USE_UMBRA

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	InitializeProfilers();
	GLoadingProfiler.FinishStage( TXT("ProfilersInit") );

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}

	DEBUG_BLINKOFF();

	// Update once now because of stuff that needs to be processed before the next tick
	GContentManager->Update();

	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}
	
	if (GDeferredInit) 
	{
		GDeferredInit->OnBaseEngineInit();
		delete GDeferredInit;
		GDeferredInit = nullptr;
	}

	// Let our swords bumper finish before the renderThread gets
	// attacked by everything waiting for it and causes a stutter

#ifdef RED_PLATFORM_CONSOLE
	CTimeCounter readyTimer;
	CTimeCounter safetyTimer;
#endif

	while ( QuickBoot::g_lastBumperVideo )
	{
		if ( !QuickBoot::g_lastBumperVideo->IsValid() )
		{
			QuickBoot::g_lastBumperVideo->Release();
			QuickBoot::g_lastBumperVideo = nullptr;
		}
		else
		{
			if ( GMessagePump != nullptr )
			{
				GMessagePump->PumpMessages();

#ifdef RED_PLATFORM_CONSOLE
				GContentManager->Update();
				//Try to get DLC loading ASAP during the loading.usm, so we check if the last bumper is actually playing yet
				// Avoid during the swords since that can cause a lot of stutter. TBD if need to try earlier anyway.
				if ( QuickBoot::g_lastBumperVideo->GetRenderVideoState() == eRenderVideoState_Playing )
				{			
					// Check for pending DLC, assuming any user is logged in even
					const Float KLUDGE_TIMEOUT_SEC = 1.f;
					const Float SAFETY_TIMEOUT_SEC = 120.f;
					const Bool isReady = GContentManager->IsReady( eContentMask_Default ) || safetyTimer.GetTimePeriod() > SAFETY_TIMEOUT_SEC;
					if ( isReady )
					{
						// Make sure it's been "ready" for a second, just in case since the message pump may not even have pumped due to its own internal
						// timer and then we miss out on starting a DLC update...
						// #tbd: not fighting against the streaming installer resetting this for attaching content
						if ( readyTimer.GetTimePeriod() > KLUDGE_TIMEOUT_SEC )
						{
							// Once ready, loop another tick to try and make sure content processed
							QuickBoot::g_lastBumperVideo->Cancel();
							QuickBoot::g_lastBumperVideo->Release();
							QuickBoot::g_lastBumperVideo = nullptr;
						}
					}
					else
					{
						readyTimer.ResetTimer();
					}
				}
				else
				{
#ifdef RED_PLATFORM_CONSOLE
					readyTimer.ResetTimer();
					safetyTimer.ResetTimer();
#endif
				}
#endif // RED_PLATFORM_CONSOLE
			}
		}
	}
	GLoadingProfiler.FinishStage( TXT("WaitForBumpers") );

	// Base engine initialized
	return true;
}

#ifndef RED_FINAL_BUILD
Bool CBaseEngine::CheckMemoryUsage()
{
#if defined( RED_PLATFORM_ORBIS ) && !defined( RED_USE_NEW_MEMORY_SYSTEM )
	// On PS4, we want to ensure we are using all the memory we can. As a result, this is called once the GPU has
	// set up its memory, so we can test the direct memory available
	Red::MemoryFramework::SystemMemoryInfo systemMemory = Red::MemoryFramework::SystemMemoryStats::RequestStatistics();
	auto memoryFreeBytes = systemMemory.m_platformStats.m_directMemoryFree;
	if( !Red::MemoryFramework::PageAllocator::GetInstance().IsExtraDebugMemoryAvailable() && memoryFreeBytes > PS4_MAX_DIRECT_MEMORY_FREE )
	{
		LOG_ENGINE( TXT( "There is %dmb free direct memory. This should be given to pools! Check the pools." ), memoryFreeBytes / ( 1024 * 1024 ) );
		return false;
	}
#endif
	return true;
}
#endif

Bool CBaseEngine::InitializeProfilers()
{
#ifdef USE_PROFILER
	CCommandLineParser commandLineParser( SGetCommandLine() );

	if ( GIsCooker || commandLineParser.HasOption( TXT("noprofilers") ) )
	{
		return true;
	}

	String activeProfilers = Config::cvActiveProfilers.Get();

	// Profiler overrides
	if( commandLineParser.GetFirstParam( TXT("profiler"), activeProfilers ) == true )
	{
		LOG_ENGINE( TXT("Custom profiler override: '%ls'"), activeProfilers.AsChar() );
	}

	String profilerServer;
	if( commandLineParser.GetFirstParam( TXT("profilerServer"), profilerServer ) == true )
	{
		LOG_ENGINE( TXT("Custom profiler server override: '%ls'"), profilerServer.AsChar() );
		Config::cvProfilerServerName.Set( profilerServer );
	}

	Int32 profilerLevel;
	if( commandLineParser.GetFirstParam( TXT("profilerLevel"), profilerLevel ) == true )
	{
		LOG_ENGINE( TXT("Custom profiler level override: '%i'"), profilerLevel );
		Config::cvProfilingLevel.Set( profilerLevel );
	}

	String profilerChannels;
	if( commandLineParser.GetFirstParam( TXT("profilerChannels"), profilerChannels ) == true )
	{
		LOG_ENGINE( TXT("Custom profiler channel override: '%ls'"), profilerChannels.AsChar() );
		Int32 channels = ProfilerChannelsHelper::ConvertChannelsToInt( profilerChannels );
		Config::cvProfilingChannels.Set( channels );
	}

	CProfilerToolPool& toolFactory = UnifiedProfilerManager::GetInstance().GetToolPool();
	if( activeProfilers != TXT("none") )
	{
		CProfilerToolPool::ProfilerToolArray newTools;
		CTokenizer tokenizer( activeProfilers, TXT("|") );
		Uint32 numTokens = tokenizer.GetNumTokens();

		for( Uint32 i=0; i<numTokens; ++i )
		{
			IProfilerTool* toolInst = toolFactory.GetTool( tokenizer.GetToken( i ) );
			if( toolInst != nullptr )
			{
				newTools.PushBack( toolInst );
			}
		}

		UnifiedProfilerManager::GetInstance().RegisterTool( newTools );
	}
	else
	{
		UnifiedProfilerManager::GetInstance().RegisterTool( toolFactory.GetDefaultTools() );
	}
#endif

	return true;
}

//------

void CBaseEngine::Shutdown()
{
	// Set the closing flag - this is a HACK to prevent some crap from happening during unload
	GIsClosing = true;

	LOG_ENGINE( TXT("Shutting down engine") );

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
	LOG_ENGINE( TXT("Destroying debug windows") );
	GDebugWin::GetInstance().DestroyDebugWindows();
#endif	// NO_DEBUG_WINDOWS
	LOG_ENGINE( TXT("Shutting down the red gui manager") );
	GRedGui::GetInstance().Shutdown();
#endif	// NO_RED_GUI

#ifndef NO_DEBUG_SERVER
	// debug server
	LOG_ENGINE( TXT("Shutting down debug sever..") );
	DBGSRV_CALL( ShutDown() );
#endif

#ifdef RED_NETWORK_ENABLED
#if defined(RED_FINAL_BUILD) || defined(RED_PROFILE_BUILD)
	// If networking is enabled in final, make sure it's only activated by a command line switch
	Bool enabled = Red::System::StringSearch( SGetCommandLine(), TXT( "-net" ) ) != nullptr;
	if( !enabled )
#endif
	// Stop Network communications
	m_network.QueueShutdown();
#endif // RED_NETWORK_ENABLED

	CClipMapCookedData::Hack_DestroyStorage();

	// Before destroying texture cache, need to be sure no further texture streaming will happen.
	// We have to do it before viewport is destroyed
	if ( GRender != nullptr )
	{
		GRender->ShutdownTextureStreaming();
	}

	if ( QuickBoot::g_quickInitViewport != nullptr )
	{
		QuickBoot::g_quickInitViewport.Reset();
	}

	if ( GGame != nullptr )
	{
		// Shut down game
		//GGame->UnloadWorld();					// Unload worldwojci
		GGame->ShutDown();						// Shutdown all the systems
		GGame->SetViewport( ViewportHandle() );
		GGame->RemoveFromRootSet();
		GGame = nullptr;
	}

	// release the startup content
	if ( m_startupContent )
	{
		m_startupContent->RemoveFromRootSet();
		m_startupContent->Discard();
		m_startupContent = nullptr;
	}

	ShutdownInputDeviceManager();
	ShutdownUserProfileManager();

	// Cleanup texture cache
	if ( GTextureCache )
	{
		GContentManager->UnregisterContentListener( GTextureCache );
		GTextureCache->Shutdown();
	}

	// Cleanup collision cache
	if ( GCollisionCache )
	{
		GContentManager->UnregisterContentListener( GCollisionCache );
		GCollisionCache->Flush();
		GCollisionCache->Shutdown();
	}

	// Cleanup stringsDB
	if ( GCookedStringsDB )
	{
		GCookedStringsDB->Shutdown();
		GContentManager->UnregisterContentListener( GCookedStringsDB );
	}

#ifdef USE_UMBRA
	// Cleanup occlusion system
	SOcclusionSystem::GetInstance().Deinit();
#endif // USE_UMBRA

	// Process discarded list
	GObjectsDiscardList->ProcessList( true );

	// Release references to gathered resources
	CGatheredResource::ReleaseAllGatheredResources();

	// GC added before RTTI deinit, because some classes destruction is RTTI dependant.
	// There should be also another GC after RTTI deinit to clean stuff that was part of RTTI skeleton.
	GObjectGC->CollectNow();

	// Kill the job manager
	SJobManager::GetInstance().ShutDownThreads();

	// Destroy animation cache
	delete GAnimationManager;
	GAnimationManager = NULL;

#ifndef NO_MARKER_SYSTEMS
	if (m_markerSystemsManager != nullptr)
	{
		m_markerSystemsManager->Shutdown();
		delete m_markerSystemsManager;
		m_markerSystemsManager = nullptr;
	}
#endif // NO_MARKER_SYSTEMS

#ifdef USE_SCALEFORM
	// Shutdown above deleting GRender since need to flush some cleanup commands
	if ( CScaleformSystem::StaticInstance() )
	{
		CScaleformSystem::StaticInstance()->Shutdown();
	}
#endif

#ifdef USE_APEX
	//Unregister materials before we delete GRender, as some materials' destructors may use it
	UnregisterApexMaterials();
#endif

	// Since there may be tasks still queued / running that require the renderer to be present (compiling materials, etc)
	// we should flush the task manager at this point
	if( GTaskManager )
	{
		GTaskManager->Flush();
	}

#ifndef NO_ASYNCHRONOUS_MATERIALS
	if ( GRender )
	{
		GRender->Flush();
		GRender->FlushRecompilingMaterials();
	}
#endif // NO_ASYNCHRONOUS_MATERIALS

	// Shut down rendering
	if ( GRender )
	{
		delete GRender;
		GRender = NULL;
	}
	
#ifdef USE_SCALEFORM
	// Now destroy scaleform system. At this point we don't have any scaleform subsystems.
	if ( CScaleformSystem::StaticInstance() )
	{
		CScaleformSystem::StaticInstance()->Destroy();
	}
#endif

	if ( GSoundSystem )
	{
		GSoundSystem->Shutdown();
		delete GSoundSystem;
		GSoundSystem = NULL;
	}


	// Release render memory
	if ( GRenderMemory )
	{
		delete GRenderMemory;
		GRenderMemory = NULL;
	}

	// Destroy particle memory pool
	if ( GParticlePool )
	{
		delete GParticlePool;
		GParticlePool = NULL;
	}

#ifndef PHYSICS_RELEASE
	// Shut down havok debugger
	if ( GPhysicsDebugger )
	{
		GPhysicsDebugger->ShutDown();
		delete GPhysicsDebugger;
		GPhysicsDebugger = NULL;
	}
#endif

#if defined(USE_HAVOK_ANIMATION) || defined(USE_HAVOK_DATA_IMPORT)
	// Shut down main Havok engine
	GHavokEngine->ShutDown();
	delete GHavokEngine;
	GHavokEngine = NULL;
#endif

	if (GPhysicEngine != nullptr)
	{
		GPhysicEngine->ShutDown();
		delete GPhysicEngine;
		GPhysicEngine = NULL;
	}

	if ( GShaderCache )
	{
		GContentManager->UnregisterContentListener( GShaderCache );
		GShaderCache->Shutdown();
	}

	if ( GDepot->IsUsingBundles() )
	{
		GContentManager->UnregisterContentListener( GDepot );
	}

	// Destroy default objects
	SRTTI::GetInstance().DestroyDefaultObjects();

	// Final GC
	if (GObjectGC != nullptr)
	{
		GObjectGC->CollectNow();
	}

	// Cleanup final GPU resources
	GpuApi::GpuApiMemory::DestroyInstance();

	// Deinitialize RTTI
	SRTTI::GetInstance().Deinit();

#ifdef RED_NETWORK_ENABLED
	// Close debug page server
	CDebugPageServer::GetInstance().Shutdown();

	// Close remote config
	Config::CRemoteConfig::Shutdown();

	// Close the remote connection
	if ( GRemoteConnection )
	{
		delete GRemoteConnection;
		GRemoteConnection = nullptr;
	}

#if defined(RED_FINAL_BUILD) || defined(RED_PROFILE_BUILD)
	// If networking is enabled in final, make sure it's only activated by a command line switch
	Bool netEnabled = Red::System::StringSearch( SGetCommandLine(), TXT( "-net" ) ) != nullptr;
	if( !netEnabled )
	{
#endif

		if (m_network.IsInitialized())
		{
			// Ensure the network thread has shut down by the time this function has finished
			m_network.WaitForShutdown();
		}
		else
		{
			m_network.ImmediateShutdown();
		}

#if defined(RED_FINAL_BUILD) || defined(RED_PROFILE_BUILD)
	}
#endif
#endif // RED_NETWORK_ENABLED

	if ( GTaskManager )
	{
		GTaskManager->Shutdown();
	}

	extern void SDestroyTaskManager();
	SDestroyTaskManager();

	RED_MEMORY_UNREGISTER_OOM_CALLBACK( OutOfMemoryCallbacks::OnCoreOutOfMemory );

	Red::Threads::ShutdownFramework();
}
