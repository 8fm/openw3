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
#include "AkActionResume.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNodeBase.h"
#include "AkPBI.h"
#include "AkModifiers.h"
#include "AkAudioMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

extern CAkAudioMgr* g_pAudioMgr;

CAkActionResume::CAkActionResume(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionActive(in_eActionType, in_ulID)
{
}

CAkActionResume::~CAkActionResume()
{

}

CAkActionResume* CAkActionResume::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionResume* pActionResume = AkNew(g_DefaultPoolId,CAkActionResume(in_eActionType, in_ulID));
	if( pActionResume )
	{
		if( pActionResume->Init() != AK_Success )
		{
			pActionResume->Release();
			pActionResume = NULL;
		}
	}

	return pActionResume;
}

AKRESULT CAkActionResume::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pAudioMgr);

	AKRESULT eResult = AK_Success;

	CAkRegisteredObj * pGameObj = in_pAction->GameObj();

	switch(CAkAction::ActionType())
	{
		case AkActionType_Resume_E_O:
		case AkActionType_Resume_E:
			{
				CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
				pTargetNode.Attach( GetAndRefTarget() );
				if( pTargetNode )
				{
					eResult = Exec( ActionParamType_Resume, pGameObj, in_pAction->TargetPlayingID );
					g_pAudioMgr->ResumePausedPendingAction( pTargetNode, pGameObj, m_bIsMasterResume, in_pAction->TargetPlayingID );
				}
			}
			break;

		case AkActionType_Resume_ALL_O:
		case AkActionType_Resume_ALL:
			AllExec( ActionParamType_Resume, pGameObj, in_pAction->TargetPlayingID );
			g_pAudioMgr->ResumePausedPendingAction( NULL, pGameObj, m_bIsMasterResume, in_pAction->TargetPlayingID );
			break;

		case AkActionType_Resume_AE_O:
		case AkActionType_Resume_AE:
			AllExecExcept( ActionParamType_Resume, pGameObj, in_pAction->TargetPlayingID );
			g_pAudioMgr->ResumePausedPendingActionAllExcept( pGameObj, &m_listElementException, m_bIsMasterResume, in_pAction->TargetPlayingID );
			break;

		default:
			AKASSERT(!"Should not happen, unsupported condition");
			break;
	}
	return eResult;
}

void CAkActionResume::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_Resume_E ||
		in_ActionType == AkActionType_Resume_E_O ||
												
		in_ActionType == AkActionType_Resume_ALL ||
		in_ActionType == AkActionType_Resume_ALL_O ||
												
		in_ActionType == AkActionType_Resume_AE ||
		in_ActionType == AkActionType_Resume_AE_O 
	);
	m_eActionType = in_ActionType;
}

AKRESULT CAkActionResume::SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt8 bIsMaster = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize);
	IsMasterResume( bIsMaster != 0 );

	return AK_Success;
}
