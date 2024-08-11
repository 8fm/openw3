/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "string.h"
#include "scriptStackFrame.h"

/////////////////////////////////////////////
// String functions
/////////////////////////////////////////////

void funcStrLen( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_INT( str.GetLength() );
}

void funcStrCmp( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, with, String::EMPTY );
	GET_PARAMETER( Int32, count, -1 );
	GET_PARAMETER( Bool, noCase, false );
	FINISH_PARAMETERS;
	
	if ( noCase )
	{
		if ( count != -1 )
		{
			count = ::Clamp< Int32 >( count, 0, str.GetLength() );
			RETURN_INT( Red::System::StringCompareNoCase( str.AsChar(), with.AsChar(), count ) );
		}
		else
		{
			RETURN_INT( Red::System::StringCompareNoCase( str.AsChar(), with.AsChar() ) );
		}
	}
	else
	{
		if ( count != -1 )
		{
			count = ::Clamp< Int32 >( count, 0, str.GetLength() );
			RETURN_INT( Red::System::StringCompare( str.AsChar(), with.AsChar(), count ) );
		}
		else
		{
			RETURN_INT( Red::System::StringCompare( str.AsChar(), with.AsChar() ) );
		}
	}
}

void funcStrFindFirst( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	size_t index;
	if(str.FindSubstring( match, index, false ))
	{
		RETURN_INT( static_cast< Int32 >( index ) );
	}
	else
	{
		RETURN_INT( -1 );
	}
}

void funcStrFindLast( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	size_t index;
	if(str.FindSubstring( match, index, true ))
	{
		RETURN_INT( static_cast< Int32 >( index ) );
	}
	else
	{
		RETURN_INT( -1 );
	}
}

void funcStrSplitFirst( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, div, String::EMPTY );
	GET_PARAMETER_REF( String, left, String::EMPTY );
	GET_PARAMETER_REF( String, right, String::EMPTY );
	FINISH_PARAMETERS;
	bool status = str.Split( div, &left, &right, false );
	RETURN_BOOL( status );
}

void funcStrSplitLast( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, div, String::EMPTY );
	GET_PARAMETER_REF( String, left, String::EMPTY );
	GET_PARAMETER_REF( String, right, String::EMPTY );
	FINISH_PARAMETERS;
	bool status = str.Split( div, &left, &right, true );
	RETURN_BOOL( status );
}

void funcStrReplace( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	GET_PARAMETER( String, with, String::EMPTY );
	FINISH_PARAMETERS;
	str.Replace( match, with );
	RETURN_STRING( str );
}

void funcStrReplaceAll( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	GET_PARAMETER( String, with, String::EMPTY );
	FINISH_PARAMETERS;
	str.ReplaceAll( match, with );
	RETURN_STRING( str );
}

void funcStrMid( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( Int32, start, 0 );
	GET_PARAMETER_OPT( Int32, length, 1000000 );
	FINISH_PARAMETERS;
	RETURN_STRING( str.MidString( start, length ) );
}

void funcStrLeft( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( Int32, length, 0 );
	FINISH_PARAMETERS;
	RETURN_STRING( str.LeftString( length ) );
}

void funcStrRight( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( Int32, length, 0 );
	FINISH_PARAMETERS;
	RETURN_STRING( str.RightString( length ) );
}

void funcStrBeforeFirst( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.StringBefore( match, false ) );
}

void funcStrBeforeLast( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.StringBefore( match, true ) );
}

void funcStrAfterFirst( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.StringAfter( match, false ) );
}

void funcStrAfterLast( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.StringAfter( match, true ) );
}

void funcStrBeginsWith( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_BOOL( str.BeginsWith( match ) );
}

void funcStrEndsWith( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER( String, match, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_BOOL( str.EndsWith( match ) );
}

void funcStrUpper( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.ToUpper() );
}

void funcStrLower( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.ToLower() );
}

void funcStrUpperUTF( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.ToUpperUnicode() );
}

void funcStrLowerUTF( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_STRING( str.ToLowerUnicode() );
}

void funcStrChar( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, chr, 0 );
	FINISH_PARAMETERS;
	if ( chr < 32 ) chr = 32;
	RETURN_STRING( String::Chr( (Char)chr ) );
}

void funcNameToString( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	String temp = name.AsString();

	RETURN_STRING( temp );
}

void funcFloatToString( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, var, 0.f );
	FINISH_PARAMETERS;

	String temp = ToString( var );

	RETURN_STRING( temp );
}

void funcFloatToStringPrec( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, var, 0.f );
	GET_PARAMETER( Int32, precision, 1 );
	FINISH_PARAMETERS;

	ASSERT( precision >= 0 );

	String format;
	const float epsilon = 0.00001f;
	if ( MAbs(var - MFloor(var)) < epsilon )
	{
		format = TXT("%.0f");
	}
	else
	{
		format = TXT("%.") + ToString(precision) + TXT("f");
	}

	RETURN_STRING( String::Printf( format.AsChar(), var ) );
}

void funcIntToString( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, var, 0 );
	FINISH_PARAMETERS;

	String temp = ToString( var );

	RETURN_STRING( temp );
}

void funcStringToInt( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER_OPT( Int32, varDef, 0 );
	FINISH_PARAMETERS;

	Int32 var = 0;
	if ( !FromString( str, var ) )
	{
		var = varDef;
	}

	RETURN_INT( var );
}

void funcStringToFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	GET_PARAMETER_OPT( Float, varDef, 0.f );
	FINISH_PARAMETERS;

	Float var = 0.f;
	if ( !FromString( str, var ) )
	{
		var = varDef;
	}

	RETURN_FLOAT( var );
}

#define NATIVE_FUNC( x )	\
	NATIVE_GLOBAL_FUNCTION( #x, func##x );

void ExportCoreStringNatives()
{
	NATIVE_FUNC( StrLen );
	NATIVE_FUNC( StrCmp );
	NATIVE_FUNC( StrFindFirst );
	NATIVE_FUNC( StrFindLast );
	NATIVE_FUNC( StrSplitFirst );
	NATIVE_FUNC( StrSplitLast );
	NATIVE_FUNC( StrReplace );
	NATIVE_FUNC( StrReplaceAll );
	NATIVE_FUNC( StrMid );
	NATIVE_FUNC( StrLeft );
	NATIVE_FUNC( StrRight );
	NATIVE_FUNC( StrBeforeFirst );
	NATIVE_FUNC( StrBeforeLast );
	NATIVE_FUNC( StrAfterFirst );
	NATIVE_FUNC( StrAfterLast );
	NATIVE_FUNC( StrBeginsWith );
	NATIVE_FUNC( StrEndsWith );
	NATIVE_FUNC( StrUpper );
	NATIVE_FUNC( StrLower );
	NATIVE_FUNC( StrChar );
	NATIVE_FUNC( NameToString );
	NATIVE_FUNC( FloatToString );
	NATIVE_FUNC( FloatToStringPrec );
	NATIVE_FUNC( IntToString );
	NATIVE_FUNC( StringToInt );
	NATIVE_FUNC( StringToFloat );
	NATIVE_FUNC( StrUpperUTF );
	NATIVE_FUNC( StrLowerUTF );
}
