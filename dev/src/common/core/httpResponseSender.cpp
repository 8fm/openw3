/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "httpHeader.h"
#include "httpResponseData.h"
#include "httpResponseSender.h"
#include "compression/zlib.h"

//-----

CHTTPResponseSender::CHTTPResponseSender( const Uint32 code )
	: m_pos( 0 )
{
	// setup HTTP response header
	CHTTPResponse response;
	response.SetResponseCode( code );

	// build response string
	m_header = response.ToString();
	m_header += "\r\n"; // extra empty line
}

CHTTPResponseSender::CHTTPResponseSender( DataBlobPtr contentData, const StringAnsi& contentType, const Bool compress /*= true*/, const Bool allowCaching /*= false*/ )
	: m_pos( 0 )
{
	CHTTPResponse response;

	// setup data
	if ( compress )
	{
		// deflate the data
		Red::Core::Compressor::CZLib zlib;
		if ( zlib.Compress( contentData->GetData(), contentData->GetDataSize() ) )
		{
			// build new data for sending
			m_data.Reset( new CDataBlob( zlib.GetResult(), zlib.GetResultSize() ) ); // TODO: we can save one memory copy here
			Red::MemoryCopy( m_data->GetData(), zlib.GetResult(), zlib.GetResultSize() );

			// setup request data
			response.SetContent( contentType, "deflate", m_data->GetDataSize() );
			response.SetResponseCode( 200 );
		}
		else
		{
			// compression was not valid
			response.SetResponseCode( 500 );
		}
	}
	else
	{
		// setup request data using raw input data
		response.SetContent( contentType, "identity", contentData->GetDataSize() );
		response.SetResponseCode( 200 );

		// use the data directly
		m_data = contentData;
	}

	// setup HTTP response header
	m_header = response.ToString();
	m_header += "\r\n"; // extra empty line
}

CHTTPResponseSender::~CHTTPResponseSender()
{
}

void CHTTPResponseSender::GetDataToSend( const void*& outDataPtr, Uint32& outDataSize ) const
{
	const Uint32 headerSize = m_header.GetLength();
	const Uint32 dataSize = m_data ? m_data->GetDataSize() : 0;

	// header data ?
	if ( m_pos < headerSize )
	{
		const Uint32 maxToSend = (headerSize - m_pos);
		outDataPtr = m_header.AsChar() + m_pos;
		outDataSize = maxToSend;
	}
	// payload data ?
	else if ( dataSize > 0 )
	{
		const Uint32 offsetInData = m_pos - headerSize;
		const Uint32 maxToSend = (dataSize - offsetInData);
		outDataPtr = (const Uint8*) m_data->GetData() + offsetInData;
		outDataSize = maxToSend;
	}
	// error
	else
	{
		outDataPtr = nullptr;
		outDataSize = 0;
	}
}

const Bool CHTTPResponseSender::AdvanceState( const Uint32 numBytesActuallySent )
{
	// advance position
	m_pos += numBytesActuallySent;

	// all data sent ?
	const Uint32 headerSize = m_header.GetLength();
	const Uint32 dataSize = m_data ? m_data->GetDataSize() : 0;
	return ( m_pos >= ( dataSize + headerSize ) );
}

//-----
