/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameplayConfig.h"
#include "../../common/game/expOracle.h"
#include "physicsCharacterWrapper.h"
#include "../core/configFileManager.h"
#include "behaviorIncludes.h"
#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"

IMPLEMENT_ENGINE_CLASS( SActorLODConfig );
IMPLEMENT_ENGINE_CLASS( SGameplayLODConfig );
IMPLEMENT_ENGINE_CLASS( SGameplayConfig );
IMPLEMENT_ENGINE_CLASS( SGameplayConfigLookAts );
IMPLEMENT_ENGINE_CLASS( SGameplayConfigGameCamera );


SGameplayConfigLookAts::SGameplayConfigLookAts() 
	: m_lookAtDurationGlance( 1.f )
	, m_lookAtDurationLook( 3.f )
	, m_lookAtDurationGaze( 7.f )
	, m_lookAtDurationStare( 16.f )
	, m_lookAtDurationRandGlance( 0.5f )
	, m_lookAtDurationRandLook( 1.f )
	, m_lookAtDurationRandGaze( 2.f )
	, m_lookAtDurationRandStare( 3.f )
	, m_lookAtRangeGlance( 160.f )
	, m_lookAtRangeLook( 160.f )
	, m_lookAtRangeGaze( 160.f )
	, m_lookAtRangeStare( 160.f )
	, m_lookAtSpeedGlance( 1.f )
	, m_lookAtSpeedLook( 1.f )
	, m_lookAtSpeedGaze( 1.f )
	, m_lookAtSpeedStare( 1.f )
	, m_lookAtSpeedRandGlance( 0.05f )
	, m_lookAtSpeedRandLook( 0.05f )
	, m_lookAtSpeedRandGaze( 0.05f )
	, m_lookAtSpeedRandStare( 0.05f )
	, m_lookAtAutoLimitGlance( true )
	, m_lookAtAutoLimitLook( false )
	, m_lookAtAutoLimitGaze( false )
	, m_lookAtAutoLimitStare( false )
	, m_lookAtDelay( 1.f )
	, m_lookAtDelayDialog( 0.f )
{

}

SGameplayConfigGameCamera::SGameplayConfigGameCamera()
	: m_verManualMax( 45.f )
	, m_verManualMin( 130.f )
	, m_horDamping( 200.f )
	, m_verDamping( 200.f )
	, m_pivotDamp( 300.f )
	, m_focusTargetDamp( 300.f )
	, m_focusActDuration( 1.f )
	, m_sensX( 250.f )
	, m_sensY( -150.f )
	, m_zoomActTime( 0.2f )
	, m_zoomDamp( 100.f )
	, m_verOffsetActTime( 0.2f )
	, m_verOffsetDamp( 100.f )
	, m_backOffsetDamp( 3.f )
	, m_collisionDampOn( 10.f )
	, m_collisionDampOff( 1000.f )
	, m_collisionBigRadius( 0.45f )
	, m_collisionBoxScale( 2.5f )
	, m_collisionAutoRotDamp( 10.f )
	, m_collisionAutoRotMaxSpeed( 1.f )
	, m_collisionVerCorrection( -60.f )
	, m_collisionPivotHeightOffset( 1.25f )
	, m_collisionPivotRadius( 1.5f )
	, m_collisionVerRadius( 0.5f )
	, m_collisionVerOffsetP( 0.5f )
	, m_collisionVerOffsetM( -0.35f )
	, m_collisionVerFactor( -10.f )
	, m_collisionVerRadiusP( 0.3f )
	, m_collisionVerRadiusM( 0.3f )
	, m_collisionAutoRotTrace( true )
	, m_collisionAutoRotTraceFactor( -0.2f )
	, m_indoorCollisionMaxZoom( 0.5f )
	, m_slopeVerFactor( 1.f )
	, m_slopeVerDamp( 10.f )
{}

SGameplayLODConfig::SGameplayLODConfig()
{
	// Actors

#define BEGIN_LOD( endDistance, deadZone ) \
	actorLOD.m_index = m_actorLODs.Size(); \
	actorLOD.m_distance = endDistance; \
	actorLOD.m_deadZone = deadZone;
#define END_LOD() \
	m_actorLODs.PushBack( actorLOD );

	SActorLODConfig actorLOD;

	BEGIN_LOD( 14.0f, 2.0f );
		actorLOD.m_suppressAnimatedComponentIfNotVisible = true;
		actorLOD.m_budgetAnimatedComponentTickIfNotVisible = true;
	END_LOD();

	BEGIN_LOD( 26.0f, 3.0f );
		actorLOD.m_enableDandles = false;
		actorLOD.m_mimicsQuality = 1;
		actorLOD.m_behaviorLOD = BL_Lod1;
		actorLOD.m_animatedComponentUpdateFrameSkip = 1;
	END_LOD();

	BEGIN_LOD( 50.0f, 4.0f );
		actorLOD.m_enableIK = false;
		actorLOD.m_animatedComponentUpdateFrameSkip = 2;
	END_LOD();

	BEGIN_LOD( 60.0f, 5.0f );
		actorLOD.m_budgetAnimatedComponentTick = true;
	END_LOD();

	BEGIN_LOD( 90.0f, 8.0f );
		actorLOD.m_animatedComponentUpdateFrameSkip = 3;
	END_LOD();

	BEGIN_LOD( -1.0f, 1.0f );
		actorLOD.m_mimicsQuality = 2;
		actorLOD.m_behaviorLOD = BL_Lod2;
		actorLOD.m_animatedComponentUpdateFrameSkip = 4;
		actorLOD.m_hide = true;
		actorLOD.m_suppressAnimatedComponent = true;
	END_LOD();

	m_actorInvisibilityTimeThreshold = 1.0f;

	// Components

	m_maxBudgetedComponentsTickTime = 0.002f;
	m_minBudgetedComponentsTickPercentage = 5;
	
	m_expectedNonBudgetedComponentsTickTime = 0.003f;

	// Non-actor components

	m_componentsTickLODUpdateTime = 0.0001f;
	m_componentsBudgetableTickDistance = 30.0f;
	m_componentsDisableTickDistance = 90.0f;

	// Entities

	m_entitiesBudgetableTickDistance = 30.0f;
	m_entitiesDisableTickDistance = 90.0f;
	m_entitiesTickTime = 0.002f;

	// Effects

	m_effectsBudgetableTickDistance = 25.0f;
	m_effectsTickTime = 0.0005f;
	m_effectsTickLODUpdateTime = 0.0001f;
}

SGameplayConfig::SGameplayConfig() 
	: m_useMultiTick( true )
	, m_horseProp( false )
	, m_horseCarSteer( true )
	, m_horseSpeedCtrl( true )
	, m_horseSpeedInc( 0.75f )
	, m_horseSpeedDec( 0.25f )
	, m_horseSpeedStep( 0.25f )
	, m_horseInputCooldown( 1.f )
	, m_horseStaminaDec( 0.1f )
	, m_horseStaminaInc( 0.05f )
	, m_horseStaminaCooldown( 3.f )
	, m_horseSpeedDecCooldown( 1.f )

	, m_playerPreviewInventory( 0 )

	, m_horsePathFactor( 500.f )
	, m_horsePathDamping( 10.f )
	, m_horseRoadSearchRadius( 5.f )
	, m_horseRoadSearchDistanceSlow( 8.f )
	, m_horseRoadSearchDistanceFast( 10.f )
	, m_horseRoadSelectionAngleCoeff( 0.2f )
	, m_horseRoadSelectionDistanceCoeff( 2.f )
	, m_horseRoadSelectionCurrentRoadPreferenceCoeff( 35.f )
	, m_horseRoadSelectionTurnAmountFeeCoeff( 2.f )
	, m_horseRoadFollowingCooldownTime( 2.f )
	, m_horseRoadFollowingCooldownDistance( 16.f )
    
	, m_debugA( 0.f )
	, m_debugB( 0.f )
	, m_debugC( 0.f )
	, m_debugD( 0.f )
	, m_debugE( 0.f )
	, m_debugF( 0.f )
	, m_debugG( 0.f )
    
	, m_interactionTestCameraRange( true )
	, m_interactionTestCameraRangeAngle( 80.f )
	, m_interactionTestPlayerRange( false )
	, m_interactionTestPlayerRangeAngle( 80.f )
	, m_interactionTestIsInPlayerRadius( false )
	, m_interactionTestPlayerRadius( 0.25f )

	, m_forceLookAtPlayer( false )
	, m_forceLookAtPlayerDist( 25.f )
	, m_forceBehaviorLod( false )
	, m_forceBehaviorLodLevel( BL_Lod0 )
	, m_useBehaviorLod( false )
#ifndef NO_MULTI_ANIMS
	, m_animationMultiUpdate( true )
#else
	, m_animationMultiUpdate( false )
#endif
	, m_animationAsyncUpdate( false )
	, m_animationAsyncJobs( true )
#ifndef NO_MULTI_ASYNC_ANIMS
	, m_animationCanUseAsyncJobs( true )
#else
	, m_animationCanUseAsyncJobs( false )
#endif
	, m_animationAsyncJobsUpdateFrustum( true )

	, m_logMissingAnimations( false )
	, m_logRequestedAnimations( false )
	, m_logSampledAnimations( false )

	, m_useWorkFreezing( true )
	, m_streamOnlyVisibleLayers( true )
	, m_workFreezingRadiusForInvisibleActors( 3.f )
	, m_workFreezingDelay( 0.5f )
	, m_workMaxFreezingTime( 5.f )
	, m_workResetInFreezing( true )
	, m_workSynchronization( false )
	, m_workAnimSpeedMulMin( 0.8f )
	, m_workAnimSpeedMulMax( 1.2f )
	, m_workMaxAnimOffset( 0.5f )

	, m_cameraHidePlayerDistMin( 0.35f )
	, m_cameraHidePlayerDistMax( 0.4f )
	, m_cameraHidePlayerSwordsDistMin( 0.9f )
	, m_cameraHidePlayerSwordsDistMax( 0.95f )
	, m_cameraHidePlayerSwordsAngleMin( 60.f )
	, m_cameraHidePlayerSwordsAngleMax( 70.f )
	, m_cameraPositionDamp( false )
	, m_cameraPositionDampLength( 0.2f )
	, m_cameraPositionDampLengthOffset( 0.05f )
	, m_cameraPositionDampSpeed( 7.f ) 
	, m_processNpcsAndCameraCollisions( true )

	, m_movementTraceOpt( true )
	, m_movementDeltaTestOpt( true )
	, m_movementSmoothing( 13.5f )
	, m_movementSmoothingOnHorse( 7.f )

	, m_physicsTerrainAdditionalElevation( 0.02f )
	, m_physicsTerrainThickness( -1.0f )
	, m_physicsKillingZVelocity( -15.f )
	, m_physicsTerrainDebugMaxSlope( SCCTDefaults::DEFAULT_MAX_SLOPE )
	, m_physicsTerrainDebugRange( 75.f )
	, m_physicsCollisionPredictionTime( 0.5f )
	, m_physicsCollisionPredictionSteps( 5 )
	, m_physicsCCTMaxDisp( 0.2f )

	, m_terrainInfluenceLimitMin( 0.3f )
	, m_terrainInfluenceLimitMax( 0.7f )
	, m_terrainInfluenceMul( 0.4f )
	, m_slidingLimitMin( 0.25f )
	, m_slidingLimitMax( 0.75f )
	, m_slidingDamping( 0.2f )
	, m_maxPlatformDisplacement( 0.3f )
	, m_probeTerrainOffset( 0.05f )
	, m_virtualRadiusTime( 1.0f )
	, m_fallingTime( 0.2f )
	, m_fallingMul( 1.2f )
	, m_jumpV0( 10.6f )
	, m_jumpTc( 2.55f )
	, m_jumpDelay( 0.18f )
	, m_jumpMinTime( 0.65f )
	, m_jumpGravityUp( -9.81f )
	, m_jumpGravityDown( -9.81f )
	, m_jumpMaxVertSpeed( -2.0f )
	, m_jumpLenMul( 1.0f )
	, m_jumpHeightMul( 1.0f )
	, m_movingSwimmingOffset( 1.55f )
	, m_emergeSpeed( 10.0f )
	, m_submergeSpeed( 20.0f )

	, m_showSegments( true )
	, m_showRotations( true )
	, m_showNodes( true )
	, m_curvePrecision( 25 )
	, m_timeScale( 1.0f )

	, m_actorOptUse( false )
    , m_actorOptDiv( 1 )

	, m_gcAfterCutscenesWithCamera( true )
	, m_gcAfterNotGameplayScenes( true )

	, m_autosaveCooldown( 40.f )
	, m_autosaveDelay( 1.f )

	, m_enableSimplePriorityLoadingInScenes( true )
	, m_enableMeshFlushInScenes( true )
	, m_enableSceneRewind( false )
	, m_enableTextureFlushInScenes( false )
	, m_enableAnimationFlushInScenes( false )
	, m_useFrozenFrameInsteadOfBlackscreen( false )
	, m_sceneIgnoreInputDuration( 1.f )

	, m_movementTorsoDamp( 50.f )
	, m_movementRotDamp( 1.f )

	, m_doNotClickMe( false )
	#ifndef RED_FINAL_BUILD
	, m_disableResetInput( false )
	#endif

	, m_idUseNewVoicePipeline( true )
	, m_woundDistanceWeight( 0.5f )
	, m_woundDirectionWeight( 0.1f )
	, m_strayActorDespawnDistance( 100.0f )
	, m_strayActorMaxHoursToKeep( 2 )
	, m_strayActorMaxActorsToKeep( 16 )
{
#ifndef RED_FINAL_BUILD
	m_enableSceneRewind = true;
#endif
}

void SGameplayConfig::Validate()
{
	m_interactionTestCameraRangeAngle = Clamp( m_interactionTestCameraRangeAngle, 0.f, 360.f );
	m_interactionTestPlayerRangeAngle = Clamp( m_interactionTestPlayerRangeAngle, 0.f, 360.f );
	m_interactionTestPlayerRadius = Clamp( m_interactionTestPlayerRangeAngle, 0.f, 1.f );
	m_workFreezingDelay = Clamp( m_workFreezingDelay, 0.25f, 1000.f );
	m_workMaxFreezingTime = Clamp( m_workMaxFreezingTime, 1.f, 1000.f );
	m_workFreezingRadiusForInvisibleActors = Clamp( m_workFreezingRadiusForInvisibleActors, 0.0f, 100.f );
	m_cameraHidePlayerDistMin = Clamp( m_cameraHidePlayerDistMin, 0.0f, 0.5f );
	m_cameraHidePlayerDistMax = Clamp( m_cameraHidePlayerDistMax, 0.0f, 0.5f );
	m_cameraHidePlayerSwordsDistMin = Clamp( m_cameraHidePlayerSwordsDistMin, 0.0f, 10.0f );
	m_cameraHidePlayerSwordsDistMax = Clamp( m_cameraHidePlayerSwordsDistMax, 0.0f, 10.0f );
	m_cameraHidePlayerSwordsAngleMin = Clamp( m_cameraHidePlayerSwordsAngleMin, 0.0f, 180.0f );
	m_cameraHidePlayerSwordsAngleMax = Clamp( m_cameraHidePlayerSwordsAngleMax, 0.0f, 180.0f );
}

void SGameplayConfig::Load()
{
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("State"), m_horseProp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("CarSteer"), m_horseCarSteer );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedCtrl"), m_horseSpeedCtrl );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedInc"), m_horseSpeedInc );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedDec"), m_horseSpeedDec );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedStep"), m_horseSpeedStep );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseInputCooldown"), m_horseInputCooldown );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseStaminaDec"), m_horseStaminaDec );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseStaminaInc"), m_horseStaminaInc );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseStaminaCooldown"), m_horseStaminaCooldown );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedDecCooldown"), m_horseSpeedDecCooldown );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Player"), TXT("m_playerPreviewInventory"), m_playerPreviewInventory );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horsePathFactor"), m_horsePathFactor );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horsePathDamping"), m_horsePathDamping );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSearchRadius"), m_horseRoadSearchRadius );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSearchDistanceSlow"), m_horseRoadSearchDistanceSlow );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSearchDistanceFast"), m_horseRoadSearchDistanceFast );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionAngleCoeff"), m_horseRoadSelectionAngleCoeff );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionDistanceCoeff"), m_horseRoadSelectionDistanceCoeff );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionCurrentRoadPreferenceCoeff"), m_horseRoadSelectionCurrentRoadPreferenceCoeff );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionTurnAmountFeeCoeff"), m_horseRoadSelectionTurnAmountFeeCoeff );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadFollowingCooldownTime"), m_horseRoadFollowingCooldownTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadFollowingCooldownDistance"), m_horseRoadFollowingCooldownDistance );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestCameraRange"), m_interactionTestCameraRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestCameraRangeAngle"), m_interactionTestCameraRangeAngle );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestPlayerRange"), m_interactionTestPlayerRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestPlayerRangeAngle"), m_interactionTestPlayerRangeAngle );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestIsInPlayerRadius"), m_interactionTestIsInPlayerRadius );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestPlayerRadius"), m_interactionTestPlayerRadius );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Reactions"), TXT("ForceLookAtPlayer"), m_forceLookAtPlayer );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Reactions"), TXT("ForceLookAtPlayerDist"), m_forceLookAtPlayerDist );		

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainInfluenceLimitMin"), m_terrainInfluenceLimitMin );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainInfluenceLimitMax"), m_terrainInfluenceLimitMax );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainInfluenceMul"), m_terrainInfluenceMul );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("SlidingLimitMin"), m_slidingLimitMin );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("SlidingLimitMax"), m_slidingLimitMax );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("SlidingDamping"), m_slidingDamping );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainAdditionalElevation"), m_physicsTerrainAdditionalElevation );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainThickness"), m_physicsTerrainThickness );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainDebugMaxSlope"), m_physicsTerrainDebugMaxSlope );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainDebugRange"), m_physicsTerrainDebugRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("CollisionPredictionTime"), m_physicsCollisionPredictionTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("CollisionPredictionSteps"), m_physicsCollisionPredictionSteps );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("CCTMaxDisp"), m_physicsCCTMaxDisp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("VirtualRadiusTime"), m_virtualRadiusTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("Moving-SwimmingOffset"), m_movingSwimmingOffset );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("EmergingSpeed"), m_emergeSpeed );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Physics"), TXT("SubmergingSpeed"), m_submergeSpeed );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Behavior"), TXT("UseBehaviorLod"), m_useBehaviorLod );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Behavior"), TXT("ForceBehaviorLod"), m_forceBehaviorLod );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Behavior"), TXT("ForceBehaviorLodLevel"), m_forceBehaviorLodLevel );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Behavior"), TXT("LogMissingAnimations"), m_logMissingAnimations );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Behavior"), TXT("LogRequestedAnimations"), m_logRequestedAnimations );	
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Behavior"), TXT("LogSampledAnimations"), m_logSampledAnimations );		

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Work"), TXT("UseWorkFreezing"), m_useWorkFreezing );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Work"), TXT("WorkResetInFreezing"), m_workResetInFreezing );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Work"), TXT("WorkSynchronization"), m_workSynchronization );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Work"), TXT("WorkFreezingRadiusForInvisibleActors"), m_workFreezingRadiusForInvisibleActors );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Work"), TXT("WorkFreezingDelay"), m_workFreezingDelay );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Work"), TXT("WorkMaxFreezingTime"), m_workMaxFreezingTime );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationGlance"), m_lookAtConfig.m_lookAtDurationGlance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationLook"), m_lookAtConfig.m_lookAtDurationLook );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationGaze"), m_lookAtConfig.m_lookAtDurationGaze );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationStare"), m_lookAtConfig.m_lookAtDurationStare );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandGlance"), m_lookAtConfig.m_lookAtDurationRandGlance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandLook"), m_lookAtConfig.m_lookAtDurationRandLook );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandGaze"), m_lookAtConfig.m_lookAtDurationRandGaze );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandStare"), m_lookAtConfig.m_lookAtDurationRandStare );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeGlance"), m_lookAtConfig.m_lookAtRangeGlance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeLook"), m_lookAtConfig.m_lookAtRangeLook );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeGaze"), m_lookAtConfig.m_lookAtRangeGaze );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeStare"), m_lookAtConfig.m_lookAtRangeStare );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedGlance"), m_lookAtConfig.m_lookAtSpeedGlance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedLook"), m_lookAtConfig.m_lookAtSpeedLook );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedGaze"), m_lookAtConfig.m_lookAtSpeedGaze );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedStare"), m_lookAtConfig.m_lookAtSpeedStare );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandGlance"), m_lookAtConfig.m_lookAtSpeedRandGlance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandLook"), m_lookAtConfig.m_lookAtSpeedRandLook );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandGaze"), m_lookAtConfig.m_lookAtSpeedRandGaze );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandStare"), m_lookAtConfig.m_lookAtSpeedRandStare );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitGlance"), m_lookAtConfig.m_lookAtAutoLimitGlance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitLook "), m_lookAtConfig.m_lookAtAutoLimitLook );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitGaze"), m_lookAtConfig.m_lookAtAutoLimitGaze );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitStare"), m_lookAtConfig.m_lookAtAutoLimitStare );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDelay"), m_lookAtConfig.m_lookAtDelay );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDelayDialog"), m_lookAtConfig.m_lookAtDelayDialog );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerDistMin"), m_cameraHidePlayerDistMin );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerDistMax"), m_cameraHidePlayerDistMax );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsDistMin"), m_cameraHidePlayerSwordsDistMin );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsDistMax"), m_cameraHidePlayerSwordsDistMax );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsAngleMin"), m_cameraHidePlayerSwordsAngleMin );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsAngleMax"), m_cameraHidePlayerSwordsAngleMax );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDamp"), m_cameraPositionDamp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDampLength"), m_cameraPositionDampLength );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDampLengthOffset"), m_cameraPositionDampLengthOffset );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDampSpeed"), m_cameraPositionDampSpeed );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verManualMin"), m_gameCamera.m_verManualMin );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verManualMax"), m_gameCamera.m_verManualMax );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_horDamping"), m_gameCamera.m_horDamping );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verDamping"), m_gameCamera.m_verDamping );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_pivotDamp"), m_gameCamera.m_pivotDamp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_focusTargetDamp"), m_gameCamera.m_focusTargetDamp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_focusActDuration"), m_gameCamera.m_focusActDuration );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_zoomDamp"), m_gameCamera.m_zoomDamp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_zoomActTime"), m_gameCamera.m_zoomActTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verOffsetDamp"), m_gameCamera.m_verOffsetDamp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verOffsetActTime"), m_gameCamera.m_verOffsetActTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_sensX3"), m_gameCamera.m_sensX );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_sensY3"), m_gameCamera.m_sensY );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_backOffsetDamp"), m_gameCamera.m_backOffsetDamp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionDampOn"), m_gameCamera.m_collisionDampOn );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionDampOff2"), m_gameCamera.m_collisionDampOff );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionBigRadius"), m_gameCamera.m_collisionBigRadius );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionPivotHeightOffset"), m_gameCamera.m_collisionPivotHeightOffset );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionPivotRadius"), m_gameCamera.m_collisionPivotRadius );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerRadius"), m_gameCamera.m_collisionVerRadius );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerOffsetP"), m_gameCamera.m_collisionVerOffsetP );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerOffsetM"), m_gameCamera.m_collisionVerOffsetM );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerFactor"), m_gameCamera.m_collisionVerFactor );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerRadiusP"), m_gameCamera.m_collisionVerRadiusP );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerRadiusM"), m_gameCamera.m_collisionVerRadiusM );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionAutoRotTrace"), m_gameCamera.m_collisionAutoRotTrace );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionAutoRotTraceFactor"), m_gameCamera.m_collisionAutoRotTraceFactor );	
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_indoorCollisionMaxZoom"), m_gameCamera.m_indoorCollisionMaxZoom );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Combat"), TXT("m_movementRotDamp"), m_movementRotDamp );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Combat"), TXT("m_movementTorsoDamp"), m_movementTorsoDamp );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugA"), m_debugA );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugB"), m_debugB );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugC"), m_debugC );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugD"), m_debugD );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugE"), m_debugE );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugF"), m_debugF );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugG"), m_debugG );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("save"), TXT("m_autosaveCooldown"), m_autosaveCooldown );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("save"), TXT("m_autosaveDelay"), m_autosaveDelay );

#ifndef RED_FINAL_BUILD
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_disableResetInput"), m_disableResetInput );
#endif

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_enableSceneRewind"), m_enableSceneRewind );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_useFrozenFrameInsteadOfBlackscreen"), m_useFrozenFrameInsteadOfBlackscreen );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Debug"), TXT("m_sceneIgnoreInputDuration"), m_sceneIgnoreInputDuration );
	

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Movement"), TXT("m_movementSmoothing"), m_movementSmoothing );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Movement"), TXT("m_movementSmoothingOnHorse"), m_movementSmoothingOnHorse );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("ID"), TXT("m_idUseNewVoicePipeline"), m_idUseNewVoicePipeline );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Wounds"), TXT("m_woundDistanceWeight"), m_woundDistanceWeight );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Wounds"), TXT("m_woundDirectionWeight"), m_woundDirectionWeight );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Despawn"), TXT("m_strayActorDespawnDistance"), m_strayActorDespawnDistance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Despawn"), TXT("m_strayActorMaxHoursToKeep"), m_strayActorMaxHoursToKeep );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Despawn"), TXT("m_strayActorMaxActorsToKeep"), m_strayActorMaxActorsToKeep );

    //////////////////////////////////////////////////////////////////////////

	ReadExplorationsDesc();
	ReadLODDesc();
}

void SGameplayConfig::Save()
{
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("State"), m_horseProp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("CarSteer"), m_horseCarSteer );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedCtrl"), m_horseSpeedCtrl );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedInc"), m_horseSpeedInc );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedDec"), m_horseSpeedDec );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedStep"), m_horseSpeedStep );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseInputCooldown"), m_horseInputCooldown );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseStaminaDec"), m_horseStaminaDec );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseStaminaInc"), m_horseStaminaInc );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseStaminaCooldown"), m_horseStaminaCooldown );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse2"), TXT("m_horseSpeedDecCooldown"), m_horseSpeedDecCooldown );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Player"), TXT("m_playerPreviewInventory"), m_playerPreviewInventory );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horsePathFactor"), m_horsePathFactor );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horsePathDamping"), m_horsePathDamping );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseExplorationRadius"), m_horseRoadSearchRadius );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSearchDistanceSlow"), m_horseRoadSearchDistanceSlow );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSearchDistanceFast"), m_horseRoadSearchDistanceFast );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionAngleCoeff"), m_horseRoadSelectionAngleCoeff );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionDistanceCoeff"), m_horseRoadSelectionDistanceCoeff );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionCurrentRoadPreferenceCoeff"), m_horseRoadSelectionCurrentRoadPreferenceCoeff );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadSelectionTurnAmountFeeCoeff"), m_horseRoadSelectionTurnAmountFeeCoeff );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadFollowingCooldownTime"), m_horseRoadFollowingCooldownTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Horse"), TXT("m_horseRoadFollowingCooldownDistance"), m_horseRoadFollowingCooldownDistance );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestCameraRange"), m_interactionTestCameraRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestCameraRangeAngle"), m_interactionTestCameraRangeAngle );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestPlayerRange"), m_interactionTestPlayerRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestPlayerRangeAngle"), m_interactionTestPlayerRangeAngle );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestIsInPlayerRadius"), m_interactionTestIsInPlayerRadius );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Targetowanie"), TXT("TestPlayerRadius"), m_interactionTestPlayerRadius );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Reactions"), TXT("ForceLookAtPlayer"), m_forceLookAtPlayer );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Reactions"), TXT("ForceLookAtPlayerDist"), m_forceLookAtPlayerDist );		 

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainInfluenceLimitMin"), m_terrainInfluenceLimitMin );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainInfluenceLimitMax"), m_terrainInfluenceLimitMax );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainInfluenceMul"), m_terrainInfluenceMul );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("SlidingLimitMin"), m_slidingLimitMin );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("SlidingLimitMax"), m_slidingLimitMax );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("SlidingDamping"), m_slidingDamping );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainAdditionalElevation"), m_physicsTerrainAdditionalElevation );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainThickness"), m_physicsTerrainThickness );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainDebugMaxSlope"), m_physicsTerrainDebugMaxSlope );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("TerrainDebugRange"), m_physicsTerrainDebugRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("CollisionPredictionTime"), m_physicsCollisionPredictionTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("CollisionPredictionSteps"), m_physicsCollisionPredictionSteps );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("CCTMaxDisp"), m_physicsCCTMaxDisp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("VirtualRadiusTime"), m_virtualRadiusTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("Moving-SwimmingOffset"), m_movingSwimmingOffset );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("EmergingSpeed"), m_emergeSpeed );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Physics"), TXT("SubmergingSpeed"), m_submergeSpeed );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Behavior"), TXT("UseBehaviorLod"), m_useBehaviorLod );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Behavior"), TXT("ForceBehaviorLod"), m_forceBehaviorLod );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Behavior"), TXT("ForceBehaviorLodLevel"), m_forceBehaviorLodLevel );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Behavior"), TXT("LogMissingAnimations"), m_logMissingAnimations );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Behavior"), TXT("LogRequestedAnimations"), m_logRequestedAnimations );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Behavior"), TXT("LogSampledAnimations"), m_logSampledAnimations );		

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Work"), TXT("UseWorkFreezing"), m_useWorkFreezing );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Work"), TXT("WorkResetInFreezing"), m_workResetInFreezing );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Work"), TXT("WorkSynchronization"), m_workSynchronization );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Work"), TXT("WorkFreezingRadiusForInvisibleActors"), m_workFreezingRadiusForInvisibleActors );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Work"), TXT("WorkFreezingDelay"), m_workFreezingDelay );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Work"), TXT("WorkMaxFreezingTime"), m_workMaxFreezingTime );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationGlance"), m_lookAtConfig.m_lookAtDurationGlance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationLook"), m_lookAtConfig.m_lookAtDurationLook );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationGaze"), m_lookAtConfig.m_lookAtDurationGaze );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationStare"), m_lookAtConfig.m_lookAtDurationStare );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandGlance"), m_lookAtConfig.m_lookAtDurationRandGlance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandLook"), m_lookAtConfig.m_lookAtDurationRandLook );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandGaze"), m_lookAtConfig.m_lookAtDurationRandGaze );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDurationRandStare"), m_lookAtConfig.m_lookAtDurationRandStare );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeGlance"), m_lookAtConfig.m_lookAtRangeGlance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeLook"), m_lookAtConfig.m_lookAtRangeLook );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeGaze"), m_lookAtConfig.m_lookAtRangeGaze );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtRangeStare"), m_lookAtConfig.m_lookAtRangeStare );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedGlance"), m_lookAtConfig.m_lookAtSpeedGlance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedLook"), m_lookAtConfig.m_lookAtSpeedLook );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedGaze"), m_lookAtConfig.m_lookAtSpeedGaze );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedStare"), m_lookAtConfig.m_lookAtSpeedStare );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandGlance"), m_lookAtConfig.m_lookAtSpeedRandGlance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandLook"), m_lookAtConfig.m_lookAtSpeedRandLook );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandGaze"), m_lookAtConfig.m_lookAtSpeedRandGaze );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtSpeedRandStare"), m_lookAtConfig.m_lookAtSpeedRandStare );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitGlance"), m_lookAtConfig.m_lookAtAutoLimitGlance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitLook "), m_lookAtConfig.m_lookAtAutoLimitLook );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitGaze"), m_lookAtConfig.m_lookAtAutoLimitGaze );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtAutoLimitStare"), m_lookAtConfig.m_lookAtAutoLimitStare );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDelay"), m_lookAtConfig.m_lookAtDelay );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("LookAt"), TXT("LookAtDelayDialog"), m_lookAtConfig.m_lookAtDelayDialog );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerDistMin"), m_cameraHidePlayerDistMin );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerDistMax"), m_cameraHidePlayerDistMax );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsDistMin"), m_cameraHidePlayerSwordsDistMin );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsDistMax"), m_cameraHidePlayerSwordsDistMax );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsAngleMin"), m_cameraHidePlayerSwordsAngleMin );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("HidePlayerSwordsAngleMax"), m_cameraHidePlayerSwordsAngleMax );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDamp"), m_cameraPositionDamp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDampLength"), m_cameraPositionDampLength );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDampLengthOffset"), m_cameraPositionDampLengthOffset );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Camera"), TXT("PositionDampSpeed"), m_cameraPositionDampSpeed );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verManualMin"), m_gameCamera.m_verManualMin );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verManualMax"), m_gameCamera.m_verManualMax );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_horDamping"), m_gameCamera.m_horDamping );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verDamping"), m_gameCamera.m_verDamping );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_pivotDamp"), m_gameCamera.m_pivotDamp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_focusTargetDamp"), m_gameCamera.m_focusTargetDamp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_focusActDuration"), m_gameCamera.m_focusActDuration );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_zoomDamp"), m_gameCamera.m_zoomDamp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_zoomActTime"), m_gameCamera.m_zoomActTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verOffsetDamp"), m_gameCamera.m_verOffsetDamp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_verOffsetActTime"), m_gameCamera.m_verOffsetActTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_sensX3"), m_gameCamera.m_sensX );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_sensY3"), m_gameCamera.m_sensY );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_backOffsetDamp"), m_gameCamera.m_backOffsetDamp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionDampOn"), m_gameCamera.m_collisionDampOn );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionDampOff2"), m_gameCamera.m_collisionDampOff );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionBigRadius"), m_gameCamera.m_collisionBigRadius );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionPivotHeightOffset"), m_gameCamera.m_collisionPivotHeightOffset );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionPivotRadius"), m_gameCamera.m_collisionPivotRadius );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerRadius"), m_gameCamera.m_collisionVerRadius );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerOffsetP"), m_gameCamera.m_collisionVerOffsetP );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerOffsetM"), m_gameCamera.m_collisionVerOffsetM );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerFactor"), m_gameCamera.m_collisionVerFactor );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerRadiusP"), m_gameCamera.m_collisionVerRadiusP );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionVerRadiusM"), m_gameCamera.m_collisionVerRadiusM );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionAutoRotTrace"), m_gameCamera.m_collisionAutoRotTrace );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_collisionAutoRotTraceFactor"), m_gameCamera.m_collisionAutoRotTraceFactor );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("GameCamera5"), TXT("m_indoorCollisionMaxZoom"), m_gameCamera.m_indoorCollisionMaxZoom );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Combat"), TXT("m_movementRotDamp"), m_movementRotDamp );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Combat"), TXT("m_movementTorsoDamp"), m_movementTorsoDamp );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugA"), m_debugA );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugB"), m_debugB );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugC"), m_debugC );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugD"), m_debugD );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugE"), m_debugE );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugF"), m_debugF );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_debugG"), m_debugG );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("save"), TXT("m_autosaveCooldown"), m_autosaveCooldown );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("save"), TXT("m_autosaveDelay"), m_autosaveDelay );

#ifndef RED_FINAL_BUILD
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_disableResetInput"), m_disableResetInput );
#endif

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_enableSceneRewind"), m_enableSceneRewind );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_useFrozenFrameInsteadOfBlackscreen"), m_useFrozenFrameInsteadOfBlackscreen );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Debug"), TXT("m_sceneIgnoreInputDuration"), m_sceneIgnoreInputDuration );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Movement"), TXT("m_movementSmoothing"), m_movementSmoothing );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Movement"), TXT("m_movementSmoothingOnHorse"), m_movementSmoothingOnHorse );
	
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("ID"), TXT("m_idUseNewVoicePipeline"), m_idUseNewVoicePipeline );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Wounds"), TXT("m_woundDistanceWeight"), m_woundDistanceWeight );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Wounds"), TXT("m_woundDirectionWeight"), m_woundDirectionWeight );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Despawn"), TXT("m_strayActorDespawnDistance"), m_strayActorDespawnDistance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Despawn"), TXT("m_strayActorMaxHoursToKeep"), m_strayActorMaxHoursToKeep );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Despawn"), TXT("m_strayActorMaxActorsToKeep"), m_strayActorMaxActorsToKeep );
    //////////////////////////////////////////////////////////////////////////

	WriteExplorationsDesc();
	WriteLODDesc();
}

void SGameplayConfig::ReadLODDesc()
{
	TDynArray< SActorLODConfig > actorLODs;
	Float distance, deadZone;
	String actorLODGroup = TXT("Actor_0");
	while ( SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("Distance"), distance ) &&
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("DeadZone"), deadZone ) )
	{
		SActorLODConfig actorLOD;
		actorLOD.m_distance = distance;
		actorLOD.m_deadZone = deadZone;

		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("Hide"), actorLOD.m_hide );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("EnableIK"), actorLOD.m_enableIK );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("MimicsQuality"), actorLOD.m_mimicsQuality );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("EnableDandles"), actorLOD.m_enableDandles );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("BehaviorLOD"), actorLOD.m_behaviorLOD );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("AnimatedComponentUpdateFrameSkip"), actorLOD.m_animatedComponentUpdateFrameSkip );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("SuppressAnimatedComponent"), actorLOD.m_suppressAnimatedComponent );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("BudgetAnimatedComponentTick"), actorLOD.m_budgetAnimatedComponentTick );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("SuppressAnimatedComponentIfNotVisible"), actorLOD.m_suppressAnimatedComponentIfNotVisible );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), actorLODGroup.AsChar(), TXT("BudgetAnimatedComponentTickIfNotVisible"), actorLOD.m_budgetAnimatedComponentTickIfNotVisible );

		actorLODs.PushBack( actorLOD );

		actorLODGroup = String::Printf( TXT("Actor_%u"), m_LOD.m_actorLODs.Size() );
	}
	if ( !actorLODs.Empty() )
	{
		m_LOD.m_actorLODs = actorLODs;

		// Force LOD 0 to be fully ON

		m_LOD.m_actorLODs[ 0 ] = SActorLODConfig();
		m_LOD.m_actorLODs[ 0 ].m_distance = actorLODs[ 0 ].m_distance;
		m_LOD.m_actorLODs[ 0 ].m_deadZone = actorLODs[ 0 ].m_deadZone;

		// Assign correct LOD indices

		for ( Uint32 i = 0; i < m_LOD.m_actorLODs.Size(); ++i )
		{
			m_LOD.m_actorLODs[ i ].m_index = i;
		}
	}

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Actors"), TXT("InvisibilityTimeThreshold"), m_LOD.m_actorInvisibilityTimeThreshold );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Components"), TXT("TickLODUpdateTime"), m_LOD.m_componentsTickLODUpdateTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Components"), TXT("BudgetableTickDistance"), m_LOD.m_componentsBudgetableTickDistance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Components"), TXT("DisableTickDistance"), m_LOD.m_componentsDisableTickDistance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Components"), TXT("MaxBudgetedTickTime"), m_LOD.m_maxBudgetedComponentsTickTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Components"), TXT("MinBudgetedTickPercentage"), m_LOD.m_minBudgetedComponentsTickPercentage );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Effects"), TXT("BudgetableTickDistance"), m_LOD.m_effectsBudgetableTickDistance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Effects"), TXT("TickTime"), m_LOD.m_effectsTickTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Effects"), TXT("TickLODUpdateTime"), m_LOD.m_effectsTickLODUpdateTime );

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Entities"), TXT("TickTime"), m_LOD.m_entitiesTickTime );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Entities"), TXT("BudgetableTickDistance"), m_LOD.m_entitiesBudgetableTickDistance );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("lod"), TXT("Entities"), TXT("DisableTickDistance"), m_LOD.m_entitiesDisableTickDistance );
}

void SGameplayConfig::WriteLODDesc()
{
	for ( Uint32 i = 0; i < m_LOD.m_actorLODs.Size(); ++i )
	{
		const SActorLODConfig& actorLOD = m_LOD.m_actorLODs[ i ];

		const String actorLODGroup = String::Printf( TXT("Actor_%u"), i );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("Distance"), actorLOD.m_distance );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("DeadZone"), actorLOD.m_deadZone );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("Hide"), actorLOD.m_hide );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("EnableIK"), actorLOD.m_enableIK );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("MimicsQuality"), actorLOD.m_mimicsQuality );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("EnableDandles"), actorLOD.m_enableDandles );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("BehaviorLOD"), actorLOD.m_behaviorLOD );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("AnimatedComponentUpdateFrameSkip"), actorLOD.m_animatedComponentUpdateFrameSkip );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("SuppressAnimatedComponent"), actorLOD.m_suppressAnimatedComponent );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("BudgetAnimatedComponentTick"), actorLOD.m_budgetAnimatedComponentTick );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("SuppressAnimatedComponentIfNotVisible"), actorLOD.m_suppressAnimatedComponentIfNotVisible );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), actorLODGroup.AsChar(), TXT("BudgetAnimatedComponentTickIfNotVisible"), actorLOD.m_budgetAnimatedComponentTickIfNotVisible );
	}

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Actors"), TXT("InvisibilityTimeThreshold"), m_LOD.m_actorInvisibilityTimeThreshold );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Components"), TXT("TickLODUpdateTime"), m_LOD.m_componentsTickLODUpdateTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Components"), TXT("BudgetableTickDistance"), m_LOD.m_componentsBudgetableTickDistance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Components"), TXT("DisableTickDistance"), m_LOD.m_componentsDisableTickDistance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Components"), TXT("MaxBudgetedTickTime"), m_LOD.m_maxBudgetedComponentsTickTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Components"), TXT("MinBudgetedTickPercentage"), m_LOD.m_minBudgetedComponentsTickPercentage );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Effects"), TXT("BudgetableTickDistance"), m_LOD.m_effectsBudgetableTickDistance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Effects"), TXT("TickTime"), m_LOD.m_effectsTickTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Effects"), TXT("TickLODUpdateTime"), m_LOD.m_effectsTickLODUpdateTime );

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Entities"), TXT("TickTime"), m_LOD.m_entitiesTickTime );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Entities"), TXT("BudgetableTickDistance"), m_LOD.m_entitiesBudgetableTickDistance );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("lod"), TXT("Entities"), TXT("DisableTickDistance"), m_LOD.m_entitiesDisableTickDistance );
}

void SGameplayConfig::ReadExplorationsDesc()
{
	const CEnum* e = SRTTI::GetInstance().FindEnum( CNAME( EExplorationType ) );
	const CEnum* zTest = SRTTI::GetInstance().FindEnum( CNAME( ExpZComparision ) );
	const CEnum* doubleSidedEnum = SRTTI::GetInstance().FindEnum( CNAME( ExpDoubleSided ) );
	if ( !e )
	{
		return;
	}

	const TDynArray< CName >& options = e->GetOptions();

	m_explorationsDesc.Resize( options.Size() );

	for ( Uint32 i=0; i<options.Size(); ++i )
	{
		const CName& option = options[ i ];

		m_explorationsDesc[ i ] = new EdExplorationDesc();
		
		EdExplorationDesc& desc = *(m_explorationsDesc[ i ]);

		String boneL, boneR;
		String testName;
		String doubleSidedName;
		String raiseBehaviorEventAtEnd;
		String callScriptEventAtEnd;

		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_horizontal"), desc.m_horizontal );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_doubleSided"), doubleSidedName );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_boneLeft"), boneL );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_boneRight"), boneR );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_edgeOffset"), desc.m_edgeOffset );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_offsetMS"), desc.m_offsetMS );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_edgeYawOffset"), desc.m_edgeYawOffset );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_useEdgeOffsetX"), desc.m_useEdgeOffsetX );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_useEdgeOffsetY"), desc.m_useEdgeOffsetY );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_useEdgeOffsetZ"), desc.m_useEdgeOffsetZ );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_useMotionExX"), desc.m_useMotionExX );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_useMotionExY"), desc.m_useMotionExY );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_useMotionExZ"), desc.m_useMotionExZ );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_edgeStartPointOffset"), desc.m_edgeStartPointOffset );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_edgeEndPointOffset"), desc.m_edgeEndPointOffset );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_edgeGranularity"), desc.m_edgeGranularity );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_alignTransToEdge"), desc.m_alignTransToEdge );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_alignRotToEdge"), desc.m_alignRotToEdge );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_alignRotToEdgeExceeding"), desc.m_alignRotToEdgeExceeding );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_blendIn"), desc.m_blendIn );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_blendOut"), desc.m_blendOut );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_earlyEndOffset"), desc.m_earlyEndOffset );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_testDist"), desc.m_testDist );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_testDistSpeed"), desc.m_testDistSpeed );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_testDistAboveSpeed"), desc.m_testDistAboveSpeed );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_testDistMove"), desc.m_testDistMove );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_testConeAngleHalf"), desc.m_testConeAngleHalf );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_zParam"), desc.m_zParam );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_zTest"), testName );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_zMaxDiff"), desc.m_zMaxDiff );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_raiseBehaviorEventAtEnd"), raiseBehaviorEventAtEnd );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("explorations"), option.AsString(), TXT("m_callScriptEventAtEnd"), callScriptEventAtEnd );

		desc.m_boneLeft = CName( boneL );
		desc.m_boneRight = CName( boneR );
		Int32 zTestVal;
		zTest->FindValue( CName( testName ), zTestVal );
		desc.m_zTest = ( ExpZComparision )zTestVal;
		Int32 doubleSidedVal;
		doubleSidedEnum->FindValue( CName( doubleSidedName ), doubleSidedVal );
		desc.m_doubleSided = ( ExpDoubleSided )doubleSidedVal;
		desc.m_raiseBehaviorEventAtEnd = CName( raiseBehaviorEventAtEnd );
		desc.m_callScriptEventAtEnd = CName( callScriptEventAtEnd );
	}
}

void SGameplayConfig::WriteExplorationsDesc()
{
	const CEnum* e = SRTTI::GetInstance().FindEnum( CNAME( EExplorationType ) );
	if ( !e )
	{
		return;
	}

	const TDynArray< CName >& options = e->GetOptions();

	for ( Uint32 i=0; i<m_explorationsDesc.Size(); ++i )
	{
		const CName& option = options[ i ];

		const EdExplorationDesc& desc = *(m_explorationsDesc[ i ]);

		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_horizontal"), desc.m_horizontal );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_doubleSided"), desc.m_doubleSided );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_boneLeft"), desc.m_boneLeft.AsString() );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_boneRight"), desc.m_boneRight.AsString() );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_edgeOffset"), desc.m_edgeOffset );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_offsetMS"), desc.m_offsetMS );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_edgeYawOffset"), desc.m_edgeYawOffset );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_useEdgeOffsetX"), desc.m_useEdgeOffsetX );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_useEdgeOffsetY"), desc.m_useEdgeOffsetY );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_useEdgeOffsetZ"), desc.m_useEdgeOffsetZ );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_useMotionExX"), desc.m_useMotionExX );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_useMotionExY"), desc.m_useMotionExY );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_useMotionExZ"), desc.m_useMotionExZ );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_edgeStartPointOffset"), desc.m_edgeStartPointOffset );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_edgeEndPointOffset"), desc.m_edgeEndPointOffset );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_edgeGranularity"), desc.m_edgeGranularity );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_alignTransToEdge"), desc.m_alignTransToEdge );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_alignRotToEdge"), desc.m_alignRotToEdge );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_alignRotToEdgeExceeding"), desc.m_alignRotToEdgeExceeding );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_blendIn"), desc.m_blendIn );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_blendOut"), desc.m_blendOut );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_earlyEndOffset"), desc.m_earlyEndOffset );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_testDist"), desc.m_testDist );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_testDistSpeed"), desc.m_testDistSpeed );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_testDistAboveSpeed"), desc.m_testDistAboveSpeed );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_testDistMove"), desc.m_testDistMove );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_testConeAngleHalf"), desc.m_testConeAngleHalf );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_zParam"), desc.m_zParam );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_zTest"), desc.m_zTest );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_zMaxDiff"), desc.m_zMaxDiff );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_raiseBehaviorEventAtEnd"), desc.m_raiseBehaviorEventAtEnd );
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("explorations"), option.AsChar(), TXT("m_callScriptEventAtEnd"), desc.m_callScriptEventAtEnd );
	}
}
