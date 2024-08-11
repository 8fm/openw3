
#include "build.h"
#include "dialogNetPanel.h"
#include "dialogEditor.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

BEGIN_EVENT_TABLE( CEdDialogNetPanel, CEdAANetPanel )
	EVT_BUTTON( XRCID( "netButton" ), CEdDialogNetPanel::OnSendData )
END_EVENT_TABLE()

CEdDialogNetPanel::CEdDialogNetPanel( wxWindow* parent, CEdSceneEditor* ed )
	: CEdAANetPanel( parent )
	, m_mediator( ed )
{

}

void CEdDialogNetPanel::ProcessPacket( aaInt32 msgType, aaServer& server )
{
	switch ( msgType )
	{
	case 'cam_': 
		{
			ProcessPacket_Camera( server ); 
			break;
		}
	case 123:
		{
			ProcessPacket_Test( server );
			break;
		}
	default:
		{
			Log(_("Unknown message id received from client"));
		}
	}

}

void CEdDialogNetPanel::ProcessPacket_Camera( aaServer& server )
{
	aaInt32 size = 0;
	if ( !server.Read( &size, sizeof( aaInt32 ) ) )
	{
		Log(_("ProcessPacket_Camera Error - can not read buffer size"));
		return;
	}

	const Uint32 CAM_DATA_SIZE = 12;
	if ( size == sizeof( Float ) * CAM_DATA_SIZE )
	{
		Float cameraData[ CAM_DATA_SIZE ];
		if ( !server.Read( cameraData, sizeof( Float ) * CAM_DATA_SIZE ) )
		{
			Log(_("ProcessPacket_Camera Error - can not read buffer"));
			return;
		}

		EulerAngles camRot;
		camRot.Pitch = cameraData[ 3 ];
		camRot.Roll = cameraData[ 4 ];
		camRot.Yaw = cameraData[ 5 ];

		Vector camPos( cameraData[ 0 ], cameraData[ 1 ], cameraData[ 2 ] );
		Float camFov = cameraData[ 6 ];

		m_mediator->OnNetwork_SetCamera( camPos, camRot, camFov );

		Int32 status = 1;
		if ( !server.Send( &status, sizeof(Int32) ) )
		{
			Log(_("ProcessPacket_Camera Error - can not send response"));
			return;
		}
	}
	else
	{
		Log(_("ProcessPacket_Camera Error - buffer size is wrong"));
	}
}

void CEdDialogNetPanel::ProcessPacket_Test( aaServer& server )
{
	aaInt32 size = 0;
	if ( !server.Read( &size, sizeof( aaInt32 ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read buffer size"));
		return;
	}

	Float var = 0.f;
	if ( !server.Read( &var, sizeof( Float ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read"));
		return;
	}
	if ( !server.Read( &var, sizeof( Float ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read"));
		return;
	}
	if ( !server.Read( &var, sizeof( Float ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read"));
		return;
	}
	if ( !server.Read( &var, sizeof( Float ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read"));
		return;
	}
	if ( !server.Read( &var, sizeof( Float ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read"));
		return;
	}
	if ( !server.Read( &var, sizeof( Float ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read"));
		return;
	}
	if ( !server.Read( &var, sizeof( Float ) ) )
	{
		Log(_("ProcessPacket_Test Error - can not read"));
		return;
	}

	wxString str = wxString::Format( wxT("ProcessPacket_Test - %d"), size );
	Log( str );

	Int32 status = 1;
	if ( !server.Send( &status, sizeof(Int32) ) )
	{
		Log(_("ProcessPacket_Test Error - can not send response"));
		return;
	}
}

void CEdDialogNetPanel::OnSendData( wxCommandEvent& event )
{
	wxTextCtrl* textX = XRCCTRL( *this, "textX", wxTextCtrl );
	wxTextCtrl* textY = XRCCTRL( *this, "textY", wxTextCtrl );
	wxTextCtrl* textZ = XRCCTRL( *this, "textZ", wxTextCtrl );

	Float x = 0.f;
	Float y = 0.f;
	Float z = 0.f;

	String _textX = textX->GetValue();
	String _textY = textY->GetValue();
	String _textZ = textZ->GetValue();

	FromString( _textX, x );
	FromString( _textY, y );
	FromString( _textZ, z );

	aaServer& server = GetServer();

	server.SetAsyncMode( false );

	Int32 msgType = 101;
	if ( !server.Send( &msgType, sizeof(Int32) ) )
	{
		Log(_("OnSendData Error - 1"));
		return;
	}

	if ( !server.Send( &x, sizeof(Float) ) )
	{
		Log(_("OnSendData Error - 2"));
		return;
	}
	if ( !server.Send( &y, sizeof(Float) ) )
	{
		Log(_("OnSendData Error - 3"));
		return;
	}
	if ( !server.Send( &z, sizeof(Float) ) )
	{
		Log(_("OnSendData Error - 4"));
		return;
	}

	Int32 status = 0;
	if ( !server.Read( &status, sizeof( Int32 ) ) )
	{
		Log(_("OnSendData Error - 5"));
		return;
	}

	server.SetAsyncMode( true );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
