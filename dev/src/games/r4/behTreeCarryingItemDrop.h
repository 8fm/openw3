#pragma once

#include "../../common/game/behTreeInstance.h"

#include "behTreeCarryingItemBaseNode.h"
#include "behTreeCarryingItemData.h"

class CBehTreeDropCarryableItemInstance;
class CBehTreeNodeDropItemDefinition : public CBehTreeNodeCarryingItemBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDropItemDefinition, CBehTreeNodeCarryingItemBaseDefinition, CBehTreeDropCarryableItemInstance, DropCarryableItem );
protected:	

public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	
};

BEGIN_CLASS_RTTI( CBehTreeNodeDropItemDefinition );
PARENT_CLASS( CBehTreeNodeCarryingItemBaseDefinition );	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
class CBehTreeDropCarryableItemInstance : public CBehTreeNodeCarryingItemBaseInstance
{
	typedef CBehTreeNodeCarryingItemBaseInstance Super;
protected:	
	Bool	m_slideDone;
	Bool	m_canBeCompleted;
public:
	typedef CBehTreeNodeDropItemDefinition Definition;

	CBehTreeDropCarryableItemInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: CBehTreeNodeCarryingItemBaseInstance( def, owner, context, parent ) 				
	{}

	Bool IsAvailable() override;
	Bool Activate() override;	
	void Update() override;	
	Bool OnEvent( CBehTreeEvent& e ) override;
};