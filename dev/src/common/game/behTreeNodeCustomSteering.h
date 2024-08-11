/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicAction.h"
#include "behTreeVarsEnums.h"
#include "behTreeSteeringGraphBase.h"

class CBehTreeNodeCustomSteeringInstance;

class CBehTreeNodeCustomSteeringDefinition : public CBehTreeNodeAtomicActionDefinition, public CBehTreeSteeringGraphCommonDef
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCustomSteeringDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeCustomSteeringInstance, CustomSteering );
protected:
	CBehTreeValEMoveType				m_moveType;
	CBehTreeValFloat					m_moveSpeed;
public:
	CBehTreeNodeCustomSteeringDefinition()
		: m_moveType( MT_Run )
		, m_moveSpeed( 1.f )											{};

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCustomSteeringDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
	PROPERTY_EDIT( m_steeringGraph, TXT("Steering graph resource") );
	PROPERTY_EDIT( m_moveType, TXT("Default movement type") );
	PROPERTY_EDIT( m_moveSpeed, TXT("Default movement speed") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeCustomSteeringInstance : public CBehTreeNodeAtomicActionInstance, public CBehTreeSteeringGraphCommonInstance, public IMovementTargeter
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	EMoveType							m_moveType;
	Float								m_moveSpeed;
public:
	typedef CBehTreeNodeCustomSteeringDefinition Definition;

	CBehTreeNodeCustomSteeringInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	// IMovementTargeter interface
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	Bool IsFinished() const override;

	EMoveType GetDefaultMoveType() const								{ return m_moveType; }
	Float GetDefaultMoveSpeed() const									{ return m_moveSpeed; }

	virtual EMoveType GetMoveType()const { return m_moveType; }
};