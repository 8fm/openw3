/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once


//---------------------------------

/// HTTP request handler
class IHTTPRequestHandler
{
public:
	virtual ~IHTTPRequestHandler() {};

	// Handle request, called always from main thread in safe place. Returning valid response means that the request was handled.
	virtual class CHTTPResponseData* OnHTTPRequest( const class CHTTPRequest& request ) = 0;
};

//---------------------------------
