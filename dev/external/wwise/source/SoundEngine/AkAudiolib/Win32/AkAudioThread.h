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
// AkAudioThread.h
//
// Win32 specific implementation of thread audio renderer loop.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIO_THREAD_H_
#define _AUDIO_THREAD_H_

#include <AK/Tools/Common/AkPlatformFuncs.h>

class CAkAudioThread
{
public:
	CAkAudioThread();

	//Start/stop the Audio Thread
	AKRESULT Start();
	void Stop();

	void WakeupEventsConsumer();
	AkThreadID GetAudioThreadID() {return m_hEventMgrThreadID;}
	HANDLE GetEvent() {return m_eventProcess;}
private:
	
	// Sound Thread function
    //
    // Return - CALLBACK - return value
    static AK_DECLARE_THREAD_ROUTINE(EventMgrThreadFunc);

	HANDLE			m_eventProcess;
	bool			m_bStopThread;

	// EventMgr Thread Information
	static AkThread	m_hEventMgrThread;
	static AkThreadID m_hEventMgrThreadID;
};

#endif //_AUDIO_THREAD_H_
