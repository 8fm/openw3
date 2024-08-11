/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "inventoryItem.h"
#include "abilities.h"
#include "../../common/engine/appearanceComponent.h"
#include "../core/gameSave.h"
#include "../engine/gameSaveManager.h"
#include "definitionsManager.h"

IMPLEMENT_ENGINE_CLASS( SInventoryItem );
IMPLEMENT_ENGINE_CLASS( SInventoryItemUIData );

const SInventoryItemUIData SInventoryItemUIData::INVALID;
SInventoryItem SInventoryItem::INVALID;

SInventoryItem::SInventoryItem()
	: m_itemEntityProxy( nullptr )
	, m_itemQuantity( 1 )
	, m_name( CName::NONE )
	, m_category( CName::NONE )
	, m_uniqueId( SItemUniqueId::INVALID )
	, m_itemDurability( -1 )									  
	, m_staticRandomSeed( 0 )
	, m_flags( 0 )
	, m_maxSlotItems( 0 )
	, m_dyeColorName( CNAME( item_dye_default ) )
	, m_dyeColorAbilityName( CNAME( dye_default ) )
{}

SInventoryItem::SInventoryItem( const CName& name, const SItemDefinition& itemDef, const SInventoryItemInitData& initData )
	: m_itemDurability( -1 )
	, m_staticRandomSeed( 0 )
	, m_flags( 0 )
	, m_dyeColorName( CNAME( item_dye_default ) )
	, m_dyeColorAbilityName( CNAME( dye_default ) )
	// the following fields are initialized inside Init() method, so no need to do it twice
	// m_itemEntityProxy( nullptr )
	// m_itemQuantity( 1 )
	// m_name( CName::NONE )
	// m_category( CName::NONE )
	// m_uniqueId( SItemUniqueId::INVALID )
	// m_maxSlotItems( 0 )
{
	Init( name, itemDef, initData );
}

void SInventoryItem::Init(const CName& name, const SItemDefinition& itemDef, const SInventoryItemInitData& initData /* = SInventoryItemInitData */)
{
	m_name = name;
	m_itemEntityProxy = initData.m_itemEntityProxy;
	m_itemQuantity = initData.m_quantity;

	m_slotItems.Clear();
	m_craftedAbilities.Clear();

	AssignStaticRandomSeed();
	ReadUniformDefinitionData( itemDef, initData.m_playerItem );

	SetFlag( FLAG_LOOTABLE, initData.m_isLootable );
	SetFlag( FLAG_CLONED, initData.m_isCloned );
	m_uiData.m_isNew = initData.m_markAsNew;
	m_uniqueId = initData.m_uniqueId;

	SetFlag( FLAG_REBALANCE, initData.m_shouldBeRebalanced);
}

void SInventoryItem::CloneOf( const SInventoryItem& item, const SItemUniqueId& newUniqueId )
{
	m_uniqueId = newUniqueId;
	m_itemQuantity = item.m_itemQuantity;
	m_itemEntityProxy = nullptr;
	m_category = item.m_category;
	m_name = item.m_name;
	m_maxSlotItems = item.m_maxSlotItems;
	m_slotItems = item.m_slotItems;
	m_baseAbilities = item.m_baseAbilities;
	m_craftedAbilities = item.m_craftedAbilities;
	m_staticRandomSeed = item.m_staticRandomSeed;
	m_itemDurability = item.m_itemDurability;
	m_modifiers = item.m_modifiers;
	m_enchantmentName = item.m_enchantmentName;
	m_enchantmentStats = item.m_enchantmentStats;
	m_dyeColorName = item.m_dyeColorName;
	m_dyeColorAbilityName = item.m_dyeColorAbilityName;
	m_dyePreviewColorName = item.m_dyePreviewColorName;
	// copy only flags defining an item, not the state flags
	m_flags = item.m_flags & COPY_MASK;
	m_itemTags = item.m_itemTags;
	m_uiData = item.m_uiData;
}

void SInventoryItem::ReadUniformDefinitionData( const SItemDefinition& itemDef, Bool isPlayer )
{
	// copy item slot abilities
	m_itemTags = itemDef.GetItemTags();
	m_category = itemDef.GetCategory();
	m_maxSlotItems = itemDef.GetEnhancementSlotCount();

	if ( false == HasFlag( FLAG_STORE_BASE_ABILITIES ) )
	{
		// (randomly) choose item abilities based on their chances/weights
		itemDef.ChooseAbilities( m_baseAbilities, isPlayer, m_staticRandomSeed );
	}

	ASSERT( ( itemDef.GetInitialDurability( isPlayer ) == -1 && itemDef.GetMaxDurability( isPlayer ) == -1 ) ||
		( itemDef.GetInitialDurability( isPlayer ) != -1 && itemDef.GetMaxDurability( isPlayer ) != -1 ) );
	ASSERT( itemDef.GetInitialDurability( isPlayer ) <= itemDef.GetMaxDurability( isPlayer ) );

	// apply initial durability only if item has durability and it wasn't restored
	if ( m_itemDurability == -1 && itemDef.GetInitialDurability( isPlayer ) != -1 )
	{
		m_itemDurability = itemDef.GetInitialDurability( isPlayer );
	}

	if ( itemDef.GetHoldSlot( isPlayer ) == CName::NONE )
	{
		SetFlag( SInventoryItem::FLAG_NO_HAND, true );
	}

	if ( itemDef.GetEquipTemplateName( isPlayer ).Empty() )
	{
		SetFlag( SInventoryItem::FLAG_NO_ENTITY, true );
	}

	if ( itemDef.IsStackable() )
	{
		SetFlag( SInventoryItem::FLAG_STACKABLE, true );
	}
	else
	{
		m_itemQuantity = itemDef.GetStackSize();
	}

	m_uiData.m_gridSize = itemDef.GetGridSize();
}

void SInventoryItem::AssignStaticRandomSeed()
{
	if ( m_staticRandomSeed == 0 )
	{
		m_staticRandomSeed = GEngine->GetRandomNumberGenerator().Get< Uint16 >( 1, GEngine->GetRandomNumberGenerator().Max< Uint16 >() );
	}
}

void SInventoryItem::CreateProxy( const SItemDefinition& itemDef, CLayer* layer, const String& templateName, Bool collapse )
{
	TDynArray< CName > slotItems;
	GetSlotItemsNames( slotItems );
	m_itemEntityProxy = SItemEntityManager::GetInstance().CreateNewProxy( m_name, itemDef, slotItems, layer, templateName, collapse, IsInvisible(), m_uniqueId );
}

void SInventoryItem::SetProxy( CItemEntityProxy* proxy, Bool cloned )
{
	if ( m_itemEntityProxy != proxy )
	{
		DestroyProxy();
		m_itemEntityProxy = proxy;
	}
	SetFlag( FLAG_CLONED, cloned );
}

void SInventoryItem::MoveProxy( SInventoryItem* item )
{
	m_itemEntityProxy = item->m_itemEntityProxy;
	item->m_itemEntityProxy = nullptr;
}

void SInventoryItem::ReleaseProxy()
{
	m_itemEntityProxy = nullptr;
}

void SInventoryItem::DestroyProxy()
{
	if ( m_itemEntityProxy != nullptr )
	{
		SItemEntityManager::GetInstance().DestroyProxy( m_itemEntityProxy );
		m_itemEntityProxy = nullptr;
	}
}

void SInventoryItem::RevalidateProxy()
{
	if ( !SItemEntityManager::GetInstance().IsProxyRegistred( m_itemEntityProxy ) )
	{
		m_itemEntityProxy = nullptr;
	}
}

CItemEntity* SInventoryItem::GetItemEntity() const
{
	if ( m_itemEntityProxy == nullptr )
	{
		return nullptr;
	}
	return SItemEntityManager::GetInstance().GetItemEntityIfSpawned( m_itemEntityProxy );
}

void SInventoryItem::SetIsInvisible( Bool isInvisible )
{
	if( m_itemEntityProxy )
	{
		m_itemEntityProxy->m_invisible = isInvisible;
	}	
	SetFlag( FLAG_INVISIBLE, isInvisible );
}

SAbilityAttributeValue SInventoryItem::GetAttribute( CName attrName ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT(defMgr);

	TDynArray< CName > allAbilities;
	GetAllAbilities( allAbilities );

	Int32 alwaysRandomSeed = GEngine->GetRandomNumberGenerator().Get< Int32 >();

	return defMgr->CalculateAttributeValue( allAbilities, attrName, alwaysRandomSeed, GetStaticRandomSeed() );
}

void SInventoryItem::Collapse( Bool collapse )
{
	 CItemEntity* entity = GetItemEntity();
	if ( entity )
	{
		entity->Collapse( collapse );
	}
	if ( m_itemEntityProxy )
	{
		m_itemEntityProxy->m_collapse = collapse;
	}
}

static void CollectAttributes( const TDynArray< CName > abilities, TDynArray< CName > & outAttributes )
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT(defMgr);

	for ( Uint32 i = 0; i < abilities.Size(); ++i )	
	{
		CName abilityName = abilities[ i ];
		const SAbility* abilityDef = defMgr->GetAbilityDefinition( abilityName );

		if ( !abilityDef )
		{
			ERR_GAME( TXT("CollectAttributes - ability '%ls' definition not found"), abilityName.AsString().AsChar() );
			continue;
		}

		for ( Uint32 j = 0; j < abilityDef->m_attributes.Size(); ++j )
		{
			outAttributes.PushBackUnique( abilityDef->m_attributes[ j ].m_name );
		}
	}
}

void SInventoryItem::GetAllAttributes( TDynArray< CName > & outAttributes ) const
{
	outAttributes.ClearFast();

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT(defMgr);

	const SItemDefinition * itemDef = defMgr->GetItemDefinition( m_name );
	if( !itemDef )
	{
		ITEM_WARN( TXT("Item definition for item '%ls' not found"), m_name.AsString().AsChar() );
		return;
	}

	TDynArray< CName > allAbilities;
	GetAllAbilities( allAbilities );

	// Get attributes from base abilities
	CollectAttributes( allAbilities, outAttributes );
}

void SInventoryItem::GetBaseAttributes( TDynArray< CName > & outAttributes ) const
{
	TDynArray< CName > baseAbilities;
	baseAbilities.PushBack( m_baseAbilities );
	baseAbilities.PushBack( m_craftedAbilities );

	// Get attributes from base abilities
	CollectAttributes( baseAbilities, outAttributes );
}

void SInventoryItem::GetBaseAbilities( TDynArray< CName >& baseAbilities ) const
{
	baseAbilities.PushBack( m_baseAbilities );
}

void SInventoryItem::GetAllAbilities( TDynArray< CName >& allAbilities, Int32 excludedSlot ) const
{
	// Collect base abilities chosen from the definition
	allAbilities.PushBack( m_baseAbilities );

	// Collect dynamic abilities
	allAbilities.PushBack( m_craftedAbilities );

	// Collect slot abilities
	GetSlotAbilities( allAbilities, excludedSlot );
}

static void CollectContainedAbilities( const TDynArray< CName > abilities, TDynArray< CName >& outAbilities )
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT(defMgr);

	for ( Uint32 i = 0; i < abilities.Size(); ++i )	
	{
		CName abilityName = abilities[ i ];
		const SAbility* abilityDef = defMgr->GetAbilityDefinition( abilityName );

		if ( !abilityDef )
		{
			ERR_GAME( TXT("CollectAttributes - ability '%ls' definition not found"), abilityName.AsString().AsChar() );
			continue;
		}

		for ( Uint32 j = 0; j < abilityDef->m_abilities.Size(); ++j )
		{
			outAbilities.PushBackUnique( abilityDef->m_abilities[ j ] );
		}
	}
}

void SInventoryItem::GetContainedAbilities( TDynArray< CName >& containedAbilities ) const
{
	containedAbilities.ClearFast();

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT(defMgr);

	const SItemDefinition * itemDef = defMgr->GetItemDefinition( m_name );
	if( !itemDef )
	{
		ITEM_WARN( TXT("Item definition for item '%ls' not found"), m_name.AsString().AsChar() );
		return;
	}

	TDynArray< CName > allAbilities;
	GetAllAbilities( allAbilities );

	// Get attributes from base abilities
	CollectContainedAbilities( allAbilities, containedAbilities );
}

void SInventoryItem::GetSlotAbilities( TDynArray< CName >& abilities, Int32 excludedSlot ) const
{
	// iterate slot attached items
	for ( Int32 i = 0; i < ( Int32 ) m_slotItems.Size(); i++ )
	{
		if ( i != excludedSlot )
		{
			abilities.PushBack( m_slotItems[i].m_abilities );
		}
	}
}

void SInventoryItem::GetSlotAbilities( Int32 slot, TDynArray< CName >& abilities ) const
{
	const SInventoryItem::SSlotItem& slotItem = m_slotItems[ slot ];
	abilities.PushBack( slotItem.m_abilities );
}

String SInventoryItem::GetInfo() const
{	
	String tags;

	for( Uint32 t=0; t< m_itemTags.Size(); t++ )
	{
		tags += m_itemTags[t].AsString();
		tags.Append( TXT(", "), 3 );
	}

	return String::Printf( TXT("(uniqueId %u) Name: %s, Weapon: %s, Lootable %s, Mount: %s, Held: %s, Tags: %s"),		
		m_uniqueId,
		m_name.AsString().AsChar(),				
		ToString(IsWeapon()).AsChar(),
		ToString(IsLootable()).AsChar(),
		ToString(IsMounted()).AsChar(),
		ToString(IsHeld()).AsChar(),
		tags.AsChar()
		);	
}

Int32 SInventoryItem::GetSlotItemIndex( const CName& name, Uint32 randSeed /* = 0 */ ) const
{
	TDynArray< SSlotItem >::const_iterator it = m_slotItems.Begin();
	TDynArray< SSlotItem >::const_iterator itEnd = m_slotItems.End();
	Int32 index = 0;
	for ( ; it != itEnd; ++it )
	{
		if ( it->m_name == name && ( randSeed == 0 || randSeed == it->m_randSeed ) )
		{
			return index;
		}
		index++;
	}
	return -1;
}

Bool SInventoryItem::GetSlotItemsNames( TDynArray< CName > & names ) const
{
	TDynArray< SSlotItem >::const_iterator it = m_slotItems.Begin();
	TDynArray< SSlotItem >::const_iterator itEnd = m_slotItems.End();
	for ( ; it != itEnd; ++it )
	{
		names.PushBack( it->m_name );
	}
	return names.Size() > 0;
}

Uint8 SInventoryItem::GetSlotItemsLimit() const
{
	CName categoryName = GetCategory();
	String tags;

	for ( Uint32 tag = 0; tag < m_itemTags.Size(); ++tag )
	{
		if ( m_itemTags[ tag ] == CNAME( mod_secondary ) )
		{
			return 0;
		}
	}

	if ( CNAME( steelsword ) == categoryName || CNAME( silversword ) == categoryName || CNAME( armor ) == categoryName )
	{
		return 3;
	}
	else if ( CNAME( pants ) == categoryName )
	{
		return 2;
	}
	else if ( CNAME( boots ) == categoryName )
	{
		return 1;
	}
	else if ( CNAME( gloves ) == categoryName )
	{
		return 1;
	}
	return 0;
}

Bool SInventoryItem::AddSlot()
{
	if ( m_maxSlotItems >= GetSlotItemsLimit() ) // m_slotItems.Size() >= GetMaxSlotItems() )
	{
		return false;
	}

	m_maxSlotItems++;

	return true;
}

Uint8 SInventoryItem::GetSlotItemsMax() const
{
	return m_maxSlotItems;
}

Bool SInventoryItem::AddSlotItem( const CName& name, Bool playerItem, Uint32 randSeed )
{
	if ( m_slotItems.Size() >= m_maxSlotItems )
	{
		return false;
	}

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT ( defMgr != nullptr );
	const SItemDefinition* itemDef = defMgr->GetItemDefinition( name );
	if ( itemDef == nullptr )
	{
		return false;
	}
	m_slotItems.PushBack( SSlotItem( name, randSeed ) );
	SSlotItem& newItem = m_slotItems.Last();
	itemDef->ChooseAbilities( newItem.m_abilities, playerItem, newItem.m_randSeed );
	return true;
}

Bool SInventoryItem::RemoveSlotItem( const CName& name, Uint32 randSeed /* = 0 */ )
{
	Bool removed = false;
	Int32 index = GetSlotItemIndex( name, randSeed );
	while ( index != -1 )
	{
		removed |= RemoveSlotItem( static_cast< Uint32 >( index ) );
		index = GetSlotItemIndex( name, randSeed );
	}
	return removed;
}

Bool SInventoryItem::RemoveSlotItem( Uint32 slotIndex )
{
	if ( slotIndex < m_slotItems.Size() )
	{
		m_slotItems.RemoveAt( slotIndex );
		return true;
	}
	return false;
}

void SInventoryItem::RemoveAllSlots()
{
	m_slotItems.ClearFast();
}

Bool SInventoryItem::AddCraftedAbility( const CName& abilityName, Bool allowDuplicate )
{
	if ( !allowDuplicate && m_craftedAbilities.Exist( abilityName ) )
	{
		return false;
	}

	m_craftedAbilities.PushBack( abilityName );
	return true;
}

Bool SInventoryItem::RemoveCraftedAbility( const CName& abilityName )
{
	return m_craftedAbilities.Remove( abilityName );
}

Bool SInventoryItem::AddBaseAbility( const CName& abilityName )
{
	SetFlag( FLAG_STORE_BASE_ABILITIES, true );
	m_baseAbilities.PushBack( abilityName );
	return true;
}

Bool SInventoryItem::RemoveBaseAbility( const CName& abilityName )
{
	SetFlag( FLAG_STORE_BASE_ABILITIES, true );
	return m_baseAbilities.Remove( abilityName );
}

SAbilityAttributeValue SInventoryItem::PreviewAttributeAfterUpgrade( const SInventoryItem* upgradeItem, const CName& attributeName ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT( defMgr != nullptr );

	TDynArray< CName > allAbilities;
	GetAllAbilities( allAbilities );

	if ( upgradeItem != nullptr )
	{
		upgradeItem->GetAllAbilities( allAbilities );
	}

	Int32 alwaysRandomSeed = GEngine->GetRandomNumberGenerator().Get< Int32 >();

	return defMgr->CalculateAttributeValue( allAbilities, attributeName, alwaysRandomSeed, GetStaticRandomSeed() );
}

Bool SInventoryItem::IsWeapon( const CName& itemName )
{
	const SItemDefinition * itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName );
	if ( itemDef != nullptr )
	{
		return itemDef->IsWeapon();
	}
	return false;
}

Bool SInventoryItem::AreAbilitiesActive() const
{
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( GetName() );
	if ( itemDef != nullptr )
	{
		return ( ( itemDef->IsAbilityEnabledOnMount() && IsMounted() ) || ( itemDef->IsAbilityEnabledOnHold() && IsHeld() ) );
	}
	return false;
}

RED_DEFINE_STATIC_NAME( modCount )
RED_DEFINE_STATIC_NAME( modType )
RED_DEFINE_STATIC_NAME( modVal )
RED_DEFINE_STATIC_NAME( modName )

Bool SInventoryItem::StreamLoad( ISaveFile* loader, Uint32 version, Bool isPlayerOwner )
{
	*loader << m_name;
	*loader << m_staticRandomSeed;
	if ( version < SAVE_VERSION_ITEMS_FLAGS_64 )
	{
		Uint16 oldFlags = 0;
		*loader << oldFlags;
		m_flags = oldFlags;
	}
	else
	{
		*loader << m_flags;
	}
	
	Uint8 maxSlotItems = m_maxSlotItems;
	if ( version >= SAVE_VERSION_STORE_NUM_INVENTORY_ITEM_SLOTS )
	{
		*loader << maxSlotItems;
	}

	if ( version >= SAVE_VERSION_STORE_ENCHANTMENTS )
	{
		if ( HasFlag( FLAG_STORE_ENCHANTMENT ) )
		{
			*loader << m_enchantmentName;
			*loader << m_enchantmentStats;
		}
	}

	if ( version >= SAVE_VERSION_ITEMS_DYE_COLOR )
	{
		if ( HasFlag( FLAG_STORE_DYE_COLOR ) )
		{
			*loader << m_dyeColorName;
			*loader << m_dyeColorAbilityName;
		}
	}

	Bool largeQuantity = false;
	if ( version >= SAVE_VERSION_32_BIT_QUANTITY_ITEMS )
	{
		largeQuantity = HasFlag( FLAG_STORE_LARGE_QUANTITY );
	}

	if ( largeQuantity )
	{
		Uint32 itemQuantity = 0;
		*loader << itemQuantity;
		m_itemQuantity = itemQuantity;
	}
	else
	{
		Uint16 itemQuantity = 0;
		*loader << itemQuantity;
		m_itemQuantity = itemQuantity;
	}

	*loader << m_itemDurability;

	Uint8 modCount = 0;
	*loader << modCount;

	m_modifiers.Resize( modCount );
	for ( Uint8 i = 0; i < modCount; ++i )
	{
		*loader << m_modifiers[ i ].m_first;
		*loader << m_modifiers[ i ].m_second.m_int;

		Uint8 type = 0;
		*loader << type;
		m_modifiers[ i ].m_second.m_type = SItemModifierVal::EType( type );
	}

	m_uniqueId.StreamLoad( loader, version );

	Uint8 count = 0;
	*loader << count;

	for ( Uint8 i = 0; i < count; ++i )
	{
		CName name;
		*loader << name;

		Uint32 randSeed;
		*loader << randSeed;

		m_slotItems.PushBack( SSlotItem( name, randSeed ) );
		SSlotItem& newItem = m_slotItems.Last();

		Uint8 abilitiesCount = 0;
		*loader << abilitiesCount;

		newItem.m_abilities.Reserve( abilitiesCount );
		for ( Uint8 j = 0; j < abilitiesCount; ++j )
		{
			CName abilityName;
			*loader << abilityName;
			if ( abilityName )
			{
				newItem.m_abilities.PushBack( abilityName );
			}
		}
	}

	{
		Uint8 craftedAbilitiesCount = 0;
		*loader << craftedAbilitiesCount;
		
		m_craftedAbilities.Resize( craftedAbilitiesCount );
		for ( Uint8 j = 0; j < craftedAbilitiesCount; ++j )
		{
			*loader << m_craftedAbilities[ j ];
		}
	}

	if( version >= SAVE_VERSION_STORE_CATEGORY_ITEM_INFO )
	{
		if( this->IsMounted() )
			*loader << m_category;
	}

	// Restore item definition
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( m_name );
	if ( !itemDef )
	{
		ITEM_WARN( TXT("Trying to restore item %s, missing a definition"), m_name.AsString().AsChar() );
		return false;
	}
	
	ReadUniformDefinitionData( *itemDef, isPlayerOwner );

	if ( version >= SAVE_VERSION_STORE_NUM_INVENTORY_ITEM_SLOTS )
	{
		m_maxSlotItems = maxSlotItems;
	}

	if ( HasFlag( FLAG_STORE_BASE_ABILITIES ) )
	{
		*loader << m_baseAbilities;
	}

	return true;
}

void SInventoryItem::StreamSave( ISaveFile* saver )
{
	// ALWAYS bump save version before changing ANYTHING in the stream format

	SetFlag( FLAG_STORE_LARGE_QUANTITY, m_itemQuantity > 65535 );
	SetFlag( FLAG_STORE_ENCHANTMENT, m_enchantmentName != CName::NONE );
	SetFlag( FLAG_STORE_DYE_COLOR, m_dyeColorName != CName::NONE );

	*saver << m_name;
	*saver << m_staticRandomSeed;
	*saver << m_flags;
	*saver << m_maxSlotItems;

	if ( HasFlag( FLAG_STORE_ENCHANTMENT ) )
	{
		*saver << m_enchantmentName;
		*saver << m_enchantmentStats;
	}

	if ( HasFlag( FLAG_STORE_DYE_COLOR ) )
	{
		*saver << m_dyeColorName;
		*saver << m_dyeColorAbilityName;
	}

	if ( HasFlag( FLAG_STORE_LARGE_QUANTITY ) )
	{
		*saver << m_itemQuantity;
	}
	else
	{
		Uint16 itemQuantity = static_cast< Uint16 > ( m_itemQuantity );
		ASSERT( m_itemQuantity < 0x10000 );
		*saver << itemQuantity;
	}
	*saver << m_itemDurability;

	ASSERT( m_modifiers.Size() < 256 );
	Uint8 modCount = static_cast< Uint8 > ( m_modifiers.Size() );
	*saver << modCount;

	for ( Uint8 i = 0; i < modCount; ++i )
	{
		*saver << m_modifiers[ i ].m_first;
		*saver << m_modifiers[ i ].m_second.m_int;
		Uint8 type = m_modifiers[ i ].m_second.m_type;
		*saver << type;
	}

	m_uniqueId.StreamSave( saver );

	ASSERT( m_slotItems.Size() < 256 );
	Uint8 count = static_cast< Uint8 > ( m_slotItems.Size() );
	*saver << count;

	for ( Uint8 i = 0; i < count; ++i )
	{
		ASSERT( !HasFlag( FLAG_CLONED ) && "Saving cloned item, report this to Drey" );
		*saver << m_slotItems[ i ].m_name;
		*saver << m_slotItems[ i ].m_randSeed;

		ASSERT( m_slotItems[ i ].m_abilities.Size() < 256 );
		Uint8 abilitiesCount = static_cast< Uint8 > ( m_slotItems[ i ].m_abilities.Size() );
		*saver << abilitiesCount;

		for ( Uint8 j = 0; j < abilitiesCount; ++j )
		{
			*saver << m_slotItems[ i ].m_abilities[ j ];
		}
	}

	{
		ASSERT( m_craftedAbilities.Size() < 256 );
		Uint8 abilitiesCount = static_cast< Uint8 > ( m_craftedAbilities.Size() );
		*saver << abilitiesCount;
		
		for ( Uint8 j = 0; j < abilitiesCount; ++j )
		{
			*saver << m_craftedAbilities[ j ];
		}
	}
		
	if( this->IsMounted() )	  // WTF is this?
	{
		*saver << m_category;
	}

	if ( HasFlag( FLAG_STORE_BASE_ABILITIES ) )
	{
		*saver << m_baseAbilities;
	}
}

void SInventoryItem::SetEnchantment( CName enchantment, CName stats )
{
	TDynArray< CName > slotItemNames;
	if ( GetSlotItemsNames( slotItemNames ) )
	{
		RemoveAllSlots();
	}

	AddBaseAbility( stats );

	m_enchantmentName = enchantment;
	m_enchantmentStats = stats;
}

void SInventoryItem::ClearEnchantment()
{
	if ( m_enchantmentName != CName::NONE )
	{
		RemoveBaseAbility( m_enchantmentStats );
		m_enchantmentName = CName::NONE;
		m_enchantmentStats = CName::NONE;
	}
}

void SInventoryItem::SetDyeColor( CName dyeItemName, Uint32 dyeColor )
{
	m_dyeColorName = dyeItemName;
	CName color = GetDyeColor( dyeColor );
	m_dyeColorAbilityName = color;
}

CName SInventoryItem::GetDyeColor( Uint32 dyeColor )
{
	if ( 1 == dyeColor )
	{
		return CNAME( dye_black );
	}
	else if ( 2 == dyeColor )
	{
		return CNAME( dye_blue );
	}
	else if ( 3 == dyeColor )
	{
		return CNAME( dye_brown );
	}
	else if ( 4 == dyeColor )
	{
		return CNAME( dye_gray );
	}
	else if ( 5 == dyeColor )
	{
		return CNAME( dye_green );
	}
	else if ( 6 == dyeColor )
	{
		return CNAME( dye_orange );
	}
	else if ( 7 == dyeColor )
	{
		return CNAME( dye_pink );
	}
	else if ( 8 == dyeColor )
	{
		return CNAME( dye_purple );
	}
	else if ( 9 == dyeColor )
	{
		return CNAME( dye_red );
	}
	else if ( 10 == dyeColor )
	{
		return CNAME( dye_turquoise );
	}
	else if ( 11 == dyeColor )
	{
		return CNAME( dye_white );
	}
	else if ( 12 == dyeColor )
	{
		return CNAME( dye_yellow );
	}
	else
	{
		return CNAME( dye_default );
	}
}

void SInventoryItem::SetDyePreviewColor( Uint32 dyeColor )
{
	if( CItemEntity* ent = GetItemEntity() )
	{
		if ( CAppearanceComponent* appearanceComponent = ent->FindComponent< CAppearanceComponent >() )
		{
			m_dyePreviewColorName = GetDyeColor( dyeColor );
		}
	}
}

void SInventoryItem::ClearDyePreviewColor()
{
	if ( m_dyePreviewColorName != CName::NONE )
	{
		m_dyePreviewColorName = CName::NONE;
	}
}

void SInventoryItem::ClearDyeColor()
{
	if ( m_dyeColorName != CName::NONE )
	{
		RemoveBaseAbility( m_dyeColorAbilityName );

		m_dyeColorName = CNAME( item_dye_default );
		m_dyeColorAbilityName = CNAME( dye_default );
	}
}

// EOF
