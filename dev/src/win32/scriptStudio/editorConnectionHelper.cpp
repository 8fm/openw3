/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "editorConnectionHelper.h"

#include "../../common/redNetwork/address.h"
#include "app.h"

wxDEFINE_EVENT( ssEVT_CONNECTION_EVENT, CConnectionEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CConnectionEvent, wxEvent );

CConnectionEvent::CConnectionEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CConnectionEvent::CConnectionEvent( EConnectionStatus status )
:	wxEvent( wxID_ANY, ssEVT_CONNECTION_EVENT )
,	m_status( status )
{

}

CConnectionEvent::~CConnectionEvent()
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS( CEditorConnectionHelper, wxEvtHandler );

const Red::System::Int32 CEditorConnectionHelper::CONNECTION_POLL_INTERVAL = 2000;
const Red::System::Int32 CEditorConnectionHelper::DISCONNECT_POLL_INTERVAL = 250;

CEditorConnectionHelper::CEditorConnectionHelper()
:	m_targetAddress( "127.0.0.1",  wxTheSSApp->GetNetworkPort() )
,	m_connectionStatus( ConnStatus_Disconnected )
{
	m_updateTimer.Bind( wxEVT_TIMER, &CEditorConnectionHelper::OnUpdate, this );
}

CEditorConnectionHelper::~CEditorConnectionHelper()
{
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		network->UnregisterListener( this );
	}
}

void CEditorConnectionHelper::SetTargetAddress( const wxChar* ip )
{
	m_targetAddress = Red::Network::Address( ip,  wxTheSSApp->GetNetworkPort() );
}

void CEditorConnectionHelper::SetTargetPort( Red::System::Uint16 port )
{
	wxTheSSApp->SetNetworkPort( port );
	m_targetAddress.SetPort( port );
}

void CEditorConnectionHelper::ToggleConnectionToEditor()
{
	switch( m_connectionStatus.GetValue() )
	{
		case ConnStatus_Disconnected:
		{
			m_connectionStatus.SetValue( ConnStatus_Connect );

			if( !m_updateTimer.IsRunning() )
			{
				// We shouldn't have too quick an update interval because we don't want to spam the network with connection requests
				m_updateTimer.Start( CONNECTION_POLL_INTERVAL );
			}

			SendConnectionEvent();
		}
		break;

		case ConnStatus_Connecting:
		case ConnStatus_JustConnected:
		case ConnStatus_Connected:
			Red::Network::Manager::GetInstance()->DisconnectFrom( m_targetAddress );
			
			// Switch to a quick update to make disconnecting more responsive
			m_updateTimer.Start( DISCONNECT_POLL_INTERVAL );
			break;

		case ConnStatus_JustDropped:
		case ConnStatus_JustDisconnected:
		case ConnStatus_Connect:
		{
			m_connectionStatus.SetValue( ConnStatus_Disconnected );

			if( m_updateTimer.IsRunning() )
			{
				m_updateTimer.Stop();
			}

			SendConnectionEvent();
		}
		break;
	}
}

void CEditorConnectionHelper::OnConnectionSucceeded( const Red::Network::Address& peer )
{
	m_connectionStatus.SetValue( ConnStatus_JustConnected );
	SendConnectionEvent();
}

void CEditorConnectionHelper::OnConnectionAvailable( const Red::Network::Address& peer )
{
	m_connectionStatus.SetValue( ConnStatus_JustConnected );
	SendConnectionEvent();
}

void CEditorConnectionHelper::OnConnectionDropped( const Red::Network::Address& peer )
{
	if( m_connectionStatus.GetValue() == ConnStatus_Connecting )
	{
		// Failed connection attempt
		m_connectionStatus.SetValue( ConnStatus_Connect );
	}
	else
	{
		// Connection dropped
		m_connectionStatus.SetValue( ConnStatus_JustDropped );
	}

	SendConnectionEvent();
}

void CEditorConnectionHelper::OnConnectionClosed( const Red::Network::Address& peer )
{
	m_connectionStatus.SetValue( ConnStatus_JustDisconnected );
	SendConnectionEvent();
}

void CEditorConnectionHelper::OnUpdate( wxTimerEvent& event )
{
	switch( m_connectionStatus.GetValue() )
	{
	case ConnStatus_JustDisconnected:
		{
			// Disconnect
			ToggleConnectionToEditor();
		}
		break;

	case ConnStatus_JustDropped:
		{
			m_connectionStatus.SetValue( ConnStatus_Connect );
			SendConnectionEvent();
		}
		break;

	case ConnStatus_Connect:
		{
			Red::Network::Manager* network = Red::Network::Manager::GetInstance();

			if( network && network->IsInitialized() )
			{
				m_connectionStatus.SetValue( ConnStatus_Connecting );
				SendConnectionEvent();

				network->ConnectTo( m_targetAddress, RED_NET_CHANNEL_SCRIPT, this );
			}
		}
		break;

	case ConnStatus_JustConnected:
		{
			m_connectionStatus.SetValue( ConnStatus_Connected );
			SendConnectionEvent();
		}
		break;
	}
}

void CEditorConnectionHelper::SendConnectionEvent()
{
	CConnectionEvent* event = new CConnectionEvent( m_connectionStatus.GetValue() );

	QueueEvent( event );
}

