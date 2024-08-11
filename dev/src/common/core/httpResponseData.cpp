/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "httpResponseData.h"

CHTTPResponseData::CHTTPResponseData( const StringAnsi& contentType, DataBlobPtr data )
	: m_resultCode( eResultCode_OK )
	, m_data( data )
	, m_contentType( contentType )
{
}

CHTTPResponseData::CHTTPResponseData( const EResultCode code )
	: m_resultCode( code )
{
}

CHTTPResponseData::~CHTTPResponseData()
{
}
