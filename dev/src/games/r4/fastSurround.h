#pragma once

#include "../../common/game/moveSteeringCondition.h"

class CMoveSCFastSurround : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCFastSurround, IMoveSteeringCondition, 0 );

	Float							m_usageDelay;
	Float							m_angleDistanceToActivate;
	Float							m_speedMinToActivate;
	Float							m_angleDistanceToBreak;
	Float							m_speedMinLimitToBreak;

	// runtime data
	TInstanceVar< Bool >			i_isFastSurrounding;
	TInstanceVar< Bool >			i_hasLowVelocity;
	TInstanceVar< EngineTime >		i_nextActivationDelay;
	TInstanceVar< EngineTime >		i_lowVelocityBreakDelay;
	TInstanceVar< EngineTime >		i_isFastSurroundingSince;

public:
	CMoveSCFastSurround();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;

	String GetConditionName() const override;

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
	void OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;
};
BEGIN_CLASS_RTTI( CMoveSCFastSurround )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_usageDelay, TXT("Reactivation delay") )
	PROPERTY_EDIT( m_angleDistanceToActivate, TXT("Minimal distance from desired target to activate ") )
	PROPERTY_EDIT( m_speedMinToActivate, TXT("Minimal speed strafing output to activate") )
	PROPERTY_EDIT( m_angleDistanceToBreak, TXT("Distance at which behavior breaks") )
	PROPERTY_EDIT( m_speedMinLimitToBreak, TXT("Maximal speed output at which behavior breaks") )
END_CLASS_RTTI()

class CMoveSCTargetsCount : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCTargetsCount, IMoveSteeringCondition, 0 );

	Uint32							m_count;

public:
	CMoveSCTargetsCount()
		: m_count( 0 )																{}

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;

	String GetConditionName() const override;
};
BEGIN_CLASS_RTTI( CMoveSCTargetsCount )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_count, TXT("Enemies count") )
END_CLASS_RTTI()

