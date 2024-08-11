/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeAtomicAction.h"

class IBehTreeNodeCombatTargetSelectionBaseInstance;

////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeCombatTargetSelectionBaseDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeCombatTargetSelectionBaseDefinition, CBehTreeNodeAtomicActionDefinition, IBehTreeNodeCombatTargetSelectionBaseInstance, CombatTargetSelectionBase );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeCombatTargetSelectionBaseDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
END_CLASS_RTTI()


////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeCombatTargetSelectionBaseInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
public:
	typedef IBehTreeNodeCombatTargetSelectionBaseDefinition Definition;

	IBehTreeNodeCombatTargetSelectionBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )								{}

};