/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeCustomMoveData.h"
#include "behTreeDecorator.h"
#include "behTreeInstance.h"

class IBehTreeNodeDecoratorSteeringTargeterInstance;
class CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance;
class CBehTreeNodeDecoratorSetSteeringCustomPositionInstance;


///////////////////////////////////////////////////////////////////////////////
// General class for decorators that acts as additional movement targeters,
// that can possibly override some steering input assigned by child nodes.
///////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeDecoratorSteeringTargeterDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeDecoratorSteeringTargeterDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeDecoratorSteeringTargeterInstance, SteeringTargeterDecorator )
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeDecoratorSteeringTargeterDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

class IBehTreeNodeDecoratorSteeringTargeterInstance : public IBehTreeNodeDecoratorInstance, public IMovementTargeter, public IMoveLocomotionListener
{
	typedef IBehTreeNodeDecoratorInstance Super;

public:
	IBehTreeNodeDecoratorSteeringTargeterInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )										{}

	Bool				Activate() override;
	void				Deactivate() override;

	// IMovementTargeter
	void				UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) = 0;
	Bool				IsFinished() const override;

	// IMoveLocomotionListener
	void				OnSegmentPushed( IMoveLocomotionSegment* segment ) override;
};

///////////////////////////////////////////////////////////////////////////////
// Sets steering target node to action/combat target
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition : public IBehTreeNodeDecoratorSteeringTargeterDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition, IBehTreeNodeDecoratorSteeringTargeterDefinition, IBehTreeNodeDecoratorSteeringTargeterInstance, SetSteeringTargetNode )
protected:
	Bool				m_combatTarget;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition()
		: m_combatTarget( true )															{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorSteeringTargeterDefinition )
	PROPERTY_EDIT( m_combatTarget, TXT("Use combat or action target") )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorSetSteeringCombatTargetNodeInstance : public IBehTreeNodeDecoratorSteeringTargeterInstance
{
	typedef IBehTreeNodeDecoratorSteeringTargeterInstance Super;
public:
	typedef CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition Definition;

	CBehTreeNodeDecoratorSetSteeringCombatTargetNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}

	void				UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};

class CBehTreeNodeDecoratorSetSteeringActionTargetNodeInstance : public IBehTreeNodeDecoratorSteeringTargeterInstance
{
	typedef IBehTreeNodeDecoratorSteeringTargeterInstance Super;
public:
	typedef CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition Definition;

	CBehTreeNodeDecoratorSetSteeringActionTargetNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}

	void				UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};


///////////////////////////////////////////////////////////////////////////////
// Sets steering named target to action/combat target
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition : public IBehTreeNodeDecoratorSteeringTargeterDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition, IBehTreeNodeDecoratorSteeringTargeterDefinition, CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance, SetSteeringNamedTarget )
protected:
	CName				m_targetName;
	Bool				m_combatTarget;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition()
		: m_combatTarget( true )															{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorSteeringTargeterDefinition )
	PROPERTY_EDIT( m_targetName, TXT("Target name referenced from steering graph") )
	PROPERTY_EDIT( m_combatTarget, TXT("Use combat or action target") )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance : public IBehTreeNodeDecoratorSteeringTargeterInstance
{
	typedef IBehTreeNodeDecoratorSteeringTargeterInstance Super;
protected:
	CName				m_targetName;
public:
	typedef CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition Definition;

	CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_targetName( def.m_targetName )													{}
};

class CBehTreeNodeDecoratorSetSteeringNamedTargetWithCombatTargetNodeInstance : public CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance
{
	typedef CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance Super;
public:
	typedef CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition Definition;

	CBehTreeNodeDecoratorSetSteeringNamedTargetWithCombatTargetNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}

	void				UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};

class CBehTreeNodeDecoratorSetSteeringNamedTargetWithActionTargetNodeInstance : public CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance
{
	typedef CBehTreeNodeDecoratorSetSteeringNamedTargetNodeInstance Super;
public:
	typedef CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition Definition;

	CBehTreeNodeDecoratorSetSteeringNamedTargetWithActionTargetNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}

	void				UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};

///////////////////////////////////////////////////////////////////////////////
// Sets steering custom position
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorSetSteeringCustomPositionDefinition : public IBehTreeNodeDecoratorSteeringTargeterDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorSetSteeringCustomPositionDefinition, IBehTreeNodeDecoratorSteeringTargeterDefinition, CBehTreeNodeDecoratorSetSteeringCustomPositionInstance, SetSteeringCustomPosition )
protected:
	CName				m_steeringParameterName;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorSetSteeringCustomPositionDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorSteeringTargeterDefinition )
	PROPERTY_EDIT( m_steeringParameterName, TXT("Name used by steering graph to obtain custom position") )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorSetSteeringCustomPositionInstance : public IBehTreeNodeDecoratorSteeringTargeterInstance
{
	typedef IBehTreeNodeDecoratorSteeringTargeterInstance Super;
protected:
	CName						m_steeringParameterName;
	CBehTreeCustomMoveDataPtr	m_customMoveData;
public:
	typedef CBehTreeNodeDecoratorSetSteeringCustomPositionDefinition Definition;

	CBehTreeNodeDecoratorSetSteeringCustomPositionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_steeringParameterName( def.m_steeringParameterName )
		, m_customMoveData( owner )													{}

	void				UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};