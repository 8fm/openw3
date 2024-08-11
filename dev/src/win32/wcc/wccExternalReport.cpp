#include "build.h"

CookerExternalReporter* GExternalReporter = NULL;

CookerExternalReporter::CookerExternalReporter()
{
	// Connect to server
	Reconnect();

	// Install as logging interface
	//SLog::GetInstance().AddOutput( this );
}

CookerExternalReporter::~CookerExternalReporter()
{
	// Remove from logging interface
	//SLog::GetInstance().RemoveOutput( this );

	// Close connection
// 	if ( m_stream )
// 	{
// 		delete m_stream;
// 		m_stream = NULL;
// 	}
}

void CookerExternalReporter::SetCookedResource( const String& resource )
{
	// Set new resource
	if ( m_resource != resource )
	{
		m_resource = resource;

		// Send
// 		if ( m_stream && !m_resource.Empty() )
// 		{
// 			// Format packet 
// 			CNetworkPacket debugPacket( TXT("CookerFile") );
// 			debugPacket.WriteString( m_resource );
// 
// 			// Send the packet
// 			m_stream->Send( debugPacket );
// 		}
	}
}

void CookerExternalReporter::SendStatus( const TCHAR* txt, ... )
{
//	if ( m_stream )
	{
		// Format message
		va_list arglist;
		va_start( arglist, txt );
		Char formattedBuf[ 1024 ];
		vswprintf(formattedBuf,  ARRAY_COUNT(formattedBuf), txt, arglist ); 

		// Format packet 
// 		CNetworkPacket debugPacket( TXT("CookerStatus") );
// 		debugPacket.WriteString( formattedBuf );
// 
// 		// Send the packet
// 		if ( !m_stream->Send( debugPacket ) )
// 		{
// 			Reconnect();
// 		}
	}
}

void CookerExternalReporter::SendProgress( Uint32 cur, Uint32 total )
{
// 	if ( m_stream )
// 	{
// 		// Format packet 
// 		CNetworkPacket debugPacket( TXT("CookerProgress") );
// 		debugPacket.WriteDWord( cur );
// 		debugPacket.WriteDWord( total );
// 
// 		// Send the packet
// 		if ( !m_stream->Send( debugPacket ) )
// 		{
// 			Reconnect();
// 		}
// 	}
}

void CookerExternalReporter::SendError( const TCHAR* txt, ... )
{
// 	if ( m_stream )
// 	{
// 		// Format message
// 		va_list arglist;
// 		va_start( arglist, txt );
// 		Char formattedBuf[ 1024 ];
// 		vswprintf(formattedBuf,  ARRAY_COUNT(formattedBuf), txt, arglist ); 
// 
// 		// Format packet 
// 		CNetworkPacket debugPacket( TXT("CookerError") );
// 		debugPacket.WriteString( m_resource );
// 		debugPacket.WriteString( formattedBuf );
// 
// 		// Send the packet
// 		if ( !m_stream->Send( debugPacket ) )
// 		{
// 			Reconnect();
// 		}
// 	}
}

void CookerExternalReporter::SendErrorRaw( const TCHAR* txt )
{
// 	if ( m_stream )
// 	{
// 		// Format packet 
// 		CNetworkPacket debugPacket( TXT("CookerError") );
// 		debugPacket.WriteString( m_resource );
// 		debugPacket.WriteString( txt );
// 
// 		// Send the packet
// 		if ( !m_stream->Send( debugPacket ) )
// 		{
// 			Reconnect();
// 		}
// 	}
}

void CookerExternalReporter::SendWarning( const TCHAR* txt, ... )
{
// 	if ( m_stream )
// 	{
// 		// Format message
// 		va_list arglist;
// 		va_start( arglist, txt );
// 		Char formattedBuf[ 1024 ];
// 		vswprintf(formattedBuf,  ARRAY_COUNT(formattedBuf), txt, arglist ); 
// 
// 		// Format packet 
// 		CNetworkPacket debugPacket( TXT("CookerWarning") );
// 		debugPacket.WriteString( m_resource );
// 		debugPacket.WriteString( formattedBuf );
// 
// 		// Send the packet
// 		if ( !m_stream->Send( debugPacket ) )
// 		{
// 			Reconnect();
// 		}
// 	}
}

void CookerExternalReporter::SendWarningRaw( const TCHAR* txt )
{
// 	if ( m_stream )
// 	{
// 		// Format packet 
// 		CNetworkPacket debugPacket( TXT("CookerWarning") );
// 		debugPacket.WriteString( m_resource );
// 		debugPacket.WriteString( txt );
// 
// 		// Send the packet
// 		if ( !m_stream->Send( debugPacket ) )
// 		{
// 			Reconnect();
// 		}
// 	}
}

void CookerExternalReporter::Reconnect()
{
// 	// Close current connection
// 	if ( m_stream )
// 	{
// 		delete m_stream;
// 		m_stream = NULL;
// 	}
// 
// 	// Try to connect to server
// 	CNetworkAddress address( TXT("127.0.0.1:47002") );
// 	m_stream = SNetwork::GetInstance().ConnectStream( address );
// 
// 	// Connected
// 	if ( m_stream )
// 	{
// 		LOG_WCC( TXT("Detected master application at %s"), m_stream->GetRemoveAddress().ToString().AsChar() );
// 	}
}
/*
void CookerExternalReporter::Write( const CName& type, const Char* str, const Uint64& tick, const EngineTime& engineTime )
{
	if ( type == TXT("Error") )
	{
		SendErrorRaw( str );
	}

	if ( type == TXT("Warning") )
	{
		SendWarningRaw( str );
	}
}
*/