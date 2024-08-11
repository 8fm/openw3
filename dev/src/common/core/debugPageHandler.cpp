/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugPageHandler.h"
#include "debugPageServer.h"
#include "debugPageHTMLDoc.h"
#include "httpResponseData.h"
#include "basicURL.h"
#include "basicDataBlob.h"

//-------

IDebugPageHandler::IDebugPageHandler( const StringAnsi& basicURL, const Bool autoRegister /*= true*/ )
{
	if ( autoRegister )
	{
		CDebugPageServer::GetInstance().RegisterDebugPage( basicURL, this );
	}
}

IDebugPageHandler::~IDebugPageHandler()
{
	CDebugPageServer::GetInstance().UnregisterDebugPage( this );
}

class CHTTPResponseData* IDebugPageHandler::OnCommand( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl, const StringAnsi& content )
{
	// command not implemented
	return new CHTTPResponseData( CHTTPResponseData::eResultCode_NotImplemented );
}

//-------

IDebugPageHandlerHTML::IDebugPageHandlerHTML( const StringAnsi& basicURL, const Bool autoRegister /*= true*/ )
	: IDebugPageHandler( basicURL, autoRegister )
{
}

class CHTTPResponseData* IDebugPageHandlerHTML::OnPage( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl )
{
	CDebugPageHTMLResponse response( fullURL );
	{
		CDebugPageHTMLDocument doc( response, GetTitle() );
		if ( !OnFillPage( fullURL, relativeUrl, doc ) )
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_NotImplemented );
	}

	return response.GetData();
}

//-------
