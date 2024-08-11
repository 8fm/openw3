/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "avCameraTest.h"
#include "..\..\common\game\pilotComponent.h"
#include "avComponentTest.h"



IMPLEMENT_ENGINE_CLASS( CAVCameraTest );











CAVCameraTest::CAVCameraTest()
	: m_position						( Vector::ZEROS )
	, m_rotation						( EulerAngles::ZEROS )

	, m_standardOffset					( Vector::ZEROS )

	, m_maxPitchChangePerSecond			( 90.0f )
	, m_maxYawChangePerSecond			( 90.0f )

	, m_maxAVOrientationChangePerSecond ( 90.0f )
	, m_prevAVOrientation				( EulerAngles::ZEROS )

	, m_currReverseAngle				( 0.0f )
	, m_reverseAngleAdjustCoef			( 0.0f )

	, m_currLookAtDistance				( 0.0f )
	, m_minSpeedLookAtDistance			( 0.0f )
	, m_maxSpeedLookAtDistance			( 0.0f )
	, m_lookAtDistanceAdjustCoef		( 0.0f )

	, m_currPitchSpeedOffset			( 0.0f )
	, m_currYawSpeedOffset				( 0.0f )
	, m_minPitchSpeedOffset				( 0.0f )
	, m_minYawSpeedOffset				( 0.0f )
	, m_maxPitchSpeedOffset				( 0.0f )
	, m_maxYawSpeedOffset				( 0.0f )
	, m_pitchOffsetAdjustCoef			( 0.0f )
	, m_yawOffsetAdjustCoef				( 0.0f )


	, m_currRoll						( 0.0f )
	, m_rollAdjustCoef					( 0.0f )
	, m_rollToAVRoll					( 0.0f )

	, m_currHorizontalHoveringAngle		( 0.0f )
	, m_currVerticalHoveringAngle		( 0.0f )
	, m_minHorizontalHoveringAngle		( 0.0f )
	, m_maxHorizontalHoveringAngle		( 0.0f )
	, m_minVerticalHoveringAngle		( 0.0f )
	, m_maxVerticalHoveringAngle		( 0.0f )
	, m_hHoveringAngleAdjustCoef		( 0.0f )
	, m_vHoveringAngleAdjustCoef		( 0.0f )

	, m_minSpeedFOV						( 60.0f )
	, m_maxSpeedFOV						( 60.0f )
{
}













void CAVCameraTest::OnActivate( const IScriptable* prevCameraObject, Bool resetCamera )
{
	RED_UNUSED( prevCameraObject );
	RED_UNUSED( resetCamera );
}














Bool CAVCameraTest::Update( Float dt )
{
	CEntity*			avEntity	= NULL;
	CAVComponentTest*	avComponent = NULL;

	if( !GetAV( avEntity, avComponent ) )
	{
		return true;	// no av to be tracked
	}

	R6_ASSERT( avEntity && avComponent );


	Vector avPos = avEntity->GetWorldPosition();
	EulerAngles avRot = avEntity->GetWorldRotation();

	{ // smooth av orientation for collisions
		Float maxDiff = m_maxAVOrientationChangePerSecond * dt;
		Float yawDiff	= EulerAngles::AngleDistance( m_prevAVOrientation.Yaw, avRot.Yaw );
		Float pitchDiff = EulerAngles::AngleDistance( m_prevAVOrientation.Pitch, avRot.Pitch );
		yawDiff		= EulerAngles::NormalizeAngle180( yawDiff );
		pitchDiff	= EulerAngles::NormalizeAngle180( pitchDiff );

		yawDiff		= Clamp( yawDiff, -maxDiff, maxDiff );
		pitchDiff	= Clamp( pitchDiff, -maxDiff, maxDiff );

		avRot.Yaw	= EulerAngles::NormalizeAngle180( m_prevAVOrientation.Yaw + yawDiff );
		avRot.Pitch = EulerAngles::NormalizeAngle180( m_prevAVOrientation.Pitch + pitchDiff );

		m_prevAVOrientation = avRot;
	}

	Float speedCoef = avComponent->GetVehicleSpeedToMaxSpeedCoefAbs();


	{ // reverse camera
		Float targetReverseAngle = avComponent->IsMovingBackward() ? 180.0f : 0.0f;
		m_currReverseAngle = Lerp( m_reverseAngleAdjustCoef * dt, m_currReverseAngle, targetReverseAngle );
		avRot.Yaw += m_currReverseAngle;
	}


	{ // update hovering angles
		Float targetHHA = Lerp( speedCoef, m_minHorizontalHoveringAngle, m_maxHorizontalHoveringAngle );
		Float targetVHA = Lerp( speedCoef, m_minVerticalHoveringAngle, m_maxVerticalHoveringAngle );

		targetHHA *= avComponent->GetHorizontalHoveringCoef();
		targetVHA *= avComponent->GetVerticalHoveringCoef();

		targetHHA *= GetCurrentInverse();

		m_currHorizontalHoveringAngle = Lerp( m_hHoveringAngleAdjustCoef * dt, m_currHorizontalHoveringAngle, targetHHA );
		m_currVerticalHoveringAngle	  = Lerp( m_vHoveringAngleAdjustCoef * dt, m_currVerticalHoveringAngle, targetVHA );

		avRot.Pitch = EulerAngles::NormalizeAngle180( avRot.Pitch + m_currVerticalHoveringAngle   );
		avRot.Yaw	= EulerAngles::NormalizeAngle180( avRot.Yaw	 - m_currHorizontalHoveringAngle );
	}


	{ // update back offsets
		Vector baseOffset = avRot.TransformVector( m_standardOffset );

		Float targetPitchOffset	= Lerp( speedCoef, m_minPitchSpeedOffset, m_maxPitchSpeedOffset );
		Float targetYawOffset	= Lerp( speedCoef, m_minYawSpeedOffset,   m_maxYawSpeedOffset );
		targetPitchOffset	*= avComponent->GetVehiclePitchSpeedToMaxSpeedCoef();
		targetYawOffset		*= avComponent->GetVehicleYawSpeedToMaxSpeedCoef();

		m_currPitchSpeedOffset	= Lerp( m_pitchOffsetAdjustCoef * dt, m_currPitchSpeedOffset, targetPitchOffset );
		m_currYawSpeedOffset	= Lerp( m_yawOffsetAdjustCoef   * dt, m_currYawSpeedOffset,   targetYawOffset );


		Vector turningOffset = avEntity->GetWorldUp() * m_currPitchSpeedOffset - avEntity->GetWorldRight() * m_currYawSpeedOffset;

		m_position = avPos + baseOffset + turningOffset;
	}


	{ // update look at point
		Float targetLookAtDistance = Lerp( speedCoef, m_minSpeedLookAtDistance, m_maxSpeedLookAtDistance );
		m_currLookAtDistance = Lerp( m_lookAtDistanceAdjustCoef * dt, m_currLookAtDistance, targetLookAtDistance );

		Vector forwardDir = avRot.TransformVector( Vector3( 0.0f, 1.0f, 0.0f ) );

		Vector lookAtPoint = avPos + forwardDir * m_currLookAtDistance;

		Vector lookDir = lookAtPoint - m_position;
		lookDir.Z = -lookDir.Z;
		EulerAngles targetRotation = lookDir.ToEulerAngles();

		targetRotation.Pitch = EulerAngles::NormalizeAngle180( targetRotation.Pitch );

		if( Abs( targetRotation.Pitch ) > 80.0f )
		{
			targetRotation.Pitch = 80.0f * Sgn( m_rotation.Pitch );
		}

		Float thisFrameMaxPitchChange = m_maxPitchChangePerSecond * dt;
		Float thisFrameMaxYawChange = m_maxYawChangePerSecond * dt;

		Float pitchDist = EulerAngles::AngleDistance( m_rotation.Pitch, targetRotation.Pitch );
		Float yawDist = EulerAngles::AngleDistance( m_rotation.Yaw, targetRotation.Yaw );

		if( Abs( pitchDist ) > thisFrameMaxPitchChange )
		{
			pitchDist = Sgn( pitchDist ) * thisFrameMaxPitchChange;
		}

		if( Abs( yawDist ) > thisFrameMaxYawChange )
		{
			yawDist = Sgn( yawDist ) * thisFrameMaxYawChange;
		}


		m_rotation.Pitch = EulerAngles::NormalizeAngle180( m_rotation.Pitch + pitchDist );
		m_rotation.Yaw = EulerAngles::NormalizeAngle180( m_rotation.Yaw + yawDist );
	}


	{ // updaate roll
		Float targetRoll = avRot.Roll * m_rollToAVRoll;
		m_currRoll = Lerp( m_rollAdjustCoef * dt, m_currRoll, targetRoll );
		m_rotation.Roll = m_currRoll;
	}



	Float fovMod = avComponent->GetVehicleSpeedToMaxSpeedCoefAbs();
	Float targetFOV = Lerp( fovMod, m_minSpeedFOV, m_maxSpeedFOV );
	m_fov = Lerp( dt * 2.0f, m_fov, targetFOV );

	return true;
}














Bool CAVCameraTest::GetData( Data& outData ) const 
{
	/// @see CCameraComponent::GetData
	outData.m_farPlane	= Map( m_farPlane );
	outData.m_nearPlane = Map( m_nearPlane );
	outData.m_fov		= m_fov;

	outData.m_dofParams.Reset();

	outData.m_position = m_position;
	outData.m_rotation = m_rotation;

	return true;
}













Bool CAVCameraTest::GetAV( CEntity*& avEntity , CAVComponentTest*& avComponent )
{
	R6_ASSERT( avEntity == NULL && avComponent == NULL );
	R6_ASSERT( GGame );

	CR6Player* player = Cast< CR6Player >( GGame->GetPlayerEntity() );

	if( !player )
	{
		return false;
	}

	avEntity = Cast< CEntity >( player->GetAttachedAVNode() );
	if( !avEntity )
	{
		return false; 
	}

	auto tmp = ComponentIterator< CAVComponentTest >( avEntity );
	if( !tmp || !( *tmp ) )
	{
		return false;
	}

	avComponent = *tmp;
	return true;
}














//RED_DEFINE_STATIC_NAME( CameraTrace )
//void CAVCameraTest::ResolveCollisions( Vector& newPos, const Vector& camDir )
//{
//	const Float TRACE_LENGTH = 1.5f;
//
//	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
//	if ( !physicsWorld )
//		return;
//
//	SPhysicsContactInfo contactInfo;
//	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | 
//		GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) )  | 
//		GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
//
//	Vector playerPos = GGame->GetPlayerEntity()->GetWorldPosition();
//	Vector start = playerPos + newPos + camDir * TRACE_LENGTH;
//	Vector end = playerPos + newPos;
//	Cast< CActor >( GGame->GetPlayerEntity() )->GetVisualDebug()->AddLine( CNAME( CameraTrace ), start, end, true, Color( 255, 0, 0 ), 1.0f );
//
//	if ( physicsWorld->RayCastWithSingleResult( start, end, include, 0, contactInfo ) )
//	{
//		// correct position
//		newPos = contactInfo.m_position + camDir * 0.05f - playerPos;
//	}
//}















