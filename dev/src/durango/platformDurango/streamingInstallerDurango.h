/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/xvcPackage.h"
#include "../../common/core/streamingInstaller.h"
#include "../../common/core/installer.h"

struct SContentPackageEvent;

class CStreamingInstallerDurango
	: public IContentInstaller
	, public IStreamingInstaller
	, public ILanguageProvider
	, private Red::System::NonCopyable
{
public:
	CStreamingInstallerDurango();
	~CStreamingInstallerDurango();
	virtual Bool Init() override;

private:
	//! IContentInstaller functions
	virtual Bool							QueryStreamingInstaller( IStreamingInstaller** outStreamingInstaller ) { *outStreamingInstaller = this; return true; }
	virtual Bool							QueryLanguageProvider( ILanguageProvider** outLanguageProvider ) { *outLanguageProvider = this; return true; }
	virtual void							Update( TDynArray< SContentPackageEvent >& outContentPackageEvents ) override;
	virtual Bool							IsContentAvailable( CName contentName, Bool& outIsAvailable ) const;
	virtual Bool							IsReady() const override;
	virtual void							OnSuspend() override;
	virtual void							OnResume() override;

public:
	//! IStreamingInstaller functions
	virtual Bool							GetPercentCompleted( CName contentName, Uint32& outPercentCompleted ) const override;
	virtual EContentStallType				GetStallForMoreContent() const override;
	virtual Bool							GetResolvedLaunchZero( CName& outLaunchZero ) const override;

public:
	//! ILanguageProvider functions
	virtual Bool							GetValidDefaultGameLanguage( String& outTextLanguage, String& outSpeechLanguage ) const override;
	virtual Bool							SetSpeechInstallLanguage( const String& speechLanguage ) override;
	virtual Bool							GetSupportedLanguages( TDynArray< String >& outTextLangauges, TDynArray< String >& outSpeechLanguages, String& outDefaultLanguage ) const override;

private:
	typedef Windows::Xbox::Management::Deployment::PackageTransferManager PackageTransferManager;
	typedef Windows::Xbox::Management::Deployment::PackageTransferWatcher PackageTransferWatcher;
	typedef Windows::Xbox::Management::Deployment::ProgressChangedEventArgs ProgressChangedEventArgs;
	typedef Windows::Xbox::Management::Deployment::ChunkCompletedEventArgs ChunkCompletedEventArgs;
	typedef Windows::Foundation::EventRegistrationToken EventRegistrationToken;

private:
	void									PostInit();

private:
	Bool									InitTransfer();
	Bool									InitLanguage();
	Bool									LoadXvcPackage( const String& filePath );

private:
	void									ShutdownTransfer();
	void									CancelProgressWatcher();

private:
	void									OnChunkCompleted( Uint32 chunkId );
	void									OnProgressChanged( Uint32 percentComplete );

private:
	void									RefreshTodoList();

private:
	void									RefreshProgressIfNeeded( const SXvcContent& contentToTrack );

private:
	void									InstallReadyContent( const TBitSet64< MAX_XVC_CHUNKS >& chunkIDs );

private:
	void									RefreshContentInstallStatus();
	void									UpdateContentInstallStatus( const SXvcContent* content );


private:
	Bool									GetPackChunk( Uint32 chunkID, CName& outChunkName ) const;
	const SXvcContent*						GetContent( Uint32 chunkID ) const;
	const SXvcContent*						GetContent( CName contentName ) const;

private:
	Bool									GetLanguageFromLocale( const SXvcLocale& locale, StringAnsi& outLang ) const;

// private:
// 	static const Float						INSTALL_TIMEOUT_INTERVAL

private:
	TArrayMap< CName, const SXvcContent* >	m_contentNameMap;
	TArrayMap< Uint32, const SXvcContent* >	m_chunkContentMap;
	const SXvcContent*						m_launchContent;
	const SXvcContent*						m_finalContent;
	Uint32									m_bootChunkID;			//<! chunk ID required for base engine init

private:
	struct SProgressInfo
	{
		CName								m_contentName;		//!< content being tracked for progress
		Uint32								m_percentComplete;

		SProgressInfo()
			: m_percentComplete( 0 )
		{}

		void Clear()
		{
			m_contentName = CName::NONE;
			m_percentComplete = 0;
		}
	};

	SProgressInfo							m_progressInfo;

private:
	TBitSet64< MAX_XVC_CHUNKS >				m_snapshotDownloadedChunkIDs;	//<! chunk IDs seen as downloaded last update, not just new chunk IDs
	TBitSet64< MAX_XVC_CHUNKS >				m_downloadedChunkIDs;			//<! chunk IDs that are in available but don't have to be installed yet
	TBitSet64< MAX_XVC_CHUNKS >				m_installedChunkIDs;			//<! chunk IDs that have been installed and content listeners notified
	TBitSet64< MAX_XVC_CHUNKS >				m_errorChunkIDs;				//<! chunk IDs that errored out and are unavailable. Hopefully never any set.
	TBitSet64< MAX_XVC_CHUNKS >				m_validChunkIDs;				//!< chunk IDs that have a valid mapping. The XDK docs say to be tolerant of system-used chunk IDs.

private:
	TSortedArray< CName >					m_installedContent;
	mutable TSortedArray< CName >			m_cachedAvailableContent;

private:
	TDynArray< SContentPackageEvent >		m_updatePackageEvents;

private:
	SXvcLocale								m_locale;
	PackageTransferManager^					m_transferManager;
	PackageTransferWatcher^					m_chunkCompletedWatcher;
	PackageTransferWatcher^					m_chunkProgressWatcher;
	EventRegistrationToken					m_chunkCompletedToken;
	EventRegistrationToken					m_progressChangedToken;
	SXvcPackage								m_xvcPackage;
	Bool									m_isInitialized;
	Bool									m_hasBootZero;
};
