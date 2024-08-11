/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/contentManager.h"
#include "../../common/core/xvcPackage.h"
#include "../../common/core/contentListener.h"
#include "../../common/core/contentManifest.h"

#include "streamingInstallerDurango.h"

#include <collection.h>

using namespace Platform::Collections;

extern Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;

CStreamingInstallerDurango::CStreamingInstallerDurango()
	: m_launchContent( nullptr )
	, m_finalContent( nullptr )
	, m_bootChunkID( INVALID_XVC_CHUNK_ID )
	, m_transferManager( nullptr )
	, m_chunkCompletedWatcher( nullptr )
	, m_chunkProgressWatcher( nullptr )
	, m_isInitialized( false )
	, m_hasBootZero( false )
{
}

CStreamingInstallerDurango::~CStreamingInstallerDurango()
{
	ShutdownTransfer();
}

Bool CStreamingInstallerDurango::Init()
{
	if ( m_isInitialized )
	{
		return true;
	}
#ifdef RED_LOGGING_ENABLED
	CTimeCounter initTimer;
#endif // RED_LOGGING_ENABLED

	if ( ! LoadXvcPackage( TXT("g:\\installer.dat") ) )
	{
		return false;
	}

	if ( ! InitTransfer() )
	{
#ifdef RED_LOGGING_ENABLED
		::DumpXvcPackage( m_xvcPackage );
#endif
		return false;
	}

	PostInit();

	RED_VERIFY( InitLanguage() );

	LOG_ENGINE( TXT("Streaming installer initialized in in %1.2fs"), initTimer.GetTimePeriod() );

#ifdef RED_LOGGING_ENABLED
	::DumpXvcPackage( m_xvcPackage );
#endif

	const Bool autoInstallChunks = false;
	m_updatePackageEvents.PushBack( SContentPackageEvent( BASE_RUNTIME_PACKAGE_ID, TXT("g:\\content\\"), autoInstallChunks ) );

	return true;
}

void CStreamingInstallerDurango::PostInit()
{
	for ( const SXvcContent& content : m_xvcPackage.m_installOrderContent )
	{
		RED_FATAL_ASSERT( content.m_contentName, "Content name cannot be NONE");

		auto insertContentResult = m_contentNameMap.Insert( content.m_contentName, &content );
		RED_VERIFY( insertContentResult != m_contentNameMap.End() );
		auto insertIdResult = m_chunkContentMap.Insert( content.m_chunkID, &content );
		RED_VERIFY( insertIdResult != m_chunkContentMap.End() );

		m_validChunkIDs.Set( content.m_chunkID );

		for ( const SXvcLanguageChunk& langChunk : content.m_languageChunks )
		{
			RED_VERIFY( m_chunkContentMap.Insert( langChunk.m_chunkID, &content ) != m_chunkContentMap.End() );
			m_validChunkIDs.Set( langChunk.m_chunkID );
		}

		// Only last one set in install order is the relevant
		if ( content.m_contentName == m_xvcPackage.m_launchContentName )
		{
			m_launchContent = &content;
		}

		m_finalContent = &content;
	}

	RED_FATAL_ASSERT( m_launchContent, "Failed to find launch content %ls", m_xvcPackage.m_launchContentName.AsChar() );
	RED_FATAL_ASSERT( m_finalContent, "Failed to find final content" );

	LOG_ENGINE(TXT("Final install order content is %ls"), m_finalContent->m_contentName.AsChar() );

	const SXvcContent* boot0 = GetContent( CNAME(content0) );
	RED_FATAL_ASSERT( boot0, "boot0 content missing!" );
	m_bootChunkID = boot0->m_chunkID;
}

void CStreamingInstallerDurango::OnSuspend()
{
	ShutdownTransfer();
	CancelProgressWatcher();
}

void CStreamingInstallerDurango::OnResume()
{
	// XR verify: assume locale can't have changed without restarting app.
	// !!! Verify no hangs from updating install too soon after resume!!! or do in a thread
	InitTransfer();
	RefreshTodoList();
}

Bool CStreamingInstallerDurango::InitTransfer()
{
	if ( m_chunkCompletedWatcher )
	{
		ERR_ENGINE(TXT("CStreamingInstallerDurango::InitTransfer - already initialized! Multiple OnResume() events without suspend?!"));
		return false;
	}

	m_transferManager = Windows::Xbox::Management::Deployment::PackageTransferManager::Current;

#ifdef RED_LOGGING_ENABLED
	CTimeCounter initTimer;
#endif // RED_LOGGING_ENABLED

	typedef Windows::Foundation::Collections::IVector<Uint32>												IChunkVector;
	typedef Platform::Collections::Vector<Uint32>															ChunkVector;
	typedef Windows::Foundation::TypedEventHandler< PackageTransferWatcher^, ChunkCompletedEventArgs^ >		TTypedEventHandler;

	// FIXME: Possibly create the watcher on a thread?
	IChunkVector^ vecChunkId = ref new ChunkVector;

	// Watch all the chunks regardless of language. We just want to know what's installed.
	LOG_ENGINE(TXT("CStreamingInstallerDurango: monitored content:"));

	vecChunkId->Append( m_xvcPackage.m_binChunkID );
	LOG_ENGINE(TXT("\tbinChunkID {%u}"), m_xvcPackage.m_binChunkID );

	for ( const SXvcContent& content : m_xvcPackage.m_installOrderContent )
	{
		vecChunkId->Append( content.m_chunkID );
		LOG_ENGINE(TXT("\tcontentName {%ls} chunkID {%u}"), content.m_contentName.AsChar(), content.m_chunkID );

		for ( const SXvcLanguageChunk& langChunk : content.m_languageChunks )
		{
			vecChunkId->Append( langChunk.m_chunkID );
			LOG_ENGINE(TXT("\t\tlocale {%hs} chunkID {%u}"), langChunk.m_locale.AsChar(), langChunk.m_chunkID );
		}
	}

	m_chunkCompletedWatcher = PackageTransferWatcher::Create( Windows::ApplicationModel::Package::Current, vecChunkId );
	m_chunkCompletedToken = m_chunkCompletedWatcher->ChunkCompleted += ref new TTypedEventHandler(
		[=](PackageTransferWatcher^ ptm, ChunkCompletedEventArgs^ args) {
			GCoreDispatcher->RunAsync( Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([=](){
				OnChunkCompleted( args->ChunkId );
	} ) ); } );

#ifdef RED_LOGGING_ENABLED
		LOG_ENGINE(TXT("InitTransfer: initialized in %1.2f sec"), initTimer.GetTimePeriod() );
#endif

	return true;	
}

Bool CStreamingInstallerDurango::InitLanguage()
{
	String textLanguage;
	String speechLanguage;
	if ( ! GetValidDefaultGameLanguage( textLanguage, speechLanguage ) )
	{
		return false;
	}

	if ( ! SetSpeechInstallLanguage( speechLanguage ) )
	{
		return false;
	}

	LOG_CORE(TXT("CStreamingInstallerDurango::InitLanguage(): Initial locale set to '%hs' (speechLanguage %ls)"), m_locale.AsChar(), speechLanguage.AsChar() );

	return true;
}

void CStreamingInstallerDurango::ShutdownTransfer()
{
	m_transferManager = nullptr;

	CancelProgressWatcher();

	if ( m_chunkCompletedWatcher )
	{
		m_chunkCompletedWatcher->ChunkCompleted -= m_chunkCompletedToken;
		m_chunkCompletedToken = EventRegistrationToken();
		m_chunkCompletedWatcher = nullptr;
	}
}

Bool CStreamingInstallerDurango::LoadXvcPackage( const String& filePath )
{
#ifdef RED_LOGGING_ENABLED
	CTimeCounter loadTimer;
#endif // RED_LOGGING_ENABLED

	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( filePath, FOF_Buffered | FOF_AbsolutePath ) );
	if ( ! file )
	{
		ERR_ENGINE(TXT("LoadXvcPackage: Failed to open '%ls'"), filePath.AsChar());
		return false;
	}

	*file << m_xvcPackage;

	RED_FATAL_ASSERT( SXvcPackage::Verify( m_xvcPackage ), "Invalid XVC package loaded!" );

#ifdef RED_LOGGING_ENABLED
	LOG_ENGINE(TXT("LoadXvcPackage: '%ls' loaded in %1.2f sec"), filePath.AsChar(), loadTimer.GetTimePeriod() );
#endif

	return true;
}

void CStreamingInstallerDurango::Update( TDynArray< SContentPackageEvent >& outContentPackageEvents )
{
	// Here we'll be in a tight update loop and events wouldn't otherwise get processed!
	if ( ! m_hasBootZero )
	{
		GCoreDispatcher->ProcessEvents( Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent );
	}

	// In the case of suspending and resuming, we'll create new transfer watchers and get a repeat
	// of chunk events. 
	// It's important that we don't try to download the content manifest multiple times if suspending
	// before it's loaded asynchronously, and we can't check for installed chunk IDs because it's not installed yet.
	
	// Therefore we get the set of chunk IDs downloaded since the last update and kick off installing those.
	// If a downloaded chunkID is redundantly set, it won't matter whether it's been processed yet
	TBitSet64< MAX_XVC_CHUNKS > newDownloadedChunkIds = m_downloadedChunkIDs;
	newDownloadedChunkIds ^= m_snapshotDownloadedChunkIDs;
	m_snapshotDownloadedChunkIDs = m_downloadedChunkIDs;

	InstallReadyContent( newDownloadedChunkIds );

	// Can't "move-push" our arrays
	outContentPackageEvents.PushBack( m_updatePackageEvents );
	m_updatePackageEvents.Clear();
}

void CStreamingInstallerDurango::RefreshTodoList()
{
	typedef Windows::Foundation::Collections::IVector<Uint32>	IChunkVector;
	typedef Platform::Collections::Vector<Uint32>				ChunkVector;

	// Technically pointless specifying launch set, since should be installed before can even launch
	// m_transferManager.AreChunksInstalled() pointless? Cross-OS call.
	// FIXME: Possibly do this on a thread?
	IChunkVector^ vecChunkId = ref new ChunkVector;

	LOG_ENGINE(TXT("CStreamingInstallerDurango::RefreshTodoList():"));
	if ( ! m_downloadedChunkIDs.Get( m_xvcPackage.m_binChunkID ) )
	{
		vecChunkId->Append( m_xvcPackage.m_binChunkID );
		LOG_ENGINE(TXT("\tbinChunkID {%u}"), m_xvcPackage.m_binChunkID);
	}
	for ( const SXvcContent& content : m_xvcPackage.m_installOrderContent )
	{
		if ( ! m_downloadedChunkIDs.Get( content.m_chunkID ) )
		{
			vecChunkId->Append( content.m_chunkID );
			LOG_ENGINE(TXT("\tcontent {%ls} chunkID {%u}"), content.m_contentName.AsChar(),content.m_chunkID);
		}
		for ( const SXvcLanguageChunk& langChunk : content.m_languageChunks )
		{
			if ( langChunk.m_locale == m_locale && ! m_downloadedChunkIDs.Get( langChunk.m_chunkID ) )
			{
				vecChunkId->Append( langChunk.m_chunkID );
				LOG_ENGINE(TXT("\t\tlocale {%hs} chunkID {%u}"), langChunk.m_locale.AsChar(), langChunk.m_chunkID);
			}
		}
	}

	// https://forums.xboxlive.com/AnswerPage.aspx?qid=d342c273-3d34-4845-a355-2710cb886d34&tgt=1
	// Our goal for the API from here on out is that UpdateInstallOrder should never cause an exception - if anything interferes with the installation process, it should just carry on right where it left off, with the last changes you pushed out.
	try
	{
		m_transferManager->UpdateInstallOrder( vecChunkId, Windows::Xbox::Management::Deployment::UpdateInstallOrderBehavior::InterruptCurrentTransfer );
		LOG_ENGINE(TXT("\tUpdateInstallOrder completed"));
	}
	catch ( Platform::Exception^ ex )
	{
		ERR_ENGINE(TXT("UpdateInstallOrder exception: HResult = 0x%x, Message = %s"), ex->HResult, ex->Message );
	}
}

Bool CStreamingInstallerDurango::GetPercentCompleted( CName contentName, Uint32& outPercentCompleted ) const
{
	outPercentCompleted = 0;
	CName resolvedContentName = contentName;
	if ( contentName == CNAME(launch0) )
	{
		RED_FATAL_ASSERT( m_launchContent, "No launch content!" );
		resolvedContentName = m_launchContent->m_contentName;
	}

	const SXvcContent* content = GetContent( resolvedContentName );
	if ( ! content )
	{
	 	// Silent fail since may be found in another content provider and error handling at a higher level
		return false;
	}

	const_cast<CStreamingInstallerDurango*>(this)->RefreshProgressIfNeeded( *content );

	outPercentCompleted = m_progressInfo.m_percentComplete;

	//A small lie if the content hasn't finished installing just yet
	if ( outPercentCompleted == 100 && (m_installedContent.Find( content->m_contentName) == m_installedContent.End()) )
	{
		outPercentCompleted = 99;
	}

	return true;
}

EContentStallType CStreamingInstallerDurango::GetStallForMoreContent() const 
{
	Bool isAvailable;
	if ( IsContentAvailable( m_finalContent->m_contentName, isAvailable ) )
	{
		return isAvailable ? eContentStall_None : eContentStall_FinalContent;
	}

	RED_FATAL( "Final content %ls not owned?!", m_finalContent->m_contentName.AsChar() );
	return eContentStall_None;
}

Bool CStreamingInstallerDurango::GetResolvedLaunchZero( CName& outLaunchZero ) const 
{
	if ( m_launchContent )
	{
		outLaunchZero = m_launchContent->m_contentName;
		return true;
	}

	return false;
}

void CStreamingInstallerDurango::RefreshProgressIfNeeded( const SXvcContent& contentToTrack )
{
	if ( m_progressInfo.m_contentName == contentToTrack.m_contentName )
	{
		return;
	}

	LOG_ENGINE(TXT("CStreamingInstallerDurango::RefreshProgressIfNeeded: Creating new progress watcher for contentName '%ls'"), contentToTrack.m_contentName.AsChar());

	CancelProgressWatcher();

	m_progressInfo.m_contentName = contentToTrack.m_contentName;

	typedef Windows::Foundation::Collections::IVector<Uint32>	IChunkVector;
	typedef Platform::Collections::Vector<Uint32>				ChunkVector;
	IChunkVector^ vecChunkId = ref new ChunkVector;

	LOG_ENGINE(TXT("\tWatched content:"));
	for ( const SXvcContent& content : m_xvcPackage.m_installOrderContent )
	{
		if ( !m_downloadedChunkIDs.Get( content.m_chunkID ) )
		{
			vecChunkId->Append( content.m_chunkID );
			LOG_ENGINE(TXT("\tcontentName {%ls} chunkID {%u}"), content.m_contentName.AsChar(), content.m_chunkID);
		}
		for ( const SXvcLanguageChunk& langChunk : content.m_languageChunks )
		{
			if ( langChunk.m_locale == m_locale && !m_downloadedChunkIDs.Get( langChunk.m_chunkID ) )
			{
				vecChunkId->Append( langChunk.m_chunkID );
				LOG_ENGINE(TXT("\t\tlocale {%hs} chunkID {%u}"), langChunk.m_locale.AsChar(), content.m_chunkID);
			}
		}

		if ( content.m_contentName == contentToTrack.m_contentName )
		{
			// Sync up to this point
			break;
		}
	}

	if ( vecChunkId->Size == 0 )
	{
		LOG_ENGINE(TXT("\tNo chunks to watch. Complete!"));
		m_progressInfo.m_percentComplete = 100;
		return;
	}


#ifdef RED_LOGGING_ENABLED
	CTimeCounter timeCounter;
#endif

	typedef Windows::Foundation::TypedEventHandler< PackageTransferWatcher^, ProgressChangedEventArgs^ > TTypedEventHandler;

	// FIXME: Possibly create the watcher on a thread?
	m_chunkProgressWatcher = PackageTransferWatcher::Create( Windows::ApplicationModel::Package::Current, vecChunkId );
	m_progressChangedToken = m_chunkProgressWatcher->ProgressChanged += ref new TTypedEventHandler(
		[=](PackageTransferWatcher^ ptm, ProgressChangedEventArgs^ args) {
			GCoreDispatcher->RunAsync( Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([=](){
				const Uint32 percentComplete = args->PercentComplete;
				if ( percentComplete == 100 )
				{
					m_chunkProgressWatcher->ProgressChanged -= m_progressChangedToken;
					m_chunkProgressWatcher = nullptr;
					m_progressChangedToken = EventRegistrationToken();
				}
				OnProgressChanged( percentComplete );	
			} ) ); } );

	LOG_ENGINE(TXT("RefreshProgressIfNeeded: PackageTransferWatcher created in %1.2f sec"), timeCounter.GetTimePeriod() );
}


void CStreamingInstallerDurango::CancelProgressWatcher()
{
	if ( m_chunkProgressWatcher )
	{
		m_chunkProgressWatcher->ProgressChanged -= m_progressChangedToken;
		m_progressChangedToken = EventRegistrationToken();
		m_chunkProgressWatcher = nullptr;
	}

	// Clear after removing the ProgressChanged event so can't come in from another thread and set it again after
	m_progressInfo.Clear();
}

Bool CStreamingInstallerDurango::SetSpeechInstallLanguage( const String& speechLanguage )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only" );

	// If a speech language from a DLC, then resort to the default so we'll install a backup language
	StringAnsi langToSet = UNICODE_TO_ANSI(speechLanguage.ToLower().AsChar());
	if ( ! m_xvcPackage.m_supportedSpeechLanguages.Exist( langToSet ) )
	{
		WARN_ENGINE(TXT("CStreamingInstallerDurango::SetSpeechInstallLanguage: speechLanguage '%ls' not supported. Using package default '%hs'."), 
			speechLanguage.AsChar(),
			m_xvcPackage.m_defaultSpeechLanguage.AsChar());
	
		langToSet = m_xvcPackage.m_defaultSpeechLanguage;
	}

	if ( ! m_xvcPackage.m_supportedSpeechLanguages.Exist( langToSet ) )
	{
		ERR_ENGINE(TXT("CStreamingInstallerDurango::SetSpeechInstallLanguage: package doesn't support speechLanguage '%hs'"), langToSet.AsChar());
		return false;
	}

	auto findIt = m_xvcPackage.m_languageLocaleMap.Find( langToSet );
	if ( findIt == m_xvcPackage.m_languageLocaleMap.End() )
	{
		ERR_ENGINE(TXT("Could not update XVC language for %hs"), langToSet.AsChar() );
		CancelProgressWatcher();
		return false;
	}

	const SXvcLocale newLocale = findIt->m_second;

	// Can set it now with a nop
	if ( newLocale == m_locale )
	{
		return true;
	}

	m_locale = Move( newLocale );
	CancelProgressWatcher();

	RefreshContentInstallStatus();

	LOG_ENGINE(TXT("XVC locale updated: %hs"), newLocale.AsChar() );
	RefreshTodoList();

	return true;
}

static String ToGameLanguage( const StringAnsi& language )
{
	String gameLanguage = ANSI_TO_UNICODE(language.AsChar());
	CLowerToUpper conv( gameLanguage.TypedData(), gameLanguage.Size() );
	return gameLanguage;
}

Bool CStreamingInstallerDurango::GetValidDefaultGameLanguage( String& outTextLanguage, String& outSpeechLanguage ) const
{
	UniChar systemLocaleName[ LOCALE_NAME_MAX_LENGTH ];
	if ( ::GetUserDefaultLocaleName( systemLocaleName, ARRAY_COUNT_U32(systemLocaleName) ) == 0 )
	{
		const DWORD dwLastError = ::GetLastError();
		ERR_ENGINE(TXT("CStreamingInstallerDurango: GetUserDefaultLocaleName failed with error code 0x%08X"), dwLastError );
		return false;
	}

	SXvcLocale systemLocale = SXvcLocale( UNICODE_TO_ANSI(systemLocaleName) );

	LOG_ENGINE(TXT("GetUserDefaultLocaleName returned %hs"), systemLocale.AsChar());

	StringAnsi validGameLang;
	if ( !GetLanguageFromLocale( systemLocale, validGameLang ) )
	{
		LOG_ENGINE(TXT("CStreamingInstallerDurango::GetValidDefaultGameLanguage: No language mapping for locale '%hs'"), systemLocale.AsChar() );
		systemLocale.RemoveRegionCode();
		if ( !GetLanguageFromLocale(systemLocale, validGameLang) )
		{
			LOG_ENGINE(TXT("CStreamingInstallerDurango::GetValidDefaultGameLanguage: No language mapping for locale '%hs'. Using default speech language '%hs'"), systemLocale.AsChar(), 
				m_xvcPackage.m_defaultSpeechLanguage.AsChar());
			validGameLang = m_xvcPackage.m_defaultSpeechLanguage;
		}
		else
		{
			LOG_ENGINE(TXT("CStreamingInstallerDurango::GetValidDefaultGameLanguage: language mapping found for locale '%hs' (lang %hs)"), systemLocale.AsChar(), validGameLang.AsChar());
		}
	}
	else
	{
		LOG_ENGINE(TXT("CStreamingInstallerDurango::GetValidDefaultGameLanguage exact match found for locale '%hs' (lang  %hs)"), systemLocale.AsChar(), validGameLang.AsChar() );
	}

	if ( validGameLang.Empty() )
	{
		ERR_ENGINE(TXT("CStreamingInstallerDurango::GetValidDefaultGameLanguage: no valid language chosen!"));
		return false;
	}

	Bool isSupportedText = m_xvcPackage.m_supportedTextLanguages.Exist( validGameLang );
	Bool isSupportedSpeech = m_xvcPackage.m_supportedSpeechLanguages.Exist( validGameLang );

	// "defaultSpeechLanguage" is the default for text and speech
	outTextLanguage = isSupportedText ? ToGameLanguage(validGameLang) : ToGameLanguage(m_xvcPackage.m_defaultSpeechLanguage);
	outSpeechLanguage = isSupportedSpeech ? ToGameLanguage(validGameLang) : ToGameLanguage(m_xvcPackage.m_defaultSpeechLanguage);

	LOG_CORE(TXT("CStreamingInstallerDurango::GetValidDefaultGameLanguage(): text=%ls [%ls] speech=%ls [%ls]"), 
		outTextLanguage.AsChar(), (isSupportedText ? TXT("supported") : TXT("default")),
		outSpeechLanguage.AsChar(), (isSupportedSpeech ? TXT("supported") : TXT("default")));

	return true;
}

Bool CStreamingInstallerDurango::GetSupportedLanguages( TDynArray< String >& outTextLangauges, TDynArray< String >& outSpeechLanguages, String& outDefaultLanguage ) const 
{
	outTextLangauges.ClearFast();
	outSpeechLanguages.ClearFast();

	for ( const StringAnsi& gameLang : m_xvcPackage.m_supportedTextLanguages )
	{
		outTextLangauges.PushBack( ToGameLanguage(gameLang) );
	}

	for ( const StringAnsi& gameLang : m_xvcPackage.m_supportedSpeechLanguages )
	{
		outSpeechLanguages.PushBack( ToGameLanguage(gameLang) );
	}

	outDefaultLanguage = ToGameLanguage(m_xvcPackage.m_defaultSpeechLanguage);

	return true;
}

void CStreamingInstallerDurango::OnChunkCompleted( Uint32 chunkId )
{
	LOG_ENGINE(TXT("CStreamingInstallerDurango::OnChunkCompleted: chunkID %u completed"), chunkId );
	m_downloadedChunkIDs.Set( chunkId );
}

void CStreamingInstallerDurango::OnProgressChanged( Uint32 percentComplete )
{
	m_progressInfo.m_percentComplete = percentComplete;
}

Bool CStreamingInstallerDurango::GetPackChunk( Uint32 chunkID, CName& outChunkName ) const
{
	auto findIt = m_xvcPackage.m_chunkNameMap.Find( chunkID );
	if ( findIt != m_xvcPackage.m_chunkNameMap.End() )
	{
		outChunkName = findIt->m_second;
		return true;
	}

	return false;
}

void CStreamingInstallerDurango::InstallReadyContent( const TBitSet64< MAX_XVC_CHUNKS >& chunkIDs  )
{
	for ( Uint32 chunkID = 0; chunkID < MAX_XVC_CHUNKS; ++chunkID )
	{
		chunkID = chunkIDs.FindNextSet( chunkID );
		if ( chunkID >= MAX_XVC_CHUNKS )
		{
			break;
		}

		if ( chunkID <= BIN_XVC_CHUNK_ID )
		{
			continue;
		}

		// The XDK docs say to be lenient of these chunks. Don't know why we would get chunk IDs we never registered to listen for.
		if ( !m_validChunkIDs.Get( chunkID ) )
		{
			WARN_ENGINE(TXT("CStreamingInstallerDurango: chunkID %u not part of package - likely for system internal use. Ignoring."), chunkID );
			continue;
		}

		// Already installed. Should have been filtered out by checking for new downloaded chunks.
		if ( m_installedChunkIDs.Get( chunkID ) )
		{
			ERR_ENGINE(TXT("CStreamingInstallerDurango: chunkID %u already installed. Skipping repeat processing"), chunkID );
			continue;
		}

		const SXvcContent* content = GetContent( chunkID );
		RED_FATAL_ASSERT( content, "Package broken. ChunkID %u not mapped to content", chunkID );

		// Gets the chunkName for the specific chunkID. E.g., 'content0_en'
		CName chunkName;
		if ( !GetPackChunk( chunkID, chunkName ) )
		{
			ERR_ENGINE(TXT("CStreamingInstallerDurango: chunkID %u not mapped to a chunkName!"), chunkID );
			m_errorChunkIDs.Set( chunkID );
			return;
		}

		LOG_ENGINE(TXT("=== CStreamingInstallerDurango === chunkID %u (%ls) INSTALLED"), chunkID, chunkName.AsChar());
		m_installedChunkIDs.Set( chunkID );
		UpdateContentInstallStatus( content );

		m_updatePackageEvents.PushBack( SContentPackageEvent( BASE_RUNTIME_PACKAGE_ID, chunkName ) );
	}
}

void CStreamingInstallerDurango::RefreshContentInstallStatus()
{
	m_installedContent.ClearFast();
	m_cachedAvailableContent.ClearFast();

	for ( const SXvcContent& content : m_xvcPackage.m_installOrderContent )
	{
		UpdateContentInstallStatus( &content );
	}
}

void CStreamingInstallerDurango::UpdateContentInstallStatus( const SXvcContent* content )
{
	if ( !m_installedChunkIDs.Get( content->m_chunkID) )
	{
		return;
	}
	if ( !m_hasBootZero && content->m_chunkID == m_bootChunkID )
	{
		LOG_ENGINE(TXT("=== CStreamingInstallerDurango === boot0 INSTALLED") );
		m_hasBootZero = true;
	}

#ifdef RED_LOGGING_ENABLED
	Uint32 numAvailLangChunks = 0;
#endif

	for ( const SXvcLanguageChunk& langChunk: content->m_languageChunks )
	{
		if ( langChunk.m_locale != m_locale )
		{
			// Not interested in other languages other than the current one
			continue;
		}
		if ( !m_installedChunkIDs.Get( langChunk.m_chunkID ) )
		{
			// Return since content can't be considered installed until all its language chunk IDs are installed too
			return;
		}
#ifdef RED_LOGGING_ENABLED
		++numAvailLangChunks;
#endif
	}

	LOG_ENGINE(TXT("=== CStreamingInstallerDurango === CONTENT INSTALLED '%ls' for locale %hs (%u masked language chunks/%u total)"), content->m_contentName.AsChar(), m_locale.AsChar(), numAvailLangChunks, content->m_languageChunks.Size() );

	m_installedContent.InsertUnique( content->m_contentName );
	if ( m_launchContent == content )
	{
		LOG_ENGINE(TXT("=== CStreamingInstallerDurango === launch0 INSTALLED for locale %hs"), m_locale.AsChar() );
	}
}

Bool CStreamingInstallerDurango::IsContentAvailable( CName contentName, Bool& outIsAvailable ) const
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
	for ( const SXvcContent& xvcContent : m_xvcPackage.m_installOrderContent )
	{
		if ( xvcContent.m_contentName == resolvedContentName )
		{
			isContentOwner = true;
		}

		// Prerequisite content not installed. Could have been uninstalled while title wasn't running.
		if ( m_installedContent.Find( xvcContent.m_contentName ) == m_installedContent.End() )
		{
			outIsAvailable = false;
			return true;
		}

		// Up to contentName is installed
		if ( xvcContent.m_contentName == resolvedContentName )
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

Bool CStreamingInstallerDurango::IsReady() const 
{
	return m_hasBootZero;
}

const SXvcContent* CStreamingInstallerDurango::GetContent( Uint32 chunkID ) const
{
	auto findIt = m_chunkContentMap.Find( chunkID );
	if ( findIt != m_chunkContentMap.End() )
	{
		const SXvcContent* content = findIt->m_second;
		return content;
	}

	return nullptr;
}

const SXvcContent* CStreamingInstallerDurango::GetContent( CName contentName ) const
{
	auto findIt = m_contentNameMap.Find( contentName );
	if ( findIt != m_contentNameMap.End() )
	{
		const SXvcContent* content = findIt->m_second;
		return content;
	}

	return nullptr;
}

Bool CStreamingInstallerDurango::GetLanguageFromLocale( const SXvcLocale& locale, StringAnsi& outLang ) const
{
	// Should probably have a reverse mapping
	for ( auto it = m_xvcPackage.m_languageLocaleMap.Begin(); it != m_xvcPackage.m_languageLocaleMap.End(); ++it )
	{
		const StringAnsi& tmpLang = it->m_first;
		const SXvcLocale& tmpLocale = it->m_second;
		if ( locale == tmpLocale )
		{
			outLang = tmpLang;
			return true;
		}
	}

	return false;
}
