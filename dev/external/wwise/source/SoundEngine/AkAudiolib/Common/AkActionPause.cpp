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
// AkActionPause.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionPause.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNodeBase.h"
#include "AkPBI.h"
#include "AkModifiers.h"
#include "AkAudioMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

extern CAkAudioMgr* g_pAudioMgr;

CAkActionPause::CAkActionPause(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionActive(in_eActionType, in_ulID)
, m_bPausePendingResume( true )
{
}

CAkActionPause::~CAkActionPause()
{

}

CAkActionPause* CAkActionPause::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionPause* pActionPause = AkNew(g_DefaultPoolId,CAkActionPause(in_eActionType, in_ulID));
	if( pActionPause )
	{
		if( pActionPause->Init() != AK_Success )
		{
			pActionPause->Release();
			pActionPause = NULL;
		}
	}

	return pActionPause;
}

AKRESULT CAkActionPause::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pAudioMgr);

	AKRESULT eResult = AK_Success;

	CAkRegisteredObj * pGameObj = in_pAction->GameObj();

	switch(CAkAction::ActionType())
	{
		case AkActionType_Pause_E_O:
		case AkActionType_Pause_E:
			{
				CAkParameterNodeBase* pNode = GetAndRefTarget();
				if( pNode )
				{
					eResult = Exec( ActionParamType_Pause, pGameObj, in_pAction->TargetPlayingID );
					g_pAudioMgr->PausePendingAction( pNode, pGameObj, m_bPausePendingResume, in_pAction->TargetPlayingID );
					pNode->Release();
				}
			}
			break;

		case AkActionType_Pause_ALL_O:
		case AkActionType_Pause_ALL:
			AllExec( ActionParamType_Pause, pGameObj, in_pAction->TargetPlayingID );
			g_pAudioMgr->PausePendingAction( NULL, pGameObj, m_bPausePendingResume, in_pAction->TargetPlayingID );
			break;

		case AkActionType_Pause_AE_O:
		case AkActionType_Pause_AE:
			AllExecExcept( ActionParamType_Pause, pGameObj, in_pAction->TargetPlayingID );
			g_pAudioMgr->PausePendingActionAllExcept( pGameObj, &m_listElementException, m_bPausePendingResume, in_pAction->TargetPlayingID );
			break;

		default:
			AKASSERT(!"Should not happen, unsupported condition");
			break;
	}

	return eResult;
}

void CAkActionPause::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_Pause_E ||
		in_ActionType == AkActionType_Pause_E_O ||
												
		in_ActionType == AkActionType_Pause_ALL ||
		in_ActionType == AkActionType_Pause_ALL_O ||
												
		in_ActionType == AkActionType_Pause_AE ||
		in_ActionType == AkActionType_Pause_AE_O
	);
	m_eActionType = in_ActionType;
}

AKRESULT CAkActionPause::SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt8 bIsMaster = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize);
	IncludePendingResume( bIsMaster != 0 );

	return AK_Success;
}
