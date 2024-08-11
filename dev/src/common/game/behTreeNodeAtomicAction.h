/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNode.h"

class CBehTreeNodeAtomicActionInstance;

////////////////////////////////////////////////////////////////////////
// DEFINITION
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicActionDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeAtomicActionDefinition, IBehTreeNodeDefinition, CBehTreeNodeAtomicActionInstance, AtomicAction );
public:
	Bool OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeAtomicActionDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicActionInstance : public IBehTreeNodeInstance
{
	typedef IBehTreeNodeInstance Super;
public:
	typedef CBehTreeNodeAtomicActionDefinition Definition;

	CBehTreeNodeAtomicActionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{ }

	static CName MovementStartedEventName()								{ return CNAME( AI_NewMovementAction ); }
};