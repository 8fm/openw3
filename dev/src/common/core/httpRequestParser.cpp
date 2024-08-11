/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "httpHeader.h"
#include "httpRequestParser.h"

CHTTPRequestParser::CHTTPRequestParser()
	: m_contentSizeLeft( 0 )
	, m_eatBytesLeft( 0 )
{
}

CHTTPRequestParser::~CHTTPRequestParser()
{
}

void CHTTPRequestParser::Reset()
{
	m_contentRequest = CHTTPRequest();
	m_contentSizeLeft = 0;
	m_eatBytesLeft = 0;
	m_buffer.Clear();
}

void CHTTPRequestParser::Append( const void* data, const Uint32 size, TDynArray< CHTTPRequest >& outRequests )
{
	// append chars parsed from the stream
	// each full request ends with \x10\x13
	for ( Uint32 i=0; i<size; ++i )
	{
		const Uint8 d = ((const Uint8*) data)[i];

		// dumb byte eating
		if ( m_eatBytesLeft > 0 )
		{
			m_eatBytesLeft -= 1;
			continue;
		}

		// eat content first, NOTE: we may receive merged shit here
		if ( m_contentSizeLeft > 0 )
		{
			// received data are pare of the content shit, append it
			m_contentRequest.m_contentData.PushBack( d );

			// whole content parsed, report request
			if ( 0 == --m_contentSizeLeft )
			{
				outRequests.PushBack( m_contentRequest );
				m_contentRequest = CHTTPRequest();
			}

			// skip to next byte
			continue;
		}

		// parse header
		if ( m_buffer.Full() )
		{
			Reset();
		}

		// filter out CRLF -> LF to simplify code
		if ( d == CHAR_LF && !m_buffer.Empty() && m_buffer.Back() == CHAR_CR )
			m_buffer.PopBack(); // remove the CR from the stream

		// append at the end of the stream
		m_buffer.PushBack( d );

		// end of line ? 
		const Uint32 len = m_buffer.Size();
		if ( len >= 2 && m_buffer.Back() == CHAR_LF )
		{
			// end of request ? (empty line - two LF in a row) ?
			if ( m_buffer[ len-2 ] == CHAR_LF )
			{
				// strip
				m_buffer[ len-1 ] = 0;

				// parse request data
				CHTTPRequest req;
				const AnsiChar* fullRequestText = (const AnsiChar*) m_buffer.Data();
				if ( req.Parse( fullRequestText ) )
				{
					// do we have content to load ?
					if ( req.m_contentLength > 0 )
					{
						// start content parsing
						m_contentSizeLeft = req.m_contentLength;
						m_contentRequest = req;

						// eat the first byte that follows (line ending)
						m_eatBytesLeft = 1;

						// reserve memory for the content
						m_contentRequest.m_contentData.Reserve( req.m_contentLength );
					}
					else
					{
						// no content, use directly
						outRequests.PushBack( req );
					}
				}

				// clear shit, start new request parsing
				m_buffer.Clear();
			}
		}
	}
}

