/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeTicketDecoratorBase.h"

class CBehTreeNodeCombatTicketLockInstance;

class CBehTreeNodeCombatTicketLockDefinition : public IBehTreeNodeCombatTicketDecoratorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatTicketLockDefinition, IBehTreeNodeCombatTicketDecoratorBaseDefinition, CBehTreeNodeCombatTicketLockInstance, TicketLock )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatTicketLockDefinition )
	PARENT_CLASS( IBehTreeNodeCombatTicketDecoratorBaseDefinition )
END_CLASS_RTTI()

class CBehTreeNodeCombatTicketLockInstance : public IBehTreeNodeCombatTicketDecoratorBaseInstance
{
	typedef IBehTreeNodeCombatTicketDecoratorBaseInstance Super;

public:
	typedef CBehTreeNodeCombatTicketLockDefinition Definition;

	CBehTreeNodeCombatTicketLockInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}

	Bool Activate() override;
	void Deactivate() override;
};