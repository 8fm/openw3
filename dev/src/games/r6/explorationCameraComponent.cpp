/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "explorationCameraComponent.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/curve.h"
#include "../../common/engine/utils.h"

IMPLEMENT_ENGINE_CLASS( CExplorationCameraComponent );











CExplorationCameraComponent::CExplorationCameraComponent()

	: m_distance					( 1.0f )
	, m_angleFromAim				( -150.0f )
	, m_cameraRadius				( 0.5f )
	, m_heightOffset				( 1.7f )

	, m_maxRoll						( 8.0f )
	, m_maxRollChangePerSecond		( 8.0f )
	, m_yawSpeedForMaxRoll			( 360.0f )
	, m_currRoll					( 0.0f )

	, m_position					( Vector::ZEROS )
	, m_rotation					( EulerAngles::ZEROS )

	, m_cameraBlobRadius			( 0.1f )

	, m_pitchToFovCurve				( NULL )
	, m_pitchToDistanceCurve		( NULL )

	, m_maxFOVChangePerSecond		( 90.0f )
	, m_maxDistanceChangePerSecond	( 1.0f )

{ 
	CreateEmptyCurves();
}













void CExplorationCameraComponent::OnActivate( const IScriptable* prevCameraObject, Bool resetCamera )
{
	RED_UNUSED( prevCameraObject );
	RED_UNUSED( resetCamera );
}












Bool CExplorationCameraComponent::Update( Float dt )
{
	Float prevYaw = m_rotation.Yaw;

	// get player, movement & parent
	CR6Player* player = Cast< CR6Player >( GGame->GetPlayerEntity() );

	if( !player )
	{
		return true;
	}

	Vector playerPos = player->GetWorldPosition();

	// get to target
	Vector toAimTarget = player->GetAimTarget() - playerPos;
	Float heightOffset = -toAimTarget.Z;
	toAimTarget.Normalize3();

	Vector toCamera = EulerAngles::YawToVector( toAimTarget.ToEulerAngles().Yaw + m_angleFromAim );
	Vector up = player->GetWorldUp();
	Float aimPitch = player->GetAimPitch();

	// camera position
	Vector cameraPos = up * ( m_heightOffset ) + 
					   toCamera * m_distance + 
					   ( toAimTarget * MCos( aimPitch ) + up * MSin( aimPitch ) ) * -m_cameraRadius;

	Vector worldCamPos = playerPos + cameraPos;

	// calculate direction
	Vector lookDir = ( player->GetAimTarget() - worldCamPos ).Normalized3();
	lookDir.Z = -lookDir.Z;

	// Calculate the rotation
	EulerAngles	rotation	= lookDir.ToEulerAngles();
	

	ResolveCollisions( worldCamPos, lookDir );

	Vector headPosition = playerPos + up * m_heightOffset;
	m_position = ResolveCollisions( headPosition, worldCamPos );


	{ // adjust roll
		Float targetRoll = 0.0f;
		if( dt > 0.0f )
		{
			Float yawSpeed = EulerAngles::AngleDistance( rotation.Yaw, prevYaw ) / dt;
			targetRoll = m_maxRoll * yawSpeed / m_yawSpeedForMaxRoll;		
		}

		AdjustValue( m_currRoll, targetRoll, m_maxRollChangePerSecond * dt );
		rotation.Roll += m_currRoll;
	}

	m_rotation = rotation;

	AdjustByPitch( dt );

	return true;
}














void CExplorationCameraComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	CreateEmptyCurves();
}














Bool CExplorationCameraComponent::GetData( Data& outData ) const
{
	/// @see CCameraComponent::GetData

	// some default values
	outData.m_farPlane	= Map( m_farPlane );
	outData.m_nearPlane = Map( m_nearPlane );
	outData.m_fov		= m_fov;

	outData.m_dofParams.Reset();

	outData.m_position = m_position;
	outData.m_rotation = m_rotation;

	return true;
}















void CExplorationCameraComponent::AdjustByPitch( Float timeDelta )
{
	Float targetFOV = GetTargetFovFromPitch( m_rotation.Pitch );
	Float targetDistance = GetTargetDistanceFromPitch( m_rotation.Pitch );

	AdjustValue( m_fov, targetFOV, m_maxFOVChangePerSecond * timeDelta );
	AdjustValue( m_distance, targetDistance, m_maxDistanceChangePerSecond * timeDelta );
}












void CExplorationCameraComponent::AdjustValue( Float& val, Float target, Float maxAbsChange )
{
	if( val < target )
	{
		val += maxAbsChange;
		if( val > target )
		{
			val = target;
		}
	}
	else
	{
		val -= maxAbsChange;
		if( val < target )
		{
			val = target;
		}
	}
}















Float CExplorationCameraComponent::GetTargetFovFromPitch( Float pitch ) const
{
	if( !m_pitchToFovCurve )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT( "Camera" ), TXT( "Pitch to fov curve for exploration camera doesn't exist." ) );
		return 60.0f;
	}

	pitch = EulerAngles::NormalizeAngle180( pitch );
	pitch = ( Clamp( -pitch, -90.0f, 90.0f ) + 90.0f ) / 90.0f;
	Float tmp = m_pitchToFovCurve->GetFloatValue( pitch );
	return tmp * 30.0f;
}














Float CExplorationCameraComponent::GetTargetDistanceFromPitch( Float pitch ) const
{
	if( !m_pitchToDistanceCurve )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT( "Camera" ), TXT( "Pitch to distance curve for exploration camera doesn't exist." ) );
		return 2.0f;
	}

	pitch = EulerAngles::NormalizeAngle180( pitch );
	pitch = ( Clamp( -pitch, -90.0f, 90.0f ) + 90.0f ) / 90.0f;
	return m_pitchToDistanceCurve->GetFloatValue( pitch );
}
















void CExplorationCameraComponent::CreateEmptyCurves()
{
	if( !m_pitchToFovCurve )
	{
		m_pitchToFovCurve = CreateObject< CCurve >( this );
		m_pitchToFovCurve->GetCurveData().AddPoint( 0.f, 2.f );
		m_pitchToFovCurve->GetCurveData().AddPoint( 2.f, 2.f );
	}

	if( !m_pitchToDistanceCurve )
	{
		m_pitchToDistanceCurve = CreateObject< CCurve >( this );
		m_pitchToDistanceCurve->GetCurveData().AddPoint( 0.f, 1.f );
		m_pitchToDistanceCurve->GetCurveData().AddPoint( 2.f, 1.f );
	}
}














/// This is a very basic collision reaction, just so that the camera doesn't enter walls.
/// It requires a lot of additional stuff - including collision prediction, smooth reaction, side offsets etc.
Vector CExplorationCameraComponent::ResolveCollisions( const Vector& playerPos, const Vector& targetPos )
{
	Vector toRet = targetPos;

	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	R6_ASSERT( physicsWorld );

	/// @todo MS: change this
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include =
		GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | 
		GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) )  | 
		GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );

	SPhysicsContactInfo sweepInfo;
	if( physicsWorld->SweepTestWithSingleResult( playerPos, targetPos, m_cameraBlobRadius, include, 0, sweepInfo ) == TRV_Hit )
	{
		sweepInfo.m_distance = Max( sweepInfo.m_distance, 0.2f );
		toRet = Lerp( sweepInfo.m_distance, playerPos, targetPos );
	}

	return toRet;
}
