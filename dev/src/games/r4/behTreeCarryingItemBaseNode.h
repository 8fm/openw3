#pragma once

#include "../../common/game/behTreeNode.h"

#include "behTreeCarryingItemData.h"

class CBehTreeNodeCarryingItemBaseInstance;

////////////////////////////////////////////////////////////////////////
// DEFINITION
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeCarryingItemBaseDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeCarryingItemBaseDefinition, IBehTreeNodeDefinition, CBehTreeNodeCarryingItemBaseInstance, BaseCarryingItemNode );
public:
	CBehTreeNodeCarryingItemBaseDefinition() {}
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeCarryingItemBaseDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeCarryingItemBaseInstance : public IBehTreeNodeInstance
{
protected:
	CBehTreeCarryingItemDataPtr		m_carryingItemsData;
public:
	typedef CBehTreeNodeCarryingItemBaseDefinition Definition;

	CBehTreeNodeCarryingItemBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );	
};