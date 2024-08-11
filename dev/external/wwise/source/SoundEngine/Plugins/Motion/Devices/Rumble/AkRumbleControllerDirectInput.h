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

#include "IAkRumbleController.h"

#define DIRECTINPUT_VERSION 0x0800
#include <Dinput.h>

class AkRumbleControllerDirectInput : public IAkRumbleController
{
public:

	AkRumbleControllerDirectInput(IDirectInputDevice8 * in_pController);
	~AkRumbleControllerDirectInput();
	
	// IAkRumbleController implementation
	virtual AKRESULT SetRumble(AkReal32 in_fLarge, AkReal32 in_fSmall);
	virtual bool IsActive();
	virtual AKRESULT Term(AK::IAkPluginMemAlloc * in_pAllocator );
private:
	HRESULT CreateEffect();
	IDirectInputEffect * m_pEffect;	
	IDirectInputDevice8 * m_pController;
};