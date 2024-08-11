/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------

#include "httpRequestHandler.h"
#include "../redNetwork/rawManager.h"

//---------------------------------

/// Connection to in-game simple HTTP server
class CHTTPConnection : public Red::Network::IRawConnectionInterface, public Red::Threads::CThread
{
public:
	CHTTPConnection( Red::Network::RawManager& network, Red::Network::TConnectionID connectionID );
	~CHTTPConnection();

	// Is this connection active ? false for closed connections
	RED_INLINE const Bool IsActive() const { return m_active.GetValue(); }

	// Get connection ID
	RED_INLINE const Red::Network::TConnectionID GetConnectionID() const { return m_connection; }

	// Pop request data, you own it now, returns NULL if empty
	class CHTTPRequest* PopRequest();

	// Push response data, should always follow a pop
	void PushResponse( class CHTTPResponseSender* response );

private:
	// IRawConnectionInterface interface
	virtual void OnDisconnected( const Red::Network::TConnectionID connectionID ) override;
	virtual void OnData( const void* data, const Uint32 dataSize, const Red::Network::Address& incomingAddress, const Red::Network::TConnectionID connectionID ) override;

	// CThread interface
	virtual void ThreadFunc() override;

	// internal lock
	Red::Threads::CMutex				m_lock;

	// incoming network connection
	Red::Network::RawManager*			m_network;
	Red::Network::TConnectionID			m_connection;

	// pending stuff (protected by lock)
	TDynArray< class CHTTPRequest* >			m_pendingRequests;
	TDynArray< class CHTTPResponseSender* >		m_pendingResponses;

	// request parser
	class CHTTPRequestParser*			m_parser;

	// active ?
	Red::Threads::CAtomic< Bool >		m_active;
};

//---------------------------------

/// In-game simple HTTP server
class CHTTPServer : public Red::Network::IRawListenerInterface
{
public:
	CHTTPServer();
	~CHTTPServer();

	// Start sever at given point
	Bool Initialize( const Uint16 port, const String& staticFilesPath );

	// Shutdown server
	void Shutdown();

	// Process synchronous requests
	void ProcessRequests();

	// Register HTTP request handler (we may have more than one)
	void RegisterHandler( IHTTPRequestHandler* handler );

	// Unregister HTTP request handler
	void UnregisterHandler( IHTTPRequestHandler* handler );

private:
	// Red::Network::IRawListenerInterface
	virtual void OnClosed( const Red::Network::TListenerID listenerID ) override;
	virtual bool OnConnection( const Red::Network::Address& incomingAddress, const Red::Network::TConnectionID connectionID, Red::Network::IRawConnectionInterface*& outConnectionInterface ) override;

	// Service function
	class CHTTPResponseSender* ServiceRequest( const class CHTTPRequest& request ) const;
	class CHTTPResponseSender* ServiceFileRequest( const class CHTTPRequest& request ) const;

	Red::Threads::CMutex				m_lock;

	// network access
	Red::Network::RawManager			m_network;
	Red::Network::TListenerID			m_listener;

	// static file access directory (server "root")
	String								m_staticFilesAbsolutePath;

	// state
	Bool								m_initialized;

	// active listeners
	TDynArray< IHTTPRequestHandler* >	m_handlers;

	// active connections
	TDynArray< CHTTPConnection* >		m_connections;
};
