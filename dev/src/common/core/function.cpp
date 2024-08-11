/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "function.h"
#include "scriptDebugger.h"
#include "scriptingSystem.h"
#include "namesRegistry.h"
#include "scriptStackFrame.h"
#include "scriptThread.h"
#include "feedback.h"
#include "configVar.h"

CFunction::CFunction( CClass* parentClass, CName name, Uint32 flags )
	: m_class( parentClass )
	, m_name( name )
	, m_flags( flags )
	, m_returnProperty( NULL )
	, m_size( 0 )
	, m_nativeFunction( 0 )
	, m_perfData( NULL )
{
#ifdef NEW_PROFILER_ENABLED
	m_profilerHandle = SScriptProfilerManager::GetInstance().RegisterScriptInstrFunc( this );
#endif
}

CFunction::CFunction( CClass* parentClass, CName name, TNativeFunc func )
	: m_class( parentClass )
	, m_name( name )
	, m_flags( FF_NativeFunction )
	, m_returnProperty( NULL )
	, m_size( 0 )
	, m_perfData( NULL )
{
	ASSERT( parentClass );

	// Map native function to index
	m_nativeFunction = GScriptingSystem->GetNativeFunctions().RegisterClassNative( func );
	ASSERT( m_nativeFunction );
#ifdef NEW_PROFILER_ENABLED
	m_profilerHandle = SScriptProfilerManager::GetInstance().RegisterScriptInstrFunc( this );
#endif
}

CFunction::CFunction( CName name, TNativeGlobalFunc func )
	: m_class( NULL )
	, m_name( name )
	, m_flags( FF_NativeFunction | FF_StaticFunction )
	, m_returnProperty( NULL )
	, m_size( 0 )
	, m_perfData( NULL )
{
	// Map native function to index
	m_nativeFunction = GScriptingSystem->GetNativeFunctions().RegisterGlobalNative( func );
	ASSERT( m_nativeFunction );
#ifdef NEW_PROFILER_ENABLED
	m_profilerHandle = SScriptProfilerManager::GetInstance().RegisterScriptInstrFunc( this );
#endif
}

CFunction::~CFunction()
{
	// Delete return property
	delete m_returnProperty;

	// Delete local variables
	for ( Uint32 i=0; i<m_localVars.Size(); i++ )
	{
		delete m_localVars[i];
	}

	// Delete parameters
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		delete m_parameters[i];
	}
}

CProperty* CFunction::FindProperty( CName propName ) const
{
	// Search in local first
	for ( Uint32 i=0; i<m_localVars.Size(); i++ )
	{
		CProperty* prop = m_localVars[i];
		if ( prop->GetName() == propName )
		{
			return prop;
		}
	}

	// Search in params
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		CProperty* prop = m_parameters[i];
		if ( prop->GetName() == propName )
		{
			return prop;
		}
	}

	// Not found
	return NULL;
}

void CFunction::GetProperties( TDynArray< CProperty*, MC_RTTI >& properties ) const
{
	// Start with the parameters
	properties.PushBack( m_parameters );

	// And the local variables
	properties.PushBack( m_localVars );
}

// Profiling
extern Bool GProfileFunctionCalls;

FunctionCallProfiler::FunctionCallProfiler( CFunction* func, CScriptStackFrame* parentStackFrame )
	: m_function( func )
	, m_parentStackFrame( parentStackFrame )
	, m_calledFunctionTicks( 0 )
{
	if ( GProfileFunctionCalls )
	{
		ASSERT( func );

		// Create profiling data
		if ( !func->m_perfData )
		{
			func->m_perfData = new FuncPerfData( func );
		}

		// Save start time
		FuncPerfData* data = func->m_perfData;
		m_saveTimer = data->m_recusion == 0;
		m_timerStart = Red::System::Clock::GetInstance().GetTimer().GetTicks();

		// Update stats
		++data->m_recusion;
		++data->m_numCalls;

		// Update caller info
		if ( func->GetName() != CNAME(__operator) && m_parentStackFrame && m_parentStackFrame->m_function )
		{
			FuncPerfData* callerData = m_parentStackFrame->m_function->m_perfData;
			if ( callerData )
			{
				callerData->m_called.PushBackUnique( func );

				// Mark a function being called
				if( !callerData->m_nestedMarker )
				{
					callerData->m_nestedMarker = func->GetName();
				}
				else if( callerData->m_nestedMarker == func->GetName() )
				{
					++callerData->m_markerCounter;
				}
			}
		}
	}
#ifdef NEW_PROFILER_ENABLED
	if( func->m_profilerHandle )
	{		
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		m_newProfilerStartTime = SProfilerManager::GetInstance().StartBlock( func->m_profilerHandle );
#else
		SProfilerManager::GetInstance().StartBlock( func->m_profilerHandle );
#endif //NEW_PROFILER_BREAKPOINTS_ENABLED		
	}
#endif//NEW_PROFILER_ENABLED
}

FunctionCallProfiler::~FunctionCallProfiler()
{
#ifdef NEW_PROFILER_ENABLED
	if( m_function->m_profilerHandle ) 
	{
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		SProfilerManager::GetInstance().EndBlock( m_function->m_profilerHandle, m_newProfilerStartTime );
#else
		SProfilerManager::GetInstance().EndBlock( m_function->m_profilerHandle );
#endif //NEW_PROFILER_BREAKPOINTS_ENABLED	
	}
#endif
	if ( GProfileFunctionCalls )
	{
		// Accumulate total ticks
		Uint64 totalTicks = Red::System::Clock::GetInstance().GetTimer().GetTicks() - m_timerStart;
		m_function->m_perfData->m_numTicks += totalTicks;

		ASSERT( m_function->m_perfData->m_recusion > 1 || ( !m_function->m_perfData->m_nestedMarker && m_function->m_perfData->m_markerCounter == 0 ) );
		ASSERT( m_function->m_perfData->m_numExTicks <= m_function->m_perfData->m_numTicks );
		    
		// If we have parent scope update it also
		if ( m_function->GetName() != CNAME(__operator) && m_parentStackFrame && m_parentStackFrame->m_function )
		{
			FuncPerfData* callerData = m_parentStackFrame->m_function->m_perfData;
			if ( callerData  )
			{
				ASSERT( callerData->m_nestedMarker );
				if( callerData->m_nestedMarker == m_function->GetName() )
				{
					if( callerData->m_markerCounter == 0 )
					{
						callerData->m_numExTicks += totalTicks;
						callerData->m_nestedMarker = CName::NONE;
					}
					else
					{
						--callerData->m_markerCounter;
					}
				}
			}
		}

		// Count recursion
		--m_function->m_perfData->m_recusion;
	}
}

Bool CFunction::Call( IScriptable* context, CScriptStackFrame& stackFrame, void* result ) const
{
#ifndef NO_PERFCOUNTERS
	FunctionCallProfiler profiler( ( CFunction* ) this, &stackFrame );
#endif

	//PC_SCOPE_PIX_NAMED( UNICODE_TO_ANSI( GetName().AsChar() ) );

	// Call to native function via virtual call
	if ( IsNative() )
	{
		// Get the function
		if ( IsStatic() )
		{
			const Uint32 nativeIndex = GetNativeFunctionIndex();
			TNativeGlobalFunc nativeFunction = CScriptNativeFunctionMap::GetGlobalNativeFunction( nativeIndex );
			if ( nativeFunction )
			{
				// Call function in context of current object
				( *nativeFunction )( context, stackFrame, result );
				return true;
			}
			else
			{
				// Error
				WARN_CORE( TXT("Trying to call nonexported global function %s."), GetName().AsString().AsChar() );
				return false;
			}
		}
		else
		{
			ASSERT( context );

			const Uint32 nativeIndex = GetNativeFunctionIndex();
			TNativeFunc nativeFunction = CScriptNativeFunctionMap::GetClassNativeFunction( nativeIndex );
			if ( nativeFunction )
			{
				// Call function in context of current object
				( context->*nativeFunction )( stackFrame, result );
				return true;
			}
			else
			{
				// Error
				WARN_CORE( TXT("Trying to call nonexported function %s from C++ class %s."), GetName().AsString().AsChar(), GetClass()->GetName().AsString().AsChar() );
				return false;
			}
		}
	}

	if ( IsLatent() )
	{
		ASSERT( stackFrame.m_thread );
		
		// Allocate stack space
		Uint8* newStack = (Uint8*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ScriptObject, m_size );
		
		// Create new stack frame
		if ( PrepareEntryFrameWithOutputs( context, stackFrame, newStack ) )
		{
			CScriptStackFrame* newFrame = new CScriptStackFrame( &stackFrame, context, this, newStack, newStack );
			newFrame->m_parentResult = result;
			stackFrame.m_thread->PushFrame( newFrame );

			// Break on function entry
			if ( stackFrame.m_debugFlags == SSDF_BreakOnFunctionEntry )
			{
				// Clear flags
				stackFrame.m_debugFlags = 0;

				newFrame->m_debugFlags = SSDF_BreakOnNext;
			}
		}
		else
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_ScriptObject, newStack );
		}
		return true;
	}

	// Allocate stack space
	Uint8* newStack = (Uint8*) RED_ALLOCA( m_size );

	if ( ! PrepareEntryFrameWithOutputs( context, stackFrame, newStack ) )
	{
		goto ExitFunction;
	}

	{
		// Create new stack frame
		CScriptStackFrame newFrame( &stackFrame, context, (CFunction*)this, newStack, newStack );
		const Uint8* codeEnd = m_code.GetCodeEnd();

		// Break on function entry
		if ( stackFrame.m_debugFlags == SSDF_BreakOnFunctionEntry )
		{
			// Clear flags
			stackFrame.m_debugFlags = 0;

#ifdef RED_NETWORK_ENABLED
			// Break on entry to this function
			CScriptDebugger debugger( context, newFrame, m_code.GetSourceLine() );
			debugger.ProcessBreakpoint();
#endif // RED_NETWORK_ENABLED
		}

		// Execute function code
		while ( newFrame.m_code < codeEnd )
		{
			// Thread exited
			if ( stackFrame.m_thread && stackFrame.m_thread->IsKilled() )
			{
				goto ExitFunction;
			}

			// Process return code
			if ( *newFrame.m_code == OP_Return )
			{
				newFrame.Step( context, result );
				break;
			}

			// Normal code, discard result
			newFrame.Step( context, NULL );
		}

		// Copy output parameters
		for ( Uint32 i=0; i< stackFrame.m_numOutputParams; i++ )
		{
			CScriptStackFrame::OutputParam &param = stackFrame.m_outputParams[i];
			param.m_type->Copy( param.m_dataOffset, param.m_localOffset );
		}

		// We've exited function while stepping over expression, make sure we enter first breakpoint in parent scope
		if ( newFrame.m_debugFlags == SSDF_BreakOnNext )
		{
			// Inform
			SCRIPT_LOG( stackFrame, TXT( "Debug: step over exited function '%" ) RED_PRIWs TXT( "'" ), GetName().AsChar() );

#ifdef RED_NETWORK_ENABLED
			// Break here
			CScriptDebugger debugger( context, stackFrame, stackFrame.m_line );
			debugger.ProcessBreakpoint();
#endif // RED_NETWORK_ENABLED
		}
	}

ExitFunction:
	stackFrame.m_numOutputParams = 0;

	// Cleanup the function stack after local parameters
	for ( Uint32 i=0; i<m_localsToDestroy.Size(); i++ )
	{
		CProperty* prop = m_localsToDestroy[i];
		void* propData = prop->GetOffsetPtr( newStack );
		prop->GetType()->Clean( propData );
	}

	// Cleanup the function after parameters
	for ( Uint32 i=0; i<m_paramsToDestroy.Size(); i++ )
	{
		CProperty* prop = m_paramsToDestroy[i];
		void* propData = prop->GetOffsetPtr( newStack );
		prop->GetType()->Clean( propData );
	}

	// Valid shit
	return true;
}

Bool CFunction::Call( IScriptable* context, const TDynArray< String >& parameters, String* result ) const
{
//  No profiler here
//	FunctionCallProfiler profiler( this, NULL );
	Bool callResult = false;

	//PC_SCOPE_PIX_NAMED( UNICODE_TO_ANSI( GetName().AsChar() ) );

	// Allocate param space
	Uint8* newStack = (Uint8*) RED_ALLOCA( m_size );
	Red::System::MemorySet( newStack, 0, m_size );

	// Evaluate function params (in previous context)
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		CProperty* prop = m_parameters[i];

		// Construct the data
		void* propData = prop->GetOffsetPtr( newStack );
		prop->GetType()->Construct( propData );

		// Evaluate from string
		if ( i < parameters.Size() )
		{
			const String& parameterValue = parameters[i];
			prop->GetType()->FromString( propData, parameterValue );
		}
	}

	// Call
	if ( result && m_returnProperty )
	{
		// Call and compute result
		CPropertyDataBuffer resultBuf( m_returnProperty );
		callResult = Call( context, newStack, resultBuf.Data() );

		// Convert to string
		resultBuf.GetType()->ToString( resultBuf.Data(), *result );
	}
	else
	{
		// Call without computing result
		callResult = Call( context, newStack, NULL );
	}

	// Cleanup the function after parameters
	for ( Uint32 i=0; i<m_paramsToDestroy.Size(); i++ )
	{
		CProperty* prop = m_paramsToDestroy[i];
		void* propData = prop->GetOffsetPtr( newStack );
		prop->GetType()->Clean( propData );
	}
	
	// Return call result
	return callResult;
}

Bool CFunction::Call( IScriptable* context, void* params, void* result ) const
{
	// shortcut.. apparently there are functions that have no code
	if ( m_code.GetCodeEnd() == NULL )
	{
		return true;
	}

#ifndef NO_PERFCOUNTERS
	FunctionCallProfiler profiler( ( CFunction* ) this, NULL );
#endif

	//PC_SCOPE_PIX_NAMED( UNICODE_TO_ANSI( GetName().AsChar() ) );

#ifndef NO_DEBUG_PAGES
	// Measure native function call
	CTimeCounter timer;
#endif
	// Allocate stack space
	Uint8 *newStack = (Uint8*)RED_ALLOCA( m_size );
	Red::System::MemorySet( newStack, 0, m_size );

	// Create stack frame
	CScriptStackFrame newFrame( NULL, context, (CFunction*)this, newStack, params );
	const Uint8* codeEnd = m_code.GetCodeEnd();

	// Execute script code
	while ( newFrame.m_code < codeEnd )
	{
		// Process return opcode
		if ( *newFrame.m_code == OP_Return )
		{
			newFrame.Step( context, result );
			break;
		}

		// Normal opcode
		newFrame.Step( context, NULL );
	}

	// Cleanup the function stack after local parameters
	for ( Uint32 i=0; i<m_localsToDestroy.Size(); i++ )
	{
		CProperty* prop = m_localsToDestroy[i];
		void* propData = prop->GetOffsetPtr( newStack );
		prop->GetType()->Clean( propData );
	}

#ifndef NO_DEBUG_PAGES
	// Was it slow ?
	const Float timeElapsed = timer.GetTimePeriod();
	if ( timeElapsed > 0.005f )
	{
		GScreenLog->PerfWarning( timeElapsed, TXT("SCRIPT"), TXT("Slow call to script function '%ls'"), GetName().AsString().AsChar() );
	}
#endif

	// Called
	return true;
}

void CFunction::DefineNativeReturnType( CName typeName )
{
	ASSERT( IsNative() );
	ASSERT( !m_returnProperty );

	// Find the type and create property
	IRTTIType* retType = SRTTI::GetInstance().FindType( typeName );
	m_returnProperty = new CProperty( retType, NULL, 0, CName( TXT("__ret") ), TXT("Return value"), PF_FuncRetValue );
}

void CFunction::DefineNativeParameter( CName typeName, Uint32 flags )
{
	ASSERT( IsNative() );

	// Find the type and create property
	IRTTIType* retType = SRTTI::GetInstance().FindType( typeName );
	CName paramName = CName( String::Printf( TXT("__param%d"), m_parameters.Size() ) );
	CProperty* prop = new CProperty( retType, NULL, 0, paramName, TXT("Parameter"), PF_FuncParam | flags );
	m_parameters.PushBack( prop );
}

void CFunction::CalcDataLayout()
{
	// Get all function properties
	TDynArray< CProperty*, MC_RTTI > properties;
	GetProperties( properties );

	// Calculate the layout
	m_size = CProperty::CalcDataLayout( properties, 0 );

	// Clear parameters list
	m_paramsToDestroy.ClearFast();
	m_localsToDestroy.ClearFast();

	// Scan local variables
	for ( TDynArray< CProperty*, MC_RTTI >::const_iterator it=m_localVars.Begin(); it!=m_localVars.End(); ++it )
	{
		if ( (*it)->GetType() && (*it)->GetType()->NeedsCleaning() )
		{
			m_localsToDestroy.PushBack( *it );
		}
	}

	// Scan parameters
	for ( TDynArray< CProperty*, MC_RTTI >::const_iterator it=m_parameters.Begin(); it!=m_parameters.End(); ++it )
	{
		if ( (*it)->GetType() && (*it)->GetType()->NeedsCleaning() )
		{
			m_paramsToDestroy.PushBack( *it );
		}
	}

	// Shrink
	m_localsToDestroy.Shrink();
	m_paramsToDestroy.Shrink();
}

void CFunction::ClearScriptData()
{
	// It's a native function, remove all properties
	if ( IsNative() )
	{
		// Clear properties
		m_parameters.ClearPtr();
		m_localVars.ClearPtr();

		// Clear properties cache
		m_paramsToDestroy.ClearFast();
		m_localsToDestroy.ClearFast();		 

		// Remove return value
		if ( m_returnProperty )
		{
			delete m_returnProperty;
			m_returnProperty = NULL;
		}
	}
	
	// Reset export flag
	m_flags &= ~FF_ExportedFunction;
}

void CFunction::InitailizerCode( const CScriptCodeGenerator& generator )
{
	m_code.Initialize( generator );
}

CScriptStackFrame* CFunction::CreateEntryFrame( IScriptable* context, CScriptStackFrame& parentStackFrame ) const
{
	ASSERT( !context || context->IsA( GetClass() ) );
	ASSERT( !IsNative() );

	// Allocate stack space
	Uint8* newStack = (Uint8*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ScriptObject, m_size );
	Red::System::MemorySet( newStack, 0, m_size );

	// Evaluate function params (in previous context)
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		CProperty* prop = m_parameters[i];

#ifndef NO_ASSERTS
		const Uint32 propFlags = prop->GetFlags();

		// Check code consistency
		Uint8 opcode = *parentStackFrame.m_code;
		if ( propFlags & PF_FuncSkipParam )
		{
			ASSERT( opcode == OP_Skip );
		}
		ASSERT( opcode != OP_ParamEnd );

#endif
		// Evaluate function input param
		void* propData = prop->GetOffsetPtr( newStack );
		parentStackFrame.Step( parentStackFrame.GetContext(), propData );
	}

	// Here we should found a "params_end" opcode
	ASSERT( *parentStackFrame.m_code == OP_ParamEnd );
	parentStackFrame.m_code++;

	// Create new stack frame
	CScriptStackFrame* newFrame = new CScriptStackFrame( NULL, context, this, newStack, newStack );
	return newFrame;
}

void CFunction::DestroyEntryFrame( CScriptStackFrame* frame ) const
{
	// Cleanup the function stack after local parameters
	for ( Uint32 i=0; i<m_localsToDestroy.Size(); i++ )
	{
		CProperty* prop = m_localsToDestroy[i];
		void* propData = prop->GetOffsetPtr( frame->m_locals );
		prop->GetType()->Clean( propData );
	}

	// Cleanup the function after parameters
	for ( Uint32 i=0; i<m_paramsToDestroy.Size(); i++ )
	{
		CProperty* prop = m_paramsToDestroy[i];
		void* propData = prop->GetOffsetPtr( frame->m_params );
		prop->GetType()->Clean( propData );
	}

	// Free memory
	void* frameMemory  = frame->m_locals;
	
	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_ScriptObject, frameMemory );
	
	// Delete frame
	delete frame;
}

Bool CFunction::PrepareEntryFrameWithOutputs( IScriptable*, CScriptStackFrame& parentStackFrame, Uint8 * newStack ) const
{
	//ASSERT( !context || context->IsA( GetClass() ) );
	ASSERT( !IsNative() );

	Red::System::MemorySet( newStack, 0, m_size );

	ASSERT( parentStackFrame.m_numOutputParams == 0 );

	// Evaluate function params (in previous context)
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		CProperty* prop = m_parameters[i];
		const Uint32 propFlags = prop->GetFlags();

#ifndef NO_ASSERTS
		// Check code consistency
		Uint8 opcode = *parentStackFrame.m_code;
		if ( propFlags & PF_FuncSkipParam )
		{
			ASSERT( opcode == OP_Skip );
		}
		ASSERT( opcode != OP_ParamEnd );
#endif

		// Evaluate function input param
		void* propData = prop->GetOffsetPtr( newStack );
		parentStackFrame.Step( parentStackFrame.GetContext(), propData );

		// Thread was killed, exit
		if ( parentStackFrame.m_thread && parentStackFrame.m_thread->IsKilled() )
		{
			parentStackFrame.m_numOutputParams = 0;
			return false;
		}

		// Remember output params
		if ( (propFlags & PF_FuncOutParam)  && (parentStackFrame.m_numOutputParams < 8) )
		{
			CScriptStackFrame::OutputParam& param = parentStackFrame.m_outputParams[ parentStackFrame.m_numOutputParams++ ];
			param.m_dataOffset  = parentStackFrame.s_offset;
			param.m_localOffset = propData;
			param.m_type = prop->GetType();
		}
	}

	// Here we should found a "params_end" opcode
	ASSERT( *parentStackFrame.m_code == OP_ParamEnd );
	parentStackFrame.m_code++;

	return true;
}

#ifdef NEW_PROFILER_ENABLED

namespace Config
{
	TConfigVar<Bool> cvProfileScripts( "Profiler", "ProfileScripts", false, eConsoleVarFlag_ReadOnly );
}

CScriptProfilerManager::CScriptProfilerManager(): m_scriptHandlesCounter(0)
{
	EnableProfileFunctionCalls( Config::cvProfileScripts.Get() );
}

CScriptProfilerManager::~CScriptProfilerManager()
{

}


void CScriptProfilerManager::EnableProfileFunctionCalls( Bool enable )
{	
	if( SProfilerManager::GetInstance().IsStarted() == false )
	{
		m_profileFunctionCalls.SetValue( enable );
		Uint32 index = 0;
		Uint32 count = Min< Uint32 >( m_scriptHandlesCounter.GetValue(), SCRIPT_PROFILER_MAX_PROFILES_COUNT );
		while( index < count )
		{
			if( m_scriptHandles[index].m_instrumentedFunction != NULL )
			{
				SProfilerManager::GetInstance().EnableInstrFunc( m_scriptHandles[index].m_instrumentedFunction, enable );
			}
			index++;
		}
	}
	else
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Can not change profiler setting during profile session !!!" ) );
	}
}

Bool CScriptProfilerManager::IsEnableProfileFunctionCalls()
{
	return m_profileFunctionCalls.GetValue();
}

NewRedProfiler::InstrumentedFunction* CScriptProfilerManager::RegisterScriptInstrFunc( const CFunction* func )
{
	if ( func && (func->GetName() != CNAME(__operator)) )
	{
		const Uint32 oldCounter = m_scriptHandlesCounter.Increment()-1;
		if ( oldCounter >= SCRIPT_PROFILER_MAX_PROFILES_COUNT )
		{
			// log only once
			if ( oldCounter == SCRIPT_PROFILER_MAX_PROFILES_COUNT )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Maximum number [%i] of InstrumentedFunction registrations for Scripts exceeded!!!" ), SCRIPT_PROFILER_MAX_PROFILES_COUNT );
			}

			return NULL;
		}

		if( func->GetClass() )
		{
			const AnsiChar* format =  "WS_%s_%s";
			if( func->GetNativeFunctionIndex() != 0 )
			{
				format = "WS_native_import_%s_%s";
			}

			AnsiChar buf[ 512 ];
			Red::SNPrintF( buf, ARRAY_COUNT_U32(buf), format, func->GetClass()->GetName().AsAnsiChar(), func->GetName().AsAnsiChar() );
			m_scriptHandles[oldCounter].m_functionName = buf;
		}
		else
		{
			const AnsiChar* format = "WS_%s";
			if( func->GetNativeFunctionIndex() != 0 )
			{
				format = "WS_native_import_%s";
			}

			AnsiChar buf[ 512 ];
			Red::SNPrintF( buf, ARRAY_COUNT_U32(buf), format, func->GetName().AsAnsiChar() );
			m_scriptHandles[oldCounter].m_functionName = buf;
		}	

		m_scriptHandles[oldCounter].m_instrumentedFunction = SProfilerManager::GetInstance().RegisterInstrFunc( m_scriptHandles[oldCounter].m_functionName.AsChar(), m_profileFunctionCalls.GetValue() );
		return m_scriptHandles[oldCounter].m_instrumentedFunction;
	}
	return NULL;
}

#endif //NEW_PROFILER_ENABLED
