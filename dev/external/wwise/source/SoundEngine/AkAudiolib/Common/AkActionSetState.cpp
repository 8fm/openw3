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
// AkActionSetState.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSetState.h"
#include "AkStateMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

extern CAkStateMgr* g_pStateMgr;

CAkActionSetState::CAkActionSetState(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkAction(in_eActionType, in_ulID)
,m_ulStateGroupID(0)
,m_ulTargetStateID(0)
#ifndef AK_OPTIMIZED
,m_bSkipTransition(false)
#endif
{
}

CAkActionSetState::~CAkActionSetState()
{

}

AKRESULT CAkActionSetState::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pStateMgr);
#ifndef AK_OPTIMIZED
	g_pStateMgr->SetStateInternal(m_ulStateGroupID, m_ulTargetStateID, m_bSkipTransition);
#else
	g_pStateMgr->SetStateInternal(m_ulStateGroupID, m_ulTargetStateID);
#endif
	return AK_Success;
}

CAkActionSetState* CAkActionSetState::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionSetState* pActionSetState = AkNew(g_DefaultPoolId,CAkActionSetState(in_eActionType, in_ulID));
	if( pActionSetState )
	{
		if( pActionSetState->Init() != AK_Success )
		{
			pActionSetState->Release();
			pActionSetState = NULL;
		}
	}

	return pActionSetState;
}

void CAkActionSetState::SetStateGroup(const AkStateGroupID in_ulStateGroupID)
{
	m_ulStateGroupID = in_ulStateGroupID;
}

void CAkActionSetState::SetTargetState(const AkStateID in_ulStateID)
{
	m_ulTargetStateID = in_ulStateID;
}

#ifndef AK_OPTIMIZED
void CAkActionSetState::SetSkipTransition(bool in_bSkipTransition)
{
	m_bSkipTransition = in_bSkipTransition;
}
#endif

AKRESULT CAkActionSetState::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	m_ulStateGroupID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	m_ulTargetStateID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	return AK_Success;
}
