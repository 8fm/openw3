/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "slotCameraComponent.h"

IMPLEMENT_ENGINE_CLASS( CSlotCameraComponent );

CSlotCameraComponent::CSlotCameraComponent() 
	: m_cameraSlot( CName::NONE ) ,
	  m_lookAtSlot( CName::NONE )
{   
}

void CSlotCameraComponent::LogicUpdate( Float _Dt )
{
	TBaseClass::LogicUpdate( _Dt );

	// get player
	CR6Player* player = Cast< CR6Player >( GGame->GetPlayerEntity() );

	if( !player )
		return;

	// get a back bone to look at
	Vector lookAtPos = player->GetWorldPosition();
	Vector cameraPos = lookAtPos - player->GetWorldForward() + player->GetWorldUp();
	Vector slotPosition, slotForward;

	// slot specified for camera?
	if( GetSlotData( player, m_cameraSlot, slotPosition, slotForward ) )
	{
		// set camera pos & look at
		cameraPos = slotPosition;
		lookAtPos = slotPosition + slotForward;
	}

	// slot specified for look at target?
	if( GetSlotData( player, m_lookAtSlot, slotPosition, slotForward ) )
	{
		lookAtPos = slotPosition;
	}

	// calculate direction
	Vector lookDir = ( lookAtPos - cameraPos ).Normalized3();
	lookDir.Z = -lookDir.Z;


	RED_ASSERT( false, TXT( "Not yet implemented (after R6CameraManager removal)" ) );

	// set parent position
	//parent->SetPosition( cameraPos );
	//parent->SetRotation( lookDir.ToEulerAngles() );
}

Bool CSlotCameraComponent::GetSlotData( CEntity* _entity, CName _slotName, Vector& _slotPosition, Vector& _slotForward )
{
	if( _slotName.Empty() )
		return false;

	// find slot?
	const EntitySlot* slot = _entity->GetEntityTemplate()->FindSlotByName( _slotName, true );
	if( !slot )
		return false;

	// get slot matrix
	Matrix matrix( Matrix::IDENTITY );
	slot->CalcMatrix( _entity, matrix, NULL );

	// get slot position & forward
	_slotPosition = matrix.GetTranslation();
	matrix.ToEulerAnglesFull().ToAngleVectors( NULL, &_slotForward, NULL );

	return true;
}