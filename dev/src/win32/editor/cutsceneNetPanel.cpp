
#include "build.h"
#include "cutsceneNetPanel.h"

//#pragma optimize("",off)

BEGIN_EVENT_TABLE( CEdCutsceneNetPanel, CEdAANetPanel )
END_EVENT_TABLE()

CEdCutsceneNetPanel::CEdCutsceneNetPanel( wxWindow* parent, CEdCutsceneEditor* ed )
	: CEdAANetPanel( parent )
	, m_editor( ed )
{

}

void CEdCutsceneNetPanel::OnConnectionAccepted()
{
	GGame->DisablePausingOnApplicationIdle( CGame::DISABLE_AUTOPAUSE_Cutscene );
}

void CEdCutsceneNetPanel::OnConnectionLost()
{
	GGame->EnablePausingOnApplicationIdle( CGame::DISABLE_AUTOPAUSE_Cutscene );
}

void CEdCutsceneNetPanel::ProcessPacket( aaInt32 msgType, aaServer& server )
{
	switch ( msgType )
	{
	case 'cam_': 
		{
			ProcessPacket_Camera( server ); 
			break;
		}
	case 'main': 
		{
			ProcessPacket_Main( server ); 
			break;
		}
	case 'skel': 
		{
			ProcessPacket_Skel( server ); 
			break;
		}
	default:
		{
			ProcessPacket_UnknownMessage( server );
			//Log(_("Unknown message id received from client"));
		}
	}

}

void CEdCutsceneNetPanel::ProcessPacket_UnknownMessage( aaServer& server )
{
	aaInt32 size = 0;
	if ( !server.Read( &size, sizeof( aaInt32 ) ) )
	{
		Log(_("ProcessPacket_UnknownMessage Error - can not read buffer size"));
		return;
	}

	/*Uint8* data = new Uint8[ size ];

	if ( !server.Read( data, size ) )
	{
		Log(_("ProcessPacket_UnknownMessage Error - can not read buffer"));
		return;
	}
	*/

	Int32 status = 1;
	if ( !server.Send( &status, sizeof(Int32) ) )
	{
		Log(_("ProcessPacket_UnknownMessage Error - can not send response"));
		return;
	}

	//delete [] data;
}

void CEdCutsceneNetPanel::ProcessPacket_Camera( aaServer& server )
{
	aaInt32 size = 0;
	if ( !server.Read( &size, sizeof( aaInt32 ) ) )
	{
		Log(_("ProcessPacket_Camera Error - can not read buffer size"));
		return;
	}

	const Uint32 CAM_DATA_SIZE = 19;
	if ( size == sizeof( Float ) * CAM_DATA_SIZE )
	{
		Float cameraData[ CAM_DATA_SIZE ];
		if ( !server.Read( cameraData, sizeof( Float ) * CAM_DATA_SIZE ) )
		{
			Log(_("ProcessPacket_Camera Error - can not read buffer"));
			return;
		}

		Matrix mat( Matrix::IDENTITY );

		mat.V[0].A[ 0 ] = cameraData[ 3 ];
		mat.V[0].A[ 1 ] = cameraData[ 4 ];
		mat.V[0].A[ 2 ] = cameraData[ 5 ];
		mat.V[1].A[ 0 ] = cameraData[ 6 ];
		mat.V[1].A[ 1 ] = cameraData[ 7 ];
		mat.V[1].A[ 2 ] = cameraData[ 8 ];
		mat.V[2].A[ 0 ] = cameraData[ 9 ];
		mat.V[2].A[ 1 ] = cameraData[ 10 ];
		mat.V[2].A[ 2 ] = cameraData[ 11 ];

		const Vector camPos( cameraData[ 0 ], cameraData[ 1 ], cameraData[ 2 ] );
		mat.SetTranslation( camPos );

		const Float camFov = cameraData[ 12 ];

		m_editor->OnNetwork_SetCamera( mat, camFov );

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


void CEdCutsceneNetPanel::ProcessPacket_Main( aaServer& server )
{
	aaInt32 size = 0;
	if ( !server.Read( &size, sizeof( aaInt32 ) ) )
	{
		Log(_("ProcessPacket_Main Error - can not read buffer size"));
		return;
	}

	const Uint32 MAIN_DATA_SIZE = 1;
	if ( size == sizeof( Float ) * MAIN_DATA_SIZE )
	{
		Float mainData[ MAIN_DATA_SIZE ];
		if ( !server.Read( mainData, sizeof( Float ) * MAIN_DATA_SIZE ) )
		{
			Log(_("ProcessPacket_Main Error - can not read buffer"));
			return;
		}

		m_editor->OnNetwork_SetTime( mainData[0] );

		Int32 status = 1;
		if ( !server.Send( &status, sizeof(Int32) ) )
		{
			Log(_("ProcessPacket_Main Error - can not send response"));
			return;
		}
	}
	else
	{
		Log(_("ProcessPacket_Main Error - buffer size is wrong"));
	}
}

void CEdCutsceneNetPanel::ProcessPacket_Skel( aaServer& server )
{
	aaInt32 size = 0;
	if ( !server.Read( &size, sizeof( aaInt32 ) ) )
	{
		Log(_("ProcessPacket_Skel Error - can not read buffer size"));
		return;
	}
	if ( size == 0 )
	{ 
		return; 
	}

	char* mainData = new char[ size ];
	Red::System::MemoryZero( mainData, size );
	if ( !server.Read( mainData, sizeof( char ) * size ) )
	{
		Log(_("ProcessPacket_Skel Error - can not read buffer"));
		delete [] mainData;
		return;
	}

	// magic
	Int32* sizes = (Int32*)mainData;
	Int32 nameSize = sizes[0];
	Int32 bonesSize = sizes[1];
	Int32 floatsSize = sizes[2];

	char* namePtr = mainData + (sizeof(Int32)*3); // no NULL
	Float* bonesData = (Float*)( mainData + sizeof(Int32)*3 + nameSize );
	Float* floatsData = bonesData + bonesSize*7;

	char* namebuf = new char[nameSize+1];
	memcpy( namebuf, namePtr, sizeof(char)*nameSize );
	namebuf[nameSize] = 0;


	SAnimationMappedPose pose;
	pose.m_mode = AMPM_Override;
	Int32 b;
	for( b = 0; b<bonesSize; ++b )
	{
		EngineQsTransform m;
		Float* arr = bonesData+(b*7);

		m.SetPosition( Vector( arr[0]*0.01f, arr[1]*0.01f, arr[2]*0.01f, 1.0f ) );
		m.SetRotation( Vector( -arr[3], -arr[4], -arr[5], arr[6] ) );
		m.SetScale   ( Vector( 1.0f, 1.0f, 1.0f, 1.0f) );

		pose.m_bones.PushBack( m );
		pose.m_bonesMapping.PushBack( b );
	}

	Int32 t;
	for( t=0;t<floatsSize;t++)
	{
		pose.m_tracks.PushBack( floatsData[t]*0.01f );
		pose.m_tracksMapping.PushBack( t );
	}
	
	m_editor->OnNetwork_SetPose( namebuf, nameSize+1, pose );

	delete [] namebuf;
	delete [] mainData;

	Int32 status = 1;
	if ( !server.Send( &status, sizeof(Int32) ) )
	{
		Log(_("ProcessPacket_Skel Error - can not send response"));
		return;
	}

}

//#pragma optimize("",on)
