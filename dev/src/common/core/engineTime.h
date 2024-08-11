/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef RED_PLATFORM_ORBIS
# pragma comment( lib, "libSceRtc_stub_weak.a" )
#include <rtc.h>
#endif

#include "classBuilder.h"

// High precision engine time, floating point values treated as seconds
struct EngineTime
{
	DECLARE_RTTI_STRUCT( EngineTime )

private:
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	LARGE_INTEGER			m_time;
	static LARGE_INTEGER	s_timerFreq;
#elif defined( RED_PLATFORM_ORBIS )
	Int64					m_time;
	static SceInt32			s_timerFreq;
#endif
	static Double			s_timerFreqDbl;

public:
	static const EngineTime ZERO;

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	RED_INLINE Bool IsValid() const { return m_time.QuadPart != 0; }

	RED_INLINE EngineTime() { m_time.QuadPart = 0; }
	RED_INLINE EngineTime( const EngineTime &val ) { m_time = val.m_time; }
	RED_INLINE EngineTime( const LONGLONG &val ) { m_time.QuadPart = val; }
	RED_INLINE EngineTime( Float val ) { m_time.QuadPart = LONGLONG( val * s_timerFreqDbl ); }
	RED_INLINE EngineTime( Double val ) { m_time.QuadPart = LONGLONG( val * s_timerFreqDbl ); }

	RED_INLINE EngineTime& operator = ( const EngineTime &val ) { m_time.QuadPart = val.m_time.QuadPart; return *this; }

	RED_INLINE EngineTime operator - ( const EngineTime& val ) const { return EngineTime( m_time.QuadPart - val.m_time.QuadPart ); }
	RED_INLINE EngineTime operator + ( const EngineTime& val ) const { return EngineTime( m_time.QuadPart + val.m_time.QuadPart ); }
	RED_INLINE EngineTime operator % ( const EngineTime& val ) const { return EngineTime( m_time.QuadPart % val.m_time.QuadPart ); }
	RED_INLINE EngineTime& operator -= ( const EngineTime& val ) { m_time.QuadPart -= val.m_time.QuadPart; return *this; }
	RED_INLINE EngineTime& operator += ( const EngineTime& val ) { m_time.QuadPart += val.m_time.QuadPart; return *this; }

	RED_INLINE bool operator == ( const EngineTime &val ) const { return m_time.QuadPart == val.m_time.QuadPart; }
	RED_INLINE bool operator != ( const EngineTime &val ) const { return m_time.QuadPart != val.m_time.QuadPart; }
	RED_INLINE bool operator >  ( const EngineTime &val ) const { return m_time.QuadPart > val.m_time.QuadPart; }
	RED_INLINE bool operator >= ( const EngineTime &val ) const { return m_time.QuadPart >= val.m_time.QuadPart; }
	RED_INLINE bool operator <  ( const EngineTime &val ) const { return m_time.QuadPart < val.m_time.QuadPart; }
	RED_INLINE bool operator <= ( const EngineTime &val ) const { return m_time.QuadPart <= val.m_time.QuadPart; }

	RED_INLINE bool operator >  ( Float val) const { return m_time.QuadPart >  LONGLONG( val * s_timerFreqDbl ); }
	RED_INLINE bool operator >= ( Float val) const { return m_time.QuadPart >= LONGLONG( val * s_timerFreqDbl ); }	
	RED_INLINE bool operator <  ( Float val) const { return m_time.QuadPart <  LONGLONG( val * s_timerFreqDbl ); }
	RED_INLINE bool operator <= ( Float val) const { return m_time.QuadPart <= LONGLONG( val * s_timerFreqDbl ); }

	RED_INLINE EngineTime operator - ( Float val ) const { return EngineTime( m_time.QuadPart - LONGLONG( val * s_timerFreqDbl ) ); }
	RED_INLINE EngineTime operator + ( Float val ) const { return EngineTime( m_time.QuadPart + LONGLONG( val * s_timerFreqDbl ) ); }
	RED_INLINE EngineTime& operator -= ( Float val ) { m_time.QuadPart -= LONGLONG( val * s_timerFreqDbl ); return *this; }
	RED_INLINE EngineTime& operator += ( Float val ) { m_time.QuadPart += LONGLONG( val * s_timerFreqDbl ); return *this; }

	RED_INLINE EngineTime operator - ( Int32 val ) const { return EngineTime( m_time.QuadPart - LONGLONG( val * s_timerFreqDbl ) ); }
	RED_INLINE EngineTime operator + ( Int32 val ) const { return EngineTime( m_time.QuadPart + LONGLONG( val * s_timerFreqDbl ) ); }
	RED_INLINE EngineTime& operator -= ( Int32 val ) { m_time.QuadPart -= LONGLONG( val * s_timerFreqDbl ); return *this; }
	RED_INLINE EngineTime& operator += ( Int32 val ) { m_time.QuadPart += LONGLONG( val * s_timerFreqDbl ); return *this; }

	RED_INLINE EngineTime operator * (Float val) const { return EngineTime( LONGLONG( Double( m_time.QuadPart ) * val ) ); }	
	RED_INLINE EngineTime operator / (Float val) const { return EngineTime( LONGLONG( Double( m_time.QuadPart ) / val ) ); }
	RED_INLINE EngineTime operator % (Float val) const { return EngineTime( m_time.QuadPart % LONGLONG( val * s_timerFreqDbl ) ); }
	RED_INLINE EngineTime& operator *= (Float val) { m_time.QuadPart = LONGLONG(Double( m_time.QuadPart ) * val ); return *this; }
	RED_INLINE EngineTime& operator /= (Float val) { m_time.QuadPart = LONGLONG(Double( m_time.QuadPart ) / val ); return *this; }

	RED_INLINE operator Float() const { return Float( Double( m_time.QuadPart ) / s_timerFreqDbl ); }
	RED_INLINE operator Double() const { return Double( m_time.QuadPart ) / s_timerFreqDbl; }
#elif defined( RED_PLATFORM_ORBIS )
	RED_INLINE Bool IsValid() const { return m_time != 0; }

	RED_INLINE EngineTime() { m_time = 0; }
	RED_INLINE EngineTime( const EngineTime &val ) { m_time = val.m_time; }
	RED_INLINE EngineTime( Int64 val ) { m_time = val; }

	RED_INLINE EngineTime( Float val ) { m_time = Int64( val * s_timerFreqDbl ); }
	RED_INLINE EngineTime( Double val ) { m_time = Int64( val * s_timerFreqDbl ); }

	RED_INLINE EngineTime& operator = ( const EngineTime &val ) { m_time = val.m_time; return *this; }

	RED_INLINE EngineTime operator - ( const EngineTime& val ) const { return EngineTime( static_cast< Int64 >( m_time - val.m_time ) ); }
	RED_INLINE EngineTime operator + ( const EngineTime& val ) const { return EngineTime( static_cast< Int64 >( m_time + val.m_time ) ); }
	RED_INLINE EngineTime operator % ( const EngineTime& val ) const { return EngineTime( static_cast< Int64 >( m_time % val.m_time ) ); }
	RED_INLINE EngineTime& operator -= ( const EngineTime& val ) { m_time-= val.m_time; return *this; }
	RED_INLINE EngineTime& operator += ( const EngineTime& val ) { m_time += val.m_time; return *this; }

	RED_INLINE bool operator == ( const EngineTime &val ) const { return m_time == val.m_time; }
	RED_INLINE bool operator != ( const EngineTime &val ) const { return m_time != val.m_time; }
	RED_INLINE bool operator >  ( const EngineTime &val ) const { return m_time > val.m_time; }
	RED_INLINE bool operator >= ( const EngineTime &val ) const { return m_time >= val.m_time; }
	RED_INLINE bool operator <  ( const EngineTime &val ) const { return m_time < val.m_time; }
	RED_INLINE bool operator <= ( const EngineTime &val ) const { return m_time <= val.m_time; }

	RED_INLINE bool operator >  ( Float val) const { return m_time >  Uint64( val * s_timerFreqDbl ); }
	RED_INLINE bool operator >= ( Float val) const { return m_time >= Uint64( val * s_timerFreqDbl ); }	
	RED_INLINE bool operator <  ( Float val) const { return m_time <  Uint64( val * s_timerFreqDbl ); }
	RED_INLINE bool operator <= ( Float val) const { return m_time <= Uint64( val * s_timerFreqDbl ); }

	RED_INLINE EngineTime operator - ( Float val ) const { return EngineTime( Int64( m_time - Int64( val * s_timerFreqDbl ) ) ); }
	RED_INLINE EngineTime operator + ( Float val ) const { return EngineTime( Int64( m_time + Int64( val * s_timerFreqDbl ) ) ); }
	RED_INLINE EngineTime& operator -= ( Float val ) { m_time -= Int64( val * s_timerFreqDbl ); return *this; }
	RED_INLINE EngineTime& operator += ( Float val ) { m_time += Int64( val * s_timerFreqDbl ); return *this; }

	RED_INLINE EngineTime operator - ( Int32 val ) const { return EngineTime( Int64( m_time - Int64( val * s_timerFreqDbl ) ) ); }
	RED_INLINE EngineTime operator + ( Int32 val ) const { return EngineTime( Int64( m_time + Int64( val * s_timerFreqDbl ) ) ); }
	RED_INLINE EngineTime& operator -= ( Int32 val ) { m_time -= Int64( val * s_timerFreqDbl ); return *this; }
	RED_INLINE EngineTime& operator += ( Int32 val ) { m_time += Int64( val * s_timerFreqDbl ); return *this; }

	RED_INLINE EngineTime operator * (Float val) const { return EngineTime( Int64( Double( m_time ) * val ) ); }	
	RED_INLINE EngineTime operator / (Float val) const { return EngineTime( Int64( Double( m_time) / val ) ); }
	RED_INLINE EngineTime operator % (Float val) const { return EngineTime( Int64( m_time % Int64( val * s_timerFreqDbl ) ) ); }
	RED_INLINE EngineTime& operator *= (Float val) { m_time = Int64(Double( m_time ) * val ); return *this; }
	RED_INLINE EngineTime& operator /= (Float val) { m_time = Int64(Double( m_time ) / val ); return *this; }

	RED_INLINE operator Float() const { return Float( Double( m_time ) / s_timerFreqDbl ); }
	RED_INLINE operator Double() const { return Double( m_time ) / s_timerFreqDbl; }
#endif

	void SetNow();

	static void Init();

	static EngineTime GetNow();
};

BEGIN_CLASS_RTTI( EngineTime );
END_CLASS_RTTI();