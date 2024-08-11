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
// AkActionPlayAndContinue.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionPlayAndContinue.h"
#include "AkPBI.h"
#include "AkContinuousPBI.h"
#include "AkAudioLibIndex.h"
#include "AkContainerBase.h"
#include "AkRanSeqCntr.h"
#include "AkContinuationList.h"
#include "PrivateStructures.h"
#include "AkMonitor.h"
#include "AkCntrHistory.h"
#include "AkTransitionManager.h"
#include "AkAudioMgr.h"
#include "AkPathManager.h"
#include "AkLEngineCmds.h"
#include "AkMultiPlayNode.h"

CAkActionPlayAndContinue::CAkActionPlayAndContinue(AkActionType in_eActionType, AkUniqueID in_ulID, CAkSmartPtr<CAkContinuationList>& in_spContList ) 
: CAkActionPlay( in_eActionType, in_ulID )
,m_spContList( in_spContList )
,m_pPreviousPBI( NULL )
,m_FadeOutTimeForLast( 0 )
,m_InitialPlaybackState( PB_Playing )
,m_SA_PBIID( AK_INVALID_SEQUENCE_ID )
,m_bNeedNotifyDelay( true )
,m_bIsfirstPlay( false )
,m_bSkipDelay( false )
,m_ePBIType( AkPBIParams::ContinuousPBI )
,m_pTransitionOwner( NULL )
,m_pInstigator( NULL )
,m_ulPauseCount( 0 )
,m_pPathOwner( NULL )
{
	m_PlayHistory.HistArray.Init();
	m_PathInfo.pPBPath = NULL;
	m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
}

CAkActionPlayAndContinue::~CAkActionPlayAndContinue()
{
	UnsetPlayStopTransition();
	UnsetPauseResumeTransition();

	if(m_PathInfo.pPBPath)
	{
		g_pPathManager->RemovePotentialUser(m_PathInfo.pPBPath);
	}
	
	if( m_pPathOwner )
	{
		m_pPathOwner->Release();
		m_pPathOwner = NULL;
	}

	if ( m_pInstigator )
		m_pInstigator->Release();
}

CAkActionPlayAndContinue* CAkActionPlayAndContinue::Create( AkActionType in_eActionType, AkUniqueID in_ulID, CAkSmartPtr<CAkContinuationList>& in_spContList )
{
	CAkActionPlayAndContinue*	pActionPAC = AkNew( g_DefaultPoolId, CAkActionPlayAndContinue( in_eActionType, in_ulID, in_spContList ) );
	if( pActionPAC )
	{
		if( pActionPAC->Init() != AK_Success )
		{
			pActionPAC->Release();
			pActionPAC = NULL;
		}
	}

	return pActionPAC;
}

AKRESULT CAkActionPlayAndContinue::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pIndex);
    AKASSERT( m_pInstigator );

    TransParams transParams;
    transParams.TransitionTime = m_FadeOutTimeForLast;
	transParams.eFadeCurve = AkCurveInterpolation_Linear;

	if(m_pPreviousPBI)
	{
		if ( m_pPreviousPBI->GetTransitionMode() == Transition_CrossFadePower )
			transParams.eFadeCurve = AkCurveInterpolation_Sine;
		((CAkPBI *)m_pPreviousPBI)->_Stop( transParams, true );
		// Even if the fade out fails, we still go on next, should not happen anyway
		// Nofity error here if required telling that the fade out didn't succeeded properly
	}
	
	//Lets suppose things went wrong
	AKRESULT eResult = AK_IDNotFound;

	CAkParameterNodeBase* pNode = GetAndRefTarget();

	if( pNode )
	{
		ContParams	Cparameters;

		Cparameters.pPlayStopTransition = m_PBTrans.pvPSTrans;
		Cparameters.pPauseResumeTransition = m_PBTrans.pvPRTrans;
		Cparameters.pPathInfo = &m_PathInfo;
		Cparameters.ulPauseCount = m_ulPauseCount;

		Cparameters.bIsPlayStopTransitionFading = m_PBTrans.bIsPSTransFading;
		Cparameters.bIsPauseResumeTransitionFading = m_PBTrans.bIsPRTransFading;
		
		Cparameters.spContList = m_spContList;

        AkPBIParams pbiParams( m_PlayHistory );

		pbiParams.bSkipDelay = m_bSkipDelay;
        pbiParams.eType = m_ePBIType;
        pbiParams.pInstigator = m_pInstigator;

        pbiParams.pGameObj = in_pAction->GameObj();
        pbiParams.userParams = in_pAction->UserParam;

        pbiParams.ePlaybackState = m_InitialPlaybackState;
        pbiParams.uFrameOffset = in_pAction->LaunchFrameOffset;
        pbiParams.bIsFirst = m_bIsfirstPlay;

        pbiParams.sequenceID = m_SA_PBIID;

        pbiParams.pContinuousParams = &Cparameters;
        pbiParams.pTransitionParameters = &transParams;

		eResult = static_cast<CAkParameterNode*>(pNode)->Play( pbiParams );

		CAkLEngineCmds::IncrementSyncCount();//We must increment it there, to ensure it will be processed.

		m_spContList = NULL;

		AKASSERT(g_pPathManager);
		if(m_PathInfo.pPBPath != NULL)
		{
			g_pPathManager->RemovePotentialUser(m_PathInfo.pPBPath);
			m_PathInfo.pPBPath = NULL;
			m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
		}
		
		if( m_pPathOwner )
		{
			m_pPathOwner->Release();
			m_pPathOwner = NULL;
		}

		pNode->Release();
	}
	else
	{
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF(in_pAction->UserParam.PlayingID(), in_pAction->GameObjID(), in_pAction->UserParam.CustomParam(), AkMonitorData::NotificationReason_PlayFailed, HistArray, m_ulElementID, false, 0 );
		MONITOR_ERROREX( AK::Monitor::ErrorCode_SelectedChildNotAvailable, in_pAction->UserParam.PlayingID(), in_pAction->GameObjID(), m_ulElementID, false );
	}
	return eResult;
}

AKRESULT CAkActionPlayAndContinue::SetPlayStopTransition( CAkTransition* in_pTransition, bool in_bTransitionFading, AkPendingAction* in_pTransitionOwner )
{
	m_pTransitionOwner = in_pTransitionOwner;
	UnsetPlayStopTransition();
	if( in_pTransition )
	{
		AKASSERT( g_pTransitionManager );
		AKRESULT eResult = g_pTransitionManager->AddTransitionUser( in_pTransition, m_pTransitionOwner );
		if( eResult != AK_Success )
		{
			return eResult;
		}
	}
	m_PBTrans.pvPSTrans = in_pTransition;
	m_PBTrans.bIsPSTransFading = in_bTransitionFading;

	return AK_Success;
}

AKRESULT CAkActionPlayAndContinue::SetPauseResumeTransition( CAkTransition* in_pTransition, bool in_bTransitionFading, AkPendingAction* in_pTransitionOwner )
{
	m_pTransitionOwner = in_pTransitionOwner;
	UnsetPauseResumeTransition();
	if( in_pTransition )
	{
		AKASSERT( g_pTransitionManager );
		AKRESULT eResult = g_pTransitionManager->AddTransitionUser( in_pTransition, m_pTransitionOwner );
		if( eResult != AK_Success )
		{
			return eResult;
		}
	}
	m_PBTrans.pvPRTrans = in_pTransition;
	m_PBTrans.bIsPRTransFading = in_bTransitionFading;

	return AK_Success;
}

void CAkActionPlayAndContinue::UnsetPlayStopTransition()
{
	if( m_PBTrans.pvPSTrans && m_pTransitionOwner )
	{
		g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPSTrans, m_pTransitionOwner );
	}
	 m_PBTrans.pvPSTrans = NULL;
	 m_PBTrans.bIsPSTransFading = false;
}

void CAkActionPlayAndContinue::UnsetPauseResumeTransition()
{
	if( m_PBTrans.pvPRTrans && m_pTransitionOwner )
	{
		g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPRTrans, m_pTransitionOwner );
	}
	 m_PBTrans.pvPRTrans = NULL;
	 m_PBTrans.bIsPRTransFading = false;
}

void CAkActionPlayAndContinue::SetPathInfo(AkPathInfo* in_pPathInfo)
{
	AKASSERT( g_pPathManager );
	//UnsetPath();
	m_PathInfo.pPBPath = NULL;
	m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
	if(in_pPathInfo->pPBPath)
	{
		g_pPathManager->AddPotentialUser(in_pPathInfo->pPBPath);
	}
	m_PathInfo.pPBPath = in_pPathInfo->pPBPath;
	m_PathInfo.PathOwnerID = in_pPathInfo->PathOwnerID;
	
	if( m_pPathOwner )
	{
		m_pPathOwner->Release();
		m_pPathOwner = NULL;
	}
	// Keep a reference on the Path owner since if the bank is unloaded, the path itself will be wiped out.
	m_pPathOwner = g_pIndex->GetNodePtrAndAddRef( in_pPathInfo->PathOwnerID, AkNodeType_Default );
}
/*
void CAkActionPlayAndContinue::UnsetPath()
{
	m_PathInfo.pPBPath = NULL;
	m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
}
*/
CAkTransition* CAkActionPlayAndContinue::GetPlayStopTransition( bool& out_rbIsFading )
{
	out_rbIsFading = m_PBTrans.bIsPSTransFading;
	return m_PBTrans.pvPSTrans;
}

CAkTransition* CAkActionPlayAndContinue::GetPauseResumeTransition( bool& out_rbIsFading )
{
	out_rbIsFading = m_PBTrans.bIsPRTransFading;
	return m_PBTrans.pvPRTrans;
}

void CAkActionPlayAndContinue::GetHistArray( AkCntrHistArray& out_rHistArray )
{
	out_rHistArray = m_PlayHistory.HistArray;
}

void CAkActionPlayAndContinue::SetHistory(PlayHistory& in_rPlayHistory)
{
	m_PlayHistory = in_rPlayHistory;
}

void CAkActionPlayAndContinue::SetInitialPlaybackState( AkPlaybackState in_eInitialPlaybackState )
{
	m_InitialPlaybackState = in_eInitialPlaybackState;
}

void CAkActionPlayAndContinue::SetInstigator( CAkPBIAware* in_pInstigator )
{
	AKASSERT( !m_pInstigator );
    m_pInstigator = in_pInstigator;
	m_pInstigator->AddRef();
}

bool CAkActionPlayAndContinue::NeedNotifyDelay()
{
	return m_bNeedNotifyDelay;
}

void CAkActionPlayAndContinue::SetIsFirstPlay( bool in_bIsFirstPlay )
{
	m_bIsfirstPlay = in_bIsFirstPlay;
}

PlayHistory& CAkActionPlayAndContinue::History()
{
	return m_PlayHistory;
}

void CAkActionPlayAndContinue::SetFadeBack( CAkContinuousPBI* in_pPBIToNotify, AkTimeMs in_CrossFadeTime )
{
	m_pPreviousPBI = in_pPBIToNotify;
	m_FadeOutTimeForLast = in_CrossFadeTime;
	m_bNeedNotifyDelay = false;
}

void CAkActionPlayAndContinue::SetSAInfo( AkUniqueID in_seqID )
{
	m_SA_PBIID = in_seqID;
}

void CAkActionPlayAndContinue::UnsetFadeBack( CAkContinuousPBI* in_pPBIToCheck )
{
	if(in_pPBIToCheck == m_pPreviousPBI)
	{
		m_pPreviousPBI = NULL;
	}
	//no, m_bNeedNotifyDelay must NOT be set to true here!!!, don't even think about it
}

void CAkActionPlayAndContinue::StartAsPaused()
{
	m_InitialPlaybackState = PB_Paused;
}

void CAkActionPlayAndContinue::Resume()
{
	if( m_PBTrans.bIsPRTransFading && m_PBTrans.pvPRTrans )
	{
		UnsetPauseResumeTransition();
	}
}

//return true if the action cn be destroyed
bool CAkActionPlayAndContinue::BreakToNode( CAkParameterNodeBase * in_pNodeToTest, CAkRegisteredObj* in_pGameObj, AkPendingAction* in_pPendingAction )
{
	if( m_spContList )
	{
		while( !m_spContList->m_listItems.IsEmpty() )
		{
			CAkContinueListItem& item = m_spContList->m_listItems.Last();
			if( !( item.m_pMultiPlayNode ) )
			{
				if( item.m_pContainer->IsOrIsChildOf( in_pNodeToTest ) )
				{
					m_PlayHistory.RemoveLast();
					while(m_PlayHistory.HistArray.uiArraySize
						&& !m_PlayHistory.IsContinuous( m_PlayHistory.HistArray.uiArraySize-1 ) )
					{
						m_PlayHistory.RemoveLast();
					}
					m_spContList->m_listItems.RemoveLast();
				}
				else
				{
					// REVIEW: Shouldn't we post a Monitor_Error if GetNextToPlayContinuous() fails?
					AkUniqueID uSelectedNodeID_UNUSED;
					AkUInt16 wPositionSelected;
					CAkParameterNodeBase * pNode = item.m_pContainer->GetNextToPlayContinuous( in_pGameObj, wPositionSelected, uSelectedNodeID_UNUSED, item.m_pContainerInfo, item.m_LoopingInfo );
					if(pNode)
					{
						if( NeedNotifyDelay() )
						{
							MONITOR_OBJECTNOTIF( in_pPendingAction->UserParam.PlayingID(), in_pGameObj->ID(), in_pPendingAction->UserParam.CustomParam(), AkMonitorData::NotificationReason_Delay_Ended, m_PlayHistory.HistArray, in_pPendingAction->pAction->ID(), false, 0 );
						}
						m_PlayHistory.HistArray.aCntrHist[m_PlayHistory.HistArray.uiArraySize - 1] = wPositionSelected;
						
						m_ulElementID = pNode->ID();
						if( NeedNotifyDelay() )
						{
							MONITOR_OBJECTNOTIF( in_pPendingAction->UserParam.PlayingID(), in_pGameObj->ID(), in_pPendingAction->UserParam.CustomParam(), AkMonitorData::NotificationReason_Delay_Started, m_PlayHistory.HistArray, in_pPendingAction->pAction->ID(), false, 0 );
						}
						pNode->Release();
						return false;
					}
					else
					{
						break;
					}

					
				}
			}
			else // Encountered a switch block
			{
				CAkSmartPtr<CAkContinuationList> l_spContList;
				item.m_pMultiPlayNode->ContGetList( item.m_pAlternateContList, l_spContList );
				m_spContList->m_listItems.RemoveLast();
				if( l_spContList )
				{
					// I am the chosen one, flush the old ContList and use the new one

					// Lets take the new list and lets continue playing!!!
					m_spContList = l_spContList;
				}
				else
				{
					// We are not the continuous one, so terminate normally with no next
					m_spContList = NULL;
					return true;
				}
			}
		}
		if( m_spContList->m_listItems.Length() == 0 )
		{
			m_spContList = NULL;
		}
	}
	return true;
}
