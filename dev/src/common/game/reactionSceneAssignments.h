/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeSelector.h"

class CBehTreeNodeReactionSceneAssignmentsInstance;

	////////////////////////////////////////////////////////////////////////
	// Definition
	////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSceneAssignmentsDefinition : public CBehTreeNodeSelectorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeReactionSceneAssignmentsDefinition, CBehTreeNodeSelectorDefinition, CBehTreeNodeReactionSceneAssignmentsInstance, ReactionSceneAssignments );
protected:	
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	
};

BEGIN_CLASS_RTTI( CBehTreeNodeReactionSceneAssignmentsDefinition );
	PARENT_CLASS( CBehTreeNodeSelectorDefinition );	
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSceneAssignmentsInstance : public CBehTreeNodeEvaluatingFirstAvailableSelector
{
	typedef CBehTreeNodeEvaluatingFirstAvailableSelector Super;	
	
public:
	typedef CBehTreeNodeReactionSceneAssignmentsDefinition Definition;

	CBehTreeNodeReactionSceneAssignmentsInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	
	Bool IsAvailable() override;	
};