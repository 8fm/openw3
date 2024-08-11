#pragma once
#include "..\engine\floatDamper.h"
#include "..\core\configVar.h"
#include "..\engine\vectorDamper.h"

//////////////////////////////////////////////////////////////////////////

static const Uint32 MAX_WATER_CONTACT_POINTS     	= 4;
static const Uint8  MAX_GEARS                    	= 3;
static const Float  KINEMATIC_CONSTRAINER_RADIUS 	= 0.3f;

//////////////////////////////////////////////////////////////////////////

#ifdef RED_CONFIGURATION_NOPTS
    #define DEBUG_DPAD_INPUT
#endif

//////////////////////////////////////////////////////////////////////////

namespace BoatConfig
{
    using namespace Config;

    extern TConfigVar<Bool>  cvBoatDrowningEnabled;
    extern TConfigVar<Float> cvBoatBuoyancyPointsSpreadX;
    extern TConfigVar<Float> cvBoatBuoyancyPointsSpreadY;    
    extern TConfigVar<Float> cvBoatMassCenter;
    extern TConfigVar<Float> cvBoatMass;
    extern TConfigVar<Float> cvBoatTiltXLimit;
    extern TConfigVar<Float> cvBoatTiltYLimit;
    extern TConfigVar<Float> cvBoatTorqueScaller;
    extern TConfigVar<Float> cvBoatForceScaller;
	extern TConfigVar<Float> cvBoatForceMax;
    extern TConfigVar<Float> cvBoatFloatingHeight;
    extern TConfigVar<Float> cvBoatDrowningAcceleration;
    extern TConfigVar<Float> cvBoatDrowningPropagationTime;
    extern TConfigVar<Float> cvBoatDrowningSystemsShutdownTime;
    extern TConfigVar<Float> cvBoatDestructionHitsAcceptTimeout;
    extern TConfigVar<Float> cvBoatDestructionHitRepelForceMultiplier;
    extern TConfigVar<Float> cvBoatBuoyancyWaterProbingPrecision;
    extern TConfigVar<Float> cvBoatBuoyancyFlyThresholdScaller;
    extern TConfigVar<Bool>  cvBoatSailingEnabled;

	extern TConfigVar<Float>  cvBoatPID_P;
	extern TConfigVar<Float>  cvBoatPID_I;
	extern TConfigVar<Float>  cvBoatPID_D;

    extern TConfigVar<Float> cvBoatSailingGearOneMaxSpeed;
    extern TConfigVar<Float> cvBoatSailingGearOneAcceleration;
    extern TConfigVar<Float> cvBoatSailingGearOneDeceleration;

    extern TConfigVar<Float> cvBoatSailingGearTwoMaxSpeed;
    extern TConfigVar<Float> cvBoatSailingGearTwoAcceleration;
    extern TConfigVar<Float> cvBoatSailingGearTwoDeceleration;

    extern TConfigVar<Float> cvBoatSailingGearThreeMaxSpeed;
    extern TConfigVar<Float> cvBoatSailingGearThreeAcceleration;
    extern TConfigVar<Float> cvBoatSailingGearThreeDeceleration;

    extern TConfigVar<Float> cvBoatSailingGearReverseMaxSpeed;
    extern TConfigVar<Float> cvBoatSailingGearReverseAcceleration;
    extern TConfigVar<Float> cvBoatSailingGearReverseDeceleration;

    extern TConfigVar<Float> cvBoatSailingForceScaller;
    extern TConfigVar<Float> cvBoatTurningZeroAngleForceCutoff;
    extern TConfigVar<Float> cvBoatTurningZeroSpeedSteerAngle;
    extern TConfigVar<Float> cvBoatTurningFristSpeedSteerAngle;
    extern TConfigVar<Float> cvBoatTurningSecondSpeedSteerAngle;
    extern TConfigVar<Float> cvBoatTurningSteerRotationTime;
    extern TConfigVar<Float> cvBoatTurningForcePosition;
    extern TConfigVar<Float> cvBoatLinearDamping;
    extern TConfigVar<Float> cvBoatAngularDamping;
    extern TConfigVar<Float> cvBoatWaveClimbAngleThreshold1;
    extern TConfigVar<Float> cvBoatWaveClimbAngleThreshold2;
    extern TConfigVar<Float> cvBoatDoubleTapThreshold;
    extern TConfigVar<Float> cvBoatGearCandidateSwitchTimeout;
    extern TConfigVar<Float> cvBoatPathFindingOutFrustumMaxDistance;

    extern TConfigVar<Float> cvBoatTiltingYSpeed;
    extern TConfigVar<Float> cvBoatTiltingMaxYTiltPercent;
    extern TConfigVar<Float> cvBoatTiltingMinYTiltPercent;
    
    extern TConfigVar<Float> cvBoatBuoyancyZDiffLimit;
    extern TConfigVar<Float> cvBoatForceUpBase;
    extern TConfigVar<Float> cvBoatForceUpExpo;

    extern TConfigVar<Float> cvBoatForceDownBase;
    extern TConfigVar<Float> cvBoatForceDownExpo;

    extern TConfigVar<Float> cvBoatTorqueUpBase;
    extern TConfigVar<Float> cvBoatTorqueUpExpo;

    extern TConfigVar<Float> cvBoatTorqueDownNormBase;
    extern TConfigVar<Float> cvBoatTorqueDownNormExpo;

    extern TConfigVar<Float> cvBoatTorqueDownInAirBase;
    extern TConfigVar<Float> cvBoatTorqueDownInAirExpo;

    extern TConfigVar< Bool  > driverBoatLocalSpaceCamera;
	extern TConfigVar< Float > driverBoatFovStand;
	extern TConfigVar< Float > driverBoatFovReverse;
	extern TConfigVar< Float > driverBoatFovGear1;
	extern TConfigVar< Float > driverBoatFovGear2;
	extern TConfigVar< Float > driverBoatFovGear3;

	extern TConfigVar< Float > driverBoatDistanceStand;
	extern TConfigVar< Float > driverBoatDistanceReverse;
	extern TConfigVar< Float > driverBoatDistanceGear1;
	extern TConfigVar< Float > driverBoatDistanceGear2;
	extern TConfigVar< Float > driverBoatDistanceGear3;

	extern TConfigVar< Float > driverBoatPitchStand;
	extern TConfigVar< Float > driverBoatPitchReverse;
	extern TConfigVar< Float > driverBoatPitchGear1;
	extern TConfigVar< Float > driverBoatPitchGear2;
	extern TConfigVar< Float > driverBoatPitchGear3;

	extern TConfigVar< Float > driverBoatPivotOffsetUpReverse;
	extern TConfigVar< Float > driverBoatPivotOffsetUpGear1;
	extern TConfigVar< Float > driverBoatPivotOffsetUpGear2;
	extern TConfigVar< Float > driverBoatPivotOffsetUpGear3;

	extern TConfigVar< Float > driverBoatCameraToSailOffsetStand;
	extern TConfigVar< Float > driverBoatCameraToSailOffsetReverse;
	extern TConfigVar< Float > driverBoatCameraToSailOffsetGear1;
	extern TConfigVar< Float > driverBoatCameraToSailOffsetGear2;
	extern TConfigVar< Float > driverBoatCameraToSailOffsetGear3;

	extern TConfigVar< Float > driverBoatFovAdjustCoef;
	extern TConfigVar< Float > driverBoatDistanceAdjustCoef;
	extern TConfigVar< Float > driverBoatPitchAdjustCoef;
	extern TConfigVar< Float > driverBoatOffsetAdjustCoef;
	extern TConfigVar< Float > driverBoatCameraToSailOffsetAdjustCoef;




	extern TConfigVar< Float > passengerBoatFovStand;
	extern TConfigVar< Float > passengerBoatFovReverse;
	extern TConfigVar< Float > passengerBoatFovGear1;
	extern TConfigVar< Float > passengerBoatFovGear2;
	extern TConfigVar< Float > passengerBoatFovGear3;

	extern TConfigVar< Float > passengerBoatDistanceStand;
	extern TConfigVar< Float > passengerBoatDistanceReverse;
	extern TConfigVar< Float > passengerBoatDistanceGear1;
	extern TConfigVar< Float > passengerBoatDistanceGear2;
	extern TConfigVar< Float > passengerBoatDistanceGear3;

	extern TConfigVar< Float > passengerBoatPitchStand;
	extern TConfigVar< Float > passengerBoatPitchReverse;
	extern TConfigVar< Float > passengerBoatPitchGear1;
	extern TConfigVar< Float > passengerBoatPitchGear2;
	extern TConfigVar< Float > passengerBoatPitchGear3;

	extern TConfigVar< Float > passengerBoatPivotOffsetUpReverse;
	extern TConfigVar< Float > passengerBoatPivotOffsetUpGear1;
	extern TConfigVar< Float > passengerBoatPivotOffsetUpGear2;
	extern TConfigVar< Float > passengerBoatPivotOffsetUpGear3;

	extern TConfigVar< Float > passengerBoatCameraToSailOffsetStand;
	extern TConfigVar< Float > passengerBoatCameraToSailOffsetReverse;
	extern TConfigVar< Float > passengerBoatCameraToSailOffsetGear1;
	extern TConfigVar< Float > passengerBoatCameraToSailOffsetGear2;
	extern TConfigVar< Float > passengerBoatCameraToSailOffsetGear3;

	extern TConfigVar< Float > passengerBoatFovAdjustCoef;
	extern TConfigVar< Float > passengerBoatDistanceAdjustCoef;
	extern TConfigVar< Float > passengerBoatPitchAdjustCoef;
	extern TConfigVar< Float > passengerBoatOffsetAdjustCoef;
	extern TConfigVar< Float > passengerBoatCameraToSailOffsetAdjustCoef;


    extern TConfigVar<Float> cvBoatHedgeRaycastMaxLength;
    extern TConfigVar<Float> cvBoatHedgeRaycastMinLength;
    extern TConfigVar<Float> cvBoatHedgeRaycastCutoffLength;
    extern TConfigVar<Float> cvBoatHedgeNormalNForceDamperSpeed;
    extern TConfigVar<Float> cvBoatHedgeInputDamperSpeed;
    extern TConfigVar<Float> cvBoatHedgeHardRepelPower;
    extern TConfigVar<Float> cvBoatHedgeSmoothRepelPower;
    extern TConfigVar<Float> cvBoatHedgeRepelForceMulti;
    extern TConfigVar<Float> cvBoatHedgeRepelTorqueMulti;

    extern TConfigVar<Float> cvBoatHedgeRayScaleX;
    extern TConfigVar<Float> cvBoatHedgeRayScaleY;
    extern TConfigVar<Float> cvBoatHedgeRayOffsetY;

    extern TConfigVar<Float> cvPhysxThreadDamperTime;
} // namespace BoatConfig

//////////////////////////////////////////////////////////////////////////

enum EBoatCollisionSource
{
    EBCS_PhysxCollider,
    EBCS_Hedgehog
};

//////////////////////////////////////////////////////////////////////////

struct SBoatGear
{
    SBoatGear()
        : maxSpeed( 0.0f )
        , acceleration( 0.0f )
        , deceleration( 0.0f )
    {}

    SBoatGear( Float maxspeed, Float accel, Float decel )
        : maxSpeed( maxspeed )
        , acceleration( accel )
        , deceleration( decel )
    {}

    Float maxSpeed;
    Float acceleration;
    Float deceleration;
};

//////////////////////////////////////////////////////////////////////////

struct SWaterContactPoint
{
    SWaterContactPoint();

    Vector	m_localOffset;
    Vector  m_globalPositionWaterOnW;
    Float   m_floatingHeight;
    Bool    m_isInAir;

    Float   m_drowningOffsetZ;
    Float   m_drowningTimeout;

    void SaveState( IGameSaver* saver );
    void LoadState( IGameLoader* saver );
};

//////////////////////////////////////////////////////////////////////////

struct SBoatSerialization
{
    Uint32          saveVersion;

    Vector			boatLinearVelocity;
    Vector			boatAngularVelocity;
    Vector			boatGlobalPosition;
	EulerAngles		boatGlobalRotation;
    Bool			boatIsDismountRequested;
    Float           boatDrowningShutdownTimeout;

    CFloatDamper    inputSpeedDamper;
    Float			inputSteerTargetAngle;
    Int32           inputCurrentGear;

    Bool			buoyancyIsDrowning;
    Float			buoyancyDrowningTimeSoFar;
    Vector2			buoyancyTiltAnglePerc;

    Bool			pathfindingIsEnabled;
    Float			pathfindingTimeOnCurve;
    Matrix			pathfindingCurveLocalToWorld;
    Vector			pathfindingNextCurveNodePos;
    Vector2			pathfindingNextCurveNodeTangent;
    Bool			pathfindingUseOutOfFrustumTeleport;
    Vector			pathfindingCurveEndPos;
    Vector			pathfindingCurveEndTangent;
    CName           pathfindingCurvesEntityTag;

    Bool            hedgeTerrainRescueMode;
    Matrix          hedgeLastWaterPose;
    CVectorDamper   hedgeRepelDamper;
    CFloatDamper    hedgeRaycastLengthDamper;
    CFloatDamper    hedgeInputDamper;
    TDynArray<Float>hedgeRaycastsDistances;

    SWaterContactPoint waterContactPoints[MAX_WATER_CONTACT_POINTS];
};

//////////////////////////////////////////////////////////////////////////
// DEBUG /////////////////////////////////////////////////////////////////
#ifndef FINAL

struct SPointNormal
{
    Vector hitpos;
    Vector normal;

    void Zero()
    {
        normal = Vector::ZEROS;
        hitpos = Vector::ZEROS;
    }
};

#endif
// DEBUG /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
