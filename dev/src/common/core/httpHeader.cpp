/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "httpHeader.h"
#include "version.h"

//---------------

namespace Helper
{
	RED_INLINE static Uint16 SafeYear( const Uint32 value )
	{
		RED_ASSERT( value >= 1900 && value <= 2100 );
		return (Uint16) value;
	}

	RED_INLINE static Uint8 SafeMonth( const Uint32 value )
	{
		RED_ASSERT( value >= 1 && value <= 12 );
		return (Uint8) value;
	}

	RED_INLINE static Uint8 SafeDay( const Uint32 value )
	{
		RED_ASSERT( value >= 1 && value <= 31 );
		return (Uint8) value;
	}

	RED_INLINE static Uint8 SafeHour( const Uint32 value )
	{
		RED_ASSERT( value >= 0 && value <= 23 );
		return (Uint8) value;
	}

	RED_INLINE static Uint8 SafeMinute( const Uint32 value )
	{
		RED_ASSERT( value >= 0 && value <= 59 );
		return (Uint8) value;
	}

	RED_INLINE static Uint8 SafeSecond( const Uint32 value )
	{
		RED_ASSERT( value >= 0 && value <= 59 );
		return (Uint8) value;
	}

	RED_INLINE Int32 DayOfTheWeek( int y, int m, int d )
	{
		const Int32 t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
		y -= m < 3;
		return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
	}

	RED_INLINE void AppendString( StringAnsi& ret, Int32 value )
	{
		AnsiChar buf[16];
		Red::SNPrintF( buf, ARRAY_COUNT_U32(buf), "%02d", value );
		ret += buf;
	}

	RED_INLINE CHTTPDate GetCurrentDate()
	{
		// get system time
		Red::DateTime currentTime;
		Red::Clock::GetInstance().GetLocalTime( currentTime );

		// convert to HTTP date
		return CHTTPDate( currentTime );
	}
}

//---------------

CHTTPDate::CHTTPDate()
	: m_year( 0)
	, m_month( 0 )
	, m_day( 0 )
	, m_hour( 0 )
	, m_min( 0 )
	, m_sec( 0 )
{
}

CHTTPDate::CHTTPDate( const Uint32 year, const Uint32 month, const Uint32 day, const Uint32 hour, const Uint32 min, const Uint32 sec )
	: m_year( Helper::SafeYear(year) )
	, m_month( Helper::SafeMonth(month) )
	, m_day( Helper::SafeDay(day) )
	, m_hour( Helper::SafeHour(hour) )
	, m_min( Helper::SafeMinute(min) )
	, m_sec( Helper::SafeSecond(sec) )
{
}

CHTTPDate::CHTTPDate( const Red::System::DateTime& time )
	: m_year( Helper::SafeYear(time.GetYear()) )
	, m_month( Helper::SafeMonth(time.GetMonth()+1) )
	, m_day( Helper::SafeDay(time.GetDay()+1) )
	, m_hour( Helper::SafeHour(time.GetHour() ) )
	, m_min( Helper::SafeMinute(time.GetMinute() ) )
	, m_sec( Helper::SafeSecond(time.GetSecond() ) )
{
}

const Bool CHTTPDate::IsValid() const
{
	return (m_year != 0) && (m_month != 0) && (m_day != 0);
}

CHTTPDate CHTTPDate::Parse( const StringAnsi& txt )
{
	RED_UNUSED( txt );
	CHTTPDate ret;

	// skip day name

	return ret;
}

const Red::System::DateTime CHTTPDate::ToSystem() const
{
	Red::System::DateTime ret;

	if ( IsValid() )
	{
		ret.SetYear( m_year );
		ret.SetMonth( m_month-1 );
		ret.SetDay( m_day-1 );

		ret.SetHour( m_hour );
		ret.SetMinute( m_min );
		ret.SetSecond( m_sec );
	}

	return ret;
}

const StringAnsi CHTTPDate::ToString() const
{
	const AnsiChar* dayName[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
	const AnsiChar* monthName[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	StringAnsi ret;

	if ( IsValid() )
	{
		// day of the week
		ret += dayName[ Helper::DayOfTheWeek( m_year, m_month, m_day ) ];
		ret += ", ";

		// date
		Helper::AppendString( ret, m_day );
		ret += " ";

		ret += monthName[ m_month-1 ];
		ret += " ";

		Helper::AppendString( ret, m_year );
		ret += " ";

		// time
		Helper::AppendString( ret, m_hour );
		ret += ":";
		Helper::AppendString( ret, m_min );
		ret += ":";
		Helper::AppendString( ret, m_sec );
	}

	return ret;
}

//---------------

const AnsiChar CHTTPRequest::SYMBOL_ARGS = ' ';
const AnsiChar CHTTPRequest::SYMBOL_KEY = ':';
const AnsiChar CHTTPRequest::SYMBOL_LF = 10;
const AnsiChar CHTTPRequest::SYMBOL_CR = 13;

CHTTPRequest::CHTTPRequest()
	: m_keepAlive( false )
	, m_contentLength( 0 )
{
}

const Bool CHTTPRequest::Parse( const AnsiChar* txt )
{
	const AnsiChar* cur = txt;

	// Parse: <method> <uri> <version>
	// Method: GET POST HEAD etc
	// URI: url part
	// Version: HTML/1.1
	if ( !ParseText( cur, m_method, SYMBOL_ARGS ) )
		return false;
	if ( !ParseText( cur, m_url, SYMBOL_ARGS ) )
		return false;
	if ( !ParseText( cur, m_version, SYMBOL_LF ) )
		return false;

	// additional data
	for (;;)
	{
		// content name
		StringAnsi key;
		if ( !ParseText( cur, key, SYMBOL_KEY ) )
			break;

		// KEY shit
		RED_ASSERT( *cur == SYMBOL_KEY );
		cur += 1;

		// content value - parse till the end of the line
		StringAnsi value;
		if ( ParseText( cur, value, SYMBOL_LF ) )
		{
			// process parsed parameter
			ProcessHeaderParam( key, value );
			
			// skip the end of the line
			ParseLineEnd( cur );
		}
	}

	// valid header parsed
	return true;
}

Bool CHTTPRequest::SkipWhitespace( const AnsiChar*& txt )
{
	// no whitespaces
	if ( !*txt || (*txt > ' ') )
		return false;

	// skip
	while ( *txt <= ' ' )
		txt += 1;

	return true;
}

Bool CHTTPRequest::ParseLineEnd( const AnsiChar*& txt )
{
	const AnsiChar* cur = txt;

	// skip the optional CR
	if ( cur[0] == SYMBOL_CR )
		cur += 1;

	// we should have the LF symbol here
	if ( cur[0] == SYMBOL_LF )
	{
		txt = cur+1;
		return true;
	}

	// we did not found the line end
	return false;
}

Bool CHTTPRequest::ParseText( const AnsiChar*& txt, StringAnsi& outText, const AnsiChar aditionalDelimiter /*= 0*/  )
{
	const AnsiChar* cur = txt;
	outText.Clear();

	SkipWhitespace( cur );

	while ( (*cur != 0) && (*cur != aditionalDelimiter) )
	{
		// do not ever output the line ending chars
		if ( *cur != SYMBOL_LF && *cur != SYMBOL_CR )
		{
			AnsiChar buf[] = { *cur, 0 };
			outText += buf;
		}

		cur += 1;
	}

	if ( outText.Empty() )
		return false;

	txt = cur;
	return true;
}

void CHTTPRequest::ProcessHeaderParam( const StringAnsi& name, const StringAnsi& value )
{
	// extract common values
	if ( 0 == Red::StringCompareNoCase( name.AsChar(), "host" ) )
	{
		m_host = value;
	}
	else if ( 0 == Red::StringCompareNoCase( name.AsChar(), "user-agent" ) )
	{
		m_userAgent = value;
	}
	else if ( 0 == Red::StringCompareNoCase( name.AsChar(), "connection" ) )
	{
		if ( nullptr != Red::StringSearch( value.AsChar(), "keep-alive" ) )
			m_keepAlive = true;
	}
	else if ( 0 == Red::StringCompareNoCase( name.AsChar(), "content-length" ) )
	{
		m_contentLength = atoi( value.AsChar() );
	}
	else if ( 0 == Red::StringCompareNoCase( name.AsChar(), "content-type" ) )
	{
		m_contentType = value;
	}

	// add to general map
	m_params.Insert( name, value );
}

//---------------

CHTTPResponse::CHTTPResponse()
	: m_version( "HTTP/1.1" )
	, m_server( "RedEngine_" APP_VERSION_BUILD )
	, m_cacheControl( "no-store, no-cache, must-revalidate" )
	, m_keepAlive( true )
	, m_date( Helper::GetCurrentDate() )
	, m_code( 501 ) // not implemented
	, m_contentSize( 0 )
{
}

void CHTTPResponse::SetContent( const StringAnsi& type, const StringAnsi& encoding, const Uint32 size )
{
	m_contentSize = size;
	m_contentType = type;
	m_contentEncoding = encoding;
}

void CHTTPResponse::SetResponseCode( const Uint32 code )
{
	m_code = code;
}

const StringAnsi CHTTPResponse::ToString() const
{
	StringAnsi ret;

	// version + response code
	ret += "HTTP/1.1 ";
	Helper::AppendString( ret, m_code );
	ret += " ";
	ret += GetResponseName( m_code );
	ret += "\r\n";

	// response date
	ret += "Date: ";
	ret += m_date.ToString();
	ret += "\r\n";

	// only in valid content
	if ( m_code == 200 && m_contentSize > 0 && !m_contentEncoding.Empty() && !m_contentType.Empty() )
	{
		// cache control
		ret += "Cache-Control: ";
		ret += m_cacheControl;
		ret += "\r\n";

		// connection keep alive
		if ( m_keepAlive )
		{
			ret += "Connection: keep-alive";
			ret += "\r\n";
		}

		// content encoding
		ret += "Content-Encoding: ";
		ret += m_contentEncoding;
		ret += "\r\n";

		// content length (after encoding)
		ret += "Content-Length: ";
		Helper::AppendString( ret, m_contentSize );
		ret += "\r\n";

		// content type and charset
		ret += "Content-Type: ";
		ret += m_contentType;
		ret += "\r\n";
	}

	return ret;
}

const AnsiChar* CHTTPResponse::GetResponseName( const Uint32 code )
{
	switch ( code )
	{
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 110: return "Connection Timed Out";
		case 111: return "Connection refused";

		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 204: return "No content";
		case 205: return "Reset content";
		case 206: return "Partial content";

		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 403: return "Forbidden";
		case 404: return "Not Found";

		case 500: return "Internal server error";
		case 501: return "Not implemented";
	}

	return "Unknown";
}

//---------------
