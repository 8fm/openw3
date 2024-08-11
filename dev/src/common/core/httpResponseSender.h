/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------

#include "basicDataBlob.h"

//---------------------------------

/// HTTP helper used to send data through network, supports partial data sending
class CHTTPResponseSender : public Red::NonCopyable
{
public:
	CHTTPResponseSender( const Uint32 errorCode ); // for raw errors
	CHTTPResponseSender( DataBlobPtr contentData, const StringAnsi& contentType, const Bool compress = true, const Bool allowCaching = false ); // for data
	~CHTTPResponseSender();

	// Do we have actual content ?
	const Bool HasContent() const { return m_data && (m_data->GetDataSize() > 0); }

	// Get data to send, returns 0 if there's no more data to send
	void GetDataToSend( const void*& outDataPtr, Uint32& outDataSize ) const;

	// Advance state by the amount of that that was actually sent, returns true when finished
	const Bool AdvanceState( const Uint32 numBytesActuallySent );

private:
	// binary version of HTTP response header + payload (may be empty)
	StringAnsi				m_header;
	DataBlobPtr				m_data;

	// current write pointer
	Uint32					m_pos;
};

//---------------------------------
