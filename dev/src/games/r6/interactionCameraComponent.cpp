/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "interactionCameraComponent.h"
#include "r6InteractionComponent.h"

IMPLEMENT_ENGINE_CLASS( CInteractionCameraComponent );

CInteractionCameraComponent::CInteractionCameraComponent()
	: m_distanceFromPlayerF( 1.2f ) ,
	m_focusBone( TXT("torso2") ) ,
	m_focusOnInteractionItem( false )
{   
}

void CInteractionCameraComponent::LogicUpdate( Float _Dt )
{
	TBaseClass::LogicUpdate( _Dt );

	

	// get player
	CR6Player* player = Cast< CR6Player >( GGame->GetPlayerEntity() );

	if( !player )
		return;

	// figure out what to focus on
	Vector lookAtPos = player->GetWorldPosition();
	CR6InteractionComponent* interactionTarget = player->GetInteractionTarget();

	if( m_focusOnInteractionItem && interactionTarget )
	{
		 lookAtPos = interactionTarget->GetWorldPosition();
	}
	else
	{
		// get a back bone to look at
		const ISkeletonDataProvider* skeleton = player->GetRootAnimatedComponent()->QuerySkeletonDataProvider();
		if ( skeleton )
		{
			// Find bone
			Int32 boneIndex = skeleton->FindBoneByName( m_focusBone );
			if ( boneIndex >= 0 )
			{
				// Query bone matrix and calculate final matrix
				lookAtPos = skeleton->GetBoneMatrixWorldSpace( boneIndex ).GetTranslation();
			}
		}
	}

	// get to target
	Vector toAimTarget = ( player->GetAimTarget() - lookAtPos ).Normalized3();

	// camera position
	Vector cameraPos = lookAtPos - toAimTarget * m_distanceFromPlayerF;

	// calculate direction
	Vector lookDir = toAimTarget;
	lookDir.Z = -lookDir.Z;

	// set parent position

	RED_ASSERT( false, TXT( "Not yet implemented (after R6CameraManager removal)" ) );

	//parent->SetPosition( cameraPos );
	//parent->SetRotation( lookDir.ToEulerAngles() );
}