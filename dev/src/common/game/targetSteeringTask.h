/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "moveSteeringTask.h"

class IMoveTargetSteeringTask : public IMoveSteeringTask
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMoveTargetSteeringTask, IMoveSteeringTask );

protected:
	CName						m_namedTarget;

	CNode*						GetTarget( const SMoveLocomotionGoal& goal ) const;
public:
	IMoveTargetSteeringTask()													{}

	String						GetTaskName() const override;

};


BEGIN_ABSTRACT_CLASS_RTTI( IMoveTargetSteeringTask )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_namedTarget, TXT("Use named target insteady of standard steering target") )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////
class IMoveTargetPositionSteeringTask : public IMoveTargetSteeringTask
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMoveTargetPositionSteeringTask, IMoveTargetSteeringTask );

protected:
	CName						m_customPosition;

	Bool						GetTargetPosition( const SMoveLocomotionGoal& goal, Vector& outTargetPosition ) const;
public:
	IMoveTargetPositionSteeringTask()											{}

	String						GetTaskName() const override;

};


BEGIN_ABSTRACT_CLASS_RTTI( IMoveTargetPositionSteeringTask )
	PARENT_CLASS( IMoveTargetSteeringTask )
	PROPERTY_EDIT( m_customPosition, TXT("Use named custom position insteady of standard steering target") )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////
// Keep distance to target
class CMoveSTKeepDistanceToTarget : public IMoveTargetPositionSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTKeepDistanceToTarget, IMoveTargetPositionSteeringTask, 0 );

protected:
	Float						m_importance;
	Float						m_acceleration;
	Float						m_moveSpeed;
	Float						m_minRange;
	Float						m_maxRange;
	Float						m_tolerance;
	Float						m_brakeDistance;
	Float						m_randomizationFrequency;

	TInstanceVar< EngineTime >	i_decisionTimeout;
	TInstanceVar< Float >		i_desiredDistance;
	TInstanceVar< Float >		i_lastOutput;

public:
	CMoveSTKeepDistanceToTarget()
		: m_importance( 0.5f )
		, m_acceleration( 2.f )
		, m_moveSpeed( 1.f )
		, m_minRange( 4.f )
		, m_maxRange( 7.f )
		, m_tolerance( 0.4f )
		, m_brakeDistance( 2.f )
		, m_randomizationFrequency( 7.5f )
	{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};

BEGIN_CLASS_RTTI( CMoveSTKeepDistanceToTarget );
	PARENT_CLASS( IMoveTargetPositionSteeringTask );
	PROPERTY_EDIT( m_importance, TXT("Heading importance") );
	PROPERTY_EDIT( m_acceleration, TXT( "Output change speed" ) );
	PROPERTY_EDIT( m_moveSpeed, TXT( "Base task speed" ) );
	PROPERTY_EDIT( m_minRange, TXT( "Min keep distance distance" ) );
	PROPERTY_EDIT( m_maxRange, TXT( "Max keep distance distance" ) );
	PROPERTY_EDIT( m_tolerance, TXT( "Max keep distance distance" ) );
	PROPERTY_EDIT( m_brakeDistance, TXT( "Max keep distance distance" ) );
	PROPERTY_EDIT( m_randomizationFrequency, TXT( "Max keep distance distance" ) );
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////
class CMoveSTNeverBackDown : public IMoveTargetPositionSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTNeverBackDown, IMoveTargetPositionSteeringTask, 0 );

protected:
	Float						m_maxAngleFromTarget;
	TInstanceVar< Float >		i_cosAngle;
	TInstanceVar< Float >		i_sinAngle;

public:
	CMoveSTNeverBackDown()
		: m_maxAngleFromTarget( 90.f )										{}

	void		CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String		GetTaskName() const override;

	void		OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void		OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};
BEGIN_CLASS_RTTI( CMoveSTNeverBackDown );
	PARENT_CLASS( IMoveTargetPositionSteeringTask );
	PROPERTY_EDIT( m_maxAngleFromTarget, TXT( "Max angle guy can be strafing 'away' of target" ) );
END_CLASS_RTTI();



///////////////////////////////////////////////////////////////////////////////
class CMoveSTFaceTarget : public IMoveTargetPositionSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTFaceTarget, IMoveTargetPositionSteeringTask, 0 );

public:
	CMoveSTFaceTarget()															{}

	void		CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String		GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTFaceTarget );
	PARENT_CLASS( IMoveTargetPositionSteeringTask );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////