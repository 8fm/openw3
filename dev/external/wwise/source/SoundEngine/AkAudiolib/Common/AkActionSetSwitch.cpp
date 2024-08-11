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
// AkActionSetSwitch.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSetSwitch.h"
#include "AkRegisteredObj.h"
#include "AkRTPCMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

#include "AkAudioMgr.h"

CAkActionSetSwitch::CAkActionSetSwitch(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkAction( in_eActionType, in_ulID )
,m_ulSwitchGroupID(0)
,m_ulSwitchStateID(0)
{
}

CAkActionSetSwitch::~CAkActionSetSwitch()
{

}

AKRESULT CAkActionSetSwitch::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pRTPCMgr);
	g_pRTPCMgr->SetSwitchInternal( m_ulSwitchGroupID, m_ulSwitchStateID, in_pAction->GameObj() );
	return AK_Success;
}

CAkActionSetSwitch* CAkActionSetSwitch::Create( AkActionType in_eActionType, AkUniqueID in_ulID )
{
	CAkActionSetSwitch* pActionSetSwitch = AkNew( g_DefaultPoolId, CAkActionSetSwitch( in_eActionType, in_ulID ) );
	if( pActionSetSwitch )
	{
		if( pActionSetSwitch->Init() != AK_Success )
		{
			pActionSetSwitch->Release();
			pActionSetSwitch = NULL;
		}
	}

	return pActionSetSwitch;
}

void CAkActionSetSwitch::SetSwitchGroup( const AkSwitchGroupID in_ulSwitchGroupID )
{
	m_ulSwitchGroupID = in_ulSwitchGroupID;
}

void CAkActionSetSwitch::SetTargetSwitch( const AkSwitchStateID in_ulSwitchID )
{
	m_ulSwitchStateID = in_ulSwitchID;
}

AKRESULT CAkActionSetSwitch::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	m_ulSwitchGroupID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	m_ulSwitchStateID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	return AK_Success;
}
