#pragma once

#include "behTreeNodeAtomicAction.h"

class CBehTreeNodeDespawnInstance;


////////////////////////////////////////////////////////////////////////
// DEFINITION
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDespawnDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDespawnDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeDespawnInstance, Despawn );

public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDespawnDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDespawnInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
public:
	typedef CBehTreeNodeDespawnDefinition Definition;

	CBehTreeNodeDespawnInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}

	Bool Activate() override;
};
