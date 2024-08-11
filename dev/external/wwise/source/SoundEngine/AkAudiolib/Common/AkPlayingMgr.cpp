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
// AkPlayingMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkPlayingMgr.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkPBI.h"
#include "AkTransportAware.h"
#include "AkActionPlayAndContinue.h"
#include "AkEvent.h"
#include "AkAudioMgr.h"
#include "AkParameterNodeBase.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkURenderer.h"
#include "AkRanSeqBaseInfo.h"
#include "AkPositionRepository.h"
#include "PrivateStructures.h"
#include "AkAudioLib.h"

extern CAkAudioMgr*		g_pAudioMgr;

AKRESULT CAkPlayingMgr::Init()
{
	AKRESULT err = m_CallbackEvent.Init();
	if (err != AK_Success)
		return err;

	m_CallbackEvent.Signal();

	m_PlayingMap.Init();

	g_pBusCallbackMgr = &m_BusCallbackMgr;

	return AK_Success;
}

void CAkPlayingMgr::Term()
{
#ifndef AK_OPTIMIZED
	for( AkPlayingMap::Iterator iter = m_PlayingMap.Begin(); iter != m_PlayingMap.End(); ++iter )
		iter.pItem->m_PBIList.Term();

#endif

	m_PlayingMap.Term();
}

AKRESULT CAkPlayingMgr::AddPlayingID( AkQueuedMsg_EventBase & in_event,
									 AkCallbackFunc in_pfnCallback,
									 void*			in_pCookie,
									 AkUInt32		in_uiRegisteredNotif,
									 AkUniqueID		in_id )
{
	AkAutoLock<CAkLock> lock( m_csMapLock );

	PlayingMgrItem* pItem;
	AkNew2(pItem, g_DefaultPoolId, PlayingMgrItem, PlayingMgrItem());
	if ( !pItem )
		return AK_Fail;

#ifdef AK_OPTIMIZED
	pItem->cPBI = 0;
#endif

	pItem->cAction = 1;
	pItem->eventID = in_id;
	pItem->GameObj = in_event.GameObjID;
	pItem->userParam.Init(in_event.PlayingID, in_event.CustomParam);
	pItem->pfnCallback = in_pfnCallback;
	pItem->pCookie = in_pCookie;
	if ( !in_pfnCallback )
	{
		in_uiRegisteredNotif &= (~( AK_CallbackBits ) | AK_WWISE_MARKER_NOTIFICATION_FLAGS ); // remove callback bits if no callback, except for markers.
	}
	pItem->uiRegisteredNotif = in_uiRegisteredNotif;

	m_PlayingMap.Set( pItem );

	return AK_Success;
}

void CAkPlayingMgr::CancelCallbackCookie( void* in_pCookie )
{
	m_csMapLock.Lock();
	for ( AkPlayingMap::Iterator iter = m_PlayingMap.Begin(); iter != m_PlayingMap.End(); ++iter )
	{
		if( iter.pItem->pCookie == in_pCookie )
		{
			iter.pItem->pfnCallback = NULL;
			iter.pItem->uiRegisteredNotif &= ~( AK_CallbackBits ); // remove callback bits if no callback
		}
	}
	m_csMapLock.Unlock();

	if (g_pAudioMgr->GetThreadID() != AKPLATFORM::CurrentThread())
	m_CallbackEvent.Wait();	//Wait until any callback are done.
}

void CAkPlayingMgr::CancelCallback( AkPlayingID in_playingID )
{
	m_csMapLock.Lock();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_playingID );
	if ( pItem )
	{
		pItem->pfnCallback = NULL;
		pItem->uiRegisteredNotif &= ~( AK_CallbackBits ); // remove callback bits if no callback
	}
	m_csMapLock.Unlock();

	if (g_pAudioMgr->GetThreadID() != AKPLATFORM::CurrentThread())
	m_CallbackEvent.Wait();	//Wait until any callback are done.
}

void CAkPlayingMgr::CheckRemovePlayingID( AkPlayingID in_PlayingID, PlayingMgrItem* in_pItem )
{
#ifndef AK_OPTIMIZED
	if( in_pItem->m_PBIList.IsEmpty() && in_pItem->cAction == 0 )
	{
		in_pItem->m_PBIList.Term();
#else
	if( in_pItem->cPBI == 0 && in_pItem->cAction == 0 )
	{
#endif				
		if( in_pItem->uiRegisteredNotif & AK_EnableGetSourcePlayPosition )
		{
			g_pPositionRepository->RemovePlayingID( in_PlayingID );
		}
		
		AkEventCallbackInfo info;
		info.pCookie = in_pItem->pCookie;
		info.gameObjID = in_pItem->GameObj;
		info.playingID = in_PlayingID;
		info.eventID = in_pItem->eventID;		
		AkCallbackFunc pFunc = in_pItem->pfnCallback;
		AkUInt32 notif = in_pItem->uiRegisteredNotif;
		
		MONITOR_EVENTENDREACHEDNOTIF( in_PlayingID, in_pItem->GameObj, in_pItem->userParam.CustomParam(), in_pItem->uniqueID );	

  		m_PlayingMap.Unset( in_PlayingID );
		AkDelete(g_DefaultPoolId, in_pItem);

		if( notif & AK_EndOfEvent )
		{
			AKASSERT( pFunc );

			//Don't call the callback in the general lock, but make sure the callback won't be removed by blocking
			//the CancelCallback
			m_CallbackEvent.Reset();
			m_csMapLock.Unlock();
			(*pFunc)( AK_EndOfEvent, &info );
			m_CallbackEvent.Signal();
		}
		else
			m_csMapLock.Unlock();
	}
	else
		m_csMapLock.Unlock();
}


void CAkPlayingMgr::NotifyEndOfDynamicSequenceItem(
		AkPlayingID in_PlayingID,
		AkUniqueID in_audioNodeID,
		void* in_pCustomInfo
		)
{
	m_csMapLock.Lock();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if ( pItem && pItem->uiRegisteredNotif & AK_EndOfDynamicSequenceItem )
	{
		AKASSERT( pItem->pfnCallback );
		AkDynamicSequenceItemCallbackInfo info;

		info.pCookie = pItem->pCookie;
		info.gameObjID = pItem->GameObj;

		info.playingID = in_PlayingID;
		info.audioNodeID = in_audioNodeID;
		info.pCustomInfo = in_pCustomInfo;

		AkCallbackFunc pFunc = pItem->pfnCallback;
		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( AK_EndOfDynamicSequenceItem, &info );
		m_CallbackEvent.Signal();
	}
	else
		m_csMapLock.Unlock();
}

void CAkPlayingMgr::NotifyDuration(
		AkPlayingID in_PlayingID,
		AkReal32 in_fDuration,
		AkReal32 in_fEstimatedDuration,
		AkUniqueID in_idAudioNode
		)
{
	m_csMapLock.Lock();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if ( pItem && pItem->uiRegisteredNotif & AK_Duration )
	{
		AKASSERT( pItem->pfnCallback );
		AkDurationCallbackInfo info;

		info.pCookie = pItem->pCookie;
		info.gameObjID = pItem->GameObj;

		info.eventID = pItem->eventID;
		info.playingID = in_PlayingID;
		
		info.fDuration = in_fDuration;
		info.fEstimatedDuration = in_fEstimatedDuration;
		info.audioNodeID = in_idAudioNode;

		AkCallbackFunc pFunc = pItem->pfnCallback;
		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( AK_Duration, &info );
		m_CallbackEvent.Signal();
	}
	else
		m_csMapLock.Unlock();
}

void CAkPlayingMgr::NotifyMusicPlayStarted( AkPlayingID in_PlayingID )
{
	m_csMapLock.Lock();

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if ( pItem 
		&& pItem->uiRegisteredNotif & AK_MusicPlayStarted 
		&& pItem->pfnCallback )
	{
		AkEventCallbackInfo info;

		info.pCookie = pItem->pCookie;
		info.gameObjID = pItem->GameObj;
		info.playingID = in_PlayingID;
		info.eventID = pItem->eventID;

		AkCallbackFunc pFunc = pItem->pfnCallback;
		
		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( AK_MusicPlayStarted, &info );
		m_CallbackEvent.Signal();
	}
	else
		m_csMapLock.Unlock();
}

// io_uSelection and io_uItemDone must contain valid values before function call.
void CAkPlayingMgr::MusicPlaylistCallback( AkPlayingID in_PlayingID, AkUniqueID in_playlistID, AkUInt32 in_uNumPlaylistItems, AkUInt32& io_uSelection, AkUInt32& io_uItemDone )
{
	m_csMapLock.Lock();

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if ( pItem 
		&& pItem->uiRegisteredNotif & AK_MusicPlaylistSelect
		&& pItem->pfnCallback )
	{
		AkMusicPlaylistCallbackInfo info;

		info.pCookie = pItem->pCookie;
		info.gameObjID = pItem->GameObj;
		info.playingID = in_PlayingID;
		info.eventID = pItem->eventID;

		info.playingID = in_PlayingID;
		info.uNumPlaylistItems = in_uNumPlaylistItems;
		info.uPlaylistSelection = io_uSelection;
		info.uPlaylistItemDone = io_uItemDone;

		AkCallbackFunc pFunc = pItem->pfnCallback;

		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( AK_MusicPlaylistSelect, &info );
		m_CallbackEvent.Signal();

		io_uSelection = info.uPlaylistSelection;
		io_uItemDone = info.uPlaylistItemDone;
	}
	else
		m_csMapLock.Unlock();
}

void CAkPlayingMgr::NotifyMarker( CAkPBI* in_pPBI, AkAudioMarker* in_pMarker )
{
	m_csMapLock.Lock();

	AkPlayingID l_playingID = in_pPBI->GetPlayingID();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( l_playingID );
	AKASSERT( pItem );

	MONITOR_EVENTMARKERNOTIF( l_playingID, pItem->GameObj, pItem->userParam.CustomParam(), pItem->uniqueID, in_pMarker->strLabel );

	if( (pItem->uiRegisteredNotif & AK_Marker) && pItem->pfnCallback)
	{
		AkMarkerCallbackInfo info;

		info.pCookie = pItem->pCookie;
		info.gameObjID = pItem->GameObj;
		info.playingID = l_playingID;
		info.eventID = pItem->eventID;

		info.uIdentifier = in_pMarker->dwIdentifier;
		info.uPosition = in_pMarker->dwPosition;
		info.strLabel = in_pMarker->strLabel;
			
		AkCallbackFunc pFunc = pItem->pfnCallback;
		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( AK_Marker, &info );
		m_CallbackEvent.Signal();
	}
	else
		m_csMapLock.Unlock();	
}

void CAkPlayingMgr::NotifySpeakerVolumeMatrix(
	AkPlayingID in_PlayingID,
	AkSpeakerVolumeMatrixCallbackInfo* in_pInfo
	)
{
	m_csMapLock.Lock();

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if ( pItem && ( pItem->uiRegisteredNotif & AK_SpeakerVolumeMatrix ) )
	{
		in_pInfo->pCookie = pItem->pCookie;
		in_pInfo->gameObjID = pItem->GameObj;
		in_pInfo->playingID = in_PlayingID;
		in_pInfo->eventID = pItem->eventID;

		AkCallbackFunc pFunc = pItem->pfnCallback;
		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( AK_SpeakerVolumeMatrix, in_pInfo );
		m_CallbackEvent.Signal();
	}
	else
		m_csMapLock.Unlock();
}

void CAkPlayingMgr::NotifyMarkers( AkPipelineBuffer& io_CurrMarkerBuffer )
{
	// Notify the markers if needed
	if( io_CurrMarkerBuffer.pMarkers )
	{
		AkBufferMarker* pCurrMarker = io_CurrMarkerBuffer.pMarkers;
		for( AkUInt32 iCurrMarker = 0; iCurrMarker < io_CurrMarkerBuffer.uNumMarkers; iCurrMarker++ )
		{
			NotifyMarker( pCurrMarker->pContext, &pCurrMarker->marker );

			pCurrMarker++;
		}

		io_CurrMarkerBuffer.FreeMarkers();
	}
}

void CAkPlayingMgr::NotifyMusic( AkPlayingID in_PlayingID, AkCallbackType in_NotifType, const AkMusicGrid& in_rGrid )
{
	m_csMapLock.Lock();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	AKASSERT( pItem );

	if( pItem->pfnCallback &&
		pItem->uiRegisteredNotif & in_NotifType )
	{
		AkMusicSyncCallbackInfo info;
		PrepareMusicNotification( in_PlayingID, pItem, in_NotifType, in_rGrid, NULL, info );

		AkCallbackFunc pFunc = pItem->pfnCallback;
		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( in_NotifType, &info );
		m_CallbackEvent.Signal();
	}
	else
		m_csMapLock.Unlock();
}

void CAkPlayingMgr::NotifyMusicUserCues( AkPlayingID in_PlayingID, const AkMusicGrid& in_rGrid, char * in_pszUserCueName )
{
	m_csMapLock.Lock();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	AKASSERT( pItem );

	MONITOR_EVENTMARKERNOTIF( in_PlayingID, pItem->GameObj, pItem->userParam.CustomParam(), pItem->uniqueID, in_pszUserCueName );

	if( pItem->pfnCallback &&
		pItem->uiRegisteredNotif & AK_MusicSyncUserCue )
	{
		AkMusicSyncCallbackInfo info;
		PrepareMusicNotification( in_PlayingID, pItem, AK_MusicSyncUserCue, in_rGrid, in_pszUserCueName, info );
			
		AkCallbackFunc pFunc = pItem->pfnCallback;
		m_CallbackEvent.Reset();
		m_csMapLock.Unlock();
		(*pFunc)( AK_MusicSyncUserCue, &info );
		m_CallbackEvent.Signal();
	}
	else
		m_csMapLock.Unlock();
}

void CAkPlayingMgr::PrepareMusicNotification( AkPlayingID in_PlayingID, PlayingMgrItem* in_pItem, AkCallbackType in_NotifType, const AkMusicGrid& in_rGrid, char * in_pszUserCueName, AkMusicSyncCallbackInfo & out_info )
{
	out_info.pCookie = in_pItem->pCookie;
	out_info.gameObjID = in_pItem->GameObj;

	out_info.playingID = in_PlayingID;
	out_info.musicSyncType = in_NotifType;

	out_info.fBarDuration = ((AkReal32)in_rGrid.uBarDuration) / AK_CORE_SAMPLERATE;
	out_info.fBeatDuration = ((AkReal32)in_rGrid.uBeatDuration) / AK_CORE_SAMPLERATE;

	out_info.fGridDuration = ((AkReal32)in_rGrid.uGridDuration) / AK_CORE_SAMPLERATE;
	out_info.fGridOffset = ((AkReal32)in_rGrid.uGridOffset) / AK_CORE_SAMPLERATE;

	out_info.pszUserCueName = in_pszUserCueName;
}

AKRESULT CAkPlayingMgr::SetPBI( AkPlayingID in_PlayingID, CAkTransportAware* in_pPBI, AkUInt32 * out_puRegisteredNotif )
{
	AKASSERT( out_puRegisteredNotif );

	AkAutoLock<CAkLock> lock( m_csMapLock );

	AKRESULT eResult = AK_Success;

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	
	if (pItem)
	{
#ifndef AK_OPTIMIZED
		if ( !pItem->m_PBIList.AddLast( in_pPBI ) )
			eResult = AK_Fail;
		else 
			*out_puRegisteredNotif = pItem->uiRegisteredNotif;
#else
		pItem->cPBI++;
		*out_puRegisteredNotif = pItem->uiRegisteredNotif;
#endif
	}

	return eResult;
}

void CAkPlayingMgr::Remove(AkPlayingID in_PlayingID, CAkTransportAware* in_pPBI)
{
	m_csMapLock.Lock();

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if( pItem )
	{
#ifndef AK_OPTIMIZED
		pItem->m_PBIList.Remove(in_pPBI); // Not RemoveSwap: order is important
#else
		pItem->cPBI--;
#endif
		CheckRemovePlayingID( in_PlayingID, pItem );
	}
	else
		m_csMapLock.Unlock();

	//WARNING, unorthodox locking... CheckRemovePlayingID unlocks the lock we have taken.
}

bool CAkPlayingMgr::IsActive(AkPlayingID in_PlayingID)
{
	AkAutoLock<CAkLock> lock( m_csMapLock );

	return ( m_PlayingMap.Exists(in_PlayingID) != NULL);
}

void CAkPlayingMgr::AddItemActiveCount(AkPlayingID in_PlayingID)
{
	if( in_PlayingID )
	{
		AkAutoLock<CAkLock> lock( m_csMapLock );

		PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );

		if ( pItem )
			++pItem->cAction;
	}
}

void CAkPlayingMgr::RemoveItemActiveCount(AkPlayingID in_PlayingID)
{
	m_csMapLock.Lock();

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	// We must check since the action may not have been given a valid Playing ID, 
	// It is not an error, it will happen for example when a Duck Action is kicked from the list on terminaison
	if( pItem )
	{
		--( pItem->cAction );
		AKASSERT( pItem->cAction >= 0 );
		CheckRemovePlayingID( in_PlayingID, pItem );
	}
	else
		m_csMapLock.Unlock();
	
	//WARNING, unorthodox locking... CheckRemovePlayingID unlocks the lock we have taken.
}

AkUniqueID CAkPlayingMgr::GetEventIDFromPlayingID( AkPlayingID in_playingID )
{
	AkAutoLock<CAkLock> lock( m_csMapLock );

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_playingID );

	if ( pItem )
		return pItem->eventID;

	return AK_INVALID_UNIQUE_ID;
}

AkGameObjectID CAkPlayingMgr::GetGameObjectFromPlayingID( AkPlayingID in_playingID )
{
	AkAutoLock<CAkLock> lock( m_csMapLock );

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_playingID );

	if ( pItem )
		return pItem->GameObj;

	return AK_INVALID_GAME_OBJECT;
}

AKRESULT CAkPlayingMgr::GetPlayingIDsFromGameObject(
	AkGameObjectID in_GameObjId,	
	AkUInt32& io_ruNumIDs,
	AkPlayingID* out_aPlayingIDs
	)
{
	AkUInt32 uNumSpacesRemainingInArray;
	if( io_ruNumIDs == 0 )
	{
		// The user requested to count only.
		out_aPlayingIDs = NULL; // discard the array pointer if any
		uNumSpacesRemainingInArray = 0xFFFFFFFF;// AkUInt32Max - No Max
	}
	else
	{
		if( out_aPlayingIDs == NULL )
			return AK_InvalidParameter;

		uNumSpacesRemainingInArray = io_ruNumIDs;
	}

	io_ruNumIDs = 0;

	AkAutoLock<CAkLock> lock( m_csMapLock );
	for( AkPlayingMap::Iterator iter = m_PlayingMap.Begin(); iter != m_PlayingMap.End(); ++iter )
	{
		PlayingMgrItem* pItem = *iter;
		if( pItem->GameObj == in_GameObjId )
		{
			--uNumSpacesRemainingInArray;
			if( out_aPlayingIDs )
				out_aPlayingIDs[io_ruNumIDs] = pItem->userParam.PlayingID();
			++io_ruNumIDs;
		}

		if( uNumSpacesRemainingInArray == 0 )
		{
			break;
		}
	}

	/// return AK_Success if succeeded.
	return AK_Success;
}

#ifndef AK_OPTIMIZED

void CAkPlayingMgr::StopAndContinue(
		AkPlayingID				in_PlayingID,
		CAkRegisteredObj *		in_GameObjPtr,
		CAkContinueListItem&	in_ContinueListItem,
		AkUniqueID				in_ItemToPlay,
		AkUInt16				in_wPosition,
        CAkPBIAware*            in_pInstigator
		)
{
	AkAutoLock<CAkLock> lock( m_csMapLock );

	AKASSERT(g_pAudioMgr);
	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if( pItem )
	{
		pItem->cAction++;
		AkPendingAction* pPendingAction =  g_pAudioMgr->GetActionMatchingPlayingID(in_PlayingID);
		CAkActionPlayAndContinue* pActionPAC = NULL;
		if( pPendingAction )
		{
			AKASSERT( pPendingAction->pAction->ActionType() == AkActionType_PlayAndContinue );
			pActionPAC = static_cast<CAkActionPlayAndContinue*>(pPendingAction->pAction);
			if( pActionPAC->GetContinuationList() )
			{
				if( in_ContinueListItem.m_LoopingInfo.bIsEnabled )
				{
					CAkContinueListItem & itemFirst = *(pActionPAC->GetContinuationList()->m_listItems.Begin());

					in_ContinueListItem.m_LoopingInfo.lLoopCount = itemFirst.m_LoopingInfo.lLoopCount;
					if( itemFirst.m_pContainerInfo )
					{
						CAkSequenceInfo* pSeqInfo = static_cast<CAkSequenceInfo*>( itemFirst.m_pContainerInfo );
						if( pSeqInfo->m_i16LastPositionChosen == 0 && pSeqInfo->m_bIsForward )
						{
							in_ContinueListItem.m_LoopingInfo.lLoopCount += 1;
						}
					}
				}
			}
		}
		if( !( pItem->m_PBIList.IsEmpty() ) )
		{
			g_pAudioMgr->ClearPendingItems( in_PlayingID );
			
			size_t i = 0;
			for( PlayingMgrItem::AkPBIList::Iterator iter = pItem->m_PBIList.Begin(); iter != pItem->m_PBIList.End(); ++iter )
			{
				if( i < pItem->m_PBIList.Length() - 1 )
				{
					(*iter)->_Stop( AkPBIStopMode_StopAndContinueSequel, true );
				}
				else
				{
					(*iter)->_StopAndContinue( in_ItemToPlay, in_wPosition, &in_ContinueListItem );
				}
				++i;
			}
		}
		else// Must take info from a pending one
		{
			if(pPendingAction)
			{
				// create the action we need
				CAkSmartPtr<CAkContinuationList> l_spContList;
				l_spContList.Attach( CAkContinuationList::Create() );
				if( l_spContList )
				{
					AKRESULT eResult = l_spContList->m_listItems.AddLast( in_ContinueListItem ) ? AK_Success : AK_Fail;

					if( eResult == AK_Success )
					{
						CAkActionPlayAndContinue* pAction = CAkActionPlayAndContinue::Create( AkActionType_PlayAndContinue, 0, l_spContList );
						if( pAction )
						{
							PlayHistory History;
							History.HistArray.Init();
							History.Add( in_wPosition, true );
							pAction->SetHistory(History);
							WwiseObjectID wwiseId( in_ItemToPlay );
							pAction->SetElementID( wwiseId );
                            pAction->SetInstigator( in_pInstigator );

							AkPendingAction* pPendingAction2 = AkNew( g_DefaultPoolId, AkPendingAction( in_GameObjPtr ) );
							if( pPendingAction2 )
							{
								(*l_spContList->m_listItems.Begin()).m_LoopingInfo = (*pActionPAC->GetContinuationList()->m_listItems.Begin()).m_LoopingInfo;

								// copy the transitions we had in the pending action
								bool l_bIsFading;
								CAkTransition* l_pTransition = pActionPAC->GetPlayStopTransition( l_bIsFading );
								pAction->SetPlayStopTransition( l_pTransition, l_bIsFading, pPendingAction2 );
								l_pTransition = pActionPAC->GetPauseResumeTransition( l_bIsFading );
								pAction->SetPauseResumeTransition( l_pTransition, l_bIsFading, pPendingAction2 );

								pAction->SetPathInfo(pActionPAC->GetPathInfo());

								pPendingAction2->pAction = pAction;
								pPendingAction2->UserParam.Init(in_PlayingID, pPendingAction->UserParam.CustomParam());

								g_pAudioMgr->ClearPendingItemsExemptOne(in_PlayingID);

								g_pAudioMgr->EnqueueOrExecuteAction(pPendingAction2);
							}

							// We are done with these
							pAction->Release();
						}
					}
				}
			}
		}
		pItem->cAction--;
	}
}
#endif
