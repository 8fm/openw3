/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "feedback.h"
#include "resource.h"
#include "filePath.h"
#include "redProfiler.h"

// Current profiling node we are in
RED_TLS CPerfCounter* CProfiler::st_CurNode = NULL;

// Global tick count
Uint32 CProfiler::st_TickCount = 0;

// Name of the root for current thread
const char* CProfiler::st_ThreadRootName = "ThreadRoot";

// Registered roots
TDynArray< CPerfCounter*, MC_Profiler > CProfiler::st_roots(Consts::MaxRoots);

// Registered profilers
TDynArray< CPerfCounter*, MC_Profiler > CProfiler::st_profilers(Consts::MaxProfilers);

// Current count of roots, can be only incremented
Red::Threads::CAtomic<Uint32> CProfiler::st_rootsCount;

// Current count of profilers, can be only incremented
Red::Threads::CAtomic<Uint32> CProfiler::st_profilersCount;

//////////////////////////////////////////////////////////////////////////////////////

void CPerfCounter::Reset()
{
	// Reset stats
	m_recursionCount = 0;
	m_startTime = 0;
	m_totalTime = 0;
	m_totalCount = 0;

	// Propagate to next sibling
	if ( m_next )
	{
		m_next->Reset();
	}

	// Propagate into hierarchy
	if ( m_child )
	{
		m_child->Reset();
	}
}

void CPerfCounter::CalcStats(CPerfCounter* counter, double& percentParent, double& time)
{		
	Uint64 freq;
	Red::System::Clock::GetInstance().GetTimer().GetFrequency( freq );

	Uint32 tickCount = CProfiler::GetTickCount();
	Uint64 total = 0;
	if ( counter->GetParent() )
	{
		total = counter->GetParent()->GetTotalTime();
	}
	percentParent = 100 * ( total != 0 ? (double)counter->GetTotalTime() / total : 1 );
	double div = ( tickCount !=0 ) ? freq * tickCount / 1000.0 : 1;
	time = counter->GetTotalTime() / div;
}

CPerfCounter* CProfiler::GetCounter( const char* name )
{
	Uint32 profilersCount = GetCountersCount();
	for ( Uint32 i=0; i<profilersCount; ++i)
	{
		if ( Red::System::StringCompare( name, st_profilers[i]->GetName() ) == 0 )
		{
			return st_profilers[i];
		}
	}

	return NULL;
}

void CProfiler::GetCounters( const char* name, TDynArray< CPerfCounter* >& counters, Int32 root )
{
	Uint32 profilersCount = GetCountersCount();
	for ( Uint32 i=0; i<profilersCount; ++i)
	{
		if ( Red::System::StringCompare( name, st_profilers[i]->GetName() ) == 0 )
		{
			if ( root != -1 )
			{
				CPerfCounter* p = st_profilers[i];

				while( p->GetParent() )
				{
					p = p->GetParent();
				}

				if ( p != GetThreadRoot( root ) )
				{
					continue;
				}
			}

			counters.PushBack( st_profilers[i] );
		}
	}
}

void CProfilerStatBox::Reset()
{
	m_lastTime = m_lastCount = 0;
	m_current = 0;
	Red::System::MemorySet( m_totalTime, 0, STAT_SIZE*sizeof(Uint64) );
	Red::System::MemorySet( m_totalCount, 0, STAT_SIZE*sizeof(Uint64) );
	Uint64 freq;
	Red::System::Clock::GetInstance().GetTimer().GetFrequency( freq );
	m_freq = freq/1000.0;
}

double CProfilerStatBox::GetAverageTime() const
{
	Uint64 incr = 0;
	for (int i=0;i<STAT_SIZE;++i)
	{
		incr += m_totalTime[i];
	}

	return incr/(m_freq*STAT_SIZE);
}

double CProfilerStatBox::GetMaxTime() const
{
	Uint64 maxTime = m_totalTime[0];
	for (int i=1;i<STAT_SIZE;++i)
	{
		if( m_totalTime[i] > maxTime )
		{
			maxTime = m_totalTime[i];		
		}
	}

	return maxTime / m_freq;
}

double CProfilerStatBox::GetAverageHitCount() const
{
	Uint64 incr = 0;
	for (int i=0;i<STAT_SIZE;++i)
	{
		incr += m_totalCount[i];
	}

	return (double)incr/STAT_SIZE;
}

void CProfilerStatBox::Tick()
{
	Uint64 timeIncr = m_counter->GetTotalTime() - m_lastTime;
	m_lastTime = m_counter->GetTotalTime();
	Uint64 tickIncr = m_counter->GetTotalCount() - m_lastCount;
	m_lastCount = m_counter->GetTotalCount();

	m_totalTime[m_current] = timeIncr;
	m_totalCount[m_current] = tickIncr;

	m_current=(m_current+1)%STAT_SIZE;
}

/************************************************************************/
/* Scoped performance issue checker										*/
/************************************************************************/

CPerformanceIssueCheckerScope::CPerformanceIssueCheckerScope( Float timeLimit, CObject* contextObject, const Char* category, const Char* text )
	: m_timeLimit( timeLimit )
	, m_context( contextObject )
	, m_category( category )
	, m_temporaryText( false )
	, m_text( text )
{
}

CPerformanceIssueCheckerScope::CPerformanceIssueCheckerScope( Float timeLimit, const Char* category, const Char* format, ... )
	: m_timeLimit( timeLimit )
	, m_context( NULL )
	, m_category( category )
	, m_temporaryText( false )
{
	// Allocate temporary text
	const Uint32 messageLength = 1024;
	Char* text = SLocalTextBufferManager::GetInstance().AllocateTemporaryBuffer( messageLength );
	if ( text )
	{
		// Text is owned by temporary buffer so make sure we free it
		m_temporaryText = true;

		// Format the message
		va_list arglist;
		va_start( arglist, format );
		Red::System::VSNPrintF( text, messageLength, format, arglist );

		m_text = text;
	}
}

CPerformanceIssueCheckerScope::~CPerformanceIssueCheckerScope()
{
	// Report performance issue
	const Float elapsedTime = m_timer.GetTimePeriod();
	if ( elapsedTime > m_timeLimit )
	{
		if ( m_temporaryText )
		{
			// Print crap using the formated message
			GScreenLog->PerfWarning( elapsedTime, m_category, TXT("%ls"), m_text );
		}
		else
		{
			// Use the context object for formating a message
			if ( !m_context )
			{
				// Print crap using given text
				GScreenLog->PerfWarning( elapsedTime, m_category, TXT("%ls"), m_text );
			}
			else if ( m_context->IsA< CResource >() )
			{
				// Get resource file path
				CResource* res = static_cast< CResource* >( m_context );
				CFilePath filePath( res->GetDepotPath() );

				// Print crap using the resource path
				GScreenLog->PerfWarning( elapsedTime, m_category, m_text, filePath.GetFileName().AsChar() );
			}
			else
			{
				// Print crap using the full object
				GScreenLog->PerfWarning( elapsedTime, m_category, m_text, m_context->GetFriendlyName().AsChar() );
			}
		}
	}

	// Release the temporary texture buffer if it were used
	if ( m_temporaryText )
	{
		SLocalTextBufferManager::GetInstance().DeallocateTemporaryBuffer( (Char*) m_text );
	}
}
