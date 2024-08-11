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

//////////////////////////////////////////////////////////////////////
//
// AkBusCallbackMgr.h
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <AK/Tools/Common/AkLock.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include "AkKeyArray.h"
#include "AkManualEvent.h"

class CAkBusCallbackMgr
{
public:

	CAkBusCallbackMgr();
	~CAkBusCallbackMgr();

public:

	AKRESULT SetCallback( AkUniqueID in_busID, AkBusCallbackFunc in_pfnCallback );

	// Safe way to actually do the callback
	// Same prototype than the callback itself, with the exception that it may actually not do the callback if the
	// event was cancelled
	bool DoCallback( AkSpeakerVolumeMatrixBusCallbackInfo& in_rCallbackInfo );

	bool IsCallbackEnabled( AkUniqueID in_busID );

private:
	typedef CAkKeyArray<AkUniqueID, AkBusCallbackFunc> AkListCallbacks;
	AkListCallbacks m_ListCallbacks;

	CAkLock m_csLock;
};
