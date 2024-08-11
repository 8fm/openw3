/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "os.h"
#include "types.h"

#ifndef RED_FINAL_BUILD
#define CONTINUOUS_SCREENSHOT_HACK
#endif

namespace Red
{
	namespace System
	{
		class Timer
		{
		public:
			Timer()
			#ifdef CONTINUOUS_SCREENSHOT_HACK
				: IsEnabledTimeHack( false )
				, TimeHackBaseTime( 0.0 )
				, TimeHackCorrection( 0.0 )
				, ScreenshotFramerate( 30.0f )
			#endif
			{
				LARGE_INTEGER frequency;
				::QueryPerformanceFrequency( &frequency );

				m_ticksPerSecond = frequency.QuadPart;
				m_secondsPerTick = 1.0 / m_ticksPerSecond;

				Reset();
			}

			RED_FORCE_INLINE Double GetFrequency() const
			{
				return static_cast< Double >( m_ticksPerSecond );
			}

			RED_FORCE_INLINE void GetFrequency( Uint64& freq ) const
			{
				freq = m_ticksPerSecond;
			}

			RED_FORCE_INLINE Uint64 GetTicks() const
			{
#ifdef RED_PLATFORM_DURANGO
				return __rdtsc();
#else
				LARGE_INTEGER counter;
				::QueryPerformanceCounter(&counter);

				return counter.QuadPart;
#endif
			}

			RED_FORCE_INLINE void GetTicks( Uint64& time ) const
			{
				time = GetTicks();
			}

			RED_INLINE Double GetSeconds() const
			{
				return ( GetTicks() * m_secondsPerTick ) + m_startSecondsNegated;
			}

			RED_FORCE_INLINE void GetSeconds( Double& seconds ) const
			{
				seconds = GetSeconds();
			}

			RED_INLINE void Reset()
			{
				m_startSecondsNegated = 0;
				m_startSecondsNegated = -GetSeconds();
			}

		private:
			mutable LARGE_INTEGER	m_startTime;
			Uint64					m_ticksPerSecond;
			Double					m_secondsPerTick;
			Double					m_startSecondsNegated;

#ifdef CONTINUOUS_SCREENSHOT_HACK
			Bool IsEnabledTimeHack;
			Double TimeHackBaseTime;
			Double TimeHackCorrection;
			Double ScreenshotFramerate;

		public:
			RED_FORCE_INLINE Bool IsTimeHackEnabled() const { return IsEnabledTimeHack; }
			Double GetTimeHackSeconds() const;
			void EnableGameTimeHack();
			void DisableGameTimeHack();
			Double NextFrameGameTimeHack();
			void SetScreenshotFramerate( Double framerate );
#endif
		};
	}
}
