/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "scriptStackFrame.h"
#include "engineTime.h"

////////////////////////////////
// Integer operators
////////////////////////////////

void funcAddIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a + b );
}

void funcSubIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a - b );
}

void funcMulIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a * b );
}

void funcDivIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( b ? (a / b) : 0 );
}

void funcNegIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( -a );
}

void funcBitNotIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( ~a );
}

void funcAndIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a & b );
}

void funcOrIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a | b );
}

void funcXorIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a ^ b );
}

void funcModIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( b ? (a % b) : 0 );
}

void funcEqualIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a == b );
}

void funcNotEqualIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a != b );
}

void funcLessIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a < b );
}

void funcLessEqualIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a <= b );
}

void funcGreaterIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a > b );
}

void funcGreaterEqualIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a >= b );
}

void funcAssignAddIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a += b );
}

void funcAssignSubIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a -= b );
}

void funcAssignMulIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a *= b );
}

void funcAssignDivIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( b ? (a /= b) : (a = 0) );
}

void funcAssignAndIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a &= b );
}

void funcAssignOrIntInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_INT( a |= b );
}

////////////////////////////////
// 64bit Unsigned Integer operators
////////////////////////////////

void funcAddUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a + b );
}

void funcSubUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a - b );
}

void funcMulUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a * b );
}

void funcDivUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( b ? (a / b) : 0 );
}

void funcAndUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a & b );
}

void funcOrUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a | b );
}

void funcXorUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a ^ b );
}

void funcModUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( b ? (a % b) : 0 );
}

void funcEqualUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a == b );
}

void funcNotEqualUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a != b );
}

void funcLessUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a < b );
}

void funcLessEqualUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a <= b );
}

void funcGreaterUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a > b );
}

void funcGreaterEqualUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a >= b );
}

void funcAssignAddUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a += b );
}

void funcAssignSubUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a -= b );
}

void funcAssignMulUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a *= b );
}

void funcAssignDivUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( b ? (a /= b) : (a = 0) );
}

void funcAssignAndUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a &= b );
}

void funcAssignOrUint64Uint64( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint64, a, 0 );
	GET_PARAMETER( Uint64, b, 0 );
	FINISH_PARAMETERS;
	RETURN_UINT64( a |= b );
}

////////////////////////////////
// String operators
////////////////////////////////

void funcAddStringString( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, a, String::EMPTY );
	GET_PARAMETER( String, b, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( a + b );
}

void funcAssignAddStringString( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( String, a, String::EMPTY );
	GET_PARAMETER( String, b, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( a += b );
}

////////////////////////////////
// Float operators
////////////////////////////////

void funcAddFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( a + b );
}

void funcSubFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( a - b );
}

void funcMulFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( a * b );
}

void funcDivFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( b ? ( a / b ) : a );
}

void funcNegFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( -a );
}

void funcEqualFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a == b );
}

void funcNotEqualFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a != b );
}

void funcLessFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a < b );
}

void funcLessEqualFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a <= b );
}

void funcGreaterFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a > b );
}

void funcGreaterEqualFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a >= b );
}

void funcAssignAddFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( a += b );
}

void funcAssignSubFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( a -= b );
}

void funcAssignMulFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( a *= b );
}

void funcAssignDivFloatFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_FLOAT( b ? (a /= b) : (a = 0.0f) );
}

////////////////////////////////
// Byte operators
////////////////////////////////

static Uint8 ClampByte( Int32 a )
{
	return (Uint8)Clamp<Int32>( a, 0 ,255 );
}

void funcAddByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( ClampByte( (Int32)a + b ) );
}

void funcSubByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( ClampByte( (Int32)a - b ) );
}

void funcMulByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( ClampByte( (Int32)a * b ) );
}

void funcDivByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0);
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( b ? ClampByte( (Int32)a / b ) : 0 );
}

void funcAndByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( a & (Uint8)b );
}

void funcOrByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( a | (Uint8)b );
}

void funcXorByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( a ^ (Uint8)b );
}

void funcModByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( b ? ClampByte( (Int32)a % b ) : 0 );
}

void funcEqualByteByte( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Uint8, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a == b );
}

void funcNotEqualByteByte( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Uint8, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a != b );
}

void funcLessByteByte( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Uint8, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a < b );
}

void funcLessEqualByteByte( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Uint8, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a <= b );
}

void funcGreaterByteByte( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Uint8, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a > b );
}

void funcGreaterEqualByteByte( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint8, a, 0 );
	GET_PARAMETER( Uint8, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( a >= b );
}

void funcAssignAddByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	a = ClampByte( (Int32)a + b );
	RETURN_BYTE( a );
}

void funcAssignSubByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	a = ClampByte( (Int32)a - b );
	RETURN_BYTE( a );
}

void funcAssignMulByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	a = ClampByte( (Int32)a * b );
	RETURN_BYTE( a );
}

void funcAssignDivByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	if ( b )
	{
		a = ClampByte( (Int32)a / b );
	}
	else
	{
		a = 0;
	}
	RETURN_BYTE( a );
}

void funcAssignAndByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( a &= b );
}

void funcAssignOrByteInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Uint8, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;
	RETURN_BYTE( a |= b );
}

////////////////////////////////
// Boolean operators
////////////////////////////////

void funcAndBoolBool( CObject*, CScriptStackFrame& stack, void* result )
{
	// Get first param
	GET_PARAMETER( Bool, a, false );

	// We expect a special op here
	ASSERT( *stack.m_code == OP_Skip );
	stack.m_code++;

	// Get skip offset
	Uint16 offset = stack.Read< Uint16 >();

	// Value is determined by the first param, skip next
	if ( !a )
	{
		// Skip the param
		stack.m_code += offset;

		// We should skip the function by now
		ASSERT( *stack.m_code == OP_ParamEnd );
		stack.m_code++;

		// Return false
		RETURN_BOOL( false );
	}
	else
	{
		// Evaluate the second parameter and use it as a result
		GET_PARAMETER( Bool, b, 0 );
		FINISH_PARAMETERS;
		RETURN_BOOL( b );
	}
}

void funcOrBoolBool( CObject*, CScriptStackFrame& stack, void* result )
{
	// Get first param
	GET_PARAMETER( Bool, a, false );

	// We expect a special op here
	ASSERT( *stack.m_code == OP_Skip );
	stack.m_code++;

	// Get skip offset
	Uint16 offset = stack.Read< Uint16 >();

	// Value is determined by the first param, skip next
	if ( a )
	{
		// Skip the param
		stack.m_code += offset;

		// We should skip the function by now
		ASSERT( *stack.m_code == OP_ParamEnd );
		stack.m_code++;

		// Return true
		RETURN_BOOL( true );
	}
	else
	{
		// Evaluate the second parameter and use it as a result
		GET_PARAMETER( Bool, b, 0 );
		FINISH_PARAMETERS;
		RETURN_BOOL( b );
	}
}

void funcNotBoolBool( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, a, false );
	FINISH_PARAMETERS;
	RETURN_BOOL( !a );
}

////////////////////////////////
// Vector operators
////////////////////////////////

void funcNegVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );	
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, -a );	
}

// ...VectorVector
void funcAddVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a + b );	
}

void funcSubVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a - b );	
}

void funcMulVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a * b );	
}

void funcDivVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a / b );	
}

void funcAssignAddVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a += b );	
}

void funcAssignSubVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a -= b );	
}

void funcAssignMulVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a *= b );	
}

void funcAssignDivVectorVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a /= b );	
}

// ...VectorFloat
void funcAddVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a + b );	
}

void funcSubVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a - b );	
}

void funcMulVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a * b );	
}

void funcDivVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a / b );	
}

void funcAssignAddVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a += b );	
}

void funcAssignSubVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a -= b );	
}

void funcAssignMulVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a *= b );	
}

void funcAssignDivVectorFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, a /= b );	
}

// ...FloatVector
void funcMulFloatVector( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Vector, b, Vector::ZEROS );	
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, b * a );
}

////////////////////////////////
// Matrix operators
////////////////////////////////

void funcMulMatrixMatrix( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, a, Matrix::ZEROS );
	GET_PARAMETER( Matrix, b, Matrix::ZEROS );	
	FINISH_PARAMETERS;
	RETURN_STRUCT( Matrix, Matrix::Mul( a, b ) );
}

void funcAssignMulMatrixMatrix( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Matrix, a, Matrix::ZEROS );
	GET_PARAMETER( Matrix, b, Matrix::ZEROS );	
	FINISH_PARAMETERS;
	RETURN_STRUCT( Matrix, a = Matrix::Mul( a, b ) );
}

////////////////////////////////
// EngineTime operators
////////////////////////////////

// add
void funcAddEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a + b );	
}

void funcAddEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a + b );	
}

void funcAssignAddEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a += b );	
}

void funcAssignAddEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a += b );	
}

// sub
void funcSubEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a - b );	
}

void funcSubEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a - b );	
}

void funcAssignSubEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a -= b );	
}

void funcAssignSubEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a -= b );	
}

// mul
void funcMulEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a * b );	
}

void funcAssignMulEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, a *= b );	
}

// div
void funcDivEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, b ? (a / b) : EngineTime::ZERO );	
}

void funcAssignDivEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineTime, b ? (a /= b) : (a = EngineTime::ZERO) );	
}
// comparison
void funcEqualEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_BOOL( a == b );
}

void funcNotEqualEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_BOOL( a != b );
}

void funcGreaterEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_BOOL( a > b );
}

void funcGreaterEqualEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_BOOL( a >= b );
}

void funcLessEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_BOOL( a < b );
}

void funcLessEqualEngineTime( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( EngineTime, b, EngineTime::ZERO );
	FINISH_PARAMETERS;
	RETURN_BOOL( a <= b );
}

// comparison against float
void funcGreaterEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a > b );
}

void funcGreaterEqualEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a >= b );
}

void funcLessEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a < b );
}

void funcLessEqualEngineTimeFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EngineTime, a, EngineTime::ZERO );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;
	RETURN_BOOL( a <= b );
}

void funcLogicAndObjectObject( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IScriptable >, a, nullptr );
	GET_PARAMETER( THandle< IScriptable >, b, nullptr );
	FINISH_PARAMETERS;

	RETURN_BOOL( a.Get() && b.Get() );
}

////////////////////////////////
// Definitions
////////////////////////////////

void ExportCoreOperators()
{
	// Integer operators
	NATIVE_BINARY_OPERATOR( Add, funcAddIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Subtract, funcSubIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Multiply, funcMulIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Divide, funcDivIntInt, Int32, Int32, Int32 );
	NATIVE_UNARY_OPERATOR( Neg, funcNegIntInt, Int32, Int32 );
	NATIVE_UNARY_OPERATOR( BitNot, funcBitNotIntInt, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( And, funcAndIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Or, funcOrIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Xor, funcXorIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Equal, funcEqualIntInt, Bool, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( NotEqual, funcNotEqualIntInt, Bool, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Less, funcLessIntInt, Bool, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( LessEqual, funcLessEqualIntInt, Bool, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( Greater, funcGreaterIntInt, Bool, Int32, Int32 );
	NATIVE_BINARY_OPERATOR( GreaterEqual, funcGreaterEqualIntInt, Bool, Int32, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAdd, funcAssignAddIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignSubtract, funcAssignSubIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignMultiply, funcAssignMulIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignDivide, funcAssignDivIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAnd, funcAssignAndIntInt, Int32, Int32, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignOr, funcAssignOrIntInt, Int32, Int32, Int32 );

	// 64bit Unsigned Integer operators
	NATIVE_BINARY_OPERATOR( Add, funcAddUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Subtract, funcSubUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Multiply, funcMulUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Divide, funcDivUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( And, funcAndUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Or, funcOrUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Xor, funcXorUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Equal, funcEqualUint64Uint64, Bool, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( NotEqual, funcNotEqualUint64Uint64, Bool, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Less, funcLessUint64Uint64, Bool, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( LessEqual, funcLessEqualUint64Uint64, Bool, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( Greater, funcGreaterUint64Uint64, Bool, Uint64, Uint64 );
	NATIVE_BINARY_OPERATOR( GreaterEqual, funcGreaterEqualUint64Uint64, Bool, Uint64, Uint64 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAdd, funcAssignAddUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignSubtract, funcAssignSubUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignMultiply, funcAssignMulUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignDivide, funcAssignDivUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAnd, funcAssignAndUint64Uint64, Uint64, Uint64, Uint64 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignOr, funcAssignOrUint64Uint64, Uint64, Uint64, Uint64 );

	// String operators
	NATIVE_BINARY_OPERATOR( Add, funcAddStringString, String, String, String );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAdd, funcAssignAddStringString, String, String, String );

	// Float operators
	NATIVE_BINARY_OPERATOR( Add, funcAddFloatFloat, Float, Float, Float );
	NATIVE_BINARY_OPERATOR( Subtract, funcSubFloatFloat, Float, Float, Float );
	NATIVE_BINARY_OPERATOR( Multiply, funcMulFloatFloat, Float, Float, Float );
	NATIVE_BINARY_OPERATOR( Divide, funcDivFloatFloat, Float, Float, Float );
	NATIVE_UNARY_OPERATOR( Neg, funcNegFloatFloat, Float, Float );
	NATIVE_BINARY_OPERATOR( Equal, funcEqualFloatFloat, Bool, Float, Float );
	NATIVE_BINARY_OPERATOR( NotEqual, funcNotEqualFloatFloat, Bool, Float, Float );
	NATIVE_BINARY_OPERATOR( Less, funcLessFloatFloat, Bool, Float, Float );
	NATIVE_BINARY_OPERATOR( LessEqual, funcLessEqualFloatFloat, Bool, Float, Float );
	NATIVE_BINARY_OPERATOR( Greater, funcGreaterFloatFloat, Bool, Float, Float );
	NATIVE_BINARY_OPERATOR( GreaterEqual, funcGreaterEqualFloatFloat, Bool, Float, Float );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAdd, funcAssignAddFloatFloat, Float, Float, Float );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignSubtract, funcAssignSubFloatFloat, Float, Float, Float );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignMultiply, funcAssignMulFloatFloat, Float, Float, Float );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignDivide, funcAssignDivFloatFloat, Float, Float, Float );

	// Byte operators
	NATIVE_BINARY_OPERATOR( Add, funcAddByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( Subtract, funcSubByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( Multiply, funcMulByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( Divide, funcDivByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( Modulo, funcModByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( And, funcAndByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( Or, funcOrByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( Xor, funcXorByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_OPERATOR( Equal, funcEqualByteByte, Bool, Uint8, Uint8 );
	NATIVE_BINARY_OPERATOR( NotEqual, funcNotEqualByteByte, Bool, Uint8, Uint8 );
	NATIVE_BINARY_OPERATOR( Less, funcLessByteByte, Bool, Uint8, Uint8 );
	NATIVE_BINARY_OPERATOR( LessEqual, funcLessEqualByteByte, Bool, Uint8, Uint8 );
	NATIVE_BINARY_OPERATOR( Greater, funcGreaterByteByte, Bool, Uint8, Uint8 );
	NATIVE_BINARY_OPERATOR( GreaterEqual, funcGreaterEqualByteByte, Bool, Uint8, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAdd, funcAssignAddByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignSubtract, funcAssignSubByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignMultiply, funcAssignMulByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignDivide, funcAssignDivByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignAnd, funcAssignAndByteInt, Uint8, Uint8, Int32 );
	NATIVE_BINARY_ASSIGNMNET_OPERATOR( AssignOr, funcAssignOrByteInt, Uint8, Uint8, Int32 );

	// Boolean operators
	NATIVE_BINARY_BOOL_OPERATOR( LogicAnd, funcAndBoolBool, Bool, Bool, Bool );
	NATIVE_BINARY_BOOL_OPERATOR( LogicOr, funcOrBoolBool, Bool, Bool, Bool );
	NATIVE_UNARY_OPERATOR( LogicNot, funcNotBoolBool, Bool, Bool );

	// Vector operators
	NATIVE_UNARY_OPERATOR( Neg, funcNegVector, Vector, Vector);

	NATIVE_BINARY_OPERATOR( Add,		funcAddVectorVector, Vector, Vector, Vector );
	NATIVE_BINARY_OPERATOR( Subtract,	funcSubVectorVector, Vector, Vector, Vector );
	NATIVE_BINARY_OPERATOR( Multiply,	funcMulVectorVector, Vector, Vector, Vector );
	NATIVE_BINARY_OPERATOR( Divide,		funcDivVectorVector, Vector, Vector, Vector );

	NATIVE_BINARY_OPERATOR( AssignAdd,		funcAssignAddVectorVector, Vector, Vector, Vector );
	NATIVE_BINARY_OPERATOR( AssignSubtract,	funcAssignSubVectorVector, Vector, Vector, Vector );
	NATIVE_BINARY_OPERATOR( AssignMultiply,	funcAssignMulVectorVector, Vector, Vector, Vector );
	NATIVE_BINARY_OPERATOR( AssignDivide,	funcAssignDivVectorVector, Vector, Vector, Vector );

	NATIVE_BINARY_OPERATOR( Add,		funcAddVectorFloat, Vector, Vector, Float );
	NATIVE_BINARY_OPERATOR( Subtract,	funcSubVectorFloat, Vector, Vector, Float );
	NATIVE_BINARY_OPERATOR( Multiply,	funcMulVectorFloat, Vector, Vector, Float );
	NATIVE_BINARY_OPERATOR( Divide,		funcDivVectorFloat, Vector, Vector, Float );

	NATIVE_BINARY_OPERATOR( AssignAdd,		funcAssignAddVectorFloat, Vector, Vector, Float );
	NATIVE_BINARY_OPERATOR( AssignSubtract,	funcAssignSubVectorFloat, Vector, Vector, Float );
	NATIVE_BINARY_OPERATOR( AssignMultiply,	funcAssignMulVectorFloat, Vector, Vector, Float );
	NATIVE_BINARY_OPERATOR( AssignDivide,	funcAssignDivVectorFloat, Vector, Vector, Float );

	NATIVE_BINARY_OPERATOR( Multiply,	funcMulFloatVector, Vector, Float, Vector );

	// Matrix
	NATIVE_BINARY_OPERATOR( Multiply,	funcMulMatrixMatrix, Matrix, Matrix, Matrix );
	NATIVE_BINARY_OPERATOR( AssignMultiply,	funcAssignMulMatrixMatrix, Matrix, Matrix, Matrix );

	// EngineTime operators
	NATIVE_BINARY_OPERATOR( Add, funcAddEngineTime, EngineTime, EngineTime, EngineTime );	
	NATIVE_BINARY_OPERATOR( Add, funcAddEngineTimeFloat, EngineTime, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( AssignAdd, funcAssignAddEngineTime, EngineTime, EngineTime, EngineTime );
	NATIVE_BINARY_OPERATOR( AssignAdd, funcAssignAddEngineTimeFloat, EngineTime, EngineTime, Float );

	NATIVE_BINARY_OPERATOR( Subtract, funcSubEngineTime, EngineTime, EngineTime, EngineTime );	
	NATIVE_BINARY_OPERATOR( Subtract, funcSubEngineTimeFloat, EngineTime, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( AssignSubtract, funcAssignSubEngineTime, EngineTime, EngineTime, EngineTime );
	NATIVE_BINARY_OPERATOR( AssignSubtract, funcAssignSubEngineTimeFloat, EngineTime, EngineTime, Float );

	NATIVE_BINARY_OPERATOR( Multiply, funcMulEngineTimeFloat, EngineTime, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( AssignMultiply, funcMulEngineTimeFloat, EngineTime, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( Divide, funcDivEngineTimeFloat, EngineTime, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( AssignDivide, funcDivEngineTimeFloat, EngineTime, EngineTime, Float );

	NATIVE_BINARY_OPERATOR( Equal, funcEqualEngineTime, Bool, EngineTime, EngineTime );
	NATIVE_BINARY_OPERATOR( NotEqual, funcNotEqualEngineTime, Bool, EngineTime, EngineTime );
	NATIVE_BINARY_OPERATOR( Greater, funcGreaterEngineTime, Bool, EngineTime, EngineTime );
	NATIVE_BINARY_OPERATOR( GreaterEqual, funcGreaterEqualEngineTime, Bool, EngineTime, EngineTime );
	NATIVE_BINARY_OPERATOR( Less, funcLessEngineTime, Bool, EngineTime, EngineTime );
	NATIVE_BINARY_OPERATOR( LessEqual, funcLessEqualEngineTime, Bool, EngineTime, EngineTime );

	NATIVE_BINARY_OPERATOR( Greater, funcGreaterEngineTimeFloat, Bool, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( GreaterEqual, funcGreaterEqualEngineTimeFloat, Bool, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( Less, funcLessEngineTimeFloat, Bool, EngineTime, Float );
	NATIVE_BINARY_OPERATOR( LessEqual, funcLessEqualEngineTimeFloat, Bool, EngineTime, Float );

	NATIVE_BINARY_OPERATOR( LogicAnd, funcLogicAndObjectObject, Bool, THandle< IScriptable >, THandle< IScriptable > );
}
