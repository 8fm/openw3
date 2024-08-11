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
// AkPlayingMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PLAYING_MGR_H_
#define _PLAYING_MGR_H_

#include "AkCommon.h"
#include "AkHashList.h"
#include <AK/Tools/Common/AkArray.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include "AkManualEvent.h"
#include "PrivateStructures.h"
#include "AkBusCallbackMgr.h"

class CAkContinueListItem;
class CAkPBI;
class CAkPBIAware;
class CAkTransportAware;
class CAkRegisteredObj;
class CAkSource;
struct AkQueuedMsg_EventBase;
struct AkQueuedMsg_Event;
struct AkQueuedMsg_OpenDynamicSequence;
struct AkMusicGrid;

const AkUInt32 AK_WWISE_MARKER_NOTIFICATION_FLAGS = AK_Marker | AK_MusicSyncUserCue;

class CAkPlayingMgr
{
public:
	AKRESULT Init();
	void Term();

	// Add a playing ID in the "Under observation" event list
	AKRESULT AddPlayingID( 
		AkQueuedMsg_EventBase & in_event,
		AkCallbackFunc in_pfnCallback,
		void * in_pCookie,
		AkUInt32 in_uiRegisteredNotif,
		AkUniqueID in_id
		);

	// Ask to not get notified for the given cookie anymore.
	void CancelCallbackCookie( void* in_pCookie );

	// Ask to not get notified for the given Playing ID anymore.
	void CancelCallback( AkPlayingID in_playingID );

	// Set the actual referenced PBI
	AKRESULT SetPBI(
		AkPlayingID in_PlayingID,	// Playing ID
		CAkTransportAware* in_pPBI,	// PBI pointer, only to be stored
		AkUInt32 * out_puRegisteredNotif // out param: registered notifications
		);

	void Remove(
		AkPlayingID in_PlayingID,	// Playing ID
		CAkTransportAware* in_pPBI	// PBI pointer
		);

	bool IsActive(
		AkPlayingID in_PlayingID // Playing ID
		);

	void AddItemActiveCount(
		AkPlayingID in_PlayingID // Playing ID
		);

	void RemoveItemActiveCount(
		AkPlayingID in_PlayingID // Playing ID
		);

	AkUniqueID GetEventIDFromPlayingID( AkPlayingID in_playingID );

	AkGameObjectID GetGameObjectFromPlayingID( AkPlayingID in_playingID );

	AKRESULT GetPlayingIDsFromGameObject(
		AkGameObjectID in_GameObjId,
		AkUInt32& io_ruNumIDs,
		AkPlayingID* out_aPlayingIDs
		);

#ifndef AK_OPTIMIZED
	void StopAndContinue(
		AkPlayingID			in_PlayingID,
		CAkRegisteredObj *	in_pGameObj,
		CAkContinueListItem&in_ContinueListItem,
		AkUniqueID			in_ItemToPlay,
		AkUInt16			in_wPosition,
        CAkPBIAware*        in_pInstigator
		);
#endif

	void NotifyEndOfDynamicSequenceItem(
		AkPlayingID in_PlayingID,
		AkUniqueID in_itemID,
		void* in_pCustomInfo
		);

	void NotifyMarker(
		CAkPBI* in_pPBI,					// PBI pointer
		AkAudioMarker* in_pMarkerInfo	// Marker being notified
		);

	void NotifyDuration(
		AkPlayingID in_PlayingID,
		AkReal32 in_fDuration,
		AkReal32 in_fEstimatedDuration,
		AkUniqueID in_idAudioNode
		);
		
	void NotifyMusicPlayStarted( AkPlayingID in_PlayingID );

	// io_uSelection and io_uItemDone must contain valid values before function call.
	void MusicPlaylistCallback( AkPlayingID in_PlayingID, AkUniqueID in_playlistID, AkUInt32 in_uNumPlaylistItems, AkUInt32& io_uSelection, AkUInt32& io_uItemDone );

	void NotifySpeakerVolumeMatrix(
		AkPlayingID in_PlayingID,
		AkSpeakerVolumeMatrixCallbackInfo* in_pInfo
		);

	void NotifyMarkers( AkPipelineBuffer& io_pCurrMarkerBuffer );

	void NotifyMusic( AkPlayingID in_PlayingID, AkCallbackType in_NotifType, const AkMusicGrid& in_rGrid );
	void NotifyMusicUserCues( AkPlayingID in_PlayingID, const AkMusicGrid& in_rGrid, char * in_pszUserCueName );

	inline unsigned int NumPlayingIDs() const { return m_PlayingMap.Length(); }

private:
	
	struct PlayingMgrItem
	{
		// Hold a list of PBI with Wwise, only keep count otherwise
	#ifndef AK_OPTIMIZED
		typedef AkArray<CAkTransportAware*, CAkTransportAware*, ArrayPoolDefault, 2> AkPBIList;
		AkPBIList m_PBIList;
	#else
		int cPBI;
	#endif
	
		int cAction; // count of actions
	
		AkUniqueID eventID;
		AkGameObjectID GameObj;
		UserParams userParam;
		AkCallbackFunc pfnCallback;
		void* pCookie;
		AkUInt32 uiRegisteredNotif;
		PlayingMgrItem* pNextItem;
	};

	struct PlayingMgrItemPolicy
	{
		static AkPlayingID Key(const PlayingMgrItem* in_pItem) {return in_pItem->userParam.PlayingID();}
	};

	void CheckRemovePlayingID( AkPlayingID in_PlayingID, PlayingMgrItem* in_pItem );

	void PrepareMusicNotification( AkPlayingID in_PlayingID, PlayingMgrItem* in_pItem, AkCallbackType in_NotifType, const AkMusicGrid& in_rGrid, char * in_pszUserCueName, AkMusicSyncCallbackInfo & out_info );

	typedef AkHashListBare<AkPlayingID, PlayingMgrItem, AK_SMALL_HASH_SIZE, PlayingMgrItemPolicy> AkPlayingMap;
	AkPlayingMap m_PlayingMap;
	CAkLock m_csMapLock;
	AkManualEvent m_CallbackEvent;

	CAkBusCallbackMgr m_BusCallbackMgr;
};

extern CAkPlayingMgr*			g_pPlayingMgr;
extern CAkBusCallbackMgr*		g_pBusCallbackMgr;

#endif
