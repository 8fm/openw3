/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "loadingJob.h"

///////////////////////////////////////////////////////////////

ILoadJob::ILoadJob( EJobPriority priority, const Bool blockGC /*= false*/ )
	: m_state( ( Uint8 ) JOB_Initialized )
	, m_isCanceled( false )
	, m_isGCBlocker( blockGC )
	, m_refCount( 1 )
	, m_priority( ( Uint8 ) priority )
{
}

ILoadJob::~ILoadJob()
{
	RED_FATAL_ASSERT( m_state.GetValue() != JOB_Pending, "Trying to destroying a job while it's on the pending job queue. Crash will occur." );
}

void ILoadJob::AddRef()
{
	ASSERT( m_refCount.GetValue() > 0 );
	m_refCount.Increment();
}

void ILoadJob::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}

void ILoadJob::Cancel()
{
	m_isCanceled = true;
}

Bool ILoadJob::HasFinishedWithoutErrors() const
{
	const EJobState state = (EJobState) m_state.GetValue();
	return ( state == JOB_Finished );
}

Bool ILoadJob::HasEnded() const
{
	const EJobState state = (EJobState) m_state.GetValue();
	return ( state == JOB_Finished ) || ( state == JOB_Failed ) || ( state == JOB_Canceled );
}

void ILoadJob::ReportIssued()
{
	RED_FATAL_ASSERT( m_state.GetValue() == JOB_Initialized, "Failure of the internal job state." );
	m_state.SetValue( JOB_Pending );
}

void ILoadJob::ReportStarted()
{
	RED_FATAL_ASSERT( m_state.GetValue() == JOB_Pending, "Failure of the internal job state." );
	m_state.SetValue( JOB_Processing );
}

void ILoadJob::ReportFinished( const EJobResult result )
{
	RED_FATAL_ASSERT( m_state.GetValue() == JOB_Processing, "Failure of the internal job state." );

	if ( m_isCanceled )
	{
		m_state.SetValue( JOB_Canceled );
	}
	else if ( result == JR_Finished )
	{
		m_state.SetValue( JOB_Finished );
	}
	else
	{
		m_state.SetValue( JOB_Failed );
	}
}

void ILoadJob::ReportCanceled()
{
	m_state.SetValue( JOB_Canceled );
}


void* ILoadJob::operator new( size_t size )			
{ 
	return RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_Engine, size );
}		

void ILoadJob::operator delete( void* ptr ) 
{ 
	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_Engine, ptr );
}	


///////////////////////////////////////////////////////////////
