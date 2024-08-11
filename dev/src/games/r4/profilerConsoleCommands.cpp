#include "build.h"
#include "../../common/core/redTelemetryServicesManager.h"
#include "profilerConsoleCommands.h"

void funProfiler_Init( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	GET_PARAMETER( Int32, bufferSize, Int32(0) );
	FINISH_PARAMETERS;
	if( bufferSize > 0 )
	{
		PROFILER_Init( bufferSize );
	}
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_InitEx( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	GET_PARAMETER( Int32, bufferSize, Int32(0) );
	GET_PARAMETER( Int32, bufferSignalsSize, Int32(0) );
	FINISH_PARAMETERS;
	if( bufferSize > 0 )
	{
		PROFILER_InitEx( bufferSize, bufferSignalsSize );
	}
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_ScriptEnable( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
#ifdef NEW_PROFILER_ENABLED
	SScriptProfilerManager::GetInstance().EnableProfileFunctionCalls( true );
#endif
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_ScriptDisable( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
#ifdef NEW_PROFILER_ENABLED
	SScriptProfilerManager::GetInstance().EnableProfileFunctionCalls( false );
#endif
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_Start( IScriptable* context, CScriptStackFrame& stack, void* result )
{	
	RED_UNUSED( context );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
	PROFILER_Start();
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_Stop( IScriptable* context, CScriptStackFrame& stack, void* result )
{	
	RED_UNUSED( context );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
	PROFILER_Stop();
}

//////////////////////////////////////////////////////////////////////////

void profiler_Store( const String& fileName )
{
	String sessionId;
	unsigned long long sessionQpc = 0;
	unsigned long long sessionQpf = 0;
#if !defined( NO_TELEMETRY )
	IRedTelemetryServiceInterface* interfaceService = SRedTelemetryServicesManager::GetInstance().GetService( TXT("telemetry") );
	if( interfaceService != NULL)
	{
		sessionId = interfaceService->GetSessionId(Telemetry::BT_RED_TEL_API);
		double time;
		unsigned long long qpf, qpc;
		interfaceService->GetTime( time, qpf, qpc );
		sessionQpf = qpf;
		sessionQpc = qpc;
	}
#endif

	PROFILER_Store( fileName, sessionId, sessionQpc, sessionQpf );
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_Store( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	GET_PARAMETER( String, fileName, String::EMPTY );
	FINISH_PARAMETERS;
	if( fileName.GetLength() > 0 )
	{
		profiler_Store( fileName );
	}
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_StoreDef( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	profiler_Store( String::EMPTY );
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_StoreInstrFuncList( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
	PROFILER_StoreInstrFuncList();
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_StartCatchBreakpoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
	PROFILER_StartCatchBreakpoint();
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_StopCatchBreakpoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
	PROFILER_StopCatchBreakpoint();
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_SetTimeBreakpoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	GET_PARAMETER( String, instrFuncName, String::EMPTY );
	GET_PARAMETER( Float, time, 0.0f );
	GET_PARAMETER( Bool, stopOnce, false );
	FINISH_PARAMETERS;
	PROFILER_SetTimeBreakpoint( UNICODE_TO_ANSI(instrFuncName.AsChar()), time, stopOnce );
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_SetHitCountBreakpoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	GET_PARAMETER( String, instrFuncName, String::EMPTY );
	GET_PARAMETER( Uint32, count, 0 );
	FINISH_PARAMETERS;
	PROFILER_SetHitCountBreakpoint( UNICODE_TO_ANSI(instrFuncName.AsChar()), count );
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_DisableTimeBreakpoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	GET_PARAMETER( String, instrFuncName, String::EMPTY );
	FINISH_PARAMETERS;
	PROFILER_DisableTimeBreakpoint( UNICODE_TO_ANSI(instrFuncName.AsChar()) );
}

//////////////////////////////////////////////////////////////////////////

void funProfiler_DisableHitCountBreakpoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );
	GET_PARAMETER( String, instrFuncName, String::EMPTY );
	FINISH_PARAMETERS;
	PROFILER_DisableHitCountBreakpoint( UNICODE_TO_ANSI(instrFuncName.AsChar()) );
}

//////////////////////////////////////////////////////////////////////////

CR4ProfilerScriptRegistration::CR4ProfilerScriptRegistration()
{

}

//////////////////////////////////////////////////////////////////////////

void CR4ProfilerScriptRegistration::RegisterScriptFunctions()const
{
	NATIVE_GLOBAL_FUNCTION( "PROFILER_Init",						funProfiler_Init );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_InitEx",						funProfiler_InitEx );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_ScriptEnable",				funProfiler_ScriptEnable );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_ScriptDisable",				funProfiler_ScriptDisable );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_Start",						funProfiler_Start );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_Stop",						funProfiler_Stop );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_Store",						funProfiler_Store );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_StoreDef",					funProfiler_StoreDef );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_StoreInstrFuncList",			funProfiler_StoreInstrFuncList );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_StartCatchBreakpoint",		funProfiler_StartCatchBreakpoint );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_StopCatchBreakpoint",			funProfiler_StopCatchBreakpoint );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_SetTimeBreakpoint",			funProfiler_SetTimeBreakpoint );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_SetHitCountBreakpoint",		funProfiler_SetHitCountBreakpoint );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_DisableTimeBreakpoint",		funProfiler_DisableTimeBreakpoint );
	NATIVE_GLOBAL_FUNCTION( "PROFILER_DisableHitCountBreakpoint",	funProfiler_DisableHitCountBreakpoint );
}

//////////////////////////////////////////////////////////////////////////
