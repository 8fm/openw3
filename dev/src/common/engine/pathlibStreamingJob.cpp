/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibStreamingJob.h"

#include "pathlibStreamingManager.h"
#include "pathlibTaskPostStreaming.h"
#include "pathlibWorld.h"


namespace PathLib
{

CStreamingJob::CStreamingJob( IStreamingItem::List::Head&& requestsList, CStreamingManager* streamingManager )
	: ILoadJob( JP_Resources )
	, m_requests( Move( requestsList ) )
	, m_shutdownRequest( false )
	, m_streamingManager( streamingManager )
{

}
EJobResult CStreamingJob::Process()
{
	for ( auto it = m_requests.Begin(), end = m_requests.End(); it != end; ++it )
	{
		if ( m_shutdownRequest.GetValue() )
		{
			return JR_Failed;
		}
		IStreamingItem& item = *it;
		item.Load();
	}

	CTaskPostStreaming* task = new CTaskPostStreaming( *m_streamingManager->GetPathLib().GetTaskManager(), Move( m_requests ) );
	m_streamingManager->GetPathLib().GetTaskManager()->AddTask( task );
	task->Release();

	m_streamingManager->MarkJobCompleted();
	
	return JR_Finished;
}

void CStreamingJob::PreLoadSync()
{
	for ( auto it = m_requests.Begin(), end = m_requests.End(); it != end; ++it )
	{
		IStreamingItem& item = *it;
		item.PreLoad();
	}
}
void CStreamingJob::ShutdownRequest()
{
	m_shutdownRequest.SetValue( true );
}

const Char* CStreamingJob::GetDebugName() const
{
	return TXT("PathLib streaming");
}

};			// namespace PathLib