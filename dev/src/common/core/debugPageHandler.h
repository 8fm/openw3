/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "basicURL.h"
#include "basicDataBlob.h"

/// Service command handler
class IDebugPageHandler
{
public:
	IDebugPageHandler( const StringAnsi& basicURL, const Bool autoRegister = true );
	virtual ~IDebugPageHandler();

	//! Get debug page title
	virtual StringAnsi GetTitle() const = 0;

	//! Get debug page category
	virtual StringAnsi GetCategory() const = 0;

	//! Do we have an index page ? (if so we are directly available in the debug menu)
	virtual Bool HasIndexPage() const { return false; }

	//! Process POST request - change the state of engine, get value of variable, etc, NO PAGE REFRESH
	virtual class CHTTPResponseData* OnCommand( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl, const StringAnsi& content );

	//! Process GET command - display a page or sth, commands are always passed via URLs, return NULL if not handled
	virtual class CHTTPResponseData* OnPage( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl ) = 0;
};

/// Service command handler that always outputs HTML window
class IDebugPageHandlerHTML : public IDebugPageHandler
{
public:
	IDebugPageHandlerHTML( const StringAnsi& basicURL, const Bool autoRegister = true );
	virtual ~IDebugPageHandlerHTML() {};

	// Fill the HTML result page
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) = 0;;

private:
	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual class CHTTPResponseData* OnPage( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl );
};
