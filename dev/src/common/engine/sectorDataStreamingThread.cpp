/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorDataStreamingThread.h"
#include "sectorDataStreamingContext.h"

CSectorDataStreamingThread::CSectorDataStreamingThread( CSectorDataStreamingContextThreadData* threadData )
	: Red::Threads::CThread( "StreamingThread" )
	, m_exit( false )
	, m_processing( false )
	, m_startLock( 0, 1 )
	, m_finishLock( 0, 1 )
	, m_threadData( threadData )
	, m_queryContext( nullptr )
{
}

CSectorDataStreamingThread::~CSectorDataStreamingThread()
{
	SendKillSignal();
	JoinThread();
}

void CSectorDataStreamingThread::Start( class CSectorDataStreamingContext& data, const Vector& referencePosition )
{
	m_queryContext = &data;
	m_queryPosition = referencePosition;
	m_startLock.Release();	
}

void CSectorDataStreamingThread::Finish()
{
	m_finishLock.Acquire();
	m_queryContext->ProcessSync( *m_threadData );
}

void CSectorDataStreamingThread::SendKillSignal()
{
	m_exit.SetValue(true);
	m_startLock.Release();
}

void CSectorDataStreamingThread::ThreadFunc()
{
#ifdef RED_PLATFORM_DURANGO
	PROCESSOR_NUMBER nProc;
	{
		nProc.Group = 0;
		nProc.Number = 6;
		nProc.Reserved = 0;
	}
	::SetThreadIdealProcessorEx( ::GetCurrentThread(), &nProc, nullptr );
#endif

	LOG_ENGINE( TXT("Streaming thread started") );

	Memory::RegisterCurrentThread();

	while ( 1 )
	{
		m_startLock.Acquire();

		if ( m_exit.GetValue() )
			break;

		{
			m_processing.SetValue(true);

			RED_FATAL_ASSERT( m_queryContext != nullptr, "Invalid processing context" );
			m_queryContext->ProcessAsync( *m_threadData, m_queryPosition );

			m_processing.SetValue( false );
		}

		m_finishLock.Release();
	}

	LOG_ENGINE( TXT("Streaming thread finished") );
}

