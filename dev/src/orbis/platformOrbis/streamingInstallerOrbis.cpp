/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include <libsysmodule.h>
#include <system_param.h>
#include <system_service.h>

#include "../../common/core/contentManager.h"
#include "../../common/core/filesys.h"
#include "../../common/core/tagList.h"
#include "../../common/engine/baseEngine.h"

#include "streamingInstallerOrbis.h"
#include "../../common/core/contentListener.h"
#include "../../common/core/tokenizer.h"
#include "userProfileOrbis.h"

static Float BLURAY_PREFETCH_MAX = 59.f; // Save a second for content to attach and the time until the installer started

#ifdef RED_LOGGING_ENABLED
extern Bool GDebugDumpPlayGoProgress;
#endif

extern Bool GPlayGoFullInstallSpeedOverride;

static ScePlayGoInstallSpeedValue MapToPlayGoInstallSpeed( CStreamingInstallerOrbis::EInstallSpeed speed )
{
	ScePlayGoInstallSpeedValue playGoSpeed = SCE_PLAYGO_INSTALL_SPEED_TRICKLE;
	switch (speed )
	{
	case CStreamingInstallerOrbis::eInstallSpeed_Suspended:
		playGoSpeed = SCE_PLAYGO_INSTALL_SPEED_SUSPENDED;
		break;
	case CStreamingInstallerOrbis::eInstallSpeed_Trickle:
		playGoSpeed = SCE_PLAYGO_INSTALL_SPEED_TRICKLE;
		break;
	case CStreamingInstallerOrbis::eInstallSpeed_Full:
		playGoSpeed = SCE_PLAYGO_INSTALL_SPEED_FULL;
		break;
	default:
		break;
	}

	return playGoSpeed;
}

static CStreamingInstallerOrbis::EInstallSpeed MapArgToInstallSpeed( const String& arg )
{
	if ( arg.EqualsNC(TXT("suspended")))
	{
		return CStreamingInstallerOrbis::eInstallSpeed_Suspended;
	}
	else if ( arg.EqualsNC(TXT("trickle")))
	{
		return CStreamingInstallerOrbis::eInstallSpeed_Trickle;
	}
	else if ( arg.EqualsNC(TXT("full")))
	{
		return CStreamingInstallerOrbis::eInstallSpeed_Full;
	}

	return CStreamingInstallerOrbis::eInstallSpeed_Trickle;
}

//////////////////////////////////////////////////////////////////////////
// CStreamingInstallerOrbis::SPrefetchProgressForLogging
//////////////////////////////////////////////////////////////////////////
#ifdef RED_LOGGING_ENABLED

static String FormatTimeForLog( Uint64 sec )
{
	const Uint64 MINUTE = 60;
	const Uint64 HOUR = 60 * MINUTE;

	Uint64 hrs = 0;
	Uint64 min = 0;
	Uint64 secLeft = 0;

	if ( sec == ~0ULL )
	{
		return TXT("n/a");
	}

	if ( sec < MINUTE )
	{
		secLeft = sec;
	}
	else if ( sec < HOUR )
	{
		min = sec / MINUTE;
		secLeft = sec % MINUTE;
	}
	else
	{
		secLeft = sec;
		hrs = secLeft / HOUR;
		secLeft -= hrs * HOUR;
		min = secLeft / MINUTE;
		secLeft %= MINUTE;
	}

	return String::Printf(TXT("%02llu:%02llu:%02llu"), hrs, min, secLeft );
}

static String FormatSizeForLog( Uint64 size )
{
	const Uint64 KB = 1024;
	const Uint64 MB = KB * KB;
	const Uint64 GB = MB * KB;

	if ( size < KB )
	{
		return String::Printf(TXT("%llu B"), size );
	}
	else if ( size < MB )
	{
		return String::Printf(TXT("%1.2f KB"), size/(Float)KB);
	}
	else if ( size < GB )
	{
		return String::Printf(TXT("%1.2f MB"), size/(Float)MB);
	}

	return String::Printf(TXT("%1.2f GB"), size/(Float)GB);
}

Bool CStreamingInstallerOrbis::SPrefetchProgressForLogging::UpdateProgress()
{
	if ( !m_chunkIDs.Empty() )
	{
		const Int32 err = ::scePlayGoGetProgress( m_playGoHandle, m_chunkIDs.TypedData(), m_chunkIDs.Size(), &m_progress );
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("scePlayGoGetProgress() returned error code 0x%08X"), err );
			return false;
		}
	}
	else
	{
		Red::System::MemoryZero( &m_progress, sizeof( m_progress ) );
	}

	return true;
}
#endif

//////////////////////////////////////////////////////////////////////////
// CStreamingInstallerOrbis
//////////////////////////////////////////////////////////////////////////
CStreamingInstallerOrbis::CStreamingInstallerOrbis()
	: m_playGoBuffer( nullptr )
	, m_playGoHandle()
	, m_languageFlag( PGLF_Invalid )
	, m_launchContent( nullptr )
	, m_bootChunkID( INVALID_PLAYGO_CHUNK_ID )
	, m_hasBootZero( false )
	, m_isPlayGoInitialized( false )
	, m_hasPendingBlurayChunks( false )
	, m_requiredSlowChunksLangMask( 0 )
	, m_isInstallingAllLanguageChunksFromBluray( false )
	, m_blurayParams()
	, m_downloadParams()
	, m_isInitBlurayPrefetchFinished( false )
{
	m_tscFreq = ::sceKernelGetTscFrequency();
	m_startPriorityIOTick = 0;
	m_allowSuspendInstaller = true;
	m_asyncSuspendThrottled = false;
}

CStreamingInstallerOrbis::~CStreamingInstallerOrbis()
{
	ShutdownPlayGo();
}

Bool CStreamingInstallerOrbis::Init()
{
	m_initStartTime = EngineTime::GetNow();

	if ( m_isPlayGoInitialized )
	{
		return true;
	}

	if ( ! InitFromCommandline(m_blurayParams, m_downloadParams) )
	{
		return false;
	}

	if ( ! InitPlayGo() )
	{
		return false;
	}

	if ( ! PostInitPlaygo() )
	{
		return false;
	}

	if ( ! InitLanguage() )
	{
		return false;
	}

	if ( ! InitPrefetch() )
	{
		return false;
	}

	const Uint32 flags = eContentPackageMountFlags_None;
	m_updatePackageEvents.PushBack( SContentPackageEvent( BASE_RUNTIME_PACKAGE_ID, TXT("\\app0\\content\\"), flags ) );

	m_isPlayGoInitialized = true;

	return true;
}

Bool CStreamingInstallerOrbis::InitFromCommandline( SBlurayParams& outBlurayParams, SDownloadParams& outDownloadParams )
{
	CTokenizer tokens( SGetCommandLine(), TXT(" ") ); 
	for ( Uint32 index = 0, end = tokens.GetNumTokens(); index != end; ++index ) 
	{	
		String token = tokens.GetToken( index );
		if ( token == TXT( "-bluray_prefetch" ) )
		{
			FromString< Float >( tokens.GetToken( index + 1 ), outBlurayParams.m_initPrefetchSec );
			if ( outBlurayParams.m_initPrefetchSec < 0.f )
			{
				outBlurayParams.m_initPrefetchSec = 0.f;
			}
		}
		else if ( token == TXT("-bluray_loadingscreen_speed") )
		{
			outBlurayParams.m_loadingScreenInstallSpeed = MapArgToInstallSpeed( tokens.GetToken( index + 1 ) );
		}
		else if ( token == TXT("-bluray_launch0_speed") )
		{
			outBlurayParams.m_needPackageLaunchContent = MapArgToInstallSpeed( tokens.GetToken( index + 1 ) );
		}
		else if ( token == TXT("-bluray_idle_speed") )
		{
			outBlurayParams.m_idleInstallSpeed = MapArgToInstallSpeed( tokens.GetToken( index + 1 ) );
		}
		else if ( token == TXT("-download_loadingscreen_speed") )
		{
			outDownloadParams.m_loadingScreenInstallSpeed = MapArgToInstallSpeed( tokens.GetToken( index + 1 ) );
		}
		else if ( token == TXT("-download_idle_speed") )
		{
			outDownloadParams.m_idleInstallSpeed = MapArgToInstallSpeed( tokens.GetToken( index + 1 ) );
		}
	}

	return true;
}

Bool CStreamingInstallerOrbis::LoadPlayGoChunksData( const String& filePath )
{
#ifdef RED_LOGGING_ENABLED
	CTimeCounter loadTimer;
#endif // RED_LOGGING_ENABLED

	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( filePath, FOF_Buffered | FOF_AbsolutePath ) );
	if ( ! file )
	{
		ERR_ENGINE(TXT("LoadPlayGoChunksData: Failed to open '%ls'"), filePath.AsChar());
		return false;
	}

	*file << m_playGoPackage;

	LOG_ENGINE(TXT("LoadPlayGoChunksData: '%ls' Loaded in %1.2f sec"), filePath.AsChar(), loadTimer.GetTimePeriod() );

	return true;
}

Bool CStreamingInstallerOrbis::InitPlayGo()
{
#ifdef RED_LOGGING_ENABLED
	CTimeCounter initTimer;
#endif // RED_LOGGING_ENABLED

	RED_FATAL_ASSERT( ::SIsMainThread(), "Init PlayGo on the main thread!");

	Int32 err = SCE_OK;
	m_playGoBuffer = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_StreamingInstaller, HEAP_SIZE, 16 );
	if ( ! m_playGoBuffer )
	{
		ERR_ENGINE( TXT("CStreamingInstallerOrbis: Failed to allocate PlayGo buffer!"));
		return false;
	}

	// Load PlayGo PRX
	err = ::sceSysmoduleLoadModule( SCE_SYSMODULE_PLAYGO );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("sceSysmoduleLoadModule(SCE_SYSMODULE_PLAYGO) returned error code 0x%08X"), err );
		return false;
	}
	
	// Initialize PlayGo library
	ScePlayGoInitParams initParams;
	Red::System::MemoryZero( &initParams, sizeof(ScePlayGoInitParams) );
	initParams.bufAddr = m_playGoBuffer;
	initParams.bufSize = HEAP_SIZE;
	err = ::scePlayGoInitialize( &initParams );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoInitialize() returned error code 0x%08X"), err );
		return false;
	}

	if ( ! OpenPlayGoHandle() )
	{
		return false;
	}

	if ( ! LoadPlayGoChunksData(TXT("/app0/installer.dat")) )
	{
		return false;
	}

	if ( ! InitPlayGoToDoList() )
	{
#ifdef RED_LOGGING_ENABLED
		::DumpPlayGoPackage( m_playGoPackage );
#endif
		return false;
	}

	LOG_ENGINE( TXT("PlayGo initialized in in %1.2f sec"), initTimer.GetTimePeriod() );

#ifdef RED_LOGGING_ENABLED
	::DumpPlayGoPackage( m_playGoPackage );
#endif

	return true;
}

Bool CStreamingInstallerOrbis::InitLanguage()
{
	String textLanguage;
	String speechLangauge;
	if ( ! GetValidDefaultGameLanguage( textLanguage, speechLangauge ) )
	{
		return false;
	}

	if ( ! SetSpeechInstallLanguage(speechLangauge) )
	{
		return false;
	}

	LOG_ENGINE(TXT("Current PlayGo language mask: 0x%016llX (game lang %ls)"), m_languageFlag, speechLangauge.AsChar() );

	return true;
}

Bool CStreamingInstallerOrbis::InitPlayGoToDoList()
{
	// The initial PlayGo todo list is masked so initially getting from scePlayGoGetToDoList, 
	// modifying, and setting isn't safe unless it's guaranteed that the mask was originally for all languages
	// Here we set it to everything and rely on the language mask to prevent us from downloading unwanted chunks
	
	m_pendingDownloadChunkIDs.PushBack( static_cast< Uint16 >( BIN_PLAYGO_CHUNK_ID ) );

	// Add all chunks and language chunks. We'll update the language mask as appropriate.
	for ( const SPlayGoContent& content : m_playGoPackage.m_installOrderContent )
	{
		for ( const SPlayGoChunk& chunk : content.m_installOrderChunks )
		{
			m_pendingDownloadChunkIDs.PushBack( static_cast< Uint16 >( chunk.m_chunkID ) );
		}
	}

	const Uint32 numEntries = m_pendingDownloadChunkIDs.Size();
	if ( numEntries <= 1 )
	{
		ERR_ENGINE(TXT("Invalid number of PlayGo chunk entries: %u. Must have at least two chunks."), numEntries );
		return false;
	}

	const size_t allocSize = numEntries * sizeof(ScePlayGoToDo);
	ScePlayGoToDo* todoList = (ScePlayGoToDo*)RED_ALLOCA( allocSize );
	RED_FATAL_ASSERT( todoList, "Alloca failed!" );
	Red::System::MemoryZero( todoList, allocSize );
	for ( Uint32 i = 0; i < m_pendingDownloadChunkIDs.Size(); ++i )
	{
		ScePlayGoToDo& todo = todoList[i];
		todo.chunkId = m_pendingDownloadChunkIDs[i];
		todo.locus = SCE_PLAYGO_LOCUS_LOCAL_FAST;
	}

	const Int32 err = ::scePlayGoSetToDoList( m_playGoHandle, todoList, numEntries );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoSetToDoList() returned error code 0x%08X"), err );
		return false;
	}

#ifdef RED_LOGGING_ENABLED
	DumpMaskedTodoList();
#endif

	return true;
}

Bool CStreamingInstallerOrbis::InitPrefetch()
{
	if ( m_hasPendingBlurayChunks )
	{
#ifdef RED_LOGGING_ENABLED
		m_prefetchProgressForLogging.m_playGoHandle = m_playGoHandle;
		m_prefetchProgressForLogging.m_chunkIDs = m_pendingDownloadChunkIDs;
		m_prefetchProgressForLogging.UpdateProgress();
#endif

		Uint32 numEntries;
		if ( !GetNumMaskedTodoListEntries( numEntries ) )
		{
			return false;
		}

		if ( numEntries < 1 )
		{
			LOG_ENGINE(TXT("CStreamingInstallerOrbis::InitPrefetch: todo list finished for languageFlag 0x%016llX..."), m_languageFlag );
			InstallAllLanguageChunksFromBluray();
		}

		LOG_ENGINE(TXT("CStreamingInstallerOrbis: Starting bluray prefetch for up to %1.2f sec with languageFlag 0x%016llX..."), 
			m_blurayParams.m_initPrefetchSec,
			m_languageFlag	);

		const SceInt32 err = ::scePlayGoSetInstallSpeed( m_playGoHandle, SCE_PLAYGO_INSTALL_SPEED_FULL );
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("scePlayGoSetInstallSpeed() returned error code 0x%08X"), err );
			return false;
		}
		LOG_ENGINE(TXT("CStreamingInstallerOrbis: Bluray install detected"));
	}
	else
	{
		LOG_ENGINE(TXT("CStreamingInstallerOrbis: No pending bluray chunks detected"));
	}

	return true;
}

Bool CStreamingInstallerOrbis::PostInitPlaygo()
{
	if ( !m_pendingDownloadChunkIDs.Empty() )
	{
		const Uint32 numEntries = m_pendingDownloadChunkIDs.Size();
		const size_t allocSize = numEntries * sizeof(ScePlayGoLocus);
		ScePlayGoLocus* locusList = (ScePlayGoLocus*)RED_ALLOCA( allocSize );
		RED_FATAL_ASSERT( locusList, "Alloca failed!" );

		const Int32 err = ::scePlayGoGetLocus( m_playGoHandle, m_pendingDownloadChunkIDs.TypedData(), numEntries, locusList );
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("scePlayGoGetLocus() returned error code 0x%08X"), err );
			return false;
		}

		for ( Uint32 i = 0; i < numEntries; ++i )
		{
			const Bool isBlurayChunk = (locusList[i] == SCE_PLAYGO_LOCUS_LOCAL_SLOW);
			if ( isBlurayChunk )
			{
				m_hasPendingBlurayChunks = true;
				break;
			}
		}
	}

	Bool foundLaunchContent = false;
	for ( const SPlayGoContent& content : m_playGoPackage.m_installOrderContent )
	{
		RED_FATAL_ASSERT( content.m_contentName, "Content name cannot be NONE");
		RED_VERIFY( m_contentNameMap.Insert( content.m_contentName, &content ) != m_contentNameMap.End() );

		if ( content.m_installOrderChunks.Empty() )
		{
			ERR_ENGINE(TXT("content '%ls' has no chunks!"), content.m_contentName.AsChar() );
			return false;
		}

		for ( const SPlayGoChunk& chunk : content.m_installOrderChunks )
		{
			RED_VERIFY( m_chunkContentMap.Insert( chunk.m_chunkID, &content ) != m_chunkContentMap.End() );
		}
		
		if ( !foundLaunchContent )
		{
			for ( const SPlayGoChunk& chunk : content.m_installOrderChunks )
			{
				const Uint32 chunkID = chunk.m_chunkID;
				m_slowChunkIDs.Set( chunkID );
				m_pendingSlowChunkIDs.PushBack( static_cast< Uint16 >( chunkID ) );
			}
		}

		// Only last one set in install order is the relevant
		if ( content.m_contentName == m_playGoPackage.m_launchContentName )
		{
			m_launchContent = &content;
			foundLaunchContent = true; // finished launch set, the rest of the chunks need to be in the fast set now
		}

		m_finalContent = &content; 
	}

	RED_FATAL_ASSERT( m_launchContent, "Failed to find launch content %ls", m_playGoPackage.m_launchContentName.AsChar() );
	RED_FATAL_ASSERT( m_finalContent, "Failed to find final content" );

	LOG_ENGINE(TXT("Final install order content is %ls"), m_finalContent->m_contentName.AsChar() );

	const SPlayGoContent* boot0 = GetContent( CNAME(content0) );
	if ( ! boot0 )
	{
		ERR_ENGINE(TXT("boot0 content missing!"));
		return false;
	}

	// For now make it the last all languages chunk. Any prefetch chunk might not be enough yet.
	m_bootChunkID = INVALID_PLAYGO_CHUNK_ID;
	for ( const SPlayGoChunk& chunk : boot0->m_installOrderChunks )
	{
		if ( chunk.m_languageFlag != PGLF_AllLanguages )
		{
			break;
		}
		m_bootChunkID = chunk.m_chunkID;
	}

	RED_FATAL_ASSERT( m_bootChunkID != INVALID_PLAYGO_CHUNK_ID, "Could not find a valid boot0 chunk!");

	return true;
}

void CStreamingInstallerOrbis::ShutdownPlayGo()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Shutdown PlayGo on the main thread!");

#ifdef RED_LOGGING_ENABLED
	CTimeCounter shutdownTimer;
#endif // RED_LOGGING_ENABLED

	if ( ! m_isPlayGoInitialized )
	{
		return;
	}

	ClosePlayGoHandle();

	Int32 err = SCE_OK;
	
	err = ::scePlayGoTerminate();
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoTerminate() returned error code 0x%08X"), err );
	}

	if ( m_playGoBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_StreamingInstaller, m_playGoBuffer );
		m_playGoBuffer = nullptr;
	}

	if ( ::sceSysmoduleIsLoaded( SCE_SYSMODULE_PLAYGO ) == SCE_SYSMODULE_LOADED )
	{
		err = ::sceSysmoduleUnloadModule(SCE_SYSMODULE_PLAYGO);
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("sceSysmoduleUnloadModule(SCE_SYSMODULE_PLAYGO) returned error code 0x%08X"), err );
		}
	}

	m_isPlayGoInitialized = false;

#ifdef RED_LOGGING_ENABLED
	LOG_ENGINE( TXT("PlayGo shut down in in %1.2fs"), shutdownTimer.GetTimePeriod() );
#endif // RED_LOGGING_ENABLED
}

Bool CStreamingInstallerOrbis::OpenPlayGoHandle()
{
	const Int32 err = ::scePlayGoOpen( &m_playGoHandle, nullptr );
	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("scePlayGoOpen() returned error code 0x%08X"), err );
		return false;
	}

	return true;
}

void CStreamingInstallerOrbis::ClosePlayGoHandle()
{
	const Int32 err = ::scePlayGoClose( m_playGoHandle );
	m_playGoHandle = ScePlayGoHandle();

	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("scePlayGoClose() returned error code 0x%08X"), err );
	}
}

static String ToGameLanguage( const StringAnsi& language )
{
	String gameLanguage = ANSI_TO_UNICODE(language.AsChar());
	CLowerToUpper conv( gameLanguage.TypedData(), gameLanguage.Size() );
	return gameLanguage;
}

EContentStallType CStreamingInstallerOrbis::GetStallForMoreContent() const 
{
	if ( m_prefetchChunkIDs.FindNextSet( 0 ) < MAX_CHUNKS )
	{
		return eContentStall_Prefetch;
	}

	if ( m_requiredSlowChunksLangMask == PGLF_AllLanguages )
	{
		return eContentStall_DiscLaunch;
	}

	Bool isAvailable;
	if ( IsContentAvailable( m_finalContent->m_contentName, isAvailable ) && !isAvailable )
	{
		return eContentStall_FinalContent;
	}

	return eContentStall_None;
}

void CStreamingInstallerOrbis::PrefetchContent( CName contentName )
{
	// Adds to the list of prefetched content
	const SPlayGoContent* content = GetContent( contentName );
	if ( !content )
	{
		return;
	}

	for ( const SPlayGoChunk& chunk : content->m_installOrderChunks )
	{
		if ( !IsSlowChunk( chunk.m_chunkID ) )
		{
			continue;
		}

		TPlayGoLanguageFlag langFlag = PGLF_AllLanguages;
		if ( chunk.m_chunkID == BIN_PLAYGO_CHUNK_ID || GetChunkLanguageFlag( chunk.m_chunkID, langFlag ) )
		{
			if ( (langFlag & m_languageFlag ) != 0 )
			{
				m_prefetchChunkIDs.Set( chunk.m_chunkID );
			}
		}
		else
		{
			ERR_ENGINE(TXT("Slow chunkID %u has no language flag mapping!"), chunk.m_chunkID);
		}	
	}
}

void CStreamingInstallerOrbis::ResetPrefetchContent()
{
	m_prefetchChunkIDs.ClearAll();
}

Bool CStreamingInstallerOrbis::GetResolvedLaunchZero( CName& outLaunchZero ) const 
{
	if ( m_launchContent )
	{
		outLaunchZero = m_launchContent->m_contentName;
		return true;
	}

	return false;
}

Bool CStreamingInstallerOrbis::GetValidDefaultGameLanguage( String& outTextLanguage, String& outSpeechLanguage ) const
{
	SceSystemParamLang systemLanguage;
	Int32 err = ::sceSystemServiceParamGetInt( SCE_SYSTEM_SERVICE_PARAM_ID_LANG, &systemLanguage );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("sceSystemServiceParamGetInt() returned error code 0x%08X"), err );
		return false;
	}

	LOG_ENGINE(TXT("sceSystemServiceParamGetInt( SCE_SYSTEM_SERVICE_PARAM_ID_LANG): systemLanguage=%d"), systemLanguage);

	const ScePlayGoLanguageMask systemLanguageMask = ::scePlayGoConvertLanguage( systemLanguage );
	if ( systemLanguageMask == 0 )
	{
		ERR_ENGINE(TXT("scePlayGoConvertLanguage() returned zero language mask for language ID '%d'"), systemLanguage );
		return false;
	}

	LOG_ENGINE(TXT("GetValidDefaultGameLanguage: systemLanguageMask (0x%016llX)"), systemLanguageMask);

	StringAnsi validGameLang;
	if ( !GetLanguageFromLanguageFlag( static_cast< TPlayGoLanguageFlag >( systemLanguageMask ), validGameLang ) )
	{
		validGameLang = m_playGoPackage.m_defaultSpeechLanguage;
		LOG_ENGINE(TXT("GetValidDefaultGameLanguage: no matching language for systemLanguageMask 0x%016llX. Choosing default language '%hs'"), systemLanguageMask, validGameLang.AsChar() );
	}

	if ( validGameLang.Empty() )
	{
		ERR_ENGINE(TXT("GetValidDefaultGameLanguage: no valid language chosen!"));
		return false;
	}

	Bool isSupportedText = m_playGoPackage.m_supportedTextLanguages.Exist( validGameLang );
	Bool isSupportedSpeech = m_playGoPackage.m_supportedSpeechLanguages.Exist( validGameLang );

	// "defaultSpeechLanguage" is the default for text and speech
	outTextLanguage = isSupportedText ? ToGameLanguage(validGameLang) : ToGameLanguage(m_playGoPackage.m_defaultSpeechLanguage);
	outSpeechLanguage = isSupportedSpeech ? ToGameLanguage(validGameLang) : ToGameLanguage(m_playGoPackage.m_defaultSpeechLanguage);

	LOG_CORE(TXT("GetValidDefaultGameLanguage(): text=%ls [%ls] speech=%ls [%ls]"), 
		outTextLanguage.AsChar(), (isSupportedText ? TXT("supported") : TXT("default")),
		outSpeechLanguage.AsChar(), (isSupportedSpeech ? TXT("supported") : TXT("default")));

	return true;
}

Bool CStreamingInstallerOrbis::SetSpeechInstallLanguage( const String& speechLanguage )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only" );

	// If a speech language from a DLC, then resort to the default so we'll install a backup language
	// And help with the bluray TRC to install all chunks eventually anyway
	StringAnsi langToSet = UNICODE_TO_ANSI(speechLanguage.ToLower().AsChar());
	if ( ! m_playGoPackage.m_supportedSpeechLanguages.Exist( langToSet ) )
	{
		WARN_ENGINE(TXT("CStreamingInstallerOrbis::SetSpeechInstallLanguage: speechLanguage '%ls' not supported. Using default '%hs'"), 
			speechLanguage.AsChar(),
			m_playGoPackage.m_defaultSpeechLanguage.AsChar());

		langToSet = m_playGoPackage.m_defaultSpeechLanguage;
	}

	if ( ! m_playGoPackage.m_supportedSpeechLanguages.Exist( langToSet ) )
	{
		ERR_ENGINE(TXT("CStreamingInstallerOrbis::SetSpeechInstallLanguage: package doesn't support speechLanguage '%hs'"), langToSet.AsChar());
		return false;
	}

	TPlayGoLanguageFlag newLanguageFlag;
	if ( !GetPlayGoLanguageFlag( langToSet, newLanguageFlag ) )
	{
		ERR_ENGINE(TXT("Could not update PlayGo language for %hs"), langToSet.AsChar() );
		return false;
	}

	// Can set it now with a nop
	if ( newLanguageFlag == m_languageFlag )
	{
		return true;
	}

	const Int32 err = ::scePlayGoSetLanguageMask( m_playGoHandle, static_cast< ScePlayGoLanguageMask >( newLanguageFlag ) );
	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("scePlayGoSetLanguageMask(0x%016llX) returned error code 0x%08X"), static_cast<Uint64>( newLanguageFlag ), err );
		return false;
	}
	LOG_ENGINE(TXT("PlayGo language flag updated: 0x%016llX"), static_cast<Uint64>(newLanguageFlag) );

	m_languageFlag = newLanguageFlag;
	m_progressInfo.Clear();

	// Clear prefetch since could include language chunks we're no longer installing off Blu-ray (yet). Requires re-prefetching after language change.
	m_prefetchChunkIDs.ClearAll();

	// We set the language mask back to prioritize the selected language
	m_isInstallingAllLanguageChunksFromBluray = false;

	RefreshContentInstallStatus();

#ifdef RED_LOGGING_ENABLED
	DumpMaskedTodoList();
#endif

	return true;
}

Bool CStreamingInstallerOrbis::GetSupportedLanguages( TDynArray< String >& outTextLangauges, TDynArray< String >& outSpeechLanguages, String& outDefaultLanguage ) const 
{
	outTextLangauges.ClearFast();
	outSpeechLanguages.ClearFast();

	for ( const StringAnsi& gameLang : m_playGoPackage.m_supportedTextLanguages )
	{
		outTextLangauges.PushBack( ToGameLanguage(gameLang) );
	}

	for ( const StringAnsi& gameLang : m_playGoPackage.m_supportedSpeechLanguages )
	{
		outSpeechLanguages.PushBack( ToGameLanguage(gameLang) );
	}

	outDefaultLanguage = ToGameLanguage(m_playGoPackage.m_defaultSpeechLanguage);

	return true;
}

Bool CStreamingInstallerOrbis::IsReady() const 
{
	if ( m_isInitBlurayPrefetchFinished )
	{
		return m_hasBootZero;
	}

	return false;
}

Bool CStreamingInstallerOrbis::GetNumMaskedTodoListEntries( Uint32& outNumEntries ) const
{
	// Check if we need to prefetch non-current language files when the bluray is almost completely copied to HDD
	// The todo list masks doesn't return content that's language-masked out
	static ScePlayGoToDo todoList[ MAX_CHUNKS ];
	const SceInt32 err = ::scePlayGoGetToDoList( m_playGoHandle, todoList, ARRAY_COUNT_U32( todoList ), &outNumEntries );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetToDoList() returned error code 0x%08X"), err );
		return false;
	}

	return true;
}

void CStreamingInstallerOrbis::Update( TDynArray< SContentPackageEvent >& outContentPackageEvents )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only" );

	TBitSet64< MAX_CHUNKS > newDownloadedChunkIDs;
	UpdateDownloadChunkLoci( newDownloadedChunkIDs );
	UpdateSlowChunkLoci();
	UpdateProgress();
	UpdatePrefetch();
	UpdateInstallSpeed();
	InstallReadyContent( newDownloadedChunkIDs );

	// Installed a new chunk, so allow suspending the installer again until the next ban
	// If no more chunks to install, then doesn't really matter that you can't suspend the installer...
	if ( !m_allowSuspendInstaller && newDownloadedChunkIDs.FindNextSet(0) != MAX_CHUNKS )
	{
		m_allowSuspendInstaller = true;
	}

	Uint32 numEntries;
	if ( m_hasPendingBlurayChunks && !m_isInstallingAllLanguageChunksFromBluray && GetNumMaskedTodoListEntries( numEntries ) && numEntries < 1 )
	{
		LOG_ENGINE(TXT("CStreamingInstallerOrbis: All chunks for language 0x%016llX downloaded. Installing the rest from bluray..."));
		InstallAllLanguageChunksFromBluray();
	}

#ifndef RED_FINAL_BUILD
	UpdateDebugStats();
#endif

#ifdef RED_LOGGING_ENABLED
	if ( GDebugDumpPlayGoProgress )
	{
		GDebugDumpPlayGoProgress = false;
		DumpMaskedTodoList();
		DumpAllProgress();
	}
#endif

	// Can't "move-push" our arrays
	outContentPackageEvents.PushBack( m_updatePackageEvents );
	m_updatePackageEvents.Clear();
}

void CStreamingInstallerOrbis::InstallReadyContent( const TBitSet64< MAX_CHUNKS >& chunkIDs )
{
	for ( Uint32 chunkID = 0; chunkID < MAX_CHUNKS; ++chunkID )
	{
		chunkID = chunkIDs.FindNextSet( chunkID );
		if ( chunkID >= MAX_CHUNKS )
		{
			break;
		}

		if ( chunkID <= BIN_PLAYGO_CHUNK_ID )
		{
			continue;
		}

		const SPlayGoContent* content = GetContent( chunkID );
		RED_FATAL_ASSERT( content, "chunkID %u not mapped to any content yet was installed", chunkID );

		// Gets the chunkName for the specific chunkID. E.g., 'content0_en'
		CName chunkName;
		if ( !GetPackChunk( chunkID, chunkName ) )
		{
			ERR_ENGINE(TXT("CStreamingInstallerOrbis: chunkID %u not mapped to a chunkName!"), chunkID );
			m_errorChunkIDs.Set( chunkID );
			return;
		}

		LOG_ENGINE(TXT("=== CStreamingInstallerOrbis === chunkID %u (%ls) INSTALLED"), chunkID, chunkName.AsChar());
		m_installedChunkIDs.Set( chunkID );
		UpdateContentInstallStatus( content );

		m_updatePackageEvents.PushBack( SContentPackageEvent( BASE_RUNTIME_PACKAGE_ID, chunkName ) );
	}
}

void CStreamingInstallerOrbis::UpdateContentInstallStatus( const SPlayGoContent* content )
{
	if ( !m_hasBootZero )
	{
		for ( const SPlayGoChunk& chunk : content->m_installOrderChunks  )
		{
			if ( chunk.m_chunkID == m_bootChunkID && m_installedChunkIDs.Get(chunk.m_chunkID) )
			{
				LOG_ENGINE(TXT("=== CStreamingInstallerOrbis === boot0 AVAILABLE") );
				m_hasBootZero = true;
				break;
			}
		}
	}

	for ( const SPlayGoChunk& chunk : content->m_installOrderChunks )
	{
		if ( (chunk.m_languageFlag & m_languageFlag) == 0 )
		{
			continue;
		}
		if ( !m_installedChunkIDs.Get( chunk.m_chunkID ) )
		{
			// Return since content can't be considered installed until all its language-masked chunk IDs are installed too
			return;
		}
	}

	LOG_ENGINE(TXT("=== CStreamingInstallerOrbis === CONTENT INSTALLED '%ls' for language mask 0x%016llX"), content->m_contentName.AsChar(), m_languageFlag );

	m_installedContent.InsertUnique( content->m_contentName );
	if ( m_launchContent == content )
	{
		LOG_ENGINE(TXT("=== CStreamingInstallerOrbis === launch0 INSTALLED for language mask 0x%016llX"), m_languageFlag );
	}
	if ( m_finalContent == content )
	{
		LOG_ENGINE(TXT("=== CStreamingInstallerOrbis === final content '%ls' INSTALLED for language mask 0x%016llX"), m_finalContent->m_contentName.AsChar(), m_languageFlag );
	}
}

// For R4153 - the letter but not necessarily the spirit of TRC. Awaiting confirmation of whether we actually need to do this.
Bool CStreamingInstallerOrbis::InstallAllLanguageChunksFromBluray()
{
	if ( m_isInstallingAllLanguageChunksFromBluray )
	{
		return true;
	}

	if ( !m_hasPendingBlurayChunks )
	{
		LOG_ENGINE(TXT("CStreamingInstallerOrbis::InstallAllLanguageChunksFromBluray enabled without pending bluray chunks. Nothing more to do."));
		return true;
	}

	const Int32 err = ::scePlayGoSetLanguageMask( m_playGoHandle, SCE_PLAYGO_LANGUAGE_MASK_ALL );
	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("CStreamingInstallerOrbis::InstallAllLanguageChunksFromBluray: scePlayGoSetLanguageMask(SCE_PLAYGO_LANGUAGE_MASK_ALL) returned error code 0x%08X"), err );
		return false;
	}

	LOG_ENGINE(TXT("CStreamingInstallerOrbis::InstallAllLanguageChunksFromBluray: PlayGo language flag updated to SCE_PLAYGO_LANGUAGE_MASK_ALL") );

	m_isInstallingAllLanguageChunksFromBluray = true;

#ifdef RED_LOGGING_ENABLED
	DumpMaskedTodoList();
#endif

	return true;
}

void CStreamingInstallerOrbis::RefreshContentInstallStatus()
{
	m_installedContent.ClearFast();
	m_cachedAvailableContent.ClearFast();

	for ( const SPlayGoContent& content : m_playGoPackage.m_installOrderContent )
	{
		UpdateContentInstallStatus( &content );
	}
}

Bool CStreamingInstallerOrbis::GetPackChunk( Uint32 chunkID, CName& outChunkName ) const
{
	auto findIt = m_playGoPackage.m_chunkNameMap.Find( chunkID );
	if ( findIt != m_playGoPackage.m_chunkNameMap.End() )
	{
		outChunkName = findIt->m_second;
		return true;
	}

	return false;
}

Bool CStreamingInstallerOrbis::IsContentAvailable( CName contentName, Bool& outIsAvailable ) const
{
	CName resolvedContentName = contentName;
	if ( contentName == CNAME(launch0) )
	{
		RED_FATAL_ASSERT( m_launchContent, "No launch content!" );
		resolvedContentName = m_launchContent->m_contentName;
	}

	if ( m_cachedAvailableContent.Find( resolvedContentName ) != m_cachedAvailableContent.End() )
	{
		outIsAvailable = true;
		return true;
	}

	Bool isContentOwner = false; // Whether this installer even has this content and can authoritatively say anything about its install status
	for ( const SPlayGoContent& playGoContent: m_playGoPackage.m_installOrderContent )
	{
		if ( playGoContent.m_contentName == resolvedContentName )
		{
			isContentOwner = true;
		}

		// Prerequisite content not installed. Could have been uninstalled while title wasn't running.
		if ( m_installedContent.Find( playGoContent.m_contentName ) == m_installedContent.End() )
		{
			outIsAvailable = false;
			return true;
		}

		// Up to contentName is installed
		if ( playGoContent.m_contentName == resolvedContentName )
		{
			m_cachedAvailableContent.InsertUnique( resolvedContentName );
			outIsAvailable = true;
			return true;
		}
	}

	// No content? Can't be installed.

	if ( isContentOwner )
	{
		outIsAvailable = false;
		return true;
	}

	// Not our content to say anything about
	return false;
}

const SPlayGoContent* CStreamingInstallerOrbis::GetContent( Uint32 chunkID ) const
{
	auto findIt = m_chunkContentMap.Find( chunkID );
	if ( findIt != m_chunkContentMap.End() )
	{
		const SPlayGoContent* content = findIt->m_second;
		return content;
	}

	return nullptr;
}

const SPlayGoContent* CStreamingInstallerOrbis::GetContent( CName contentName ) const
{
	auto findIt = m_contentNameMap.Find( contentName );
	if ( findIt != m_contentNameMap.End() )
	{
		const SPlayGoContent* content = findIt->m_second;
		return content;
	}

	return nullptr;
}

void CStreamingInstallerOrbis::UpdatePrefetch()
{
	// Currently this would include language chunks for the non-current language, but we're still installing these to satisfy TRCs so let's
	// fetch them too
	if ( !m_isInitBlurayPrefetchFinished )
	{
		const Float prefetchTime = EngineTime::GetNow() - m_initStartTime;
		const Float timeLimit = Min< Float >( BLURAY_PREFETCH_MAX, m_blurayParams.m_initPrefetchSec );
		if ( !m_hasPendingBlurayChunks || ( prefetchTime >= timeLimit ) )
		{
#ifdef RED_LOGGING_ENABLED
			DumpPrefetchStats();
			DumpAllProgress();
#endif
			m_isInitBlurayPrefetchFinished = true;
		}
	}
}


void CStreamingInstallerOrbis::SuspendInstallerAsync()
{
	// Don't need to be too strict about updating m_allowSuspendInstaller/m_asyncSuspendThrottled asyncly. Will be unsuspended during tick if needs be.
	// The check is more to avoid hammering suspend indefinitely
	if ( !GPlayGoFullInstallSpeedOverride && !m_asyncSuspendThrottled && m_allowSuspendInstaller )
	{
		m_asyncSuspendThrottled = true;

		const Int32 err = ::scePlayGoSetInstallSpeed( m_playGoHandle, SCE_PLAYGO_INSTALL_SPEED_SUSPENDED );
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("scePlayGoSetInstallSpeed() returned error code 0x%08X"), err );
		}
	}
}

// After which time, PriorityIO() ignored until it goes a tick without requesting it..
// Giving it about the TRC limit time to actually mount/save/unmount
static const Uint64 PRIORITY_IO_MAX_DURATION_SEC = 15;

void CStreamingInstallerOrbis::UpdateInstallSpeed()
{
	ScePlayGoInstallSpeedValue installSpeedToSet = SCE_PLAYGO_INSTALL_SPEED_TRICKLE;

	const Uint64 savedStartPriorityIOTick = m_startPriorityIOTick;
	m_startPriorityIOTick = 0;
	const Bool requiresPriorityIO = GUserProfileManager ? static_cast< CUserProfileManagerOrbis* >( GUserProfileManager )->RequiresPriorityIO() : false;
	const Bool checkSuspend = m_asyncSuspendThrottled || requiresPriorityIO;
	if ( GPlayGoFullInstallSpeedOverride )
	{
		installSpeedToSet = SCE_PLAYGO_INSTALL_SPEED_FULL;
	}
	else if ( m_allowSuspendInstaller && checkSuspend )
	{
		const Uint64 tick = ::sceKernelReadTsc();
		m_startPriorityIOTick = savedStartPriorityIOTick != 0 ? savedStartPriorityIOTick : tick;
		const Uint64 diff = (tick >= m_startPriorityIOTick) ? (tick - m_startPriorityIOTick) : 0; // just in case
		static const Uint64 timeoutTicks = PRIORITY_IO_MAX_DURATION_SEC * m_tscFreq;
		if ( diff < timeoutTicks )
		{
			m_asyncSuspendThrottled = false; // unthrottle here to avoid SuspendInstallerAsync() from always just suspending again after a tick
			if ( requiresPriorityIO )
			{
				installSpeedToSet = SCE_PLAYGO_INSTALL_SPEED_SUSPENDED;
			}
		}
		else
		{
			WARN_ENGINE(TXT("CStreamingInstallerOrbis: ignoring installer suspend longer than %.2f sec"), (Double)diff/m_tscFreq);
			m_allowSuspendInstaller = false;
		}
	}
	else if ( !m_isInitBlurayPrefetchFinished )
	{
		installSpeedToSet = SCE_PLAYGO_INSTALL_SPEED_FULL;

		Uint32 numEntries;
		if ( GetNumMaskedTodoListEntries( numEntries ) && numEntries < 1 )
		{
			InstallAllLanguageChunksFromBluray();
		}
	}
	else if ( m_pendingDownloadChunkIDs.Empty() && m_pendingSlowChunkIDs.Empty() )
	{
		installSpeedToSet = SCE_PLAYGO_INSTALL_SPEED_SUSPENDED;
	}
	else if ( GGame && GGame->IsLoadingScreenShown() )
	{
		const EInstallSpeed speed = ( m_hasPendingBlurayChunks ? m_blurayParams.m_loadingScreenInstallSpeed : m_downloadParams.m_loadingScreenInstallSpeed );
		installSpeedToSet = MapToPlayGoInstallSpeed( speed );
	}
	else
	{
		if (  m_requiredSlowChunksLangMask != 0 )
		{
			const EInstallSpeed speed = m_blurayParams.m_needPackageLaunchContent;
			installSpeedToSet = MapToPlayGoInstallSpeed( speed );
		}
		else
		{
			// TBD: check for paused? Game will be inactive at some when when changing worlds, so also
			// adding the progressInfo query. This should only happen in the main menu when waiting for content
			if ( m_progressInfo.m_queried && GGame && !GGame->IsActive() /*|| IsBackgrounded()*/ )
			{
				const EInstallSpeed speed = ( m_hasPendingBlurayChunks ? m_blurayParams.m_idleInstallSpeed : m_downloadParams.m_idleInstallSpeed );
				installSpeedToSet = MapToPlayGoInstallSpeed( speed );
			}
		}
	}

	const Int32 err = ::scePlayGoSetInstallSpeed( m_playGoHandle, installSpeedToSet );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoSetInstallSpeed() returned error code 0x%08X"), err );
	}
}

Bool CStreamingInstallerOrbis::IsBackgrounded() const
{
	SceSystemServiceStatus status;
	const Int32 err = ::sceSystemServiceGetStatus( &status );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("sceSystemServiceGetStatus() returned error code 0x%08X"), err );
		return false;
	}
	//TBD: Use isSystemUiOverlaid too?
	return status.isInBackgroundExecution;
}

Bool CStreamingInstallerOrbis::GetLanguageFromLanguageFlag( TPlayGoLanguageFlag flag, StringAnsi& outLang ) const
{
	// Should probably have a reverse mapping
	for ( auto it = m_playGoPackage.m_languageFlagMap.Begin(); it != m_playGoPackage.m_languageFlagMap.End(); ++it )
	{
		const StringAnsi& tmpLang = it->m_first;
		const Uint64 tmpFlag = it->m_second;
		if ( flag == tmpFlag )
		{
			outLang = tmpLang;
			return true;
		}
	}

	return false;
}

Bool CStreamingInstallerOrbis::GetPlayGoLanguageFlag( const StringAnsi& lang, TPlayGoLanguageFlag& outFlag ) const
{
	auto findIt = m_playGoPackage.m_languageFlagMap.Find( lang.ToLower() );
	if ( findIt != m_playGoPackage.m_languageFlagMap.End() )
	{
		outFlag = static_cast< TPlayGoLanguageFlag >( findIt->m_second );
		return true;
	}

	return false;
}

Bool CStreamingInstallerOrbis::GetDefaultPlayGoLanguageFlag( TPlayGoLanguageFlag& outFlag ) const
{
	const StringAnsi& defaultGameLang = m_playGoPackage.m_defaultSpeechLanguage;
	auto findIt = m_playGoPackage.m_languageFlagMap.Find( defaultGameLang );
	if ( findIt != m_playGoPackage.m_languageFlagMap.End() )
	{
		outFlag = static_cast< TPlayGoLanguageFlag >( findIt->m_second );
		return true;
	}

	return false;
}

Bool CStreamingInstallerOrbis::GetChunkLanguageFlag( Uint32 chunkID, TPlayGoLanguageFlag& outFlag ) const
{
	auto findIt = m_chunkContentMap.Find( chunkID );
	if ( findIt != m_chunkContentMap.End() )
	{
		const SPlayGoContent* content = findIt->m_second;
		for ( const SPlayGoChunk& chunk : content->m_installOrderChunks )
		{
			if ( chunk.m_chunkID == chunkID )
			{
				outFlag = chunk.m_languageFlag;
				return true;
			}
		}
	}

	return false;
}

void CStreamingInstallerOrbis::UpdateDownloadChunkLoci( TBitSet64< MAX_CHUNKS >& outNewDownloadedChunkIDs )
{
	m_hasPendingBlurayChunks = false;

	if ( m_pendingDownloadChunkIDs.Empty() )
	{
		return;
	}

	const Uint32 numEntries = m_pendingDownloadChunkIDs.Size();
	const size_t allocSize = numEntries * sizeof(ScePlayGoLocus);
	ScePlayGoLocus* locusList = (ScePlayGoLocus*)RED_ALLOCA( allocSize );
	RED_FATAL_ASSERT( locusList, "Alloca failed!" );
	
	const Int32 err = ::scePlayGoGetLocus( m_playGoHandle, m_pendingDownloadChunkIDs.TypedData(), numEntries, locusList );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetLocus() returned error code 0x%08X"), err );
		return;
	}

	if ( !m_isInitBlurayPrefetchFinished )
	{
		// Know if we should stop prefetch before timeout
		for ( Uint32 i = 0; i < numEntries; ++i )
		{
			const Bool isBlurayChunk = (locusList[i] == SCE_PLAYGO_LOCUS_LOCAL_SLOW);
			if ( isBlurayChunk )
			{
				m_hasPendingBlurayChunks = true;
				break;
			}
		}

		return; // early return, no download updates yet
	}

	for ( Int32 j = (Int32)numEntries - 1; j >= 0; --j )
	{
		const Bool isBlurayChunk = (locusList[j] == SCE_PLAYGO_LOCUS_LOCAL_SLOW);
		const Bool isHDDChunk = (locusList[j] == SCE_PLAYGO_LOCUS_LOCAL_FAST);
		
		m_hasPendingBlurayChunks |= isBlurayChunk;

		const Uint32 chunkID = m_pendingDownloadChunkIDs[ j ]; 

		// checks for bluray prefetch so nothing will start using this chunk from disc while still trying to prefetch it
		// or interfere with copying to HDD
		Bool isDownloaded = false;
		if ( isHDDChunk )
		{
			isDownloaded = true;
		}
		else if ( isBlurayChunk && IsSlowChunk( chunkID ) )
		{
			// We're counting slow chunks as installed even if they're still on bluray. So we have to do own manual language masking here.
			TPlayGoLanguageFlag slowChunkLangFlag = PGLF_AllLanguages;
			if ( chunkID == BIN_PLAYGO_CHUNK_ID || GetChunkLanguageFlag( chunkID, slowChunkLangFlag ) )
			{
				isDownloaded = ( slowChunkLangFlag & m_languageFlag ) != 0;
			}
			else
			{
				ERR_ENGINE(TXT("Slow chunkID %u has no language flag mapping!"), chunkID);
			}
		}

		if ( isDownloaded )
		{		
			m_downloadedChunkIDs.Set( chunkID );
			outNewDownloadedChunkIDs.Set( chunkID );
			m_pendingDownloadChunkIDs.RemoveAt( j );
		}
	}
}

void CStreamingInstallerOrbis::UpdateSlowChunkLoci()
{
	m_requiredSlowChunksLangMask = 0;

	if ( m_pendingSlowChunkIDs.Empty() || !m_isInitBlurayPrefetchFinished )
	{
		return;
	}

	const Uint32 numEntries = m_pendingSlowChunkIDs.Size();
	const size_t allocSize = numEntries * sizeof(ScePlayGoLocus);
	ScePlayGoLocus* locusList = (ScePlayGoLocus*)RED_ALLOCA( allocSize );
	RED_FATAL_ASSERT( locusList, "Alloca failed!" );

	const Int32 err = ::scePlayGoGetLocus( m_playGoHandle, m_pendingSlowChunkIDs.TypedData(), numEntries, locusList );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetLocus() returned error code 0x%08X"), err );
		return;
	}

	for ( Int32 j = (Int32)numEntries - 1; j >= 0; --j )
	{
		const Uint32 chunkID = m_pendingSlowChunkIDs[ j ];
		const Bool isBlurayChunk = (locusList[j] == SCE_PLAYGO_LOCUS_LOCAL_SLOW);

		if ( !isBlurayChunk )
		{
			m_pendingSlowChunkIDs.RemoveAt( j );

			// No longer a slow chunk. Currently used to know whether certain content still needs to be cached to the HDD.
			m_slowChunkIDs.Clear( chunkID );

			m_prefetchChunkIDs.Clear( chunkID ); // Won't be reset since m_slowChunkIDs has been cleared as well
		}
		else
		{
			TPlayGoLanguageFlag slowChunkLangFlag = PGLF_AllLanguages;
			if ( chunkID == BIN_PLAYGO_CHUNK_ID || GetChunkLanguageFlag( chunkID, slowChunkLangFlag ) )
			{
				if ( (slowChunkLangFlag & m_languageFlag) != 0 )
				{
					m_requiredSlowChunksLangMask |= slowChunkLangFlag;
				}
			}
			else
			{
				ERR_ENGINE(TXT("Slow chunkID %u has no language flag mapping!"));
			}	
		}
	}
}

Bool CStreamingInstallerOrbis::IsSlowChunk( Uint32 chunkID ) const
{
	return m_slowChunkIDs.Get( chunkID );
}

Bool CStreamingInstallerOrbis::GetPercentCompleted( CName contentName, Uint32& outPercentCompleted ) const 
{
	outPercentCompleted = 0;
	CName resolvedContentName = contentName;
	if ( contentName == CNAME(launch0) )
	{
		RED_FATAL_ASSERT( m_launchContent, "No launch content!" );
		resolvedContentName = m_launchContent->m_contentName;
	}
	
	const SPlayGoContent* content = GetContent( resolvedContentName );
	if ( ! content )
	{
		// Silent fail since may be found in another content provider and error handling at a higher level
		return false;
	}

	const_cast<CStreamingInstallerOrbis*>(this)->RefreshProgressIfNeeded( *content );

	outPercentCompleted = m_progressInfo.m_percentComplete;

	//A small lie if the content hasn't finished installing just yet
	if ( outPercentCompleted == 100 && m_installedContent.Find( content->m_contentName) == m_installedContent.End() )
	{
		outPercentCompleted = 99;
	}

	return true;
}

void CStreamingInstallerOrbis::RefreshProgressIfNeeded( const SPlayGoContent& contentToTrack )
{
	if ( m_progressInfo.m_contentName == contentToTrack.m_contentName )
	{
		m_progressInfo.m_queried = true;
		return;
	}

	LOG_ENGINE(TXT("CStreamingInstallerOrbis::RefreshProgressIfNeeded: Creating new progress watcher for contentName '%ls'"), contentToTrack.m_contentName.AsChar());

	m_progressInfo.Clear();
	m_progressInfo.m_contentName = contentToTrack.m_contentName;
	m_progressInfo.m_queried = true;

	LOG_ENGINE(TXT("\tWatched content:"));
	for ( const SPlayGoContent& content : m_playGoPackage.m_installOrderContent )
	{
		for ( const SPlayGoChunk& chunk : content.m_installOrderChunks )
		{
			if ( (chunk.m_languageFlag & m_languageFlag) != 0 && !m_downloadedChunkIDs.Get( chunk.m_chunkID ) )
			{
				m_progressInfo.m_chunkIDs.PushBack( static_cast< Uint16 >( chunk.m_chunkID ) );
				LOG_ENGINE(TXT("\tcontentName %ls language mask {0x%016llX} chunkID {%u}"), content.m_contentName.AsChar(), chunk.m_languageFlag, chunk.m_chunkID);
			}
		}

		if ( content.m_contentName == contentToTrack.m_contentName )
		{
			// Sync up to this point
			break;
		}
	}

	if ( m_progressInfo.m_chunkIDs.Empty() )
	{
		LOG_ENGINE(TXT("CStreamingInstallerOrbis::RefreshProgressIfNeeded: chunkIDs empty! Complete."));
		m_progressInfo.m_percentComplete = 100;
	}
}

void CStreamingInstallerOrbis::UpdateProgress()
{
	if ( ! m_progressInfo.m_queried )
	{
		return;
	}

	m_progressInfo.m_queried = false;

	if ( ! m_progressInfo.m_contentName || m_progressInfo.m_percentComplete == 100 || m_progressInfo.m_chunkIDs.Empty() )
	{
		// Nothing to do
		return;
	}

	ScePlayGoProgress progress;
	const Int32 err = ::scePlayGoGetProgress( m_playGoHandle, m_progressInfo.m_chunkIDs.TypedData(), m_progressInfo.m_chunkIDs.Size(), &progress );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetProgress() returned error code 0x%08X"), err );
		return;
	}

	// During emulation, progressSize and totalSize are zero!
	if ( progress.progressSize == progress.totalSize )
	{
		m_progressInfo.m_percentComplete = 100;
		return;
	}

	m_progressInfo.m_percentComplete = progress.totalSize > 0 ? static_cast< Uint32 >( progress.progressSize * 100 / progress.totalSize ) : 100;
}

#ifndef RED_FINAL_BUILD
void CStreamingInstallerOrbis::UpdateDebugStats()
{
	Int32 err = SCE_OK;

	ScePlayGoInstallSpeed installSpeed;
	err = ::scePlayGoGetInstallSpeed( m_playGoHandle, &installSpeed );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetInstallSpeed() returned error code 0x%08X"), err );
		return;
	}

	const Float timeDelta = 0.f;// No GEngine yet: GEngine->GetLastTimeDeltaUnclamped();
	switch ( installSpeed )
	{
	case SCE_PLAYGO_INSTALL_SPEED_SUSPENDED:
		m_debugStats.m_totalTimeSuspended += timeDelta;
		break;
	case SCE_PLAYGO_INSTALL_SPEED_TRICKLE:
		m_debugStats.m_totalTimeTrickle += timeDelta;
		break;
	case SCE_PLAYGO_INSTALL_SPEED_FULL:
		m_debugStats.m_totalTimeFull += timeDelta;
		break;
	default:
		RED_FATAL( "Unexpected install speed '%u'", (Uint32)installSpeed );
		break;
	}
}
#endif // RED_FINAL_BUILD

#ifdef RED_LOGGING_ENABLED
void CStreamingInstallerOrbis::DumpPrefetchStats()
{
	Float prefetchTime = EngineTime::GetNow() - m_initStartTime;
	const ScePlayGoProgress startProgress = m_prefetchProgressForLogging.m_progress;
	m_prefetchProgressForLogging.UpdateProgress();
	const ScePlayGoProgress& curProgress = m_prefetchProgressForLogging.m_progress;
	const Uint64 amountFetched = curProgress.progressSize - startProgress.progressSize;
	const Uint64 totalProgress = curProgress.progressSize;
	const Uint64 totalSize = curProgress.totalSize;
	const Uint64 amountAtStart = startProgress.progressSize;
	const Float prefetchSpeed = prefetchTime > 0.f ? amountFetched/prefetchTime : FLT_MAX;
	m_prefetchProgressForLogging = SPrefetchProgressForLogging();

	LOG_ENGINE( TXT("------------------------------") );
	LOG_ENGINE( TXT("]PlayGo bluray prefetch stats[") );
	LOG_ENGINE( TXT("------------------------------") );
	LOG_ENGINE(TXT("prefetch time: %1.2f sec"), prefetchTime );
	LOG_ENGINE(TXT("prefetch size: %ls"),	FormatSizeForLog( amountFetched).AsChar() );
	LOG_ENGINE(TXT("prefetch speed: %ls/sec"), FormatSizeForLog( prefetchSpeed ).AsChar() );
	LOG_ENGINE(TXT("installed before prefetch: %ls"), FormatSizeForLog( amountAtStart ).AsChar() );
	LOG_ENGINE(TXT("total progress: %ls/%ls"), FormatSizeForLog( totalProgress ).AsChar(), FormatSizeForLog( totalSize ).AsChar() );
}

void CStreamingInstallerOrbis::DumpAllProgress()
{
	CTimeCounter timer;

	LOG_ENGINE(TXT("----------------------"));
	LOG_ENGINE(TXT("]PlayGo progress dump["));
	LOG_ENGINE(TXT("----------------------"));

	SceInt32 err;

	ScePlayGoLanguageMask langMask;
	::scePlayGoGetLanguageMask( m_playGoHandle, &langMask );
	LOG_ENGINE(TXT("languageMask: 0x%016llX"), langMask );

	ScePlayGoInstallSpeed installSpeed;
	err = ::scePlayGoGetInstallSpeed( m_playGoHandle, &installSpeed );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetProgress() returned error code 0x%08X"), err );
		return;
	}

	const Char* installSpeedTxt = TXT("<UNKNOWN>");
	switch (installSpeed)
	{
	case SCE_PLAYGO_INSTALL_SPEED_SUSPENDED:
		installSpeedTxt = TXT("suspended");
		break;
	case SCE_PLAYGO_INSTALL_SPEED_TRICKLE:
		installSpeedTxt = TXT("trickle");
		break;
	case SCE_PLAYGO_INSTALL_SPEED_FULL:
		installSpeedTxt = TXT("full");
	default:
		break;
	}

	LOG_ENGINE(TXT("installSpeed: %ls"), installSpeedTxt );

	ScePlayGoProgress progress;
	ScePlayGoEta eta;

	// Total progress/ETA
	//////////////////////////////////////////////////////////////////////////
	Uint16 chunkIDs[ MAX_CHUNKS ];
	Uint32 numChunks = 0;
	chunkIDs[ numChunks++ ] = static_cast< Uint16 >( BIN_PLAYGO_CHUNK_ID );

	for ( const SPlayGoContent& content : m_playGoPackage.m_installOrderContent )
	{
		for ( const SPlayGoChunk& chunk : content.m_installOrderChunks )
		{
			chunkIDs[ numChunks++ ] = chunk.m_chunkID;
		}
	}

	err = ::scePlayGoGetProgress( m_playGoHandle, chunkIDs, numChunks, &progress );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetProgress() returned error code 0x%08X"), err );
		return;
	}
	err = ::scePlayGoGetEta( m_playGoHandle, chunkIDs, numChunks, &eta );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetEta() returned error code 0x%08X"), err );
		return;
	}

	LOG_ENGINE(TXT("Total progress: {%ls/%ls} ETA {%ls}"),
		FormatSizeForLog( progress.progressSize ).AsChar(),
		FormatSizeForLog( progress.totalSize ).AsChar(),
		FormatTimeForLog( eta ).AsChar() );

	// Bin chunk progress/ETA
	//////////////////////////////////////////////////////////////////////////

	numChunks = 0;
	chunkIDs[ numChunks++ ] = static_cast< Uint16 >( BIN_PLAYGO_CHUNK_ID );
	err = ::scePlayGoGetProgress( m_playGoHandle, chunkIDs, numChunks, &progress );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetProgress() returned error code 0x%08X"), err );
		return;
	}
	err = ::scePlayGoGetEta( m_playGoHandle, chunkIDs, numChunks, &eta );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetEta() returned error code 0x%08X"), err );
		return;
	}

	LOG_ENGINE(TXT("bin chunkID {%u} {%ls/%ls} ETA {%ls}"), 
		BIN_PLAYGO_CHUNK_ID,
		FormatSizeForLog( progress.progressSize ).AsChar(),
		FormatSizeForLog( progress.totalSize ).AsChar(),
		FormatTimeForLog( eta ).AsChar() );


	Uint32 numEtaChunks = 0;
	Uint16 etaChunkIDs[ MAX_CHUNKS ];
	etaChunkIDs[ numEtaChunks++ ] = static_cast< Uint16 >( BIN_PLAYGO_CHUNK_ID );

	for ( const SPlayGoContent& content : m_playGoPackage.m_installOrderContent )
	{
		numChunks = 0;
		Int32 err;
		for ( const SPlayGoChunk& chunk : content.m_installOrderChunks )
		{
			if ( (chunk.m_languageFlag & m_languageFlag ) != 0 )
			{
				chunkIDs[ numChunks++ ] = chunk.m_chunkID;
				etaChunkIDs[ numEtaChunks++ ] = chunk.m_chunkID;
			}
		}
		err = ::scePlayGoGetProgress( m_playGoHandle, chunkIDs, numChunks, &progress );
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("scePlayGoGetProgress() returned error code 0x%08X"), err );
			return;
		}

		err = ::scePlayGoGetEta( m_playGoHandle, etaChunkIDs, numEtaChunks, &eta );
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("scePlayGoGetEta() returned error code 0x%08X"), err );
			return;
		}

		LOG_ENGINE(TXT("content {%ls} {%ls/%ls}, ETA {%ls}"), 
			content.m_contentName.AsChar(),	
			FormatSizeForLog( progress.progressSize ).AsChar(),
			FormatSizeForLog( progress.totalSize ).AsChar(),
			FormatTimeForLog( eta ).AsChar());
	}
}

static const Char* LocusAsName( ScePlayGoLocus locus )
{
	const Char* name = TXT("<UNKNOWN>");
	switch ( locus )
	{
	case SCE_PLAYGO_LOCUS_NOT_DOWNLOADED:
		name = TXT("SCE_PLAYGO_LOCUS_NOT_DOWNLOADED");
		break;
	case SCE_PLAYGO_LOCUS_LOCAL_SLOW:
		name = TXT("SCE_PLAYGO_LOCUS_LOCAL_SLOW");
		break;
	case SCE_PLAYGO_LOCUS_LOCAL_FAST:
		name = TXT("SCE_PLAYGO_LOCUS_LOCAL_FAST");
	default:
		break;
	}
	return name;
}

void CStreamingInstallerOrbis::DumpMaskedTodoList()
{
	Uint32 numEntries;
	static ScePlayGoToDo todoList[ MAX_CHUNKS ];
	const SceInt32 err = ::scePlayGoGetToDoList( m_playGoHandle, todoList, ARRAY_COUNT_U32( todoList ), &numEntries );
	if ( err != SCE_OK )
	{
		ERR_ENGINE( TXT("scePlayGoGetToDoList() returned error code 0x%08X"), err );
		return;
	}

	LOG_ENGINE(TXT("-----------------------"));
	LOG_ENGINE(TXT("]PlayGo todo list dump["));
	LOG_ENGINE(TXT("-----------------------"));

	ScePlayGoLanguageMask langMask;
	::scePlayGoGetLanguageMask( m_playGoHandle, &langMask );
	LOG_ENGINE(TXT("languageMask: 0x%016llX"), langMask );

	LOG_ENGINE(TXT("number of masked entries left: %u"), numEntries );
	for ( Uint32 i = 0; i < numEntries; ++i )
	{
		const ScePlayGoToDo& entry = todoList[i];
		const Char* contentTxt = TXT("<Unknown>");
		if ( entry.chunkId == BIN_PLAYGO_CHUNK_ID )
		{
			contentTxt = TXT("<bin>");
		}
		else
		{
			const SPlayGoContent* playGoContent = GetContent( entry.chunkId );
			if ( playGoContent )
			{
				contentTxt = playGoContent->m_contentName.AsChar();
			}
		}
		LOG_ENGINE(TXT("contentName {%ls} chunkID {%u} locus {%ls}"), contentTxt, (Uint32)entry.chunkId, LocusAsName(entry.locus));
	}
}
#endif
