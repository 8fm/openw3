/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "streamingInstaller.h"
#include "contentListener.h"
#include "contentManager.h"
#include "filePath.h"
#include "profilerTypes.h"

#include "cacheFilenames.h"
#include "configVar.h"
#include "contentManifestAsyncLoader.h"
#include "installer.h"
#include "contentVersion.h"
#include "configVarSystem.h"
#include "configVarLegacyWrapper.h"
#include "scriptingSystem.h"

CContentManager* GContentManager = nullptr;

#ifdef RED_LOGGING_ENABLED
Bool GDebugDumpPlayGoProgress;
#endif

const String DLC_SCAN_DIR = TXT("dlc\\");
const String CONTENT_SCAN_DIR = TXT("content\\");
const String MODS_SCAN_DIR = TXT("mods\\");
const Char* const DEFAULT_LANG_TXT = TXT("EN");

#ifdef RED_MOD_SUPPORT
namespace Config
{
	TConfigVar< Bool > cvEnableMods( "ContentManager/Mods", "Enabled", true , eConsoleVarFlag_Save );
	TConfigVar< Int32, Validation::IntRange< 1, 64 > > cvMaxModNameLength( "ContentManager/Mods", "MaxNameLength", 64, eConsoleVarFlag_Save );
}
#endif

#ifdef RED_LOGGING_ENABLED
namespace
{
	const Char* GetContentTxtForLog( EContentSource contentSource );
}
#endif

// FileExist() always returns false on consoles and don't want to nuke an existing cache.
#ifndef RED_PLATFORM_CONSOLE
void MakeEmptyCacheIfNeeded( const String& absoluteFilePath )
{
	if ( GFileManager->IsReadOnly() )
	{
		return;
	}

	if ( ! GFileManager->FileExist( absoluteFilePath ) )
	{
		LOG_CORE(TXT("MakeEmptyCacheIfNeeded: No cache at '%ls' found. Creating empty default"), absoluteFilePath.AsChar() );

		IFile* writer = GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath );
		if ( ! writer )
		{
			ERR_CORE(TXT("MakeEmptyCacheIfNeeded: Failed to create empty '%ls'!"), absoluteFilePath.AsChar() );
		}

		delete writer;
	}
}
#endif // !RED_PLATFORM_CONSOLE

namespace Config { namespace Debug {

	const Int32 AUTO_STALL = 0;
	const Int32 NEVER_STALL = 1;
	const Int32 ALWAYS_STALL = 2;

	TConfigVar< Int32 > cvStallForContent( "ContentManager/Debug", "StallForContent2", AUTO_STALL, eConsoleVarFlag_Developer );
	TConfigVar< Bool > cvAllContentAvailable( "ContentManager/Debug", "AllContentAvailable2", false, eConsoleVarFlag_Developer );
} }

#ifdef RED_LOGGING_ENABLED
namespace Config { namespace Debug {
	TConfigVar< Bool > cvLogContentQueries( "ContentManager/Debug", "LogContentQueries", false, eConsoleVarFlag_Developer );
	TConfigVar< Bool > cvDumpPlayGoProgress( "ContentManager/Debug", "DumpPlayGoProgress", false, eConsoleVarFlag_Developer );

void Update()
{
	if ( cvDumpPlayGoProgress.Get() )
	{
		GDebugDumpPlayGoProgress = true;
		cvDumpPlayGoProgress.Set( false );
	}
}

} } // namespace Config/Debug

#endif

namespace ContentHelpers
{
	const AnsiChar* ExtractFileName( const SContentFile& contentFile )
	{
		const AnsiChar* pch = contentFile.m_path.AsChar();
		const AnsiChar* lastSlash = Red::System::StringSearchLast( contentFile.m_path.AsChar(), '\\' );
		if ( lastSlash )
		{
			pch = lastSlash + 1;
		}

		return pch;
	}

	const Char* ExtractFileName( const String& path )
	{
		const Char* pch = path.AsChar();
		const Char* lastSlash = Red::System::StringSearchLast( path.AsChar(), '\\' );
		if ( lastSlash )
		{
			pch = lastSlash + 1;
		}

		return pch;
	}

	Bool MatchesWildcards( const SContentFile& contentFile, const StringAnsi fileFilterWildcards[], Uint32 numFileFilterWildcards )
	{
		// TBD: faster than creating a CFilePath and converting back and forth from ANSI to Unicode
		// Paths should already be conformed
		const AnsiChar* pch = ExtractFileName( contentFile );
		for ( Uint32 i = 0; i < numFileFilterWildcards; ++i )
		{
			const StringAnsi& wildcard = fileFilterWildcards[i];
			if ( StringHelpers::WildcardMatch( pch, wildcard.AsChar() ) )
			{
				return true;
			}
		}

		return false;
	}
}

//////////////////////////////////////////////////////////////////////////
// CContentManager
//////////////////////////////////////////////////////////////////////////
CContentManager::CContentManager()
	: m_contentSource( eContentSource_Installer )
	, m_contentStallType( eContentStall_None )
	, m_listenerAdded( false )
	, m_taintedFlags( 0 )
	, m_requiresUncookedScripts( false )
{
	m_manifestAsyncLoader = new CContentManifestAsyncLoader;

	// Can be overriden by streaming installers.
	m_resolvedLaunchZero = CNAME(content0);

	const CName BASE_CONTENT_NAMES[] = 
		{ CNAME( content0 ), CNAME( content1 ), CNAME( content2 ), CNAME( content3 ), CNAME( content4 ), CNAME( content5 ),
		  CNAME( content6 ), CNAME( content7 ), CNAME( content8 ), CNAME( content9 ), CNAME( content10 ), CNAME( content11 ),
		  CNAME( content12 ), CNAME( content13 ), CNAME( content14 ), CNAME( content15 ), CNAME( content16 ) };

	const Uint32 len = ARRAY_COUNT_U32(BASE_CONTENT_NAMES);
	m_baseContent.Reserve( len );
	for ( Uint32 i = 0; i < len; ++i )
	{
		m_baseContent.PushBack( BASE_CONTENT_NAMES[i] );
	}
}

CContentManager::~CContentManager()
{
	delete m_manifestAsyncLoader;
	for ( auto p : m_packageInstanceMap )
	{
		delete p.m_second;
	}
}

void CContentManager::DumpProgressToLog()
{
#ifdef RED_LOGGING_ENABLED
	GDebugDumpPlayGoProgress = true;
#endif
}

Bool CContentManager::Init()
{
	return true;
}

Bool CContentManager::ShouldForceContentAvailable() const
{
	return m_contentSource != eContentSource_Installer;
}

Bool CContentManager::ScanForContentFiles( EContentSource source, SContentPack& outContentPack )
{
#ifndef RED_PLATFORM_CONSOLE
	MakeEmptyCacheIfNeeded( GFileManager->GetDataDirectory() + TXT("collision.cache") );
	MakeEmptyCacheIfNeeded( GFileManager->GetDataDirectory() + TXT("staticshader.cache") );
	MakeEmptyCacheIfNeeded( GFileManager->GetDataDirectory() + TXT("shader.cache") );
#endif

	ScanDiskLanguageFiles( m_scannedTextLanguages, m_scannedSpeechLanguages );
	if ( m_scannedTextLanguages.Empty() || m_scannedSpeechLanguages.Empty() )
	{
		m_scannedDefaultLanguage = DEFAULT_LANG_TXT;
	}
	else
	{
		m_scannedDefaultLanguage = m_scannedSpeechLanguages.Exist( DEFAULT_LANG_TXT ) ? DEFAULT_LANG_TXT : m_scannedSpeechLanguages[0];
	}

	CTimeCounter timeCounter;
	const String rootDir = GFileManager->GetRootDirectory().ToLower();

	Bool retval = false;
	switch ( source )
	{
	case eContentSource_LooseFiles:
		retval = ScanForLooseFiles( outContentPack );
		break;
	case eContentSource_Cook:
		retval = ScanForCookFiles( outContentPack );
		break;
	case eContentSource_SplitCook:
		retval = ScanForSplitCookFiles( outContentPack );
		break;
	default:
		RED_FATAL( "Unhandled content source %u", source );
		break;
	}

	LOG_CORE(TXT("CContentManager: content scanned in %1.2f sec"), timeCounter.GetTimePeriod());

#ifdef RED_LOGGING_ENABLED
	Uint32 totalNumFiles = 0;
	for ( const SContentChunk& contentChunk : outContentPack.m_contentChunks )
	{
		const Uint32 numFiles = contentChunk.m_contentFiles.Size();
		totalNumFiles += contentChunk.m_contentFiles.Size();
		LOG_CORE(TXT("CContentManager: contentChunk '%ls' with %u files"), contentChunk.m_chunkLabel.AsChar(), numFiles);
	}
	LOG_CORE(TXT("CContentManager: Total %u files across %u manifests"), totalNumFiles, outContentPack.m_contentChunks.Size());
#endif

	return retval;
}

Bool CContentManager::IsContentAvailable( CName contentName ) const
{
	if ( ShouldForceContentAvailable() )
	{
		m_taintedFlags |= eContentTaintedFlag_AllContentAvailable;

		return true;
	}

	CName resolvedContentName = contentName;
	if ( contentName == CNAME(launch0) )
	{
		resolvedContentName = m_resolvedLaunchZero;
	}

	if ( Config::Debug::cvAllContentAvailable.Get() )
	{
		m_taintedFlags |= eContentTaintedFlag_AllContentAvailable;
#ifdef RED_LOGGING_ENABLED
		if ( Config::Debug::cvLogContentQueries.Get() )
		{
			LOG_CORE(TXT("CContentManager::IsContentAvailable %ls (resolved=%ls) cvAllContentAvailable: true"), contentName.AsChar(), resolvedContentName.AsChar());
		}
#endif
		return true;
	}

	Bool isChunkAttached = false;
	for ( const SContentChunkContext& chunkContext : m_attachedChunkContexts )
	{
		if ( chunkContext.m_packInstance->m_isHidden )
		{
			continue;
		}

		if ( chunkContext.m_contentName == resolvedContentName )
		{
			isChunkAttached = true;
			break;
		}
	}

#ifdef RED_LOGGING_ENABLED
	if ( Config::Debug::cvLogContentQueries.Get() )
	{
		LOG_CORE(TXT("CContentManager::IsContentAvailable %ls (resolved=%ls) chunkAttached=[%ls]"), contentName.AsChar(), resolvedContentName.AsChar(), ( isChunkAttached ? TXT("y") : TXT("n") ) );
	}
#endif

	if ( !isChunkAttached )
	{
		return false;
	}

	// FIXME - change the design. Giving streaming installers a chance to veto whether it's really available. This is a bit confusing because it's really checking
	// whether the languages for it are ready
	Bool isAvailable = true;
	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		IStreamingInstaller* streamingInstaller = nullptr;
		if ( contentInstaller->QueryStreamingInstaller( &streamingInstaller ) )
		{
			RED_FATAL_ASSERT( streamingInstaller, "No streaming installer!");
			Bool localIsAvailable = false;
			if ( streamingInstaller->IsContentAvailable( contentName, localIsAvailable ) && !localIsAvailable)
			{
				isAvailable = false;
				break;
			}
		}
	}

#ifdef RED_LOGGING_ENABLED
	if ( Config::Debug::cvLogContentQueries.Get() )
	{
		LOG_CORE(TXT("CContentManager::IsContentAvailable %ls (resolved=%ls) isAvailable=[%ls]"), contentName.AsChar(), resolvedContentName.AsChar(), ( isAvailable ? TXT("y") : TXT("n") ) );
	}
#endif

	return isAvailable;
}

Bool CContentManager::IsContentActivated( CName contentName ) const
{
	// NOTE: Don't create a "return true" debug variable unless locally testing something.
	// Relies on intial content manager activation and activation from the quest system so the quests are deterministic
	// no matter how much content is installed - it's gated by actually being activated.
	return m_activatedContent.HasTag( contentName );
}

EContentStallType CContentManager::GetStallForMoreContent() const
{
	switch ( Config::Debug::cvStallForContent.Get() )
	{
	case Config::Debug::NEVER_STALL:
		{
			m_taintedFlags |= eContentTaintedFlag_Skipped;
			return eContentStall_None;
		}
		break;
	case Config::Debug::ALWAYS_STALL:
		{
			m_taintedFlags |= eContentTaintedFlag_Skipped;
			return eContentStall_FinalContent;
		}
		break;
	default:
		break;
	}

	return m_contentStallType;
}

Uint32 CContentManager::GetPercentCompleted( CName contentName ) const
{
	if ( ShouldForceContentAvailable() )
	{
		m_taintedFlags |= eContentTaintedFlag_AllContentAvailable;

		return 100;
	}

	CName resolvedContentName = contentName;
	if ( contentName == CNAME(launch0) )
	{
		resolvedContentName = m_resolvedLaunchZero;
	}

	if ( Config::Debug::cvAllContentAvailable.Get() )
	{
		m_taintedFlags |= eContentTaintedFlag_AllContentAvailable;

#ifdef RED_LOGGING_ENABLED
		if ( Config::Debug::cvLogContentQueries.Get() )
		{
			LOG_CORE(TXT("CContentManager::GetPercentCompleted %ls (resolved=%ls) cvAllContentAvailable: true"), contentName.AsChar(), resolvedContentName.AsChar());
		}
#endif
		return 100;
	}

	Uint32 percentCompleted = 0;

	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		IStreamingInstaller* streamingInstaller = nullptr;
		if ( contentInstaller->QueryStreamingInstaller( &streamingInstaller ) )
		{
			RED_FATAL_ASSERT( streamingInstaller, "No streaming installer!");
			if ( streamingInstaller->GetPercentCompleted( resolvedContentName, percentCompleted ) )
			{
				break;
			}
		}
	}

#ifdef RED_LOGGING_ENABLED
	if ( Config::Debug::cvLogContentQueries.Get() )
	{
		LOG_CORE(TXT("CContentManager::GetPercentCompleted %ls (resolved=%ls) [%u]"), contentName.AsChar(), resolvedContentName.AsChar(), percentCompleted);
	}
#endif

	// TBD: check attached chunks in content mgr and lie with 99% if not yet attached, but the content installed.
	// TBD: "hidden" chunks for DLC returning 0%

	return percentCompleted;
}

void CContentManager::EnumScriptFiles( TDynArray<String>& outScriptFiles ) const
{
	if ( !GScriptingSystem )
	{
		return;
	}

	const String& baseScriptDirPath = GScriptingSystem->GetRootPath();

	if ( GIsCooker || GIsEditor )
	{
		GFileManager->FindFiles( baseScriptDirPath, TXT( "*.ws" ), outScriptFiles, true );
		for ( String& filePath : outScriptFiles )
		{
			CUpperToLower conv( filePath.TypedData(), filePath.Size() );
		}
		Sort( outScriptFiles.Begin(), outScriptFiles.End() );
		return;
	}

	THashMap< String, String > mergedScriptFiles;

	// Get base files
	{
		TDynArray< String > baseScriptFiles;
		GFileManager->FindFiles( baseScriptDirPath, TXT( "*.ws" ), baseScriptFiles, true );
		for ( String& filePath : baseScriptFiles )
		{
			CUpperToLower conv( filePath.TypedData(), filePath.Size() );
			String scriptFile(filePath.AsChar() + baseScriptDirPath.GetLength());
			mergedScriptFiles[scriptFile] = Move(filePath);
		}
	}

	TDynArray< const SContentPackInstance* > modPackages;
	modPackages.Reserve( m_packageInstanceMap.Size() );
	for ( const auto& it : m_packageInstanceMap )
	{
		const SContentPackInstance* const pkg = it.m_second;
		if ( pkg->m_isMod && pkg->m_packageStatus == SContentPackInstance::ePackageStatus_Ready )
		{
			RED_FATAL_ASSERT( pkg->m_packageID > BASE_RUNTIME_PACKAGE_ID, "Invalid packageID %u for a mod!", pkg->m_packageID );
			modPackages.PushBack( pkg);
		}
	}
	Sort(	modPackages.Begin(),
			modPackages.End(),
			[](const SContentPackInstance* a, const SContentPackInstance* b) { return a->m_packageID < b->m_packageID; }
	);

	for ( const SContentPackInstance* pkg : modPackages )
	{
		const String modScriptsDirPath = pkg->m_mountPath + TXT("scripts\\");
		TDynArray< String > modScriptFiles;
		GFileManager->FindFiles( modScriptsDirPath, TXT( "*.ws" ), modScriptFiles, true );
		for ( String& filePath : modScriptFiles )
		{
			CUpperToLower conv( filePath.TypedData(), filePath.Size() );
			String scriptFile(filePath.AsChar() + modScriptsDirPath.GetLength());
			mergedScriptFiles[scriptFile] = Move(filePath);
		}
	}

	// Order by base script name so we try to feed the compiler the scripts in the same order
	// regardless of which mod they're coming from. For CRC checks and if there are compiler issues
	// then they're more reproducible.
	TDynArray< String > keys;
	mergedScriptFiles.GetKeys( keys );
	Sort( keys.Begin(), keys.End() );
	outScriptFiles.Reserve( keys.Size() );
	for ( const String& key : keys )
	{
		outScriptFiles.PushBack( mergedScriptFiles[key] );
	}
}

void CContentManager::EnumScriptDirectories( THashMap< String, String >& outScriptDirectories ) const
{
	if ( !GScriptingSystem )
	{
		return;
	}

	const String& baseScriptDirPath = GScriptingSystem->GetRootPath();
	outScriptDirectories[ TXT( "content0" ) ] = baseScriptDirPath;

	if ( GIsCooker || GIsEditor )
	{
		return;
	}

	for ( const auto& it : m_packageInstanceMap )
	{
		const SContentPackInstance* const pkg = it.m_second;
		if ( pkg->m_isMod && pkg->m_packageStatus == SContentPackInstance::ePackageStatus_Ready )
		{
			RED_FATAL_ASSERT( pkg->m_packageID > BASE_RUNTIME_PACKAGE_ID, "Invalid packageID %u for a mod!", pkg->m_packageID );

			outScriptDirectories[ pkg->m_contentPack.m_id.AsChar() ] = pkg->m_mountPath + TXT("scripts\\");
		}
	}
}

void CContentManager::Update()
{
	TDynArray< SContentPackageEvent > contentPackageEvents;

	m_contentStallType = eContentStall_None;

	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		IStreamingInstaller* streamingInstaller = nullptr;
		if ( contentInstaller->QueryStreamingInstaller( &streamingInstaller ) )
		{
			RED_FATAL_ASSERT( streamingInstaller, "No streaming installer!");
			m_contentStallType = Max< EContentStallType >( m_contentStallType, streamingInstaller->GetStallForMoreContent() );
		}

		contentInstaller->Update( contentPackageEvents );
	}

	const Uint32 oldAttachedContentSize = m_attachedChunkContexts.Size();
	const Uint32 oldAttachedPackagesSize = m_attachedPackageIDs.Size();
	ProcessPackageEvents( contentPackageEvents );

	m_manifestAsyncLoader->Update();

	TDynArray< SContentPackInstance* > finishedPackages;
	m_manifestAsyncLoader->FlushFinishedPackages( finishedPackages );

	ProcessFinishedPackages( finishedPackages );
	ProcessPendingAttachedContent();

	const Bool notifyListeners = ( m_listenerAdded || m_attachedChunkContexts.Size() > oldAttachedContentSize || m_attachedPackageIDs.Size() > oldAttachedPackagesSize );

	m_listenerAdded = false;

// #ifdef RED_LOGGING_ENABLED
// 	CTimeCounter timer;
// #endif

	if ( notifyListeners )
	{
		NotifyListeners();
	}

	// Update again after notifying listeners, to kick off more loads
	m_manifestAsyncLoader->Update();

//	LOG_CORE(TXT("CContentManager: Listener '%ls' notifed of %u content manifest(s) in %1.2f sec"), listener->GetName(), contentManifests.Size(), timer.GetTimePeriod());

#ifdef RED_LOGGING_ENABLED
	Config::Debug::Update();
#endif
}

Bool CContentManager::IsReady( EContentMask contentMask /*= eContentMask_Default*/ ) const
{
	// Checking for content0 regardless of language activation and before asking any content installer, since we want the manifest loaded
	auto findIt = ::FindIf( m_attachedChunkContexts.Begin(), m_attachedChunkContexts.End(), [](const SContentChunkContext& context){
								return context.m_contentName == CNAME(content0);
	});

	if ( findIt == m_attachedChunkContexts.End() )
	{
		return false;
	}

	if ( contentMask == eContentMask_BootUp )
	{
		return true;
	}

	// TBD: Potentially cache during Update() call instead

	if ( m_manifestAsyncLoader->IsPending() )
	{
		return false;
	}

	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		if ( !contentInstaller->IsReady() )
		{
			return false;
		}
	}

	return true;
}

void CContentManager::OnSuspend()
{
	for ( IContentInstaller* installer : m_registeredContentInstallers )
	{
		installer->OnSuspend();
	}
}

void CContentManager::OnResume()
{
	for ( IContentInstaller* installer : m_registeredContentInstallers )
	{
		installer->OnResume();
	}
}

void CContentManager::OnEnterConstrain()
{
	for ( IContentInstaller* installer : m_registeredContentInstallers )
	{
		installer->OnEnterConstrain();
	}
}

void CContentManager::OnExitConstrain()
{
	for ( IContentInstaller* installer : m_registeredContentInstallers )
	{
		installer->OnExitConstrain();
	}
}

static void GetFilteredContentFiles( const SContentChunk& chunk, const TDynArray< StringAnsi >& fileFilterWildcards, TDynArray< const SContentFile* >& outFilteredContentFiles )
{
	outFilteredContentFiles.ClearFast();

	if ( fileFilterWildcards.Empty() )
	{
		outFilteredContentFiles.Reserve( chunk.m_contentFiles.Size() );
		for ( const SContentFile& contentFile : chunk.m_contentFiles )
		{
			outFilteredContentFiles.PushBack( &contentFile );
			return;
		}
	}
	
	for ( const SContentFile& contentFile : chunk.m_contentFiles )
	{
		if ( ContentHelpers::MatchesWildcards(contentFile, fileFilterWildcards.TypedData(), fileFilterWildcards.Size() ) )
		{
			outFilteredContentFiles.PushBack( &contentFile );
		}
	}
}

void CContentManager::NotifyListeners()
{
	// TBD: Notify in chunk order (vs. for each listener: each chunk ) to try to avoid I/O seeks, and esp. expensive layer switches on Blu-ray
	for ( Int32 i = 0; i < m_attachedChunkContexts.SizeInt(); ++i )
	{
		const SContentChunkContext& chunkContext = m_attachedChunkContexts[i];
		const String& mountPath = chunkContext.m_packInstance->m_mountPath;
		const RuntimePackageID packageID = chunkContext.m_packInstance->m_packageID;

		TDynArray< const SContentFile* > filteredContentFiles;

		for ( SContentListenerContext& listenerContext : m_contentListeners )
		{
			if ( listenerContext.m_maxNotifiedContentChunkIndex < i )
			{
				listenerContext.m_maxNotifiedContentChunkIndex = i;

				RED_FATAL_ASSERT( chunkContext.m_chunkPtr, "No chunk pointer!");
				const SContentChunk& chunk = *chunkContext.m_chunkPtr;
				GetFilteredContentFiles( chunk, listenerContext.m_fileFilterWildcards, filteredContentFiles );
				//if ( filteredContentFiles.Size() > 0 ) // TBD: don't bother notifying of zero files and potentially have something mess up
				{
					const SContentInfo contentInfo( packageID, chunk.m_chunkLabel, mountPath, filteredContentFiles, chunkContext.m_packInstance->m_isMod );
					listenerContext.m_contentListener->OnContentAvailable( contentInfo );
				}
			}
		}
	}

	// Notify of any package after notifying chunk content listeners
	for ( Int32 i = 0; i < m_attachedPackageIDs.SizeInt(); ++i )
	{
		const RuntimePackageID packageID = m_attachedPackageIDs[ i ];

		for ( SContentListenerContext& listenerContext : m_contentListeners )
		{
			if ( listenerContext.m_maxNotifiedPackageIndex < i )
			{
				listenerContext.m_maxNotifiedPackageIndex = i;
				listenerContext.m_contentListener->OnPackageAvailable( packageID );
			}
		}
	}
}

void CContentManager::ProcessPackageEvents( const TDynArray< SContentPackageEvent >& contentPackageEvents )
{
	for ( const SContentPackageEvent& event : contentPackageEvents )
	{
		switch ( event.m_eventType )
		{
		case eContentPackageEventType_NewPackageMounted:
			OnPackageMounted( event.m_packageID, event.m_mountPath, event.m_mountFlags );
			break;
		case eContentPackageEventType_NewChunkInstalled:
			OnPackageChunkInstalled( event.m_packageID, event.AsChunkName() );
			break;
		case eContentPackageEventType_LicenseChanged:
			OnPackageLicenseChanged( event.m_packageID, event.m_isLicensed );
			break;
		default:
			RED_FATAL( "Unhandled content package event %u", event.m_eventType );
			break;
		}
	}
}

void CContentManager::ProcessFinishedPackages( const TDynArray< SContentPackInstance* >& finishedPackages )
{
	for ( SContentPackInstance* pkg : finishedPackages )
	{
		if ( pkg->m_packageStatus != SContentPackInstance::ePackageStatus_Ready )
		{
			WARN_CORE(TXT("CContentManager::ProcessFinishedPackages Skipping failed packageID=%u, packageStatus=%u"), pkg->m_packageID, pkg->m_packageStatus );
			continue;
		}

		// Just stop processing the package - treat as if it failed. Licensing changes can still occur, but we won't send the package installed events
		// or send any chunk events
		const Uint32 minVersion = pkg->m_contentPack.m_minVersion;
		if ( CONTENT_VERSION < minVersion )
		{
			WARN_CORE(TXT("CContentManager::ProcessFinishedPackages Skipping failed packageID=%u, packageStatus=%u. CONTENT_VERSION (%u) < package minVersion (%u)"),
				pkg->m_packageID, pkg->m_packageStatus, CONTENT_VERSION, minVersion );
			pkg->m_packageStatus = SContentPackInstance::ePackageStatus_LoadFailed;
			continue;
		}

		for ( const SContentChunk& chunk : pkg->m_contentPack.m_contentChunks )
		{
			const CName contentName = chunk.m_chunkLabel;
			if ( ! contentName )
			{
				ERR_CORE(TXT("Failed to map contentName '%ls', packageID=%u"), contentName.AsChar(), pkg->m_packageID );
				continue;
			}
		}

		m_attachedPackageIDs.PushBack( pkg->m_packageID );

		if ( pkg->m_autoAttachChunks )
		{
			for ( const SContentChunk& chunk : pkg->m_contentPack.m_contentChunks )
			{
				m_pendingAttachedContent.PushBack( TPair< RuntimePackageID, CName >( pkg->m_packageID, chunk.m_chunkLabel ) );
			}
		}
	}
}

void CContentManager::ProcessPendingAttachedContent()
{
	// Attach in FIFO order
	TDynArray< SContentChunkContext > contentToAttach;
	for ( Int32 j = m_pendingAttachedContent.SizeInt() - 1; j >= 0; --j )
	{
		const RuntimePackageID packageID = m_pendingAttachedContent[ j ].m_first;
		const CName contentName = m_pendingAttachedContent[ j ].m_second;
		
		auto findIt = m_packageInstanceMap.Find( packageID );
		if ( findIt == m_packageInstanceMap.End() )
		{
			ERR_CORE(TXT("ProcessPendingAttachedContent: packageID=%u has no content pack instance!"), packageID );
			m_pendingAttachedContent.RemoveAt( j );
			continue;
		}

		SContentPackInstance* packInstance = findIt->m_second;

		if ( packInstance->m_packageStatus == SContentPackInstance::ePackageStatus_Pending )
		{
			continue;
		}
		
		if ( packInstance->m_packageStatus == SContentPackInstance::ePackageStatus_Ready )
		{
			const SContentChunk* chunkPtr = nullptr;
			const TDynArray< SContentChunk>& chunks = packInstance->m_contentPack.m_contentChunks;
			for ( const SContentChunk& chunk : chunks )
			{
				if ( chunk.m_chunkLabel == contentName )
				{
					chunkPtr = &chunk;
					break;
				}
			}

			if ( !chunkPtr )
			{
				ERR_CORE(TXT("ProcessPendingAttachedContent: can't find chunkPtr! packageID=%u, chunkName=%ls"), packageID, contentName.AsChar());
				continue;
			}

			const SContentChunkContext context( packInstance, chunkPtr );
			contentToAttach.PushBack( context );
		}
		else if ( packInstance->m_packageStatus == SContentPackInstance::ePackageStatus_LoadFailed )
		{
			ERR_CORE(TXT("ProcessPendingAttachedContent: Can't attach since package load failed for packageID=%u, chunkName=%ls"), packageID, contentName.AsChar());
		}

		m_pendingAttachedContent.RemoveAt( j );
	}

	for ( Int32 j = contentToAttach.SizeInt() - 1; j >= 0; --j )
	{
		const SContentChunkContext& context = contentToAttach[j];
		if ( !m_attachedChunkContexts.PushBackUnique( context ) )
		{
			ERR_CORE(TXT("Attached context not unique! packageID=%u, contentName=%ls is not unique"), context.m_packInstance->m_packageID, context.m_contentName.AsChar() );
		}
	}
}

void CContentManager::OnPackageMounted( RuntimePackageID packageID, const String& mountPath, Uint32 mountFlags )
{
	const Bool isAutoAttachChunks = ( mountFlags & eContentPackageMountFlags_AutoAttachChunks ) != 0;
	const Bool isHidden = ( mountFlags & eContentPackageMountFlags_IsHidden ) != 0;

	LOG_CORE(TXT("CContentManager[OnPackageMounted] packageID=%u, mountPath='%ls', isAutoAttachChunks=%d, isHidden=%d"), 
		packageID, mountPath.AsChar(), (isAutoAttachChunks ? 1 : 0), (isHidden? 1 : 0 ) );
	SContentPackInstance* packInstance = nullptr;
	auto findIt = m_packageInstanceMap.Find( packageID );
	if ( findIt == m_packageInstanceMap.End() )
	{
		String normalizedPath( mountPath );
		normalizedPath.ReplaceAll(TXT('/'), TXT('\\'));
		if ( !normalizedPath.EndsWith(TXT("\\")) )
		{
			normalizedPath += TXT('\\');
		}

		// If not actually licensed anymore, should be corrected by a later event
		packInstance = new SContentPackInstance( packageID, Move( normalizedPath ) );
		packInstance->SetAutoAttachChunks( isAutoAttachChunks );
		packInstance->SetHidden( isHidden );
		m_packageInstanceMap.Insert( packageID, packInstance );
		m_manifestAsyncLoader->BeginLoad( packInstance );
	}
	else
	{
		ERR_CORE(TXT("Multiple mount events for packageID=%u, mountPath=%hs"), packageID, mountPath.AsChar());
	}
}

void CContentManager::OnPackageLicenseChanged( RuntimePackageID packageID, Bool isLicensed )
{
	LOG_CORE(TXT("CContentManager[OnPackageLicenseChanged] packageID=%u, isLicensed=d"), packageID, (isLicensed ? 1 : 0) );

	auto findIt = m_packageInstanceMap.Find( packageID );
	if ( findIt == m_packageInstanceMap.End() )
	{
		ERR_CORE(TXT("CContentManager::OnPackageLicenseChanged: No package instance! packageID=%u, isLicensed=%d"), packageID, (isLicensed ? 1 : 0));
		return;
	}

	if ( packageID == BASE_RUNTIME_PACKAGE_ID )
	{
		WARN_CORE(TXT("Changing base packageID=%u, isLicensed=%d"), packageID, (isLicensed ? 1 : 0 ));
	}

	SContentPackInstance* packInstance = findIt->m_second;
	packInstance->m_isLicensed = isLicensed;
}

void CContentManager::OnPackageChunkInstalled( RuntimePackageID packageID, CName chunkName )
{
	LOG_CORE(TXT("CContentManager[OnPackageChunkInstalled] packageID=%u, chunkName=%ls"), packageID, chunkName.AsChar());
	m_pendingAttachedContent.PushBack( TPair< RuntimePackageID, CName >( packageID, chunkName ) );
}

Bool CContentManager::ActivateContent( CName content )
{
	if ( !content || !IsContentAvailable( content ) )
	{
		ERR_CORE(TXT("CContentManager::ActivateContent: failed to activate content '%ls'"), content.AsChar());
		return false;
	}

	LOG_CORE(TXT("CContentManager::ActivateContent: activated '%ls'"), content.AsChar());

	m_activatedContent.AddTag( content );

	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		IStreamingInstaller* streamingInstaller = nullptr;
		if ( contentInstaller->QueryStreamingInstaller( &streamingInstaller ) )
		{
			RED_FATAL_ASSERT( streamingInstaller, "No streaming installer!");
			streamingInstaller->PrefetchContent( content );
		}
	}

	return true;
}

void CContentManager::ActivateAllContentForDebugQuests()
{
	LOG_CORE(TXT(">>> CContentManager::ActivateAllContentForDebugQuests() <<<"));
	m_taintedFlags |= eContentTaintedFlag_AllContentActivated;

	const Uint32 contentNumberMax = 12;

	for ( Uint32 i = 0; i <= contentNumberMax; ++i )
	{
		CName contentName( String::Printf(TXT("content%u"), i) );
		m_activatedContent.AddTag( contentName );
	}
}

void CContentManager::RefreshActivatedContent()
{
	// This doesn't clear activated content so we can use it during CreateSession() which can be transitioning worlds
	// so we definitely DO NOT want to reset activated content. However, we want to reset the prefetch so it's for the current language etc
	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		IStreamingInstaller* streamingInstaller = nullptr;
		if ( contentInstaller->QueryStreamingInstaller( &streamingInstaller ) )
		{
			RED_FATAL_ASSERT( streamingInstaller, "No streaming installer!");
			streamingInstaller->ResetPrefetchContent();

			for ( CName activatedContent : m_activatedContent.GetTags() )
			{
				streamingInstaller->PrefetchContent( activatedContent );
			}
		}
	}
}

void CContentManager::ResetActivatedContent()
{
	m_activatedContent.Clear();
	m_taintedFlags &= ~eContentTaintedFlagGroup_Activated;

	RefreshActivatedContent();
}

void CContentManager::RegisterContentListener( IContentListener* listener, const AnsiChar* fileFilterWildcard /*= ""*/ )
{
	RED_FATAL_ASSERT( listener, "Null listener" );
	TDynArray< StringAnsi > filters;
	filters.PushBack( fileFilterWildcard );
	if ( m_contentListeners.PushBackUnique( SContentListenerContext( listener, Move( filters ) ) ) )
	{
		m_listenerAdded = true;
	}
}

void CContentManager::RegisterContentListener( IContentListener* listener, const AnsiChar* fileFilterWildcards[], Uint32 numFileFilterWildcards )
{
	RED_FATAL_ASSERT( listener, "Null listener" );
	TDynArray< StringAnsi > filters;
	filters.Reserve( numFileFilterWildcards );
	for ( Uint32 i = 0; i < numFileFilterWildcards; ++i )
	{
		filters.PushBack( fileFilterWildcards[i] );
	}

	if ( m_contentListeners.PushBackUnique( SContentListenerContext( listener, Move( filters ) ) ) )
	{
		m_listenerAdded = true;
	}
}

void CContentManager::UnregisterContentListener( IContentListener* listener )
{
	RED_FATAL_ASSERT( listener, "Null listener" );
	TDynArray< StringAnsi> filters;
	m_contentListeners.Remove( SContentListenerContext( listener, filters ) );
}

void CContentManager::RegisterContentInstaller( IContentInstaller* contentInstaller )
{
	RED_FATAL_ASSERT( contentInstaller, "Null installer" );
	if ( m_registeredContentInstallers.PushBackUnique( contentInstaller ) )
	{
		IStreamingInstaller* streamingInstaller = nullptr;
		if ( contentInstaller->QueryStreamingInstaller( &streamingInstaller) )
		{
			RED_FATAL_ASSERT(streamingInstaller, "No streaming installer!");
			CName newResolvedLaunchZero;
			if ( streamingInstaller->GetResolvedLaunchZero( newResolvedLaunchZero ) )
			{
				LOG_CORE(TXT("Streaming installer updated launch0 from %ls to %ls"), m_resolvedLaunchZero.AsChar(), newResolvedLaunchZero.AsChar());
				m_resolvedLaunchZero = newResolvedLaunchZero;
			}
			else
			{
				LOG_CORE(TXT("Streaming installer FAILED to update launch0 from %ls"), m_resolvedLaunchZero.AsChar());
			}
		}
	}
}

Bool CContentManager::GetValidDefaultGameLanguage( String& outTextLanguage, String& outSpeechLanguage ) const
{
	// TBD FIXME: Merging the default language. Need installer priority or a "preferred" language to begin with.
	// Also DLC installers need to have the language available before user setting try switching to it
	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		ILanguageProvider* languageProvider = nullptr;
		if ( contentInstaller->QueryLanguageProvider( &languageProvider ) )
		{
			RED_FATAL_ASSERT(languageProvider, "No language provider!");
			if ( languageProvider->GetValidDefaultGameLanguage( outTextLanguage, outSpeechLanguage ) )
			{
				return true;
			}
		}
	}

	// See if a system language has been specified for text, and if it's valid
	if( !m_systemTextLanguage.Empty() && m_scannedTextLanguages.Exist( m_systemTextLanguage ) )
	{
		outTextLanguage = m_systemTextLanguage;
	}

	// Fall back to valid default
	else if( !m_scannedDefaultLanguage.Empty() )
	{
		outTextLanguage = m_scannedDefaultLanguage;
	}

	// Same again for speech
	if( !m_systemSpeechLanguage.Empty() && m_scannedSpeechLanguages.Exist( m_systemSpeechLanguage ) )
	{
		outSpeechLanguage = m_systemSpeechLanguage;
	}
	else if( !m_scannedDefaultLanguage.Empty() )
	{
		outSpeechLanguage = m_scannedDefaultLanguage;
	}

	// Success if both are specified
	return !outTextLanguage.Empty() && !outSpeechLanguage.Empty();
}

void CContentManager::SetSystemLanguages( const String& text, const String& speech )
{
	m_systemTextLanguage = text;
	m_systemSpeechLanguage = speech;
}

Bool CContentManager::GetSupportedLanguages( TDynArray< String >& outTextLangauges, TDynArray< String >& outSpeechLanguages, String& outDefaultLanguage ) const
{
	TDynArray< String > textLanguages;
	TDynArray< String > speechLangauges;

	if ( !m_scannedTextLanguages.Empty() )
	{
		outTextLangauges.PushBackUnique( m_scannedTextLanguages );
	}

	if ( !m_scannedSpeechLanguages.Empty() )
	{
		outSpeechLanguages.PushBackUnique( m_scannedSpeechLanguages );
	}

	if ( !m_scannedDefaultLanguage.Empty() )
	{
		outDefaultLanguage = m_scannedDefaultLanguage;
	}

	// TBD FIXME: Merging the default language. Need installer priority or a "preferred" language to begin with.
	// Also DLC installers need to have the language available before user setting try switching to it
	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		textLanguages.ClearFast();
		speechLangauges.ClearFast();
		ILanguageProvider* languageProvider = nullptr;
		if ( contentInstaller->QueryLanguageProvider( &languageProvider ) )
		{
			RED_FATAL_ASSERT(languageProvider, "No language provider!");
			if ( languageProvider->GetSupportedLanguages( textLanguages, speechLangauges, outDefaultLanguage ) )
			{
				outTextLangauges.PushBackUnique( textLanguages );
				outSpeechLanguages.PushBackUnique( speechLangauges );
			}
		}
	}

	if ( outSpeechLanguages.Empty() )
	{
		outSpeechLanguages.PushBack(DEFAULT_LANG_TXT);
	}
	if ( outTextLangauges.Empty() )
	{
		outTextLangauges.PushBack(DEFAULT_LANG_TXT);
	}
	if ( outDefaultLanguage.Empty() )
	{
		outDefaultLanguage = DEFAULT_LANG_TXT;
	}

	// TBD: maybe return false if not everything set
	return true;
}

void CContentManager::GetLicensedPackages( TDynArray< RuntimePackageID >& outPackagesIDs ) const
{
	for ( auto it = m_packageInstanceMap.Begin(); it != m_packageInstanceMap.End(); ++it )
	{
		SContentPackInstance* packInstance = it->m_second;
		if ( packInstance->m_isLicensed && packInstance->m_packageStatus == SContentPackInstance::ePackageStatus_Ready )
		{
			outPackagesIDs.PushBack( packInstance->m_packageID );
		}
	}
}

void CContentManager::SetSpeechInstallLanguage( const String& speechLang )
{
	for ( IContentInstaller* contentInstaller : m_registeredContentInstallers )
	{
		ILanguageProvider* languageProvider = nullptr;
		if ( contentInstaller->QueryLanguageProvider( &languageProvider ) )
		{
			RED_FATAL_ASSERT(languageProvider, "No language provider!");
			languageProvider->SetSpeechInstallLanguage( speechLang );
		}
	}
}

void CContentManager::ScanDiskLanguageFiles( TDynArray< String >& outTextLanguages, TDynArray< String >& outSpeechLanguages ) const
{
	outTextLanguages.Clear();
	outSpeechLanguages.Clear();

	const String& dataDir = GFileManager->GetDataDirectory();

	TDynArray< String > speechFiles;
	GFileManager->FindFiles( dataDir, TXT("*.w3speech"), speechFiles, false );

	TDynArray< String > stringsFiles;
	GFileManager->FindFiles( dataDir, TXT("*.w3strings"), stringsFiles, false );

	for ( const String& file : speechFiles )
	{
		String lang;
		if ( ParseSpeechesLanguage(file, lang) )
		{
			LOG_CORE(TXT("Found speeches lang '%ls' from file '%ls'"), lang.AsChar(), file.AsChar() );
			outSpeechLanguages.PushBackUnique( lang );
		}
	}

	for ( const String& file : stringsFiles )
	{
		String lang;
		if ( ParseStringsLanguage(file, lang) )
		{
			LOG_CORE(TXT("Found strings lang '%ls' from file '%ls'"), lang.AsChar(), file.AsChar() );
			outTextLanguages.PushBackUnique( lang );
		}
	}

	if ( outTextLanguages.Empty() || outSpeechLanguages.Empty() )
	{
		WARN_CORE(TXT("CContentManager: no cooked language files found. Defaulting to %ls"), DEFAULT_LANG_TXT );
		outTextLanguages.PushBackUnique( DEFAULT_LANG_TXT );
		outSpeechLanguages.PushBackUnique( DEFAULT_LANG_TXT );
	}
}

Bool CContentManager::ParseStringsLanguage( const String& path, String& outLang ) const
{
	CFilePath filePath( path );
	const String fileName = filePath.GetFileNameWithExt();

	const String lang = fileName.StringBefore( STRINGS_FILE_PATH_POSTFIX ).ToUpper();
	if ( lang.Empty() )
	{
		ERR_CORE(TXT("fileName '%ls' did not map to any language!"), lang.AsChar() );
		return false;
	}

	outLang = Move( lang );

	return true;
}

Bool CContentManager::ParseSpeechesLanguage( const String& path, String& outLang ) const
{
	CFilePath filePath( path );
	const String fileName = filePath.GetFileNameWithExt();

	if ( !fileName.EndsWith( CURRENT_SPEECH_FILE_PATH_POSTFIX) )
	{
		WARN_CORE(TXT("speech file '%ls' not for current platform"), path.AsChar() );
		return false;
	}

	const String lang = fileName.StringBefore( CURRENT_SPEECH_FILE_PATH_POSTFIX ).ToUpper();
	if ( lang.Empty() )
	{
		ERR_CORE(TXT("fileName '%ls' did not map to any language!"), lang.AsChar() );
		return false;
	}

	outLang = Move( lang );

	return true;
}

Bool CContentManager::ScanForLooseFiles( SContentPack& outContentPack ) const
{
	LOG_CORE(TXT("CContentManager::ScanForLooseFiles..."));
	// Perform a non-recursive directory scan to find cache files
	TDynArray< String > absolutePaths;
	GFileManager->FindFiles( GFileManager->GetDataDirectory(), TXT("*.*"), absolutePaths, false );
	for ( String& path : absolutePaths )
	{
		path.ReplaceAll( TXT("/"), TXT("\\") );
	}

	const String rootDir = GFileManager->GetRootDirectory().ToLower();
	SContentChunk newContentChunk;
	Red::System::DateTime timestamp;
	if ( !Helper::CreateContentChunk( newContentChunk, timestamp, CNAME(content0), absolutePaths, rootDir, false, true ) )
	{
		return false;
	}

	SContentPack newContentPack;
	newContentPack.m_id = CNAME(content);
	newContentPack.m_contentChunks.PushBack( Move( newContentChunk ) );

	outContentPack = Move( newContentPack );

	return true;
}

Bool CContentManager::ScanForCookFiles( SContentPack& outContentPack ) const
{
	LOG_CORE(TXT("CContentManager::ScanForCookFiles..."));

	TDynArray< String > absolutePaths;

	// Perform a non-recursive directory scan to find cache files
	GFileManager->FindFiles( GFileManager->GetDataDirectory(), TXT("*.*"), absolutePaths, false );

	// Go deep and find bundles and their other files
	GFileManager->FindFiles( GFileManager->GetBundleDirectory(), TXT("*.*"), absolutePaths, true );

	for ( String& path : absolutePaths )
	{
		path.ReplaceAll( TXT("/"), TXT("\\") );
	}

	const String rootDir = GFileManager->GetRootDirectory().ToLower();
	SContentChunk newContentChunk;
	Red::System::DateTime timestamp;
	if ( !Helper::CreateContentChunk( newContentChunk, timestamp, CNAME(content0), absolutePaths, rootDir, false, true ) )
	{
		return false;
	}

	SContentPack newContentPack;
	newContentPack.m_id = CNAME(content);
	newContentPack.m_contentChunks.PushBack( Move( newContentChunk ) );

	outContentPack = Move( newContentPack );

	return true;
}

Bool CContentManager::ScanForSplitCookFiles( SContentPack& outContentPack ) const
{
	const String contentDir = GFileManager->GetRootDirectory() + CONTENT_SCAN_DIR;

	LOG_CORE(TXT("CContentManager::ScanForSplitCookFiles under '%ls'..."), contentDir.AsChar());

	// Get all the content

	TDynArray< String > splitContentDirs;
	GFileManager->FindDirectories( contentDir, splitContentDirs );
	
	// Processed specially. Here under content because of how bundles work, and need to share a common root path
	// Remove anything that shouldn't be here. Don't just remove for "mod*" because who knows how users will actually
	// name them
	for ( Int32 j = splitContentDirs.SizeInt()-1; j>=0; --j )
	{
		const String toLower = splitContentDirs[j].ToLower();
		if ( !toLower.BeginsWith(TXT("content")) && !toLower.BeginsWith(TXT("patch")) )
		{
			splitContentDirs.RemoveAt(j);
		}
	}

	struct Chunk
	{
		static Bool Split( const String& str, String& outPrefix, Uint32& outNumber )
		{
			const Char* ch = str.AsChar();
			for(; *ch; ++ch )
			{
				if ( *ch >= '0' && *ch <= '9' )
					break;
			}

			if ( !*ch )
			{
				return false; // no number
			}

			const Char* const prefixEndPlusOne = ch;

			Uint32 num = 0;
			if ( !GParseInteger( ch, num ) )
			{
				return false;
			}

			const size_t prefixLen = prefixEndPlusOne - str.AsChar();
			outPrefix.Set( str.AsChar(), prefixLen );
			outNumber = num;
			return true;
		}
	};

	// Sort content directories into numerical order so mounting is always in the same order
	Sort( splitContentDirs.Begin(), splitContentDirs.End(), 
		[](const String& a, const String& b )
	{
		Uint32 valA = 0;
		Uint32 valB = 0;

		String prefixA;
		String prefixB;

		if ( !Chunk::Split(a, prefixA, valA) )
		{
			RED_FATAL( "Invalid content dir name '%ls' has no content number", a.AsChar() );
		}
		if ( !Chunk::Split(b, prefixB, valB) )
		{
			RED_FATAL( "Invalid content dir name '%ls' has no content number", b.AsChar() );
		}

		if ( prefixA == prefixB )
		{
			return valA < valB;
		}
		else
		{
			return !(prefixA < prefixB); // lame hack, make patch come first
		}
	});

	SContentPack newContentPack;
	newContentPack.m_id = CNAME(content);

	for ( const String& contentName : splitContentDirs )
	{
		LOG_CORE(TXT("CContentManager::ScanForSplitCookFiles looking in '%ls'..."), contentName.AsChar());

		SContentChunk newContentChunk;

		const String scanDir = String::Printf(TXT("%ls%ls\\"), contentDir.AsChar(), contentName.AsChar() );
		TDynArray< String > absolutePaths;
		GFileManager->FindFiles( scanDir, TXT("*.*"), absolutePaths, true );

		// For some extra determinism across platforms
		Sort( absolutePaths.Begin(), absolutePaths.End() );

		for ( String& path : absolutePaths )
		{
			path.ReplaceAll( TXT("/"), TXT("\\") );
		}

		const String rootDir = GFileManager->GetRootDirectory().ToLower() + TXT("content\\");
		Red::System::DateTime timestamp;
		if ( !Helper::CreateContentChunk( newContentChunk, timestamp, CName( contentName.ToLower() ), absolutePaths, rootDir, false, true ) )
		{
			return false;
		}

		// hack
		if ( contentName.BeginsWith(TXT("patch")) )
		{
			for ( auto& chunkIt : newContentPack.m_contentChunks )
			{
				for ( auto& fileIt : chunkIt.m_contentFiles )
				{
					fileIt.m_isPatch = true;
				}
			}
		}

		newContentPack.m_contentChunks.PushBack( newContentChunk );
	}

	outContentPack = Move( newContentPack );

	if ( outContentPack.m_contentChunks.Empty() )
	{
		WARN_CORE(TXT("ScanForSplitCookFiles: no split content was found!"));
	}

	return true;
}

Bool CContentManager::ScanForDlcFiles( TDynArray< SContentPack >& outContentPacks ) const
{
	const String dlcRootDir = GFileManager->GetRootDirectory() + DLC_SCAN_DIR;

	LOG_CORE(TXT("CContentManager::ScanForDlcFiles under '%ls'..."), dlcRootDir.AsChar() );

	CTimeCounter timeCounter;

	// Get all the content
	TDynArray< String > dlcDirs;
	GFileManager->FindDirectories( dlcRootDir, dlcDirs );
	struct DlcNumber
	{
		static Bool Get( const String& str, Uint32& outVal )
		{
			const Char* ch = str.AsChar();
			for(; *ch; ++ch )
			{
				if ( *ch >= '0' && *ch <= '9' )
					break;
			}

			return GParseInteger( ch, outVal );
		}
	};

	// Sort dlc directories into numerical order so mounting is always in the same order.
	// If no content number is found then a regular string comparison is used instead.
	Sort( dlcDirs.Begin(), dlcDirs.End(), 
		[](const String& a, const String& b )
	{
		Uint32 valA = 0;
		Uint32 valB = 0;

		if ( !DlcNumber::Get(a, valA) )
		{
			return a < b;
		}
		if ( !DlcNumber::Get(b, valB) )
		{
			return a < b;
		}

		return valA < valB;
	});

	for ( const String& dlcName : dlcDirs )
	{
		const String scanDir = String::Printf(TXT("%ls%ls\\content\\"), dlcRootDir.AsChar(), dlcName.AsChar() );
		TDynArray< String > absolutePaths;
		GFileManager->FindFiles( scanDir, TXT("*.*"), absolutePaths, true );

		// For some extra determinism across platforms
		Sort( absolutePaths.Begin(), absolutePaths.End() );

		for ( String& path : absolutePaths )
		{
			path.ReplaceAll( TXT("/"), TXT("\\") );
		}

		const String rootDir = scanDir;
		Red::System::DateTime timestamp;
		SContentChunk newContentChunk;
		if ( !Helper::CreateContentChunk( newContentChunk, timestamp, CName( dlcName.ToLower() ), absolutePaths, rootDir, false, true ) )
		{
			return false;
		}

		if ( newContentChunk.m_contentFiles.Empty() )
		{
			WARN_CORE(TXT("ScanForDlcFiles: no dlc content was found in '%ls'!"), dlcName.AsChar());
		}

		SContentPack newContentPack;
		newContentPack.m_id = CName( dlcName );
		newContentPack.m_dependency = CNAME(content);
		newContentPack.m_contentChunks.PushBack( Move( newContentChunk ) );
		outContentPacks.PushBack( Move( newContentPack ) );
	}

	if ( outContentPacks.Empty() )
	{
		WARN_CORE(TXT("ScanForDlcFiles: no dlc packs were found!"));
	}


	LOG_CORE(TXT("CContentManager: DLC content scanned in %1.2f sec"), timeCounter.GetTimePeriod());

#ifdef RED_LOGGING_ENABLED
	for ( const SContentPack& contentPack : outContentPacks )
	{
		Uint32 numFiles = 0;
		for ( const SContentChunk& contentChunk : contentPack.m_contentChunks )
		{
			numFiles += contentChunk.m_contentFiles.Size();
		}
		LOG_CORE(TXT("Found DLC contentPack id='%ls' dep='%ls', numChunks=%u, numFiles=%u"),
			contentPack.m_id.AsChar(), contentPack.m_dependency.AsChar(), contentPack.m_contentChunks.Size(), numFiles);
	}
#endif

	return true;
}

#ifdef RED_MOD_SUPPORT
static Bool IsAllowedModName( const String& modName )
{
	if ( modName.Empty() )
	{
		return false;
	}
	if ( modName.GetLength() > (Uint32)Config::cvMaxModNameLength.Get() )
	{
		return false;
	}
	if ( Red::System::StringCompareNoCase( modName.AsChar(), TXT("dlc"), Red::System::StringLengthCompileTime(TXT("dlc"))) == 0 )
	{
		return false;
	}
	if ( Red::System::StringCompareNoCase( modName.AsChar(), TXT("patch"), Red::System::StringLengthCompileTime(TXT("patch"))) == 0 )
	{
		return false;
	}

	Bool isValid = true;
	for ( Uint32 i = 0; i < modName.GetLength(); ++i )
	{
		const Char ch = modName[i];
		if ( ch == TXT('-') || ch == TXT('_') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') )
		{
			continue;
		}

		isValid = false;
		break;
	}
	
	return isValid;
}
#endif // RED_MOD_SUPPORT

#ifdef RED_MOD_SUPPORT
Bool CContentManager::ScanForModFiles( TDynArray< SContentPack >& outContentPacks ) const
{
	if ( GIsEditor || GIsCooker )
	{
		return true;
	}

	const Bool noMods = !Config::cvEnableMods.Get() || Red::System::StringSearch( SGetCommandLine(), TXT("-nomods") ) != nullptr;
	if ( noMods )
	{
		LOG_CORE(TXT("CContentManager::ScanForModFiles. Mods disabled by use. Eithe") );
		return true;
	}

	TDynArray< SContentPackInstance* > finishedPackages;

	// Bundles need to be under the common content root path
	const String modsRootDir = GFileManager->GetRootDirectory() + MODS_SCAN_DIR;

	LOG_CORE(TXT("CContentManager::ScanForModFiles under '%ls'..."), modsRootDir.AsChar() );

	CTimeCounter timeCounter;

	// Get all the content
	TDynArray< String > modDirs;
	GFileManager->FindDirectories( modsRootDir, modDirs );

	// Yeah
	for ( Int32 j = modDirs.SizeInt()-1; j >=0; --j )
	{
		if ( !modDirs[j].ToLower().BeginsWith(TXT("mod")) )
		{
			modDirs.RemoveAt(j);
		}
	}
	
	// Bail fast to avoid pulling in *.ini. Should make no huge diff.
	if ( modDirs.Empty() )
	{
		WARN_CORE(TXT("ScanForModFiles: no mod packs were found!"));
		return true;
	}

	// Make it all lower case for consistent processing later
	for ( auto& modDir : modDirs )
	{
		modDir = modDir.ToLower();
	}

	// HACK: normally was just local to this function, but exposing to depotBundles now
	TModConfigMap& modConfigMap = const_cast<TModConfigMap&>(m_modConfigMap);
	modConfigMap.ClearFast();

	Config::Legacy::CConfigLegacyFile* modsConfig = SConfig::GetInstance().GetLegacy().GetFileWithAbsolutePathAndExtension( 
		GFileManager->GetUserDirectory().AsChar(), TXT("mods"), TXT("settings") );

	if( modsConfig )
	{
		const auto& sections = modsConfig->GetSections();
		for( auto sectionIter = sections.Begin(); sectionIter != sections.End(); ++sectionIter )
		{
			SModConfig modConfig;
			String modName = sectionIter->m_first.ToLower();
			Config::Legacy::CConfigLegacySection* section = sectionIter->m_second;
			if ( section )
			{
				String strEnabled;
				String strPriority;
				if ( section->ReadValue( TXT("Enabled"), strEnabled ) )
					FromString( strEnabled, modConfig.m_enabled );
				if ( section->ReadValue( TXT("Priority"), strPriority ) )
					FromString( strPriority, modConfig.m_priority );
			}
			modConfigMap.Insert( modName, Move( modConfig ) );
		}
	}

	// Disable a mod if it has a disallowed name
	for ( auto& modName : modDirs )
	{
		modConfigMap[modName].m_enabled &= IsAllowedModName( modName );
	}

	// Prune disabled mods
	for ( Int32 j = modDirs.SizeInt()-1; j >= 0; --j )
	{
		if ( !modConfigMap[modDirs[j]].m_enabled )
		{
			LOG_CORE(TXT("Mod '%ls' DISABLED"), modDirs[j].AsChar());
			modDirs.RemoveAt(j);
		}
	}

	// Sort mod directories by priority so will override files properly
	Sort( modDirs.Begin(), modDirs.End(), 
		[&modConfigMap](const String& a, const String& b )
	{
		const SModConfig& cfgA = modConfigMap[a];
		const SModConfig& cfgB = modConfigMap[b];
		if ( cfgA.m_priority == cfgB.m_priority )
			return a > b;
		return cfgA.m_priority > cfgB.m_priority;
	});

	for ( const String& modName : modDirs )
	{
		const String scanDir = String::Printf(TXT("%ls%ls\\content\\"), modsRootDir.AsChar(), modName.AsChar() );
		TDynArray< String > absolutePaths;
		GFileManager->FindFiles( scanDir, TXT("*.*"), absolutePaths, true );

		// For some extra determinism across platforms
		Sort( absolutePaths.Begin(), absolutePaths.End() );

		for ( String& path : absolutePaths )
		{
			path.ReplaceAll( TXT("/"), TXT("\\") );
		}

		// One above modDir for the sake of bundle paths processing
		const String rootDir = scanDir;
		Red::System::DateTime timestamp;
		SContentChunk newContentChunk;
		if ( !Helper::CreateContentChunk( newContentChunk, timestamp, CName( modName.ToLower() ), absolutePaths, rootDir, false, true ) )
		{
			continue;
		}

		if ( newContentChunk.m_contentFiles.Empty() )
		{
			WARN_CORE(TXT("ScanForModFiles: no mod content was found in '%ls'!"), modName.AsChar());
		}

		SContentPack newContentPack;
		newContentPack.m_id = CName( modName );
		newContentPack.m_dependency = CNAME(content);
		newContentPack.m_contentChunks.PushBack( Move( newContentChunk ) );
		for ( auto& chunkIt : newContentPack.m_contentChunks )
		{
			// Currently a mod is a patch that can override base game or base-game-patch files
			for ( auto& fileIt : chunkIt.m_contentFiles )
			{
				fileIt.m_isPatch = true;

				if ( fileIt.m_path.BeginsWith("scripts\\") && fileIt.m_path.EndsWith(".ws") )
				{
					m_requiresUncookedScripts |= true;
				}
			}
		}

		outContentPacks.PushBack( Move( newContentPack ) );
	}

	if ( outContentPacks.Empty() )
	{
		WARN_CORE(TXT("ScanForModFiles: no mod packs were found!"));
	}

	LOG_CORE(TXT("CContentManager: Mods content scanned in %1.2f sec"), timeCounter.GetTimePeriod());

#ifdef RED_LOGGING_ENABLED
	for ( const SContentPack& contentPack : outContentPacks )
	{
		Uint32 numFiles = 0;
		for ( const SContentChunk& contentChunk : contentPack.m_contentChunks )
		{
			numFiles += contentChunk.m_contentFiles.Size();
		}
		LOG_CORE(TXT("Found mod contentPack id='%ls' dep='%ls', numChunks=%u, numFiles=%u"),
			contentPack.m_id.AsChar(), contentPack.m_dependency.AsChar(), contentPack.m_contentChunks.Size(), numFiles);
	}
#endif

	return true;
}
#endif // RED_MOD_SUPPORT

Bool CContentManager::ResolveOverridePath( const String& path, String& outOverridePath ) const
{

	// TBD: keep a map for faster lookup. But right now only needs to be for a few files.
	// Order based purely on packageID

#ifndef RED_MOD_SUPPORT
	return false;
#else

	if ( GIsEditor || GIsCooker )
	{
		return false;
	}

	const StringAnsi overrideFilePath = UNICODE_TO_ANSI( ContentHelpers::ExtractFileName( path.ToLower() ) );
	RuntimePackageID maxID = -1;
	const StringAnsi ansiPath = UNICODE_TO_ANSI(path.ToLower().AsChar());
	typedef TPair< const SContentPackInstance*, const SContentFile* > TResolvedEntry;
	TResolvedEntry resolvedEntry;

	for ( const SContentChunkContext& context : m_attachedChunkContexts )
	{
		const RuntimePackageID id = context.m_packInstance->m_packageID;
		if ( id <= maxID )
		{
			// Don't bother checking the files
			continue;
		}

		for ( const SContentFile& contentFile: context.m_chunkPtr->m_contentFiles )
		{
			const AnsiChar* filePath = ContentHelpers::ExtractFileName( contentFile );
			if ( overrideFilePath.EqualsNC( filePath ) )
			{
				resolvedEntry = TResolvedEntry( context.m_packInstance, &contentFile );
				maxID = id;
				break;
			}
		}
	}

	if ( resolvedEntry.m_first )
	{
		RED_FATAL_ASSERT( resolvedEntry.m_second, "No file?");
		outOverridePath = resolvedEntry.m_first->m_mountPath + ANSI_TO_UNICODE(resolvedEntry.m_second->m_path.AsChar());

		LOG_CORE(TXT("ResolveOverridePath: '%ls'->'%ls'"), path.AsChar(), outOverridePath.AsChar());

		return true;
	}

	return false;
#endif
}

void CContentManager::ScanFoldersForContent( EContentSource contentSource )
{
	LOG_CORE(TXT("CContentManager scanning for contentSource '%ls'"), GetContentTxtForLog( contentSource ) );

	m_contentSource = contentSource;

	m_taintedFlags |= eContentTaintedFlag_Scanned;

	SContentPack newBaseContentPack;
	if ( !ScanForContentFiles( m_contentSource, newBaseContentPack ) )
	{
		return;
	}

	String rootMountPath = GFileManager->GetRootDirectory();
	rootMountPath.ReplaceAll(TXT('/'), TXT('\\'));
	if ( !rootMountPath.EndsWith(TXT("\\")) )
	{
		rootMountPath += TXT("\\");
	}

	if ( contentSource == eContentSource_SplitCook )
	{
		rootMountPath += TXT("content\\");
	}

	TDynArray< SContentPackInstance* > finishedPackages;

	// Base files
	{
		SContentPackInstance* newPackInstance = new SContentPackInstance( Move( newBaseContentPack ), BASE_RUNTIME_PACKAGE_ID, rootMountPath );
		newPackInstance->SetAutoAttachChunks( true );
		m_packageInstanceMap.Insert( BASE_RUNTIME_PACKAGE_ID, newPackInstance );
		finishedPackages.PushBack( newPackInstance );
	}

	TDynArray< SContentPack > dlcPacks;
	if ( ScanForDlcFiles( dlcPacks ) )
	{

		const String dlcRootDir = GFileManager->GetRootDirectory() + DLC_SCAN_DIR;
		
		for ( SContentPack& newDlcPack : dlcPacks )
		{
			// When scanning the id is the same as the directory name and under the root mount path. On consoles the mount path
			// prefix can be arbitrary
			const RuntimePackageID dlcPackageID = Helper::AllocRuntimePackageID();
			const String scanDir = String::Printf(TXT("%ls%ls\\content\\"), dlcRootDir.AsChar(), newDlcPack.m_id.AsChar() );
			SContentPackInstance* newDlcPackInstance = new SContentPackInstance( Move( newDlcPack ), dlcPackageID, scanDir );
			newDlcPackInstance->SetAutoAttachChunks( true );
			newDlcPackInstance->SetHidden( true );
			m_packageInstanceMap.Insert( dlcPackageID, newDlcPackInstance );
			finishedPackages.PushBack( newDlcPackInstance );
		}
	}

#ifdef RED_MOD_SUPPORT
	TDynArray< SContentPack > modPacks;
	if ( ScanForModFiles( modPacks ) )
	{
		if ( !modPacks.Empty() )
		{
			m_taintedFlags |= eContentTaintedFlag_Mods;
		}

		const String modRootDir = GFileManager->GetRootDirectory() + MODS_SCAN_DIR;

		for ( SContentPack& newModPack : modPacks )
		{
			// When scanning the id is the same as the directory name and under the root mount path. On consoles the mount path
			// prefix can be arbitrary
			const RuntimePackageID modPackageID = Helper::AllocRuntimePackageID();
			const String scanDir = String::Printf(TXT("%ls%ls\\content\\"), modRootDir.AsChar(), newModPack.m_id.AsChar() );
			SContentPackInstance* newModPackInstance = new SContentPackInstance( Move( newModPack ), modPackageID, scanDir );
			newModPackInstance->SetAutoAttachChunks( true );
			newModPackInstance->SetHidden( true );
			newModPackInstance->SetMod( true );
			m_packageInstanceMap.Insert( modPackageID, newModPackInstance );
			finishedPackages.PushBack( newModPackInstance );
		}
	}
#endif

	ProcessFinishedPackages( finishedPackages );
}

void CContentManager::AddTaintedFlags( Uint32 flags )
{
	LOG_CORE(TXT("CContentManager::AddTaintedFlags adding 0x%08X to 0x%08X"), flags,  m_taintedFlags );
	m_taintedFlags |= flags;
}

#ifdef RED_LOGGING_ENABLED
namespace
{
	const Char* GetContentTxtForLog( EContentSource contentSource )
	{
		const Char* txt = TXT("<Unknown>");
		switch (contentSource)
		{
		case eContentSource_LooseFiles:
			txt = TXT("eContentSource_LooseFiles");
			break;
		case eContentSource_Cook:
			txt = TXT("eContentSource_Cook");
			break;
		case eContentSource_SplitCook:
			txt = TXT("eContentSource_SplitCook");
			break;
		default:
			break;
		}

		return txt;
	}
}
#endif // RED_LOGGING_ENABLED
