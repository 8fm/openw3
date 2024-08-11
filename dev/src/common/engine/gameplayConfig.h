/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorIncludes.h"

//////////////////////////////////////////////////////////////////////////

class EdExplorationDesc;

struct SGameplayConfigLookAts
{
	SGameplayConfigLookAts();

	Int32			m_reactionDebugType;

	Float			m_lookAtDurationGlance;
	Float			m_lookAtDurationLook;
	Float			m_lookAtDurationGaze;
	Float			m_lookAtDurationStare;

	Float			m_lookAtDurationRandGlance;
	Float			m_lookAtDurationRandLook;
	Float			m_lookAtDurationRandGaze;
	Float			m_lookAtDurationRandStare;

	Float			m_lookAtRangeGlance;
	Float			m_lookAtRangeLook;
	Float			m_lookAtRangeGaze;
	Float			m_lookAtRangeStare;
																																			
	Float			m_lookAtSpeedGlance;
	Float			m_lookAtSpeedLook;
	Float			m_lookAtSpeedGaze;
	Float			m_lookAtSpeedStare;

	Float			m_lookAtSpeedRandGlance;
	Float			m_lookAtSpeedRandLook;
	Float			m_lookAtSpeedRandGaze;
	Float			m_lookAtSpeedRandStare;

	Bool			m_lookAtAutoLimitGlance;
	Bool			m_lookAtAutoLimitLook;
	Bool			m_lookAtAutoLimitGaze;
	Bool			m_lookAtAutoLimitStare;

	Float			m_lookAtDelay;
	Float			m_lookAtDelayDialog;

	DECLARE_RTTI_STRUCT( SGameplayConfigLookAts ); 
};

BEGIN_CLASS_RTTI( SGameplayConfigLookAts );
	PROPERTY_EDIT( m_reactionDebugType, TXT("") );

	PROPERTY_EDIT( m_lookAtDurationGlance, TXT("") );
	PROPERTY_EDIT( m_lookAtDurationLook, TXT("") );
	PROPERTY_EDIT( m_lookAtDurationGaze, TXT("") );
	PROPERTY_EDIT( m_lookAtDurationStare, TXT("") );

	PROPERTY_EDIT( m_lookAtDurationRandGlance, TXT("") );
	PROPERTY_EDIT( m_lookAtDurationRandLook, TXT("") );
	PROPERTY_EDIT( m_lookAtDurationRandGaze, TXT("") );
	PROPERTY_EDIT( m_lookAtDurationRandStare, TXT("") );

	PROPERTY_EDIT( m_lookAtRangeGlance, TXT("") );
	PROPERTY_EDIT( m_lookAtRangeLook, TXT("") );
	PROPERTY_EDIT( m_lookAtRangeGaze, TXT("") );
	PROPERTY_EDIT( m_lookAtRangeStare, TXT("") );

	PROPERTY_EDIT( m_lookAtSpeedGlance, TXT("") );
	PROPERTY_EDIT( m_lookAtSpeedLook, TXT("") );
	PROPERTY_EDIT( m_lookAtSpeedGaze, TXT("") );
	PROPERTY_EDIT( m_lookAtSpeedStare, TXT("") );

	PROPERTY_EDIT( m_lookAtSpeedRandGlance, TXT("") );
	PROPERTY_EDIT( m_lookAtSpeedRandLook, TXT("") );
	PROPERTY_EDIT( m_lookAtSpeedRandGaze, TXT("") );
	PROPERTY_EDIT( m_lookAtSpeedRandStare, TXT("") );

	PROPERTY_EDIT( m_lookAtAutoLimitGlance, TXT("") );
	PROPERTY_EDIT( m_lookAtAutoLimitLook, TXT("") );
	PROPERTY_EDIT( m_lookAtAutoLimitGaze, TXT("") );
	PROPERTY_EDIT( m_lookAtAutoLimitStare, TXT("") );

	PROPERTY_EDIT( m_lookAtDelay, TXT("") );
	PROPERTY_EDIT( m_lookAtDelayDialog, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SGameplayConfigGameCamera
{
	SGameplayConfigGameCamera();

	Float	m_verManualMax;
	Float	m_verManualMin;

	Float	m_horDamping;
	Float	m_verDamping;

	Float	m_pivotDamp;
	Float	m_focusTargetDamp;
	Float	m_focusActDuration;

	Float	m_zoomDamp;
	Float	m_zoomActTime;

	Float	m_verOffsetDamp;
	Float	m_verOffsetActTime;

	Float	m_backOffsetDamp;

	Float	m_collisionDampOn;
	Float	m_collisionDampOff;
	Float	m_collisionBigRadius;
	Float	m_collisionBoxScale;
	Float	m_collisionAutoRotDamp;
	Float	m_collisionAutoRotMaxSpeed;
	Float	m_collisionVerCorrection;
	Float	m_collisionPivotHeightOffset;
	Float	m_collisionPivotRadius;
	Float	m_collisionVerRadius;
	Float	m_collisionVerOffsetP;
	Float	m_collisionVerOffsetM;
	Float	m_collisionVerFactor;
	Float	m_collisionVerRadiusP;
	Float	m_collisionVerRadiusM;
	Bool	m_collisionAutoRotTrace;
	Float	m_collisionAutoRotTraceFactor;
	Float	m_indoorCollisionMaxZoom;
	Float	m_slopeVerFactor;
	Float	m_slopeVerDamp;

	Float	m_sensX;
	Float	m_sensY;

	DECLARE_RTTI_SIMPLE_CLASS( SGameplayConfigGameCamera ); 
};

BEGIN_CLASS_RTTI( SGameplayConfigGameCamera );
	PROPERTY_EDIT( m_verManualMax, TXT("") );
	PROPERTY_EDIT( m_verManualMin, TXT("") );
	PROPERTY_EDIT( m_horDamping, TXT("") );
	PROPERTY_EDIT( m_verDamping, TXT("") );
	PROPERTY_EDIT( m_pivotDamp, TXT("") );
	PROPERTY_EDIT( m_focusTargetDamp, TXT("") );
	PROPERTY_EDIT( m_focusActDuration, TXT("") );
	PROPERTY_EDIT( m_zoomDamp, TXT("") );
	PROPERTY_EDIT( m_zoomActTime, TXT("") );
	PROPERTY_EDIT( m_verOffsetDamp, TXT("") );
	PROPERTY_EDIT( m_verOffsetActTime, TXT("") );
	PROPERTY_EDIT( m_backOffsetDamp, TXT("") );
	PROPERTY_EDIT( m_collisionDampOn, TXT("") );
	PROPERTY_EDIT( m_collisionDampOff, TXT("") );
	PROPERTY_EDIT( m_collisionBigRadius, TXT("") );
	PROPERTY_EDIT( m_collisionBoxScale, TXT("") );
	PROPERTY_EDIT( m_collisionAutoRotDamp, TXT("") );
	PROPERTY_EDIT( m_collisionAutoRotMaxSpeed, TXT("") );
	PROPERTY_EDIT( m_collisionVerCorrection, TXT("") );
	PROPERTY_EDIT( m_collisionPivotHeightOffset, TXT("") );
	PROPERTY_EDIT( m_collisionPivotRadius, TXT("") );
	PROPERTY_EDIT( m_collisionVerRadius, TXT("") );
	PROPERTY_EDIT( m_collisionVerOffsetP, TXT("") );
	PROPERTY_EDIT( m_collisionVerOffsetM, TXT("") );
	PROPERTY_EDIT( m_collisionVerFactor, TXT("") );
	PROPERTY_EDIT( m_collisionVerRadiusP, TXT("") );
	PROPERTY_EDIT( m_collisionVerRadiusM, TXT("") );
	PROPERTY_EDIT( m_collisionAutoRotTrace, TXT("") );
	PROPERTY_EDIT( m_collisionAutoRotTraceFactor, TXT("") );
	PROPERTY_EDIT( m_indoorCollisionMaxZoom, TXT("") );
	PROPERTY_EDIT( m_slopeVerFactor, TXT("") );
	PROPERTY_EDIT( m_slopeVerDamp, TXT("") );
	PROPERTY_EDIT( m_sensX, TXT("") );
	PROPERTY_EDIT( m_sensY, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SActorLODConfig
{
	Uint32			m_index;

	Float			m_distance;				// LOD toggle distance
	Float			m_deadZone;				// LOD toggle deadzone

	Bool			m_hide;					// Hide the actor
	Bool			m_enableIK;				// Enable IK
	Bool			m_enableDandles;		// Enable dandles (e.g. used for hair or tits)
	Uint32			m_mimicsQuality;		// Mimics quality level (0 - highest)
	EBehaviorLod	m_behaviorLOD;			// Behavior/skeleton quality level
	Uint32			m_animatedComponentUpdateFrameSkip;			// Animated component's update (and sample) frame skip
	Bool			m_suppressAnimatedComponent;				// Suppress animated component animation if not visible
	Bool			m_budgetAnimatedComponentTick;				// Budget animated component tick if not visible
	Bool			m_suppressAnimatedComponentIfNotVisible;	// Suppress animated component animation if not visible
	Bool			m_budgetAnimatedComponentTickIfNotVisible;	// Budget animated component tick if not visible

	SActorLODConfig()
		: m_index( 0 )
		, m_distance( 0.0f )
		, m_deadZone( 4.0f )
		, m_hide( false )
		, m_enableIK( true )
		, m_enableDandles( true )
		, m_mimicsQuality( 0 )
		, m_behaviorLOD( BL_Lod0 )
		, m_animatedComponentUpdateFrameSkip( 0 )
		, m_suppressAnimatedComponent( false )
		, m_budgetAnimatedComponentTick( false )
		, m_suppressAnimatedComponentIfNotVisible( false )
		, m_budgetAnimatedComponentTickIfNotVisible( false )
	{}

	DECLARE_RTTI_STRUCT( SActorLODConfig ); 
};

BEGIN_CLASS_RTTI( SActorLODConfig );
	PROPERTY_EDIT( m_distance, TXT("End LOD distance") );
	PROPERTY_EDIT( m_deadZone, TXT("LOD toggling dead zone") );
	PROPERTY_EDIT( m_hide, TXT("") );
	PROPERTY_EDIT( m_enableIK, TXT("") );
	PROPERTY_EDIT( m_enableDandles, TXT("") );
	PROPERTY_EDIT( m_mimicsQuality, TXT("0 - highest") );
	PROPERTY_EDIT( m_behaviorLOD, TXT("") );
	PROPERTY_EDIT( m_animatedComponentUpdateFrameSkip, TXT("How many animation sampling frames to skip") );
	PROPERTY_EDIT( m_suppressAnimatedComponent, TXT("") );
	PROPERTY_EDIT( m_budgetAnimatedComponentTick, TXT("") );
	PROPERTY_EDIT( m_suppressAnimatedComponentIfNotVisible, TXT("") );
	PROPERTY_EDIT( m_budgetAnimatedComponentTickIfNotVisible, TXT("") );
END_CLASS_RTTI()

struct SGameplayLODConfig
{
	// Actors

	TDynArray< SActorLODConfig >		m_actorLODs;
	Float								m_actorInvisibilityTimeThreshold;

	// Components

	Float								m_maxBudgetedComponentsTickTime;
	Uint32								m_minBudgetedComponentsTickPercentage;
	Float								m_expectedNonBudgetedComponentsTickTime;

	// Non-actor components

	Float								m_componentsTickLODUpdateTime;
	Float								m_componentsBudgetableTickDistance;
	Float								m_componentsDisableTickDistance;

	// Entities

	Float								m_entitiesBudgetableTickDistance;
	Float								m_entitiesDisableTickDistance;
	Float								m_entitiesTickTime;

	// Effects

	Float								m_effectsBudgetableTickDistance;
	Float								m_effectsTickTime;
	Float								m_effectsTickLODUpdateTime;

	SGameplayLODConfig();

	DECLARE_RTTI_STRUCT( SGameplayLODConfig ); 
};

BEGIN_CLASS_RTTI( SGameplayLODConfig );
	PROPERTY_EDIT( m_actorLODs, TXT("Actor LOD configuration") )
	PROPERTY_EDIT( m_actorInvisibilityTimeThreshold, TXT("Time after which an actor is considered invisible") )

	PROPERTY_EDIT( m_maxBudgetedComponentsTickTime, TXT("") )
	PROPERTY_EDIT( m_minBudgetedComponentsTickPercentage, TXT("") )

	PROPERTY_EDIT( m_componentsTickLODUpdateTime, TXT("") );
	PROPERTY_EDIT( m_componentsBudgetableTickDistance, TXT("") )
	PROPERTY_EDIT( m_componentsDisableTickDistance, TXT("") )

	PROPERTY_EDIT( m_entitiesBudgetableTickDistance, TXT("") )
	PROPERTY_EDIT( m_entitiesDisableTickDistance, TXT("") )
	PROPERTY_EDIT( m_entitiesTickTime, TXT("") );

	PROPERTY_EDIT( m_effectsBudgetableTickDistance, TXT("") )
	PROPERTY_EDIT( m_effectsTickLODUpdateTime, TXT("") )
	PROPERTY_EDIT( m_effectsTickTime, TXT("") );
END_CLASS_RTTI()

struct SGameplayConfig
{
	SGameplayConfig();

	void Validate();
	void Load();
	void Save();

	void ReadExplorationsDesc();
	void WriteExplorationsDesc();

	void ReadLODDesc();
	void WriteLODDesc();

	//////////////////////////////////////////////////////////////////////////
	// LOD
	SGameplayLODConfig m_LOD;

	//////////////////////////////////////////////////////////////////////////
	// Explorations
	TDynArray< EdExplorationDesc* > m_explorationsDesc;

	//////////////////////////////////////////////////////////////////////////
	// Horse
	Bool			m_horseProp;
	Bool			m_horseCarSteer;
	Bool			m_horseSpeedCtrl;
	Float			m_horseSpeedInc;
	Float			m_horseSpeedDec;
	Float			m_horseSpeedStep;
	Float			m_horseInputCooldown;
	Float			m_horseStaminaInc;
	Float			m_horseStaminaDec;
	Float			m_horseStaminaCooldown;
	Float			m_horseSpeedDecCooldown;
	Float			m_horsePathFactor;
	Float			m_horsePathDamping;
	Float			m_horseRoadSearchRadius;
	Float			m_horseRoadSearchDistanceSlow;
	Float			m_horseRoadSearchDistanceFast;
	Float			m_horseRoadSelectionAngleCoeff;
	Float			m_horseRoadSelectionDistanceCoeff;
	Float			m_horseRoadSelectionCurrentRoadPreferenceCoeff;
	Float			m_horseRoadSelectionTurnAmountFeeCoeff;
	Float			m_horseRoadFollowingCooldownTime;
	Float			m_horseRoadFollowingCooldownDistance;

	//////////////////////////////////////////////////////////////////////////
	// Player
	Int32			m_playerPreviewInventory;

	//////////////////////////////////////////////////////////////////////////
	// Debug
	Float			m_debugA;
	Float			m_debugB;
	Float			m_debugC;
	Float			m_debugD;
	Float			m_debugE;
	Float			m_debugF;
	Float			m_debugG;
	
	//////////////////////////////////////////////////////////////////////////
	// Interactions
	Bool			m_interactionTestCameraRange;
	Float			m_interactionTestCameraRangeAngle;

	Bool			m_interactionTestPlayerRange;
	Float			m_interactionTestPlayerRangeAngle;

	Bool			m_interactionTestIsInPlayerRadius;
	Float			m_interactionTestPlayerRadius;

	//////////////////////////////////////////////////////////////////////////
	// Animations
	Bool			m_forceLookAtPlayer;
	Float			m_forceLookAtPlayerDist;

	Bool			m_useBehaviorLod;
	Bool			m_forceBehaviorLod;
	Int32			m_forceBehaviorLodLevel;

	Bool			m_logMissingAnimations;
	Bool			m_logRequestedAnimations;
	Bool			m_logSampledAnimations;

	Bool			m_animationMultiUpdate;
	Bool			m_animationAsyncUpdate;
	Bool			m_animationAsyncJobs;
	Bool			m_animationCanUseAsyncJobs;
	Bool			m_animationAsyncJobsUpdateFrustum;

	//////////////////////////////////////////////////////////////////////////
	// Work
	Bool			m_streamOnlyVisibleLayers;
	Bool			m_useWorkFreezing;
	Bool			m_workResetInFreezing;
	Bool			m_workSynchronization;
	Float			m_workFreezingRadiusForInvisibleActors;
	Float			m_workFreezingDelay;
	Float			m_workMaxFreezingTime;
	Float			m_workAnimSpeedMulMin;
	Float			m_workAnimSpeedMulMax;
	Float			m_workMaxAnimOffset;

	//////////////////////////////////////////////////////////////////////////
	// Physics
	Float			m_physicsTerrainAdditionalElevation;
	Float			m_physicsTerrainThickness;
	Float			m_physicsKillingZVelocity;
	Float			m_physicsTerrainDebugMaxSlope;
	Float			m_physicsTerrainDebugRange;
	Float			m_physicsCollisionPredictionTime;
	Uint32			m_physicsCollisionPredictionSteps;
	Float			m_physicsCCTMaxDisp;

	//////////////////////////////////////////////////////////////////////////
	// Physics Character Controller
	Float			m_terrainInfluenceLimitMin;		// terrain influence min slope value <0-1>
	Float			m_terrainInfluenceLimitMax;		// terrain influence max slope value <0-1>
	Float			m_terrainInfluenceMul;			// terrain influence mul <0-10>
	Float			m_slidingLimitMin;				// sliding min slope limit
	Float			m_slidingLimitMax;				// sliding max slope limit
	Float			m_slidingDamping;				// sliding damping/inertia sim
	Float			m_maxPlatformDisplacement;		// maximal displacement from platform movement <0-5>
	Float			m_probeTerrainOffset;			// terrain probing ray-casts offset
	Float			m_virtualRadiusTime;			// virtual radius time
	Float			m_fallingTime;					// falling delay time
	Float			m_fallingMul;					// falling multiplier
	Float			m_jumpV0;						// *jumping
	Float			m_jumpTc;
	Float			m_jumpDelay;
	Float			m_jumpMinTime;
	Float			m_jumpGravityUp;
	Float			m_jumpGravityDown;
	Float			m_jumpMaxVertSpeed;
	Float			m_jumpLenMul;
	Float			m_jumpHeightMul;
	Float			m_movingSwimmingOffset;
	Float			m_emergeSpeed;
	Float			m_submergeSpeed;

	//////////////////////////////////////////////////////////////////////////
	// Animated properties
	Int32			m_curvePrecision;
	Bool			m_showSegments;
	Bool			m_showRotations;
	Bool			m_showNodes;
	Float			m_timeScale;

	//////////////////////////////////////////////////////////////////////////
	// Actor lod
	Bool					m_actorOptUse;
	Int32					m_actorOptDiv;

    //////////////////////////////////////////////////////////////////////////
	// LookAt
	SGameplayConfigLookAts m_lookAtConfig;

	//////////////////////////////////////////////////////////////////////////
	// Camera
	Float			m_cameraHidePlayerDistMin;
	Float			m_cameraHidePlayerDistMax;

	Float			m_cameraHidePlayerSwordsDistMin;
	Float			m_cameraHidePlayerSwordsDistMax;
	Float			m_cameraHidePlayerSwordsAngleMin;
	Float			m_cameraHidePlayerSwordsAngleMax;

	Bool			m_cameraPositionDamp;
	Float			m_cameraPositionDampLength;
	Float			m_cameraPositionDampLengthOffset;
	Float			m_cameraPositionDampSpeed;
	
	Bool			m_processNpcsAndCameraCollisions;

	//////////////////////////////////////////////////////////////////////////
	// Movement
	Bool			m_movementTraceOpt;
	Bool			m_movementDeltaTestOpt;
	Float			m_movementSmoothing;
	Float			m_movementSmoothingOnHorse;

	//////////////////////////////////////////////////////////////////////////
	// Combat
	Float			m_movementTorsoDamp;
	Float			m_movementRotDamp;

	//////////////////////////////////////////////////////////////////////////
	// Scenes
	Bool			m_enableMeshFlushInScenes;
	Bool			m_enableTextureFlushInScenes;
	Bool			m_enableAnimationFlushInScenes;
	Bool			m_enableSimplePriorityLoadingInScenes;
	Bool			m_enableSceneRewind;
	Bool			m_useFrozenFrameInsteadOfBlackscreen;
	Float			m_sceneIgnoreInputDuration;

	//////////////////////////////////////////////////////////////////////////
	// GC
	Bool			m_gcAfterCutscenesWithCamera;
	Bool			m_gcAfterNotGameplayScenes;

	//////////////////////////////////////////////////////////////////////////
	// Prog
	Bool			m_useMultiTick;
	
	//////////////////////////////////////////////////////////////////////////
	// Save
	Float			m_autosaveCooldown;		// how much time needs to pass after auto-saving to make another auto-saving (like chekpoint) possible
	Float			m_autosaveDelay;		// how much time needs to pass after the system decides that the autosave is needed and the actual saving

	//////////////////////////////////////////////////////////////////////////
	// Game camera
	SGameplayConfigGameCamera m_gameCamera;

	//////////////////////////////////////////////////////////////////////////
	//Debug
	Bool			m_doNotClickMe;

	// Interactive dialogs
	Bool			m_idUseNewVoicePipeline;

	//////////////////////////////////////////////////////////////////////////
	// Nearest wound weights
	Float			m_woundDistanceWeight;
	Float			m_woundDirectionWeight;

	//////////////////////////////////////////////////////////////////////////
	// Stray actors
	Float			m_strayActorDespawnDistance;
	Int32			m_strayActorMaxHoursToKeep;
	Int32			m_strayActorMaxActorsToKeep;


    Float m_boatTiltingYSpeed;
    //Float m_boatTiltCentrifugalForcePercent;
    Float m_boatTiltingMaxYTiltPercent;
    Float m_boatTiltingMinYTiltPercent;

    Float  m_boatWind1stGearScaller;
    Float  m_boatWind2ndGearScaller;
    Float  m_boatWind3rdGearScaller;

    //////////////////////////////////////////////////////////////////////////

	#ifndef RED_FINAL_BUILD
	Bool			m_disableResetInput;
	#endif

	//////////////////////////////////////////////////////////////////////////

	DECLARE_RTTI_STRUCT( SGameplayConfig ); 
};

BEGIN_CLASS_RTTI( SGameplayConfig );
	PROPERTY_EDIT( m_gameCamera, TXT("") );

	PROPERTY_EDIT( m_LOD, TXT("LOD parameters") );

	PROPERTY_EDIT( m_debugA, TXT("") );
	PROPERTY_EDIT( m_debugB, TXT("") );
	PROPERTY_EDIT( m_debugC, TXT("") );
	PROPERTY_EDIT( m_debugD, TXT("") );
	PROPERTY_EDIT( m_debugE, TXT("") );
	PROPERTY_EDIT( m_debugF, TXT("") );
	PROPERTY_EDIT( m_debugG, TXT("") );

	PROPERTY_EDIT( m_horseProp, TXT("") );
	PROPERTY_EDIT( m_horseSpeedCtrl, TXT("") );
	PROPERTY_EDIT( m_horseSpeedInc, TXT("") );
	PROPERTY_EDIT( m_horseSpeedDec, TXT("") );
	PROPERTY_EDIT( m_horseSpeedStep, TXT("") );
	PROPERTY_EDIT( m_horseInputCooldown, TXT("") );
	PROPERTY_EDIT( m_horseStaminaInc, TXT("") );
	PROPERTY_EDIT( m_horseStaminaDec, TXT("") );
	PROPERTY_EDIT( m_horseStaminaCooldown, TXT("") );
	PROPERTY_EDIT( m_horseSpeedDecCooldown, TXT("") );

	PROPERTY_EDIT( m_horsePathFactor, TXT("") );
	PROPERTY_EDIT( m_horsePathDamping, TXT("") );
	PROPERTY_EDIT( m_horseRoadSearchRadius, TXT("") );
	PROPERTY_EDIT( m_horseRoadSearchDistanceSlow, TXT("") );
	PROPERTY_EDIT( m_horseRoadSearchDistanceFast, TXT("") );
	PROPERTY_EDIT( m_horseRoadSelectionAngleCoeff, TXT("") );
	PROPERTY_EDIT( m_horseRoadSelectionDistanceCoeff, TXT("") );
	PROPERTY_EDIT( m_horseRoadSelectionCurrentRoadPreferenceCoeff, TXT("") );
	PROPERTY_EDIT( m_horseRoadSelectionTurnAmountFeeCoeff, TXT("") );
	PROPERTY_EDIT( m_horseRoadFollowingCooldownTime, TXT("") );
	PROPERTY_EDIT( m_horseRoadFollowingCooldownDistance, TXT("") );

	PROPERTY_EDIT( m_strayActorDespawnDistance, TXT("") );
	PROPERTY_EDIT( m_strayActorMaxHoursToKeep, TXT("") );
	PROPERTY_EDIT( m_strayActorMaxActorsToKeep, TXT("") );

	PROPERTY_CUSTOM_EDIT( m_playerPreviewInventory, TXT(""), TXT("ScriptedEnum_EPlayerPreviewInventory") );

	PROPERTY_EDIT( m_interactionTestCameraRange, TXT("") );
	PROPERTY_EDIT( m_interactionTestCameraRangeAngle, TXT("") );
	PROPERTY_EDIT( m_interactionTestPlayerRange, TXT("") );
	PROPERTY_EDIT( m_interactionTestPlayerRangeAngle, TXT("") );
	PROPERTY_EDIT( m_interactionTestIsInPlayerRadius, TXT("") );
	PROPERTY_EDIT( m_interactionTestPlayerRadius, TXT("") );

	PROPERTY_EDIT( m_forceLookAtPlayer, TXT("") );
	PROPERTY_EDIT( m_forceLookAtPlayerDist, TXT("") );

	PROPERTY_EDIT( m_useBehaviorLod, TXT("") );
	PROPERTY_EDIT( m_forceBehaviorLod, TXT("") );
	PROPERTY_EDIT( m_forceBehaviorLodLevel, TXT("") );
	PROPERTY_EDIT( m_logMissingAnimations, TXT("") );
	PROPERTY_EDIT( m_logRequestedAnimations, TXT("") );
	PROPERTY_EDIT( m_logSampledAnimations, TXT("") );

	PROPERTY_EDIT( m_animationMultiUpdate, TXT("") );
	PROPERTY_EDIT( m_animationAsyncUpdate, TXT("") );
	PROPERTY_EDIT( m_animationAsyncJobs, TXT("") );
	PROPERTY_EDIT( m_animationCanUseAsyncJobs, TXT("") );
	PROPERTY_EDIT( m_animationAsyncJobsUpdateFrustum, TXT("") );

	PROPERTY_EDIT( m_useWorkFreezing, TXT("") );
	PROPERTY_EDIT( m_streamOnlyVisibleLayers, TXT("") );
	PROPERTY_EDIT( m_workFreezingRadiusForInvisibleActors, TXT("") );
	PROPERTY_EDIT( m_workSynchronization, TXT("") );
	PROPERTY_EDIT( m_workResetInFreezing, TXT("") );
	PROPERTY_EDIT( m_workFreezingDelay, TXT("") );
	PROPERTY_EDIT( m_workMaxFreezingTime, TXT("") );
	PROPERTY_EDIT( m_workAnimSpeedMulMin, TXT("") ); 
	PROPERTY_EDIT( m_workAnimSpeedMulMax, TXT("") ); 
	PROPERTY_EDIT( m_workMaxAnimOffset, TXT("") );

	PROPERTY_EDIT( m_lookAtConfig, TXT("") );

	PROPERTY_EDIT( m_cameraHidePlayerDistMin, TXT("") );
	PROPERTY_EDIT( m_cameraHidePlayerDistMax, TXT("") );
	PROPERTY_EDIT( m_cameraHidePlayerSwordsDistMin, TXT("") );
	PROPERTY_EDIT( m_cameraHidePlayerSwordsDistMax, TXT("") );
	PROPERTY_EDIT( m_cameraHidePlayerSwordsAngleMin, TXT("") );
	PROPERTY_EDIT( m_cameraHidePlayerSwordsAngleMax, TXT("") );
	PROPERTY_EDIT( m_cameraPositionDamp, TXT("") );
	PROPERTY_EDIT( m_cameraPositionDampLength, TXT("") );
	PROPERTY_EDIT( m_cameraPositionDampLengthOffset, TXT("") );
	PROPERTY_EDIT( m_cameraPositionDampSpeed, TXT("") );
	PROPERTY_EDIT( m_processNpcsAndCameraCollisions, TXT("") );

	PROPERTY_EDIT_RANGE( m_physicsTerrainAdditionalElevation, TXT(""), -0.1f, 0.1f );
	PROPERTY_EDIT_RANGE( m_physicsTerrainThickness, TXT(""), -5.f, 5.f );
	PROPERTY_EDIT_RANGE( m_physicsKillingZVelocity, TXT(""), -1000.f, -0.1f );
	PROPERTY_EDIT_RANGE( m_physicsTerrainDebugMaxSlope, TXT(""), FLT_EPSILON, 90.f - FLT_EPSILON );
	PROPERTY_EDIT_RANGE( m_physicsTerrainDebugRange, TXT(""), 10.f, 1000.f );
	PROPERTY_EDIT_RANGE( m_physicsCollisionPredictionTime, TXT(""), 0.f, 10.f );
	PROPERTY_EDIT_RANGE( m_physicsCollisionPredictionSteps, TXT(""), 1, 100 );
	PROPERTY_EDIT_RANGE( m_physicsCCTMaxDisp, TXT(""), 0.001f, 100.f );
	PROPERTY_EDIT_RANGE( m_virtualRadiusTime, TXT(""), 0.01f, 10.f );
	PROPERTY_EDIT_RANGE( m_movingSwimmingOffset, TXT("Offset between moving and swimming"), -10.0f, 10.0f );
	PROPERTY_EDIT_RANGE( m_emergeSpeed, TXT("Emerging Speed"), 0.1f, 100.0f );
	PROPERTY_EDIT_RANGE( m_submergeSpeed, TXT("Submerging Speed"), 0.1f, 100.0f );

	PROPERTY_EDIT_RANGE( m_terrainInfluenceLimitMin, TXT("Terrain influence min slope limit <0-1>"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_terrainInfluenceLimitMax, TXT("Terrain influence max slope limit <0-1>"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_terrainInfluenceMul, TXT("Terrain influence multiplier"), 0.0f, 10.0f );
	PROPERTY_EDIT_RANGE( m_slidingLimitMin, TXT("Sliding min slope limit"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_slidingLimitMax, TXT("Sliding max slope limit"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_slidingDamping, TXT("Sliding damping"), 0.025f, 1.0f );
	PROPERTY_EDIT_RANGE( m_maxPlatformDisplacement, TXT(""), 0.0f, 10.0f );
	
	PROPERTY_EDIT( m_showSegments, TXT("Show segments") );
	PROPERTY_EDIT( m_showRotations, TXT("Show rotations") );
	PROPERTY_EDIT( m_showNodes, TXT("Show nodes") );
	PROPERTY_EDIT_RANGE( m_curvePrecision, TXT("Curve precision"), 1, 100 );
	PROPERTY_EDIT_RANGE( m_timeScale, TXT(""), 0.0f, 5.0f );

	PROPERTY_EDIT( m_gcAfterCutscenesWithCamera, TXT("") );
	PROPERTY_EDIT( m_gcAfterNotGameplayScenes, TXT("") );

	PROPERTY_EDIT( m_autosaveCooldown,	TXT("how much time needs to pass after auto-saving to make another auto-saving (like chekpoint) possible") );	
	PROPERTY_EDIT( m_autosaveDelay,		TXT("how much time needs to pass after the system decides that the autosave is needed and the actual saving") );

	PROPERTY_EDIT( m_doNotClickMe, TXT("") );
	#ifndef RED_FINAL_BUILD
	PROPERTY_EDIT( m_disableResetInput, TXT("") );
	#endif

	PROPERTY_EDIT( m_enableMeshFlushInScenes, TXT("") );
	PROPERTY_EDIT( m_enableSceneRewind, TXT("") );
	PROPERTY_EDIT( m_enableTextureFlushInScenes, TXT("") );
	PROPERTY_EDIT( m_enableAnimationFlushInScenes, TXT("") );
	PROPERTY_EDIT( m_enableSimplePriorityLoadingInScenes, TXT("") );
	PROPERTY_EDIT( m_useFrozenFrameInsteadOfBlackscreen, TXT("") );
	PROPERTY_EDIT( m_sceneIgnoreInputDuration, TXT("") );
	

	PROPERTY_EDIT( m_movementTorsoDamp, TXT("") );
	PROPERTY_EDIT( m_movementRotDamp, TXT("") );
	PROPERTY_EDIT_RANGE( m_movementSmoothing, TXT(""), 0.f, 1000.f );
	PROPERTY_EDIT_RANGE( m_movementSmoothingOnHorse, TXT(""), 0.f, 1000.f );

	PROPERTY_EDIT( m_idUseNewVoicePipeline, TXT("") );

	PROPERTY_EDIT( m_woundDistanceWeight, TXT( "Nearest wound distance weight" ) );
	PROPERTY_EDIT( m_woundDirectionWeight, TXT( "Nearest wound direction weight" ) );

END_CLASS_RTTI();
