#pragma once

namespace Telemetry
{
#define DEFAULT_BATCH_TIME_SECONDS		1
#define DEFAULT_MAX_EVENTS_PER_BATCH	500
#define DEFAULT_TIMEOUT_TIME			30

	typedef char* utf8;
	typedef const char* const_utf8;

	typedef unsigned char* byte_ptr;
	typedef const unsigned char* const_byte_ptr;

	typedef wchar_t char16 ;
	typedef wchar_t* utf16;
	typedef const wchar_t* const_utf16;
	

	typedef void (*TelemetryAssert)( bool exp, const_utf8 msg );
	typedef void (*TelemetryDebug)( const char * format, ...  );

	enum EUserCategory
	{
		UC_NONE,
		UC_DEBUG,
		UC_DEV,
		UC_QA,
		UC_BETA,
		UC_RETAIL,
	};

	enum EPlatform
	{
		P_PC = 0,
		P_XBONE = 1,
		P_PS4 = 2,
	};
}
