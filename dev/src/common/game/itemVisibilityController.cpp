#include "build.h"
#include "itemVisibilityController.h"

ItemVisibilityController::ItemVisibilityController()
	: m_hiddenFlag( false )
{
	m_items.Resize( 4 );
	m_items[ 0 ].m_category = CNAME( steelsword );
	m_items[ 1 ].m_category = CNAME( steel_scabbard );
	m_items[ 2 ].m_category = CNAME( silversword );
	m_items[ 3 ].m_category = CNAME( silver_scabbard );
}

ItemVisibilityController::ItemWrapper::ItemWrapper()
	: m_id( SItemUniqueId::INVALID )
{

}

void ItemVisibilityController::ItemWrapper::SetHiddenInGame( CInventoryComponent* inv, Bool flag )
{
	// 1. Is valid
	if ( m_id == SItemUniqueId::INVALID )
	{
		return;
	}

	// 2. Get entity
	CItemEntity* entity = m_entity.Get();
	if ( !entity )
	{
		entity = inv->GetItemEntityUnsafe( m_id );
	}

	// 3. Set hide flag
	if ( entity && entity->IsHiddenInGame() != flag )
	{
		// OMG !!!
		for ( CComponent* component : entity->GetComponents() )
		{
			if ( flag )
			{
				component->SetFlag( NF_HideInGame );
			}
			else
			{
				component->ClearFlag( NF_HideInGame );
			}
		}

		entity->SetHideInGame( flag );

		/*Bool CNode::IsHiddenInGame() const
		{
			// Node is hidden in game
			if ( m_objectFlags & NF_HideInGame )
			{
				return true;
			}

			// Check transform parent
			if ( m_transformParent && m_transformParent->GetParent() )	<========= This is the problem 1. !!! for example CMovingAgentComponent
			{																													||
				return m_transformParent->GetParent()->IsHiddenInGame();														||
			}																													||
																																||
			// Component case																									||
			const CComponent* component = AsComponent();																		||
			if ( component )																									||
			{																													\/
				CEntity* entity = component->GetEntity();				<========= This is the problem 2. !!!	This is not CItemEntity but CActor
				if ( entity )
				{
					return entity->IsHiddenInGame();
				}
			}

			// Not hidden
			return false;
		}*/
	}
}

void ItemVisibilityController::SetItemsHiddenInGame( CInventoryComponent* inv, Bool flag )
{
	m_hiddenFlag = flag;

	if ( inv )
	{
		for ( Uint32 i=0; i<m_items.Size(); ++i )
		{
			m_items[ i ].SetHiddenInGame( inv, m_hiddenFlag );
		}
	}
}

void ItemVisibilityController::SetItemHiddenInGame( CInventoryComponent* inv, ItemVisibilityController::ItemWrapper* item, bool flag )
{
	// Find item by id
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		if ( &(m_items[ i ]) == item )
		{
			m_items[ i ].SetHiddenInGame( inv, flag );
			return;
		}
	}
}

ItemVisibilityController::ItemWrapper* ItemVisibilityController::FindItemWrapperByCategory( const CName& category )
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		if ( m_items[ i ].m_category == category )
		{
			return &( m_items[ i ] );
		}
	}

	return nullptr;
}

void ItemVisibilityController::OnMountItem( CInventoryComponent* inv, SItemUniqueId itemId )
{
	if ( inv && itemId != SItemUniqueId::INVALID )
	{
		const SInventoryItem* item = inv->GetItem( itemId );
		if ( item )
		{
			ItemWrapper* internalItem = FindItemWrapperByCategory( item->GetCategory() );
			if ( internalItem )
			{
				if ( internalItem->m_id != SItemUniqueId::INVALID )
				{
					SetItemHiddenInGame( inv, internalItem, false );
				}

				// Add to list
				internalItem->m_id = itemId;
				internalItem->m_entity = nullptr;

				// Set item visibility
				SetItemHiddenInGame( inv, internalItem, m_hiddenFlag );
			}
		}
	}
}

void ItemVisibilityController::OnUnmountItem( CInventoryComponent* inv, SItemUniqueId itemId )
{
	if ( inv && itemId != SItemUniqueId::INVALID )
	{
		const SInventoryItem* item = inv->GetItem( itemId );
		if ( item )
		{
			ItemWrapper* internalItem = FindItemWrapperByCategory( item->GetCategory() );
			if ( internalItem )
			{
				if ( internalItem->m_id != SItemUniqueId::INVALID )
				{
					// Always force item visibility
					SetItemHiddenInGame( inv, internalItem, false );

					// Remove from list
					internalItem->m_id = SItemUniqueId::INVALID;
				}
				internalItem->m_entity = nullptr;
			}
		}
	}
}
