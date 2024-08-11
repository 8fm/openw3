/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeDynamicNode.h"

class CBehTreeNodeDynamicCombatStyleInstance;

class CBehTreeNodeDynamicCombatStyleDefinition : public IBehTreeDynamicNodeBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDynamicCombatStyleDefinition, IBehTreeDynamicNodeBaseDefinition, CBehTreeDynamicNodeInstance, DynamicCombatStyle );
public:
	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDynamicCombatStyleDefinition )
	PARENT_CLASS( IBehTreeDynamicNodeBaseDefinition )
END_CLASS_RTTI()