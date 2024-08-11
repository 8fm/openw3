/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "scriptableStateMachine.h"
#include "scriptableState.h"
#include "scriptDebugger.h"
#include "scriptThreadSerializer.h"
#include "uniPointer.h"
#include "scriptStackFrame.h"
#include "sortedset.h"

void OpNop( IScriptable*, CScriptStackFrame&, void* )
{
}

void OpNull( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( NULL );
}

void OpIntOne( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_INT( 1 );
}

void OpIntZero( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_INT( 0 );
}

void OpIntConst( IScriptable*, CScriptStackFrame& stack, void* result )
{
	Int32 value = stack.Read< Int32 >();
	RETURN_INT( value );
}

void OpFloatConst( IScriptable*, CScriptStackFrame& stack, void* result )
{
	Float value = stack.Read< Float >();
	RETURN_FLOAT( value );
}

void OpShortConst( IScriptable*, CScriptStackFrame& stack, void* result )
{
	Int16 value = stack.Read< Int16 >();
	RETURN_SHORT( value );
}

void OpStringConst( IScriptable*, CScriptStackFrame& stack, void* result )
{
	Uint32 length = stack.Read< Uint32 >();

	if ( NULL != result)
	{
		String* value = (String*)result;
		if ( length > 0 )
		{
			// prepare string buffer
			value->Resize( length + 1 );
			(*value)[ length ] = 0;

			// copy string chars
			Red::System::MemoryCopy( &(*value)[0], stack.m_code, sizeof(Char) * length );
		}
		else
		{
			// empty string
			value->Clear();
		}
	}

	// go to the end of the string data
	stack.m_code += sizeof(Char) * length;
}

void OpNameConst( IScriptable*, CScriptStackFrame& stack, void* result )
{
	CName value = stack.Read< CName >();
	RETURN_NAME( value );
}

void OpByteConst( IScriptable*, CScriptStackFrame& stack, void* result )
{
	Uint8 value = stack.Read< Uint8 >();
	RETURN_BYTE( value );
}

void OpBoolTrue( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_BOOL( true );
}

void OpBoolFalse( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_BOOL( false );
}

void OpBreakpoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Read the line
	Uint32 line = stack.Read< Uint32 >();
	stack.m_line = line;

	// Read the status
	Uint8 breakpointEnabled = stack.Read< Uint8 >();

#ifdef RED_NETWORK_ENABLED
	// Enter breakpoint :)
	const Bool userSteppingOver = stack.m_debugFlags == SSDF_BreakOnNext;
	if ( breakpointEnabled || userSteppingOver )
	{
		// Break, and process messages from debugger
		CScriptDebugger debugger( context, stack, line );
		debugger.ProcessBreakpoint();
	}
#endif // RED_NETWORK_ENABLED

	// Step to the rest of the code
	stack.Step( context, result );

	// We wanted to step into a function but we didn't, stop on next breakpoint
	if ( stack.m_debugFlags == SSDF_BreakOnFunctionEntry )
	{
		stack.m_debugFlags = SSDF_BreakOnNext;
	}
}

void OpVirtualFunction( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( context );

	// Calculate the address to jump in case state entry failed
	Uint16 skipSize = stack.Read< Uint16 >();
	const Uint8* codeSkip = stack.m_code + skipSize; 

	// Get line
	const Uint32 scriptLine = stack.Read< Uint16 >();
	stack.m_line = scriptLine;

	// Get the function name 
	CName funcName = stack.Read<CName>();

	// Find function to call
	const CFunction* functionToCall = context->FindFunction( context, funcName );
	if ( !functionToCall )
	{
		SCRIPT_ERROR( stack, TXT("Missing virtual function '%ls', line %d, context '%ls'"), 
			funcName.AsChar(), scriptLine, context->GetClass()->GetName().AsChar() );
		stack.m_code = codeSkip;
		return;
	}

	// Call the function
	if ( !functionToCall->Call( context, stack, result ) )
	{
		stack.m_code = codeSkip;
		return;
	}

#ifdef RED_NETWORK_ENABLED
	// Returned from function
	if ( stack.m_debugFlags == SSDF_BreakOnFunctionReturn )
	{
		CScriptDebugger debugger( context, stack, scriptLine );
		debugger.ProcessBreakpoint();
	}
#endif // RED_NETWORK_ENABLED
}

void OpVirtualParentFunction( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( context );

	// Calculate the address to jump in case state entry failed
	Uint16 skipSize = stack.Read< Uint16 >();
	const Uint8* codeSkip = stack.m_code + skipSize; 

	// Get line
	const Uint32 scriptLine = stack.Read< Uint16 >();
	stack.m_line = scriptLine;

	// Get the function name 
	CName funcName = stack.Read<CName>();

	// Find function to call
	const Bool allowStates = false; // direct object function call
	const CFunction* functionToCall = context->FindFunction( context, funcName, allowStates );
	if ( !functionToCall )
	{
		SCRIPT_ERROR( stack, TXT("Missing virtual parent function '%ls', line %d, context '%ls'"), 
			funcName.AsChar(), scriptLine, context->GetClass()->GetName().AsChar() );
		stack.m_code = codeSkip;
		return;
	}

	// Call the function
	if ( !functionToCall->Call( context, stack, result ) )
	{
		stack.m_code = codeSkip;
		return;
	}

#ifdef RED_NETWORK_ENABLED
	// Returned from function
	if ( stack.m_debugFlags == SSDF_BreakOnFunctionReturn )
	{
		CScriptDebugger debugger( context, stack, scriptLine );
		debugger.ProcessBreakpoint();
	}
#endif // RED_NETWORK_ENABLED
}

void OpFinalFunction( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Calculate the address to jump in case state entry failed
	Uint16 skipSize = stack.Read< Uint16 >();
	const Uint8* codeSkip = stack.m_code + skipSize; 

	// Get line
	const Uint32 scriptLine = stack.Read< Uint16 >();
	stack.m_line = scriptLine;

	// Get the function to call
	TUniPointer< CFunction > func = stack.Read< TUniPointer< CFunction > >();
	ASSERT( func );

	// Call the function
	if ( !func->Call( context, stack, result ) )
	{
		stack.m_code = codeSkip;
		return;
	}

#ifdef RED_NETWORK_ENABLED
	// Returned from function
	if ( stack.m_debugFlags == SSDF_BreakOnFunctionReturn )
	{
		CScriptDebugger debugger( context, stack, scriptLine );
		debugger.ProcessBreakpoint();
	}
#endif // RED_NETWORK_ENABLED
}

void OpEntryFunction( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Calculate the address to jump in case state entry failed
	Uint16 skipSize = stack.Read< Uint16 >();
	const Uint8* codeSkip = stack.m_code + skipSize; 

	// Get the name of the function to call
	CName functionName = stack.Read<CName>();
	ASSERT( functionName );

	CScriptableState* contextState = NULL;

	// Context should be an state machine
	IScriptable* stateMachine = context;
	if ( context->IsA< CScriptableState >() )
	{
		CScriptableState* state = static_cast< CScriptableState* >( context );
		contextState = state;
		stateMachine = state->GetStateMachine();
	}

	// Find the state entry function
	const CFunction* func = stateMachine->FindEntryFunction( functionName );
	if ( !func )
	{
#ifndef NO_ASSERTS
		RED_LOG_WARNING( RED_LOG_CHANNEL( Script ), TXT( "Unable to find state entry function '%ls' in state machine '%ls'" ), 
			functionName.AsString().AsChar(), 
			stateMachine->GetClass()->GetName().AsString().AsChar() );
		SCRIPT_DUMP_STACK_TO_LOG( stack );
#endif
		stack.m_code = codeSkip;
		RETURN_BOOL( false );
		return;
	}

	// Get the name of the state
	const CClass* stateClass = func->GetClass();
	ASSERT( stateClass->IsState() );

	// If we are in state derived from state with given entry function, call entry function in context of current state
	CName stateName;	
	if( contextState && contextState->GetClass()->IsBasedOn( stateClass ) )
	{
		stateName = contextState->GetClass()->GetStateName();
	}
	else
	{
		stateName = stateClass->GetStateName();
	}

	// Enter the state
	Bool flag = stateMachine->EnterState( stateName, func, stack );

	// Check consistency
	if ( flag )
	{
		// Since entering of the state succeeded we should be past the parameters block
		ASSERT( stack.m_code == codeSkip );
	}
	else 
	{
		// Entering state failed, skip the parameters
		stack.m_code = codeSkip;
	}

	// Return true if we have entered a new state
	RETURN_BOOL( flag );
}

void OpReturn( IScriptable*, CScriptStackFrame& stack, void* result )
{
	// Calculate result
	stack.Step( stack.GetContext(), result );
}

void OpLocalVar( IScriptable*, CScriptStackFrame& stack, void* result )
{
	// Read property
	TUniPointer< CProperty > prop = stack.Read< TUniPointer< CProperty > >();;
	ASSERT( prop );

	// Setup property reference
	void* propData = prop->GetOffsetPtr( stack.m_locals );
	CScriptStackFrame::s_offset = propData;

	// Copy property value if requested
	if ( result )
	{
		IRTTIType* type = prop->GetType();
		type->Copy( result, propData );
	}
}

void OpParamVar( IScriptable*, CScriptStackFrame& stack, void* result )
{
	// Read property
	TUniPointer< CProperty > prop = stack.Read< TUniPointer< CProperty > >();;
	ASSERT( prop );

	// Setup property reference
	void* propData = prop->GetOffsetPtr( stack.m_params );
	CScriptStackFrame::s_offset = propData;

	// Copy property value if requested
	if ( result )
	{
		IRTTIType* type = prop->GetType();
		type->Copy( result, propData );
	}
}

void OpObjectVar( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( context );

	// Read property
	TUniPointer< CProperty > prop = stack.Read< TUniPointer< CProperty >>();;
	ASSERT( prop );

	// Setup property reference
	void* propData = prop->GetOffsetPtr( context );
	CScriptStackFrame::s_offset = propData;

	// Copy property value if requested
	if ( result )
	{
		IRTTIType* type = prop->GetType();
		type->Copy( result, propData );
	}
}

void OpObjectBindableVar( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( context );

	// Read property
	TUniPointer< CProperty > prop = stack.Read< TUniPointer< CProperty >>();;
	ASSERT( prop );

	// Setup property reference
	void* propData = prop->GetOffsetPtr( context );
	CScriptStackFrame::s_offset = propData;

	// Is the handle lost/empty ?
	RED_FATAL_ASSERT( prop->GetType()->GetType() == RT_Handle, "Bindable property '%ls' is not a handle but a '%ls'", prop->GetName().AsChar(), prop->GetType()->GetName().AsChar() );
	BaseSafeHandle* propHandleData = (BaseSafeHandle*) propData;
	if ( !propHandleData->IsValid() )
	{
		// Try to resolve the property dynamically
		if ( !context )
		{
			if ( !prop->IsAutoBindOptional() )
			{
				SCRIPT_ERROR( stack, TXT( "Trying to access bindable property '%ls' from NULL context. Property will not be resolved." ), 
					prop->GetName().AsChar() );
			}
		}
		else
		{
			// resolve current property binding - use dynamic class
			const CName resolvedBinding = context->GetClass()->FindPropertyBinding( prop->GetName() );
			if ( !resolvedBinding )
			{
				if ( !prop->IsAutoBindOptional() )
				{
					SCRIPT_ERROR( stack, TXT( "Unable to resolve binding for property '%ls' in class '%ls'." ), 
						prop->GetName().AsChar(), context->GetClass()->GetName().AsChar() );
				}
			}
			else
			{
				// use the binding name to resolve actual property value
				if ( !context->ResolveAutoBindProperty( prop.Get(), resolvedBinding, propData ) )
				{
					if ( !prop->IsAutoBindOptional() )
					{
						SCRIPT_ERROR( stack, TXT( "Unable to resolve value for property '%ls' with binding '%ls' in class '%ls', object '%ls'." ), 
							prop->GetName().AsChar(), resolvedBinding.AsChar(), 
							context->GetClass()->GetName().AsChar(), context->GetFriendlyName().AsChar() );
					}
				}
			}
		}
	}

	// Copy property value if requested
	if ( result )
	{
		IRTTIType* type = prop->GetType();
		type->Copy( result, propData );
	}
}

void OpDefaultVar( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( context );

	// Use default object of object's class
	IScriptable* defaultObject = context->GetClass()->GetDefaultObject< IScriptable >();

	// Read property
	TUniPointer< CProperty > prop = stack.Read< TUniPointer< CProperty > >();
	ASSERT( prop );

	// Setup property reference
	void* propData = prop->GetOffsetPtr( defaultObject );
	CScriptStackFrame::s_offset = propData;

	// Copy property value if requested
	if ( result )
	{
		IRTTIType* type = prop->GetType();
		type->Copy( result, propData );
	}
}

void OpParent( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Read the context
	THandle< IScriptable > handle;
	stack.Step( context, &handle );

	// Get the state machine
	CScriptableState* state = Cast< CScriptableState >( handle.Get() );
	if ( !state )
	{
		SCRIPT_LOG( stack, TXT( "Trying to access NULL state for parent scope" ) );
		RETURN_OBJECT( NULL );
		return;
	}

	// Get state machine
	IScriptable* stateMachine = state->GetStateMachine();
	if ( !stateMachine )
	{
		SCRIPT_LOG( stack, TXT( "Trying to access nonstate machine object as a parent scope" ) );
		RETURN_OBJECT( NULL );
		return;
	}

	// Valid, return state machine
	RETURN_OBJECT( stateMachine );
}

void OpContext( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Calculate the address to jump in case context will be invalid
	Uint16 skipSize = stack.Read< Uint16 >();
	const Uint8* codeSkip = stack.m_code + skipSize; 

	// Evaluate the new context
	THandle< IScriptable > newContextHandle;
	stack.Step( context, &newContextHandle );

	// If context is valid, execute in new context
	IScriptable* newContext = newContextHandle.Get();
	if ( newContext )
	{
		// Execute rest of the code in new context
		stack.Step( newContext, result );
	}
	else
	{
		// Error !
		SCRIPT_WARN_ONCE( stack, TXT( "Accessing invalid handle" ) ); 

		// Skip the code
		stack.m_code = codeSkip;

		// Clear the data pointer
		CScriptStackFrame::s_offset = NULL;
	}
}

void OpStructMember( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Read structure property to access
	TUniPointer<CProperty> structProp = stack.Read< TUniPointer<CProperty> >();
	ASSERT( structProp );

	// Get the data pointer
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );
	
	// the script log below is just enough for this, assert is not needed
	// ASSERT( CScriptStackFrame::s_offset );
	
	if( CScriptStackFrame::s_offset )
	{
		// Get the data
		void* propData = structProp->GetOffsetPtr( CScriptStackFrame::s_offset );
		CScriptStackFrame::s_offset = propData;

		// Copy data
		if ( result )
		{
			structProp->GetType()->Copy( result, propData );
		}
	}
	else
	{
		// Error !
		SCRIPT_WARN_ONCE( stack, TXT( "Invalid structure member access" ) );
	}
}

void OpAssign( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Evaluate left side to get the target address
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// No place to write the variable to
	void* destPtr = CScriptStackFrame::s_offset;
	if ( !destPtr )
	{
		// Emit error
		SCRIPT_ERROR( stack, TXT( "Trying to assign value from invalid context" ) );

		// Clear buffer - we don't store the type any more so use a large enough buffer here
		// (was to costly + was causing issues with 32/64 bit compatibility)
		const Uint32 typeSize = 512;
		destPtr = RED_ALLOCA( typeSize );
		Red::System::MemorySet( destPtr, 0, typeSize );
	}

	// Write target to destination
	stack.Step( context, destPtr );
}

void OpJump( IScriptable*, CScriptStackFrame& stack, void* )
{
	// Jump
	Int16 offset = stack.Read<Int16>();
	stack.m_code += offset; 
}

void OpJumpIfFalse( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Get the offset to jump
	Int16 offset = stack.Read<Int16>();
	const Uint8* codeSkip = stack.m_code + offset; 

	// Evaluate expression
	Bool condition = false;
	stack.Step( context, &condition );

	// Execute jump
	if ( !condition )
	{
		stack.m_code = codeSkip;
	}
}

void OpSwitch( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Read the type of the switch expression
	TUniPointer<IRTTIType> exprType = stack.Read< TUniPointer<IRTTIType> >();
	ASSERT( exprType );

	// Read the offset to first label
	Int16 skipOffset = stack.Read<Int16>();
	const Uint8* switchLabel = stack.m_code + skipOffset; 
	ASSERT( *switchLabel == OP_SwitchLabel || *switchLabel == OP_SwitchDefault );

	// Create expression buffer
	void* propData1 = RED_ALLOCA( exprType->GetSize() );
	Red::System::MemoryZero( propData1, exprType->GetSize() );
	exprType->Construct( propData1 );

	// Evaluate expression
	stack.Step( context, propData1 );

	// Create comparison buffer 
	void* propData2 = RED_ALLOCA( exprType->GetSize() );
	Red::System::MemoryZero( propData2, exprType->GetSize() );
	exprType->Construct( propData2 );

	// Process linked list
	Bool handled = false;
	while ( *switchLabel == OP_SwitchLabel )
	{	
		// Skip the opcode
		ASSERT( *switchLabel == OP_SwitchLabel );
		stack.m_code = switchLabel + 1;

		// Get the offset to next label
		Int16 skipOffset = stack.Read<Int16>();
		const Uint8* nextSwitchLabel = stack.m_code + skipOffset; 
		/*Int16 exprSkipOffset =*/ stack.Read<Int16>();

		// Load value
		stack.Step( context, propData2 );

		// Compare label value with switch expression
		if ( exprType->Compare( propData1, propData2, 0 ) )
		{
			handled = true;
			break;
		}

		// Default case
		if ( *nextSwitchLabel == OP_SwitchDefault )
		{
			stack.m_code = nextSwitchLabel + 1;
			handled = true;
			break;
		}

		// Skip to next
		switchLabel = nextSwitchLabel;
	}

	// Log any non supported case
	if ( !handled )
	{
		// Error
#ifndef NO_ASSERTS
		String val;
		exprType->ToString( propData1, val );
		SCRIPT_ERROR( stack, TXT( "Value '%" ) RED_PRIWs TXT( "' not handled by '%" ) RED_PRIWs TXT( "' switch case" ), val.AsChar(), exprType->GetName().AsChar() );
		SCRIPT_DUMP_STACK_TO_LOG( stack );
#endif
		// Continue at break label
		stack.m_code = switchLabel;
	}

	// Cleanup
	exprType->Destruct( propData1 );
	exprType->Destruct( propData2 );
}

void OpSwitchCase( IScriptable*, CScriptStackFrame& stack, void* )
{
	// Get the offset to next label
	/*Int16 skipOffset =*/ stack.Read<Int16>();
	Int16 exprSkipOffset = stack.Read<Int16>();

	// Skip the expression
	const Uint8* codeSkip = stack.m_code + exprSkipOffset; 
	stack.m_code = codeSkip;
}

void OpSwitchCaseDefault( IScriptable*, CScriptStackFrame&, void* )
{
	// Do nothing
}

void OpTestEqual( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get the types
	TUniPointer<IRTTIType> type = stack.Read< TUniPointer<IRTTIType> >();
	
	// Allocate buffer for first data
	void* dataA = RED_ALLOCA( type->GetSize() );
	Red::System::MemorySet( dataA, 0, type->GetSize() );
	type->Construct( dataA );

	// Get first data
	stack.Step( context, dataA );

	// Allocate buffer for second data
	void* dataB = RED_ALLOCA( type->GetSize() );
	Red::System::MemorySet( dataB, 0, type->GetSize() );
	type->Construct( dataB );

	// Get first data
	stack.Step( context, dataB );

	// Compare data
	Bool isEqual = type->Compare( dataA, dataB, 0 );
	
	// Destroy data
	type->Destruct( dataA );
	type->Destruct( dataB );

	// Return result
	RETURN_BOOL( isEqual );
}

void OpTestNotEqual( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get the types
	TUniPointer<IRTTIType> type = stack.Read< TUniPointer<IRTTIType> >();

	// Allocate buffer for first data
	void* dataA = RED_ALLOCA( type->GetSize() );
	Red::System::MemorySet( dataA, 0, type->GetSize() );
	type->Construct( dataA );

	// Get first data
	stack.Step( context, dataA );

	// Allocate buffer for second data
	void* dataB = RED_ALLOCA( type->GetSize() );
	Red::System::MemorySet( dataB, 0, type->GetSize() );
	type->Construct( dataB );

	// Get first data
	stack.Step( context, dataB );

	// Compare data
	Bool isEqual = type->Compare( dataA, dataB, 0 );

	// Destroy data
	type->Destruct( dataA );
	type->Destruct( dataB );

	// Return result
	RETURN_BOOL( !isEqual );
}

void OpBoolToByte( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Bool value = false;
	stack.Step( context, &value );
	RETURN_BYTE( value ? 1 : 0 );
}

void OpBoolToInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Bool value = false;
	stack.Step( context, &value );
	RETURN_INT( value ? 1 : 0 );
}

void OpBoolToFloat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Bool value = false;
	stack.Step( context, &value );
	RETURN_FLOAT( value ? 1.0f : 0.0f );
}

void OpBoolToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Bool value = false;
	stack.Step( context, &value );
	RETURN_STRING( value ? TXT("true") : TXT("false") );
}

void OpByteToBool( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Uint8 value = 0;
	stack.Step( context, &value );
	RETURN_BOOL( value != 0 );
}

void OpByteToInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Uint8 value = 0;
	stack.Step( context, &value );
	RETURN_INT( value );
}

void OpByteToFloat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Uint8 value = 0;
	stack.Step( context, &value );
	RETURN_FLOAT( value );
}

void OpByteToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Uint8 value = 0;
	stack.Step( context, &value );
	Char txt[ 32 ];
	Red::System::SNPrintF( txt, ARRAY_COUNT( txt ), TXT("%d"), (Int32)value );
	RETURN_STRING( txt );
}

void OpIntToBool( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Int32 value = 0;
	stack.Step( context, &value );
	RETURN_BOOL( value != 0 );
}

void OpIntToByte( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Int32 value = 0;
	stack.Step( context, &value );
	Uint8 clamped = (Uint8)( Clamp< Int32 >( value, 0, 255 ) );
	RETURN_BYTE( clamped );
}

void OpIntToFloat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Int32 value = 0;
	stack.Step( context, &value );
	RETURN_FLOAT( (Float)value );
}

void OpIntToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Int32 value = 0;
	stack.Step( context, &value );
	Char txt[ 32 ];
	Red::System::SNPrintF( txt, ARRAY_COUNT( txt ), TXT("%d"), value );
	RETURN_STRING( txt );
}

void OpFloatToBool( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Float value = 0.0f;
	stack.Step( context, &value );
	RETURN_BOOL( value != 0.0f );
}

void OpFloatToByte( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Float value = 0;
	stack.Step( context, &value );
	Uint8 clamped = (Uint8)( Clamp< Float >( value, 0, 255 ) );
	RETURN_BYTE( clamped );
}

void OpFloatToInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Float value = 0.0f;
	stack.Step( context, &value );
	RETURN_INT( (Int32)value );
}

void OpFloatToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	Float value = 0;
	stack.Step( context, &value );
	Char txt[ 32 ];
	Red::System::SNPrintF( txt, ARRAY_COUNT( txt ), TXT("%f"), value );
	RETURN_STRING( txt );
}

void OpNameToBool( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	CName value;
	stack.Step( context, &value );
	RETURN_BOOL( value != CName::NONE );
}

void OpNameToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	CName value;
	stack.Step( context, &value );
	RETURN_STRING( value.AsString() );
}

void OpStringToBool( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	String value;
	stack.Step( context, &value );
	Bool retValue = false;
	FromString< Bool >( value, retValue );
	RETURN_BOOL( retValue );
}

void OpStringToByte( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	String value;
	stack.Step( context, &value );
	Int32 retValue = 0;
	FromString< Int32 >( value, retValue );
	retValue = Clamp< Int32 >( retValue, 0, 255 );
	RETURN_BYTE( (Uint8)retValue );
}

void OpStringToInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	String value;
	stack.Step( context, &value );
	Int32 retValue = 0;
	FromString< Int32 >( value, retValue );
	RETURN_INT( retValue );
}

void OpStringToFloat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	String value;
	stack.Step( context, &value );
	Float retValue = 0.0f;
	FromString< Float >( value, retValue );
	RETURN_FLOAT( retValue );
}

void OpObjectToBool( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	THandle< IScriptable > value;
	stack.Step( context, &value );
	RETURN_BOOL( value.Get() == NULL ? false : true );
}

void OpObjectToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	THandle< IScriptable > value;
	stack.Step( context, &value );
	IScriptable *pValue = value.Get();
	if ( pValue )
	{
		RETURN_STRING( pValue->GetFriendlyName() );
	}
	else
	{
		RETURN_STRING( TXT("NULL") );
	}
}

void OpEnumToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get the types
	TUniPointer<IRTTIType> type = stack.Read< TUniPointer<IRTTIType> >();

	const Uint32 typeSize = type->GetSize();
	Uint8* buffer = (Uint8*) RED_ALLOCA( typeSize );
	stack.Step( context, buffer );

	String string;
	type->ToString( buffer, string );
	RETURN_STRING( string );
}

void OpEnumToInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get the types
	TUniPointer<IRTTIType> type = stack.Read< TUniPointer<IRTTIType> >();

	void* ptr = RED_ALLOCA( type->GetSize() );
	stack.Step( context, ptr );

	Int32 val = -1;
	switch ( type->GetSize() )
	{
		case 1: 
			val = *(Int8*)ptr;
			break;
		case 2: 
			val =  *(Int16*)ptr;
			break;
		case 4: 
			val = *(Int32*)ptr;
			break;
	}	
	RETURN_INT( val );
}

void OpIntToEnum( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Read the enum type
	TUniPointer<IRTTIType> type = stack.Read< TUniPointer<IRTTIType> >();

	// Read the int
	Int32 value = 0;
	stack.Step( context, &value );
	
	// There are 3 enum sizes possible:
	switch ( type->GetSize() )
	{
	case 1: 
		*( Int8* ) result = ( Int8 ) value;
		break;
	case 2: 
		*( Int16* ) result = ( Int16 ) value;
		break;
	case 4: 
		*( Int32* ) result = ( Int32 ) value;
		break;
	}

}

void OpDynamicCast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Read the cast type
	TUniPointer< IRTTIType > castToType = stack.Read< TUniPointer< IRTTIType > >();
	ASSERT( castToType && castToType->GetType() == RT_Class );
	CClass* castToClass = static_cast< CClass* >( castToType.Get() );

	// Read handle
	THandle< IScriptable > handle;
	stack.Step( context, &handle );

	// Return casted handle
	if ( result )
	{
		// Get object
		IScriptable* object = handle.Get();
		if ( object && !object->IsA( castToClass ) )
		{
			object = NULL;
		}

		// Copy to result
		THandle< IScriptable > newHandle( object );
		(*( THandle< IScriptable >* ) result) = newHandle;
	}
}

void OpConstructor( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get number of parameters
	Uint8 numParameters = stack.Read<Uint8>();

	// Get the type being constructed
	TUniPointer<IRTTIType> type = stack.Read< TUniPointer<IRTTIType> >();
	ASSERT( type->GetType() == RT_Class );

	// No result pointer given, skip parameters
	if ( !result )
	{
		for ( Uint32 i=0; i<numParameters; i++ )
		{
			stack.Step( context, NULL );
		}
		return;
	}

	// Construct type
	type->Construct( result );

	// Get properties to initialize
	CClass* classType = static_cast< CClass* >( type.Get() );
	TDynArray< CProperty* > properties;
	properties.Reserve( classType->GetLocalProperties().Size() );
	classType->GetProperties( properties );

	// Initialize structure properties
	ASSERT( numParameters <= properties.Size() );
	for ( Uint32 i=0; i<numParameters; i++ )
	{
		// Read value
		void* targetData = properties[i]->GetOffsetPtr( result );
		stack.Step( context, targetData );
	}
}

void OpDelete( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Get the handle to object to destroy
	THandle< IScriptable > handle;
	stack.Step( context, &handle );

	CObject* object = Cast< CObject >( handle.Get() );
	if ( object )
	{
		// We cannot destroy object that were not created by script
		if ( object->HasFlag( OF_ScriptCreated ) )
		{
			// This will invalidate handles to object and release object memory
			object->Discard();
		}
		else
		{
			SCRIPT_ERROR( stack, TXT( "Cannot delete object '%" ) RED_PRIWs TXT( "': was not created by script" ), object->GetFriendlyName().AsChar() );
		}
	}
}

// helper class that filters which scriptable classes can have memory managment enabled and which not (tempshit)
class CScriptMemoryManagmentFilter
{
public:
	THashMap< const CClass*, Bool >		m_allowed;
	THashMap< const CClass*, Bool >		m_visited;

	CScriptMemoryManagmentFilter()
	{
		//m_allowed.Set( ClassID< IScriptable >(), false );		
	}

	void AddClass( const Char* className )
	{
		const CClass* objectClass = SRTTI::GetInstance().FindClass( CName( className ) );
		if ( nullptr != objectClass )
		{
			m_allowed.Set( objectClass, true );
		}
	}

	const Bool CanMemoryManager( const CClass* objectClass )
	{
		// get from cache
		Bool reportFlag = true;
		if ( m_visited.Find( objectClass, reportFlag ) )
		{
			return reportFlag;
		}

		// return true if given class is allowed to be memory managed by THandle system
		const CClass* curClass = objectClass;
		while ( nullptr != curClass )
		{
			Bool canManage = false;
			if ( m_allowed.Find( curClass, canManage ) )
			{
				reportFlag = canManage;
				break;
			}

			curClass = curClass->GetBaseClass();
		}

		// stats
		LOG_CORE( TXT("Class '%ls' memory managment is '%ls'"), objectClass->GetName().AsChar(), reportFlag ? TXT("ENABLED") : TXT("DISABLED") );
		m_visited.Set( objectClass, reportFlag );
		return reportFlag;
	}

	static CScriptMemoryManagmentFilter& GetInstance()
	{
		static CScriptMemoryManagmentFilter theInstance;
		return theInstance;
	}
};

void OpNew( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Read class
	TUniPointer< CClass > objectClass = stack.Read< TUniPointer< CClass > >();

	// Get the parent object context
	THandle< IScriptable > parentObject;
	stack.Step( context, &parentObject );

	// Create object
	IScriptable* newObject = nullptr;
	if ( objectClass->IsObject() )
	{
		// Get the context (always a CObject)
		CObject* realParentObject = Cast< CObject >( parentObject.Get() );

		// Create object
		newObject = ::CreateObject<CObject>( objectClass.Get(), realParentObject, OF_ScriptCreated );
		SCRIPT_ERROR_CONDITION( !newObject, stack, TXT( "Unable to create object '%ls'" ), objectClass->GetName().AsChar() );
	}
	else
	{
		// Create object directly
		newObject = objectClass.Get()->CreateObject< IScriptable >();
	}

	// Warn
	SCRIPT_ERROR_CONDITION( !newObject, stack, TXT( "Unable to create object '%ls'" ), objectClass->GetName().AsChar() );

	// Enable memory management
	if ( newObject != nullptr )
	{
		const Bool canMemoryManage = CScriptMemoryManagmentFilter::GetInstance().CanMemoryManager( objectClass.Get() );
		if ( canMemoryManage )
		{
			if( ! newObject->IsHandleReferenceCounted() ) // MS: this check is called to avoid assert on enabling if enable was already called in constructor
			{
				newObject->EnableReferenceCounting();
			}
		}
	}

	// Write to result
	RETURN_OBJECT( newObject );
}

void OpThis( IScriptable* context, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( context );
}

void OpArrayClear( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Clean the array
	if ( CScriptStackFrame::s_offset )
	{
		arrayType->Clean( CScriptStackFrame::s_offset );
	}
}

void OpArraySize( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Clean the array
	if ( CScriptStackFrame::s_offset )
	{
		Uint32 size = arrayType->GetArraySize( CScriptStackFrame::s_offset );
		RETURN_INT( size );
	}
	else
	{
		RETURN_INT( 0 );
	}
}

void OpArrayPushBack( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// We have an array, allocate element
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate element
		Uint32 index = arrayType->AddArrayElement( arrayData, 1 );
		void* elementData = arrayType->GetArrayElement( arrayData, index );

		// Set element content
		stack.Step( context, elementData );
	}
	else
	{
		// Just evaluate the parameter
		stack.Step( context, NULL );
	}
}

void OpArrayPopBack( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );
	void* arrayData = CScriptStackFrame::s_offset;

	// Reset context
	CScriptStackFrame::s_offset = NULL;

	// Get the index of element to erase
	if ( arrayData )
	{
		// Check that range is valid
		const Int32 arraySize = arrayType->GetArraySize( arrayData );
		if ( arraySize < 1 )
		{
			// Info
#ifndef NO_ASSERTS
			SCRIPT_ERROR( stack, TXT( "Cannot pop element from empty array." ) );
			SCRIPT_DUMP_STACK_TO_LOG( stack );
#endif

			// Clear the result
			if ( result )
			{
				innerType->Destruct( result );
			}
		}
		else
		{
			// Write to result
			void* elementData = arrayType->GetArrayElement( arrayData, arraySize-1 );
			CScriptStackFrame::s_offset = elementData;

			// Write result
			if ( result )
			{
				innerType->Copy( result, elementData );
			}
			
			arrayType->DeleteArrayElement( arrayData, arraySize-1 );
		}
	}
	else
	{
		// Clear the result
		if ( result )
		{
			innerType->Destruct( result );
		}
	}
}

void OpArrayInsert( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );
	void* arrayData = CScriptStackFrame::s_offset;

	// Get index
	Int32 index = 0;
	stack.Step( context, &index );

	// Reset context
	CScriptStackFrame::s_offset = NULL;

	if ( arrayData )
	{
		// Check that range is valid
		const Int32 arraySize = arrayType->GetArraySize( arrayData );
		if ( index < 0 || index > arraySize )
		{
			// Info
			SCRIPT_WARN_ONCE( stack, TXT( "Index %i is out of array bounds. Size = %i" ), index, arraySize );

			// Just evaluate the parameter
			stack.Step( context, NULL );
		}
		else
		{
			// Allocate element
			arrayType->InsertArrayElementAt( arrayData, index );
			void* elementData = arrayType->GetArrayElement( arrayData, index );

			// Set element content
			stack.Step( context, elementData );
		}
	}
	else
	{
		// Just evaluate the parameter
		stack.Step( context, NULL );
	}
}

void OpArrayResize( IScriptable* context, CScriptStackFrame& stack, void* )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );
	void* arrayData = CScriptStackFrame::s_offset;

	// Get new array size
	Int32 arraySize = 0;
	stack.Step( context, &arraySize );

	// Array to small
	if ( arraySize < 0 )
	{
		SCRIPT_ERROR( stack, TXT( "Negative array size used in Resize: %i" ), arraySize );
		arraySize = 0;
	}

	// Resize array
	if ( arrayData )
	{
		Int32 currentSize = arrayType->GetArraySize( arrayData );
		if ( arraySize > currentSize )
		{
			const Int32 numItemsToAdd = arraySize - currentSize;
			arrayType->AddArrayElement( arrayData, numItemsToAdd );
		}
		else if ( arraySize < currentSize )
		{
			// Cleanup elements
			const Int32 numItemsToRemove = currentSize - arraySize;
			const Int32 firstElementToRemove = arraySize;
			for ( Int32 i=0; i<numItemsToRemove; i++ )
			{
				void* itemData = arrayType->GetArrayElement( arrayData, firstElementToRemove + i );
				arrayType->GetInnerType()->Destruct( itemData );
			}

			// Resize array
			CBaseArray* baseArray = ( CBaseArray* ) arrayData;
			baseArray->ShrinkBuffer( numItemsToRemove );
		}		
	}
}

void OpArrayRemove( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate space for element
		IRTTIType* innerType = arrayType->GetInnerType();
		void* elementData = RED_ALLOCA( innerType->GetSize() );
		innerType->Construct( elementData );

		// Read element
		stack.Step( context, elementData );

		// Find the element in the array
		Int32 elementIndex=-1;
		const Uint32 arraySize = arrayType->GetArraySize( arrayData );
		for ( Uint32 i=0; i<arraySize; i++ )
		{
			const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
			if ( innerType->Compare( arrayElementData, elementData, 0 ) )
			{
				elementIndex = i;
				break;				
			}
		}

		// Cleanup temporary shit
		innerType->Destruct( elementData );

		// Remove element
		if ( elementIndex != -1 )
		{
			arrayType->DeleteArrayElement( arrayData, elementIndex );
		}
		
		// Return true if element was removed
		RETURN_BOOL( elementIndex != -1 );
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_BOOL( false );
	}
}

void OpArrayRemoveFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get the element to remove
		CScriptStackFrame::s_offset = NULL;
		stack.Step( context, NULL );
		void* elementData = CScriptStackFrame::s_offset;
		if ( elementData )
		{
			// Find the element in the array
			Int32 elementIndex=-1;
			const Uint32 arraySize = arrayType->GetArraySize( arrayData );
			for ( Uint32 i=0; i<arraySize; i++ )
			{
				const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
				if ( innerType->Compare( arrayElementData, elementData, 0 ) )
				{
					elementIndex = i;
					break;				
				}
			}

			// Remove element
			if ( elementIndex != -1 )
			{
				arrayType->DeleteArrayElement( arrayData, elementIndex );
			}

			// Return true if element was removed
			RETURN_BOOL( elementIndex != -1 );
		}
		else
		{
			// Not removed
			RETURN_BOOL( false );
		}
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_BOOL( false );
	}
}

void OpArrayContains( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate space for element
		IRTTIType* innerType = arrayType->GetInnerType();
		void* elementData = RED_ALLOCA( innerType->GetSize() );
		innerType->Construct( elementData );

		// Read element
		stack.Step( context, elementData );

		// Find the element in the array
		Int32 elementIndex=-1;
		const Uint32 arraySize = arrayType->GetArraySize( arrayData );
		for ( Uint32 i=0; i<arraySize; i++ )
		{
			const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
			if ( innerType->Compare( arrayElementData, elementData, 0 ) )
			{
				elementIndex = i;
				break;				
			}
		}

		// Cleanup temporary shit
		innerType->Destruct( elementData );

		// Return true if element was removed
		RETURN_BOOL( elementIndex != -1 );
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_BOOL( false );
	}
}

void OpArrayContainsFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get the element to remove
		CScriptStackFrame::s_offset = NULL;
		stack.Step( context, NULL );
		void* elementData = CScriptStackFrame::s_offset;
		if ( elementData )
		{
			// Find the element in the array
			Int32 elementIndex=-1;
			const Uint32 arraySize = arrayType->GetArraySize( arrayData );
			for ( Uint32 i=0; i<arraySize; i++ )
			{
				const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
				if ( innerType->Compare( arrayElementData, elementData, 0 ) )
				{
					elementIndex = i;
					break;				
				}
			}

			// Return true if element was removed
			RETURN_BOOL( elementIndex != -1 );
		}
		else
		{
			// Not removed
			RETURN_BOOL( false );
		}
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_BOOL( false );
	}
}

void OpArrayFindFirst( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate space for element
		IRTTIType* innerType = arrayType->GetInnerType();
		void* elementData = RED_ALLOCA( innerType->GetSize() );
		innerType->Construct( elementData );

		// Read element
		stack.Step( context, elementData );

		// Find the element in the array
		Int32 elementIndex=-1;
		const Uint32 arraySize = arrayType->GetArraySize( arrayData );
		for ( Uint32 i=0; i<arraySize; i++ )
		{
			const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
			if ( innerType->Compare( arrayElementData, elementData, 0 ) )
			{
				elementIndex = i;
				break;				
			}
		}

		// Cleanup temporary shit
		innerType->Destruct( elementData );

		// Return index of the element
		RETURN_INT( elementIndex );
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not found
		RETURN_INT( -1 );
	}
}

void OpArrayFindFirstFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get the element to remove
		CScriptStackFrame::s_offset = NULL;
		stack.Step( context, NULL );
		void* elementData = CScriptStackFrame::s_offset;
		if ( elementData )
		{
			// Find the element in the array
			Int32 elementIndex=-1;
			const Uint32 arraySize = arrayType->GetArraySize( arrayData );
			for ( Uint32 i=0; i<arraySize; i++ )
			{
				const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
				if ( innerType->Compare( arrayElementData, elementData, 0 ) )
				{
					elementIndex = i;
					break;				
				}
			}

			// Return index of element
			RETURN_INT( elementIndex );
		}
		else
		{
			// Not found
			RETURN_INT( -1 );
		}
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_INT( -1 );
	}
}

void OpArrayFindLast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate space for element
		IRTTIType* innerType = arrayType->GetInnerType();
		void* elementData = RED_ALLOCA( innerType->GetSize() );
		innerType->Construct( elementData );

		// Read element
		stack.Step( context, elementData );

		// Find the element in the array
		Int32 elementIndex=-1;
		const Int32 arraySize = arrayType->GetArraySize( arrayData );
		for ( Int32 i=arraySize-1; i>=0; --i )
		{
			const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
			if ( innerType->Compare( arrayElementData, elementData, 0 ) )
			{
				elementIndex = i;
				break;				
			}
		}

		// Cleanup temporary shit
		innerType->Destruct( elementData );

		// Return index of the element
		RETURN_INT( elementIndex );
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not found
		RETURN_INT( -1 );
	}
}

void OpArrayFindLastFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get the element to remove
		CScriptStackFrame::s_offset = NULL;
		stack.Step( context, NULL );
		void* elementData = CScriptStackFrame::s_offset;
		if ( elementData )
		{
			// Find the element in the array
			Int32 elementIndex=-1;
			const Int32 arraySize = arrayType->GetArraySize( arrayData );
			for ( Int32 i=arraySize-1; i>=0; --i )
			{
				const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
				if ( innerType->Compare( arrayElementData, elementData, 0 ) )
				{
					elementIndex = i;
					break;				
				}
			}

			// Return index of element
			RETURN_INT( elementIndex );
		}
		else
		{
			// Not found
			RETURN_INT( -1 );
		}
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_INT( -1 );
	}
}

void OpArrayErase( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Get the index of element to erase
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get index
		Int32 index = 0;
		stack.Step( context, &index );

		// Erase if the index is valid
		const Int32 arraySize = arrayType->GetArraySize( arrayData );
		if ( index >= 0 && index < arraySize )
		{
			// Delete element
			arrayType->DeleteArrayElement( arrayData, index );
			RETURN_BOOL( true );
		}
		else
		{
			SCRIPT_ERROR( stack, TXT( "Cannot delete item in array with index %i (array only has %i elements!)" ), index, arraySize );
			RETURN_BOOL( false );
		}
	}
	else
	{
		stack.Step( context, NULL );
		RETURN_BOOL( false );
	}
}

void OpArrayEraseFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Get the index of element to erase
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get index
		Int32 index = 0;
		stack.Step( context, &index );

		if( arrayType->DeleteArrayElementFast( arrayData, index ) )
		{
			RETURN_BOOL( true );
		}
		else
		{
			const Int32 arraySize = arrayType->GetArraySize( arrayData );
			SCRIPT_ERROR( stack, TXT( "Cannot delete item in array with index %i (array only has %i elements!)" ), index, arraySize );
			RETURN_BOOL( false );
		}
	}
	else
	{
		stack.Step( context, NULL );
		RETURN_BOOL( false );
	}
}

void OpArrayLast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Return the last element
	void* arrayData = CScriptStackFrame::s_offset;
	const Uint32 size = arrayType->GetArraySize( arrayData );
	if ( size > 0 )
	{
		// Copy value
		if ( result )
		{
			IRTTIType* innerType = arrayType->GetInnerType();
			const void* elementData = arrayType->GetArrayElement( arrayData, size-1 );
			innerType->Copy( result, elementData );
		}
	}
	else
	{
		SCRIPT_WARNING( stack, TXT( "Cannot call Last() on an array with no elements" ) );
	}
}

void OpArrayGrow( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Get the index of element to erase
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get size to alloc
		Int32 growSize = 0;
		stack.Step( context, &growSize );

		// Add elements
		if ( growSize > 0 )
		{
			Int32 firstIndex = arrayType->AddArrayElement( arrayData, growSize );
			RETURN_INT( firstIndex );
		}
		else
		{
			Int32 size = arrayType->GetArraySize( arrayData );
			RETURN_INT( size );
		}
	}
	else
	{
		stack.Step( context, NULL );
		RETURN_INT( -1 );
	}
}

void OpArrayElement( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTIArrayType> arrayType = stack.Read< TUniPointer<CRTTIArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );
	void* arrayData = CScriptStackFrame::s_offset;

	// Evaluate index
	Int32 index=0;
	stack.Step( context, &index );

	// Reset context
	CScriptStackFrame::s_offset = NULL;

	// Get the index of element to erase
	if ( arrayData )
	{
		// Check that range is valid
		const Int32 arraySize = arrayType->GetArraySize( arrayData );
		if ( index < 0 || index >= arraySize )
		{
			// Info
			SCRIPT_WARN_ONCE( stack, TXT( "Index %i is out of array bounds. Size = %i" ), index, arraySize );

			// Clear the result
			if ( result )
			{
				innerType->Destruct( result );
			}
		}
		else
		{
			// Write to result
			void* elementData = arrayType->GetArrayElement( arrayData, index );
			CScriptStackFrame::s_offset = elementData;

			// Write result
			if ( result )
			{
				innerType->Copy( result, elementData );
			}
		}
	}
	else
	{
		// Clear the result
		if ( result )
		{
			innerType->Destruct( result );
		}
	}
}

void OpStaticArraySize( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Clean the array
	if ( CScriptStackFrame::s_offset )
	{
		Uint32 size = arrayType->GetArraySize( CScriptStackFrame::s_offset );
		RETURN_INT( size );
	}
	else
	{
		RETURN_INT( 0 );
	}
}

void OpStaticArrayContains( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate space for element
		IRTTIType* innerType = arrayType->GetInnerType();
		void* elementData = RED_ALLOCA( innerType->GetSize() );
		innerType->Construct( elementData );

		// Read element
		stack.Step( context, elementData );

		// Find the element in the array
		Int32 elementIndex=-1;
		const Uint32 arraySize = arrayType->GetArraySize( arrayData );
		for ( Uint32 i=0; i<arraySize; i++ )
		{
			const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
			if ( innerType->Compare( arrayElementData, elementData, 0 ) )
			{
				elementIndex = i;
				break;				
			}
		}

		// Cleanup temporary shit
		innerType->Destruct( elementData );

		// Return true if element was removed
		RETURN_BOOL( elementIndex != -1 );
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_BOOL( false );
	}
}

void OpStaticArrayContainsFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get the element to remove
		CScriptStackFrame::s_offset = NULL;
		stack.Step( context, NULL );
		void* elementData = CScriptStackFrame::s_offset;
		if ( elementData )
		{
			// Find the element in the array
			Int32 elementIndex=-1;
			const Uint32 arraySize = arrayType->GetArraySize( arrayData );
			for ( Uint32 i=0; i<arraySize; i++ )
			{
				const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
				if ( innerType->Compare( arrayElementData, elementData, 0 ) )
				{
					elementIndex = i;
					break;				
				}
			}

			// Return true if element was removed
			RETURN_BOOL( elementIndex != -1 );
		}
		else
		{
			// Not removed
			RETURN_BOOL( false );
		}
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_BOOL( false );
	}
}

void OpStaticArrayFindFirst( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate space for element
		IRTTIType* innerType = arrayType->GetInnerType();
		void* elementData = RED_ALLOCA( innerType->GetSize() );
		innerType->Construct( elementData );

		// Read element
		stack.Step( context, elementData );

		// Find the element in the array
		Int32 elementIndex=-1;
		const Uint32 arraySize = arrayType->GetArraySize( arrayData );
		for ( Uint32 i=0; i<arraySize; i++ )
		{
			const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
			if ( innerType->Compare( arrayElementData, elementData, 0 ) )
			{
				elementIndex = i;
				break;				
			}
		}

		// Cleanup temporary shit
		innerType->Destruct( elementData );

		// Return index of the element
		RETURN_INT( elementIndex );
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not found
		RETURN_INT( -1 );
	}
}

void OpStaticArrayFindFirstFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get the element to remove
		CScriptStackFrame::s_offset = NULL;
		stack.Step( context, NULL );
		void* elementData = CScriptStackFrame::s_offset;
		if ( elementData )
		{
			// Find the element in the array
			Int32 elementIndex=-1;
			const Uint32 arraySize = arrayType->GetArraySize( arrayData );
			for ( Uint32 i=0; i<arraySize; i++ )
			{
				const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
				if ( innerType->Compare( arrayElementData, elementData, 0 ) )
				{
					elementIndex = i;
					break;				
				}
			}

			// Return index of element
			RETURN_INT( elementIndex );
		}
		else
		{
			// Not found
			RETURN_INT( -1 );
		}
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_INT( -1 );
	}
}

void OpStaticArrayFindLast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Allocate space for element
		IRTTIType* innerType = arrayType->GetInnerType();
		void* elementData = RED_ALLOCA( innerType->GetSize() );
		innerType->Construct( elementData );

		// Read element
		stack.Step( context, elementData );

		// Find the element in the array
		Int32 elementIndex=-1;
		const Int32 arraySize = arrayType->GetArraySize( arrayData );
		for ( Int32 i=arraySize-1; i>=0; --i )
		{
			const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
			if ( innerType->Compare( arrayElementData, elementData, 0 ) )
			{
				elementIndex = i;
				break;				
			}
		}

		// Cleanup temporary shit
		innerType->Destruct( elementData );

		// Return index of the element
		RETURN_INT( elementIndex );
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not found
		RETURN_INT( -1 );
	}
}

void OpStaticArrayFindLastFast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Remove from array
	void* arrayData = CScriptStackFrame::s_offset;
	if ( arrayData )
	{
		// Get the element to remove
		CScriptStackFrame::s_offset = NULL;
		stack.Step( context, NULL );
		void* elementData = CScriptStackFrame::s_offset;
		if ( elementData )
		{
			// Find the element in the array
			Int32 elementIndex=-1;
			const Int32 arraySize = arrayType->GetArraySize( arrayData );
			for ( Int32 i=arraySize-1; i>=0; --i )
			{
				const void* arrayElementData = arrayType->GetArrayElement( arrayData, i );
				if ( innerType->Compare( arrayElementData, elementData, 0 ) )
				{
					elementIndex = i;
					break;				
				}
			}

			// Return index of element
			RETURN_INT( elementIndex );
		}
		else
		{
			// Not found
			RETURN_INT( -1 );
		}
	}
	else
	{
		// Skip element
		stack.Step( context, NULL );

		// Not removed
		RETURN_INT( -1 );
	}
}

void OpStaticArrayLast( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );

	// Return the last element
	void* arrayData = CScriptStackFrame::s_offset;
	const Uint32 size = arrayType->GetArraySize( arrayData );
	if ( size > 0 )
	{
		// Copy value
		if ( result )
		{
			IRTTIType* innerType = arrayType->GetInnerType();
			const void* elementData = arrayType->GetArrayElement( arrayData, size-1 );
			innerType->Copy( result, elementData );
		}
	}
	else
	{
		SCRIPT_WARNING( stack, TXT( "Cannot call Last() on an array with no elements" ) );
	}
}

void OpStaticArrayElement( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	// Get array type
	TUniPointer<CRTTINativeArrayType> arrayType = stack.Read< TUniPointer<CRTTINativeArrayType> >();
	ASSERT( arrayType && arrayType->GetType() == RT_Array );
	IRTTIType* innerType = arrayType->GetInnerType();

	// Get the pointer to array
	CScriptStackFrame::s_offset = NULL;
	stack.Step( context, NULL );
	void* arrayData = CScriptStackFrame::s_offset;

	// Evaluate index
	Int32 index=0;
	stack.Step( context, &index );

	// Reset context
	CScriptStackFrame::s_offset = NULL;

	// Get the index of element to erase
	if ( arrayData )
	{
		// Check that range is valid
		const Int32 arraySize = arrayType->GetArraySize( arrayData );
		if ( index < 0 || index >= arraySize )
		{
			// Info
			SCRIPT_WARN_ONCE( stack, TXT( "Index %i is out of array bounds. Size = %i" ), index, arraySize );

			// Clear the result
			if ( result )
			{
				innerType->Destruct( result );
			}
		}
		else
		{
			// Write to result
			void* elementData = arrayType->GetArrayElement( arrayData, index );
			CScriptStackFrame::s_offset = elementData;

			// Write result
			if ( result )
			{
				innerType->Copy( result, elementData );
			}
		}
	}
	else
	{
		// Clear the result
		if ( result )
		{
			innerType->Destruct( result );
		}
	}
}

void OpSavePoint( IScriptable*, CScriptStackFrame& stack, void* )
{
	// read the offset of the SavePoint end
	Uint16 skipSize = stack.Read< Uint16 >();

	if ( !stack.m_thread || !stack.m_thread->QuerySerializer() )
	{
		const Uint8* codeSkip = stack.m_code + skipSize; 
		stack.m_code = codeSkip;
	}
	else
	{
		// start saving data...
		CScriptThreadSerializer* saver = stack.m_thread->QuerySerializer();

		// get the name of the SavePoint
		CName savepointName = stack.Read< CName >();

		// iterate through subsequent opcodes, until
		// the end-of-data-block marker is reached, 
		// gathering all parameters the values of which we want saved
		TDynArray< SSavePointValue* > params;
		const CFunction* function = stack.m_function;
		while ( stack.Read< Uint8 >() != OP_SavePointEnd )
		{
			// save the properties
			CName paramName = stack.Read< CName >();
			CProperty* property = function->FindProperty( paramName );
			if ( property )
			{
				params.PushBack( new SSavePointValue( stack, *property ) );
			}
		}

		// record the state in the serializer
		saver->Record( savepointName, params );
	}
}

void OpGetGame( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_GAME ) );
}

void OpGetPlayer( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_PLAYER ) );
}

void OpGetCamera( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_CAMERA ) );
}

void OpGetHud( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_HUD ) );
}

void OpGetSound( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_SOUND ) );
}

void OpGetDebug( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_DEBUG ) );
}

void OpGetTelemetry( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_TELEMETRY ) );
}

void OpGetTimer( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_TIMER ) );
}

void OpGetInput( IScriptable*, CScriptStackFrame&, void* result )
{
	RETURN_OBJECT( GScriptingSystem->GetGlobal( CScriptingSystem::GP_INPUT ) );
}


#define SET_OPCODE( x, y )	\
	functionMap.SetOpcode( x, (TNativeGlobalFunc) &y );

void ExportCoreOpcodes( CScriptNativeFunctionMap& functionMap )
{
	// Register opcodes
	SET_OPCODE( OP_Nop, OpNop );
	SET_OPCODE( OP_Null, OpNull );
	SET_OPCODE( OP_IntOne, OpIntOne );
	SET_OPCODE( OP_IntZero,	OpIntZero );
	SET_OPCODE( OP_IntConst, OpIntConst );
	SET_OPCODE( OP_ShortConst, OpShortConst );
	SET_OPCODE( OP_StringConst,	OpStringConst );
	SET_OPCODE( OP_NameConst, OpNameConst );
	SET_OPCODE( OP_ByteConst, OpByteConst );
	SET_OPCODE( OP_FloatConst, OpFloatConst );
	SET_OPCODE( OP_BoolTrue, OpBoolTrue );
	SET_OPCODE( OP_BoolFalse, OpBoolFalse );
	SET_OPCODE( OP_Breakpoint, OpBreakpoint );
	SET_OPCODE( OP_VirtualFunc, OpVirtualFunction );
	SET_OPCODE( OP_FinalFunc, OpFinalFunction );
	SET_OPCODE( OP_VirtualParentFunc, OpVirtualParentFunction );
	SET_OPCODE( OP_EntryFunc, OpEntryFunction );
	SET_OPCODE( OP_Return, OpReturn );
	SET_OPCODE( OP_LocalVar, OpLocalVar );
	SET_OPCODE( OP_ParamVar, OpParamVar );
	SET_OPCODE( OP_ObjectVar, OpObjectVar );
	SET_OPCODE( OP_ObjectBindableVar, OpObjectBindableVar );
	SET_OPCODE( OP_DefaultVar, OpDefaultVar );
	SET_OPCODE( OP_Context, OpContext );
	SET_OPCODE( OP_StructMember, OpStructMember );
	SET_OPCODE( OP_Assign, OpAssign );
	SET_OPCODE( OP_Jump, OpJump );
	SET_OPCODE( OP_JumpIfFalse, OpJumpIfFalse );
	SET_OPCODE( OP_TestEqual, OpTestEqual );
	SET_OPCODE( OP_TestNotEqual, OpTestNotEqual );
	SET_OPCODE( OP_Constructor, OpConstructor );
	SET_OPCODE( OP_Delete, OpDelete );
	SET_OPCODE( OP_New, OpNew );
	SET_OPCODE( OP_This, OpThis );
	SET_OPCODE( OP_Parent, OpParent );
	SET_OPCODE( OP_SavePoint, OpSavePoint );
	SET_OPCODE( OP_Switch, OpSwitch );
	SET_OPCODE( OP_SwitchDefault, OpSwitchCaseDefault );
	SET_OPCODE( OP_SwitchLabel, OpSwitchCase );

	// Dynamic array
	SET_OPCODE( OP_ArrayClear, OpArrayClear );
	SET_OPCODE( OP_ArraySize, OpArraySize );
	SET_OPCODE( OP_ArrayPushBack, OpArrayPushBack );
	SET_OPCODE( OP_ArrayPopBack, OpArrayPopBack );
	SET_OPCODE( OP_ArrayInsert, OpArrayInsert );
	SET_OPCODE( OP_ArrayResize, OpArrayResize );
	SET_OPCODE( OP_ArrayRemove, OpArrayRemove );
	SET_OPCODE( OP_ArrayRemoveFast, OpArrayRemoveFast );
	SET_OPCODE( OP_ArrayContains, OpArrayContains );
	SET_OPCODE( OP_ArrayContainsFast, OpArrayContainsFast );
	SET_OPCODE( OP_ArrayFindFirst, OpArrayFindFirst );
	SET_OPCODE( OP_ArrayFindFirstFast, OpArrayFindFirstFast );
	SET_OPCODE( OP_ArrayFindLast, OpArrayFindLast );
	SET_OPCODE( OP_ArrayFindLastFast, OpArrayFindLastFast );
	SET_OPCODE( OP_ArrayGrow, OpArrayGrow );
	SET_OPCODE( OP_ArrayErase, OpArrayErase );
	SET_OPCODE( OP_ArrayEraseFast, OpArrayEraseFast );
	SET_OPCODE( OP_ArrayLast, OpArrayLast );
	SET_OPCODE( OP_ArrayElement, OpArrayElement );

	// Static array
	SET_OPCODE( OP_StaticArraySize, OpStaticArraySize );
	SET_OPCODE( OP_StaticArrayContains, OpStaticArrayContains );
	SET_OPCODE( OP_StaticArrayContainsFast, OpStaticArrayContainsFast );
	SET_OPCODE( OP_StaticArrayFindFirst, OpStaticArrayFindFirst );
	SET_OPCODE( OP_StaticArrayFindFirstFast, OpStaticArrayFindFirstFast );
	SET_OPCODE( OP_StaticArrayFindLast, OpStaticArrayFindLast );
	SET_OPCODE( OP_StaticArrayFindLastFast, OpStaticArrayFindLastFast );
	SET_OPCODE( OP_StaticArrayLast, OpStaticArrayLast );
	SET_OPCODE( OP_StaticArrayElement, OpStaticArrayElement );

	// Casts
	SET_OPCODE( OP_BoolToByte, OpBoolToByte )
	SET_OPCODE( OP_BoolToInt, OpBoolToInt )
	SET_OPCODE( OP_BoolToFloat, OpBoolToFloat )
	SET_OPCODE( OP_BoolToString, OpBoolToString )
	SET_OPCODE( OP_ByteToBool, OpByteToBool )
	SET_OPCODE( OP_ByteToInt, OpByteToInt )
	SET_OPCODE( OP_ByteToFloat, OpByteToFloat )
	SET_OPCODE( OP_ByteToString, OpByteToString )
	SET_OPCODE( OP_IntToBool, OpIntToBool )
	SET_OPCODE( OP_IntToByte, OpIntToByte )
	SET_OPCODE( OP_IntToFloat, OpIntToFloat )
	SET_OPCODE( OP_IntToString, OpIntToString )
	SET_OPCODE( OP_IntToEnum, OpIntToEnum )
	SET_OPCODE( OP_FloatToBool, OpFloatToBool )
	SET_OPCODE( OP_FloatToByte, OpFloatToByte )
	SET_OPCODE( OP_FloatToInt, OpFloatToInt )
	SET_OPCODE( OP_FloatToString, OpFloatToString )
	SET_OPCODE( OP_NameToBool, OpNameToBool )
	SET_OPCODE( OP_NameToString, OpNameToString )
	SET_OPCODE( OP_StringToBool, OpStringToBool )
	SET_OPCODE( OP_StringToByte, OpStringToByte )
	SET_OPCODE( OP_StringToInt, OpStringToInt )
	SET_OPCODE( OP_StringToFloat, OpStringToFloat )
	SET_OPCODE( OP_ObjectToBool, OpObjectToBool )
	SET_OPCODE( OP_ObjectToString, OpObjectToString )
	SET_OPCODE( OP_EnumToString, OpEnumToString )
	SET_OPCODE( OP_EnumToInt, OpEnumToInt )
	SET_OPCODE( OP_DynamicCast, OpDynamicCast )

	// Globals
	SET_OPCODE( OP_GetGame, OpGetGame )
	SET_OPCODE( OP_GetPlayer, OpGetPlayer )
	SET_OPCODE( OP_GetCamera, OpGetCamera )
	SET_OPCODE( OP_GetHud, OpGetHud )
	SET_OPCODE( OP_GetSound, OpGetSound )
	SET_OPCODE( OP_GetDebug, OpGetDebug )
	SET_OPCODE( OP_GetTimer, OpGetTimer )
	SET_OPCODE( OP_GetInput, OpGetInput )
	SET_OPCODE( OP_GetTelemetry, OpGetTelemetry );
}
