/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Define before #include'ing playgo.h
#ifndef RED_FINAL_BUILD
# define SCE_DBG_PLAYGO_ENABLED 0
#endif

#include "../../common/core/playGoPackage.h"
#include "../../common/core/streamingInstaller.h"
#include "../../common/core/installer.h"

#include <playgo.h>

// See Restrictions on Using Patches to Change Application Chunk Configurations: https://ps4.scedev.net/technotes/view/222/1
// Basically can't change scenario, change initial chunks, add new chunks, or change language masks.
class CStreamingInstallerOrbis
	: public IContentInstaller
	, public IStreamingInstaller
	, public ILanguageProvider
	, private Red::System::NonCopyable
{
public:
	enum EInstallSpeed
	{
		eInstallSpeed_Suspended,
		eInstallSpeed_Trickle,
		eInstallSpeed_Full,
	};

private:
	struct SBlurayParams
	{
		Float			m_initPrefetchSec;				//!< How much time to spend trying to prefetch data to the HDD before the installer is ready
		EInstallSpeed	m_needPackageLaunchContent;		//!< Install speed when package launch content not installed yet
		EInstallSpeed	m_loadingScreenInstallSpeed;	//!< Install speed during loading screen
		EInstallSpeed	m_idleInstallSpeed;				//!< Install speed when game idle

		SBlurayParams()
			: m_initPrefetchSec( 0.f )
			, m_needPackageLaunchContent( eInstallSpeed_Full )
			, m_loadingScreenInstallSpeed( eInstallSpeed_Trickle )
			, m_idleInstallSpeed( eInstallSpeed_Full )
		{}
	};

	struct SDownloadParams
	{
		EInstallSpeed	m_loadingScreenInstallSpeed;	//!< Install speed during loading screen
		EInstallSpeed	m_idleInstallSpeed;				//!< Install speed when game idle

		SDownloadParams()
			: m_loadingScreenInstallSpeed( eInstallSpeed_Trickle )
			, m_idleInstallSpeed( eInstallSpeed_Full )
		{}
	};

public:
#ifndef RED_FINAL_BUILD
	struct SDebugStats
	{
		EngineTime					m_lastWarnTime;
		EngineTime					m_lastSuspendTime;

		EngineTime					m_totalTimeSuspended;
		EngineTime					m_totalTimeTrickle;
		EngineTime					m_totalTimeFull;
		
		// TODO: Histogram of layer switches (keyed to events like loading)
		// etc
	};
#endif // RED_FINAL_BUILD

#ifndef RED_FINAL_BUILD
	SDebugStats m_debugStats;
#endif // RED_FINAL_BUILD

private:
	enum { HEAP_SIZE = SCE_PLAYGO_HEAP_SIZE };
	enum { MAX_CHUNKS = SCE_PLAYGO_CHUNK_INDEX_MAX };

private:
	void*										m_playGoBuffer;
	ScePlayGoHandle								m_playGoHandle;

private:
	TDynArray< SContentPackageEvent >			m_updatePackageEvents;

private:
	SPlayGoPackage								m_playGoPackage;

private:
	TBitSet64< MAX_CHUNKS >						m_downloadedChunkIDs;		//<! chunk IDs that are in available but don't have to be installed yet
	TBitSet64< MAX_CHUNKS >						m_installedChunkIDs;		//<! chunk IDs that have been installed and content listeners notified
	TBitSet64< MAX_CHUNKS >						m_errorChunkIDs;			//<! chunk IDs that errored out and are unavailable. Hopefully never any set.
	TBitSet64< MAX_CHUNKS >						m_slowChunkIDs;				//<! chunk IDs that can be on disc but count as downloaded.
	TDynArray< Uint16 >							m_pendingDownloadChunkIDs;	//<! chunk IDs that aren't available yet
	TDynArray< Uint16 >							m_pendingSlowChunkIDs;		//<! chunk IDs that are still on bluray but counted as downloaded anyway
	TBitSet64< MAX_CHUNKS >						m_prefetchChunkIDs;			//!< chunk IDs that are on bluray but we want on HDD.

private:
	Uint64										m_tscFreq;
	Uint64										m_startPriorityIOTick;
	Bool										m_allowSuspendInstaller;
	Bool										m_asyncSuspendThrottled;

#ifdef RED_LOGGING_ENABLED
	struct SPrefetchProgressForLogging
	{
		ScePlayGoHandle		m_playGoHandle;
		TDynArray< Uint16 >	m_chunkIDs;				//<! chunk IDs that were pending when we started initial prefetch
		ScePlayGoProgress	m_progress;				//<! progress amount for given chunks when prefetching started

		SPrefetchProgressForLogging()
			: m_playGoHandle()
			, m_progress()
		{}

		Bool UpdateProgress();
	};

	SPrefetchProgressForLogging m_prefetchProgressForLogging;
#endif

private:
	struct SProgressInfo
	{
		CName									m_contentName;		//!< content being tracked for progress
		TDynArray< Uint16 >						m_chunkIDs;			//!< chunk IDs that are being tracked for progress
		Uint32									m_percentComplete;
		Bool									m_queried;

		SProgressInfo()
			: m_percentComplete( 0 )
			, m_queried( false )
		{}

		void Clear()
		{
			m_contentName = CName::NONE;
			m_chunkIDs.ClearFast();
			m_percentComplete = 0;
			m_queried = false;
		}
	};

	SProgressInfo								m_progressInfo;

private:
	TPlayGoLanguageFlag							m_languageFlag;
	TArrayMap< CName, const SPlayGoContent* >	m_contentNameMap;
	TArrayMap< Uint32, const SPlayGoContent* >	m_chunkContentMap;

	const SPlayGoContent*						m_launchContent;
	const SPlayGoContent*						m_finalContent;
	Uint32										m_bootChunkID;			//<! chunk ID required for base engine init
	Bool										m_hasBootZero;

private:
	TSortedArray< CName >						m_installedContent;
	mutable TSortedArray< CName >				m_cachedAvailableContent;

private:
	EngineTime									m_initStartTime;

private:
	Bool										m_isPlayGoInitialized;
	Bool										m_hasPendingBlurayChunks;
	TPlayGoLanguageFlag							m_requiredSlowChunksLangMask;
	Bool										m_isInstallingAllLanguageChunksFromBluray;

private:
	SBlurayParams								m_blurayParams;
	SDownloadParams								m_downloadParams;
	Bool										m_isInitBlurayPrefetchFinished;

public:
											CStreamingInstallerOrbis();
	virtual									~CStreamingInstallerOrbis();
	virtual Bool							Init() override;

public:
	// HACK for savegames to give them I/O priority: for when you just can't wait for an update.
	void									SuspendInstallerAsync();

public:
	//! IContentInstaller functions
	virtual Bool							QueryStreamingInstaller( IStreamingInstaller** outStreamingInstaller ) { *outStreamingInstaller = this; return true; }
	virtual Bool							QueryLanguageProvider( ILanguageProvider** outLanguageProvider ) { *outLanguageProvider = this; return true; }
	virtual void							Update( TDynArray< SContentPackageEvent >& outContentPackageEvents ) override;
	virtual Bool							IsContentAvailable( CName contentName, Bool& outIsAvailable ) const;
	virtual Bool							IsReady() const override;

public:
	//! IStreamingInstaller functions
	virtual Bool							GetPercentCompleted( CName contentName, Uint32& outPercentCompleted ) const override;
	virtual Bool							GetResolvedLaunchZero( CName& outLaunchZero ) const override;

	virtual EContentStallType				GetStallForMoreContent() const override;
	virtual void							PrefetchContent( CName contentName ) override;
	virtual void							ResetPrefetchContent() override;

public:
	//! ILanguageProvider functions
	virtual Bool							GetValidDefaultGameLanguage( String& outTextLanguage, String& outSpeechLanguage ) const override;
	virtual Bool							SetSpeechInstallLanguage( const String& speechLanguage ) override;
	virtual Bool							GetSupportedLanguages( TDynArray< String >& outTextLangauges, TDynArray< String >& outSpeechLanguages, String& outDefaultLanguage ) const override;

private:
	Bool									InitFromCommandline( SBlurayParams& outBlurayParams, SDownloadParams& outDownloadParams );
	Bool									InitPlayGo();
	Bool									InitPlayGoToDoList();
	Bool									InitPrefetch();
	Bool									PostInitPlaygo();
	Bool									InitLanguage();
	void									ShutdownPlayGo();

private:
	Bool									LoadPlayGoChunksData( const String& filePath );

private:
	Bool									OpenPlayGoHandle();
	void									ClosePlayGoHandle();

private:
	void									UpdatePrefetch();
	void									UpdateInstallSpeed();
	void									UpdateDownloadChunkLoci( TBitSet64< MAX_CHUNKS >& outNewDownloadedChunkIDs );
	void									UpdateSlowChunkLoci();
	void									UpdateProgress();

private:
	void									InstallReadyContent( const TBitSet64< MAX_CHUNKS >& chunkIDs );

private:
	void									RefreshProgressIfNeeded( const SPlayGoContent& contentToTrack );

private:
	void									RefreshContentInstallStatus();
	void									UpdateContentInstallStatus( const SPlayGoContent* content );

private:
	Bool									GetNumMaskedTodoListEntries( Uint32& outNumEntries ) const;

private:
	Bool									InstallAllLanguageChunksFromBluray();

private:
	Bool									GetPackChunk( Uint32 chunkID, CName& outChunkName ) const;
	const SPlayGoContent*					GetContent( Uint32 chunkID ) const;
	const SPlayGoContent*					GetContent( CName contentName ) const;
	Bool									GetLanguageFromLanguageFlag( TPlayGoLanguageFlag flag, StringAnsi& outLang ) const;
	Bool									GetPlayGoLanguageFlag( const StringAnsi& lang, TPlayGoLanguageFlag& outFlag ) const;
	Bool									GetDefaultPlayGoLanguageFlag( TPlayGoLanguageFlag& outFlag ) const;
	Bool									GetChunkLanguageFlag( Uint32 chunkID, TPlayGoLanguageFlag& outFlag ) const;

private:
	Bool									IsSlowChunk( Uint32 chunkID ) const;

private:
	Bool									IsBackgrounded() const;

private:
	Bool									CheckForSuspendAbuse();


	private:
#ifdef RED_LOGGING_ENABLED
	void									DumpAllProgress();
	void									DumpPrefetchStats();
	void									DumpMaskedTodoList();
#endif

#ifndef RED_FINAL_BUILD
	void									UpdateDebugStats();
#endif
};
