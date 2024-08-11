/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "clock.h"

#if defined( RED_PLATFORM_ORBIS )
#	include "rtc.h"
#	pragma comment ( lib, "libSceRtc_stub_weak.a" )
#endif

void Red::System::DateTime::SetRaw( Uint64 dateTime )
{
	m_date = static_cast< Uint32 >( dateTime >> 32 ); m_time = static_cast< Uint32 >( dateTime );
}

Red::System::Clock::Clock()
{

}

Red::System::Clock::~Clock()
{

}

void Red::System::Clock::GetLocalTime( DateTime& dt ) const
{
	// If this variable is being reused or is non-zero then weirdness will ensue!
	dt.Clear();

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	SYSTEMTIME lt;

#	if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	::GetLocalTime( &lt );
#	elif defined( RED_PLATFORM_DURANGO )
	FILETIME sft, lft;
	::GetSystemTimeAsFileTime( &sft );
	::FileTimeToLocalFileTime( &sft, &lft );
	::FileTimeToSystemTime( &lft, &lt );
#	endif

	dt.SetYear( static_cast< Uint32 >( lt.wYear ) );
	dt.SetMonth( static_cast< Uint32 >( lt.wMonth ) - 1 );
	dt.SetDay( static_cast< Uint32 >( lt.wDay ) - 1 );
	dt.SetHour( static_cast< Uint32 >( lt.wHour ) );
	dt.SetMinute( static_cast< Uint32 >( lt.wMinute ) );
	dt.SetSecond( static_cast< Uint32 >( lt.wSecond ) );
	dt.SetMilliSeconds( static_cast< Uint32 >( lt.wMilliseconds ) );
#elif defined( RED_PLATFORM_ORBIS )
	SceRtcDateTime lt;
	sceRtcGetCurrentClockLocalTime( &lt );

	dt.SetYear( static_cast< Uint32 >( lt.year ) );
	dt.SetMonth( static_cast< Uint32 >( lt.month ) - 1 );
	dt.SetDay( static_cast< Uint32 >( lt.day ) - 1 );
	dt.SetHour( static_cast< Uint32 >( lt.hour ) );
	dt.SetMinute( static_cast< Uint32 >( lt.minute ) );
	dt.SetSecond( static_cast< Uint32 >( lt.second ) );
	dt.SetMilliSeconds( static_cast< Uint32 >( lt.microsecond ) / 100 );
#else
	RED_UNUSED( dt );
#endif
}

void Red::System::Clock::GetUTCTime( DateTime& dt ) const
{
	// If this variable is being reused or is non-zero then weirdness will ensue!
	dt.Clear();

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	SYSTEMTIME st;
	::GetSystemTime( &st );

	dt.SetYear( static_cast< Uint32 >( st.wYear ) );
	dt.SetMonth( static_cast< Uint32 >( st.wMonth ) - 1 );
	dt.SetDay( static_cast< Uint32 >( st.wDay ) - 1 );
	dt.SetHour( static_cast< Uint32 >( st.wHour ) );
	dt.SetMinute( static_cast< Uint32 >( st.wMinute ) );
	dt.SetSecond( static_cast< Uint32 >( st.wSecond ) );
	dt.SetMilliSeconds( static_cast< Uint32 >( st.wMilliseconds ) );
#elif defined( RED_PLATFORM_ORBIS )
	SceRtcDateTime st;
	sceRtcGetCurrentClock( &st, 0 );

	dt.SetYear( static_cast< Uint32 >( st.year ) );
	dt.SetMonth( static_cast< Uint32 >( st.month ) - 1 );
	dt.SetDay( static_cast< Uint32 >( st.day ) - 1 );
	dt.SetHour( static_cast< Uint32 >( st.hour ) );
	dt.SetMinute( static_cast< Uint32 >( st.minute ) );
	dt.SetSecond( static_cast< Uint32 >( st.second ) );
	dt.SetMilliSeconds( static_cast< Uint32 >( st.microsecond ) / 100 );
#else
	RED_UNUSED( dt );
#endif
}
