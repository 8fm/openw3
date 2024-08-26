/**
* Copyright (c) 2018 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once

#include "os.h"
#include "types.h"

extern Red::System::Uint64 red_rdtsc();
extern Red::System::Uint64 red_rdtsc_freq();

namespace Red
{
	namespace System
	{
		class Timer
		{
		public:
			Timer()
				: m_startSecondsNegated( 0.0 )
			{
				// There isn't a reliable user-space way of getting this
				m_ticksPerSecond = red_rdtsc_freq();
				m_secondsPerTick = 1.0 / m_ticksPerSecond;

				Reset();
			}

			RED_FORCE_INLINE static Uint64 GetTicks()
			{
				return red_rdtsc();
			}

			RED_FORCE_INLINE static void GetTicks( Uint64& time )
			{
				time = GetTicks();
			}

			RED_FORCE_INLINE Double GetFrequency() const
			{
				return static_cast< Double >( m_ticksPerSecond );
			}

			RED_FORCE_INLINE void GetFrequency( Uint64& freq ) const
			{
				freq = m_ticksPerSecond;
			}

			RED_INLINE Double GetSeconds() const
			{
				return ( GetTicks() * m_secondsPerTick ) + m_startSecondsNegated;
			}

			RED_FORCE_INLINE void GetSeconds( Double& seconds ) const
			{
				seconds = GetSeconds();
			}

			RED_FORCE_INLINE void Reset()
			{
				m_startSecondsNegated = 0;
				m_startSecondsNegated = -GetSeconds();
			}

		private:

			Uint64 m_ticksPerSecond;
			Double m_secondsPerTick;
			Double m_startSecondsNegated;
		};
	}
}
