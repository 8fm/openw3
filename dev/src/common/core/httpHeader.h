/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "sortedmap.h"

//--------------------------------------

/// HTTP Date/Time format
class CHTTPDate
{
public:
	Uint16				m_year;
	Uint8				m_month;
	Uint8				m_day;
	Uint8				m_hour;
	Uint8				m_min;
	Uint8				m_sec;

public:
	CHTTPDate();
	CHTTPDate( const Uint32 year, const Uint32 month, const Uint32 day, const Uint32 hour, const Uint32 min, const Uint32 sec );
	CHTTPDate( const Red::System::DateTime& time );

	// Is this a valid date ?
	const Bool IsValid() const;

	// Parse from string
	static CHTTPDate Parse( const StringAnsi& txt );

	// Convert to system DateTime
	const Red::System::DateTime ToSystem() const;

	// Convert to string (HTTP protocol standard)
	const StringAnsi ToString() const;
};

//---------------------------------

/// HTTP Request (http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html)
class CHTTPRequest
{
public:
	typedef TSortedMap< StringAnsi, StringAnsi >		TCookies;
	typedef TSortedMap< StringAnsi, StringAnsi >		TParams;
	typedef TDynArray< StringAnsi >						TAcceptedData;

	// method + version
	StringAnsi			m_method;
	StringAnsi			m_version;
	StringAnsi			m_url;
	StringAnsi			m_host;

	// cookies
	TCookies			m_cookies;

	// accepted content types
	TAcceptedData		m_accept;

	// client information
	StringAnsi			m_userAgent;
	Bool				m_keepAlive;

	// custom content (POST mostly)
	StringAnsi			m_contentType;
	Uint32				m_contentLength;
	TDynArray< Uint8 >	m_contentData; // may not be allocated

	// all header parameters
	TParams				m_params;

public:
	CHTTPRequest();

	// parse request from text buffer, returns true if successful
	const Bool Parse( const AnsiChar* txt );

private:
	// message delimiters
	static const AnsiChar SYMBOL_ARGS;
	static const AnsiChar SYMBOL_KEY;
	static const AnsiChar SYMBOL_CR;
	static const AnsiChar SYMBOL_LF;

	// parsing helpers
	static Bool SkipWhitespace( const AnsiChar*& txt );
	static Bool ParseText( const AnsiChar*& txt, StringAnsi& outText, const AnsiChar aditionalDelimiter = 0 );
	static Bool ParseLineEnd( const AnsiChar*& txt );

	// process header parameter
	void ProcessHeaderParam( const StringAnsi& name, const StringAnsi& value );
};

//---------------------------------

/// HTTP Response (http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html)
class CHTTPResponse
{
public:
	CHTTPResponse();

	// setup response code
	void SetResponseCode( const Uint32 code );

	// setup content type and size
	void SetContent( const StringAnsi& type, const StringAnsi& encoding, const Uint32 size );

	// format text data
	const StringAnsi ToString() const;

private:
	StringAnsi			m_version;
	StringAnsi			m_server;
	CHTTPDate			m_date;
	Uint32				m_code;

	// cookies
	StringAnsi			m_cookies;

	// params
	StringAnsi			m_cacheControl;	
	Bool				m_keepAlive;

	// basic flags
	Uint32				m_contentSize;
	StringAnsi			m_contentEncoding;
	StringAnsi			m_contentType;

	// get name for response code
	static const AnsiChar* GetResponseName( const Uint32 code );
};