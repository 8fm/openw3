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
#include "AkSink.h"

using namespace AKPLATFORM;

extern AkPlatformInitSettings			g_PDSettings;

AkThread		CAkAudioThread::m_hEventMgrThread;
AkThreadID		CAkAudioThread::m_hEventMgrThreadID;

CAkAudioThread::CAkAudioThread()
	:m_eventProcess( CAkLEngine::GetProcessEvent() )
	,m_bStopThread(false)
{
}

void CAkAudioThread::WakeupEventsConsumer()
{
	if ( m_eventProcess != NULL )
		::SetEvent( m_eventProcess );
}

//-----------------------------------------------------------------------------
// Name: EventMgrThreadFunc
// Desc: Audio loop
//-----------------------------------------------------------------------------
AK_DECLARE_THREAD_ROUTINE(CAkAudioThread::EventMgrThreadFunc)
{
	m_hEventMgrThreadID = AKPLATFORM::CurrentThread();
	AKRANDOM::AkRandomInit();

#ifndef AK_WINPHONE
	// Need to CoInitialize so that CoCreateInstance succeeds from this thread if we instantiate a new sink on-the-fly.
	// This is only necessary if the multi-thread initialize from the main thread did not succeed.
	bool bCoInitializeSucceeded = false;
	if ( !CAkLEngine::HasCoInitializeSucceeded() ) 
		bCoInitializeSucceeded = SUCCEEDED( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED ) );	
#endif

	// get our info from the parameter
	CAkAudioThread* pAudioThread = AK_GET_THREAD_ROUTINE_PARAMETER_PTR( CAkAudioThread );

	CAkLEngine::StartVoice();

	AKASSERT(g_pAudioMgr);

	DWORD dwWaitRes = WAIT_TIMEOUT;
	do
    {
		switch( dwWaitRes )
		{
		case WAIT_OBJECT_0:			//ThreadProcessEvent
		case WAIT_TIMEOUT:			// Default Time Out
			g_pAudioMgr->Perform();
			break;
		default:
			AKASSERT( !"Unexpected event received on main thread" );
		}

#ifdef AK_USE_METRO_API
		dwWaitRes = ::WaitForSingleObjectEx( pAudioThread->m_eventProcess, g_pAkSink->GetThreadWaitTime(), FALSE ); 
#else
		dwWaitRes = ::WaitForSingleObject( pAudioThread->m_eventProcess, g_pAkSink->GetThreadWaitTime() ); 
#endif
    }
	while ( !pAudioThread->m_bStopThread );

#ifndef AK_WINPHONE
	if ( bCoInitializeSucceeded )
		CoUninitialize();
#endif

	AkExitThread( AK_RETURN_THREAD_OK );
}


AKRESULT CAkAudioThread::Start()
{
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
}
