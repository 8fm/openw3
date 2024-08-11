/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

//////////////////////////////////////////////////////////////////////////

CTrackerArray* GTrackerArray = NULL;

void* PolicyMemoryAllocate( size_t size, size_t align )
{
	return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Singleton, size, align );
}

void PolicyMemoryFree( void* mem )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Singleton, mem );
}

void AtExitFn()
{
	ASSERT(GTrackerArray->m_trackers && GTrackerArray->m_elements > 0);

	CLifetimeTracker* tracker = GTrackerArray->m_trackers[GTrackerArray->m_elements - 1];

	GTrackerArray->m_elements --;

	tracker->~CLifetimeTracker();

	PolicyMemoryFree(tracker);

	if (GTrackerArray->m_elements == 0)
	{
		GTrackerArray->~CTrackerArray();

		PolicyMemoryFree(GTrackerArray);

		GTrackerArray = NULL;
	}
}
