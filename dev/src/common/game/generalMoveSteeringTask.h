/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "moveSteeringTask.h"
#include "movementGoal.h"
#include "movementCommandBuffer.h"

enum EMovementFlags;

#define KEEP_AWAY_WALLS_USE_STATIC_VAR

///////////////////////////////////////////////////////////////////////////////

class CMoveSTResetSteering : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTResetSteering, IMoveSteeringTask, 0 );

public:
	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTResetSteering );
	PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Reset output heading and speed to ZERO
class CMoveSTStop : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTStop, IMoveSteeringTask, 0 );

public:
	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTStop );
	PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveSTStopOnFreezing : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTStopOnFreezing, IMoveSteeringTask, 0 );

public:
	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTStopOnFreezing );
PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveSTChangeSpeed : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTChangeSpeed, IMoveSteeringTask, 0 );

private:
	EMoveType			m_speedType;
	Float				m_absSpeed;

public:
	CMoveSTChangeSpeed();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

};
BEGIN_CLASS_RTTI( CMoveSTChangeSpeed );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_speedType, TXT( "Speed type" ) );
	PROPERTY_EDIT( m_absSpeed, TXT( "Absolute speed" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Rotate to requested orientation 
class CMoveSTRotate : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTRotate, IMoveSteeringTask, 0 );

protected:
	void			ProcessRotation( IMovementCommandBuffer& comm ) const;

public:
	CMoveSTRotate()																{}

	void			CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String			GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTRotate );
	PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////
// Snap to minimal speed limits boost speed if its below some minimal value
// It uses velocity 
class CMoveSTSnapToMinimalVelocity : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSnapToMinimalVelocity, IMoveSteeringTask, 0 );

private:
	Float				m_minVelocity;

public:
	CMoveSTSnapToMinimalVelocity()
		: m_minVelocity( 0.25f )													{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

};
BEGIN_CLASS_RTTI( CMoveSTSnapToMinimalVelocity );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_minVelocity, TXT( "Minimal movement velocity. If all algorithms up to this point sets speed to something lesser - boost it." ) );
END_CLASS_RTTI();

class CMoveSTMove : public CMoveSTRotate
{
	DECLARE_ENGINE_CLASS( CMoveSTMove, CMoveSTRotate, 0 );

private:
	Float			m_headingImportance;
	Float			m_speedImportance;

protected:
	void			ProcessMovement( IMovementCommandBuffer& comm ) const;

public:
	CMoveSTMove();

	void			CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String			GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTMove );
	PARENT_CLASS( CMoveSTRotate );
	PROPERTY_EDIT( m_headingImportance, TXT( "Heading importance" ) );
	PROPERTY_EDIT( m_speedImportance, TXT( "Speed importance" ) );
END_CLASS_RTTI();

class CMoveSTMoveWithOffset : public CMoveSTRotate
{
	DECLARE_ENGINE_CLASS( CMoveSTMoveWithOffset, CMoveSTRotate, 0 );

private:
	Float			m_headingImportance;
	Float			m_speedImportance;
	Float			m_offset;

protected:
	void			ProcessMovement( IMovementCommandBuffer& comm, const Vector& target ) const;

public:
	CMoveSTMoveWithOffset();

	void			CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String			GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTMoveWithOffset );
	PARENT_CLASS( CMoveSTRotate );
	PROPERTY_EDIT( m_headingImportance, TXT( "Heading importance" ) );
	PROPERTY_EDIT( m_speedImportance, TXT( "Speed importance" ) );
	PROPERTY_EDIT( m_offset, TXT( "Perperndicular offset to target" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// check cpp for more info "how it works"

class CMoveSTFinalStep : public CMoveSTMove
{
	DECLARE_ENGINE_CLASS( CMoveSTFinalStep, CMoveSTMove, 0 );

private:
	Bool			m_ignoreGoalToleranceForFinalLocation;
	CName			m_finalStepPositionVar;
	CName			m_finalStepDistanceVar;
	CName			m_finalStepActiveVar;
	CName			m_finalStepEvent;
	CName			m_finalStepActivationNotification;
	CName			m_finalStepDeactivationNotification;
	Float			m_finalStepDeactivationNotificationTimeOut;
	Float			m_finalStepDistanceLimit;

private:
	TInstanceVar< EngineTime >	i_finalStepDeactivationTimeout;
	TInstanceVar< Bool >		i_finalStepInProgress;

public:
	CMoveSTFinalStep();

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTFinalStep );
	PARENT_CLASS( CMoveSTMove );
	PROPERTY_EDIT( m_ignoreGoalToleranceForFinalLocation, TXT("") );
	PROPERTY_EDIT( m_finalStepPositionVar, TXT( "'Final position' behavior vector variable name" ) );
	PROPERTY_EDIT( m_finalStepDistanceVar, TXT( "'Distance to final position' behavior variable name" ) );
	PROPERTY_EDIT( m_finalStepActiveVar, TXT( "'Final step is active' behavior variable name" ) );
	PROPERTY_EDIT( m_finalStepEvent, TXT( "'Final step' behavior event name to raise" ) );
	PROPERTY_EDIT( m_finalStepActivationNotification, TXT( "'Final step' deactivation notification name" ) );
	PROPERTY_EDIT( m_finalStepDeactivationNotification, TXT( "'Final step' deactivation notification name" ) );
	PROPERTY_EDIT( m_finalStepDeactivationNotificationTimeOut, TXT( "'Final step' deactivation notification timeout" ) );
	PROPERTY_EDIT( m_finalStepDistanceLimit, TXT("Max distance for final step. If final point is in further distance, it will be clamped.") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Go back to navigation data from an unaccessible location
class CMoveSTKeepNavdata : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTKeepNavdata, IMoveSteeringTask, 0 );

private:
	Float			m_slidingRate;
	Float			m_maxSlidingRange;
	Float			m_maxTeleportationRange;
	Bool			m_applyStandardConditions;

	TInstanceVar< Bool >		i_isSliding;
	TInstanceVar< Float >		i_slidingTimeout;
	TInstanceVar< Vector3 >		i_slidingTarget;

	void			Deactivate( CMovingAgentComponent& agent, InstanceBuffer& data ) const;
public:
	CMoveSTKeepNavdata()
		: m_slidingRate( 0.5f )
		, m_maxSlidingRange( 0.5f )
		, m_maxTeleportationRange( 3.0f )
		, m_applyStandardConditions( false )									{}

	void			CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	void			OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;
	void			OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;

	void			GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;

	void			OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void			OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;

	String			GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTKeepNavdata );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_slidingRate, TXT( "Sliding rate" ) );
	PROPERTY_EDIT( m_maxSlidingRange, TXT( "Max range on which we do slide" ) );
	PROPERTY_EDIT( m_maxTeleportationRange, TXT("Range at which we do insta teleporty") );
	PROPERTY_EDIT( m_applyStandardConditions, TXT("If set to true, keep navdata will be performed only if goal is set or pullback to navigation is requested.") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Rotate in heading direction
class CMoveSTMatchHeadingOrientation : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTMatchHeadingOrientation, IMoveSteeringTask, 0 );

protected:
	Bool				m_limitSpeedOnTurns;
	Float				m_maxAngleNotLimitingSpeed;
	Float				m_speedLimitOnRotation;
public:
	CMoveSTMatchHeadingOrientation()
		: m_limitSpeedOnTurns( false )
		, m_maxAngleNotLimitingSpeed( 30.f )
		, m_speedLimitOnRotation( 0.25f )										{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTMatchHeadingOrientation );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_limitSpeedOnTurns, TXT("Turns on 'limit speed on turns' feature") );
	PROPERTY_EDIT( m_maxAngleNotLimitingSpeed, TXT("If creature desired angle is above this value we don't limit movement speed ") );
	PROPERTY_EDIT( m_speedLimitOnRotation, TXT("If creature is rotating, we limit speed to this ammount") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Keep away from walls in pathfollow
class CMoveSTKeepAwayWallsInPathfollow : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTKeepAwayWallsInPathfollow, IMoveSteeringTask, 0 );

protected:
	Float				m_wallDetectionDistance;
	Float				m_headingImportance;

public:
	CMoveSTKeepAwayWallsInPathfollow()
		: m_wallDetectionDistance( 2.f )
		, m_headingImportance( 0.5f )											{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTKeepAwayWallsInPathfollow );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_wallDetectionDistance, TXT("Distance in which we are detecting walls") );
	PROPERTY_EDIT( m_headingImportance, TXT( "Heading importance" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Keep away from walls
class CMoveSTKeepAwayWalls : public CMoveSTKeepAwayWallsInPathfollow
{
	DECLARE_ENGINE_CLASS( CMoveSTKeepAwayWalls, CMoveSTKeepAwayWallsInPathfollow, 0 );

#ifndef KEEP_AWAY_WALLS_USE_STATIC_VAR
protected:
	TInstanceVar< SNavigationCollectCollisionInCircleData >		i_queryProxy;
#endif
public:
	CMoveSTKeepAwayWalls();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

#ifndef KEEP_AWAY_WALLS_USE_STATIC_VAR
	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;

	void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;
#endif
};
BEGIN_CLASS_RTTI( CMoveSTKeepAwayWalls );
	PARENT_CLASS( CMoveSTKeepAwayWallsInPathfollow );
END_CLASS_RTTI();



///////////////////////////////////////////////////////////////////////////////
class CMoveSTSeparateFromActors : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSeparateFromActors, IMoveSteeringTask, 0 );

protected:
	Float						m_separationDistance;
	Float						m_headingImportance;
	Float						m_updateFrequency;								// TODO: Update frequency should be heuristic depending (when we are in crowd we update a lot, when alone we do it once per eg. second)

	TInstanceVar< Float >		i_lastUpdateTime;
	TInstanceVar< Vector2 >		i_lastOutput;
	TInstanceVar< Float >		i_lastImportance;
	TInstanceVar< Vector2 >		i_currentOutput;
	TInstanceVar< Float >		i_currentImportance;
public:
	CMoveSTSeparateFromActors()
		: m_separationDistance( 2.f )
		, m_headingImportance( 0.5f )
		, m_updateFrequency( 0.15f )											{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};

BEGIN_CLASS_RTTI( CMoveSTSeparateFromActors );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_separationDistance, TXT("Distance to separate from") );
	PROPERTY_EDIT( m_headingImportance, TXT( "Heading importance" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveSTArrive : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTArrive, IMoveSteeringTask, 0 );

private:
	CName			m_rotationVar;
	CName			m_rotationEvent;
	CName			m_rotationNotification;

public:
	CMoveSTArrive();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override { return TXT( "Arrive" ); }
};
BEGIN_CLASS_RTTI( CMoveSTArrive );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_rotationVar, TXT( "Behavior rotation variable name" ) );
	PROPERTY_EDIT( m_rotationEvent, TXT( "Behavior rotation event name" ) );
	PROPERTY_EDIT( m_rotationNotification, TXT( "Behavior rotation event end notification" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Apply steering
class CMoveSTApplySteering : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTApplySteering, IMoveSteeringTask, 0 );
protected:
	Float			m_minSpeed;

	void ApplyHeading( IMovementCommandBuffer& comm, const Vector2& heading ) const;
	virtual void ApplyRotation( IMovementCommandBuffer& comm, Float rotation ) const;
	void ApplySpeed( IMovementCommandBuffer& comm, Float speed ) const;

public:
	CMoveSTApplySteering();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTApplySteering )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_minSpeed,  TXT( "Minimal movement speed. If all algorithms up to this point sets speed to something lesser - boost it." ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Apply "animation" steering with custom animation based rotations
class CMoveSTApplyAnimationSteering : public CMoveSTApplySteering
{
	DECLARE_ENGINE_CLASS( CMoveSTApplyAnimationSteering, CMoveSTApplySteering, 0 );
protected:
	void ApplyRotation( IMovementCommandBuffer& comm, Float rotation ) const override;

	CName			m_rotationVar;

public:
	CMoveSTApplyAnimationSteering();

	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTApplyAnimationSteering );
	PARENT_CLASS( CMoveSTApplySteering );
	PROPERTY_EDIT( m_rotationVar, TXT( "Behavior rotation variable name" ) )
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////

class CMoveSTStep : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTStep, IMoveSteeringTask, 0 );

private:
	CName			m_stepDistanceVar;
	CName			m_stepHeadingVar;
	CName			m_stepEvent;
	CName			m_stepNotification;

public:
	CMoveSTStep();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override { return TXT( "Step" ); }
};
BEGIN_CLASS_RTTI( CMoveSTStep );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_stepDistanceVar, TXT( "Behavior step distance variable name" ) );
	PROPERTY_EDIT( m_stepHeadingVar, TXT( "Behavior step heading variable name" ) );
	PROPERTY_EDIT( m_stepEvent, TXT( "Behavior step event name" ) );
	PROPERTY_EDIT( m_stepNotification, TXT( "Behavior step event end notification" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveSTSlide : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSlide, IMoveSteeringTask, 0 );

public:
	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override { return TXT( "Slide" ); }
};
BEGIN_CLASS_RTTI( CMoveSTSlide );
	PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveSTSetBehaviorVariable : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSetBehaviorVariable, IMoveSteeringTask, 0 );

private:
	EBehaviorVarContext	m_variableContext;
	CName				m_variableName;
	Float				m_value;

public:
	CMoveSTSetBehaviorVariable();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override { return TXT( "SetBehaviorVariable" ); }
};
BEGIN_CLASS_RTTI( CMoveSTSetBehaviorVariable );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_BITFIELD_EDIT( m_variableContext, EBehaviorVarContext, TXT("Variable context") );
	PROPERTY_EDIT( m_variableName, TXT( "Variable to set" ) );
	PROPERTY_EDIT( m_value, TXT( "Value to set" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum EBitOperation
{
	BO_And,
	BO_Or,
	BO_Xor
};
BEGIN_ENUM_RTTI( EBitOperation );
	ENUM_OPTION( BO_And );
	ENUM_OPTION( BO_Or );
	ENUM_OPTION( BO_Xor );
END_ENUM_RTTI();

class CMoveSTSetFlags : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSetFlags, IMoveSteeringTask, 0 );

private:
	Uint8				m_movementFlags;
	EBitOperation		m_bitOperation;

public:
	CMoveSTSetFlags();

	virtual void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const;

	virtual String GetTaskName() const { return TXT( "SetMovementFlags" ); }
};
BEGIN_CLASS_RTTI( CMoveSTSetFlags )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_BITFIELD_EDIT( m_movementFlags, EMovementFlags, TXT("Movement flags to set") );
	PROPERTY_EDIT( m_bitOperation, TXT( "Bit operation to us" ) );
END_CLASS_RTTI()

	///////////////////////////////////////////////////////////////////////////////

class CMoveSTSetupRotationChange : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSetupRotationChange, IMoveSteeringTask, 0 );
protected:
	Float			m_maxDirectionChange;
	Float			m_maxRotationChange;
public:
	CMoveSTSetupRotationChange();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTSetupRotationChange )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_maxDirectionChange, TXT( "Max direction change" ) );
	PROPERTY_EDIT( m_maxRotationChange, TXT( "Max rotation change" ) );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSTMapRotationChangeUsingCustomRotation : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTMapRotationChangeUsingCustomRotation, IMoveSteeringTask, 0 );
protected:
	Float								m_defaultMaxDirectionChange;
	Float								m_defaultMaxRotationChange;

public:
	CMoveSTMapRotationChangeUsingCustomRotation();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTMapRotationChangeUsingCustomRotation )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_defaultMaxDirectionChange, TXT( "Default max direction change" ) );
	PROPERTY_EDIT( m_defaultMaxRotationChange, TXT( "Default max rotation change" ) );
END_CLASS_RTTI()

	///////////////////////////////////////////////////////////////////////////////

class CMoveSTSetMaxDirectionChange : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSetMaxDirectionChange, IMoveSteeringTask, 0 );

private:
	Float			m_angle;

public:
	CMoveSTSetMaxDirectionChange();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTSetMaxDirectionChange )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_angle, TXT( "Max direction change" ) );
END_CLASS_RTTI()


	///////////////////////////////////////////////////////////////////////////////

class CMoveSTSetMaxRotationChange : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSetMaxRotationChange, IMoveSteeringTask, 0 );

private:
	Float			m_angle;

public:
	CMoveSTSetMaxRotationChange();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTSetMaxRotationChange )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_angle, TXT( "Max rotation change" ) );
END_CLASS_RTTI()

	///////////////////////////////////////////////////////////////////////////////

class CMoveSTSetAcceleration : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTSetAcceleration, IMoveSteeringTask, 0 );

private:
	Float			m_acceleration;
	Float			m_deceleration;

public:
	CMoveSTSetAcceleration();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTSetAcceleration )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_acceleration, TXT( "Max acceleration" ) );
	PROPERTY_EDIT( m_deceleration, TXT( "Max deceleration" ) );
END_CLASS_RTTI()

	///////////////////////////////////////////////////////////////////////////////

	enum EAvoidObstacleStrategy
{
	AOS_None,
	AOS_Left,
	AOS_Right,
	AOS_Follow,
	AOS_SlowDown,
	AOS_SpeedUp,
	AOS_Stop
};

BEGIN_ENUM_RTTI( EAvoidObstacleStrategy );
	ENUM_OPTION( AOS_None );
	ENUM_OPTION( AOS_Left );
	ENUM_OPTION( AOS_Right );
	ENUM_OPTION( AOS_Follow );
	ENUM_OPTION( AOS_SlowDown );
	ENUM_OPTION( AOS_SpeedUp );
	ENUM_OPTION( AOS_Stop );
END_ENUM_RTTI();

class CMoveSTAvoidObstacles : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTAvoidObstacles, IMoveSteeringTask, 0 );

private:
	Float				m_avoidObstaclesImportance;
	Float				m_timeToChooseNextObstacle;
	Float				m_maxDistanceToObstacle;
	Float				m_furthestImpactTime;
	Float				m_directionChangeOverride;
	Float				m_minVelocityToOverrideDirectionChange;
	Bool				m_overrideValues;
	Bool				m_modifyRotation;
	Bool				m_modifyHeading;
	Bool				m_modifySpeed;

	TInstanceVar< Float >					i_timeToChooseNextObstacle;
	TInstanceVar< Int32 >					i_avoidingSameObstacleCount;
	TInstanceVar< Int32 >					i_switchAvoidDirectionsCounter;
	TInstanceVar< THandle< CActor > >		i_avoidObstacle;
	TInstanceVar< THandle< CActor > >		i_prevAvoidObstacle;
	TInstanceVar< EAvoidObstacleStrategy >	i_avoidObstacleStrategy;
	TInstanceVar< Bool >					i_clearRotation;
	TInstanceVar< Float >					i_requiredRotationChange;
	TInstanceVar< Float >					i_predictedImpactTime;
	TInstanceVar< Vector >					i_obstacleColLoc;
	TInstanceVar< Vector >					i_agentColLoc;
	TInstanceVar< Float >					i_avoidanceTurnActive;
	TInstanceVar< Float >					i_avoidanceRadius;

public:
	CMoveSTAvoidObstacles();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String GetTaskName() const override;
	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	void OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;

private:
	RED_INLINE static Float CalcSqDist( Float t, const Vector& aLoc, const Vector& aVel, const Vector& bLoc, const Vector& bVel );
	RED_INLINE static void CalcClosestTimeWorker( Bool& collided, Float& t, Float stepT, Float minSqDist, const Vector& aLoc, const Vector& aVel, const Vector& bLoc, const Vector& bVel );
	RED_INLINE static Bool CalcClosestTime( Float& outT, Float minT, Float maxT, Float minDist, const Vector& aLoc, const Vector& aVel, const Vector& bLoc, const Vector& bVel );

	RED_INLINE void FindObstacleToAvoid( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const;
	RED_INLINE void ProcessAvoidance( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const;

	RED_INLINE void ModifySpeed( IMovementCommandBuffer& comm, Float newSpeed, Float importance ) const;
	RED_INLINE void ModifyRotation( IMovementCommandBuffer& comm, Float rotationChange, Float importance ) const;
	RED_INLINE void ModifyHeading( IMovementCommandBuffer& comm, const Vector2& newHeading, Float importance ) const;
};

BEGIN_CLASS_RTTI( CMoveSTAvoidObstacles );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_avoidObstaclesImportance, TXT( "Obstacle avoidance importance" ) );
	PROPERTY_EDIT( m_timeToChooseNextObstacle, TXT( "Choose obstacle interval") );
	PROPERTY_EDIT( m_maxDistanceToObstacle, TXT( "Maximal distance to obstacle" ) );
	PROPERTY_EDIT( m_furthestImpactTime, TXT( "Furthest time to investigate when looking for collision" ))
	PROPERTY_EDIT( m_overrideValues, TXT( "Override values instead of adding (will use even if locked)" ) );
	PROPERTY_EDIT( m_modifyRotation, TXT( "Modify rotation in locomotion" ) );
	PROPERTY_EDIT( m_modifyHeading, TXT( "Modify heading in locomotion" ) );
	PROPERTY_EDIT( m_modifySpeed, TXT( "Modify speed in locomotion" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveSTCollisionResponse : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTCollisionResponse, IMoveSteeringTask, 0 );
protected:
	Float			m_headingImportanceMin;
	Float			m_headingImportanceMax;
	Float			m_radiusMult;

	void			CollisionResponse( IMovementCommandBuffer& comm, InstanceBuffer& data ) const;
public:
	CMoveSTCollisionResponse()
		: m_headingImportanceMin( 0.2f )
		, m_headingImportanceMax( 0.5f )
		, m_radiusMult( 1.f )													{}

	void			CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String			GetTaskName() const override;
	void			OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void			OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	void			OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};
BEGIN_CLASS_RTTI( CMoveSTCollisionResponse );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_headingImportanceMin, TXT("Minimum heading importance") )
	PROPERTY_EDIT( m_headingImportanceMax, TXT("Maximum heading importance") )
	PROPERTY_EDIT( m_radiusMult, TXT("Physical radius multiplier") )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CMoveSTForwardCollisionResponse : public CMoveSTCollisionResponse
{
	DECLARE_ENGINE_CLASS( CMoveSTForwardCollisionResponse, CMoveSTCollisionResponse, 0 );
protected:
	Float						m_probeDistanceInTime;
	CName						m_crowdThroughVar;

	TInstanceVar< Float >		i_crowdThrough;

	Float			ForwardCollisionResponse( IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	void			DeactivateAnimation( CMovingAgentComponent& agent, InstanceBuffer& data ) const;
public:
	CMoveSTForwardCollisionResponse()
		: m_probeDistanceInTime( 2.f )
		, m_crowdThroughVar( CNAME( CrowdPushThrough ) )						{}

	void			CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String			GetTaskName() const override;

	void			OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void			OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	void			OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;
	void			OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;
};
BEGIN_CLASS_RTTI( CMoveSTForwardCollisionResponse )
	PARENT_CLASS( CMoveSTCollisionResponse )
	PROPERTY_EDIT( m_probeDistanceInTime, TXT("Probe for collision in given distance ahead (in seconds)") )
	PROPERTY_EDIT( m_crowdThroughVar, TXT("Crowd through var") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

enum EAdjustRotationChangesScenario
{
	ARCS_None,
	ARCS_CasualMovement,
};

BEGIN_ENUM_RTTI( EAdjustRotationChangesScenario );
ENUM_OPTION( ARCS_None );
ENUM_OPTION( ARCS_CasualMovement );
END_ENUM_RTTI();

class CMoveSTAdjustRotationChanges : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTAdjustRotationChanges, IMoveSteeringTask, 0 );

private:
	EAdjustRotationChangesScenario m_scenario;

	TInstanceVar< Bool > i_doingLargeTurnStart;
	TInstanceVar< Float > i_largeTurnStartRotationRate;
	TInstanceVar< Bool > i_stopping;

public:
	CMoveSTAdjustRotationChanges();

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTAdjustRotationChanges );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_scenario, TXT( "Scenario") );
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////
class CMoveSTKeepAwayTarget : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTKeepAwayTarget, IMoveSteeringTask, 0 );

protected:
	Float m_headingImportance;
	Float m_speed;
public:
	CMoveSTKeepAwayTarget()
		: m_headingImportance( 0.5f )
		, m_speed( 1.0f )				{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTKeepAwayTarget );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_headingImportance, TXT( "put a negative importance if you want to be attracted by the steering goal") );
	PROPERTY_EDIT( m_speed, TXT( "speed of the actor" ) )
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////
class CMoveSTMoveTightening : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTMoveTightening, IMoveSteeringTask, 0 );
public:
	CMoveSTMoveTightening()														{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTMoveTightening );
	PARENT_CLASS( IMoveSteeringTask );
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////
class CMoveSTResolveStucking : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTResolveStucking, IMoveSteeringTask, 0 );
protected:
	TInstanceVar< Uint32 > i_numberOfFramesStuck;
	TInstanceVar< Bool > i_isColliding;

	CName m_signalName;
	Uint32 m_stuckFramesThreshold;
	Float m_distanceThreshold;

protected:
	Bool IsColliding( const CActor& actor, float radius ) const;
	void OnDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const;

public:
	CMoveSTResolveStucking() : m_stuckFramesThreshold( 5 ), m_distanceThreshold( 0.0001f ) {}

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	virtual void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	virtual void OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;
	virtual void OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;
	virtual String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTResolveStucking );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_stuckFramesThreshold, TXT( "Disable collision if npc is colliding and not moving for stuckFramesThreshold frames" ) )
	PROPERTY_EDIT( m_distanceThreshold, TXT( "Distance threshold" ) )
	PROPERTY_EDIT( m_signalName, TXT( "Signal name" ) )
END_CLASS_RTTI();
