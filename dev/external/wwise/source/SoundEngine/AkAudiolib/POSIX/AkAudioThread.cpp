/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version:  Build: 
  Copyright (c) 2006-2019 Audiokinetic Inc.
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

extern AkInitSettings g_settings;
extern AkPlatformInitSettings			g_PDSettings;

AkThread		CAkAudioThread::m_hEventMgrThread;
AkThreadID		CAkAudioThread::m_hEventMgrThreadID;

CAkAudioThread::CAkAudioThread():
	m_bStopThread(false) 
{
	AkClearEvent(m_eventProcess);
    m_bEventError = (AkCreateEvent(m_eventProcess) == AK_Fail);
	AkClearThread(&m_hEventMgrThread);
}

void CAkAudioThread::WakeupEventsConsumer()
{
	if(g_settings.bUseLEngineThread)
		AkSignalEvent( m_eventProcess );
}

//-----------------------------------------------------------------------------
// Name: EventMgrThreadFunc
// Desc: Audio loop
//-----------------------------------------------------------------------------
AK_DECLARE_THREAD_ROUTINE(CAkAudioThread::EventMgrThreadFunc)
{
	AK_THREAD_INIT_CODE(g_PDSettings.threadLEngine);

	AK_INSTRUMENT_THREAD_START( "CAkAudioThread::EventMgrThreadFunc" );

	m_hEventMgrThreadID = AKPLATFORM::CurrentThread();
	
	// get our info from the parameter
	CAkAudioThread* pAudioThread = AK_GET_THREAD_ROUTINE_PARAMETER_PTR( CAkAudioThread );

	CAkLEngine::OnThreadStart();
	
	AKASSERT( g_pAudioMgr );
	do
    {
		g_pAudioMgr->Perform();
		AkWaitForEvent( pAudioThread->m_eventProcess );
	}
	while ( !pAudioThread->m_bStopThread );

	CAkLEngine::OnThreadEnd();
	
	AK::MemoryMgr::TermForThread();
    AkExitThread( AK_RETURN_THREAD_OK );
}


AKRESULT CAkAudioThread::Start()
{	
#ifndef __EMSCRIPTEN_
	if (m_bEventError) 
	{
		AkClearEvent(m_eventProcess);
		return AK_Fail;
	}
#endif
	
	m_bStopThread	= false;
	
	if ( g_settings.bUseLEngineThread )
	{
		//Create the EventManagerThread
		AkCreateThread(	EventMgrThreadFunc,					// Start address
						this,								// Parameter
						g_PDSettings.threadLEngine,			// Properties 
						&m_hEventMgrThread,					// AkHandle
						"AK::EventManager" );				// Debug name
		// is the thread ok ?
		if ( !AkIsValidThread(&m_hEventMgrThread) )
			return AK_Fail;	
	}	
	
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
		AkClearThread(&m_hEventMgrThread);
		m_hEventMgrThreadID = 0;
	}
	
	AkDestroyEvent( m_eventProcess );
	
	AkCreateEvent( m_eventProcess );
}
