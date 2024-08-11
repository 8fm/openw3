/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeCustomMoveData.h"
#include "behTreeNodeCustomSteering.h"

class CBehTreeAtomicSteerWithTargetInstance;
class CBehTreeAtomicSteerWithCustomTargetInstance;


class CBehTreeAtomicSteerWithTargetDefinition : public CBehTreeNodeCustomSteeringDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeAtomicSteerWithTargetDefinition, CBehTreeNodeCustomSteeringDefinition, CBehTreeAtomicSteerWithTargetInstance, SteerWithTarget );
protected:
	Bool									m_useCombatTarget;
public:
	CBehTreeAtomicSteerWithTargetDefinition()
		: m_useCombatTarget( true )											{}

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeAtomicSteerWithTargetDefinition );
	PARENT_CLASS( CBehTreeNodeCustomSteeringDefinition );
	PROPERTY_EDIT( m_useCombatTarget, TXT("Use combat or action target") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeAtomicSteerWithTargetInstance : public CBehTreeNodeCustomSteeringInstance
{
	typedef CBehTreeNodeCustomSteeringInstance Super;
protected:
	Bool									m_useCombatTarget;
public:

	typedef CBehTreeAtomicSteerWithTargetDefinition Definition;

	CBehTreeAtomicSteerWithTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_useCombatTarget( def.m_useCombatTarget )					{}

	////////////////////////////////////////////////////////////////////
	void Update() override;
	////////////////////////////////////////////////////////////////////
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	Bool IsFinished() const override;
};


//////////////////////////////////////////////////////////////////////////
// Steer with custom movement target
class CBehTreeAtomicSteerWithCustomTargetDefinition : public CBehTreeNodeCustomSteeringDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeAtomicSteerWithCustomTargetDefinition, CBehTreeNodeCustomSteeringDefinition, CBehTreeAtomicSteerWithCustomTargetInstance, SteerWithCustomTarget );
public:
	CBehTreeAtomicSteerWithCustomTargetDefinition() {}

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeAtomicSteerWithCustomTargetDefinition );
	PARENT_CLASS( CBehTreeNodeCustomSteeringDefinition );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeAtomicSteerWithCustomTargetInstance : public CBehTreeNodeCustomSteeringInstance
{
	typedef CBehTreeNodeCustomSteeringInstance Super;
protected:
	CBehTreeCustomMoveDataPtr			m_ptr;
public:

	typedef CBehTreeAtomicSteerWithCustomTargetDefinition Definition;

	CBehTreeAtomicSteerWithCustomTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	void Update() override;
	////////////////////////////////////////////////////////////////////
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	Bool IsFinished() const override;
};

