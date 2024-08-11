/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "basicURL.h"

const AnsiChar* CBasicURL::URL_PROTOCOL = "://";
const AnsiChar CBasicURL::URL_SEPARATOR = '/';
const AnsiChar CBasicURL::URL_FIRSTKEY = '?';
const AnsiChar CBasicURL::URL_KEY = '&';
const AnsiChar CBasicURL::URL_EQUALS = '=';

CBasicURL::CBasicURL()
{
}

CBasicURL::CBasicURL( const CBasicURL& other )
	: m_keys( other.m_keys )
	, m_protocol( other.m_protocol )
	, m_path( other.m_path )
	, m_address( other.m_address )
{
}

CBasicURL::CBasicURL( CBasicURL&& other )
	: m_keys( std::move( other.m_keys ) )
	, m_protocol( std::move( other.m_protocol ) )
	, m_path( std::move( other.m_path ) )
	, m_address( std::move( other.m_address ) )
{
}

// eats text, stops only at white line, : and //
Bool CBasicURL::ParseURLPart( const AnsiChar*& pos, StringAnsi& out )
{
	const AnsiChar* txt = pos;
	while ( *txt != 0 && *txt != ':' && *txt != '/' )
	{
		AnsiChar buf[] = {*txt++, 0};
		out += buf;
	}

	if ( out.Empty() )
		return false;

	pos = txt;
	return true;
}

// eats text, stops only at ?
Bool CBasicURL::ParseURLPath( const AnsiChar*& pos, StringAnsi& out )
{
	const AnsiChar* txt = pos;
	while ( *txt != 0 && *txt != URL_FIRSTKEY )
	{
		AnsiChar buf[] = {*txt++, 0};
		out += buf;
	}

	if ( out.Empty() )
		return false;

	pos = txt;
	return true;
}

// eats text, stops only at ?
Bool CBasicURL::ParseURLKey( const AnsiChar*& pos, StringAnsi& out )
{
	const AnsiChar* txt = pos;
	while ( *txt != 0 && *txt != URL_EQUALS && *txt != URL_KEY )
	{
		AnsiChar buf[] = {*txt++, 0};
		out += buf;
	}

	if ( out.Empty() )
		return false;

	pos = txt;
	return true;
}

// match URL part
Bool CBasicURL::MatchURLPart( const AnsiChar*& pos, const AnsiChar* txt )
{
	const auto len = Red::StringLength( txt );
	if ( 0 == Red::StringCompare( pos, txt, len ) )
	{
		pos += len;
		return true;
	}

	return false;
}

// match URL part - single char
Bool CBasicURL::MatchURLPart( const AnsiChar*& pos, const AnsiChar ch )
{
	if ( pos[0] == ch )
	{
		pos += 1;
		return true;
	}

	return false;
}

void CBasicURL::SetProtocol( const StringAnsi& protocol )
{
	m_protocol = protocol;
}

void CBasicURL::SetAddress( const StringAnsi& address )
{
	m_address = address;
}

void CBasicURL::SetPath( const StringAnsi& path )
{
	m_path = path;
}

void CBasicURL::SetKey( const StringAnsi& key, const StringAnsi& value )
{
	m_keys.Set( key, value );
}

void CBasicURL::SetKey( const StringAnsi& key, const Int32 value )
{
	AnsiChar valueText[16];
	Red::SNPrintF( valueText, ARRAY_COUNT(valueText), "%d", value );
	m_keys.Set( key, valueText );
}

CBasicURL CBasicURL::Parse( const StringAnsi& text, const Bool isRelative /*= false*/ )
{
	const AnsiChar* cur = text.AsChar();
	CBasicURL ret;

	// protocol + address
	if ( !isRelative )
	{
		// parse first part, assume it's a protocol name or an address
		StringAnsi token;
		if ( !ParseURLPart( cur, token ) )
			return CBasicURL();

		// protocol signature ?
		if ( MatchURLPart( cur, URL_PROTOCOL ) )
		{
			ret.m_protocol = token;

			// parse address
			if ( !ParseURLPart( cur, token ) )
				return CBasicURL();
		}

		// address
		ret.m_address = token;
	
		// eat the / at the end of the address, if it's not there it's the end of the URL
		if ( !MatchURLPart( cur, URL_SEPARATOR ) )
			return ret;
	}

	// eat the first path separator
	MatchURLPart( cur, URL_SEPARATOR );

	// parse the path until we encounter and of string or key marker ?
	ParseURLPath( cur, ret.m_path );

	// eat the first key marker - if it's not there it's the end of the URL
	if ( !MatchURLPart( cur, URL_FIRSTKEY ) )
		return ret;

	// parse the values
	for (;;)
	{
		// parse the key name - should always be here
		StringAnsi token;
		if ( !ParseURLKey( cur, token ) )
			return CBasicURL();

		// parse the key equal sign, if it's there than we have a value
		if ( MatchURLPart( cur, URL_EQUALS ) )
		{
			StringAnsi value;
			if ( !ParseURLKey( cur, value ) )
				return CBasicURL();

			// add key-value pair to the list
			ret.m_keys.Insert( token, value );
		}

		// more keys ?
		if ( !MatchURLPart( cur, URL_KEY ) )
			break;
	}

	// done
	return ret;
}

CBasicURL CBasicURL::ParseParams( const StringAnsi& text )
{
	const AnsiChar* cur = text.AsChar();
	CBasicURL ret;

	// parse the values
	for (;;)
	{
		// parse the key name - should always be here
		StringAnsi token;
		if ( !ParseURLKey( cur, token ) )
			return CBasicURL();

		// parse the key equal sign, if it's there than we have a value
		if ( MatchURLPart( cur, URL_EQUALS ) )
		{
			StringAnsi value;
			if ( !ParseURLKey( cur, value ) )
				return CBasicURL();

			// add key-value pair to the list
			ret.m_keys.Insert( token, value );
		}

		// more keys ?
		if ( !MatchURLPart( cur, URL_KEY ) )
			break;
	}

	// done
	return ret;
}

namespace Helper
{
	static const Int32 GetHexCode( const AnsiChar ch )
	{
		switch ( ch )
		{
			case 'A': return 10;
			case 'B': return 11;
			case 'C': return 12;
			case 'D': return 13;
			case 'E': return 14;
			case 'F': return 15;

			case 'a': return 10;
			case 'b': return 11;
			case 'c': return 12;
			case 'd': return 13;
			case 'e': return 14;
			case 'f': return 15;

			case '0': return 0;
			case '1': return 1;
			case '2': return 2;
			case '3': return 3;
			case '4': return 4;
			case '5': return 5;
			case '6': return 6;
			case '7': return 7;
			case '8': return 8;
			case '9': return 9;
		}

		return -1;
	}
}

StringAnsi CBasicURL::Decode( const StringAnsi& text )
{
	const AnsiChar* cur = text.AsChar();
	const AnsiChar* end = cur + text.GetLength();

	// convert URL (assumes application/x-www-form-urlencoded)
	// see (http://en.wikipedia.org/wiki/Percent-encoding) for more details
	StringAnsi ret;
	while ( cur < end )
	{
		const AnsiChar ch = *cur++;

		if ( ch == '+' )
		{
			ret.Append( ' ' );
		}
		else if ( ch == '%' )
		{
			// parse valid char code
			auto code0 = Helper::GetHexCode( cur[0] );
			auto code1 = Helper::GetHexCode( cur[1] );
			if ( code0 >= 0 && code1 >= 0 )
			{
				const AnsiChar ch = (code0 << 4) | code1;
				ret.Append( ch );

				cur += 2;
			}
		}
		else
		{
			ret.Append( ch );
		}
	}

	return ret;
}

StringAnsi CBasicURL::ToString() const
{
	StringAnsi ret;

	// protocol part
	if ( !m_protocol.Empty() )
	{
		ret += m_protocol;
		ret += URL_PROTOCOL;
	}

	// address part
	if ( !m_address.Empty() )
	{
		ret += m_address;
		ret += URL_SEPARATOR;
	}

	// path
	if ( !m_path.Empty() )
	{
		ret += m_path;
	}

	// keys
	if ( !m_keys.Empty() )
	{
		// key-value pairs
		bool firstKey = true;
		for ( auto it : m_keys )
		{
			// prefix
			if ( firstKey )
			{
				ret += URL_FIRSTKEY;
				firstKey = false;
			}
			else
			{
				ret += URL_KEY;
			}

			// name
			ret += it.m_first;
			ret += URL_EQUALS;
			ret += it.m_second;
		}
	}

	// return full URL
	return ret;
}



