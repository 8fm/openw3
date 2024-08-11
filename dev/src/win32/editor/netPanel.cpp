
#include "build.h"
#include "netPanel.h"

//#pragma optimize("",off)

BEGIN_EVENT_TABLE( CEdAANetPanel, wxPanel )
	EVT_TOGGLEBUTTON( XRCID( "netBtn"), CEdAANetPanel::OnNetConnected )
	EVT_IDLE( CEdAANetPanel::OnIdle )
END_EVENT_TABLE()

CEdAANetPanel::CEdAANetPanel( wxWindow* parent )
	: m_running( false )
	, m_connected( false )
{
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("AANetPanel") );

	m_log = XRCCTRL( *this, "netMsg", wxTextCtrl );
	m_status = XRCCTRL( *this, "netText", wxStaticText );

	RefreshStatus();
}

CEdAANetPanel::~CEdAANetPanel()
{
	
}

Bool CEdAANetPanel::ConnectNetwork()
{
	m_running = m_server.Initialize( 3000, true );

	ASSERT( !m_connected );
	if ( m_connected )
	{
		SetConnection( false );
	}

	if ( m_running )
	{
		Log(_("Server is listening..."));
	}
	else
	{
		Log(_("Could not initialize server") );
	}

	RefreshStatus();

	return m_running;
}

void CEdAANetPanel::DisconnectNetwork()
{
	if ( m_connected )
	{
		SetConnection( false );
	}

	if ( !m_server.Shutdown() )
	{
		Log(_("Server could not shutdown properly"));
	}
	else
	{
		Log(_("Server shutdown") );
	}

	m_running = false;

	RefreshStatus();
}

void CEdAANetPanel::SetConnection( Bool flag )
{
	m_connected = flag;

	if ( flag )
	{
		OnConnectionAccepted();
	}
	else
	{
		OnConnectionLost();
	}
}

void CEdAANetPanel::RefreshStatus()
{
	if ( m_running )
	{
		if ( m_connected )
		{
			m_status->SetLabelText( _("Status: Connected") );
		}
		else
		{
			m_status->SetLabelText( _("Status: Listening...") );
		}
	}
	else
	{
		m_status->SetLabelText( _("Status: Disconnected") );
	}
}

void CEdAANetPanel::Log( const wxString& str )
{
	m_log->AppendText( str );
	m_log->AppendText( _("\n") );
}

aaServer& CEdAANetPanel::GetServer()
{
	return m_server;
}

void CEdAANetPanel::OnIdle( wxIdleEvent& event )
{
	if ( m_running )
	{
		if ( m_connected )
		{
			if ( m_server.HasPendingData() )
			{
				aaInt32 msgType = -1;
				const Bool state = m_server.Read( &msgType, sizeof( aaInt32 ) );
				if ( state )
				{
					ProcessPacket( msgType, m_server );
				}
				else
				{
					Log(_("Connection was lost") );
					Log(_("Server will be restarted") );

					DisconnectNetwork();
					ConnectNetwork();
				}
			}
		}
		else
		{
			if ( m_server.AsyncAccept() )
			{
				SetConnection( true );

				Log(_("Server is connected") );
				RefreshStatus();
			}
		}
	}
}

void CEdAANetPanel::OnNetConnected( wxCommandEvent& event )
{
	const Bool state = event.IsChecked();
	if ( state )
	{
		if ( !ConnectNetwork() )
		{
			wxToggleButton* b = XRCCTRL( *this, "netBtn", wxToggleButton );
			b->SetValue( false );
		}
	}
	else
	{
		DisconnectNetwork();
	}
}

void CEdAANetPanel::ProcessPacket( aaInt32 msgType, aaServer& server )
{
	
}

//#pragma optimize("",on)
