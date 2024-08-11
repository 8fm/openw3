/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behTreeNode.h"

class CBehTreeNodeReactionSceneEndNodeInstance;

class CBehTreeNodeReactionSceneEndNodeDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeReactionSceneEndNodeDefinition, IBehTreeNodeDefinition, CBehTreeNodeReactionSceneEndNodeInstance, SceneEndNode );
protected:

public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeNodeReactionSceneEndNodeDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );	
END_CLASS_RTTI();

class CBehTreeNodeReactionSceneEndNodeInstance : public IBehTreeNodeInstance
{
private:	

public:
	typedef CBehTreeNodeReactionSceneEndNodeDefinition Definition;

	CBehTreeNodeReactionSceneEndNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Update() override;
	Bool IsAvailable() override{ return true; }
};