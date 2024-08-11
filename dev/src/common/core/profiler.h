/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

/*
	How to use this thread-safe lightweight profiling system.

	1. Remember that although lightweight, profiler is not cost-free. Try to profile one thing at once not all.
	3. PerfCounters can be nested providing a stack trace profiling
	4. PerfCounters can have their own grouping for better visual feedback.

	Profile block looks like this

	{
		PC_SCOPE( MyBlock )
		... code ...
	}

	In order to count frame ticks somewhere in code there should be a line PC_TICK()
	Dynamic profilers are currently not supported but this is an option. They are not supported because
	name is a static literal and name comparisons are based on pointers (means very fast). Dynamic names are a little more tedious though.

	For all you guys and gels that would like to "enhance" profililng system. This is a bare minimum to profile thus lightweight. Adding any other "smart" logic
	will downgrade performance and could potentially provide non-thread-safe system (deadlocks etc.)

	Any "smart" logic is preferably contained in wrapper classes like CProfilerStatBox, that gathers samples, counts average of last STAT_SIZE samples.
	Any hi-level logic is done elsewhere (on debugPages) so when off id doesn't add overhead. Consult profile debug pages to see how this works.

*/

#pragma once

#include "profilerConfiguration.h"

#ifdef USE_CONCURRENCY_VISUALIZER
# include "../../../external/ConcurrencyVisualizer/SDK/Native/Inc/cvmarkersobj.h"
extern Concurrency::diagnostic::marker_series CVThreadMarkerSeries;
#endif	

///////////////////////////////////////////////////////////

#include "profilerChannels.h"
#include "profilerManager.h"
#include "profiler/newRedProfiler.h"		// Used in function.h
#include "profiler.inl"

///////////////////////////////////////////////////////////

#if !defined(NO_PERFCOUNTERS) && defined(USE_PROFILER)

// FIX: add ##name to __handle_, but this means that PC_SCOPEs need some fixes

#define PC_SCOPE_LVL(name, level, channel)	static CProfilerHandle* __handle_ = UnifiedProfilerManager::GetInstance().RegisterHandler( #name, level, channel );	\
											CProfilerBlock __profilerBlock_( __handle_ );
#define PC_SCOPE(name)						PC_SCOPE_LVL(name, 2, PBC_CPU)
#define PC_SCOPE_PIX(name)					PC_SCOPE_LVL(name, 1, PBC_CPU)
#define PC_SCOPE_PIX_NAMED( name )			PC_SCOPE_LVL(name, 1, PBC_CPU)
#define PC_SIGNAL( handler, value )			UnifiedProfilerManager::GetInstance().Message( handler, value );
#define PC_SIGNAL_WITH_NAME( name, value, level, channel )  static CProfilerHandle* __handle_##name = UnifiedProfilerManager::GetInstance().RegisterHandler( #name, level, channel );	\
											CProfilerBlock __profilerBlock_##name( __handle_##name );	\
											PC_SIGNAL( &__profilerBlock_##name, value );

#define PC_SCOPE_LV0( name, channel )		PC_SCOPE_LVL(name, 0, channel)
#define PC_SCOPE_LV1( name, channel )		PC_SCOPE_LVL(name, 1, channel)
#define PC_SCOPE_LV2( name, channel )		PC_SCOPE_LVL(name, 2, channel)
#define PC_SCOPE_LV3( name, channel )		PC_SCOPE_LVL(name, 3, channel)
#define PC_SCOPE_LV4( name, channel )		PC_SCOPE_LVL(name, 4, channel)

#define PC_CHECKER_SCOPE( timeLimit, category, text, ... ) CPerformanceIssueCheckerScope perfIssue( timeLimit, category, text, ##__VA_ARGS__ );

#else	// NO_PERFCOUNTERS

#define PC_SCOPE_LVL(name, level, channel)
#define PC_SCOPE(name)
#define PC_SCOPE_PIX(name)
#define PC_SCOPE_PIX_NAMED( name )
#define PC_SIGNAL( handler, value )
#define PC_SIGNAL_WITH_NAME( name, value, level, channel )

#define PC_SCOPE_LV0( name, channel )
#define PC_SCOPE_LV1( name, channel )
#define PC_SCOPE_LV2( name, channel )
#define PC_SCOPE_LV3( name, channel )
#define PC_SCOPE_LV4( name, channel )

#define PC_CHECKER_SCOPE( timeLimit, category, text, ... )

#endif	// NO_PERFCOUNTERS

#ifdef USE_PROFILER
#	define RED_PROFILING_TIMER( timerIdentifier ) Red::System::StopClock timerIdentifier
#	define RED_PROFILING_TIMER_GET_DELTA( destination, timerIdentifier ) destination = timerIdentifier.GetDelta()

#	define RED_PROFILING_TIMER_SCOPED( value ) Red::System::ScopedStopClock scopedTimer( value )

#	ifndef NO_LOG
#		define RED_PROFILING_TIMER_LOG( ... )	LOG_CORE( __VA_ARGS__ )
#	else
#		define RED_PROFILING_TIMER_LOG( ... )
#	endif
#else
#	define RED_PROFILING_TIMER( identifier )
#	define RED_PROFILING_TIMER_GET_DELTA( destination, timerIdentifier )

#	define RED_PROFILING_TIMER_LOG( ... )
#endif