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

#include "FeedbackMgrProxyLocal.h"
#include <AK/MotionEngine/Common/AkMotionEngine.h>

#ifdef AK_WIN
#include <AK/SoundEngine/Common/aksoundengine.h>
#include "AkDirectInputHelper.h"								// Sample directInput implementation

	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif

FeedbackMgrProxyLocal::FeedbackMgrProxyLocal()
{
}

FeedbackMgrProxyLocal::~FeedbackMgrProxyLocal()
{
}

void FeedbackMgrProxyLocal::Enable( AkInt16 in_usCompany, AkInt16 in_usPlugin, AkCreatePluginCallback in_fnInit, bool in_bEnable )
{
	//Use "player" 0 to represent the Wwise application.
	if (in_bEnable)
	{
		AK::MotionEngine::RegisterMotionDevice(in_usCompany, in_usPlugin, in_fnInit);
		
	#ifdef AK_WIN
		extern AkPlatformInitSettings g_PDSettings;
		AkDirectInputDevice::InitControllers(g_PDSettings.hWnd);
		if ( !AkDirectInputDevice::GetControllers().m_nXInputControllerCount)
			AK::MotionEngine::AddPlayerMotionDevice( 0, in_usCompany, in_usPlugin, AkDirectInputDevice::GetFirstDirectInputController() );
		else
	#endif
			AK::MotionEngine::AddPlayerMotionDevice(0, in_usCompany, in_usPlugin);

		AK::SoundEngine::SetListenerPipeline(1, true, true);
		AK::MotionEngine::SetPlayerListener(0, 1);
	}
	else
		AK::MotionEngine::RemovePlayerMotionDevice(0, in_usCompany, in_usPlugin);
}
#endif // #ifndef AK_OPTIMIZED
