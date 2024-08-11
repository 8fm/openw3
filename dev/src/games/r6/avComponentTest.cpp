/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "avComponentTest.h"
#include "..\..\common\core\algorithms.h"
#include "..\..\common\core\mathUtils.h"
#include "..\..\common\engine\renderFrame.h"




IMPLEMENT_ENGINE_CLASS( SAVIdleData );
IMPLEMENT_ENGINE_CLASS( SAVSpeedData );
IMPLEMENT_ENGINE_CLASS( SAVTurningData );
IMPLEMENT_ENGINE_CLASS( SAVHoveringData );
IMPLEMENT_ENGINE_CLASS( SAVSafetyBrakesData );
IMPLEMENT_ENGINE_CLASS( CAVComponentTest );








/// @todo MS: maybe this is allready done, I didn't want to bother Tomek Wojcik and couldn't find it, so its a place holder
static Vector3 ReflectWithStrength( const Vector3& V, const Vector3& N, Float strength = 1.0f )
{
	Vector3 NN = N.Normalized();
	return V - NN * ( NN.Dot( V ) * ( 1.0f + strength ) );
}









// ----------------------- idle default values -----------------------
SAVIdleData::SAVIdleData()
	: m_previewInEditor					( false )
	, m_time							( 0.0f )
	, m_center							( Vector::ZEROS )
	, m_waveFrequency					( 1.0f )
	, m_waveAmplitude					( 0.01f )
	, m_currStrength					( 0.0f )
	, m_maxStrengthAfter				( 2.0f )
{
}







// ----------------------- accelerating and breaking default values -----------------------
SAVSpeedData::SAVSpeedData()
	: m_driving							( false )
	, m_accForward						( 20.0f )
	, m_dccForward						( 10.0f )
	, m_accReverse						( 20.0f )
	, m_dccReverse						( 10.0f )
	, m_maxSpeedForward					( 40.0f )
	, m_maxSpeedReverse					( 20.0f )
	, m_currSpeed						( 0.0f )
{
}







// ----------------------- turning default values -----------------------
SAVTurningData::SAVTurningData()
	: m_turning							( false )
	, m_yawAcc							( 80.0f )
	, m_pitchAcc						( 80.0f )
	, m_maxYawSpeed						( 50.0f )
	, m_maxPitchSpeed					( 50.0f )
	, m_currYawSpeed					( 0.0f )
	, m_currPitchSpeed					( 0.0f )

	, m_currRoll						( 0.0f )
	, m_maxRoll							( 50.0f )

	, m_pitchSnappingWhileStanding		( 2.0f )
	, m_pitchSnappingWhileFullSpeed		( 0.0f )
{
}







// ----------------------- hovering default values -----------------------
SAVHoveringData::SAVHoveringData()
	: m_hovering						( false )
	, m_minAccVertical					( 5.0f )
	, m_maxAccVertical					( 12.0f )
	, m_minTopSpeedVertical				( 10.0f )
	, m_maxTopSpeedVertical				( 24.0f )
	, m_currSpeedVertical				( 0.0f )
	, m_maxPitch						( 0.0f )
	, m_minAccHorizontal				( 2.5f )
	, m_maxAccHorizontal				( 7.5f )
	, m_minTopSpeedHorizontal			( 5.0f )
	, m_maxTopSpeedHorizontal			( 15.0f )
	, m_currSpeedHorizontal				( 0.0f )
	, m_currRoll						( 0.0f )
	, m_maxRoll							( 15.0f )
	, m_minDropDistance					( 20.0f )
	, m_maxDropDistance					( 100.0f )
	, m_dropMultiplier					( 4.5f )
	, m_minSideDropDistance				( -5.0f )
	, m_maxSideDropDistance				( 50.0f )
	, m_sideDropMultiplier				( 4.0f )
{
}








// ----------------------- safety brakes default values -----------------------
SAVSafetyBrakesData::SAVSafetyBrakesData()
	: m_minDistanceToGround				( 1.0f )
	, m_startBrakingDistance			( 45.0f )
	, m_currBrakingForce				( 0.0f )
	, m_maxBrakingForce					( 30.0f )
	, m_currBrakingACC					( 0.0f )
	, m_maxBrakingACC					( 60.0f )
	, m_currBrakingSpeed				( 0.0f )
	, m_maxBrakingSpeed					( 100.0f )
{
}













CAVComponentTest::CAVComponentTest()


// ----------------------- general default values -----------------------
	: m_mass							( 1.0f )
	, m_velocity						( Vector::ZEROS )
	, m_maxAltitude						( 180.0f )
	, m_altitudeCorrectionCoef			( 0.1f )




// ----------------------- input default values -----------------------
	, m_accelerateInput					( 0.0f )
	, m_steeringAxisInputX				( 0.0f )
	, m_steeringAxisInputY				( 0.0f )
	, m_hoveringAxisInputX				( 0.0f )
	, m_hoveringAxisInputY				( 0.0f )




// ----------------------- debug default values -----------------------
	, m_DEV_STEERING_RIGHT_AXIS			( false )
	, m_DEV_STEERING_INVERSE			( false )
	, m_DEV_HOVERING_RIGHT_AXIS			( false )
	, m_DEV_HOVERING_INVERSE			( false )
	, m_debugCurrACC					( 0.0f )
	, m_debugCurrDropCoef				( 0.0f )
	, m_debugCurrSideACCMult			( 0.0f )
	, m_debugCurrSideSpeedMult			( 0.0f )

{
}










void CAVComponentTest::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
}









void CAVComponentTest::OnTickIdle( Float dt )
{
	if( !GetEntity()->IsInGame() && !m_idleData.m_previewInEditor )
	{
		return;
	}

	m_speedData.m_driving	= false;
	m_turningData.m_turning	= false;
	m_hoveringData.m_hovering	= false;

	UpdateIdle( dt );
}










void CAVComponentTest::UpdatePlayerInput( Float timeDelta )
{
	auto IM = GCommonGame->GetInputManager();
	R6_ASSERT( IM );
	Float acc = IM->GetActionValue( CNAME( GI_AV_ACC ) );
	m_speedData.m_currSpeed += acc * timeDelta;

	{ // acceleration / deceleration input
		Float posAcc = IM->GetActionValue( CNAME( GI_AV_ACC ) );
		Float negAcc = IM->GetActionValue( CNAME( GI_AV_DCC ) );
		m_accelerateInput = posAcc - negAcc;


		/// @todo MS: acceleration input deadzone
		if( Abs( m_accelerateInput ) < 0.01f )
		{
			m_accelerateInput = 0.0f;
		}
	}

	{ // steering input
		if( m_DEV_STEERING_RIGHT_AXIS )
		{
			m_steeringAxisInputX = -IM->GetActionValue( CNAME( GI_AV_AxisR_X ) );
			m_steeringAxisInputY =  IM->GetActionValue( CNAME( GI_AV_AxisR_Y ) );
		}
		else
		{
			m_steeringAxisInputX = -IM->GetActionValue( CNAME( GI_AV_AxisL_X ) );
			m_steeringAxisInputY =  IM->GetActionValue( CNAME( GI_AV_AxisL_Y ) );
		}

		if( m_DEV_STEERING_INVERSE )
		{
			m_steeringAxisInputY = -m_steeringAxisInputY;
		}
	}


	{ // Hovering input
		if( m_DEV_HOVERING_RIGHT_AXIS )
		{
			m_hoveringAxisInputX = IM->GetActionValue( CNAME( GI_AV_AxisR_X ) );
			m_hoveringAxisInputY = IM->GetActionValue( CNAME( GI_AV_AxisR_Y ) );
		}
		else
		{
			m_hoveringAxisInputX = IM->GetActionValue( CNAME( GI_AV_AxisL_X ) );
			m_hoveringAxisInputY = IM->GetActionValue( CNAME( GI_AV_AxisL_Y ) );
		}

		if( m_DEV_HOVERING_INVERSE )
		{
			m_hoveringAxisInputY = -m_hoveringAxisInputY;
		}
	}
}











void CAVComponentTest::UpdateLogic( Float dt )
{
	UpdateLinearSpeed( dt );

	m_turningData.m_turning = false;
	UpdateTurningPitch(dt);
	UpdateTurningYaw(dt);

	UpdateHovering( dt );



	{ // safety brakes
		Float groundDist = GetDistanceInDirection( Vector3( 0.0f, 0.0f, -1.0f ) );
		m_sbData.m_currBrakingForce = 0.0f;

		if( ( groundDist <= m_sbData.m_minDistanceToGround ) || ( groundDist == FLT_MAX ) )
		{ // to close to ground?
			//m_currBrakingForce = m_maxBrakingForce;
		}
		else if( m_hoveringData.m_currSpeedVertical < 0.0f )
		{ // going down?
			
			//if( groundDist <= m_minDistanceToGround )
			//{
			//	m_currBrakingForce = m_maxBrakingForce;
			//}
			//else
			if( groundDist < m_sbData.m_startBrakingDistance )
			{
				///// @todo MS: division by zero
				//Float coef = ( groundDist - m_minDistanceToGround ) / ( m_startBrakingDistance - m_minDistanceToGround );
				//coef *= Abs( GetVerticalHoveringCoef() );
				//coef = Clamp( 1.0f - coef, 0.0f, 1.0f );
				//coef *= coef;	// make it exponential
				//m_currBrakingForce = coef * m_maxBrakingForce;

				Float H = groundDist - m_sbData.m_minDistanceToGround;
				if( H < 1e-5f )
				{
					/// @todo
				//	m_currBrakingForce = m_maxBrakingForce;
				}
				else
				{
					m_sbData.m_currBrakingForce = (m_mass * m_hoveringData.m_currSpeedVertical * m_hoveringData.m_currSpeedVertical) / H;
				}

			}

			//m_currBrakingSpeed += m_currBrakingACC * dt;
			//m_currBrakingSpeed = Min( m_currBrakingSpeed, m_maxBrakingSpeed );
			//m_currBrakingSpeed = Min( m_currBrakingSpeed, Abs( m_velocity.Z ) );
		}
		else
		{ // stop braking
			m_sbData.m_currBrakingForce = 0.0f;

			//if( m_currBrakingSpeed > 0.0f )
			//{
			//	m_currBrakingACC = -m_maxBrakingACC;
			//	m_currBrakingSpeed += m_currBrakingACC * dt;

			//	if( m_currBrakingSpeed < 0.0f )
			//	{
			//		m_currBrakingSpeed = 0.0f;
			//		m_currBrakingACC = 0.0f;
			//	}
			//}
		}

		if( m_sbData.m_currBrakingForce > 0.0f )
		{
			m_sbData.m_currBrakingACC += m_sbData.m_currBrakingForce * dt;
			m_sbData.m_currBrakingACC = Min( m_sbData.m_currBrakingACC, m_sbData.m_maxBrakingACC );
		}
		else
		{
			m_sbData.m_currBrakingACC = 0.0f;
		}

		m_hoveringData.m_currSpeedVertical += m_sbData.m_currBrakingACC * dt;

		//m_currHoveringSpeedVertical += m_currBrakingSpeed;
	}



	CombineRoll();

	TryMove( dt );

	UpdateIdle( dt );


	{ // altitude correction
		Vector3 currPos = GetEntity()->GetPosition();

		if( currPos.Z >= m_maxAltitude )
		{
			Float dist = currPos.Z - m_maxAltitude;

			currPos.Z = Lerp( dt * m_altitudeCorrectionCoef, currPos.Z, m_maxAltitude );

			if( m_hoveringData.m_currSpeedVertical > 0.0f )
			{
				m_hoveringData.m_currSpeedVertical = Lerp( dt * m_altitudeCorrectionCoef * dist, m_hoveringData.m_currSpeedVertical, 0.0f );
			}

			EulerAngles currRot = GetEntity()->GetRotation();

			currRot.Pitch = EulerAngles::NormalizeAngle180( currRot.Pitch );
			if( currRot.Pitch > 0.0f )
			{
				currRot.Pitch = Lerp( dt * m_altitudeCorrectionCoef * dist, currRot.Pitch, 0.0f );
				GetEntity()->SetRotation( currRot );
			}

			GetEntity()->SetPosition( currPos );
		}
	}

}










void CAVComponentTest::UpdateLinearSpeed( Float dt )
{
	Float targetSpeed =  ( ( m_accelerateInput >= 0.0f ) ? m_speedData.m_maxSpeedForward : m_speedData.m_maxSpeedReverse );
	targetSpeed *= m_accelerateInput;

	Float diff = targetSpeed - m_speedData.m_currSpeed;

	Float speedChange = GetCurrentAccFromInput( diff ) * dt;

	/// @todo MS: extract this if to some function, it is used more than once
	if( Abs( diff ) < speedChange )
	{
		speedChange = diff;
	}
	else if( diff < 0.0f )
	{
		speedChange = -speedChange;
	}

	m_speedData.m_currSpeed += speedChange;

	m_debugCurrACC = speedChange / dt;

	m_velocity = GetEntity()->GetWorldForward() * m_speedData.m_currSpeed;;
	m_speedData.m_driving = ( Abs( m_speedData.m_currSpeed ) > 0.01f ) || ( Abs( m_accelerateInput ) > 0.01f );
}













void CAVComponentTest::UpdateTurningPitch( Float dt )
{
	Float targetAngleSpeedV = m_turningData.m_maxPitchSpeed * m_steeringAxisInputY;


	// @todo MS: extract this to a new method
	{ // try to reach angular (pitch) speed with acc
		Float diff = targetAngleSpeedV - m_turningData.m_currPitchSpeed;
		Float pitchSpeedChange = m_turningData.m_pitchAcc * dt;

		if( Abs( diff ) < pitchSpeedChange )
		{
			pitchSpeedChange = diff;
		}
		else if( diff < 0.0f )
		{
			pitchSpeedChange = -pitchSpeedChange;
		}

		m_turningData.m_currPitchSpeed += pitchSpeedChange;
	}

	Float pitchChange = m_turningData.m_currPitchSpeed * dt;

	{	
		EulerAngles currRot = GetEntity()->GetRotation();

		{ // update entity rot
			currRot.Pitch = EulerAngles::NormalizeAngle180( currRot.Pitch + pitchChange );
			ClampPitch( currRot.Pitch );
		}

		{ // perform pitch snapping
			Float speedCoef = GetVehicleSpeedToMaxSpeedCoefAbs();
			Float currPitchSnapping = Lerp( speedCoef, m_turningData.m_pitchSnappingWhileStanding, m_turningData.m_pitchSnappingWhileFullSpeed );			
			currRot.Pitch = Lerp( currPitchSnapping * dt, currRot.Pitch, 0.0f );
		}

		GetEntity()->SetRotation( currRot );
	}

	m_turningData.m_turning |= ( Abs( pitchChange ) > 0.01f ) || ( Abs( m_steeringAxisInputY ) > 0.01f );
}













void CAVComponentTest::UpdateTurningYaw( Float dt )
{
	Float targetAngleSpeedH = m_turningData.m_maxYawSpeed * m_steeringAxisInputX;


	// @todo MS: extract this to a new method
	{ // try to reach angular (yaw) speed with acc
		Float diff = targetAngleSpeedH - m_turningData.m_currYawSpeed;
		Float yawSpeedChange = m_turningData.m_yawAcc * dt;

		if( Abs( diff ) < yawSpeedChange )
		{
			yawSpeedChange = diff;
		}
		else if( diff < 0.0f )
		{
			yawSpeedChange = -yawSpeedChange;
		}

		m_turningData.m_currYawSpeed += yawSpeedChange;
	}

	Float yawChange = m_turningData.m_currYawSpeed * dt;

	{ // update entity rot
		EulerAngles currRot = GetEntity()->GetRotation();
		currRot.Yaw = EulerAngles::NormalizeAngle180( currRot.Yaw + yawChange );

		/// @todo MS: division by zero check
		Float targetRoll = m_turningData.m_maxRoll * m_turningData.m_currYawSpeed / m_turningData.m_maxYawSpeed;

		if( m_speedData.m_currSpeed >= 0.0f )
		{
			targetRoll = -targetRoll;
		}

		/// @todo MS: coef to config
		m_turningData.m_currRoll = Lerp( dt * 2.0f, m_turningData.m_currRoll, targetRoll );

		GetEntity()->SetRotation( currRot );
	}


	m_turningData.m_turning |=  ( Abs( yawChange ) > 0.01f ) || ( Abs( m_steeringAxisInputX ) > 0.01f );
}











void CAVComponentTest::UpdateHovering( Float dt )
{
	//Vector3 currPos = GetEntity()->GetPosition();

	Float speedCoef = GetVehicleSpeedToMaxSpeedCoefAbs();
	Float currHVAcc		 = Lerp( speedCoef, m_hoveringData.m_minAccVertical, m_hoveringData.m_maxAccVertical );
	Float currHVTopSpeed = Lerp( speedCoef, m_hoveringData.m_minTopSpeedVertical, m_hoveringData.m_maxTopSpeedVertical );
	Float currHHAcc		 = Lerp( speedCoef, m_hoveringData.m_minAccHorizontal, m_hoveringData.m_maxAccHorizontal );
	Float currHHTopSpeed = Lerp( speedCoef, m_hoveringData.m_minTopSpeedHorizontal, m_hoveringData.m_maxTopSpeedHorizontal );

	Float dropMultiplier = 1.0f;
	{ // horizontal drop
		m_debugCurrDropCoef = 0.0f;

		Float groundDist = GetDistanceInDirection( Vector3( 0.0f, 0.0f, -1.0f ) );
		if( groundDist >= m_hoveringData.m_maxDropDistance )
		{
			dropMultiplier = m_hoveringData.m_dropMultiplier;
			m_debugCurrDropCoef = 1.0f;
		}
		else if( groundDist >= m_hoveringData.m_minDropDistance )
		{
			// @todo MS: division by zero
			Float coef = ( groundDist - m_hoveringData.m_minDropDistance ) / (m_hoveringData.m_maxDropDistance - m_hoveringData.m_minDropDistance);
			dropMultiplier = Lerp( coef, 1.0f, m_hoveringData.m_dropMultiplier );
			m_debugCurrDropCoef = coef;
		}

		
	}



	UpdateHoveringSpeed(
		dt,
		m_hoveringAxisInputY,
		currHVAcc * dropMultiplier,
		currHVTopSpeed * dropMultiplier,
		m_hoveringData.m_currSpeedVertical
	);

	Float hhTopSpeedForRoll = currHHTopSpeed;
	UpdateCurrentSideDrops( currHHTopSpeed, currHHAcc );
	UpdateHoveringSpeed(
		dt,
		m_hoveringAxisInputX,
		currHHAcc,
		currHHTopSpeed,
		m_hoveringData.m_currSpeedHorizontal
	);



	m_velocity += Vector3( 0.0f, 0.0f, m_hoveringData.m_currSpeedVertical );
	m_velocity += GetHorizontalHoveringVelocity();

	{
		/// @todo MS: division by zero check
		//Float hoveringPitch = m_maxHoveringPitch * m_currHoveringSpeedVertical / m_maxHoveringTopSpeedVertical;

		if(currHHTopSpeed < 1e-5f) 
		{
			m_hoveringData.m_currRoll = 0.0f;
		}
		else
		{
			m_hoveringData.m_currRoll = m_hoveringData.m_maxRoll * Clamp( ( m_hoveringData.m_currSpeedHorizontal / hhTopSpeedForRoll ), -1.0f, 1.0f );
		}
	}

	Bool hoveringInput = ( Abs( m_hoveringAxisInputY ) > 0.01f ) || ( Abs( m_hoveringAxisInputX ) > 0.01f );
	Bool hoveringResult = ( ( Abs( m_hoveringData.m_currSpeedVertical ) > 0.01f ) || ( Abs( m_hoveringData.m_currSpeedHorizontal ) > 0.01f ) );
	m_hoveringData.m_hovering = hoveringInput || hoveringResult;

	//GetEntity()->SetPosition( currPos );
}











void CAVComponentTest::UpdateIdle( Float dt )
{
	// if we are not in the idle state
	if( m_speedData.m_driving || m_turningData.m_turning || m_hoveringData.m_hovering )
	{
		m_idleData.m_time = 0.0f;
		return;
	}

	R6_ASSERT( GetEntity() );

	// if idling is shorter than one frame (doesn't have to be exact one frame)
	if( m_idleData.m_time < ( 1.0f/60.0f ) )
	{
		m_idleData.m_center = GetEntity()->GetPosition();		
	}


	m_idleData.m_time += dt;

	if( m_idleData.m_maxStrengthAfter > 0.0f )
	{
		m_idleData.m_currStrength = Lerp( m_idleData.m_time / m_idleData.m_maxStrengthAfter, 0.0f, 1.0f );
	}
	else
	{
		m_idleData.m_currStrength = 1.0f;
	}


	Vector3 currIdleOffset( 0.0f, 0.0f, 0.0f );
	/// @todo MS: sinf? Or some red smth?
	currIdleOffset.Z = sinf( m_idleData.m_time * m_idleData.m_waveFrequency ) * m_idleData.m_waveAmplitude * m_idleData.m_currStrength;



	GetEntity()->SetPosition( m_idleData.m_center + currIdleOffset );
}












/// @todo MS: rewrite this whole function!
/// This method is implemented just to have SOME collision response,
/// The collision is very inaccurate and this is nothing that we might want.
void CAVComponentTest::TryMove( Float dt )
{
	m_sbData.m_currBrakingSpeed = 0.0f;

	Vector3 currPos = GetEntity()->GetPosition();
	Vector3 targetPos = currPos + m_velocity * dt;


	if( dt < 1e-5f )
	{
		GetEntity()->SetPosition( targetPos );
		return;
	}


	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	R6_ASSERT( physicsWorld );

	/// @todo MS: change this
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include =
		GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | 
		GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) )  | 
		GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );

	const float RAD = 2.0f;

	SPhysicsContactInfo sweepInfo;
	if( physicsWorld->SweepTestWithSingleResult( currPos, targetPos, RAD, include, 0, sweepInfo ) == TRV_Hit )
	{
		if( sweepInfo.m_distance == 0.0f )
		{
			// @todo MS: tweak this
			Vector3 fromTop = currPos + Vector3( 0.0f, 0.0f, 5.0f );

			SPhysicsContactInfo sweepInfoTop;
			if( physicsWorld->SweepTestWithSingleResult( fromTop, currPos, RAD, include, 0, sweepInfoTop ) == TRV_Hit )
			{
				if( sweepInfoTop.m_distance > 0.0f )
				{
					sweepInfo = sweepInfoTop;
					targetPos = currPos;
					currPos = fromTop;
				}
			}
		}



		if( sweepInfo.m_distance < 0.1f )
		{
			sweepInfo.m_distance = 0.0f;
		}

		sweepInfo.m_distance *= 0.9f;



		targetPos = Lerp( sweepInfo.m_distance, currPos, targetPos );

		{ // velocities correction based on collision
			const Float ENERGY_PRESERVE = 0.4f;
			const Float ENERGY_PRESERVE_ANGLE = ENERGY_PRESERVE * 0.5f;

			{ // resolve vertical hovering
				if( sweepInfo.m_normal.Z * m_hoveringData.m_currSpeedVertical < 0.0f )
				{
					m_hoveringData.m_currSpeedVertical = -m_hoveringData.m_currSpeedVertical * ENERGY_PRESERVE;
				}
			}

			{ // resolve horizontal hovering
				Vector3 hhVel = GetHorizontalHoveringVelocity();
				if( hhVel.Dot( sweepInfo.m_normal ) < 0.0f )
				{
					m_hoveringData.m_currSpeedHorizontal = -m_hoveringData.m_currSpeedHorizontal * ENERGY_PRESERVE;
				}
			}

			{ // resolve linear speed
				Vector3 currDir = GetEntity()->GetWorldForward().Normalized3();
				Float d = currDir.Dot( sweepInfo.m_normal.Normalized3() );
				if( d < 0.0f)
				{
					if( d < -0.965925f ) // impact angle is smaller than 15 degrees
					{
						// simply bounce the AV
						m_speedData.m_currSpeed = -m_speedData.m_currSpeed * ENERGY_PRESERVE;
					}
					else
					{
						// we want the av to bounce more when the impact angle is smaller (more facial)
						Float reflectStrength = Clamp( d * d, 0.1f, 1.0f );

						Vector reflected = ReflectWithStrength( currDir, sweepInfo.m_normal, ENERGY_PRESERVE_ANGLE * reflectStrength );
						reflected.Z = -reflected.Z;
						reflected.W = 1.0f;
						EulerAngles tmpRot = reflected.ToEulerAngles();
						tmpRot.Roll = GetEntity()->GetRotation().Roll;
						GetEntity()->SetRotation( tmpRot );

						Float speedLoss = (1.0f - ENERGY_PRESERVE ) * reflectStrength;
						m_speedData.m_currSpeed *= Clamp( 1.0f - speedLoss, ENERGY_PRESERVE, 1.0f );
					}
				}
			}
		}
	}

	GetEntity()->SetPosition( targetPos );
}













void CAVComponentTest::CombineRoll()
{
	Float maxAbsRoll = Max( m_turningData.m_maxRoll, m_hoveringData.m_maxRoll );

	m_turningData.m_currRoll	= EulerAngles::NormalizeAngle180( m_turningData.m_currRoll );
	m_hoveringData.m_currRoll = EulerAngles::NormalizeAngle180( m_hoveringData.m_currRoll );

	Float newRoll = m_turningData.m_currRoll + m_hoveringData.m_currRoll;

	if( Abs( newRoll ) > maxAbsRoll )
	{
		newRoll = maxAbsRoll * Sgn( newRoll );
	}

	EulerAngles rot = GetEntity()->GetRotation();
	rot.Roll = newRoll;
	GetEntity()->SetRotation( rot );
}











Float CAVComponentTest::GetCurrentAccFromInput( Float diff )
{
	Bool positiveDiff = ( diff >= 0.0f );
	Bool positiveInput = ( m_accelerateInput >= 0.0f );

	Float currAcc = 0.0f;

	if( positiveInput )
	{
		if( positiveDiff )
		{
			// player wants to accelerate or keep speed
			currAcc = m_speedData.m_accForward;
		}
		else
		{
			// player wants to decelerate a bit but not brake
			currAcc = m_speedData.m_dccForward;
		}
	}
	else
	{
		if( positiveDiff )
		{
			// player released reverse acceleration a bit
			currAcc = m_speedData.m_dccReverse;
		}
		else
		{
			// player wants to accelerate on reverse gear
			currAcc = m_speedData.m_accReverse;
		}
	}

	return currAcc;
}











void CAVComponentTest::UpdateHoveringSpeed( Float dt, Float axisInput, Float axisAcc, Float axisMaxSpeed, Float& axisCurrSpeed )
{
	Float targetHovering = axisInput * axisMaxSpeed;


	// @todo MS: extract this to a new method
	{ // try to reach speed with acc
		Float diff = targetHovering - axisCurrSpeed;
		Float speedChange = axisAcc * dt;

		/// @todo MS: fabs?
		if( Abs( diff ) < speedChange )
		{
			speedChange = diff;
		}
		else if( diff < 0.0f )
		{
			speedChange = -speedChange;
		}

		axisCurrSpeed += speedChange;
	}
}












void CAVComponentTest::ClampPitch( Float& pitch )
{
	if( pitch >= 90.0f )
	{
		pitch = 90.0f;

		if( m_turningData.m_currPitchSpeed > 0.0f )
		{
			m_turningData.m_currPitchSpeed = 0.0f;
		}
	}
	else if( pitch <= -90.0f )
	{
		pitch = -90.0f;

		if( m_turningData.m_currPitchSpeed < 0.0f )
		{
			m_turningData.m_currPitchSpeed = 0.0f;
		}
	}
}














void CAVComponentTest::OnPilotMounted( CPilotComponent* pilot )
{
	TBaseClass::OnPilotMounted( pilot );
	SetIdle( false );
}









void CAVComponentTest::OnPilotDisMounted()
{
	TBaseClass::OnPilotDisMounted();
	SetIdle( true );
}












void CAVComponentTest::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if( !m_isPlayerControlled )
	{
		return;
	}

	R6_ASSERT( GetEntity() );

	{
		Vector pos = GetEntity()->GetPosition();
		frame->AddDebugScreenFormatedText( 100, 140, TXT( "AV.position = [ %f , %f , %f ]" ), pos.X, pos.Y, pos.Z );
	}

	{
		struct DistancePrinter
		{
			RED_INLINE static void Print( CRenderFrame* frame, Int32 Y, const Char* name, Float dist )
			{
				if( dist < FLT_MAX )
				{
					frame->AddDebugScreenFormatedText( 100, Y, TXT( "%s distance = [ %f ]" ), name, dist );
				}
				else
				{
					frame->AddDebugScreenFormatedText( 100, Y, TXT( "%s distance = [ INFINITY ]" ), name );
				}
			}
		};

		DistancePrinter::Print( frame, 160, TXT( "Ground" ),	GetGroundDistance() );
		DistancePrinter::Print( frame, 180, TXT( "Top" ),		GetTopDistance()	);
		DistancePrinter::Print( frame, 200, TXT( "Right" ),		GetRightDistance()	);
		DistancePrinter::Print( frame, 220, TXT( "Left" ),		GetLeftDistance()	);
	}

	{ // speed gauge
		Float speedCoef = m_speedData.m_currSpeed / Max( m_speedData.m_maxSpeedForward, m_speedData.m_maxSpeedReverse );
		DrawDebugGauge( frame, speedCoef, 100, 730, Color::RED, TXT( "SPEED" ) );
	}

	{ // acc gauge
		Float maxACC = Max( Max( m_speedData.m_accForward, m_speedData.m_accReverse ), Max( m_speedData.m_dccForward, m_speedData.m_dccReverse ) );
		Float accCoef = m_debugCurrACC / maxACC;
		DrawDebugGauge( frame, accCoef, 100, 600, Color::RED, TXT( "ACC" ) );
	}

	{ // hovering gauge
		Float tmp = GetVehicleSpeedToMaxSpeedCoefAbs();
		Float currMaxVH = Lerp( tmp, m_hoveringData.m_minTopSpeedVertical, m_hoveringData.m_maxTopSpeedVertical );
		Float currMaxHH = Lerp( tmp, m_hoveringData.m_minTopSpeedHorizontal, m_hoveringData.m_maxTopSpeedHorizontal );
		Float VHCoef = m_hoveringData.m_currSpeedVertical / currMaxVH;
		Float HHCoef = m_hoveringData.m_currSpeedHorizontal / currMaxHH;

		DrawDebugGauge( frame, VHCoef, 200, 730, Color::RED, TXT( "VH" ) );
		DrawDebugGauge( frame, HHCoef, 200, 600, Color::RED, TXT( "HH" ) );
	}

	{ // turning
		Float yawCoef = -m_turningData.m_currYawSpeed / m_turningData.m_maxYawSpeed;
		Float pitchCoef = m_turningData.m_currPitchSpeed / m_turningData.m_maxPitchSpeed;
		DrawDebugGauge( frame, yawCoef, 300, 730, Color::RED, TXT( "Yaw Speed" ) );
		DrawDebugGauge( frame, pitchCoef, 300, 600, Color::RED, TXT( "Pitch Speed" ) );
	}

	{ // drops
		DrawDebugGauge( frame, m_debugCurrDropCoef, 400, 730, Color::RED, TXT( "Drop strength" ) );
	}

	{ // safety brakes
		DrawDebugGauge( frame, m_sbData.m_currBrakingForce / m_sbData.m_maxBrakingForce, 500, 730, Color::RED, TXT( "SB FORCE" ) );
		DrawDebugGauge( frame, m_sbData.m_currBrakingACC / m_sbData.m_maxBrakingACC, 600, 730, Color::RED, TXT( "SB ACC" ) );
		DrawDebugGauge( frame, m_sbData.m_currBrakingSpeed / m_sbData.m_maxBrakingSpeed, 600, 600, Color::RED, TXT( "COLLISION" ) );
	}

	{ // side drops
		DrawDebugGauge( frame, m_debugCurrSideACCMult,	 700, 600, Color::RED, TXT( "SD ACC" ) );
		DrawDebugGauge( frame, m_debugCurrSideSpeedMult, 700, 730, Color::RED, TXT( "SD SPEED" ) );
	}
}








Vector3 CAVComponentTest::ProjectToUnitOnXYPlane( const Vector3& v ) const
{
	Vector3 toRet( v.X, v.Y, 0.0f );
	/// @todo MS: check if no zero division
	return toRet.Normalized();
}










Float CAVComponentTest::GetDistanceInDirection( const Vector3& dir ) const
{
	// @todo MS: rewrite this
	static const Float MAX_DIST = 10000.0f;

	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	R6_ASSERT( physicsWorld );
	
	SPhysicsContactInfo contactInfo;
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include =
		GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | 
		GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) )  | 
		GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );

	Vector pos = GetEntity()->GetPosition();

	if ( physicsWorld->RayCastWithSingleResult( pos, pos + dir.Normalized() * MAX_DIST, include, 0, contactInfo ) == TRV_Hit )
	{
		return (contactInfo.m_position - pos).Mag3();
	}

	/// @todo MS: FLT_MAX in RED? ( refactor all references )
	return FLT_MAX;
}











void CAVComponentTest::UpdateCurrentSideDrops( Float& outSpeed, Float& outAcc)
{
	Float rightDist = Clamp( GetRightDistance(), m_hoveringData.m_minSideDropDistance, m_hoveringData.m_maxSideDropDistance );
	Float leftDist	= Clamp( GetLeftDistance(),	 m_hoveringData.m_minSideDropDistance, m_hoveringData.m_maxSideDropDistance );

	Float w = m_hoveringData.m_maxSideDropDistance - m_hoveringData.m_minSideDropDistance;

	/// @todo MS: Division by zero
	Float rightDistCoef = ( rightDist - m_hoveringData.m_minSideDropDistance ) / w;
	Float leftDistCoef	= ( leftDist  - m_hoveringData.m_minSideDropDistance ) / w;

	R6_ASSERT( rightDistCoef >= 0.0f && rightDistCoef <= 1.0f );
	R6_ASSERT( leftDistCoef  >= 0.0f && leftDistCoef  <= 1.0f );


	if( m_hoveringData.m_currSpeedHorizontal > 0.0f )
	{
		outSpeed *= m_hoveringData.m_sideDropMultiplier * rightDistCoef;

		m_debugCurrSideSpeedMult = rightDistCoef;

		if( m_hoveringAxisInputX > 0.0f )
		{
			outAcc *= Lerp( rightDistCoef, 1.0f, m_hoveringData.m_sideDropMultiplier );
			m_debugCurrSideACCMult = rightDistCoef;
		}
		else
		{
			outAcc *= Lerp( leftDistCoef, 1.0f, m_hoveringData.m_sideDropMultiplier );
			m_debugCurrSideACCMult = - leftDistCoef;
		}
	}
	else if( m_hoveringData.m_currSpeedHorizontal < 0.0f )
	{
		outSpeed *= m_hoveringData.m_sideDropMultiplier * leftDistCoef;

		m_debugCurrSideSpeedMult = -leftDistCoef;

		if( m_hoveringAxisInputX < 0.0f )
		{
			outAcc *= Lerp( leftDistCoef, 1.0f, m_hoveringData.m_sideDropMultiplier );
			m_debugCurrSideACCMult = - leftDistCoef;
		}
		else
		{
			outAcc *= Lerp( rightDistCoef, 1.0f, m_hoveringData.m_sideDropMultiplier );
			m_debugCurrSideACCMult = rightDistCoef;
		}
	}
}













void CAVComponentTest::DrawDebugGauge( CRenderFrame* frame, Float val, Float X, Float Y, const Color& color, const Char* caption /*= NULL */ )
{
	Vector2 from( X, Y );
	Vector2 len = MathUtils::GeometryUtils::Rotate2D( Vector2( 0.0f, -75.0f ), val * M_PI  * 0.5f );
	frame->AddDebugLineOnScreen( from, from + len, color );

	if( caption )
	{
		frame->AddDebugScreenFormatedText( (Int32)X, (Int32)Y + 10, caption );
	}
}













