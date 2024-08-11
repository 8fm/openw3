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
#include "RumbleDeviceHelper.h"
#include "AkRumbleControllerXInput.h"

#if defined AK_WIN && !defined AK_USE_METRO_API
#include "AkRumbleControllerDirectInput.h"
#endif


namespace RumbleDeviceHelper
{

IAkRumbleController * InitRumbleController(AK::IAkPluginMemAlloc * in_pAllocator, AkUInt8 in_iPlayer, void * in_pDevice)
{
	IAkRumbleController * pController(NULL);
#if defined AK_WIN && !defined AK_USE_METRO_API
	if (in_pDevice)
		//create direct input device
		pController = AK_PLUGIN_NEW(in_pAllocator, AkRumbleControllerDirectInput((IDirectInputDevice8 *)in_pDevice ));
	else
#endif
		//create XInput device
		pController = AK_PLUGIN_NEW( in_pAllocator , AkRumbleControllerXInput( in_iPlayer ) );
	
	return pController;
}

}