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
// AkLEngine.h
//
// Implementation of the Lower Audio Engine.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_LENGINE_H_
#define _AK_LENGINE_H_

#include "AkSrcBase.h"
#include "AkCommon.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkKeyList.h"
#include "AkLEngineStructs.h"
#include "AkList2.h"
#include <AK/Tools/Common/AkListBare.h>
#include <AK/Tools/Common/AkLock.h>
#include "AkStaticArray.h"
#include "AkVPLSrcCbxNode.h"
#include "AkVPLMixBusNode.h"
#include "AkVPLFinalMixNode.h"
#include "AkLEngineCmds.h"
#include "AkVPL.h"

#ifndef AK_USE_METRO_API
#include <dbt.h>
#endif

class CAkFeedbackDeviceMgr;
class CAkMMNotificationClient;

#define CACHED_BUFFER_SIZE_DIVISOR		(LE_MAX_FRAMES_PER_BUFFER*2) // 16-bit minimum sample size
#define NUM_CACHED_BUFFER_SIZES			(AK_VOICE_MAX_NUM_CHANNELS*2) // in CACHED_BUFFER_SIZE_DIVISOR increments
#define NUM_CACHED_BUFFERS				2 // should be somewhat like the max number of buffers allocated at once during a single voice execution

#define LENGINE_DEFAULT_POOL_SIZE		(16 * 1024 * 1024)
#define LENGINE_DEFAULT_ALLOCATION_TYPE	AkMalloc
#define UENGINE_DEFAULT_ALLOCATION_TYPE	AkMalloc
#define LENGINE_DEFAULT_POOL_ALIGN		(16)

class CAkSink;

//-----------------------------------------------------------------------------
// CAkLEngine class.
//-----------------------------------------------------------------------------
class CAkLEngine
{
	#include "AkLEngine_Common_incl.h"
	#include "AkLEngine_SoftwarePipeline_incl.h"

public:
	static void ResetAudioDevice();
	static bool HasCoInitializeSucceeded() { return m_bCoInitializeSucceeded; }

	static HANDLE GetProcessEvent() {return m_eventProcess;}

private:
	static AKRESULT ReplaceCurrentSink(CAkSink * in_pSink);
	static void RegisterDeviceChange();
	static void UnregisterDeviceChange();
	static LRESULT CALLBACK InternalWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
private: 
	static bool					m_bCoInitializeSucceeded;	// Tell if the CoInitialized succeeded - then we need to coUninitialize
	static bool					m_bResetAudioDevice;		// When set to true, sound engine will respawn a new sink

#ifndef AK_USE_METRO_API
	static WNDPROC				m_WndProc;					// The WndProc of the game.
	static HDEVNOTIFY			m_hDevNotify;
	static CAkMMNotificationClient * m_pMMNotifClient;
#endif

	static HANDLE m_eventProcess;
};
#endif
