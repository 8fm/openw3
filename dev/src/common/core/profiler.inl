/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "redProfiler.h"

// Get named child of profiler
RED_INLINE CPerfCounter* CPerfCounter::GetChild( const char* name )
{
	CPerfCounter* node = m_child;
	while ( node )
	{
		if ( node->GetName() == name )
		{
			return node;
		}
		node = node->m_next;
	}

	node = CProfiler::CreateNode( name, this );
	return node;
}

// Enter profiling scope
RED_INLINE void CPerfCounter::Enter()
{
	++m_totalCount;
	if ( m_recursionCount++ == 0 )
	{
		Red::System::Clock::GetInstance().GetTimer().GetTicks( m_startTime );
	}
}

// Leave profiling scope
RED_INLINE bool CPerfCounter::Return()
{
	if ( m_totalCount != 0 && --m_recursionCount == 0 )
	{
		Uint64 time = Red::System::Clock::GetInstance().GetTimer().GetTicks();
		m_totalTime += time - m_startTime;
	}
	return ( m_recursionCount == 0 );
}
