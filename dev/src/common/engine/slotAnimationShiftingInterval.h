/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

struct CSlotAnimationShiftingInterval
{
private:
	Vector	m_shift;

	Float	m_startTime;
	Float	m_stopTime;	
	Float	m_totalTimeInv;

public:
	RED_INLINE Float          GetStartTime() const { return m_startTime;  }
	RED_INLINE Float          GetStopTime()  const { return m_stopTime;   }
	RED_INLINE const Vector & GetShift()     const { return m_shift; }
	RED_INLINE void  SetShift( const Vector & shift ) { m_shift = shift; }

	RED_INLINE Bool IsValid() const { return m_startTime <= m_stopTime; }

	//! Create invalid shift
	CSlotAnimationShiftingInterval() : m_startTime(0.f), m_stopTime(-1.f), m_shift(Vector::ZEROS), m_totalTimeInv(0.f) {}

	//! Create valid shift
	CSlotAnimationShiftingInterval( Float startTime, Float stopTime, const Vector & shift )
		: m_shift(shift), m_startTime(startTime), m_stopTime(stopTime)
	{
		Float totalTime = stopTime - startTime;
		RED_ASSERT ( totalTime >= 0.f );
		m_totalTimeInv = ( totalTime <= 0.f ) ? 0.f : 1.f / totalTime;
	}

	Vector GetShiftForInterval( Float prevTime, Float currTime );

	Bool operator<( const CSlotAnimationShiftingInterval& rhs ) const 
	{
		return m_startTime < rhs.m_startTime;
	}
};
