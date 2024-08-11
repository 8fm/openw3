/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "definitionsManager.h"
#include <limits>
#include "abilities.h"
#include "definitionsHelpers.h"
#include "lootDefinitions.h"
#include "definitionsManagerListener.h"

#include "../core/depot.h"
#include "../core/xmlFileReader.h"
#include "../core/dataError.h"
#include "../engine/localizationManager.h"
#include "../../common/core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CDefinitionsManagerAccessor );
IMPLEMENT_ENGINE_CLASS( SItemSet );
IMPLEMENT_ENGINE_CLASS( SItemStat );
IMPLEMENT_ENGINE_CLASS( SItemDamageCurve );
IMPLEMENT_ENGINE_CLASS( SItemTagModifier );
IMPLEMENT_ENGINE_CLASS( SIngredientCategoryElement );
IMPLEMENT_ENGINE_CLASS( SIngredientCategory );
IMPLEMENT_ENGINE_CLASS( SCustomNodeAttribute );
IMPLEMENT_ENGINE_CLASS( SCustomNode );

IMPLEMENT_RTTI_ENUM( EInventoryItemClass );
IMPLEMENT_RTTI_ENUM( EUsableItemType );

namespace
{
	static SCustomNode NullCustomNode;
};


const String CDefinitionsManager::EXTENSION				= TXT( "w2ent" );
const String CDefinitionsManager::NODE_ROOT				= TXT( "redxml" );
const String CDefinitionsManager::NODE_DEFINITIONS		= TXT( "definitions" );
const String CDefinitionsManager::NODE_CUSTOM			= TXT( "custom" );

const String CDefinitionsManager::ITEMS_TEMPLATES_DIR	= TXT( "items\\" );
const String CDefinitionsManager::ITEMS_DEFINITIONS_DIR	= TXT( "gameplay\\items\\" );
const String CDefinitionsManager::ABILITIES_DIR			= TXT( "gameplay\\abilities\\" );
const String CDefinitionsManager::ITEMS_ALT_DIR			= TXT( "gameplay\\items_plus\\" );
const String CDefinitionsManager::ABILITIES_ALT_DIR		= TXT( "gameplay\\abilities_plus\\" );

const String CDefinitionsManager::ITEM_DEFINITIONS		= TXT( "gameplay\\globals\\tooltip_settings.csv" );
const String CDefinitionsManager::ITEM_DAMAGECURVES		= TXT( "gameplay\\globals\\economy_damagecurves.csv" );
const String CDefinitionsManager::ITEM_TAGMODIFIERS		= TXT( "gameplay\\globals\\economy_tagmodifiers.csv" );

// Alchemy
//---------------------

const String CDefinitionsManager::NODE_ALCHEMY_SYSTEM			= TXT( "alchemy_system_properties" );
const String CDefinitionsManager::ATTR_AMMO_DECREASE_TIME		= TXT( "ammoDecreaseTime" );
const String CDefinitionsManager::ATTR_VALUE					= TXT( "value" );
const String CDefinitionsManager::NODE_BASE_BONUSES				= TXT( "base_bonuses" );
const String CDefinitionsManager::NODE_BASE_BONUS				= TXT( "base_bonus" );
const String CDefinitionsManager::NODE_SUBSTANCE_MAX_LEVELS		= TXT( "substance_max_levels" );
const String CDefinitionsManager::NODE_SUBSTANCE_MAX			= TXT( "substance_max" );
const String CDefinitionsManager::NODE_TOXICITY_BONUSES			= TXT( "toxicity_bonuses" );
const String CDefinitionsManager::NODE_TOXICITY_THRESHOLD		= TXT( "toxicityThreshold" );

//---------------------

const String CDefinitionsManager::NODE_LOOT_DEFINITIONS = TXT( "loot_definitions" );
const String CDefinitionsManager::NODE_ABILITIES		= TXT( "abilities" );
const String CDefinitionsManager::NODE_ABILITY			= TXT( "ability" );
const String CDefinitionsManager::NODE_TAGS				= TXT( "tags" );
const String CDefinitionsManager::NODE_ITEMS			= TXT( "items" );
const String CDefinitionsManager::NODE_ITEM				= TXT( "item" );
const String CDefinitionsManager::NODE_ITEMS_EXTENSIONS	= TXT( "items_extensions" );
const String CDefinitionsManager::NODE_ITEM_EXTENSION	= TXT( "item_extension" );
const String CDefinitionsManager::NODE_ITEM_CATEGORY	= TXT( "item_category" );
const String CDefinitionsManager::NODE_PLAYER_OVERRIDE	= TXT( "player_override" );
const String CDefinitionsManager::NODE_PLAYER_OVERRIDE_EXTENSION	= TXT( "player_override_extension" );
const String CDefinitionsManager::NODE_BASE_ABILITIES	= TXT( "base_abilities" );
const String CDefinitionsManager::NODE_SLOT_ITEMS		= TXT( "slot_items" );
const String CDefinitionsManager::NODE_BOUND_ITEMS		= TXT( "bound_items" );
const String CDefinitionsManager::NODE_VARIANTS			= TXT( "variants" );
const String CDefinitionsManager::NODE_VARIANT			= TXT( "variant" );
const String CDefinitionsManager::NODE_COLLAPSE			= TXT( "collapse" );
const String CDefinitionsManager::NODE_ITEM_COND		= TXT( "item_cond" );
const String CDefinitionsManager::NODE_CATEGORY_COND	= TXT( "category_cond" );
const String CDefinitionsManager::NODE_A				= TXT( "a" );
const String CDefinitionsManager::NODE_ANIM_SWITCHES	= TXT( "anim_switches" );
const String CDefinitionsManager::NODE_ANIM_SWITCH		= TXT( "anim_switch" );
const String CDefinitionsManager::NODE_ANIM_ACTIONS		= TXT( "anim_actions" );
const String CDefinitionsManager::NODE_ACTION			= TXT( "action" );
const String CDefinitionsManager::NODE_RECYCLING_PARTS	= TXT( "recycling_parts" );
const String CDefinitionsManager::NODE_PARTS	        = TXT( "parts" );
const String CDefinitionsManager::NODE_PARTS_SET	    = TXT( "parts_set" );
const String CDefinitionsManager::NODE_PART	            = TXT( "part" );
const String CDefinitionsManager::NODE_ITEM_SET     	= TXT( "set" );
const String CDefinitionsManager::NODE_ITEM_ELEMENT 	= TXT( "element" );
const String CDefinitionsManager::NODE_SCHEMATIC        = TXT( "schematic" );
const String CDefinitionsManager::NODE_ITEM_SETS        = TXT( "item_sets" );

const String CDefinitionsManager::ATTR_NAME				= TXT( "name" );
const String CDefinitionsManager::ATTR_ICON			    = TXT( "icon" );
const String CDefinitionsManager::ATTR_TIMEOUT			= TXT( "timeout" );
const String CDefinitionsManager::ATTR_ON_ADDED_EVENT	= TXT( "on_added" );
const String CDefinitionsManager::ATTR_ON_REMOVED_EVENT	= TXT( "on_removed" );
const String CDefinitionsManager::ATTR_PREREQUISITES	= TXT( "prerequisites" );
const String CDefinitionsManager::ATTR_MULT				= TXT( "type" );
const String CDefinitionsManager::ATTR_DISPLAY_PERC		= TXT( "display_perc");
const String CDefinitionsManager::ATTR_ALWAYS_RANDOM	= TXT( "always_random" );
const String CDefinitionsManager::ATTR_TEMPLATE_EQUIP	= TXT( "equip_template" );
const String CDefinitionsManager::ATTR_UPGRADE_BASED_TEMPLATE	= TXT( "upgrade_based_template" );
const String CDefinitionsManager::ATTR_TEMPLATE_HOLD	= TXT( "hold_template" );
const String CDefinitionsManager::ATTR_APPEARANCE		= TXT( "appearance" );
const String CDefinitionsManager::ATTR_COLOR_VARIANT	= TXT( "color_variant" );
const String CDefinitionsManager::ATTR_STACKABLE		= TXT( "stackable" );
const String CDefinitionsManager::ATTR_PRICE			= TXT( "price" );
const String CDefinitionsManager::ATTR_WEIGHT			= TXT( "weight" );
const String CDefinitionsManager::ATTR_UPGRADE_LIST_NAME= TXT( "upgrade_list_name" );
const String CDefinitionsManager::ATTR_IS_ABILITY		= TXT( "is_ability" );

const String CDefinitionsManager::ATTR_ICON_PATH		= TXT( "icon_path" );
const String CDefinitionsManager::ATTR_LOCALIZATION_KEY_NAME            = TXT( "localisation_key_name" );
const String CDefinitionsManager::ATTR_LOCALIZATION_KEY_DESCCRIPTION    = TXT( "localisation_key_description" );

const String CDefinitionsManager::ATTR_MIN				= TXT( "min" );
const String CDefinitionsManager::ATTR_MAX				= TXT( "max" );
const String CDefinitionsManager::ATTR_PRECISION		= TXT( "precision" );
const String CDefinitionsManager::ATTR_CHANCE			= TXT( "chance" );
const String CDefinitionsManager::ATTR_ENHANCEMENT_SLOTS= TXT( "enhancement_slots" );
const String CDefinitionsManager::ATTR_EQUIP_SLOT		= TXT( "equip_slot" );
const String CDefinitionsManager::ATTR_HOLD_SLOT		= TXT( "hold_slot" );
const String CDefinitionsManager::ATTR_GRID_SIZE		= TXT( "grid_size" );
const String CDefinitionsManager::ATTR_WEAPON			= TXT( "weapon" );
const String CDefinitionsManager::ATTR_CATEGORY			= TXT( "category" );
const String CDefinitionsManager::ATTR_COLLAPSE			= TXT( "collapse" );
const String CDefinitionsManager::ATTR_ABILITIES_APPLY	= TXT( "ability_mode" );
const String CDefinitionsManager::ATTR_INGREDIENT_QUANTITY	= TXT( "quantity" );
const String CDefinitionsManager::ATTR_INGREDIENT_CAT_SPECIFIED = TXT( "specified" );
const String CDefinitionsManager::ATTR_COUNT            = TXT( "count" );
const String CDefinitionsManager::ATTR_ELEMENT_PRIORITY = TXT( "priority" );
const String CDefinitionsManager::ATTR_TYPE         	= TXT( "type" );
const String CDefinitionsManager::ATTR_LEVEL         	= TXT( "level" );

const String CDefinitionsManager::ATTR_CRAFTED_ITEM		= TXT( "crafted_item" );
const String CDefinitionsManager::ATTR_ACTOR_ANIM_STATE	= TXT( "anim_state" );
const String CDefinitionsManager::ATTR_EVENT			= TXT( "event" );
const String CDefinitionsManager::ATTR_SWITCH_EQUIP_SLOT= TXT( "equip_slot" );
const String CDefinitionsManager::ATTR_SWITCH_ACT		= TXT( "switch_act");
const String CDefinitionsManager::ATTR_SWITCH_DEACT		= TXT( "switch_deact");
const String CDefinitionsManager::ATTR_ACT				= TXT( "act");
const String CDefinitionsManager::ATTR_DEACT			= TXT( "deact");
const String CDefinitionsManager::ATTR_INIT_DURABILITY	= TXT( "initial_durability" );
const String CDefinitionsManager::ATTR_DURABILITY		= TXT( "max_durability");
const String CDefinitionsManager::ATTR_CRAFTSMAN_TYPE	= TXT( "craftsman_type");
const String CDefinitionsManager::ATTR_CRAFTSMAN_LEVEL	= TXT( "craftsman_level");
const String CDefinitionsManager::ATTR_RARITY   		= TXT( "rarity");
const String CDefinitionsManager::ATTR_ALL   			= TXT( "all" );

const String CDefinitionsManager::NODE_USABLE_ITEM_TYPES= TXT( "usable_item_types" );
const String CDefinitionsManager::NODE_USABLE_ITEM		= TXT( "usable_item" );

THashMap< CName, EActorAnimState > CDefinitionsManager::m_actorAnimStates = THashMap< CName, EActorAnimState >();

const SItemDefinition::SItemAnimAction* SItemDefinition::FindAnimAction( const CName& actionName, Bool forPlayer ) const
{
	const TDynArray< SItemAnimAction >& animActions = GetAnimActions( forPlayer );
	for ( Uint32 i=0; i<animActions.Size(); ++i )
	{
		if ( animActions[ i ].m_name == actionName )
		{
			return &( animActions[ i ] );
		}
	}
	return NULL;
}

Bool SItemDefinition::GetBaseAbilities( TDynArray< const SItemAbility* > & baseAbilities, Bool player ) const
{
	const TDynArray< SItemAbility >& sourceBaseAbilities = GetBaseAbilities( player ); 
	TDynArray< SItemAbility >::const_iterator it = sourceBaseAbilities.Begin();
	TDynArray< SItemAbility >::const_iterator itEnd = sourceBaseAbilities.End();
	for ( ; it != itEnd; ++it )
	{
		baseAbilities.PushBack( &( *it ) );
	}
	return baseAbilities.Size() > 0;
}

Bool SItemDefinition::HasAbility( const CName& abilityName, Bool player ) const
{
	const TDynArray< SItemAbility >& sourceBaseAbilities = GetBaseAbilities( player ); 
	TDynArray< SItemAbility >::const_iterator it = sourceBaseAbilities.Begin();
	TDynArray< SItemAbility >::const_iterator itEnd = sourceBaseAbilities.End();
	for ( ; it != itEnd; ++it )
	{
		if ( it->GetName() == abilityName )
		{
			return true;
		}
	}
	return false;
}

Uint32 SItemDefinition::ChooseAbilitiesCount( Bool playerItem, Uint32 randSeed ) const
{
	Int32 maxAbilities = GetMaxAbilities( playerItem );
	if ( maxAbilities == -1 )
	{
		return GetBaseAbilities( playerItem ).Size();
	}
	Int32 minAbilities = GetMinAbilities( playerItem );
	ASSERT( maxAbilities >= minAbilities );

	Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( randSeed );
	return noiseMaker.Get< Uint32 >( minAbilities, maxAbilities + 1 );
}

void SItemDefinition::ChooseAbilities( TDynArray< CName > & abilities, Bool playerItem, Uint32 randSeed ) const
{
	Uint32 count = ChooseAbilitiesCount( playerItem, randSeed );
	const TDynArray< SItemAbility >& sourceBaseAbilities = GetBaseAbilities( playerItem ); 
	if ( count >= sourceBaseAbilities.Size() )
	{
		CDefinitionsHelpers::CopyNames( sourceBaseAbilities, abilities );
	}
	else
	{
		TDynArray< const SItemAbility* > baseAbilities;
		GetBaseAbilities( baseAbilities, playerItem );
		CDefinitionsHelpers::DefaultDefinitionValidator validator;
		Uint32 chosenCount = CDefinitionsHelpers::ChooseDefinitions( baseAbilities, count, validator, randSeed );
		CDefinitionsHelpers::CopyNames( baseAbilities, abilities, chosenCount ); // Is this ok?
	}
}

void SItemDefinition::GetItemDescription( String& desc, Bool player ) const
{
	desc.Clear();
	if ( GetEquipSlot( player ) )
	{
		desc += String::Printf( TXT("Equip slot:\t\t %s \n"), GetEquipSlot( player ).AsString().AsChar() );
	}
	if ( GetHoldSlot( player ) )
	{
		desc += String::Printf( TXT("Hold slot:\t\t %s \n"), GetHoldSlot( player ).AsString().AsChar() );
	}
	if ( !GetEquipTemplateName( player ).Empty() )
	{
		desc += String::Printf( TXT("Equip template:\t\t %s \n"), GetEquipTemplateName( player ).AsChar() );
	}
	if ( !GetHoldSlot( player ).Empty() )
	{
		desc += String::Printf( TXT("Hold template:\t\t %s \n"), GetHoldSlot( player ).AsChar() );
	}

}

SItemDefinition::SItemDefinition( const SItemDefinition& itemDefinition )
{
	m_defaultProperties = itemDefinition.m_defaultProperties;
	m_playerOverridableProperties = itemDefinition.m_playerOverridableProperties ? new OverridableProperties( *itemDefinition.m_playerOverridableProperties ) : nullptr;

	m_category				= itemDefinition.m_category;

	m_price					= itemDefinition.m_price;
	m_weight				= itemDefinition.m_weight;
	m_itemTags				= itemDefinition.m_itemTags;
	m_enhancementSlotCount	= itemDefinition.m_enhancementSlotCount;
	m_flags					= itemDefinition.m_flags;
	m_ingredients			= itemDefinition.m_ingredients;

	m_craftedItemName		= itemDefinition.m_craftedItemName;
	m_craftsmanType			= itemDefinition.m_craftsmanType;
	m_craftsmanLevel		= itemDefinition.m_craftsmanLevel;

	m_recyclingParts		= itemDefinition.m_recyclingParts;

	m_gridSize				= itemDefinition.m_gridSize;

	m_setName				= itemDefinition.m_setName;

	m_iconPath				= itemDefinition.m_iconPath;
	m_localizationKeyName	= itemDefinition.m_localizationKeyName;
	m_localizationKeyDesc	= itemDefinition.m_localizationKeyDesc;
	m_stackSize				= itemDefinition.m_stackSize;

	auto attachedCopy = itemDefinition.m_attachedItemExtDefinition; // do the copy (the loop modifies m_attachedItemExtDefinition)
	for ( auto itemExtDefinitionPtr : attachedCopy )
	{
		itemExtDefinitionPtr->AttachToItem( this );
	}

	m_creatorTag			= itemDefinition.m_creatorTag;

#ifndef NO_DATA_ASSERTS
	m_debugXMLFile			= itemDefinition.m_debugXMLFile;
#endif
}

SItemDefinition::~SItemDefinition()
{
	auto attachedCopy = m_attachedItemExtDefinition; // do the copy (the loop modifies m_attachedItemExtDefinition)

	for ( auto itemExtDefinitionPtr : attachedCopy )
	{
		itemExtDefinitionPtr->DettachFromItem( this );
	}

	if( m_playerOverridableProperties )
	{
		delete m_playerOverridableProperties;
	}
}

//////////////////////////////////////////////////////////////
// CDefinitionsManager
CDefinitionsManager::CDefinitionsManager()
	: m_lootDefinitions( NULL )
	, m_useAlternativePaths( false )
{
	m_creatorTag = Red::System::GUID::Create();
	PrepareAllowedElementsMap();
	ReloadAll();
}

CDefinitionsManager::~CDefinitionsManager()
{
	for( auto& itemExts : m_itemExtDefinitions )
	{
		TDynArray<SItemExtDefinition*>* itemExtsArray = itemExts.m_second;
		for ( Int32 i = itemExtsArray->SizeInt() - 1; i >= 0; --i )
		{
			SItemExtDefinition* itemExtDefinition = (*itemExtsArray)[i];
			itemExtsArray->Erase( itemExtsArray->Begin() + i );
			delete itemExtDefinition;
		}
		delete itemExtsArray;
	}
	m_itemExtDefinitions.Clear();

	delete m_lootDefinitions;
}

void CDefinitionsManager::PrepareAllowedElementsMap()
{
	PrepareAllowedNodesMap();
	PrepareAllowedAttributesMap();
}

void CDefinitionsManager::PrepareAllowedNodesMap()
{
	TDynArray< String >& rootNodes = m_allowedNodes.GetRef( NODE_ROOT );
	rootNodes.Reserve( 2 );
	rootNodes.PushBack( NODE_DEFINITIONS );
	rootNodes.PushBack( NODE_CUSTOM );

	TDynArray< String >& defNodes = m_allowedNodes.GetRef( NODE_DEFINITIONS );
	defNodes.Reserve( 4 );
	defNodes.PushBack( NODE_LOOT_DEFINITIONS );
	defNodes.PushBack( NODE_ABILITIES );
	defNodes.PushBack( NODE_ITEMS );
	defNodes.PushBack( NODE_ITEMS_EXTENSIONS );
	defNodes.PushBack( NODE_ITEM_SETS );

	TDynArray< String >& abilitiesNodes = m_allowedNodes.GetRef( NODE_ABILITIES );
	abilitiesNodes.PushBack( NODE_ABILITY );

	TDynArray< String >& itemsNodes = m_allowedNodes.GetRef( NODE_ITEMS );
	itemsNodes.PushBack( NODE_ITEM );

	TDynArray< String >& itemsExtensionsNodes = m_allowedNodes.GetRef( NODE_ITEMS_EXTENSIONS );
	itemsExtensionsNodes.PushBack( NODE_ITEM_EXTENSION );

	TDynArray< String >& biNodes = m_allowedNodes.GetRef( NODE_BOUND_ITEMS );
	biNodes.PushBack( NODE_ITEM );

	TDynArray< String >& varNodes = m_allowedNodes.GetRef( NODE_VARIANT );
	varNodes.PushBack( NODE_ITEM );

	TDynArray< String >& isNodes = m_allowedNodes.GetRef( NODE_ITEM_SETS );
	isNodes.PushBack( NODE_ITEM_SET );

	TDynArray< String >& varsNodes = m_allowedNodes.GetRef( NODE_VARIANTS );
	varsNodes.PushBack( NODE_VARIANT );

	TDynArray< String >& collapseNodes = m_allowedNodes.GetRef( NODE_COLLAPSE );
	collapseNodes.PushBack( NODE_ITEM_COND );
	collapseNodes.PushBack( NODE_CATEGORY_COND );

	TDynArray< String >& baseAbilititesNodes = m_allowedNodes.GetRef( NODE_BASE_ABILITIES );
	baseAbilititesNodes.PushBack( NODE_A );

	TDynArray< String >& slotItemsNodes = m_allowedNodes.GetRef( NODE_SLOT_ITEMS );
	slotItemsNodes.PushBack( NODE_A );

	TDynArray< String >& animActionsNodes = m_allowedNodes.GetRef( NODE_ANIM_ACTIONS );
	animActionsNodes.PushBack( NODE_ACTION );

	TDynArray< String >& animSwitchesNodes = m_allowedNodes.GetRef( NODE_ANIM_SWITCHES );
	animSwitchesNodes.PushBack( NODE_ANIM_SWITCH );

	TDynArray< String >& itemNodes = m_allowedNodes.GetRef( NODE_ITEM );
	itemNodes.Reserve( 4 );
	itemNodes.PushBack( NODE_RECYCLING_PARTS );
	itemNodes.PushBack( NODE_ITEM_SET );
	itemNodes.PushBack( NODE_TAGS );
	itemNodes.PushBack( NODE_PLAYER_OVERRIDE );

	TDynArray< String >& recPartsNodes = m_allowedNodes.GetRef( NODE_RECYCLING_PARTS );
	recPartsNodes.PushBack( NODE_PARTS );

	TDynArray< String >& itemSetNodes = m_allowedNodes.GetRef( NODE_ITEM_SET );
	itemSetNodes.Reserve( 2 );
	itemSetNodes.PushBack( NODE_PARTS_SET );
	itemSetNodes.PushBack( NODE_BASE_ABILITIES );

	TDynArray< String >& partsSetNodes = m_allowedNodes.GetRef( NODE_PARTS_SET );
	partsSetNodes.PushBack( NODE_PART );

	TDynArray< String >& overridenNodes = m_allowedNodes.GetRef( TXT( "overriden" ) );
	overridenNodes.Reserve( 6 );
	overridenNodes.PushBack( NODE_BOUND_ITEMS );
	overridenNodes.PushBack( NODE_VARIANTS );
	overridenNodes.PushBack( NODE_COLLAPSE );
	overridenNodes.PushBack( NODE_BASE_ABILITIES );
	overridenNodes.PushBack( NODE_ANIM_ACTIONS );
	overridenNodes.PushBack( NODE_ANIM_SWITCHES );
	overridenNodes.PushBack( NODE_SLOT_ITEMS );
}

void CDefinitionsManager::PrepareAllowedAttributesMap()
{
	TDynArray< String >& abilityAttr = m_allowdAttributes.GetRef( NODE_ABILITY );
	abilityAttr.Reserve( 5 );
	abilityAttr.PushBack( ATTR_NAME );
	abilityAttr.PushBack( ATTR_ON_ADDED_EVENT );
	abilityAttr.PushBack( ATTR_ON_REMOVED_EVENT );
	abilityAttr.PushBack( ATTR_PREREQUISITES );
	abilityAttr.PushBack( ATTR_TYPE );
	
	TDynArray< String >& notAbilityAttr = m_allowdAttributes.GetRef( NODE_ABILITY + TXT("_and_not_") + ATTR_IS_ABILITY );
	notAbilityAttr.Reserve( 5 );
	notAbilityAttr.PushBack( ATTR_MULT );
	notAbilityAttr.PushBack( ATTR_DISPLAY_PERC );
	notAbilityAttr.PushBack( ATTR_ALWAYS_RANDOM );
	notAbilityAttr.PushBack( ATTR_MIN );
	notAbilityAttr.PushBack( ATTR_MAX );
	
	TDynArray< String >& itemAttr = m_allowdAttributes.GetRef( NODE_ITEM );
	itemAttr.Reserve( 14 );
	itemAttr.PushBack( ATTR_NAME );
	itemAttr.PushBack( ATTR_CATEGORY );
	itemAttr.PushBack( ATTR_PRICE );
	itemAttr.PushBack( ATTR_WEIGHT );
	itemAttr.PushBack( ATTR_ENHANCEMENT_SLOTS );
	itemAttr.PushBack( ATTR_STACKABLE );
	itemAttr.PushBack( ATTR_ICON_PATH );
	itemAttr.PushBack( ATTR_LOCALIZATION_KEY_NAME );
	itemAttr.PushBack( ATTR_LOCALIZATION_KEY_DESCCRIPTION );
	itemAttr.PushBack( ATTR_CRAFTED_ITEM );
	itemAttr.PushBack( ATTR_CRAFTSMAN_TYPE );
	itemAttr.PushBack( ATTR_CRAFTSMAN_LEVEL );
	itemAttr.PushBack( ATTR_ABILITIES_APPLY );
	itemAttr.PushBack( ATTR_GRID_SIZE );
	itemAttr.PushBack( ATTR_WEAPON );
	
	TDynArray< String >& itemExtAttr = m_allowdAttributes.GetRef( NODE_ITEM_EXTENSION );
	itemExtAttr.Reserve( 1 );
	itemExtAttr.PushBack( ATTR_NAME );

	TDynArray< String >& varAttr = m_allowdAttributes.GetRef( NODE_VARIANT );
	varAttr.Reserve( 2 );
	varAttr.PushBack( ATTR_CATEGORY );
	varAttr.PushBack( ATTR_TEMPLATE_EQUIP );

	TDynArray< String >& ccondAttr = m_allowdAttributes.GetRef( NODE_ITEM_COND );
	ccondAttr.Reserve( 1 );
	ccondAttr.PushBack( ATTR_NAME );

	TDynArray< String >& icondAttr = m_allowdAttributes.GetRef( NODE_CATEGORY_COND );
	icondAttr.Reserve( 2 );
	icondAttr.PushBack( ATTR_NAME );
	icondAttr.PushBack( ATTR_COLLAPSE );

	
	TDynArray< String >& baseAbilitiesAttr = m_allowdAttributes.GetRef( NODE_BASE_ABILITIES );
	baseAbilitiesAttr.Reserve( 2 );
	baseAbilitiesAttr.PushBack( ATTR_MIN );
	baseAbilitiesAttr.PushBack( ATTR_MAX );
	
	TDynArray< String >& actionAttr = m_allowdAttributes.GetRef( NODE_ACTION );
	actionAttr.Reserve( 4 );
	actionAttr.PushBack( ATTR_NAME );
	actionAttr.PushBack( ATTR_EVENT );
	actionAttr.PushBack( ATTR_ACT );
	actionAttr.PushBack( ATTR_DEACT );
	
	TDynArray< String >& animSwitchAttr = m_allowdAttributes.GetRef( NODE_ANIM_SWITCH );
	animSwitchAttr.Reserve( 5 );
	animSwitchAttr.PushBack( ATTR_CATEGORY );
	animSwitchAttr.PushBack( ATTR_SWITCH_EQUIP_SLOT );
	animSwitchAttr.PushBack( ATTR_EVENT );
	animSwitchAttr.PushBack( ATTR_SWITCH_ACT );
	animSwitchAttr.PushBack( ATTR_SWITCH_DEACT );
	
	TDynArray< String >& partsAttr = m_allowdAttributes.GetRef( NODE_PARTS );	
	partsAttr.PushBack( ATTR_COUNT );
	
	TDynArray< String >& itemSetAttr = m_allowdAttributes.GetRef( NODE_ITEM_SET );	
	itemSetAttr.PushBack( ATTR_NAME );
	
	TDynArray< String >& aAttr = m_allowdAttributes.GetRef( NODE_A );
	aAttr.PushBack( ATTR_CHANCE );
	
	TDynArray< String >& overridenAttr = m_allowdAttributes.GetRef( TXT( "overriden" ) );
	overridenAttr.Reserve( 11 );
	overridenAttr.PushBack( ATTR_UPGRADE_LIST_NAME );
	overridenAttr.PushBack( ATTR_TEMPLATE_EQUIP );
	overridenAttr.PushBack( ATTR_UPGRADE_BASED_TEMPLATE );
	overridenAttr.PushBack( ATTR_TEMPLATE_HOLD );
	overridenAttr.PushBack( ATTR_APPEARANCE );
	overridenAttr.PushBack( ATTR_EQUIP_SLOT );
	overridenAttr.PushBack( ATTR_HOLD_SLOT );
	overridenAttr.PushBack( ATTR_COLOR_VARIANT );
	overridenAttr.PushBack( ATTR_ACTOR_ANIM_STATE );
	overridenAttr.PushBack( ATTR_INIT_DURABILITY );
	overridenAttr.PushBack( ATTR_DURABILITY );
	
	TDynArray< String >& weaponAttr = m_allowdAttributes.GetRef( TXT( "isWeapon" ) );
	weaponAttr.Reserve( 10 );
	weaponAttr.PushBack( TXT( "lethal" ) );
	weaponAttr.PushBack( TXT( "hand" ) );
	weaponAttr.PushBack( TXT( "sound_identification" ) );
	weaponAttr.PushBack( TXT( "draw_event" ) );
	weaponAttr.PushBack( TXT( "draw_act" ) );
	weaponAttr.PushBack( TXT( "draw_deact" ) );
	weaponAttr.PushBack( TXT( "holster_event" ) );
	weaponAttr.PushBack( TXT( "holster_act" ) );
	weaponAttr.PushBack( TXT( "holster_deact" ) );
	weaponAttr.PushBack( TXT( "hand" ) );
	
}

void CDefinitionsManager::ReloadAll()
{
	LOG_GAME( TXT("Reloading items and definitions...") );

	LoadTemplateNames();
	LoadDefinitions();

	for ( Uint32 i = 0; i < m_listerners.Size(); ++i )
	{
		m_listerners[ i ]->OnDefinitionsReloaded();
	}

	// Shrink the data
	m_ingredientsCategoriesMap.Shrink();
	m_itemSetsMap.Shrink();
	m_itemStatMap.Shrink();
	m_itemDamageCurveMap.Shrink();
	m_itemTagModifierMap.Shrink();
	m_abilityDefinitions.Shrink();
	m_itemDefinitions.Shrink();
	m_templateNameMap.Shrink();
	m_itemCategories.Shrink();
	m_itemsByCategory.Shrink();
	m_customNodes.Shrink();
	for ( auto& category : m_itemsByCategory )
	{
		category.m_second.Shrink();
	}

	LOG_GAME( TXT("Reloading items and definitions... done!") );
}

void CDefinitionsManager::LoadTemplateNames()
{	
	CDirectory * dir = GDepot->FindPath( ITEMS_TEMPLATES_DIR.AsChar() );
	if( dir )
	{
		m_templateNameMap.Clear();
		LoadTemplateNamesRecursive( dir );
	}
	else
	{
		LOG_GAME( TXT("CItemManager: directory '%ls' not found!"), ITEMS_TEMPLATES_DIR.AsChar() );
	}
}

void CDefinitionsManager::LoadTemplateNamesRecursive( CDirectory * dir )
{
	// Load files
	for ( CDiskFile* file : dir->GetFiles() )
	{
		CFilePath path( file->GetFileName() );
		if ( path.GetExtension() == EXTENSION )
		{	
			if( !m_templateNameMap.Insert( path.GetFileName(), file->GetDepotPath() ) )
			{
				WARN_GAME( TXT("CItemManager: item '%ls', already exists - ignoring! %ls"), path.GetFileName().AsChar(), file->GetDepotPath().AsChar() );
			}
		}		
	}

	// Call this function for child directories
	for ( CDirectory * pDir : dir->GetDirectories() )
	{
		LoadTemplateNamesRecursive( pDir );
	}
}

//! Recursively remove form m_templateNameMap
void CDefinitionsManager::RemoveTemplateNamesRecursive( CDirectory * dir )
{
	// Load files
	for ( CDiskFile* file : dir->GetFiles() )
	{
		CFilePath path( file->GetFileName() );
		if ( path.GetExtension() == EXTENSION )
		{	
			if( !m_templateNameMap.Erase( path.GetFileName() ) )
			{
				WARN_GAME( TXT("CItemManager: item '%ls', not exists - ignoring! %ls"), path.GetFileName().AsChar(), file->GetDepotPath().AsChar() );
			}
		}		
	}

	// Call this function for child directories
	for ( CDirectory * pDir : dir->GetDirectories() )
	{
		RemoveTemplateNamesRecursive( pDir );
	}
}

void CDefinitionsManager::LoadDefinitions()
{
    m_ingredientsCategoriesMap.Clear();
    m_itemSetsMap.Clear();
	m_itemStatMap.Clear();
	m_itemDamageCurveMap.Clear();
	m_itemTagModifierMap.Clear();
	m_abilityDefinitions.Clear();
	m_itemDefinitions.Clear();
	m_itemDefinitions.Reserve( 1000 );
	m_itemExtDefinitions.Clear();
	m_itemCategories.Clear();
	m_itemsByCategory.Clear();
	m_customNodes.Clear();
	m_usableItemTypes.Clear();

	delete m_lootDefinitions;
	m_lootDefinitions = NULL;
	GetLootDefinitions();

	FillActorAnimStatesMap( m_actorAnimStates );

	TDynArray< String > dirNames;

	if ( m_useAlternativePaths )
	{
		dirNames.PushBack( ITEMS_ALT_DIR );
		dirNames.PushBack( ABILITIES_ALT_DIR );
	}
	else
	{
		dirNames.PushBack( ITEMS_DEFINITIONS_DIR );
		dirNames.PushBack( ABILITIES_DIR );	
	}

	for ( const String& dirName : dirNames )
	{
		// Find directory with definition files
		CDirectory* dir = GDepot->FindPath( dirName );
		if ( !dir )
		{
			continue;
		}

		// iterate definition files and load item/ability data
		for ( CDiskFile* file : dir->GetFiles() )
		{
			const String path = file->GetDepotPath();
			if ( path.EndsWith( TXT(".xml") ) )
			{
				LoadDefinitions( path, m_creatorTag );
			}
		}
	}

	// Cache contained abilities
	for( auto it = m_abilityDefinitions.Begin(), end = m_abilityDefinitions.End(); it != end; ++it )
	{
		SAbility& ability = (*it).m_second;

		for( auto it2 = ability.m_abilities.Begin(), end2 = ability.m_abilities.End(); it2 != end2; ++it2 )
		{
			SAbility* contained = m_abilityDefinitions.FindPtr( *it2 );
			if( contained )
			{
				ability.m_cachedAbilities.PushBack( contained );
			}
		}
	}

#ifndef NO_LOG
	for ( Uint32 i=0; i<m_itemCategories.Size(); ++i )
	{
		if ( GetItemDefinition( m_itemCategories[i] ) )
		{
			ERR_GAME( TXT( "There is a category and an item with the same name: %s, it is not allowed! Correct it!" ), m_itemCategories[i].AsString().AsChar() );
			XML_ERROR( TXT( "General" ), TXT( "ERROR: Found category and item with the same name %ls, this is not allowed!" ), m_itemCategories[i].AsString().AsChar() );
		}
	}
#endif

	HashInventoryData();
}

void CDefinitionsManager::HashInventoryData()
{
	// Load Item Stat Definitions
	CGatheredResource itemStatDefinitions( ITEM_DEFINITIONS.AsChar(), 0 );
	C2dArray* itemStatDefinitionsArray = itemStatDefinitions.LoadAndGet< C2dArray >();
	RED_ASSERT( itemStatDefinitionsArray != nullptr, TXT( ">>>>> Definitions Manager cannot find tooltip_settings.csv. <<<<<" ) );

	CName tag;
	SItemStat itemStat;
	Uint32 statDefinitionsSize = itemStatDefinitionsArray->GetNumberOfRows();
	for ( Uint32 s = 0; s < statDefinitionsSize; ++s )
	{
		itemStat.m_statType = CName( itemStatDefinitionsArray->GetValue( 3, s ) ); //FromString( itemStatDefinitionsArray->GetValue( 3, s ),  );
		FromString( itemStatDefinitionsArray->GetValue( 4, s ), itemStat.m_statWeight );
		FromString( itemStatDefinitionsArray->GetValue( 2, s ), itemStat.m_statIsPercentage );
		tag = CName( itemStatDefinitionsArray->GetValue( 0, s ) );

		m_itemStatMap.Insert( tag, itemStat );
	}

	// Load Category-based Price Curves
	CGatheredResource damageCurveData( ITEM_DAMAGECURVES.AsChar(), 0 );
	C2dArray* damageCurveArray = damageCurveData.LoadAndGet< C2dArray >();
	RED_ASSERT( damageCurveArray != nullptr, TXT( ">>>>> Definitions Manager cannot find economy_damagecurves.csv. <<<<<" ) );

	SItemDamageCurve damageCurve;
	Uint32 damageCurveSize = damageCurveArray->GetNumberOfRows();
	for ( Uint32 pc = 0; pc < damageCurveSize; ++pc )
	{
		FromString( damageCurveArray->GetValue( 1, pc ), damageCurve.m_term1 );
		FromString( damageCurveArray->GetValue( 2, pc ), damageCurve.m_term2 );
		FromString( damageCurveArray->GetValue( 3, pc ), damageCurve.m_term3 );
		tag = CName( damageCurveArray->GetValue( 0, pc ) ); // FromString( damageCurveArray->GetValue( 0, pc ),  );

		m_itemDamageCurveMap.Insert( tag, damageCurve );
	}

	// Load Category-based Price Curves
	CGatheredResource tagModifierData( ITEM_TAGMODIFIERS.AsChar(), 0 );
	C2dArray* tagModifierArray = tagModifierData.LoadAndGet< C2dArray >();
	RED_ASSERT( tagModifierArray != nullptr, TXT( ">>>>> Definitions Manager cannot find economy_tagmodifiers.csv. <<<<<" ) );

	SItemTagModifier tagModifier;
	Uint32 numRows = tagModifierArray->GetNumberOfRows();
	Uint32 numCols = tagModifierArray->GetNumberOfColumns();
	Float modifierValue = -1.0f;
	CName modifierTag;

	for ( Uint32 row = 0; row < numRows; ++row )
	{
		tagModifier.m_modifierMap.Clear();

		for ( Uint32 col = 1; col < numCols; ++col )
		{
			FromString( tagModifierArray->GetValue( col, row ), modifierValue );
			modifierTag = CName( tagModifierArray->GetHeader( col ) ); //FromString( tagModifierArray->GetHeader( col ), modifierTag );
			tagModifier.m_modifierMap.Insert( modifierTag, modifierValue );
		}

		tag = CName( tagModifierArray->GetValue( 0, row ) ); // FromString( tagModifierArray->GetValue( 0, row ), tag );
		m_itemTagModifierMap.Insert( tag, tagModifier );
	}
}

Uint32 CDefinitionsManager::FilterAbilitiesByTags( TDynArray< CName >& outAbilities, const TDynArray< CName >& inAbilities, const TagList& tags, Bool withoutTags ) const
{
	Uint32 numCopied( 0 );
	ASSERT( outAbilities.Size() == 0 ); // output table is supposed to be empty

	for ( Uint32 i = 0; i < inAbilities.Size(); ++i )
	{
		const SAbility* abl = GetAbilityDefinition( inAbilities[ i ] );
		if ( abl && ( abl->HasAnyOfTags( tags ) != withoutTags ) )
		{
			outAbilities.PushBack( inAbilities[ i ] );
			++numCopied;
		}
	}

	return numCopied;
}

RED_DEFINE_STATIC_NAME( EAbilityType );

Bool CDefinitionsManager::ReadCName( CXMLFileReader& xmlReader, const String& name, CName& attr )
{
	String temp;
	if( xmlReader.Attribute( name, temp ) )
	{
		if ( Red::System::StringCompare( temp.AsChar(), TXT( "none") ) == 0 )
		{
			attr = CName::NONE;
			DATA_HALT( DES_Tiny, nullptr, TXT("XML definitions"), TXT("Incorrect property %s name 'none' (use 'None')"), name.AsChar() );
		}
		else
		{
			attr = CName( temp );
		}
		return true;
	}
	return false;
}

void CDefinitionsManager::LoadItemOverrideProperties( CXMLFileReader& xmlReader, SItemDefinition::OverridableProperties& properties, const String& itemName )
{
	String temp;

	// Upgrade list name
	ReadCName( xmlReader, ATTR_UPGRADE_LIST_NAME, properties.m_upgradeListName );

	// optional attributes
	if( xmlReader.Attribute( ATTR_TEMPLATE_EQUIP, temp) )
	{
		properties.m_equipTemplateName = temp;
	}
	if ( xmlReader.Attribute( ATTR_UPGRADE_BASED_TEMPLATE, temp ) )
	{
		properties.m_upgradeBasedTemplateName = temp;
	}
	if ( xmlReader.Attribute( ATTR_TEMPLATE_HOLD, temp ) )
	{
		properties.m_holdTemplateName = temp;
	}

	ReadCName( xmlReader, ATTR_APPEARANCE, properties.m_itemAppearanceName );

	ReadCName( xmlReader, ATTR_EQUIP_SLOT, properties.m_equipSlot );

	ReadCName( xmlReader, ATTR_HOLD_SLOT, properties.m_holdSlot );

	// Color variants
	ReadCName( xmlReader, ATTR_COLOR_VARIANT, properties.m_colorVariantName );

	ReadCName( xmlReader, ATTR_ACTOR_ANIM_STATE, properties.m_actorAnimState );

	// Initial Durability
	if( xmlReader.Attribute( ATTR_INIT_DURABILITY, temp ) )
	{
		Float initialDurability = -1;
		if ( FromString( temp, initialDurability ) )
		{
			initialDurability = Clamp< Float > ( initialDurability, -1, 1000 );
		}
		else
		{
			ReportParseErrorAttr( ATTR_INIT_DURABILITY, xmlReader.GetCurrentNode()->name(), itemName, xmlReader.GetFileNameForDebug() );
		}
		properties.m_initialDurability = initialDurability;
	}

	// Max Durability
	if ( xmlReader.Attribute( ATTR_DURABILITY, temp ) )
	{
		Float maxDurability = -1;
		if ( FromString( temp, maxDurability ) )
		{
			maxDurability = Clamp< Float > ( maxDurability, -1, 1000 );
		}
		else
		{
			ReportParseErrorAttr( ATTR_DURABILITY, xmlReader.GetCurrentNode()->name(), itemName, xmlReader.GetFileNameForDebug() );
		}
		properties.m_maxDurability = maxDurability;
	}

	// Bound items
	if ( xmlReader.BeginNode( NODE_BOUND_ITEMS, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_BOUND_ITEMS, xmlReader.GetFileNameForDebug() );

		properties.m_boundItems.Clear();
		while ( xmlReader.BeginNode( NODE_ITEM ) )
		{
			if ( xmlReader.Value( temp ) && !temp.Empty() )
			{
				properties.m_boundItems.PushBack( CName( temp ) );
			}
			else
			{
				ReportParseErrorValue( NODE_ITEM, itemName, xmlReader.GetFileNameForDebug() );
			}
			xmlReader.EndNode(); // NODE_ITEM
		}
		properties.m_boundItems.Shrink();
		xmlReader.EndNode( false ); // NODE_BOUND_ITEMS
	}

	// Variants
	if ( xmlReader.BeginNode( NODE_VARIANTS, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_VARIANTS, xmlReader.GetFileNameForDebug() );

		properties.m_variants.Clear();
		while ( xmlReader.BeginNode( NODE_VARIANT ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_VARIANT, xmlReader.GetFileNameForDebug() );

			SItemDefinition::SItemVariant variant;

			CName category;
			if ( ReadCName( xmlReader, ATTR_CATEGORY, category ) )
			{
				variant.m_categories.PushBack( category );
			}

			if ( xmlReader.Attribute( ATTR_TEMPLATE_EQUIP, temp ) )
			{
				variant.m_template = temp;
			}
			if ( xmlReader.Attribute( ATTR_ALL, temp ) )
			{
				FromString( temp, variant.m_expectAll );
			}
			while ( xmlReader.BeginNode( NODE_ITEM ) )
			{
				if ( xmlReader.Value( temp ) && !temp.Empty() )
				{
					variant.m_affectingItems.PushBack( CName( temp ) );
				}
				else
				{
					ReportParseErrorValue( NODE_ITEM, itemName, xmlReader.GetFileNameForDebug() );
				}
				xmlReader.EndNode(); // item
			}
			while ( xmlReader.BeginNode( NODE_ITEM_CATEGORY ) )
			{
				if ( xmlReader.Value( temp ) && !temp.Empty() )
				{
					variant.m_categories.PushBack( CName( temp ) );
				}
				else
				{
					ReportParseErrorValue( NODE_ITEM_CATEGORY, itemName, xmlReader.GetFileNameForDebug() );
				}
				xmlReader.EndNode(); // item category
			}

			properties.m_variants.PushBack( variant );

			xmlReader.EndNode(); 
		}
		properties.m_variants.Shrink();
		xmlReader.EndNode( false ); // Variant
	}

	// Collapse
	if ( xmlReader.BeginNode( NODE_COLLAPSE, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_COLLAPSE, xmlReader.GetFileNameForDebug() );

		while ( xmlReader.BeginNode( NODE_CATEGORY_COND ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_CATEGORY_COND, xmlReader.GetFileNameForDebug() );

			CName category;
			if ( ReadCName( xmlReader, ATTR_NAME, category ) )
			{
				properties.m_collapseCond.m_collapseCategoryCond.PushBack( category );
			}
			xmlReader.EndNode(); 
		}

		while ( xmlReader.BeginNode( NODE_ITEM_COND ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_ITEM_COND, xmlReader.GetFileNameForDebug() );
			CName item;
			if ( ReadCName( xmlReader, ATTR_NAME, item ) )
			{					
				if ( xmlReader.Attribute( ATTR_COLLAPSE, temp ) )
				{
					Bool collapse = false;
					if ( FromString( temp, collapse ) )
					{
						if( collapse )
						{
							properties.m_collapseCond.m_collapseItemCond.PushBack( item );
						}
						else
						{
							properties.m_collapseCond.m_uncollapseItemCond.PushBack( item );
						}
					}
					else
					{
						ReportParseErrorAttr( ATTR_COLLAPSE, xmlReader.GetCurrentNode()->name(), itemName, xmlReader.GetFileNameForDebug() );
					}
				}
			}
					
			xmlReader.EndNode(); 
		}
		properties.m_collapseCond.m_collapseItemCond.Shrink();
		properties.m_collapseCond.m_uncollapseItemCond.Shrink();
		properties.m_collapseCond.m_collapseCategoryCond.Shrink();
		xmlReader.EndNode( false ); // Variant
	}

	// Base abilities
	if( xmlReader.BeginNode( NODE_BASE_ABILITIES, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_BASE_ABILITIES, xmlReader.GetFileNameForDebug() );

		if ( xmlReader.Attribute( ATTR_MIN, temp ) )
		{
			if ( !FromString( temp, properties.m_minAbilities ) )
			{
				properties.m_minAbilities = 0;
				ReportParseErrorAttr( ATTR_MIN, NODE_BASE_ABILITIES, itemName, xmlReader.GetFileNameForDebug() );
			}
		}
		if ( xmlReader.Attribute( ATTR_MAX, temp ) )
		{
			if ( !FromString( temp, properties.m_maxAbilities ) )
			{
				properties.m_maxAbilities = -1;
				ReportParseErrorAttr( ATTR_MAX, NODE_BASE_ABILITIES, itemName, xmlReader.GetFileNameForDebug() );
			}
		}
		properties.m_baseAbilities.Clear();
		while( xmlReader.BeginNode( NODE_A ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_A, xmlReader.GetFileNameForDebug() );

			SItemDefinition::SItemAbility itemAbility;
			if( xmlReader.Value( temp ) && !temp.Empty() )
			{
				itemAbility.m_name = CName( temp );
			}
			else
			{
				ReportParseErrorValue( NODE_BASE_ABILITIES, itemName, xmlReader.GetFileNameForDebug() );
			}

			itemAbility.m_chance = -1.0f;
			if ( xmlReader.Attribute( ATTR_CHANCE, temp ) && !FromString( temp, itemAbility.m_chance ) )
			{
				ReportParseErrorAttr( ATTR_CHANCE, NODE_A, itemName, xmlReader.GetFileNameForDebug() );
			}
			properties.m_baseAbilities.PushBack( itemAbility );
			
			xmlReader.EndNode(); // NODE_A
		}
		properties.m_baseAbilities.Shrink();
		xmlReader.EndNode( false ); // NODE_BASE_ABILITIES
	}

	// Item anim actions
	if( xmlReader.BeginNode( NODE_ANIM_ACTIONS, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_ANIM_ACTIONS, xmlReader.GetFileNameForDebug() );

		properties.m_animActions.Clear();
		while( xmlReader.BeginNode( NODE_ACTION ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_ACTION, xmlReader.GetFileNameForDebug() );

			SItemDefinition::SItemAnimAction animAction;

			ReadCName( xmlReader, ATTR_NAME, animAction.m_name );
			ReadCName( xmlReader, ATTR_EVENT, animAction.m_event );
			ReadCName( xmlReader, ATTR_ACT, animAction.m_act );
			ReadCName( xmlReader, ATTR_DEACT, animAction.m_deact );

			if ( animAction.m_name && animAction.m_event && animAction.m_act && animAction.m_deact )
			{
				properties.m_animActions.PushBack( animAction );
			}
			xmlReader.EndNode(); // NODE_ACTION
		}
		properties.m_animActions.Shrink();
		xmlReader.EndNode( false ); // NODE_ANIM_ACTIONS
	}

	// Item anim switches
	if( xmlReader.BeginNode( NODE_ANIM_SWITCHES, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_ANIM_SWITCHES, xmlReader.GetFileNameForDebug() );

		properties.m_animSwitches.Clear();
		while( xmlReader.BeginNode( NODE_ANIM_SWITCH ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_ANIM_SWITCH, xmlReader.GetFileNameForDebug() );

			SItemDefinition::SItemAnimSwitch animSwitch;

			ReadCName( xmlReader, ATTR_CATEGORY, animSwitch.m_category );

			ReadCName( xmlReader, ATTR_SWITCH_EQUIP_SLOT, animSwitch.m_equipSlot );

			ReadCName( xmlReader, ATTR_EVENT, animSwitch.m_eventName );

			ReadCName( xmlReader, ATTR_SWITCH_ACT, animSwitch.m_actName );

			ReadCName( xmlReader, ATTR_SWITCH_DEACT, animSwitch.m_deactName );

			if ( animSwitch.m_category && animSwitch.m_equipSlot && animSwitch.m_eventName )
			{
				properties.m_animSwitches.PushBack( animSwitch );
			}

			xmlReader.EndNode(); // NODE_ANIM_SWITCH
		}
		properties.m_animSwitches.Shrink();
		xmlReader.EndNode( false ); // NODE_ANIM_SWITCHES
	}

	// Slot abilities
	if( xmlReader.BeginNode( NODE_SLOT_ITEMS, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_SLOT_ITEMS, xmlReader.GetFileNameForDebug() );

		properties.m_slotItems.Clear();
		while( xmlReader.BeginNode( NODE_A ) )
		{
			if( xmlReader.Value( temp ) && !temp.Empty() )
			{
				properties.m_slotItems.PushBack( CName( temp ) );
			}
			else
			{
				ReportParseErrorValue( NODE_SLOT_ITEMS, itemName, xmlReader.GetFileNameForDebug() );
			}
			xmlReader.EndNode(); // NODE_A
		}
		properties.m_slotItems.Shrink();
		xmlReader.EndNode( false ); // NODE_SLOT_ABILITIES
	}
}

void CDefinitionsManager::AttachItemExtToItem( SItemDefinition* itemDefinition, SItemExtDefinition* itemExtDefinition ) const
{	
	itemExtDefinition->AttachToItem( itemDefinition );
}

void CDefinitionsManager::AttachItemExtsToItem( const CName& itemName )
{
	TDynArray<SItemExtDefinition*>** itemExtDefinitions = m_itemExtDefinitions.FindPtr( itemName );
	if( itemExtDefinitions != nullptr )
	{
		SItemDefinition* itemDefinition = m_itemDefinitions.FindPtr( itemName );
		for ( auto& itemExtDefinition : (**itemExtDefinitions) )
		{
			AttachItemExtToItem( itemDefinition, itemExtDefinition );
		}
	}	
}

void CDefinitionsManager::LoadDefinitions( const String& xmlFilePath, const Red::System::GUID& creatorTag )
{
	RED_LOG( Def, TXT("Reding defs from file: '%ls'"), xmlFilePath.AsChar() );

	CLEAR_XML_ERRORS( xmlFilePath.AsChar() );

	//LOG_GAME( TXT("CDefinitionsManager: Loading definitions from %s"), xmlFilePath.AsChar() );

	IFile* file = NULL;
	CDiskFile *diskFile = GDepot->FindFile( xmlFilePath );
	
	if( diskFile )
		file = diskFile->CreateReader();

	if( file )
	{
		String abilityName;
		String itemName;
		String temp;
		String previousNodeName = TXT( "No previous nodes - first node" );

		// Separators for item tags parsing
		TDynArray< String > separators;
		separators.PushBack( TXT( "," ) );
		separators.PushBack( TXT( " " ) );
		separators.PushBack( TXT( ";" ) );
		separators.PushBack( TXT( "\n" ) );
		separators.PushBack( TXT( "\t" ) );

		CXMLFileReader xmlReader( *file );

		if ( !xmlReader.GetChildCount() )
		{
			RED_WARNING( false, "Definitions Manager: Invalid XML File: %s", xmlFilePath.AsChar() );
			XML_ERROR( xmlFilePath.AsChar(), TXT( "ERROR: No childnodes found" ) );
		}

		CEnum* abilityTypes = SRTTI::GetInstance().FindEnum( CNAME( EAbilityType ) );

		if ( xmlReader.BeginNode( NODE_ROOT, true ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_ROOT, xmlFilePath );

			if ( xmlReader.BeginNode( NODE_DEFINITIONS, true ) )
			{
				SearchForDisallowedElements( xmlReader, NODE_DEFINITIONS, xmlFilePath );

				// Abilities
				if( xmlReader.BeginNode( NODE_ABILITIES, true ) )
				{
					SearchForDisallowedElements( xmlReader, NODE_ABILITIES, xmlFilePath );

					while( xmlReader.BeginNode( NODE_ABILITY ) )
					{
						SearchForDisallowedElements( xmlReader, NODE_ABILITY, xmlFilePath );
												
						if ( xmlReader.Attribute( ATTR_NAME, abilityName ) )
						{
							SAbility ab;
							ab.m_name = CName( abilityName );
							ab.m_creatorTag = creatorTag;

							// Prerequisites
							if ( xmlReader.Attribute( ATTR_PREREQUISITES, temp ) )
							{
								TDynArray< String > prerequisitesStrings = temp.Split( TXT(",") );
								for ( TDynArray< String >::iterator i = prerequisitesStrings.Begin();
									i != prerequisitesStrings.End();
									++i )
								{
									i->Trim();
									ab.m_prerequisites.PushBack( CName(*i) );
								}
							}

							// Type
							ab.m_type = 0;
							if ( abilityTypes && xmlReader.Attribute( ATTR_TYPE, temp ) )
							{
								if( !abilityTypes->FindValue( CName( temp ), ab.m_type ) )
								{
									ReportParseErrorValue( ATTR_TYPE + TXT(" = ") + temp, abilityName, xmlFilePath );
								}
							}

							// Attributes
							while( xmlReader.BeginNextNode() )
							{
								if( xmlReader.GetNodeName( temp ) )
								{
									// crafting
									if ( temp.EqualsNC( TXT("crafting") ) )
									{
										if ( xmlReader.Attribute( ATTR_TYPE, temp ) )
										{
											ab.m_crafting.m_type = CName( temp );
										}

										if ( xmlReader.Attribute( ATTR_LEVEL, temp ) )
										{
											Uint32 level = 1;
											if ( FromString( temp, level ) )
											{
												level = Clamp< Uint32 > ( level, 1, 1000 );
												ab.m_crafting.m_level = level;
											}
											else
											{
												ReportParseErrorAttr( ATTR_LEVEL, TXT( "crafting" ), ATTR_LEVEL + TXT( "=" ) + temp, xmlFilePath );
											}
										}
									}
									else if ( temp.EqualsNC( TXT("tags") ) )
									{
										if ( xmlReader.Value( temp ) && !temp.Empty() )
										{
											TDynArray< String > tags = temp.Split( separators );
											for ( Uint32 i = 0; i < tags.Size(); ++i )
											{
												tags[ i ].RemoveWhiteSpacesAndQuotes();
												if ( !tags[ i ].Empty() )
												{
													ab.m_tags.AddTag( CName( tags[ i ] ) );
												}
											}
										}
									}
									else
									{
										// temp shouldnt contain = or "
										const CName currentNode = CName( temp );
#ifndef NO_DATA_ASSERTS
										if ( temp.ContainsSubstring( TXT( "=" ) ) || temp.ContainsSubstring( TXT( "\"" ) ) )
										{
											XML_ERROR( xmlFilePath.AsChar(), TXT( "ERROR: Node with '=' or '\"' is not allowed!! (Node '%ls' in ability '%ls')" ), temp.AsChar(), abilityName.AsChar() );
										}
#endif //! NO_DATA_ASSERTS
										Bool isAbility = false;
										if( xmlReader.Attribute( ATTR_IS_ABILITY, temp ) )
										{
											if( !FromString( temp, isAbility ) )
											{
												ReportParseErrorAttr( ATTR_IS_ABILITY, currentNode.AsString(), abilityName, xmlFilePath );
											}
										}

										if( !isAbility )
										{
											SearchForDisallowedElements( xmlReader, NODE_ABILITY + TXT("_and_not_") + ATTR_IS_ABILITY, xmlFilePath );
											SAbilityAttribute attr;

											attr.m_name = CName( currentNode );

											if( xmlReader.Attribute( ATTR_MULT, temp ) )
											{												
												if( Red::System::StringCompareNoCase( temp.AsChar(), TXT("base") ) == 0 )
												{
													attr.m_type = EAttrT_Base;
												}
												else if( Red::System::StringCompareNoCase( temp.AsChar(), TXT("multi") ) == 0 || Red::System::StringCompareNoCase( temp.AsChar(), TXT("multiplicative") ) == 0 || Red::System::StringCompareNoCase( temp.AsChar(), TXT("mult") ) == 0 )
												{
													attr.m_type = EAttrT_Multi;
												}
												else if( Red::System::StringCompareNoCase( temp.AsChar(), TXT("add") ) == 0 || Red::System::StringCompareNoCase( temp.AsChar(), TXT("additive") ) == 0 || Red::System::StringCompareNoCase( temp.AsChar(), TXT("addit") ) == 0 )
												{
													attr.m_type = EAttrT_Additive;
												}
												else
												{
													ReportParseErrorAttr( ATTR_MULT, attr.m_name.AsString(), abilityName, xmlFilePath );
												}									    
											}

											if( xmlReader.Attribute( ATTR_DISPLAY_PERC, temp ) && ! FromString( temp, attr.m_displayPerc )  )
											{
												ReportParseErrorAttr( ATTR_DISPLAY_PERC, attr.m_name.AsString(), abilityName, xmlFilePath );
												attr.m_displayPerc = attr.m_type == EAttrT_Multi; // default value
											}								  

											if( xmlReader.Attribute( ATTR_ALWAYS_RANDOM, temp ) && ! FromString( temp, attr.m_alwaysRandom ) )
											{
												ReportParseErrorAttr( ATTR_ALWAYS_RANDOM, attr.m_name.AsString(), abilityName, xmlFilePath );
											}

											Bool minParsed = false;
											if( xmlReader.Attribute( ATTR_MIN, temp ) && ! FromString( temp, attr.m_min ) )
											{
												ReportParseErrorAttr( ATTR_MIN, attr.m_name.AsString(), abilityName, xmlFilePath );
											}
											else
											{
												minParsed = true;
												attr.m_max = attr.m_min;
											}

											if( xmlReader.Attribute( ATTR_MAX, temp ) && ! FromString( temp, attr.m_max ) )
											{
												ReportParseErrorAttr( ATTR_MAX, attr.m_name.AsString(), abilityName, xmlFilePath );
											}
											else
											{
												if( !minParsed )
												{
													attr.m_min = attr.m_max;
												}
											}

											if( xmlReader.Attribute( ATTR_PRECISION, temp ) && ! FromString( temp, attr.m_precision ) )
											{
												ReportParseErrorAttr( ATTR_PRECISION, attr.m_name.AsString(), abilityName, xmlFilePath );
												attr.m_precision = -1;
											}

											ab.m_attributes.PushBack( attr );
										}
										else
										{
											ab.m_abilities.PushBack( currentNode );
										}
									}
								}

								xmlReader.EndNode(); // attribute node
							}

							auto foundAbility = m_abilityDefinitions.Find( ab.m_name );
							if(      foundAbility != m_abilityDefinitions.End()
								&& (*foundAbility).m_second.m_creatorTag != creatorTag )
							{
								ERR_GAME( TXT("CDefinitionsManager: try to override '%ls' ability with definition from another creator (e.g. DLC mounter)"), ab.m_name.AsChar() );
							}
							else
							{
								m_abilityDefinitions.Insert( ab.m_name, ab );
							}
							
							previousNodeName = abilityName;
						}
						else
						{
							ReportParseErrorAttr( ATTR_NAME, NODE_ABILITY, xmlFilePath, xmlFilePath );
						}

						xmlReader.EndNode(); // NODE_ABILITY
					}

					xmlReader.EndNode( false ); // NODE_ABILITIES
				}

				// Items
				if( xmlReader.BeginNode( NODE_ITEMS, true ) )
				{
					SearchForDisallowedElements( xmlReader, NODE_ITEMS, xmlFilePath );

					while( xmlReader.BeginNode( NODE_ITEM ) )
					{
						// Item attributes
						SItemDefinition itemDef;
						itemDef.m_creatorTag = creatorTag;

#ifndef NO_DATA_ASSERTS
						itemDef.m_debugXMLFile = xmlFilePath;
#endif

						Bool ok = xmlReader.Attribute( ATTR_NAME, itemName );
						// Item weapon flags
						if( xmlReader.Attribute( ATTR_WEAPON, temp ) )
						{
							Bool weapon = false;
							if( !FromString( temp, weapon ) )						
							{
								ReportParseErrorAttr( ATTR_WEAPON, NODE_ITEM, itemName, xmlFilePath );
								weapon = false;
							}

							if( weapon )
							{
								itemDef.m_flags |= SItemDefinition::FLAG_WEAPON;
							}
						}
						SearchForDisallowedElements( xmlReader, NODE_ITEM, xmlFilePath, true, itemDef.IsWeapon() );

						// Category of the item
						if( xmlReader.Attribute( ATTR_CATEGORY, temp ) )
						{
							itemDef.m_category = CName( temp );
						}
						else
						{
							itemDef.m_category = CNAME( default );
						}
						m_itemCategories.PushBackUnique( itemDef.m_category );

						if( xmlReader.Attribute( ATTR_ENHANCEMENT_SLOTS, temp ) )
						{
							if ( !FromString( temp, itemDef.m_enhancementSlotCount ) )
							{
								itemDef.m_enhancementSlotCount = 0;
								ReportParseErrorAttr( ATTR_ENHANCEMENT_SLOTS, NODE_ITEM, itemName, xmlFilePath );
							}
						}

						// Price of the item
						if( xmlReader.Attribute( ATTR_PRICE, temp ) )
						{
							Int32 price = 0;
							if ( !FromString( temp, price ) )
							{
								ReportParseErrorAttr( ATTR_PRICE, NODE_ITEM, itemName, xmlFilePath );
								price = 0;
							}
							else if ( price < 0 )
							{
								price = 0;
							}
							itemDef.m_price = static_cast< Uint32 >( price );
						}
						else
						{
							itemDef.m_price = 0;
						}

						// Weight of the item
						if( xmlReader.Attribute( ATTR_WEIGHT, temp ) )
						{
							Float weight = 0;
							if ( !FromString( temp, weight ) )
							{
								ReportParseErrorAttr( ATTR_WEIGHT, NODE_ITEM, itemName, xmlFilePath );
								weight = 0;
							}
							else if ( weight < 0 )
							{
								weight = 0;
							}
							itemDef.m_weight = static_cast< Float >( weight );
						}
						else
						{
							itemDef.m_weight = 0;
						}

						if ( xmlReader.Attribute( ATTR_STACKABLE, temp ) )
						{
							Int32 noStack = 0;

							if ( !FromString( temp, noStack ) )
							{
								ReportParseErrorAttr( ATTR_STACKABLE, NODE_ITEM, itemName, xmlFilePath );
								noStack = 1;
							}
							else if( noStack < 1 )
								itemDef.m_stackSize = 1;
							else
								itemDef.m_stackSize = noStack;
						}

						// Icon path
						if ( xmlReader.Attribute( ATTR_ICON_PATH, temp ) )
							itemDef.m_iconPath = temp;

						// Localization name
						if ( xmlReader.Attribute( ATTR_LOCALIZATION_KEY_NAME, temp ) )
							itemDef.m_localizationKeyName = temp;

						// Localization description
						if ( xmlReader.Attribute( ATTR_LOCALIZATION_KEY_DESCCRIPTION, temp ) )
							itemDef.m_localizationKeyDesc = temp;

						if ( xmlReader.Attribute( ATTR_CRAFTED_ITEM, temp ) )
						{
							itemDef.m_craftedItemName = CName( temp );
						}

						if ( xmlReader.Attribute( ATTR_CRAFTSMAN_TYPE, temp ) )
						{
							itemDef.m_craftsmanType = CName( temp );
						}

						if ( xmlReader.Attribute( ATTR_CRAFTSMAN_LEVEL, temp ) )
						{
							itemDef.m_craftsmanLevel = CName( temp );
						}

						if ( xmlReader.Attribute( ATTR_ABILITIES_APPLY, temp ) )
						{
							if ( Red::System::StringCompare( temp.AsChar(), TXT("OnMount") ) == 0 )
							{
								itemDef.m_flags |= SItemDefinition::FLAG_ABILITY_ON_MOUNT;
							}
							else if ( Red::System::StringCompare( temp.AsChar(), TXT("OnHold") ) == 0 )
							{
								itemDef.m_flags |= SItemDefinition::FLAG_ABILITY_ON_HOLD;
							}
							else if ( Red::System::StringCompare( temp.AsChar(), TXT("OnHidden") ) == 0 )
							{
								itemDef.m_flags |= SItemDefinition::FLAG_ABILITY_ON_HIDDEN;
							}
							else if ( !temp.Empty() )
							{
								ReportParseErrorAttr( ATTR_ABILITIES_APPLY, NODE_ITEM, itemName, xmlFilePath );
							}
							// if no ability_mode, item abilities are manually added in script when additional requirements are met
						}


						if( xmlReader.Attribute( ATTR_GRID_SIZE, temp ) )
						{
							Uint32 gridSize = 1;
							if ( FromString( temp, gridSize ) )
							{
								gridSize = Clamp< Uint32 > ( gridSize, 1, 4 );
								itemDef.m_gridSize = gridSize;
							}
							else
							{
								ReportParseErrorAttr( ATTR_GRID_SIZE, NODE_ITEM, itemName, xmlFilePath );
							}
						}

						// Recycling Parts Items
						if ( xmlReader.BeginNode( NODE_RECYCLING_PARTS, true ) )
						{
							SearchForDisallowedElements( xmlReader, NODE_RECYCLING_PARTS, xmlFilePath );

							while ( xmlReader.BeginNode( NODE_PARTS ) )
							{
								SearchForDisallowedElements( xmlReader, NODE_PARTS, xmlFilePath );

								SItemDefinition::SItemPartsEntry parts;
								parts.m_count = 1;
								if ( xmlReader.Attribute( ATTR_COUNT, temp ) )
								{
									Uint32 count = 1;
									if ( FromString( temp, count ) )
									{
										count = Clamp< Uint32 > ( count, 1, 1000 );
										parts.m_count = count;
									}
									else
									{
										ReportParseErrorAttr( ATTR_COUNT, NODE_PARTS, itemName, xmlFilePath );
									}
								}

								if ( xmlReader.Value( temp ) && !temp.Empty() )
								{
									parts.m_name = CName( temp );
								}
								else
								{
									ReportParseErrorValue( NODE_PARTS, itemName, xmlFilePath );
								}

								itemDef.m_recyclingParts.PushBack( parts );

								xmlReader.EndNode(); // NODE_PARTS
							}
							xmlReader.EndNode( false ); // NODE_RECYCLING_PARTS
						}

						// Recycling Parts Items
						if ( xmlReader.BeginNode( NODE_ITEM_SET, true ) )
						{
							SearchForDisallowedElements( xmlReader, NODE_ITEM_SET, xmlFilePath );
							if ( xmlReader.Attribute( ATTR_NAME, temp ) )
							{
								itemDef.m_setName = CName( temp );
							}

							xmlReader.EndNode( false ); // NODE_ITEM_SET
						}

						// Optional - End

						// Item tags
						if ( xmlReader.BeginNode( NODE_TAGS, true ) )
						{
							if ( xmlReader.Value( temp ) && !temp.Empty() )
							{
								TDynArray<String> tags = temp.Split( separators );
								for ( Uint32 i=0; i<tags.Size(); ++i )
								{
									itemDef.m_itemTags.PushBackUnique( CName( tags[i] ) );
								}
							}
							xmlReader.EndNode( false ); // NODE_TAGS
						}

						LoadItemOverrideProperties( xmlReader, itemDef.m_defaultProperties, itemName );

						if ( xmlReader.BeginNode( NODE_PLAYER_OVERRIDE, true ) )
						{
							if ( xmlReader.GetChildCount() > 0 || xmlReader.HasAnyAttribute() )
							{
								SearchForDisallowedElements( xmlReader, NODE_PLAYER_OVERRIDE, xmlFilePath, true );

								SItemDefinition::OverridableProperties* overridableProps = new SItemDefinition::OverridableProperties( itemDef.m_defaultProperties );
								itemDef.m_playerOverridableProperties = overridableProps;

								LoadItemOverrideProperties( xmlReader, *itemDef.m_playerOverridableProperties, itemName );
							}
							xmlReader.EndNode( false ); // NODE_PLAYER_OVERRIDE
						}

						if( ok )
						{
							CName nameItemName = CName( itemName );
							previousNodeName = itemName;

							if ( m_itemDefinitions.KeyExist( nameItemName ) )
							{
								ERR_GAME( TXT("CDefinitionsManager: item '%ls' is duplicated in XML"), itemName.AsChar() );
								XML_ERROR( xmlFilePath.AsChar(), TXT( "ERROR: Duplicated item: %ls" ), itemName.AsChar() );
							}
							else
							{
								itemDef.m_itemTags.Shrink();
								itemDef.m_ingredients.Shrink();
								itemDef.m_recyclingParts.Shrink();
																
								m_itemDefinitions.Insert( nameItemName, itemDef );
								
								AttachItemExtsToItem( nameItemName );
								
								m_itemsByCategory[ itemDef.m_category ].PushBackUnique( nameItemName );
								previousNodeName = itemName;

#ifndef RED_FINAL_BUILD
								//a poor check for whether the current file is in a DLC or main game depot
								if ( diskFile->GetDirectory()->GetParent()->GetParent()->GetParent() )
								{
									String depotName = diskFile->GetDirectory()->GetParent()->GetParent()->GetParent()->GetName().AsChar(); //e.g. 'bob' , 'ep1'
									String depotCategory = depotName + itemDef.m_category.AsString();
									m_itemsByDepot[ depotName ].PushBackUnique( nameItemName );
									m_itemsByDepotAndCategory[ depotCategory ].PushBackUnique( nameItemName );
								}
								else
								{
									String depotName = TXT("W3");
									String depotCategory = depotName + itemDef.m_category.AsString();
									m_itemsByDepot[ depotName ].PushBackUnique( nameItemName );
									m_itemsByDepotAndCategory[ depotCategory ].PushBackUnique( nameItemName );
								}
#endif //RED_FINAL_BUILD
							}
						}
						else
						{
							ERR_GAME( TXT("CDefinitionsManager: failed to load item '%ls'"), itemName.AsChar() );
							XML_ERROR( xmlFilePath.AsChar(), TXT( "ERROR: Couldn't find name attribute in '%ls' node. Previous node name: %ls" ), NODE_ITEM.AsChar(), previousNodeName.AsChar() );
						}

						xmlReader.EndNode(); // NODE_ITEM
					}

					xmlReader.EndNode( false ); // NODE_ITEMS
				}

				// Item extensions
				if( xmlReader.BeginNode( NODE_ITEMS_EXTENSIONS, true ) )
				{
					SearchForDisallowedElements( xmlReader, NODE_ITEMS_EXTENSIONS, xmlFilePath );

					while( xmlReader.BeginNode( NODE_ITEM_EXTENSION ) )
					{
						// Item attributes
						SItemExtDefinition itemExtDef;
						itemExtDef.m_creatorTag = creatorTag;

						Bool ok = xmlReader.Attribute( ATTR_NAME, itemName );
						
						ParseItemExtDefinition( xmlReader, itemExtDef.m_extDefaultProperties, itemName );

						if ( xmlReader.BeginNode( NODE_PLAYER_OVERRIDE_EXTENSION, true ) )
						{
							if ( xmlReader.GetChildCount() > 0 )
							{
								SearchForDisallowedElements( xmlReader, NODE_PLAYER_OVERRIDE_EXTENSION, xmlFilePath, true );

								SItemExtDefinition::ExtOverridableProperties* extOverridableProps = new SItemExtDefinition::ExtOverridableProperties( itemExtDef.m_extDefaultProperties );
								itemExtDef.m_extPlayerOverridableProperties = extOverridableProps;

								ParseItemExtDefinition( xmlReader, *itemExtDef.m_extPlayerOverridableProperties, itemName );
							}
							xmlReader.EndNode( false ); // NODE_PLAYER_OVERRIDE_EXTENSION
						}

						if( ok )
						{
							CName nameItemName = CName( itemName );
							previousNodeName = itemName;						
							
							TDynArray<SItemExtDefinition*>** itemExtDefinitions = m_itemExtDefinitions.FindPtr( nameItemName );
							if( itemExtDefinitions == nullptr )
							{
								m_itemExtDefinitions.Set( nameItemName, new TDynArray<SItemExtDefinition*>() );
								itemExtDefinitions = m_itemExtDefinitions.FindPtr( nameItemName );								
							}

							(*itemExtDefinitions)->PushBack( new SItemExtDefinition( itemExtDef ) );

							SItemDefinition* itemDefinition = m_itemDefinitions.FindPtr( nameItemName );
							if( itemDefinition )
							{
								SItemExtDefinition* itemExtDefinition = (*itemExtDefinitions)->Back();
								AttachItemExtToItem( itemDefinition, itemExtDefinition );
							}						
						}
						else
						{
							ERR_GAME( TXT("CDefinitionsManager: failed to load item extension'%ls'"), itemName.AsChar() );
							XML_ERROR( xmlFilePath.AsChar(), TXT( "ERROR: Couldn't find name attribute in '%ls' node. Previous node name: %ls" ), NODE_ITEM_EXTENSION.AsChar(), previousNodeName.AsChar() );
						}

						xmlReader.EndNode(); // NODE_ITEM_EXT
					}

					xmlReader.EndNode( false ); // NODE_ITEM_EXTENSIONS
				}				

				// Item Sets
				if( xmlReader.BeginNode( NODE_ITEM_SETS, true ) )
				{
					SearchForDisallowedElements( xmlReader, NODE_ITEM_SETS, xmlFilePath );

					while( xmlReader.BeginNode( NODE_ITEM_SET ) )
					{
						SearchForDisallowedElements( xmlReader, NODE_ITEM_SET, xmlFilePath );

						SItemSet set;
						set.m_creatorTag = creatorTag;

						if ( xmlReader.Attribute( ATTR_NAME, temp ) )
						{
							set.m_name = CName( temp );
							previousNodeName = temp;
						}
						else
						{
							XML_ERROR( xmlFilePath.AsChar(), TXT( "Couldn't find name attribute in '%ls' node. Previous node name: %ls" ), NODE_ITEM_SET.AsChar(), previousNodeName.AsChar() );
						}

						if( xmlReader.BeginNode( NODE_PARTS_SET, true ) )
						{
							SearchForDisallowedElements( xmlReader, NODE_PARTS_SET, xmlFilePath );
							while( xmlReader.BeginNode( NODE_PART ) )
							{
								if ( xmlReader.Value( temp ) && !temp.Empty() )
								{
									set.m_parts.PushBack(CName( temp ));
								}
								xmlReader.EndNode(); // NODE_PART
							}
							xmlReader.EndNode( false ); // NODE_PARTS_SET
						}

						if( xmlReader.BeginNode( NODE_BASE_ABILITIES, true ) )
						{
							SearchForDisallowedElements( xmlReader, NODE_BASE_ABILITIES, xmlFilePath );

							while( xmlReader.BeginNode( NODE_A ) )
							{
								if( xmlReader.Value( temp ) && !temp.Empty() )
								{
									set.m_abilities.PushBack(CName( temp ));
								}
								else
								{
									ReportParseErrorValue( NODE_BASE_ABILITIES, set.m_name.AsString(), xmlFilePath );
								}
								xmlReader.EndNode(); // NODE_A
							}
							xmlReader.EndNode( false ); // NODE_BASE_ABILITIES
						}

						auto foundItemSet = m_itemSetsMap.Find( set.m_name );
						if(      foundItemSet != m_itemSetsMap.End()
							&& (*foundItemSet).m_second.m_creatorTag != creatorTag )
						{
							ERR_GAME( TXT("CDefinitionsManager: try to override '%ls' item set with definition from another creator (e.g. DLC mounter)"), set.m_name.AsChar()  );
						}
						else
						{
							m_itemSetsMap.Insert( set.m_name, set );
						}						

						xmlReader.EndNode(); // NODE_ITEM_SET
					}
					xmlReader.EndNode( false ); // NODE_ITEM_SETS
				}

				// Usable items types
				if( xmlReader.BeginNode( NODE_USABLE_ITEM_TYPES, true ) )
				{
					while( xmlReader.BeginNode( NODE_USABLE_ITEM ) )
					{
						String itemName;
						String usableType;
						if ( xmlReader.Attribute( ATTR_NAME, itemName ) && xmlReader.Attribute( ATTR_TYPE, usableType ) )
						{
							SUsableItem usableItem;
							usableItem.m_creatorTag = creatorTag;
							usableItem.ParseUsableItemType( usableType );
							m_usableItemTypes[ CName( itemName) ] = usableItem;
						}
						xmlReader.EndNode(); // NODE_USABLE_ITEM
					}
					xmlReader.EndNode( false ); // NODE_USABLE_ITEM_TYPES
				}

				GetLootDefinitions()->ReadNode( &xmlReader, creatorTag );

				xmlReader.EndNode( false ); // NODE_DEFINITIONS
			}

			if ( xmlReader.BeginNode( NODE_CUSTOM, true ) )
			{
				while( xmlReader.BeginNextNode() )
				{
					SCustomNode newCustomNode;
					newCustomNode.m_creatorTag= creatorTag;
					if ( LoadCustomNode( xmlReader, newCustomNode ) )
					{
						m_customNodes.PushBack( newCustomNode );
					}

					xmlReader.EndNode(); // Base custom node
				}

				xmlReader.EndNode( false ); // NODE_CUSTOM
			}
		}
		else
		{
			ERR_GAME( TXT( "CDefinitionsManager: file doesn't contain <redxml> node - %s" ), xmlFilePath.AsChar() );
			XML_ERROR( xmlFilePath.AsChar(), TXT( "File doesn't contain <redxml> node") );
		}
	}
	else
	{
		ERR_GAME( TXT( "CDefinitionsManager: cannot open '%ls'" ), xmlFilePath.AsChar() );
		XML_ERROR( TXT( "General" ), TXT( "Cannot open file %ls" ), xmlFilePath.AsChar() );
	}

	// Deletion after xmlReader destroyed
	if( file )
	{
		delete file;
		file = NULL;	
	}
}

void CDefinitionsManager::RemoveDefinitions( const Red::System::GUID& creatorTag )
{
	TDynArray<CName> m_definitionsToRemove;
	for( auto& ability : m_abilityDefinitions )
	{
		if( ability.m_second.m_creatorTag == creatorTag )
		{
			m_definitionsToRemove.PushBack( ability.m_first );
		}
	}

	for( auto& removeDefinitionName : m_definitionsToRemove )
	{
		m_abilityDefinitions.Erase( removeDefinitionName );
	}

	for( auto& itemExts : m_itemExtDefinitions )
	{
		TDynArray<SItemExtDefinition*>* itemExtsArray = itemExts.m_second;
		for ( Int32 i = itemExtsArray->SizeInt() - 1; i >= 0; --i )
		{
			SItemExtDefinition* itemExtDefinition = (*itemExtsArray)[i];
			if( itemExtDefinition->m_creatorTag == creatorTag )
			{
				itemExtsArray->Erase( itemExtsArray->Begin() + i );
				delete itemExtDefinition;
			}
		}
	}

	m_definitionsToRemove.Clear();

	for( auto& item : m_itemDefinitions )
	{
		if( item.m_second.m_creatorTag == creatorTag )
		{
			m_definitionsToRemove.PushBack( item.m_first );
		}
	}

	for( auto removeDefinitionName : m_definitionsToRemove )
	{
		auto itemToRemove = m_itemDefinitions.Find( removeDefinitionName );
		if( itemToRemove != m_itemDefinitions.End() )
		{
			TDynArray< CName >& categoryItems = m_itemsByCategory[ itemToRemove.Value().m_category ];

			CName* foundCategoryItem = categoryItems.FindPtr( removeDefinitionName );

			if( foundCategoryItem != NULL )
			{
				categoryItems.Erase( foundCategoryItem );
			}

 			m_itemDefinitions.Erase( itemToRemove );
		}		
	}
	
	m_definitionsToRemove.Clear();

	for( auto& itemSet : m_itemSetsMap )
	{
		if( itemSet.m_second.m_creatorTag == creatorTag )
		{
			m_definitionsToRemove.PushBack( itemSet.m_first );
		}
	}

	for( auto& removeDefinitionName : m_definitionsToRemove )
	{
		m_itemSetsMap.Erase( removeDefinitionName );
	}	

	GetLootDefinitions()->RemoveDefinitions( creatorTag );

	for ( Int32 i = m_customNodes.SizeInt() - 1; i >= 0; --i )
	{
		if( m_customNodes[i].m_creatorTag == creatorTag )
		{
			m_customNodes.Erase( m_customNodes.Begin() + i );
		}
	}

	m_definitionsToRemove.Clear();

	for( auto& usableItem : m_usableItemTypes )
	{
		if( usableItem.m_second.m_creatorTag == creatorTag )
		{
			m_definitionsToRemove.PushBack( usableItem.m_first );
		}
	}

	for( auto& removeDefinitionName : m_definitionsToRemove )
	{
		m_usableItemTypes.Erase( removeDefinitionName );
	}
}

void CDefinitionsManager::ReportParseErrorValue( const String& nodeName, const String& context, const String& filepath )
{
	String err = String::Printf( TXT( "CDefinitionsManager : value parse error for node '%ls', context '%ls'" ), nodeName.AsChar(), context.AsChar() );
	XML_ERROR( filepath.AsChar(), err.AsChar() );
	ERR_GAME( err.AsChar() );
}

void CDefinitionsManager::ReportParseErrorAttr( const String& attrName, const String& nodeName, const String& context, const String& filepath )
{
	String err = String::Printf( TXT( "CDefinitionsManager : attribute parse error for node '%ls', attribute '%ls', context '%ls'" ), nodeName.AsChar(), attrName.AsChar(), context.AsChar() );
	XML_ERROR( filepath.AsChar(), err.AsChar() );
	ERR_GAME( err.AsChar() );
}

const SItemDefinition* CDefinitionsManager::GetItemDefinition( CName itemName ) const
{
	TItemDefMap::const_iterator iter = m_itemDefinitions.Find( itemName );	
	if( iter != m_itemDefinitions.End() )
	{
		const SItemDefinition& def = iter->m_second;
		return &def;
	}
	else
	{
		return NULL;
	}
}

void CDefinitionsManagerAccessor::funcGetItemRecyclingParts( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	TDynArray< SItemParts >& resultArr = *static_cast< TDynArray< SItemParts >* >( result );

	m_manager->GetItemRecyclingParts( itemName, resultArr );
}

const SAbility*	CDefinitionsManager::GetAbilityDefinition( CName abilityName ) const
{
	TAbilityDefMap::const_iterator iter = m_abilityDefinitions.Find( abilityName );
	if( iter != m_abilityDefinitions.End() )
	{
		const SAbility& def = iter->m_second;
		return &def;
	}
	else
	{
		return NULL;
	}
}

const String& CDefinitionsManager::TranslateTemplateName( const String& shortName ) const
{
	THashMap< String, String >::const_iterator iter = m_templateNameMap.Find( shortName );
	if( iter == m_templateNameMap.End() )
	{
		return String::EMPTY;
	}
	else
	{
		return iter->m_second;
	}
}

void CDefinitionsManager::GetItemList( TDynArray< CName >& items ) const
{
	for( TItemDefMap::const_iterator iter=m_itemDefinitions.Begin(), end = m_itemDefinitions.End(); iter != end; ++iter )
	{
		items.PushBack( iter->m_first );
	}
}


void CDefinitionsManager::GetAbilitiesList( TDynArray< CName >& abilities ) const
{
	for( auto abil=m_abilityDefinitions.Begin(), end = m_abilityDefinitions.End(); abil != end; ++abil )
	{
		abilities.PushBack( abil->m_first );
	}
}

void CDefinitionsManager::GetUniqueContainedAbilities( const TDynArray< CName > & abilities, TDynArray< CName > & containedAbilities ) const
{
	containedAbilities.ClearFast();
	for( auto it = abilities.Begin(), end = abilities.End(); it != end; ++it )
	{
		const SAbility* ability = GetAbilityDefinition( *it );
		if ( ability != nullptr )
		{
			containedAbilities.PushBackUnique( ability->m_abilities );
		}
	}
}

void CDefinitionsManager::GetAbilityTags( CName abilityName, TDynArray< CName > & tags ) const
{
	const SAbility* ability = GetAbilityDefinition( abilityName );
	if( ability )
	{
		tags = ability->m_tags.GetTags();
	}	
}

Bool CDefinitionsManager::AbilityHasTag( CName abilityName, CName tag ) const
{
	const SAbility* ability = GetAbilityDefinition( abilityName );
	if ( ability != nullptr )
	{
		return ability->m_tags.HasTag( tag );
	}
	return false;
}

Bool CDefinitionsManager::AbilityHasTags( CName abilityName, const TDynArray< CName > & tags, Bool all ) const
{
	const SAbility* ability = GetAbilityDefinition( abilityName );
	if ( ability != nullptr )
	{
		if ( all )
		{
			return TagList::MatchAll( ability->m_tags, tags );
		}
		else
		{
			return TagList::MatchAny( ability->m_tags, tags );
		}
	}
	return false;
}

void CDefinitionsManager::GetItemCategories( TDynArray< CName >& categories ) const
{
	categories.Clear();
	categories = m_itemCategories;
}

void CDefinitionsManager::GetTemplateFilesList( TDynArray< String >& templates ) const
{
	for ( auto iter = m_templateNameMap.Begin(); iter != m_templateNameMap.End(); ++iter )
	{
		templates.PushBack( iter->m_second );
	}
}

void CDefinitionsManager::ApplyAttributeModifierForAbility( SAbilityAttributeValue& outModifier, const SAbility& ability, const CName& attrName, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const
{
	// For all attributes modified by ability
	const Uint32 s = ability.m_attributes.Size();
	for( Uint32 attr=0; attr<s; ++attr )
	{
		const SAbilityAttribute& attribute = ability.m_attributes[attr];
		if( attribute.m_name == attrName )
		{
			// Calculate modification
			Float modVal = ComputeAttributeValue( attribute, alwaysRandomSeed, staticRandomSeed );

			if( attribute.m_type == EAttrT_Multi )
			{
				outModifier.m_valueMultiplicative += modVal;
			}
			else if( attribute.m_type == EAttrT_Additive )
			{
				outModifier.m_valueAdditive += modVal;
			}
			else if( attribute.m_type == EAttrT_Base )
			{
				outModifier.m_valueBase += modVal;
			}
		}
	}
}

SAbilityAttributeValue CDefinitionsManager::CalculateAttributeValue( const TDynArray< CName >& abilities, CName attrName, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const
{
	SAbilityAttributeValue result;

	// For all abilities
	Uint32 s = abilities.Size();
	for( Uint32 i=0; i<s; i++ )
	{
		const SAbility * ability = GetAbilityDefinition( abilities[i] );
		if( ability )
		{
			ApplyAttributeModifierForAbility( result, *ability, attrName, alwaysRandomSeed, staticRandomSeed );
		}
		else
		{
			WARN_GAME( TXT("Ability definition '%ls', not found!"), abilities[i].AsString().AsChar() );
		}
	}

	return result;
}

SAbilityAttributeValue CDefinitionsManager::CalculateAttributeValue( CName abilityName, CName attrName, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const
{
	SAbilityAttributeValue result;
	const SAbility * ability = GetAbilityDefinition( abilityName );
	if( ability )
	{
		ApplyAttributeModifierForAbility( result, *ability, attrName, alwaysRandomSeed, staticRandomSeed );
	}
	else
	{
		WARN_GAME( TXT("Ability definition '%ls', not found!"), abilityName.AsString().AsChar() );
	}
	return result;
}

void CDefinitionsManager::GetAbilityAttributeValue( CName abilityName, CName attributeName, SAbilityAttributeValue& outMin, SAbilityAttributeValue& outMax ) const
{
	outMin = SAbilityAttributeValue();
	outMax = SAbilityAttributeValue();

	const SAbility* ability = GetAbilityDefinition( abilityName );
	if ( ability != nullptr )
	{
		for ( auto i = ability->m_attributes.Begin() ; i != ability->m_attributes.End() ; ++i )
		{
			if ( i->m_name == attributeName )
			{

				outMin.m_valueMultiplicative	+= i->m_type == EAttrT_Multi ? i->m_min : 0.f;
				outMax.m_valueMultiplicative	+= i->m_type == EAttrT_Multi ? i->m_max : 0.f;
				outMax.m_valueAdditive			+= i->m_type == EAttrT_Additive ? i->m_max : 0.f;
				outMin.m_valueAdditive			+= i->m_type == EAttrT_Additive ? i->m_min : 0.f;
				outMax.m_valueBase				+= i->m_type == EAttrT_Base ? i->m_max : 0.f;
				outMin.m_valueBase				+= i->m_type == EAttrT_Base ? i->m_min : 0.f;
			}
		}
	}
}

void CDefinitionsManager::GetAbilitiesAttributeValue( const TDynArray< CName > & abilitiesNames, CName attributeName, SAbilityAttributeValue& outMin, SAbilityAttributeValue& outMax, const TDynArray< CName > & tags ) const
{
	outMin = SAbilityAttributeValue();
	outMax = SAbilityAttributeValue();

	const Uint32 size = abilitiesNames.Size();
	const Uint32 tagsSize = tags.Size();
	for ( Uint32 i = 0; i < size; i++ )
	{
		if ( tagsSize == 0 || AbilityHasTags( abilitiesNames[ i ], tags, false ) )
		{
			SAbilityAttributeValue tmpMin;
			SAbilityAttributeValue tmpMax;
			GetAbilityAttributeValue( abilitiesNames[ i ], attributeName, tmpMin, tmpMax );
			outMin += tmpMin;
			outMax += tmpMax;
		}
	}
}

void CDefinitionsManager::GetAttributeModifiersNames( CName ability, TDynArray< CName >& modifiers ) const
{
	const SAbility* abilityDef = GetAbilityDefinition( ability );

	if ( !abilityDef )
	{
		ERR_GAME( TXT("CDefinitionsManager::GetAttributeModifiersNames - ability definition not found") );
		return;
	}

	modifiers.Clear();
	const TDynArray<SAbilityAttribute>& abilityAttributes = abilityDef->m_attributes;
	for ( Uint32 j=0; j<abilityAttributes.Size(); ++j )
	{
		modifiers.PushBackUnique( abilityAttributes[j].m_name );
	}
}

Uint32 CDefinitionsManager::CalculateAttributeModifiers( THashMap< CName, SAbilityAttributeValue >& outModifiers, const TDynArray< CName >& abilities, Int32 staticRandomSeed ) const
{
	Uint32 numModifications = 0;
	for ( Uint32 i=0; i<abilities.Size(); ++i )
	{
		numModifications += CalculateAttributeModifiers( outModifiers, abilities[i], staticRandomSeed );
	}
	return numModifications;
}

Uint32 CDefinitionsManager::CalculateAttributeModifiers( THashMap< CName, SAbilityAttributeValue >& outModifiers, CName ability, Int32 staticRandomSeed ) const
{
	const SAbility* abilityDef = GetAbilityDefinition( ability );

	if ( !abilityDef )
	{
		ERR_GAME( TXT("CDefinitionsManager::CalculateAttributeModifiers - ability '%ls' definition not found"), ability.AsString().AsChar() );
		return 0;
	}

	Uint32 numModifications = 0;
	Int32 alwaysRandomSeed = GEngine->GetRandomNumberGenerator().Get< Int32 >();
	const TDynArray<SAbilityAttribute>& abilityAttributes = abilityDef->m_attributes;

	for ( Uint32 j=0; j<abilityAttributes.Size(); ++j )
	{
		const SAbilityAttribute& attribute = abilityAttributes[j];
		const CName& attrName = attribute.m_name;
		if ( !outModifiers.KeyExist( attrName ) ) 
		{
			outModifiers.Insert( attrName, SAbilityAttributeValue() );
		}

		Float value = ComputeAttributeValue( attribute, alwaysRandomSeed, staticRandomSeed );
		SAbilityAttributeValue* modData = outModifiers.FindPtr( attrName );
		if ( modData )
		{
			if( attribute.m_type == EAttrT_Multi )
			{
				modData->m_valueMultiplicative += value;
			}
			else if( attribute.m_type == EAttrT_Additive )
			{
				modData->m_valueAdditive += value;
			}
			else if( attribute.m_type == EAttrT_Base )
			{
				modData->m_valueBase += value;
			}
		}

		++numModifications;
	}

	return numModifications;
}

Float CDefinitionsManager::ComputeAttributeValue( const SAbilityAttribute& attribute, Int32 alwaysRandomSeed, Int32 staticRandomSeed ) const
{
	float value = attribute.m_min;
	if( Abs<Float>(attribute.m_min - attribute.m_max ) > NumericLimits<Float>::Epsilon() )
	{
		Int32 seed = staticRandomSeed;
		if( attribute.m_alwaysRandom )
		{
			seed = alwaysRandomSeed;
		}

		Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( seed );
		value = noiseMaker.Get< Float >( attribute.m_min, attribute.m_max );
	}

	if( attribute.m_precision <= -1 )
	{
		return value;
	}
	if( attribute.m_precision == 0 )
	{
		return Red::Math::MRound( value );
	}
	const float precisionModifier = Red::Math::MPow( 10.0f, attribute.m_precision );
	return Red::Math::MRound( value * precisionModifier ) / precisionModifier;
}

const TDynArray< CName >& CDefinitionsManager::GetItemsOfCategory( CName category ) const
{
	TCategorizedItemsMap::const_iterator it = m_itemsByCategory.Find( category );
	if ( it == m_itemsByCategory.End() )
	{
		static TDynArray< CName > empty;
		return empty;
	}
	return it->m_second;
}

const TDynArray< CName >& CDefinitionsManager::GetItemsOfDepot( String depot ) const
{
#ifndef RED_FINAL_BUILD
	THashMap< String, TDynArray<CName> >::const_iterator it = m_itemsByDepot.Find( depot );
	if ( it == m_itemsByDepot.End() )
	{
#endif //RED_FINAL BUILD
		static TDynArray< CName > empty;
		return empty;
#ifndef RED_FINAL_BUILD
	}
	return it->m_second;
#endif //RED_FINAL_BUILD
}

const TDynArray< CName >& CDefinitionsManager::GetItemsOfDepotAndCategory( String depotCategory ) const
{
#ifndef RED_FINAL_BUILD
	THashMap< String, TDynArray<CName> >::const_iterator it = m_itemsByDepotAndCategory.Find( depotCategory );
	if ( it == m_itemsByDepotAndCategory.End() )
	{
#endif //RED_FINAL_BUILD
		static TDynArray< CName > empty;
		return empty;
#ifndef RED_FINAL_BUILD
	}
	return it->m_second;
#endif //RED_FINAL_BUILD
}

// This code won't work properly if you pass ingredient with 0 quantity, so please do not do it :)
const SItemDefinition* CDefinitionsManager::GetSchematicForIngredients( const THashMap< CName, Uint32 >& ingredients ) const
{
	Uint32 numIngredientsTypesGiven = ingredients.Size();
	if ( numIngredientsTypesGiven == 0 )
	{
		return NULL;
	}

	for ( TItemDefMap::const_iterator currDef = m_itemDefinitions.Begin(); currDef != m_itemDefinitions.End(); ++currDef )
	{
		const SItemDefinition & itemDef = currDef->m_second;
		if ( itemDef.m_ingredients.Size() != numIngredientsTypesGiven )
		{
			continue;
		}

		Bool matched = true;
		for ( Uint32 ingrIdx = 0; ingrIdx < itemDef.m_ingredients.Size(); ++ingrIdx )
		{
			// ZBR HACK : lower case names before definitions in XML files will be corrected
			const Uint32 * numFound = ingredients.FindPtr( CName( itemDef.m_ingredients[ ingrIdx ].m_itemName.AsString().ToLower() ) );
			Uint32   numGiven = numFound ? *numFound : 0;
			Uint32   numAsked = itemDef.m_ingredients[ ingrIdx ].m_quantity;
			if ( numGiven != numAsked )
			{
				matched = false;
				break;
			}
		}
		if ( matched )
		{
			return &itemDef;
		}
	}

	return NULL;
}

CLootDefinitions* CDefinitionsManager::GetLootDefinitions() const
{
	if ( m_lootDefinitions == NULL )
	{
		m_lootDefinitions = GCommonGame->CreateLootDefinitions();
	}
	return m_lootDefinitions;
}

void CDefinitionsManager::GetItemRecyclingParts( CName itemName, TDynArray< SItemParts >& resultArr ) const
{
	const SItemDefinition* itemDef = GetItemDefinition( itemName );
	if ( itemDef != nullptr )
	{
		typedef TDynArray< SItemDefinition::SItemPartsEntry > TItemPartsArray;
		const TItemPartsArray& itemParts = itemDef->GetRecyclingParts();

		for ( TItemPartsArray::const_iterator it = itemParts.Begin(); it != itemParts.End(); ++it )
		{
			SItemParts parts;
			parts.m_itemName = it->m_name;
			parts.m_quantity = static_cast< Int32 >( it->m_count );
			resultArr.PushBack( parts );
		}
	}
}

void CDefinitionsManager::TestWitchcraft() const
{
	CInventoryComponent::SAddItemInfo addItemInfo;
	addItemInfo.m_informGui = false;

	CInventoryComponent* invComp = GCommonGame->GetPlayer()->GetInventoryComponent();

	if ( invComp != nullptr )
	{
		invComp->ClearInventory();

		TDynArray< CName > categories;
		GetItemCategories( categories );

		for ( Uint32 i=0; i < categories.Size(); ++i )
		{
			const TDynArray< CName >& itemNames = GetItemsOfCategory( categories[ i ] );

			for ( Uint32 j = 0; j < itemNames.Size(); ++j )
			{
				const SItemDefinition* itemDef = GetItemDefinition( itemNames[ j ] );
				if ( itemDef != nullptr )
				{
					TDynArray< SItemUniqueId > ids = invComp->AddItem( itemNames[ j ], addItemInfo );
				
					if( !ids.Empty() )
					{
						const SInventoryItem& item = *invComp->GetItem( ids[0] );

// 						RED_LOG
// 						(
// 							RED_LOG_CHANNEL( WITCHCRAFT ),
// 							TXT( "C: %-20s R: %-10.2f BP: %-10i W(Kg): %-10.2f Item: %s Name:%s Desc:%s" ),
// 							itemDef->GetCategory().AsChar(),
// 							invComp->TotalItemStats( item ),
// 							invComp->GetItemPrice( item ),
// 							invComp->GetItemWeight( item ),
// 							itemNames[ j ].AsChar(),
// 							itemDef->GetLocalizationKeyName().AsChar(),
// 							SLocalizationManager::GetInstance().GetStringByStringKeyCachedWithFallback( itemDef->GetLocalizationKeyName() ).AsChar()
// 						);

						if ( !SLocalizationManager::GetInstance().GetStringByStringKeyCachedWithFallback( itemDef->GetLocalizationKeyName() ).Empty() )
						{
//  							CName slAttributeName = TXT( "SlashingDamage" );
// 							CName siAttributeName = TXT( "SilverDamage" );
// 							Int32 alwaysRandomSeed = GEngine->GetRandomNumberGenerator().Get< Int32 >();
// 							finalValue = defMgr->CalculateAttributeValue( abilityName, attributeName, alwaysRandomSeed, baseItem->GetStaticRandomSeed() );
// 							
							String localizedText = itemDef->GetLocalizationKeyName() + String( TXT( "_text" ) );

							RED_LOG
							(
								RED_LOG_CHANNEL( WITCHCRAFT ),
								TXT( "%s\t %i\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %s\t %s\t %s\t %s" ),
								itemDef->GetCategory().AsChar(),
								invComp->GetItemPrice( item ),
								invComp->GetItemWeight( item ),
								invComp->TotalItemStats( item ),
								item.GetAttribute( CName( TXT("BludgeoningDamage") ) ).m_valueBase,
								item.GetAttribute( CName( TXT("PiercingDamage") ) ).m_valueBase,
								item.GetAttribute( CName( TXT("SlashingDamage") ) ).m_valueBase,
								item.GetAttribute( CName( TXT("SilverDamage") ) ).m_valueBase,
								item.GetAttribute( CName( TXT("armor") ) ).m_valueBase,
								itemNames[ j ].AsChar(),
								itemDef->GetLocalizationKeyName().AsChar(),
								SLocalizationManager::GetInstance().GetStringByStringKeyCachedWithFallback( itemDef->GetLocalizationKeyName() ).AsChar(),
								SLocalizationManager::GetInstance().GetStringByStringKeyCachedWithFallback( localizedText ).AsChar()
							);
						}
// 
// 						RED_LOG
// 						(
// 							RED_LOG_CHANNEL( WITCHCRAFT ),
// 							TXT( "%s\t %s\t BP: %-10i W(Kg): %-10.2f" ),
// 							itemDef->GetLocalizationKeyName().AsChar(),
// 							SLocalizationManager::GetInstance().GetStringByStringKeyCachedWithFallback( itemDef->GetLocalizationKeyName() ).AsChar(),
// 							invComp->GetItemPrice( item ),
// 							invComp->GetItemWeight( item )
// 						);
					}
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( ">>>>> ERROR >>>>> Witchcraft cannot find Item Definition: %s " ), itemNames[ j ].AsChar() );
				}
			}
		}
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( ">>>>> ERROR >>>>> Witchcraft cannot find Player Inventory Component." ) );
	}

	RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( "__________> WITCHCRAFT REPORT COMPLETE <__________" ) );
}

void CDefinitionsManager::ValidateLootDefinitions( bool listAllItemDefs ) const
{
	GetLootDefinitions()->ValidateLootDefinitions( listAllItemDefs );
}

void CDefinitionsManager::ValidateRecyclingParts( bool listAllItemDefs ) const
{
	TDynArray< CName > categories;
	GetItemCategories( categories );

	for ( Uint32 i=0; i < categories.Size(); ++i )
	{
		const TDynArray< CName >& itemNames = GetItemsOfCategory( categories[ i ] );
		
		for ( Uint32 j=0; j < itemNames.Size(); ++j )
		{
			const SItemDefinition* itemDef = GetItemDefinition( itemNames[j] );
			if ( itemDef != nullptr )
			{
				const TDynArray< SItemDefinition::SItemPartsEntry >& recyclingParts = itemDef->GetRecyclingParts();

				for ( Uint32 k=0; k < recyclingParts.Size(); ++k )
				{
					const SItemDefinition* partDef = GetItemDefinition( recyclingParts[k].m_name );
					
					if ( nullptr == partDef )
					{
						RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( ">>>>> ERROR >>>>> %s Recycling Parts cannot find Item Definition: %s " ), itemNames[j].AsChar(), recyclingParts[k].m_name.AsChar() );
					}
					else if ( listAllItemDefs )
					{
						RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( "%s Recycling Parts found Item Definition: %s" ), itemNames[j].AsChar(), recyclingParts[k].m_name.AsChar() );
					}
				}
			}
		}
	}

	RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( "__________> RECYCLING PARTS REPORT COMPLETE <__________" ) );
}

void CDefinitionsManager::ValidateCraftingDefinitions( bool listAllItemDefs ) const
{
	bool alchemyDef = false;

	const TDynArray< SCustomNode >& rootNodes = GetCustomNodes();

	Int32 rootNodesCount = rootNodes.SizeInt();
	for ( Int32 rootNodeIndex = 0; rootNodeIndex < rootNodesCount; ++rootNodeIndex )
	{
		if ( listAllItemDefs && rootNodes[ rootNodeIndex ].m_nodeName == CNAME( crafting_schematics ) )
		{
			RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( "__________________________________________________CRAFTING SCHEMATICS" ) );
		}
		else if ( listAllItemDefs && rootNodes[ rootNodeIndex ].m_nodeName == CNAME( alchemy_recipes ) )
		{
			RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( "__________________________________________________ALCHEMY RECIPES" ) );
			alchemyDef = true;
		}
		else
		{
			continue;
		}
		
		const SCustomNode& rootNode = rootNodes[ rootNodeIndex ];

		Int32 subNodeCount = rootNode.m_subNodes.SizeInt();
		for ( Int32 subNodeIndex = 0; subNodeIndex < subNodeCount; ++subNodeIndex )
		{
			// look for specific attribute with given name and value
			const SCustomNode& schematicNode = rootNode.m_subNodes[ subNodeIndex ];

			CName craftedItem_name;

			Int32 attributeCount = schematicNode.m_attributes.SizeInt();
			for ( Int32 attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex )
			{
				if ( alchemyDef )
				{
					if ( schematicNode.m_attributes[ attributeIndex ].GetAttributeName() == TXT( "name_name" ) )
					{
						craftedItem_name = schematicNode.m_attributes[ attributeIndex ].GetValueAsCName();
					}
				}
				else
				{
					if ( schematicNode.m_attributes[ attributeIndex ].GetAttributeName() == CNAME( craftedItem_name ) )
					{
						craftedItem_name = schematicNode.m_attributes[ attributeIndex ].GetValueAsCName();
					}
				}
			}
				
			if ( listAllItemDefs )
			{
				if ( alchemyDef )
				{
					RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT("Recipe: %s" ), craftedItem_name.AsChar() );
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT("Schematic: %s" ), craftedItem_name.AsChar() );
				}
			}

			const TDynArray< SCustomNode >& ingredientsNode = rootNode.m_subNodes[ subNodeIndex ].m_subNodes;
			Int32 ingredientsCount = ingredientsNode[0].m_subNodes.SizeInt();
			for ( Int32 ingredientsIndex = 0; ingredientsIndex < ingredientsCount; ++ingredientsIndex )
			{
				CName item_name;
				Int32 item_quantity;
				
				const SCustomNode& ingredient = ingredientsNode[0].m_subNodes[ingredientsIndex];
				
				Int32 attributeCount = ingredient.m_attributes.SizeInt();
				for ( Int32 attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex )
				{
					if ( ingredient.m_attributes[ attributeIndex ].GetAttributeName() == CNAME( quantity ) )
					{
						ingredient.m_attributes[ attributeIndex ].GetValueAsInt( item_quantity );
					}
					if ( ingredient.m_attributes[ attributeIndex ].GetAttributeName() == CNAME( item_name ) )
					{
						item_name = ingredient.m_attributes[ attributeIndex ].GetValueAsCName();
					}
				}

				const SItemDefinition* itemDef = GetItemDefinition( item_name );
				if ( nullptr == itemDef )
				{
					if ( alchemyDef )
					{
						RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( ">>>>> ERROR >>>>> Alchemy Recipe: %s has Ingredient: %s that does not exist." ), craftedItem_name.AsChar(), item_name.AsChar() );
					}
					else
					{
						RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( ">>>>> ERROR >>>>> Crafting Schematic: %s has Ingredient: %s that does not exist." ), craftedItem_name.AsChar(), item_name.AsChar() );
					}
				}
				else if ( listAllItemDefs )
				{
					RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( "%i: %s" ), item_quantity, item_name.AsChar() );
				}
			}
		}
	}

	RED_LOG( RED_LOG_CHANNEL( WITCHCRAFT ), TXT( "__________> CRAFTING DEFINITIONS REPORT COMPLETE <__________" ) );
}

void CDefinitionsManager::AddAllItems( CName category , String depot , Bool invisibleItems ) const
{
	if ( category == CName::NONE && depot == TXT("") )
	{
		TDynArray< CName > categories;
		GetItemCategories( categories );

		//there's no point in accessing items in the following categories ever (at least from UI perspective):
		String ignoreCats[] = { TXT("steel_scabbards") , TXT("silver_scabbards") , TXT("fist") , TXT("hair") , TXT("trophy") , TXT("head") , TXT("secondary") , 
								TXT("monster_weapon") , TXT("horse_tail") , TXT("horse_hair") , TXT("horse_reins") , TXT("work_secondary") , 
								TXT("lute") , TXT("work") , TXT("cooking_recipe") , TXT("halberd2h") , TXT("bow") , TXT("spear2h") , TXT("staff2h") , 
								TXT("axe2h") , TXT("shield") , TXT("axe1h") , TXT("cleaver1h") ,TXT("blunt1h") , TXT("hammer2h") , TXT("polearm") };

		RED_LOG( Script, TXT("Please specify a category from the following: ") );
		for ( Uint32 i=0; i < categories.Size(); ++i )
		{
			Bool noShow = false;

			if ( !invisibleItems )
			{
				for ( Uint32 j=0, jmax = sizeof(ignoreCats)/sizeof(ignoreCats[0]); j<jmax; ++j )
				{
					if ( categories[i].AsString() == ignoreCats[j] )
					{
						noShow = true;
						break;
					}	
				}
			}

			if ( !noShow )
			{
				RED_LOG( Script, TXT("%ls"), categories[i].AsChar() );
			}
		}
	}
	else
	{
		CInventoryComponent* invComp = GCommonGame->GetPlayer()->GetInventoryComponent();
		CInventoryComponent::SAddItemInfo addItemInfo;

		if ( invComp != nullptr )
		{		
			TDynArray< CName > items;

			if ( depot != TXT("") && category != CName::NONE )
			{
				items = GetItemsOfDepotAndCategory( depot + category.AsString() );
			}
			else if ( depot != TXT("") )
			{
				items = GetItemsOfDepot( depot );
			}
			else
			{
				items = GetItemsOfCategory( category );
			}

			for( Uint32 i=0; i < items.Size(); ++i )
			{
				const SItemDefinition* itemDef = GetItemDefinition( items[i] );
				TDynArray< CName > itemTags = itemDef->GetItemTags();

				Bool noShow = false;

				for ( Uint32 j=0, jmax=itemTags.Size(); j<jmax; ++j)
				{
					if ( itemTags[j].AsChar() == TXT("NoShow") )
					{
						noShow = true;
						break;
					}
				}
				
				if ( !noShow )
				{
					addItemInfo.m_quantity = 1;
					addItemInfo.m_informGui = false;
					addItemInfo.m_markAsNew = false;
					invComp->AddItem( items[i], addItemInfo );
				}
			}
		}
	}
}

void CDefinitionsManager::FillActorAnimStatesMap( THashMap< CName, EActorAnimState >& aasMap )
{
	CEnum* actorAnimStateType = SRTTI::GetInstance().FindEnum( CNAME( EActorAnimState ) );
	const TDynArray< CName  >& aasOptions = actorAnimStateType->GetOptions();

	aasMap.Clear();
	for ( Uint32 i=0; i<aasOptions.Size(); ++i )
	{
		VERIFY( aasMap.Insert( aasOptions[i], EActorAnimState( i ) ) );
	}
}

EActorAnimState CDefinitionsManager::MapNameToActorAnimState( CName aasName )
{
	const EActorAnimState* statePtr = m_actorAnimStates.FindPtr( aasName );
	return statePtr ? *statePtr : AAS_Default;
}

Bool CDefinitionsManager::CategoryExists( CName category ) const
{
	return m_itemCategories.Exist( category );
}

const SItemSet* CDefinitionsManager::GetItemSetDefinition( CName itemSetName ) const
{
    TItemSetsMap::const_iterator iter = m_itemSetsMap.Find( itemSetName );	
    if( iter != m_itemSetsMap.End() )
    {
        const SItemSet& def = iter->m_second;
        return &def;
    }
    else
    {
        return NULL;
    }
}

const SItemStat* CDefinitionsManager::GetItemStatDefinition( CName itemName ) const
{
	TItemStatMap::const_iterator iter = m_itemStatMap.Find( itemName );
	if ( iter != m_itemStatMap.End() )
	{
		const SItemStat& def = iter->m_second;
		return &def;
	}
	
	return NULL;
}

const SItemDamageCurve* CDefinitionsManager::GetItemDamageCurve( CName categoryName ) const
{
	TItemDamageCurveMap::const_iterator iter = m_itemDamageCurveMap.Find( categoryName );
	if ( iter != m_itemDamageCurveMap.End() )
	{
		const SItemDamageCurve& def = iter->m_second;
		return &def;
	}

	return NULL;
}

const SItemTagModifier* CDefinitionsManager::GetItemTagModifier( CName itemTag ) const
{
	TItemTagModifierMap::const_iterator iter = m_itemTagModifierMap.Find( itemTag );
	if ( iter != m_itemTagModifierMap.End() )
	{
		const SItemTagModifier& def = iter->m_second;
		return &def;
	}

	return NULL;
}

Float CDefinitionsManager::GetItemWeight( CName itemName )
{
	const SItemDefinition* itemDefinition = GetItemDefinition( itemName );

	if ( itemDefinition != nullptr )
	{
		return itemDefinition->GetWeight();
	}

	// No item definition
	return -1;
}

const SIngredientCategory* CDefinitionsManager::GetIngredientDefinition( CName ingredientName ) const
{
    TIngredientCategory::const_iterator iter = m_ingredientsCategoriesMap.Find( ingredientName );	
    if( iter != m_ingredientsCategoriesMap.End() )
    {
        const SIngredientCategory& def = iter->m_second;
        return &def;
    }
    else
    {
        return NULL;
    }
}

void CDefinitionsManager::GetIngredientCategories( TDynArray< CName >& ingredientCategories ) const
{
    THashMap< CName, SIngredientCategory>::const_iterator iter = m_ingredientsCategoriesMap.Begin();

    for(;iter!= m_ingredientsCategoriesMap.End();++iter)
    {
        ingredientCategories.PushBack(iter->m_second.m_name);
    }
}

Bool CDefinitionsManager::LoadCustomNode( CXMLFileReader& xmlReader, SCustomNode& node )
{
	String temp;
	if( xmlReader.GetNodeName( temp ) )
	{
		temp.Trim();
		node.m_nodeName = CName( temp );

		const XMLNode* currentNode = xmlReader.GetCurrentNode();

		const XMLAttr* attribute = currentNode->first_attribute();
		while( attribute )
		{
			String attributeValue = String( attribute->value() );
			String attributeName = String( attribute->name() );
			attributeName.Trim();
			if ( attributeName.EndsWith( String( TXT("_name") ) ) )
			{
				node.m_attributes.PushBack( SCustomNodeAttribute( CName( attributeName ), CName( attributeValue ) ) );
			}
			else
			{
				node.m_attributes.PushBack( SCustomNodeAttribute( CName( attributeName ), attributeValue ) );
			}
			attribute = attribute->next_attribute();
		}

		if( xmlReader.Value( temp ) && !temp.Empty() )
		{
			TDynArray< String > values = temp.Split( TXT(",") );
			const TDynArray< String >::const_iterator end = values.End();
			TDynArray< String >::const_iterator it;
			for( it = values.Begin(); it != end; ++it )
			{
				if( !it->Empty() )
				{
					node.m_values.PushBack( CName( *it ) );
				}
			}
		}

		while( xmlReader.BeginNextNode() )
		{
			SCustomNode subNode;
			if( LoadCustomNode( xmlReader, subNode ) )
			{
				node.m_subNodes.PushBack( subNode );
			}

			xmlReader.EndNode();
		}
	}
	else
	{
		return false;
	}

	return true;
}

void CDefinitionsManagerAccessor::funcTestWitchcraft( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_manager != nullptr );
	m_manager->TestWitchcraft();
}

void CDefinitionsManagerAccessor::funcValidateCraftingDefinitions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, listAllItemDefs, false );
	FINISH_PARAMETERS;

	ASSERT( m_manager != nullptr );
	m_manager->ValidateCraftingDefinitions( listAllItemDefs );
}

void CDefinitionsManagerAccessor::funcValidateLootDefinitions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, listAllItemDefs, false );
	FINISH_PARAMETERS;

	ASSERT( m_manager != nullptr );
	m_manager->ValidateLootDefinitions( listAllItemDefs );
}

void CDefinitionsManagerAccessor::funcValidateRecyclingParts( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, listAllItemDefs, false );
	FINISH_PARAMETERS;

	ASSERT( m_manager != nullptr );
	m_manager->ValidateRecyclingParts( listAllItemDefs );
}

void CDefinitionsManagerAccessor::funcAddAllItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( CName, category, CName::NONE );
	GET_PARAMETER_OPT( String, depot, TXT("") );
	GET_PARAMETER_OPT( Bool, invisibleItems, false );
	FINISH_PARAMETERS;

	ASSERT( m_manager != nullptr );
	m_manager->AddAllItems( category , depot , invisibleItems );
}

EUsableItemType CDefinitionsManager::GetUsableItemType( CName itemName ) const
{
	SUsableItem usableItem;
	usableItem.m_type = UI_None;
	m_usableItemTypes.Find( itemName, usableItem );
	return usableItem.m_type;
}

void CDefinitionsManager::SetAlternativePathsMode( Bool set )
{
	//if ( m_useAlternativePaths == set )
	//{
	//	// do nothing, it's already done
	//	return;
	//}

	m_useAlternativePaths = set;
	ReloadAll();
}

void SUsableItem::ParseUsableItemType( const String& usableItemType )
{
	String lowType = usableItemType.ToLower();
	if ( lowType == TXT( "torch" ) ) { m_type = UI_Torch; return; }
	if ( lowType == TXT( "horn" ) ) { m_type = UI_Horn; return; }
	if ( lowType == TXT( "bell" ) ) { m_type = UI_Bell; return; }
	if ( lowType == TXT( "oillamp" ) || lowType == TXT( "oil_lamp") ) { m_type = UI_OilLamp; return; }
	if ( lowType == TXT( "mask" ) ) { m_type = UI_Mask; return; }
	if ( lowType == TXT( "fiendlure" ) || lowType == TXT( "fiend_lure" ) ) { m_type = UI_FiendLure; return; }
	if ( lowType == TXT( "meteor" ) ) { m_type = UI_Meteor; return; }
	if ( lowType == TXT( "censer" ) ) { m_type = UI_Censer; return; }
	if ( lowType == TXT( "apple" ) ) { m_type = UI_Apple; return; }
	if ( lowType == TXT( "cookie" ) ) { m_type = UI_Cookie; return; }
	if ( lowType == TXT( "basket" ) ) { m_type = UI_Basket; return; }
	m_type = UI_None; // by default
}
void CDefinitionsManager::ParseItemExtDefinition( CXMLFileReader& xmlReader, SItemExtDefinition::ExtOverridableProperties& extOverridableProperties, const String& itemName )
{
	String temp;
	// Variants
	if ( xmlReader.BeginNode( NODE_VARIANTS, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_VARIANTS, xmlReader.GetFileNameForDebug() );

		extOverridableProperties.m_variants.Clear();
		while ( xmlReader.BeginNode( NODE_VARIANT ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_VARIANT, xmlReader.GetFileNameForDebug() );

			SItemDefinition::SItemVariant variant;

			CName category;
			if ( ReadCName( xmlReader, ATTR_CATEGORY, category ) )
			{
				variant.m_categories.PushBack( category );
			}

			if ( xmlReader.Attribute( ATTR_TEMPLATE_EQUIP, temp ) )
			{
				variant.m_template = temp;
			}
			if ( xmlReader.Attribute( ATTR_ALL, temp ) )
			{
				FromString( temp, variant.m_expectAll );
			}
			while ( xmlReader.BeginNode( NODE_ITEM ) )
			{
				if ( xmlReader.Value( temp ) && !temp.Empty() )
				{
					variant.m_affectingItems.PushBack( CName( temp ) );
				}
				else
				{
					ReportParseErrorValue( NODE_ITEM, itemName, xmlReader.GetFileNameForDebug() );
				}
				xmlReader.EndNode(); // item
			}
			while ( xmlReader.BeginNode( NODE_ITEM_CATEGORY ) )
			{
				if ( xmlReader.Value( temp ) && !temp.Empty() )
				{
					variant.m_categories.PushBack( CName( temp ) );
				}
				else
				{
					ReportParseErrorValue( NODE_ITEM_CATEGORY, itemName, xmlReader.GetFileNameForDebug() );
				}
				xmlReader.EndNode(); // item category
			}

			extOverridableProperties.m_variants.PushBack( variant );

			xmlReader.EndNode(); 
		}
		extOverridableProperties.m_variants.Shrink();
		xmlReader.EndNode( false ); // Variant
	}

	// Collapse
	if ( xmlReader.BeginNode( NODE_COLLAPSE, true ) )
	{
		SearchForDisallowedElements( xmlReader, NODE_COLLAPSE, xmlReader.GetFileNameForDebug() );

		while ( xmlReader.BeginNode( NODE_CATEGORY_COND ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_CATEGORY_COND, xmlReader.GetFileNameForDebug() );

			CName category;
			if ( ReadCName( xmlReader, ATTR_NAME, category ) )
			{
				extOverridableProperties.m_collapseCond.m_collapseCategoryCond.PushBack( category );
			}
			xmlReader.EndNode(); 
		}

		while ( xmlReader.BeginNode( NODE_ITEM_COND ) )
		{
			SearchForDisallowedElements( xmlReader, NODE_ITEM_COND, xmlReader.GetFileNameForDebug() );
			CName item;
			if ( ReadCName( xmlReader, ATTR_NAME, item ) )
			{					
				if ( xmlReader.Attribute( ATTR_COLLAPSE, temp ) )
				{
					Bool collapse = false;
					if ( FromString( temp, collapse ) )
					{
						if( collapse )
						{
							extOverridableProperties.m_collapseCond.m_collapseItemCond.PushBack( item );
						}
						else
						{
							extOverridableProperties.m_collapseCond.m_uncollapseItemCond.PushBack( item );
						}
					}
					else
					{
						ReportParseErrorAttr( ATTR_COLLAPSE, xmlReader.GetCurrentNode()->name(), itemName, xmlReader.GetFileNameForDebug() );
					}
				}
			}

			xmlReader.EndNode(); 
		}
		extOverridableProperties.m_collapseCond.m_collapseItemCond.Shrink();
		extOverridableProperties.m_collapseCond.m_uncollapseItemCond.Shrink();
		extOverridableProperties.m_collapseCond.m_collapseCategoryCond.Shrink();
		xmlReader.EndNode( false ); // Variant
	}
}

void CDefinitionsManager::SearchForDisallowedElements( const CXMLFileReader& reader, const String& nodeName, const String& filepath, Bool allowOverrideProperties, Bool isWeapon ) const
{
#ifndef NO_DATA_VALIDATION
	TDynArray< String > allowedAttr, allowedChildnodes;
	if ( const TDynArray< String >* allowedAttributes = m_allowdAttributes.FindPtr( nodeName ) )
	{
		allowedAttr.PushBack( *allowedAttributes );
	}
	if ( const TDynArray< String >* allowedNodes = m_allowedNodes.FindPtr( nodeName ) )
	{
		allowedChildnodes.PushBack( *allowedNodes );
	}

	if ( allowOverrideProperties )
	{
		if ( const TDynArray< String >* opAttributes = m_allowdAttributes.FindPtr( TXT( "overriden" ) ) )
		{
			allowedAttr.PushBack( *opAttributes );
		}
		if ( const TDynArray< String >* opNodes = m_allowedNodes.FindPtr( TXT( "overriden" ) ) )
		{
			allowedChildnodes.PushBack( *opNodes );
		}
	}

	if ( isWeapon )
	{
		if ( const TDynArray< String >* weaponAttributes = m_allowdAttributes.FindPtr( TXT( "isWeapon" ) ) )
		{
			allowedAttr.PushBack( *weaponAttributes );
		}
	}

	TDynArray< String > foundUnallowedElements;
	if ( !allowedAttr.Empty() )
	{
		if( reader.HasNotAllowedAttributes( allowedAttr, foundUnallowedElements ) )
		{
			for ( const String& attribute : foundUnallowedElements )
			{
				XML_ERROR( filepath.AsChar(), TXT( "WARNING: Attribute '%ls' in node '%ls' seems to be unused." ), attribute.AsChar(), nodeName.AsChar() );
			}
		}
	}

	foundUnallowedElements.Clear();
	if ( !allowedChildnodes.Empty() )
	{
		if ( reader.HasNotAllowedChildnodes( allowedChildnodes, foundUnallowedElements ) )
		{
			for ( const String& node : foundUnallowedElements )
			{
				XML_ERROR( filepath.AsChar(), TXT( "WARNING: Node '%ls' in node '%ls' seems to be unused." ), node.AsChar(), nodeName.AsChar() );
			}
		}
	}
#endif
}

const SCustomNode* CDefinitionsManager::GetCustomNode( const CName& nodeName ) const
{
	TDynArray< SCustomNode >::const_iterator it = m_customNodes.Begin();
	TDynArray< SCustomNode >::const_iterator itEnd = m_customNodes.End();
	for ( ; it != itEnd; ++it )
	{
		if ( it->m_nodeName == nodeName )
		{
			return &(*it);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

void CDefinitionsManagerAccessor::funcGetIngredientCategoryElements( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( String, name, String::EMPTY );
    GET_PARAMETER_REF( TDynArray< String >, outNames, TDynArray< String >() );
    GET_PARAMETER_REF( TDynArray< Int32 >, outPriorites, TDynArray< Int32 >() );
    FINISH_PARAMETERS;

    const SIngredientCategory* ingredientCategory = m_manager->GetIngredientDefinition(CName( name ));

    
    if(ingredientCategory)
    {
        TDynArray< SIngredientCategoryElement >::const_iterator iter = ingredientCategory->m_elements.Begin();
        
        for(;iter!= ingredientCategory->m_elements.End();iter++)
        {
            outNames.PushBack(iter->m_name.AsString());
            outPriorites.PushBack(iter->m_priority);
        }
    }
    
}

void CDefinitionsManagerAccessor::funcGetItemAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( Bool, playerItem, false );
	GET_PARAMETER_REF( TDynArray< CName >, outAbilities, TDynArray< CName >() );
	FINISH_PARAMETERS;

	ASSERT( m_manager != NULL );
	const SItemDefinition* itemDefinition = m_manager->GetItemDefinition( itemName );

	if ( itemDefinition != nullptr )
	{
		const TDynArray< SItemDefinition::SItemAbility >& baseAbilities = itemDefinition->GetBaseAbilities( playerItem );
		TDynArray< SItemDefinition::SItemAbility >::const_iterator it = baseAbilities.Begin();
		TDynArray< SItemDefinition::SItemAbility >::const_iterator itEnd = baseAbilities.End();
		for ( ; it != itEnd; ++it )
		{
			outAbilities.PushBack( it->GetName() );
		}
	}
}

void CDefinitionsManagerAccessor::funcGetItemAbilitiesWithWeights( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( Bool, playerItem, false );
	GET_PARAMETER_REF( TDynArray< CName >, outAbilities, TDynArray< CName >() );
	GET_PARAMETER_REF( TDynArray< Float >, outWeights, TDynArray< Float >() );
	GET_PARAMETER_REF( Int32, minAbilities, 0 );
	GET_PARAMETER_REF( Int32, maxAbilities, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_manager != nullptr );
	const SItemDefinition* itemDefinition = m_manager->GetItemDefinition( itemName );

	if ( itemDefinition != nullptr )
	{
		const TDynArray< SItemDefinition::SItemAbility >& baseAbilities = itemDefinition->GetBaseAbilities( playerItem );
		TDynArray< SItemDefinition::SItemAbility >::const_iterator it = baseAbilities.Begin();
		TDynArray< SItemDefinition::SItemAbility >::const_iterator itEnd = baseAbilities.End();
		for ( ; it != itEnd; ++it )
		{
			outAbilities.PushBack( it->GetName() );
			outWeights.PushBack( it->GetChance() );
		}
		minAbilities = itemDefinition->GetMinAbilities( playerItem );
		maxAbilities = itemDefinition->GetMaxAbilities( playerItem );
	}
}

void CDefinitionsManagerAccessor::funcGetItemAttributesFromAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( Bool, playerItem, false );
	GET_PARAMETER( TDynArray< CName >, abilities, TDynArray< CName >() );
	GET_PARAMETER_REF( TDynArray< CName >, attributes, TDynArray< CName >() );
	FINISH_PARAMETERS;

	ASSERT( m_manager != NULL );
	const SItemDefinition* itemDefinition = m_manager->GetItemDefinition( itemName );
	if ( itemDefinition == NULL )
	{
		return;
	}

	const TDynArray< SItemDefinition::SItemAbility >& baseAbilities = itemDefinition->GetBaseAbilities( playerItem );
	for ( TDynArray< SItemDefinition::SItemAbility >::const_iterator abilityIter = baseAbilities.Begin();
		abilityIter != baseAbilities.End(); ++abilityIter )
	{
		if ( abilities.Exist( abilityIter->GetName() ) )
		{
			const SAbility* abilityDefinition = m_manager->GetAbilityDefinition( abilityIter->GetName() );

			if ( abilityDefinition == NULL )
			{
				continue;
			}

			for ( TDynArray< SAbilityAttribute >::const_iterator attributeIter = abilityDefinition->m_attributes.Begin();
				attributeIter != abilityDefinition->m_attributes.End(); ++attributeIter )
			{
				attributes.PushBackUnique( attributeIter->m_name );
			}
		}
	}
}

void CDefinitionsManagerAccessor::funcApplyItemAbilityAttributeModifier( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( Bool, playerItem, false );
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER( CName, attribute, CName::NONE );
	GET_PARAMETER_REF( SAbilityAttributeValue, outModifier, SAbilityAttributeValue() );
	FINISH_PARAMETERS;

	ASSERT( m_manager );

	const SItemDefinition* itemDefinition = m_manager->GetItemDefinition( itemName );
	if ( !itemDefinition )
	{
		return;
	}

	if ( itemDefinition->HasAbility( abilityName, playerItem ) )
	{
		const SAbility* abilityDefinition = m_manager->GetAbilityDefinition( abilityName );
		if( !abilityDefinition )
		{
			return;
		}

		m_manager->ApplyAttributeModifierForAbility( outModifier, *abilityDefinition, attribute, GEngine->GetRandomNumberGenerator().Get< Int32 >(), 0 );
	}
}

void CDefinitionsManagerAccessor::funcApplyAbilityAttributeModifier( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER( CName, attribute, CName::NONE );
	GET_PARAMETER_REF( SAbilityAttributeValue, outModifier, SAbilityAttributeValue() );
	FINISH_PARAMETERS;

	ASSERT( m_manager );

	const SAbility* abilityDefinition = m_manager->GetAbilityDefinition( abilityName );
	if( !abilityDefinition )
	{
		return;
	}

	m_manager->ApplyAttributeModifierForAbility( outModifier, *abilityDefinition, attribute, GEngine->GetRandomNumberGenerator().Get< Int32 >(), 0 );
}

void CDefinitionsManagerAccessor::funcGetCustomDefinition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, nodeName, CName::NONE );
	FINISH_PARAMETERS;

	ASSERT( m_manager );

	const TDynArray< SCustomNode >& nodes = m_manager->GetCustomNodes();
	const TDynArray< SCustomNode >::const_iterator end = nodes.End();
	TDynArray< SCustomNode >::const_iterator it;
	SCustomNode node;
	node.m_nodeName = nodeName;

	Uint32 subNodesCount = 0;
	Uint32 attributesCount = 0;
	Uint32 valueCount = 0;
	for( it = nodes.Begin(); it != end; ++it )
	{
		if( it->m_nodeName == nodeName )
		{
			subNodesCount += it->m_subNodes.Size();
			attributesCount += it->m_attributes.Size();
			valueCount += it->m_values.Size();
		}
	}
	node.m_subNodes.Reserve( subNodesCount );
	node.m_attributes.Reserve( attributesCount );
	node.m_values.Reserve( valueCount );

	for( it = nodes.Begin(); it != end; ++it )
	{
		if( it->m_nodeName == nodeName )
		{
			node.m_subNodes.PushBack( (*it).m_subNodes );
			// does that make sense? to put all attributes and values from all nodes to a single one?
			//   	 |
			//       V
			node.m_attributes.PushBack( (*it).m_attributes );
			node.m_values.PushBack( (*it).m_values );			
		}
	}

	RETURN_STRUCT( SCustomNode, node );
}

void CDefinitionsManagerAccessor::funcGetSubNodeByAttributeValueAsCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, outNode, SCustomNode() );
	GET_PARAMETER( CName, rootNodeName, CName::NONE );
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER( CName, attributeValue, CName::NONE );
	FINISH_PARAMETERS;

	const TDynArray< SCustomNode >& rootNodes = m_manager->GetCustomNodes();

	Int32 rootNodesCount = rootNodes.SizeInt();
	for ( Int32 rootNodeIndex = 0; rootNodeIndex < rootNodesCount; ++rootNodeIndex )
	{
		// look for root nodes i.e. 'alchemy_recipes'
		if ( rootNodes[ rootNodeIndex ].m_nodeName == rootNodeName )
		{
			const SCustomNode& rootNode = rootNodes[ rootNodeIndex ];
			Int32 subNodeCount = rootNode.m_subNodes.SizeInt();
			for ( Int32 subNodeIndex = 0; subNodeIndex < subNodeCount; ++subNodeIndex )
			{
				// look for specific attribute with given name and value
				const SCustomNode& subNode = rootNode.m_subNodes[ subNodeIndex ];
				Int32 attributeCount = subNode.m_attributes.SizeInt();
				for ( Int32 attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex )
				{
					if ( subNode.m_attributes[ attributeIndex ].GetAttributeName() == attributeName )
					{
						if ( subNode.m_attributes[ attributeIndex ].GetValueAsCName() == attributeValue )
						{
							outNode = subNode;
							RETURN_BOOL( true );
							return;
						}
					}
				}
			}
		}
	}

	RETURN_BOOL( false );
}

Bool SCustomNodeAttribute::GetValueAsInt(Int32 &val) const
{
	return FromString(m_attributeValue,val);
}

Bool SCustomNodeAttribute::GetValueAsFloat(Float &val) const
{
	return FromString(m_attributeValue,val);
}

Bool SCustomNodeAttribute::GetValueAsBool(Bool &val) const
{
	return FromString(m_attributeValue,val);
}

void CDefinitionsManagerAccessor::funcGetAttributeValueAsInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNodeAttribute, attribute, SCustomNodeAttribute() );
	GET_PARAMETER_REF( Int32, value, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( attribute.GetValueAsInt( value) );
}

void CDefinitionsManagerAccessor::funcGetAttributeValueAsFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNodeAttribute, attribute, SCustomNodeAttribute() );
	GET_PARAMETER_REF( Float, value, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( attribute.GetValueAsFloat( value ) );
}

void CDefinitionsManagerAccessor::funcGetAttributeValueAsBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNodeAttribute, attribute, SCustomNodeAttribute() );
	GET_PARAMETER_REF( Bool, value, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( attribute.GetValueAsBool(value ) );
}

void CDefinitionsManagerAccessor::funcGetAttributeValueAsString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNodeAttribute, attribute, SCustomNodeAttribute() );
	FINISH_PARAMETERS;
	RETURN_STRING( attribute.GetValueAsString( ) );
}

void CDefinitionsManagerAccessor::funcGetAttributeValueAsCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNodeAttribute, attribute, SCustomNodeAttribute() );
	FINISH_PARAMETERS;
	RETURN_NAME( attribute.GetValueAsCName( ) );
}

void CDefinitionsManagerAccessor::funcGetAttributeName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNodeAttribute, attribute, SCustomNodeAttribute() );
	FINISH_PARAMETERS;
	RETURN_NAME( attribute.GetAttributeName( ) );
}


void CDefinitionsManagerAccessor::funcGetAbilityTags( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER_REF( TDynArray< CName >, tags, TDynArray< CName >() );
	FINISH_PARAMETERS;

	m_manager->GetAbilityTags( abilityName, tags );
}

void CDefinitionsManagerAccessor::funcAbilityHasTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER( CName, tagName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( m_manager->AbilityHasTag( abilityName, tagName ) );
}

void CDefinitionsManagerAccessor::funcGetAbilityAttributes( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER_REF( TDynArray< CName >, attributes, TDynArray< CName >() );
	FINISH_PARAMETERS;
	
	attributes.ClearFast();
	const SAbility * ability = m_manager->GetAbilityDefinition( abilityName );
	if( ability )
	{
		const auto& attributesList = ability->m_attributes;
		attributes.ResizeFast( attributesList.Size() );
		for( Uint32 i = 0, n = attributesList.Size(); i != n; ++i )
		{
			attributes[ i ] = attributesList[ i ].m_name;
		}
	}
}

void CDefinitionsManagerAccessor::funcGetAbilityAttributeValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER_REF( SAbilityAttributeValue, valMin, SAbilityAttributeValue() );
	GET_PARAMETER_REF( SAbilityAttributeValue, valMax, SAbilityAttributeValue() );
	FINISH_PARAMETERS;

	m_manager->GetAbilityAttributeValue( abilityName, attributeName, valMin, valMax );
}

void CDefinitionsManagerAccessor::funcGetAbilitiesAttributeValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName >, abilitiesNames, TDynArray< CName >() );
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER_REF( SAbilityAttributeValue, valMin, SAbilityAttributeValue() );
	GET_PARAMETER_REF( SAbilityAttributeValue, valMax, SAbilityAttributeValue() );
	GET_PARAMETER_OPT( TDynArray< CName >, tags, TDynArray< CName >() );
	FINISH_PARAMETERS;

	m_manager->GetAbilitiesAttributeValue( abilitiesNames, attributeName, valMin, valMax, tags );
}

void CDefinitionsManagerAccessor::funcIsAbilityDefined( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( m_manager->GetAbilityDefinition( abilityName ) != NULL );
}

void CDefinitionsManagerAccessor::funcGetContainedAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER_REF( TDynArray<CName>, abilities, TDynArray<CName>() );
	FINISH_PARAMETERS;

	const SAbility * ability = m_manager->GetAbilityDefinition( abilityName );
	if( ability )
	{
		abilities = ability->m_abilities;
	}
}

void CDefinitionsManagerAccessor::funcGetUniqueContainedAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray<CName>, abilityNames, TDynArray<CName>() );
	GET_PARAMETER_REF( TDynArray<CName>, outAbilities, TDynArray<CName>() );
	FINISH_PARAMETERS;

	m_manager->GetUniqueContainedAbilities( abilityNames, outAbilities );
}

void CDefinitionsManagerAccessor::funcGetItemHoldSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( Bool, playerItem, false );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_NAME( item ? item->GetHoldSlot( playerItem ) : CName::NONE );
}

void CDefinitionsManagerAccessor::funcGetItemPrice( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_INT( item ? item->GetPrice() : 0 );
}

void CDefinitionsManagerAccessor::funcGetItemCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_NAME( item ? item->GetCategory() : CName::NONE );
}

void CDefinitionsManagerAccessor::funcGetItemEnhancementSlotCount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_INT( item ? item->GetEnhancementSlotCount() : 0 );
}

void CDefinitionsManagerAccessor::funcGetItemUpgradeListName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( Bool, playerItem, false );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_NAME( item ? item->GetUpgradeListName( playerItem ) : CName::NONE );
}

void CDefinitionsManagerAccessor::funcGetItemLocalisationKeyName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_STRING( item ? item->GetLocalizationKeyName() : String::EMPTY );
}

void CDefinitionsManagerAccessor::funcGetItemLocalisationKeyDesc( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_STRING( item ? item->GetLocalizationKeyDesc() : String::EMPTY );
}

void CDefinitionsManagerAccessor::funcGetItemIconPath( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_STRING( item ? item->GetIconPath() : String::EMPTY );
}

void CDefinitionsManagerAccessor::funcItemHasTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* item = m_manager->GetItemDefinition( itemName );
	RETURN_BOOL( item ? item->GetItemTags().Exist( tag ) : false );
}

void CDefinitionsManagerAccessor::funcGetItemsWithTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< CName > & resultArr = *(TDynArray< CName >*) result;

		m_manager->GetItemList( resultArr );

		for ( Int32 i = resultArr.Size() - 1; i >= 0; --i )
		{
			CName name = resultArr[ i ];
			const SItemDefinition* item = m_manager->GetItemDefinition( resultArr[ i ] );
			if ( !item || !item->GetItemTags().Exist( tag ) )
			{
				resultArr.EraseFast( resultArr.Begin() + i );
			}
		}
	}
}

void CDefinitionsManagerAccessor::funcGetItemEquipTemplate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	const SItemDefinition* itemDef = m_manager->GetItemDefinition( itemName );
	if ( itemDef )
	{
		const String& templatePath = GCommonGame->GetDefinitionsManager()->TranslateTemplateName( itemDef->GetEquipTemplateName( true ) );
		RETURN_STRING( templatePath );
		return;
	}

	RETURN_STRING( String::EMPTY );
}

void CDefinitionsManagerAccessor::funcGetUsableItemType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_INT( m_manager->GetUsableItemType( itemName ) );
}

void CDefinitionsManagerAccessor::funcGetCustomDefinitionSubNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, node, ::NullCustomNode );
	GET_PARAMETER( CName, subnode, CName::NONE );
	FINISH_PARAMETERS;

	if( CName::NONE == subnode )
	{ 
		RETURN_STRUCT( SCustomNode, ::NullCustomNode );
		return;
	}

	for( Uint32 i = 0; i< node.m_subNodes.Size(); ++i )
	{
		if( node.m_subNodes[i].m_nodeName == subnode )
		{
			RETURN_STRUCT( SCustomNode, node.m_subNodes[i] );
			return;
		}
	}

	RETURN_STRUCT( SCustomNode, ::NullCustomNode );
	return;
}

Int32 CDefinitionsManagerAccessor::FindAttributeIndex( SCustomNode& node, CName& attName )
{
	if( CName::NONE == attName )
	{
		return -1;
	}

	for( Uint32 i = 0; i < node.m_attributes.Size(); ++i )
	{
		if( node.m_attributes[i].m_attributeName == attName )
		{
			return (Int32)i;
		}
	}
	return -1;
}

void CDefinitionsManagerAccessor::funcFindAttributeIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, node, ::NullCustomNode );
	GET_PARAMETER( CName, attName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_INT( FindAttributeIndex( node, attName ) );
	return;
}

void CDefinitionsManagerAccessor::funcGetCustomNodeAttributeValueString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, node, ::NullCustomNode );
	GET_PARAMETER( CName, attName, CName::NONE );
	GET_PARAMETER_REF( String, val, String::EMPTY );
	FINISH_PARAMETERS;

	Int32 i = FindAttributeIndex( node, attName );

	if( i >= 0 )
	{
		val = node.m_attributes[i].GetValueAsString( );	
		RETURN_BOOL( true );
		return;
	}

	RETURN_BOOL( false );
	return;
}

void CDefinitionsManagerAccessor::funcGetCustomNodeAttributeValueName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, node, ::NullCustomNode );
	GET_PARAMETER( CName, attName, CName::NONE );
	GET_PARAMETER_REF( CName, val, CName::NONE );
	FINISH_PARAMETERS;

	Int32 i = FindAttributeIndex( node, attName );

	if( i >= 0 )
	{
		val = node.m_attributes[i].GetValueAsCName();	
		RETURN_BOOL( true );
		return;
	}

	RETURN_BOOL( false );
	return;
}

void CDefinitionsManagerAccessor::funcGetCustomNodeAttributeValueInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, node, ::NullCustomNode );
	GET_PARAMETER( CName, attName, CName::NONE );
	GET_PARAMETER_REF( Int32, val, 0 );
	FINISH_PARAMETERS;

	Int32 i = FindAttributeIndex( node, attName );

	if( i >= 0 )
	{
		RETURN_BOOL( node.m_attributes[i].GetValueAsInt( val ) );
		return;
	}

	RETURN_BOOL( false );
	return;
}

void CDefinitionsManagerAccessor::funcGetCustomNodeAttributeValueBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, node, ::NullCustomNode );
	GET_PARAMETER( CName, attName, CName::NONE );
	GET_PARAMETER_REF( Bool, val, false );
	FINISH_PARAMETERS;

	Int32 i = FindAttributeIndex( node, attName );

	if( i >= 0 )
	{
		RETURN_BOOL( node.m_attributes[i].GetValueAsBool( val ) );
		return;
	}

	RETURN_BOOL( false );
	return;
}

void CDefinitionsManagerAccessor::funcGetCustomNodeAttributeValueFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SCustomNode, node, ::NullCustomNode );
	GET_PARAMETER( CName, attName, CName::NONE );
	GET_PARAMETER_REF( Float, val, 0.0f );
	FINISH_PARAMETERS;

	Int32 i = FindAttributeIndex( node, attName );

	if( i >= 0 )
	{
		RETURN_BOOL( node.m_attributes[i].GetValueAsFloat( val ) );
		return;
	}

	RETURN_BOOL( false );
	return;		
}	

SItemExtDefinition::SItemExtDefinition():
	m_extPlayerOverridableProperties( nullptr ),
	m_item( nullptr )
{

}

SItemExtDefinition::SItemExtDefinition( const SItemExtDefinition& itemExtDefinition )
	: m_item( nullptr )
{
	m_extDefaultProperties				= itemExtDefinition.m_extDefaultProperties;
	m_extPlayerOverridableProperties	= itemExtDefinition.m_extPlayerOverridableProperties ? new ExtOverridableProperties( *itemExtDefinition.m_extPlayerOverridableProperties ) : nullptr;

	m_creatorTag				= itemExtDefinition.m_creatorTag;

#ifndef NO_DATA_ASSERTS
	m_debugXMLFile				= itemExtDefinition.m_debugXMLFile;
#endif

	if ( itemExtDefinition.m_item )
	{
		AttachToItem( itemExtDefinition.m_item );
	}
}

SItemExtDefinition::~SItemExtDefinition()
{
	if( m_item )
	{
		DettachFromItem( m_item );
	}
	if( m_extPlayerOverridableProperties )
	{
		delete m_extPlayerOverridableProperties;
	}
}

static void AddNewVarinats( TDynArray< SItemDefinition::SItemVariant >& dest, const TDynArray< SItemDefinition::SItemVariant >&  src )
{
	if( src.Size() == 0 )
		return;

	//! find first category variant 
	TDynArray< SItemDefinition::SItemVariant >::iterator firstVariantWithCategory = FindIf( dest.Begin(), dest.End(),  [=]( const SItemDefinition::SItemVariant& variant ){ return !variant.m_categories.Empty() && !variant.m_expectAll; } );
	Uint32 firstVariantWithCategoryIndex = static_cast< Uint32 >( firstVariantWithCategory - dest.Begin() );

	for( const SItemDefinition::SItemVariant& variant : src )
	{
		//! insert variant expecting all conditions to be fulfilled at the beginning of the array
		if ( variant.m_expectAll )
		{
			dest.Insert( 0, variant );
			//! we need to update index of the first element containing category
			firstVariantWithCategoryIndex++;
		}
		//! insert non category variants before first category variant
		//! otherwise non category variant can be never selected
		else if ( variant.m_categories.Empty() )
		{
			dest.Insert( firstVariantWithCategoryIndex++, variant );
		}
		else
		{
			dest.PushBack( variant );//! category variant always push back 
		}
	}
}

void SItemExtDefinition::AttachToItem( SItemDefinition* item )
{
	if( m_item )
	{
		DettachFromItem( m_item );
	}

	AddNewVarinats( item->m_defaultProperties.m_variants, m_extDefaultProperties.m_variants );
	item->m_defaultProperties.m_collapseCond.m_collapseCategoryCond.PushBack( m_extDefaultProperties.m_collapseCond.m_collapseCategoryCond );
	item->m_defaultProperties.m_collapseCond.m_collapseItemCond.PushBack( m_extDefaultProperties.m_collapseCond.m_collapseItemCond );
	item->m_defaultProperties.m_collapseCond.m_uncollapseItemCond.PushBack( m_extDefaultProperties.m_collapseCond.m_uncollapseItemCond );

	if( item->m_playerOverridableProperties )
	{
		if( m_extPlayerOverridableProperties )
		{
			AddNewVarinats( item->m_playerOverridableProperties->m_variants, m_extPlayerOverridableProperties->m_variants );
			item->m_playerOverridableProperties->m_collapseCond.m_collapseCategoryCond.PushBack( m_extPlayerOverridableProperties->m_collapseCond.m_collapseCategoryCond );
			item->m_playerOverridableProperties->m_collapseCond.m_collapseItemCond.PushBack( m_extPlayerOverridableProperties->m_collapseCond.m_collapseItemCond );
			item->m_playerOverridableProperties->m_collapseCond.m_uncollapseItemCond.PushBack(  m_extPlayerOverridableProperties->m_collapseCond.m_uncollapseItemCond );
		}
	}	
	m_item = item;
	item->m_attachedItemExtDefinition.PushBack( this );
}

Bool SItemExtDefinition::DettachFromItem( SItemDefinition* item )
{
	if( m_item == item )
	{
		item->m_attachedItemExtDefinition.Remove( this );

		for( auto& variant : m_extDefaultProperties.m_variants )
		{
			item->m_defaultProperties.m_variants.Remove( variant );
		}
		for( auto& collapseCategoryCond : m_extDefaultProperties.m_collapseCond.m_collapseCategoryCond )
		{
			item->m_defaultProperties.m_collapseCond.m_collapseCategoryCond.Remove( collapseCategoryCond );
		}
		for( auto& collapseItemCond : m_extDefaultProperties.m_collapseCond.m_collapseItemCond )
		{
			item->m_defaultProperties.m_collapseCond.m_collapseItemCond.Remove( collapseItemCond );
		}
		for( auto& uncollapseItemCond : m_extDefaultProperties.m_collapseCond.m_uncollapseItemCond )
		{
			item->m_defaultProperties.m_collapseCond.m_uncollapseItemCond.Remove( uncollapseItemCond );
		}
		if( item->m_playerOverridableProperties )
		{
			if( m_extPlayerOverridableProperties )
			{
				for( auto& variant : m_extPlayerOverridableProperties->m_variants )
				{
					item->m_playerOverridableProperties->m_variants.Remove( variant );
				}
				for( auto& collapseCategoryCond :m_extPlayerOverridableProperties->m_collapseCond.m_collapseCategoryCond )
				{
					item->m_playerOverridableProperties->m_collapseCond.m_collapseCategoryCond.Remove( collapseCategoryCond );
				}
				for( auto& collapseItemCond : m_extPlayerOverridableProperties->m_collapseCond.m_collapseItemCond )
				{
					item->m_playerOverridableProperties->m_collapseCond.m_collapseItemCond.Remove( collapseItemCond );
				}
				for( auto& uncollapseItemCond : m_extPlayerOverridableProperties->m_collapseCond.m_uncollapseItemCond )
				{
					item->m_playerOverridableProperties->m_collapseCond.m_uncollapseItemCond.Remove( uncollapseItemCond );
				}
			}
		}

		m_item = nullptr;
		return true;
	}
	return false;
}
