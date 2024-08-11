/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behTreeNode.h"

class CBehTreeNodeReactionSceneAssignActorNodeInstance;

class CBehTreeNodeReactionSceneAssignActorNodeDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeReactionSceneAssignActorNodeDefinition, IBehTreeNodeDefinition, CBehTreeNodeReactionSceneAssignActorNodeInstance, AssignActor );
protected:
	CName m_roleName;

public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeNodeReactionSceneAssignActorNodeDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );
	PROPERTY_EDIT( m_roleName, TXT("Role name") );
END_CLASS_RTTI();

class CBehTreeNodeReactionSceneAssignActorNodeInstance : public IBehTreeNodeInstance
{
private:
	CName m_roleName;

public:
	typedef CBehTreeNodeReactionSceneAssignActorNodeDefinition Definition;

	CBehTreeNodeReactionSceneAssignActorNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	Bool IsAvailable() override;
};