/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_CLOCK_H_
#define _RED_CLOCK_H_

#include "types.h"
#include "timer.h"

// This packs the date and time into a single 64 bit integer

#define RED_DATETIME_SET( item, shift, value )	item |= ( value << shift )
#define RED_DATETIME_GET( item, shift, mask )	( item & mask ) >> shift

#define YEAR_MASK32			0xFFF00000
#define YEAR_MASK64			0xFFF0000000000000ull
#define YEAR_SHIFT32		20
#define YEAR_SHIFT64		( YEAR_SHIFT32 + 32 )

#define MONTH_MASK32		0x000F8000
#define MONTH_MASK64		0x000F800000000000ull
#define MONTH_SHIFT32		15
#define MONTH_SHIFT64		( MONTH_SHIFT32 + 32 )

#define DAY_MASK32			0x00007C00
#define DAY_MASK64			0x00007C0000000000ull
#define DAY_SHIFT32			10
#define DAY_SHIFT64			( DAY_SHIFT32 + 32 )

#define HOUR_MASK32			0x07C00000
#define HOUR_MASK64			0x0000000007C00000ull
#define HOUR_SHIFT32		22
#define HOUR_SHIFT64		HOUR_SHIFT32

#define MINUTE_MASK32		0x03F0000
#define MINUTE_MASK64		0x0000000003F0000ull
#define MINUTE_SHIFT32		16
#define MINUTE_SHIFT64		MINUTE_SHIFT32

#define SECOND_MASK32		0x0000FC00
#define SECOND_MASK64		0x000000000000FC00ull
#define SECOND_SHIFT32		10
#define SECOND_SHIFT64		SECOND_SHIFT32

#define MILLISECOND_MASK32	0x000003FF
#define MILLISECOND_MASK64	0x00000000000003FFull
#define MILLISECOND_SHIFT32	0
#define MILLISECOND_SHIFT64	MILLISECOND_SHIFT32

namespace Red
{
	namespace System
	{
		class DateTime
		{
		protected:
			Uint32 m_date;
			Uint32 m_time;

		public:
			RED_INLINE DateTime()
			:	m_date( 0 )
			,	m_time( 0 )
			{
			}

			RED_INLINE DateTime( const DateTime& other )
			:	m_date( other.m_date )
			,	m_time( other.m_time )
			{
			}

			RED_INLINE void Clear()
			{
				m_date = 0;
				m_time = 0;
			}

			RED_INLINE Bool IsValid() const { return m_date != 0u && m_time != 0; }

			// 2013 etc
			RED_INLINE void SetYear( Uint32 year ) { RED_DATETIME_SET( m_date, YEAR_SHIFT32, year ); }
			RED_INLINE Uint32 GetYear() const { return RED_DATETIME_GET( m_date, YEAR_SHIFT32, YEAR_MASK32 ); }

			// 0-11 / Jan-Dec
			RED_INLINE void SetMonth( Uint32 month ) { RED_DATETIME_SET( m_date, MONTH_SHIFT32, month ); }
			RED_INLINE Uint32 GetMonth() const { return RED_DATETIME_GET( m_date, MONTH_SHIFT32, MONTH_MASK32 ); }

			// 0-30 "1-31"
			RED_INLINE void SetDay( Uint32 day ) { RED_DATETIME_SET( m_date, DAY_SHIFT32, day ); }
			RED_INLINE Uint32 GetDay() const { return RED_DATETIME_GET( m_date, DAY_SHIFT32, DAY_MASK32 ); }

			// 0-23
			RED_INLINE void SetHour( Uint32 hour ) { RED_DATETIME_SET( m_time, HOUR_SHIFT32, hour ); }
			RED_INLINE Uint32 GetHour() const { return RED_DATETIME_GET( m_time, HOUR_SHIFT32, HOUR_MASK32 ); }

			// 0-59
			RED_INLINE void SetMinute( Uint32 minute ) { RED_DATETIME_SET( m_time, MINUTE_SHIFT32, minute ); }
			RED_INLINE Uint32 GetMinute() const { return RED_DATETIME_GET( m_time, MINUTE_SHIFT32, MINUTE_MASK32 ); }

			// 0-59
			RED_INLINE void SetSecond( Uint32 second ) { RED_DATETIME_SET( m_time, SECOND_SHIFT32, second ); }
			RED_INLINE Uint32 GetSecond() const { return RED_DATETIME_GET( m_time, SECOND_SHIFT32, SECOND_MASK32 ); }

			// 0-999
			RED_INLINE void SetMilliSeconds( Uint32 milliseconds ) { RED_DATETIME_SET( m_time, MILLISECOND_SHIFT32, milliseconds ); }
			RED_INLINE Uint32 GetMilliSeconds() const { return RED_DATETIME_GET( m_time, MILLISECOND_SHIFT32, MILLISECOND_MASK32 ); }

			// Comparison operators
			RED_INLINE Bool operator<( const DateTime& other ) const	{ return GetRaw() <  other.GetRaw(); }
			RED_INLINE Bool operator<=( const DateTime& other ) const	{ return GetRaw() <= other.GetRaw(); }
			RED_INLINE Bool operator>( const DateTime& other ) const	{ return GetRaw() >  other.GetRaw(); }
			RED_INLINE Bool operator>=( const DateTime& other ) const	{ return GetRaw() >= other.GetRaw(); }
			RED_INLINE Bool operator==( const DateTime& other ) const	{ return GetRaw() == other.GetRaw(); }
			RED_INLINE Bool operator!=( const DateTime& other ) const	{ return GetRaw() != other.GetRaw(); }

			// Assignment operator
			RED_INLINE void operator=( const DateTime& other );

			// The raw packed bits
			void SetRaw( Uint64 dateTime );
			RED_INLINE Uint64 GetRaw() const { return ( static_cast< Uint64 >( m_date ) << 32 ) | m_time; }

			RED_INLINE void SetDateRaw( Uint32 date ) { m_date = date; }
			RED_INLINE Uint32 GetDateRaw() const { return m_date; }

			RED_INLINE void SetTimeRaw( Uint32 time ) { m_time = time; }
			RED_INLINE Uint32 GetTimeRaw() const { return m_time; }
		};

		RED_INLINE void Red::System::DateTime::operator=( const DateTime& other )
		{
			m_date = other.m_date;
			m_time = other.m_time;
		}

		class Clock
		{
		public:
			Clock();
			~Clock();

			// Date and time that the system we're running on is set to
			void GetLocalTime( DateTime& dt ) const;

			// Date and time in UTC + 0
			void GetUTCTime( DateTime& dt ) const;

			RED_INLINE Timer& GetTimer() { return m_timer; }
			RED_INLINE const Timer& GetTimer() const { return m_timer; }

			RED_INLINE static Clock& GetInstance()
			{
				static Clock instance;
				return instance;
			}

		private:
			Timer m_timer;
		};
	}
}

#endif //_RED_CLOCK_H_
