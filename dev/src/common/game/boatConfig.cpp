#include "build.h"
#include "boatConfig.h"

namespace BoatConfig
{
    TConfigVar<Bool>  cvBoatDrowningEnabled                       ( "Gameplay/Boat", "BoatDrowningEnabled",                             true,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatBuoyancyPointsSpreadX                 ( "Gameplay/Boat", "BoatBuoyancyPointsSpreadX",                       1.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatBuoyancyPointsSpreadY                 ( "Gameplay/Boat", "BoatBuoyancyPointsSpreadY",                       1.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatMassCenter                            ( "Gameplay/Boat", "BoatMassCenter",                                  0.9f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatMass                                  ( "Gameplay/Boat", "BoatMass",                                        23.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTiltXLimit                            ( "Gameplay/Boat", "BoatTiltXLimit",                                  30.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTiltYLimit                            ( "Gameplay/Boat", "BoatTiltYLimit",                                  30.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTorqueScaller                         ( "Gameplay/Boat", "BoatTorqueScaller",                               50.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatForceScaller                          ( "Gameplay/Boat", "BoatForceScaller",                                150.0f,  eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatForceMax						      ( "Gameplay/Boat", "BoatForceMax",                                    1000.0f, eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatFloatingHeight                        ( "Gameplay/Boat", "BoatFloatingHeight",                              -0.2f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatDrowningAcceleration                  ( "Gameplay/Boat", "BoatDrowningAcceleration",                        0.3f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatDrowningPropagationTime               ( "Gameplay/Boat", "BoatDrowningPopagationTime",                      3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatDrowningSystemsShutdownTime           ( "Gameplay/Boat", "BoatDrowningTime",                                8.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatDestructionHitsAcceptTimeout          ( "Gameplay/Boat", "BoatDestructionHitsAcceptTimeout",                3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatDestructionHitRepelForceMultiplier    ( "Gameplay/Boat", "BoatDestructionHitRepelForceMultiplier",          3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatBuoyancyWaterProbingPrecision         ( "Gameplay/Boat", "BoatBuoyancyWaterProbingPrecision",               0,       eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatBuoyancyFlyThresholdScaller           ( "Gameplay/Boat", "BoatBuoyancyFlyThresholdScaller",                 0.5f,    eConsoleVarFlag_Developer );    
    TConfigVar<Bool>  cvBoatSailingEnabled                        ( "Gameplay/Boat", "BoatSailingEnabled",                              true,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatSailingGearOneMaxSpeed                ( "Gameplay/Boat", "BoatSailingGearOneMaxSpeed",                      2.5f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearOneAcceleration            ( "Gameplay/Boat", "BoatSailingGearOneAcceleration",                  3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearOneDeceleration            ( "Gameplay/Boat", "BoatSailingGearOneDeceleration",                  4.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatSailingGearTwoMaxSpeed                ( "Gameplay/Boat", "BoatSailingGearTwoMaxSpeed",                      6.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearTwoAcceleration            ( "Gameplay/Boat", "BoatSailingGearTwoAcceleration",                  3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearTwoDeceleration            ( "Gameplay/Boat", "BoatSailingGearTwoDeceleration",                  4.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatSailingGearThreeMaxSpeed              ( "Gameplay/Boat", "BoatSailingGearThreeMaxSpeed",                    8.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearThreeAcceleration          ( "Gameplay/Boat", "BoatSailingGearThreeAcceleration",                3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearThreeDeceleration          ( "Gameplay/Boat", "BoatSailingGearThreeDeceleration",                4.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatSailingGearReverseMaxSpeed            ( "Gameplay/Boat", "BoatSailingGearReverseMaxSpeed",                  1.5f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearReverseAcceleration        ( "Gameplay/Boat", "BoatSailingGearReverseAcceleration",              3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatSailingGearReverseDeceleration        ( "Gameplay/Boat", "BoatSailingGearReverseDeceleration",              4.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatSailingForceScaller                   ( "Gameplay/Boat", "BoatSailingForceScaller",                         100.0f,  eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTurningZeroAngleForceCutoff           ( "Gameplay/Boat", "BoatTurningZeroAngleForceCutoff",                 0.7f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTurningZeroSpeedSteerAngle            ( "Gameplay/Boat", "BoatTurningZeroSpeedSteerAngle",                  40.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTurningFristSpeedSteerAngle           ( "Gameplay/Boat", "BoatTurningFristSpeedSteerAngle",                 22.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTurningSecondSpeedSteerAngle          ( "Gameplay/Boat", "BoatTurningSecondSpeedSteerAngle",                7.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTurningSteerRotationTime              ( "Gameplay/Boat", "BoatTurningSteerRotationTime",                    0.5f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTurningForcePosition                  ( "Gameplay/Boat", "BoatTurningForcePosition",                        -4.3f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatLinearDamping                         ( "Gameplay/Boat", "BoatLinearDamping",                               4.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatAngularDamping                        ( "Gameplay/Boat", "BoatAngularDamping",                              6.9f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatWaveClimbAngleThreshold1              ( "Gameplay/Boat", "BoatWaveClimbAngleThreshold1",                    10.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatWaveClimbAngleThreshold2              ( "Gameplay/Boat", "BoatWaveClimbAngleThreshold2",                    20.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatDoubleTapThreshold                    ( "Gameplay/Boat", "BoatDoubleTapThreshold",                          0.3f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatGearCandidateSwitchTimeout            ( "Gameplay/Boat", "BoatGearCandidateSwitchTimeout",                  0.15f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatPathFindingOutFrustumMaxDistance      ( "Gameplay/Boat", "BoatPathFindingOutFrustumMaxDistance",            60.0f,   eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatTiltingYSpeed                         ( "Gameplay/Boat", "BoatTiltingYSpeed",                               1.5f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTiltingMaxYTiltPercent                ( "Gameplay/Boat", "BoatTiltingMaxYTiltPercent",                      0.6f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTiltingMinYTiltPercent                ( "Gameplay/Boat", "BoatTiltingMinYTiltPercent",                      0.3f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatBuoyancyZDiffLimit                    ( "Gameplay/Boat", "BoatBuoyancyZDiffLimit2",                         1.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatForceUpBase                           ( "Gameplay/Boat", "BoatForceUpBase2",                                1.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatForceUpExpo                           ( "Gameplay/Boat", "BoatForceUpExpo2",                                2.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatForceDownBase                         ( "Gameplay/Boat", "BoatForceDownBase2",                              1.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatForceDownExpo                         ( "Gameplay/Boat", "BoatForceDownExpo2",                              2.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatTorqueUpBase                          ( "Gameplay/Boat", "BoatTorqueUpBase",                                5.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTorqueUpExpo                          ( "Gameplay/Boat", "BoatTorqueUpExpo",                                1.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatTorqueDownNormBase                    ( "Gameplay/Boat", "BoatTorqueDownNormBase",                          4.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTorqueDownNormExpo                    ( "Gameplay/Boat", "BoatTorqueDownNormExpo",                          1.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatPID_P							      ( "Gameplay/PID2", "P",                                               1200.0f, eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatPID_I							      ( "Gameplay/PID2", "I",                                               0.f,     eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatPID_D								  ( "Gameplay/PID2", "D",                                               500.f,   eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatTorqueDownInAirBase                   ( "Gameplay/Boat", "BoatTorqueDownInAirBase",                         4.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatTorqueDownInAirExpo                   ( "Gameplay/Boat", "BoatTorqueDownInAirExpo",                         1.5f,    eConsoleVarFlag_Developer );


    TConfigVar< Bool  > driverBoatLocalSpaceCamera                ( "Gameplay/BoatCamera", "BoatLocalSpaceCamera",                      false,   eConsoleVarFlag_Save      );
    TConfigVar< Float > driverBoatFovStand						  ( "Gameplay/BoatCamera", "BoatFovStand",                              45.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatFovReverse					  ( "Gameplay/BoatCamera", "BoatFovReverse",                            45.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatFovGear1						  ( "Gameplay/BoatCamera", "BoatFovGear1",                              47.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatFovGear2						  ( "Gameplay/BoatCamera", "BoatFovGear2",                              49.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatFovGear3						  ( "Gameplay/BoatCamera", "BoatFovGear3",                              51.0f,   eConsoleVarFlag_Developer );

    TConfigVar< Float > driverBoatDistanceStand					  ( "Gameplay/BoatCamera", "BoatDistanceStand",                         4.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatDistanceReverse				  ( "Gameplay/BoatCamera", "BoatDistanceReverse",                       15.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatDistanceGear1					  ( "Gameplay/BoatCamera", "BoatDistanceGear1",                         5.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatDistanceGear2					  ( "Gameplay/BoatCamera", "BoatDistanceGear2",                         6.5f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatDistanceGear3					  ( "Gameplay/BoatCamera", "BoatDistanceGear3",                         8.8f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > driverBoatPitchStand					  ( "Gameplay/BoatCamera", "BoatPitchStand",                            0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPitchReverse					  ( "Gameplay/BoatCamera", "BoatPitchReverse",                          -40.0f,  eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPitchGear1					  ( "Gameplay/BoatCamera", "BoatPitchGear1",                            0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPitchGear2					  ( "Gameplay/BoatCamera", "BoatPitchGear2",                            0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPitchGear3					  ( "Gameplay/BoatCamera", "BoatPitchGear3",                            0.0f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > driverBoatPivotOffsetUpReverse			  ( "Gameplay/BoatCamera", "BoatPivotOffsetUpReverse",                  0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPivotOffsetUpGear1			  ( "Gameplay/BoatCamera", "BoatPivotOffsetUpGear1",                    1.1f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPivotOffsetUpGear2			  ( "Gameplay/BoatCamera", "BoatPivotOffsetUpGear2",                    1.6f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPivotOffsetUpGear3			  ( "Gameplay/BoatCamera", "BoatPivotOffsetUpGear3",                    2.1f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > driverBoatCameraToSailOffsetStand		  ( "Gameplay/BoatCamera", "BoatCameraToSailOffsetStand",               0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatCameraToSailOffsetReverse		  ( "Gameplay/BoatCamera", "BoatCameraToSailOffsetReverse",             0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatCameraToSailOffsetGear1		  ( "Gameplay/BoatCamera", "BoatCameraToSailOffsetGear1",               1.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatCameraToSailOffsetGear2		  ( "Gameplay/BoatCamera", "BoatCameraToSailOffsetGear2",               1.6f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatCameraToSailOffsetGear3		  ( "Gameplay/BoatCamera", "BoatCameraToSailOffsetGear3",               2.2f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > driverBoatFovAdjustCoef					  ( "Gameplay/BoatCamera", "BoatFovAdjustCoef",                         1.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatDistanceAdjustCoef			  ( "Gameplay/BoatCamera", "BoatDistanceAdjustCoef",                    0.4f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatPitchAdjustCoef				  ( "Gameplay/BoatCamera", "BoatPitchAdjustCoef",                       1.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatOffsetAdjustCoef				  ( "Gameplay/BoatCamera", "BoatOffsetAdjustCoef",                      0.4f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > driverBoatCameraToSailOffsetAdjustCoef	  ( "Gameplay/BoatCamera", "BoatCameraToSailOffsetAdjustCoef",          1.0f,    eConsoleVarFlag_Developer );




    TConfigVar< Float > passengerBoatFovStand					  ( "Gameplay/BoatPassengerCamera", "BoatFovStand",                     45.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatFovReverse					  ( "Gameplay/BoatPassengerCamera", "BoatFovReverse",                   45.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatFovGear1					  ( "Gameplay/BoatPassengerCamera", "BoatFovGear1",                     47.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatFovGear2					  ( "Gameplay/BoatPassengerCamera", "BoatFovGear2",                     49.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatFovGear3					  ( "Gameplay/BoatPassengerCamera", "BoatFovGear3",                     51.0f,   eConsoleVarFlag_Developer );

    TConfigVar< Float > passengerBoatDistanceStand				  ( "Gameplay/BoatPassengerCamera", "BoatDistanceStand",                4.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatDistanceReverse			  ( "Gameplay/BoatPassengerCamera", "BoatDistanceReverse",              15.0f,   eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatDistanceGear1				  ( "Gameplay/BoatPassengerCamera", "BoatDistanceGear1",                5.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatDistanceGear2				  ( "Gameplay/BoatPassengerCamera", "BoatDistanceGear2",                6.5f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatDistanceGear3				  ( "Gameplay/BoatPassengerCamera", "BoatDistanceGear3",                8.8f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > passengerBoatPitchStand					  ( "Gameplay/BoatPassengerCamera", "BoatPitchStand",                   0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPitchReverse				  ( "Gameplay/BoatPassengerCamera", "BoatPitchReverse",                 -40.0f,  eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPitchGear1					  ( "Gameplay/BoatPassengerCamera", "BoatPitchGear1",                   0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPitchGear2					  ( "Gameplay/BoatPassengerCamera", "BoatPitchGear2",                   0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPitchGear3					  ( "Gameplay/BoatPassengerCamera", "BoatPitchGear3",                   0.0f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > passengerBoatPivotOffsetUpReverse		  ( "Gameplay/BoatPassengerCamera", "BoatPivotOffsetUpReverse",         0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPivotOffsetUpGear1			  ( "Gameplay/BoatPassengerCamera", "BoatPivotOffsetUpGear1",           1.1f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPivotOffsetUpGear2			  ( "Gameplay/BoatPassengerCamera", "BoatPivotOffsetUpGear2",           1.6f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPivotOffsetUpGear3			  ( "Gameplay/BoatPassengerCamera", "BoatPivotOffsetUpGear3",           2.1f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > passengerBoatCameraToSailOffsetStand	  ( "Gameplay/BoatPassengerCamera", "BoatCameraToSailOffsetStand",      0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatCameraToSailOffsetReverse	  ( "Gameplay/BoatPassengerCamera", "BoatCameraToSailOffsetReverse",    0.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatCameraToSailOffsetGear1	  ( "Gameplay/BoatPassengerCamera", "BoatCameraToSailOffsetGear1",      1.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatCameraToSailOffsetGear2	  ( "Gameplay/BoatPassengerCamera", "BoatCameraToSailOffsetGear2",      1.6f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatCameraToSailOffsetGear3	  ( "Gameplay/BoatPassengerCamera", "BoatCameraToSailOffsetGear3",      2.2f,    eConsoleVarFlag_Developer );

    TConfigVar< Float > passengerBoatFovAdjustCoef				  ( "Gameplay/BoatPassengerCamera", "BoatFovAdjustCoef",                1.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatDistanceAdjustCoef			  ( "Gameplay/BoatPassengerCamera", "BoatDistanceAdjustCoef",           0.4f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatPitchAdjustCoef			  ( "Gameplay/BoatPassengerCamera", "BoatPitchAdjustCoef",              1.0f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatOffsetAdjustCoef			  ( "Gameplay/BoatPassengerCamera", "BoatOffsetAdjustCoef",             0.4f,    eConsoleVarFlag_Developer );
    TConfigVar< Float > passengerBoatCameraToSailOffsetAdjustCoef ( "Gameplay/BoatPassengerCamera", "BoatCameraToSailOffsetAdjustCoef", 1.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatHedgeRaycastMaxLength    	          ( "Gameplay/Boat", "BoatHedgeRaycastLength",                          3.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatHedgeRaycastMinLength    	          ( "Gameplay/Boat", "BoatHedgeRaycastMinLength",                       1.8f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatHedgeRaycastCutoffLength 	          ( "Gameplay/Boat", "BoatHedgeRaycastCutoffLength",                    0.7f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatHedgeRepelForceMulti	              ( "Gameplay/Boat", "BoatHedgeRepelForceMulti",                        10.0f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatHedgeRepelTorqueMulti	              ( "Gameplay/Boat", "BoatHedgeRepelTorqueMulti",                       3000.0f, eConsoleVarFlag_Developer );


    TConfigVar<Float> cvBoatHedgeInputDamperSpeed      	          ( "Gameplay/Boat", "BoatHedgeInputDamperSpeed",                       0.2f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatHedgeNormalNForceDamperSpeed          ( "Gameplay/Boat", "BoatHedgeNormalNForceDamperSpeed",                0.2f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatHedgeHardRepelPower	                  ( "Gameplay/Boat", "BoatHedgeHardRepelPower",                         2.0f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatHedgeSmoothRepelPower	              ( "Gameplay/Boat", "BoatHedgeSmoothRepelPower",                       3.0f,    eConsoleVarFlag_Developer );

    TConfigVar<Float> cvBoatHedgeRayScaleX                        ( "Gameplay/Boat", "cvBoatHedgeRayScaleX",                            0.7f,    eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatHedgeRayScaleY                        ( "Gameplay/Boat", "cvBoatHedgeRayScaleY",                            0.88f,   eConsoleVarFlag_Developer );
    TConfigVar<Float> cvBoatHedgeRayOffsetY                       ( "Gameplay/Boat", "cvBoatHedgeRayOffsetY",                           0.14f,   eConsoleVarFlag_Developer );

    TConfigVar<Float> cvPhysxThreadDamperTime                     ( "Gameplay/Boat", "PhysxThreadDamperTime",                           0.2f,    eConsoleVarFlag_Developer );
};

//////////////////////////////////////////////////////////////////////////

SWaterContactPoint::SWaterContactPoint()
    : m_drowningOffsetZ( 0.0f )
    , m_drowningTimeout( 0.0f )
    , m_floatingHeight( 0.0f )
    , m_localOffset( Vector::ZEROS )
    , m_isInAir( false )
    , m_globalPositionWaterOnW( Vector::ZEROS )
{

}

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( waterContactPoint );
RED_DEFINE_STATIC_NAME( waterContactPointDrowningOffsetZ );
RED_DEFINE_STATIC_NAME( waterContactPointDrowningTimeout );
RED_DEFINE_STATIC_NAME( waterContactPointFloatingHeight );
RED_DEFINE_STATIC_NAME( waterContactPointLocalOffset );
RED_DEFINE_STATIC_NAME( waterContactPointIsInAir );
RED_DEFINE_STATIC_NAME( waterContactPointGlobalPositionWaterOnW );

//////////////////////////////////////////////////////////////////////////

void SWaterContactPoint::SaveState( IGameSaver* saver )
{
    saver->BeginBlock( CNAME(waterContactPoint) );
    {
        saver->WriteValue( CNAME(waterContactPointDrowningOffsetZ),        m_drowningOffsetZ        );
        saver->WriteValue( CNAME(waterContactPointDrowningTimeout),        m_drowningTimeout        );
        saver->WriteValue( CNAME(waterContactPointFloatingHeight),         m_floatingHeight         );
        saver->WriteValue( CNAME(waterContactPointLocalOffset),            m_localOffset            );
        saver->WriteValue( CNAME(waterContactPointIsInAir),                m_isInAir                );
        saver->WriteValue( CNAME(waterContactPointGlobalPositionWaterOnW), m_globalPositionWaterOnW );
    }
    saver->EndBlock( CNAME(waterContactPoint) );
}

//////////////////////////////////////////////////////////////////////////

void SWaterContactPoint::LoadState( IGameLoader* loader )
{
    loader->BeginBlock( CNAME(waterContactPoint) );
    {
        loader->ReadValue( CNAME(waterContactPointDrowningOffsetZ),        m_drowningOffsetZ        );
        loader->ReadValue( CNAME(waterContactPointDrowningTimeout),        m_drowningTimeout        );
        loader->ReadValue( CNAME(waterContactPointFloatingHeight),         m_floatingHeight         );
        loader->ReadValue( CNAME(waterContactPointLocalOffset),            m_localOffset            );
        loader->ReadValue( CNAME(waterContactPointIsInAir),                m_isInAir                );
        loader->ReadValue( CNAME(waterContactPointGlobalPositionWaterOnW), m_globalPositionWaterOnW );
    }
    loader->EndBlock( CNAME(waterContactPoint) );
}

//////////////////////////////////////////////////////////////////////////
