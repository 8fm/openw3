/**
* Copyright (c) 2018 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "timerLinux.h"

#include <errno.h>

using namespace Red::System;

static const Uint64 NANOSEC_PER_SEC = 1000000000L;

Uint64 red_rdtsc()
{
	Uint32 lo;
	Uint32 hi;
	asm volatile( "rdtsc": "=a" ( lo ), "=d" ( hi ) );
	return ( ( ( Uint64 )hi ) << 32 ) | ( Uint64 )lo;
}

static void red_diffTime( const struct timespec& a, const struct timespec& b, struct timespec* c )
{
	*c = a;

	if ( c->tv_nsec < b.tv_nsec )
	{
		c->tv_sec -= 1;
		c->tv_nsec += NANOSEC_PER_SEC;
	}

	c->tv_sec -= b.tv_sec;
	c->tv_nsec -= b.tv_nsec;
}

// relies on constant_tsc/nonstop_tsc
// lame calibration. Should really replace all this with EngineTime properly
Uint64 red_cpufreq = 0;
static void red_freq_init()
{
	struct timespec startTime;
	if ( clock_gettime( CLOCK_MONOTONIC_RAW, &startTime ) < 0 )
	{
		const Int32 err = errno;
		//RED_FATAL("Failed to call CLOCK_MONOTONIC_RAW errno=%d", err);
		red_cpufreq = NANOSEC_PER_SEC;
		return;
	}

	const Uint64 startTick = red_rdtsc();

	struct timespec sleepTime;
	sleepTime.tv_sec = 0;
	sleepTime.tv_nsec = NANOSEC_PER_SEC / 2;
	nanosleep( &sleepTime, nullptr );

	struct timespec endTime;
	clock_gettime( CLOCK_MONOTONIC_RAW, &endTime );
	const Uint64 endTick = red_rdtsc();

	struct timespec diffTime;
	red_diffTime( endTime, startTime, &diffTime );
	const Uint64 nsec = diffTime.tv_sec * NANOSEC_PER_SEC + diffTime.tv_nsec;
	const Double sec = Double( nsec ) / NANOSEC_PER_SEC;
	red_cpufreq = Uint64( endTick - startTick ) / sec;
}

Uint64 red_rdtsc_freq()
{
	// hacky, but being hit by static init issues otherwise
	static bool DoInit = true;
	if ( DoInit )
	{
		DoInit = false;
		red_freq_init();
	}

	return red_cpufreq;
}
