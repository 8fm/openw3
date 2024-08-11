#pragma once

#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/behTreeVars.h"

#include "behTreeCarryingItemBaseDecorator.h"
#include "behTreeCarryingItemData.h"

class CBehTreeDecoratorFindSourceItemStoreInstance;
class CBehTreeDecoratorFindSourceItemStoreDefinition : public CBehTreeDecoratorCarryingItemsBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorFindSourceItemStoreDefinition, CBehTreeDecoratorCarryingItemsBaseDefinition, CBehTreeDecoratorFindSourceItemStoreInstance, FindSourceItemStore );
protected:

public:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorFindSourceItemStoreDefinition );
	PARENT_CLASS( CBehTreeDecoratorCarryingItemsBaseDefinition );		
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
class CBehTreeDecoratorFindSourceItemStoreInstance : public CBehTreeDecoratorCarryingItemsBaseInstance
{
	typedef CBehTreeDecoratorCarryingItemsBaseInstance Super;
protected:

	Bool IfStorePointValid( CCarryableItemStorePointComponent* storePoint, CBehTreeCarryingItemData* carryingData ) override;
	void SetTargetMoveData( CCarryableItemStorePointComponent* storePoint ) override;

public:
	typedef CBehTreeDecoratorFindSourceItemStoreDefinition Definition;

	CBehTreeDecoratorFindSourceItemStoreInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );		

	Bool IsAvailable() override;	
};