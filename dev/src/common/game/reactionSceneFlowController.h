/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeSequence.h"

class CBehTreeNodeReactionSceneFlowControllerInstance;
class CReactionScene;

	////////////////////////////////////////////////////////////////////////
	// Definition
	////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSceneFlowControllerDefinition : public CBehTreeNodeSequenceCheckAvailabilityDefinition//CBehTreeNodeSelectorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeReactionSceneFlowControllerDefinition, CBehTreeNodeSequenceCheckAvailabilityDefinition, CBehTreeNodeReactionSceneFlowControllerInstance, ReactionSceneFlowController );
protected:	
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	
};

BEGIN_CLASS_RTTI( CBehTreeNodeReactionSceneFlowControllerDefinition );
	PARENT_CLASS( CBehTreeNodeSequenceCheckAvailabilityDefinition );	
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSceneFlowControllerInstance : public CBehTreeNodeSequenceCheckAvailabilityInstance  
{
	typedef CBehTreeNodeSequenceCheckAvailabilityInstance Super;	
protected:
	THandle< CReactionScene >	m_reactionScene;
	Bool						m_delayedActivation;
public:
	typedef CBehTreeNodeReactionSceneFlowControllerDefinition Definition;

	CBehTreeNodeReactionSceneFlowControllerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );	

	Bool IsAvailable() override;
	Bool Activate() override;
	void Update() override;	
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

private:
	Bool FindAndActivateStartNode();
};