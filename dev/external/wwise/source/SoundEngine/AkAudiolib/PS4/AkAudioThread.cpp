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
// AkAudioThread.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAudioThread.h"
#include "AkAudioMgr.h"
#include "AkRandom.h"
#include "AkLEngine.h"

using namespace AKPLATFORM;

extern AkPlatformInitSettings			g_PDSettings;

AkThread		CAkAudioThread::m_hEventMgrThread;
AkThreadID		CAkAudioThread::m_hEventMgrThreadID;

CAkAudioThread::CAkAudioThread():
	m_bStopThread(false) 
{
	AkClearEvent(m_eventProcess);
	AkClearThread(&m_hEventMgrThread);
	AkCreateNamedEvent( m_eventProcess, "AudioThread" );
}

void CAkAudioThread::WakeupEventsConsumer()
{
	AkSignalEvent( m_eventProcess );
}

//-----------------------------------------------------------------------------
// Name: EventMgrThreadFunc
// Desc: Audio loop
//-----------------------------------------------------------------------------
AK_DECLARE_THREAD_ROUTINE(CAkAudioThread::EventMgrThreadFunc)
{
	m_hEventMgrThreadID = AKPLATFORM::CurrentThread();
	AKRANDOM::AkRandomInit();
	
	// get our info from the parameter
	CAkAudioThread* pAudioThread = AK_GET_THREAD_ROUTINE_PARAMETER_PTR( CAkAudioThread );
	
	AKASSERT( g_pAudioMgr );
	do
    {
		g_pAudioMgr->Perform();
		AkWaitForEvent( pAudioThread->m_eventProcess );
	}
	while ( !pAudioThread->m_bStopThread );
	
    AkExitThread( AK_RETURN_THREAD_OK );
}


AKRESULT CAkAudioThread::Start()
{	
	if ( m_eventProcess == NULL ) 
		return AK_Fail;
	
	m_bStopThread	= false;
	
	//Create the EventManagerThread
	AkCreateThread(	EventMgrThreadFunc,					// Start address
					this,								// Parameter
					g_PDSettings.threadLEngine,			// Properties 
					&m_hEventMgrThread,					// AkHandle
					"AK::EventManager" );				// Debug name
	// is the thread ok ?
	if ( !AkIsValidThread(&m_hEventMgrThread) )
		return AK_Fail;
	
	CAkLEngine::StartVoice();
	
	return AK_Success;
}

void CAkAudioThread::Stop()
{
	m_bStopThread = true;
	if ( AkIsValidThread( &m_hEventMgrThread ) )
	{
		WakeupEventsConsumer();
		AkWaitForSingleThread( &m_hEventMgrThread );
		AkCloseThread(&m_hEventMgrThread);
	}
	
	AkDestroyEvent( m_eventProcess );
	
	AkCreateNamedEvent( m_eventProcess, "CAkAudioThread::Stop" );
}
