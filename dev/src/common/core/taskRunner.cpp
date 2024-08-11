/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "task.h"
#include "taskDispatcher.h"
#include "taskRunner.h"
#include "profiler.h"

#ifdef USE_PIX
# include <pix.h>
#endif

#ifndef FINAL
void (* volatile onTaskStartedProcessing)( const CTask* ) = nullptr;
void (* volatile onTaskFinishedProcessing)( const CTask* ) = nullptr;
#endif

void CTaskRunner::RunTask( CTask& task, CTaskDispatcher& taskDispatcher )
{
	if ( ! task.MarkRunning() )
	{
		return;
	}

	task.m_taskDispatcher = &taskDispatcher;

#ifndef FINAL
	if( onTaskStartedProcessing ) onTaskStartedProcessing( &task );
#endif


	// The converted string needs to live past PC_SCOPE_PIX_NAMED
	// so don't just do PC_SCOPE_PIX_NAMED( UNICODE_TO_ANSI( task.GetDebugName() ) )
	// because then the temp string becomes garbage.
#if !defined( NO_DEBUG_PAGES ) && defined( USE_PIX )
	struct SScopedPixMarker
	{
		SScopedPixMarker( const Char* eventName, DWORD color = PIX_COLOR( 0, 0, 0 ) )
		{
# ifndef NO_PERFCOUNTERS
			PIXBeginEvent( color, eventName );
# endif
		}

		~SScopedPixMarker()
		{
# ifndef NO_PERFCOUNTERS
			PIXEndEvent();
# endif
		}
	};
#endif // !defined( NO_DEBUG_PAGES ) && defined( USE_PIX )

	{
#if !defined( NO_DEBUG_PAGES ) && defined( USE_PIX )
	Uint32 color = task.GetDebugColor();
#ifndef RED_PLATFORM_ORBIS
		color = _byteswap_ulong( color );
#endif
	SScopedPixMarker scopedPixMarker( task.GetDebugName(), color );
#else
	PC_SCOPE_PIX( RunTask );
#endif // !defined( NO_DEBUG_PAGES ) && defined( USE_PIX )

#ifdef USE_CONCURRENCY_VISUALIZER
		//TBD: Maybe use category for filters
# ifndef NO_DEBUG_PAGES
		Concurrency::diagnostic::span spanRunTask( CVThreadMarkerSeries, TXT("Run task '%ls' [%p]"), task.GetDebugName(), &task );
#else
		Concurrency::diagnostic::span spanRunTask( CVThreadMarkerSeries, TXT("Run task [%p]"), &task );
# endif
#endif

		task.Run();
	}

	task.MarkFinished();

#ifndef FINAL
	if( onTaskFinishedProcessing ) onTaskFinishedProcessing( &task );
#endif

	task.m_taskDispatcher = nullptr;

	// Leave it to the caller to call release on the run task
}
