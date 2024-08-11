#pragma once

#include "../../common/game/behTreeNodeCondition.h"
#include "behTreeCarryingItemData.h"
#include "behTreeCarryingItemBaseDecorator.h"

class CBehTreeDecoratorFindDestinationItemStoreInstance;
class CBehTreeDecoratorFindDestinationItemStoreDefinition : public CBehTreeDecoratorCarryingItemsBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorFindDestinationItemStoreDefinition, CBehTreeDecoratorCarryingItemsBaseDefinition, CBehTreeDecoratorFindDestinationItemStoreInstance, FindDestinationItemStore );
protected:

public:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorFindDestinationItemStoreDefinition );
	PARENT_CLASS( CBehTreeDecoratorCarryingItemsBaseDefinition );	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
class CBehTreeDecoratorFindDestinationItemStoreInstance : public CBehTreeDecoratorCarryingItemsBaseInstance
{
	typedef CBehTreeDecoratorCarryingItemsBaseInstance Super;
protected:	

	Bool IfStorePointValid( CCarryableItemStorePointComponent* storePoint, CBehTreeCarryingItemData* carryingData ) override;
	void SetTargetMoveData( CCarryableItemStorePointComponent* storePoint ) override;
public:
	typedef CBehTreeDecoratorFindDestinationItemStoreDefinition Definition;

	CBehTreeDecoratorFindDestinationItemStoreInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: CBehTreeDecoratorCarryingItemsBaseInstance( def, owner, context, parent ) 
		
	{}
	
	Bool IsAvailable() override;	
};