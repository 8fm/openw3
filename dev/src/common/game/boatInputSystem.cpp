#include "build.h"
#include "boatInputSystem.h"
#include "boatComponent.h"
#include "boatConfig.h"
#include "../physics/physicsWrapper.h"
#include "..\engine\renderFrame.h"

//////////////////////////////////////////////////////////////////////////

CBoatInputSystem::CBoatInputSystem(void)
    : m_isInitialized( false )
    , m_boatComponent( nullptr )
    , m_analogInput( 0.0 )
    , m_steerTargetAngleDeg( 0.0f )
    , m_currentGear( 0 )
    , m_tappingNumTaps( 0 )
    , m_tappingSubsequentTapsTime( 1000.0f )
    , m_analogInputThresholdSign( 1.0f )
    , m_speedDamperAcceleration( 0.5f )
    , m_gearAnalogMode( true )
    , m_turningSpeed( BoatConfig::cvBoatTurningZeroSpeedSteerAngle.Get() / BoatConfig::cvBoatTurningZeroSpeedSteerAngle.Get() )
    , m_stickInput( 0, 0 )
    , m_speedLimit( BoatConfig::cvBoatSailingGearThreeMaxSpeed.Get() )
{    
    m_speedDamper.Setup( 0, 0, 1.0f, EET_Linear, EET_InOut );
}

//////////////////////////////////////////////////////////////////////////

CBoatInputSystem::~CBoatInputSystem(void)
{
    Deinitialize();
}

//////////////////////////////////////////////////////////////////////////

Bool CBoatInputSystem::Initialize( CBoatComponent* boatComponent )
{
    if( m_isInitialized )
        return true;

    if( boatComponent == nullptr )
        return false;

    m_boatComponent = boatComponent;

    m_isInitialized = true;
    return true;
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::Deinitialize()
{
    m_isInitialized             = false;
    m_boatComponent             = nullptr;
    
    ResetState();
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::ResetState()
{
    m_analogInput               = 0.0;
    m_steerTargetAngleDeg       = 0.0f;
    m_currentGear               = 0;
    m_tappingNumTaps            = 0;
    m_tappingSubsequentTapsTime = 1000.0f;
    m_analogInputThresholdSign  = 1.0f;
    m_speedDamperAcceleration   = 0.5f;
    m_gearAnalogMode            = true;
    m_inputSlowDown             = SInputStatus();
    m_inputSpeedUp              = SInputStatus();
    m_stickInput.Set(0, 0);
    // Stop the damper
    SetDamperCurrentSpeedInstantly( 0 );
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::GatherInputGlobalSpaceCamera()
{
    if( !m_isInitialized || m_boatComponent->m_user.Get() == nullptr )
    {
        return;
    }

    CInputManager* inputMgr = GGame->GetInputManager();

    ASSERT(inputMgr);

    // Reset params
    m_analogInput = 0.0f;

    // Gather input
    m_stickInput.Set( inputMgr->GetActionValue( CNAME( GI_AxisLeftX ) ), inputMgr->GetActionValue( CNAME( GI_AxisLeftY ) ) );    
    m_inputSlowDown.UpdateInput( inputMgr->GetInputActionByName( CNAME( GI_Decelerate ) ) );
    m_inputSpeedUp.UpdateInput( inputMgr->GetInputActionByName( CNAME( GI_Accelerate ) ) );

    if( m_inputSlowDown.IsHold() && !m_inputSpeedUp.IsHold() )
    {
        m_analogInput = -1.0f;
        return;
    }

    if( m_stickInput.Y < 0 )
    {
        static Float sSTICK_INPUT_THRESHOLD = -0.2f;

        if( m_stickInput.Y < sSTICK_INPUT_THRESHOLD )
        {
            m_stickInput.Y = 0;
            m_analogInput = 0;
            return;
        }
        else
        {
            m_stickInput.Y = Abs( m_stickInput.Y );
        }
    }

    m_analogInput = m_stickInput.Mag();
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::GatherInputLocalSpaceCamera( const SInputAction& accelerate, const SInputAction& decelerate, const Vector& stickTilt, Float localSpaceCameraTurnPercent )
{
    if( !m_isInitialized || m_boatComponent->m_user.Get() == nullptr )
    {
        return;
    }

    // Reset params
    m_analogInput = 0.0f;

    m_stickInput = stickTilt.AsVector2();
    m_inputSlowDown.UpdateInput( decelerate );
    m_inputSpeedUp.UpdateInput( accelerate );

    // Rotate boat towards camera look target
    Float tiltXSum = fabsf( stickTilt.X + localSpaceCameraTurnPercent );

    if( tiltXSum > 1.0f )
    {
        tiltXSum = fabsf( stickTilt.X * 2.0f + localSpaceCameraTurnPercent );
        m_stickInput.X = ( stickTilt.X * 2.0f ) / tiltXSum + localSpaceCameraTurnPercent / tiltXSum;
    }
    else
    {
        m_stickInput.X = m_stickInput.X + localSpaceCameraTurnPercent;
    }

    m_analogInput = m_stickInput.Mag();

    // Prevent applying propelling force when stick is tilted backward
    // Yet allow slight backward tilt (20%)
    if( m_stickInput.Y < 0 )
    {
        static Float sSTICK_INPUT_THRESHOLD = -0.2f;

        if( m_stickInput.Y < sSTICK_INPUT_THRESHOLD )
        {
            m_stickInput.Y = 0;
            m_analogInput = 0;
            return;
        }
        else
        {
            m_stickInput.Y = Abs( m_stickInput.Y );
        }
    }

    // In backward movement disable stick propelling force dosage
    if( m_inputSlowDown.IsHold() && !m_inputSpeedUp.IsHold() )
    {
        m_analogInput = -1.0f;
        return;
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::InputUpdate
    (
    Vector2& outMovementForce
    , Float deltaTime
    , const Matrix& globalRotation
    , CPhysicsWrapperInterface* wrapper
    , Bool pathfindingEnabled
    )
{
    if( !m_isInitialized )
        return;
    
	if( m_boatComponent->IsDriverDismounted() )
	{
		m_currentGear = 0;
		m_gearCandidate = -666;
	}

    //////////////////////////////////////////////////////////////////////////
    // Dismount action
    if( m_boatComponent->m_isDismountRequested )
    {
        Vector linearVelocity;
        Vector angularVelocity;
        wrapper->GetVelocity( linearVelocity, angularVelocity );

        // Wait with dismount till boat comes to full stop
        if( linearVelocity.SquareMag2() < 0.04f || m_boatComponent->m_user.Get() == nullptr )
        {
            m_boatComponent->m_isDismountRequested = false;
            SteerFullStop();
            m_boatComponent->CallEvent( CNAME(OnTriggerBoatDismountAnim) );
        }
        // Slow down boat
        else if( m_currentGear != 0 || m_speedDamper.GetDestValue() != 0.0f )
        {
            if( m_currentGear < 0 )
            {
                SetSpeedDamper( 0, m_boatComponent->m_sailSpeeds.Back().deceleration );
            }
            else
            {
                SetSpeedDamper( 0, m_boatComponent->m_sailSpeeds[m_currentGear-1].deceleration );
            }

            m_currentGear = 0;
        }
    }    
    //////////////////////////////////////////////////////////////////////////
    // Apply speed from gear that is set
    else
    {
        if( !pathfindingEnabled )
        {
            ProcessMashingControlls( deltaTime );
        }
        
        // Slow down while climbing high wave
        const Float xTiltDeg = RAD2DEG( asinf( globalRotation.V[1].Z ) );
        Float climbRatio = 1.0f;

        if( xTiltDeg >= BoatConfig::cvBoatWaveClimbAngleThreshold2.Get() )
            climbRatio = 0.6f;
        else if( xTiltDeg >= BoatConfig::cvBoatWaveClimbAngleThreshold1.Get() )
            climbRatio = 0.8f;

        // Get target speed on this gear
        Float maxSpeedOnThisGear = 0.0f;
        if( m_currentGear > 0 )
        {
            maxSpeedOnThisGear = m_boatComponent->m_sailSpeeds[ m_currentGear-1 ].maxSpeed;
        }
        else if( m_currentGear == 0 )
        {
            maxSpeedOnThisGear = 0.0f;
        }
        else
        {
            maxSpeedOnThisGear = -m_boatComponent->m_sailSpeeds.Back().maxSpeed;
        }

        maxSpeedOnThisGear *= climbRatio;

        // Change target speed and acceleration based on analog stick tilt
        if( m_analogInput != 0 && m_currentGear <= 1)
            maxSpeedOnThisGear *= fabsf( m_analogInput );

        // Change speed only if its necessary
        if( maxSpeedOnThisGear != m_speedDamper.GetDestValue() )
        {
            if( m_currentGear > 0 )
                SetSpeedDamper( maxSpeedOnThisGear, m_boatComponent->m_sailSpeeds[m_currentGear-1].acceleration );
            else
                SetSpeedDamper( maxSpeedOnThisGear, m_boatComponent->m_sailSpeeds[0].acceleration );
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Step speed damper
    Float sailingTargetSpeed = Clamp<Float>( m_speedDamper.Update( deltaTime ), -m_speedLimit, m_speedLimit );    
    
    //////////////////////////////////////////////////////////////////////////
    // Rotation    
    if( !pathfindingEnabled )
    {
        if( m_stickInput.X != 0.0f && fabsf( sailingTargetSpeed ) > 0.0f )    // Turn steer when LS is tilted
        {
            // get max ster angle
            Float maxSteerAngleDeg = BoatConfig::cvBoatTurningZeroSpeedSteerAngle.Get();

            const Bool isGearOne = m_currentGear == 1 || m_currentGear == -1;
            const Float absHorizontalInput = fabsf(m_stickInput.X);

            // Limit max propelling force when tight turning on gear one and reverse
            if( isGearOne )
            {
                Float stickTiltAngle = 0.0f;
                if( m_stickInput.X != 0.0f )
                {
                    stickTiltAngle = m_stickInput.Y / m_stickInput.X;
                    stickTiltAngle = RAD2DEG( atanf( stickTiltAngle ) );
                    stickTiltAngle = fabsf( stickTiltAngle );
                    stickTiltAngle = 90.0f - stickTiltAngle;
                }

                Float forceCutoffRatio = 1.0f;
                if( stickTiltAngle > BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get() )
                {
                    forceCutoffRatio = ( stickTiltAngle - BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get() ) / ( 90.0f - BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get() );                    
                    forceCutoffRatio = 1.0f - forceCutoffRatio;
                }

                const Float expo = EasingFunctions::ExpoEaseIn( forceCutoffRatio, BoatConfig::cvBoatTurningZeroAngleForceCutoff.Get(), 1.0f - BoatConfig::cvBoatTurningZeroAngleForceCutoff.Get(), 1.0f );

                sailingTargetSpeed *= expo;

                // Limit first gear angle if we re slowing down
                // Prevent quick 180 degrees turn
                const Float realSpeed = m_boatComponent->GetLinearVelocityXY();
                if( realSpeed > m_boatComponent->GetGear(2).maxSpeed )
                {
                    maxSteerAngleDeg = BoatConfig::cvBoatTurningSecondSpeedSteerAngle.Get();
                }
                else if( realSpeed > m_boatComponent->GetGear(1).maxSpeed )
                {
                    maxSteerAngleDeg = BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get();
                }
            }            
            else if( m_currentGear != 0 ) // Limit max steer angle, its speed dependent
            {
                const Float firstToLastRatio = fabsf( m_boatComponent->m_sailSpeeds[0].maxSpeed / m_boatComponent->m_sailSpeeds[MAX_GEARS-1].maxSpeed );
                const Float speedRatioHi = fabsf( sailingTargetSpeed / m_boatComponent->m_sailSpeeds[MAX_GEARS-1].maxSpeed ) - firstToLastRatio;
                maxSteerAngleDeg = BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get() + ( speedRatioHi * ( BoatConfig::cvBoatTurningSecondSpeedSteerAngle.Get() - BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get() ) );
            }
            else // Zero speed, zero angle
            {
                maxSteerAngleDeg = 0.0f;
            }

            // Match max rotation with stick tilt
            maxSteerAngleDeg *= absHorizontalInput;

            // Rotation speed depends on max tilt angle
            m_turningSpeed = maxSteerAngleDeg / BoatConfig::cvBoatTurningSteerRotationTime.Get();

#ifndef FINAL
            DBG_maxSteerAngle = maxSteerAngleDeg;
#endif

            // Turn steer to side
            m_steerTargetAngleDeg += m_stickInput.X * m_turningSpeed * deltaTime;

            // Limit sailing speed
            m_steerTargetAngleDeg = Clamp<Float>( m_steerTargetAngleDeg, -maxSteerAngleDeg, maxSteerAngleDeg );

#ifndef FINAL
            DBG_steerAngleLimitMin = -maxSteerAngleDeg;
            DBG_steerAngleLimitMax =  maxSteerAngleDeg;
#endif
        }
        else if( m_steerTargetAngleDeg != 0.0f )    // Move steer to zero angle when LS is 0,0
        {
            const Float decayStep = m_turningSpeed * deltaTime * 2.0f;
            if( decayStep >= fabsf( m_steerTargetAngleDeg ) )
            {
                m_steerTargetAngleDeg = 0.0f;
            }
            else
            {
                const Float sign = m_steerTargetAngleDeg > 0 ? 1.0f : -1.0f;
                m_steerTargetAngleDeg -= sign * decayStep;
            }
        }
    }

    // Transform inertia to global frame
    const EulerAngles angles( 0, 0, m_steerTargetAngleDeg );
    Matrix globalSteerRotation;
    angles.ToMatrix(globalSteerRotation);
    globalSteerRotation = globalRotation * globalSteerRotation;

    // Update movement force
    const Float forceValue = sailingTargetSpeed * BoatConfig::cvBoatSailingForceScaller.Get();
    const Vector2 movementForceNormal = globalSteerRotation.TransformPoint( Vector(0,1,0) );

    // Set output movement force
    outMovementForce = movementForceNormal * forceValue;

#ifndef FINAL
    DBG_forceDirectionVector = movementForceNormal;
#endif
    
#ifdef DEBUG_DPAD_INPUT
    //////////////////////////////////////////////////////////////////////////
    // DEBUG controlls
    if( m_boatComponent->m_user == GGame->GetPlayerEntity() )
    {
        CRawInputManager& rawInput = SRawInputManager::GetInstance();

        // Impulse
        if( rawInput.WasKeyJustPressed( EInputKey::IK_Pad_DigitUp ) )
            DBG_input1 = 1.0f;
        if( rawInput.WasKeyJustReleased( EInputKey::IK_Pad_DigitUp ) )
            DBG_input1 = 0.0f;

        if( rawInput.WasKeyJustPressed( EInputKey::IK_Pad_DigitDown ) )
            DBG_input1 = -1.0f;
        if( rawInput.WasKeyJustReleased( EInputKey::IK_Pad_DigitDown ) )
            DBG_input1 = 0.0f;

        // Torque
        if( rawInput.WasKeyJustPressed( EInputKey::IK_Pad_DigitLeft ) )
            DBG_input2 = 1.0f;
        if( rawInput.WasKeyJustReleased( EInputKey::IK_Pad_DigitLeft ) )
            DBG_input2 = 0.0f;

        if( rawInput.WasKeyJustPressed( EInputKey::IK_Pad_DigitRight ) )
            DBG_input2 = -1.0f;
        if( rawInput.WasKeyJustReleased( EInputKey::IK_Pad_DigitRight ) )
            DBG_input2 = 0.0f;

        const Float mass = wrapper->GetMass();
        const Matrix pose = wrapper->GetPose();
        const Vector yAligned = Vector::Cross( Vector::EZ, pose.GetAxisX() ); // Y = Z cross X

        static Float torqueScaller = 60.0f;
        Vector impulse = pose.GetAxisZ() * mass * DBG_input2 * torqueScaller * deltaTime;
        wrapper->ApplyTorqueImpulse( impulse );

        static Float forceScaler = 150.0f;
        impulse = yAligned * mass * DBG_input1 * forceScaler * deltaTime;
        wrapper->ApplyImpulse( impulse, wrapper->GetCenterOfMassPosition() );
    }
    // DEBUG controlls
    //////////////////////////////////////////////////////////////////////////
#endif
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::SteerGearUp()
{
    if( !m_isInitialized )
        return;

    if( m_currentGear < (Int32)MAX_GEARS )
    {
        if( m_currentGear >= 0 )
        {
            SetSpeedDamper( m_boatComponent->m_sailSpeeds[m_currentGear].maxSpeed, m_boatComponent->m_sailSpeeds[m_currentGear].acceleration );
        }
        else if( m_currentGear < 0 )
        {
            SetSpeedDamper( 0, m_boatComponent->m_sailSpeeds.Back().deceleration );
        }

        ++m_currentGear;
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::SteerGearDown()
{
    if( !m_isInitialized )
        return;

    if( m_currentGear > -1 )
    {
        if( m_currentGear == 0 )
            SetSpeedDamper( -(m_boatComponent->m_sailSpeeds.Back().maxSpeed), m_boatComponent->m_sailSpeeds.Back().acceleration );
        else
        {
            const Int32 gear = m_currentGear-1;
            if( gear == 0)
            {
                SetSpeedDamper( 0, m_boatComponent->m_sailSpeeds[gear].deceleration );
            }
            else
            {
                SetSpeedDamper( m_boatComponent->m_sailSpeeds[gear-1].maxSpeed, m_boatComponent->m_sailSpeeds[gear-1].deceleration );
            }
        }

        --m_currentGear;
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::SteerSetGear( Int32 gear )
{
    if( !m_isInitialized )
        return;

    gear = Clamp<Int32>( gear, -1, (Int32)MAX_GEARS );
    if( gear > m_currentGear ) // gear up
    {
        m_currentGear = gear - 1;
        SteerGearUp();
    }
    else if( gear < m_currentGear ) // gear down
    {
        if( gear == -1 ) // reverse gear == -1
        {
            SetSpeedDamper( -(m_boatComponent->m_sailSpeeds.Back().maxSpeed), m_boatComponent->m_sailSpeeds.Back().acceleration );
        }
        else if( gear == 0 ) // slow down to zero using current gear deceleration gear == 0
        {
            SetSpeedDamper( 0, m_boatComponent->m_sailSpeeds[m_currentGear-1].deceleration );
        }
        else // gear == 1, 2
        {
            SetSpeedDamper( m_boatComponent->m_sailSpeeds[gear-1].maxSpeed, m_boatComponent->m_sailSpeeds[m_currentGear-1].deceleration );
        }

        m_currentGear = gear;
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::SteerFullStop()
{
    if( !m_isInitialized )
        return;

    SteerSetGear(0);
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::ProcessMashingControlls( Float deltaTime )
{
	Bool inputPC = GGame->GetInputManager() ? GGame->GetInputManager()->LastUsedPCInput() : false;

	if( inputPC )
	{
		ProcessMashingControllsPC( deltaTime );
	}
	else
	{
		ProcessMashingControllsPad( deltaTime );
	}
}

void CBoatInputSystem::ProcessMashingControllsPad( Float deltaTime )
{
    // Sum up time between double tap
    if( m_tappingNumTaps == 1 && m_tappingSubsequentTapsTime < BoatConfig::cvBoatDoubleTapThreshold.Get() )
        m_tappingSubsequentTapsTime += deltaTime;

    // Stop when holding X
    if( m_inputSlowDown.IsHold() )
    {
        m_tappingNumTaps = 0;
        m_tappingSubsequentTapsTime = 0.0f;
        m_gearCandidate = -666;
        m_gearCandidateTimeout = 0.0f;

        m_gearAnalogMode = true;
    }
    // Resume to last gear used after slow down is released
    else if( m_inputSlowDown.WasReleased() )
    {
        if( m_inputSpeedUp.IsHold() )   // Resume to second gear if LB is pressed
        {
            m_gearAnalogMode = false;
            m_tappingNumTaps = 1;
            m_tappingSubsequentTapsTime = 0;
            SteerSetGear( 2 );
        }

    }
    // Gather speed up presses
    else if( m_inputSpeedUp.WasPressed() && !m_boatComponent->IsDriverDismounted() )  // Start mashing timer if its first press, sum up mashing time if its second press
    {
        ++m_tappingNumTaps;

        if( m_tappingNumTaps == 1 )
        {
            SetGearCandidate( 2 );
            m_tappingSubsequentTapsTime = 0;
        }
        else if( m_tappingNumTaps == 2 )
        {
            if(  m_tappingSubsequentTapsTime < BoatConfig::cvBoatDoubleTapThreshold.Get()  ) // Set 3rd gear only if subsequent presses were near echo other
                SetGearCandidate( 3 );
            else    // Back to 2nd gear
            {
                SetGearCandidate( 2 );
                m_tappingNumTaps = 1;
                m_tappingSubsequentTapsTime = 0;    // ...and taps time reset
            }
        }
    }
    else if( m_inputSpeedUp.WasReleased() ) // Go back to analog from 2nd and 3rd gear
    {
        SetGearCandidate( 0 );

        if( m_tappingNumTaps >= 2 ) // if back from 3rd reset taps status
        {
            m_tappingNumTaps = 0;
            m_tappingSubsequentTapsTime = 0.0f;
        }
    }

	UpdateGearCandidate( deltaTime ); // Update gear candidate time or switch to new gear

	if( m_gearAnalogMode )
	{
		SteerSetGear( Sgn(m_analogInput) );
	}
}

void CBoatInputSystem::ProcessMashingControllsPC( Float deltaTime )
{
	// Stop when holding X
	if( m_inputSlowDown.IsHold() )
	{
		m_gearCandidate = -666;
		m_gearCandidateTimeout = 0.0f;
		m_gearAnalogMode = true;
	}
	// Resume to last gear used after slow down is released
	else if( m_inputSlowDown.WasReleased() )
	{
		if( m_inputSpeedUp.IsHold() )   // Resume to second gear if LB is pressed
		{
			m_gearAnalogMode = false;
			SteerSetGear( 3 );
		}

	}
	// Gather speed up presses
	else if( m_inputSpeedUp.WasPressed() && !m_boatComponent->IsDriverDismounted() )  // Start mashing timer if its first press, sum up mashing time if its second press
	{
		SetGearCandidate( 3 );
	}
	else if( m_inputSpeedUp.WasReleased() ) // Go back to analog from 2nd and 3rd gear
	{
		SetGearCandidate( 0 );
	}

	UpdateGearCandidate( deltaTime ); // Update gear candidate time or switch to new gear

	if( m_gearAnalogMode )
	{
		if( m_analogInput < 0.0f )
		{
			SteerSetGear( -1 );
		}
		else if( m_analogInput > 0.0f )
		{
			SteerSetGear( 2 );
		}
		else
		{
			SteerSetGear( 0 );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset )
{
    ++inOutTestOffset;
    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Current gear: %i"), m_currentGear )
        ,   inOutTestOffset
        ,   true
        );

    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Analog input horizontal: %f"), m_stickInput.X )
        ,   inOutTestOffset
        ,   true
        );

    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Analog input tilt: %f"), m_analogInput )
        ,   inOutTestOffset
        ,   true
        );

    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Steer max angle deg: %f"), DBG_maxSteerAngle )
        ,   inOutTestOffset
        ,   true
        );

    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Steer target angle deg: %f"), m_steerTargetAngleDeg )
        ,   inOutTestOffset
        ,   true
        );

    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Steer turning speed deg/s: %f"), m_turningSpeed )
        ,   inOutTestOffset
        ,   true
        );

    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Speed damper acceleration: %f"), m_speedDamperAcceleration )
        ,   inOutTestOffset
        ,   true
        );    
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::SetSpeedDamper( Float targetSpeed, Float acceleration )
{
    // Change target value
    Bool targetSet = false;
    if( m_speedDamper.GetDestValue() != targetSpeed )
    {
        targetSet = true;
        m_speedDamper.SetStartValue( m_speedDamper.GetInterpolatedValue() );
        m_speedDamper.SetDestValue( targetSpeed );
        m_speedDamper.ResetInterpolationTime();
    }

    // Set new acceleration
    if( m_speedDamperAcceleration != acceleration || targetSet )
    {
        if( acceleration != 0.0f )
            m_speedDamperAcceleration = acceleration;

        const Float speedDiff = m_speedDamper.GetDestValue() - m_speedDamper.GetInterpolatedValue();
        const Float dampingTime = fabsf( speedDiff / m_speedDamperAcceleration );

        m_speedDamper.SetDampingTime( dampingTime );
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::SetDamperCurrentSpeedInstantly( Float targetSpeed )
{
    SetSpeedDamper( targetSpeed );
    m_speedDamper.SetInterpolationTime( 1.0f );
}

//////////////////////////////////////////////////////////////////////////

Float CBoatInputSystem::GetTargetSpeed() const
{
    return m_speedDamper.GetDestValue();
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::SetGearCandidate( Int32 candidate )
{
    m_gearCandidateTimeout = 0.0f;
    m_gearCandidate = candidate;
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::UpdateGearCandidate( Float deltaTime )
{
    if( m_gearCandidate == -666 )
        return;

    m_gearCandidateTimeout += deltaTime;

    if( m_gearCandidateTimeout > BoatConfig::cvBoatGearCandidateSwitchTimeout.Get() )
    {
        if( m_gearCandidate == 0 )
        {
            SteerSetGear( Sgn(m_analogInput) );
            m_gearAnalogMode = true;
        }
        else
        {
            m_gearAnalogMode = false;
            SteerSetGear( m_gearCandidate );
        }

        m_gearCandidate = -666;
        m_gearCandidateTimeout = 0.0f;
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::LimitSpeed( Float speedRatio )
{
    const Float maxSpeed = m_boatComponent->m_sailSpeeds[MAX_GEARS-1].maxSpeed;
    const Float minSpeed = ( m_currentGear > 0 ? m_boatComponent->m_sailSpeeds[0].maxSpeed : m_boatComponent->m_sailSpeeds[MAX_GEARS].maxSpeed );
    m_speedLimit = maxSpeed;

    if( speedRatio < 0.0f ) // Boat entered shore, disable all input
    {
        m_speedLimit = 0.0f;
    }
    else if( speedRatio < minSpeed / maxSpeed ) // Damper is to low, allow boat to move at least at first or reverse gear's max speed
    {
        m_speedLimit = minSpeed;
    }
    else // Use dampers ratio directly
    {
        m_speedLimit *= speedRatio;
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatInputSystem::BoatBodyCollision()
{
    // Slow down to gear one if moving fast
    if( m_currentGear > 1 && m_currentGear != 0 )
    {
        SteerSetGear( 1 );
    }

    // Immediate stop the damper
    SetSpeedDamper( GetTargetSpeed(), 100.0f  );
}

//////////////////////////////////////////////////////////////////////////
