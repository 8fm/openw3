/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "string.h"

// Convert value to string
template< class T >
RED_INLINE String ToString( const T& )
{
	return String::EMPTY;
}

// Convert string to value
template< class T >
RED_INLINE Bool FromString( const String&, T& )
{
	return false;
}

template<> RED_INLINE String ToString( const CGUID& value )
{
	Char buffer[ RED_GUID_STRING_BUFFER_SIZE ];
	value.ToString( buffer, RED_GUID_STRING_BUFFER_SIZE );
	return String( buffer );
}

enum EDateFormat
{
	DateFormat_DayMonthYear = 0,
	DateFormat_MonthDayYear,
	DateFormat_YearMonthDay,
};

RED_INLINE String ToString( const Red::System::DateTime& dt, EDateFormat format )
{
	Uint32 dateFront = 0;
	Uint32 dateMiddle = 0;
	Uint32 dateRear = 0;

	switch( format )
	{
	case DateFormat_DayMonthYear:
		dateFront	= dt.GetDay() + 1;
		dateMiddle	= dt.GetMonth() + 1;
		dateRear	= dt.GetYear();
		break;

	case DateFormat_MonthDayYear:
		dateFront	= dt.GetMonth() + 1;
		dateMiddle	= dt.GetDay() + 1;
		dateRear	= dt.GetYear();
		break;

	case DateFormat_YearMonthDay:
		dateFront	= dt.GetYear();
		dateMiddle	= dt.GetMonth() + 1;
		dateRear	= dt.GetDay() + 1;
		break;

	default:
		RED_HALT( "Invalid format specified in DateTime ToString(): %i", format );
	}

	// Build a string showing the date and time.
	return String::Printf
	(
		TXT( "%02d/%02d/%02d %02d:%02d:%02d" ),
		dateFront,
		dateMiddle,
		dateRear,

		dt.GetHour(),
		dt.GetMinute(),
		dt.GetSecond()
	);
}

template<> RED_INLINE String ToString( const Red::System::DateTime& dt )
{
	return ToString( dt, DateFormat_DayMonthYear );
}

// Conversion for floating point type type
template<> RED_INLINE String ToString( const Float& value )
{
	return String::Printf( TXT("%g"), value );
}

// Conversion for floating point type type
template<> RED_INLINE String ToString( const Double& value )
{
	return String::Printf( TXT("%g"), value );
}

// Conversion for string
template<> RED_INLINE String ToString( const String& value )
{
	return value;
}

// Conversion for stringansi
template<> RED_INLINE String ToString( const StringAnsi& value )
{
	const char* ptr = value.AsChar();
	
	size_t len = value.GetLength();
	String result;
	for ( size_t i = 0; i < len; ++i )
	{
		result.Append( (Char)ptr[i] );
	}

	return result;
}

// Conversion for boolean value
template<> RED_INLINE String ToString( const Bool& value )
{
	return value ? TXT("true") : TXT("false");
}

template<> RED_INLINE String ToString( const Int8& value )		{ return String::Printf( TXT( "%hhi" ), value ); }
template<> RED_INLINE String ToString( const Uint8& value )		{ return String::Printf( TXT( "%hhu" ), value ); }
template<> RED_INLINE String ToString( const Int16& value )		{ return String::Printf( TXT( "%hi" ), value ); }
template<> RED_INLINE String ToString( const Uint16& value )	{ return String::Printf( TXT( "%hu" ), value ); }
template<> RED_INLINE String ToString( const Int32& value )		{ return String::Printf( TXT( "%i" ), value ); }
template<> RED_INLINE String ToString( const Uint32& value )	{ return String::Printf( TXT( "%u" ), value ); }
template<> RED_INLINE String ToString( const Uint64& value )	{ return String::Printf( TXT( "%llu" ), ( Uint64 ) value ); }

// String parsing
Bool GParseWhitespaces( const Char*& stream );
Bool GParseIdentifier( const Char*& stream, String& string );
Bool GParseKeyword( const Char*& stream, const Char* keyword );
Bool GParseString( const Char*& stream, String& value );
Bool GParseToken( const Char*& stream, String& token );
Bool GParseInteger( const Char*& stream, Int32& value );
Bool GParseInteger( const Char*& stream, Uint32& value );
Bool GParseBool( const Char*& stream, Bool& value );
Bool GParseDouble( const Char*& stream, Double& value );
Bool GParseFloat( const Char*& stream, Float& value );

// ANSI string parsing
Bool GParseWhitespaces( const AnsiChar*& stream );
Bool GParseKeyword( const AnsiChar*& stream, const AnsiChar* keyword );
Bool GParseInteger( const AnsiChar*& stream, Uint64& value );
Bool GParseHex( const AnsiChar*& stream, Uint64& value );

//RED_MESSAGE("FIXME>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
//Bool GParseString( const AnsiChar*& stream, String& value );
//Bool GParseToken( const AnsiChar*& stream, String& token );

// Convert string to string
template<>
RED_INLINE Bool FromString( const String& text, String& value )
{
	value = text;
	return true;
};

// Convert string to stringansi
template<>
RED_INLINE Bool FromString( const String& text, StringAnsi& value )
{
	if ( text.Empty() )
	{
		value = StringAnsi::EMPTY;
		return true;
	}
	else
	{
		value.Resize( text.Size() );
		return Red::System::StringConvert( value.TypedData(), text.TypedData(), value.Size() );
	}
}

// Convert string to integer
template<>
RED_INLINE Bool FromString( const String& text, Int32& value )
{
	const Char* ptr = text.AsChar();
	return GParseInteger( ptr, value );
};

// Convert string to integer
template<>
RED_INLINE Bool FromString( const String& text, Int16& value )
{
	const Char* ptr = text.AsChar();
	Int32 val = value;
	const Bool retVal = GParseInteger( ptr, val );
	value = Int16( val );
	return retVal;
};


template<>
RED_INLINE Bool FromString( const String& text, Uint32& value )
{
	const Char* ptr = text.AsChar();
	Uint32 intVal = 0;
	Bool retVal = GParseInteger( ptr, intVal );
	value = intVal;
	return retVal;
};

// Convert string to boolean
template<>
RED_INLINE Bool FromString( const String& text, Bool& value )
{
	const Char* ptr = text.AsChar();
	return GParseBool( ptr, value );
};

// Convert string to float
template<>
RED_INLINE Bool FromString( const String& text, Float& value )
{
	const Char* ptr = text.AsChar();
	return GParseFloat( ptr, value );
};

// Convert string to double
template<>
RED_INLINE Bool FromString( const String& text, Double& value )
{
	const Char* ptr = text.AsChar();
	return GParseDouble( ptr, value );
};

// Conversion for simple numeric types
template<> RED_INLINE Bool FromString( const String& text, Int8& value )
{ 
	Int32 val = 0; 
	if ( FromString<Int32>( text, val ) )
	{
		value = (Int8) val;
		return true;
	}
	return false;
}

// Conversion for simple numeric types
template<> RED_INLINE Bool FromString( const String& text, Uint8& value )
{ 
	Int32 val = 0; 
	if ( FromString<Int32>( text, val ) )
	{
		value = (Uint8) val;
		return true;
	}
	return false;
}

// Conversion for simple numeric types
template<> RED_INLINE Bool FromString( const String& text, Uint16& value )
{ 
	Int32 val = 0; 
	if ( FromString<Int32>( text, val ) )
	{
		value = (Uint16) val;
		return true;
	}
	return false;
}


RED_INLINE Bool FromString( const String& strValue, Red::System::DateTime& dt, EDateFormat format )
{
	//return false;
	TDynArray< String > separators;
	separators.PushBack( TXT("/") );
	separators.PushBack( TXT(":") );
	separators.PushBack( TXT(" ") );

	TDynArray< String > dateParts = strValue.Split( separators );
	RED_ASSERT( dateParts.Size() == 6 );
	if ( dateParts.Size() != 6 )
	{
		return false;
	}

	Uint32 dateFront = 0, dateMiddle = 0, dateRear = 0;
	Uint32 hour = 0, minute = 0, second = 0;

	if ( !FromString( dateParts[0], dateFront ) || !FromString( dateParts[1], dateMiddle ) 
		|| !FromString( dateParts[2], dateRear ) || !FromString( dateParts[3], hour ) 
		|| !FromString( dateParts[4], minute ) || !FromString( dateParts[5], second ) )
	{
		return false;
	}

	switch( format )
	{
	case DateFormat_DayMonthYear:
		dt.SetDay( dateFront - 1 );
		dt.SetMonth( dateMiddle - 1 );
		dt.SetYear( dateRear );
		break;

	case DateFormat_MonthDayYear:
		dt.SetMonth( dateFront - 1 );
		dt.SetDay( dateMiddle - 1 );
		dt.SetYear( dateRear );
		break;

	case DateFormat_YearMonthDay:
		dt.SetYear( dateFront );
		dt.SetMonth( dateMiddle - 1 );
		dt.SetDay( dateRear - 1 );
		break;

	default:
		RED_HALT( "Invalid format specified in DateTime FromString(): %i", format );
	}

	dt.SetHour( hour );
	dt.SetMinute( minute );
	dt.SetSecond( second );

	return true;
}

template<> RED_INLINE Bool FromString( const String& text, Red::System::DateTime& dt )
{
	return FromString( text, dt, DateFormat_DayMonthYear );
}
