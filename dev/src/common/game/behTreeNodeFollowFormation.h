/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiFormationData.h"
#include "behTreeNodeAtomicAction.h"
#include "behTreeSteeringGraphBase.h"
#include "formationLogic.h"
#include "formationSteeringInput.h"

class CBehTreeNodeFollowFormationInstance;
class CBehTreeNodeCombatFollowFormationInstance;

class CBehTreeNodeFollowFormationDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeFollowFormationDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeFollowFormationInstance, FollowFormation );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeFollowFormationDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition)
END_CLASS_RTTI()


class CBehTreeNodeFollowFormationInstance : public CBehTreeNodeAtomicActionInstance, public CBehTreeSteeringGraphCommonInstance, public IMovementTargeter
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	CAIFormationDataPtr							m_runtimeData;
	SFormationSteeringInput						m_steeringInput;
	Vector3										m_cachupPoint;
	Float										m_cachupTestDelay;
	Bool										m_hasPathComputed;
	Bool										m_requestCachup;
	Bool										m_completed;
public:
	typedef CBehTreeNodeFollowFormationDefinition Definition;

	CBehTreeNodeFollowFormationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Deactivate() override;
	void Update() override;

	// IMovementTargeter interface
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	Bool IsFinished() const override;
};


class CBehTreeNodeCombatFollowFormationDefinition : public CBehTreeNodeFollowFormationDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatFollowFormationDefinition, CBehTreeNodeFollowFormationDefinition, CBehTreeNodeCombatFollowFormationInstance, FollowCombatFormation );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatFollowFormationDefinition )
	PARENT_CLASS( CBehTreeNodeFollowFormationDefinition)
END_CLASS_RTTI()


class CBehTreeNodeCombatFollowFormationInstance : public CBehTreeNodeFollowFormationInstance
{
	typedef CBehTreeNodeFollowFormationInstance Super;
public:
	typedef CBehTreeNodeCombatFollowFormationDefinition Definition;

	CBehTreeNodeCombatFollowFormationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )											{}

	// IMovementTargeter interface
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};