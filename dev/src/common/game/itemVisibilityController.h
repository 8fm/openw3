
#pragma once

class ItemVisibilityController
{
	struct ItemWrapper
	{
		CName					m_category;
		SItemUniqueId			m_id;
		THandle< CItemEntity >	m_entity;

		ItemWrapper();

		void SetHiddenInGame( CInventoryComponent* inv, Bool flag );
	};

	Bool						m_hiddenFlag;
	TDynArray< ItemWrapper >	m_items;

public:
	ItemVisibilityController();

	void SetItemsHiddenInGame( CInventoryComponent* inv, Bool flag );

	void OnMountItem( CInventoryComponent* inv, SItemUniqueId itemId );
	void OnUnmountItem( CInventoryComponent* inv, SItemUniqueId itemId );

private:
	ItemWrapper* FindItemWrapperByCategory( const CName& category );
	void SetItemHiddenInGame( CInventoryComponent* inv, ItemWrapper* item, bool flag );
};
