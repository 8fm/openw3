/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_UTILITY_TIMER_H_
#define _RED_UTILITY_TIMER_H_

#include "utility.h"
#include "clock.h"

namespace Red
{
	namespace System
	{
		//////////////////////////////////////////////////////////////////////////
		// Stop Clock
		class StopClock : public NonCopyable 
		{
		public:
			RED_INLINE StopClock()
			{
				Reset();
			}

			RED_INLINE void Reset()
			{
				m_start = Clock::GetInstance().GetTimer().GetSeconds();
			}

			RED_INLINE Double GetDelta() const
			{
				return Clock::GetInstance().GetTimer().GetSeconds() - m_start;
			}

		protected:
			Double m_start;
		};

		//////////////////////////////////////////////////////////////////////////
		// Scoped Stop Clock
		class ScopedStopClock : public StopClock
		{
		public:
			RED_INLINE ScopedStopClock( Double& result )
				: m_result( result )
			{
			}

			RED_INLINE ~ScopedStopClock()
			{
				m_result = GetDelta();
			}

		private:
			Double& m_result;
		};
	}
}

#endif //_RED_UTILITY_TIMER_H_
