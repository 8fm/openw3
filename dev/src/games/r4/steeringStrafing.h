#pragma once

#include "../../common/game/targetSteeringTask.h"

///////////////////////////////////////////////////////////////////////////////
// Base class for strafing tasks
class IMoveSTBaseStrafeTarget : public IMoveTargetPositionSteeringTask
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMoveSTBaseStrafeTarget, IMoveTargetPositionSteeringTask );

protected:
	Float						m_importance;
	Float						m_acceleration;
	Float						m_moveSpeed;

	TInstanceVar< Float >		i_lastOutput;

	virtual Float CalculateDesiredOutput( IMovementCommandBuffer& comm, InstanceBuffer& data, const Vector& targetPos, Float timeDelta ) const;
public:
	IMoveSTBaseStrafeTarget()
		: m_importance( 0.5f )
		, m_acceleration( 1.f )
		, m_moveSpeed( 1.f )													{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IMoveSTBaseStrafeTarget );
	PARENT_CLASS( IMoveTargetPositionSteeringTask );
	PROPERTY_EDIT( m_importance, TXT("Heading importance") );
	PROPERTY_EDIT( m_acceleration, TXT( "Output change speed" ) );
	PROPERTY_EDIT( m_moveSpeed, TXT( "Base task speed" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
// Strafe around target taking into account other target attackers
class CMoveSTStrafeSurroundTarget : public IMoveTargetSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTStrafeSurroundTarget, IMoveTargetSteeringTask, 0 );

protected:
	Float						m_importance;
	Float						m_acceleration;
	Float						m_moveSpeed;
	Float						m_desiredSeparationAngle;
	Float						m_toleranceAngle;
	Float						m_smoothAngle;
	Int32						m_strafingRing;
	Bool						m_gravityToSeparationAngle;

	TInstanceVar< Float >		i_lastOutput;
public:
	CMoveSTStrafeSurroundTarget()
		: m_importance( 0.5f )
		, m_acceleration( 1.f )
		, m_moveSpeed( 1.f )
		, m_desiredSeparationAngle( 180.f )
		, m_toleranceAngle( 10.f )
		, m_smoothAngle( 30.f )
		, m_strafingRing( -1 )
		, m_gravityToSeparationAngle( true )									{}

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};

BEGIN_CLASS_RTTI( CMoveSTStrafeSurroundTarget );
	PARENT_CLASS( IMoveTargetSteeringTask );
	PROPERTY_EDIT( m_importance, TXT("Heading importance") );
	PROPERTY_EDIT( m_acceleration, TXT( "Output change speed" ) );
	PROPERTY_EDIT( m_moveSpeed, TXT( "Base task speed" ) );
	PROPERTY_EDIT( m_desiredSeparationAngle, TXT( "Desired angular distance to closest ally" ) );
	PROPERTY_EDIT( m_toleranceAngle, TXT( "Angle in which algorithm don't produce output" ) );
	PROPERTY_EDIT( m_smoothAngle, TXT( "Angle to start decreasing speed" ) );
	PROPERTY_EDIT( m_strafingRing, TXT("Strafing ring") );
	PROPERTY_EDIT( m_gravityToSeparationAngle, TXT( "If this flag is on, if npc is above desiredSeparationAngle from ally, he will steer back to him." ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
class CMoveSTStrafeTargetRandomly : public IMoveSTBaseStrafeTarget
{
	DECLARE_ENGINE_CLASS( CMoveSTStrafeTargetRandomly, IMoveSTBaseStrafeTarget, 0 );

protected:
	Float						m_randomizationFrequency;
	Float						m_outputRandomizationPower;
	Float						m_changeDirectionOnBlockDelay;

	TInstanceVar< Float >		i_desiredOutput;
	TInstanceVar< EngineTime >	i_decisionTimeout;
	TInstanceVar< EngineTime >	i_blockTestTimeout;
	TInstanceVar< EngineTime >	i_blockTestLockTimeout;
	TInstanceVar< Bool >		i_isBlocked;

	Float CalculateDesiredOutput( IMovementCommandBuffer& comm, InstanceBuffer& data, const Vector& targetPos, Float timeDelta ) const override;

public:
	CMoveSTStrafeTargetRandomly()
		: m_randomizationFrequency( 8.f )
		, m_outputRandomizationPower( 0 )
		, m_changeDirectionOnBlockDelay( -1.f )								{ m_importance = 0.25f; }

	String GetTaskName() const override;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	void OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;

	void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;
};

BEGIN_CLASS_RTTI( CMoveSTStrafeTargetRandomly );
	PARENT_CLASS( IMoveSTBaseStrafeTarget );
	PROPERTY_EDIT( m_randomizationFrequency, TXT( "Frequency of output change" ) );
	PROPERTY_EDIT( m_outputRandomizationPower, TXT( "Power for output randomization. 0 - no randomization, 0.5 - sqrt randomization (movement using mostly base speed), 1 - full random" ) );
	PROPERTY_EDIT( m_changeDirectionOnBlockDelay, TXT( "How often we can change output direction if we are blocked in chosen direction. (<0 for never)" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
class CMoveSTStrafeTargetOneWay : public IMoveSTBaseStrafeTarget
{
	DECLARE_ENGINE_CLASS( CMoveSTStrafeTargetOneWay, IMoveSTBaseStrafeTarget, 0 );

protected:
	Bool						m_left;

	Float		CalculateDesiredOutput( IMovementCommandBuffer& comm, InstanceBuffer& data, const Vector& targetPos, Float timeDelta ) const override;
public:
	CMoveSTStrafeTargetOneWay()
		: m_left( true )														{}

	String		GetTaskName() const override;
};
BEGIN_CLASS_RTTI( CMoveSTStrafeTargetOneWay );
	PARENT_CLASS( IMoveSTBaseStrafeTarget );
	PROPERTY_EDIT( m_left, TXT( "Mark to go counter clockwise." ) );
END_CLASS_RTTI();
