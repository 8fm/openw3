/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkBankMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkBankMgr.h"
#include "AkCommon.h"
#include "AkEvent.h"
#include "AkAction.h"
#include "AkSound.h"
#include "AkAudioLibIndex.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkMonitor.h"
#include "AkActorMixer.h"
#include "AkState.h"
#include "AkBus.h"
#include "AkAuxBus.h"
#include "AkPathManager.h"
#include "AkPath.h"
#include "AudiolibDefs.h"
#include "AkStateMgr.h"
#include "AkSwitchCntr.h"
#include "AkRanSeqCntr.h"
#include "AkLayerCntr.h"
#include "AkEnvironmentsMgr.h"
#include "AkFxBase.h"
#include "AkAudioLib.h"
#include "AkAudioMgr.h"
#include "AkRTPCMgr.h"
#include "AkBusCtx.h"
#include "AkAttenuationMgr.h"
#include "AkDialogueEvent.h"
#include <AK/Tools/Common/AkSyncCaller.h>

#ifdef AK_MOTION
	#include "AkFeedbackBus.h"
	#include "AkFeedbackNode.h"
#endif // AK_MOTION

#include "AkProfile.h"
#include "AkURenderer.h"

extern AkExternalBankHandlerCallback g_pExternalBankHandlerCallback;
extern AkPlatformInitSettings g_PDSettings;
extern AkInitSettings g_settings;

#define AK_READ_BUFFER_SIZE (32*1024)	// 32k, Arbitrarily should be optimal size read and be platform specific

#define AK_BANK_DEFAULT_LIST_SIZE 10

#define HOOKEDPOOLID g_settings.uPrepareEventMemoryPoolID

#define ALLOCWITHHOOK( _size_, _Alignment_ )	AkMalign( HOOKEDPOOLID, _size_, _Alignment_ )
#define FREEWITHHOOK( _ptr_tofree_ )			AkFalign( HOOKEDPOOLID, _ptr_tofree_ )

#define REAL_HEADER_SIZE ( sizeof( AkSubchunkHeader ) + sizeof( AkBankHeader ) )

#if defined( AK_XBOX360 )
	// On Xbox 360, we load banks aligned on AK_BANK_PLATFORM_DATA_ALIGNMENT (2048)
	// to support having XMA in any bank on a sound by sound basis. When no XMA is
	// implied, it is not required.

	// Case where we have the codec ID
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID( __alignment__, __codecID__ )		\
		if ( (__codecID__) != AKCODECID_XMA )												\
		{																					\
			__alignment__ = AK_BANK_PLATFORM_DATA_NON_XMA_ALIGNMENT;						\
		};

	// Case where we don't have the Codec ID
	// If the sound is not exactly a multiple of 2048, it is not XMA,
	// and we can optimize by changing the memory alignment constraint.
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_MEDIA_SIZE( __alignment__, __mediaSize__ )	\
		if ( __mediaSize__ % AK_BANK_PLATFORM_DATA_ALIGNMENT )								\
		{																					\
			__alignment__ = AK_BANK_PLATFORM_DATA_NON_XMA_ALIGNMENT;						\
		};

	// Case where we may or may not have the Codec ID
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID_OR_MEDIA_SIZE( __alignment__, __codecID__, __mediaSize__ )	\
		if ( (__codecID__) == 0 )																					\
		{																											\
			AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_MEDIA_SIZE( (__alignment__), (__mediaSize__) )						\
		}																											\
		else																										\
		{																											\
			AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID( (__alignment__), (__codecID__) );							\
		}

#elif defined( AK_VITA )
	// On Vita, we load banks aligned on AK_BANK_PLATFORM_DATA_ALIGNMENT (256)
	// to support having ATRAC9 in any bank on a sound by sound basis. When no ATRAC9 is
	// implied, it is not required.

	#include <AK\Plugin\AkATRAC9Factory.h>

	// Case where we have the codec ID
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID( __alignment__, __codecID__ )		\
		if ( (__codecID__) != AKCODECID_ATRAC9 )											\
		{																					\
			__alignment__ = AK_BANK_PLATFORM_DATA_NON_ATRAC9_ALIGNMENT;						\
		}

	// If the sound is not exactly a multiple of 256, it is not ATRAC9,
	// and we can optimize by changing the memory alignment constraint.
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_MEDIA_SIZE( __alignment__, __mediaSize__ )	\
		if( __mediaSize__ % AK_BANK_PLATFORM_DATA_ALIGNMENT )								\
		{																					\
			__alignment__ = AK_BANK_PLATFORM_DATA_NON_ATRAC9_ALIGNMENT;						\
		}

	// Case where we may or may not have the Codec ID
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID_OR_MEDIA_SIZE( __alignment__, __codecID__, __mediaSize__ )	\
		if ( (__codecID__) == 0 )																					\
		{																											\
			AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_MEDIA_SIZE( (__alignment__), (__mediaSize__) )						\
		}																											\
		else																										\
		{																											\
			AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID( (__alignment__), (__codecID__) );							\
		}

#else
	// Nothing to do on other platforms
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID( __alignment__, __codecID__ )
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_MEDIA_SIZE( __alignment__, __mediaSize__ )
	#define AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID_OR_MEDIA_SIZE( __alignment__, __codecID__, __mediaSize__ )
#endif

AkThread CAkBankMgr::m_BankMgrThread;
AkThreadID CAkBankMgr::m_idThread = 0;

CAkUsageSlot::CAkUsageSlot( AkUniqueID in_BankID, AkMemPoolId in_memPoolId, AkInt32 in_mainRefCount, AkInt32 in_prepareEventRefCount, AkInt32 in_prepareBankRefCount )
	: m_BankID( in_BankID )
	, m_pData( NULL )
	, m_uLoadedDataSize( 0 )
    , m_memPoolId( in_memPoolId )
    , m_bIsInternalPool(false)
	, m_uNumLoadedItems( 0 )
	, m_uIndexSize( 0 )	
	, m_paLoadedMedia( NULL )
	, m_pfnBankCallback( NULL )
	, m_pCookie( NULL )
	, m_iRefCount( in_mainRefCount )
	, m_iPrepareRefCount( in_prepareEventRefCount+in_prepareBankRefCount )
	, m_bWasLoadedAsABank( in_mainRefCount != 0 )
	, m_bWasIndexAllocated( false )
	, m_bIsMediaPrepared( false )
	, m_bUsageProhibited( false )
	, m_iWasPreparedAsABankCounter( in_prepareBankRefCount )
{
}

CAkUsageSlot::~CAkUsageSlot()
{
	CheckFreeIndexArray();
}

void CAkUsageSlot::AddRef()
{
	// TODO
	// maybe we should increment only if one of the two count is non zero.
	// Which also suggest the AddRef could fail... which would be an issue too. 
	// ...kind of in the middle of some problem here...
	// Must check where the AddRefs are done.
	AkInterlockedIncrement( &m_iRefCount );
}

void CAkUsageSlot::AddRefPrepare()
{
	AkInterlockedIncrement( &m_iPrepareRefCount );
}

void CAkUsageSlot::Release( bool in_bSkipNotification )
{
	CAkBankList::Lock();

	AkInt32 iNewRefCount = AkInterlockedDecrement( &m_iRefCount );
	AKASSERT( iNewRefCount >= 0 );

	if ( iNewRefCount <= 0 )
	{
		Unload();
		g_pBankManager->UnloadMedia( this );

		if( m_iPrepareRefCount <= 0 )
		{			
			CAkBankList::Unlock();
			MONITOR_LOADEDBANK( this, true );

			if( !in_bSkipNotification )
			{
				UnloadCompletionNotification();
			}
			
			AkDelete( g_DefaultPoolId, this );
		}
		else 
		{
			CAkBankList::Unlock();
			MONITOR_LOADEDBANK( this, false );
			if( !in_bSkipNotification )
			{
				UnloadCompletionNotification();
			}
		}
	}
	else
	{
		CAkBankList::Unlock();
	}
}

// ReleasePrepare() is callon only by the Bank thread.
void CAkUsageSlot::ReleasePrepare( bool in_bIsFinal /*= false*/ )
{	
	CAkBankList::Lock();
	AKASSERT( m_iPrepareRefCount != 0 );

	AkInt32 iNewRefCount = 0;
	if( in_bIsFinal )
		m_iPrepareRefCount = 0;
	else
		iNewRefCount = AkInterlockedDecrement( &m_iPrepareRefCount );

	AKASSERT( iNewRefCount >= 0 );

	if ( iNewRefCount <= 0 )
	{
		g_pBankManager->UnPrepareMedia( this );
		if( m_iRefCount <= 0 )
		{
			g_pBankManager->m_BankList.Remove( AkBankKey(m_BankID, NULL) );
			CAkBankList::Unlock();
			RemoveContent();
			Unload();
			
			MONITOR_LOADEDBANK( this, true );

			AkDelete( g_DefaultPoolId, this );
		}
		else
		{
			CAkBankList::Unlock();
		}
	}
	else
	{
		CAkBankList::Unlock();
	}
}

void CAkUsageSlot::RemoveContent()
{
	AkListLoadedItem::Iterator iter = m_listLoadedItem.Begin();
	while( iter != m_listLoadedItem.End() )
	{
		//To avoid locking for EACH object we release, break the lock only once in a while.  
		//This will allow the audio thread to do its stuff normally.  Anyway, releasing should not be long.
		AkUInt16 uLockCount = 1;	
		CAkFunctionCritical SpaceSetAsCritical;
		while( iter != m_listLoadedItem.End() && (uLockCount & 0x00FF))
		{
			// At this point, only sounds, track and music objects are remaining
			CAkIndexable* pNode = static_cast<CAkIndexable*>(*iter);
			pNode->Release();

			++iter;
			++uLockCount;
		}
	}
	m_listLoadedItem.Term();
}

void CAkUsageSlot::Unload()
{
	if( m_pData )
	{
		AK_PERF_DECREMENT_BANK_MEMORY( m_uLoadedDataSize );
		AKASSERT( m_memPoolId != AK_INVALID_POOL_ID );
		if ( AK::MemoryMgr::GetPoolAttributes( m_memPoolId ) & AkBlockMgmtMask )
			AK::MemoryMgr::ReleaseBlock( m_memPoolId, m_pData );
		else
			AkFree( m_memPoolId, m_pData );
		// If it was an internally created pool, it should be empty now and we destroy it.
		m_pData = NULL;
		if ( m_bIsInternalPool )
		{
			AKVERIFY( AK::MemoryMgr::DestroyPool( m_memPoolId ) == AK_Success );
			m_memPoolId = AK_INVALID_POOL_ID;
		}
	}
}

void CAkUsageSlot::UnloadCompletionNotification()
{
	MONITOR_BANKNOTIF( m_BankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankUnloaded );
	// Notify user.

	if( m_pfnBankCallback )
	{
		g_pBankManager->DoCallback( m_pfnBankCallback, m_BankID, key.pInMemoryPtr, AK_Success, m_memPoolId, m_pCookie );
		m_pfnBankCallback = NULL;// to make sure the notification is sent only once
	}
}

void CAkUsageSlot::CheckFreeIndexArray()
{
	if( WasIndexAllocated() )
	{
		AkFree( g_DefaultPoolId, m_paLoadedMedia );
		WasIndexAllocated(false);
	}
	m_paLoadedMedia = NULL;
}

CAkBankMgr::CAkBankMgr()
	: m_bStopThread( false )
	, m_bIsFirstBusLoaded( false )
	, m_bFeedbackInBank( false )
	, m_bAccumulating( false )
{
	AkClearEvent( m_eventQueue );
	AkClearThread(&m_BankMgrThread);
}

CAkBankMgr::~CAkBankMgr()
{
}

AKRESULT CAkBankMgr::Init()
{
	AKASSERT( HOOKEDPOOLID == AK_INVALID_POOL_ID || (AK::MemoryMgr::GetPoolAttributes( HOOKEDPOOLID )&AkBlockMgmtMask) != AkFixedSizeBlocksMode );
	m_BankList.Init();

	AKRESULT eResult = m_BankReader.Init();
	if(eResult == AK_Success)
	{
		m_MediaHashTable.Init( g_DefaultPoolId );
		m_BankIDToFileName.Init( g_DefaultPoolId );
		eResult = m_BankQueue.Init( AK_BANK_DEFAULT_LIST_SIZE ,AK_NO_MAX_LIST_SIZE );
	}
	if(eResult == AK_Success)
	{
		eResult = StartThread();
	}

	return eResult;
}

AKRESULT CAkBankMgr::Term()
{
	while( m_BankQueue.Length() != 0 )
	{
		CAkBankMgr::AkBankQueueItem Item = m_BankQueue.First();
		m_BankQueue.RemoveFirst();

		switch ( Item.eType )
		{
		case QueueItemLoadMediaFile:
		case QueueItemLoadMediaFileSwap:
			if( Item.loadMediaFile.pszAllocatedFileName )
			{
				AkFree( g_DefaultPoolId, Item.loadMediaFile.pszAllocatedFileName );
			}
			break;
		}
	}
	m_BankQueue.Term();

	UnloadAll();

	m_BankList.Term();

	AKASSERT( m_MediaHashTable.Length() == 0 );

	m_MediaHashTable.Term();

	FlushFileNameTable();

	m_BankReader.Term();

	m_PreparationAccumulator.Term();

	return AK_Success;
}

void CAkBankMgr::FlushFileNameTable()
{
	if( m_BankIDToFileName.IsInitialized() )
	{
		for( AkIDtoStringHash::Iterator iter = m_BankIDToFileName.Begin(); iter != m_BankIDToFileName.End(); ++iter )
		{
			AKASSERT( iter.pItem->Assoc.item );
			AkFree( g_DefaultPoolId, iter.pItem->Assoc.item );
		}
	}
	m_BankIDToFileName.Term();
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkBankMgr::StartThread()
{
	if(AkIsValidThread(&m_BankMgrThread))
	{
		AKASSERT( !"Wrong thread trying to start another thread." );
		return AK_Fail;
	}

	m_bStopThread = false;

	if ( AkCreateEvent( m_eventQueue ) != AK_Success )
	{
		AKASSERT( !"Could not create event required to start BankMgr thread." );
		return AK_Fail;
	}

    AkCreateThread(	BankThreadFunc,		// Start address
					this,				// Parameter
					g_PDSettings.threadBankManager, // Thread properties 
					&m_BankMgrThread,
					"AK::BankManager" );	// Name
	if ( !AkIsValidThread(&m_BankMgrThread) )
	{
		AKASSERT( !"Could not create bank manager thread" );
		return AK_Fail;
	}
	return AK_Success;
}
//====================================================================================================
//====================================================================================================
void CAkBankMgr::StopThread()
{
	m_bStopThread = true;
	if ( AkIsValidThread( &m_BankMgrThread ) )
	{
		// stop the eventMgr thread
		AkSignalEvent( m_eventQueue );
		AkWaitForSingleThread( &m_BankMgrThread );

		AkCloseThread( &m_BankMgrThread );
	}
	AkDestroyEvent( m_eventQueue );
}
//====================================================================================================
//====================================================================================================
AK_DECLARE_THREAD_ROUTINE( CAkBankMgr::BankThreadFunc )
{
	CAkBankMgr& rThis = *AK_GET_THREAD_ROUTINE_PARAMETER_PTR(CAkBankMgr);
	m_idThread = AKPLATFORM::CurrentThread();

	while(true)
	{
		AkWaitForEvent( rThis.m_eventQueue );

		if ( rThis.m_bStopThread )
			break;
			
		// Something in the queue!
		rThis.ExecuteCommand();
	}

	AkExitThread(AK_RETURN_THREAD_OK);
}

AKRESULT CAkBankMgr::QueueBankCommand( AkBankQueueItem in_Item  )
{
	AkAutoLock<CAkLock> gate(m_queueLock);

	AKRESULT eResult = AK_Success;
	if( in_Item.callbackInfo.pfnBankCallback )
		eResult = m_CallbackMgr.AddCookie( in_Item.callbackInfo.pCookie );

	if(eResult == AK_Success)
	{
		eResult = m_BankQueue.AddLast( in_Item ) ? AK_Success : AK_Fail;
		if( in_Item.callbackInfo.pfnBankCallback && eResult != AK_Success )
			m_CallbackMgr.RemoveOneCookie( in_Item.callbackInfo.pCookie );
	}
	if(eResult == AK_Success)
	{
		switch( in_Item.eType )
		{
		case QueueItemLoad:
			MONITOR_BANKNOTIF( in_Item.load.BankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankLoadRequestReceived );
			break;

		case QueueItemUnload:
			MONITOR_BANKNOTIF( in_Item.load.BankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankUnloadRequestReceived );
			break;

		case QueueItemPrepareEvent:
			MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_PrepareEventRequestReceived, in_Item.prepare.numEvents );
			break;

		case QueueItemUnprepareEvent:
			MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_UnPrepareEventRequestReceived, in_Item.prepare.numEvents );
			break;

		case QueueItemSupportedGameSync:
			if( !in_Item.gameSync.bSupported )
			{
				MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_UnPrepareGameSyncRequested, in_Item.gameSync.uNumGameSync );
			}
			else
			{
				MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_PrepareGameSyncRequested, in_Item.gameSync.uNumGameSync );
			}
			break;

		case QueueItemUnprepareAllEvents:
			MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_ClearPreparedEventsRequested, 0 );
			break;

		case QueueItemClearBanks:
			MONITOR_BANKNOTIF( AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_ClearAllBanksRequestReceived );
			break;

		case QueueItemPrepareBank:
			MONITOR_PREPAREBANKREQUESTED(  AkMonitorData::NotificationReason_PrepareBankRequested, in_Item.bankPreparation.BankID, in_Item.bankPreparation.uFlags );
			break;

		case QueueItemUnprepareBank:
			MONITOR_PREPAREBANKREQUESTED(  AkMonitorData::NotificationReason_UnPrepareBankRequested, in_Item.bankPreparation.BankID, in_Item.bankPreparation.uFlags );
			break;
		}

		AkSignalEvent( m_eventQueue );
	}

	return eResult;
}

AKRESULT CAkBankMgr::ExecuteCommand()
{
	AKRESULT eResult = AK_Success;
	while( true )
	{
		m_queueLock.Lock();
		if( m_BankQueue.Length() )
		{
			CAkBankMgr::AkBankQueueItem Item = m_BankQueue.First();
			m_BankQueue.RemoveFirst();
			m_queueLock.Unlock();

			switch ( Item.eType )
			{
			case QueueItemLoad:
				eResult = LoadBankPre( Item );
				break;

			case QueueItemUnload:
				eResult = UnloadBankPre( Item );
				break;

			case QueueItemPrepareEvent:
				eResult = PrepareEvents( Item );
				break;

			case QueueItemUnprepareEvent:
				eResult = UnprepareEvents( Item );
				break;

			case QueueItemSupportedGameSync:
				eResult = PrepareGameSync( Item );
				break;

			case QueueItemUnprepareAllEvents:
				eResult = UnprepareAllEvents( Item );
				break;

			case QueueItemClearBanks:
				eResult = ClearBanksInternal( Item );
				break;

			case QueueItemPrepareBank:
				eResult = PrepareBank( Item );
				break;

			case QueueItemUnprepareBank:
				eResult = UnPrepareBank( Item );
				break;

			case QueueItemLoadMediaFile:
				eResult = LoadMediaFile( Item );
				break;

			case QueueItemUnloadMediaFile:
				eResult = UnloadMediaFile( Item );
				break;

			case QueueItemLoadMediaFileSwap:
				eResult = MediaSwap( Item );
				break;
			}
		}
		else //Yes, the queue may be empty even if the semaphore was signaled, because a cancellation may have occurred
		{
			m_queueLock.Unlock();
			break; //exit while( true ) loop
		}
	}

	return eResult;
}

void CAkBankMgr::NotifyCompletion( AkBankQueueItem & in_rItem, AKRESULT in_OperationResult )
{
	AkMemPoolId memPoolID = AK_INVALID_POOL_ID;
	AkUniqueID itemID = AK_INVALID_UNIQUE_ID;
	switch( in_rItem.eType )
	{
	case QueueItemLoad:
	case QueueItemUnload:
		memPoolID = in_rItem.load.memPoolId;
		itemID = in_rItem.load.BankID;
		break;

	case QueueItemSupportedGameSync:
		// nothing
		break;

	case QueueItemPrepareBank:
	case QueueItemUnprepareBank:
		itemID = in_rItem.bankPreparation.BankID;
		break;

	default:
		itemID = (in_rItem.prepare.numEvents == 1) ? in_rItem.prepare.eventID : AK_INVALID_UNIQUE_ID;
		break;
	}

	DoCallback( in_rItem.callbackInfo.pfnBankCallback, itemID, in_rItem.GetFromMemPtr(), in_OperationResult, memPoolID, in_rItem.callbackInfo.pCookie );
}

AKRESULT CAkBankMgr::SetFileReader( AkFileID in_FileID, AkUInt32 in_uFileOffset, AkUInt32 in_codecID, void * in_pCookie, bool in_bIsLanguageSpecific /*= true*/ )
{
	// We are in a Prepare Event or Prepare Bank or Load Bank by String.

	if( in_uFileOffset != 0 || in_codecID == AKCODECID_BANK )
	{
		// If we find the bank name, we use the string, otherwise, we use the file ID directly.
		char** ppString = m_BankIDToFileName.Exists( in_FileID );
		if( ppString )
		{
			AKASSERT( *ppString );
			return m_BankReader.SetFile( *ppString, in_uFileOffset, in_pCookie );
		}
		else
		{
			return m_BankReader.SetFile( in_FileID, in_uFileOffset, AKCODECID_BANK, in_pCookie );
		}
	}
	else
	{
		AKASSERT( in_codecID != AKCODECID_BANK );
		return m_BankReader.SetFile( in_FileID, in_uFileOffset, in_codecID, in_pCookie, in_bIsLanguageSpecific );
	}
}

const char* CAkBankMgr::GetBankFileName( AkBankID in_bankID )
{
	char** ppString = m_BankIDToFileName.Exists( in_bankID );

	if( ppString )
		return *ppString;

	return NULL;
}

AKRESULT CAkBankMgr::LoadSoundFromFile( AkSrcTypeInfo& in_rMediaInfo, AkUInt8* io_pData )
{
	m_BankReader.Reset();
	
	AKRESULT eResult = SetFileReader( in_rMediaInfo.mediaInfo.uFileID, in_rMediaInfo.mediaInfo.uFileOffset, CODECID_FROM_PLUGINID( in_rMediaInfo.dwID ), NULL, in_rMediaInfo.mediaInfo.bIsLanguageSpecific );

	AkUInt32 uReadSize = 0;
	if( eResult == AK_Success )
	{
		eResult = m_BankReader.FillData( io_pData, in_rMediaInfo.mediaInfo.uInMemoryMediaSize, uReadSize
#if defined AK_WII_FAMILY
			,true // flush cache range on Wii.
#endif
			);
	}
	if( eResult == AK_Success && in_rMediaInfo.mediaInfo.uInMemoryMediaSize != uReadSize )
		eResult = AK_Fail;

	m_BankReader.CloseFile();

	return eResult;
}

AKRESULT CAkBankMgr::LoadBank( AkBankQueueItem in_Item, CAkUsageSlot *& out_pUsageSlot, AkLoadBankDataMode in_eLoadMode, bool in_bIsFromPrepareBank )
{
	AkUniqueID bankID = in_Item.load.BankID;

	CAkBankList::Lock();

	CAkUsageSlot* pSlot = m_BankList.Get( in_Item.GetBankKey() );
	if( pSlot && pSlot->WasLoadedAsABank() )
	{
		CAkBankList::Unlock();
		AKASSERT( in_eLoadMode == AkLoadBankDataMode_OneBlock );// If the call is not preparing media, then it should not pass here.
		MONITOR_BANKNOTIF( bankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankAlreadyLoaded );
		return AK_BankAlreadyLoaded;
	}

	bool l_LoadingOnlyMedia = false;

	bool bDone = false;

	AKRESULT eResult = AK_Success;
	CAkBankList::Unlock();
	if( pSlot == NULL )
	{
		AkInt32 uMainRefCount = 0;
		AkInt32 uPrepareEventRefCount = 0;
		AkInt32 uPrepareBankRefCount = 0;
		AkMemPoolId memPoolID = g_settings.uPrepareEventMemoryPoolID;

		if( in_eLoadMode == AkLoadBankDataMode_OneBlock )
		{
			uMainRefCount = 1;
			memPoolID = in_Item.load.memPoolId;
		}
		else if( in_bIsFromPrepareBank )
		{
			uPrepareBankRefCount=1;
		}
		else
		{
			uPrepareEventRefCount = 1;
		}

		out_pUsageSlot = AkNew( g_DefaultPoolId, CAkUsageSlot( bankID, memPoolID, uMainRefCount, uPrepareEventRefCount, uPrepareBankRefCount ) );

		if ( !out_pUsageSlot )
		{
			eResult = AK_InsufficientMemory;
		}
	}
	else
	{
		out_pUsageSlot = pSlot;
		if( in_bIsFromPrepareBank )
		{
			out_pUsageSlot->WasPreparedAsABank( true );
		}
	}

	m_BankReader.Reset();

	bool bIsLoadedFromMemory = in_Item.bankLoadFlag == AkBankLoadFlag_InMemory;

	if(eResult == AK_Success)
	{
		switch( in_eLoadMode )
		{
		case AkLoadBankDataMode_OneBlock:
			if( in_Item.bankLoadFlag == AkBankLoadFlag_UsingFileID )
			{
				eResult = m_BankReader.SetFile( bankID, 0, AKCODECID_BANK, in_Item.callbackInfo.pCookie );
				break;
			}
			else if( bIsLoadedFromMemory )
			{
				eResult = m_BankReader.SetFile( in_Item.load.pInMemoryBank, in_Item.load.ui32InMemoryBankSize );
				break;
			}
			// No break here intentionnal, default to SetFileReader(

		default:
			eResult = SetFileReader( bankID, 0, AKCODECID_BANK, in_Item.callbackInfo.pCookie );
			break;
		}
	}
	
	// Getting the bank information from its header
    AkBankHeader BankHeaderInfo = {0};
	if(eResult == AK_Success)
	{
		eResult = ProcessBankHeader( BankHeaderInfo );
	}

	// Clearing the Master busses.
	// Why:
	//    The init bank contains the master bus, but it is possible that another
	//    temporary master bus has been added if Wwise has been connected prior to this init bank load bank.
	//    The ID of the Master bus wouldn't match, and the results would be that global events and information about master bus volume and more be wrong.	
#ifdef AK_MOTION
	CAkFeedbackBus* pOldFeedbackMasterBus = NULL;
	if (!m_bIsFirstBusLoaded)
		pOldFeedbackMasterBus = CAkFeedbackBus::ClearTempMasterBus();
#endif // AK_MOTION

	// Loading the Bank
	while (!bDone && (eResult == AK_Success))
	{
		AkSubchunkHeader SubChunkHeader;

		AkUInt32 ulTotalRead = 0;
		AkUInt32 ulReadBytes = 0;

		eResult = m_BankReader.FillData(&SubChunkHeader, sizeof(SubChunkHeader), ulReadBytes);
		ulTotalRead += ulReadBytes;
		if(eResult != AK_Success)
		{
			break;
		}
		if(ulTotalRead == sizeof(SubChunkHeader))
		{
			switch( SubChunkHeader.dwTag )
			{
			case BankDataChunkID:
				switch( in_eLoadMode )
				{
				case AkLoadBankDataMode_Structure:
					m_BankReader.Skip(SubChunkHeader.dwChunkSize, ulReadBytes);
					if(ulReadBytes != SubChunkHeader.dwChunkSize)
					{
						eResult = AK_InvalidFile;
						bDone = true;
					}
					break;

				case AkLoadBankDataMode_MediaAndStructure:
				case AkLoadBankDataMode_Media:
					AKASSERT( bIsLoadedFromMemory == false );

					eResult = PrepareMedia( 
						out_pUsageSlot, 
						SubChunkHeader.dwChunkSize // required to skip if already loaded...
						);
					break;

				case AkLoadBankDataMode_OneBlock:
					AkUInt8* pDataBank = NULL;
					if( bIsLoadedFromMemory )
					{
						//Bank loaded from memory
						pDataBank = (AkUInt8*)m_BankReader.GetData( SubChunkHeader.dwChunkSize );
						m_BankReader.ReleaseData();
					}
					else
					{
						//Bank loaded from file
						eResult = ProcessDataChunk( SubChunkHeader.dwChunkSize, out_pUsageSlot );
						pDataBank = out_pUsageSlot->m_pData;
					}

					if( eResult == AK_Success )
					{
						eResult = LoadMedia( 
							pDataBank, 
							out_pUsageSlot, 
							bIsLoadedFromMemory
							);
					}
					break;
				}
				break;

			case BankDataIndexChunkID:
				switch( in_eLoadMode )
				{
				case AkLoadBankDataMode_Structure:
					m_BankReader.Skip(SubChunkHeader.dwChunkSize, ulReadBytes);
					if( ulReadBytes != SubChunkHeader.dwChunkSize )
					{
						eResult = AK_InvalidFile;
						bDone = true;
					}
					break;

				case AkLoadBankDataMode_MediaAndStructure:
				case AkLoadBankDataMode_Media:
					AKASSERT( bIsLoadedFromMemory == false );
					// No break here normal.
				case AkLoadBankDataMode_OneBlock:
					eResult = LoadMediaIndex( out_pUsageSlot, SubChunkHeader.dwChunkSize, bIsLoadedFromMemory );
					break;
				}
				break;

			case BankHierarchyChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessHircChunk( out_pUsageSlot, bankID );
				break;

			case BankStrMapChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessStringMappingChunk( SubChunkHeader.dwChunkSize, out_pUsageSlot );
				break;

			case BankStateMgrChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessGlobalSettingsChunk( SubChunkHeader.dwChunkSize );
				break;

			case BankEnvSettingChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessEnvSettingsChunk( SubChunkHeader.dwChunkSize );
				break;

			default:
				AKASSERT(!"Unknown Bank chunk for this Reader version, it will be ignored");
				//Skip it
				m_BankReader.Skip(SubChunkHeader.dwChunkSize, ulReadBytes);
				if(ulReadBytes != SubChunkHeader.dwChunkSize)
				{
					eResult = AK_InvalidFile;
					bDone = true;
				}
				break;

			}
		}
		else
		{
			if(ulReadBytes != 0)
			{
				AKASSERT(!"Should not happen on a valid file");
				eResult = AK_InvalidFile;
			}
			bDone = true;
		}
	}

#ifdef AK_MOTION
	// If a temporary bus was set for a device from Wwise but there wasn't any of that type loaded in the bank, 
	// set the old (from Wwise).
	if(pOldFeedbackMasterBus != NULL)
	{
		CAkFeedbackBus* pMotionBus = CAkFeedbackBus::GetMasterMotionBusAndAddRef();
		if (pMotionBus == NULL)
			CAkFeedbackBus::ResetMasterBus(pOldFeedbackMasterBus);
		else
			pMotionBus->Release();

		pOldFeedbackMasterBus->Release();
	}
#endif // AK_MOTION

	m_BankReader.CloseFile();

	AkMonitorData::NotificationReason Reason;
	AkUniqueID l_langageID = AK_INVALID_UNIQUE_ID;
	if(eResult == AK_Success)
	{
		Reason = AkMonitorData::NotificationReason_BankLoaded;
		l_langageID = BankHeaderInfo.dwLanguageID;
	}
	else
	{
		Reason = AkMonitorData::NotificationReason_BankLoadFailed;
		MONITOR_ERROR( AK::Monitor::ErrorCode_BankLoadFailed );
	}
	MONITOR_BANKNOTIF( bankID, l_langageID, Reason );

	return eResult;
}

AKRESULT CAkBankMgr::LoadBankPre( AkBankQueueItem& in_rItem )
{
	AKRESULT eLoadResult = AK_Success;

	CAkUsageSlot * pUsageSlot = NULL;
	AKRESULT eResult = LoadBank( in_rItem, pUsageSlot, AkLoadBankDataMode_OneBlock, false );
	if( eResult == AK_BankAlreadyLoaded )
	{
		eResult = AK_Success;
		eLoadResult = AK_BankAlreadyLoaded;
	}
	else if ( eResult == AK_Success )
	{
		pUsageSlot->WasLoadedAsABank( true );

		m_BankList.Set( in_rItem.GetBankKey(), pUsageSlot );
	}

	MONITOR_LOADEDBANK( pUsageSlot, false );

	if( eResult != AK_Success )
	{
		// Here, should not pass the notification flag.
		if ( pUsageSlot )
		{
			pUsageSlot->RemoveContent();
			pUsageSlot->Release( true );
		}

		eLoadResult = eResult;
	}

	// Notify user.
	NotifyCompletion( in_rItem, eLoadResult );

	return eResult;
}

AKRESULT CAkBankMgr::UnloadBankPre( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

	CAkBankList::Lock();

	AkBankKey bankKey = in_Item.GetBankKey();

	CAkUsageSlot * pUsageSlot = m_BankList.Get( bankKey );
	if( pUsageSlot )
	{
		if( pUsageSlot->WasLoadedAsABank() )
		{
			m_BankList.Remove( bankKey );
			CAkBankList::Unlock();
			eResult = KillSlot(pUsageSlot, in_Item.callbackInfo.pfnBankCallback, in_Item.callbackInfo.pCookie);
		}
		else
		{
			CAkBankList::Unlock();
			eResult = AK_Fail;

			// Notify user.
			NotifyCompletion( in_Item, eResult );
		}
	}
	else // not found
	{
		CAkBankList::Unlock();
		eResult = AK_UnknownBankID;

		MONITOR_ERRORMSG( AKTEXT("Unload bank failed, requested bank was not found.") );

		// Notify user.
		NotifyCompletion( in_Item, eResult );
	}

	return eResult;
}

AKRESULT CAkBankMgr::ClearBanksInternal( AkBankQueueItem in_Item )
{
	int i = 0;
	CAkUsageSlot** l_paSlots = NULL;
	{
		AkAutoLock<CAkBankList> BankListGate( m_BankList );

		CAkBankList::AkListLoadedBanks& rBankList = m_BankList.GetUNSAFEBankListRef();

		AkUInt32 l_ulNumBanks = rBankList.Length();
		
		if( l_ulNumBanks )
		{
			l_paSlots = (CAkUsageSlot**)AkAlloca( l_ulNumBanks * sizeof( CAkUsageSlot* ) );
			{
				CAkBankList::AkListLoadedBanks::IteratorEx iter = rBankList.BeginEx();
				while(iter != rBankList.End() )
				{
					if( (*iter)->WasLoadedAsABank() )
					{
						l_paSlots[i] = *iter;
						++i;
						iter = rBankList.Erase(iter);
					}
					else
						++iter;
				}
			}
		}
	}

	i--;

	// unloading in reverse order
	while ( i >= 0 )
	{
		CAkUsageSlot * pUsageSlot = l_paSlots[i];
		AK::SoundEngine::AkBankSyncLoader syncLoader;
		AKRESULT eResult = syncLoader.Init();
		AKASSERT( eResult == AK_Success );

		eResult = m_CallbackMgr.AddCookie( &syncLoader );// If the cookie fails to be added, the system will not wait for completion
		if (eResult == AK_Success)
			eResult = KillSlot(pUsageSlot, AK::SoundEngine::DefaultBankCallbackFunc, &syncLoader);

		syncLoader.Wait( eResult );
		i--;
	}

	NotifyCompletion( in_Item, AK_Success );

	return AK_Success;
}

AKRESULT CAkBankMgr::PrepareEvents( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( in_Item.prepare.numEvents );

	EnableAccumulation();

	if( in_Item.prepare.numEvents == 1 )
	{
		eResult = PrepareEvent( in_Item, in_Item.prepare.eventID );

		if( eResult == AK_Success )
		{
			eResult = ProcessAccumulated();
			
			if( eResult != AK_Success )
			{
				UnprepareEvent( in_Item.prepare.eventID );
			}
		}

		if( eResult == AK_Success )
		{
			MONITOR_PREPAREEVENTNOTIF( AkMonitorData::NotificationReason_EventPrepareSuccess, in_Item.prepare.eventID );
		}
		else
		{
			MONITOR_PREPAREEVENTNOTIF( AkMonitorData::NotificationReason_EventPrepareFailure, in_Item.prepare.eventID );
		}
	}
	else
	{
		// Multiple events to prepare
		AKASSERT( in_Item.prepare.pEventID );

		for( AkUInt32 i = 0; i < in_Item.prepare.numEvents; ++i )
		{
			eResult = PrepareEvent( in_Item, in_Item.prepare.pEventID[i] );
			if( eResult != AK_Success )
			{
				while( i > 0 )
				{
					--i;
					UnprepareEvent( in_Item.prepare.pEventID[i] );
				}
				break;
			}
		}

		if( eResult == AK_Success )
		{
			eResult = ProcessAccumulated();
			
			if( eResult != AK_Success )
			{
				// Handle Error, revert prepared nodes.
				for( AkUInt32 i = 0; i < in_Item.prepare.numEvents; ++i )
				{
					UnprepareEvent( in_Item.prepare.pEventID[i] );
				}
			}
		}

		AkMonitorData::NotificationReason eReason = eResult == AK_Success ? AkMonitorData::NotificationReason_EventPrepareSuccess : AkMonitorData::NotificationReason_EventPrepareFailure;
		for( AkUInt32 i = 0; i < in_Item.prepare.numEvents; ++i )
		{
			MONITOR_PREPAREEVENTNOTIF( eReason, in_Item.prepare.pEventID[i] );
		}

		AkFree( g_DefaultPoolId, in_Item.prepare.pEventID );
		in_Item.prepare.pEventID = NULL;
	}

	DisableAccumulation();

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::PrepareEvent( AkBankQueueItem in_Item, AkUniqueID in_EventID )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( g_pIndex );
	CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_EventID );
	if( pEvent )
	{
		if( !pEvent->IsPrepared() )
		{
			for( CAkEvent::AkActionList::Iterator iter = pEvent->m_actions.Begin(); iter != pEvent->m_actions.End(); ++iter )
			{
				CAkAction* pAction = *iter;
				AkActionType aType = pAction->ActionType();
				if( aType == AkActionType_Play )
				{
					CAkActionPlay* pActionPlay = (CAkActionPlay*)pAction;
					eResult = PrepareBankInternal( in_Item, pActionPlay->GetFileID(), AkLoadBankDataMode_Structure );

					if( eResult == AK_Success )
					{
						eResult = CAkParameterNodeBase::PrepareNodeData( pActionPlay->ElementID() );
						if( eResult != AK_Success )
						{
							UnPrepareBankInternal( pActionPlay->GetFileID() );
						}
					}
					if( eResult != AK_Success )
					{
						// Iterate to undo the partial prepare
						for( CAkEvent::AkActionList::Iterator iterFlush = pEvent->m_actions.Begin(); iterFlush != iter; ++iterFlush )
						{
							pAction = *iterFlush;
							AkActionType aType = pAction->ActionType();
							if( aType == AkActionType_Play )
							{
								pActionPlay = (CAkActionPlay*)pAction;
								CAkParameterNodeBase::UnPrepareNodeData( pActionPlay->ElementID() );
								UnPrepareBankInternal( pActionPlay->GetFileID() );
							}
						}
						break;
					}
				}
			}
			// If successfully prepared, we increment the refcount
			if( eResult == AK_Success )
			{
				AK_PERF_INCREMENT_PREPARE_EVENT_COUNT();
				pEvent->AddRef();
			}
		}

		if( eResult == AK_Success )
			pEvent->IncrementPreparedCount();

		MONITOR_EVENTPREPARED( pEvent->ID(), pEvent->GetPreparationCount() );

		pEvent->Release();
	}
	else
	{
		eResult = AK_IDNotFound;
	}
	return eResult;
}

AKRESULT CAkBankMgr::UnprepareEvents( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( in_Item.prepare.numEvents );

	if( in_Item.prepare.numEvents == 1 )
	{
		eResult = UnprepareEvent( in_Item.prepare.eventID );
	}
	else
	{
		// Multiple events to prepare
		AKASSERT( in_Item.prepare.pEventID );

		for( AkUInt32 i = 0; i < in_Item.prepare.numEvents; ++i )
		{
			eResult = UnprepareEvent( in_Item.prepare.pEventID[i] );
			if( eResult != AK_Success ) 
				break;
		}

		AkFree( g_DefaultPoolId, in_Item.prepare.pEventID );
		in_Item.prepare.pEventID = NULL;
	}

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::UnprepareAllEvents( AkBankQueueItem in_Item )
{
	ClearPreparedEvents();
	
	// Notify user.
	NotifyCompletion( in_Item, AK_Success );

	return AK_Success;
}

AKRESULT CAkBankMgr::UnprepareEvent( AkUniqueID in_EventID )
{
	AKRESULT eResult = AK_Success;

	CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_EventID );
	if( pEvent )
	{
		UnprepareEvent( pEvent );
		pEvent->Release();
	}
	else
	{
		eResult = AK_IDNotFound;
	}

	MONITOR_PREPAREEVENTNOTIF( 
		eResult == AK_Success ? AkMonitorData::NotificationReason_EventUnPrepareSuccess : AkMonitorData::NotificationReason_EventUnPrepareFailure, 
		in_EventID );

	return eResult;
}

void CAkBankMgr::UnprepareEvent( CAkEvent* in_pEvent, bool in_bCompleteUnprepare /*= false*/ )
{
	if( in_pEvent->IsPrepared() )
	{
		if( in_bCompleteUnprepare )
		{
			in_pEvent->FlushPreparationCount();
		}
		else
		{
			in_pEvent->DecrementPreparedCount();
		}

		if( !in_pEvent->IsPrepared() )// must check again as DecrementPreparedCount() just changed it.
		{
			AK_PERF_DECREMENT_PREPARE_EVENT_COUNT();
			CAkEvent::AkActionList::Iterator iter = in_pEvent->m_actions.Begin();
			while( iter != in_pEvent->m_actions.End() )
			{
				CAkAction* pAction = *iter;
				++iter;// incrementing iter BEFORE unpreparing hirarchy and releasing the event.

				AkActionType aType = pAction->ActionType();
				if( aType == AkActionType_Play )
				{
					CAkParameterNodeBase::UnPrepareNodeData( ((CAkActionPlay*)(pAction))->ElementID() );
					UnPrepareBankInternal( ((CAkActionPlay*)(pAction))->GetFileID() );
				}
			}
			// If successfully unprepared, we increment decrement the refcount.
			in_pEvent->Release();
		}
	}
	MONITOR_EVENTPREPARED( in_pEvent->ID(), in_pEvent->GetPreparationCount() );
}

static void NotifyPrepareGameSync( AkPrepareGameSyncQueueItem in_Item, AKRESULT in_Result )
{
	AkMonitorData::NotificationReason reason;
	if( in_Result == AK_Success )
	{
		if( in_Item.bSupported )
			reason = AkMonitorData::NotificationReason_PrepareGameSyncSuccess;
		else
			reason = AkMonitorData::NotificationReason_UnPrepareGameSyncSuccess;
	}
	else
	{
		if( in_Item.bSupported )
			reason = AkMonitorData::NotificationReason_PrepareGameSyncFailure;
		else
			reason = AkMonitorData::NotificationReason_UnPrepareGameSyncFailure;
	}
	MONITOR_PREPAREGAMESYNCNOTIF( reason, in_Item.uGameSyncID, in_Item.uGroupID, in_Item.eGroupType );
}

AKRESULT CAkBankMgr::PrepareGameSync( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

#ifndef AK_OPTIMIZED
	if( !g_settings.bEnableGameSyncPreparation )
	{
		MONITOR_ERRORMSG( AKTEXT("Unexpected call to PrepareGameSyncs. ")
			AKTEXT("See: \"bEnableGameSyncPreparation\" parameter in AkInitSettings for more information") );
	}
#endif

	if( in_Item.gameSync.bSupported )//do not accumulate when unloading game sync.
		EnableAccumulation();

	if( in_Item.gameSync.uNumGameSync == 1 )
	{
		eResult = g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.uGameSyncID, in_Item.gameSync.bSupported );
		NotifyPrepareGameSync( in_Item.gameSync, eResult );

		if( in_Item.gameSync.bSupported &&eResult == AK_Success )
		{
			eResult = ProcessAccumulated();
			
			if( eResult != AK_Success )
			{
				// Handle Error, revert prepared nodes.
				g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.uGameSyncID, false );
			}
		}
	}
	else
	{
		AKASSERT( in_Item.gameSync.uNumGameSync && in_Item.gameSync.pGameSyncID );

		for( AkUInt32 i = 0; i < in_Item.gameSync.uNumGameSync; ++i )
		{
			eResult = g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.pGameSyncID[i], in_Item.gameSync.bSupported );
			if( eResult != AK_Success )
			{
				// Rollback if was adding content and one failed
				AKASSERT( in_Item.gameSync.bSupported );// Must not fail when unsupporting

				for( AkUInt32 k = 0; k < i; ++k )
				{
					g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.pGameSyncID[k], false );
				}
				break;
			}
			NotifyPrepareGameSync( in_Item.gameSync, eResult );

		}

		if( in_Item.gameSync.bSupported && eResult == AK_Success )
		{
			eResult = ProcessAccumulated();
			
			if( eResult != AK_Success )
			{
				// Handle Error, revert prepared nodes.
				for( AkUInt32 i = 0; i < in_Item.gameSync.uNumGameSync; ++i )
				{
					g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.pGameSyncID[i], false );
				}
			}
		}

		// flush data if in_Item.gameSync.uNumGameSync greater than 1
		AkFree( g_DefaultPoolId, in_Item.gameSync.pGameSyncID );
	}

	if( in_Item.gameSync.bSupported )//do not accumulate when unloading game sync.
		DisableAccumulation();

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::PrepareBank( AkBankQueueItem in_Item )
{
	AkUniqueID BankID	= in_Item.bankPreparation.BankID;

	AkLoadBankDataMode loadMode = in_Item.bankPreparation.uFlags == AK::SoundEngine::AkBankContent_All ? 
							AkLoadBankDataMode_MediaAndStructure : AkLoadBankDataMode_Structure;

	AKRESULT eResult = PrepareBankInternal( in_Item, BankID, loadMode, true );

	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::UnPrepareBank( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

	AkUniqueID BankID	= in_Item.bankPreparation.BankID;

	UnPrepareBankInternal( BankID, true );

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::LoadMediaFile( char* in_pszFileName, AkAlignedPtrSet& out_AlignedPtrSet )
{
	AKRESULT eResult = AK_Success;

#ifndef PROXYCENTRAL_CONNECTED

	// Clean out params.
	out_AlignedPtrSet.pAllocated = NULL;
	out_AlignedPtrSet.pAlignedPtr = NULL;
	out_AlignedPtrSet.uMediaSize = 0;

	// Create the stream.
    AkFileSystemFlags fsFlags;
    fsFlags.uCompanyID = AKCOMPANYID_AUDIOKINETIC;
    fsFlags.uCodecID = AKCODECID_BANK;
    fsFlags.pCustomParam = NULL;
    fsFlags.uCustomParamSize = 0;
	fsFlags.bIsLanguageSpecific = false;
	fsFlags.bIsFromRSX = false;

	AK::IAkStdStream * pStream = NULL;

	AkOSChar * pszFileName;
	CONVERT_CHAR_TO_OSCHAR( in_pszFileName, pszFileName );
	eResult =  AK::IAkStreamMgr::Get()->CreateStd( 
                                    pszFileName, 
                                    &fsFlags,
                                    AK_OpenModeRead,
                                    pStream,
									true );	// Force synchronous open.

	if( eResult != AK_Success )
	{
		return eResult;
	}

	// Optional: Set the name of the Stream for the profiling.
#ifndef AK_OPTIMIZED
	pStream->SetStreamName( pszFileName );
#endif

	// Call GetInfo(...) on the stream to get its size and some more...
	AkStreamInfo info;
	pStream->GetInfo( info );

	// Allocate the buffer.

	// Check the size is not NULL.
	if( info.uSize != 0 )
	{
		out_AlignedPtrSet.uMediaSize = (AkUInt32)info.uSize;
		// Use the external memory allocator.
		// Respect the alignment on allocation.
		// Allocate more to allow memory alignment.
		out_AlignedPtrSet.pAllocated = (AkUInt8*)(AK::AllocHook( (size_t)info.uSize + AK_BANK_PLATFORM_DATA_ALIGNMENT ));
		if( out_AlignedPtrSet.pAllocated )
		{
			// Align allocated ptr.
			out_AlignedPtrSet.pAlignedPtr = out_AlignedPtrSet.pAllocated + AK_BANK_PLATFORM_DATA_ALIGNMENT - 1;
			out_AlignedPtrSet.pAlignedPtr -= ((AkUIntPtr)(out_AlignedPtrSet.pAlignedPtr))%AK_BANK_PLATFORM_DATA_ALIGNMENT;

			// Read the data, blocking:

			AkUInt32 ulActualSizeRead;
			eResult =  pStream->Read( out_AlignedPtrSet.pAlignedPtr, 
										(AkUInt32)info.uSize, 
										true, 
										AK_DEFAULT_BANK_IO_PRIORITY,
										( info.uSize / AK_DEFAULT_BANK_THROUGHPUT ), // deadline (s) = size (bytes) / throughput (bytes/s).
										ulActualSizeRead );

			if ( eResult == AK_Success && pStream->GetStatus() == AK_StmStatusCompleted )
			{
				out_AlignedPtrSet.uMediaSize = ulActualSizeRead;
				// Make sure we read the expected size.
				AKASSERT( ulActualSizeRead == info.uSize );
			}
			else
			{
				AK::FreeHook( out_AlignedPtrSet.pAllocated );
				out_AlignedPtrSet.pAllocated = NULL;
				out_AlignedPtrSet.pAlignedPtr = NULL;
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}
	else
	{
		// No pain no gain.
		eResult = AK_InvalidFile;
	}

	if( pStream )
		pStream->Destroy();

#endif

	return eResult;
}

AKRESULT CAkBankMgr::LoadMediaFile( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;
#ifndef PROXYCENTRAL_CONNECTED
	{// Bracket to release lock before notification
		AkAutoLock<CAkLock> gate( m_MediaLock );

		AkUniqueID sourceID = in_Item.loadMediaFile.MediaID;

		AkMediaEntry* pMediaEntry = m_MediaHashTable.Exists( sourceID );
		if( pMediaEntry )
		{
			pMediaEntry->AddRef();
		}
		else
		{
			// Add the missing entry
			pMediaEntry = m_MediaHashTable.Set( sourceID );
			if( !pMediaEntry )
			{
				eResult = AK_InsufficientMemory;
			}
			else
			{
				pMediaEntry->SetSourceID( sourceID );

				AkAlignedPtrSet mediaPtrs;
				{
					m_MediaLock.Unlock(); // no need to keep the lock during the IO. pMediaEntry is safe as it is already addref'ed at this point.
					eResult = LoadMediaFile( in_Item.loadMediaFile.pszAllocatedFileName, mediaPtrs );
					m_MediaLock.Lock();
				}

				if( eResult == AK_Success )
				{
					AKASSERT( mediaPtrs.pAlignedPtr != NULL );
					pMediaEntry->SetPreparedData( mediaPtrs.pAlignedPtr, mediaPtrs.uMediaSize, mediaPtrs.pAllocated );
				}
				else
				{
					if( pMediaEntry->Release() == 0 ) // if the memory was released
					{
						m_MediaHashTable.Unset( sourceID );
					}
				}
			}
		}
	}

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	// Free the file name that was in the queue, we are responsible to free it.
	if( in_Item.loadMediaFile.pszAllocatedFileName )
	{
		AkFree( g_DefaultPoolId, in_Item.loadMediaFile.pszAllocatedFileName );
		in_Item.loadMediaFile.pszAllocatedFileName = NULL;
	}
#endif
	return eResult;
}

AKRESULT CAkBankMgr::UnloadMediaFile( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;
	AkAutoLock<CAkLock> gate( m_MediaLock );

	AkUniqueID sourceID = in_Item.loadMediaFile.MediaID;

	AkMediaEntry* pMediaEntry = m_MediaHashTable.Exists( sourceID );
	if( pMediaEntry )
	{
		if( pMediaEntry->Release() == 0 ) // if the memory was released
		{
			m_MediaHashTable.Unset( sourceID );
		}
		else
		{
			// Our target was to unload the media, we failed as it was most likely in use.
			// returning an error code will allow Wwise to re-try later
			eResult = AK_Fail;
		}
	}

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::MediaSwap( AkBankQueueItem in_Item )
{
	// Here is the short story on how the powerswap should work.
	// Pseudo code:
	//
	// result = UnloadMedia()
	// if( result == success )
	// {
	//   LoadMedia
	// }
	// else
	// {
	//     This is the tricky part, here we want to perform the Hot Swap.
	//		Re-addref the media
	//     if the addref fail, that mean the data was free, now load it again.
	//     if the addref succeed, Perform the HotSwap

	// }

	AKRESULT eResult = AK_Success;
#ifndef PROXYCENTRAL_CONNECTED
	{// Bracket to release lock before notification
		AkAutoLock<CAkLock> gate( m_MediaLock );

		AkUniqueID sourceID = in_Item.loadMediaFile.MediaID;

		AkMediaEntry* pMediaEntry = m_MediaHashTable.Exists( sourceID );
		if( pMediaEntry )
		{
			if( pMediaEntry->Release() == 0 ) // if the memory was released
			{
				m_MediaHashTable.Unset( sourceID );
			}
			else
			{
				// Our target was to unload the media, we failed as it was most likely in use.
				// returning an error code will allow Wwise to re-try later
				eResult = AK_Fail;
			}
		}

		if( eResult == AK_Success )
		{
			return LoadMediaFile( in_Item );
		}

		// If we end up here, it is because we really need to perform the HotSwap of the Data.

		//Reset the Error flag to success, We are going to fix the problem with HotSwapping of the data.
		eResult = AK_Success;

		// try to re-acquire the media another time and Addref it to keep it alive during the swap.
		pMediaEntry = m_MediaHashTable.Exists( sourceID );
		if( pMediaEntry )
		{
			pMediaEntry->AddRef();

			// Load the media First.
			AkAlignedPtrSet mediaPtrs;
			{
				m_MediaLock.Unlock(); // no need to keep the lock during the IO. pMediaEntry is safe as it is already addref'ed at this point.
				eResult = LoadMediaFile( in_Item.loadMediaFile.pszAllocatedFileName, mediaPtrs );
				m_MediaLock.Lock();
			}

			if( eResult == AK_Success )
			{
				AKASSERT( mediaPtrs.pAlignedPtr != NULL );

				// Performing the Hot Swap

				// We are going to play out of our playground; acquire main lock.
				CAkFunctionCritical SpaceSetAsCritical;

				//Backup old memory pointer.
				const AkUInt8* pOldPointer = pMediaEntry->GetPreparedMemoryPointer();

				//Free the Old Outdated Media:
				pMediaEntry->FreeMedia();
				//Set the new Media
				pMediaEntry->SetPreparedData( mediaPtrs.pAlignedPtr, mediaPtrs.uMediaSize, mediaPtrs.pAllocated );

				// Do some magic to Reset effects without stopping them if possible.
				CAkURenderer::ResetAllEffectsUsingThisMedia( pOldPointer );
			}
			else
			{
				// We unloaded but fail to load... so overall it was an unload: Release.
				if( pMediaEntry->Release() == 0 ) // if the memory was released
				{
					m_MediaHashTable.Unset( sourceID );
				}
			}
		}
		else
		{
			// Some Black magic did fix the problem in the lasst few Microseconds, no hot swap needed, do normal swap.
			return LoadMediaFile( in_Item );
		}
	}
	// Notify user.
	NotifyCompletion( in_Item, eResult );

	// Free the file name that was in the queue, we are responsible to free it.
	if( in_Item.loadMediaFile.pszAllocatedFileName )
	{
		AkFree( g_DefaultPoolId, in_Item.loadMediaFile.pszAllocatedFileName );
		in_Item.loadMediaFile.pszAllocatedFileName = NULL;
	}

#endif
	return eResult;
}

void CAkBankMgr::UnPrepareAllBank()
{
	int i = 0;
	AkBankKey* l_paBankIDs = NULL;
	{
		AkAutoLock<CAkBankList> BankListGate( m_BankList );

		CAkBankList::AkListLoadedBanks& rBankList = m_BankList.GetUNSAFEBankListRef();

		AkUInt32 l_ulNumBanks = rBankList.Length();
		if( l_ulNumBanks )
		{
			l_paBankIDs = (AkBankKey*)AkAlloca( l_ulNumBanks * sizeof( AkBankKey ) );
			{
				for( CAkBankList::AkListLoadedBanks::Iterator iter = rBankList.Begin(); iter != rBankList.End(); ++iter )
				{
					if( (*iter)->WasPreparedAsABank() )
					{
						l_paBankIDs[i] = (*iter)->key;
						++i;
					}
				}
			}
		}
	}
	// Unloading in reverse order
	while ( i > 0 )
	{
		AkBankKey bankKey = l_paBankIDs[ --i ];

		CAkBankList::Lock();
		CAkUsageSlot * pUsageSlot = m_BankList.Get( bankKey );
		if( pUsageSlot && pUsageSlot->WasPreparedAsABank())
		{
			CAkBankList::Unlock();
			UnPrepareBankInternal( bankKey.bankID, true, true );
		}
		else
		{
			CAkBankList::Unlock();
		}
	}
}

AKRESULT CAkBankMgr::PrepareBankInternal( AkBankQueueItem in_Item, AkFileID in_FileID, AkLoadBankDataMode in_LoadBankMode, bool in_bIsFromPrepareBank /*= false*/ )
{
	AkBankKey bankKey( in_FileID, NULL );
	{ // Lock brackets
		AkAutoLock<CAkBankList> BankListGate( m_BankList );
		
		CAkUsageSlot* pUsageSlot = m_BankList.Get( bankKey );
		if( pUsageSlot )
		{
			pUsageSlot->AddRefPrepare();
			pUsageSlot->WasPreparedAsABank( true );
			if( in_LoadBankMode == AkLoadBankDataMode_Structure )
			{
				return AK_Success;
			}

			AKASSERT( in_LoadBankMode == AkLoadBankDataMode_MediaAndStructure );
			
			if( pUsageSlot->IsMediaPrepared() )
			{
				return AK_Success;
			}

			// Only media must be loaded, correct the target and continue.
			in_LoadBankMode = AkLoadBankDataMode_Media;
		}
	}

	CAkUsageSlot * pUsageSlot = NULL;

	in_Item.load.BankID = in_FileID;
	AKRESULT eResult = LoadBank( in_Item, pUsageSlot, in_LoadBankMode, in_bIsFromPrepareBank );

	if ( eResult == AK_Success )
	{
		m_BankList.Set( bankKey, pUsageSlot );
	}
	else if( eResult != AK_BankAlreadyLoaded )
	{
		// Here, should not pass the notification flag.
		if ( pUsageSlot )
		{
			m_BankList.Remove( bankKey );
			pUsageSlot->ReleasePrepare();
		}
	}

	MONITOR_LOADEDBANK( pUsageSlot, false );

	return eResult;
}

void CAkBankMgr::UnPrepareBankInternal( AkUniqueID in_BankID, bool in_bIsFromPrepareBank, bool in_bIsFinal /*= false*/ )
{
	AkBankKey bankKey( in_BankID, NULL );
	CAkUsageSlot* pSlot = m_BankList.Get( bankKey );

	if( pSlot )
	{
		if( in_bIsFromPrepareBank && pSlot->WasPreparedAsABank() )
		{
			pSlot->WasPreparedAsABank( false );
		}
		pSlot->ReleasePrepare( in_bIsFinal );
	}
}

// Not to be called directly, the thread must be stopped for it to be possible
void  CAkBankMgr::UnloadAll()
{
	ClearPreparedEvents();
	UnPrepareAllBank();

	AKASSERT(!AkIsValidThread(&m_BankMgrThread));

	CAkBankList::AkListLoadedBanks& rBankList = m_BankList.GetUNSAFEBankListRef();
	CAkBankList::AkListLoadedBanks::IteratorEx it = rBankList.BeginEx();
	while ( it != rBankList.End() )
	{
		CAkUsageSlot* pSlot = *it;
		it = rBankList.Erase(it);
		pSlot->RemoveContent();
		pSlot->Release( true );
	}
}
	
void CAkBankMgr::AddLoadedItem(CAkUsageSlot* in_pUsageSlot, CAkIndexable* in_pIndexable)
{
	// Guaranteed to succeed because the necessary space has already been reserved
	AKVERIFY( in_pUsageSlot->m_listLoadedItem.AddLast(in_pIndexable) );
}

AkBankID CAkBankMgr::GetBankIDFromInMemorySpace( const void* in_pData, AkUInt32 in_uSize )
{
	// The bank must at least be the size of the header so we can read the bank ID.
	if( in_uSize >= AK_MINIMUM_BANK_SIZE )
	{
		AkUInt8* pdata = (AkUInt8*)in_pData;
		pdata += sizeof(AkSubchunkHeader);
		return ((AkBankHeader*)(pdata))->dwSoundBankID;
	}
	else
	{
		AKASSERT( !"Invalid memory to load bank from" );
		return AK_INVALID_BANK_ID;
	}
}

AKRESULT CAkBankMgr::ProcessBankHeader(AkBankHeader& in_rBankHeader)
{
	AKRESULT eResult = AK_Success;

	AkSubchunkHeader SubChunkHeader;

	eResult = m_BankReader.FillDataEx(&SubChunkHeader, sizeof(SubChunkHeader));
	if( eResult == AK_Success && SubChunkHeader.dwTag == BankHeaderChunkID )
	{
		eResult = m_BankReader.FillDataEx(&in_rBankHeader, sizeof(in_rBankHeader) );
#ifdef AK_DEMO
		if ( eResult == AK_Success )
		{
			in_rBankHeader.dwBankGeneratorVersion ^= AK_DEMOKEY1;
			in_rBankHeader.dwSoundBankID ^= AK_DEMOKEY2;
			in_rBankHeader.dwLanguageID ^= AK_DEMOKEY3;
			in_rBankHeader.bFeedbackInBank ^= AK_DEMOKEY4;
			in_rBankHeader.dwProjectID ^= AK_DEMOKEY5;
		}
#endif
		AkUInt32 uSizeToSkip = SubChunkHeader.dwChunkSize - sizeof(in_rBankHeader);
		if(eResult == AK_Success && uSizeToSkip )
		{
			AkUInt32 ulSizeSkipped = 0;
			eResult = m_BankReader.Skip( uSizeToSkip, ulSizeSkipped);
			if(eResult == AK_Success && ulSizeSkipped != uSizeToSkip)
			{
				eResult = AK_BankReadError;
			}
		}
		if( eResult == AK_Success && in_rBankHeader.dwBankGeneratorVersion != AK_BANK_READER_VERSION )
		{
			MONITOR_ERRORMSG( AKTEXT("Load bank failed: incompatible bank version") );
			eResult = AK_WrongBankVersion;
		}
	}
	else
	{
		eResult = AK_InvalidFile;
	}

	m_bFeedbackInBank = (in_rBankHeader.bFeedbackInBank != 0);

	return eResult;
}

AKRESULT CAkBankMgr::ProcessDataChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot )
{
	AKRESULT eResult = AK_Success;
	if ( in_dwDataChunkSize == 0 )
		return eResult; // Empty chunk -- return immediately

	AKASSERT(in_pUsageSlot->m_pData == NULL);

    if ( in_pUsageSlot->m_memPoolId == AK_INVALID_POOL_ID )
    {
		// No pool was specified by client: we need to create one internally for this bank.
        // Size is the exact size needed for data in this bank, 1 block.
		// A Fixed-size mode pool is used.
        in_pUsageSlot->m_memPoolId = AK::MemoryMgr::CreatePool( 
			NULL,
			in_dwDataChunkSize,
			in_dwDataChunkSize,
			AK_BANK_PLATFORM_ALLOC_TYPE | AkFixedSizeBlocksMode,
			AK_BANK_PLATFORM_DATA_ALIGNMENT
            );

		if( in_pUsageSlot->m_memPoolId != AK_INVALID_POOL_ID)
		{
#ifndef AK_OPTIMIZED
			char** ppString = m_BankIDToFileName.Exists( in_pUsageSlot->m_BankID );
			if( ppString )
			{
				AKASSERT( *ppString );
				AK_SETPOOLNAME( in_pUsageSlot->m_memPoolId, *ppString );
			}
#endif
			// Flag usage slot as using an internal pool. Store the generated pool id.
            in_pUsageSlot->m_bIsInternalPool = true;
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	// Allocate a bank from a pre-existing pool
	if ( eResult == AK_Success )
	{
		eResult = AK::MemoryMgr::CheckPoolId( in_pUsageSlot->m_memPoolId );

		// Allocate data in the pool.
		if ( eResult == AK_Success )
		{
			if ( AK::MemoryMgr::GetPoolAttributes( in_pUsageSlot->m_memPoolId ) & AkBlockMgmtMask )
			{
				// Check if fixed block size is large enough.
				if ( AK::MemoryMgr::GetBlockSize( in_pUsageSlot->m_memPoolId ) >= in_dwDataChunkSize )
				{
					in_pUsageSlot->m_pData = (AkUInt8*)AK::MemoryMgr::GetBlock( in_pUsageSlot->m_memPoolId );
				}
			}
			else
			{
				in_pUsageSlot->m_pData = (AkUInt8*)AkAlloc( in_pUsageSlot->m_memPoolId, in_dwDataChunkSize );
			}
			if ( in_pUsageSlot->m_pData == NULL )
			{
				// Cannot allocate bank memory.
				eResult = AK_InsufficientMemory;
			}
			else
			{
				in_pUsageSlot->m_uLoadedDataSize = in_dwDataChunkSize;
				AK_PERF_INCREMENT_BANK_MEMORY( in_pUsageSlot->m_uLoadedDataSize );
			}
		}
	}

	if(eResult == AK_Success)
	{
		AkUInt32 ulReadBytes = 0;
		eResult = m_BankReader.FillData( in_pUsageSlot->m_pData, in_dwDataChunkSize, ulReadBytes 
#if defined AK_WII_FAMILY
			, true
#endif
			);
		if(eResult == AK_Success && ulReadBytes != in_dwDataChunkSize)
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_ErrorWhileLoadingBank );
			eResult = AK_InvalidFile;
		}
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_InsufficientSpaceToLoadBank );
	}

	return eResult;
}

AKRESULT CAkBankMgr::ProcessHircChunk(CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID)
{
	AkUInt32 l_NumReleasableHircItem = 0;

	AKRESULT eResult = m_BankReader.FillDataEx(&l_NumReleasableHircItem, sizeof(l_NumReleasableHircItem));
	if( eResult == AK_Success && l_NumReleasableHircItem )
	{
		eResult = in_pUsageSlot->m_listLoadedItem.GrowArray( l_NumReleasableHircItem ) ? AK_Success : AK_Fail;
	}
	
	bool bWarnedUnknownContent = false;

	for( AkUInt32 i = 0; i < l_NumReleasableHircItem && eResult == AK_Success; ++i )
	{
		AKBKSubHircSection Section;

		eResult = m_BankReader.FillDataEx(&Section, sizeof(Section));
		if(eResult == AK_Success)
		{
			switch( Section.eHircType )
			{
			case HIRCType_Action:
				eResult = ReadAction(Section, in_pUsageSlot);
				break;

			case HIRCType_Event:
				eResult = ReadEvent(Section, in_pUsageSlot);
				break;

			case HIRCType_Sound:
				eResult = ReadSourceParent<CAkSound>(Section, in_pUsageSlot, in_dwBankID);
				break;

			case HIRCType_RanSeqCntr:
				eResult = StdBankRead<CAkRanSeqCntr, CAkParameterNodeBase>( Section, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Default ) );
				break;

			case HIRCType_SwitchCntr:
				eResult = StdBankRead<CAkSwitchCntr, CAkParameterNodeBase>( Section, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Default ) );
				break;

			case HIRCType_LayerCntr:
				eResult = StdBankRead<CAkLayerCntr, CAkParameterNodeBase>( Section, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Default ) );
				break;

			case HIRCType_ActorMixer:
				eResult = StdBankRead<CAkActorMixer, CAkParameterNodeBase>( Section, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Default ) );
				break;

			case HIRCType_State:
					eResult = ReadState( Section, in_pUsageSlot );
				break;

			case HIRCType_Bus:
				eResult = ReadBus( Section, in_pUsageSlot );
				break;

			case HIRCType_AuxBus:
				eResult = ReadAuxBus( Section, in_pUsageSlot );
				break;

			case HIRCType_Attenuation:
				eResult = StdBankRead<CAkAttenuation, CAkAttenuation>( Section, in_pUsageSlot, g_pIndex->m_idxAttenuations );
				break;

			case HIRCType_DialogueEvent:
				eResult = StdBankRead<CAkDialogueEvent, CAkDialogueEvent>( Section, in_pUsageSlot, g_pIndex->m_idxDialogueEvents );
				break;

#ifdef AK_MOTION
			case HIRCType_FeedbackBus:
				eResult = StdBankRead<CAkFeedbackBus, CAkParameterNodeBase>( Section, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Bus ) );
				break;

			case HIRCType_FeedbackNode:
				eResult = ReadSourceParent<CAkFeedbackNode>(Section, in_pUsageSlot, in_dwBankID);
				break;
#else // AK_MOTION
			case HIRCType_FeedbackBus:
			case HIRCType_FeedbackNode:
				{
					AkUInt32 ulReadBytes = 0;
					m_BankReader.Skip( Section.dwSectionSize, ulReadBytes );
					if ( ulReadBytes != Section.dwSectionSize )
					{
						eResult = AK_InvalidFile;
					}
				}
				break;
#endif // AK_MOTION

			case HIRCType_FxShareSet:
				eResult = StdBankRead<CAkFxShareSet, CAkFxShareSet>( Section, in_pUsageSlot, g_pIndex->m_idxFxShareSets );
				break;

			case HIRCType_FxCustom:
				eResult = StdBankRead<CAkFxCustom, CAkFxCustom>( Section, in_pUsageSlot, g_pIndex->m_idxFxCustom );
				break;

			default:
				{
					if( g_pExternalBankHandlerCallback )
					{
						eResult = g_pExternalBankHandlerCallback( Section, in_pUsageSlot, in_dwBankID );
						if( eResult != AK_PartialSuccess )
						{
							break;
						}
					}
					else if( !bWarnedUnknownContent )
					{
						if(	   Section.eHircType == HIRCType_Segment 
							|| Section.eHircType == HIRCType_Track
							|| Section.eHircType == HIRCType_MusicSwitch 
							|| Section.eHircType == HIRCType_MusicRanSeq )
						{
							bWarnedUnknownContent = true;
							MONITOR_ERRORMSG( AKTEXT("Music engine not initialized : Content can not be loaded from bank") );
						}
					}
					AkUInt32 ulReadBytes = 0;
					m_BankReader.Skip(Section.dwSectionSize, ulReadBytes );
					if(ulReadBytes != Section.dwSectionSize)
					{
						eResult = AK_InvalidFile;
					}
				}
				break;
			}
		}
	}

	return eResult;
}

AKRESULT CAkBankMgr::ProcessStringMappingChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* /*in_pUsageSlot*/ )
{
	AKRESULT eResult = AK_Success;

	while( in_dwDataChunkSize && eResult == AK_Success )
	{
		// Read header

		AKBKHashHeader hdr;

		eResult = m_BankReader.FillDataEx( &hdr, sizeof( hdr ) );
		if ( eResult != AK_Success ) break;

		in_dwDataChunkSize -= sizeof( hdr );

		// StringType_Bank is a special case
		
		AKASSERT( hdr.uiType == StringType_Bank );

		////////////////////////////////////////////////////
		//    StringType_Bank
		////////////////////////////////////////////////////
		// hdr.uiSize is the number of entries following
		// each entry is
		// UINT32 bankID
		// UINT8 stringSize
		// char[] string (NOT NULL TERMINATED)
		////////////////////////////////////////////////////

		for( AkUInt32 stringNumber = 0; stringNumber < hdr.uiSize; ++stringNumber )
		{	
			// read bank ID
			AkBankID bankID;
			eResult = m_BankReader.FillDataEx( &bankID, sizeof( bankID ) );
			if( eResult != AK_Success )break;
			in_dwDataChunkSize -= sizeof( bankID );

			// read string size (NOT NULL TERMINATED)
			AkUInt8 stringsize;
			eResult = m_BankReader.FillDataEx( &stringsize, sizeof( stringsize ) );
			if( eResult != AK_Success )break;
			in_dwDataChunkSize -= sizeof( stringsize );

			// Check the list.
			if( m_BankIDToFileName.Exists( bankID ) )
			{
				AkUInt32 uIgnoredSkippedSize;
				m_BankReader.Skip( stringsize, uIgnoredSkippedSize );
				AKASSERT( uIgnoredSkippedSize == stringsize );
				in_dwDataChunkSize -= stringsize;
			}
			else
			{
				// Alloc string of size stringsize + 1 ( for missing NULL character )
				// Alloc string of size stringsize + 4 ( for missing .bnk extension )
				char* pString = (char*)AkAlloc( g_DefaultPoolId, stringsize + 1 + 4 );
				if( pString )
				{
					// Read/Copy the string
					pString[ stringsize ] = '.';
					pString[ stringsize + 1] = 'b';
					pString[ stringsize + 2] = 'n';
					pString[ stringsize + 3] = 'k';
					pString[ stringsize + 4] = 0;
					eResult = m_BankReader.FillDataEx( pString, stringsize );

					if( eResult == AK_Success )
					{
						in_dwDataChunkSize -= stringsize;
						// Add the entry in the table
						char** ppString = m_BankIDToFileName.Set( bankID );
						if( ppString )
						{
							*ppString = pString;
						}
						else
						{
							eResult = AK_InsufficientMemory;
						}
					}
					if( eResult != AK_Success )
					{
						AkFree( g_DefaultPoolId, pString );
						break;
					}
				}
				else
				{
					// Fill eResult and break.
					eResult = AK_InsufficientMemory;
					break;
				}
			}
		} // for( AkUInt32 stringNumber = 0; stringNumber < hdr.uiSize; ++stringNumber )
	}

	return eResult;
}

void CAkBankMgr::UpdateBankName( AkBankID in_bankID, char* in_pStringWithoutExtension )
{
	// Check the list.
	if( !m_BankIDToFileName.Exists( in_bankID ) )
	{
		// Alloc string of size stringsize + 1 ( for missing NULL character )
		// Alloc string of size stringsize + 4 ( for missing .bnk extension )
		size_t stringsize = strlen( in_pStringWithoutExtension );
		char* pString = (char*)AkAlloc( g_DefaultPoolId, stringsize + 1 + 4 );
		if( pString )
		{
			// Copy the string + .bnk
			memcpy( pString, in_pStringWithoutExtension, stringsize );
			
			pString[ stringsize ] = '.';
			pString[ stringsize + 1] = 'b';
			pString[ stringsize + 2] = 'n';
			pString[ stringsize + 3] = 'k';
			pString[ stringsize + 4] = 0;

			// Add the entry in the table
			char** ppString = m_BankIDToFileName.Set( in_bankID );
			if( ppString )
			{
				*ppString = pString;
			}
			else
			{
				AkFree( g_DefaultPoolId, pString );
			}
		}
	}
}

AKRESULT CAkBankMgr::ProcessGlobalSettingsChunk( AkUInt32 in_dwDataChunkSize )
{
	AKRESULT eResult = AK_Success;

	AKASSERT(g_pStateMgr);

	if( in_dwDataChunkSize )
	{
		//Here, read the first item, which is the volume threshold
		AkReal32 fVolumeThreshold;
		eResult = m_BankReader.FillDataEx( &fVolumeThreshold, sizeof( fVolumeThreshold ) );
		AK::SoundEngine::SetVolumeThresholdInternal( fVolumeThreshold, AK::SoundEngine::AkCommandPriority_InitDefault );
		if( eResult == AK_Success )
		{
			AkUInt16 u16MaxVoices;
			eResult = m_BankReader.FillDataEx( &u16MaxVoices, sizeof( u16MaxVoices ) );
			AK::SoundEngine::SetMaxNumVoicesLimitInternal( u16MaxVoices, AK::SoundEngine::AkCommandPriority_InitDefault );
		}

		AkUInt32 ulNumStateGroups = 0;
		if( eResult == AK_Success )
		{
			eResult = m_BankReader.FillDataEx( &ulNumStateGroups, sizeof( ulNumStateGroups ) );
		}
		if( eResult == AK_Success )
		{
			for( AkUInt32 i = 0; i < ulNumStateGroups; ++i ) //iterating trough state groups
			{
				AkUInt32 ulStateGroupID = 0;
				AkTimeMs DefaultTransitionTime = 0;
				AkUInt32 ulNumTransitions = 0;

				eResult = m_BankReader.FillDataEx( &ulStateGroupID, sizeof( ulStateGroupID ) );
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &DefaultTransitionTime, sizeof( DefaultTransitionTime ) );
				}
				if( eResult == AK_Success )
				{
					eResult = g_pStateMgr->AddStateGroup( ulStateGroupID )? AK_Success:AK_Fail;
				}
				if( eResult == AK_Success )
				{
					eResult = g_pStateMgr->SetdefaultTransitionTime( ulStateGroupID, DefaultTransitionTime );
				}
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &ulNumTransitions, sizeof( ulNumTransitions ) );
				}
				if( eResult == AK_Success )
				{
					for( AkUInt32 j = 0; j < ulNumTransitions; ++j )//iterating trough Transition time
					{
						AkUInt32		StateFrom;
						AkUInt32		StateTo;
						AkTimeMs	TransitionTime;

						eResult = m_BankReader.FillDataEx( &StateFrom, sizeof( StateFrom ) );
						if( eResult == AK_Success )
						{
							eResult = m_BankReader.FillDataEx( &StateTo, sizeof( StateTo ) );
						}
						if( eResult == AK_Success )
						{
							eResult = m_BankReader.FillDataEx( &TransitionTime, sizeof( TransitionTime ) );
						}
						if( eResult == AK_Success )
						{
							eResult = g_pStateMgr->AddStateTransition( ulStateGroupID, StateFrom, StateTo, TransitionTime );
						}
						// If failed, quit!
						if( eResult != AK_Success)
						{
							break;
						}
					}
				}
				// If failed, quit!
				if( eResult != AK_Success )
				{
					break;
				}
			}

			AkUInt32 ulNumSwitchGroups = 0;
			if ( eResult == AK_Success )
			{
				eResult = m_BankReader.FillDataEx( &ulNumSwitchGroups, sizeof( ulNumSwitchGroups ) );
			}
			if( eResult == AK_Success )
			{
				for( AkUInt32 i = 0; i < ulNumSwitchGroups; ++i ) //iterating trough switch groups
				{
					AkUInt32	SwitchGroupID;
					AkUInt32	RTPC_ID;
					AkUInt32	ulSize;

					eResult = m_BankReader.FillDataEx( &SwitchGroupID, sizeof( SwitchGroupID ) );
					if( eResult == AK_Success )
					{
						eResult = m_BankReader.FillDataEx( &RTPC_ID, sizeof( RTPC_ID ) );
					}
					if( eResult == AK_Success )
					{
						eResult = m_BankReader.FillDataEx( &ulSize, sizeof( ulSize ) );
					}
					if( eResult == AK_Success )
					{
						// It is possible that the size be 0 in the following situation:
						// Wwise user created a switch to RTPC graph, and disabled it.
						if( ulSize )
						{
							AkUInt32 l_blockSize = ulSize*sizeof( AkRTPCGraphPointInteger );
							AkRTPCGraphPointInteger* pGraphPoints = (AkRTPCGraphPointInteger*)AkAlloc( g_DefaultPoolId, l_blockSize );
							if( pGraphPoints )
							{
								eResult = m_BankReader.FillDataEx( pGraphPoints, l_blockSize );
								if( eResult == AK_Success )
								{
									eResult = g_pRTPCMgr->AddSwitchRTPC( SwitchGroupID, RTPC_ID, pGraphPoints, ulSize );
								}
								AkFree( g_DefaultPoolId, pGraphPoints );
							}
							else
							{
								eResult = AK_InsufficientMemory;
							}
						}
					}
					// If failed, quit!
					if( eResult != AK_Success)
					{
						break;
					}
				}
			}

			//Load default values for game parameters
			AkUInt32 ulNumParams = 0;
			if ( eResult == AK_Success )
				eResult = m_BankReader.FillDataEx( &ulNumParams, sizeof( ulNumParams ) );

			if( eResult == AK_Success )
			{
				for (; ulNumParams > 0; ulNumParams--)
				{
					AkReal32	fValue;
					AkUInt32	RTPC_ID;

					eResult = m_BankReader.FillDataEx( &RTPC_ID, sizeof( RTPC_ID ) );
					if( eResult == AK_Success )
						eResult = m_BankReader.FillDataEx( &fValue, sizeof( fValue ) );

					if( eResult == AK_Success )
						g_pRTPCMgr->SetDefaultParamValue(RTPC_ID, fValue);

					if( eResult != AK_Success)
						break;
				}
			}
		}
	}
	else
	{
		AKASSERT(!"Invalid STMG chunk found in the Bank");
	}

	return eResult;
}

AKRESULT CAkBankMgr::ProcessEnvSettingsChunk( AkUInt32 in_dwDataChunkSize )
{
	AKRESULT eResult = AK_Success;

	AKASSERT(g_pStateMgr);
	if(!g_pStateMgr)
	{
		return AK_Fail;
	}

	if( in_dwDataChunkSize )
	{
		for ( int i=0; i<CAkEnvironmentsMgr::MAX_CURVE_X_TYPES; ++i )
		{
			for( int j=0; j<CAkEnvironmentsMgr::MAX_CURVE_Y_TYPES; ++j )
			{
				AkUInt8 bCurveEnabled;
				eResult = m_BankReader.FillDataEx( &bCurveEnabled, sizeof( bCurveEnabled ) );
				if( eResult == AK_Success )
				{
					g_pEnvironmentMgr->SetCurveEnabled( (CAkEnvironmentsMgr::eCurveXType)i, (CAkEnvironmentsMgr::eCurveYType)j, bCurveEnabled ? true : false );
				}

				if( eResult == AK_Success )
				{
					AkUInt8 l_eCurveScaling;
					eResult = m_BankReader.FillDataEx( &l_eCurveScaling, sizeof( AkUInt8 ) );

					AkUInt16 l_ulCurveSize;
					if( eResult == AK_Success )
						eResult = m_BankReader.FillDataEx( &l_ulCurveSize, sizeof( AkUInt16 ) );

					if( eResult == AK_Success )
					{
						AkRTPCGraphPoint* aPoints = (AkRTPCGraphPoint *) AkAlloc( g_DefaultPoolId, sizeof( AkRTPCGraphPoint ) * l_ulCurveSize );
						if ( aPoints )
                        {
                            eResult = m_BankReader.FillDataEx( &aPoints[0], sizeof( AkRTPCGraphPoint ) * l_ulCurveSize );
                            if( eResult == AK_Success )
						    {
							    g_pEnvironmentMgr->SetObsOccCurve( (CAkEnvironmentsMgr::eCurveXType)i, (CAkEnvironmentsMgr::eCurveYType)j, l_ulCurveSize, aPoints, (AkCurveScaling) l_eCurveScaling );
						    }
						    AkFree( g_DefaultPoolId, aPoints ); //env mgr will have copied the curve
                        }
                        else
                        	eResult = AK_InsufficientMemory;
					}
				}

				// If failed, quit!
				if( eResult != AK_Success)
				{
					break;
				}
			}

			// If failed, quit!
			if( eResult != AK_Success)
			{
				break;
			}
		}
	}
	else
	{
		AKASSERT(!"Invalid ENVS chunk found in the Bank");
		eResult = AK_Fail;
	}

	return eResult;
}

/////////////////////////////  HIRC PARSERS  ////////////////////////////////////
//
// STATE HIRC PARSER
AKRESULT CAkBankMgr::ReadState( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot )
{
	AKRESULT eResult = AK_Success;

	const void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUniqueID ulStateID = ReadUnaligned<AkUInt32>((AkUInt8*) pData);

	CAkState* pState = g_pIndex->m_idxCustomStates.GetPtrAndAddRef( ulStateID );

	if( pState == NULL )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pState = CAkState::Create( ulStateID );
		if(!pState)
		{
			eResult = AK_Fail;
		}
		else
		{
			eResult = pState->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize );
			if(eResult != AK_Success)
			{
				pState->Release();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem( in_pUsageSlot, pState ); //This will allow it to be removed on unload
	}

	m_BankReader.ReleaseData();

	return eResult;
}


AKRESULT CAkBankMgr::ReadBus(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot)
{
	AKRESULT eResult = AK_Success;

	const void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUniqueID ulID = ReadUnaligned<AkUniqueID>((AkUInt8*)pData);

	CAkBus* pBus = static_cast<CAkBus*>( g_pIndex->GetNodePtrAndAddRef( ulID, AkNodeType_Bus ) );
	if( pBus == NULL )
	{
		if( !m_bIsFirstBusLoaded )
		{
			// Clearing the Master bus.
			// Why:
			//    The init bank contains the master bus, but it is possible that another
			//    temporary master bus has been added if Wwise has been connected prior to this init bank load bank.
			//    The ID of the Master bus wouldn<t match, and the results would be that global events and information about master bus volume and more be wrong.	
			CAkBus::ClearMasterBus();
		}

		CAkFunctionCritical SpaceSetAsCritical;
		pBus = CAkBus::Create(ulID);
		if(!pBus)
		{
			eResult = AK_Fail;
		}
		else
		{
			eResult = pBus->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize );
			if(eResult != AK_Success)
			{
				pBus->Release();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem( in_pUsageSlot, pBus); //This will allow it to be removed on unload

		//at least one bus has been loaded with success in the init bank
		SetIsFirstBusLoaded( true );
	}

	m_BankReader.ReleaseData();

	return eResult;
}

AKRESULT CAkBankMgr::ReadAuxBus( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot )
{
	AKRESULT eResult = AK_Success;

	const void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUniqueID ulID = ReadUnaligned<AkUniqueID>((AkUInt8*)pData);

	CAkBus* pBus = static_cast<CAkBus*>( g_pIndex->GetNodePtrAndAddRef( ulID, AkNodeType_Bus ) );
	if( pBus == NULL )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pBus = CAkAuxBus::Create(ulID);
		if(!pBus)
		{
			eResult = AK_Fail;
		}
		else
		{
			eResult = pBus->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize );
			if(eResult != AK_Success)
			{
				pBus->Release();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem( in_pUsageSlot, pBus); //This will allow it to be removed on unload
	}

	m_BankReader.ReleaseData();

	return eResult;
}

// ACTION HIRC PARSER
AKRESULT CAkBankMgr::ReadAction(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot)
{
	AKRESULT eResult = AK_Success;

	const void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUInt32 ulID = ReadUnaligned<AkUInt32>(((AkUInt8*)pData));
	AkActionType ulActionType = (AkActionType) ReadUnaligned<AkUInt16>(((AkUInt8*)pData), sizeof(AkUInt32));

	CAkAction* pAction = g_pIndex->m_idxActions.GetPtrAndAddRef(ulID);
	if( pAction == NULL )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pAction = CAkAction::Create(ulActionType, ulID );
		if(!pAction)
		{
			eResult = AK_Fail;
		}
		else
		{
			eResult = pAction->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize);
			if(eResult != AK_Success)
			{
				pAction->Release();
			}
		}
	}
	else
	{
		// When reading actions of type play, we must read them again it they were posted By Wwise.
		// This is because play actions can be prepared, and need to have the correct Bank ID to load structure from.
		if( ulActionType == AkActionType_Play )
		{
			if( !static_cast<CAkActionPlay*>(pAction)->WasLoadedFromBank() )
			{
				CAkFunctionCritical SpaceSetAsCritical;
				eResult = pAction->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize);
				if(eResult != AK_Success)
				{
					pAction->Release();
				}
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem(in_pUsageSlot,pAction); //This will allow it to be removed on unload
	}

	m_BankReader.ReleaseData();

	return eResult;
}

// EVENT HIRC PARSER
AKRESULT CAkBankMgr::ReadEvent(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot)
{
	AKRESULT eResult = AK_Success;

	const void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUniqueID ulID = ReadUnaligned<AkUniqueID>((AkUInt8*)pData);

	//If the event already exists, simply addref it
	CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef(ulID);
	if( pEvent == NULL )
	{
		// Using CreateNoIndex() instead of Create(), we will do the init right after. 
		// The goal is to avoid the event to be in the index prior it is completely loaded.
		pEvent = CAkEvent::CreateNoIndex(ulID);
		if(!pEvent)
		{
			eResult = AK_Fail;
		}
		else
		{
			CAkFunctionCritical SpaceSetAsCritical;
			eResult = pEvent->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize);
			if(eResult != AK_Success)
			{
				pEvent->Release();
			}
			else
			{
				// And here we proceed with the AddToIndex() call, required since we previously used CreateNoIndex() to create the event.
				pEvent->AddToIndex();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem(in_pUsageSlot,pEvent); //This will allow it to be removed on unload
	}

	m_BankReader.ReleaseData();

	return eResult;
}
//
//
/////////////////////////////  END OF HIRC PARSERS  ////////////////////////////////////

void CAkBankMgr::ClearPreparedEvents()
{
	AKASSERT( g_pIndex );
	CAkIndexItem<CAkEvent*>& l_rIdx = g_pIndex->m_idxEvents;

	CAkFunctionCritical SpaceSetAsCritical;// To avoid possible deadlocks
	// This function could potentially slow down the audio thread, but this is required to avoid deadlocks.
	AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

	CAkIndexItem<CAkEvent*>::AkMapIDToPtr::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
	while( iter != l_rIdx.m_mapIDToPtr.End() )
	{
		CAkEvent* pEvent = static_cast<CAkEvent*>( *iter );
		
		if( pEvent->IsPrepared() )
		{
			pEvent->AddRef();
			UnprepareEvent( pEvent, true );
			++iter;	// Must be done before releasing the event if we unprepared it.
					// It must be done after calling UnprepareEvent as UnprepareEvent may destroy multiple Events and they can all de removed from the index.
			pEvent->Release();
		}
		else
		{
			++iter;
		}
	}
}

AKRESULT CAkBankMgr::LoadMediaIndex( CAkUsageSlot* in_pCurrentSlot, AkUInt32 in_uIndexChunkSize, bool in_bIsLoadedFromMemory )
{
	AKRESULT eResult = AK_Success;
	if( in_pCurrentSlot->m_uNumLoadedItems )// m_uNumLoadedItems != 0 means it has already been loaded.
	{
		// skip the chunk
		AkUInt32 ulSkippedBytes;
		m_BankReader.Skip( in_uIndexChunkSize, ulSkippedBytes );
		AKASSERT( ulSkippedBytes == in_uIndexChunkSize );
	}
	else
	{
		AkUInt32 uNumMedias = in_uIndexChunkSize / sizeof( AkBank::MediaHeader );

		AKASSERT( uNumMedias != 0 );

		AkUInt32 uArraySize = uNumMedias * sizeof( AkBank::MediaHeader );

		if( !in_bIsLoadedFromMemory )
		{
			in_pCurrentSlot->m_paLoadedMedia = (AkBank::MediaHeader*)AkAlloc( g_DefaultPoolId, uArraySize );
			if( in_pCurrentSlot->m_paLoadedMedia )
			{
				in_pCurrentSlot->WasIndexAllocated( true );
			}
			if( in_pCurrentSlot->m_paLoadedMedia )
			{
				AKVERIFY( m_BankReader.FillDataEx( in_pCurrentSlot->m_paLoadedMedia, uArraySize ) == AK_Success );
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		else
		{
			// We must get it that we use it or not.
			AkBank::MediaHeader* pLoadedMedia = (AkBank::MediaHeader*)m_BankReader.GetData( uArraySize );
			if( !in_pCurrentSlot->m_paLoadedMedia )// we can already have it if we used PrepareBank with media.
				in_pCurrentSlot->m_paLoadedMedia = pLoadedMedia;

			m_BankReader.ReleaseData();
			AKASSERT( in_pCurrentSlot->m_paLoadedMedia );
		}
		if( in_pCurrentSlot->m_paLoadedMedia )
		{
			in_pCurrentSlot->m_uIndexSize = uNumMedias;
		}
	}
	return eResult;
}

AKRESULT CAkBankMgr::LoadMedia( AkUInt8* in_pDataBank, CAkUsageSlot* in_pCurrentSlot, bool /*in_bIsLoadedFromMemory*/ )
{
	if( in_pCurrentSlot->m_uNumLoadedItems != 0 || in_pCurrentSlot->m_uIndexSize == 0 )
		return AK_Success;

	AKRESULT eResult = AK_InsufficientMemory;

	if( in_pCurrentSlot->m_paLoadedMedia )
	{
		AkUInt32 nNumLoadedMedias = 0;
		for( ; nNumLoadedMedias < in_pCurrentSlot->m_uIndexSize; ++nNumLoadedMedias )
		{
			AkBank::MediaHeader& rMediaHeader = in_pCurrentSlot->m_paLoadedMedia[ nNumLoadedMedias ];
			if ( rMediaHeader.id != AK_INVALID_UNIQUE_ID )
			{
				AkAutoLock<CAkLock> gate( m_MediaLock );// Will have to decide if performance would be bettre if acquiring the lock outside the for loop.

				bool b_WasAlreadyEnlisted = false;
				AkMediaEntry* pMediaEntry = m_MediaHashTable.Set( rMediaHeader.id, b_WasAlreadyEnlisted );
				if( b_WasAlreadyEnlisted && pMediaEntry->GetPreparedMemoryPointer() != NULL )
				{
					// TODO : this optimization may be source of some confusion.
					// Maybe we should call add alternate bank anyway...
					pMediaEntry->AddRef();
				}
				else if( pMediaEntry )
				{
					pMediaEntry->SetSourceID( rMediaHeader.id );
					eResult = pMediaEntry->AddAlternateBank(
											in_pDataBank + rMediaHeader.uOffset,// Data pointer
											rMediaHeader.uSize,					// Data size
											in_pCurrentSlot						// pUsageSlot
											);

					if( eResult != AK_Success )
					{
						m_MediaHashTable.Unset( rMediaHeader.id );
						break;
					}
					else if( b_WasAlreadyEnlisted )
					{
						pMediaEntry->AddRef();
					}
				}
				else
				{
					break;
				}

				if( pMediaEntry )
				{
					MONITOR_MEDIAPREPARED( *pMediaEntry );
				}
			}

			++( in_pCurrentSlot->m_uNumLoadedItems );
		}

		if( in_pCurrentSlot->m_uIndexSize == nNumLoadedMedias ) // if everything loaded successfully.
		{
			eResult = AK_Success;
		}
	}

	if( eResult != AK_Success )
	{
		UnloadMedia( in_pCurrentSlot );
	}

	return eResult;
}

AKRESULT CAkBankMgr::PrepareMedia( CAkUsageSlot* in_pCurrentSlot, AkUInt32 in_dwDataChunkSize )
{
#ifndef PROXYCENTRAL_CONNECTED
	AKASSERT( !"Illegal to Prepare data when running the Wwise sound engine" );
#endif

	AKRESULT eResult = AK_Success;
	if ( in_dwDataChunkSize == 0 )
		return eResult; // Empty chunk -- return immediately

	int iIndexPos = 0;// external from the for for backward cleanup in case of error.
	AkUInt32 uLastReadPosition = 0;
	AkUInt32 uToSkipSize = 0;
	for( ; in_pCurrentSlot->m_uNumLoadedItems < in_pCurrentSlot->m_uIndexSize; ++iIndexPos )
	{
		AkUInt32 uTempToSkipSize = in_pCurrentSlot->m_paLoadedMedia[iIndexPos].uOffset - uLastReadPosition;
		uToSkipSize += uTempToSkipSize;
		uLastReadPosition += uTempToSkipSize;

		AkUniqueID sourceID = in_pCurrentSlot->m_paLoadedMedia[iIndexPos].id;
		if ( sourceID == AK_INVALID_UNIQUE_ID ) // missing file get a cleared entry in media table
		{
			++( in_pCurrentSlot->m_uNumLoadedItems );
			continue;
		}

		AkUInt32 uMediaSize = in_pCurrentSlot->m_paLoadedMedia[iIndexPos].uSize;
		AKASSERT( uMediaSize );

		AkAutoLock<CAkLock> gate( m_MediaLock );

		AkMediaEntry* pMediaEntry = m_MediaHashTable.Exists( sourceID );
		if( pMediaEntry )
		{
			pMediaEntry->AddRef();
			if( pMediaEntry->IsDataPrepared() )
			{
				// Data already available, simply return success.
				++( in_pCurrentSlot->m_uNumLoadedItems );
				continue;
			}
		}
		else
		{
			// Add the missing entry
			pMediaEntry = m_MediaHashTable.Set( sourceID );
			if( !pMediaEntry )
			{
				eResult = AK_Fail;
				break;
			}
			pMediaEntry->SetSourceID( sourceID );
		}
		AKASSERT( pMediaEntry );// From this point, we are guaranteed to have one. 

		AkUInt8* pAllocated = NULL;

		// Take the data and copy it over
		if( pMediaEntry->HasBankSource() )
		{
			// Will allocate pAllocated and set uMediaSize to the effective media size (i.e. size of pAllocated)
			eResult = pMediaEntry->PrepareFromBank(
				pAllocated,
				uMediaSize,
				0 /* Don't know codec ID */
				);
		}
		else
		{
			// Stream/Load it
			AkUInt32 uAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;
			AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_MEDIA_SIZE( uAlignment, uMediaSize );

			if( HOOKEDPOOLID != AK_INVALID_POOL_ID )
				pAllocated = (AkUInt8*)ALLOCWITHHOOK( uMediaSize, uAlignment );
			else
				MONITOR_ERRORMSG( AKTEXT("No memory space specified for preparing data. Make sure you specified a valid memory pool ID in the init parameter: AkInitSettings::uPrepareEventMemoryPoolID.") );

			if ( pAllocated )
			{
				m_MediaLock.Unlock();// no need to keep the lock during the IO. pMediaEntry is safe as it is already addref'ed at this point.

				{
					if( uToSkipSize )
					{
						AkUInt32 uSkipped = 0;
						eResult = m_BankReader.Skip( uToSkipSize, uSkipped );
						if( uToSkipSize != uSkipped )
							eResult = AK_Fail;
						else
						{
							uToSkipSize = 0; // Reset the skip counter for next time.
						}
					}
                    
                    if (eResult == AK_Success)
                    {
                        AkUInt32 uReadSize = 0;
                        eResult = m_BankReader.FillData( pAllocated, uMediaSize, uReadSize
    #if defined AK_WII_FAMILY
                            ,true // flush cache range on Wii.
    #endif
                            );
                        if( eResult == AK_Success && uMediaSize != uReadSize )
                            eResult = AK_Fail;
                        else
                            uLastReadPosition += uReadSize;
                    }
				}

				m_MediaLock.Lock();
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}

		if( eResult == AK_Success )
		{
			pMediaEntry->SetPreparedData( pAllocated, uMediaSize );
			++( in_pCurrentSlot->m_uNumLoadedItems );
			continue;
		}

		// Failed, clear the memory...
		if ( pAllocated )
		{
			FREEWITHHOOK( pAllocated );
		}

		if( pMediaEntry->Release() == 0 ) // if the memory was released
		{
			m_MediaHashTable.Unset( sourceID );
		}

		break;
	}

	if( eResult == AK_Success )
	{
		uToSkipSize += in_dwDataChunkSize-uLastReadPosition;
		if( uToSkipSize )
		{
			AkUInt32 uSkipped = 0;
			m_BankReader.Skip( uToSkipSize, uSkipped );
			if( uToSkipSize != uSkipped )
				eResult = AK_Fail;
		}
	}

	if( eResult != AK_Success )
	{
		while(iIndexPos > 0 )
		{
			--iIndexPos;
			AkMediaID mediaID =  in_pCurrentSlot->m_paLoadedMedia[iIndexPos].id;
			if ( mediaID != AK_INVALID_UNIQUE_ID ) // missing file get a cleared entry in media table
				ReleaseSingleMedia( mediaID );
		}
	}
	else
	{
		in_pCurrentSlot->IsMediaPrepared( true );
	}

	return eResult;
}

void CAkBankMgr::UnloadMedia( CAkUsageSlot* in_pCurrentSlot )
{
	if( in_pCurrentSlot->m_paLoadedMedia )
	{
		AkAutoLock<CAkLock> gate( m_MediaLock );// Will have to decide if performance would be better if acquiring the lock inside the for loop.
		while( in_pCurrentSlot->m_uNumLoadedItems )
		{
			--in_pCurrentSlot->m_uNumLoadedItems;

			AkMediaID mediaID = in_pCurrentSlot->m_paLoadedMedia[ in_pCurrentSlot->m_uNumLoadedItems ].id;
			if ( mediaID == AK_INVALID_UNIQUE_ID ) // missing file get a cleared entry in media table
				continue;

			AkMediaHashTable::IteratorEx iter = m_MediaHashTable.FindEx( mediaID );
			AKASSERT( iter != m_MediaHashTable.End() ); // this can indicate a corruption of m_paLoadedMedia

			if( iter != m_MediaHashTable.End() )
			{
				AkMediaEntry& rMediaEntry = iter.pItem->Assoc.item;
				rMediaEntry.RemoveAlternateBank( in_pCurrentSlot );
				MONITOR_MEDIAPREPARED( rMediaEntry );

				if( rMediaEntry.Release() == 0 ) // if the memory was released
				{
					m_MediaHashTable.Erase( iter );
				}
			}
		}
	}
}

void CAkBankMgr::UnPrepareMedia( CAkUsageSlot* in_pCurrentSlot )
{
	if( !in_pCurrentSlot->IsMediaPrepared() )
		return;	// Wasn't prepared.

	if( in_pCurrentSlot->m_paLoadedMedia )
	{
		{// Locking bracket.
			AkAutoLock<CAkLock> gate( m_MediaLock );// Will have to decide if performance would be better if acquiring the lock inside the for loop.
			for( AkUInt32 i = 0; i < in_pCurrentSlot->m_uNumLoadedItems; ++i )
			{
				AkMediaID mediaID = in_pCurrentSlot->m_paLoadedMedia[ i ].id;
				if ( mediaID == AK_INVALID_UNIQUE_ID ) // missing file get a cleared entry in media table
					continue;

				AkMediaHashTable::IteratorEx iter = m_MediaHashTable.FindEx( mediaID );
				AKASSERT( iter != m_MediaHashTable.End() ); // this can indicate a corruption of m_paLoadedMedia
	
				if( iter != m_MediaHashTable.End() )
				{
					if( iter.pItem->Assoc.item.Release() == 0 ) // if the memory was released
					{
						m_MediaHashTable.Erase( iter );
					}
				}
			}
		}

		in_pCurrentSlot->IsMediaPrepared( false );
	}
}

AkMediaInfo CAkBankMgr::GetMedia( AkMediaID in_mediaId, CAkUsageSlot* &out_pUsageSlot )
{
	AkMediaInfo returnedMediaInfo( NULL, 0 );

	AkAutoLock<CAkBankList> BankListGate( m_BankList ); // CAkBankList lock must be acquired before MediaLock.
	AkAutoLock<CAkLock> gate( m_MediaLock );

	AkMediaEntry* pMediaEntry = m_MediaHashTable.Exists( in_mediaId );

	if( pMediaEntry )
	{
		pMediaEntry->GetMedia( returnedMediaInfo, out_pUsageSlot );
	}

	return returnedMediaInfo;
}

// GetMedia() and ReleaseMedia() calls must be balanced. 
void CAkBankMgr::ReleaseMedia( AkMediaID in_mediaId )
{
	AkAutoLock<CAkBankList> BankListGate( m_BankList );
	AkAutoLock<CAkLock> gate( m_MediaLock );
	AkMediaHashTable::IteratorEx iter = m_MediaHashTable.FindEx( in_mediaId );

	if( iter != m_MediaHashTable.End() )
	{
		AkMediaEntry& rMediaEntry = iter.pItem->Assoc.item;

		if( rMediaEntry.Release() == 0 ) // if the memory was released
		{
			m_MediaHashTable.Erase( iter );
		}
	}
}
AKRESULT CAkBankMgr::PrepareSingleMedia( AkSrcTypeInfo& in_rmediaInfo )
{
	if( m_bAccumulating )
	{
		AkSrcTypeInfo * pItem = m_PreparationAccumulator.AddNoSetKey( in_rmediaInfo.mediaInfo ); // allows duplicate keys.
		if( !pItem )
			return AK_InsufficientMemory;
		else
		{
			*pItem = in_rmediaInfo;
			return AK_Success;
		}
	}
	else
	{
		return LoadSingleMedia( in_rmediaInfo );
	}
}

void CAkBankMgr::UnprepareSingleMedia( AkUniqueID in_SourceID )
{
	// In accumulation mode, UnprepareSingleMedia should do nothing since PrepareSingleMedia did nothing.
	if( !m_bAccumulating )
		ReleaseSingleMedia( in_SourceID );
}

void CAkBankMgr::EnableAccumulation()
{
	AKASSERT( m_bAccumulating == false );
	m_bAccumulating = true;
}

void CAkBankMgr::DisableAccumulation()
{
	AKASSERT( m_bAccumulating == true );
	m_bAccumulating = false;
	m_PreparationAccumulator.RemoveAll();
}

AKRESULT CAkBankMgr::ProcessAccumulated()
{
	AKRESULT eResult = AK_Success;

	for( AkSortedPreparationList::Iterator iter = m_PreparationAccumulator.Begin();
		iter != m_PreparationAccumulator.End();
		++iter )
	{
		eResult = LoadSingleMedia( *iter );
		if( eResult != AK_Success )
		{
			// Reverse Cleanup.
			for( AkSortedPreparationList::Iterator iterDestructor = m_PreparationAccumulator.Begin();
					iterDestructor != iter;
					++iterDestructor )
			{
				ReleaseSingleMedia( (*iterDestructor).mediaInfo.sourceID );
			}
			break; // Will return last error code.
		}
	}

	return eResult;
}

AKRESULT CAkBankMgr::LoadSingleMedia( AkSrcTypeInfo& in_rMediaInfo )
{
#ifndef PROXYCENTRAL_CONNECTED
	AKASSERT( !"Illegal to Prepare data when running the Wwise sound engine" );
#endif

	AkUInt32 uMediaSize = in_rMediaInfo.mediaInfo.uInMemoryMediaSize;
	if( uMediaSize == 0 )
	{
		// If the size is 0, well... there is nothing to load.
		return AK_Success;
	}

	AkAutoLock<CAkLock> gate( m_MediaLock );

	AkUniqueID sourceID = in_rMediaInfo.mediaInfo.sourceID;
	AkMediaEntry* pMediaEntry = m_MediaHashTable.Exists( sourceID );
	if( pMediaEntry )
	{
		pMediaEntry->AddRef();
		if( pMediaEntry->IsDataPrepared() )
		{
			// Data already available, simply return success.
			return AK_Success;
		}
	}
	else
	{
		// Add the missing entry
		pMediaEntry = m_MediaHashTable.Set( sourceID );
		if( !pMediaEntry )
			return AK_Fail;
		pMediaEntry->SetSourceID( sourceID );
	}
	AKASSERT( pMediaEntry );// From this point, we are guaranteed to have one. 

	AkUInt8* pAllocated = NULL;

	// Take the data and copy it over
	AKRESULT eResult = AK_Success;
	if( pMediaEntry->HasBankSource() )
	{
		// Will allocate pAllocated and set uMediaSize to the effective media size (i.e. size of pAllocated)
		eResult = pMediaEntry->PrepareFromBank(
			pAllocated,
			uMediaSize,
			CODECID_FROM_PLUGINID( in_rMediaInfo.dwID )
			);
	}
	else
	{
		// Stream/Load it
		AkUInt32 uAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;
		AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID( uAlignment, CODECID_FROM_PLUGINID( in_rMediaInfo.dwID ) )

		if( HOOKEDPOOLID != AK_INVALID_POOL_ID )
			pAllocated = (AkUInt8*)ALLOCWITHHOOK( uMediaSize, uAlignment );
		else
			MONITOR_ERRORMSG( AKTEXT("No memory space specified for preparing data. Make sure you specified a valid memory pool ID in the init parameter: AkInitSettings::uPrepareEventMemoryPoolID.") );

		if ( pAllocated )
		{
			m_MediaLock.Unlock();// no need to keep the lock during the IO. pMediaEntry is safe as it is already addref'ed at this point.
			eResult = LoadSoundFromFile( in_rMediaInfo, pAllocated );
			m_MediaLock.Lock();
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	if( eResult == AK_Success )
	{
		AKASSERT( pAllocated != NULL );
		pMediaEntry->SetPreparedData( pAllocated, uMediaSize );
	}
	else
	{
		// Failed, clear the memory...
		if ( pAllocated )
		{
			FREEWITHHOOK( pAllocated );
		}
	
		if( pMediaEntry->Release() == 0 ) // if the memory was released
		{
			m_MediaHashTable.Unset( sourceID );
		}
	}

	return eResult;
}

void CAkBankMgr::ReleaseSingleMedia( AkUniqueID in_SourceID )
{
	AkAutoLock<CAkLock> gate( m_MediaLock );

	AkMediaHashTable::IteratorEx iter = m_MediaHashTable.FindEx( in_SourceID );

	if( iter != m_MediaHashTable.End() )
	{
		AkMediaEntry& rMediaEntry = iter.pItem->Assoc.item;
		if( rMediaEntry.Release() == 0 ) // if the memory was released
		{
			m_MediaHashTable.Erase( iter );
		}
	}
}

void CAkBankMgr::SignalLastBankUnloaded()
{
	CAkFunctionCritical SpaceSetAsCritical;

	// Clear bank states
	g_pStateMgr->RemoveAllStateGroups( true );
}

AKRESULT CAkBankMgr::LoadSource(AkUInt8*& io_pData, AkUInt32 &io_ulDataSize, AkBankSourceData& out_rSource)
{
	memset(&out_rSource, 0, sizeof(out_rSource));

	//Read Source info
	out_rSource.m_PluginID = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);

	AkUInt32 StreamType = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);	

	out_rSource.m_MediaInfo.sourceID		= READBANKDATA( AkUInt32, io_pData, io_ulDataSize );
	out_rSource.m_MediaInfo.uFileID		= READBANKDATA( AkFileID, io_pData, io_ulDataSize );
	if ( StreamType != SourceType_Streaming )
	{
		out_rSource.m_MediaInfo.uFileOffset		 = READBANKDATA( AkUInt32, io_pData, io_ulDataSize );//in bytes
		out_rSource.m_MediaInfo.uInMemoryMediaSize = READBANKDATA( AkUInt32, io_pData, io_ulDataSize );//in bytes
	}
	else
	{
		out_rSource.m_MediaInfo.uFileOffset = 0;
		out_rSource.m_MediaInfo.uInMemoryMediaSize = 0;
	}	

	AkUInt8 uSourceBits = READBANKDATA( AkUInt8, io_pData, io_ulDataSize );
	out_rSource.m_MediaInfo.bIsLanguageSpecific = (uSourceBits >> BANK_SOURCE_LANGUAGE_BITSHIFT) & 1;
	out_rSource.m_MediaInfo.bIsFromRSX = (uSourceBits >> BANK_SOURCE_ISSTREAMEDFROMRSX_BITSHIFT) & 1;
	out_rSource.m_MediaInfo.bPrefetch		= ( StreamType == SourceType_PrefetchStreaming );

	AkPluginType PluginType = (AkPluginType) ( out_rSource.m_PluginID & AkPluginTypeMask );

	if( PluginType == AkPluginTypeCodec )
	{
		if ( StreamType == SourceType_Data )
		{
			// In-memory source.
			out_rSource.m_MediaInfo.Type = SrcTypeMemory;
		}
		else
		{
			// Streaming file source.
			if( StreamType != SourceType_Streaming &&
				StreamType != SourceType_PrefetchStreaming )
			{
				return AK_Fail;
			}
			out_rSource.m_MediaInfo.Type = SrcTypeFile;
		}
	}
	// Set source (according to type).
	else if ( PluginType == AkPluginTypeMotionSource || PluginType == AkPluginTypeSource )
	{
		out_rSource.m_uSize = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);
		out_rSource.m_pParam = io_pData;

		// Skip BLOB.
		io_pData += out_rSource.m_uSize;
		io_ulDataSize -= out_rSource.m_uSize;
	}
	else if ( PluginType != AkPluginTypeNone )
	{
		//invalid PluginType
		//do not assert here as this will cause a pseudo-deadlock
		//if we are loading the banks in synchronous mode (WG-1592)
		return AK_Fail;
	}

	return AK_Success;
}

AKRESULT CAkBankMgr::KillSlot( CAkUsageSlot * in_pUsageSlot, AkBankCallbackFunc in_pCallBack, void* in_pCookie )
{
	in_pUsageSlot->RemoveContent();			

	// Prepare bank for final release notification
	in_pUsageSlot->m_pfnBankCallback	= in_pCallBack;
	in_pUsageSlot->m_pCookie			= in_pCookie;

	AkQueuedMsg Item( QueuedMsgType_KillBank );
	Item.killbank.pUsageSlot = in_pUsageSlot;

	MONITOR_LOADEDBANK( in_pUsageSlot, false );// Must be done before enqueuing the command as the pUsageSlot could be destroyed after that.

	in_pUsageSlot->m_bUsageProhibited = true;
	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_KillBank() );

	return AK::SoundEngine::RenderAudio();
}
///////////////////////////////////////////////////////////////////////////////////////
// class AkMediaEntry
///////////////////////////////////////////////////////////////////////////////////////

void AkMediaEntry::AddRef()
{
	++uRefCount;
}

AkUInt32 AkMediaEntry::Release()
{
	AKASSERT( uRefCount );// Should never be 0 at this point
	if( !(--uRefCount) )
	{
		if( m_preparedMediaInfo.pInMemoryData )
		{
			FreeMedia();
		}
		MONITOR_MEDIAPREPARED( *this );
	}
	return uRefCount;
}

AKRESULT AkMediaEntry::AddAlternateBank( AkUInt8* in_pData, AkUInt32 in_uSize, CAkUsageSlot* in_pSlot )
{
	return m_BankSlots.SetFirst( in_pSlot, AkMediaInfo( in_pData, in_uSize ) ) ? AK_Success: AK_InsufficientMemory;
}

void AkMediaEntry::RemoveAlternateBank( CAkUsageSlot* in_pSlot )
{
	m_BankSlots.UnsetSwap( in_pSlot );
}

void AkMediaEntry::SetPreparedData( 
		AkUInt8* in_pData, 
		AkUInt32 in_uSize
#ifndef PROXYCENTRAL_CONNECTED
		,AkUInt8* in_pAllocatedPtr
#endif
		)
	{
		m_preparedMediaInfo.uInMemoryDataSize = in_uSize;
		m_preparedMediaInfo.pInMemoryData = in_pData;
#ifndef PROXYCENTRAL_CONNECTED
		m_preparedMediaInfo.pAllocatedPtr = in_pAllocatedPtr;
#endif
		AK_PERF_INCREMENT_PREPARED_MEMORY( in_uSize );
		MONITOR_MEDIAPREPARED( *this );
	}

void AkMediaEntry::GetMedia( AkMediaInfo& out_mediaInfo, CAkUsageSlot* &out_pUsageSlot )
{
	AddRef();

	if( m_preparedMediaInfo.pInMemoryData != NULL )
	{
		// We have prepared media, let's use it
		AKASSERT( m_preparedMediaInfo.uInMemoryDataSize );
		out_mediaInfo = m_preparedMediaInfo;
	}
	else
	{
		// No dynamic data available, check for data from bank.
		bool bFound = false;
		for( AkUInt32 i = 0; i < m_BankSlots.Length(); ++i )
		{
			if( !m_BankSlots[i].key->m_bUsageProhibited )
			{
				out_pUsageSlot = m_BankSlots[i].key;
				out_mediaInfo = m_BankSlots[i].item; // Get data + size from bank

				out_pUsageSlot->AddRef(); // AddRef the Slot as one sound will be using it

				bFound = true;
				break;
			}
		}

		if( !bFound )
		{
			// We end here in case of error or if data was not ready.

			// No AddRef() if not enough memory to trace it. So we simply release.
			// or
			// The entry is in the table, but no data is not yet ready.
			out_mediaInfo.pInMemoryData = NULL;
			out_mediaInfo.uInMemoryDataSize = 0;
			Release();// cancel the AddRef done at the beginning of the function.
		}
	}
}

void AkMediaEntry::FreeMedia()
{
	AKASSERT( m_preparedMediaInfo.uInMemoryDataSize );

#ifndef PROXYCENTRAL_CONNECTED
	AK::FreeHook( m_preparedMediaInfo.pAllocatedPtr );
	m_preparedMediaInfo.pAllocatedPtr = NULL;
#else
	FREEWITHHOOK( m_preparedMediaInfo.pInMemoryData );
#endif
	m_preparedMediaInfo.pInMemoryData = NULL;
	
	AK_PERF_DECREMENT_PREPARED_MEMORY( m_preparedMediaInfo.uInMemoryDataSize );
	m_preparedMediaInfo.uInMemoryDataSize = 0;
}

AKRESULT AkMediaEntry::PrepareFromBank(
	AkUInt8*& out_pAllocatedData,
	AkUInt32& out_uMediaSize,
	AkCodecID in_codecID // Specify if you know it, otherwise pass 0
)
{
#ifndef PROXYCENTRAL_CONNECTED
	AKASSERT( !"Illegal to Prepare data when running the Wwise sound engine" );
#endif

	AKASSERT( !m_preparedMediaInfo.pInMemoryData && !m_preparedMediaInfo.uInMemoryDataSize );
	AKASSERT( m_BankSlots.Length() != 0 );

	out_uMediaSize = m_BankSlots[0].item.uInMemoryDataSize;

	AkUInt32 uAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;
	AK_BANK_ADJUST_DATA_ALIGNMENT_FROM_CODECID_OR_MEDIA_SIZE( uAlignment, in_codecID, out_uMediaSize );

	if( HOOKEDPOOLID != AK_INVALID_POOL_ID )
	{
		out_pAllocatedData = (AkUInt8*)ALLOCWITHHOOK( out_uMediaSize, uAlignment );
	}
	else
	{
		MONITOR_ERRORMSG( AKTEXT("No memory space specified for preparing data. Make sure you specified a valid memory pool ID in the init parameter: AkInitSettings::uPrepareEventMemoryPoolID.") );
		out_pAllocatedData = NULL;
	}

	if ( ! out_pAllocatedData )
		return AK_InsufficientMemory;

	memcpy( out_pAllocatedData, m_BankSlots[0].item.pInMemoryData, out_uMediaSize );

#if defined AK_WII_FAMILY
	DCStoreRange( out_pAllocatedData, out_uMediaSize );
#endif	

	return AK_Success;
}


