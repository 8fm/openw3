/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "editorCompilationHelper.h"
#include "packetUtils.h"
#include "events/eventGoto.h"
#include "app.h"
#include "editorConnectionHelper.h"

wxDEFINE_EVENT( ssEVT_COMPILATION_STARTED_EVENT, CCompilationStartedEvent );
wxDEFINE_EVENT( ssEVT_COMPILATION_ENDED_EVENT, CCompilationEndedEvent );
wxDEFINE_EVENT( ssEVT_COMPILATION_LOG_EVENT, CCompilationLogEvent );
wxDEFINE_EVENT( ssEVT_COMPILATION_ERROR_EVENT, CCompilationErrorEvent );
wxDEFINE_EVENT( ssEVT_PATH_CONFIRMATION_EVENT, CPathConfirmationEvent );
wxDEFINE_EVENT( ssEVT_PACKAGE_SYNC_LISTING_EVENT, CPackageSyncListingEvent );

wxIMPLEMENT_DYNAMIC_CLASS( CCompilationStartedEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CCompilationEndedEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CCompilationLogEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CCompilationErrorEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CPathConfirmationEvent, wxEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CPackageSyncListingEvent, wxEvent );

//////////////////////////////////////////////////////////////////////////

CCompilationStartedEvent::CCompilationStartedEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CCompilationStartedEvent::CCompilationStartedEvent( Red::System::Bool strictMode, Red::System::Bool finalBuild )
:	wxEvent( wxID_ANY, ssEVT_COMPILATION_STARTED_EVENT )
,	m_strictMode( strictMode )
,	m_finalBuild( finalBuild )
{

}

CCompilationStartedEvent::~CCompilationStartedEvent()
{

}

//////////////////////////////////////////////////////////////////////////


CCompilationEndedEvent::CCompilationEndedEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CCompilationEndedEvent::CCompilationEndedEvent( Red::System::Bool success )
:	wxEvent( wxID_ANY, ssEVT_COMPILATION_ENDED_EVENT )
,	m_success( success )
{

}

CCompilationEndedEvent::~CCompilationEndedEvent()
{

}

//////////////////////////////////////////////////////////////////////////

CCompilationLogEvent::CCompilationLogEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CCompilationLogEvent::CCompilationLogEvent( const wxString& message )
:	wxEvent( wxID_ANY, ssEVT_COMPILATION_LOG_EVENT )
,	m_message( message )
{
}

CCompilationLogEvent::CCompilationLogEvent( const CCompilationLogEvent& event )
:	wxEvent( wxID_ANY, ssEVT_COMPILATION_LOG_EVENT )
,	m_message( event.m_message )
{
}

CCompilationLogEvent::~CCompilationLogEvent()
{

}

//////////////////////////////////////////////////////////////////////////

CCompilationErrorEvent::CCompilationErrorEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CCompilationErrorEvent::CCompilationErrorEvent( ESeverity severity, Red::System::Int32 line, const wxString& file, const wxString& message )
:	wxEvent( wxID_ANY, ssEVT_COMPILATION_ERROR_EVENT )
,	m_severity( severity )
,	m_line( line )
,	m_file( file )
,	m_message( message )
{

}

CCompilationErrorEvent::CCompilationErrorEvent( const CCompilationErrorEvent& event )
:	wxEvent( wxID_ANY, ssEVT_COMPILATION_ERROR_EVENT )
,	m_severity( event.m_severity )
,	m_line( event.m_line )
,	m_file( event.m_file )
,	m_message( event.m_message )
{

}

CCompilationErrorEvent::~CCompilationErrorEvent()
{

}

//////////////////////////////////////////////////////////////////////////

CPathConfirmationEvent::CPathConfirmationEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CPathConfirmationEvent::CPathConfirmationEvent( const wxString& path )
:	wxEvent( wxID_ANY, ssEVT_PATH_CONFIRMATION_EVENT )
,	m_path( path )
{

}

CPathConfirmationEvent::CPathConfirmationEvent( const CPathConfirmationEvent& event )
:	wxEvent( wxID_ANY, ssEVT_PATH_CONFIRMATION_EVENT )
,	m_path( event.m_path )
{

}

CPathConfirmationEvent::~CPathConfirmationEvent()
{

}

//////////////////////////////////////////////////////////////////////////

CPackageSyncListingEvent::CPackageSyncListingEvent( wxEventType commandType, int winid  )
:	wxEvent( commandType, winid )
{

}

CPackageSyncListingEvent::CPackageSyncListingEvent( const map< wxString, wxString >& packages )
:	wxEvent( wxID_ANY, ssEVT_PACKAGE_SYNC_LISTING_EVENT )
,	m_map( packages )
{

}

CPackageSyncListingEvent::CPackageSyncListingEvent( map< wxString, wxString >&& packages )
:	wxEvent( wxID_ANY, ssEVT_PACKAGE_SYNC_LISTING_EVENT )
,	m_map( std::move( packages ) )
{

}

CPackageSyncListingEvent::~CPackageSyncListingEvent()
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS( CEditorCompilationHelper, wxEvtHandler );

CEditorCompilationHelper::CEditorCompilationHelper()
{
}

CEditorCompilationHelper::~CEditorCompilationHelper()
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_SCRIPT, this );
		Red::Network::Manager::GetInstance()->UnregisterListener( RED_NET_CHANNEL_SCRIPT_COMPILER, this );
	}
}

Red::System::Bool CEditorCompilationHelper::Initialize( const wxChar* ip )
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Address address( ip,  wxTheSSApp->GetNetworkPort() );
		Red::Network::Manager::GetInstance()->ConnectTo( address, RED_NET_CHANNEL_SCRIPT_COMPILER );
		Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_COMPILER, this );
		Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT, this );

		Red::Network::ChannelPacket confirmPathPacket( RED_NET_CHANNEL_SCRIPT_COMPILER );
		RED_VERIFY( confirmPathPacket.WriteString( "RootPath" ) );
		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, confirmPathPacket );

		return true;
	}

	return false;
}

void CEditorCompilationHelper::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	if( Red::System::StringCompare( channelName, RED_NET_CHANNEL_SCRIPT_COMPILER ) == 0 )
	{
		Red::System::AnsiChar type[ 32 ];
		packet.ReadString( type, ARRAY_COUNT_U32( type ) );

		if( Red::System::StringCompare( type, "log" ) == 0 )
		{
			wxString message;
			PacketUtilities::ReadString( packet, message );

			CCompilationLogEvent* event = new CCompilationLogEvent( message );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( type, "warn" ) == 0 )
		{
			wxString file;
			wxString message;
			Red::System::Int32 line;

			RED_VERIFY( packet.Read( line ) );
			PacketUtilities::ReadString( packet, file );
			PacketUtilities::ReadString( packet, message );

			CCompilationErrorEvent* event = new CCompilationErrorEvent( CCompilationErrorEvent::Severity_Warning, line, file, message );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( type, "error" ) == 0 )
		{
			wxString file;
			wxString message;
			Red::System::Int32 line;

			RED_VERIFY( packet.Read( line ) );
			PacketUtilities::ReadString( packet, file );
			PacketUtilities::ReadString( packet, message );

			CCompilationErrorEvent* event = new CCompilationErrorEvent( CCompilationErrorEvent::Severity_Error, line, file, message );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( type, "started" ) == 0 )
		{
			Red::System::Bool strictMode;
			Red::System::Bool finalMode;

			RED_VERIFY( packet.Read( finalMode ) );
			RED_VERIFY( packet.Read( strictMode ) );

			CCompilationStartedEvent* event = new CCompilationStartedEvent( strictMode, finalMode );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( type, "finished" ) == 0 )
		{
			Red::System::Bool compilationFailed;

			RED_VERIFY( packet.Read( compilationFailed ) );

			CCompilationEndedEvent* event = new CCompilationEndedEvent( !compilationFailed );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( type, "Goto" ) == 0 )
		{
			wxString file;
			Red::System::Int32 line;

			PacketUtilities::ReadString( packet, file );
			RED_VERIFY( packet.Read( line ) );

			CGotoEvent* event = new CGotoEvent( file, line );
			QueueEvent( event );
		}
		else if( Red::System::StringCompare( type, "RootPathConfirm" ) == 0 )
		{
			wxString rootPath;

			PacketUtilities::ReadString( packet, rootPath );

			CPathConfirmationEvent* event = new CPathConfirmationEvent( rootPath );
			QueueEvent( event );
		}
	}
	else if( Red::System::StringCompare( channelName, RED_NET_CHANNEL_SCRIPT ) == 0 )
	{
		Red::System::AnsiChar type[ 32 ];
		packet.ReadString( type, ARRAY_COUNT_U32( type ) );

		if( Red::System::StringCompare( type, "pkgSyncListing" ) == 0 )
		{
			map< wxString, wxString > packages;

			Red::System::Uint32 numPackages = 0;
			RED_VERIFY( packet.Read( numPackages ) );

			for( Red::System::Uint32 i = 0; i < numPackages; ++i )
			{
				wxString name;
				wxString path;

				PacketUtilities::ReadString( packet, name );
				PacketUtilities::ReadString( packet, path );

				packages[ name ] = path;
			}

			CPackageSyncListingEvent* event = new CPackageSyncListingEvent( std::move( packages ) );
			QueueEvent( event );
		}
	}
}
