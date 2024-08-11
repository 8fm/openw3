/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "engineTime.h"
#include "scriptStackFrame.h"

IMPLEMENT_RTTI_CLASS( EngineTime );

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
LARGE_INTEGER	EngineTime::s_timerFreq		= {0, 0};
#elif defined( RED_PLATFORM_ORBIS )
SceInt32		EngineTime::s_timerFreq = 0;
#endif

Double			EngineTime::s_timerFreqDbl = 0.0;

const EngineTime EngineTime::ZERO;

void EngineTime::Init()
{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	QueryPerformanceFrequency(&s_timerFreq);
	s_timerFreqDbl = Double( s_timerFreq.QuadPart );
#elif defined( RED_PLATFORM_ORBIS )
	s_timerFreq = ::sceRtcGetTickResolution();
	RED_ASSERT( s_timerFreq > 0 );
	s_timerFreqDbl = Double( s_timerFreq );
#endif
}

EngineTime EngineTime::GetNow()
{
	EngineTime result;
	result.SetNow();
	return result;
}

void EngineTime::SetNow()
{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	QueryPerformanceCounter(&m_time);
#elif defined( RED_PLATFORM_ORBIS )
	SceRtcTick tick;
	RED_VERIFY( ::sceRtcGetCurrentTick( &tick ) == SCE_OK );
	m_time = tick.tick;
#endif
}

static void funcEngineTimeFromFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, EngineTime( a ) );
};


static void funcEngineTimeToFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_FLOAT( Float( a ) );
};

static void funcEngineTimeToString( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_STRING( ToString( Double( a ) ) );
};

void ExportCoreEngineTimeNatives()
{
	NATIVE_GLOBAL_FUNCTION( "EngineTimeFromFloat",	funcEngineTimeFromFloat );
	NATIVE_GLOBAL_FUNCTION( "EngineTimeToFloat",	funcEngineTimeToFloat );
	NATIVE_GLOBAL_FUNCTION( "EngineTimeToString",	funcEngineTimeToString );
}
