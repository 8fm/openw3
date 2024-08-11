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

#include "MusicTransAwareProxyConnected.h"
#include "AkMusicTransAware.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "IMusicTransAwareProxy.h"

MusicTransAwareProxyConnected::MusicTransAwareProxyConnected()
{
}

MusicTransAwareProxyConnected::~MusicTransAwareProxyConnected()
{
}

void MusicTransAwareProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkMusicTransAware* pTransAwareNode = static_cast<CAkMusicTransAware*>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IMusicTransAwareProxy::MethodSetRules:
		{
			MusicTransAwareProxyCommandData::SetRules setRules;
			if (in_rSerializer.Get( setRules ))
				pTransAwareNode->SetRules( setRules.m_NumRules, setRules.m_pRules );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
