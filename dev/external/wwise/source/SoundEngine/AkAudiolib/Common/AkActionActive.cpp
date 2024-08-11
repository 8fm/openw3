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
// AkActionActive.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionActive.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNodeBase.h"
#include "AkPBI.h"
#include "AkRegistryMgr.h"
#include "AkAudioMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkModifiers.h"
#include "AkBus.h"
#include "AkDynamicSequence.h"

#ifdef AK_MOTION
	#include "AkFeedbackBus.h"
#endif // AK_MOTION

extern CAkAudioMgr* g_pAudioMgr;

CAkActionActive::CAkActionActive(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionExcept(in_eActionType, in_ulID)
, m_bIsMasterResume(false)
{
}

CAkActionActive::~CAkActionActive()
{
}

AKRESULT CAkActionActive::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt8 ucFadeCurveType = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize );

	m_eFadeCurve = (AkCurveInterpolation)ucFadeCurveType;

	AKRESULT eResult = SetActionSpecificParams(io_rpData, io_rulDataSize);

	if( eResult == AK_Success )
		eResult = SetExceptParams( io_rpData, io_rulDataSize );

	return eResult;
}

AKRESULT CAkActionActive::Exec( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj, AkPlayingID in_TargetPlayingID )
{
	CAkParameterNodeBase* pNode = GetAndRefTarget();
	if(pNode)
	{
		ActionParams l_Params;
		l_Params.bIsFromBus = false;
		l_Params.bIsMasterCall = false;
		l_Params.bIsMasterResume = m_bIsMasterResume;
		l_Params.transParams.eFadeCurve = (AkCurveInterpolation)m_eFadeCurve;
		l_Params.eType = in_eType;
		l_Params.pGameObj = in_pGameObj;
		l_Params.playingID = in_TargetPlayingID;
		l_Params.transParams.TransitionTime = GetTransitionTime();

		AKRESULT eResult = pNode->ExecuteAction( l_Params );

		pNode->Release();

		return eResult;
	}
	else
	{
		return AK_IDNotFound;
	}
}

void CAkActionActive::AllExec( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj, AkPlayingID in_TargetPlayingID )
{
	AKASSERT( g_pIndex );
	CAkIndexItem<CAkDynamicSequence*>& l_rIdx = g_pIndex->m_idxDynamicSequences;

	{	//Bracket for autolock
		AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

		CAkIndexItem<CAkDynamicSequence*>::AkMapIDToPtr::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
		while( iter != l_rIdx.m_mapIDToPtr.End() )
		{
			static_cast<CAkDynamicSequence*>( *iter )->AllExec( in_eType, in_pGameObj );
			++iter;
		}
	}

	ActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = m_bIsMasterResume;
	l_Params.transParams.eFadeCurve = (AkCurveInterpolation)m_eFadeCurve;
	l_Params.eType = in_eType;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_TargetPlayingID;
	l_Params.transParams.TransitionTime = GetTransitionTime();

	if( l_Params.pGameObj == NULL )
		l_Params.bIsMasterCall = true;
	else
		l_Params.bIsMasterCall = false;

	CAkBus::ExecuteMasterBusAction(l_Params);

#ifdef AK_MOTION
	CAkFeedbackBus* pMotionBus = CAkFeedbackBus::GetMasterMotionBusAndAddRef();
	if (pMotionBus != NULL)
	{
		pMotionBus->ExecuteAction(l_Params);
		pMotionBus->Release();
	}
#endif // AK_MOTION

}

void CAkActionActive::AllExecExcept( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj, AkPlayingID in_TargetPlayingID )
{
	ActionParamsExcept l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = m_bIsMasterResume;
    l_Params.transParams.eFadeCurve = (AkCurveInterpolation)m_eFadeCurve;
	l_Params.eType = in_eType;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_TargetPlayingID;
	l_Params.pExeceptionList = &m_listElementException;
	l_Params.transParams.TransitionTime = GetTransitionTime();

	CAkBus::ExecuteMasterBusActionExcept(l_Params);

#ifdef AK_MOTION
	CAkFeedbackBus* pMotionBus = CAkFeedbackBus::GetMasterMotionBusAndAddRef();
	if (pMotionBus != NULL)
	{
		pMotionBus->ExecuteActionExcept(l_Params);
		pMotionBus->Release();
	}
#endif // AK_MOTION

}
