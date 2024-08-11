/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameTime.h"
#include "../core/scriptStackFrame.h"
#include "game.h"
#include "gameTimeManager.h"


const GameTime GameTime::DAY( 1, 0, 0, 0 );
const GameTime GameTime::HOUR( 0, 1, 0, 0 );
const GameTime GameTime::MINUTE( 0, 0, 1, 0 );

IMPLEMENT_ENGINE_CLASS( GameTime );
IMPLEMENT_ENGINE_CLASS( GameTimeWrapper );

String GameTime::ToString() const
{
	Uint32 days = Days();
	Uint32 hours = Hours();
	Uint32 minutes = Minutes();
	Uint32 seconds = Seconds();
	if ( days > 0 )
	{
		return String::Printf(TXT("Days: %u, Hours: %u, Mins: %u, Secs: %u"), days, hours, minutes, seconds );
	}
	else if ( hours > 0 )
	{
		return String::Printf(TXT("Hours: %u, Mins: %u, Secs: %u"), hours, minutes, seconds );
	}
	else if ( minutes > 0)
	{
		return String::Printf(TXT("Minutes: %u, Seconds: %u"), minutes, seconds );
	}
	else
	{
		return String::Printf(TXT("Seconds: %u"), seconds );
	}
}

String GameTime::ToPreciseString() const
{
	Uint32 days = Days();
	Uint32 hours = Hours();
	Uint32 minutes = Minutes();
	Uint32 seconds = Seconds();

	return String::Printf(TXT("%02u:%02u:%02u:%02u"), days, hours, minutes, seconds );
}

static void funcGameTimeSeconds( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, time, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( time.Seconds() );
};

static void funcGameTimeMinutes( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, time, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( time.Minutes() );
};

static void funcGameTimeHours( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, time, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( time.Hours() );
};

static void funcGameTimeDays( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, time, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( time.Days() );
};

static void funcGameTimeToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, time, 0 );
	FINISH_PARAMETERS;
	RETURN_STRING( time.ToString() );
};

static void funcGameTimeToSeconds( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, time, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( time.GetSeconds() );
};

static void funcGameTimeCreate( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	#define INT_NOT_USED -10000000

	GET_PARAMETER_OPT( Int32, a, INT_NOT_USED );
	GET_PARAMETER_OPT( Int32, b, INT_NOT_USED );
	GET_PARAMETER_OPT( Int32, c, INT_NOT_USED );
	GET_PARAMETER_OPT( Int32, d, INT_NOT_USED );
	FINISH_PARAMETERS;

	// No parameters specified, use current time
	if ( a == INT_NOT_USED )
	{
		if ( GGame->IsActive() )
		{
			RETURN_STRUCT( GameTime, GGame->GetTimeManager()->GetTime() );
		}
		else
		{
			RETURN_STRUCT( GameTime, GameTime(0) );
		}
	}
	else if ( b == INT_NOT_USED )
	{
		// Only seconds given
		RETURN_STRUCT( GameTime, a );
	}	
	else if ( c == INT_NOT_USED )
	{
		// Seconds and minutes given
		RETURN_STRUCT( GameTime, GameTime( 0, 0, a, b ) );
	}
	else if ( d == INT_NOT_USED )
	{
		// Seconds, minutes and hours given
		RETURN_STRUCT( GameTime, GameTime( 0, a, b, c ) );
	}
	else
	{
		// Seconds, minutes, hours and days given
		RETURN_STRUCT( GameTime, GameTime( a, b, c, d ) );
	}
};

static void funcAddGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, a + b );
}

static void funcSubGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, a - b );
}

static void funcAddGameTimeInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, a + b );
}

static void funcSubGameTimeInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, a - b );
}

static void funcMulGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( Float, b, 1.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, a * b );
}

static void funcDivGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( Float, b, 1.0f );
	FINISH_PARAMETERS;
	if ( b ) a = a * ( 1.0f / b );
	RETURN_STRUCT( GameTime, a );
}

static void funcNegGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	FINISH_PARAMETERS;
	RETURN_STRUCT( GameTime, -a.GetSeconds() );
}

static void funcEqualGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a == b );
}

static void funcNotEqualGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( !(a == b) );
}

static void funcLessGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a < b );
}

static void funcLessEqualGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a <= b );
}

static void funcGreaterGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a > b );
}

static void funcGreaterEqualGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a >= b );
}

static void funcAssignAddGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	a += b;
	RETURN_STRUCT( GameTime, a );
}

static void funcAssignSubGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( GameTime, a, 0 );
	GET_PARAMETER( GameTime, b, 0 );
	FINISH_PARAMETERS;
	a -= b;
	RETURN_STRUCT( GameTime, a );
}

static void funcAssignAddGameTimeInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( GameTime, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	a += b;
	RETURN_STRUCT( GameTime, a );
}

static void funcAssignSubGameTimeInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( GameTime, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	a -= b;
	RETURN_STRUCT( GameTime, a );
}

static void funcAssignMulGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( GameTime, a, 0 );
	GET_PARAMETER( Float, b, 1.0f );
	FINISH_PARAMETERS;
	a = a * b;
	RETURN_STRUCT( GameTime, a );
}

static void funcAssignDivGameTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( GameTime, a, 0 );
	GET_PARAMETER( Float, b, 1.0f );
	FINISH_PARAMETERS;
	if ( b ) a = a * ( 1.0f / b );
	RETURN_STRUCT( GameTime, a );
}

void ExportEngineGameTimeNatives()
{
	// Functions
	NATIVE_GLOBAL_FUNCTION( "GameTimeSeconds",		funcGameTimeSeconds );
	NATIVE_GLOBAL_FUNCTION( "GameTimeMinutes",		funcGameTimeMinutes );
	NATIVE_GLOBAL_FUNCTION( "GameTimeHours",		funcGameTimeHours );
	NATIVE_GLOBAL_FUNCTION( "GameTimeDays",			funcGameTimeDays );
	NATIVE_GLOBAL_FUNCTION( "GameTimeToString",		funcGameTimeToString );
	NATIVE_GLOBAL_FUNCTION( "GameTimeToSeconds",	funcGameTimeToSeconds );
	NATIVE_GLOBAL_FUNCTION( "GameTimeCreate",		funcGameTimeCreate );
	extern void funcScheduleTimeEvent( IScriptable* context, CScriptStackFrame& stack, void* result );
	NATIVE_GLOBAL_FUNCTION( "ScheduleTimeEvent",	funcScheduleTimeEvent );

	// Operators
	NATIVE_BINARY_OPERATOR( Add, funcAddGameTime, GameTime, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( Add, funcAddGameTimeInt, GameTime, GameTime, Int32 );
	NATIVE_BINARY_OPERATOR( Subtract, funcSubGameTime, GameTime, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( Subtract, funcSubGameTimeInt, GameTime, GameTime, Int32 );
	NATIVE_BINARY_OPERATOR( Multiply, funcMulGameTime, GameTime, GameTime, Float );
	NATIVE_BINARY_OPERATOR( Divide, funcDivGameTime, GameTime, GameTime, Float );
	NATIVE_UNARY_OPERATOR( Neg, funcNegGameTime, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( Equal, funcEqualGameTime, Bool, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( NotEqual, funcNotEqualGameTime, Bool, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( Less, funcLessGameTime, Bool, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( LessEqual, funcLessEqualGameTime, Bool, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( Greater, funcGreaterGameTime, Bool, GameTime, GameTime );
	NATIVE_BINARY_OPERATOR( GreaterEqual, funcGreaterEqualGameTime, Bool, GameTime, GameTime );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAdd, funcAssignAddGameTime, GameTime, GameTime, GameTime );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignSubtract, funcAssignSubGameTime, GameTime, GameTime, GameTime );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAdd, funcAssignAddGameTimeInt, GameTime, GameTime, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignSubtract, funcAssignSubGameTimeInt, GameTime, GameTime, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignMultiply, funcAssignMulGameTime, GameTime, GameTime, Float );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignDivide, funcAssignDivGameTime, GameTime, GameTime, Float );
}
