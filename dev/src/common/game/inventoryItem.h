/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "itemUniqueId.h"
#include "definitionsManager.h"


class CItemEntityProxy;
struct SItemUniqueId;
class CItemEntity;

struct SInventoryItemUIData
{
	DECLARE_RTTI_STRUCT( SInventoryItemUIData );

public:
	Int32					m_gridPosition;
	Int32					m_gridSize;
	Bool				m_isNew;


	SInventoryItemUIData()
		: m_gridPosition( -1 ) // negative value means "i don't care, it can be anywhere"
		, m_gridSize( 1 )
		, m_isNew( false )
	{}

	RED_INLINE SInventoryItemUIData& operator= ( const SInventoryItemUIData& other )
	{
		m_gridPosition = other.m_gridPosition;
		m_gridSize = other.m_gridSize;
		m_isNew = other.m_isNew;
		return *this;
	}

	static const SInventoryItemUIData INVALID;
};

BEGIN_CLASS_RTTI( SInventoryItemUIData );	
	PROPERTY( m_gridPosition );
	PROPERTY( m_gridSize );
	PROPERTY( m_isNew );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SInventoryItemInitData
{
	CItemEntityProxy*	m_itemEntityProxy;
	Uint32				m_quantity;
	Bool				m_isLootable;
	Bool				m_isCloned;
	Bool				m_markAsNew;
	SItemUniqueId		m_uniqueId;
	Bool				m_playerItem;
	Bool				m_shouldBeRebalanced;

	SInventoryItemInitData( Bool playerItem )
		: m_itemEntityProxy( nullptr )
		, m_quantity( 1 )
		, m_isLootable( false )
		, m_isCloned( false )
		, m_markAsNew( false )
		, m_uniqueId( SItemUniqueId::INVALID )
		, m_playerItem( playerItem )
		, m_shouldBeRebalanced( false )
	{}
};

//////////////////////////////////////////////////////////////////////////

struct SInventoryItem
{
	DECLARE_RTTI_STRUCT( SInventoryItem );

	struct SSlotItem
	{
		CName				m_name;
		Uint32				m_randSeed;
		TDynArray< CName >	m_abilities;

		SSlotItem()
			: m_name( CName::NONE )
			, m_randSeed( 0 )
		{}

		SSlotItem( CName name, Uint32 randSeed )
			: m_name( name )
			, m_randSeed( randSeed )
		{}

		//! Equality test for item stacking
		friend Bool operator==( const SSlotItem& lhs, const SSlotItem& rhs )
		{
			return ( lhs.m_name == rhs.m_name
				&&	lhs.m_randSeed == rhs.m_randSeed
				&&	lhs.m_abilities == rhs.m_abilities );
		}
	};

public:

	SInventoryItem();
	SInventoryItem( const CName& name, const SItemDefinition& itemDef, const SInventoryItemInitData& initData );

	//! Init item with definition data
	void Init( const CName& name, const SItemDefinition& itemDef, const SInventoryItemInitData& initData );

	//! Clone item
	void CloneOf( const SInventoryItem& item, const SItemUniqueId& newUniqueId );

	//! Create proxy for item definition on given layer
	void CreateProxy( const SItemDefinition& itemDef, CLayer* layer, const String& templateName, Bool collapse );

	//! Set item proxy (always mark item as "Cloned", if proxy is owned by another item/inventory)
	void SetProxy( CItemEntityProxy* proxy, Bool cloned );

	//! Move proxy from another item
	void MoveProxy( SInventoryItem* item );

	//! Release proxy (set to null)
	void ReleaseProxy();

	//! Destroy item entity proxy
	void DestroyProxy();

	//! Reset proxy if hasn't been registered
	void RevalidateProxy();

	//! Get item entity
	CItemEntity* GetItemEntity() const;

	//! Get index of slot for given item. Return -1 if there's no such item in slots.
	//! If randSeed = 0 return first occurrence of item specified by name.
	Int32 GetSlotItemIndex( const CName& name, Uint32 randSeed = 0 ) const;

	//! Get slot items names. Returns false if there is none.
	Bool GetSlotItemsNames( TDynArray< CName > & names ) const;

	//! Returns max number of enhancement slots based on Item Category.
	Uint8 GetSlotItemsLimit() const;

	Uint8 GetSlotItemsMax() const;

	//! Add slot
	Bool AddSlot();

	//! Add slot item
	Bool AddSlotItem( const CName& name, Bool playerItem, Uint32 randSeed );

	//! Remove slot item (if randSeed = 0 remove all items with given name)
	Bool RemoveSlotItem( const CName& name, Uint32 slotItemSeed = 0 );
	Bool RemoveSlotItem( Uint32 slotIndex );
	void RemoveAllSlots();

	//! Add crafted ability
	Bool AddCraftedAbility( const CName& abilityName, Bool allowDuplicate );

	//! Remove crafted ability
	Bool RemoveCraftedAbility( const CName& abilityName );

	Bool AddBaseAbility( const CName& abilityName );
	Bool RemoveBaseAbility( const CName& abilityName );

	//! Get slot abilities ( abilities from slot items )
	void GetSlotAbilities( TDynArray< CName >& abilities, Int32 excludedSlot = -1 ) const;

	//! Get given slot abilities ( abilities from given slot item )
	void GetSlotAbilities( Int32 slot, TDynArray< CName >& abilities ) const;

	//! Get base abilities
	void GetBaseAbilities( TDynArray< CName >& abilities ) const;

	//! Get all abilities (base and slot)
	void GetAllAbilities( TDynArray< CName >& allAbilities, Int32 excludedSlot = -1 ) const;

	//! Get effects within abilities
	void GetContainedAbilities( TDynArray< CName >& containedAbilities ) const;

	//! Get attributes names
	void GetAllAttributes( TDynArray< CName > & outAttributes ) const;

	//! Get base attributes names (base and crafting abilities)
	void GetBaseAttributes( TDynArray< CName >& abilities ) const;

	//! Get attribute value from item
	SAbilityAttributeValue GetAttribute( CName attrName ) const;

	//! Returns the value of given attribute which would be a result of hypothetical upgrade the item. Does not upgrade item.
	SAbilityAttributeValue PreviewAttributeAfterUpgrade( const SInventoryItem* upgradeItem, const CName& attributeName ) const;

	//! Get info string
	String GetInfo() const;

	//! Is item a weapon
	static Bool IsWeapon( const CName& itemName );

	//! Are item abilities active (when item is mounted or held)
	Bool AreAbilitiesActive() const;

	Bool StreamLoad( ISaveFile* loader, Uint32 version, Bool isPlayerOwner );
	void StreamSave( ISaveFile* saver );

	//! Get item name
	RED_INLINE CName GetName() const;

	//! Get item category
	RED_INLINE CName GetCategory() const;

	//! Get slot items
	RED_INLINE const TDynArray< SSlotItem > & GetSlotItems() const;

	//! Get crafted abilities
	RED_INLINE const TDynArray< CName > & GetCraftedAbilities() const;

	//! Get item entity proxy
	RED_INLINE CItemEntityProxy* GetItemEntityProxy();
	RED_INLINE const CItemEntityProxy* GetItemEntityProxy() const;

	//! Get item tags
	RED_INLINE TDynArray< CName > & GetTags();
	RED_INLINE const TDynArray< CName > & GetTags() const;

	//! Check item tag(s)
	RED_INLINE Bool HasTag( CName tag ) const;
	RED_INLINE Bool HasTags( const TDynArray< CName > & tag, Bool all = false ) const;

	//! Get item quantity
	RED_INLINE Uint32 GetQuantity() const;

	//! Set item quantity
	RED_INLINE void SetQuantity( Uint32 quantity );

	//! Change item quantity
	RED_INLINE void ChangeQuantity( Int32 quantityChange );

	//! Get item durability
	RED_INLINE Float GetDurability() const;

	//! Set item durability
	RED_INLINE void SetDurability( Float durability );

	//! Set item unique ID
	RED_INLINE void SetUniqueId( const SItemUniqueId& id ) { m_uniqueId = id; }

	//! Get item unique ID
	RED_INLINE SItemUniqueId GetUniqueId() const;

	//! Get static random seed
	RED_INLINE Uint16 GetStaticRandomSeed() const;

	//! Is item a weapon
	RED_INLINE Bool IsWeapon() const;

	//! Return flags in a one bitfield
	RED_INLINE Uint64 GetFlags() const;

	//! Is there entity specified in item's definition
	RED_INLINE Bool HasEntityDefined() const;

	//! Is item stackable
	RED_INLINE Bool IsStackable() const;

	//! Is item dropped after death
	RED_INLINE Bool IsLootable() const;

	//! Set item is lootable
	RED_INLINE void SetIsLootable( Bool isLootable );

	//! Is item cloned
	RED_INLINE Bool IsCloned() const;

	//! Is item held
	RED_INLINE Bool IsHeld() const;

	//! Set item is held
	RED_INLINE void SetIsHeld( Bool isHeld );

	//! Is item mounted
	RED_INLINE Bool IsMounted() const;

	//! Set item is mounted
	RED_INLINE void SetIsMounted( Bool isMounted );

	void SetIsInvisible( Bool isInvisible );	
	RED_INLINE Bool IsInvisible() const;

	//! Is item proxy held by other inventory
	RED_INLINE Bool IsProxyTaken() const;

	//! Set is item proxy held by other inventory
	RED_INLINE void SetIsProxyTaken( Bool isProxyTaken );

	//!Is item added by animation event (wasn't present in the inventory before)
	RED_INLINE Bool IsAddedByAnimation() const;

	//! Set item is added by animation event (wasn't present in the inventory before)
	RED_INLINE void SetIsAddedByAnimation( Bool addedByAnimation );

	//! Get item class
	RED_INLINE EInventoryItemClass GetItemClass() const;

	//! Should be rebalanced
	RED_INLINE Bool ShouldBeRebalanced() const { return HasFlag( FLAG_REBALANCE ) && !m_itemTags.Exist( CNAME( Quest ) ); }

	//! Is quest item (has "Quest" tag but doesn't have "NoShow" tag)
	RED_INLINE Bool IsQuestItem() const { return m_itemTags.Exist( CNAME( Quest ) ) && !m_itemTags.Exist( CNAME( NoShow ) ); }

	//! Does item have "NoDrop" or "NoShow" flag (which prevents item from being dropped as a loot)
	RED_INLINE Bool IsNoDropNoShow() const { return m_itemTags.Exist( CNAME( NoDrop ) ) || m_itemTags.Exist( CNAME( NoShow ) ); }

	void Collapse( Bool collapse );

	RED_INLINE CName GetEnchantment() const { return m_enchantmentName; }
	RED_INLINE CName GetEnchantmentStats() const { return m_enchantmentStats; }
	RED_INLINE Bool IsEnchanted() const { return m_enchantmentName != CName::NONE; }

	void SetEnchantment( CName enchantment, CName stats );
	void ClearEnchantment();

	RED_INLINE CName GetDyeColorName() const { return m_dyeColorName; }
	RED_INLINE CName GetDyeColorStats() const { return m_dyeColorAbilityName; }
	RED_INLINE Bool IsItemColored() const { return m_dyeColorName != CNAME( item_dye_default ); }

	RED_INLINE CName GetDyePreviewColor() const { return m_dyePreviewColorName; }
	void SetDyePreviewColor( Uint32 dyeColor );
	void ClearDyePreviewColor();

	void SetDyeColor( CName dyeItemName, Uint32 dyeColor );
	CName GetDyeColor( Uint32 colorIndex );
	void ClearDyeColor();

private:

	//! Initialize item with minimal item definition info
	void ReadUniformDefinitionData( const SItemDefinition& itemDef, Bool isPlayer );

	//! Assign static random seed
	void AssignStaticRandomSeed();

public:

	//! Equality test for item stacking
	friend Bool operator==( const SInventoryItem& lhs, const SInventoryItem& rhs )
	{
		return ( lhs.m_name == rhs.m_name
			&&	lhs.m_slotItems == rhs.m_slotItems
			&&	lhs.m_craftedAbilities == rhs.m_craftedAbilities
			&&	lhs.m_staticRandomSeed == rhs.m_staticRandomSeed );
	}

	// Flags from item definition, to be copied when item is cloned
	static const Uint64 FLAG_NO_ENTITY				= FLAG( 0 );		//!< Item has no entity defined at all, actions like grabbing and putting will be simplified
	static const Uint64 FLAG_NO_HAND				= FLAG( 1 );		//!< Item requires no hands to hold ( basically means that no grabbing or putting should occur )
	static const Uint64 FLAG_STACKABLE				= FLAG( 2 );		//!< Item is stacked if more than one
	static const Uint64 FLAG_LOOTABLE				= FLAG( 3 );		//!< Item is stacked if more than one

	// Flags related to actual item state, should not be copied
	static const Uint64 FLAG_CLONED					= FLAG( 4 );		//!< Item is a clone of an action point item
	static const Uint64	FLAG_MOUNT					= FLAG( 5 );		//!< Item is mount
	static const Uint64	FLAG_HELD					= FLAG( 6 );		//!< Item is held
	static const Uint64	FLAG_PROXY_TAKEN			= FLAG( 7 );		//!< Is item proxy held by other inventory

	static const Uint64 FLAG_REBALANCE				= FLAG( 8 );		//!< Should this item be rebalanced?
	static const Uint64 FLAG_INVISIBLE				= FLAG( 9 );		//!< Item entity is invisible
	static const Uint64 FLAG_ADDED_BY_ANIM			= FLAG( 10 );		//!< Item was added by animation event (wasn't present in the inventory before)
	static const Uint64 FLAG_STORE_QUEST_TAG		= FLAG( 11 );		//!< This item should have a 'Quest' tag (i'm just using flag instead of saving/restoring tags to save some memory)
	static const Uint64 FLAG_STORE_NOSHOW_TAG		= FLAG( 12 );		//!< This item should have a 'NoShow' tag (i'm just using flag instead of saving/restoring tags to save some memory)
	static const Uint64 FLAG_STORE_LARGE_QUANTITY	= FLAG( 13 );		//!< This item is stacked in large quantity
	static const Uint64 FLAG_STORE_ENCHANTMENT		= FLAG( 14 );		//!< This item stores enchantments
	static const Uint64 FLAG_STORE_BASE_ABILITIES	= FLAG( 15 );		//!< This item have non-standard base abilities set (changed form script)
	static const Uint64 FLAG_STORE_DYE_COLOR		= FLAG( 16 );		//!< This item stores dye color

private:
	static const Uint64 COPY_MASK = FLAG_REBALANCE | FLAG_NO_ENTITY | FLAG_NO_HAND | FLAG_STACKABLE | FLAG_LOOTABLE | FLAG_CLONED | FLAG_ADDED_BY_ANIM | FLAG_STORE_QUEST_TAG | FLAG_STORE_NOSHOW_TAG | FLAG_STORE_ENCHANTMENT | FLAG_STORE_BASE_ABILITIES;

public:

	struct SItemModifierVal
	{
		enum EType
		{
			EMVT_None,
			EMVT_Float,
			EMVT_Int
		};

		EType			m_type;
		union
		{
			Float		m_float;
			Int32		m_int;
		};
	};	

	SItemModifierVal* GetItemMod( CName modName, Bool insertIfMissing = false )
	{
		TArrayMap< CName, SItemModifierVal >::iterator pair =  m_modifiers.Find( modName );
		if ( ( pair == m_modifiers.End() ) && insertIfMissing )
		{
			pair = m_modifiers.Insert( modName, SItemModifierVal() );
		}
		return ( pair != m_modifiers.End() ) ? &pair->m_second : nullptr;
	}

private:	

	TDynArray< SSlotItem >	m_slotItems;			//!< Names of the items in extension slots
	TDynArray< CName >		m_baseAbilities;		//!< (Randomly chosen from the definition) base item abilities
	TDynArray< CName >		m_craftedAbilities;		//!< Names of dynamicaly added abilities (e.g. from crafting or effects)
	TDynArray< CName >		m_itemTags;				//!< Tags of the item
	TArrayMap< CName, SItemModifierVal >	m_modifiers;	
	CItemEntityProxy*		m_itemEntityProxy;		//!< Entity representation of this item
public:
	SInventoryItemUIData	m_uiData;				//!< UI-specific data
private:
	Uint32					m_itemQuantity;			//!< Quantity of this item in inventory
	CName					m_name;					//!< Item name ( visible in editor )
	CName					m_category;				//!< Item category
	SItemUniqueId			m_uniqueId;				//!< Unique identifier of item (unique per inventory)
	Float					m_itemDurability;		//!< Durability of the item. -1 means not set or not applicable.
	Uint16					m_staticRandomSeed;		//!< Seed for static random number generation
	Uint64					m_flags;				//!< Flags related to item definition
	Uint8					m_maxSlotItems;			//!< Maximum number of slot items
	CName					m_enchantmentName;		//!< Name of enchantment
	CName					m_enchantmentStats;		//!< Name of enchantment ability stats
	CName					m_dyePreviewColorName;	//!< Name of preview color
	CName					m_dyeColorName;			//!< Name of dye color
	CName					m_dyeColorAbilityName;	//!< Name of dye color ability name
public:

	//! Check if flag exists
	RED_INLINE Bool HasFlag( Uint64 flag ) const;

	//! Set or reset flag
	RED_INLINE void SetFlag( Uint64 flag, Bool set );

	//! Item with default values
	static SInventoryItem INVALID;
};

BEGIN_CLASS_RTTI( SInventoryItem );	
	PROPERTY_CUSTOM_EDIT( m_name, TXT( "Item name" ), TXT("ItemSelection") );
	PROPERTY_EDIT( m_itemQuantity, TXT("Item quantity") );
	PROPERTY( m_uniqueId );
	PROPERTY( m_flags );
	PROPERTY( m_staticRandomSeed );
	PROPERTY( m_uiData );
	PROPERTY( m_craftedAbilities );
	PROPERTY( m_enchantmentName );
	PROPERTY( m_enchantmentStats );
	PROPERTY( m_dyeColorName );
	PROPERTY( m_dyeColorAbilityName );
	PROPERTY( m_dyePreviewColorName );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

RED_INLINE CName SInventoryItem::GetName() const
{
	return m_name;
}

RED_INLINE CName SInventoryItem::GetCategory() const
{
	return m_category;
}

RED_INLINE const TDynArray< SInventoryItem::SSlotItem > & SInventoryItem::GetSlotItems() const
{
	return m_slotItems;
}

RED_INLINE const TDynArray< CName > & SInventoryItem::GetCraftedAbilities() const
{
	return m_craftedAbilities;
}

RED_INLINE CItemEntityProxy* SInventoryItem::GetItemEntityProxy()
{
	return m_itemEntityProxy;
}

RED_INLINE const CItemEntityProxy* SInventoryItem::GetItemEntityProxy() const
{
	return m_itemEntityProxy;
}

RED_INLINE TDynArray< CName > & SInventoryItem::GetTags()
{
	return m_itemTags;
}

RED_INLINE const TDynArray< CName > & SInventoryItem::GetTags() const
{
	return m_itemTags;
}

RED_INLINE Bool SInventoryItem::HasTag( CName tag ) const
{
	return m_itemTags.Exist( tag );
}

RED_INLINE Bool SInventoryItem::HasTags( const TDynArray< CName > & tags, Bool all /* = false */ ) const
{
	if ( tags.Size() == 0 )
	{
		return false;
	}
	Uint32 count = 0;
	for ( TDynArray< CName >::const_iterator it = tags.Begin(), itEnd = tags.End(); it != itEnd; ++it )
	{
		if ( m_itemTags.Exist( *it ) )
		{
			if ( !all )
			{
				return true;
			}
			else
			{
				count++;
			}
		}
	}
	return ( all && count == tags.Size() );
}

RED_INLINE Uint32 SInventoryItem::GetQuantity() const
{
	return m_itemQuantity;
}

RED_INLINE void SInventoryItem::SetQuantity( Uint32 quantity )
{
	m_itemQuantity = quantity;
}

RED_INLINE void SInventoryItem::ChangeQuantity( Int32 quantityChange )
{
	m_itemQuantity += quantityChange;
}

RED_INLINE Float SInventoryItem::GetDurability() const
{
	return m_itemDurability;
}

RED_INLINE void SInventoryItem::SetDurability( Float durability )
{
	m_itemDurability = durability;
}

RED_INLINE SItemUniqueId SInventoryItem::GetUniqueId() const
{
	return m_uniqueId;
}

RED_INLINE Uint16 SInventoryItem::GetStaticRandomSeed() const
{
	return m_staticRandomSeed;
}

RED_INLINE Bool SInventoryItem::IsWeapon() const
{
	return IsWeapon( m_name );
}

RED_INLINE Uint64 SInventoryItem::GetFlags() const
{
	return m_flags;
}

RED_INLINE Bool SInventoryItem::HasEntityDefined() const
{
	return ( m_flags & FLAG_NO_ENTITY ) == 0;
}

RED_INLINE Bool SInventoryItem::IsStackable() const
{
	return HasFlag( FLAG_STACKABLE );
}

RED_INLINE Bool SInventoryItem::IsLootable() const
{
	return HasFlag( FLAG_LOOTABLE );
}

RED_INLINE void SInventoryItem::SetIsLootable( Bool isLootable )
{
	SetFlag( FLAG_LOOTABLE, isLootable );
}

RED_INLINE Bool SInventoryItem::IsCloned() const
{
	return HasFlag( FLAG_CLONED );
}

RED_INLINE Bool SInventoryItem::IsHeld() const
{
	return HasFlag( FLAG_HELD );
}

RED_INLINE void SInventoryItem::SetIsHeld( Bool isHeld )
{
	SetFlag( FLAG_HELD, isHeld );
	if ( isHeld )
	{
		// this should be exclusive - either mounted or held
		SetFlag( FLAG_MOUNT, false );
	}
}

RED_INLINE Bool SInventoryItem::IsMounted() const
{
	return HasFlag( FLAG_MOUNT );
}

RED_INLINE void SInventoryItem::SetIsMounted( Bool isMounted )
{
	SetFlag( FLAG_MOUNT, isMounted );
	if ( isMounted )
	{
		// this should be exclusive - either mounted or held
		SetFlag( FLAG_HELD, false );
	}
}

RED_INLINE Bool SInventoryItem::IsInvisible() const
{
	return HasFlag( FLAG_INVISIBLE );
}

RED_INLINE Bool SInventoryItem::IsProxyTaken() const
{
	return HasFlag( FLAG_PROXY_TAKEN );
}

RED_INLINE void SInventoryItem::SetIsProxyTaken( Bool isProxyTaken )
{
	SetFlag( FLAG_PROXY_TAKEN, isProxyTaken );
}

RED_INLINE Bool SInventoryItem::IsAddedByAnimation() const
{
	return HasFlag( FLAG_ADDED_BY_ANIM );
}

RED_INLINE void SInventoryItem::SetIsAddedByAnimation( Bool addedByAnimation )
{
	SetFlag( FLAG_ADDED_BY_ANIM, addedByAnimation );
}

RED_INLINE EInventoryItemClass SInventoryItem::GetItemClass() const
{
	return SItemDefinition::GetItemClass( m_itemTags );
}

RED_INLINE Bool SInventoryItem::HasFlag( Uint64 flag ) const
{
	return ( m_flags & flag ) != 0;
}

RED_INLINE void SInventoryItem::SetFlag( Uint64 flag, Bool set )
{
	if ( set )
	{
		m_flags |= flag;
	}
	else
	{
		m_flags &= ~flag;
	}
}