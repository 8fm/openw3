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
#ifndef PROXYCENTRAL_CONNECTED

#include "MusicSwitchProxyLocal.h"
#include "AkMusicSwitchCntr.h"
#include "AkAudioLib.h"
#include "AkCritical.h"

MusicSwitchProxyLocal::MusicSwitchProxyLocal( AkUniqueID in_id ): 
	MultiSwitchProxyLocal<MusicTransAwareProxyLocal, CAkMusicSwitchCntr, AkIdxType_AudioNode>( in_id )
{
}

MusicSwitchProxyLocal::~MusicSwitchProxyLocal()
{
}

void MusicSwitchProxyLocal::ContinuePlayback( bool in_bContinuePlayback )
{
	CAkMusicSwitchCntr* pMusicSwitch = static_cast<CAkMusicSwitchCntr*>( GetIndexable() );
	if( pMusicSwitch )
	{
		pMusicSwitch->ContinuePlayback( in_bContinuePlayback );
	}
}

#endif
#endif // #ifndef AK_OPTIMIZED
