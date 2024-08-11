#pragma once
#include "boatConfig.h"
#include "..\engine\floatDamper.h"

//////////////////////////////////////////////////////////////////////////

class CBoatComponent;

//////////////////////////////////////////////////////////////////////////

struct SInputStatus
{
    SInputStatus()
        : isHold( false )
        , isReleased( false )
        , isPressed( false )
    {}

    void UpdateInput( const SInputAction& inputAction )
    {
        if( inputAction.m_value > 0.7f && inputAction.m_lastFrameValue <= 0.7f )
        {
            isPressed = true;
            isReleased = false;
            isHold = true;
        }
        else if( inputAction.m_value < 0.7f && inputAction.m_lastFrameValue >= 0.7f )
        {
            isReleased = true;
            isPressed = false;
            isHold = false;
        }
        else
        {
            isReleased = false;
            isPressed = false;
        }
    }

    Bool WasPressed() const { return isPressed; }
    Bool WasReleased() const { return isReleased; }
    Bool IsHold() const { return isHold; }

private:
    Bool isHold;
    Bool isPressed;
    Bool isReleased;
};

//////////////////////////////////////////////////////////////////////////

class CBoatInputSystem
{
//friends:
    friend class CBoatComponent;

public:
    CBoatInputSystem(void);
    ~CBoatInputSystem(void);
    
    Bool Initialize( CBoatComponent* boatComponent );
    void Deinitialize();
    void ResetState();
    void DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset );

    void GatherInputLocalSpaceCamera( const SInputAction& accelerate, const SInputAction& decelerate, const Vector& stickTilt, Float localSpaceCameraTurnPercent );
    void GatherInputGlobalSpaceCamera();
    void InputUpdate
    (
          Vector2& outMovementForce
        , Float deltaTime
        , const Matrix& globalRotation
        , CPhysicsWrapperInterface* wrapper
        , Bool pathfindingEnabled
    );

    // Steering
    void SteerGearUp();
    void SteerGearDown();
    void SteerSetGear( Int32 gear );
    void SteerFullStop();
    Int32 SteerGetCurrentGear() const;

    void BoatBodyCollision();
    void LimitSpeed( Float speedRatio );

private:
    void ProcessMashingControlls( Float deltaTime );
	void ProcessMashingControllsPad( Float deltaTime );
	void ProcessMashingControllsPC( Float deltaTime );

private:
    Bool           	m_isInitialized;
    CBoatComponent*	m_boatComponent;

    // Input gathering
    Float          	m_analogInput;
    Vector2        	m_stickInput;

    // Status
    Float          	m_steerTargetAngleDeg;
    Float          	m_analogInputThresholdSign;

    Int32          	m_currentGear;

    SInputStatus   	m_inputSlowDown;
    SInputStatus   	m_inputSpeedUp;

    // Mashing and tapping
    Float          	m_tappingSubsequentTapsTime;
    Uint32         	m_tappingNumTaps;
    Float          	m_turningSpeed;

    // Speed damper
    CFloatDamper   	m_speedDamper;
    Float          	m_speedDamperAcceleration;

    // Gears
    Float          	m_gearCandidateTimeout;
    Int32          	m_gearCandidate;
    Bool           	m_gearAnalogMode;
    Float           m_speedLimit;
        
#ifdef DEBUG_DPAD_INPUT
    Float DBG_input1;
    Float DBG_input2;
#endif
#ifndef FINAL
    Vector2 DBG_forceDirectionVector;
    Float DBG_steerAngleLimitMin;
    Float DBG_steerAngleLimitMax;
    Float DBG_maxSteerAngle;
#endif

private:

    void SetSpeedDamper( Float targetSpeed, Float acceleration = 0.0f );
    void SetDamperCurrentSpeedInstantly( Float targetSpeed );
    Float GetTargetSpeed() const;
    void SetGearCandidate( Int32 candidate );
    void UpdateGearCandidate( Float deltaTime );
};

//////////////////////////////////////////////////////////////////////////

RED_INLINE Int32 CBoatInputSystem::SteerGetCurrentGear() const
{
    return m_currentGear;
}

//////////////////////////////////////////////////////////////////////////
