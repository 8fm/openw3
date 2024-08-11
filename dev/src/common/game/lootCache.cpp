/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "lootCache.h"
#include "lootDefinitions.h"
#include "../core/gameSave.h"

CLootCache::CLootCache()
	: m_inventory( nullptr )
	, m_enabled( true )
	, m_nextLootUpdate( EngineTime::ZERO )
	, m_lootManager( nullptr )
{}

void CLootCache::Enable( Bool enable )
{
	m_enabled = enable;
}

void CLootCache::Update( CInventoryComponent* inventory, Bool force )
{
	if ( !m_enabled )
	{
		return;
	}

	if ( m_inventory == nullptr || force )
	{
		if( !inventory )
		{
			return;
		}

		Init( inventory );
	}

	RED_ASSERT( m_inventory != nullptr, TXT( "Updating CLootCache with no parent CInventoryComponent" ) );

	EngineTime now = GGame->GetEngineTime();
	if ( m_nextLootUpdate.IsValid() && now < m_nextLootUpdate )
	{
		return;
	}
	m_nextLootUpdate = EngineTime::ZERO;

	// Respawn items whose time to respawn passed
	const Uint32 entriesCount = m_entries.Size();
	for ( Uint32 i = 0; i < entriesCount; ++i )
	{
		SLootEntry& entry = m_entries[i];
		// no respawn
		if ( !entry.m_nextRespawnTime.IsValid() )
		{
			continue;
		}
		if ( entry.m_nextRespawnTime < now )
		{
			// we need to check if we can still generate this item
			if ( entry.m_hasAreaLimit && !ValidateMaxCount( entry ) )
			{
				// if we cannot do this anymore, we mark item as "no respawn" and continue
				entry.m_nextRespawnTime = EngineTime::ZERO;
				continue;
			}
			entry.m_nextRespawnTime = now + static_cast< Int32 >( entry.m_respawnTime );
			AddItemToInventory( m_inventory, entry );
		}
		if ( !m_nextLootUpdate.IsValid() || entry.m_nextRespawnTime < m_nextLootUpdate )
		{
			m_nextLootUpdate = entry.m_nextRespawnTime;
		}
	}
}

void CLootCache::AddItemsFromDefinition( CInventoryComponent* inventory, const CName& lootDefinitionName )
{
	if ( inventory == nullptr || lootDefinitionName == CName::NONE )
	{
		return;
	}

	RED_ASSERT( GCommonGame->GetDefinitionsManager() != nullptr, TXT( "Cannot find DefinitionsManager" ) );
	CLootDefinitions* defs = GCommonGame->GetDefinitionsManager()->GetLootDefinitions();
	RED_ASSERT( defs != nullptr, TXT( "DefinitionsManager does not contain loot definitions." ) );

	if( !defs ) 
	{
		return;
	}

	CLootDefinitionBase* lootDefinition = defs->GetDefinition( lootDefinitionName );
	if ( lootDefinition == nullptr )
	{
		return;
	}

	TDynArray< SLootEntry > entries;
	defs->ChooseLootEntries( lootDefinition, entries, inventory );
	const Uint32 entriesCount = m_entries.Size();
	for ( Uint32 i = 0; i < entriesCount; ++i )
	{
		SLootEntry& entry = entries[i];
		// we need to check if we can still generate this item
		if ( entry.m_hasAreaLimit && !ValidateMaxCount( entry ) )
		{
			continue;
		}
		AddItemToInventory( inventory, entry );
	}
}

Bool CLootCache::IsRenewable() const
{
	return m_nextLootUpdate.IsValid();
}

Bool CLootCache::IsReadyToRenew() const
{
	EngineTime now = GGame->GetEngineTime();
	return ( m_nextLootUpdate.IsValid() && ( now >= m_nextLootUpdate ) ); 
}

void CLootCache::Init( CInventoryComponent* inventory )
{
	if( !inventory )
	{
		return;
	}

	m_inventory = inventory;
	m_nextLootUpdate = EngineTime::ZERO;
	m_lootManager = GCommonGame->GetLootManager();
	
	RED_ASSERT( GCommonGame->GetDefinitionsManager() != nullptr, TXT( "Cannot find DefinitionsManager" ) );
	CLootDefinitions* defs = GCommonGame->GetDefinitionsManager()->GetLootDefinitions();
	RED_ASSERT( defs != nullptr, TXT( "DefinitionsManager does not contain loot definitions." ) );

	m_entries.ClearFast();
	if ( !defs || !defs->ChooseLootEntries( inventory->GetEntity(), m_entries, inventory ) )
	{
		return;
	}

	EngineTime now = GGame->GetEngineTime();

	const Uint32 entriesCount = m_entries.Size();
	for ( Uint32 i = 0; i < entriesCount; ++i )
	{
		SLootEntry& entry = m_entries[i];
		if ( entry.m_respawnTime > 0 )
		{
			entry.m_nextRespawnTime = now + static_cast< Int32 >( entry.m_respawnTime );
			if ( !m_nextLootUpdate.IsValid() || entry.m_nextRespawnTime < m_nextLootUpdate )
			{
				m_nextLootUpdate = entry.m_nextRespawnTime;
			}
		}

		entry.m_shouldBeRebalanced = true;
		AddItemToInventory( m_inventory, entry );
	}
}

void CLootCache::AddItemToInventory( CInventoryComponent* inventory, const SLootEntry& entry )
{
	Uint32 newQuantity = entry.GetRandomQuantity();
	Uint32 currentQuantity = inventory->GetItemQuantityByName( entry.m_itemName );
	if ( newQuantity > currentQuantity )
	{
		CInventoryComponent::SAddItemInfo addItemInfo;
		addItemInfo.m_quantity = newQuantity - currentQuantity;
		addItemInfo.m_informGui = true;
		addItemInfo.m_markAsNew = false;
		addItemInfo.m_isLootable = true;
		addItemInfo.m_shouldBeRebalanced = entry.m_shouldBeRebalanced;

		TDynArray< SItemUniqueId > newItemId = inventory->AddItem( entry.m_itemName, addItemInfo );

		if( !newItemId.Empty() )
		{
			SInventoryItem* invItem = inventory->GetItem( newItemId[0] );
			if ( invItem != nullptr && invItem->GetDurability() > 0 )
			{ 
				SInventoryItem::SItemModifierVal* itemMod = invItem->GetItemMod( CNAME( ItemDurabilityModified ), true );
				if ( itemMod && itemMod->m_int < 1 )
				{
					inventory->ReduceLootableItemDurability( *invItem );
				}
			}

			UpdateItemMaxCount( entry.m_itemName, addItemInfo.m_quantity );
		}
	}
}

Bool CLootCache::ValidateMaxCount( SLootEntry& entry )
{
	if ( m_lootManager == nullptr )
	{
		return true;
	}
	Int32 maxCount = m_lootManager->GetItemMaxCount( entry.m_itemName );
	if ( maxCount < 0 )
	{
		entry.m_hasAreaLimit = false;
		return true;
	}
	if ( static_cast< Uint32 >( maxCount ) < entry.m_quantityMin )
	{
		// we cannot generate it anymore, but for a bigger safety lets update its quantities
		entry.m_quantityMin = entry.m_quantityMax = static_cast< Uint32 >( maxCount );
		return false;
	}
	if ( static_cast< Uint32 >( maxCount ) < entry.m_quantityMax )
	{
		entry.m_quantityMax = static_cast< Uint32 >( maxCount );
	}
	return true;
}

void CLootCache::UpdateItemMaxCount( const CName& itemName, Uint32 generatedQuantity )
{
	if ( m_lootManager == nullptr )
	{
		return;
	}
	m_lootManager->UpdateItemMaxCount( itemName, generatedQuantity );
}

void CLootCache::Save( IGameSaver* saver )
{
	saver->WriteValue< Bool >( CNAME( Enable ), m_enabled );
	const Uint32 lootCount = m_entries.Size();
	saver->WriteValue< Float >( CNAME( Time ), static_cast< Float>( m_nextLootUpdate ) );
	saver->WriteValue< Uint32 >( CNAME( count ), lootCount );
	for ( Uint32 i = 0; i < lootCount; ++i )
	{
		CGameSaverBlock block( saver, CNAME( l ) );
		saver->WriteValue< CName >( CNAME( i ), m_entries[i].m_itemName );
		saver->WriteValue< Uint32 >( CNAME( Min ), m_entries[i].m_quantityMin );
		saver->WriteValue< Uint32 >( CNAME( Max ), m_entries[i].m_quantityMax );
		saver->WriteValue< Uint32 >( CNAME( t ), m_entries[i].m_respawnTime );
		saver->WriteValue< Float >( CNAME( n ), static_cast< Float >( m_entries[i].m_nextRespawnTime ) );
	}
}

void CLootCache::Load( IGameLoader* loader )
{
	m_enabled = loader->ReadValue< Bool >( CNAME( Enable ), true );
	m_nextLootUpdate = EngineTime( loader->ReadValue< Float >( CNAME( Time ) ) );
	const Uint32 lootCount = loader->ReadValue<Uint32>( CNAME( count ) );
	m_entries.Resize( lootCount );
	for ( Uint32 i = 0; i < lootCount; ++i )
	{
		CGameSaverBlock block( loader, CNAME( l ) );
		m_entries[i].m_itemName = loader->ReadValue< CName >( CNAME( i ), CName::NONE );
		m_entries[i].m_quantityMin = loader->ReadValue< Uint32 >( CNAME( Min ) );
		m_entries[i].m_quantityMax = loader->ReadValue< Uint32 >( CNAME( Max ) );
		m_entries[i].m_respawnTime = loader->ReadValue< Uint32 >( CNAME( t ) );
		m_entries[i].m_nextRespawnTime = EngineTime( loader->ReadValue< Float >( CNAME( n ) ) );
	}
}

void CLootCache::StreamLoad( ISaveFile* loader, Uint32 version )
{
	*loader << m_enabled;

	Float nextLootUpdate = 0.f;
	*loader << nextLootUpdate;
	m_nextLootUpdate = nextLootUpdate;

	Uint8 lootCount = 0;
	*loader << lootCount;
	m_entries.Resize( lootCount );

	for ( Uint8 i = 0; i < lootCount; ++i )
	{
		*loader << m_entries[ i ].m_itemName;

		Uint8 qty = 0;
		*loader << qty;
		m_entries[ i ].m_quantityMin = qty;

		qty = 0;
		*loader << qty;
		m_entries[ i ].m_quantityMax = qty;

		*loader << m_entries[ i ].m_respawnTime;

		Float nextRespawnTime = 0.f;
		*loader << nextRespawnTime;
		m_entries[ i ].m_nextRespawnTime = nextRespawnTime;
	}
}

void CLootCache::StreamSave( ISaveFile* saver )
{
	// ALWAYS bump save version before changing ANYTHING in the stream format

	*saver << m_enabled;

	Float nextLootUpdate = m_nextLootUpdate;
	*saver << nextLootUpdate;

	ASSERT( m_entries.Size() < 256 );
	Uint8 lootCount = static_cast< Uint8 > ( m_entries.Size() );
	*saver << lootCount;

	for ( Uint8 i = 0; i < lootCount; ++i )
	{
		*saver << m_entries[ i ].m_itemName;

		ASSERT( m_entries[ i ].m_quantityMin < 256 );
		Uint8 qty = static_cast< Uint8 > ( m_entries[ i ].m_quantityMin );
		*saver << qty;

		ASSERT( m_entries[ i ].m_quantityMax < 256 );
		qty = static_cast< Uint8 > ( m_entries[ i ].m_quantityMax );
		*saver << qty;

		*saver << m_entries[ i ].m_respawnTime;

		Float nextRespawnTime = m_entries[ i ].m_nextRespawnTime;
		*saver << nextRespawnTime;
	}
}
