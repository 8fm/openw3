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
#include <AK/Tools/Common/AkListBare.h>

#include "AkMeterFX.h"

class CAkMeterManager
{
public:
	CAkMeterManager( AK::IAkPluginMemAlloc * in_pAllocator );
	~CAkMeterManager();

	AkForceInline void Register( CAkMeterFX * in_pFX ) { m_meters.AddLast( in_pFX ); }

	static void BehavioralExtension( bool in_bLastCall ) { pInstance->Execute( in_bLastCall ); }

	static CAkMeterManager * Instance( AK::IAkPluginMemAlloc * in_pAllocator );

private:
	typedef AkListBare<CAkMeterFX,AkListBareNextItem<CAkMeterFX>,AkCountPolicyWithCount> Meters;

	void Execute( bool in_bLastCall );

	AK::IAkPluginMemAlloc * m_pAllocator;

	Meters m_meters;

	static CAkMeterManager * pInstance;
};
