/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/appearanceComponent.h"

#include "../../common/game/definitionsManager.h"
#include "equipmentState.h"
#include "inventoryEditor.h"

#include "../../common/core/depot.h"

namespace InventoryEditor
{

void GetItemList( TDynArray< CName >& items )
{
	CDefinitionsManager* im = GCommonGame->GetDefinitionsManager();
	if( im )
	{
		im->GetItemList( items );
	}
}

void GetAbilityList( TDynArray< CName >& abilities )
{
	CDefinitionsManager* im = GCommonGame->GetDefinitionsManager();
	if( im )
	{
		im->GetAbilitiesList( abilities );
	}
}

void GetItemCategoriesList( TDynArray< CName >& categories )
{
	CDefinitionsManager* im = GCommonGame->GetDefinitionsManager();
	if( im )
	{
		im->GetItemCategories( categories );
	}
}

void RelodDefinitions()
{
	CDefinitionsManager* im = GCommonGame->GetDefinitionsManager();
	if( im )
	{
		im->ReloadAll();
	}
}

void GetItemsOfCategory( TDynArray< CName >& items, CName category )
{
	CDefinitionsManager* im = GCommonGame->GetDefinitionsManager();
	if( im )
	{
		items = im->GetItemsOfCategory( category );
	}
}

};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void IEquipmentPreview::ApplyEquipmentPreview( CEntity* entity )
{
	if ( !entity )
	{
		return;
	}

	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	if ( !entityTemplate )
	{
		return; 
	}

	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
	if ( !appearanceComponent )
	{
		return;
	}

	const CEntityAppearance* appearance = entityTemplate->GetAppearance( appearanceComponent->GetAppearance(), true );
	if ( appearance )
	{
		CEquipmentDefinition* equipmentDefinition = appearance->FindParameter<CEquipmentDefinition>();
		if ( !equipmentDefinition )
		{
			return;
		}

		CLayer* dynamicLayer = entity->GetLayer();
		ASSERT( dynamicLayer );
		// Apply equipment definition
		const TDynArray< CEquipmentDefinitionEntry* >& eqEntries = equipmentDefinition->GetEntries();
		TDynArray< CEquipmentDefinitionEntry* >::const_iterator entryIt = eqEntries.Begin();
		for ( ; entryIt != eqEntries.End(); ++entryIt )
		{
			// Get name of the item to mount
			CEquipmentDefinitionEntry* entry = *entryIt;
			CName itemToMountName = entry->GetItem();
			if ( itemToMountName == CName::NONE )
			{
				itemToMountName = entry->m_defaultItemName;
			}

			const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemToMountName );
			if ( !itemDef )
			{
				continue;
			}

			const String& templatePath = GCommonGame->GetDefinitionsManager()->TranslateTemplateName( itemDef->GetEquipTemplateName( entity->IsA< CPlayer >() ) );
			CEntityTemplate * entityTemplate = Cast< CEntityTemplate >( GDepot->LoadResource( templatePath ) );
			if ( !entityTemplate )
			{
				continue;
			}

			// Template ready, spawn entity now!
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_template = entityTemplate;
			spawnInfo.m_spawnPosition = Vector::ZERO_3D_POINT;
			if ( itemDef->GetItemAppearanceName( entity->IsA< CPlayer >() ) != CName::NONE )
			{
				spawnInfo.m_appearances.PushBack( itemDef->GetItemAppearanceName( entity->IsA< CPlayer >() ) );
			}

			
			CEntity* spawnedEntity = dynamicLayer->CreateEntitySync( spawnInfo );
			CAppearanceComponent* spawnedEntityAppearanceComponent = CAppearanceComponent::GetAppearanceComponent( spawnedEntity );
			if ( !spawnedEntity )
			{
				continue;
			}

			CItemEntity* itemEntity = Cast< CItemEntity >( spawnedEntity );
			if ( !itemEntity )
			{
				spawnedEntity->Destroy();
				continue;
			}

			Bool result = itemEntity->ApplyAttachmentTo( entity, itemDef->GetEquipSlot( entity->IsA< CPlayer >() ) );
			if ( result )
			{
				m_itemsInPreview.PushBack( itemEntity );
			}
		}
	}
}

void IEquipmentPreview::ClearEquipmentPreviewItems( CEntity* entity )
{
	// Destroy all previously spawned items
	for ( Uint32 i=0; i<m_itemsInPreview.Size(); ++i )
	{
		m_itemsInPreview[i]->Destroy();
	}
	m_itemsInPreview.Clear();

	if ( entity )
	{
		if ( CAppearanceComponent* appearanceComponent = entity->FindComponent<CAppearanceComponent>() )
		{
			// Destroy all previously spawned items
			for ( Uint32 i=0; i<m_templatesInPreview.Size(); ++i )
			{
				appearanceComponent->ExcludeAppearanceTemplate( m_templatesInPreview[i] );
			}
		}
	}

	m_templatesInPreview.Clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CItemDisplayer::DisplayItem( CName itemName, EItemDisplaySlot slot )
{
	CActor* actor = GetActorEntity();
	if ( !actor )
	{
		return;
	}

	CInventoryComponent* inventory = actor->GetInventoryComponent();
	if ( !inventory )
	{
		return;
	}

	if ( !inventory->HasItem( itemName ) )
	{
		inventory->AddItem( itemName );
	}

	SItemUniqueId itemid = inventory->GetItemId( itemName );
	ASSERT( itemid != SItemUniqueId::INVALID );
	if ( itemid == SItemUniqueId::INVALID )
	{
		return;
	}

	CInventoryComponent::SMountItemInfo mountInfo;

	switch( slot )
	{
	case IDS_LeftHand:
		mountInfo.m_toHand = true;
		mountInfo.m_slotOverride = CNAME( l_weapon );
		break;
	case IDS_RightHand:
		mountInfo.m_toHand = true;
		mountInfo.m_slotOverride = CNAME( r_weapon );
		break;
	}

	inventory->MountItem( itemid, mountInfo );

	m_lastItemDisplayed = itemid;
}

void CItemDisplayer::UndoItemDisplay()
{
	if ( m_lastItemDisplayed == SItemUniqueId::INVALID )
	{
		return;
	}

	CActor* actor = GetActorEntity();
	if ( !actor )
	{
		return;
	}

	CInventoryComponent* inventory = actor->GetInventoryComponent();
	if ( !inventory )
	{
		return;
	}

	inventory->UnMountItem( m_lastItemDisplayed, true );	
}
