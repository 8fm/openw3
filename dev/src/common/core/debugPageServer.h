/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "staticArray.h"
#include "basicURL.h"
#include "basicDataBlob.h"
#include "debugPageHandler.h"
#include "httpRequestHandler.h"

/// Service command system
class CDebugPageServer : public Red::NonCopyable, public IHTTPRequestHandler
{
public:
	// Get instance of the command server
	static CDebugPageServer& GetInstance();

	// Initialize server - creates the network listener and the processing thread
	Bool Initialize();

	// Shutdown system - closes the network
	void Shutdown();

	// Register debug page
	void RegisterDebugPage( const StringAnsi& basePath, IDebugPageHandler* handler );

	// Unregister debug page
	void UnregisterDebugPage( IDebugPageHandler* handler );

	// Process commands, called in Tick from main thread
	void ProcessServiceCommands();

protected:
	CDebugPageServer();
	~CDebugPageServer();

	static const Uint32				MAX_HANDLERS = 128;

	struct Handler
	{
		StringAnsi				m_basePath;
		IDebugPageHandler*		m_handler;
	};

	class CHTTPServer*						m_server;

	Red::Threads::CMutex					m_lock;
	TStaticArray< Handler, MAX_HANDLERS >	m_handlers;

	// IHTTPRequestHandler interface
	virtual class CHTTPResponseData* OnHTTPRequest( const class CHTTPRequest& request ) override;

	// Handle special pages
	class CHTTPResponseData* HandleIndexPage( const CBasicURL& fullURL );
};
