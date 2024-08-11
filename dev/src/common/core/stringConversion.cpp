/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "stringConversion.h"

Bool GParseWhitespaces( const Char*& stream )
{
	// Eat white spaces
	while ( *stream && *stream <= ' ' )
	{
		stream++;
	}

	// 
	return *stream != 0;
}

Bool GParseWhitespaces( const AnsiChar*& stream )
{
	// Eat white spaces
	while ( *stream && *stream <= ' ' )
	{
		stream++;
	}

	// 
	return *stream != 0;
}

Bool GParseIdentifier( const Char*& stream, String& value )
{	
	String outputVal;

	if( GParseWhitespaces( stream ) )
	{
		if( *stream == '_' || iswalpha(*stream) )
		{
			// Grab normal text
			while( *stream && ( *stream == '_' || iswalnum(*stream) ) )
			{
				Char chars[2] = { *stream, 0 };
				outputVal += chars;
				stream++;				
			}
		}
		if( outputVal.GetLength() )
		{
			// Return grabbed string
			value = outputVal;
			return true;
		}
	}

	return false;
}

Bool GParseKeyword( const Char*& stream, const Char* keyword )
{
	if ( GParseWhitespaces( stream ) )
	{
		const Int32 N = static_cast< Int32 >( Red::System::StringLength( keyword ) );
		if ( 0 == Red::System::StringCompareNoCase( stream, keyword, N ) )
		{
			// Matched, move pointer
			stream += N;
			return true;
		}
	}

	// Not matched
	return false;
}

Bool GParseKeyword( const AnsiChar*& stream, const AnsiChar* keyword )
{
	if ( GParseWhitespaces( stream ) )
	{
		const Int32 N = static_cast< Int32 >( Red::System::StringLength( keyword ) );
		if ( 0 == Red::System::StringCompareNoCase( stream, keyword, N ) )
		{
			// Matched, move pointer
			stream += N;
			return true;
		}
	}

	// Not matched
	return false;
}

Bool GParseString( const Char*& stream, String& value )
{	
	String outputVal;

	if ( GParseWhitespaces( stream ) )
	{
		if ( *stream == '\"' )
		{
			// Grab quoted text
			stream++;
			while ( *stream && *stream != '\"' )
			{
				Char chars[2] = { *stream, 0 };
				outputVal += chars;
				stream++;
			}
			if( *stream )
				stream++;
		}
		else
		{
			// Grab normal text
			while ( *stream && *stream > ' ' )
			{
				Char chars[2] = { *stream, 0 };
				outputVal += chars;
				stream++;				
			}
		}
	}

	// Return grabbed string
	value = outputVal;
	return true;
}

//RED_MESSAGE("FIXME>>>>>>>>>>>>>>>>>>>>>>>>")
// Bool GParseString( const AnsiChar*& stream, String& value )
// {	
// 	String outputVal;
// 
// 	if ( GParseWhitespaces( stream ) )
// 	{
// 		if ( *stream == '\"' )
// 		{
// 			// Grab quoted text
// 			stream++;
// 			while ( *stream && *stream != '\"' )
// 			{
// 				Char chars[2] = { *stream, 0 };
// 				outputVal += chars;
// 				stream++;
// 			}
// 		}
// 		else
// 		{
// 			// Grab normal text
// 			while ( *stream && *stream > ' ' )
// 			{
// 				Char chars[2] = { *stream, 0 };
// 				outputVal += chars;
// 				stream++;				
// 			}
// 		}
// 	}
// 
// 	// Return grabbed string
// 	value = outputVal;
// 	return true;
// }

static Bool IsAlphaNum( Char ch )
{
	if ( ch >= '0' && ch <= '9') return true;
	if ( ch >= 'A' && ch <= 'Z') return true;
	if ( ch >= 'a' && ch <= 'z') return true;
	if ( ch == '_' ) return true;
	return false;
}

Bool GParseToken( const Char*& stream, String& token )
{
	String outputVal;

	const Char* original = stream;
	if ( GParseWhitespaces( stream ) )
	{
		// Starts with '\"'
		if ( *stream == '\"' )
		{
			// Skip
			stream++;

			// Extract chars
			while ( *stream && *stream != '\"' )
			{
				Char chars[2] = { *stream, 0 };
				outputVal += chars;
				stream++;				
			}

			// Skip ending \"
			if ( *stream == '\"' )
			{
				stream++;
				token = outputVal;
				return true;
			}

			// Not parsed
			stream = original;
			return false;
		}

		// Grab normal text - alpha numerical only
		while ( IsAlphaNum( *stream ) )
		{
			Char chars[2] = { *stream, 0 };
			outputVal += chars;
			stream++;				
		}

		// No token was parsed
		if ( outputVal.Empty() )
		{
			stream = original;
			return false;
		}

		// Parsed
		token = outputVal;
		return true;
	}

	// Not grabbed
	return false;
}

//RED_MESSAGE("FIXME>>>>>>>>>>>>>>>>>>>>>>>>")
// Bool GParseToken( const AnsiChar*& stream, String& token )
// {
// 	String outputVal;
// 
// 	if ( GParseWhitespaces( stream ) )
// 	{
// 		// Grab normal text
// 		while ( *stream && *stream > ' ' )
// 		{
// 			Char chars[2] = { *stream, 0 };
// 			outputVal += chars;
// 			stream++;				
// 		}
// 
// 		if ( outputVal.GetLength() )
// 		{
// 			token = outputVal;
// 			return true;
// 		}
// 	}
// 
// 	// Not grabbed
// 	return false;
// }

static Bool IsIntNum( Uint32 index, Char ch )
{
	if ( ch >= '0' && ch <= '9' ) return true;
	if ( (ch == '+' || ch == '-') && (index == 0) ) return true;
	return false;
}

static Bool IsHex( Char ch )
{
	if ( ch >= '0' && ch <= '9' ) return true;
	if ( ch >= 'A' && ch <= 'F' ) return true;
	if ( ch >= 'a' && ch <= 'a' ) return true;
	return false;
}

// Parse a *base ten* integer.
Bool GParseInteger( const Char*& stream, Int32& value )
{
	const Char* original = stream;
	if ( GParseWhitespaces( stream ) )
	{
		const Uint32 maxTxtLen = 64;
		Char intTxt[ maxTxtLen + 1 ];
		Uint32 numChars = 0;

		// Grab float number text
		while ( IsIntNum( numChars, *stream ) )
		{
			intTxt[ numChars++ ] = *stream++;

			// Overflow
			if ( numChars == maxTxtLen )
			{
				stream = original;
				return false;
			}
		}

		// Convert
		if ( numChars )
		{
			intTxt[ numChars ] = 0;

			return Red::System::StringToInt( value, intTxt, nullptr, Red::System::BaseTen );
		}
	}

	// Not parsed, restore
	stream = original;
	return false;
}

Bool GParseInteger( const Char*& stream, Uint32& value )
{
	const Char* original = stream;
	if ( GParseWhitespaces( stream ) )
	{
		const Uint32 maxTxtLen = 64;
		Char intTxt[ maxTxtLen + 1 ];
		Uint32 numChars = 0;

		// Grab float number text
		while ( IsIntNum( numChars, *stream ) )
		{
			intTxt[ numChars++ ] = *stream++;

			// Overflow
			if ( numChars == maxTxtLen )
			{
				stream = original;
				return false;
			}
		}

		// Convert
		if ( numChars )
		{
			intTxt[ numChars ] = 0;
			return Red::System::StringToInt( value, intTxt, nullptr, Red::System::BaseTen );
		}
	}

	// Not parsed, restore
	stream = original;
	return false;
}

Bool GParseInteger( const AnsiChar*& stream, Uint64& value )
{
	const AnsiChar* original = stream;
	if ( GParseWhitespaces( stream ) )
	{
		const Uint32 maxTxtLen = 64;
		AnsiChar intTxt[ maxTxtLen + 1 ];
		Uint32 numChars = 0;

		// Grab float number text
		while ( IsIntNum( numChars, *stream ) )
		{
			intTxt[ numChars++ ] = *stream++;

			// Overflow
			if ( numChars == maxTxtLen )
			{
				stream = original;
				return false;
			}
		}

		// Convert
		if ( numChars )
		{
			intTxt[ numChars ] = 0;
			//return Red::System::StringToInt( value, intTxt, nullptr, Red::System::BaseTen );
			value = strtoul( intTxt, NULL, 10 );
			return true;
		}
	}

	// Not parsed, restore
	stream = original;
	return false;
}

Bool GParseHex( const AnsiChar*& stream, Uint64& value )
{
	const AnsiChar* original = stream;
	if ( GParseWhitespaces( stream ) )
	{
		const Uint32 maxTxtLen = 64;
		AnsiChar intTxt[ maxTxtLen + 1 ];
		Uint32 numChars = 0;

		// Grab float number text
		while ( IsHex( *stream ) )
		{
			intTxt[ numChars++ ] = *stream++;

			// Overflow
			if ( numChars == maxTxtLen )
			{
				stream = original;
				return false;
			}
		}

		// Convert
		if ( numChars )
		{
			intTxt[ numChars ] = 0;
			//return Red::System::StringToInt( value, intTxt, nullptr, Red::System::BaseSixteen );
			value = strtoul( intTxt, NULL, 16 );
			return true;
		}
	}

	// Not parsed, restore
	stream = original;
	return false;
}

Bool GParseBool( const Char*& stream, Bool& value )
{
	if ( GParseWhitespaces( stream ) )
	{
		// True
		if ( Red::System::StringCompareNoCase( stream, TXT("true"), 4 ) == 0 )
		{
			value = true;
			stream += 4;
			return true;
		}

		// False
		if ( Red::System::StringCompareNoCase( stream, TXT("false"), 4 ) == 0 )
		{
			value = false;
			stream += 5;
			return true;
		}

		// Parse as number
		Int32 numericValue = 0;
		if ( GParseInteger( stream, numericValue ) )
		{
			value = (numericValue != 0);
			return true;
		}
	}

	// Not parsed
	return false;
}

static Bool IsFloatNum( Uint32 index, Char ch )
{
	if ( ch == '.' ) return true;
	if ( ch >= '0' && ch <= '9' ) return true;
	if ( ch == 'f' && (index>0) ) return true;
	if ( (ch == '+' || ch == '-') && (index == 0) ) return true;
	return false;
}

Bool GParseFloat( const Char*& stream, Float& value )
{
	const Char* original = stream;
	if ( GParseWhitespaces( stream ) )
	{
		const Uint32 maxTxtLen = 64;
		Char floatTxt[ maxTxtLen + 1 ];
		Uint32 numChars = 0;

		// Grab float number text
		while ( IsFloatNum( numChars, *stream ) )
		{
			floatTxt[ numChars++ ] = *stream++;

			// Overflow
			if ( numChars == maxTxtLen )
			{
				stream = original;
				return false;
			}
		}

		// Convert
		if ( numChars )
		{
			floatTxt[ numChars ] = 0;
			value = (Float)Red::System::StringToDouble( floatTxt );
			return true;
		}
	}

	// Not parsed, restore
	stream = original;
	return false;
}

Bool GParseDouble( const Char*& stream, Double& value )
{
	const Char* original = stream;
	if ( GParseWhitespaces( stream ) )
	{
		const Uint32 maxTxtLen = 128;
		Char floatTxt[ maxTxtLen + 1 ];
		Uint32 numChars = 0;

		// Grab float number text
		while ( IsFloatNum( numChars, *stream ) )
		{
			floatTxt[ numChars++ ] = *stream++;

			// Overflow
			if ( numChars == maxTxtLen )
			{
				stream = original;
				return false;
			}
		}

		// Convert
		if ( numChars )
		{
			floatTxt[ numChars ] = 0;
			value = Red::System::StringToDouble( floatTxt );
			return true;
		}
	}

	// Not parsed, restore
	stream = original;
	return false;
}
