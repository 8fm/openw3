/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "datetime.h"

#if defined( RED_PLATFORM_ORBIS )
#	include <rtc.h>
#endif

RED_DEFINE_RTTI_NAME( CDateTime );

IMPLEMENT_RTTI_TYPE( CSimpleRTTITypeCDateTime );

CDateTime CDateTime::INVALID;

void CDateTime::ImportFromOldFileTimeFormat( Uint64 winStyleTimestamp )
{
	// FILETIME on Windows:
	// Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
	
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	FILETIME windowsFileFormat;
	windowsFileFormat.dwHighDateTime	= static_cast< DWORD >( winStyleTimestamp >> 32 );
	windowsFileFormat.dwLowDateTime		= static_cast< DWORD >( winStyleTimestamp );

	SYSTEMTIME windowsDateTimeFormat;
	Red::System::MemoryZero( &windowsDateTimeFormat, sizeof( SYSTEMTIME ) );

	FileTimeToSystemTime( &windowsFileFormat, &windowsDateTimeFormat );

	SetYear			( static_cast< Uint32 >( windowsDateTimeFormat.wYear ) );
	SetMonth		( static_cast< Uint32 >( windowsDateTimeFormat.wMonth ) - 1 );
	SetDay			( static_cast< Uint32 >( windowsDateTimeFormat.wDay ) - 1 );
	SetHour			( static_cast< Uint32 >( windowsDateTimeFormat.wHour ) );
	SetMinute		( static_cast< Uint32 >( windowsDateTimeFormat.wMinute ) );
	SetSecond		( static_cast< Uint32 >( windowsDateTimeFormat.wSecond ) );
	SetMilliSeconds	( static_cast< Uint32 >( windowsDateTimeFormat.wMilliseconds ) );

#elif defined( RED_PLATFORM_ORBIS )

	SceRtcDateTime orbisDateTime;
	if( sceRtcSetWin32FileTime( &orbisDateTime, winStyleTimestamp ) == SCE_OK )
	{
		SetYear			( static_cast< Uint32 >( orbisDateTime.year ) );
		SetMonth		( static_cast< Uint32 >( orbisDateTime.month ) - 1 );
		SetDay			( static_cast< Uint32 >( orbisDateTime.day ) - 1 );
		SetHour			( static_cast< Uint32 >( orbisDateTime.hour ) );
		SetMinute		( static_cast< Uint32 >( orbisDateTime.minute ) );
		SetSecond		( static_cast< Uint32 >( orbisDateTime.second ) );
		SetMilliSeconds	( static_cast< Uint32 >( orbisDateTime.microsecond ) / 100 );
	}

#else
#	error Unsupported platform in timestamp conversion
#endif
}

void CDateTime::Serialize( IFile& file )
{
	file << m_date;
	file << m_time;
}
