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
#ifndef AK_OPTIMIZED

#include "MusicSwitchProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "IMusicSwitchProxy.h"

MusicSwitchProxyConnected::MusicSwitchProxyConnected( AkUniqueID in_id ):
	MultiSwitchProxyConnected<MusicTransAwareProxyConnected, CAkMusicSwitchCntr, AkIdxType_AudioNode>( in_id )
{
}

MusicSwitchProxyConnected::~MusicSwitchProxyConnected()
{
}

void MusicSwitchProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkMusicSwitchCntr* pMusicSwitch = static_cast<CAkMusicSwitchCntr*>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IMusicSwitchProxy::MethodContinuePlayback:
		{
			MusicSwitchProxyCommandData::ContinuePlayback continuePlayback;
			if(in_rSerializer.Get( continuePlayback ))
				pMusicSwitch->ContinuePlayback( continuePlayback.m_bContinuePlayback );
			break;
		}
	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
