/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "movementGoal.h"

///////////////////////////////////////////////////////////////////////////////

enum EBehaviorVarContext
{
	BVC_Speed							= FLAG( 0 ),
	BVC_RotationAngle					= FLAG( 1 ),
	BVC_Extra							= FLAG( 2 ),
};

BEGIN_BITFIELD_RTTI( EBehaviorVarContext, 1 );
	BITFIELD_OPTION( BVC_Speed );
	BITFIELD_OPTION( BVC_RotationAngle );
	BITFIELD_OPTION( BVC_Extra );
END_BITFIELD_RTTI();

///////////////////////////////////////////////////////////////////////////////

#define MOVEMENT_COMMAND_BUFFER_CLEAR_VAR_MAX_NUM 4

class IMovementCommandBuffer
{
protected:
	CMovingAgentComponent*			m_movingAgent;
	SMoveLocomotionGoal				m_goal;

	Vector2							m_heading;
	Float							m_speed;
	Float							m_rotation;

	Float							m_headingImportance;
	Float							m_speedImportance;
	Float							m_rotationImportance;

	Bool							m_headingIsLocked;
	Bool							m_speedIsLocked;
	Bool							m_rotationIsLocked;
	Bool							m_goalLocked;

	void							ResetSteering();

	CName							m_varToClear[MOVEMENT_COMMAND_BUFFER_CLEAR_VAR_MAX_NUM];
	Uint16							m_varToClearNum;

	Bool							m_destinationIsReached;

public:
	IMovementCommandBuffer();
	~IMovementCommandBuffer();

	void							OnTick();
	void							OnDeactivate( CMovingAgentComponent* agent );

	CMovingAgentComponent&			GetAgent()									{ return *m_movingAgent; }
	const CMovingAgentComponent&	GetAgent() const							{ return *m_movingAgent; }

	SMoveLocomotionGoal&			LockGoal()									{ ASSERT( !m_goalLocked ); m_goalLocked = true; return m_goal; }
	void							UnlockGoal()								{ ASSERT( !m_goalLocked ); m_goalLocked = false; }

	SMoveLocomotionGoal&			GetGoal()									{ return m_goal; }
	const SMoveLocomotionGoal&		GetGoal() const								{ return m_goal; }

	void							LockHeading( const Vector2& velocity );
	void							LockSpeed( Float speed );
	void							LockRotation( Float rotation, Bool clamped = false );

	void							AddHeading( const Vector2& velocity, Float importance );
	void							AddSpeed( Float speed, Float importance );
	void							AddRotation( Float rotation, Float importance );

	void							OverrideHeading( const Vector2& velocity, Float importance );
	void							OverrideSpeed( Float speed, Float importance );
	void							OverrideRotation( Float rotation, Float importance );

	void							ModifyHeading( const Vector2& velocity )	{ m_heading = velocity; }
	void							ModifySpeed( const Float speed )			{ m_speed = speed; }

	const Vector2&					GetHeading() const							{ return m_heading; }
	Float							GetSpeed() const							{ return m_speed; }
	Float							GetRotation() const							{ return m_rotation; }

	Bool							IsHeadingSet() const						{ return m_headingImportance > 0.f || m_headingIsLocked; }
	Bool							IsSpeedSet() const							{ return m_speedImportance > 0.f || m_speedIsLocked; }
	Bool							IsRotationSet() const						{ return m_rotationImportance > 0.f || m_rotationIsLocked; }

	Float							GetHeadingImportance() const				{ return m_headingImportance; }
	Float							GetSpeedImportance() const					{ return m_speedImportance; }
	Float							GetRotationImportance() const				{ return m_rotationImportance; }

	Bool							IsDestinationReached() const				{ return m_destinationIsReached; }
	void							SetDestinationReached( Bool b = true )		{ m_destinationIsReached = b; }

	void							ResetBehavior();

	void							ClearVarOnNextTick( const CName& varName );
	void							SetVar( EBehaviorVarContext context, const CName& varName, Float val );
	void							SetVectorVar( const CName& varName, const Vector& val );
	Bool							RaiseEvent( const CName& eventName, const CName& endNotificationName, Float endNotificationTimeOut = 1.0f );
	Bool							RaiseForceEvent( const CName& eventName, const CName& endNotificationName, Float endNotificationTimeOut = 1.0f );

	void							LockHeading( const Vector& velocity )								{ LockHeading( velocity.AsVector2() ); }
	void							AddHeading( const Vector& velocity, Float importance )				{ AddHeading( velocity.AsVector2(), importance ); }
	void							OverrideHeading( const Vector& velocity, Float importance )			{ OverrideHeading( velocity.AsVector2(), importance ); }

private:
	void ClearVarsToClear( CMovingAgentComponent* agent = nullptr );
};

///////////////////////////////////////////////////////////////////////////////
