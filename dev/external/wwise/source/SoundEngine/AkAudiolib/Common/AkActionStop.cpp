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
// AkActionStop.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionStop.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNodeBase.h"
#include "AkPBI.h"
#include "AkModifiers.h"
#include "AkAudioMgr.h"
#include "AkAudioLib.h"

extern CAkAudioMgr* g_pAudioMgr;

CAkActionStop::CAkActionStop(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionActive(in_eActionType, in_ulID)
{
}

CAkActionStop::~CAkActionStop()
{

}

CAkActionStop* CAkActionStop::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionStop* pActionStop = AkNew(g_DefaultPoolId,CAkActionStop(in_eActionType, in_ulID));
	if( pActionStop )
	{
		if( pActionStop->Init() != AK_Success )
		{
			pActionStop->Release();
			pActionStop = NULL;
		}
	}

	return pActionStop;
}

AKRESULT CAkActionStop::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pAudioMgr);

	AKRESULT eResult = AK_Success;

	CAkRegisteredObj * pGameObj = in_pAction->GameObj();

	switch(CAkAction::ActionType())
	{
		case AkActionType_Stop_E:
		case AkActionType_Stop_E_O:
			{
				CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
				pTargetNode.Attach( GetAndRefTarget() );
				if( pTargetNode )
				{
					eResult = Exec( ActionParamType_Stop, pGameObj, in_pAction->TargetPlayingID );
					g_pAudioMgr->StopPendingAction( pTargetNode, pGameObj, in_pAction->TargetPlayingID );
				}
			}
			break;

		case AkActionType_Stop_ALL:
		case AkActionType_Stop_ALL_O:
			AllExec( ActionParamType_Stop, pGameObj, in_pAction->TargetPlayingID );
			g_pAudioMgr->StopPendingAction( NULL, pGameObj, in_pAction->TargetPlayingID );
			break;

		case AkActionType_Stop_AE_O:
		case AkActionType_Stop_AE:
			AllExecExcept( ActionParamType_Stop, pGameObj, in_pAction->TargetPlayingID );
			g_pAudioMgr->StopPendingActionAllExcept( pGameObj, &m_listElementException, in_pAction->TargetPlayingID );
			break;

		default:
			AKASSERT(!"Should not happen, unsupported Stop condition");
			break;
	}
	return eResult;
}

void CAkActionStop::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_Stop_E ||
		in_ActionType == AkActionType_Stop_E_O ||
												
		in_ActionType == AkActionType_Stop_ALL ||
		in_ActionType == AkActionType_Stop_ALL_O ||
												
		in_ActionType == AkActionType_Stop_AE ||
		in_ActionType == AkActionType_Stop_AE_O 
	);
	m_eActionType = in_ActionType;
}
