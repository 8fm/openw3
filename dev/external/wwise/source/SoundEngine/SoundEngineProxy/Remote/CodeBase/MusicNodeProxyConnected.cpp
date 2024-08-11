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

#include "MusicNodeProxyConnected.h"
#include "AkMusicNode.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "IMusicNodeProxy.h"

MusicNodeProxyConnected::MusicNodeProxyConnected()
{
}

MusicNodeProxyConnected::~MusicNodeProxyConnected()
{
}

void MusicNodeProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkMusicNode* pMusicNode = static_cast<CAkMusicNode*>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IMusicNodeProxy::MethodMeterInfo:
		{
			MusicNodeProxyCommandData::MeterInfo meterInfo;
			if (in_rSerializer.Get( meterInfo ))
				pMusicNode->MeterInfo( meterInfo.m_bIsOverrideParent ? &meterInfo.m_MeterInfo : NULL );
			break;
		}

	case IMusicNodeProxy::MethodSetStingers:
		{
			MusicNodeProxyCommandData::SetStingers setStingers;
			if(in_rSerializer.Get( setStingers ))
				pMusicNode->SetStingers( setStingers.m_pStingers, setStingers.m_NumStingers );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
