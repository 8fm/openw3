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

#include "MusicTransAwareProxyLocal.h"
#include "AkMusicTransAware.h"
#include "AkCritical.h"

void MusicTransAwareProxyLocal::SetRules(
		AkUInt32 in_NumRules,
		AkWwiseMusicTransitionRule* in_pRules
		)
{
	CAkMusicTransAware* pTransAwareNode = static_cast<CAkMusicTransAware*>( GetIndexable() );
	if( pTransAwareNode )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pTransAwareNode->SetRules( in_NumRules, in_pRules );
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
