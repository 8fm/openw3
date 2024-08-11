/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "staticarray.h"

/// Request parser - helper structure that can be feed bytestream directly from network connection
class CHTTPRequestParser
{
public:
	CHTTPRequestParser();
	~CHTTPRequestParser();

	// Reset parsing state
	void Reset();

	// Append data, returns true if we managed to parse the request
	void Append( const void* data, const Uint32 size, TDynArray< class CHTTPRequest >& outRequests );

private:
	static const Uint32			MAX_REQUEST		= 4096;
	static const Uint32			MAX_CONTENT		= 4096;

	static const Uint8			CHAR_CR = 10;
	static const Uint8			CHAR_LF = 13;

	// helper buffer
	TStaticArray< Uint8, MAX_REQUEST >	m_buffer;

	// parsing state
	CHTTPRequest				m_contentRequest;
	Uint32						m_contentSizeLeft;

	// eating
	Uint32						m_eatBytesLeft;
};