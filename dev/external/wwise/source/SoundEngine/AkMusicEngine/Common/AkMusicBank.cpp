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
// AkMusicBank.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMusicBank.h"
#include "AkBanks.h"
#include "AkMusicSegment.h"
#include "AkMusicRanSeqCntr.h"
#include "AkMusicSwitchCntr.h"
#include "AkMusicTrack.h"
#include "AkBankMgr.h"
#include "AkCritical.h"

class CAkUsageSlot;

AKRESULT AkMusicBank::LoadBankItem( const AkBank::AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID )
{
	AKRESULT eResult = AK_PartialSuccess;
	switch( in_rSection.eHircType )
	{
	case AkBank::HIRCType_Track:
		eResult = g_pBankManager->ReadSourceParent<CAkMusicTrack>( in_rSection, in_pUsageSlot, in_dwBankID );
		break;

	case AkBank::HIRCType_Segment:
		eResult = g_pBankManager->StdBankRead<CAkMusicSegment, CAkParameterNodeBase>( in_rSection, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Default ) );
		break;

	case AkBank::HIRCType_MusicSwitch:
		eResult = g_pBankManager->StdBankRead<CAkMusicSwitchCntr, CAkParameterNodeBase>( in_rSection, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Default ) );
		break;

	case AkBank::HIRCType_MusicRanSeq:
		eResult = g_pBankManager->StdBankRead<CAkMusicRanSeqCntr, CAkParameterNodeBase>( in_rSection, in_pUsageSlot, g_pIndex->GetNodeIndex( AkNodeType_Default ) );
		break;

	}
	return eResult;
}
