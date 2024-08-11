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
#pragma once

#ifndef AK_OPTIMIZED
#ifndef PROXYCENTRAL_CONNECTED

#include "ParameterNodeProxyLocal.h"
#include "IMusicNodeProxy.h"

class MusicNodeProxyLocal : public ParameterNodeProxyLocal
							, virtual public IMusicNodeProxy
{
public:

	virtual void MeterInfo(
		bool in_bIsOverrideParent,
        const AkMeterInfo& in_MeterInfo
        );

	virtual void SetStingers( 
		CAkStinger* in_pStingers, 
		AkUInt32 in_NumStingers
		);
};
#endif
#endif // #ifndef AK_OPTIMIZED
