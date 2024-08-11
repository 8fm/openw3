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
// AkSwitchCntr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkSwitchCntr.h"
#include "AkAudioLibIndex.h"
#include "PrivateStructures.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkAudioMgr.h"
#include "AkContinuationList.h"
#include "AkCntrHistory.h"
#include "AkTransitionManager.h"
#include "AkPathManager.h"
#include "AkPlayingMgr.h"
#include "AkPBI.h"
#include "AkRegistryMgr.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>

extern CAkRegistryMgr* g_pRegistryMgr;

//////////////////////////////////////////////////////////////////////
// CAkSwitchCntr
//////////////////////////////////////////////////////////////////////
CAkSwitchCntr::CAkSwitchCntr( AkUniqueID in_ulID )
: CAkMultiPlayNode( in_ulID )
, m_eGroupType( AkGroupType_Switch )
, m_ulGroupID( AK_INVALID_UNIQUE_ID )
, m_ulDefaultSwitch( AK_INVALID_UNIQUE_ID )
{
	// List initiated in the init function
}

CAkSwitchCntr::~CAkSwitchCntr()
{
	Term();
}

AKRESULT CAkSwitchCntr::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA( AkUInt32, in_pData, in_ulDataSize );

	//ReadParameterNode
	eResult = SetNodeBaseParams( in_pData, in_ulDataSize, false );
    if( eResult == AK_Success )
	{
		AkGroupType		l_groupType					= (AkGroupType) READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		AkUInt32		l_ulSwitchGroup				= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		AkUInt32		l_ulDefaultSwitch			= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

		AkUInt8		l_bIsContinuousValidation		= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

		eResult = SetSwitchGroup( l_ulSwitchGroup, l_groupType );
		if( eResult == AK_Success )
		{
			SetDefaultSwitch( l_ulDefaultSwitch );
			eResult = SetContinuousValidation( l_bIsContinuousValidation != 0 );
		}
	}

	if(eResult == AK_Success)
	{
		eResult = SetChildren( in_pData, in_ulDataSize );
	}

	if(eResult == AK_Success)
	{
		//Process Switching options
		AkUInt32 ulNumSwitchGroups = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		m_SwitchList.Reserve( ulNumSwitchGroups );

		for( AkUInt32 i = 0; i < ulNumSwitchGroups; ++i )
		{
			//Read the Actual Switch ID
			AkUInt32 ulSwitchID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

			CAkSwitchPackage * pPack = AddSwitch( ulSwitchID );
			if( !pPack )
			{
				eResult = AK_Fail;
				break;
			}

			AkUInt32 ulNumItems = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPack->m_list.Reserve( ulNumItems );

			for( AkUInt32 j = 0; j < ulNumItems; ++j )
			{
				AkUniqueID l_ID = READBANKDATA( AkUniqueID, in_pData, in_ulDataSize );

				eResult = AddNodeInSwitch( pPack, l_ID );
				if( eResult != AK_Success )
					break;
			}

			if( eResult != AK_Success )
				break;
		}
	}

	if(eResult == AK_Success)
	{
		//Process Switching options
		AkUInt32 ulNumSwitchParams = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		m_listParameters.Reserve( ulNumSwitchParams );

		for( AkUInt32 i = 0; i < ulNumSwitchParams; ++i )
		{
			//Read the Actual Switch ID
			AkSwitchNodeParams l_params;

			AkUInt8 l_cTempByte;
			AkUInt32	 ulNodeID =			READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			l_cTempByte =					READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
			l_params.bIsFirstOnly =			l_cTempByte?true:false;
			l_cTempByte =					READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
			l_params.bContinuePlayback =	l_cTempByte?true:false;
			l_params.eOnSwitchMode	=		(AkOnSwitchMode) READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			l_params.FadeOutTime	=		READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );
			l_params.FadeInTime	=			READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );

			eResult = SetAllParams( ulNodeID, l_params );

			if( eResult != AK_Success )
			{// Something went wrong, quit and report error
				break;
			}
		}
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSwitchCntr::Init()
{
	AKRESULT eResult = CAkMultiPlayNode::Init();
	if( eResult == AK_Success )
	{
		eResult = m_SwitchList.Init( 0, AK_MAX_SWITCH_LIST_SIZE );
	}
	if( eResult == AK_Success )
	{
		eResult = m_listParameters.Init( 0, AK_MAX_SWITCH_LIST_SIZE ); 
	}
	if( eResult == AK_Success )
	{
		eResult = m_listSwitchContPlayback.Init( 0, AK_MAX_SWITCH_LIST_SIZE ); 
	}
	return eResult;
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::Term()
{
    CAkSwitchAware::UnsubscribeSwitches();

	ClearSwitches();

	m_listSwitchContPlayback.Term();
	m_listParameters.Term();
	m_SwitchList.Term();

	CAkMultiPlayNode::Term();
}


//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::OnPreRelease()
{
	// Ensure the clean-up of the switch container histories is done before we disconnect from
	// our parent and busses, so the activity count is propagated to them
	// See WG-19700
	CleanSwitchContPlayback(); // required to free the play histories

	CAkMultiPlayNode::OnPreRelease();
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::CleanSwitchContPlayback()
{
	if ( m_listSwitchContPlayback.IsInitialized() )
	{
		for( AkListSwitchContPlayback::Iterator iter = m_listSwitchContPlayback.Begin(); iter != m_listSwitchContPlayback.End(); ++iter )
		{
			NotifyEndContinuous( *iter );
		}
	}
}

//====================================================================================================
//====================================================================================================
CAkSwitchCntr* CAkSwitchCntr::Create( AkUniqueID in_ulID )
{
	CAkSwitchCntr* pAkSwitchCntr = AkNew( g_DefaultPoolId, CAkSwitchCntr( in_ulID ) );

	if( pAkSwitchCntr && pAkSwitchCntr->Init() != AK_Success )
	{
		pAkSwitchCntr->Release();
		pAkSwitchCntr = NULL;
	}
	
	return pAkSwitchCntr;
}

//====================================================================================================
//====================================================================================================
inline AkNodeCategory CAkSwitchCntr::NodeCategory()
{
	return AkNodeCategory_SwitchCntr;
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSwitchCntr::PlayInternal( AkPBIParams& in_rPBIParams )
{
	bool l_bIsContinuousPlay = in_rPBIParams.eType != AkPBIParams::PBI;
	//Suppose things went wrong
	AKRESULT	eResult = AK_Fail;

	// Get the switch to use
    AkSwitchStateID ulSwitchState = GetSwitchToUse( in_rPBIParams.pGameObj, m_ulGroupID, m_eGroupType );

	if( m_bIsContinuousValidation )
	{
		SwitchContPlaybackItem l_ContItem;
        l_ContItem.ePlaybackState = in_rPBIParams.ePlaybackState;
        l_ContItem.GameObject = in_rPBIParams.pGameObj;
        l_ContItem.UserParameters = in_rPBIParams.userParams;

        l_ContItem.PlayHist = in_rPBIParams.playHistory;

		if ( !IncrementActivityCount() )
		{
			DecrementActivityCount();
			return AK_Fail;
		}
		if ( !m_listSwitchContPlayback.AddLast( l_ContItem ) )
		{
			DecrementActivityCount();
			return AK_Fail;
			//These were the ONLY return statement allowed in this function, the NotificationReason_IncrementCount must absolutely send NotificationReason_DecrementCount
		}
		l_ContItem.GameObject->AddRef();

		//Sending an additional overflow notif to handle continuosity in the switch container in continuous mode
        MONITOR_OBJECTNOTIF( in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_rPBIParams.userParams.CustomParam(), AkMonitorData::NotificationReason_EnterSwitchContPlayback, in_rPBIParams.playHistory.HistArray, ID(), false, 0 );
		g_pPlayingMgr->AddItemActiveCount( in_rPBIParams.userParams.PlayingID() );
	}

	// We must use this backup since there may be multiple playback, and this field will be overriden if multiple children are played.
	SafeContinuationList safeContList( in_rPBIParams, this );

	// This variable will be useful to know when to send notifications for additionnal sounds
	AkUInt32 l_ulNumSoundsStarted = 0;
	AkUInt32 l_ulNumSoundLaunched = 0;
	
	CAkSwitchPackage* pSwPack = m_SwitchList.Exists( ulSwitchState );
	if( !pSwPack )
	{
		ulSwitchState = m_ulDefaultSwitch;
		pSwPack = m_SwitchList.Exists( ulSwitchState );
	}
	if( !pSwPack )
	{
		// No choice but to signal play failed here
		MONITOR_ERROREX( AK::Monitor::ErrorCode_NoValidSwitch, in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), ID(), false );

		if( !l_bIsContinuousPlay )
		{
			// Must not be an error if in step mode if there is nothing to play or if no switch group was assigned.
			// Simply nothing to play. (otherwise MultiPlay Nodes will not be consistently playing WG-20018).
			eResult = AK_Success;
		}
	}
	else
	{
        AkSwitchHistItem SwitchHistItem = g_pRegistryMgr->GetSwitchHistItem( in_rPBIParams.pGameObj, ID() );

		SwitchHistItem.IncrementPlayback( ulSwitchState );
		bool l_bIsFirstPlay = ( SwitchHistItem.NumPlayBack == 1 );
        g_pRegistryMgr->SetSwitchHistItem( in_rPBIParams.pGameObj, ID(), SwitchHistItem );

		AkUniqueID item_UniqueID;
		AkSwitchNodeParams l_Params;
		if( !l_bIsContinuousPlay && pSwPack->m_list.Length() == 0 )
		{
			// Must not be an error if in step mode there is nothing to play in the switch.
			eResult = AK_Success;
		}
		else
		{
			for( CAkSwitchPackage::AkIDList::Iterator iter = pSwPack->m_list.Begin(); iter != pSwPack->m_list.End(); ++iter )
			{
				AkPBIParams params = in_rPBIParams;

				item_UniqueID = *iter;

				GetAllParams( item_UniqueID, l_Params );

				if( ( l_bIsFirstPlay || !l_Params.bIsFirstOnly ) && ( params.sequenceID == AK_INVALID_SEQUENCE_ID || l_ulNumSoundsStarted == 0 ) )
				{
					CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( item_UniqueID, AkNodeType_Default );
					if( pNode )
					{
						if( !l_bIsContinuousPlay )
						{
							eResult = static_cast<CAkParameterNode*>(pNode)->Play( params );
						}
						else
						{
							AkContParamsAndPath continuousParams( params.pContinuousParams );

							// Cross fade on switches containers should be possible only when there is only one sound in the actual switch, 
							// otherwise, we do the normal behavior
							if( pSwPack->m_list.Length() == 1 )
							{
								ContGetList( params.pContinuousParams->spContList, continuousParams.Get().spContList );
								eResult = AK_Success;
							}
							else
							{
								continuousParams.Get().spContList.Attach( CAkContinuationList::Create() );

								if( continuousParams.Get().spContList )
									eResult = AddMultiplayItem( continuousParams, params, safeContList );
								else
									eResult = AK_InsufficientMemory;
							}

							if( eResult == AK_Success )
							{
								params.pContinuousParams = &continuousParams.Get();
								eResult = static_cast<CAkParameterNode*>(pNode)->Play( params );
							}
						}
						if( eResult == AK_Success )
						{
							++l_ulNumSoundsStarted;
						}
						++l_ulNumSoundLaunched;

						pNode->Release();
					}
					else
					{
						MONITOR_ERROR_PARAM( AK::Monitor::ErrorCode_SelectedChildNotAvailable, item_UniqueID, in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), ID(), false );
					}
				}
			}
		}

		if( l_ulNumSoundLaunched == 0 && !m_bIsContinuousValidation )
		{
            MONITOR_OBJECTNOTIF( in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_rPBIParams.userParams.CustomParam(), AkMonitorData::NotificationReason_NothingToPlay, in_rPBIParams.playHistory.HistArray, ID(), false, 0 );
		}
	}

    if( l_bIsContinuousPlay )
	{
		if( l_ulNumSoundsStarted == 0 && !m_bIsContinuousValidation )
		{
			eResult = PlayAndContinueAlternateMultiPlay( in_rPBIParams );
		}
		else
		{
			if( safeContList.Get() )
			{
				eResult = ContUnrefList( safeContList.Get() );
			}
		}
	}

	return eResult;//there is another return upper in this function
}

AKRESULT CAkSwitchCntr::ExecuteAction( ActionParams& in_rAction )
{
	switch( in_rAction.eType )
	{
	case ActionParamType_Stop:
	case ActionParamType_Break:
		StopContSwitchInst( in_rAction.pGameObj, in_rAction.playingID );
		break;
	case ActionParamType_Pause:
		PauseContSwitchInst( in_rAction.pGameObj, in_rAction.playingID );
		break;
	case ActionParamType_Resume:
		ResumeContSwitchInst( in_rAction.pGameObj, in_rAction.playingID );
		break;
	}
	return CAkActiveParent<CAkParameterNode>::ExecuteAction( in_rAction );
}

AKRESULT CAkSwitchCntr::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	switch( in_rAction.eType )
	{
	case ActionParamType_Stop:
		StopContSwitchInst( in_rAction.pGameObj );
		break;
	case ActionParamType_Pause:
		PauseContSwitchInst( in_rAction.pGameObj );
		break;
	case ActionParamType_Resume:
		ResumeContSwitchInst( in_rAction.pGameObj );
		break;
	}
	return CAkActiveParent<CAkParameterNode>::ExecuteActionExcept( in_rAction );
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::StopContSwitchInst( CAkRegisteredObj * in_pGameObj, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	bool bResetPlayBack = false;

	AkListSwitchContPlayback::IteratorEx iter = m_listSwitchContPlayback.BeginEx();
	while( iter != m_listSwitchContPlayback.End() )
	{
		SwitchContPlaybackItem& rItem = *iter;
		if( ( in_pGameObj == NULL || in_pGameObj == rItem.GameObject ) &&
			( in_PlayingID == AK_INVALID_PLAYING_ID || rItem.UserParameters.PlayingID() == in_PlayingID ) )
		{
			iter = m_listSwitchContPlayback.Erase( iter );
			NotifyEndContinuous( rItem );

			bResetPlayBack = true;
		}
		else
		{
			++iter;
		}
	}

	if ( bResetPlayBack )
		g_pRegistryMgr->ClearSwitchHist( ID(), in_pGameObj );
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::ResumeContSwitchInst( CAkRegisteredObj * in_pGameObj, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	for( AkListSwitchContPlayback::Iterator iter = m_listSwitchContPlayback.Begin(); iter != m_listSwitchContPlayback.End(); ++iter )
	{
		SwitchContPlaybackItem& l_rItem = *iter;
		if( ( in_pGameObj == NULL || in_pGameObj == l_rItem.GameObject ) &&
			( in_PlayingID == AK_INVALID_PLAYING_ID || l_rItem.UserParameters.PlayingID() == in_PlayingID ) )
		{
			if( l_rItem.ePlaybackState != PB_Playing)
			{
				l_rItem.ePlaybackState = PB_Playing;
				NotifyResumedContinuous( l_rItem );
			}
		}
	}
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::PauseContSwitchInst( CAkRegisteredObj * in_pGameObj, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */)
{
	for( AkListSwitchContPlayback::Iterator iter = m_listSwitchContPlayback.Begin(); iter != m_listSwitchContPlayback.End(); ++iter )
	{
		SwitchContPlaybackItem& l_rItem = *iter;
		if( ( in_pGameObj == NULL || in_pGameObj == l_rItem.GameObject ) &&
			( in_PlayingID == AK_INVALID_PLAYING_ID || l_rItem.UserParameters.PlayingID() == in_PlayingID ) )
		{
			if( l_rItem.ePlaybackState != PB_Paused )
			{
				l_rItem.ePlaybackState = PB_Paused;
				NotifyPausedContinuous( l_rItem );
			}
		}
	}
}

void CAkSwitchCntr::NotifyEndContinuous( SwitchContPlaybackItem& in_rSwitchContPlayback )
{
	//Sending an additional overflow notif to handle continuosity in the switch container in continuous mode
	if( in_rSwitchContPlayback.ePlaybackState == PB_Paused )
	{
		NotifyPausedContinuousAborted( in_rSwitchContPlayback );
	}
	MONITOR_OBJECTNOTIF( in_rSwitchContPlayback.UserParameters.PlayingID(), in_rSwitchContPlayback.GameObject->ID(), in_rSwitchContPlayback.UserParameters.CustomParam(), AkMonitorData::NotificationReason_ExitSwitchContPlayback, in_rSwitchContPlayback.PlayHist.HistArray, ID(), false, 0 );
	g_pPlayingMgr->RemoveItemActiveCount( in_rSwitchContPlayback.UserParameters.PlayingID() );
	DecrementActivityCount();
	in_rSwitchContPlayback.GameObject->Release();
}

void CAkSwitchCntr::NotifyPausedContinuous( SwitchContPlaybackItem& in_rSwitchContPlayback )
{
	//Sending an additional overflow notif to handle continuosity in the switch container in continuous mode
	MONITOR_OBJECTNOTIF( in_rSwitchContPlayback.UserParameters.PlayingID(), in_rSwitchContPlayback.GameObject->ID(), in_rSwitchContPlayback.UserParameters.CustomParam(), AkMonitorData::NotificationReason_PauseSwitchContPlayback, in_rSwitchContPlayback.PlayHist.HistArray, ID(), false, 0 );
}

void CAkSwitchCntr::NotifyPausedContinuousAborted( SwitchContPlaybackItem& in_rSwitchContPlayback )
{
	//Sending an additional overflow notif to handle continuosity in the switch container in continuous mode
	MONITOR_OBJECTNOTIF( in_rSwitchContPlayback.UserParameters.PlayingID(), in_rSwitchContPlayback.GameObject->ID(), in_rSwitchContPlayback.UserParameters.CustomParam(), AkMonitorData::NotificationReason_PauseSwitchContPlayback_Aborted, in_rSwitchContPlayback.PlayHist.HistArray, ID(), false, 0 );
}

void CAkSwitchCntr::NotifyResumedContinuous( SwitchContPlaybackItem& in_rSwitchContPlayback )
{
	//Sending an additional overflow notif to handle continuosity in the switch container in continuous mode
	MONITOR_OBJECTNOTIF( in_rSwitchContPlayback.UserParameters.PlayingID(), in_rSwitchContPlayback.GameObject->ID(), in_rSwitchContPlayback.UserParameters.CustomParam(), AkMonitorData::NotificationReason_ResumeSwitchContPlayback, in_rSwitchContPlayback.PlayHist.HistArray, ID(), false, 0 );
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::RemoveChild( CAkParameterNodeBase* in_pChild )
{
	AKASSERT( in_pChild );

	bool bToBeReleased = false;
	AkUniqueID childID = in_pChild->ID();
	if( in_pChild->Parent() == this )
    {
		in_pChild->Parent( NULL );
        m_mapChildId.Unset( childID );
		bToBeReleased = true;
    }

	m_listParameters.Unset( childID );

	if( bToBeReleased )
	{
		// Must be last call of this finction, this is why the bool "bToBeReleased" is used...
		this->Release();
	}
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::SetSwitch( AkUInt32 in_Switch, CAkRegisteredObj * in_GameObj /* = NULL*/ )
{
	if( m_bIsContinuousValidation )
	{
		PerformSwitchChange( in_Switch, in_GameObj );
	}
}

AKRESULT CAkSwitchCntr::PrepareNodeList( const CAkSwitchPackage::AkIDList& in_rNodeList )
{
	AKRESULT eResult = AK_Success;
	for( CAkSwitchPackage::AkIDList::Iterator iterNode = in_rNodeList.Begin(); iterNode != in_rNodeList.End(); ++iterNode )
	{
		eResult = PrepareNodeData( *iterNode.pItem );

		if( eResult != AK_Success )
		{
			// Do not let this half-prepared, unprepare what has been prepared up to now.
			for( CAkSwitchPackage::AkIDList::Iterator iterNodeFlush = in_rNodeList.Begin(); iterNodeFlush != iterNode; ++iterNodeFlush )
			{
				UnPrepareNodeData( *iterNodeFlush.pItem );
			}
		}
	}
	return eResult;
}

void CAkSwitchCntr::UnPrepareNodeList( const CAkSwitchPackage::AkIDList& in_rNodeList )
{
	for( CAkSwitchPackage::AkIDList::Iterator iterNode = in_rNodeList.Begin(); iterNode != in_rNodeList.End(); ++iterNode )
	{
		UnPrepareNodeData( *iterNode.pItem );
	}
}

AKRESULT CAkSwitchCntr::ModifyActiveState( AkUInt32 in_stateID, bool in_bSupported )
{
	AKRESULT eResult = AK_Success;

	if( m_uPreparationCount != 0 )
	{
		// seek in the switch list, to get the right  node list.

		AkSwitchList::Iterator iter = m_SwitchList.FindEx( in_stateID );
		if( iter != m_SwitchList.End() )
		{
			// We now have the node list in the iter, simply prepare it if in_bSupported or unprepare it if !in_bSupported
			CAkSwitchPackage::AkIDList& rNodeIDList = iter.pItem->Item.item.m_list;
			
			if( in_bSupported )
			{
				eResult = PrepareNodeList( rNodeIDList );
			}
			else
			{
				UnPrepareNodeList( rNodeIDList );
			}
		}
		//else
		//{
			//not finding a switch is a success.
		//}
	}

	return eResult;
}

AKRESULT CAkSwitchCntr::PrepareData()
{
	extern AkInitSettings g_settings;
	if( !g_settings.bEnableGameSyncPreparation )
	{
		return CAkActiveParent<CAkParameterNode>::PrepareData();
	}

	AKRESULT eResult = AK_Success;

	if( m_uPreparationCount == 0 )
	{
		CAkPreparedContent* pPreparedContent = GetPreparedContent( m_ulGroupID, m_eGroupType );
		if( pPreparedContent )
		{
			for( AkSwitchList::Iterator iter = m_SwitchList.Begin(); iter != m_SwitchList.End(); ++iter )
			{
				if( pPreparedContent->IsIncluded( iter.pItem->Item.key ) )
				{
					eResult = PrepareNodeList( iter.pItem->Item.item.m_list );
				}
				if( eResult != AK_Success )
				{
					// Do not let this half-prepared, unprepare what has been prepared up to now.
					for( AkSwitchList::Iterator iterFlush = m_SwitchList.Begin(); iterFlush != iter; ++iterFlush )
					{
						if( pPreparedContent->IsIncluded( iterFlush.pItem->Item.key ) )
						{
							UnPrepareNodeList( iterFlush.pItem->Item.item.m_list );
						}
					}
				}
			}
			if( eResult == AK_Success )
			{
				++m_uPreparationCount;
				eResult = SubscribePrepare( m_ulGroupID, m_eGroupType );
				if( eResult != AK_Success )
					UnPrepareData();
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}
	else
	{
		++m_uPreparationCount;
	}
	return eResult;
}

void CAkSwitchCntr::UnPrepareData()
{
	extern AkInitSettings g_settings;
	if( !g_settings.bEnableGameSyncPreparation )
	{
		return CAkActiveParent<CAkParameterNode>::UnPrepareData();
	}

	if( m_uPreparationCount != 0 ) // must check in case there were more unprepare than prepare called that succeeded.
	{
		if( --m_uPreparationCount == 0 )
		{
			CAkPreparedContent* pPreparedContent = GetPreparedContent( m_ulGroupID, m_eGroupType );
			if( pPreparedContent )
			{
				for( AkSwitchList::Iterator iter = m_SwitchList.Begin(); iter != m_SwitchList.End(); ++iter )
				{
					if( pPreparedContent->IsIncluded( iter.pItem->Item.key ) )
					{
						UnPrepareNodeList( iter.pItem->Item.item.m_list );
					}
				}
			}
			CAkPreparationAware::UnsubscribePrepare( m_ulGroupID, m_eGroupType );
		}
	}
}

AKRESULT CAkSwitchCntr::SetSwitchGroup( 
    AkUInt32 in_ulGroup, 
    AkGroupType in_eGroupType 
    )
{
    AKRESULT eResult = AK_Success;
    // Change it only if required
    if( in_ulGroup != m_ulGroupID || in_eGroupType != m_eGroupType)
	{
		m_ulGroupID = in_ulGroup;
		m_eGroupType = in_eGroupType;

		eResult = SubscribeSwitch( in_ulGroup, in_eGroupType );
	}
    return eResult;
}

//====================================================================================================
//====================================================================================================
bool CAkSwitchCntr::IsAContinuousSwitch( CAkSwitchPackage* in_pSwitchPack, AkUniqueID in_ID )
{
	bool l_bIsContnuousSwitch = false;

	if( in_pSwitchPack )
	{
		for( CAkSwitchPackage::AkIDList::Iterator iter = in_pSwitchPack->m_list.Begin(); iter != in_pSwitchPack->m_list.End(); ++iter )
		{
			if( (*iter) == in_ID )
			{
				l_bIsContnuousSwitch = true;
				break;
			}
		}
	}

	return l_bIsContnuousSwitch;
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSwitchCntr::PerformSwitchChange( AkSwitchStateID in_SwitchTo, CAkRegisteredObj * in_GameObj /*= NULL*/ )
{
	AKRESULT	eResult = AK_Success;

	AKASSERT( g_pIndex );

	AKASSERT( m_bIsContinuousValidation );
	if( in_GameObj == NULL )
	{
		AkUInt32 l_ulListSize = m_listSwitchContPlayback.Length();
		if ( l_ulListSize )
		{
			CAkRegisteredObj** pArrayGameObj;
            pArrayGameObj = (CAkRegisteredObj**)AkAlloc( g_DefaultPoolId, AkUInt32(l_ulListSize*sizeof(CAkRegisteredObj*)));
			if(pArrayGameObj)
			{
				AkUInt32 i = 0;
				for( AkListSwitchContPlayback::Iterator iter = m_listSwitchContPlayback.Begin(); iter != m_listSwitchContPlayback.End(); ++iter )
				{
					pArrayGameObj[i++] = (*iter).GameObject;
				}

				for( i = 0; i < l_ulListSize; ++i)
				{
					PerformSwitchChangeContPerObject( in_SwitchTo, pArrayGameObj[i] );
				}
				AkFree( g_DefaultPoolId, pArrayGameObj );
			}
			else
			{
				eResult = AK_Fail;
			}
		}
		else
		{
			g_pRegistryMgr->ClearSwitchHist( ID() );
		}
	}
	else
	{
		PerformSwitchChangeContPerObject( in_SwitchTo, in_GameObj );
	}
	return eResult;
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSwitchCntr::PerformSwitchChangeContPerObject( AkSwitchStateID in_SwitchTo, CAkRegisteredObj * in_GameObj )
{
	AKRESULT eResult = AK_Success;
	AKASSERT( in_GameObj != NULL );

	// Perform switch only for a given Game obj = in_GameObj
	AkSwitchHistItem SwitchHistItem = g_pRegistryMgr->GetSwitchHistItem( in_GameObj, ID() );
	if( in_SwitchTo != SwitchHistItem.LastSwitch )
	{
		CAkSwitchPackage* pPreviousSwitchPack = m_SwitchList.Exists( SwitchHistItem.LastSwitch );
		CAkSwitchPackage* pNextSwitchPack = m_SwitchList.Exists( in_SwitchTo );
		if( !pNextSwitchPack )
		{
			in_SwitchTo = m_ulDefaultSwitch;
			pNextSwitchPack = m_SwitchList.Exists( in_SwitchTo );
		}
		if( !pNextSwitchPack )
		{
			MONITOR_ERROREX( AK::Monitor::ErrorCode_NoValidSwitch, AK_INVALID_PLAYING_ID, in_GameObj->ID(), ID(), false );
		}
		// If previous exists, stop them if required
		eResult = StopPrevious( pPreviousSwitchPack, pNextSwitchPack, in_GameObj );

		g_pRegistryMgr->ClearSwitchHist( ID(), in_GameObj );

		SwitchHistItem.NumPlayBack = 0;
		SwitchHistItem.LastSwitch = AK_INVALID_UNIQUE_ID;

		for( AkListSwitchContPlayback::Iterator iterSCP = m_listSwitchContPlayback.Begin(); iterSCP != m_listSwitchContPlayback.End(); ++iterSCP )
		{
			SwitchContPlaybackItem& rContItem = *iterSCP;

			if( rContItem.GameObject == in_GameObj )
			{
                SwitchHistItem.IncrementPlayback( in_SwitchTo );

				if(pNextSwitchPack)
				{
					for( CAkSwitchPackage::AkIDList::Iterator iter = pNextSwitchPack->m_list.Begin(); iter != pNextSwitchPack->m_list.End(); ++iter )
					{
						AkUniqueID item_UniqueID = *iter;

						AkSwitchNodeParams l_Params;

						GetAllParams( item_UniqueID, l_Params );
						
						if( !(l_Params.bContinuePlayback)
							|| !IsAContinuousSwitch( pPreviousSwitchPack, item_UniqueID ) )
						{
							eResult = PlayOnSwitch( item_UniqueID, rContItem );
						}
					}
				}
			}
		}

		g_pRegistryMgr->SetSwitchHistItem( in_GameObj, ID(), SwitchHistItem );
	}

	return eResult;
}

//====================================================================================================
//====================================================================================================
// helper only to help function comprehension
AKRESULT CAkSwitchCntr::PlayOnSwitch( AkUniqueID in_ID, SwitchContPlaybackItem& in_rContItem )
{
	AKRESULT eResult = AK_Success;
	AKASSERT( g_pIndex );

	CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( in_ID, AkNodeType_Default );
	if( pNode )
	{
		TransParams l_TParams;
		l_TParams.eFadeCurve = AkCurveInterpolation_Linear;
		l_TParams.TransitionTime = GetFadeInTime( in_ID );

		// Making a flexible copy of the play history, we cannot allow the base one to be modified
        AkPBIParams pbiParams( in_rContItem.PlayHist );

        pbiParams.eType = AkPBIParams::PBI;
        pbiParams.pInstigator = pNode;
        pbiParams.bIsFirst = true;

        pbiParams.pGameObj = in_rContItem.GameObject;
        pbiParams.pTransitionParameters = &l_TParams;
        pbiParams.userParams = in_rContItem.UserParameters;
        pbiParams.ePlaybackState = in_rContItem.ePlaybackState;
        pbiParams.uFrameOffset = 0;

        pbiParams.pContinuousParams = NULL;
        pbiParams.sequenceID = AK_INVALID_SEQUENCE_ID;


		eResult = static_cast<CAkParameterNode*>(pNode)->Play( pbiParams );
		pNode->Release();
	}
	else
	{
		eResult = AK_Fail;
	}

	return eResult;
}

//====================================================================================================
//====================================================================================================
// helper only to help function comprehension
AKRESULT CAkSwitchCntr::StopOnSwitch( AkUniqueID in_ID, AkSwitchNodeParams& in_rSwitchNodeParams, CAkRegisteredObj * in_GameObj )
{
	AKRESULT eResult = AK_Success;
	AKASSERT( g_pIndex );

	CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( in_ID, AkNodeType_Default );
	if( pNode )
	{
		AKASSERT( g_pAudioMgr );
		AKASSERT( in_GameObj != NULL );
		g_pAudioMgr->StopPendingAction( pNode, in_GameObj, AK_INVALID_PLAYING_ID);

		if( in_rSwitchNodeParams.eOnSwitchMode == AkOnSwitchMode_Stop )
		{
			ActionParams l_Params;
			l_Params.bIsFromBus = false;
			l_Params.bIsMasterResume = false;
			l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
			l_Params.eType = ActionParamType_Stop;
			l_Params.pGameObj = in_GameObj;
			l_Params.playingID = AK_INVALID_PLAYING_ID;
			l_Params.transParams.TransitionTime = in_rSwitchNodeParams.FadeOutTime;
			l_Params.bIsMasterCall = false;
			eResult = pNode->ExecuteAction( l_Params );
		}
		else
		{
			pNode->PlayToEnd( in_GameObj, this );
		}

		pNode->Release();
	}

	return eResult;
}

AKRESULT CAkSwitchCntr::StopPrevious( CAkSwitchPackage* in_pPreviousSwitchPack, CAkSwitchPackage* in_pNextSwitchPack, CAkRegisteredObj * in_GameObj )
{
	AKRESULT eResult = AK_Success;

	if(in_pPreviousSwitchPack)
	{
		for( CAkSwitchPackage::AkIDList::Iterator iter = in_pPreviousSwitchPack->m_list.Begin(); iter != in_pPreviousSwitchPack->m_list.End(); ++iter )
		{
			AkUniqueID item_UniqueID = *iter;

			AkSwitchNodeParams l_Params;
			GetAllParams( item_UniqueID, l_Params );
			
			if( !m_bIsContinuousValidation
				|| !(l_Params.bContinuePlayback)
				|| !IsAContinuousSwitch( in_pNextSwitchPack, item_UniqueID ) )
			{
				eResult = StopOnSwitch( item_UniqueID, l_Params, in_GameObj );
				if( eResult != AK_Success )
				{
					break;
				}
			}
		}
	}

	return eResult;
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::ClearSwitches()
{
	if ( m_SwitchList.IsInitialized() )
	{
		for( AkSwitchList::Iterator iter = m_SwitchList.Begin(); iter != m_SwitchList.End(); ++iter )
		{
			(*iter).item.Term();
		}
		m_SwitchList.RemoveAll();
	}
}

//====================================================================================================
//====================================================================================================
CAkSwitchPackage * CAkSwitchCntr::AddSwitch( AkSwitchStateID in_Switch )
{
	CAkSwitchPackage* pPack = m_SwitchList.Set( in_Switch );
	return pPack;
}

//====================================================================================================
//====================================================================================================
void CAkSwitchCntr::RemoveSwitch( AkSwitchStateID in_Switch )
{
	CAkSwitchPackage* pPack = m_SwitchList.Exists( in_Switch );
	if( pPack != NULL )
	{
		pPack->Term();
		m_SwitchList.Unset( in_Switch );
	}
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSwitchCntr::AddNodeInSwitch( 
	AkUInt32			in_Switch,
	AkUniqueID		in_NodeID
	)
{
	AKASSERT( in_NodeID != AK_INVALID_UNIQUE_ID );

	if( in_NodeID == AK_INVALID_UNIQUE_ID)
	{
		return AK_InvalidParameter;
	}

	AKRESULT eResult = AK_InvalidSwitchType;

	CAkSwitchPackage* pPack = m_SwitchList.Exists( in_Switch );
	if( pPack != NULL )
		eResult = AddNodeInSwitch( pPack, in_NodeID );

	return eResult;
}

AKRESULT CAkSwitchCntr::AddNodeInSwitch(
	CAkSwitchPackage * in_pPack,
	AkUniqueID		in_NodeID
	)
{
	AKRESULT eResult = AK_InvalidSwitchType;

	AkUniqueID* pID = in_pPack->m_list.Exists( in_NodeID );
	if( !pID )
	{
		if ( in_pPack->m_list.AddLast( in_NodeID ) )
			eResult = AK_Success;
		else
			eResult = AK_Fail;
	}

	return eResult;
}
	
//====================================================================================================
//====================================================================================================
AKRESULT CAkSwitchCntr::RemoveNodeFromSwitch( 
	AkUInt32			in_Switch,
	AkUniqueID		in_NodeID
	)
{
	AKASSERT( in_NodeID != AK_INVALID_UNIQUE_ID );

	if( in_NodeID == AK_INVALID_UNIQUE_ID)
	{
		return AK_InvalidParameter;
	}

	CAkSwitchPackage* pPack = m_SwitchList.Exists( in_Switch );
	if( pPack != NULL )
	{
		AkUniqueID* pID = pPack->m_list.Exists( in_NodeID );
		if( pID )
		{
			pPack->m_list.Remove( in_NodeID );
		}
	}

	return AK_Success;
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSwitchCntr::SetContinuousValidation( bool in_bIsContinuousCheck )
{
#ifndef AK_OPTIMIZED
	// Handle the special case where while wwise is editing it changes from step to continuous, 
	// so it must not remind past sequences of playback
	if( in_bIsContinuousCheck && !m_bIsContinuousValidation )
	{
		g_pRegistryMgr->ClearSwitchHist( ID() );
	}
#endif //AK_OPTIMIZED

	m_bIsContinuousValidation = in_bIsContinuousCheck;
	return AK_Success;
}

//////////////////////////////////////////////////////////////////
// Define default values... used if none is available
//////////////////////////////////////////////////////////////////
#define AK_SWITCH_DEFAULT_ContinuePlayback (false)
#define AK_SWITCH_DEFAULT_IsFirstOnly (false)
#define AK_SWITCH_DEFAULT_OnSwitchMode (AkOnSwitchMode_PlayToEnd)
#define AK_SWITCH_DEFAULT_FadeInTime (0)
#define AK_SWITCH_DEFAULT_FadeOutTime (0)

//////////////////////////////////////////////////////////////////
//SET
//////////////////////////////////////////////////////////////////

AKRESULT CAkSwitchCntr::SetContinuePlayback( AkUniqueID in_NodeID, bool in_bContinuePlayback )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		pParams->bContinuePlayback = in_bContinuePlayback;
	}
	else
	{
		AkSwitchNodeParams l_SwitchNodeParams;
		l_SwitchNodeParams.bContinuePlayback = in_bContinuePlayback;
		l_SwitchNodeParams.bIsFirstOnly = AK_SWITCH_DEFAULT_IsFirstOnly;
		l_SwitchNodeParams.eOnSwitchMode = AK_SWITCH_DEFAULT_OnSwitchMode;
		l_SwitchNodeParams.FadeInTime = AK_SWITCH_DEFAULT_FadeInTime;
		l_SwitchNodeParams.FadeOutTime = AK_SWITCH_DEFAULT_FadeOutTime;

		if ( !m_listParameters.Set( in_NodeID, l_SwitchNodeParams ) )
			return AK_Fail;
	}

	return AK_Success;
}

AKRESULT CAkSwitchCntr::SetFadeInTime( AkUniqueID in_NodeID, AkTimeMs in_time )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		pParams->FadeInTime = in_time;
	}
	else
	{
		AkSwitchNodeParams l_SwitchNodeParams;
		l_SwitchNodeParams.bContinuePlayback = AK_SWITCH_DEFAULT_ContinuePlayback;
		l_SwitchNodeParams.bIsFirstOnly = AK_SWITCH_DEFAULT_IsFirstOnly;
		l_SwitchNodeParams.eOnSwitchMode = AK_SWITCH_DEFAULT_OnSwitchMode;
		l_SwitchNodeParams.FadeInTime = in_time;
		l_SwitchNodeParams.FadeOutTime = AK_SWITCH_DEFAULT_FadeOutTime;

		if ( !m_listParameters.Set( in_NodeID, l_SwitchNodeParams ) )
			return AK_Fail;
	}

	return AK_Success;
}

AKRESULT CAkSwitchCntr::SetFadeOutTime( AkUniqueID in_NodeID, AkTimeMs in_time )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		pParams->FadeOutTime = in_time;
	}
	else
	{
		AkSwitchNodeParams l_SwitchNodeParams;
		l_SwitchNodeParams.bContinuePlayback = AK_SWITCH_DEFAULT_ContinuePlayback;
		l_SwitchNodeParams.bIsFirstOnly = AK_SWITCH_DEFAULT_IsFirstOnly;
		l_SwitchNodeParams.eOnSwitchMode = AK_SWITCH_DEFAULT_OnSwitchMode;
		l_SwitchNodeParams.FadeInTime = AK_SWITCH_DEFAULT_FadeInTime;
		l_SwitchNodeParams.FadeOutTime = in_time;

		if ( !m_listParameters.Set( in_NodeID, l_SwitchNodeParams ) )
			return AK_Fail;
	}

	return AK_Success;
}

AKRESULT CAkSwitchCntr::SetOnSwitchMode( AkUniqueID in_NodeID, AkOnSwitchMode in_eSwitchMode )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		pParams->eOnSwitchMode = in_eSwitchMode;
	}
	else
	{
		AkSwitchNodeParams l_SwitchNodeParams;
		l_SwitchNodeParams.bContinuePlayback = AK_SWITCH_DEFAULT_ContinuePlayback;
		l_SwitchNodeParams.bIsFirstOnly = AK_SWITCH_DEFAULT_IsFirstOnly;
		l_SwitchNodeParams.eOnSwitchMode = in_eSwitchMode;
		l_SwitchNodeParams.FadeInTime = AK_SWITCH_DEFAULT_FadeInTime;
		l_SwitchNodeParams.FadeOutTime = AK_SWITCH_DEFAULT_FadeOutTime;

		if ( !m_listParameters.Set( in_NodeID, l_SwitchNodeParams ) )
			return AK_Fail;
	}

	return AK_Success;
}

AKRESULT CAkSwitchCntr::SetIsFirstOnly( AkUniqueID in_NodeID, bool in_bIsFirstOnly )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		pParams->bIsFirstOnly = in_bIsFirstOnly;
	}
	else
	{
		AkSwitchNodeParams l_SwitchNodeParams;
		l_SwitchNodeParams.bContinuePlayback = AK_SWITCH_DEFAULT_ContinuePlayback;
		l_SwitchNodeParams.bIsFirstOnly = in_bIsFirstOnly;
		l_SwitchNodeParams.eOnSwitchMode = AK_SWITCH_DEFAULT_OnSwitchMode;
		l_SwitchNodeParams.FadeInTime = AK_SWITCH_DEFAULT_FadeInTime;
		l_SwitchNodeParams.FadeOutTime = AK_SWITCH_DEFAULT_FadeOutTime;

		if ( !m_listParameters.Set( in_NodeID, l_SwitchNodeParams ) )
			return AK_Fail;
	}

	return AK_Success;
}

AKRESULT CAkSwitchCntr::SetAllParams( AkUniqueID in_NodeID, AkSwitchNodeParams& in_rParams )
{
	AkSwitchNodeParams* pParams = m_listParameters.Set( in_NodeID, in_rParams );
	return pParams ? AK_Success : AK_Fail;
}

//////////////////////////////////////////////////////////////////
//Get
//////////////////////////////////////////////////////////////////
bool CAkSwitchCntr::GetContinuePlayback( AkUniqueID in_NodeID )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		return pParams->bContinuePlayback;
	}
	else
	{
		return AK_SWITCH_DEFAULT_ContinuePlayback;
	}
}

AkTimeMs CAkSwitchCntr::GetFadeInTime( AkUniqueID in_NodeID )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		return pParams->FadeInTime;
	}
	else
	{
		return AK_SWITCH_DEFAULT_FadeInTime;
	}
}

AkTimeMs CAkSwitchCntr::GetFadeOutTime( AkUniqueID in_NodeID )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		return pParams->FadeOutTime;
	}
	else
	{
		return AK_SWITCH_DEFAULT_FadeOutTime;
	}
}

AkOnSwitchMode CAkSwitchCntr::GetOnSwitchMode( AkUniqueID in_NodeID )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		return (AkOnSwitchMode)(pParams->eOnSwitchMode);
	}
	else
	{
		return AK_SWITCH_DEFAULT_OnSwitchMode;
	}
}

bool CAkSwitchCntr::GetIsFirstOnly( AkUniqueID in_NodeID )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		return pParams->bIsFirstOnly;
	}
	else
	{
		return AK_SWITCH_DEFAULT_IsFirstOnly;
	}
}

void CAkSwitchCntr::GetAllParams( AkUniqueID in_NodeID, AkSwitchNodeParams& out_rParams )
{
	AkSwitchNodeParams* pParams = m_listParameters.Exists( in_NodeID );
	if( pParams )
	{
		out_rParams = *pParams;
	}
	else
	{
		out_rParams.bContinuePlayback = AK_SWITCH_DEFAULT_ContinuePlayback;
		out_rParams.bIsFirstOnly = AK_SWITCH_DEFAULT_IsFirstOnly;
		out_rParams.eOnSwitchMode = AK_SWITCH_DEFAULT_OnSwitchMode;
		out_rParams.FadeInTime = AK_SWITCH_DEFAULT_FadeInTime;
		out_rParams.FadeOutTime = AK_SWITCH_DEFAULT_FadeOutTime;
	}
}

// AkMultiPlayNode interface implementation:
bool CAkSwitchCntr::IsContinuousPlayback()
{
	return m_bIsContinuousValidation;
}


