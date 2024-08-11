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

#include "stdafx.h"
#include "AkActionBreak.h"

#include "AkAudioLibIndex.h"
#include "AkAudioMgr.h"
#include "AkMonitor.h"
#include "AkRegisteredObj.h"

extern CAkAudioMgr* g_pAudioMgr;

CAkActionBreak* CAkActionBreak::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionBreak*	pActionBreak = AkNew( g_DefaultPoolId,CAkActionBreak( in_eActionType, in_ulID ) );

	if( pActionBreak && pActionBreak->Init() != AK_Success )
	{
		pActionBreak->Release();
		pActionBreak = NULL;
	}

	return pActionBreak;
}

CAkActionBreak::CAkActionBreak( AkActionType in_eActionType, AkUniqueID in_ulID ) 
	: CAkAction(in_eActionType, in_ulID)
{
}

CAkActionBreak::~CAkActionBreak()
{
	
}

AKRESULT CAkActionBreak::Execute(
	AkPendingAction * in_pAction )
{
	CAkParameterNodeBase* pNode = GetAndRefTarget();
	if(!pNode)
	{
		AkGameObjectID goID = AK_INVALID_GAME_OBJECT;
		if( in_pAction->GameObj() )
		{
			goID = in_pAction->GameObj()->ID();
		}
		MONITOR_ERROREX( AK::Monitor::ErrorCode_SelectedNodeNotAvailable, AK_INVALID_PLAYING_ID, goID, m_ulElementID, IsBusElement() );
		return AK_IDNotFound;
	}

	ActionParams l_Params;
	l_Params.eType = ActionParamType_Break;
	l_Params.pGameObj = in_pAction->GameObj();
	l_Params.playingID = in_pAction->TargetPlayingID;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterCall = false;
	l_Params.bIsMasterResume = false;
	l_Params.targetNodePtr = pNode;
	AKRESULT eResult = pNode->ExecuteAction( l_Params );
	if( eResult == AK_Success )
	{
		// Breaking delayed actions and X-Faded actions stands for stopping them simply, excepted if they are continuous and should target an higher level instead.
		// this implementation of BreakPendingAction() will stop only actions that target contained elements.
		eResult = g_pAudioMgr->BreakPendingAction( pNode, in_pAction->GameObj(), in_pAction->TargetPlayingID );
	}

	pNode->Release();

	return eResult;
}
