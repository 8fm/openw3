/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r6AdvancedVehicleComponent.h"

IMPLEMENT_ENGINE_CLASS( CR6AdvancedVehicleComponent );


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6AdvancedVehicleComponent::OnPilotMounted( CPilotComponent* pilot )
{
	TBaseClass::OnPilotMounted( pilot );

	
	//if( m_isPlayerControlled )
	//{
	//	// Camera
	//	CR6CameraManager*	camManager	= Cast< CR6Game >( GCommonGame )->GetCameraManager();
	//	if( pilot )
	//	{
	//		camManager->SwitchToCamera( m_cameraTag, m_cameraBlendTime );
	//	}
	//	else
	//	{
	//		camManager->SwitchToCamera( CNAME( ExplorationCamera ), m_cameraBlendTime );
	//	}
	//}
	
}
