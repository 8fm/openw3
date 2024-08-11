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

#include "MusicNodeProxyLocal.h"
#include "AkMusicNode.h"
#include "AkCritical.h"

void MusicNodeProxyLocal::MeterInfo(
		bool in_bIsOverrideParent,
        const AkMeterInfo& in_MeterInfo
        )
{
	CAkMusicNode* pMusicNode = static_cast<CAkMusicNode*>( GetIndexable() );
	if( pMusicNode )
	{
		pMusicNode->MeterInfo( in_bIsOverrideParent ? &in_MeterInfo : NULL );
	}
}

void MusicNodeProxyLocal::SetStingers( 
		CAkStinger* in_pStingers, 
		AkUInt32 in_NumStingers
		)
{
	CAkMusicNode* pMusicNode = static_cast<CAkMusicNode*>( GetIndexable() );
	if( pMusicNode )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pMusicNode->SetStingers( in_pStingers, in_NumStingers );
	}
}

#endif
#endif // #ifndef AK_OPTIMIZED
