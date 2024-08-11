/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/actorInterface.h"
#include "abilities.h"

class CLootDefinitions;
class CXMLFileReader;
struct SItemExtDefinition;

//////////////////////////////////////////////////////////////////////////
enum EInventoryItemClass
{
	InventoryItemClass_Common	= 1,
	InventoryItemClass_Magic	= 2,
	InventoryItemClass_Rare		= 3,
	InventoryItemClass_Epic		= 4,
};

BEGIN_ENUM_RTTI( EInventoryItemClass )
	ENUM_OPTION( InventoryItemClass_Common );
	ENUM_OPTION( InventoryItemClass_Magic );
	ENUM_OPTION( InventoryItemClass_Rare );
	ENUM_OPTION( InventoryItemClass_Epic );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SItemSet
{
	DECLARE_RTTI_STRUCT( SItemSet );
	
	CName	m_name;
	TDynArray< CName > m_parts;
	TDynArray< CName > m_abilities;

	Red::System::GUID			m_creatorTag; //! tag is used for determination of definition creator (e.g. DLC mounter)
};

BEGIN_CLASS_RTTI( SItemSet );
	PROPERTY( m_name );
	PROPERTY( m_parts );
	PROPERTY( m_abilities );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SItemStat
{
	CName	m_statType;
	Float	m_statWeight;
	Bool	m_statIsPercentage;

	SItemStat() 
		: m_statType ( nullptr )
		, m_statWeight ( -1.0f )
		, m_statIsPercentage ( false )
	{}

	DECLARE_RTTI_STRUCT( SItemStat );
};

BEGIN_CLASS_RTTI( SItemStat );
	PROPERTY( m_statType );
	PROPERTY( m_statWeight );
	PROPERTY( m_statIsPercentage );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SItemDamageCurve
{
	Float	m_term1;
	Float	m_term2;
	Float	m_term3;

	SItemDamageCurve() 
		: m_term1 ( -1.0f )
		, m_term2 ( -1.0f )
		, m_term3 ( -1.0f )
	{}

	DECLARE_RTTI_STRUCT( SItemDamageCurve );
};

BEGIN_CLASS_RTTI( SItemDamageCurve );
	PROPERTY( m_term1 );
	PROPERTY( m_term2 );
	PROPERTY( m_term3 );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SItemTagModifier
{
	//!< Modifiers used to weight each tag for Price calculation.
	THashMap< CName, Float > m_modifierMap;

	DECLARE_RTTI_STRUCT( SItemTagModifier );
};

BEGIN_CLASS_RTTI( SItemTagModifier );
	PROPERTY( m_modifierMap );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SIngredientCategoryElement
{
	DECLARE_RTTI_STRUCT( SIngredientCategoryElement );
	
	CName	m_name;
	Uint32	m_priority;
};

BEGIN_CLASS_RTTI( SIngredientCategoryElement );
PROPERTY( m_name );
PROPERTY( m_priority );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SIngredientCategory
{
	DECLARE_RTTI_STRUCT( SIngredientCategory );
	
	CName		m_name;
	Bool		m_specified;
	TDynArray< SIngredientCategoryElement > m_elements;
};

BEGIN_CLASS_RTTI( SIngredientCategory );
PROPERTY( m_name );
PROPERTY( m_specified );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SItemDefinition
{
	friend class CDefinitionsManager;

	struct SItemAbility
	{
		CName	m_name;
		Float	m_chance;
		
		SItemAbility() 
			: m_chance ( -1.0f )
		{}

		RED_INLINE CName GetName() const { return m_name; }
		RED_INLINE Float GetChance() const { return m_chance; }
	};

	struct SIngredientEntry
	{
		CName	m_itemName;
		Uint32	m_quantity;
	};

	struct SItemAnimSwitch
	{
		CName	m_category;		//< Category of the item this entry can switch to
		CName	m_equipSlot;	//< Equip slot of the item that this entry can switch to
		CName	m_eventName;	//< Behavior event to call for a switch
		CName	m_actName;		//< Behavior activation name
		CName	m_deactName;	//< Behavior deactivation name
	};

	struct SItemAnimAction
	{
		CName	m_name;
		CName	m_event;
		CName	m_act;
		CName	m_deact;
	};

	struct SItemVariant
	{
		TDynArray< CName > m_categories;
		String m_template;
		TDynArray< CName > m_affectingItems;
		Bool	m_expectAll;

		SItemVariant()
			: m_expectAll( false )
		{}

		RED_INLINE Bool operator ==( const SItemVariant& itemVariant ) const
		{
			return		(this->m_categories == itemVariant.m_categories)
					&&	(this->m_template == itemVariant.m_template)
					&&  (this->m_affectingItems == itemVariant.m_affectingItems)
					&&  (this->m_expectAll == itemVariant.m_expectAll);
		}
	};

	struct SCollapseCond
	{
		TDynArray< CName >			m_collapseCategoryCond;	//!< categories that makes this item to collapse
		TDynArray< CName >			m_collapseItemCond;		//!< items that makes this item to collapse
		TDynArray< CName >			m_uncollapseItemCond;	//!< items that makes this item not to collapse
	};

	struct SItemPart
	{
		CName	m_name;
	};

	struct SItemPartsEntry
	{
		CName	m_name;
		Uint32	m_count;
	};

	struct OverridableProperties
	{
		CName						m_equipSlot;			//!< Slot used when not held
		String						m_equipTemplateName;	//!< Template used when the item is equiped
		String						m_upgradeBasedTemplateName;	//!< Entity tamplate compatible with upgrade system
		CName						m_holdSlot;				//!< Slot used when held
		String						m_holdTemplateName;		//!< Template used when the item is hold (m_equipTemplateName is used if this is empty)
		CName						m_itemAppearanceName;	//!< Item appearance name
		CName						m_colorVariantName;		//!< Item color variant name
		Int32						m_minAbilities;			//!< Minimum number of used abilities
		Int32						m_maxAbilities;			//!< Maximum number of used abilities. If equal to -1 (default) there are no restrictions.
		TDynArray< SItemAbility >	m_baseAbilities;		//!< Basic abilities	
		TDynArray< CName >			m_slotItems;			//!< Extension items	
		TDynArray< CName >			m_boundItems;			//!< Items to be equipped along with this one
		TDynArray< SItemVariant >	m_variants;				//!< templates that should be loaded when item of given category is mounted
		SCollapseCond				m_collapseCond;			//!< Condition to (un)collapse this item
		Float						m_initialDurability;	//!< Initial Durability value
		Float						m_maxDurability;		//!< Max Durability value
		TDynArray< SItemAnimAction > m_animActions;			//!< Anim actions for item ( draw/holster/attack... )
		CName						m_actorAnimState;		//!< Item carry mode for actor behavior setup
		TDynArray< SItemAnimSwitch > m_animSwitches;
		CName						m_upgradeListName;		//!< Name of upgrade list

		OverridableProperties()
			: m_minAbilities( 0 )
			, m_maxAbilities( -1 )
			, m_initialDurability( -1 )
			, m_maxDurability( -1 )
		{
			m_upgradeListName = CNAME( default );
		}

		OverridableProperties( const OverridableProperties& overridableProps )
		{
			m_equipSlot = overridableProps.m_equipSlot;
			m_equipTemplateName = overridableProps.m_equipTemplateName;
			m_upgradeBasedTemplateName = overridableProps.m_upgradeBasedTemplateName;
			m_holdSlot = overridableProps.m_holdSlot;
			m_holdTemplateName = overridableProps.m_holdTemplateName;
			m_itemAppearanceName = overridableProps.m_itemAppearanceName;
			m_colorVariantName = overridableProps.m_colorVariantName;
			m_minAbilities = overridableProps.m_minAbilities;
			m_maxAbilities = overridableProps.m_maxAbilities;
			m_baseAbilities = overridableProps.m_baseAbilities;
			m_slotItems = overridableProps.m_slotItems;
			m_boundItems = overridableProps.m_boundItems;
			m_variants = overridableProps.m_variants;
			m_collapseCond = overridableProps.m_collapseCond;
			m_initialDurability = overridableProps.m_initialDurability;
			m_maxDurability = overridableProps.m_maxDurability;
			m_animActions = overridableProps.m_animActions;
			m_actorAnimState = overridableProps.m_actorAnimState;
			m_animSwitches = overridableProps.m_animSwitches;
			m_upgradeListName = overridableProps.m_upgradeListName;
		}
	};

	SItemDefinition()
		: m_enhancementSlotCount( 0 )
		, m_flags( 0 )
		, m_stackSize( 1 )
		, m_price( 0 )
		, m_weight( -1.0f )
		, m_gridSize( 1 )
		, m_playerOverridableProperties( nullptr )
	{}


	SItemDefinition( const SItemDefinition& itemDefinition );

	~SItemDefinition();

	static const Uint16			FLAG_WEAPON		= FLAG( 0 );	//<! Item is weapon

	// The below flags exclude each other, but on the level of parsing it is made sure that no conflict can happen
	static const Uint16			FLAG_ABILITY_ON_MOUNT	= FLAG( 1 );	//<! Item abilities apply when item is mount
	static const Uint16			FLAG_ABILITY_ON_HOLD	= FLAG( 2 );	//<! Item abilities apply when item is held
	static const Uint16			FLAG_ABILITY_ON_HIDDEN	= FLAG( 3 );	//<! Item abilities apply when item is hidden

	OverridableProperties		m_defaultProperties;
	OverridableProperties*		m_playerOverridableProperties;
	
	CName						m_category;				//!< Category this item belongs to

	Uint32						m_price;				//!< Price of item
	Float						m_weight;				//!< Weight of Item
	TDynArray< CName >			m_itemTags;				//!< Tags of this item
	Uint8						m_enhancementSlotCount;	//!< Number of ability slots for item	
	Uint16						m_flags;				//!< Item definition flags
	TDynArray< SIngredientEntry > m_ingredients;		//!< List of items this item can produce

	CName						m_craftedItemName;		//!< Name of the item crafted with ingredients list
	CName						m_craftsmanType;		//!< Type name of the the crafstman (e.g., Smith )
	CName						m_craftsmanLevel;		//!< Required level of the craftsman (e.g., Grandmaster )

	TDynArray< SItemPartsEntry > m_recyclingParts;		//!< Tags for recycling parts

	Uint32						m_gridSize;
	
	CName						m_setName;				//!< Set Name
	
	String						m_iconPath;				//!< Icon path
	String						m_localizationKeyName;	//!< Localistaion name
	String						m_localizationKeyDesc;	//!< Localisation description
	Uint32						m_stackSize;			//!< Amount of items on stack

	TDynArray< SItemExtDefinition*> m_attachedItemExtDefinition; //!< item extensions attached to this item, this field is modifying by SItemExtDefinition::AttachToItem and SItemExtDefinition::DettachFromItem

	Red::System::GUID			m_creatorTag; //! tag is used for determination of definition creator (e.g. DLC mounter)

	public:
			
		RED_INLINE CName GetEquipSlot( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_equipSlot : m_defaultProperties.m_equipSlot; }
		RED_INLINE const String& GetEquipTemplateName( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_equipTemplateName : m_defaultProperties.m_equipTemplateName; }
		RED_INLINE const String& GetUpgradeBasedTemplateName( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_upgradeBasedTemplateName : m_defaultProperties.m_upgradeBasedTemplateName; }
		RED_INLINE CName GetHoldSlot( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_holdSlot : m_defaultProperties.m_holdSlot; }
		RED_INLINE const String& GetHoldTemplateName( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_holdTemplateName : m_defaultProperties.m_holdTemplateName; }
		RED_INLINE CName GetItemAppearanceName( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_itemAppearanceName : m_defaultProperties.m_itemAppearanceName; }
		RED_INLINE Int32 GetMinAbilities( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_minAbilities : m_defaultProperties.m_minAbilities; }
		RED_INLINE Int32 GetMaxAbilities( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_maxAbilities : m_defaultProperties.m_maxAbilities; }
		RED_INLINE const TDynArray< SItemAbility >& GetBaseAbilities( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_baseAbilities : m_defaultProperties.m_baseAbilities; }
		RED_INLINE const TDynArray< CName >& GetBoundItems( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_boundItems : m_defaultProperties.m_boundItems; }
		RED_INLINE const TDynArray< SItemVariant >& GetVariants( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_variants : m_defaultProperties.m_variants; }
		RED_INLINE CName GetActorAnimState( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_actorAnimState : m_defaultProperties.m_actorAnimState; }
		RED_INLINE const TDynArray< SItemAnimSwitch >& GetAnimSwitches( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_animSwitches : m_defaultProperties.m_animSwitches; }
		RED_INLINE Float GetInitialDurability( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_initialDurability : m_defaultProperties.m_initialDurability; }
		RED_INLINE Float GetMaxDurability( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_maxDurability : m_defaultProperties.m_maxDurability; }
		RED_INLINE const TDynArray< SItemAnimAction >& GetAnimActions( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_animActions : m_defaultProperties.m_animActions; }
		RED_INLINE CName GetUpgradeListName( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_upgradeListName : m_defaultProperties.m_upgradeListName; }
		RED_INLINE const SCollapseCond& GetCollapseCond( Bool player ) const { return player && m_playerOverridableProperties ? m_playerOverridableProperties->m_collapseCond : m_defaultProperties.m_collapseCond; }

		RED_INLINE CName GetCategory() const { return m_category; }
		RED_INLINE const TDynArray< SItemPartsEntry >& GetRecyclingParts() const { return m_recyclingParts; }
		RED_INLINE Uint32 GetPrice() const { return m_price; }
		RED_INLINE Float GetWeight() const { return m_weight; }
		RED_INLINE const TDynArray< CName >& GetItemTags() const { return m_itemTags; }
		RED_INLINE Uint8 GetEnhancementSlotCount() const { return m_enhancementSlotCount; }
		RED_INLINE const TDynArray< SIngredientEntry >& GetIngredients() const { return m_ingredients; }
		RED_INLINE CName GetCraftedItemName() const { return m_craftedItemName; }
		RED_INLINE CName GetCraftsmanType() const { return m_craftsmanType; }
		RED_INLINE CName GetCraftsmanLevel() const { return m_craftsmanLevel; }
		RED_INLINE Uint32 GetGridSize() const { return m_gridSize; }		
		RED_INLINE CName GetSetName() const { return m_setName; }
		RED_INLINE const String& GetIconPath() const { return m_iconPath; }
		RED_INLINE const String& GetLocalizationKeyName() const { return m_localizationKeyName; }
		RED_INLINE const String& GetLocalizationKeyDesc() const { return m_localizationKeyDesc; }
		RED_INLINE Uint32 GetStackSize() const { return m_stackSize; }
		
		
	public:
 	void GetItemDescription( String& desc, Bool player ) const;

	Bool IsWeapon() const { return ( m_flags & FLAG_WEAPON ) != 0; }
	Bool IsStackable() const { return m_stackSize > 1; }

	Bool IsAbilityEnabledOnMount() const { return ( m_flags & FLAG_ABILITY_ON_MOUNT ) != 0; }
	Bool IsAbilityEnabledOnHold() const { return ( m_flags & FLAG_ABILITY_ON_HOLD ) != 0; }
	Bool IsAbilityEnabledOnHidden() const { return ( m_flags & FLAG_ABILITY_ON_HIDDEN ) != 0; }
	Bool HasAbility( const CName& abilityName, Bool player ) const;
	Bool GetBaseAbilities( TDynArray< const SItemAbility* > & baseAbilities, Bool player ) const;
	Uint32 ChooseAbilitiesCount( Bool playerItem, Uint32 randomSeed = 0 ) const;
	void ChooseAbilities( TDynArray< CName > & abilities, Bool playerItem, Uint32 randomSeed = 0 ) const;

	RED_INLINE static EInventoryItemClass GetItemClass( const TDynArray<CName> & itemTags )
	{
		if ( itemTags.Exist( CNAME( TypeEpic ) ) )		return InventoryItemClass_Epic;
		if ( itemTags.Exist( CNAME( TypeRare ) ) )		return InventoryItemClass_Rare;
		if ( itemTags.Exist( CNAME( TypeMagic ) ) )		return InventoryItemClass_Magic;
		return InventoryItemClass_Common;
	}
	RED_INLINE EInventoryItemClass GetItemClass() const
	{
		return GetItemClass( m_itemTags );
	}

	const SItemAnimAction* FindAnimAction( const CName& actionName, Bool forPlayer ) const;

#ifndef NO_DATA_ASSERTS
	String m_debugXMLFile;
#endif
};

//////////////////////////////////////////////////////////////////////////

struct SItemExtDefinition
{
	struct ExtOverridableProperties
	{
		TDynArray< SItemDefinition::SItemVariant >	m_variants;				//!< templates that should be loaded when item of given category is mounted
		SItemDefinition::SCollapseCond				m_collapseCond;			//!< Condition to (un)collapse this item
	};

	ExtOverridableProperties	m_extDefaultProperties;
	ExtOverridableProperties*	m_extPlayerOverridableProperties;

	Red::System::GUID			m_creatorTag; //! tag is used for determination of definition creator (e.g. DLC mounter)

#ifndef NO_DATA_ASSERTS
	String						m_debugXMLFile;
#endif

	SItemDefinition*			m_item;

public:
	SItemExtDefinition();
	SItemExtDefinition( const SItemExtDefinition& itemExtDefinition );
	~SItemExtDefinition();

	void AttachToItem( SItemDefinition* item );
	Bool DettachFromItem( SItemDefinition* item );	
};

//////////////////////////////////////////////////////////////////////////
// always add new enums here at the end

enum EUsableItemType
{
	UI_Torch,
	UI_Horn,
	UI_Bell,
	UI_OilLamp,
	UI_Mask,
	UI_FiendLure,
	UI_Meteor,
	UI_None,
	UI_Censer,
	UI_Apple,
	UI_Cookie,
	UI_Basket,
};

BEGIN_ENUM_RTTI( EUsableItemType );
	ENUM_OPTION( UI_Torch );
	ENUM_OPTION( UI_Horn );
	ENUM_OPTION( UI_Bell );
	ENUM_OPTION( UI_OilLamp );
	ENUM_OPTION( UI_Mask );
	ENUM_OPTION( UI_FiendLure );
	ENUM_OPTION( UI_Meteor );
	ENUM_OPTION( UI_None );
	ENUM_OPTION( UI_Censer );
	ENUM_OPTION( UI_Apple );
	ENUM_OPTION( UI_Cookie );
	ENUM_OPTION( UI_Basket );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SUsableItem
{

public:
	Red::System::GUID	m_creatorTag; 
	EUsableItemType		m_type;

public:
	//! Parse and set usable item type
	void ParseUsableItemType( const String& usableItemType );
};

//////////////////////////////////////////////////////////////////////////

struct SCustomNodeAttribute
{
	CName	m_attributeValueAsCName;
	CName	m_attributeName;
	String	m_attributeValue;
public:
	DECLARE_RTTI_STRUCT( SCustomNodeAttribute );

	SCustomNodeAttribute() {}

	// Contructor used when attribute value, is not a CName but can be String, Bool, Float or Int32
	SCustomNodeAttribute( const CName& name, const String& value )
		: m_attributeName( name )
		, m_attributeValue( value )
	{
	}
	// Contructor used when attribute value is a CName. CName value can't be conversed to String, Bool, Float or Int32 using GetValue. Attribute should be parsed as a CName if attribute name ends with "_name"
	SCustomNodeAttribute( const CName& name, const CName& value )
		: m_attributeName( name )
		, m_attributeValueAsCName( value )
	{
	}

	Bool GetValueAsInt(Int32 &val) const;
	Bool GetValueAsFloat(Float &val) const;
	Bool GetValueAsBool(Bool &val) const;
	RED_INLINE const String& GetValueAsString() const { return m_attributeValue; }
	RED_INLINE const CName& GetValueAsCName() const { return m_attributeValueAsCName; }
	RED_INLINE const CName& GetAttributeName() const { return m_attributeName; }

private:
};

BEGIN_CLASS_RTTI( SCustomNodeAttribute );
	PROPERTY( m_attributeName );
	PROPERTY( m_attributeValue );
	PROPERTY( m_attributeValueAsCName );
END_CLASS_RTTI();

struct SCustomNode
{
	DECLARE_RTTI_STRUCT( SCustomNode );

	CName								m_nodeName;
	TDynArray< SCustomNodeAttribute >	m_attributes;
	TDynArray< CName >					m_values;
	TDynArray< SCustomNode >			m_subNodes;

	Red::System::GUID					m_creatorTag; //! tag is used for determination of definition creator (e.g. DLC mounter)
};

BEGIN_CLASS_RTTI( SCustomNode );
	PROPERTY( m_nodeName );
	PROPERTY( m_attributes );
	PROPERTY( m_values );
	PROPERTY( m_subNodes );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CDefinitionsManagerListener;
class CDefinitionsManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

public:
	typedef THashMap< CName, SAbility >						TAbilityDefMap;
	typedef THashMap< CName, SItemDefinition >				TItemDefMap;
	typedef THashMap< CName, TDynArray<SItemExtDefinition*>* > TItemExtDefMap;
	//typedef THashMap< CName, SItemSchematic >				TItemSchematicMap;
	typedef THashMap< CName, SIngredientCategory>			TIngredientCategory;
	typedef THashMap< CName, SItemSet>						TItemSetsMap;
	typedef THashMap< CName, TDynArray< CName > >			TCategorizedItemsMap;
	typedef THashMap< CName, CName >						TAttributeTypeMap;
	typedef THashMap< CName, SItemStat >					TItemStatMap;
	typedef THashMap< CName, SItemDamageCurve >				TItemDamageCurveMap;
	typedef THashMap< CName, SItemTagModifier >			TItemTagModifierMap;

private:
    TIngredientCategory							m_ingredientsCategoriesMap; //!< Ingredients Definitions
	//TItemSchematicMap							m_itemSchematicMap;		//!< Item Schematics
    TItemSetsMap								m_itemSetsMap;          //!< Item Sets
	TAbilityDefMap								m_abilityDefinitions;	//!< Ability definitions
	TItemDefMap									m_itemDefinitions;		//!< Item definitions
	TItemExtDefMap								m_itemExtDefinitions;	//!< Item extensions definitions
	THashMap< String, String >					m_templateNameMap;		//!< Maps template names (no extension) to depot paths
	TDynArray< CName >							m_itemCategories;		//!< Collection of all categories from parsed items
	TCategorizedItemsMap						m_itemsByCategory;		//!< Once again, item definitions but grouped by category
#ifndef RED_FINAL_BUILD
	THashMap< String, TDynArray<CName> >		m_itemsByDepot;				//!< item definitions grouped by depot according to the path name, e.g. 'ep1', 'bob', etc., or 'W3' if vanilla
	THashMap< String, TDynArray<CName> >		m_itemsByDepotAndCategory;  //!< item definitions grouped by concatenated depot+category e.g. 'EP1sword', 'W3crafting_schematic', 'bobarmor'
#endif //RED_FINAL_BUILD
	TDynArray< SCustomNode >					m_customNodes;
	mutable CLootDefinitions*					m_lootDefinitions;
	THashMap< CName, SUsableItem >				m_usableItemTypes;		//!< Types of animation assigned to usable items
	TDynArray< CDefinitionsManagerListener * >	m_listerners;

	static THashMap< CName, EActorAnimState >	m_actorAnimStates;	
	THashMap< String, TDynArray< String > >		m_allowedNodes;
	THashMap< String, TDynArray< String > >		m_allowdAttributes;

	TItemStatMap								m_itemStatMap;			//!< Item Stats
	TItemDamageCurveMap							m_itemDamageCurveMap;	//!< Category-based Damage Curve data
	TItemTagModifierMap							m_itemTagModifierMap;	//!< Merchant price modifications based on Item Tag

	Red::System::GUID							m_creatorTag;
	Bool										m_useAlternativePaths;

	static const String					ITEM_DEFINITIONS;
	static const String					ITEM_DAMAGECURVES;
	static const String					ITEM_TAGMODIFIERS;

	static const String					ABILITIES_DIR;			//!< Abilities directory
	static const String					ITEMS_DEFINITIONS_DIR;	//!< Item XMLs directory
	static const String					ABILITIES_ALT_DIR;		//!< Abilities alternative directory
	static const String					ITEMS_ALT_DIR;			//!< Item XMLs alternative directory
	static const String					ITEMS_TEMPLATES_DIR;	//!< Item en(.)(.) directory
	static const String					EXTENSION;				//!< Entity file extension

	static const String					NODE_ROOT;
	static const String					NODE_DEFINITIONS;
	static const String					NODE_CUSTOM;
	static const String					NODE_LOOT_DEFINITIONS;
	static const String					NODE_ABILITIES;
	static const String					NODE_ABILITY;
	static const String					NODE_ITEMS;
    static const String					NODE_ITEM;
	static const String					NODE_ITEMS_EXTENSIONS;
	static const String					NODE_ITEM_EXTENSION;
	static const String					NODE_ITEM_CATEGORY;
	static const String					NODE_PLAYER_OVERRIDE;
	static const String					NODE_PLAYER_OVERRIDE_EXTENSION;
    static const String					NODE_ITEM_SET;
    static const String					NODE_ITEM_SETS;
    static const String					NODE_ITEM_ELEMENT;
	static const String					NODE_TAGS;
	static const String					NODE_BASE_ABILITIES;
	static const String					NODE_SLOT_ITEMS;	
	static const String					NODE_BOUND_ITEMS;
	static const String					NODE_VARIANTS;
	static const String					NODE_VARIANT;
	static const String					NODE_COLLAPSE;
	static const String					NODE_ITEM_COND;
	static const String					NODE_CATEGORY_COND;
	static const String					NODE_A;
	static const String					NODE_ANIM_SWITCHES;
	static const String					NODE_ANIM_SWITCH;
	static const String					NODE_ANIM_ACTIONS;
	static const String					NODE_ACTION;
    static const String					NODE_RECYCLING_PARTS;
    static const String					NODE_PARTS;
    static const String					NODE_PARTS_SET;
    static const String					NODE_PART;
    static const String					NODE_SCHEMATIC;

    static const String					ATTR_NAME;
    static const String					ATTR_ICON;
	static const String					ATTR_TIMEOUT;
	static const String					ATTR_ON_ADDED_EVENT;
	static const String					ATTR_ON_REMOVED_EVENT;
	static const String					ATTR_PREREQUISITES;
	static const String					ATTR_MULT;
	static const String					ATTR_DISPLAY_PERC;
	static const String					ATTR_ALWAYS_RANDOM;
	static const String					ATTR_STACKABLE;
	static const String					ATTR_PRICE;
	static const String					ATTR_WEIGHT;
	static const String					ATTR_UPGRADE_LIST_NAME;
	static const String					ATTR_IS_ABILITY;
	static const String					ATTR_ICON_PATH;
	static const String					ATTR_LOCALIZATION_KEY_NAME;
	static const String					ATTR_LOCALIZATION_KEY_DESCCRIPTION;
	static const String					ATTR_APPEARANCE;
	static const String					ATTR_MIN;
	static const String					ATTR_MAX;
	static const String					ATTR_PRECISION;
	static const String					ATTR_CHANCE;
	static const String					ATTR_ENHANCEMENT_SLOTS;
	static const String					ATTR_TEMPLATE_EQUIP;
	static const String					ATTR_UPGRADE_BASED_TEMPLATE;
	static const String					ATTR_TEMPLATE_HOLD;
	static const String					ATTR_EQUIP_SLOT;
	static const String					ATTR_HOLD_SLOT;
	static const String					ATTR_GRID_SIZE;
	static const String					ATTR_WEAPON;
	static const String					ATTR_CATEGORY;
	static const String					ATTR_COLLAPSE;
	static const String					ATTR_ABILITIES_APPLY;
	static const String					ATTR_INGREDIENT_QUANTITY;
	static const String					ATTR_INGREDIENT_CAT_SPECIFIED;
	static const String					ATTR_COUNT;
	static const String					ATTR_TYPE;
	static const String					ATTR_LEVEL;
	
	static const String					ATTR_ELEMENT_PRIORITY;
	static const String					ATTR_CRAFTED_ITEM;
	static const String					ATTR_COLOR_VARIANT;
	static const String					ATTR_ACTOR_ANIM_STATE;
	static const String					ATTR_EVENT;
	static const String					ATTR_SWITCH_EQUIP_SLOT;
	static const String					ATTR_SWITCH_ACT;
	static const String					ATTR_SWITCH_DEACT;
	static const String					ATTR_ACT;
	static const String					ATTR_DEACT;
	static const String					ATTR_INIT_DURABILITY;
    static const String					ATTR_DURABILITY;
    static const String					ATTR_CRAFTSMAN_TYPE;
	static const String					ATTR_CRAFTSMAN_LEVEL;
    static const String					ATTR_RARITY;
	static const String					ATTR_ALL;

	static const String					NODE_USABLE_ITEM_TYPES;
	static const String					NODE_USABLE_ITEM;

	// Alchemy
	//--------------------------------------------------------------------------------------------------
	static const String NODE_ALCHEMY_SYSTEM;
	static const String ATTR_AMMO_DECREASE_TIME;
	static const String ATTR_VALUE;
	static const String NODE_BASE_BONUSES;
	static const String NODE_BASE_BONUS;
	static const String NODE_SUBSTANCE_MAX_LEVELS;
	static const String NODE_SUBSTANCE_MAX;
	static const String NODE_TOXICITY_BONUSES;
	static const String NODE_TOXICITY_THRESHOLD;

	static const String ATTR_BUFF_EFFECT_NAME;

	//--------------------------------------------------------------------------------------------------


public:
	CDefinitionsManager();
	~CDefinitionsManager();

	//! Reload all data
	void ReloadAll();

	//! Load template filenames
	void LoadTemplateNames();

	//! Recursively fill m_templateNameMap
	void LoadTemplateNamesRecursive( CDirectory * dir );

	//! Recursively remove form m_templateNameMap
	void RemoveTemplateNamesRecursive( CDirectory * dir );

	//! Load item definitions
	void LoadDefinitions();	

	//! Load item definitions
	void LoadDefinitions( const String& xmlFilePath, const Red::System::GUID& creatorTag );	

	void RemoveDefinitions( const Red::System::GUID& creatorTag );	

	void HashInventoryData();

	//! Get item definition
	const SItemDefinition* GetItemDefinition( CName itemName ) const;

	//! Get ability definition
	const SAbility*	GetAbilityDefinition( CName abilityName ) const;

	//! Get item set definition
	const SItemSet* GetItemSetDefinition( CName itemSetName ) const;

	//! Get item stat definition
	const SItemStat* GetItemStatDefinition( CName itemName ) const;

	//! Get price curves for given category.
	const SItemDamageCurve* GetItemDamageCurve( CName categoryName ) const;

	//! Get tag modifier based on given item tag.
	const SItemTagModifier* GetItemTagModifier( CName itemTag ) const;

	//! Get ingriedient definition
	const SIngredientCategory* GetIngredientDefinition( CName igredientName ) const;

    void GetIngredientCategories( TDynArray< CName >& ingredientCategories ) const;

	void TestWitchcraft() const;

	//Add all items, narrowed down by depot (e.g. vanilla, EP1), category (e.g. 'steelsword') or both
	void AddAllItems( CName category , String depot , Bool invisibleItems ) const;

	//! Get loot definitions
	CLootDefinitions* GetLootDefinitions() const;
	void ValidateLootDefinitions( bool listAllItemDefs ) const;

	void GetItemRecyclingParts( CName itemName, TDynArray< SItemParts >& resultArr ) const;
	void ValidateRecyclingParts( bool listAllItemDefs ) const;

	void ValidateCraftingDefinitions( bool listAllItemDefs ) const;

	//! Translate template name to depot path
	const String& TranslateTemplateName( const String& shortName ) const;

	//! Get item list
	void GetItemList( TDynArray< CName >& items ) const;

	//! Get abilities list
	void GetAbilitiesList( TDynArray< CName >& abilities ) const;

	//! Get unique contained abilities
	void GetUniqueContainedAbilities( const TDynArray< CName > & abilities, TDynArray< CName > & containedAbilities ) const;

	//! Get ability tags
	void GetAbilityTags( CName abilityName, TDynArray< CName > & tags ) const;

	//! Does ability have a tag
	Bool AbilityHasTag( CName abilityName, CName tag ) const;

	//! Does ability have any/all of tag
	Bool AbilityHasTags( CName abilityName, const TDynArray< CName > & tags, Bool all ) const;

	//! Get all existing item categories
	void GetItemCategories( TDynArray< CName >& categories ) const;

	//! Check whether category of given name exists
	Bool CategoryExists( CName category ) const;

	//! Get all items fitting given category
	const TDynArray< CName >& GetItemsOfCategory( CName category ) const;

	const TDynArray< CName >& GetItemsOfDepot( String depot ) const;
	const TDynArray< CName >& GetItemsOfDepotAndCategory( String depotCategory ) const;

	//! Get schematic item definition for given ingredients. This code won't work properly if you pass ingredient with 0 quantity, so please do not do it :)
	const SItemDefinition* GetSchematicForIngredients( const THashMap< CName, Uint32 >& ingredients ) const;

	//! Returns weight value from the named item's definition.
	Float GetItemWeight( CName itemName );

	//! Get template names list
	void GetTemplateFilesList( TDynArray< String >& templates ) const;

	//! Get items directory
	static const String &GetItemsDefinitionsDir() { return ITEMS_DEFINITIONS_DIR; };

	//! Get items directory
	static const String &GetItemsTemplatesDir() { return ITEMS_TEMPLATES_DIR; };

	//! Get abilities directory
	static const String &GetAbilitiesDir() { return ABILITIES_DIR; };

	//! Get all attribute modifiers coming from given ability set
	Uint32 CalculateAttributeModifiers( THashMap< CName, SAbilityAttributeValue >& outModifiers, const TDynArray< CName >& abilities, Int32 staticRandomSeed ) const;

	//! Get all modifiers names from given ability
	void GetAttributeModifiersNames( CName ability, TDynArray< CName >& modifiers ) const;

	//! Get all attribute modifiers coming from given ability
	Uint32 CalculateAttributeModifiers( THashMap< CName, SAbilityAttributeValue >& outModifiers, CName ability, Int32 staticRandomSeed ) const;

	//! Calculate attribute value from given abilities
	SAbilityAttributeValue CalculateAttributeValue( const TDynArray< CName >& abilities, CName attrName, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const;
	SAbilityAttributeValue CalculateAttributeValue( CName ability, CName attrName, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const;

	//! Calculate attribute min/max value for given ability
	void GetAbilityAttributeValue( CName abilityName, CName attributeName, SAbilityAttributeValue& outMin, SAbilityAttributeValue& outMax ) const;

	//! Calculate attribute min/max value for given ability - optionally filter abilities by tags
	void GetAbilitiesAttributeValue( const TDynArray< CName > & abilitiesNames, CName attributeName, SAbilityAttributeValue& outMin, SAbilityAttributeValue& outMax, const TDynArray< CName > & tags ) const;

	//! Apply attribute modifier for given ability
	void ApplyAttributeModifierForAbility( SAbilityAttributeValue& outModifier, const SAbility& ability, const CName& attrName, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const;

	//! Fill actor anim states map
	static void FillActorAnimStatesMap( THashMap< CName, EActorAnimState >& aasMap );

	//! Get actor anim state enum value for item definition
	static EActorAnimState	MapNameToActorAnimState( CName aasName );

	//! Get abilities by tag
	Uint32 FilterAbilitiesByTags( TDynArray< CName >& outAbilities, const TDynArray< CName >& inAbilities, const TagList& tags, Bool withoutTags = false ) const;

	RED_INLINE const TDynArray< SCustomNode >& GetCustomNodes() const { return m_customNodes; }

	const SCustomNode* GetCustomNode( const CName& nodeName ) const;

	//! Adds a listener for events happening inside the definitions manager
	void AddListener( CDefinitionsManagerListener *const listener ){ m_listerners.PushBackUnique( listener ); }

	//! Removes a listener
	void RemoveListener( CDefinitionsManagerListener *const listener ){ m_listerners.Remove( listener ); }

	//! Get usable item type
	EUsableItemType GetUsableItemType( CName itemName ) const;

	const Red::System::GUID& GetDefaultCreatorTag() const { return m_creatorTag; }

	void SetAlternativePathsMode( Bool set );

protected:
	//! Loads overridable item properties
	void LoadItemOverrideProperties( CXMLFileReader& xmlReader, SItemDefinition::OverridableProperties& properties, const String& itemName );

	// Reads CName attr (if equals none it is changed to None)
	Bool ReadCName( CXMLFileReader& xmlReader, const String& name, CName& attr );

	//! Compute modifier value with given seeds
	Float ComputeAttributeValue( const SAbilityAttribute& attribute, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const;

	//! Report value parse error
	void ReportParseErrorValue( const String& nodeName, const String& context, const String& filepath );

	//! Report attribute parse error
	void ReportParseErrorAttr( const String& attrName, const String& nodeName, const String& context, const String& filepath );

	//! Load a custom node and all its sub-nodes
	Bool LoadCustomNode( CXMLFileReader& xmlReader, SCustomNode& node );

	//! Prepare hashmaps of attributes and nodes allowed for given nodes
	void PrepareAllowedElementsMap();
	void PrepareAllowedNodesMap();
	void PrepareAllowedAttributesMap();

	void SearchForDisallowedElements( const CXMLFileReader& reader, const String& nodeName, const String& filepath, Bool allowOverrideProperties = false, Bool isWeapon = false ) const;
	
	void ParseItemExtDefinition( CXMLFileReader& xmlReader, SItemExtDefinition::ExtOverridableProperties& extOverridableProperties, const String& itemName );

	void AttachItemExtToItem( SItemDefinition* itemDefinition, SItemExtDefinition* itemExtDefinition ) const;
	void AttachItemExtsToItem( const CName& itemName );	
};

//////////////////////////////////////////////////////////////////////////

class CDefinitionsManagerAccessor : public CObject
{
    DECLARE_ENGINE_CLASS( CDefinitionsManagerAccessor, CObject, 0 );

public:
    CDefinitionsManagerAccessor(){};

    void SetManager(const CDefinitionsManager* manager){m_manager = manager;};

private:
	Int32 FindAttributeIndex( SCustomNode& node, CName& attName );

private:
    void funcGetIngredientCategoryElements( CScriptStackFrame& stack, void* result );
	void funcGetItemAbilities( CScriptStackFrame& stack, void* result );
	void funcGetItemAbilitiesWithWeights( CScriptStackFrame& stack, void* result );
	void funcGetItemAttributesFromAbilities( CScriptStackFrame& stack, void* result );
	void funcGetItemCategory( CScriptStackFrame& stack, void* result );
	void funcApplyItemAbilityAttributeModifier( CScriptStackFrame& stack, void* result );
	void funcApplyAbilityAttributeModifier( CScriptStackFrame& stack, void* result );
	void funcItemHasTag( CScriptStackFrame& stack, void* result );
	void funcGetItemsWithTag( CScriptStackFrame& stack, void* result );
	void funcGetCustomDefinition( CScriptStackFrame& stack, void* result );
	void funcGetSubNodeByAttributeValueAsCName( CScriptStackFrame& stack, void* result );

	void funcGetAttributeValueAsInt( CScriptStackFrame& stack, void* result );
	void funcGetAttributeValueAsFloat( CScriptStackFrame& stack, void* result );
	void funcGetAttributeValueAsBool( CScriptStackFrame& stack, void* result );
	void funcGetAttributeValueAsString( CScriptStackFrame& stack, void* result );
	void funcGetAttributeValueAsCName( CScriptStackFrame& stack, void* result );
	void funcGetAttributeName( CScriptStackFrame& stack, void* result );

	void funcGetAbilityTags( CScriptStackFrame& stack, void* result );
	void funcGetAbilityAttributes( CScriptStackFrame& stack, void* result );
	void funcGetAbilityAttributeValue( CScriptStackFrame& stack, void* result );
	void funcGetAbilitiesAttributeValue( CScriptStackFrame& stack, void* result );
    void funcIsAbilityDefined( CScriptStackFrame& stack, void* result );
	void funcGetContainedAbilities( CScriptStackFrame& stack, void* result );
	void funcGetUniqueContainedAbilities( CScriptStackFrame& stack, void* result );
	void funcAbilityHasTag( CScriptStackFrame& stack, void* result );

	void funcGetItemHoldSlot( CScriptStackFrame& stack, void* result );
	void funcGetItemPrice( CScriptStackFrame& stack, void* result );
	void funcGetItemEnhancementSlotCount( CScriptStackFrame& stack, void* result );
	void funcGetItemUpgradeListName( CScriptStackFrame& stack, void* result );
	void funcGetItemLocalisationKeyName( CScriptStackFrame& stack, void* result );
	void funcGetItemLocalisationKeyDesc( CScriptStackFrame& stack, void* result );
	void funcGetItemIconPath( CScriptStackFrame& stack, void* result );
	void funcGetItemRecyclingParts( CScriptStackFrame& stack, void* result );
	void funcGetItemEquipTemplate( CScriptStackFrame& stack, void* result );
	void funcGetUsableItemType( CScriptStackFrame& stack, void* result );

	void funcGetCustomDefinitionSubNode( CScriptStackFrame& stack, void* result );
	void funcFindAttributeIndex( CScriptStackFrame& stack, void* result );
	void funcGetCustomNodeAttributeValueString( CScriptStackFrame& stack, void* result );
	void funcGetCustomNodeAttributeValueName( CScriptStackFrame& stack, void* result );
	void funcGetCustomNodeAttributeValueInt( CScriptStackFrame& stack, void* result );
	void funcGetCustomNodeAttributeValueBool( CScriptStackFrame& stack, void* result );
	void funcGetCustomNodeAttributeValueFloat( CScriptStackFrame& stack, void* result );

	void funcTestWitchcraft( CScriptStackFrame& stack, void* result );
	void funcValidateLootDefinitions( CScriptStackFrame& stack, void* result );
	void funcValidateRecyclingParts( CScriptStackFrame& stack, void* result );
	void funcValidateCraftingDefinitions( CScriptStackFrame& stack, void* result );

	void funcAddAllItems( CScriptStackFrame& stack, void* result );

    const CDefinitionsManager*    m_manager;
};

BEGIN_CLASS_RTTI( CDefinitionsManagerAccessor );
PARENT_CLASS( CObject )
    NATIVE_FUNCTION( "GetIngredientCategoryElements", funcGetIngredientCategoryElements );
	NATIVE_FUNCTION( "GetItemAbilities", funcGetItemAbilities );
	NATIVE_FUNCTION( "GetItemAbilitiesWithWeights", funcGetItemAbilitiesWithWeights );
	NATIVE_FUNCTION( "GetItemAttributesFromAbilities", funcGetItemAttributesFromAbilities );
	NATIVE_FUNCTION( "GetItemCategory", funcGetItemCategory );
	NATIVE_FUNCTION( "ItemHasTag", funcItemHasTag );
	NATIVE_FUNCTION( "GetItemsWithTag", funcGetItemsWithTag );
	NATIVE_FUNCTION( "GetItemEquipTemplate", funcGetItemEquipTemplate );
	NATIVE_FUNCTION( "GetUsableItemType", funcGetUsableItemType );

	NATIVE_FUNCTION( "TestWitchcraft", funcTestWitchcraft );
	NATIVE_FUNCTION( "ValidateLootDefinitions", funcValidateLootDefinitions );
	NATIVE_FUNCTION( "ValidateRecyclingParts", funcValidateRecyclingParts );
	NATIVE_FUNCTION( "ValidateCraftingDefinitions", funcValidateCraftingDefinitions );

	NATIVE_FUNCTION( "AddAllItems", funcAddAllItems );

	NATIVE_FUNCTION( "GetItemRecyclingParts", funcGetItemRecyclingParts );

	NATIVE_FUNCTION( "ApplyItemAbilityAttributeModifier", funcApplyItemAbilityAttributeModifier );
	NATIVE_FUNCTION( "ApplyAbilityAttributeModifier", funcApplyAbilityAttributeModifier );
	NATIVE_FUNCTION( "GetCustomDefinition", funcGetCustomDefinition );
	NATIVE_FUNCTION( "GetSubNodeByAttributeValueAsCName", funcGetSubNodeByAttributeValueAsCName );

	NATIVE_FUNCTION( "GetItemHoldSlot", funcGetItemHoldSlot );
	NATIVE_FUNCTION( "GetItemPrice", funcGetItemPrice );
	NATIVE_FUNCTION( "GetItemEnhancementSlotCount", funcGetItemEnhancementSlotCount );
	NATIVE_FUNCTION( "GetItemUpgradeListName", funcGetItemUpgradeListName );
	NATIVE_FUNCTION( "GetItemLocalisationKeyName", funcGetItemLocalisationKeyName );
	NATIVE_FUNCTION( "GetItemLocalisationKeyDesc", funcGetItemLocalisationKeyDesc );
	NATIVE_FUNCTION( "GetItemIconPath", funcGetItemIconPath );

	NATIVE_FUNCTION( "GetAttributeValueAsInt", funcGetAttributeValueAsInt );
	NATIVE_FUNCTION( "GetAttributeValueAsFloat", funcGetAttributeValueAsFloat );
	NATIVE_FUNCTION( "GetAttributeValueAsBool", funcGetAttributeValueAsBool );
	NATIVE_FUNCTION( "GetAttributeValueAsString", funcGetAttributeValueAsString );
	NATIVE_FUNCTION( "GetAttributeValueAsCName", funcGetAttributeValueAsCName );
	NATIVE_FUNCTION( "GetAttributeName", funcGetAttributeName );

	NATIVE_FUNCTION( "GetAbilityTags", funcGetAbilityTags );
	NATIVE_FUNCTION( "GetAbilityAttributes", funcGetAbilityAttributes );
	NATIVE_FUNCTION( "GetAbilityAttributeValue", funcGetAbilityAttributeValue );
	NATIVE_FUNCTION( "GetAbilitiesAttributeValue", funcGetAbilitiesAttributeValue );
	NATIVE_FUNCTION( "IsAbilityDefined", funcIsAbilityDefined );
	NATIVE_FUNCTION( "GetContainedAbilities", funcGetContainedAbilities );
	NATIVE_FUNCTION( "GetUniqueContainedAbilities", funcGetUniqueContainedAbilities );
	NATIVE_FUNCTION( "AbilityHasTag", funcAbilityHasTag );

	NATIVE_FUNCTION( "GetCustomDefinitionSubNode", funcGetCustomDefinitionSubNode );
	NATIVE_FUNCTION( "FindAttributeIndex" ,funcFindAttributeIndex );
	NATIVE_FUNCTION( "GetCustomNodeAttributeValueString", funcGetCustomNodeAttributeValueString );
	NATIVE_FUNCTION( "GetCustomNodeAttributeValueName", funcGetCustomNodeAttributeValueName );
	NATIVE_FUNCTION( "GetCustomNodeAttributeValueInt", funcGetCustomNodeAttributeValueInt );
	NATIVE_FUNCTION( "GetCustomNodeAttributeValueBool", funcGetCustomNodeAttributeValueBool );
	NATIVE_FUNCTION( "GetCustomNodeAttributeValueFloat", funcGetCustomNodeAttributeValueFloat );
END_CLASS_RTTI();
