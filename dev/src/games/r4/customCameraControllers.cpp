
#include "build.h"
#include "customCameraControllers.h"
#include "customCameraCollision.h"
#include "customCamera.h"
#include "r4Player.h"
#include "../../common/game/entityParams.h"
#include "../../common/engine/characterControllerParam.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/debugWindowsManager.h"
#include "../../common/engine/curve.h"
#include "../../common/engine/renderFrame.h"

//#define DISABLE_CAMERA_COLLISIONS


IMPLEMENT_ENGINE_CLASS( ICustomCameraBaseController );

RED_DEFINE_NAME( ControllerActivate );
RED_DEFINE_NAME( ControllerDeactivate );
RED_DEFINE_NAME( ControllerUpdate );
RED_DEFINE_NAME( ControllerSetDesiredPosition );

RED_DEFINE_NAME( ControllerSetDesiredYaw );
RED_DEFINE_NAME( ControllerSetDesiredPitch );
RED_DEFINE_NAME( ControllerRotateHorizontal );
RED_DEFINE_NAME( ControllerRotateVertical );
RED_DEFINE_NAME( ControllerStopRotating );
RED_DEFINE_NAME( ControllerGetRotationDelta );
RED_DEFINE_NAME( ControllerUpdateInput );

RED_DEFINE_NAME( ControllerSetDesiredDistance );

RED_DEFINE_NAME( ControllerGetPosition );
RED_DEFINE_NAME( ControllerGetRotation );

//RED_DEFINE_STATIC_NAME( playerSpeed );

//#define LOG_CAMERA_ENABLED

#ifdef LOG_CAMERA_ENABLED
	#define LOG_CAMERA( channel, message, ... ) RED_LOG( channel, message, ##__VA_ARGS__ )
#else
	#define LOG_CAMERA( ... )
#endif

//////////////////////////////////////////////////////////////////////////
// PIVOT CONTROLLERS
//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( SCameraMovementData );
IMPLEMENT_ENGINE_CLASS( SCustomCameraPreset );
IMPLEMENT_ENGINE_CLASS( ICustomCameraPivotPositionController );

ICustomCameraPivotPositionController::ICustomCameraPivotPositionController()
	: m_pivotZSmoothTime( 0.f )
	, m_dampPivotZ( m_pivotZSmoothTime )
{

}

void ICustomCameraPivotPositionController::funcSetDesiredPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZERO_3D_POINT );
	GET_PARAMETER_OPT( Float, mult, 1.f );
	FINISH_PARAMETERS;

	SetDesiredPosition( position, mult );
}

void ICustomCameraPivotPositionController::funcUpdate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, currentPosition, Vector::ZERO_3D_POINT );
	GET_PARAMETER_REF( Vector, currentVelocity, Vector::ZEROS );
	GET_PARAMETER( Float, timeDelta, 0.f );
	FINISH_PARAMETERS;

	Update( currentPosition, currentVelocity, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraRopePPC );

CCustomCameraRopePPC::CCustomCameraRopePPC()
	: m_mult( 1.f )
	, m_dampFactor( 1.f )
	, m_damp( m_mult )
	, m_ropeLength( 1.5f )
	, m_smoothZ( 0.5f )
	, m_dampZ( m_smoothZ )
	, m_desiredPosition( GGame->GetPlayerEntity() ? GGame->GetPlayerEntity()->GetWorldPositionRef() : Vector::ZERO_3D_POINT )
{
	m_damp.Force( m_desiredPosition );
	m_dampZ.Force( m_offsetZ );
}

void CCustomCameraRopePPC::Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta )
{
	UpdateStep( currentPosition, currentVelocity, timeDelta );
	m_damp.Update( currentPosition, currentVelocity, m_desiredPosition, timeDelta );
}

void CCustomCameraRopePPC::UpdateStep( Vector& currentPosition, Vector& currentVelocity, Float timeDelta )
{
	CPlayer* player = GCommonGame->GetPlayer();

	// Apply rope constraints
	const Vector playerPos = player ? player->GetWorldPositionRef() : Vector::ZERO_3D_POINT;
	const Vector direction = playerPos - m_desiredPosition;
	const Float distance = direction.Mag2();

	if ( distance > m_ropeLength )
	{
		m_desiredPosition = playerPos + direction.Normalized2() * -m_ropeLength;
	}

	Float offsetIK = player ? (player->GetMovingAgentComponent() ? player->GetMovingAgentComponent()->GetAnimationProxy().GetAppliedIKOffsetWS().Z : 0.f) : 0.f;

	m_dampZ.Update( m_offsetZ, timeDelta );
	m_dampPivotZ.Update( playerPos.Z, timeDelta );
	m_desiredPosition.Z = m_dampPivotZ.GetValue() + m_dampZ.GetValue() + offsetIK;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraPlayerPPC );

CCustomCameraPlayerPPC::CCustomCameraPlayerPPC()
	: m_mult( 1.f )
	, m_dampFactor( 0.1f )
	, m_damp( m_mult )
	, m_smoothZ( 0.5f )
	, m_dampZ( m_smoothZ )
{
	m_dampZ.Force( m_offsetZ );
}

void CCustomCameraPlayerPPC::Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta )
{
	CPlayer* player = GCommonGame->GetPlayer();

	Vector position = player ? player->GetWorldPositionRef() : Vector::ZERO_3D_POINT;

	Float offsetIK = player ? player->GetMovingAgentComponent()->GetAnimationProxy().GetAppliedIKOffsetWS().Z : 0.f;

	m_dampZ.Update( m_offsetZ, timeDelta );
	m_dampPivotZ.Update( position.Z, timeDelta );
	position.Z = m_dampPivotZ.GetValue() + m_dampZ.GetValue() + offsetIK;

	m_damp.Update( currentPosition, currentVelocity, position, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraBlendPPC );

CCustomCameraBlendPPC::CCustomCameraBlendPPC()
	: m_from( NULL )
	, m_to( NULL )
	, m_ownerRef( NULL )
	, m_weight( 0.f )
	, m_blendTime( 1.f )
{}

void CCustomCameraBlendPPC::Setup( ICustomCameraPivotPositionController* from, ICustomCameraPivotPositionController* to, Float blendTime )
{
	m_from = from;
	m_to = to;
	m_blendTime = blendTime;
	m_controllerName = m_to->GetControllerName();
}

void CCustomCameraBlendPPC::Activate( const Vector& currentPosition, Float currentOffsetZ )
{
	ASSERT( m_from && m_to );

	m_to->Activate( currentPosition, currentOffsetZ );
	m_weight = 0.f;
}

void CCustomCameraBlendPPC::Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta )
{
	ASSERT( m_from && m_to );

	// Just in case
	m_from->SetZOffset( m_offsetZ );
	m_to->SetZOffset( m_offsetZ );

	// create copy for move data
	Vector currentPos1 = currentPosition;
	Vector tempVel = currentVelocity;
	m_from->Update( currentPos1, tempVel, timeDelta );
	
	Vector currentPos2 = currentPosition;
	m_to->Update( currentPos2, currentVelocity, timeDelta );

	if( tempVel.SquareMag3() < currentVelocity.SquareMag3() )
	{
		currentVelocity = tempVel;
	}

	m_weight = Min( m_weight + timeDelta / m_blendTime, 1.f );

	const Vector oldPosition = currentPosition;
	currentPosition = Vector::Interpolate( currentPos1, currentPos2, m_weight );

	if( m_weight == 1.f && m_ownerRef )
	{
		*m_ownerRef = m_to;
	}
}

void CCustomCameraBlendPPC::SetDesiredPosition( const Vector& position, Float mult )
{
	ASSERT( m_from && m_to );

	m_from->SetDesiredPosition( position, mult );
	m_to->SetDesiredPosition( position, mult );
}

Float CCustomCameraBlendPPC::GetCurrentZOffset() const
{
	ASSERT( m_from && m_to );

	return (1.f - m_weight) * m_from->GetCurrentZOffset() + m_weight * m_to->GetCurrentZOffset();
}

void CCustomCameraBlendPPC::GenerateDebugFragments( CRenderFrame* frame )
{
	ASSERT( m_from && m_to );

	m_from->GenerateDebugFragments( frame );
	m_to->GenerateDebugFragments( frame );
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraBoatPPC );

CCustomCameraBoatPPC::CCustomCameraBoatPPC()
	: m_offset( 0.0f, 0.0f, 0.0f )
{
}

void CCustomCameraBoatPPC::Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta )
{
	UpdateStep( currentPosition, currentVelocity, timeDelta );
	Vector tmp = m_desiredPosition + m_offset;
	currentPosition = tmp;
	m_damp.Update( currentPosition, currentVelocity, tmp, timeDelta );
}

void CCustomCameraBoatPPC::funcSetPivotOffset( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, offset, Vector::ZEROS );
	FINISH_PARAMETERS;

	m_offset = offset;
}

//////////////////////////////////////////////////////////////////////////
// PIVOT ROTATION CONTROLLERS
//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( ICustomCameraPivotRotationController );

void ICustomCameraPivotRotationController::Activate( const EulerAngles& currentRotation, Uint32 flags )
{
	 m_flags = flags; 
	 ClearRotationFlag( ECCPRF_DesiredPitch | ECCPRF_DesiredYaw ); 
	 GGame->GetInputManager()->SetSensitivityPreset( m_sensitivityPreset );
}

void ICustomCameraPivotRotationController::funcSetDesiredHeading( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, heading, 0.f );
	GET_PARAMETER_OPT( Float, mult, 1.f );
	FINISH_PARAMETERS;

	SetDesiredYaw( heading, mult );
}

void ICustomCameraPivotRotationController::funcSetDesiredPitch( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, pitch, 0.f );
	GET_PARAMETER_OPT( Float, mult, 1.f );
	FINISH_PARAMETERS;

	SetDesiredPitch( pitch, mult );
}

void ICustomCameraPivotRotationController::funcRotateHorizontal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, right, false );
	GET_PARAMETER_OPT( Float, mult, 1.f );
	FINISH_PARAMETERS;

	RotateHorizontal( right, mult );
}

void ICustomCameraPivotRotationController::funcRotateVertical( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, up, false );
	GET_PARAMETER_OPT( Float, mult, 1.f );
	FINISH_PARAMETERS;

	RotateVertical( up, mult );
}

void ICustomCameraPivotRotationController::funcStopRotating( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	StopRotating();
}

void ICustomCameraPivotRotationController::funcUpdate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EulerAngles, currentRotation, EulerAngles::ZEROS );
	GET_PARAMETER_REF( EulerAngles, currentVelocity, EulerAngles::ZEROS );
	GET_PARAMETER( Float, timeDelta, 0.f );
	FINISH_PARAMETERS;

	Update( currentRotation, currentVelocity, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraDefaultPRC );

CCustomCameraDefaultPRC::CCustomCameraDefaultPRC()
	: m_dampYawFactor( 1.f )
	, m_dampPitchFactor( 1.f )
	, m_horMult( 1.f )
	, m_verMult( 1.f )
	, m_yawSmooth( m_dampYawFactor )
	, m_pitchSmooth( m_dampPitchFactor )
	, m_yawDamp( m_yawSmooth )
	, m_pitchDamp( m_pitchSmooth )
	, m_desiredYaw( 0.f )
	, m_desiredPitch( 0.f )
	, m_yawAcceleration( 1000.f )
	, m_yawMaxVelocity( 250.f )
	, m_pitchAcceleration( 1000.f )
	, m_pitchMaxVelocity( 250.f )
	, m_rotationDelta( EulerAngles::ZEROS )
{
}

void CCustomCameraDefaultPRC::UpdateInput( Bool& movedHorizontal, Bool& movedVertical )
{
	const CInputManager* inputManager = GGame->GetInputManager();

	const Float cameraInputWeight = GGame->GetActiveWorld()->GetCameraDirector()->GetInputWeight();

	Float valueMouseX = inputManager->GetActionValue( CNAME( GI_MouseDampX ) ) * cameraInputWeight;
	Float valueMouseY = inputManager->GetActionValue( CNAME( GI_MouseDampY ) ) * cameraInputWeight;


#ifndef NO_DEBUG_WINDOWS
	if( GDebugWin::GetInstance().GetVisible() )
	{
		valueMouseX = valueMouseY = 0.0f;
	}
#endif	// NO_DEBUG_WINDOWS
	

	if( valueMouseX || valueMouseY )
	{
		movedHorizontal = true;
		movedVertical = true;

		SetRotationFlags( ECCPRF_ManualHorizontal | ECCPRF_ManualVertical | ECCPRF_AbsoluteHorizontal | ECCPRF_AbsoluteVertical );
		
		m_absoluteYawDelta = valueMouseX * 0.3f;
		m_absolutePitchDelta = valueMouseY * 0.3f;
	}
	else
	{
		m_absoluteYawDelta = 0.f;
		m_absolutePitchDelta = 0.f;

		ClearRotationFlag( ECCPRF_ManualHorizontal | ECCPRF_ManualVertical | ECCPRF_AbsoluteHorizontal | ECCPRF_AbsoluteVertical );

		Float value = inputManager->GetActionValue( CNAME( GI_AxisRightX ) ) * cameraInputWeight;
		if( value != 0.f  )
		{
			ClearRotationFlag( ECCPRF_HorizontalRotation );

			movedHorizontal = true;

			SetRotationFlag( ECCPRF_ManualHorizontal );

			SetRotationFlag( value > 0.f ? ECCPRF_RotateLeft : ECCPRF_RotateRight );
			m_horMult = MAbs( value );
		}
		else if( HasRotationFlag( ECCPRF_ManualHorizontal ) )
		{
			ClearRotationFlag( ECCPRF_ManualHorizontal | ECCPRF_HorizontalRotation );
		}

		value = inputManager->GetActionValue( CNAME( GI_AxisRightY ) ) * cameraInputWeight;
		if( value != 0.f )
		{
			ClearRotationFlag( ECCPRF_VerticalRotation );

			movedVertical = true;

			SetRotationFlag( ECCPRF_ManualVertical );

			SetRotationFlag( value < 0.f ? ECCPRF_RotateUp : ECCPRF_RotateDown );
			m_verMult = MAbs( value );
		}
		else if( HasRotationFlag( ECCPRF_ManualVertical ) )
		{
			ClearRotationFlag( ECCPRF_ManualVertical | ECCPRF_VerticalRotation );
		}
	}
}

#define ANGLE_DIFF_TO_DECELERATE 50.f
#define ANGLE_DIFF_MULT 0.02f

void CCustomCameraDefaultPRC::Update( EulerAngles& currentRotation, EulerAngles& currentVelocity, Float timeDelta )
{
	const EulerAngles prevRot = currentRotation;

	// This should happen only if max/min-Pitch changes
	const Bool pitchOutOfBounds = currentRotation.Pitch > m_maxPitch || currentRotation.Pitch < m_minPitch;
	if( ( currentRotation.Pitch > m_maxPitch && !( m_flags & ECCPRF_RotateUp ) ) || ( currentRotation.Pitch < m_minPitch && !( m_flags & ECCPRF_RotateDown ) ) )
	{
		m_flags |= ECCPRF_DesiredPitch;
	}

	// Slow down when reaching peak values
	Float peakValuesMult = 1.f;
	if( currentRotation.Pitch <= m_maxPitch && m_flags & ECCPRF_RotateDown )
	{
		const Float toMax = MAbs( m_maxPitch - currentRotation.Pitch );
		
		peakValuesMult = toMax <= ANGLE_DIFF_TO_DECELERATE ? toMax * ANGLE_DIFF_MULT : 1.f;
	}
	else if( currentRotation.Pitch >= m_minPitch && m_flags & ECCPRF_RotateUp )
	{
		const Float toMin = MAbs( m_minPitch - currentRotation.Pitch );

		peakValuesMult = toMin <= ANGLE_DIFF_TO_DECELERATE ? toMin * ANGLE_DIFF_MULT : 1.f;
	}

	if( m_flags & (ECCPRF_AbsoluteHorizontal | ECCPRF_AbsoluteVertical) )
	{
		currentRotation.Yaw -= m_absoluteYawDelta;
		currentRotation.Pitch = Clamp( currentRotation.Pitch - m_absolutePitchDelta, m_minPitch, m_maxPitch );

		m_yawDamp.Force( currentRotation.Yaw );
		m_pitchDamp.Force( currentRotation.Pitch );

		m_rotationDelta = EulerAngles::AngleDistance( prevRot, currentRotation );

		currentVelocity = timeDelta > 0.f ? m_rotationDelta / timeDelta : EulerAngles::ZEROS;
	}
	else
	{
		if( m_flags & ECCPRF_HorizontalRotation )
		{
			if( m_flags & ECCPRF_DesiredYaw )
			{
				m_yawDamp.Update( currentRotation.Yaw, currentVelocity.Yaw, m_desiredYaw, timeDelta );
			}
			else
			{
				if( m_flags & ECCPRF_RotateRight )
				{
					const Float actualMaxVel = m_yawMaxVelocity * m_horMult;
					if( currentVelocity.Yaw > actualMaxVel )
					{
						ApplyDeceleration( currentVelocity.Yaw, m_yawAcceleration * timeDelta, actualMaxVel );
					}
					else
					{
						currentVelocity.Yaw = Min( currentVelocity.Yaw + m_yawAcceleration * timeDelta * m_horMult, actualMaxVel );
					}
				}
				else// if( m_mode & ECCPRM_RotateLeft )
				{
					const Float actualMaxVel = -m_yawMaxVelocity * m_horMult;
					if( currentVelocity.Yaw < actualMaxVel )
					{
						ApplyDeceleration( currentVelocity.Yaw, m_yawAcceleration * timeDelta, actualMaxVel );
					}
					else
					{
						currentVelocity.Yaw = Max( currentVelocity.Yaw - m_yawAcceleration * timeDelta * m_horMult, actualMaxVel );
					}
				}

				currentRotation.Yaw += currentVelocity.Yaw * timeDelta;
			}
		}
		else if( currentVelocity.Yaw != 0.f )
		{
#ifdef RED_PLATFORM_CONSOLE
			ApplyDeceleration( currentVelocity.Yaw, m_yawAcceleration * timeDelta );
#else
			if( GGame->GetInputManager() && GGame->GetInputManager()->LastUsedPCInput() )
			{
				currentVelocity.Yaw = 0.0f;
			}
			else
			{
				ApplyDeceleration( currentVelocity.Yaw, m_yawAcceleration * timeDelta );
			}
#endif

			currentRotation.Yaw += currentVelocity.Yaw * timeDelta;
		}

		if( m_flags & ECCPRF_VerticalRotation )
		{
			if( m_flags & ECCPRF_DesiredPitch )
			{
				m_pitchDamp.Update( currentRotation.Pitch, currentVelocity.Pitch, m_desiredPitch, timeDelta );
			}
			else
			{
				if( m_flags & ECCPRF_RotateDown )
				{
					const Float actualMaxVel = m_pitchMaxVelocity * m_verMult * peakValuesMult;
					LOG_CAMERA( RED_LOG_CHANNEL( CustomCamera ), TXT("Current Max Pitch = %f | Current Pitch = %f"), m_maxPitch, currentRotation.Pitch );
					LOG_CAMERA( RED_LOG_CHANNEL( CustomCamera ), TXT("Current Pitch Vel = %f | Actual Max Vel = %f"), currentVelocity.Pitch, actualMaxVel );
					if( currentVelocity.Pitch > actualMaxVel )
					{
						currentVelocity.Pitch = Max( currentVelocity.Pitch - m_pitchAcceleration * timeDelta * m_verMult, actualMaxVel );
						LOG_CAMERA( RED_LOG_CHANNEL( CustomCamera ), TXT("New Pitch Vel = %f"), currentVelocity.Pitch );
					}
					else
					{
						currentVelocity.Pitch = Min( currentVelocity.Pitch + m_pitchAcceleration * timeDelta * m_verMult, actualMaxVel );
					}
				}
				else// if( m_mode & ECCPRM_RotateUp )
				{
					const Float actualMaxVel = -m_pitchMaxVelocity * m_verMult * peakValuesMult;
					if( currentVelocity.Pitch < actualMaxVel )
					{
						currentVelocity.Pitch = Min( currentVelocity.Pitch + m_pitchAcceleration * timeDelta * m_verMult, actualMaxVel );
					}
					else
					{
						currentVelocity.Pitch = Max( currentVelocity.Pitch - m_pitchAcceleration * timeDelta * m_verMult, actualMaxVel );
					}
				}

				currentRotation.Pitch += currentVelocity.Pitch * timeDelta;
				if( !pitchOutOfBounds && (currentRotation.Pitch < m_minPitch || currentRotation.Pitch > m_maxPitch) )
				{
					currentRotation.Pitch = Clamp( currentRotation.Pitch, m_minPitch, m_maxPitch );
					currentVelocity.Pitch = 0.f;
				}
			}
		}
		else if( currentVelocity.Pitch != 0.f )
		{
			ApplyDeceleration( currentVelocity.Pitch, m_pitchAcceleration * timeDelta );
			currentRotation.Pitch += currentVelocity.Pitch * timeDelta;
			if( !pitchOutOfBounds && (currentRotation.Pitch < m_minPitch || currentRotation.Pitch > m_maxPitch) )
			{
				currentRotation.Pitch = Clamp( currentRotation.Pitch, m_minPitch, m_maxPitch );
				currentVelocity.Pitch = 0.f;
			}
		}

		m_rotationDelta = EulerAngles::AngleDistance( prevRot, currentRotation );
	}
}

void CCustomCameraDefaultPRC::SetDesiredYaw( Float yaw, Float mult /*= 1.f */ )
{
	m_desiredYaw = yaw;
	if( mult > 0 ) m_yawSmooth = m_dampYawFactor / mult;
	else m_yawSmooth = FLT_MAX;

	if( !(m_flags & ECCPRF_ManualHorizontal) )
	{
		m_flags |= ECCPRF_DesiredYaw;
	}
}

void CCustomCameraDefaultPRC::SetDesiredPitch( Float pitch, Float mult /*= 1.f */ )
{
	m_desiredPitch = Clamp( pitch, m_minPitch, m_maxPitch );
	if( mult > 0 ) m_pitchSmooth = m_dampPitchFactor / mult;
	else m_pitchSmooth = FLT_MAX;

	if( !(m_flags & ECCPRF_ManualVertical) )
	{
		m_flags |= ECCPRF_DesiredPitch;
	}
}

void CCustomCameraDefaultPRC::RotateHorizontal( Bool right, Float mult /*= 1.f */ )
{
	if( !(m_flags & ECCPRF_ManualHorizontal) )
	{
		m_flags |= right ? ECCPRF_RotateRight : ECCPRF_RotateLeft;
		m_horMult = mult;
	}
}

void CCustomCameraDefaultPRC::RotateVertical( Bool up, Float mult /*= 1.f */ )
{
	if( !(m_flags & ECCPRF_ManualVertical ) )
	{
		m_flags |= up ? ECCPRF_RotateUp : ECCPRF_RotateDown;
		m_verMult = mult;
	}
}

void CCustomCameraDefaultPRC::StopRotating()
{
	if( !(m_flags & ECCPRF_ManualHorizontal) )
	{
		m_flags &= ~ECCPRF_HorizontalRotation;
	}
	
	if( !(m_flags & ECCPRF_ManualVertical) )
	{
		m_flags &= ~ECCPRF_VerticalRotation;
	}
}

void CCustomCameraDefaultPRC::RotateAbsolute( Float yaw, Float pitch )
{

}


//////////////////////////////////////////////////////////////////////////
// PIVOT DISTANCE CONTROLLERS
//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( ICustomCameraPivotDistanceController );

void ICustomCameraPivotDistanceController::funcSetDesiredDistance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, distance, 0.f );
	GET_PARAMETER_OPT( Float, mult, 1.f );
	FINISH_PARAMETERS;

	SetDesiredDistance( distance, mult );
}

void ICustomCameraPivotDistanceController::funcUpdate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Float, currentDistance, 0.f );
	GET_PARAMETER_REF( Float, currentVelocity, 0.f );
	GET_PARAMETER( Float, timeDelta, 0.f );
	FINISH_PARAMETERS;

	Update( currentDistance, currentVelocity, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraDefaultPDC );

CCustomCameraDefaultPDC::CCustomCameraDefaultPDC()
	: m_mult( 1.f )
	, m_dampFactor( 0.7f )
	, m_damp( m_mult )
	, m_desiredDistance( 3.f )
{
}

void CCustomCameraDefaultPDC::Activate( Float currentDistance )
{
	m_desiredDistance = Clamp( currentDistance, m_minDist, m_maxDist );
}

void CCustomCameraDefaultPDC::Update( Float& currentDistance, Float& currentVelocity, Float timeDelta )
{
	m_damp.Update( currentDistance, currentVelocity, m_desiredDistance, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraAdditivePDC );

CCustomCameraAdditivePDC::CCustomCameraAdditivePDC()
	: CCustomCameraDefaultPDC()
	, m_addedValue( 0.f )
{
}


//////////////////////////////////////////////////////////////////////////
// CAMERA POSITION CONTROLLERS
//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( ICustomCameraPositionController );

void ICustomCameraPositionController::OnBeforeStartBlendFrom( SCameraMovementData& out, const ICamera::Data& cameraData )
{
	CEntity* player = GGame->GetPlayerEntity();
	const Vector& playerPosition = player->GetWorldPositionRef();

	const Float defaultCameraZOffset = 1.5f;
	const Float defaultCameraAngle = -9.5f;

	out.m_pivotPositionValue = playerPosition;
	out.m_pivotPositionValue.Z += defaultCameraZOffset;
	out.m_pivotPositionVelocity = Vector::ZEROS;

	out.m_pivotRotationValue = cameraData.m_rotation;
	out.m_pivotRotationValue.Pitch = defaultCameraAngle;
	out.m_pivotRotationValue.Roll = 0.0f;
	out.m_pivotRotationVelocity = EulerAngles::ZEROS;

	out.m_pivotDistanceValue = cameraData.m_position.DistanceTo( out.m_pivotPositionValue );
	out.m_pivotDistanceVelocity = 0.0f;

	out.m_cameraLocalSpaceOffset = m_currentPreset.m_explorationOffset;
	out.m_cameraLocalSpaceOffsetVel = Vector::ZEROS;
	out.m_cameraOffset = Vector::ZEROS;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraSimplePositionController );

CCustomCameraSimplePositionController::CCustomCameraSimplePositionController()
	: m_finalPosition( Vector::ZERO_3D_POINT )
	, m_finalRotation( EulerAngles::ZEROS )
	, m_collisionController( NULL )
	, m_collisionController2( NULL )
{
	m_enableAutoCollisionAvoidance = false;
}

void CCustomCameraSimplePositionController::OnPostLoad()
{
	if( !m_collisionController )
	{
		m_collisionController = CreateObject< CCustomCameraAutoAvoidanceCollisionController >( this );
		m_collisionController->OnPostLoad();
	}

	if( !m_collisionController2 )
	{
		m_collisionController2 = CreateObject< CCustomCameraOutdoorCollisionController >( this );
		m_collisionController2->OnPostLoad();
	}
}

void CCustomCameraSimplePositionController::Update( SCameraMovementData& moveData, Float timeDelta )
{
	moveData.m_pivotPositionController.Get()->Update( moveData.m_pivotPositionValue, moveData.m_pivotPositionVelocity, timeDelta );
	moveData.m_pivotRotationController.Get()->Update( moveData.m_pivotRotationValue, moveData.m_pivotRotationVelocity, timeDelta );
	moveData.m_pivotDistanceController.Get()->Update( moveData.m_pivotDistanceValue, moveData.m_pivotDistanceVelocity, timeDelta );

	if( m_enableAutoCollisionAvoidance && !moveData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal ) )
	{
		m_collisionController->Update( moveData, timeDelta );
	}
	else
	{
		m_collisionController->Force( moveData );
	}

	m_collisionController2->Update( moveData, timeDelta );

	m_finalPosition = m_collisionController2->GetPosition();
	m_finalRotation = m_collisionController2->GetRotation();

	//////////////////////////////////////////////////////////////////////////

	/*const Vector direction = moveData.m_pivotRotationValue.ToMatrix().GetAxisY();
	m_finalPosition = direction * -moveData.m_pivotDistanceValue + moveData.m_pivotPositionValue;
	m_finalRotation = moveData.m_pivotRotationValue;*/
}

void CCustomCameraSimplePositionController::GenerateDebugFragments( CRenderFrame* frame )
{
	m_collisionController->GenerateDebugFragments( frame );
}

void CCustomCameraSimplePositionController::SetColisionOriginOffset( const Vector& offset )
{	
	if( m_collisionController ) 
	{ 
		m_collisionController->SetColisionOriginOffset( offset ); 
	}	
	if( m_collisionController2 ) 
	{ 
		m_collisionController2->SetColisionOriginOffset( offset); 
	} 
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CTrailerCameraPositionController );

CTrailerCameraPositionController::CTrailerCameraPositionController()
	: m_pullCamera( false )
	, m_defaultPitch( -15.f )
	, m_defaultZOffset( 2.5f )
	, m_offsetSmoothTime( 0.5f )
	, m_offsetDamp( m_offsetSmoothTime )
	, m_prevPos( Vector::ZERO_3D_POINT )
	, m_collisionController( NULL )
	, m_defaultCollisionOriginOffset( Vector( 0.f, 0.f, 2.8f ) )
{
}

void CTrailerCameraPositionController::OnPostLoad()
{
	if( !m_collisionController )
	{
		m_collisionController = CreateObject< CCustomCameraOutdoorCollisionController >( this );
		m_collisionController->OnPostLoad();
		m_collisionController->SetColisionOriginOffset( m_defaultCollisionOriginOffset );
	}
}

void CTrailerCameraPositionController::Activate( SCameraMovementData& currData )
{
	CEntity* player = GGame->GetPlayerEntity();
	if( player )
	{
		currData.m_pivotPositionController.Get()->Activate( player->GetWorldPositionRef(), m_defaultZOffset );
		currData.m_pivotPositionValue = player->GetWorldPosition();
		currData.m_pivotPositionVelocity = Vector::ZEROS;
		currData.m_pivotRotationValue.Yaw = player->GetWorldRotation().Yaw;
		currData.m_pivotRotationValue.Pitch = m_defaultPitch;
		currData.m_pivotRotationVelocity = EulerAngles::ZEROS;
		currData.m_pivotDistanceValue = m_currentPreset.m_explorationDistance;
		currData.m_pivotDistanceVelocity = 0.f;
		currData.m_cameraLocalSpaceOffset = m_currentPreset.m_explorationOffset;
		currData.m_cameraLocalSpaceOffsetVel = Vector::ZEROS;
	}

	currData.m_pivotPositionController.Get()->Activate( currData.m_pivotPositionValue, currData.m_pivotPositionController.Get()->GetZOffset() );
	currData.m_pivotRotationController.Get()->Activate( currData.m_pivotRotationValue, 0 );
	currData.m_pivotDistanceController.Get()->Activate( currData.m_pivotDistanceValue );

	m_pullCamera = false;
}

void CTrailerCameraPositionController::Activate( SCameraMovementData& currData, const SCameraMovementData& prevData )
{
	currData.m_pivotPositionValue    = prevData.m_pivotPositionValue;
	currData.m_pivotPositionVelocity = prevData.m_pivotPositionVelocity;
	currData.m_pivotRotationValue    = prevData.m_pivotRotationValue;
	currData.m_pivotRotationVelocity = prevData.m_pivotRotationVelocity;
	currData.m_pivotDistanceValue    = prevData.m_pivotDistanceValue;
	currData.m_pivotDistanceVelocity = prevData.m_pivotDistanceVelocity;
	currData.m_cameraLocalSpaceOffset = prevData.m_cameraLocalSpaceOffset;
	currData.m_cameraLocalSpaceOffsetVel = prevData.m_cameraLocalSpaceOffsetVel;

	currData.m_pivotPositionController.Get()->Activate( prevData.m_pivotPositionValue, prevData.m_pivotPositionController.Get()->GetCurrentZOffset() );
	currData.m_pivotRotationController.Get()->Activate( prevData.m_pivotRotationValue, prevData.m_pivotRotationController.Get()->GetFlags() );
	currData.m_pivotDistanceController.Get()->Activate( prevData.m_pivotDistanceValue );

	m_pullCamera = false;
}

RED_DEFINE_STATIC_NAME( OnGameCameraTick )
RED_DEFINE_STATIC_NAME( OnGameCameraPostTick )
RED_DEFINE_STATIC_NAME( OnGameCameraExplorationRotCtrlChange )

void CTrailerCameraPositionController::PreUpdate( CCustomCamera& camera, Float timeDelta )
{
	SCameraMovementData& moveData = camera.GetMoveData();

	CEntity* player = GGame->GetPlayerEntity();
	if( !player )
	{
		RED_ASSERT( false, TXT( "This would cause a crash. Should this even be processed?" ) );
		return;
	}

	if( !CallGameCameraEvent( player, CNAME( OnGameCameraTick ), moveData, timeDelta ) )
	{
		moveData.m_pivotPositionController.Get()->SetZOffset( m_defaultZOffset );
		moveData.m_pivotDistanceController.Get()->SetDesiredDistance( m_currentPreset.m_explorationDistance );
		moveData.m_pivotPositionController.Get()->SetDesiredPosition( player->GetWorldPositionRef() );
		moveData.m_pivotRotationController.Get()->SetDesiredPitch( m_defaultPitch );

		m_offsetDamp.Update( moveData.m_cameraLocalSpaceOffset, moveData.m_cameraLocalSpaceOffsetVel, m_currentPreset.m_explorationOffset, timeDelta );

		m_pullCamera = !camera.IsManualControlledHor();
	}
	else
	{
		m_pullCamera = false;
	}

	if( CallGameCameraEvent( player, CNAME( OnGameCameraPostTick ), moveData, timeDelta ) )
	{
		m_pullCamera = false;
	}
}

void CTrailerCameraPositionController::Update( SCameraMovementData& moveData, Float timeDelta )
{
	moveData.m_pivotPositionController.Get()->Update( moveData.m_pivotPositionValue, moveData.m_pivotPositionVelocity, timeDelta );
	moveData.m_pivotRotationController.Get()->Update( moveData.m_pivotRotationValue, moveData.m_pivotRotationVelocity, timeDelta );
	moveData.m_pivotDistanceController.Get()->Update( moveData.m_pivotDistanceValue, moveData.m_pivotDistanceVelocity, timeDelta );

	//////////////////////////////////////////////////////////////////////////

	if( m_pullCamera )
	{
		const Vector2 direction2D = m_prevPos - moveData.m_pivotPositionValue;

		const Float yaw = RAD2DEG( -atan2f( -direction2D.X, -direction2D.Y ) );
		moveData.m_pivotRotationValue.Yaw = yaw;
	}

	moveData.m_cameraOffset = moveData.m_pivotRotationValue.TransformPoint( moveData.m_cameraLocalSpaceOffset );

	m_prevPos = moveData.m_pivotRotationValue.ToMatrix().GetAxisY() * -moveData.m_pivotDistanceValue + moveData.m_pivotPositionValue;

	m_collisionController->Update( moveData, timeDelta );

	m_finalPosition = m_collisionController->GetPosition();
	m_finalRotation = m_collisionController->GetRotation();
}

//////////////////////////////////////////////////////////////////////////
// CAMERA SCRIPTED CONTROLLERS
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedPivotPositionController );

void ICustomCameraScriptedPivotPositionController::Activate( const Vector& currentPosition, Float currentOffsetZ )
{
	CallFunction( this, CNAME( ControllerActivate ), currentOffsetZ );
}

void ICustomCameraScriptedPivotPositionController::Deactivate()
{
	CallFunction( this, CNAME( ControllerDeactivate ) );
}

void ICustomCameraScriptedPivotPositionController::Update( Vector& currentPosition, Vector& currentVelocity, Float timeDelta )
{
	CallFunctionRef2Val1( this, CNAME( ControllerUpdate ), currentPosition, currentVelocity, timeDelta );
}

void ICustomCameraScriptedPivotPositionController::SetDesiredPosition( const Vector& position, Float mult /*= 1.f */ )
{
	CallFunction( this, CNAME( ControllerSetDesiredPosition ), position, mult );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedPivotRotationController );

void ICustomCameraScriptedPivotRotationController::Activate( const EulerAngles& currentRotation, Uint32 flags )
{
	TBaseClass::Activate( currentRotation, flags );

	CallFunction( this, CNAME( ControllerActivate ), currentRotation );
}

void ICustomCameraScriptedPivotRotationController::Deactivate()
{
	CallFunction( this, CNAME( ControllerDeactivate ) );
}

void ICustomCameraScriptedPivotRotationController::Update( EulerAngles& currentRotation, EulerAngles& currentVelocity, Float timeDelta )
{
	CallFunctionRef2Val1( this, CNAME( ControllerUpdate ), currentRotation, currentVelocity, timeDelta );
}

void ICustomCameraScriptedPivotRotationController::SetDesiredYaw( Float yaw, Float mult /*= 1.f */ )
{
	CallFunction( this, CNAME( ControllerSetDesiredYaw ), yaw, mult );
}

void ICustomCameraScriptedPivotRotationController::SetDesiredPitch( Float pitch, Float mult /*= 1.f */ )
{
	CallFunction( this, CNAME( ControllerSetDesiredPitch ), pitch, mult );
}

void ICustomCameraScriptedPivotRotationController::RotateHorizontal( Bool right, Float mult /*= 1.f */ )
{
	CallFunction( this, CNAME( ControllerRotateHorizontal ), right, mult );
}

void ICustomCameraScriptedPivotRotationController::RotateVertical( Bool up, Float mult /*= 1.f */ )
{
	CallFunction( this, CNAME( ControllerRotateVertical ), up, mult );
}

void ICustomCameraScriptedPivotRotationController::StopRotating()
{
	CallFunction( this, CNAME( ControllerStopRotating ) );
}

EulerAngles ICustomCameraScriptedPivotRotationController::GetRotationDelta()
{
	EulerAngles result = EulerAngles::ZEROS;
	CallFunctionRet( this, CNAME( ControllerGetRotationDelta ), result );

	return result;
}

void ICustomCameraScriptedPivotRotationController::UpdateInput( Bool& movedHorizontal, Bool& movedVertical )
{
	CallFunctionRef( this, CNAME( ControllerUpdateInput ), movedHorizontal, movedVertical );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedPivotDistanceController );

void ICustomCameraScriptedPivotDistanceController::Activate( Float currentDistance )
{
	CallFunction( this, CNAME( ControllerActivate ), currentDistance );
}

void ICustomCameraScriptedPivotDistanceController::Deactivate()
{
	CallFunction( this, CNAME( ControllerDeactivate ) );
}

void ICustomCameraScriptedPivotDistanceController::Update( Float& currentDistance, Float& currentVelocity, Float timeDelta )
{
	CallFunctionRef2Val1( this, CNAME( ControllerUpdate ), currentDistance, currentVelocity, timeDelta );
}

void ICustomCameraScriptedPivotDistanceController::SetDesiredDistance( Float distance, Float mult /*= 1.f */ )
{
	CallFunction( this, CNAME( ControllerSetDesiredDistance ), distance, mult );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedPositionController );

void ICustomCameraScriptedPositionController::Update( SCameraMovementData& moveData, Float timeDelta )
{
	CallFunctionRef1Val1( this, CNAME( ControllerUpdate ), moveData, timeDelta );
}

Vector ICustomCameraScriptedPositionController::GetPosition()
{
	Vector result = Vector::ZERO_3D_POINT;
	CallFunctionRet( this, CNAME( ControllerGetPosition ), result );

	return result;
}

EulerAngles ICustomCameraScriptedPositionController::GetRotation()
{
	EulerAngles result = EulerAngles::ZEROS;
	CallFunctionRet( this, CNAME( ControllerGetRotation ), result );

	return result;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedCurveSetPivotPositionController );

void ICustomCameraScriptedCurveSetPivotPositionController::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("curveSet") )
	{
		m_curveNames.Resize( m_curveSet.Size() );
	}
	else if ( property->GetName() == TXT("curveNames") )
	{
		m_curveSet.Resize( m_curveNames.Size() );
	}

	const Uint32 size = m_curveSet.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( !m_curveSet[ i ] )
		{
			m_curveSet[ i ] = CreateObject< CCurve >( this );
			m_curveSet[ i ]->GetCurveData().AddPoint( 0.f, 0.f );
			m_curveSet[ i ]->GetCurveData().AddPoint( 1.f, 1.f );
		}
	}
}

void ICustomCameraScriptedCurveSetPivotPositionController::funcFindCurve( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	CCurve* curveH = NULL;

	ASSERT( m_curveSet.Size() == m_curveNames.Size() );

	if ( name != CName::NONE )
	{
		const Uint32 size = m_curveSet.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( m_curveNames[ i ] == name )
			{
				curveH = m_curveSet[ i ];
			}
		}
	}

	RETURN_OBJECT( curveH );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedCurveSetPivotRotationController );

void ICustomCameraScriptedCurveSetPivotRotationController::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("curveSet") )
	{
		m_curveNames.Resize( m_curveSet.Size() );
	}
	else if ( property->GetName() == TXT("curveNames") )
	{
		m_curveSet.Resize( m_curveNames.Size() );
	}

	const Uint32 size = m_curveSet.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( !m_curveSet[ i ] )
		{
			m_curveSet[ i ] = CreateObject< CCurve >( this );
			m_curveSet[ i ]->GetCurveData().AddPoint( 0.f, 0.f );
			m_curveSet[ i ]->GetCurveData().AddPoint( 1.f, 1.f );
		}
	}
}

void ICustomCameraScriptedCurveSetPivotRotationController::funcFindCurve( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	CCurve* curveH = NULL;

	ASSERT( m_curveSet.Size() == m_curveNames.Size() );

	if ( name != CName::NONE )
	{
		const Uint32 size = m_curveSet.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( m_curveNames[ i ] == name )
			{
				curveH = m_curveSet[ i ];
			}
		}
	}

	RETURN_OBJECT( curveH );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedCurveSetPivotDistanceController );

void ICustomCameraScriptedCurveSetPivotDistanceController::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("curveSet") )
	{
		m_curveNames.Resize( m_curveSet.Size() );
	}
	else if ( property->GetName() == TXT("curveNames") )
	{
		m_curveSet.Resize( m_curveNames.Size() );
	}

	const Uint32 size = m_curveSet.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( !m_curveSet[ i ] )
		{
			m_curveSet[ i ] = CreateObject< CCurve >( this );
			m_curveSet[ i ]->GetCurveData().AddPoint( 0.f, 0.f );
			m_curveSet[ i ]->GetCurveData().AddPoint( 1.f, 1.f );
		}
	}
}

void ICustomCameraScriptedCurveSetPivotDistanceController::funcFindCurve( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	CCurve* curveH = NULL;

	ASSERT( m_curveSet.Size() == m_curveNames.Size() );

	if ( name != CName::NONE )
	{
		const Uint32 size = m_curveSet.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( m_curveNames[ i ] == name )
			{
				curveH = m_curveSet[ i ];
			}
		}
	}

	RETURN_OBJECT( curveH );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( ICustomCameraScriptedCurveSetPositionController );

void ICustomCameraScriptedCurveSetPositionController::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("curveSet") )
	{
		m_curveNames.Resize( m_curveSet.Size() );
	}
	else if ( property->GetName() == TXT("curveNames") )
	{
		m_curveSet.Resize( m_curveNames.Size() );
	}

	const Uint32 size = m_curveSet.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( !m_curveSet[ i ] )
		{
			m_curveSet[ i ] = CreateObject< CCurve >( this );
			m_curveSet[ i ]->GetCurveData().AddPoint( 0.f, 0.f );
			m_curveSet[ i ]->GetCurveData().AddPoint( 1.f, 1.f );
		}
	}
}

void ICustomCameraScriptedCurveSetPositionController::funcFindCurve( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	CCurve* curveH = NULL;

	ASSERT( m_curveSet.Size() == m_curveNames.Size() );

	if ( name != CName::NONE )
	{
		const Uint32 size = m_curveSet.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( m_curveNames[ i ] == name )
			{
				curveH = m_curveSet[ i ];
			}
		}
	}

	RETURN_OBJECT( curveH );
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( SCameraDistanceInfo );
IMPLEMENT_ENGINE_CLASS( CCombatCameraPositionController );

#define RUN_THRESHOLD 0.9f

CCombatCameraPositionController::CCombatCameraPositionController()
	: m_finalPosition( Vector::ZERO_3D_POINT )
	, m_finalRotation( EulerAngles::ZEROS )
	, m_isResetScheduled( true )
	, m_collisionController( NULL )
	, m_collisionController2( NULL )
	, m_defaultCollisionOriginOffset( Vector( 0.f, 0.f, 1.5f ) )
	, m_baseSmoothTime( 0.5f )
	, m_smoothTime( m_baseSmoothTime )
	, m_offsetSmoothTime( 2.f )
	, m_offsetDamp( m_offsetSmoothTime )
	, m_defaultCameraAngle( -18.f )
	, m_defaultCameraZOffset( 1.5f )
	, m_flipCameraAngle( -18.f )
	, m_followRotation( NULL )
	, m_followRotationSprint( NULL )
	, m_followRotationFlip( NULL )
	, m_slopeCameraAngleChange( NULL )
	, m_slopeAngleCameraSpaceMultiplier( NULL )
	, m_slopeResetTimeout( NULL )
	, m_cameraPivotDampMult( NULL )
	, m_pivotMultDamp( m_smoothTime )
	, m_slopeTimer( 0.f )
	, m_combatPivotDampMult( 10.f )
	, m_bigMonsterHeightThreshold( 2.f )
	, m_180FlipThreshold( 160.f )
	, m_flipTriggered( false )
	, m_explorationRotationCtrlName( CName::NONE )
	, m_combatRotationCtrlName( CName::NONE )
	, m_combatPitch( -26.f )
	, m_bigMonsterCountMultiplier( 7 )
	, m_1v1Pitch( NULL )
	, m_1v1AdditivePitch( NULL )
	, m_1v1BigMonsterPitch( NULL )
	, m_1v1BMAdditivePitch( NULL )
	, m_monsterSizeAdditiveOffset( NULL )
	, m_monsterSizeAdditivePitch( NULL )
	, m_1v1Distance( NULL )
	, m_1v1SignificanceAddDistance( NULL )
	, m_1v1ZOffset( NULL )
	, m_1v1BigMonsterZOffset( NULL )
	, m_1v1PivotMultiplier( 0.5f )
	, m_1v1KeepAngle( 45.f )
	, m_1v1OffScreenMult( 1.f )
	, m_oneOnOneCtrlName( CName::NONE )
	, m_screenSpaceXRatio( 0.9f )
	, m_screenSpaceYRatio( 0.8f )
	, m_ssPivotCorrSmooth( 0.2f )
	, m_ssPivotCorrActualSmooth( m_ssPivotCorrSmooth )
	, m_ssPivotCorrVelocity( Vector::ZERO_3D_POINT )
	, m_ssPivotCorrection( m_ssPivotCorrActualSmooth )
	, m_ssDistCorrSmooth( 0.2f )
	, m_ssDistCorrActualSmooth( m_ssDistCorrSmooth )
	, m_ssDistCorrVelocity( 0.f )
	, m_ssDistCorrection( m_ssDistCorrActualSmooth )
	, m_ssCorrectionXTreshold( 0.9f )
	, m_ssCorrectionYTreshold( -0.8f )
	, m_useExplorationCamInSprint( true )
{
	m_enableAutoCollisionAvoidance = false;
	m_enableScreenSpaceCorrections = true;

	m_pivotMultDamp.Force( 1.f );

	ResetHistory();
}

void CCombatCameraPositionController::OnPostLoad()
{
	if( !m_followRotation )
	{
		m_followRotation = CreateObject< CCurve >( this );
		m_followRotation->GetCurveData().AddPoint( 0.f, 0.f );
		m_followRotation->GetCurveData().AddPoint( 180.f, 0.f );
	}

	if( !m_followRotationSprint )
	{
		m_followRotationSprint = CreateObject< CCurve >( this );
		m_followRotationSprint->GetCurveData() = m_followRotation->GetCurveData();
	}

	if( !m_followRotationFlip )
	{
		m_followRotationFlip = CreateObject< CCurve >( this );
		m_followRotationFlip->GetCurveData().AddPoint( 0.f, 0.f );
		m_followRotationFlip->GetCurveData().AddPoint( 180.f, 0.f );
	}

	if( !m_slopeCameraAngleChange )
	{
		m_slopeCameraAngleChange = CreateObject< CCurve >( this );
		m_slopeCameraAngleChange->GetCurveData().AddPoint( -90.f, 60.f );
		m_slopeCameraAngleChange->GetCurveData().AddPoint( 90.f, -60.f );
	}

	if( !m_slopeAngleCameraSpaceMultiplier )
	{
		m_slopeAngleCameraSpaceMultiplier = CreateObject< CCurve >( this );
		m_slopeAngleCameraSpaceMultiplier->GetCurveData().AddPoint( 0, 1.f );
		m_slopeAngleCameraSpaceMultiplier->GetCurveData().AddPoint( 180.f, -1.f );
	}

	if( !m_slopeResetTimeout )
	{
		m_slopeResetTimeout = CreateObject< CCurve >( this );
		m_slopeResetTimeout->GetCurveData().AddPoint( -90.f, 3.f );
		m_slopeResetTimeout->GetCurveData().AddPoint( 90.f, 3.f );
	}

	if( !m_cameraPivotDampMult )
	{
		m_cameraPivotDampMult = CreateObject< CCurve >( this );
		m_cameraPivotDampMult->GetCurveData().AddPoint( 0.f, 1.f );
		m_cameraPivotDampMult->GetCurveData().AddPoint( 90.f, 1.f );
		m_cameraPivotDampMult->GetCurveData().AddPoint( 180.f, 0.01f );
	}

	if( !m_1v1Pitch )
	{
		m_1v1Pitch = CreateObject< CCurve >( this );
		m_1v1Pitch->GetCurveData().AddPoint( -90.f, 60.f );
		m_1v1Pitch->GetCurveData().AddPoint( 90.f, -60.f );
	}

	if( !m_1v1AdditivePitch )
	{
		m_1v1AdditivePitch = CreateObject< CCurve >( this );
		m_1v1AdditivePitch->GetCurveData().AddPoint( 0.f, 0.f );
		m_1v1AdditivePitch->GetCurveData().AddPoint( 10.f, 0.f );
	}

	if( !m_1v1BigMonsterPitch )
	{
		m_1v1BigMonsterPitch = CreateObject< CCurve >( this );
		m_1v1BigMonsterPitch->GetCurveData().AddPoint( -90.f, 60.f );
		m_1v1BigMonsterPitch->GetCurveData().AddPoint( 90.f, -60.f );
	}

	if( !m_1v1BMAdditivePitch )
	{
		m_1v1BMAdditivePitch = CreateObject< CCurve >( this );
		m_1v1BMAdditivePitch->GetCurveData().AddPoint( 0.f, 0.f );
		m_1v1BMAdditivePitch->GetCurveData().AddPoint( 10.f, 0.f );
	}

	if( !m_monsterSizeAdditiveOffset )
	{
		m_monsterSizeAdditiveOffset = CreateObject< CCurve >( this );
		m_monsterSizeAdditiveOffset->GetCurveData().AddPoint( 0.f, 0.f );
		m_monsterSizeAdditiveOffset->GetCurveData().AddPoint( 10.f, 0.f );
	}

	if( !m_monsterSizeAdditivePitch )
	{
		m_monsterSizeAdditivePitch = CreateObject< CCurve >( this );
		m_monsterSizeAdditivePitch->GetCurveData().AddPoint( 0.f, 0.f );
		m_monsterSizeAdditivePitch->GetCurveData().AddPoint( 10.f, 0.f );
	}

	if( !m_1v1Distance )
	{
		m_1v1Distance = CreateObject< CCurve >( this );
		m_1v1Distance->GetCurveData().AddPoint( 0.f, 1.f );
		m_1v1Distance->GetCurveData().AddPoint( 10.f, 4.f );
	}

	if( !m_1v1SignificanceAddDistance )
	{
		m_1v1SignificanceAddDistance = CreateObject< CCurve >( this );
		m_1v1SignificanceAddDistance->GetCurveData().AddPoint( 0.f, 0.f );
		m_1v1SignificanceAddDistance->GetCurveData().AddPoint( 10.f, 0.f );
	}

	if( !m_1v1ZOffset )
	{
		m_1v1ZOffset = CreateObject< CCurve >( this );
		m_1v1ZOffset->GetCurveData().AddPoint( -90.f, 1.5f );
		m_1v1ZOffset->GetCurveData().AddPoint( 90.f, 1.5f );
	}

	if( !m_1v1BigMonsterZOffset )
	{
		m_1v1BigMonsterZOffset = CreateObject< CCurve >( this );
		m_1v1BigMonsterZOffset->GetCurveData().AddPoint( -90.f, 1.5f );
		m_1v1BigMonsterZOffset->GetCurveData().AddPoint( 90.f, 1.5f );
	}

	if( !m_collisionController )
	{
		m_collisionController = CreateObject< CCustomCameraAutoAvoidanceCollisionController >( this );
		m_collisionController->OnPostLoad();
	}

	if( !m_collisionController2 )
	{
		m_collisionController2 = CreateObject< CCustomCameraOutdoorCollisionController >( this );
		m_collisionController2->OnPostLoad();
		m_collisionController2->SetColisionOriginOffset( m_defaultCollisionOriginOffset );
	}

	if( m_combatEnemiesToDistanceMap.Empty() )
	{
		SCameraDistanceInfo info;
		info.m_minDistance = 4.f;
		info.m_distanceRange = 2.f;
		info.m_enemiesMaxDistanceToCamera = 10.f;
		info.m_enemiesMaxDistanceToPlayer = 5.f;
		info.m_standardDeviationRelevance = 0.5f;
		info.m_cameraZOffset = 1.f;
		info.m_cameraZOffsetRange = -0.5f;
		m_combatEnemiesToDistanceMap.PushBack( info );
	}
}

void CCombatCameraPositionController::OnBeforeStartBlendFrom( SCameraMovementData& out, const ICamera::Data& cameraData )
{
	CEntity* player = GGame->GetPlayerEntity();
	const Vector& playerPosition = player->GetRootAnimatedComponent()->GetThisFrameTempPositionWSRef();

	out.m_pivotPositionValue = playerPosition;
	out.m_pivotPositionValue.Z += m_defaultCameraZOffset;
	out.m_pivotPositionVelocity = Vector::ZEROS;

	out.m_pivotRotationValue = cameraData.m_rotation;
	out.m_pivotRotationValue.Pitch = m_defaultCameraAngle;
	out.m_pivotRotationValue.Roll = 0.0f;
	out.m_pivotRotationVelocity = EulerAngles::ZEROS;

	out.m_pivotDistanceValue = cameraData.m_position.DistanceTo( out.m_pivotPositionValue );
	out.m_pivotDistanceVelocity = 0.0f;

	out.m_cameraLocalSpaceOffset = m_currentPreset.m_explorationOffset;
	out.m_cameraLocalSpaceOffsetVel = Vector::ZEROS;
	out.m_cameraOffset = Vector::ZEROS;
}

void CCombatCameraPositionController::Activate( SCameraMovementData& currData )
{
	m_isResetScheduled = true;
}

void CCombatCameraPositionController::Activate( SCameraMovementData& currData, const SCameraMovementData& prevData )
{
	currData.m_pivotPositionValue    = prevData.m_pivotPositionValue;
	currData.m_pivotPositionVelocity = prevData.m_pivotPositionVelocity;
	currData.m_pivotRotationValue    = prevData.m_pivotRotationValue;
	currData.m_pivotRotationVelocity = prevData.m_pivotRotationVelocity;
	currData.m_pivotDistanceValue    = prevData.m_pivotDistanceValue;
	currData.m_pivotDistanceVelocity = prevData.m_pivotDistanceVelocity;
	currData.m_cameraLocalSpaceOffset = prevData.m_cameraLocalSpaceOffset;
	currData.m_cameraLocalSpaceOffsetVel = prevData.m_cameraLocalSpaceOffsetVel;

	currData.m_pivotPositionController.Get()->Activate( prevData.m_pivotPositionValue, prevData.m_pivotPositionController.Get()->GetCurrentZOffset() );
	currData.m_pivotRotationController.Get()->Activate( prevData.m_pivotRotationValue, prevData.m_pivotRotationController.Get()->GetFlags() );
	currData.m_pivotDistanceController.Get()->Activate( prevData.m_pivotDistanceValue );
}

void CCombatCameraPositionController::ResetColliisons() 
{ 
	m_collisionController2->Reset(); 
}


void CCombatCameraPositionController::ResetHistory()
{
	Red::System::MemoryZero( m_slopeHistory, sizeof( Float ) * SlopeHistorySize );

	m_currHistory = 0;
}

Float CCombatCameraPositionController::CalcNewSlopeValue( Float currentSlope )
{
	ASSERT( m_currHistory >= 0 && m_currHistory < SlopeHistorySize );

	m_slopeHistory[ m_currHistory ] = currentSlope;

	Float sum = 0.f;
	for ( Uint32 i = 0; i < SlopeHistorySize; ++i )
	{
		sum += m_slopeHistory[ i ];
	}

	// Go to next slot in array
	m_currHistory = ++m_currHistory % SlopeHistorySize;
	
	return sum / (Float)SlopeHistorySize;
}

void CCombatCameraPositionController::SetColisionOriginOffset( const Vector& offset )
{	
	if( m_collisionController ) 
	{ 
		m_collisionController->SetColisionOriginOffset( offset ); 
	}	
	if( m_collisionController2 ) 
	{ 
		m_collisionController2->SetColisionOriginOffset( offset); 
	} 
}

void CCombatCameraPositionController::PreUpdate( CCustomCamera& camera, Float timeDelta )
{
	CR4Player* player = SafeCast<CR4Player>( GGame->GetPlayerEntity() );

	if ( player == nullptr )
		return;

	SCameraMovementData& moveData = camera.GetMoveData();	
	const Vector& playerPosition = player->GetWorldPositionRef();
	static Vector prevPlayerPos = playerPosition;

	if ( m_isResetScheduled )
	{
		moveData.m_pivotPositionController.Get()->Activate( playerPosition, m_defaultCameraZOffset );
		moveData.m_pivotPositionValue = playerPosition;
		moveData.m_pivotPositionValue.Z += m_defaultCameraZOffset;
		moveData.m_pivotPositionVelocity = Vector::ZEROS;
		moveData.m_pivotRotationValue.Yaw = player->GetWorldRotation().Yaw;
		moveData.m_pivotRotationValue.Pitch = m_defaultCameraAngle;
		moveData.m_pivotRotationVelocity = EulerAngles::ZEROS;
		moveData.m_pivotDistanceValue = m_currentPreset.m_explorationDistance;
		moveData.m_pivotDistanceVelocity = 0.f;
		moveData.m_cameraLocalSpaceOffset = m_currentPreset.m_explorationOffset;
		moveData.m_cameraLocalSpaceOffsetVel = Vector::ZEROS;
		prevPlayerPos = playerPosition;
		m_isResetScheduled = false;
	}

	if ( CallGameCameraEvent( player, CNAME( OnGameCameraTick ), moveData, timeDelta ) )
	{
		m_pivotMultDamp.Update( 1.f, timeDelta );
	}
	else 
	{
		const CActor* moveTarget = player->GetScriptMoveTarget();
		const CAnimatedComponent* ac = player->GetRootAnimatedComponent();
		const Float playerSpeed = ( ac && ac->GetBehaviorStack() ) ? ac->GetRelativeMoveSpeed() /*ac->GetBehaviorStack()->GetBehaviorFloatVariable( CNAME( playerSpeed ) )*/ : 0.f;
		const Bool isSprinting = playerSpeed > 1.1f;
		
		if( !player->IsInCombat() || !moveTarget || ( isSprinting && m_useExplorationCamInSprint && !player->IsLockedToTarget() ) )
		{
			const Vector playerVelocity = playerPosition - prevPlayerPos;
			PreUpdateNonCombat( player, timeDelta, moveData, camera, playerVelocity, playerSpeed, isSprinting );
		}
		else
		{
			PreUpdateCombat( player, timeDelta, moveData, camera, moveTarget );
		}
	}

	CallFunctionRef1Val1( player, CNAME( OnGameCameraPostTick ), moveData, timeDelta );
	prevPlayerPos = playerPosition;
}

void CCombatCameraPositionController::PreUpdateNonCombat( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, const Vector& playerVelocity, Float playerSpeed, Bool isSprinting )
{
	const Vector& playerPosition = player->GetWorldPositionRef();

	if ( player->CallEvent( CNAME( OnGameCameraExplorationRotCtrlChange )/*, camera*/ ) != CR_EventSucceeded )
	{
		camera.ChangePivotRotationController( m_explorationRotationCtrlName );
	}

	if ( (MAbs(playerVelocity.X) > 0.00001f || MAbs(playerVelocity.Y) > 0.00001f) && timeDelta > 0.f )
	{
		m_slopeTimer = 0.f;

		const Float pitch = RAD2DEG( atan2f( -playerVelocity.Z, MSqrt( playerVelocity.X*playerVelocity.X + playerVelocity.Y*playerVelocity.Y )) );
		const Float yaw = RAD2DEG( -atan2f( playerVelocity.X, playerVelocity.Y ) );
		const Float playerCameraSpaceHeading = MAbs( EulerAngles::AngleDistance( yaw, moveData.m_pivotRotationValue.Yaw ) );

		if( !m_flipTriggered && isSprinting )
		{
			if( playerCameraSpaceHeading > m_180FlipThreshold )
			{
				m_flipTriggered = true;
			}
		}
		else if( playerCameraSpaceHeading < 90.f )
		{
			m_flipTriggered = false;
		}

		const Float yawMult = ( m_flipTriggered ? 
			m_followRotationFlip->GetFloatValue( playerCameraSpaceHeading ) :
			isSprinting ? m_followRotationSprint->GetFloatValue( playerCameraSpaceHeading ) : m_followRotation->GetFloatValue( playerCameraSpaceHeading ) )
			* ( playerSpeed + 1.f );
		if ( yawMult > 0.f )
		{
			moveData.m_pivotRotationController.Get()->SetDesiredYaw( yaw, yawMult );
		}
		else if ( !moveData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal ) )
		{
			moveData.m_pivotRotationController.Get()->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_HorizontalRotation );
		}

		const Float currentSlope = CalcNewSlopeValue( pitch );

		const Float desiredPitch = m_flipTriggered ? m_flipCameraAngle :
			(m_defaultCameraAngle + m_slopeCameraAngleChange->GetFloatValue( currentSlope )
			* m_slopeAngleCameraSpaceMultiplier->GetFloatValue( playerCameraSpaceHeading ));
		moveData.m_pivotRotationController.Get()->SetDesiredPitch( desiredPitch );

		m_smoothTime = m_baseSmoothTime / m_pivotMultDamp.GetValue();
		m_pivotMultDamp.Update( m_cameraPivotDampMult->GetFloatValue( playerCameraSpaceHeading ), timeDelta );
#ifndef RED_FINAL_BUILD
		m_dbgCurrentSlope = currentSlope;
		m_dbgPlayerCamSpaceHeading = playerCameraSpaceHeading;
#endif
	}
	else
	{
		m_smoothTime = m_baseSmoothTime;
		m_pivotMultDamp.Update( 1.f, timeDelta );

		if ( m_slopeTimer < 0.f || m_slopeTimer > m_slopeResetTimeout->GetFloatValue( moveData.m_pivotRotationValue.Pitch ) )
		{
			// Once the timer hit, set it to negative just in case timeout changes for new Pitch
			m_slopeTimer = -1.f;

			//const Float currentSlope = CalcNewSlopeValue( 0.f );
			//const Float desiredPitch = m_defaultCameraAngle + m_slopeCameraAngleChange->GetFloatValue( currentSlope );
			//moveData.m_pivotRotationController.Get()->SetDesiredPitch( desiredPitch );
#ifndef RED_FINAL_BUILD
			//m_dbgCurrentSlope = currentSlope;
#endif
		}
		else
		{
			m_slopeTimer += timeDelta;

			if( !moveData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal ) )
			{
				moveData.m_pivotRotationController.Get()->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_HorizontalRotation );
			}
		}
	}

	m_offsetDamp.Update( moveData.m_cameraLocalSpaceOffset, moveData.m_cameraLocalSpaceOffsetVel, m_currentPreset.m_explorationOffset, timeDelta );

	moveData.m_pivotPositionController.Get()->SetZOffset( m_defaultCameraZOffset );
	moveData.m_pivotPositionController.Get()->SetDesiredPosition( playerPosition, m_pivotMultDamp.GetValue() );
	moveData.m_pivotDistanceController.Get()->SetDesiredDistance( m_currentPreset.m_explorationDistance );
}

void CCombatCameraPositionController::PreUpdateCombat( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, const CActor* moveTarget )
{
	m_offsetDamp.Update( moveData.m_cameraLocalSpaceOffset, moveData.m_cameraLocalSpaceOffsetVel, Vector::ZEROS, timeDelta );
	camera.ChangePivotRotationController( m_combatRotationCtrlName );

	TDynArray< THandle<CActor> > enemies;
	GetHostileEnemies( player, enemies );

	if ( player->IsLockedToTarget() )
	{
		PreUpdateLockedToTarget( player, timeDelta, moveData, camera, moveTarget );
	}
	else if ( enemies.Empty() )
	{
		PreUpdateNoEnemies( player, timeDelta, moveData );
	}
	else
	{
		PreUpdateEnemies( player, timeDelta, moveData, camera, enemies );
	}
}

void CCombatCameraPositionController::GetHostileEnemies( CR4Player* player, TDynArray< THandle<CActor> >& hostileEnemies )
{
	TDynArray< THandle<CActor> > visibleEnemies;
	player->GetEnemyData().GetVisibleEnemies( visibleEnemies );	

	for ( const auto& enemy : visibleEnemies )
	{
		if ( enemy->GetAttitude( player ) == AIA_Hostile )
		{
			hostileEnemies.PushBack( enemy );
		}
		else
		{
			EAIAttitude groupAttitude = CAttitudes::GetDefaultAttitude();		
			GCommonGame->GetSystem< CAttitudeManager >()->GetGlobalAttitude( player->GetBaseAttitudeGroup(), enemy->GetBaseAttitudeGroup(), groupAttitude );
			if ( groupAttitude == AIA_Hostile )
			{
				hostileEnemies.PushBack( enemy );
			}
		}
	}
}

void CCombatCameraPositionController::PreUpdateLockedToTarget( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, const CActor* moveTarget )
{
	camera.ChangePivotRotationController( m_oneOnOneCtrlName );

	const CActor* target = player->GetScriptTarget() ? player->GetScriptTarget() : moveTarget;

	if ( target )
	{
		m_offsetDamp.Update( moveData.m_cameraLocalSpaceOffset, moveData.m_cameraLocalSpaceOffsetVel, Vector::ZEROS, timeDelta );

		Vector targetPosition = target->GetWorldPosition();
		const CCurve* pitchCurve;
		const CCurve* addPitchCurve;
		const CCurve* offsetZCurve;

		const CCharacterControllerParam* ctlrParam = target ? target->GetEntityTemplate()->FindGameplayParamT< CCharacterControllerParam >( true )
			: moveTarget->GetEntityTemplate()->FindGameplayParamT< CCharacterControllerParam >( true );

		if ( !ctlrParam )
		{
			ctlrParam = CCharacterControllerParam::GetStaticClass()->GetDefaultObject< CCharacterControllerParam >();
		}

		const Float capsuleHeight = ctlrParam->m_height;
		const Float significance = ctlrParam->m_significance;

		if ( capsuleHeight > m_bigMonsterHeightThreshold )
		{
			targetPosition.Z += capsuleHeight;
			pitchCurve = m_1v1BigMonsterPitch;
			addPitchCurve = m_1v1BMAdditivePitch;
			offsetZCurve = m_1v1BigMonsterZOffset;
		}
		else
		{
			pitchCurve = m_1v1Pitch;
			addPitchCurve = m_1v1AdditivePitch;
			offsetZCurve = m_1v1ZOffset;
		}

		const Vector& playerPosition = player->GetWorldPositionRef();
		const Vector direction = targetPosition - playerPosition;
		const Float dist = direction.Mag3();
		const Float yaw = RAD2DEG( -atan2f( direction.X, direction.Y ) );

		const Float angleDist = EulerAngles::AngleDistance( yaw, moveData.m_pivotRotationValue.Yaw );
		if ( MAbs( angleDist ) > m_1v1KeepAngle && timeDelta > 0.f )
		{
			Float mult = 1.f;

			Float screenX, screenY;
			if( GGame->GetActiveWorld()->GetCameraDirector()->WorldVectorToViewRatio( targetPosition, screenX, screenY ) )
			{
				const Float absScreenY = MAbs( screenY );
				if( absScreenY > m_screenSpaceYRatio )
				{
					mult += (absScreenY - m_screenSpaceYRatio) * m_1v1OffScreenMult;

					LOG_CAMERA( RED_LOG_CHANNEL( CustomCamera ), TXT("Off Screen Mult Y = %f"), (absScreenY - m_screenSpaceYRatio) * m_1v1OffScreenMult );
				}

				const Float absScreenX = MAbs( screenX );
				if( absScreenX > m_screenSpaceXRatio )
				{
					mult += (absScreenX - m_screenSpaceXRatio) * m_1v1OffScreenMult;

					LOG_CAMERA( RED_LOG_CHANNEL( CustomCamera ), TXT("Off Screen Mult X = %f"), (absScreenX - m_screenSpaceXRatio) * m_1v1OffScreenMult );
				}
			}
			else
			{
				mult = 1.f + m_1v1OffScreenMult;
			}

			LOG_CAMERA( RED_LOG_CHANNEL( CustomCamera ), TXT("Off Screen Mult Total = %f"), mult );

			mult = Min( mult, 5.f );

			moveData.m_pivotRotationController.Get()->SetDesiredYaw( yaw + (angleDist < 0 ? -m_1v1KeepAngle : m_1v1KeepAngle), mult );
		}

		m_pivotMultDamp.Force( m_1v1PivotMultiplier );

		moveData.m_pivotPositionController.Get()->SetDesiredPosition( (playerPosition + targetPosition) * 0.5f, m_pivotMultDamp.GetValue() );

		moveData.m_pivotDistanceController.Get()->SetDesiredDistance( m_1v1Distance->GetFloatValue( capsuleHeight )
			+ m_1v1SignificanceAddDistance->GetFloatValue( significance ) );

		const Float pitchToEnemy = RAD2DEG( atan2f( -direction.Z, MSqrt( direction.X*direction.X + direction.Y*direction.Y ) ) );
		moveData.m_pivotRotationController.Get()->SetDesiredPitch( pitchCurve->GetFloatValue( pitchToEnemy ) + addPitchCurve->GetFloatValue( dist ) );
		moveData.m_pivotPositionController.Get()->SetZOffset( offsetZCurve->GetFloatValue( pitchToEnemy ) );
#ifndef RED_FINAL_BUILD
		m_dbgCurrentSlope = pitchToEnemy;
#endif
	}
	else
	{
		ASSERT( false, TXT("How come in one on one with no enemy!!??") );
	}
}

void CCombatCameraPositionController::PreUpdateNoEnemies( CR4Player* player, Float timeDelta, SCameraMovementData& moveData )
{
	m_smoothTime = m_baseSmoothTime;
	m_pivotMultDamp.Update( 1.f, timeDelta );

	const Vector& playerPosition = player->GetWorldPositionRef();
	moveData.m_pivotPositionController.Get()->SetDesiredPosition( playerPosition, m_pivotMultDamp.GetValue() );
	moveData.m_pivotDistanceController.Get()->SetDesiredDistance( m_currentPreset.m_explorationDistance );
	moveData.m_pivotRotationController.Get()->SetDesiredPitch( m_defaultCameraAngle );
	moveData.m_pivotPositionController.Get()->SetZOffset( m_defaultCameraZOffset );
}

void CCombatCameraPositionController::PreUpdateEnemies( CR4Player* player, Float timeDelta, SCameraMovementData& moveData, CCustomCamera& camera, TDynArray< THandle<CActor> >& enemies )
{
	const CCharacterControllerParam* defaultParam = CCharacterControllerParam::GetStaticClass()->GetDefaultObject< CCharacterControllerParam >();

	Float mostSignificant = defaultParam->m_significance;
	Float totalSignificance = 0.f;
	Float mostSignificantHeight = defaultParam->m_height;

	Vector avgPosition = Vector::ZEROS;

	struct CachedStuff
	{
		Float	weight;
		Vector	position;
	};

	const Uint32 enemiesCount = enemies.Size();
	TDynArray< CachedStuff > cachedarray;
	cachedarray.Resize( enemiesCount );

	for ( Uint32 i = 0; i < enemiesCount; ++i )
	{
		Float currentSignificance = defaultParam->m_significance;

		const CActor* current = enemies[ i ].Get();
		if ( current == nullptr )
			continue;

		const Vector currentPosition = current->GetWorldPosition();
		const CEntityTemplate* entityTemplate = current->GetEntityTemplate();
		const CCharacterControllerParam* ctlrParam = entityTemplate ? entityTemplate->FindGameplayParamT< CCharacterControllerParam >( true ) : nullptr;

		if ( ctlrParam )
		{
			currentSignificance = ctlrParam->m_significance;
			if ( currentSignificance > mostSignificant )
			{
				mostSignificant = currentSignificance;
				mostSignificantHeight = ctlrParam->m_height;
			}
		}
		totalSignificance += currentSignificance;

		avgPosition += currentPosition * currentSignificance;

		CachedStuff& cachedInfo = cachedarray[ i ];
		cachedInfo.weight = currentSignificance;
		cachedInfo.position = currentPosition;
	}

	avgPosition /= totalSignificance;

	// calculate the variance
	Float numerator = 0.f;
	for ( Uint32 i = 0; i < enemiesCount; ++i )
	{
		CachedStuff& cachedInfo = cachedarray[ i ];
		numerator += Vector::Sub3( cachedInfo.position, avgPosition ).SquareMag3() * cachedInfo.weight;
	}

	const Float standardDeviation = MSqrt( numerator / totalSignificance );
	Vector camPosNoOffset = camera.GetWorldPosition();
	camPosNoOffset.Z -= moveData.m_pivotPositionController->GetCurrentZOffset();
	const Float distanceToCamera = Max( (avgPosition - camPosNoOffset).Mag3() - 1.f, 0.f );

	const Vector& playerPosition = player->GetWorldPositionRef();
	const Vector tempDir = avgPosition - playerPosition;
	const Float fromPlayerDist = Max( tempDir.Mag2(), NumericLimits<Float>::Epsilon() );
	const Float fromPlayerDistSqrt = MSqrt(fromPlayerDist);
	const Vector fromPlayerDir2D = tempDir / fromPlayerDist;
	const Float magicDot = Vector::Dot2( fromPlayerDir2D, camera.GetWorldForward().Normalized2() );
	const Float magicNumberVertical = Max( 1.f - (magicDot + 1.f), 0.f );
	const Float magicNumberHorizontal = 1.f - Min( MAbs(magicDot), 0.4f ) / 0.4f;
	Float magicNumber2 = 1.f;
	if ( magicDot > 0.05f )
	{
		magicNumber2 = 1.f / (MSqrt(magicDot) * fromPlayerDistSqrt);
	}

	const Uint32 numEnemies = (Uint32)MRound( totalSignificance );
	const Uint32 distIndex = Min( numEnemies, m_combatEnemiesToDistanceMap.Size() ) - 1;
	const SCameraDistanceInfo& distInfo = m_combatEnemiesToDistanceMap[distIndex];

	const Float factor1 = (1.f - Min( distanceToCamera / distInfo.m_enemiesMaxDistanceToCamera, 1.f )) * distInfo.m_distanceRange * magicNumberVertical
		+ Min( fromPlayerDist / distInfo.m_enemiesMaxDistanceToPlayer, 1.f ) * distInfo.m_distanceRange * magicNumberHorizontal;
	const Float factor2 = standardDeviation * distInfo.m_standardDeviationRelevance * magicNumber2;
	moveData.m_pivotDistanceController.Get()->SetDesiredDistance( distInfo.m_minDistance + Min( factor1 + factor2, distInfo.m_distanceRange ) );

	moveData.m_pivotPositionController.Get()->SetZOffset( distInfo.m_cameraZOffset + distInfo.m_cameraZOffsetRange * factor1 + m_monsterSizeAdditiveOffset->GetFloatValue( mostSignificantHeight ) );

	moveData.m_pivotRotationController.Get()->StopRotating();
	moveData.m_pivotRotationController.Get()->SetDesiredPitch( m_combatPitch + m_monsterSizeAdditivePitch->GetFloatValue( mostSignificantHeight ) );

	m_smoothTime = m_baseSmoothTime;
	if( m_pivotMultDamp.GetValue() < m_combatPivotDampMult )
	{
		m_pivotMultDamp.Update( m_combatPivotDampMult, timeDelta );
	}
	else
	{
		m_pivotMultDamp.Force( m_combatPivotDampMult );
	}

	moveData.m_pivotPositionController.Get()->SetDesiredPosition( playerPosition, m_pivotMultDamp.GetValue() );
}

void CCombatCameraPositionController::Update( SCameraMovementData& moveData, Float timeDelta )
{
	moveData.m_pivotPositionController.Get()->Update( moveData.m_pivotPositionValue, moveData.m_pivotPositionVelocity, timeDelta );
	moveData.m_pivotRotationController.Get()->Update( moveData.m_pivotRotationValue, moveData.m_pivotRotationVelocity, timeDelta );
	moveData.m_pivotDistanceController.Get()->Update( moveData.m_pivotDistanceValue, moveData.m_pivotDistanceVelocity, timeDelta );

	moveData.m_cameraOffset = moveData.m_pivotRotationValue.TransformPoint( moveData.m_cameraLocalSpaceOffset );

	ASSERT( moveData.m_pivotPositionValue.IsOk() );

	// Screen-space correction
	if( m_enableScreenSpaceCorrections )
	{
		const CPlayer* player = Cast< CPlayer >( GGame->GetPlayerEntity() );
		Int32 boneIndex = -1;
		const CAnimatedComponent* ac = player ? player->GetRootAnimatedComponent() : NULL;
		if ( ac )
		{
			const ISkeletonDataProvider* provider = ac->QuerySkeletonDataProvider();
			if ( provider )
			{
				boneIndex = provider->FindBoneByName( CNAME( pelvis ) );
			}
		}

		if( boneIndex != -1 )
		{
			Vector bonePosWS;
			player->HACK_ForceGetBonePosWS( boneIndex, bonePosWS );

			const Vector& playerPos = bonePosWS;
			const CCameraDirector* director = GGame->GetActiveWorld()->GetCameraDirector();

			const Vector direction = moveData.m_pivotRotationValue.ToMatrix().GetAxisY();
			const Vector cameraPosition = direction * -moveData.m_pivotDistanceValue + moveData.m_pivotPositionValue;
			const EulerAngles cameraRotation = moveData.m_pivotRotationValue;
			CRenderCamera renderCam;
			director->OnSetupCamera( renderCam, cameraPosition, cameraRotation );

			Vector projected = director->ProjectPoint( playerPos, renderCam );
			if( projected.W > 0.9f )
			{
				Vector correction = Vector::ZERO_3D_POINT;
				Float smoothMult = 1.f;

				if( projected.Y < m_ssCorrectionYTreshold )
				{
					// Forward/Backward correction
					Vector toPlayerVec = playerPos - director->GetCameraPosition();
					const Float toPlayerDist = toPlayerVec.Normalize3();
					const Float cosBeta = MAbs( toPlayerVec.Dot3( director->GetCameraForward() ) );
					const Float y = cosBeta * toPlayerDist;

					const Float dist = Min( ( y * projected.Y / m_ssCorrectionYTreshold ) - y, 1.5f );

					// Up/Down & Left/Right correction
					const Vector point = director->UnprojectPoint( Vector( projected.X, m_ssCorrectionYTreshold, projected.Z ), renderCam );
					const Vector diff = point - playerPos;

					// Ratio
					Vector cameraForward = director->GetCameraForward();
					Vector cameraXY = cameraForward;
					cameraXY.Z = 0.f;
					cameraXY.Normalize3();

					Float dot = cameraXY.Dot3( cameraForward );
					dot *= dot;

					smoothMult = Max( m_ssCorrectionYTreshold / projected.Y, 0.5f );
					m_ssDistCorrActualSmooth = m_ssDistCorrSmooth * smoothMult;
					m_ssDistCorrection.Update( moveData.m_pivotDistanceValue, m_ssDistCorrVelocity, moveData.m_pivotDistanceValue + dist * dot, timeDelta );
					correction = diff * (1.f - dot);

					LOG_CAMERA( RED_LOG_CHANNEL(CustomCameraSSCorrection), TXT("Dist Correction Length = %f"), dist );
				}
				else
				{
					m_ssDistCorrVelocity = 0.f;
				}

				if( MAbs( projected.X ) > m_ssCorrectionXTreshold )
				{
					const Float ratioX = projected.X;
					projected.X = ratioX < 0.f ? -m_ssCorrectionXTreshold : m_ssCorrectionXTreshold;

					const Float newSmoothMult = Max( projected.X / ratioX, 0.5f );
					if( newSmoothMult < smoothMult )
					{
						smoothMult = newSmoothMult;
					}

					//Left/Right correction
					const Vector point = director->UnprojectPoint( projected, renderCam );
					const Vector diff = point - playerPos;

					correction += diff;
				}

				const Float correctionMag = correction.Mag3();
				if( correctionMag > 0.f )
				{
					if( correctionMag > 1.5f )
					{
						correction /= correctionMag;
						correction *= 1.5f;
					}

					m_ssPivotCorrActualSmooth = m_ssPivotCorrSmooth * smoothMult;
					m_ssPivotCorrection.Update( moveData.m_pivotPositionValue, m_ssPivotCorrVelocity, moveData.m_pivotPositionValue - correction, timeDelta );

					LOG_CAMERA( RED_LOG_CHANNEL(CustomCameraSSCorrection), TXT("Pivot Correction Length = %f"), correction.Mag3() );

					LOG_CAMERA( RED_LOG_CHANNEL(CustomCameraSSCorrection), TXT("Pivot Velocity = %f *** Dist Velocity = %f *** Pivot Smooth = %f *** Dist Smooth = %f"),
						m_ssPivotCorrVelocity.Mag3(), m_ssDistCorrVelocity, m_ssPivotCorrActualSmooth, m_ssDistCorrActualSmooth );
				}
				else
				{
					m_ssPivotCorrVelocity = Vector::ZERO_3D_POINT;
				}
			}
			else
			{
				m_ssPivotCorrActualSmooth = m_ssPivotCorrSmooth * 2.f;
				m_ssPivotCorrection.Update( moveData.m_pivotPositionValue, m_ssPivotCorrVelocity, playerPos, timeDelta );
			}
		}
	}

#ifndef DISABLE_CAMERA_COLLISIONS
	if( m_enableAutoCollisionAvoidance && !moveData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal ) )
	{
		m_collisionController->Update( moveData, timeDelta );
	}
	else
	{
		m_collisionController->Force( moveData );
	}

	m_collisionController2->Update( moveData, timeDelta );

	m_finalPosition = m_collisionController2->GetPosition();
	m_finalRotation = m_collisionController2->GetRotation();
#else
	const Vector direction = moveData.m_pivotRotationValue.ToMatrix().GetAxisY();
	m_finalPosition = direction * -moveData.m_pivotDistanceValue + moveData.m_pivotPositionValue + moveData.m_cameraOffset;
	m_finalRotation = moveData.m_pivotRotationValue;
#endif

#ifndef RED_FINAL_BUILD
	m_dbgMoveData = moveData;
#endif
}

void CCombatCameraPositionController::GenerateDebugFragments( CRenderFrame* frame )
{
	m_collisionController->GenerateDebugFragments( frame );
	m_collisionController2->GenerateDebugFragments( frame );

#ifndef RED_FINAL_BUILD
	const CR4Player* player = SafeCast<CR4Player>( GGame->GetPlayerEntity() );
	if( !player ) return;

	const CAnimatedComponent* ac = player->GetRootAnimatedComponent();
	const Float playerSpeed = ( ac && ac->GetBehaviorStack() ) ? ac->GetRelativeMoveSpeed()/*ac->GetBehaviorStack()->GetBehaviorFloatVariable( CNAME( playerSpeed ) )*/ : 0.f;

	String text;
	if( !player->IsInCombat() || !player->GetScriptMoveTarget() || playerSpeed > 1.1f )
	{
		text = String::Printf( TXT("Player-Cam heading - %f\
									\nCurrent Slope - %f\
									\nPitch - %f\
									\nCam Dist - %f\
									\nZ Offset - %f\
									\nPivot damp mult - %f\
									\nPivotDamp curr - %f")
									, m_dbgPlayerCamSpaceHeading, m_dbgCurrentSlope, m_dbgMoveData.m_pivotRotationValue.Pitch, m_dbgMoveData.m_pivotDistanceValue
									, m_dbgMoveData.m_pivotPositionController.Get()->GetZOffset(), m_cameraPivotDampMult->GetFloatValue( m_dbgPlayerCamSpaceHeading )
									, m_pivotMultDamp.GetValue() );
	}
	else
	{
		text = String::Printf( TXT("Angle to enemies - %f\
								   \nPitch - %f\
								   \nCam Dist - %f\
								   \nZ Offset - %f\
								   \nPivotDamp curr - %f")
								   , m_dbgCurrentSlope, m_dbgMoveData.m_pivotRotationValue.Pitch
								   , m_dbgMoveData.m_pivotDistanceValue, m_dbgMoveData.m_pivotPositionController.Get()->GetZOffset()
								   , m_pivotMultDamp.GetValue() );
	}
	frame->AddDebugScreenText( 20, 200, text, 0, true );
#endif
}
