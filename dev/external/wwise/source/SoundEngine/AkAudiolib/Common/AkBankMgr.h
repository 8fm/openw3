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
// AkBankMgr.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _BANK_MGR_H_
#define _BANK_MGR_H_

#include "AkBanks.h"
#include "AkBankReader.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Tools/Common/AkLock.h>
#include "AkAudioLibIndex.h"// PS3 requires it for unused templates. compiler specific problem.
#include "AkCritical.h"
#include "AkSource.h"
#include "AkList2.h"
#include "AkBankCallbackMgr.h"
#include "AkBankList.h"

class CAkBankMemMgr;
class CAkParameterNode;
class CAkUsageSlot;
class CAkBankMgr;

using namespace AKPLATFORM;
using namespace AkBank;

#define AK_MINIMUM_BANK_SIZE ( sizeof(AkSubchunkHeader) + sizeof( AkBankHeader ) )

// Could be the ID of the SoundBase object directly, maybe no need to create it.
typedef AkUInt32 AkMediaID; // ID of a specific source

extern CAkBankMgr*		g_pBankManager;

struct AkMediaInfo
{
	AkMediaInfo() // NO INITIALIZATION!
	{
	}

	AkMediaInfo( AkUInt8* in_pInMemoryData, AkUInt32 in_uInMemoryDataSize )
		: pInMemoryData( in_pInMemoryData )
		, uInMemoryDataSize( in_uInMemoryDataSize )
#ifndef PROXYCENTRAL_CONNECTED
		, pAllocatedPtr( NULL )
#endif
	{
	}

	AkUInt8*			pInMemoryData;
	AkUInt32			uInMemoryDataSize;
#ifndef PROXYCENTRAL_CONNECTED
	AkUInt8*			pAllocatedPtr;
#endif
};

class AkMediaEntry
{
	friend class AkMonitor; // for profiling purpose.
private:
	typedef CAkKeyArray< CAkUsageSlot*, AkMediaInfo, 1 > AkBankSlotsArray;

public:
	// Constructor
	AkMediaEntry()
		:uRefCount( 1 )
	{
		m_preparedMediaInfo.pInMemoryData = NULL;
		m_preparedMediaInfo.uInMemoryDataSize = 0;
#ifndef PROXYCENTRAL_CONNECTED
		m_preparedMediaInfo.pAllocatedPtr = NULL;
#endif
	}

	~AkMediaEntry()
	{
		AKASSERT( m_BankSlots.Length() == 0);
		m_BankSlots.Term();
	}

	void AddRef();
	AkUInt32 Release();

	void GetMedia( AkMediaInfo& out_mediaInfo, CAkUsageSlot* &out_pUsageSlot );
	void FreeMedia();

	AKRESULT AddAlternateBank( AkUInt8* in_pData, AkUInt32 in_uSize, CAkUsageSlot* in_pSlot );
	void RemoveAlternateBank( CAkUsageSlot* in_pSlot );

	bool IsDataPrepared()
	{ 
		return m_preparedMediaInfo.pInMemoryData != NULL; 
	}

	void SetPreparedData( 
		AkUInt8* in_pData, 
		AkUInt32 in_uSize
#ifndef PROXYCENTRAL_CONNECTED
		,AkUInt8* in_pAllocatedPtr = NULL
#endif
		);

	AkUInt8* GetPreparedMemoryPointer()
	{ 
		return m_preparedMediaInfo.pInMemoryData; 
	}

	bool HasBankSource()
	{
		return (m_BankSlots.Length() != 0);
	}

	AkUInt32 GetNumBankOptions()
	{
		return m_BankSlots.Length();
	}

	AKRESULT PrepareFromBank(
		AkUInt8*& out_pAllocatedData,
		AkUInt32& out_uMediaSize,
		AkCodecID in_codecID // Specify if you know it, otherwise pass 0
		);

	AkUniqueID GetSourceID(){ return m_sourceID; }
	void SetSourceID( AkUniqueID in_sourceID ){ m_sourceID = in_sourceID; }

private:

	// Members
	AkMediaInfo m_preparedMediaInfo;

	AkBankSlotsArray m_BankSlots;

	AkUInt32 uRefCount;

	AkUniqueID m_sourceID;
};

struct AkBankCompletionNotifInfo
{
	AkBankCallbackFunc pfnBankCallback;
    void * pCookie;
};

struct AkBankCompletionItem
{
	AkUniqueID BankID;
	AkMemPoolId memPoolId;
};

struct AkPrepareEventQueueItemLoad
{
	// TODO use the pointer instead of the ID here, will simplify things...
	AkUInt32	numEvents;
	// To avoid doing 4 bytes allocation, when the size is one, the ID itself will be passed instead of the array.
	union
	{
		AkUniqueID	eventID;
		AkUniqueID* pEventID;
	};
};

struct AkPrepareGameSyncQueueItem
{
	AkGroupType eGroupType;
	AkUInt32	uGroupID;
	bool		bSupported;

	AkUInt32	uNumGameSync;
	union
	{
		AkUInt32 uGameSyncID;
		AkUInt32* pGameSyncID;
	};
};

class CAkUsageSlot
{
public:
	AkBankKey key;		// for AkListLoadedBanks
	CAkUsageSlot * pNextItem;

public:

	AkUniqueID	m_BankID;
	AkUInt8 *	m_pData;
	AkUInt32	m_uLoadedDataSize;
	AkMemPoolId m_memPoolId;
	bool		m_bIsInternalPool;

	AkUInt32		m_uNumLoadedItems;
	AkUInt32		m_uIndexSize;
	AkBank::MediaHeader*	m_paLoadedMedia;
	
	// These two members are there only for the notify completion callback
    AkBankCallbackFunc m_pfnBankCallback;
    void * m_pCookie;

	typedef AkArray<CAkIndexable*, CAkIndexable*, ArrayPoolDefault, 0> AkListLoadedItem;
	AkListLoadedItem m_listLoadedItem;	//	Contains the list of CAkIndexable to release in case of unload

	CAkUsageSlot( AkUniqueID in_BankID, AkMemPoolId in_memPoolId, AkInt32 in_mainRefCount, AkInt32 in_prepareEventRefCount, AkInt32 in_prepareBankRefCount );
	~CAkUsageSlot();

	void AddRef();
	void Release( bool in_bSkipNotification );
	void AddRefPrepare();
	void ReleasePrepare( bool in_bIsFinal = false );

	void StopContent();
	void RemoveContent();
	void Unload();

	bool WasLoadedAsABank() const { return m_bWasLoadedAsABank; }
	void WasLoadedAsABank( bool in_bWasLoadedAsABank ){ m_bWasLoadedAsABank = in_bWasLoadedAsABank; }

	bool WasPreparedAsABank(){ return m_iWasPreparedAsABankCounter != 0; }
	void WasPreparedAsABank( bool in_bWasPreparedAsABank )
	{ 
		if( in_bWasPreparedAsABank )
			++m_iWasPreparedAsABankCounter;
		else
			--m_iWasPreparedAsABankCounter;
		AKASSERT( m_iWasPreparedAsABankCounter >=0 );
	}

	bool WasIndexAllocated() { return m_bWasIndexAllocated; }
	void WasIndexAllocated(bool in_bWasIndexAllocated ){ m_bWasIndexAllocated = in_bWasIndexAllocated; }

	bool IsMediaPrepared() { return m_bIsMediaPrepared; }
	void IsMediaPrepared( bool in_bInMediaPrepared ){ m_bIsMediaPrepared = in_bInMediaPrepared; }

	void CheckFreeIndexArray();

	void UnloadCompletionNotification();

private:
	AkInt32 m_iRefCount;
	AkInt32 m_iPrepareRefCount;
	
	AkUInt8 m_bWasLoadedAsABank		:1;
	AkUInt8 m_bWasIndexAllocated	:1;
	AkUInt8 m_bIsMediaPrepared		:1;
public:
	AkUInt8 m_bUsageProhibited		:1;

private:

	AkInt32 m_iWasPreparedAsABankCounter;
};

enum AkBankLoadFlag
{
	AkBankLoadFlag_None,
	AkBankLoadFlag_InMemory,
	AkBankLoadFlag_UsingFileID
};

struct AkBankSourceData
{
	AkUniqueID			m_srcID; 
	AkUInt32			m_PluginID;
	AkMediaInformation	m_MediaInfo;
	void *				m_pParam;
	AkUInt32			m_uSize;
};

// The Bank Manager
//		Manage bank slots allocation
//		Loads and unloads banks
class CAkBankMgr
{
	friend class CAkUsageSlot;
	friend class AkMonitor;
public:
	enum AkBankQueueItemType
	{
		QueueItemLoad,
		QueueItemUnload,
		QueueItemPrepareEvent,
		QueueItemUnprepareEvent,
		QueueItemSupportedGameSync,
		QueueItemUnprepareAllEvents,
		QueueItemPrepareBank,
		QueueItemUnprepareBank,
		QueueItemClearBanks,
		QueueItemLoadMediaFile,
		QueueItemUnloadMediaFile,
		QueueItemLoadMediaFileSwap
	};

	// Structure used to Store the information about a loaded bank
	// This information will mostly be used to identify a free Slot and keep information
	// on what has to be removed from the hyerarchy once removed

	struct AkBankQueueItemLoad
		: public AkBankCompletionItem
	{
		const void* pInMemoryBank;
		AkUInt32 ui32InMemoryBankSize;
	};

	struct AkBankPreparation
	{
		AkUniqueID						BankID;
		AK::SoundEngine::AkBankContent	uFlags;
	};

	struct AkLoadMediaFile
	{
		AkUniqueID	MediaID;
		char*		pszAllocatedFileName;
	};

	struct AkBankQueueItem
	{
		AkBankQueueItemType eType;
		AkBankCompletionNotifInfo callbackInfo;
		AkBankLoadFlag bankLoadFlag;
		union
		{
			AkPrepareEventQueueItemLoad prepare;
			AkBankQueueItemLoad load;
			AkPrepareGameSyncQueueItem gameSync;
			AkBankPreparation bankPreparation;
			AkLoadMediaFile loadMediaFile;
		};

		const void* GetFromMemPtr()
		{
			return bankLoadFlag == AkBankLoadFlag_InMemory ? load.pInMemoryBank : NULL;
		}

		AkBankKey GetBankKey()
		{
			AkBankID bankID = AK_INVALID_UNIQUE_ID;
			if( eType == QueueItemLoad || eType == QueueItemUnload )
			{
				bankID = load.BankID;
			}
			else if( eType == QueueItemPrepareBank || eType == QueueItemUnprepareBank )
			{
				bankID = bankPreparation.BankID;
			}

			AkBankKey bankKey( bankID, GetFromMemPtr() );
			return bankKey;
		}
	};

	// Constructor and Destructor
	CAkBankMgr();
	~CAkBankMgr();

	AKRESULT Init();
	AKRESULT Term();

	AKRESULT QueueBankCommand( AkBankQueueItem in_Item );

	void NotifyCompletion( AkBankQueueItem & in_rItem, AKRESULT in_OperationResult );

	void ClearPreparedEvents();

	AKRESULT SetBankLoadIOSettings( AkReal32 in_fThroughput, AkPriority in_priority ) { return m_BankReader.SetBankLoadIOSettings( in_fThroughput, in_priority ); }

	void StopBankContent( AkUniqueID in_BankID );

	// This template is to be used by all STD audionodes
	// Any audio node that has special requirements on bank loading ( as sounds and track ) should not use this template.
	// It is exposed in header file so that external plugins can use this template too
	template <class T_Type, class T_Index_Type>
	AKRESULT StdBankRead( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, CAkIndexItem<T_Index_Type*>& in_rIndex )
	{
		AKRESULT eResult = AK_Success;

		const void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

		if(!pData)
		{
			return AK_Fail;
		}

		AkUniqueID ulID = *(AkUniqueID*)( pData );

		T_Type* pObject = static_cast<T_Type*>( in_rIndex.GetPtrAndAddRef( ulID ) );
		if( pObject == NULL )
		{
			CAkFunctionCritical SpaceSetAsCritical;
			pObject = T_Type::Create(ulID);
			if( !pObject )
			{
				eResult = AK_Fail;
			}
			else
			{
				eResult = pObject->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize );
				if( eResult != AK_Success )
				{
					pObject->Release();
				}
			}
		}

		if(eResult == AK_Success)
		{
			AddLoadedItem( in_pUsageSlot,pObject ); //This will allow it to be removed on unload
		}

		m_BankReader.ReleaseData();

		return eResult;
	}

	template<class T>
	AKRESULT ReadSourceParent(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUInt32 /*in_dwBankID*/ )
	{
		AKRESULT eResult = AK_Success;

		const void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );
		if(!pData)
			return AK_Fail;

		AkUniqueID ulID = *(AkUniqueID*)(pData);

		T* pSound = static_cast<T*>( g_pIndex->GetNodePtrAndAddRef( ulID, AkNodeType_Default ) );
		if( pSound )
		{
			//The sound already exists, simply update it
			if( !pSound->SourceLoaded() || !pSound->HasBankSource() )
			{
				CAkFunctionCritical SpaceSetAsCritical;
				eResult = pSound->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize, in_pUsageSlot, true );
			}

			if(eResult != AK_Success)
				pSound->Release();
		}
		else
		{
			CAkFunctionCritical SpaceSetAsCritical;
			pSound = T::Create(ulID);
			if(!pSound)
			{
				eResult = AK_Fail;
			}
			else
			{
				eResult = pSound->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize, in_pUsageSlot, false );
				if(eResult != AK_Success)
				{
					pSound->Release();
				}
			}
		}

		if(eResult == AK_Success)
		{
			AddLoadedItem( in_pUsageSlot, pSound ); //This will allow it to be removed on unload
		}

		m_BankReader.ReleaseData();

		return eResult;
	}

	static AKRESULT LoadSource(AkUInt8* & io_pData, AkUInt32 &io_ulDataSize, AkBankSourceData& out_rSource);

	friend AKRESULT ReadTrack( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID );

	AKRESULT LoadSoundFromFile( AkSrcTypeInfo& in_rMediaInfo, AkUInt8* io_pData );


	AKRESULT LoadMediaIndex( CAkUsageSlot* in_pCurrentSlot, AkUInt32 in_uIndexChunkSize, bool in_bIsLoadedFromMemory );

	AKRESULT LoadMedia( AkUInt8* in_pDataBank, CAkUsageSlot* in_pCurrentSlot, bool in_bIsLoadedFromMemory );
	AKRESULT PrepareMedia( CAkUsageSlot* in_pCurrentSlot, AkUInt32 in_dwDataChunkSize );
	void UnloadMedia( CAkUsageSlot* in_pCurrentSlot );	// Works to cancel both Load and index.
	void UnPrepareMedia( CAkUsageSlot* in_pCurrentSlot );

	// Returns the best it can do. 
	// If the media is in memory, give it in memory.
	// If the media is prefetched, give the prefetch.
	AkMediaInfo GetMedia( AkMediaID in_mediaId, CAkUsageSlot* &out_pUsageSlot );

	// GetMedia() and ReleaseMedia() calls must be balanced. 
	void ReleaseMedia( AkMediaID in_mediaId );

	AKRESULT LoadSingleMedia( AkSrcTypeInfo& in_rmediaInfo );
	void ReleaseSingleMedia( AkUniqueID in_SourceID );

	AKRESULT PrepareSingleMedia( AkSrcTypeInfo& in_rmediaInfo );
	void UnprepareSingleMedia( AkUniqueID in_SourceID );

	void SetIsFirstBusLoaded( bool in_Loaded ){ m_bIsFirstBusLoaded = in_Loaded; }

	static void SignalLastBankUnloaded();

	static AkBankID GetBankIDFromInMemorySpace( const void* in_pData, AkUInt32 in_uSize );

	const char* GetBankFileName( AkBankID in_bankID );

	void UpdateBankName( AkBankID in_bankID, char* in_pStringWithoutExtension );

	static bool BankHasFeedback() {return g_pBankManager->m_bFeedbackInBank;}

	// Safe way to actually do the callback
	// Same prototype than the callback itself, with the exception that it may actually not do the callback if the
	// event was cancelled
	void DoCallback(
		AkBankCallbackFunc	in_pfnBankCallback,
		AkBankID			in_bankID,
		const void *		in_pInMemoryPtr,
		AKRESULT			in_eLoadResult,
		AkMemPoolId			in_memPoolId,
		void *				in_pCookie
		)
	{
		m_CallbackMgr.DoCallback( 
			in_pfnBankCallback,
			in_bankID,
			in_pInMemoryPtr,
			in_eLoadResult,
			in_memPoolId,
			in_pCookie 
			);
	}

	void CancelCookie( void* in_pCookie ){ m_CallbackMgr.CancelCookie( in_pCookie ); }

	void StopThread();

	static AkThreadID GetThreadID() {return m_idThread;}

private:

	enum AkLoadBankDataMode
	{
		AkLoadBankDataMode_OneBlock,			// Normal bank load, complete.
		AkLoadBankDataMode_MediaAndStructure,	// Prepare Structure AND Media.
		AkLoadBankDataMode_Structure,			// Prepare Structure only.
		AkLoadBankDataMode_Media				// Prepare Media only.
	};
	// Load the Specified bank
	AKRESULT LoadBank( AkBankQueueItem in_Item, CAkUsageSlot *& out_pUsageSlot, AkLoadBankDataMode in_eLoadMode, bool in_bIsFromPrepareBank );	

	AKRESULT LoadBankPre( AkBankQueueItem& in_rItem );
	// Called upon an unload request, the Unload may be delayed if the Bank is un use.
	AKRESULT UnloadBankPre( AkBankQueueItem in_Item );

	AKRESULT ClearBanksInternal( AkBankQueueItem in_Item );

	AKRESULT PrepareEvents( AkBankQueueItem in_Item );
	AKRESULT PrepareEvent( AkBankQueueItem in_Item, AkUniqueID in_EventID );
	AKRESULT UnprepareEvents( AkBankQueueItem in_Item );
	AKRESULT UnprepareEvent( AkUniqueID in_EventID );
	void	 UnprepareEvent( CAkEvent* in_pEvent, bool in_bCompleteUnprepare = false );
	AKRESULT UnprepareAllEvents( AkBankQueueItem in_Item );

	AKRESULT PrepareGameSync( AkBankQueueItem in_Item );

	AKRESULT PrepareBank( AkBankQueueItem in_Item );
	AKRESULT UnPrepareBank( AkBankQueueItem in_Item );

	AKRESULT LoadMediaFile( AkBankQueueItem in_Item );
	AKRESULT UnloadMediaFile( AkBankQueueItem in_Item );
	AKRESULT MediaSwap( AkBankQueueItem in_Item );

	struct AkAlignedPtrSet
	{
		AkUInt8* pAllocated;
		AkUInt8* pAlignedPtr;
		AkUInt32 uMediaSize;
	};

	AKRESULT LoadMediaFile( char* in_pszFileName, AkAlignedPtrSet& out_AlignedPtrSet );

	// WG-9055 - do not pass AkBankQueueItem in_rItem by reference.
	AKRESULT PrepareBankInternal( AkBankQueueItem in_rItem, AkFileID in_FileID, AkLoadBankDataMode in_LoadBankMode, bool in_bIsFromPrepareBank = false );
	void UnPrepareBankInternal( AkUniqueID in_BankID, bool in_bIsFromPrepareBank = false, bool in_bIsFinal = false );

	//NOT TO BE CALLED DIRECTLY, internal tool only
	void UnloadAll();
	void UnPrepareAllBank();

	//Add the CAkIndexable* to the ToBeRemoved list associated with this slot
	void AddLoadedItem( CAkUsageSlot* in_pUsageSlot, CAkIndexable* in_pIndexable);

	AKRESULT ProcessBankHeader( AkBankHeader& in_rBankHeader );
	AKRESULT ProcessDataChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ProcessHircChunk( CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID );
	AKRESULT ProcessStringMappingChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ProcessGlobalSettingsChunk( AkUInt32 in_dwDataChunkSize );
	AKRESULT ProcessEnvSettingsChunk( AkUInt32 in_dwDataChunkSize );

	AKRESULT ReadState( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ReadAction( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ReadEvent( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );
	
	AKRESULT ReadBus( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ReadAuxBus( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );

#ifdef AK_MOTION
	AKRESULT ReadFeedbackBus(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot);
#endif // AK_MOTION

	CAkBankReader m_BankReader;

	AKRESULT SetFileReader( AkFileID in_FileID, AkUInt32 in_uFileOffset, AkUInt32 in_codecID, void * in_pCookie, bool in_bIsLanguageSpecific = true );

// threading
    static AK_DECLARE_THREAD_ROUTINE( BankThreadFunc );
	AKRESULT StartThread();

	AKRESULT ExecuteCommand();

	static AkThread	m_BankMgrThread;
	static AkThreadID m_idThread;

	AkEvent m_eventQueue;
	bool	m_bStopThread;


	CAkLock m_queueLock;
	CAkLock m_MediaLock;

	typedef CAkList2<AkBankQueueItem, const AkBankQueueItem&, AkAllocAndFree> AkBankQueue;
	AkBankQueue m_BankQueue;

	bool m_bIsFirstBusLoaded;

	// Hash table containing all ready media.
	// Actual problem : some of this information is a duplicate of the index content. but it would be a faster
	// seek in a hash table than in the linear indexes.
	// We could find better memory usage, but will be more complex too.
	typedef AkHashList< AkMediaID, AkMediaEntry, AK_LARGE_HASH_SIZE > AkMediaHashTable;
	AkMediaHashTable m_MediaHashTable;

	CAkBankList m_BankList;

	////////////////////////////////////////////////////////////////////////////////////////
	// This funstion is to be used with precautions.
	// You must acquire the CAkBankList lock for all the time you are manipulating this list.
	CAkBankList& GetBankListRef(){ return m_BankList; }
	////////////////////////////////////////////////////////////////////////////////////////

	void FlushFileNameTable();
	AKRESULT KillSlot( CAkUsageSlot * in_pUsageSlot, AkBankCallbackFunc in_pCallBack, void* in_pCookie );

	typedef AkHashList< AkBankID, char*, AK_SMALL_HASH_SIZE > AkIDtoStringHash;
	AkIDtoStringHash m_BankIDToFileName;

	bool m_bFeedbackInBank;

	CAkBankCallbackMgr m_CallbackMgr;

	struct AkSortedPreparationListGetKey
	{
		/// Default policy.
		static AkForceInline AkMediaInformation& Get( AkSrcTypeInfo& in_item ) 
		{
			return in_item.mediaInfo;
		}
	};
	typedef AkSortedKeyArray<AkMediaInformation, AkSrcTypeInfo, ArrayPoolDefault, AkSortedPreparationListGetKey, 8 > AkSortedPreparationList;

	void EnableAccumulation();
	void DisableAccumulation();
	AKRESULT ProcessAccumulated();

	AkSortedPreparationList m_PreparationAccumulator;
	bool					m_bAccumulating;
};

#endif



