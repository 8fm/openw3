/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fileSys.h"
#include "debugPageHandler.h"
#include "debugPageServer.h"
#include "debugPageHTMLDoc.h"

#include "httpHeader.h"
#include "httpServer.h"
#include "httpResponseData.h"
#include "basicURL.h"
#include "basicDataBlob.h"
#include "configVar.h"

namespace Config
{
	TConfigVar< Int32, Validation::IntRange<100,65535> >	cvDebugPagesPort( "DebugPages", "MainPort", 37010, eConsoleVarFlag_Developer | eConsoleVarFlag_ReadOnly );
	TConfigVar< String >									cvDebugPagesStaticContent( "DebugPages", "StaticContentDir", TXT("debug\\http\\"), eConsoleVarFlag_Developer | eConsoleVarFlag_ReadOnly );
}

CDebugPageServer::CDebugPageServer()
	: m_server( nullptr )
{
}

CDebugPageServer::~CDebugPageServer()
{
}

CDebugPageServer& CDebugPageServer::GetInstance()
{
	static CDebugPageServer theInstance;
	return theInstance;
}

Bool CDebugPageServer::Initialize()
{
	// initialize only once
	if ( !m_server )
	{
		m_server = new CHTTPServer();

		// register debug page system as HTTP request handler
		m_server->RegisterHandler( this );

		// determine the directory with static content (files that are always there)
#ifdef RED_PLATFORM_ORBIS
		const String contentDir = GFileManager->GetBaseDirectory() + TXT("bin/") + Config::cvDebugPagesStaticContent.Get();
#else
		const String contentDir = GFileManager->GetBaseDirectory() + Config::cvDebugPagesStaticContent.Get();
#endif

		// create HTTP server that will service the debug requests
		const Uint16 port = (Uint16) Config::cvDebugPagesPort.Get();
		if ( !m_server->Initialize( port, contentDir ) )
		{
			delete m_server;
			m_server = nullptr;

			ERR_CORE( TXT("Failed to initialize service handler at port %d"), port );
			return false;
		}

		// set a initialized
		LOG_CORE( TXT("Initialized service handler at port %d"), port );
	}

	// initialized
	return true;
}

void CDebugPageServer::Shutdown()
{
	if ( m_server )
	{
		// unregister
		m_server->UnregisterHandler( this );

		// close HTTP server
		m_server->Shutdown();
		delete m_server;
		m_server = nullptr;
	}
}

void CDebugPageServer::ProcessServiceCommands()
{
	if ( m_server )
	{
		m_server->ProcessRequests();
	}
}

void CDebugPageServer::RegisterDebugPage( const StringAnsi& basePath, IDebugPageHandler* handler )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// register handler
	if ( !m_handlers.Full() )
	{
		Handler info;
		info.m_basePath = basePath;
		info.m_handler = handler;

		// make sure base path ends with "/"
		if ( !info.m_basePath.EndsWith( "/" ) )
			info.m_basePath += "/";

		m_handlers.PushBack( info );
	}
}

void CDebugPageServer::UnregisterDebugPage( IDebugPageHandler* handler )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	for ( auto it = m_handlers.Begin(); it != m_handlers.End(); ++it )
	{
		if ( it->m_handler == handler )
		{
			it->m_handler = nullptr;
		}
	}
}

class CHTTPResponseData* CDebugPageServer::OnHTTPRequest( const class CHTTPRequest& request )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// we only support POST/GET in here
	if ( request.m_method != "GET" && request.m_method != "POST" )
	{
		return new class CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );
	}

	// create full URL by parsing the requested address
	CBasicURL fullURL = CBasicURL::Parse( request.m_url, true );
	fullURL.SetProtocol( "http" );
	fullURL.SetAddress( request.m_host );

	// process handlers
	const Bool isPostRequest = (request.m_method == "POST");
	for ( auto it = m_handlers.Begin(); it != m_handlers.End(); ++it )
	{
		// skip unregistered ones
		if ( !it->m_handler )
			continue;

		// valid request path ?
		if ( !request.m_url.BeginsWith( it->m_basePath ) )
			continue;;

		// create relative URL string (with host name) and split it into parts
		const StringAnsi relativePath = request.m_url.StringAfter( it->m_basePath );
		const CBasicURL relativeURL = CBasicURL::Parse( relativePath, /*relative*/ true );

		// allow the command handler to service the request
		if ( isPostRequest )
		{
			const StringAnsi contentString( (const AnsiChar*)request.m_contentData.TypedData(), request.m_contentLength );
			return it->m_handler->OnCommand( fullURL, relativeURL, contentString );
		}
		else
		{
			return it->m_handler->OnPage( fullURL, relativeURL );

		}
	}

	// HACK: index page
	if ( request.m_url == "/" || request.m_url == "/index.html" )
	{
		return HandleIndexPage( fullURL );
	}

	// no handlers found
	return new class CHTTPResponseData( CHTTPResponseData::eResultCode_NotFound );
}

class CHTTPResponseData* CDebugPageServer::HandleIndexPage( const CBasicURL& fullURL )
{
	CDebugPageHTMLResponse response( fullURL );
	{
		CDebugPageHTMLDocument doc( response, "RedEngine Runtime Debug System" );

		doc << "<p>Welcome in the arcane debug, select your weapon:</p>";

		// list of of the registered debug page handlers
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

			// collect categories
			TDynArray< StringAnsi > categories;
			for ( auto it = m_handlers.Begin(); it != m_handlers.End(); ++it )
			{
				if ( it->m_handler && it->m_handler->HasIndexPage() )
				{
					categories.PushBackUnique( it->m_handler->GetCategory() );
				}
			}

			// sort categories by name
			::Sort( categories.Begin(), categories.End() );

			// show debug groups with links to index pages of each debug page
			for ( const StringAnsi& debugCategory : categories )
			{
				// category 
				doc << "<span class=\"debugcategory\">";
				doc << debugCategory;
				doc << "</span>";

				// elements
				doc << "<ul class=\"debuglist\">";

				for ( auto it = m_handlers.Begin(); it != m_handlers.End(); ++it )
				{
					if ( it->m_handler && it->m_handler->HasIndexPage() && it->m_handler->GetCategory() == debugCategory )
					{
						doc.Open( "li" ).Doc().Link( "%hs", it->m_basePath.AsChar()+1 ).Write( it->m_handler->GetTitle().AsChar() );
					}
				}

				doc << "</ul>";
			}
		}
	}
	return response.GetData();
}