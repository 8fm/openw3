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

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/SoundEngine/Common/AkTypes.h>

class IAkRumbleController
{
public:	
	virtual AKRESULT SetRumble(AkReal32 in_fLarge, AkReal32 in_fSmall) = 0; // values in range [0..1]
	virtual bool IsActive() = 0;
	virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator ) = 0;
};
