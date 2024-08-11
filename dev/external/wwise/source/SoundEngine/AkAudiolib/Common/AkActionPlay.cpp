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
// AkActionPlay.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionPlay.h"
#include "AkPBI.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNode.h"
#include "AkMonitor.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkCntrHistory.h"
#include "AkModifiers.h"

#include "AkAudioMgr.h"

CAkActionPlay::CAkActionPlay(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkAction(in_eActionType, in_ulID)
, m_fileID( AK_INVALID_FILE_ID )
{
}

CAkActionPlay::~CAkActionPlay()
{
	
}

AKRESULT CAkActionPlay::Execute( AkPendingAction * in_pAction )
{
	AKRESULT eResult = AK_Success;

	AkPropValue * pProbability = m_props.FindProp( AkPropID_Probability );
	if ( AK_EXPECT_FALSE( pProbability != NULL ) )
	{
		// Must be using a long 30 bits random, a normal random would make it impossible for a sound under 0.003% to ever happen.
		if( ( pProbability->fValue == 0.0f
			|| ( AKRANDOM::AkRandom_30_bits() / (AkReal64)AKRANDOM::AK_RANDOM_MAX_30_BITS ) * 100 > pProbability->fValue ) )
			return AK_Success;
	}

	CAkParameterNodeBase* pNode = GetAndRefTarget();

	if(pNode)
	{
		TransParams	Tparameters;

		Tparameters.TransitionTime = GetTransitionTime();
		Tparameters.eFadeCurve = (AkCurveInterpolation)m_eFadeCurve;

		AkPBIParams pbiParams;
        
		pbiParams.eType = AkPBIParams::PBI;
        pbiParams.pInstigator = pNode;
		pbiParams.userParams = in_pAction->UserParam;
		pbiParams.ePlaybackState = PB_Playing;
		pbiParams.uFrameOffset = in_pAction->LaunchFrameOffset;
        pbiParams.bIsFirst = true;

		pbiParams.pGameObj = in_pAction->GameObj();

		pbiParams.pTransitionParameters = &Tparameters;
        pbiParams.pContinuousParams = NULL;
        pbiParams.sequenceID = AK_INVALID_SEQUENCE_ID;

		eResult = static_cast<CAkParameterNode*>(pNode)->Play( pbiParams );

		pNode->Release();
	}
	else
	{
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( in_pAction->UserParam.PlayingID(), in_pAction->GameObjID(), in_pAction->UserParam.CustomParam(), AkMonitorData::NotificationReason_PlayFailed, HistArray, m_ulElementID, false, 0 );
		MONITOR_ERROREX( AK::Monitor::ErrorCode_SelectedNodeNotAvailablePlay, in_pAction->UserParam.PlayingID(), in_pAction->GameObjID(), m_ulElementID, false );
		eResult = AK_IDNotFound;
	}
	return eResult;
}

CAkActionPlay* CAkActionPlay::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionPlay*	pActionPlay = AkNew(g_DefaultPoolId,CAkActionPlay(in_eActionType, in_ulID));
	if( pActionPlay )
	{
		if( pActionPlay->Init() != AK_Success )
		{
			pActionPlay->Release();
			pActionPlay = NULL;
		}
	}

	return pActionPlay;
}

void CAkActionPlay::GetHistArray( AkCntrHistArray& out_rHistArray )
{
	//we don't have any so we give away a clean copy
	out_rHistArray.Init();
}

AKRESULT CAkActionPlay::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt8 ucFadeCurveType = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	m_eFadeCurve = (AkCurveInterpolation)ucFadeCurveType;

	SetFileID( READBANKDATA( AkFileID, io_rpData, io_rulDataSize ) );

	m_bWasLoadedFromBank = true;

	return AK_Success;
}
