/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "filePath.h"
#include "scopedPtr.h"
#include "httpHeader.h"
#include "httpServer.h"
#include "httpRequestParser.h"
#include "httpResponseData.h"
#include "httpResponseSender.h"
#include "configVar.h"

//-----

namespace Config
{
	TConfigVar< Int32, Validation::IntRange< 0, 3600 > >		cvHTTPKeepAliveTime( "HTTP", "KeepAliveTime", 120, eConsoleVarFlag_ReadOnly | eConsoleVarFlag_Developer );
	TConfigVar< Int32, Validation::IntRange< 0, INT_MAX > >		cvHTTPMaxFileTransferSizeKB( "HTTP", "MaxFileSizeKB", 4096, eConsoleVarFlag_ReadOnly | eConsoleVarFlag_Developer );
	TConfigVar< Bool >											cvHTTPAllowAllFiles( "HTTP", "AllowAllFiles", false, eConsoleVarFlag_ReadOnly | eConsoleVarFlag_Developer );
}

//-----

CHTTPConnection::CHTTPConnection( Red::Network::RawManager& network, Red::Network::TConnectionID connectionID )
	: Red::Threads::CThread( "HTTPConnection" )
	, m_network( &network ) 
	, m_connection( connectionID )
	, m_parser( new CHTTPRequestParser() )
	, m_active( true )
{
	InitThread();
}

CHTTPConnection::~CHTTPConnection()
{
	// close thread
	m_active.SetValue( false );
	JoinThread();

	// close input parser
	delete m_parser;
}

class CHTTPRequest* CHTTPConnection::PopRequest()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	if ( m_pendingRequests.Empty() )
		return nullptr;

	// pop the first request from queue
	auto ret = m_pendingRequests.Front();
	m_pendingRequests.RemoveAt(0);

	return ret;
}

void CHTTPConnection::PushResponse( class CHTTPResponseSender* response )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	m_pendingResponses.PushBack( response );
}

void CHTTPConnection::OnDisconnected( const Red::Network::TConnectionID connectionID )
{
	m_active.SetValue( false );
}

void CHTTPConnection::OnData( const void* data, const Uint32 dataSize, const Red::Network::Address& incomingAddress, const Red::Network::TConnectionID connectionID )
{
	// parse new requests from network stream
	TDynArray< CHTTPRequest > newRequests;
	m_parser->Append( data, dataSize, newRequests );

	// add any newly parsed requests to the list
	if ( !newRequests.Empty() )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		for ( const auto& request : newRequests )
		{
			CHTTPRequest* newRequest = new CHTTPRequest( request );
			m_pendingRequests.PushBack( newRequest );
		}
	}
}

void CHTTPConnection::ThreadFunc()
{
	while ( m_active.GetValue() )
	{
		// get the shit to send
		CHTTPResponseSender* httpResponse = nullptr;
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
			if ( !m_pendingResponses.Empty() )
			{
				httpResponse = m_pendingResponses.Front();
			}
		}

		// send data
		if ( httpResponse != nullptr )
		{
			// get data that is left to send
			// this will only return pointer to the data that is not yet send
			const void* payloadData = nullptr;
			Uint32 payloadSize = 0;
			httpResponse->GetDataToSend( payloadData, payloadSize );

			// send data
			if ( payloadSize > 0 )
			{
				const Uint32 numberOfBytesSent = m_network->Send( m_connection, payloadData, payloadSize );
				if ( numberOfBytesSent > 0 )
				{
					// advance internal state, this returns TRUE if we've send the whole message
					if ( httpResponse->AdvanceState( numberOfBytesSent ) )
					{
						// we've sent the full data, delete the response sender from the outgoing queue
						Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
						RED_FATAL_ASSERT( m_pendingResponses.Front() == httpResponse, "Illegal queue state" );
						m_pendingResponses.RemoveAt(0);

						// was this an error response ? if so, close the connection
						if ( !httpResponse->HasContent() )
						{
							LOG_CORE( TXT("HTTP connection will close") );

							// close the connection
							m_network->CloseConnection( m_connection );
							m_connection = 0;

							// exit the thread
							break;
						}
					}
				}
			}
		}
		else
		{
			// nothing to send yet, schedule a long wait
			Red::Threads::SleepOnCurrentThread(50);
		}
	}
}

//-----

CHTTPServer::CHTTPServer()
	: m_initialized( false )
{
}

CHTTPServer::~CHTTPServer()
{

}

Bool CHTTPServer::Initialize( const Uint16 port, const String& staticFilesPath )
{
	// already initialized
	if ( m_initialized )
		return false;

	// initialize network
	m_network.Initialize();

	// create HTTP listener
	m_listener = m_network.CreateListener( port, this );
	if ( !m_listener )
	{
		m_network.Shutdown(); // make sure the network interface is closed
		return false;
	}

	// initialized
	m_staticFilesAbsolutePath = staticFilesPath;
	m_initialized = true;
	return true;
}

void CHTTPServer::Shutdown()
{
	// stop network service
	if ( m_initialized )
	{
		// close all of the connections
		m_connections.ClearPtr();

		// close listener
		if ( m_listener )
		{
			m_network.CloseListener( m_listener );
			m_listener = 0;
		}

		// close network
		m_network.Shutdown();

		// mark as closed
		m_initialized = false;
	}
}

void CHTTPServer::ProcessRequests()
{
	TDynArray< CHTTPConnection* > activeConnections;

	// filter out closed connections, prepare list of connections to process
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		// prepare array
		activeConnections.Reserve( m_connections.Size() );

		// process closed connections
		TDynArray< CHTTPConnection* > connections( std::move( m_connections ) );
		for ( auto it : connections )
		{
			if ( !it->IsActive() )
			{
				// close connection
				m_network.CloseConnection( it->GetConnectionID() );
				delete it;
			}
			else
			{
				// keep in the list
				m_connections.PushBack( it );
				activeConnections.PushBack( it );
			}
		}
	}

	// process requests from active connections
	for ( auto it : activeConnections )
	{
		CHTTPRequest* request = it->PopRequest();
		while ( request )
		{
			auto response = ServiceRequest( *request );
			RED_FATAL_ASSERT( response != nullptr, "There should always be a valid response for every request" );

			// send response
			it->PushResponse( response );

			// get new request
			request = it->PopRequest();
		}
	}
}

class CHTTPResponseSender* CHTTPServer::ServiceRequest( const class CHTTPRequest& request ) const
{
	// URL to get a static file
	if ( request.m_url.BeginsWith( "/static/") && request.m_method == "GET" )
	{
		return ServiceFileRequest( request );
	}

	// go through the registered handlers
	for ( auto& it : m_handlers )
	{
		CHTTPResponseData* retPtr = it->OnHTTPRequest( request );
		if ( retPtr )
		{
			Red::TScopedPtr< CHTTPResponseData > ret( retPtr );
			if ( retPtr->GetData() )
			{
				return new CHTTPResponseSender( ret->GetData(), ret->GetContentType() );
			}
			else
			{
				return new CHTTPResponseSender( ret->GetResultCode() );
			}
		}
	}

	// not handled
	return new CHTTPResponseSender( 404 );
}

class CHTTPResponseSender* CHTTPServer::ServiceFileRequest( const class CHTTPRequest& request ) const
{
	// no file serving dir
	if ( m_staticFilesAbsolutePath.Empty() )
		return new CHTTPResponseSender( 400 ); // bad request

	// Get the relative path to the file being requested
	StringAnsi relativePath;
	CFilePath::GetConformedPath( request.m_url.StringAfter( "static/" ), relativePath );

	// conform the path
	String filePath = m_staticFilesAbsolutePath;
	filePath += ANSI_TO_UNICODE( relativePath.AsChar() ); 

	// LOG ALL REQUESTS - to have an evidence of people snooping
	LOG_CORE( TXT("HTTP file request: '%ls'"), filePath.AsChar() );

	// open the file - always use RELATIVE PATH - to prevent people from snooping on your hard drive !!!
	Red::TScopedPtr< IFile > inputFile( GFileManager->CreateFileReader(	filePath, FOF_AbsolutePath ) );
	if ( !inputFile )
		return new CHTTPResponseSender( 404 ); // not found

	// do not send files that are to big or empty
	const Uint32 fileSize = (Uint32) inputFile->GetSize();
	const Uint32 maxSize = Config::cvHTTPMaxFileTransferSizeKB.Get() * 1024;
	if ( (fileSize > maxSize) || !fileSize )
		return new CHTTPResponseSender( 403 ); // forbidden

	// Only few types of files are allowed 
	// TODO: maybe we should have this somewhere else
	StringAnsi contentType;
	const StringAnsi fileExt = StringHelpers::GetFileExtension( relativePath );
	if ( fileExt == "js" )
	{
		contentType = "application/javascript";
	}
	else if ( fileExt == "css" )
	{
		contentType = "text/css";
	}
	else if ( fileExt == "xml" )
	{
		contentType = "text/xml";
	}
	else if ( fileExt == "txt" || fileExt == "csv ")
	{
		contentType = "text/plain";
	}
	else if ( fileExt == "json" )
	{
		contentType = "application/json";
	}
	else if ( fileExt == "png" )
	{
		contentType = "image/png";
	}
	else if ( fileExt == "jpg" || fileExt == "jpeg")
	{
		contentType = "image/jpeg";
	}
	else if ( fileExt == "gif" )
	{
		contentType = "image/gif";
	}
	else
	{
		// for security we may disallow getting files of any format
		if ( !Config::cvHTTPAllowAllFiles.Get() )
		{
			return new CHTTPResponseSender( 403 ); // forbidden
		}

		// if it's allowed, use a generic format for it
		contentType = "application/octet-stream";
	}

	// send the content of the file
	DataBlobPtr data( new CDataBlob( fileSize ) );
	inputFile->Serialize( data->GetData(), data->GetDataSize() );
	return new CHTTPResponseSender( data, contentType );
}

void CHTTPServer::RegisterHandler( IHTTPRequestHandler* handler )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	m_handlers.PushBack( handler );
}

void CHTTPServer::UnregisterHandler( IHTTPRequestHandler* handler )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	m_handlers.Remove( handler );
}

void CHTTPServer::OnClosed( const Red::Network::TListenerID listenerID )
{
	// close the listener
	if ( m_listener == listenerID )
	{
		WARN_CORE( TXT("HTTP server socket closed") );
		m_listener = 0;
	}
}

bool CHTTPServer::OnConnection( const Red::Network::Address& incomingAddress, const Red::Network::TConnectionID connectionID, Red::Network::IRawConnectionInterface*& outConnectionInterface )
{
	// create new connection wrapper
	CHTTPConnection* connection = new CHTTPConnection( m_network, connectionID );
	outConnectionInterface = static_cast< Red::Network::IRawConnectionInterface* >( connection );

	// log connection
	AnsiChar buf[64];
	incomingAddress.GetIp(buf, ARRAY_COUNT_U32(buf) );
	WARN_CORE( TXT("HTTP server new connection %d from %hs"), connectionID, buf );

	// add to list of active connections
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
		m_connections.PushBack( connection );
	}

	// connection created
	return true;
}
