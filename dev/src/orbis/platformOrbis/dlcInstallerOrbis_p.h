/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/redThreads/redThreadsThread.h"
#include "../../common/core/tokenizer.h"

template<>
class DefaultHashFunc< SceNpUnifiedEntitlementLabel, false >
{
public:
	static RED_FORCE_INLINE Uint32 GetHash( const SceNpUnifiedEntitlementLabel& key ) { return StringAnsi::SimpleHash( key.data, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE ); }
};

template<>
struct DefaultEqualFunc< SceNpUnifiedEntitlementLabel >
{
	static RED_INLINE Bool Equal( const SceNpUnifiedEntitlementLabel& key1, const SceNpUnifiedEntitlementLabel& key2 ) { return Red::System::StringCompare( key1.data, key2.data, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE ) == 0; }
};

Bool operator==( const SceNpUnifiedEntitlementLabel& a, const SceNpUnifiedEntitlementLabel& b ) { return Red::System::StringCompare( a.data, b.data, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE ) == 0; }
Bool operator<( const SceNpUnifiedEntitlementLabel& a, const SceNpUnifiedEntitlementLabel& b ) { return Red::System::StringCompare( a.data, b.data, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE ) < 0; }
Bool operator==( const SceAppContentAddcontInfo& a, const SceAppContentAddcontInfo& b ) { return a.entitlementLabel == b.entitlementLabel; }
Bool operator<( const SceAppContentAddcontInfo& a, const SceAppContentAddcontInfo& b ) { return a.entitlementLabel < b.entitlementLabel; }

#ifdef RED_LOGGING_ENABLED
static const Char* GetStatusTxtForLog( SceAppContentAddcontDownloadStatus status );
static const Char* GetMountErrTxtForLog( Int32 err );
#endif // RED_LOGGING_ENABLED

namespace Helper
{

//////////////////////////////////////////////////////////////////////////
// CDlcPackageAutoMounterThread
//////////////////////////////////////////////////////////////////////////
class CDlcPackageAutoMounterThread : public Red::Threads::CThread
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CDlcPackageAutoMounterThread( const TDynArray< String >& blacklistedDlcs );

	void RequestExit();
	void RequestUpdate( TDynArray< SPackageInfo >* outPackageInfos, volatile Bool* outFinishedFlag );

private:
	virtual void ThreadFunc() override;

private:
	Bool PopulatePackageInfos( const TDynArray< SceAppContentAddcontInfo >& infoList, TDynArray< SPackageInfo >& outPackageInfos );

private:
	Bool IsBlacklisted( const SceNpUnifiedEntitlementLabel& label ) const;

private:
	struct SPackagePersistentInfo
	{
		SceAppContentMountPoint m_mountPoint;
		RuntimePackageID		m_packageID;

		SPackagePersistentInfo()
			: m_packageID( INVALID_RUNTIME_PACKAGE_ID )
		{
			Red::System::MemoryZero( &m_mountPoint, sizeof(SceAppContentMountPoint ) );
		}

		SPackagePersistentInfo( const SceAppContentMountPoint& mountPoint, RuntimePackageID packageID )
			: m_mountPoint( mountPoint )
			, m_packageID( packageID )
		{}
	};

private:
	typedef Red::Threads::CSemaphore CSemaphore;

private:
	TDynArray< SceNpUnifiedEntitlementLabel >							m_blacklistedLabels;
	THashMap< SceNpUnifiedEntitlementLabel, SPackagePersistentInfo >	m_labelPersistentInfoMap;

	CSemaphore									m_semaHasWork;
	Bool										m_exitFlag;
	TDynArray< SPackageInfo >*					m_outPackageInfos;
	volatile Bool*								m_outFinishedFlag;
};

//////////////////////////////////////////////////////////////////////////
// CDlcPackageAutoMounter
//////////////////////////////////////////////////////////////////////////
class CDlcPackageAutoMounter
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CDlcPackageAutoMounter( const TDynArray< String >& blacklistedDlcs );
	~CDlcPackageAutoMounter();

	void Update();
	Bool IsDirty() const;
	void RequestRefresh();
	Bool FlushPackageInfos( TDynArray< SPackageInfo >& outPackageInfos );

private:
	CDlcPackageAutoMounterThread*													m_autoMounterThread;
	TDynArray< SPackageInfo >														m_packageInfos[2];
	Uint32																			m_currentPacakgeIndex;
	volatile Bool																	m_isWorkerThreadIdle;
	Bool																			m_isRefreshPending;
	Bool																			m_isRefreshRequested;
	Bool																			m_hasNewPackageInfos;
};

//////////////////////////////////////////////////////////////////////////
// CDlcPackageAutoMounterThread
//////////////////////////////////////////////////////////////////////////
CDlcPackageAutoMounterThread::CDlcPackageAutoMounterThread( const TDynArray< String >& blacklistedDlcs )
#ifdef RED_LOGGING_ENABLED
	: Red::Threads::CThread( "DLC AutoMounter", Red::Threads::SThreadMemParams( 1024 * 1024 ) ) // Search for MIGHTY_STACK_SIZE and weep
#else
	: Red::Threads::CThread( "DLC AutoMounter", Red::Threads::SThreadMemParams( 32 * 1024 ) )
#endif
	, m_semaHasWork( 0, 1 )
	, m_exitFlag( false )
	, m_outPackageInfos( nullptr )
	, m_outFinishedFlag( nullptr )
{
	m_blacklistedLabels.Reserve( blacklistedDlcs.Size() );
	for ( const String& id : blacklistedDlcs )
	{
		SceNpUnifiedEntitlementLabel label;
		Red::System::MemoryZero( &label, sizeof(SceNpUnifiedEntitlementLabel) );
		Red::System::MemoryCopy( label.data, UNICODE_TO_ANSI(id.AsChar()), Min<Uint32>( SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE, id.GetLength() ) );
		m_blacklistedLabels.PushBack( label );
	}
}

SceNpServiceLabel GetDLCServiceLabel()
{
	static SceNpServiceLabel serviceLabel = -1;
	if( serviceLabel == -1 )
	{
		serviceLabel = 0;
		CTokenizer tok( SGetCommandLine(), TXT(" ") );
		for ( Uint32 i=0; i<tok.GetNumTokens(); ++i )
		{
			String token = tok.GetToken( i );
			if( token == TXT( "-ps4ServiceLabel" ) )
			{
				const String param = tok.GetToken( i + 1 );
				FromString< Int32 >( param, (Int32&)serviceLabel );
			}
		}
	}
	return serviceLabel;
}

static Bool GetAddcontInfoList( TDynArray< SceAppContentAddcontInfo >& outInfoList )
{
	SceNpServiceLabel serviceLabel = GetDLCServiceLabel();

	Int32 err = SCE_OK;

	Uint32 hitNum = 0;
	err = ::sceAppContentGetAddcontInfoList( serviceLabel, nullptr, 0, &hitNum );
	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("Failed to call sceAppContentGetAddcontInfoList: 0x%08X"), err );
		return false;
	}

	outInfoList.ResizeFast( hitNum );
	err = ::sceAppContentGetAddcontInfoList( serviceLabel, outInfoList.TypedData(), outInfoList.Size(), &hitNum );
	if ( err != SCE_OK )
	{
		ERR_ENGINE(TXT("Failed to call sceAppContentGetAddcontInfoList: 0x%08X"), err );
		return false;
	}

	LOG_ENGINE(TXT("GetAddcontInfoList found %u DLC packages"), hitNum );

	return true;
}

static Bool GetEntitlementKey( const SceNpServiceLabel& serviceLabel, const SceNpUnifiedEntitlementLabel& entitlementLabel, SceAppContentEntitlementKey& outKey )
{
	const SceInt32 err = ::sceAppContentGetEntitlementKey( serviceLabel, &entitlementLabel, &outKey );
	if ( err != SCE_OK )
	{
		if ( err != SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT )
		{
			ERR_ENGINE(TXT("sceAppContentGetEntitlementKey entitlementLabel=%hs returned error 0x%08X "), entitlementLabel.data, err );
		}
		else
		{
			LOG_ENGINE(TXT("sceAppContentGetEntitlementKey entitlementLabel=%hs has no entitlement (SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT)"), entitlementLabel.data );
		}

		return false;
	}

	return true;
}

Bool CDlcPackageAutoMounterThread::IsBlacklisted( const SceNpUnifiedEntitlementLabel& label ) const
{
	return m_blacklistedLabels.Exist( label );
}

Bool CDlcPackageAutoMounterThread::PopulatePackageInfos( const TDynArray< SceAppContentAddcontInfo >& infoList, TDynArray< SPackageInfo >& outPackageInfos )
{
	SceNpServiceLabel serviceLabel = GetDLCServiceLabel();
	SceInt32 err = SCE_OK;

	for ( const SceAppContentAddcontInfo& info : infoList )
	{
		if ( IsBlacklisted( info.entitlementLabel ) )
		{
			LOG_ENGINE(TXT("Skipping blacklisted entitlementLabel '%hs'"), info.entitlementLabel.data );
			continue; // try to process other packages
		}

		LOG_ENGINE(TXT("CDlcPackageAutoMounterThread: entitlementLabel '%hs', status=%ls"), info.entitlementLabel.data, GetStatusTxtForLog( info.status ));

		// Not handling SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_NO_EXTRA_DATA either
		if ( info.status != SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_INSTALLED )
		{
			LOG_ENGINE(TXT("Skipping not-ready entitlementLabel '%hs' with status"), info.entitlementLabel.data );
			continue; // try to process other packages
		}

		SceAppContentEntitlementKey key;
		const Bool isLicensed = GetEntitlementKey( serviceLabel, info.entitlementLabel, key );
		SPackagePersistentInfo* persistentInfo = m_labelPersistentInfoMap.FindPtr( info.entitlementLabel );
		if ( persistentInfo )
		{
			const SPackageInfo packageInfo( persistentInfo->m_packageID, persistentInfo->m_mountPoint.data, isLicensed );
			outPackageInfos.PushBack( Move( packageInfo ) );
			continue;
		}

		// Don't try mounting
		if ( !isLicensed )
		{
			continue; // try to process other packages
		}

		SceAppContentMountPoint mountPoint;
		err = ::sceAppContentAddcontMount( serviceLabel, &info.entitlementLabel, &mountPoint );
		if ( err != SCE_OK )
		{
			ERR_ENGINE( TXT("CDlcPackageAutoMounterThread: sceAppContentAddcontMount entitlementLabel=%hs returned error 0x%08X (%ls)"), info.entitlementLabel.data, GetMountErrTxtForLog( err ) );

			if ( err == SCE_APP_CONTENT_ERROR_MOUNT_FULL )
			{
				ERR_ENGINE(TXT("CDlcPackageAutoMounterThread: exceeded maximum mount points %u"), SCE_APP_CONTENT_ADDCONT_MOUNT_MAXNUM );
				return false;
			}

			continue; // try to process other packages
		}

		const RuntimePackageID newPackageID = Helper::AllocRuntimePackageID();
		const SPackageInfo packageInfo( newPackageID, mountPoint.data, isLicensed );
		outPackageInfos.PushBack( Move( packageInfo ) );
		m_labelPersistentInfoMap.Insert( info.entitlementLabel, SPackagePersistentInfo( mountPoint, newPackageID ) );
	}

	return true;
}

void CDlcPackageAutoMounterThread::RequestUpdate( TDynArray< SPackageInfo >* outPackageInfos, volatile Bool* outFinishedFlag )
{
	RED_FATAL_ASSERT( outPackageInfos, "No package infos!" );
	RED_FATAL_ASSERT( outFinishedFlag, "No ready flag!" );
	m_outPackageInfos = outPackageInfos;
	m_outFinishedFlag = outFinishedFlag;
	*m_outFinishedFlag = false;
	m_semaHasWork.Release();
}

void CDlcPackageAutoMounterThread::RequestExit()
{
	m_exitFlag = true;
	m_semaHasWork.Release();
}

void CDlcPackageAutoMounterThread::ThreadFunc()
{
	for(;;)
	{
		m_semaHasWork.Acquire();

		if ( m_exitFlag )
		{
			*m_outFinishedFlag = true;
			break;
		}

		TDynArray< SceAppContentAddcontInfo > additionalContentInfos;
		if ( GetAddcontInfoList( additionalContentInfos ) )
		{
			PopulatePackageInfos( additionalContentInfos, *m_outPackageInfos );
		}

		*m_outFinishedFlag = true;
	}
}

//////////////////////////////////////////////////////////////////////////
// CDlcPackageAutoMounter
//////////////////////////////////////////////////////////////////////////
CDlcPackageAutoMounter::CDlcPackageAutoMounter( const TDynArray< String >& blacklistedDlcs )
	: m_currentPacakgeIndex( 0 )
	, m_isWorkerThreadIdle( true )
	, m_isRefreshPending( false )
	, m_isRefreshRequested( false )
	, m_hasNewPackageInfos( false )
{
	m_autoMounterThread = new CDlcPackageAutoMounterThread( blacklistedDlcs );
	m_autoMounterThread->InitThread();
	m_autoMounterThread->SetAffinityMask( (1U<<4)|(1U<<5) );
}

CDlcPackageAutoMounter::~CDlcPackageAutoMounter()
{
	while ( !m_isWorkerThreadIdle )
	{
		Red::Threads::YieldCurrentThread();
	}

	m_autoMounterThread->RequestExit();
	m_autoMounterThread->JoinThread();
	delete m_autoMounterThread;
}

void CDlcPackageAutoMounter::Update()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!" );

	if ( m_isWorkerThreadIdle )
	{
		if ( m_isRefreshPending )
		{
			m_isRefreshPending = false;
			m_hasNewPackageInfos = true;

			const Uint32 updatePackageIndex = m_currentPacakgeIndex ^ 1;
			m_currentPacakgeIndex = updatePackageIndex;
		}

		if ( m_isRefreshRequested )
		{
			m_isRefreshRequested = false;
			m_isRefreshPending = true;

			const Uint32 updatePackageIndex = m_currentPacakgeIndex ^ 1;			
			TDynArray< SPackageInfo >& updatePacakgeInfos = m_packageInfos[ updatePackageIndex ];
			updatePacakgeInfos.ClearFast();
			m_autoMounterThread->RequestUpdate( &updatePacakgeInfos, &m_isWorkerThreadIdle );
		}
	}
}

Bool CDlcPackageAutoMounter::IsDirty() const
{
	return m_isRefreshRequested || m_isRefreshPending;
}

void CDlcPackageAutoMounter::RequestRefresh()
{	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!" );
	m_isRefreshRequested = true;
}

Bool CDlcPackageAutoMounter::FlushPackageInfos( TDynArray< SPackageInfo >& outPackageInfos )
{
	if ( m_hasNewPackageInfos )
	{
		m_hasNewPackageInfos = false;
		outPackageInfos = Move( m_packageInfos[m_currentPacakgeIndex] );
		return true;
	}

	return false;
}

} // namespace Helper

#ifdef RED_LOGGING_ENABLED
static const Char* GetStatusTxtForLog( SceAppContentAddcontDownloadStatus status )
{
	const Char* txt = TXT("<Unknown>");
	switch (status)
	{
	case SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_NO_EXTRA_DATA:
		txt = TXT("SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_NO_EXTRA_DATA");
		break;
	case SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_NO_IN_QUEUE:
		txt = TXT("SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_NO_IN_QUEUE");
		break;
	case SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_DOWNLOADING:
		txt = TXT("SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_DOWNLOADING");
		break;
	case SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_DOWNLOAD_SUSPENDED:
		txt = TXT("SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_DOWNLOAD_SUSPENDED");
		break;
	case SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_INSTALLED:
		txt = TXT("SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_INSTALLED");
		break;
	default:
		break;
	}

	return txt;
}

static const Char* GetMountErrTxtForLog( Int32 err )
{
	const Char* txt = TXT("<Unknown>");

	switch( err )
	{
	case SCE_APP_CONTENT_ERROR_NOT_INITIALIZED:
		txt = TXT("SCE_APP_CONTENT_ERROR_NOT_INITIALIZED");
		break;
	case SCE_APP_CONTENT_ERROR_PARAMETER:
		txt = TXT("SCE_APP_CONTENT_ERROR_PARAMETER");
		break;
	case SCE_APP_CONTENT_ERROR_BUSY:
		txt = TXT("SCE_APP_CONTENT_ERROR_BUSY");
		break;
	case SCE_APP_CONTENT_ERROR_NOT_FOUND:
		txt = TXT("SCE_APP_CONTENT_ERROR_NOT_FOUND");
		break;
	case SCE_APP_CONTENT_ERROR_MOUNT_FULL:
		txt = TXT("SCE_APP_CONTENT_ERROR_MOUNT_FULL");
		break;
	case SCE_APP_CONTENT_ERROR_INTERNAL:
		txt= TXT("SCE_APP_CONTENT_ERROR_INTERNAL");
		break;
	default:
		break;
	}

	return txt;
}
#endif // RED_LOGGING_ENABLED