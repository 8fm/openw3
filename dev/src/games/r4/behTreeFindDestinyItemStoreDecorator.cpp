#include "build.h"

#include "behTreeFindDestinyItemStoreDecorator.h"

IBehTreeNodeDecoratorInstance* CBehTreeDecoratorFindDestinationItemStoreDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}


Bool CBehTreeDecoratorFindDestinationItemStoreInstance::IsAvailable()
{
	CAreaComponent* carryingArea = m_carryingArea.Get();
	CBehTreeCarryingItemData* carryData = m_carryingItemsData.Get();

	Bool toRet = ( m_storeTag != CName::NONE && carryingArea && carryData && carryData->GetCarriedItem() && Super::IsAvailable() );
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}

	return toRet;
}

Bool CBehTreeDecoratorFindDestinationItemStoreInstance::IfStorePointValid( CCarryableItemStorePointComponent* storePoint, CBehTreeCarryingItemData* carryingData )
{
	return storePoint->IfHasFreeSpace() && carryingData->IfCanBeChoosenAsDestiny( storePoint );
}

void CBehTreeDecoratorFindDestinationItemStoreInstance::SetTargetMoveData( CCarryableItemStorePointComponent* storePoint )
{
	m_customMoveData->SetTarget( storePoint->GetWorldPosition() );
	m_customMoveData->SetHeading( storePoint->CalculateHeadingForDropItem() );
}