/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderFence.h"
#include "renderThread.h"

CRenderFence::CRenderFence()
	: m_isSignaled( false )
	, m_semaphore( 0, 1 )
{
}

CRenderFence::~CRenderFence()
{
}

#ifndef RED_FINAL_BUILD
Double createTime = 0.0f;
Double issueTime = 0.0f;
Double acceptTime = 0.0f;
#endif

#ifdef RED_PLATFORM_WINPC
extern void Hack_PumpWindowsMessagesForDXGI();
#endif // RED_PLATFORM_WINPC

void CRenderFence::FlushFence()
{
	PC_SCOPE( RenderFenceWaitTime );

#ifndef RED_FINAL_BUILD
	issueTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
#endif

	// Wait for signal
#ifdef RENDERFENCE_FAIL_HACK
	//this is really risky especially on startup
	Bool result = m_semaphore.TryAcquire( 60000 );
	if ( !result )
	{
		WARN_RENDERER( TXT("RenderingFence failed. Releasing to prevent deadlocks !") );
	}
#else

	// DXGI needs events processed to avoid hangs during present and other calls
	// launcher only for now to keep the change limited
# if defined( RED_PLATFORM_WINPC )
	while ( !m_semaphore.TryAcquire( 100 ) )
	{
		Hack_PumpWindowsMessagesForDXGI();
	}
# else
	m_semaphore.Acquire();
# endif
#endif

	//Double globalTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	//Float time = (Float)(( globalTime - issueTime ) * 1000.0);
	//Float time2 = (Float)(( globalTime - acceptTime ) * 1000.0);
	//Float time3 = (Float)(( globalTime - createTime ) * 1000.0);
	//LOG_RENDERER( TXT("Process time: %1.2fms, Response time: %1.2fms, Issue time: %1.2fms"), time3, time2, time );
}

void CRenderFence::SignalWaitingThreads()
{
#ifndef RED_FINAL_BUILD
	acceptTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
#endif
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_internalMutex );

	// Mark as signaled
	m_semaphore.Release();
}
