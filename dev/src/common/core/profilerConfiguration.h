/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#define RED_BUSY_WAIT() do {}while(0)

#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
#define USE_PROFILER
#endif

#ifdef USE_PROFILER
//#define PROFILE_BUSY_WAIT

# ifdef PROFILE_BUSY_WAIT
#  ifdef RED_PLATFORM_ORBIS
#  undef RED_BUSY_WAIT
#    define RED_BUSY_WAIT() do { ::scePthreadYield(); }while(0)
#  endif
# endif

/***** Profiler defines - platform dependant *****/

#if defined(RED_PLATFORM_WINPC)

#define USE_RED_PROFILER
#define USE_NEW_RED_PROFILER
#define USE_INTEL_ITT
#define USE_NVIDIA_TOOLS
#define USE_RAD_TELEMETRY_PROFILER
//#define USE_CONCURRENCY_VISUALIZER

#elif defined(RED_PLATFORM_DURANGO)

#define USE_RED_PROFILER
#define USE_NEW_RED_PROFILER
#define USE_PIX
#define USE_RAD_TELEMETRY_PROFILER

#elif defined(RED_PLATFORM_ORBIS)

#define USE_RED_PROFILER
#define USE_NEW_RED_PROFILER
#define USE_RAZOR_PROFILER
//#define USE_RAD_TELEMETRY_PROFILER

#endif

/********** Configuration per profiler ***********/

#ifdef USE_NEW_RED_PROFILER

#define NEW_PROFILER_ENABLED

#endif

/*************************************************/

#endif // USE_PROFILER
