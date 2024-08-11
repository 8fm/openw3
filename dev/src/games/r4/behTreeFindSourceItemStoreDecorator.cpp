#include "build.h"

#include "behTreeFindSourceItemStoreDecorator.h"

IBehTreeNodeDecoratorInstance* CBehTreeDecoratorFindSourceItemStoreDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

Bool CBehTreeDecoratorFindSourceItemStoreInstance::IsAvailable()
{
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();

	Bool toRet = ( m_storeTag != CName::NONE && !carryingData->GetCarriedItem() && Super::IsAvailable() );
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}

	return toRet;
}

CBehTreeDecoratorFindSourceItemStoreInstance::CBehTreeDecoratorFindSourceItemStoreInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeDecoratorCarryingItemsBaseInstance( def, owner, context, parent ) 	
{	
}

Bool CBehTreeDecoratorFindSourceItemStoreInstance::IfStorePointValid( CCarryableItemStorePointComponent* storePoint, CBehTreeCarryingItemData* carryingData )
{
	return  !storePoint->IsEmpty() && carryingData->IfCanBeChoosenAsSource( storePoint );
}

void CBehTreeDecoratorFindSourceItemStoreInstance::SetTargetMoveData( CCarryableItemStorePointComponent* storePoint )
{
	m_customMoveData->SetTarget( storePoint->GetWorldPosition() );
	m_customMoveData->SetHeading( storePoint->CalculateHeadingForPickItem() );
}