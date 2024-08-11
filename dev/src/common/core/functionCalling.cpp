/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "functionCalling.h"
#include "object.h"

#define SCRIPT_API_ERROR( message, ... )  RED_LOG_ERROR( RED_LOG_CHANNEL( ScriptApi ), MACRO_TXT( message ), ##__VA_ARGS__ ); RED_HALT( message, ##__VA_ARGS__ )

Bool CallFunction( IScriptable* context, CName functionName )
{
	const CFunction* function = NULL;
	if ( FindFunction( context, functionName, function ) )
	{
		CHECK_FUNCTION( context, function, 0 );

		// Call the function
		function->Call( context, NULL, NULL );
		return true;
	}

	// Not found
	return false;
}

Bool CheckFunction( IScriptable* context, const CFunction* function, Uint32 numParameters )
{
	// No function...
	if ( !function )
	{
		SCRIPT_API_ERROR( "Missing function to call" );
		return false;
	}

	// Non static functions require context
	if ( !function->IsStatic() )
	{
		if ( !context )
		{
			SCRIPT_API_ERROR( "Non static function '%" RED_PRIWs TXT("' requires a context (CObject). None given."), function->GetName().AsString().AsChar() );
			return false;
		}
	}

	// Check if has parameters
	if ( function->GetNumParameters() < numParameters )
	{
		SCRIPT_API_ERROR( "Trying to call function '%" RED_PRIWs TXT("' with too many parameters (%i, expects %i)"), function->GetName().AsChar(), numParameters, function->GetNumParameters() );
		return false;
	}

	// Check if has parameters
	if ( function->GetNumParameters() > numParameters )
	{
		// Check to see if the missing parameters are optional
		if( !( function->GetParameter( numParameters )->GetFlags() & PF_FuncOptionaParam ) )
		{
			SCRIPT_API_ERROR( "Trying to call function '%" RED_PRIWs TXT("' with too few parameters (%i, expects %i)"), function->GetName().AsChar(), numParameters, function->GetNumParameters() );
			return false;
		}
	}

	// Seems ok
	return true;
}

Bool CheckTypeForInput( const CFunction* function, const CProperty* scriptProperty, const IRTTIType* nativeType, Bool silentCheck = false )
{
	// note the cast order for INPUT to scripts: "c++ type" -> "property type (in scripts)"
	if ( SRTTI::GetInstance().CanCast( nativeType, scriptProperty->GetType() ) )
	{
		return true;
	}
	
	// Allow conversion from an int to an enum if an OK size. TBD: Sanity check int value is in enum
	if ( scriptProperty->GetType()->GetType() == RT_Enum 
		&& (   nativeType->GetName() == CNAME( Int32 ) 
			|| nativeType->GetName() == CNAME( Uint32 )
			|| nativeType->GetName() == CNAME( Int16 )
			|| nativeType->GetName() == CNAME( Uint16 )
			|| nativeType->GetName() == CNAME( Int8 )
			|| nativeType->GetName() == CNAME( Uint8 ) ) )
	{
		return true;
	}

	RED_UNUSED( function );
	if ( ! silentCheck )
	{
		// Error
		SCRIPT_API_ERROR
			(
			"Input parameter '%" RED_PRIWs TXT( "' in function '%" ) RED_PRIWs TXT( "' is of type '%" ) RED_PRIWs TXT( "', expected '%" ) RED_PRIWs TXT( "'" ),
			scriptProperty->GetName().AsChar(),
			function->GetName().AsChar(),
			nativeType->GetName().AsChar(),
			scriptProperty->GetType()->GetName().AsChar()
			);
	}

	return false;
}

Bool CheckTypeForOutput( const CFunction* function, const CProperty* scriptProperty, const IRTTIType* nativeType, Bool silentCheck = false )
{
	// note the cast order for OUTPUT from scripts: "property type (in scripts)" -> "c++ type"
	if ( SRTTI::GetInstance().CanCast( scriptProperty->GetType(), nativeType ) )
	{
		return true;
	}
	
	// Error
	RED_UNUSED( function );

	if ( ! silentCheck)
	{
		SCRIPT_API_ERROR
			(
			"Output parameter '%" RED_PRIWs TXT( "' in function '%" ) RED_PRIWs TXT( "' is of type '%" ) RED_PRIWs TXT( "', expected '%" ) RED_PRIWs TXT( "'" ),
			scriptProperty->GetName().AsChar(),
			function->GetName().AsChar(),
			nativeType->GetName().AsChar(),
			scriptProperty->GetType()->GetName().AsChar()
			);
	}

	return false;
}

Bool CheckFunctionReturnParameter( const CFunction* function, const IRTTIType* returnType )
{
	if ( !function->GetReturnValue() )
	{
		SCRIPT_API_ERROR( "Function '%" RED_PRIWs TXT( "' does not return any value."), function->GetName().AsString().AsChar() );
		return false;
	}

	CheckTypeForOutput( function, function->GetReturnValue(), returnType );

	// Seems ok
	return true;
}

Bool CheckFunctionParameter( const CFunction* function, Uint32 parameterIndex, const IRTTIType* returnType, Bool silentCheck /*= false */ )
{
	// Check if has parameters
	if ( function->GetNumParameters() < parameterIndex )
	{
		if ( ! silentCheck )
		{
			SCRIPT_API_ERROR( "Trying to add too many parameters to function '%" RED_PRIWs TXT( "'" ), function->GetName().AsChar(), parameterIndex, function->GetNumParameters() );
		}
		return false;
	}

	// Check type of parameter
	CProperty* param = function->GetParameter( parameterIndex );
	if ( !CheckTypeForInput( function, param, returnType, silentCheck ) )
	{
		return false;
	}

	// Additional checks for bi-directional or output parameters
	if ( param->GetType()->GetSize() !=  returnType->GetSize() )
	{
		if ( ! silentCheck )
		{
			// Error
			SCRIPT_API_ERROR
				(
				"Input parameter '%" RED_PRIWs TXT( "' in function '%" ) RED_PRIWs TXT( "' has size %i bytes, expected %i bytes" ) ,
				param->GetName().AsChar(),
				function->GetName().AsChar(),
				returnType->GetSize(),
				param->GetType()->GetSize()
				);
		}
		return false;
	}

	if ( ( param->GetFlags() & PF_FuncOutParam ) != 0 )
	{
		if ( !CheckTypeForOutput( function, param, returnType, silentCheck ) )
		{
			return false;
		}
	}

	// Seems ok
	return true;
}

Bool FindFunction( IScriptable*& context, CName functionName, const CFunction*& function )
{
	// Use global function
	if ( !context )
	{
		function = SRTTI::GetInstance().FindGlobalFunction( functionName );
		return function != NULL;
	}

	// Find function inside the context object
	function = context->FindFunction( context, functionName );
	return function != NULL;
}
