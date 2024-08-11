#include "build.h"

#include "behTreeCarryingItemData.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeCarryingItemData );


CName CBehTreeCarryingItemData::GetStorageName()
{
	return CNAME( CarryingItemData );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeWorkData::CInitializer
////////////////////////////////////////////////////////////////////////
CName CBehTreeCarryingItemData::CInitializer::GetItemName() const
{
	return CBehTreeCarryingItemData::GetStorageName();
}
void CBehTreeCarryingItemData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
	CBehTreeCarryingItemData* workData = static_cast< CBehTreeCarryingItemData* >( item.Item() );
	//workData->m_owner = m_owner;
}
IRTTIType* CBehTreeCarryingItemData::CInitializer::GetItemType() const
{
	return CBehTreeCarryingItemData::GetStaticClass();
}

Bool CBehTreeCarryingItemData::IfCanBeChoosenAsDestiny( CCarryableItemStorePointComponent* storePoint )
{
	return storePoint 
		&& m_prevStorePoint != storePoint 
		&& m_currentStorePoint != storePoint 
		&& m_carriedItemType == storePoint->GetStoredItemType();
}

Bool CBehTreeCarryingItemData::IfCanBeChoosenAsSource( CCarryableItemStorePointComponent* storePoint )
{
	return storePoint 
		&& m_prevStorePoint != storePoint 
		&& m_currentStorePoint != storePoint;		
}