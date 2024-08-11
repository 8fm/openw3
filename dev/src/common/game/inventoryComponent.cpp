/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "inventoryComponent.h"
#include "equipmentState.h"
#include "itemEntity.h"
#include "inventoryDefinition.h"

#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/physics/physicsWrapper.h"

#include "../../common/game/definitionsManager.h"
#include "../../common/game/abilities.h"
#include "../../common/game/factsDB.h"

#include "../../common/core/gatheredResource.h"
#include "../../common/core/depot.h"
#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"
#include "../../common/core/xmlWriter.h"
#include "../engine/idTagManager.h"

#include "../core/dataError.h"
#include "../core/gameSave.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/dynamicLayer.h"
#include "../engine/tagManager.h"
#include "../engine/pathlibWorld.h"
#include "../engine/utils.h"
#include "../engine/gameTimeManager.h"
#include "../engine/containerManager.h"
#include "container.h"

IMPLEMENT_ENGINE_CLASS( CItemAnimationSyncToken );
IMPLEMENT_ENGINE_CLASS( SItemParts );
IMPLEMENT_ENGINE_CLASS( SItemChangedData );
IMPLEMENT_ENGINE_CLASS( SItemNameProperty );

RED_DEFINE_NAME( ItemContainer );
RED_DEFINE_NAME( OnInventoryScriptedEvent );
RED_DEFINE_STATIC_NAME( GetAssociatedInventory );
RED_DEFINE_STATIC_NAME( W3PlayerWitcher );
RED_DEFINE_STATIC_NAME( W3HorseManager );
RED_DEFINE_STATIC_NAME( SecondaryWeapon );
RED_DEFINE_STATIC_NAME( GetEquippedItems );
RED_DEFINE_STATIC_NAME( OnItemAboutToGive );
IMPLEMENT_RTTI_ENUM( EInventoryEventType );

//////////////////////////////////////////////////////////////////////////
// Scripting support

static void funcIsWeapon( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( SInventoryItem::IsWeapon( itemName ) );
}

static void funcSetLootChancesScale( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, scale, 1.0f );
	FINISH_PARAMETERS;

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), TXT("Gameplay"), TXT("TempChancesMultiplier"), scale );
}

static void funcGetLootChancesScale( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float scale = 1.0f;
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("Gameplay"), TXT("TempChancesMultiplier"), scale );

	RETURN_FLOAT( scale );
}

void RegisterInventoryItemScriptFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "IsItemWeapon", funcIsWeapon );

	NATIVE_GLOBAL_FUNCTION( "SetLootChancesScale", funcSetLootChancesScale );
	NATIVE_GLOBAL_FUNCTION( "GetLootChancesScale", funcGetLootChancesScale );
}

//////////////////////////////////////////////////////////////////////////

CItemAnimationSyncToken::CItemAnimationSyncToken()
	: m_itemUniqueId()
	, m_inventoryComponent( nullptr )
	, m_syncedAnimated( nullptr )
	, m_tokenFailedToLoad( false )
	, m_lastUsedAnimationName( CName::NONE )
{

}

void CItemAnimationSyncToken::Sync( CName animationName, const CSyncInfo& syncInfo, Float weight ) 
{
	CAnimatedComponent* animated = m_syncedAnimated.Get();
	if ( !m_tokenFailedToLoad && !animated )
	{
		CInventoryComponent* inventoryComponent = m_inventoryComponent.Get();
		m_tokenFailedToLoad = inventoryComponent ? !m_inventoryComponent->TryToLoadSyncToken( m_inventoryComponent->GetItem( m_itemUniqueId ), animationName, animated ) : true;
		m_syncedAnimated = animated;
	}
	if ( animated && animated->IsAttached() && animationName )
	{
		m_lastUsedAnimationName = animationName;
		animated->PlayAnimationOnSkeletonWithSync( animationName, syncInfo );
	}
}

void CItemAnimationSyncToken::Reset()
{
	CAnimatedComponent* animated = m_syncedAnimated.Get();
	if ( animated && m_lastUsedAnimationName != CName::NONE && m_lastUsedAnimationName == animated->GetAsyncPlayedAnimName() )
	{
		animated->StopAllAnimationsOnSkeleton();
		animated->ForceTPose( false );
	}
}

Bool CItemAnimationSyncToken::IsValid() const
{
	return !m_tokenFailedToLoad;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CInventoryComponent );

CInventoryComponent::CInventoryComponent()
	: m_containerTemplate( nullptr )
	, m_suppressEvents( false )
	, m_notifyScriptedListeners( false )
	, m_turnOffSpawnItemsBudgeting( false )
	, m_itemsStateHash( 0 )
	, m_nextRebalance( 0 )
	, m_rebalanceEveryNSeconds( 259200 ) // ( 3600 * 24 * 3 ) ~= 3 days
	, m_associatedInventory( nullptr )
{
	RED_FATAL_ASSERT( !IsStreamed(), "Inventory component must not be streamed! You shall not pass!" );
	m_currentLOD = ILODable::LOD_2; // to init by LOD0 init
}

CInventoryComponent::~CInventoryComponent()
{
	m_listeners.ClearFast();
}

Bool CInventoryComponent::IsIdValid( SItemUniqueId itemId ) const
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		if ( m_items[i].GetUniqueId() == itemId )
		{
			return true;
		}
	}
	return false;
}

void CInventoryComponent::ClearInventory()
{
	HideAllItemsRaw();
	m_items.Clear();
	MarkItemsListChanged();
}

SInventoryItem* CInventoryComponent::GetItem( CItemEntity * itemEntity )
{
	const CItemEntityProxy * proxy = itemEntity->GetItemProxy();

	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		if ( m_items[ i ].GetItemEntityProxy() == proxy )
		{
			return & m_items[ i ];
		}
	}

	return NULL;
}

SInventoryItem* CInventoryComponent::GetItem( SItemUniqueId itemId )
{
	// Validate Item
	if ( itemId == SItemUniqueId::INVALID )
	{
		//RED_ASSERT( itemId != SItemUniqueId::INVALID, TXT( "Invalid Item ID: %d" ), itemId );
		ERR_GAME( TXT( "Invalid Item ID: %d" ), itemId );
		return nullptr;
	}

	Int32 index = UniqueIdToIndex( itemId );
	if ( IsIndexValid( index ) == false )
	{
		//RED_ASSERT( index != false, TXT( "Cannot find valid index with item id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find valid index with item id: %d" ), itemId );
		return nullptr;
	}

	// Validate Item
	SInventoryItem* invItem =  &m_items[ index ];
	return invItem;
}

const SInventoryItem* CInventoryComponent::GetItem( SItemUniqueId itemId ) const
{
	// Validate Item
	if ( itemId == SItemUniqueId::INVALID )
	{
		//RED_ASSERT( itemId != SItemUniqueId::INVALID, TXT( "Invalid Item ID: %d" ), itemId );
		ERR_GAME( TXT( "Invalid Item ID: %d" ), itemId );
		return nullptr;
	}

	Int32 index = UniqueIdToIndex( itemId );
	if ( IsIndexValid( index ) == false )
	{
		//RED_ASSERT( index != false, TXT( "Cannot find valid index with item id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find valid index with item id: %d" ), itemId );
		return nullptr;
	}

	// Validate Item
	const SInventoryItem* invItem = &m_items[ index ];
	return invItem;
}

CName CInventoryComponent::GetCraftedItemName( SItemUniqueId itemId ) const
{
	const SInventoryItem* item = GetItem( itemId );
	if ( !item )
	{
		return CName::NONE;
	}

	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );
	if ( !itemDef )
	{
		return CName::NONE;
	}

	return itemDef->GetCraftedItemName();
}

SItemUniqueId CInventoryComponent::GetItemByCategoryForScene( CName category, CName ignoreTag ) const
{
	SFindItemInfo info;
	SItemUniqueId itemId;
	info.m_category = category;
	info.m_eligibleToMount = true;
	{
		info.m_mountOnly = true;
		info.m_withoutTag = ignoreTag ? ignoreTag : CNAME( SecondaryWeapon );		
		itemId = FindItem( info );
	}	
	if( !itemId && !ignoreTag )
	{
		info.m_withoutTag = CName::NONE;
		itemId = FindItem( info );
	}
	if ( !itemId )
	{
		info.m_mountOnly = false;
		info.m_withoutTag = ignoreTag ? ignoreTag : CNAME( SecondaryWeapon );;
		itemId = FindItem( info );
	}
	if ( !itemId && !ignoreTag )
	{
		info.m_withoutTag = CName::NONE;
		itemId = FindItem( info );
	}

	return itemId;
}

SItemUniqueId CInventoryComponent::GetItemByCategory( CName category, Bool mountOnly /*= true*/, Bool ignoreDefaultItem /*= false*/ ) const
{	
	CName defaultItemName = ignoreDefaultItem ? GetCategoryDefaultItem( category ) : CName::NONE;
	const auto itemsEndIt = m_items.End();
	for ( auto itemIt = m_items.Begin(); itemIt != itemsEndIt; ++itemIt )
	{
		if ( ( itemIt->GetCategory() == category ) && ( !mountOnly || itemIt->IsMounted() || itemIt->IsHeld() ) )
		{
			if ( ignoreDefaultItem && ( defaultItemName == itemIt->GetName() ) )
			{
				// Default item mounted, ignore it
				continue;
			}

			// That's our item
			return itemIt->GetUniqueId();
		}
	}

	return SItemUniqueId::INVALID;
}

void CInventoryComponent::GetItemsByCategory( CName category, TDynArray< SItemUniqueId > & items )
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];
		if ( item.GetCategory() == category )
		{
			items.PushBack( item.GetUniqueId() );
		}
	}
}

RED_DEFINE_STATIC_NAME( HasRequiredLevelToEquipItem );

SItemUniqueId CInventoryComponent::FindItem( const SFindItemInfo& info ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	CName defaultItemName = info.m_ignoreDefaultItem ? GetCategoryDefaultItem( info.m_category ) : CName::NONE;
	const auto itemsEndIt = m_items.End();
	for ( auto itemIt = m_items.Begin(); itemIt != itemsEndIt; ++itemIt )
	{
		if (   ( !info.m_category  || itemIt->GetCategory() == info.m_category ) 
			&& ( !info.m_mountOnly || itemIt->IsMounted() || itemIt->IsHeld()  ) 
			&& ( !info.m_itemTag   || itemIt->GetTags().Exist( info.m_itemTag ) ) 
			&& ( !info.m_withoutTag || !itemIt->GetTags().Exist( info.m_withoutTag ) ) 
			)
		{
			if ( info.m_ignoreDefaultItem && ( defaultItemName == itemIt->GetName() ) )
			{
				// Default item mounted, ignore it
				continue;
			}
			if ( info.m_holdSlot )
			{
				const SItemDefinition* itemDef = defMgr->GetItemDefinition( itemIt->GetName() );
				RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), itemIt->GetName().AsChar(), GetEntity()->GetName().AsChar() );
				if ( itemDef && itemDef->GetHoldSlot( IsPlayerOwner() ) != info.m_holdSlot )
				{
					// Hold slot is incorrect
					continue;
				}				
			}
			if( info.m_eligibleToMount )
			{
				CEntity* parent = GetEntity();
				if( parent && parent->IsPlayer() )
				{
					Bool res = false;
					CallFunctionRet<Bool, SItemUniqueId>( parent, CNAME( HasRequiredLevelToEquipItem), itemIt->GetUniqueId(), res );
					if( !res )
					{
						continue;
					}					
				}				
			}

			// That's our item
			return itemIt->GetUniqueId();
		}
	}

	return SItemUniqueId::INVALID;
}

 

Bool CInventoryComponent::IsPlayerOwner() const
{
	CObject* parent = GetParent();
	if ( !parent )
	{
		return false;
	}
	if ( parent->IsA< CPlayer >() )
	{
		return true;
	}

	// if parent isn't player check if it is entity created from player template =  if it is UI Geralt preview
	if ( CObject* parentTemplate = parent->GetTemplate() )
	{
		const CEntity* pEntity = Cast< CEntity >( parentTemplate->GetTemplateInstance() );
		if ( pEntity )
		{
			return pEntity->IsPlayer();
		}
	}
	return false;
}

SItemUniqueId CInventoryComponent::GetItemIdHeldInSlot( CName holdSlot )
{
	SInventoryItem* item = GetItemHeldInSlot( holdSlot );
	return item ? item->GetUniqueId() : SItemUniqueId::INVALID;
}

SInventoryItem* CInventoryComponent::GetItemHeldInSlot( CName holdSlot )
{
	//called quite often - could use some caching especially once number of items will go up

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();

	auto itemIter = m_items.Begin();
	const auto itemsEndIt = m_items.End();
	while ( itemIter != itemsEndIt )
	{
		if ( itemIter->IsHeld() )
		{
			const SItemDefinition* itemDef = defMgr->GetItemDefinition( itemIter->GetName() );
			if ( itemDef && holdSlot == itemDef->GetHoldSlot( IsPlayerOwner() ) )
			{
				return &(*itemIter);
			}
		}
		++itemIter;
	}

	return nullptr;
}

SItemUniqueId CInventoryComponent::GetItemId( CName itemName ) const
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];
		if( item.GetName() == itemName )
		{
			return item.GetUniqueId();
		}
	}

	return SItemUniqueId::INVALID;
}

SItemUniqueId CInventoryComponent::GetItemId( const SInventoryItem& item ) const
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& itemIter = m_items[ i ];
		if ( itemIter == item )
		{
			return itemIter.GetUniqueId();
		}
	}

	return SItemUniqueId::INVALID;
}

Bool CInventoryComponent::GetItemTags( SItemUniqueId itemId, TDynArray< CName >& tags ) const
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( IsIndexValid( index ) == false )
	{
		return false;
	}
	tags = m_items[index].GetTags();
	return true;
}

void CInventoryComponent::ActivateQuestBonus( void )
{
	m_tags.AddTag( CNAME( quest_bonus ) );
}

Float CInventoryComponent::CalcItemTagModifierWeight( CName tagModifier ) const
{
	PC_SCOPE( CalcItemTagModifierWeight );

	Float valueModifier = 1.0f;

	const SItemTagModifier* itemModifier = GetItemTagModifier( tagModifier );
	if ( itemModifier != nullptr )
	{
		Float tagWeight = 1.0f;

		if ( GetTags().Empty() )
		{
			// ERR_GAME( TXT( "No modifier tags were found for Entity Template's Inventory Component." ) );
			if ( itemModifier->m_modifierMap.Find( CNAME( type_general ) ), tagWeight )
			{
				if ( tagWeight < 0.0f )
				{
					ERR_GAME( TXT( "WARNING!!!  %s modifier weight cannot be negative." ), CNAME( type_general ).AsChar() );
					tagWeight = 1.0f;
				}

				valueModifier *= tagWeight;
			}
		}
		else
		{
			const CFactsDB* factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;

			const TagList& componentTagList = GetTags();
			for ( Uint32 tag = 0; tag < componentTagList.GetTags().Size(); ++tag )
			{
				if ( itemModifier->m_modifierMap.Find( componentTagList.GetTag( tag ), tagWeight ) )
				{
					if ( tagWeight < 0.0f )
					{
						ERR_GAME( TXT( "WARNING!!!  %s modifier weight cannot be negative." ), componentTagList.GetTag( tag ).AsChar() );
						tagWeight = 1.0f;
					}

					valueModifier *= tagWeight;

					if ( factsDB && factsDB->DoesExist( componentTagList.GetTag( tag ).AsString() ) )
					{
						CName factTag = CName( componentTagList.GetTag( tag ).AsString() + CNAME( _bonus ).AsString() );
						if ( itemModifier->m_modifierMap.Find( factTag, tagWeight ) )
						{
							if ( tagWeight < 0.0f )
							{
								ERR_GAME( TXT( "%s weight cannot be negative." ), factTag.AsChar() );
								tagWeight = 1.0f;
							}

							valueModifier *= tagWeight;
						}
					}
				}
			}
		}
	}

	return valueModifier;
}

Float CInventoryComponent::GetFundsModifier() const
{
	return CalcItemTagModifierWeight( CNAME( mod_funds ) );
}

Bool CInventoryComponent::AddSlot( SInventoryItem& invItem ) const
{
	PC_SCOPE( AddSlot );

	return invItem.AddSlot();
}

Uint8 CInventoryComponent::GetSlotItemsLimit( SInventoryItem& invItem ) const
{
	PC_SCOPE( GetSlotItemsLimit );

	return invItem.GetSlotItemsLimit();
}

//! Returns cost of adding a slot to a given item.
Int32 CInventoryComponent::GetItemPriceCrafting( const SInventoryItem& invItem ) const
{
	PC_SCOPE( GetItemPriceCrafting );

	Float priceBase = ( (Float) GetItemPrice( invItem ) );

	Float valueModifier = CalcItemTagModifierWeight( CNAME( mod_craftitem ) );

	return (Int32)Red::Math::MCeil( priceBase * valueModifier );
}

//! Returns cost of adding a slot to a given item.
Int32 CInventoryComponent::GetItemPriceAddSlot( const SInventoryItem& invItem ) const
{
	PC_SCOPE( GetItemPriceAddSlot );

	Float priceBase = ( (Float) GetItemPrice( invItem ) );

	Float valueModifier = CalcItemTagModifierWeight( CNAME( mod_addslot ) );

	Uint32 slotCount = invItem.GetSlotItemsMax() + 1;
	
	return (Int32)Red::Math::MCeil( ( priceBase * valueModifier * slotCount ) + ( 293.0f * slotCount ) );
}

//! Returns cost of removing an enchantment on a given item.
Int32 CInventoryComponent::GetItemPriceRemoveEnchantment( const SInventoryItem& invItem ) const
{
	PC_SCOPE( GetItemPriceRemoveEnchantment );

	Float priceBase = ( (Float) GetItemPrice( invItem ) );

	Float valueModifier = CalcItemTagModifierWeight( CNAME( mod_removeenchantment ) );

	return (Int32)Red::Math::MCeil( priceBase * valueModifier );
}

//! Returns cost of adding an enchantment to a given item.
Int32 CInventoryComponent::GetItemPriceEnchantItem( const SInventoryItem& invItem ) const
{
	PC_SCOPE( GetItemPriceEnchantItem );

	Float priceBase = ( (Float) GetItemPrice( invItem ) );

	Float valueModifier = CalcItemTagModifierWeight( CNAME( mod_enchantment ) );

	return (Int32)Red::Math::MCeil( priceBase * valueModifier );
}

Int32 CInventoryComponent::GetItemPriceDisassemble( const SInventoryItem& invItem ) const
{
	PC_SCOPE( GetItemPriceDisassemble );

	Float priceBase = ( (Float) GetItemPrice( invItem ) );

	Float valueModifier = CalcItemTagModifierWeight( CNAME( mod_disassemble ) );

	return (Int32)Red::Math::MCeil( priceBase * valueModifier );
}

Int32 CInventoryComponent::GetItemPriceRemoveUpgrade( const SInventoryItem& invItem ) const
{
	PC_SCOPE( GetItemPriceRemoveUpgrade );
	
	Float priceBase = ( (Float) GetItemPrice( invItem ) );

	Float valueModifier = CalcItemTagModifierWeight( CNAME( mod_removeupgrade ) );

	return (Int32)Red::Math::MCeil( priceBase * valueModifier );
}

void CInventoryComponent::GetItemPriceRepair( const SInventoryItem& invItem, Int32& costRepairPoint, Int32& costRepairTotal ) const
{
	PC_SCOPE( GetItemPriceRepair );

	// Set Price based on Definition
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( nullptr == defMgr )
	{
		ERR_GAME( TXT( "Definitions Manager is NULL" ) );
		return;
	}

	const SItemDefinition* itemDef = defMgr->GetItemDefinition( invItem.GetName() );
	if ( !itemDef )
	{
		ERR_GAME( TXT( "Cannot find Item Definition for Item Name: %s in entity %s" ), invItem.GetName().AsChar(), GetEntity()->GetName().AsChar() );
		return;
	}

	Float itemPrice = (Float)itemDef->GetPrice();

	const SItemDamageCurve* itemDamageCurve = defMgr->GetItemDamageCurve( invItem.GetCategory() );
	if ( itemDamageCurve != nullptr )
	{
		Float statTotal = TotalItemStats( invItem );

		Float damageAdjustment = itemDamageCurve->m_term1 + itemDamageCurve->m_term2 * Red::Math::MPow( statTotal, 1 ) + itemDamageCurve->m_term3 * Red::Math::MPow( statTotal, 2 );
		if ( damageAdjustment < 0.0f )
		{
			damageAdjustment = 1.0f;
		}

		itemPrice += damageAdjustment;
	}

	Float tagMod = CalcItemTagModifierWeight( CNAME( mod_repair ) );

	costRepairPoint = (Int32)Red::Math::MCeil( itemPrice * tagMod * 0.01f );

	Float durabilityPercentage = GetItemDurabilityPercentage( invItem );

	costRepairTotal = (Int32)Red::Math::MCeil( costRepairPoint * ( 1.0f - durabilityPercentage ) * 100.0f );
}

Int32 CInventoryComponent::GetItemPriceModified( const SInventoryItem& invItem, Bool playerSellingItem ) const
{
	PC_SCOPE( GetItemPriceModified );

	Float tagMod = 1.0f;
	Bool canSellItem = true;

	// Set Price based on Definition
	Float itemPrice = (Float)GetItemPrice( invItem );
	if ( 0 == itemPrice )
	{
		return (Int32)itemPrice;
	}

	if ( !GetTags().Empty() )
	{
		const TagList& componentTagList = GetTags();

		const TDynArray< CName >& itemTags = invItem.GetTags();
		if ( !itemTags.Empty() )
		{
			CName tagName;
			Uint32 tag = 0;
			Float tagWeight = 1.0f;
			const SItemTagModifier* itemModifier;
			const CFactsDB* factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;

			TDynArray< CName >::const_iterator tagIter = itemTags.Begin();
			const auto itemTagsEndIt = itemTags.End();
			while( tagIter != itemTagsEndIt )
			{
				// Modifier is already applied in GetItemPrice().
				if ( *tagIter != CNAME( mod_legendary ) )
				{
					itemModifier = GetItemTagModifier( *tagIter );

					if ( itemModifier != nullptr )
					{
						for ( tag = 0; tag < componentTagList.GetTags().Size(); ++tag )
						{
							tagName = componentTagList.GetTag( tag );

							if ( itemModifier->m_modifierMap.Find( tagName, tagWeight ) )
							{
								if ( tagWeight < 0 )
								{
									canSellItem = false;
								}

								tagMod *= Red::Math::MAbs( tagWeight );

								const String tagString = componentTagList.GetTag( tag ).AsString();
								if ( factsDB && factsDB->DoesExist( tagString ) )
								{
									if ( CNAME( area_nml ) == tagName )
									{
										itemModifier->m_modifierMap.Find( CNAME( area_nml_bonus ), tagWeight );
										tagMod *= Red::Math::MAbs( tagWeight );
									}
									else if ( CNAME( area_novigrad ) == tagName )
									{
										itemModifier->m_modifierMap.Find( CNAME( area_novigrad_bonus ), tagWeight );
										tagMod *= Red::Math::MAbs( tagWeight );
									}
									else if ( CNAME( area_skellige ) == tagName )
									{
										itemModifier->m_modifierMap.Find( CNAME( area_skellige_bonus ), tagWeight );
										tagMod *= Red::Math::MAbs( tagWeight );
									}
								}
							}
						}
					}
				}
				++tagIter;
			}
		}
	}

	Float priceModified = Red::Math::MCeil( itemPrice * tagMod );

	if ( invItem.GetCategory() == CNAME( bolt ) )
	{
		priceModified /= 10.0f; // Distributes price across 10 bolts
	}

	if ( playerSellingItem )
	{
		Float priceModSale = CalcItemTagModifierWeight( CNAME( mod_sale ) );
		priceModified *= priceModSale;

		if ( priceModified < 1.0f )
		{
			priceModified = 1.0f;
		}

		// A Negative Value is required to indicate if the Player cannot sell this Item.
		if ( !canSellItem )
		{
			priceModified *= -1.0f;
		}
	}

	//ITEM_LOG( TXT( "The Item's worth is %f but the Merchant will only purchase for %f Crowns." ), priceBase * priceModVendor, priceBase * priceModVendor * priceModSale );
	return (Int32)priceModified;
}

Int32 CInventoryComponent::GetItemPriceModified( SItemUniqueId itemId, Bool playerSellingItem ) const
{
	const SInventoryItem* invItem = GetItem( itemId );
	if ( nullptr == invItem )
	{
		return 0;
	}

	return GetItemPriceModified( *invItem, playerSellingItem );
}

Int32 CInventoryComponent::GetItemPrice( SItemUniqueId itemId ) const
{
	const SInventoryItem* invItem = GetItem( itemId );
	if ( nullptr == invItem )
	{
		return 0;
	}

	return GetItemPrice( *invItem );
}

// Determine price as a function of damage-based stats scaled by item's durability.
Int32 CInventoryComponent::GetItemPrice( const SInventoryItem& invItem ) const
{
	if ( invItem.GetTags().Exist( CNAME( mod_noprice ) ) )
	{
		return 0;
	}

	// Set Price based on Definition
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( nullptr == defMgr )
	{
		ERR_GAME( TXT( "Definitions Manager is NULL" ) );
		return 0;
	}

	const SItemDefinition* itemDef = defMgr->GetItemDefinition( invItem.GetName() );
	if ( !itemDef )
	{
		ERR_GAME( TXT( "Cannot find Item Definition for Item Name: %s in entity %s" ), invItem.GetName().AsChar(), GetEntity()->GetName().AsChar() );
		return 0;
	}

	Float itemPrice = (Float)itemDef->GetPrice();

	const SItemDamageCurve* itemDamageCurve = defMgr->GetItemDamageCurve( invItem.GetCategory() );
	if ( nullptr == itemDamageCurve )
	{
		return (Int32)Red::Math::MCeil( itemPrice );
	}

	Float statTotal = TotalItemStats( invItem );

	Float damageAdjustment = itemDamageCurve->m_term1 + itemDamageCurve->m_term2 * Red::Math::MPow( statTotal, 1 ) + itemDamageCurve->m_term3 * Red::Math::MPow( statTotal, 2 );
	if ( damageAdjustment < 0.0f )
	{
		damageAdjustment = 1.0f;
	}

	Float durabilityModifier = GetItemDurabilityPercentage( invItem );

	itemPrice = Red::Math::MCeil( itemPrice + ( 0.5f * damageAdjustment ) + ( 0.5f * damageAdjustment * durabilityModifier ) );
	
	if ( invItem.GetTags().Exist( CNAME( mod_legendary ) ) )
	{
		Float priceModLegendary = CalcItemTagModifierWeight( CNAME( mod_legendary ) );
		itemPrice *= priceModLegendary;
	}

	//ITEM_LOG( TXT( "The %s durability has reduced it's worth to %f Crowns out of a possible %f Crowns." ), invItem->GetName().AsChar(), actualPrice, price );
	return (Int32)itemPrice;
}

// Accumulate all ability stats for this Item
void CInventoryComponent::AccumulateItemStats( const SInventoryItem* invItem, TDynArray< SItemStat >& itemStats ) const
{
	Float statValue;
	
	SAbilityAttributeValue attrValue;

	// Set Base Price based on Definition
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( nullptr == defMgr )
	{
		//RED_ASSERT( defMgr != nullptr, TXT( "Definitions Manager is NULL" ) );
		ERR_GAME( TXT( "Definitions Manager is NULL" ) );
		return;
	}

	TDynArray< CName > attributeNames;
	invItem->GetAllAttributes( attributeNames );

	const Uint32 attrListSize = attributeNames.Size();
	for ( Uint32 j = 0; j < attrListSize; ++j )
	{
		const CName& attributeName = attributeNames[ j ];
		const SItemStat* itemStat = defMgr->GetItemStatDefinition( attributeName );
		if ( itemStat != nullptr )
		{
			attrValue = invItem->GetAttribute( attributeName );

			statValue = 0;
			if ( attrValue.m_valueBase != 0 )
			{
				statValue += attrValue.m_valueBase;
			}
			if ( attrValue.m_valueMultiplicative != 0 )
			{
				statValue += attrValue.m_valueMultiplicative;
			}
			if ( attrValue.m_valueAdditive != 0 )
			{
				statValue += attrValue.m_valueAdditive;
			}

			// Convert floating / percentage values to int to maintain consistency with other stats
			if ( itemStat->m_statIsPercentage )
			{
				AccumulateItemStat( itemStats, itemStat->m_statType, Red::Math::MAbs( statValue ) * 100.0f * itemStat->m_statWeight );
				//ITEM_LOG( TXT( "%s item has a %s with %f points of %s." ), invItem->GetName().AsChar(), itemStat->m_statType.AsChar(), statValue * 100.0f * itemStat->m_statWeight, attributeName.AsChar() );
			}
			else
			{
				AccumulateItemStat( itemStats, itemStat->m_statType, Red::Math::MAbs( statValue ) * itemStat->m_statWeight );
				//ITEM_LOG( TXT( "%s item has a %s with %f points of %s." ), invItem->GetName().AsChar(), itemStat->m_statType.AsChar(), statValue * itemStat->m_statWeight, attributeName.AsChar() );
			}
		}
	}
}

void CInventoryComponent::AccumulateItemStat(TDynArray< SItemStat >& itemStats, CName statType, Float statWeight ) const
{
	SItemStat newStat;

	const Uint32 itemStatsCount = itemStats.Size();

	for ( Uint32 i = 0; i < itemStatsCount; ++i )
	{
		SItemStat& itemStat = itemStats[ i ];
		if ( itemStat.m_statType == statType )
		{
			itemStat.m_statWeight += statWeight;
			return;
		}
	}

	newStat.m_statType = statType;
	newStat.m_statWeight = statWeight;
	itemStats.PushBack( newStat );
}

Float CInventoryComponent::TotalItemStats( const SInventoryItem& invItem ) const
{
	Float total = 0.0f;

	TDynArray< SItemStat > itemStats;
	AccumulateItemStats( &invItem, itemStats );

	const Uint32 itemStatsCount = itemStats.Size();
	for ( Uint32 i = 0; i < itemStatsCount; ++i )
	{
		// ToDo - MAS - If desired, determine weight of each stat based on attribute type.
		// E.g. attack=1, defence=1.5, magic=2, vitality=2.5, toxicity = 1.75, etc.
		total += itemStats[ i ].m_statWeight;
	}

	//ITEM_LOG( TXT( "The statTotal for %s is %f." ), invItem.GetName().AsChar(), total );

	return total;
}

const SItemTagModifier* CInventoryComponent::GetItemTagModifier( CName itemTag ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( nullptr == defMgr )
	{
		//RED_ASSERT( defMgr != nullptr, TXT( "Definitions Manager is NULL" ) );
		ERR_GAME( TXT( "Definitions Manager is NULL" ) );
		return nullptr;
	}

	return defMgr->GetItemTagModifier( itemTag );
}

Uint32 CInventoryComponent::GetItemCount(  Bool useAssociatedInventory /* = false */ )
{
	Uint32 count = m_items.Size();

	if ( useAssociatedInventory )
	{
		CInventoryComponent* inv = GetAssociatedInventory();
		if ( inv != nullptr )
		{
			count += inv->GetItemCount( false );
		}
	}

	return count;
}

Uint32 CInventoryComponent::GetItemQuantityByName( CName itemName, Bool useAssociatedInventory /* = false */, TDynArray< CName > * ignoreTags /* = nullptr */ )
{
	Uint32 count = 0;

	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];
		if ( item.GetName() == itemName )
		{
			if ( ignoreTags == nullptr || !m_items[ i ].HasTags( *ignoreTags ) )
			{
				count += item.GetQuantity();
			}
		}
	}

	if ( useAssociatedInventory )
	{
		CInventoryComponent* inv = GetAssociatedInventory();
		if ( inv != nullptr )
		{
			count += inv->GetItemQuantityByName( itemName, false, ignoreTags );
		}
	}

	return count;
}

Uint32 CInventoryComponent::GetItemQuantity( SItemUniqueId itemId ) const
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( index ) )
	{
		return 0;
	}
	return m_items[ index ].GetQuantity();
}

Uint32 CInventoryComponent::GetItemQuantityByCategory( CName itemCategory, Bool useAssociatedInventory /* = false */, TDynArray< CName > * ignoreTags /* = nullptr */ )
{
	Uint32 count = 0;

	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];
		if ( item.GetCategory() == itemCategory )
		{
			if ( ignoreTags == nullptr || !m_items[ i ].HasTags( *ignoreTags ) )
			{
				count += item.GetQuantity();
			}
		}
	}

	if ( useAssociatedInventory )
	{
		CInventoryComponent* inv = GetAssociatedInventory();
		if ( inv != nullptr )
		{
			count += inv->GetItemQuantityByCategory( itemCategory, false, ignoreTags );
		}
	}

	return count;
}

Uint32 CInventoryComponent::GetItemQuantityByTag( CName itemTag, Bool useAssociatedInventory /* = false */, TDynArray< CName > * ignoreTags /* = nullptr */ )
{
	Uint32 count = 0;

	const Uint32 itemsCount = m_items.Size();
	for( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];
		if ( item.HasTag( itemTag ) )
		{
			if ( ignoreTags == nullptr || !m_items[ i ].HasTags( *ignoreTags ) )
			{
				count += item.GetQuantity();
			}
		}
	}

	if ( useAssociatedInventory )
	{
		CInventoryComponent* inv = GetAssociatedInventory();
		if ( inv != nullptr )
		{
			count += inv->GetItemQuantityByTag( itemTag, false, ignoreTags );
		}
	}

	return count;
}

Uint32 CInventoryComponent::GetAllItemsQuantity( Bool useAssociatedInventory /* = false */, TDynArray< CName > * ignoreTags /* = nullptr */ )
{
	Uint32 count = 0;

	const Uint32 itemsCount = m_items.Size();
	for( Uint32 i = 0; i < itemsCount; ++i )
	{
		if ( ignoreTags == nullptr || !m_items[ i ].HasTags( *ignoreTags ) )
		{
			count += m_items[ i ].GetQuantity();
		}
	}

	if ( useAssociatedInventory )
	{
		CInventoryComponent* inv = GetAssociatedInventory();
		if ( inv != nullptr )
		{
			count += inv->GetAllItemsQuantity( false, ignoreTags );
		}
	}

	return count;
}

Bool CInventoryComponent::ItemHasTag( SItemUniqueId itemId, CName tag ) const
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( index >= 0 )
	{
		return m_items[ index ].GetTags().Exist( tag );
	}
	else
	{
		return false;
	}
}

Bool CInventoryComponent::AddItemTag( SItemUniqueId itemId, CName tag )
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( index >= 0 )
	{
		if ( m_items[ index ].GetTags().PushBackUnique( tag ) )
		{
			MarkItemsListChanged();
			NotifyListeners( IET_ItemTagChanged, itemId, 0, false );
			return true;
		}
	}
	return false;
}

Bool CInventoryComponent::RemoveItemTag( SItemUniqueId itemId, CName tag )
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( index >= 0 )
	{
		if ( m_items[ index ].GetTags().Remove( tag ) )
		{
			MarkItemsListChanged();
			NotifyListeners( IET_ItemTagChanged, itemId, 0, false );
			return true;
		}
	}
	return false;
}

Bool CInventoryComponent::HasItemDurability( SItemUniqueId itemId ) const
{
	return GetItemMaxDurability( itemId ) != -1;
}

Float CInventoryComponent::GetItemDurability( SItemUniqueId itemId ) const
{
	const SInventoryItem* invItem  = GetItem( itemId );
	if ( !invItem )
	{
		//RED_ASSERT( invItem != nullptr, TXT( "Cannot find item with id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find item with id: %d" ), itemId );
	    return -1;
	}
	return invItem->GetDurability();
}

// Returns ratio of Item Current Durability over Total Durability.
Float CInventoryComponent::GetItemDurabilityPercentage( const SInventoryItem& invItem ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( nullptr == defMgr )
	{
		ERR_GAME( TXT( "Definitions Manager is NULL" ) );
		//RED_ASSERT( defMgr != nullptr, TXT( "Definitions Manager is NULL" ) );
		return -1;
	}

	const SItemDefinition* itemDef = defMgr->GetItemDefinition( invItem.GetName() );
	if ( !itemDef )
	{
		ERR_GAME( TXT( "Cannot find Item Definition for Item Name: %s in entity %s" ), invItem.GetName().AsChar(), GetEntity()->GetName().AsChar() );
		//RED_ASSERT( itemDef != nullptr, TXT( "Cannot find Item Definition for Item Name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		return -1;
	}

	// Current value of item is modified by Item Durability
	Float maxDurability = itemDef->GetMaxDurability( IsPlayerOwner() );
	if ( maxDurability <= 0.0f )
	{
		// Protects from divide by 0 if data is missing MaxDurability value
		// or if Item does not require Durability stat. E.g. Potion.
		return -1;
	}

	Float itemDurability = invItem.GetDurability();
	if ( itemDurability < 0.0f )
	{
		itemDurability = 0.0f;
		ERR_GAME( TXT( "%s Durability cannot be less than Zero." ), invItem.GetName().AsChar() );
	}

	return itemDurability / maxDurability;
}

void CInventoryComponent::SetItemDurability( SItemUniqueId itemId, Float durability )
{
	SInventoryItem* invItem  = GetItem( itemId );
	if ( !invItem )
	{
		//RED_ASSERT( invItem != nullptr, TXT( "Cannot find item with id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find item with id: %d" ), itemId );
	    return;
	}

#ifndef NO_EDITOR
	if ( !HasItemDurability( itemId ) )
	{
		RED_HALT( "Setting durability to item that doesn't have durability!" );
		return;
	}
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );
	if ( !itemDef )
	{
		//RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		ERR_GAME( TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		return;
	}
	if ( itemDef->IsStackable() )
	{
		RED_HALT( "Setting durability to stackable item!" );
		return;
	}
#endif //NO_EDITOR

	invItem->SetDurability( durability );
	//ITEM_LOG( TXT( "%s has Durability = %f" ), invItem->GetName().AsChar(), invItem->GetDurability() );
}

void CInventoryComponent::ReduceLootableItemDurability( SInventoryItem& item )
{
	SItemUniqueId itemId = item.GetUniqueId();
	if ( !IsIdValid( itemId ) )
	{
		return;
	}

	CClass* classMerchantNPC = SRTTI::GetInstance().FindClass( CNAME( W3MerchantNPC ) );
	if ( classMerchantNPC && GetEntity()->IsA( classMerchantNPC ) )
	{
		return;
	}

	Float itemDurability = GetItemDurability( itemId );
	if ( itemDurability > 0 )
	{
		Red::Math::Random::Generator< Red::Math::Random::StandardRand > randGen;

		// ToDo - Move magic numbers to designer friendly file / data.
		Float durabilityReduction = randGen.Get( 20.0f, 35.0f );

		SetItemDurability( itemId, itemDurability - durabilityReduction );

		SInventoryItem::SItemModifierVal* itemMod = item.GetItemMod( CNAME( ItemDurabilityModified ), true );
		itemMod->m_type = SInventoryItem::SItemModifierVal::EMVT_Int;
		itemMod->m_int = 1;
	}
}

Float CInventoryComponent::GetItemInitialDurability( SItemUniqueId itemId ) const
{
	const SInventoryItem* invItem  = GetItem( itemId );
	if ( !invItem )
	{
		//RED_ASSERT( invItem != nullptr, TXT( "Cannot find item with id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find item with id: %d" ), itemId );
		return -1;
	}
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );
	if ( !itemDef )
	{
		//RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		ERR_GAME( TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		return -1;
	}
	return itemDef->GetInitialDurability( IsPlayerOwner() );
}

Float CInventoryComponent::GetItemMaxDurability( const SInventoryItem& invItem ) const
{
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem.GetName() );

	if ( !itemDef )
	{
		//RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		ERR_GAME( TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem.GetName().AsChar(), GetEntity()->GetName().AsChar() );
		return -1;
	}

	return itemDef->GetMaxDurability( IsPlayerOwner() );
}

Float CInventoryComponent::GetItemMaxDurability( SItemUniqueId itemId ) const
{
	const SInventoryItem* invItem	= GetItem( itemId );
	if ( !invItem )
	{
		//RED_ASSERT( invItem != nullptr, TXT( "Cannot find item with id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find item with id: %d" ), itemId );
		return -1;
	}

	return GetItemMaxDurability( *invItem );
}

Uint32 CInventoryComponent::GetItemGridSize( SItemUniqueId itemId ) const
{
	const SInventoryItem* invItem  = GetItem( itemId );
	if ( !invItem )
	{
		//RED_ASSERT( invItem != nullptr, TXT( "Cannot find item with id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find item with id: %d" ), itemId );
		return 0;
	}
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );
	if ( !itemDef )
	{
		//RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		ERR_GAME( TXT( "Cannot find item definition for item name: %s in entity %s" ), invItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );
		return 0;
	}
	return itemDef->GetGridSize();
}

Bool CInventoryComponent::FillIndices( TDynArray< Int32 >& outIndices ) const
{
	const Uint32 itemsCount = m_items.Size();
	if( itemsCount > 0 )
	{
		outIndices.Resize( itemsCount );
		for ( Uint32 i = 0; i < itemsCount; ++i )
		{	
			outIndices[ i ] = Int32( i );
		}

		return true;
	}

	return false;
}

TDynArray< SItemUniqueId > CInventoryComponent::GiveItem( CInventoryComponent* otherInventory, SItemUniqueId itemId, Int32 quantity /*=1*/ )
{
	RED_ASSERT( otherInventory != this, TXT( "Trying to give items to the same inventory" ) );

    TDynArray< SItemUniqueId > returnVals;

	if ( !otherInventory )
	{
        returnVals.PushBack(SItemUniqueId::INVALID);
		return returnVals;
	}

	if ( quantity < 1 )
	{
        ITEM_WARN( TXT("CInventoryComponent::GiveItem - quantity is %i, skipping. Entity: %s"), quantity, GetEntity()->GetFriendlyName().AsChar() );
        returnVals.PushBack(SItemUniqueId::INVALID);
        return returnVals;
	}

	CallEvent( CNAME( OnItemAboutToGive ), itemId, quantity );

	Int32 index = UniqueIdToIndex( itemId );
	if ( IsIndexValid( index ) )
	{
		SInventoryItem copiedItem;
		{
			const SInventoryItem& item = m_items[ index ];

			// Avoid giving bigger quantity than we have
			if ( item.GetQuantity() < (Uint32)quantity )
			{
				quantity = item.GetQuantity();
			}

			// Clone item data and quantity
			copiedItem.CloneOf( item, SItemUniqueId::INVALID );
			copiedItem.SetQuantity( quantity );
		}
		
		// Below code doesn't allow to take too heavy items by the player.
		// For now it is commented as this feature isn't needed, but
		// I didn't remove that code yet as maybe in the near future
		// designers will change their minds (again).
		// For player check if an item isn't to heavy
		// CPlayer *player = Cast< CPlayer >( otherInventory->GetEntity() );
		//if ( player )
		//{
		//	if ( !CanPlayerCarryItem(player, item.m_name, quantity) )
		//	{
		//		player->CallEvent( CNAME(OnItemTooHeavy), item.m_name );
		//		return SItemUniqueId::INVALID;
		//	}
		//}

		SAddItemInfo addItemInfo;
		addItemInfo.m_quantity = quantity;
		TDynArray< SItemUniqueId > newItemId = otherInventory->AddItem( copiedItem, addItemInfo );

		CEntity* otherEntity = otherInventory->GetEntity();
		if( otherEntity )
		{
			W3Container* otherContainer = Cast< W3Container >( otherEntity );
			if( otherContainer )
			{
				for( Uint32 k = 0; k < newItemId.Size(); ++k )
				{
					SInventoryItem* item = otherInventory->GetItem( newItemId[ k ] );
					if( item )
					{
						item->SetFlag( SInventoryItem::FLAG_REBALANCE, false );
					}
				}

				otherContainer->SetShouldBeFullySaved( true );
			}
		}

		RemoveItem( itemId, quantity );
		return newItemId;
	}
	else
	{
        ITEM_WARN( TXT("CInventoryComponent::GiveItem - wrong item index, entity: %s"), GetEntity()->GetFriendlyName().AsChar() );
        returnVals.PushBack(SItemUniqueId::INVALID);
        return returnVals;
	}
}

Bool CInventoryComponent::GiveItems( CInventoryComponent* otherInventory, const TDynArray< Int32 >& indices )
{
	RED_ASSERT( otherInventory, TXT( "The spawned entity that gets items cannot have streamed inventory!!" ) );
	RED_ASSERT( otherInventory != this, TXT( "Trying to give items to the same inventory" ) );
	if( indices.Size() == 0 )
	{
		return false;
	}

	TDynArray< Int32 > sortedIndices = indices;
	Sort( sortedIndices.Begin(), sortedIndices.End() );

	Bool ok = true;

	// Copy items to other inventory
	for( Int32 i=(Int32)sortedIndices.Size()-1; i>=0; i-- )
	{
		Int32 idx = sortedIndices[i];
		if( IsIndexValid( idx ) )
		{
			HideItem( idx );
			if( otherInventory )
			{
				otherInventory->AddItem( m_items[idx] );
			}
		}
		else
		{
			ok = false;
		}
	}

	CEntity* otherEntity = otherInventory->GetEntity();
	if( otherEntity )
	{
		W3Container* otherContainer = Cast< W3Container >( otherEntity );
		if( otherContainer )
		{
			otherContainer->SetShouldBeFullySaved( true );
		}
	}

	// Remove from this inventory
	for( Int32 i=(Int32)sortedIndices.Size()-1; i>=0; i-- )
	{
		Int32 idx = sortedIndices[i];
		if( IsIndexValid( idx ) )
		{
			const SInventoryItem& item = m_items[ idx ];
			RemoveItem( item.GetUniqueId(), item.GetQuantity() );
		}
	}

	return ok;
	//return true;
}

SItemUniqueId CInventoryComponent::SplitItem( SItemUniqueId itemId, Int32 qty )
{
	const Int32 index = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( index ) )
	{
		return SItemUniqueId::INVALID;
	}
	
	SInventoryItem newItem;
	
	SInventoryItem& item = m_items[ index ];
	if ( item.GetQuantity() < 2 )
	{
		// item can not be split
		return SItemUniqueId::INVALID;
	}

	// clamp new quantity to current qantity - 1
	qty = Clamp( qty, 1, Min( Int32( item.GetQuantity() ) - 1, qty ) );

	// clone item 
	SItemUniqueId newID = m_uniqueIdGenerator.GenerateNewId();
	newItem.CloneOf( item, newID );
	newItem.SetQuantity( qty );

	// reduce quantity of the old one
	item.SetQuantity( item.GetQuantity() - qty );
	NotifyListeners( IET_ItemQuantityChanged, itemId, -qty, false );

	// add that new item directly (HACK ALERT!)
	m_items.PushBack( newItem );
	
	// notify 
	TDynArray< SItemUniqueId > ids;
	ids.PushBack( newID );
	
	SItemChangedData changeData =  SItemChangedData( item.GetName(), qty, ids, true );
	OnItemAdded( changeData );

	// return created id
	return newID;
}

void CInventoryComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	RED_ASSERT( world );
	m_currentLOD = ILODable::LOD_2; // to init by LOD0 init
	world->GetComponentLODManager().Register( this, !m_turnOffSpawnItemsBudgeting );
	m_nextRebalance = 0;

	if ( GetParent() != nullptr && GetParent()->IsA< CGameplayEntity >() )
	{
		static_cast< CGameplayEntity* >( GetParent() )->InitInventory( true );
	}
}

void CInventoryComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	RED_ASSERT( world );
	world->GetComponentLODManager().Unregister( this, !m_turnOffSpawnItemsBudgeting );
	DespawnAllItems();
	BreakAllAttachments();
	m_listeners.ClearFast();
	ClearInventory();
}

void CInventoryComponent::OnDestroyed()
{
	BreakAllAttachments();
	TBaseClass::OnDestroyed();
}

void CInventoryComponent::OnEntityLoaded()
{
	TBaseClass::OnEntityLoaded();
}

void CInventoryComponent::OnPropertyPreChange( IProperty* property )
{
	TBaseClass::OnPropertyPreChange( property );
}

void CInventoryComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

Bool CInventoryComponent::HasItem( CName itemName ) const
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		if( m_items[ i ].GetName() == itemName )
		{
			return true;
		}
	}

	return false;
};

TDynArray< SItemUniqueId > CInventoryComponent::AddItem( CName itemName, const SAddItemInfo& addItemInfo )
{
    TDynArray< SItemUniqueId > returnVals;
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	const SItemDefinition* itemDef = defMgr ? defMgr->GetItemDefinition( itemName ) : nullptr;

	if( itemDef && addItemInfo.m_quantity > 0 )
	{
		SInventoryItemInitData itemInitData( IsPlayerOwner() );
		itemInitData.m_quantity = addItemInfo.m_quantity;
		itemInitData.m_isLootable = addItemInfo.m_isLootable;
		itemInitData.m_markAsNew = addItemInfo.m_markAsNew;
		itemInitData.m_shouldBeRebalanced = addItemInfo.m_shouldBeRebalanced;
		SInventoryItem item( itemName, *itemDef, itemInitData );
        return AddItem( item, addItemInfo );
	}
	else
	{
        RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name %s in entity %s" ), itemName.AsChar(), GetEntity()->GetName().AsChar() );
		RED_ASSERT( addItemInfo.m_quantity > 0, TXT( "Trying to add invalid quantity of item: %s quantity: %d in entity %s"), itemName.AsChar(), addItemInfo.m_quantity, GetEntity()->GetName().AsChar() );
        returnVals.PushBack(SItemUniqueId::INVALID);
        return returnVals;
	}
}

TDynArray< SItemUniqueId > CInventoryComponent::AddItem( const SInventoryItem& item, const SAddItemInfo& addItemInfo )
{
    TDynArray< SItemUniqueId > returnVals;
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );

	RED_ASSERT( item.GetQuantity() > 0, TXT( "Trying to add invalid quantity of item %s quantity: %d in entity %s" ), item.GetName().AsChar(), item.GetQuantity(), GetEntity()->GetName().AsChar() );
	if ( item.GetQuantity() < 1 )
	{
        ITEM_WARN( TXT("AddItem called with quantity %i, skipping."), item.GetQuantity() );
        returnVals.PushBack(SItemUniqueId::INVALID);
        return returnVals;
	}

	if ( item.IsStackable() )
	{
		Int32 totalItemQuantity = item.GetQuantity();

		// find stack size for this item
		Uint32 stackSize = 1;
		const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
		if ( defMgr )
		{
			const SItemDefinition* itemDef = defMgr->GetItemDefinition( item.GetName() );
			RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), item.GetName().AsChar(), GetEntity()->GetName().AsChar() );
			if ( itemDef )
			{
				stackSize = itemDef->GetStackSize();
			}
		}

		// Try to find the identical item
		const Uint32 itemsCount = m_items.Size();
		for ( Uint32 i = 0; i < itemsCount; ++i )
		{
			SInventoryItem& itemIter = m_items[ i ];
			if ( itemIter.IsStackable() && itemIter.GetName() == item.GetName() && CompareItems( &itemIter, &item ) )
			{
				const SItemUniqueId uniqueId = itemIter.GetUniqueId();
				Int32 availableSpaceOnThisStack = stackSize - itemIter.GetQuantity();
				if ( availableSpaceOnThisStack > 0 )
				{
					Int32 addedItemQuantity = Min( availableSpaceOnThisStack, totalItemQuantity );			
					itemIter.ChangeQuantity( addedItemQuantity );
					NotifyListeners( IET_ItemQuantityChanged, itemIter.GetUniqueId(), addedItemQuantity, false );
					returnVals.PushBack(uniqueId);
					totalItemQuantity -= addedItemQuantity;
					if ( totalItemQuantity <= 0 )
					{
						break;
					}
				}
			}
		}

		// If there's still some quantity yo add, create new items
		while ( totalItemQuantity > 0 )
		{			
			// We need to calculate and update quantity to add before checking if item was looted
			Int32 addedItemQuantity = Min< Int32 >( totalItemQuantity, stackSize );
			totalItemQuantity -= addedItemQuantity;
			SItemUniqueId newID = m_uniqueIdGenerator.GenerateNewId();
			if ( !WasItemLooted( newID.GetValue() ) )
			{
				SInventoryItem addedItem;
				addedItem.CloneOf( item, newID );
				addedItem.SetQuantity( addedItemQuantity );
				m_items.PushBack( addedItem );
				returnVals.PushBack( addedItem.GetUniqueId() );
			}
		}
	}
    else
    {
        for ( Uint32 i = 0; i < addItemInfo.m_quantity; ++i )
        {
			SItemUniqueId newID = m_uniqueIdGenerator.GenerateNewId();
			if( !WasItemLooted( newID.GetValue() ) )
			{
				SInventoryItem addedItem;
				addedItem.CloneOf( item, newID );
				addedItem.SetQuantity( 1 );
				m_items.PushBack( addedItem );
				returnVals.PushBack( addedItem.GetUniqueId() );
			}
        }
    }

	SItemChangedData data( item.GetName(), addItemInfo.m_quantity, returnVals, addItemInfo.m_informGui );
	OnItemAdded( data );
	return returnVals;
}

SItemUniqueId CInventoryComponent::AddFakeItem( CName itemName, CItemEntityProxy* proxy )
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	const SItemDefinition* itemDef = defMgr != nullptr ? defMgr->GetItemDefinition( itemName ) : nullptr;
	if ( itemDef )
	{
		SItemUniqueId alreadyPosessedFakeItemId = GetItemId( itemName );
		if ( alreadyPosessedFakeItemId )
		{
			// if there is already such item, we destroy its proxy, and take proxy from other inventory
			SInventoryItem* alreadyPosessedFakeItem = GetItem( alreadyPosessedFakeItemId );
			alreadyPosessedFakeItem->SetProxy( proxy, true );
			return alreadyPosessedFakeItemId;
		}

		// Add the item normally if it is not stackable or it's the first one
		SInventoryItemInitData itemInitData( IsPlayerOwner() );
		itemInitData.m_itemEntityProxy = proxy;
		itemInitData.m_isCloned = true;
		itemInitData.m_uniqueId = m_uniqueIdGenerator.GenerateNewId();
		SInventoryItem item( itemName, *itemDef, itemInitData );
		m_items.PushBack( item );

		TDynArray< SItemUniqueId > id;
		id.PushBack( item.GetUniqueId() );
		SItemChangedData data( itemName, 1, id, false );
		OnItemAdded( data );
		return id[0];
	}
	else
	{
		RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), itemName.AsChar(), GetEntity()->GetName().AsChar() );
		return SItemUniqueId::INVALID;
	}
}

void CInventoryComponent::RemoveFakeItem( SItemUniqueId itemId )
{
	auto itemIter = m_items.Begin(), lastIter = m_items.End();
	for ( ; itemIter != lastIter; ++itemIter )
	{
		if ( itemIter->GetUniqueId() == itemId )
		{			
			OnItemRemoved( itemId, 1 );
			m_items.Erase( itemIter );
			NotifyListeners( IET_ItemRemoved, itemId, -1, false );
			return;
		}
	}
}

Bool CInventoryComponent::CompareItems( const SInventoryItem* itemA, const SInventoryItem* itemB ) const
{
	if ( itemA->GetName() != itemB->GetName() )
	{
		return false;
	}

	// Collect all abilities from both items
	TDynArray<CName> allAbilitiesA, allAbilitiesB;
	itemA->GetAllAbilities( allAbilitiesA );
	itemB->GetAllAbilities( allAbilitiesB );

	if ( allAbilitiesA != allAbilitiesB )
	{
		// Abilities lists don't match, items are not equal
		return false;
	}

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( defMgr == nullptr )
	{
		return false;
	}

	THashMap< CName, SAbilityAttributeValue > modifiersA, modifiersB;
	
	// let definitions manager calculate exact modifiers values for item A
	defMgr->CalculateAttributeModifiers( modifiersA, allAbilitiesA, itemA->GetStaticRandomSeed() );
	// let definitions manager calculate exact modifiers values for item B
	defMgr->CalculateAttributeModifiers( modifiersB, allAbilitiesB, itemB->GetStaticRandomSeed() );
	
	if ( modifiersA.Size() != modifiersB.Size() )
	{
		// Not equal number of modificators, items are not equal
		return false;
	}

	THashMap< CName, SAbilityAttributeValue >::const_iterator modIterA = modifiersA.Begin();
	THashMap< CName, SAbilityAttributeValue >::const_iterator modIterB = modifiersB.Begin();

	for ( ; modIterA != modifiersA.End(); ++modIterA, ++modIterB )
	{
		const SAbilityAttributeValue& valA = modIterA->m_second;
		const SAbilityAttributeValue& valB = modIterB->m_second;

		if ( MAbs( valA.m_valueAdditive - valB.m_valueAdditive )			 > NumericLimits<Float>::Epsilon() ||
			 MAbs( valA.m_valueMultiplicative - valB.m_valueMultiplicative ) > NumericLimits<Float>::Epsilon() ||
			 MAbs( valA.m_valueBase - valB.m_valueBase )					 > NumericLimits<Float>::Epsilon() )
		{
			// Some modifiers are not equal, items are not equal
			return false;
		}
	}

	if ( itemA->GetSlotItems() != itemB->GetSlotItems() )
	{
		// Not equal item enhancements, items are not equal
		return false;
	}

	if ( itemA->GetCraftedAbilities() != itemB->GetCraftedAbilities() )
	{
		// Not equal crafted abilities, items are not equal
		return false;
	}

	// Items seem equal
	return true;
}

Bool CInventoryComponent::SetPreviewColor( SItemUniqueId itemId, Uint32 colorId )
{
	SInventoryItem* invItem = GetItem( itemId );
	
	if ( invItem )
	{
		invItem->SetDyePreviewColor( colorId );
		return true;
	}

	return false;
}

Bool CInventoryComponent::ClearPreviewColor( SItemUniqueId itemId )
{
	SInventoryItem* invItem = GetItem( itemId );

	if ( invItem )
	{
		invItem->ClearDyePreviewColor();
		return true;
	}

	return false;
}

void CInventoryComponent::funcSetPreviewColor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Uint32, colorId, 0 );

	FINISH_PARAMETERS;

	RETURN_BOOL( SetPreviewColor( itemId, colorId ) );
}

void CInventoryComponent::funcClearPreviewColor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );

	FINISH_PARAMETERS;

	ClearPreviewColor( itemId );
}

Bool CInventoryComponent::ColorItem( SItemUniqueId itemId, SItemUniqueId dyeId )
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( nullptr == defMgr )
	{
		ERR_GAME( TXT( "Definitions Manager is NULL" ) );
		return false;
	}

	// Get Item to be colored
	Int32 itemIndex = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( itemIndex ) )
	{
		ITEM_WARN( TXT("Trying to enchant Item with invalid item id") );
		return false;
	}

	// Get Dye for Color Info
	if ( const SInventoryItem* dyeItem = GetItem( dyeId ) )
	{
		SAbilityAttributeValue attValue;
		TDynArray< CName > allAbilities;
		dyeItem->GetAllAbilities( allAbilities );

		attValue = defMgr->CalculateAttributeValue( allAbilities, CNAME( item_color ), false, 0 );

		SInventoryItem& invItem = m_items[ itemIndex ];

		invItem.SetDyeColor( dyeItem->GetName(), (Uint32)attValue.m_valueBase );

		if( CItemEntity* ent = invItem.GetItemEntity() )
		{
			if ( CAppearanceComponent* appearanceComponent = ent->FindComponent< CAppearanceComponent >() )
			{
				appearanceComponent->ApplyAppearance( invItem.GetDyeColorStats() );
			}
		}

		return true;
	}

	return false;
}

Bool CInventoryComponent::ClearItemColor( SItemUniqueId itemId )
{
	Int32 itemIndex = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( itemIndex ) )
	{
		ITEM_WARN( TXT("Trying to enchant Item with invalid item id") );
		return false;
	}

	SInventoryItem& invItem = m_items[ itemIndex ];
	CName colorStat = invItem.GetDyeColorStats();

	invItem.ClearDyeColor();

	return true;
}

Bool CInventoryComponent::HasEnhancementItemTag( SItemUniqueId enhancedItemId, Uint32 itemSlotIndex, CName tag ) const
{
	Int32 enhancedIdx = UniqueIdToIndex( enhancedItemId );
	if ( !IsIndexValid( enhancedIdx ) )
	{
		ITEM_WARN( TXT("Trying to remove slot item using invalid item id") );
		return false;
	}

	const SInventoryItem& enhancedItem = m_items[enhancedIdx];

	TDynArray< CName > names;
	if ( !enhancedItem.GetSlotItemsNames( names ) || names.Size() <= itemSlotIndex )
	{
		return false;
	}

	const SItemDefinition* enhancedItemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( names[ itemSlotIndex ] );
	if ( !enhancedItemDef )
	{
		return false;
	}
	return enhancedItemDef->GetItemTags().Exist( tag );
}

Bool CInventoryComponent::EnchantItem( SItemUniqueId itemId, CName enchantmentName, CName enchantmentStat )
{
	Int32 itemIndex = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( itemIndex ) )
	{
		ITEM_WARN( TXT("Trying to enchant Item with invalid item id") );
		return false;
	}

	SInventoryItem& invItem = m_items[ itemIndex ];

	invItem.SetEnchantment( enchantmentName, enchantmentStat );

	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
	if ( gameplayEntity != nullptr )
	{
		gameplayEntity->OnItemAbilityAdded( itemId, enchantmentStat );
	}

	return true;
}

Bool CInventoryComponent::UnenchantItem( SItemUniqueId itemId )
{
	Int32 itemIndex = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( itemIndex ) )
	{
		ITEM_WARN( TXT("Trying to enchant Item with invalid item id") );
		return false;
	}

	SInventoryItem& invItem = m_items[ itemIndex ];
	CName enchantmentStat = invItem.GetEnchantmentStats();

	if (enchantmentStat != CName::NONE)
	{
		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
		if ( gameplayEntity != nullptr )
		{
			gameplayEntity->OnItemAbilityRemoved( itemId, enchantmentStat );
		}
	}

	invItem.ClearEnchantment();

	return true;
}

Bool CInventoryComponent::EnhanceItem( SItemUniqueId enhancedItemId, SItemUniqueId extensionItemId )
{
	Int32 enhancedIdx = UniqueIdToIndex( enhancedItemId );
	Int32 extensionIdx = UniqueIdToIndex( extensionItemId );
	if ( !IsIndexValid( enhancedIdx ) || !IsIndexValid( extensionIdx ) )
	{
		ITEM_WARN( TXT("Trying to perform item enhancement with invalid item ids") );
		return false;
	}

	SInventoryItem& enhancedItem = m_items[enhancedIdx];
	SInventoryItem& extensionItem = m_items[extensionIdx];

	const SItemDefinition* enhancedItemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( enhancedItem.GetName() );
	if ( !enhancedItemDef )
	{
		return false;
	}

	if ( enhancedItem.GetQuantity() > 1 )
	{
		// Enhance single item
		SInventoryItem newItem;
		newItem.CloneOf( enhancedItem, SItemUniqueId::INVALID );
		enhancedItem = newItem;
	}

	// Attach item to extension slot
	if ( !enhancedItem.AddSlotItem( extensionItem.GetName(), IsPlayerOwner(), extensionItem.GetStaticRandomSeed() ) )
	{
		ITEM_WARN( TXT("Failed to enhance item %s with item %s, all slots are occupied, or no slots at all"), enhancedItem.GetName().AsString().AsChar(), extensionItem.GetName().AsString().AsChar() );
		return false;
	}

	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
	if ( gameplayEntity )
	{
		Int32 slotIndex = enhancedItem.GetSlotItemIndex( extensionItem.GetName() );
		// Let entity perform additional setup
		gameplayEntity->OnEnhanceItem( enhancedItemId, slotIndex );
	}

	// Remove extension item from inventory, as it is treated as part of the enhanced item
	RemoveItem( extensionItemId, 1 );

	// done
	return true;
}

Bool CInventoryComponent::RemoveEnhancementItem( SItemUniqueId enhancedItemId, CName extensionItemName )
{
	Int32 enhancedIdx = UniqueIdToIndex( enhancedItemId );
	if ( !IsIndexValid( enhancedIdx ) )
	{
		ITEM_WARN( TXT("Trying to remove slot item using invalid item id") );
		return false;
	}

	SInventoryItem& enhancedItem = m_items[enhancedIdx];

	Int32 slotIndex = enhancedItem.GetSlotItemIndex( extensionItemName );

	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
	if ( gameplayEntity )
	{
		// Let entity perform additional setup
		gameplayEntity->OnRemoveEnhancementItem( enhancedItemId, slotIndex );
	}

	// Remove extension item
	if ( !enhancedItem.RemoveSlotItem( extensionItemName ) )
	{
		ITEM_WARN( TXT("Extension item %s not found in %s, extension removal failed"), extensionItemName.AsString().AsChar(), enhancedItem.GetName().AsString().AsChar() );
		return false;
	}

	// Spawning extension item back in inventory is designers choice
	return true;
}

Bool CInventoryComponent::RemoveEnhancementItem( SItemUniqueId enhancedItemId, Uint32 itemSlotIndex )
{
	Int32 enhancedIdx = UniqueIdToIndex( enhancedItemId );
	if ( !IsIndexValid( enhancedIdx ) )
	{
		ITEM_WARN( TXT("Trying to remove slot item using invalid item id") );
		return false;
	}
	
	SInventoryItem& enhancedItem = m_items[enhancedIdx];

	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
	if ( gameplayEntity )
	{
		// Let entity perform additional setup
		gameplayEntity->OnRemoveEnhancementItem( enhancedItemId, itemSlotIndex );
	}

	// Check index validity
	if ( !enhancedItem.RemoveSlotItem( itemSlotIndex ) )
	{
		ITEM_WARN( TXT("Removing extension item: item %s has no item in slot %i"), enhancedItem.GetName().AsString().AsChar(), itemSlotIndex );
		return false;
	}

	// Spawning extension item back in inventory is designers choice
	return true;
}

Bool CInventoryComponent::RemoveItem( SItemUniqueId itemId, Int32 quantity /*=1*/ )
{
	RED_ASSERT( quantity > 0, TXT( "Trying to remove invalid quantity %d of item with id %d in entity %s" ), quantity, itemId, GetEntity()->GetName().AsChar() );
	if ( quantity < 1 )
	{
		ITEM_WARN( TXT("RemoveItem called with quantity %i, skipping."), quantity );
		return false;
	}

	Int32 idx = UniqueIdToIndex( itemId );

	// Index valid and not a default item
	if( IsIndexValid( idx ) && m_items[idx].GetName() != GetCategoryDefaultItem( m_items[idx].GetCategory() ) )
	{
		OnItemRemoved( itemId, quantity );

		// items are possibly changed
		idx = UniqueIdToIndex( itemId );

		if( IsIndexValid( idx ) )
		{
			SItemUniqueId removedId = m_items[idx].GetUniqueId();

			if ( m_items[idx].IsStackable() && m_items[idx].GetQuantity() > static_cast< Uint32 >( quantity ) )
			{
				m_items[idx].ChangeQuantity( -quantity );
				NotifyListeners( IET_ItemQuantityChanged, removedId, -quantity, false );
			}
			else
			{
				HideItem( idx );		
				m_items.RemoveAt( idx );	
				NotifyListeners( IET_ItemRemoved, removedId, -quantity, false );
			}

			return true;
		}
	}
	return false;
}

void CInventoryComponent::RemoveAllItems()
{
	TDynArray< SItemUniqueId >	uniqueIds;
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		uniqueIds.PushBack( m_items[i].GetUniqueId() );
	}

	const Uint32 uniqueIdsCount = uniqueIds.Size();
	for ( Uint32 i = 0; i < uniqueIdsCount; ++i )
	{
		const SItemUniqueId& uniqueId = uniqueIds[ i ];
		if ( IsIndexValid( UniqueIdToIndex( uniqueId ) ) )
		{
			RemoveItem( uniqueId, GetItemQuantity( uniqueId ) );
		}
	}
}

CItemEntity* CInventoryComponent::GetItemEntityUnsafe( SItemUniqueId itemId )
{
	SInventoryItem* item = GetItem( itemId );

	if ( !item )
	{
		return NULL;
	}

	if ( !item->GetItemEntityProxy() )
	{
		return NULL;
	}

	return SItemEntityManager::GetInstance().GetItemEntity( item->GetItemEntityProxy() );
}

CEntity* CInventoryComponent::GetDeploymentItemEntity( SItemUniqueId itemId, const Vector& position, const EulerAngles& rotation, Bool allocateIdTag )
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( index ) )
	{
		ITEM_ERR( TXT("Used invalid unique id in %s, make sure the item is still in this inventory"), GetEntity()->GetFriendlyName().AsChar() );
		return NULL;
	}

	CName itemName = m_items[index].GetName();
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName );
	if ( !itemDef )
	{
		return NULL;
	}

	const String& templatePath = GCommonGame->GetDefinitionsManager()->TranslateTemplateName( itemDef->GetHoldTemplateName( IsPlayerOwner() ) );
	if ( templatePath == String::EMPTY )
	{
		return NULL;
	}

	// Look for preloaded template
	THandle< CEntityTemplate > entityTemplate = SItemEntityManager::GetInstance().GetPreloadedEntityTemplate( itemName, templatePath );
	if ( !entityTemplate )
	{
		// Template is not preloaded, load sync
		entityTemplate = LoadResource< CEntityTemplate >( templatePath );
	}

	// Failed
	if ( !entityTemplate )
	{
		return NULL;
	}

	// Attach itemEntity
	HardAttachmentSpawnInfo info;
	info.m_parentSlotName = itemDef->GetEquipSlot( IsPlayerOwner() );
	
	// Template ready, spawn entity now!
	EntitySpawnInfo spawnInfo;
	spawnInfo.m_template = entityTemplate;
	spawnInfo.m_spawnPosition = position;
	spawnInfo.m_spawnRotation = rotation;	
	spawnInfo.m_entityFlags = EF_DestroyableFromScript;
	if ( allocateIdTag )
	{
		spawnInfo.m_entityFlags |= EF_ManagedEntity;
		spawnInfo.m_idTag = GGame->GetIdTagManager()->Allocate();
	}

	CLayer* dynamicLayer = GetEntity()->GetLayer()->GetWorld()->GetDynamicLayer();

	CEntity* spawnedEntity = dynamicLayer->CreateEntitySync( spawnInfo );
	if ( !spawnedEntity )
	{
		ITEM_ERR( TXT("Failed to spawn deployment entity for item %s"), itemName.AsString().AsChar() );
		return NULL;
	}
	return spawnedEntity;
}

void CInventoryComponent::SpawnItemIfMounted( SItemUniqueId itemId )
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( IsIndexValid( index ) == true )
	{
		SpawnItemIfMounted( index );
	}
}

void CInventoryComponent::SpawnItemIfMounted( Int32 itemIndex )
{
	const SInventoryItem& item = m_items[ itemIndex ];

	if( ( item.IsMounted() || item.IsHeld() ) && !item.GetItemEntityProxy() && !item.IsProxyTaken() )
	{
		if ( item.HasEntityDefined() )
		{
			Bool toHand = item.IsHeld();

			SMountItemInfo mountInfo;
			mountInfo.m_toHand = toHand;
			MountItem( item.GetUniqueId(), mountInfo );
		}
		else
		{
			SItemUniqueId itemId = item.GetUniqueId();
			Bool spawn = false;
			CallFunctionRet< Bool >( this, CNAME( ForceSpawnItemOnStart ), itemId, spawn );

			if ( spawn )
			{
				SMountItemInfo mountInfo;
				mountInfo.m_toHand = item.IsHeld();
				mountInfo.m_force = true;
				MountItem( item.GetUniqueId(), mountInfo );
			}
		}
	}
}

void CInventoryComponent::SpawnMountedItems()
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		if ( m_items[ i ].GetName() != CName::NONE )
		{
			SpawnItemIfMounted( i );			
		}
	}
}

void CInventoryComponent::HideAllItems()
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		HideItem( i );
	}
}

void CInventoryComponent::HideAllItemsRaw()
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		m_items[ i ].DestroyProxy();
	}
}

void CInventoryComponent::HideItem( Int32 itemIndex )
{
	const SInventoryItem& item = m_items[ itemIndex ];
	if ( item.IsMounted() || item.IsHeld() )
	{
		UnMountItem( item.GetUniqueId(), true );
	}	
}

CName CInventoryComponent::GetItemEquipSlotName( SItemUniqueId itemId )
{
	SInventoryItem* item = GetItem( itemId );

	if ( !item )
	{
		return CName::NONE;
	}

	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );
	if ( !itemDef )
	{
		return CName::NONE;
	}

	return itemDef->GetEquipSlot(  IsPlayerOwner() );
}


void CInventoryComponent::SetItemVisible( SItemUniqueId itemId, Bool flag )
{
	if( SInventoryItem* item = GetItem( itemId ) )
	{
		item->SetIsInvisible( !flag );
		if( CItemEntity* ent = item->GetItemEntity() )
		{
			ent->SetHideInGame( !flag, true );
		}
	}
}

Bool CInventoryComponent::MountItem( SItemUniqueId itemId, const SMountItemInfo& info )
{
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );	
	SInventoryItem* item ;
	const SItemDefinition* itemDef ;

	if(	(item = GetItem( itemId )) == NULL ||
		(itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() )) == NULL )
	{
		ITEM_ERR( TXT("CInventoryComponent::MountItem - error mounting item") );
		return false;
	}

	Bool wasHeld = item->IsHeld();
	CName attachSlot = info.m_toHand ? itemDef->GetHoldSlot( IsPlayerOwner() ) : itemDef->GetEquipSlot(  IsPlayerOwner() );
	if ( info.m_slotOverride != CName::NONE )
	{
		attachSlot = info.m_slotOverride;
	}

	//don't equip default item if other item is equipped - unequip current item first
	const CName itemCategory = item->GetCategory();
	SItemUniqueId currentItemid = GetItemByCategory( itemCategory, true, true );
	const CName defaultItemName = GetCategoryDefaultItem( itemCategory );
	if ( defaultItemName == item->GetName() && currentItemid != SItemUniqueId::INVALID && !info.m_force && !info.m_forceOverwritingCurrentItem )
	{
		return false;
	}

	// TODO: Rethink this
	// If it is a grab, check if the hold slot is empty and clear it if necessary
	if ( info.m_toHand )
	{
		SItemUniqueId itemInHoldSlotId = GetItemIdHeldInSlot( attachSlot );
		if ( itemInHoldSlotId && itemInHoldSlotId != currentItemid )
		{
			SInventoryItem* heldItem = GetItem( itemInHoldSlotId );
			RED_ASSERT( heldItem != nullptr, TXT( "Cannot find item with id: %d" ), itemInHoldSlotId );
			if ( GetItemEquipSlotName( itemInHoldSlotId ) )
			{
				ITEM_LOG( TXT("MountItem: Hand slot %s was occupied by item %s when grabbing item %s, the previous item is being mount to holster slot"), attachSlot.AsString().AsChar(), heldItem->GetName().AsString().AsChar(), item->GetName().AsString().AsChar() );
				MountItem( itemInHoldSlotId, SMountItemInfo() );
			}
			else
			{
				ITEM_LOG( TXT("MountItem: Hand slot %s was occupied by item %s when grabbing item %s, the previous item is being unmount"), attachSlot.AsString().AsChar(), heldItem->GetName().AsString().AsChar(), item->GetName().AsString().AsChar() );
				// the last argument (emptyHand = false) states, that hand won't be emptied
				UnMountItem( itemInHoldSlotId, true, false );
			}
		}
	}

	if( attachSlot )
	{
		currentItemid = GetItemIdHeldInSlot( attachSlot );
	}
	else
	{
		currentItemid = GetItemByCategory( itemCategory, true, false );
	}
	
	SInventoryItem* currentItem = GetItem( currentItemid );

	if ( currentItem != nullptr && !info.m_force )
	{ 
		// Let gameplay entity perform custom onPut/onUnmount stuff
		if ( gameplayEntity )
		{
			currentItem->IsHeld() ? gameplayEntity->OnPutItem( currentItemid, false ) : gameplayEntity->OnUnmountItem( currentItemid );
		}
		currentItem->SetIsMounted( false );
		currentItem->SetIsHeld( false );
	}

	if ( wasHeld && !info.m_toHand )
	{
		gameplayEntity->OnPutItem( itemId, true );
	}

	// obtain valid pointers (items array may be reallocated in OnXXX events)
	currentItem = GetItem( currentItemid );
	item = GetItem( itemId );

	Bool done = false;
	if ( ( currentItemid != itemId || info.m_forceOverwritingCurrentItem ) && currentItem != nullptr && currentItem->GetItemEntityProxy() && !info.m_force && !currentItem->IsCloned() )
	{
		if ( item != nullptr )
		{
			// if we just want to change template in item
			if ( currentItemid == itemId )
			{
				TDynArray< CName > slotItems;
				item->GetSlotItemsNames( slotItems );
				SItemEntityManager::GetInstance().ChangeProxyItem( item->GetItemEntityProxy(), item->GetName(), attachSlot, slotItems, GetTemplate( item->GetName() ), ShouldCollapse( item->GetName() ), item->GetUniqueId() );
				done = true;
			}
			// Current item has entity, destroy it or exploit it eventually ...
			else if ( !item->HasEntityDefined() || item->GetItemEntityProxy() )
			{
				// We have no use for the current item entity, screw it!
				currentItem->DestroyProxy();
			}
			else if ( item->HasEntityDefined() )
			{
				// Great, previous item has entity spawned, let's steal it's proxy and use it until the new item entity gets spawned
				// This way we avoid gap between destroying previous item entity and spawning new ( for example new huge and heavy armor )
				// (there is a slight possibility that this proxy has not yet spawned entity, but item entity manager will handle that case properly )
				item->MoveProxy( currentItem );
				TDynArray< CName > slotItems;
				item->GetSlotItemsNames( slotItems );
				SItemEntityManager::GetInstance().ChangeProxyItem( item->GetItemEntityProxy(), item->GetName(), attachSlot, slotItems, GetTemplate( item->GetName() ), ShouldCollapse( item->GetName() ), item->GetUniqueId() );
				done = true;
			}
		}
		const SItemDefinition* currentItemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( currentItem->GetName() );
		HelperUnmountBoundItems( currentItemDef );

		UnregisterDependency( currentItemid, *currentItemDef );
	}

	// HelperUnmountBoundItems may reallocate items array
	item = GetItem( itemId );

	if ( item != nullptr )
	{
		if ( !done && item->HasEntityDefined() )
		{
			if ( info.m_proxy != nullptr )
			{
				item->SetProxy( info.m_proxy, false );
				item->RevalidateProxy();
			}

			// proxy can be null after revalidation
			if ( !item->GetItemEntityProxy() )
			{
				// Have to create entity proxy
				item->CreateProxy( *itemDef, GetEntity()->GetLayer()->GetWorld()->GetDynamicLayer(), GetTemplate( item->GetName() ), ShouldCollapse( item->GetName() ) );
			}

			if( item->GetItemEntityProxy() )
			{
				SItemEntityManager::GetInstance().SetProxyAttachment( item->GetItemEntityProxy(), GetEntity(), attachSlot, false );
			}
		}

		if ( info.m_toHand )
		{
			item->SetIsHeld( true );
		}
		else
		{
			item->SetIsMounted( true );
		}
	}

	if ( gameplayEntity )
	{
		// Let entity perform additional setup
		info.m_toHand ? gameplayEntity->OnGrabItem( itemId, attachSlot ) : gameplayEntity->OnMountItem( itemId, wasHeld );
	}

	HelperMountBoundItems( itemDef );
	RegisterDependency( itemId, *itemDef );

	// HelperMountBoundItems and OnXXX events may reallocate items array
	item = GetItem( itemId );

	if ( item != nullptr )
	{
		OnAffectingItemChanged( item->GetName(), itemDef->GetCategory() );
	}

	return true;
}

Bool CInventoryComponent::UnMountItem( SItemUniqueId itemId, Bool destroyEntity /*=true*/, Bool emptyHand /* = true */ )
{	
	if ( ! IsIdValid( itemId ) )
	{
		ITEM_ERR( TXT("CInventoryComponent::UnMountItem - Invalid item index, actor %s"), GetEntity()->GetFriendlyName().AsChar() );
		return false;
	}

	SInventoryItem* item = GetItem( itemId );
	if ( item == nullptr || !( item->IsMounted() || item->IsHeld() ) )
	{
		return false;
	}

	CName defaultItemName = GetCategoryDefaultItem( item->GetCategory() );
	if ( defaultItemName == item->GetName() )
	{
		// trying to unmount default item, not gonna happen
		return false;
	}

	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
	if ( gameplayEntity )
	{
		// Let entity perform additional setup
		if ( item->IsHeld() )
		{ 
			gameplayEntity->OnPutItem( itemId, emptyHand );
		}
		else
		{
			gameplayEntity->OnUnmountItem( itemId );
		}
	}

	// OnXXX events may reallocate items array
	item = GetItem( itemId );
	if ( item != nullptr )
	{
		item->SetIsHeld( false );
		item->SetIsMounted( false );
	}

	// Remove bound items
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );
	RED_ASSERT( itemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), item->GetName().AsChar(), GetEntity()->GetName().AsChar() );
	HelperUnmountBoundItems( itemDef );

	UnregisterDependency( itemId, *itemDef );

	// HelperUnmountBoundItems may reallocate items array
	item = GetItem( itemId );

	SInventoryItem* defaultItem = nullptr;
	SItemUniqueId defaultItemId;
	if ( defaultItemName )
	{
		defaultItemId = GetItemId( defaultItemName );
		RED_ASSERT( defaultItemId != SItemUniqueId::INVALID, TXT( "Default item named %s for category %s is not in inventory, have to debug" ), defaultItemName.AsChar(), item->GetCategory().AsChar() );
		defaultItem = GetItem( defaultItemId );
	}

	if ( item != nullptr && item->HasEntityDefined() && item->GetItemEntityProxy() )
	{
		if ( defaultItem )
		{
			defaultItem->MoveProxy( item );
			TDynArray< CName > slotItems;
			item->GetSlotItemsNames( slotItems );
			SItemEntityManager::GetInstance().ChangeProxyItem( defaultItem->GetItemEntityProxy(), defaultItemName, CName::NONE, slotItems, GetTemplate( defaultItemName), ShouldCollapse( defaultItemName ), defaultItem->GetUniqueId() );
			ITEM_LOG( TXT("UnmountItem: changing item %s to %s"), item->GetName().AsString().AsChar(), defaultItemName.AsString().AsChar() );

			if ( gameplayEntity )
			{
				// Let entity perform additional setup
				gameplayEntity->OnMountItem( defaultItemId, false );
			}

			// Apply bound items of the default item
			{
				const SItemDefinition* defItemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( defaultItemName );
				RED_ASSERT( defItemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), defaultItemName.AsChar(), GetEntity()->GetName().AsChar() );
				HelperMountBoundItems( defItemDef );

				RegisterDependency( defaultItemId, *defItemDef );
			}
		}
		else
		{
			SItemEntityManager::GetInstance().SetProxyAttachment( item->GetItemEntityProxy(), NULL, CName::NONE, true );
			if ( destroyEntity && !item->IsCloned() )
			{
				item->DestroyProxy();
			}
		}
	}

	if ( defaultItem )
	{
		defaultItem->SetIsMounted( true );
	}

	// HelperMountBoundItems may reallocate items array
	item = GetItem( itemId );
	if ( item != nullptr )
	{
		OnAffectingItemChanged( item->GetName(), itemDef->GetCategory() );
	}
	
	return true;
}

Bool CInventoryComponent::DoesBehaviorResponse( const CName& eventName ) const
{
	CAnimatedComponent* animatedComponent = GetEntity()->GetRootAnimatedComponent();
	if ( animatedComponent )
	{
		CBehaviorGraphStack* stack = animatedComponent->GetBehaviorStack();
		if ( stack )
		{
			return stack->GenerateBehaviorStackEvent( eventName ) || stack->GenerateBehaviorStackForceEvent( eventName );
		}
	}

	return false;
}

Bool CInventoryComponent::DrawItem( SItemUniqueId itemId, Bool instant, Bool sendEvents )
{
	CActor* actor = Cast< CActor >( GetEntity() );
	if ( !actor )
	{
		return false;
	}

	Int32 itemIndex = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( itemIndex ) )
	{
		return false;
	}

	// Get item definition
	const SInventoryItem& newItem = m_items[ itemIndex ];
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( newItem.GetName() );

	// Can register latent action
	if ( !instant && SItemEntityManager::GetInstance().HasActorCollidableLatentActionsInProgress( actor, itemDef ) )
	{
		// Queue latent draw action
		CLatentItemQueuedActionDraw* action = new CLatentItemQueuedActionDraw( actor, itemId, itemDef );

		SItemEntityManager::GetInstance().QueueItemLatentAction( action );

		return true;
	}

	// Check is draw item is valid operation
	SInventoryItem* currItem = GetItemHeldInSlot( itemDef->GetHoldSlot( IsPlayerOwner() ) );
	if ( currItem && currItem->GetUniqueId() == itemId )
	{
		// Do nothing because item is already in hold slot
		ITEM_LOG( TXT("Draw item is nothing to do - item is already in hold slot. Actor '%ls', item '%ls'"),
			GetEntity()->GetName().AsChar(), currItem->GetName().AsString().AsChar() );
		return true;
	}

	// Ok, draw action can be processed now

	if ( !instant )
	{
		// Has item in new item hold slot?
		if ( currItem )
		{
			const SItemDefinition* currItemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( currItem->GetName() );
			RED_ASSERT( currItemDef != nullptr, TXT( "Cannot find item definition for item name: %s in entity %s" ), currItem->GetName().AsChar(), GetEntity()->GetName().AsChar() );

			// Two options:
			// 1. Switch from current to new
			// 2. Holster current nicely and draw new
			// Fallback: Holster current instantly and draw new

			// Check 1.
			{
				const TDynArray< SItemDefinition::SItemAnimSwitch >& switches = itemDef->GetAnimSwitches( IsPlayerOwner() );
				Uint32 size = switches.Size();
				
				CName currItemCategory = currItem->GetCategory();

				for ( Uint32 i=0; i<size; ++i )
				{
					const SItemDefinition::SItemAnimSwitch& animSwitch = switches[ i ];

					if ( animSwitch.m_category && animSwitch.m_category == currItemCategory && animSwitch.m_equipSlot == currItemDef->GetEquipSlot( IsPlayerOwner() ) )
					{
						if ( animSwitch.m_eventName && DoesBehaviorResponse( animSwitch.m_eventName ) )
						{
							// Register switch latent action
							CLatentItemActionSmoothSwitch* action = new CLatentItemActionSmoothSwitch(	
								actor,
								currItem->GetUniqueId(),
								itemId,
								animSwitch.m_actName,
								animSwitch.m_deactName,
								itemDef->GetEquipSlot(  IsPlayerOwner() ),
								itemDef->GetHoldSlot( IsPlayerOwner() ) );

							if ( SItemEntityManager::GetInstance().RegisterItemLatentAction( action ) )
							{
								return true;
							}
						}

						break;
					}
				}
			}

			// Check 2.
			{
				const SItemDefinition::SItemAnimAction* animAction = currItemDef->FindAnimAction( CNAME( holster ), IsPlayerOwner() );

				if ( animAction && DoesBehaviorResponse( animAction->m_event ) )
				{
					// Register holster & draw latent action
					CLatentItemActionSequentialSwitch* action = new CLatentItemActionSequentialSwitch(	
						actor,
						currItem->GetUniqueId(),
						itemId,
						animAction->m_act,
						animAction->m_deact,
						currItemDef->GetEquipSlot(  IsPlayerOwner() ),
						currItemDef->GetHoldSlot( IsPlayerOwner() ) );

					if ( SItemEntityManager::GetInstance().RegisterItemLatentAction( action ) )
					{
						return true;
					}
				}
				else if ( animAction )
				{
					ITEM_WARN( TXT("Behavior event '%ls' for holster item '%ls' is invalid."), animAction->m_event.AsString().AsChar(), currItem->GetName().AsString().AsChar() );
				}
				else
				{ 
					ITEM_WARN( TXT("Couldn't find item 'holster' anim action for item '%ls'."), currItem->GetName().AsString().AsChar() );
				}
			}

			// Fallback
			{
				ITEM_LOG( TXT("Holstering item %s instantly"), currItem->GetName().AsString().AsChar() );
				HolsterItem( currItem->GetUniqueId(), true, false );
			}
		}

		// Hold slot is empty so actor can draw item

		// Latent draw action
		const SItemDefinition::SItemAnimAction* animAction = itemDef->FindAnimAction( CNAME( draw ), IsPlayerOwner() );

		if ( animAction && DoesBehaviorResponse( animAction->m_event ) )
		{
			// Register latent draw action
			CLatentItemActionDraw* action = new CLatentItemActionDraw(	actor,
																		itemId,
																		animAction->m_act,
																		animAction->m_deact,
																		itemDef->GetEquipSlot(  IsPlayerOwner() ),
																		itemDef->GetHoldSlot( IsPlayerOwner() ) );

			if ( SItemEntityManager::GetInstance().RegisterItemLatentAction( action ) )
			{
				return true;
			}
		}
		else if ( animAction )
		{
			ITEM_WARN( TXT("Behavior event '%ls' for draw item '%ls' is invalid."), animAction->m_event.AsString().AsChar(), newItem.GetName().AsString().AsChar() );
		}
		else
		{ 
			ITEM_WARN( TXT("Couldn't find item 'draw' anim action for item '%ls'."), newItem.GetName().AsString().AsChar() );
		}
	}

	// Just mount the item to hand

	SMountItemInfo mountInfo;
	mountInfo.m_toHand = true;
	if ( MountItem( itemId, mountInfo ) )
	{
		if ( sendEvents )
		{
			if ( const SItemDefinition::SItemAnimAction* animAction = itemDef->FindAnimAction( CNAME( draw ), IsPlayerOwner() ) )
			{
				const CName eventInstantName( animAction->m_event.AsString() + TXT("Instant") );
				DoesBehaviorResponse( eventInstantName );
			}
		}
		return true;
	}

	return false;
}

Bool CInventoryComponent::HolsterItem( SItemUniqueId itemId, Bool instant, Bool sendEvents )
{
	CActor* actor = Cast< CActor >( GetEntity() );
	if ( !actor )
	{
		return false;
	}

	Int32 itemIndex = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( itemIndex ) )
	{
		return false;
	}

	// Get item
	SInventoryItem& item = m_items[ itemIndex ];
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item.GetName() );

	// Can register latent action
	if ( !instant && SItemEntityManager::GetInstance().HasActorCollidableLatentActionsInProgress( actor, itemDef ) )
	{
		// Queue latent draw action
		CLatentItemQueuedActionHolster* action = new CLatentItemQueuedActionHolster( actor, itemId, itemDef );

		SItemEntityManager::GetInstance().QueueItemLatentAction( action );

		return true;
	}

	if ( !item.IsHeld() )
	{
		// Do nothing because item is not in hold slot
		ITEM_LOG( TXT("Holster item is nothing to do - item is not in hold slot. Actor '%ls', item '%ls'"),
			GetEntity()->GetName().AsChar(), item.GetName().AsString().AsChar() );
		return true;
	}

	if ( !instant )
	{
		const SItemDefinition::SItemAnimAction* animAction = itemDef->FindAnimAction( CNAME( holster ), IsPlayerOwner() );

		if ( animAction && DoesBehaviorResponse( animAction->m_event ) )
		{
			// Register latent holster action
			CLatentItemActionHolster* action = new CLatentItemActionHolster(	actor,
																				itemId,
																				animAction->m_act,
																				animAction->m_deact,
																				itemDef->GetEquipSlot(  IsPlayerOwner() ),
																				itemDef->GetHoldSlot( IsPlayerOwner() ) );
		
			if ( SItemEntityManager::GetInstance().RegisterItemLatentAction( action ) )
			{
				return true;
			}
		}
		else if ( animAction )
		{
			ITEM_WARN( TXT("Behavior event '%ls' for holster item '%ls' is invalid."), animAction->m_event.AsString().AsChar(), item.GetName().AsString().AsChar() );
		}
		else
		{ 
			ITEM_WARN( TXT("Couldn't find item 'holster' anim action for item '%ls'."), item.GetName().AsString().AsChar() );
		}
	}

	// Holster through animation failed, just unmount the item from hand
	if ( GetItemEquipSlotName( itemId ) )
	{
		// Equip slot defined, mount to it
		if ( MountItem( itemId, SMountItemInfo() ) )
		{
			if ( sendEvents )
			{
				if ( const SItemDefinition::SItemAnimAction* animAction = itemDef->FindAnimAction( CNAME( holster ), IsPlayerOwner() ) )
				{
					const CName eventInstantName( animAction->m_event.AsString() + TXT("Instant") );
					DoesBehaviorResponse( eventInstantName );
				}
			}
			return true;
		}
		return false;
	}
	else 
	{
		// Equip slot not defined, destroy item
		return UnMountItem( itemId, true );
	}
}

Bool CInventoryComponent::DrawWeaponAndAttack( SItemUniqueId itemId )
{
	CActor* actor = Cast< CActor >( GetEntity() );
	if ( !actor )
	{
		return false;
	}

	Int32 itemIndex = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( itemIndex ) )
	{
		return false;
	}

	// Get item definition
	SInventoryItem& newItem = m_items[ itemIndex ];
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( newItem.GetName() );

	// Can register latent action
	if ( SItemEntityManager::GetInstance().HasActorCollidableLatentActionsInProgress( actor, itemDef ) )
	{
		// Queue latent draw action
		CLatentItemQueuedActionDraw* action = new CLatentItemQueuedActionDraw( actor, itemId, itemDef );

		SItemEntityManager::GetInstance().QueueItemLatentAction( action );

		return true;
	}

	// Check is draw and attack item is valid operation
	SInventoryItem* currItem = GetItemHeldInSlot( itemDef->GetHoldSlot( IsPlayerOwner() ) );
	if ( currItem )
	{
		ITEM_LOG( TXT("Couldn't process 'Draw and attack' action for actor '%ls'. Actor has got item '%ls' in hold slot. Actor will holster this item and draw new without attack action."),
			GetEntity()->GetName().AsChar(), currItem->GetName().AsString().AsChar() );

		return DrawItem( itemId, false );
	}

	// Latent draw and attack action
	const SItemDefinition::SItemAnimAction* animAction = itemDef->FindAnimAction( CNAME( attack ), IsPlayerOwner() );

	if ( animAction && DoesBehaviorResponse( animAction->m_event ) )
	{
		// Register latent draw action
		CLatentItemActionDraw* action = new CLatentItemActionDraw(	actor,
			itemId,
			animAction->m_act,
			animAction->m_deact,
			itemDef->GetEquipSlot( IsPlayerOwner() ),
			itemDef->GetHoldSlot( IsPlayerOwner() ) );

		if ( SItemEntityManager::GetInstance().RegisterItemLatentAction( action ) )
		{
			return true;
		}
	}
	else if ( animAction )
	{
		ITEM_WARN( TXT("Behavior event '%ls' for attack and attack item '%ls' is invalid."), animAction->m_event.AsString().AsChar(), newItem.GetName().AsString().AsChar() );
	}
	else
	{ 
		ITEM_WARN( TXT("Couldn't find item 'attack' anim action for item '%ls'."), newItem.GetName().AsString().AsChar() );
	}

	return false;
}

void CInventoryComponent::DropItem( SItemUniqueId itemId, Bool removeFromInv /*= false*/, Float duration /* = -1.0f */ )
{
	// Unmount item but don't destroy entity
	UnMountItem( itemId, false );
	
	SInventoryItem* item = GetItem( itemId );
	if ( !item )
	{
		return;
	}

	// Drop it
	if ( item->GetItemEntityProxy() )
	{
		if ( m_droppedProxies.Size() > MAX_DROPPED_PROXIES )
		{
			m_droppedProxies.ClearFast();
		}
		m_droppedProxies.Insert( itemId, item->GetItemEntityProxy() );
		SItemEntityManager::GetInstance().DropItemByProxy( item->GetItemEntityProxy(), duration );
		item->ReleaseProxy();
	}

	if ( removeFromInv )
	{
		if ( item->IsCloned() )
		{
			RemoveFakeItem( itemId );
		}
		else
		{
			RemoveItem( itemId );
		}
	}
}

CItemEntityProxy* CInventoryComponent::GetDroppedProxy( SItemUniqueId itemId ) const
{
	CItemEntityProxy* proxy = nullptr;
	m_droppedProxies.Find( itemId, proxy );
	return proxy;
}

void CInventoryComponent::PlayItemAnimation( SItemUniqueId itemId, CName animationName )
{
	SInventoryItem* item = GetItem( itemId );
	if ( item && item->GetItemEntityProxy() )
	{
		SItemEntityManager::GetInstance().PlayAnimationOnEntity( item->GetItemEntityProxy(), animationName );
	}
}

void CInventoryComponent::StopItemAnimation( SItemUniqueId itemId )
{
	SInventoryItem* item = GetItem( itemId );
	if ( item && item->GetItemEntityProxy() )
	{
		SItemEntityManager::GetInstance().StopAnimationOnEntity( item->GetItemEntityProxy() );
	}
}

void CInventoryComponent::RaiseItemBehaviorEvent( SItemUniqueId itemId, CName eventName )
{
	SInventoryItem* item = GetItem( itemId );
	if ( item && item->GetItemEntityProxy() )
	{
		SItemEntityManager::GetInstance().RaiseBehaviorEventOnEntity( item->GetItemEntityProxy(), eventName );
	}
}

void CInventoryComponent::PlayItemEffect( SItemUniqueId itemId, CName effectName )
{
	SInventoryItem* item = GetItem( itemId );
	if ( item && item->GetItemEntityProxy() )
	{
		SItemEntityManager::GetInstance().PlayEffectOnEntity( item->GetItemEntityProxy(), effectName );
	}
}

void CInventoryComponent::StopItemEffect( SItemUniqueId itemId, CName effectName )
{
	SInventoryItem* item = GetItem( itemId );
	if ( item && item->GetItemEntityProxy() )
	{
		SItemEntityManager::GetInstance().StopEffectOnEntity( item->GetItemEntityProxy(), effectName );
	}
}

void CInventoryComponent::GetItemAbilities( SItemUniqueId itemId, TDynArray< CName >& abilities ) const
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( index ) )
	{
		ITEM_WARN( TXT("Using GetItemAbilities with invalid id") );
		return;
	}

	m_items[index].GetAllAbilities( abilities );
}

CEntity* CInventoryComponent::ThrowAwayItem( SItemUniqueId itemId, Uint32 quantity /*= 1 */ )
{
	// Are we throwing away quest item?
	Bool containsQuestItems = false;
	TDynArray< CName > itemTags;
	itemTags.ClearFast();
	if ( GetItemTags( itemId, itemTags ) )
	{
		if ( itemTags.Exist( CNAME( NoDrop ) ) == true )
		{
			return NULL;
		}

		containsQuestItems = itemTags.Exist( CNAME( Quest ) );
	}


	CEntity* containerEntity = NULL;
	Vector thisPosition = GetEntity()->GetWorldPosition();

	// Look for nearby sack
	TDynArray< CEntity* > nearbyContainers;
	GetLayer()->GetWorld()->GetTagManager()->CollectTaggedEntities( CNAME( ItemContainer ), nearbyContainers );
	const auto nearbyContainersEndIt = nearbyContainers.End();
	for ( TDynArray< CEntity* >::iterator containerIter = nearbyContainers.Begin();	containerIter != nearbyContainersEndIt; ++containerIter )
	{
		if ( containsQuestItems == true && (*containerIter)->HasFlag( EF_ManagedEntity ) == false )
		{
			continue;
		}

		if ( thisPosition.DistanceSquaredTo( (*containerIter)->GetTransform().GetPosition() ) < 1.0f )
		{
			containerEntity = *containerIter;
			break;
		}
	}


	if( !containerEntity && m_containerTemplate )
	{
		if ( m_containerTemplate )
		{
			// Creating a sack
			CEntity * thisEntity = GetEntity();
			RED_ASSERT( thisEntity != nullptr, TXT( "CInventoryComponent parent entity is null." ) );

			EntitySpawnInfo spawnInfo;		
			spawnInfo.m_template = m_containerTemplate;
			spawnInfo.m_spawnPosition = thisPosition;
			spawnInfo.m_spawnRotation = thisEntity->GetWorldRotation();
			spawnInfo.m_entityFlags = EF_DestroyableFromScript;
			spawnInfo.m_tags.AddTag( CNAME( ItemContainer ) );
			if ( containsQuestItems )
			{
				spawnInfo.m_idTag = GGame->GetIdTagManager()->Allocate();
				spawnInfo.m_entityFlags |= EF_ManagedEntity;
			}

			CDynamicLayer* dynamicLayer = GetLayer()->GetWorld()->GetDynamicLayer();
			containerEntity = dynamicLayer->CreateEntitySync( spawnInfo );
		}
		else
		{
			ITEM_ERR( TXT("CInventoryComponent::ThrowAwayItems NULL m_containerTemplate") );
		}
	}
		

	// Putting item to sack
	if ( containerEntity )
	{
		GiveItem( containerEntity->FindComponent< CInventoryComponent >(), itemId, quantity );
	}
	else
	{
		ITEM_ERR( TXT("CInventoryComponent::ThrowAwayItems cannot create entity from template '%ls'"), m_containerTemplate->GetFriendlyName().AsChar() );
	}

	return containerEntity;
}

CEntity* CInventoryComponent::ThrowAwayItems( const TDynArray< Int32 >& indices )
{
	if( indices.Size() == 0 )
	{
		return NULL;
	}

	// Do not create container if there aren't any valid indices
	Bool anyValid = false;
	Bool containsQuestItems = false;
	static TDynArray< CName > itemTags;
	const Uint32 indicesCount = indices.Size();
	for( Uint32 i = 0; i < indicesCount; ++i )
	{
		const Int32 indice = indices[ i ];
		if( IsIndexValid( indice ) )
		{
			anyValid = true;

			const SItemUniqueId itemId = m_items[ indice ].GetUniqueId();
			itemTags.ClearFast();
			if ( GetItemTags( itemId, itemTags ) )
			{
				containsQuestItems |= itemTags.Exist( CNAME( Quest ) );
			}
		}
	}

	if( !anyValid )
	{
		return NULL;
	}

	if ( m_containerTemplate )
	{
		CEntity* thisEntity = GetEntity();
		RED_ASSERT( thisEntity != nullptr, TXT( "CInventoryComponent parent entity is null." ) );

		const Vector& entityPos = thisEntity->GetWorldPosition();
		Vector finalPos = entityPos;
		if ( containsQuestItems )
		{
			const CWorld* world = GGame->GetActiveWorld();
			if ( world )
			{
				const CPathLibWorld* pathlibWorld = world->GetPathLibWorld();
				if ( pathlibWorld )
				{
					pathlibWorld->FindSafeSpot( PathLib::INVALID_AREA_ID, entityPos.AsVector3(), 10, 0.1f, finalPos.AsVector3() );
				}
			}
		}
		EntitySpawnInfo spawnInfo;		
		spawnInfo.m_template = m_containerTemplate;
		spawnInfo.m_spawnPosition = finalPos;
		spawnInfo.m_spawnRotation = thisEntity->GetWorldRotation();
		spawnInfo.m_entityFlags = EF_DestroyableFromScript;
		if ( containsQuestItems )
		{
			spawnInfo.m_idTag = GGame->GetIdTagManager()->Allocate();
			spawnInfo.m_entityFlags |= EF_ManagedEntity;
		}

		CDynamicLayer* dynamicLayer = GetLayer()->GetWorld()->GetDynamicLayer();
		CEntity* entity = dynamicLayer->CreateEntitySync( spawnInfo );
		if ( entity )
		{
			GiveItems( entity->FindComponent< CInventoryComponent >(), indices );
		}
		else
		{
			ITEM_ERR( TXT("CInventoryComponent::ThrowAwayItems cannot create entity from template '%ls'"), m_containerTemplate->GetFriendlyName().AsChar() );
		}

		return entity;
	}
	else
	{
		ITEM_ERR( TXT("CInventoryComponent::ThrowAwayItems NULL m_containerTemplate") );
	}

	return NULL;

}

CEntity* CInventoryComponent::ThrowAwayAllItems()
{
	TDynArray< Int32 > indices;
	if ( FillIndices( indices ) )
	{
		return ThrowAwayItems( indices );
	}
	return NULL;
}

CEntity* CInventoryComponent::ThrowAwayItemsFiltered( const TDynArray< CName >& excludedTags )
{
	TDynArray< Int32 > indices;

	const Uint32 itemsCount = m_items.Size();
	if( itemsCount > 0 )
	{		
		indices.Reserve( itemsCount );
		const Uint32 excludedTagsCount = excludedTags.Size();

		for ( Uint32 i = 0; i < itemsCount; i++ )
		{
			Bool ok = true;
			for ( Uint32 t = 0; t < excludedTagsCount; t++ )
			{
				if( m_items[i].GetTags().Exist( excludedTags[t] ) )
				{
					ok = false;
					break;
				}
			}

			if( ok )
			{
				indices.PushBack( i );
			}
		}
	}

	if ( indices.Size() > 0 )
	{
		return ThrowAwayItems( indices );
	}

	return NULL;
}

CEntity* CInventoryComponent::ThrowAwayLootableItems( Bool skipNoDropNoShow /* = false */ )
{
	SInventoryItem::SItemModifierVal* itemMod;
	TDynArray< Int32 > indices;
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; i++ )
	{
		SInventoryItem& item = m_items[ i ];
		if ( item.IsLootable() )
		{
			if ( skipNoDropNoShow && !item.IsQuestItem() && item.IsNoDropNoShow() )
			{
				continue;
			}
			if ( item.GetDurability() > 0 )
			{ 
				itemMod = item.GetItemMod( CNAME( ItemDurabilityModified ), true );
				if ( itemMod && itemMod->m_int < 1 )
				{
					ReduceLootableItemDurability( item );
				}
			}

			indices.PushBack( i );
		}
	}

	if ( indices.Size() > 0 )
	{
		return ThrowAwayItems( indices );
	}

	return nullptr;
}

void CInventoryComponent::DespawnAllItems()
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		SInventoryItem& item = m_items[ i ];
		if ( item.GetItemEntityProxy() && !item.IsCloned() )
		{
			item.DestroyProxy();
		}
	}
}

void CInventoryComponent::DespawnOwnedItems()
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		SInventoryItem& item = m_items[ i ];
		if ( item.GetItemEntityProxy() && !item.IsProxyTaken() )
		{
			item.DestroyProxy();
		}
	}
}

void CInventoryComponent::DespawnItem( SItemUniqueId itemId )
{
	SInventoryItem* item = GetItem( itemId );
	if ( item && item->GetItemEntityProxy() && !item->IsCloned() )
	{
		item->DestroyProxy();
	}
}

void CInventoryComponent::PrintInfo()
{
	if( GetItemCount() == 0 )
	{
		ITEM_LOG( TXT("Empty") );
	}
	else
	{
		const Uint32 itemsCount = m_items.Size();
		for ( Uint32 i = 0; i < itemsCount; i++ )
		{
			const SInventoryItem& item = m_items[i];
			ITEM_LOG( TXT("%u. %s"), i, item.GetInfo().AsChar() );
		};
	}
};

Bool CInventoryComponent::IsWeapon( SItemUniqueId itemId ) const
{
	Int32 index = UniqueIdToIndex( itemId );
	if ( IsIndexValid( index ) )
	{
		return m_items[ index ].IsWeapon();
	}

	// Not found
	return false;
}

Bool CInventoryComponent::IsWeaponHeld() const
{
	auto itemIter = m_items.Begin();
	while( itemIter != m_items.End() )
	{
		if( itemIter->IsHeld() && itemIter->IsWeapon() )
		{
			return true;
		}
		++itemIter;
	}
	return false;
}

Int32 CInventoryComponent::UniqueIdToIndex( SItemUniqueId itemId ) const
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		if ( m_items[ i ].GetUniqueId() == itemId )
		{
			return i;
		}
	}

	return INVALID_INDEX;
}

Int32 CInventoryComponent::NameToIndex( CName itemName ) const
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		if ( m_items[ i ].GetName() == itemName )
		{
			return i;
		}
	}

	return INVALID_INDEX;
}

CName CInventoryComponent::GetCategoryDefaultItem( CName category ) const
{
	CEntityTemplate* entityTemplate =  GetEntity()->GetEntityTemplate();
	if ( entityTemplate )
	{
		CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( GetEntity() );
		const CEntityAppearance* appearance = 
			entityTemplate->GetAppearance( appearanceComponent ? appearanceComponent->GetAppearance() : CName::NONE, true );
		if ( appearance )
		{
			const CEquipmentDefinition* equipmentDef = appearance->FindParameter< CEquipmentDefinition >();
			if ( equipmentDef )
			{
				// return default item if specified
				return equipmentDef->GetDefaultItemForCategory( category );
			}
		}
	}

	// No equipment def
	return CName::NONE;
}

void CInventoryComponent::OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens ) const
{
	if ( animationName )
	{
		const Uint32 itemsCount = m_items.Size();
		for ( Uint32 i = 0; i < itemsCount; ++i )
		{
			const SInventoryItem& item = m_items[ i ];
			if ( ( item.IsMounted() || item.IsHeld() ) && item.GetItemEntityProxy() )
			{
				CAnimatedComponent* outItemAnimatedComponent;
				if ( TryToLoadSyncToken( &item, animationName, outItemAnimatedComponent ) )
				{
					CItemAnimationSyncToken* syncToken = new CItemAnimationSyncToken;
					syncToken->m_itemUniqueId = item.GetUniqueId();
					syncToken->m_inventoryComponent = this;
					syncToken->m_syncedAnimated = outItemAnimatedComponent;
					tokens.PushBack( syncToken );
				}
			}
		}
	}
}

Bool CInventoryComponent::TryToLoadSyncToken( const SInventoryItem* item, CName animationName, CAnimatedComponent*& outItemAnimatedComponent ) const
{
	Bool tokenShouldBeCreated = false;
	outItemAnimatedComponent = nullptr;

	if ( item )
	{
		if ( CItemEntity* itemEntity = item->GetItemEntity() )
		{
			if ( CAnimatedComponent* animated = itemEntity->GetRootAnimatedComponent() )
			{
				if ( const CSkeletalAnimationContainer* animContainer =	animated->GetAnimationContainer() )
				{
					if ( animContainer->FindAnimation( animationName ) )
					{
						outItemAnimatedComponent = animated;
						return true;
					}
				}
			}
		}
		else
		{
			// Check if item is being spawned at the moment
			if ( const CItemEntityProxy* proxy = item->GetItemEntityProxy() )
			{
				return proxy->m_dirty;
			}
		}
	}

	return false;
}

Bool CInventoryComponent::CheckShouldSave() const
{
	return true;
}

void CInventoryComponent::Export( CXMLWriter& writer ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();

	writer.BeginNode( TXT("Inventory" ) );

	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; i++ )
	{
		const SInventoryItem& item = m_items[ i ];
		if( item.GetTags().Exist( CNAME( NoShow ) ) || item.GetTags().Exist( CNAME( NoDrop ) ) )
		{
			continue;
		}		

		writer.BeginNode( TXT("Item") );
		writer.SetAttribute( TXT("name"), item.GetName().AsString() );
		writer.AttributeT( TXT("quantity"), item.GetQuantity() );
		
		TDynArray< CName > slotItems;
		item.GetSlotItemsNames( slotItems );
		writer.BeginNode( TXT("SlotItems") );
		for( Uint32 j = 0; j < slotItems.Size(); j++ )
		{
			writer.BeginNode( TXT("Item" ) );
			writer.SetAttribute( TXT("name"), slotItems[j].AsString() );
			writer.EndNode(); // Item
		}			
		writer.EndNode(); // SlotItems

		writer.BeginNode( TXT( "CraftedAbilities" ) );
		const TDynArray< CName >& craftedAbilities = item.GetCraftedAbilities();
		const Uint32 craftedAbilitiesCount = craftedAbilities.Size();
		for ( Uint32 j = 0; j < craftedAbilitiesCount; ++j )
		{
			writer.BeginNode( TXT( "Ability" ) );
			writer.SetAttribute( TXT( "name" ), craftedAbilities[ j ].AsString() );
			writer.EndNode(); // Ability
		}
		writer.EndNode(); // CraftedAbilities
		
		TDynArray< CName > baseAbilities;
		item.GetBaseAbilities( baseAbilities );
		writer.BeginNode( TXT("BaseAbilities") );
		const Uint32 baseAbilitiesCount = baseAbilities.Size();
		for ( Uint32 j = 0; j< baseAbilitiesCount; j++ )
		{
			writer.BeginNode( TXT("Ability") );
			CName abilityName = baseAbilities[ j ];
			writer.SetAttribute( TXT("name"), abilityName.AsString() );

			const SAbility* ab = defMgr->GetAbilityDefinition( abilityName );
			if( ab )
			{
				writer.BeginNode( TXT("Attributes") );
				const Uint32 attributesCount = ab->m_attributes.Size();
				for ( Uint32 a = 0; a < attributesCount; ++a )
				{
					const SAbilityAttribute& attr = ab->m_attributes[a];
					writer.BeginNode( TXT("Attrib" ) );
					
					writer.SetAttribute( TXT("name"), attr.m_name.AsString() );
					writer.AttributeT( TXT("type"), attr.m_type );
					writer.AttributeT( TXT("min"), attr.m_min );
					writer.AttributeT( TXT("max"), attr.m_max );						

					writer.EndNode(); // Attrib
				}
				writer.EndNode(); // Attributes
			}
				
			writer.EndNode(); // Ability
		}

		writer.EndNode(); // BaseAbilities
		writer.EndNode(); // Item		
	}

	writer.EndNode(); // Inventory
}

void CInventoryComponent::HelperMountBoundItems( const SItemDefinition* itemDef )
{
	if ( itemDef )
	{
		// Mount any bound items
		const TDynArray< CName >& boundItems = itemDef->GetBoundItems( IsPlayerOwner() );
		const Uint32 boundItemsCount = boundItems.Size();
		for ( Uint32 i = 0; i < boundItemsCount; ++i )
		{
			const CName boundItemName = boundItems[i];
			TDynArray< SItemUniqueId > boundItemId;
            boundItemId.PushBack( GetItemId( boundItemName ) );

			if ( !boundItemId[0] )
			{
				// not possessing bound item, add it		
				SAddItemInfo addItemInfo;
				addItemInfo.m_informGui = false;
				boundItemId = AddItem( boundItemName, addItemInfo );
				
				if ( boundItemId[0] == SItemUniqueId::INVALID )
				{
					DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT( "Inventory Component" ), TXT( "Failed to add bound Item %s to inventory. Item definition: %s" ), boundItemName.AsString().AsChar(), itemDef->m_debugXMLFile.AsChar() );
				}
			}

			ITEM_LOG( TXT("Mounting bound item %s"), boundItemName.AsString().AsChar() );
			SMountItemInfo info;
			MountItem( boundItemId[0], info );
		}
	}
}

Bool CInventoryComponent::IsItemMounted( CName itemName ) const
{
	SItemUniqueId id = GetItemId( itemName );
	if( id == SItemUniqueId::INVALID )
	{
		id = GetItemByCategory( itemName, true );
	}

	return IsItemMounted( id );
}

Bool CInventoryComponent::IsAnyItemMounted( CName itemName ) const
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];
		if( item.GetName() == itemName )
		{
			if( item.IsMounted() )
			{
				return true;
			}
		}
	}

	return false;
}


Bool CInventoryComponent::IsItemMounted( SItemUniqueId id ) const
{
	if ( id == SItemUniqueId::INVALID )
	{
		return false;
	}
	const SInventoryItem* inventoryItem = GetItem( id );
	return ( inventoryItem && inventoryItem->IsMounted() );
}

Bool CInventoryComponent::IsItemHeld( CName itemName ) const
{
	SItemUniqueId id = GetItemId( itemName );
	if( id == SItemUniqueId::INVALID )
	{
		id = GetItemByCategory( itemName, true );
	}

	return IsItemHeld( id );
}

Bool CInventoryComponent::IsItemHeld( SItemUniqueId id ) const
{
	if ( id == SItemUniqueId::INVALID )
	{
		return false;
	}

	const SInventoryItem* inventoryItem = GetItem( id );	
	return ( inventoryItem && inventoryItem->IsHeld() );
}

Bool CInventoryComponent::ShouldCollapse( CName itemName ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	const SItemDefinition* itemDef = defMgr->GetItemDefinition( itemName );
	if ( itemDef )
	{
		const SItemDefinition::SCollapseCond& conds = itemDef->GetCollapseCond( IsPlayerOwner() );

		for ( auto collItem : conds.m_collapseItemCond )
		{
			if ( IsAnyItemMounted( collItem ) )
			{
				return true;
			}
		}

		for ( auto collItem : conds.m_uncollapseItemCond )
		{
			if ( IsAnyItemMounted( collItem ) )
			{
				return false;
			}
		}

		// check if there is any (but not default) item with affecting category (then use variant template)
		for ( auto category : conds.m_collapseCategoryCond )
		{
			SItemUniqueId currentItemid = GetItemByCategory( category, true, true );

			if ( currentItemid )
			{
				return true;
			}
		}
	}
	return false;
}

const String& CInventoryComponent::GetTemplate( CName itemName ) const
{
	const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	const Bool isPlayerOwner = IsPlayerOwner();
	// Lookup item definition for entity template path
	const SItemDefinition* itemDef = defMgr->GetItemDefinition( itemName );
	if ( !itemDef )
	{
		return String::EMPTY;
	}
	const TDynArray< SItemDefinition::SItemVariant >& variants = itemDef->GetVariants( isPlayerOwner );
	auto iter = variants.Begin();
	const auto variantsEndIt = variants.End();

	// if item has variants - check if we should use non default template
	for ( ; iter != variantsEndIt; ++iter )
	{
		// check if there is affecting item mounted (then use variant template)
		Uint32 found = 0;
		auto nameIter = iter->m_affectingItems.Begin();
		const auto affectingItemsEndIt = iter->m_affectingItems.End();
		for ( ; nameIter != affectingItemsEndIt; ++nameIter )
		{			  
			SItemUniqueId itemId = GetItemId( *nameIter );
			if ( itemId != SItemUniqueId::INVALID )
			{
				const SInventoryItem* inventoryItem = GetItem( itemId );
				if ( inventoryItem && inventoryItem->IsMounted() )
				{
					//! LOG FOR DEBUG
					//! RED_LOG( RED_LOG_CHANNEL( InventoryComponent ), TXT("Use variant '%ls' for item '%ls' because '%ls' item is equipped"), iter->m_template.AsChar(), itemName.AsChar(), nameIter->AsChar() ); 
					// if we expect only one affected item to be mounted, this is the variant we've been looking for
					if ( !iter->m_expectAll )
					{
						return iter->m_template;
					}
					// if we expect all affected items to be mounted just count the item as "found"
					else
					{
						found++;
					}
				}
			}
		}

		// if we expect all items to be mounted BUT not all of them are -> check next variant
		if ( iter->m_expectAll && found < iter->m_affectingItems.Size() )
		{
			continue;
		}

		// we didn't found any mounted (if expected any) item OR all expected items are mounted (if expected all)
		// check if there is any (but not default) item with affecting category (then use variant template)
		found = 0;
		for ( CName category : iter->m_categories )
		{
			SItemUniqueId currentItemid = GetItemByCategory( category, true, true );
			if ( currentItemid )
			{
				// if we expect only one category to be found, this is the variant we've been looking for
				if ( !iter->m_expectAll )
				{
					return iter->m_template;
				}
				// if we expect all affected items to be mounted just count the item as "found"
				else
				{
					found++;
				}
			}
		}

		// we're here so either we expected all and all was founded
		// OR we expected ANY and nothing was found
		if ( iter->m_expectAll && found == iter->m_categories.Size() )
		{
			return iter->m_template;
		}
	}

	const String& templateName = itemDef->GetEquipTemplateName( isPlayerOwner );
	if ( templateName.Empty() )
	{
		return itemDef->GetHoldTemplateName( isPlayerOwner );
	}
	return templateName;
}

void CInventoryComponent::UpdateDependingItems( const TDynArray< SItemUniqueId >& list )
{
	for ( SItemUniqueId itemId : list )
	{
		SInventoryItem* inventoryItem = GetItem( itemId );
		if ( inventoryItem && inventoryItem->GetItemEntityProxy() && Red::System::StringCompareNoCase( inventoryItem->GetItemEntityProxy()->m_template.AsChar(), GetTemplate( inventoryItem->GetName() ).AsChar() ) == 0 )
		{
			if ( inventoryItem->GetItemEntityProxy()->m_collapse != ShouldCollapse( inventoryItem->GetName() ) )
			{
				inventoryItem->Collapse( !inventoryItem->GetItemEntityProxy()->m_collapse );
			}
			continue;
		}

		SMountItemInfo info;
		info.m_forceOverwritingCurrentItem = true;
		MountItem( itemId, info );
	}
}

void CInventoryComponent::OnAffectingItemChanged( CName itemName, CName category )
{
	if ( const TDynArray< SItemUniqueId >* list = m_itemDependentItems.FindPtr( itemName ) )
	{
		TDynArray< SItemUniqueId > listCopy = *list; // Use a copy; list might change as part of (Un)RegisterDependency calls from inside of UpdateDependingItems
		UpdateDependingItems( listCopy );
	}

	if ( const TDynArray< SItemUniqueId >* list = m_categoryDependentItems.FindPtr( category ) )
	{
		TDynArray< SItemUniqueId > listCopy = *list; // Use a copy; list might change as part of (Un)RegisterDependency calls from inside of UpdateDependingItems
		UpdateDependingItems( listCopy );
	}
}

void CInventoryComponent::RegisterDependency( SItemUniqueId itemId, const SItemDefinition& itemDef )
{
	const TDynArray< SItemDefinition::SItemVariant >& variants = itemDef.GetVariants( IsPlayerOwner() );
	for ( const SItemDefinition::SItemVariant& variant : variants )
	{
		for ( CName name : variant.m_affectingItems )
		{
			TDynArray< SItemUniqueId >& list = m_itemDependentItems.GetRef( name, TDynArray< SItemUniqueId >() );
			list.PushBackUnique( itemId );
		}

		for ( CName category : variant.m_categories )
		{
			TDynArray< SItemUniqueId >& list = m_categoryDependentItems.GetRef( category, TDynArray< SItemUniqueId >() );
			list.PushBackUnique( itemId );
		}
	}
	const SItemDefinition::SCollapseCond& conds = itemDef.GetCollapseCond( IsPlayerOwner() );

	for ( auto collItem : conds.m_collapseItemCond )
	{
		TDynArray< SItemUniqueId >& list = m_itemDependentItems.GetRef( collItem, TDynArray< SItemUniqueId >() );
		list.PushBackUnique( itemId );
	}

	for ( auto collItem : conds.m_uncollapseItemCond )
	{
		TDynArray< SItemUniqueId >& list = m_itemDependentItems.GetRef( collItem, TDynArray< SItemUniqueId >() );
		list.PushBackUnique( itemId );
	}

	for ( auto category : conds.m_collapseCategoryCond )
	{
		TDynArray< SItemUniqueId >& list = m_categoryDependentItems.GetRef( category, TDynArray< SItemUniqueId >() );
		list.PushBackUnique( itemId );
	}
}

void CInventoryComponent::UnregisterDependency( SItemUniqueId itemId, const TDynArray< CName >& itemList, THashMap< CName, TDynArray< SItemUniqueId > >& dependencyList )
{
	for ( CName name : itemList )
	{
		auto listIter = dependencyList.Find( name );
		if ( listIter != dependencyList.End() )
		{
			listIter->m_second.Remove( itemId );
			if ( listIter->m_second.Empty() )
			{
				dependencyList.Erase( listIter );
			}
		}
	}
}
void CInventoryComponent::UnregisterDependency( SItemUniqueId itemId, const SItemDefinition& itemDef )
{
	const TDynArray< SItemDefinition::SItemVariant >& variants = itemDef.GetVariants( IsPlayerOwner() );
	for ( const SItemDefinition::SItemVariant& variant : variants )
	{
		UnregisterDependency( itemId, variant.m_affectingItems, m_itemDependentItems );

		for ( CName category : variant.m_categories )
		{
			auto listIter = m_categoryDependentItems.Find( category );
			if ( listIter != m_categoryDependentItems.End() )
			{
				listIter->m_second.Remove( itemId );
				if ( listIter->m_second.Empty() )
				{
					m_categoryDependentItems.Erase( listIter );
				}
			}
		}
	}

	const SItemDefinition::SCollapseCond& conds = itemDef.GetCollapseCond( IsPlayerOwner() );

	UnregisterDependency( itemId, conds.m_collapseItemCond, m_itemDependentItems );
	UnregisterDependency( itemId, conds.m_uncollapseItemCond, m_itemDependentItems );
	UnregisterDependency( itemId, conds.m_collapseCategoryCond, m_categoryDependentItems );
}

void CInventoryComponent::HelperUnmountBoundItems( const SItemDefinition* itemDef )
{
	if ( itemDef )
	{
		// Unmount any bound items
		const TDynArray< CName >& boundItems = itemDef->GetBoundItems( IsPlayerOwner() );
		const Uint32 boundItemsCount = boundItems.Size();
		for ( Uint32 i = 0; i < boundItemsCount; ++i )
		{
			const CName& boundItemName = boundItems[ i ];
			SItemUniqueId boundItemId = GetItemId( boundItemName );

			if ( boundItemId )
			{
				UnMountItem( boundItemId );	
			}
		}
	}
}

Bool CInventoryComponent::AreAllMountedItemsSpawned()
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];

		if ( item.IsMounted() && item.GetItemEntityProxy() )
		{
			const CItemEntityManager& manager = SItemEntityManager::GetInstance();

			const CItemEntityProxy* proxy = item.GetItemEntityProxy();
			const CItemEntity* itemEntity = manager.GetItemEntityIfSpawned( proxy );
			if ( itemEntity == nullptr && !manager.EntityItemFaliedToLoad( proxy ) )
			{
				return false;
			}

			if ( manager.IsProxyInSpawnQue( proxy ) || manager.IsProxyInAttachQue( proxy ) )
			{
				return false;
			}
		}
	}
	return true;
}

Bool CInventoryComponent::OnGrabItem( SItemUniqueId itemId )
{
	SInventoryItem* item = GetItem( itemId );

	if ( !item ) return false;
	if ( !item->GetItemEntityProxy() ) return false;

	CItemEntity* itemEntity = SItemEntityManager::GetInstance().GetItemEntity( item->GetItemEntityProxy() );
	if( !itemEntity ) return false;

	itemEntity->CallEvent( CNAME( OnGrab ) );
	return true;
}

Bool CInventoryComponent::OnPutItem( SItemUniqueId itemId )
{
	SInventoryItem* item = GetItem( itemId );

	if ( !item ) return false;
	if ( !item->GetItemEntityProxy() ) return false;

	CItemEntity* itemEntity = SItemEntityManager::GetInstance().GetItemEntity( item->GetItemEntityProxy() );
	if( !itemEntity ) return false;

	itemEntity->CallEvent( CNAME( OnPut ) );
	return true;
}

Bool CInventoryComponent::AddListener( IInventoryListener* listener )
{
	return m_listeners.PushBackUnique( listener );
}

Bool CInventoryComponent::RemoveListener( IInventoryListener* listener )
{
	return m_listeners.Remove( listener );
}

void CInventoryComponent::NotifyListeners( EInventoryEventType eventType, SItemUniqueId id, Int32 quantity, Bool fromAssociatedInventory )
{
	if ( m_suppressEvents )
	{
		return;
	}
	for ( IInventoryListener* it : m_listeners )
	{
		it->OnInventoryEvent( this, eventType, id, quantity, fromAssociatedInventory );
	}
	// always notify player's inventory
	if ( m_notifyScriptedListeners || ( GetParent() != nullptr && GetParent()->IsA< CPlayer >() ) )
	{
		CallEvent( CNAME( OnInventoryScriptedEvent ), eventType, id, quantity, fromAssociatedInventory );
	}
	if ( !fromAssociatedInventory )
	{
		// if event wasn't called from associated inventory, but there is one, let's inform it about changes
		CInventoryComponent* inv = GetAssociatedInventory();
		if ( inv != nullptr )
		{
			inv->NotifyListeners( eventType, SItemUniqueId::INVALID, quantity, true );
		}
	}
}

void CInventoryComponent::OnItemAdded( SItemChangedData& addItemInfo )
{
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );	
	if ( addItemInfo.m_ids.Size() > 0 )
	{
		//In some cases is not necessary to inform about every Id that changed
		SItemUniqueId firstID = addItemInfo.m_ids[0];
		if ( gameplayEntity != nullptr && gameplayEntity->IsAttached() )
		{
			gameplayEntity->OnAddedItem( firstID );
			CallFunction( this, CNAME( OnItemAdded ), addItemInfo );
		}
		NotifyListeners( IET_ItemAdded, firstID, addItemInfo.m_quantity, false );
		MarkItemsListChanged();
	}	
}

void CInventoryComponent::OnItemRemoved( SItemUniqueId itemId, Uint32 quantity )
{
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
	if ( gameplayEntity != nullptr && gameplayEntity->IsAttached() )
	{
		gameplayEntity->OnRemovedItem( itemId );
		CallFunction( this, CNAME( OnItemRemoved ), itemId, quantity );
		MarkItemsListChanged();
	}	
}

void CInventoryComponent::AddItemsFromTemplate( const CEntityTemplate* temp )
{
	RED_ASSERT( temp != nullptr, TXT( "CInventoryComponent::InitFromTemplate - template is null" ) );

	if ( !GCommonGame->GetDefinitionsManager() )
	{
		return;
	}

	// Get entity template parameters
	TDynArray< CInventoryDefinition* > eqStateDefs;
	temp->GetAllParameters< CInventoryDefinition >( eqStateDefs );

	// Setup equipment state based on equipment definition from entity template
	for ( Uint32 i=0; i<eqStateDefs.Size(); ++i )
	{
		const CInventoryDefinition* definition = eqStateDefs[i];
		const TDynArray< CInventoryDefinitionEntry* >& definitionEntries = definition->GetEntries();
		auto entryIt = definitionEntries.Begin();
		const auto endEntry = definitionEntries.End();
		for ( ; entryIt != endEntry; ++entryIt )
		{
			const CInventoryDefinitionEntry* entry = *entryIt;

			if ( entry->m_initializer )
			{
				// Evaluate quantity 
				const Uint32& quantity = entry->EvaluateQuantity();
				if ( quantity > 0 )
				{
					if ( !GCommonGame->GetDefinitionsManager()->CategoryExists( entry->m_category ) )
					{
						return;
					}

					// Evaluate item name
					const CName& itemName = entry->m_initializer->EvaluateItemName( entry->m_category );

					// Finally, add item to inventory
					CInventoryComponent::SAddItemInfo addItemInfo;
					addItemInfo.m_quantity = quantity;
					addItemInfo.m_informGui = false;
					addItemInfo.m_markAsNew = false;
					addItemInfo.m_isLootable = entry->m_isLootable;
					AddItem( itemName, addItemInfo );

					// Mount the item automatically
					if ( entry->m_isMount )
					{
						// Mount item if expected to
						SItemUniqueId itemId = GetItemId( itemName );
						if ( itemId != SItemUniqueId::INVALID )
						{
							SInventoryItem* item = GetItem( itemId );
							RED_ASSERT( item != nullptr, TXT( "Cannot find item with id: %d" ), itemId );
							if ( item != nullptr )
							{
								item->SetIsMounted( true );
							}
						}
					}
				}
			}
		}
	}

	// Apply some custom items that are related to current appearance
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( GetEntity() );
	if ( appearanceComponent )
	{
		const CEntityAppearance* appearance = temp->GetAppearance( appearanceComponent->GetAppearance(), true );
		if ( appearance )
		{
			Cast<CGameplayEntity>( GetEntity() )->ApplyAppearanceEquipment( appearance );
		}
	}
}

void CInventoryComponent::FixMountedUnequippedWeapons()
{
	// The following logic is valid only before the actual ItemEntity spawn.
	// That's why this method is expected to be called right after gameplay state load and before item entities spawn.

	if ( GetParent() == nullptr || !GetParent()->IsA< CPlayer >() || GCommonGame == nullptr )
	{
		return;
	}

	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
	if ( dm == nullptr )
	{
		return;
	}

	// let's take the list of all equipped items
	TDynArray< SItemUniqueId > equippedItems;
	IScriptable* context = GetParent();
	if ( !CallFunctionRet( context, CNAME( GetEquippedItems ), equippedItems ) )
	{
		return;
	}

	// if there is a weapon that is marked as mounted or held, but is not equipped -> mark it as not mounted nor held
	for ( SInventoryItem& item : m_items )
	{
		if ( ( item.IsMounted() || item.IsHeld() ) && !equippedItems.Exist( item.GetUniqueId() ) )
		{
			const SItemDefinition* itemDef = dm->GetItemDefinition( item.GetName() );
			if ( itemDef != nullptr && itemDef->IsWeapon() )
			{
				item.SetIsMounted( false );
				item.SetIsHeld( false );

				// unmount bounded items as well				
				const TDynArray< CName >& boundItems = itemDef->GetBoundItems( true );
				for ( CName boundItemName : boundItems )
				{
					Int32 boundItemIndex = NameToIndex( boundItemName );
					if ( IsIndexValid( boundItemIndex ) )
					{
						m_items[ boundItemIndex ].SetIsMounted( false );
						m_items[ boundItemIndex ].SetIsHeld( false );
					}
				}
			}
		}
	}
}

void CInventoryComponent::BalanceItemsWithPlayerLevel( Uint32 playerLevel )
{
	if( m_rebalanceEveryNSeconds == 0 ) 
	{
		return;
	}

	RED_ASSERT( GGame && GGame->GetTimeManager() );
	GameTime currTime = GGame->GetTimeManager()->GetTime();
	Uint32 currSeconds = currTime.GetSeconds();

	if( currSeconds < m_nextRebalance )
	{
		return;
	}

	m_suppressEvents = true;
	m_nextRebalance = currSeconds + m_rebalanceEveryNSeconds;

	{ // remove items that should be rebalanced
		TDynArray< SItemUniqueId >	uniqueIds;
		const Uint32 itemsCount = m_items.Size();
		for ( Uint32 i = 0; i < itemsCount; ++i )
		{
			const SInventoryItem& item = m_items[ i ];
			if( item.ShouldBeRebalanced() )
			{
				uniqueIds.PushBack( item.GetUniqueId() );
			}
		}

		const Uint32 uniqueIdsCount = uniqueIds.Size();
		for ( Uint32 i = 0; i < uniqueIdsCount; ++i )
		{
			const SItemUniqueId& uniqueId = uniqueIds[ i ];
			RemoveItem( uniqueId, GetItemQuantity( uniqueId ) );
		}
	}

	m_uniqueIdGenerator.Reset();
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		SItemUniqueId newId = m_uniqueIdGenerator.GenerateNewId();
		if ( newId != m_items[ i ].GetUniqueId() )
		{
			// mark old id as looted preserving it's looted state
			NotifyItemLooted( m_items[ i ].GetUniqueId().GetValue(), WasItemLooted( newId ) );

			// assign new id and mark as not looted
			m_items[ i ].SetUniqueId( newId );
			NotifyItemLooted( newId, false );
		}
	}

	//RemoveAllItems();
	m_loot.Update( this, true );

	m_suppressEvents = false;
	NotifyListeners( IET_InventoryRebalanced, SItemUniqueId::INVALID, 0, false );
}

void CInventoryComponent::UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager )
{
	ASSERT( m_currentLOD != newLOD );
	m_currentLOD = newLOD;

	if( m_currentLOD == ILODable::LOD_0 )
	{
		SpawnMountedItems();
	}
	else
	{
		DespawnOwnedItems();
	}
}

ILODable::LOD CInventoryComponent::ComputeLOD( CLODableManager* manager ) const
{
	if ( !GetEntity()->IsGameplayLODable() )
	{
		return ILODable::LOD_0;
	}

	const Float distSqr = manager->GetPosition().DistanceSquaredTo2D( GetEntity()->GetWorldPositionRef() );
	if ( distSqr < manager->GetBudgetableDistanceSqr() )
	{
		return ILODable::LOD_0;
	}
	else if ( distSqr < manager->GetDisableDistanceSqr() )
	{
		return m_currentLOD;
	}

	return ILODable::LOD_1;
}

void CInventoryComponent::InitFromTemplate( const CEntityTemplate* temp )
{
	RED_ASSERT( m_items.Size() == 0, TXT("Double inventory initialization") );

	AddItemsFromTemplate( temp );
}

CInventoryComponent* CInventoryComponent::GetAssociatedInventory()
{
	struct SHasAssociatedInventoryEvaluator
	{
		Bool operator()( const CGameplayEntity* entity ) const
		{
			CName entityClassName = entity->GetClass()->GetName();
			return ( entityClassName == CNAME( W3PlayerWitcher ) || entityClassName == CNAME( W3HorseManager ) );
		}
	} evaluator;

	CEntity* entity = GetEntity();
	if ( entity != nullptr && entity->IsA< CGameplayEntity >() )
	{
		CGameplayEntity* gameplayEntity = static_cast< CGameplayEntity* >( entity );
		if ( gameplayEntity->GetInfoCache().Get( gameplayEntity, GICT_Custom3, evaluator ) )
		{
			if ( !m_associatedInventory.IsValid() )
			{
				CallFunctionRet( GetEntity(), CNAME( GetAssociatedInventory ), m_associatedInventory );
			}
			return m_associatedInventory.Get();
		}
	}

	return nullptr;
}

Bool CInventoryComponent::WasItemLooted( Uint32 itemID ) const
{
	CPeristentEntity* ent = Cast< CPeristentEntity >( GetEntity() );
	if( !ent )
	{
		return false;
	}

	return GGame->GetContainerManager().WasItemLooted( ent->GetIdTag(), itemID );
}

void CInventoryComponent::NotifyItemLooted( Uint32 itemID, Bool looted /* = true */  )
{
	CPeristentEntity* ent = Cast< CPeristentEntity >( GetEntity() );
	if( !ent )
	{
		return;
	}

	GGame->GetContainerManager().NotifyItemLooted( ent->GetIdTag(), itemID, looted );
}

void CInventoryComponent::NotifyQuestItemLooted( Uint32 itemID )
{
	CPeristentEntity* ent = Cast< CPeristentEntity >( GetEntity() );
	if( !ent )
	{
		return;
	}

	GGame->GetContainerManager().NotifyQuestItemLooted( ent->GetIdTag(), itemID );
}

void CInventoryComponent::ResetContainerData()
{
	CPeristentEntity* ent = Cast< CPeristentEntity >( GetEntity() );
	if( !ent )
	{
		return;
	}

	GGame->GetContainerManager().ResetContainerData( ent->GetIdTag() );
}

Uint32 CInventoryComponent::GenerateSeed() const
{
	CEntity* entity = GetEntity();
	if( !entity )
	{
		return 0xABCDEF; // some value, should never be returned, just for crash-safe
	}

	const String& name = entity->GetName();
	Uint32 crc = 0;
	name.Checksum( crc );

	const Vector pos = entity->GetWorldPosition();
	Uint32 px = ( Uint32 )Red::Math::MFloor( pos.X );
	Uint32 py = ( Uint32 )Red::Math::MFloor( pos.Y );
	Uint32 pz = ( Uint32 )Red::Math::MFloor( pos.Z / 10.0f );
	return px + py + pz + crc;
}

Bool CInventoryComponent::IsAContainer() const
{
	CEntity* entity = GetEntity();
	if( !entity )
	{
		return false;
	}

	CClass* entityClass = entity->GetClass();
	CClass* componentClass = SRTTI::GetInstance().FindClass( CNAME( W3Container ) );
	return entityClass->IsA( componentClass );
}


Float CInventoryComponent::GetItemWeight( SItemUniqueId itemId ) const
{
	if ( itemId == SItemUniqueId::INVALID )
	{
		//RED_ASSERT( itemId != SItemUniqueId::INVALID, TXT( "Invalid Item ID: %d" ), itemId );
		ERR_GAME( TXT( "Invalid Item ID: %d" ), itemId );
		return -1;
	}

	Int32 index = UniqueIdToIndex( itemId );
	if ( IsIndexValid( index ) == false )
	{
		//RED_ASSERT( index != false, TXT( "Cannot find valid index with item id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find valid index with item id: %d" ), itemId );
		return -1;
	}

	// Validate Item
	const SInventoryItem* invItem = GetItem( itemId );
	if ( nullptr == invItem )
	{
		//RED_ASSERT( invItem != nullptr, TXT( "Cannot find item with id: %d" ), itemId );
		ERR_GAME( TXT( "Cannot find item with id: %d" ), itemId );
		return -1;
	}

	return GetItemWeight( *invItem );
}

// Return Item specified Weight else search / return weight from item ability stats.
Float CInventoryComponent::GetItemWeight( const SInventoryItem& invItem ) const
{
	// Set Price based on Definition
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	if ( nullptr == defMgr )
	{
		//RED_ASSERT( defMgr != nullptr, TXT( "Definitions Manager is NULL" ) );
		ERR_GAME( TXT( "Definitions Manager is NULL" ) );
		return -1;
	}

	Float itemWeight = defMgr->GetItemWeight( invItem.GetName() );

	if ( itemWeight <= 0 )
	{
		SAbilityAttributeValue attValue;
		
		TDynArray< CName > allAbilities;
		invItem.GetAllAbilities( allAbilities );

		attValue = defMgr->CalculateAttributeValue( allAbilities, CNAME( weight ), false, 0 );

		return attValue.m_valueBase;// * attValue.m_valueMultiplicative + attValue.m_valueAdditive;
	}

	return itemWeight;
}

// ----------------------------------------------------------------------------
// Scripting
// ----------------------------------------------------------------------------

void CInventoryComponent::funcGetItemsNames( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	TDynArray< CName > & resultArr = *(TDynArray< CName >*) result;
	for (auto it = m_items.Begin(); it != m_items.End(); ++it)
	{
		resultArr.PushBack( it->GetName() );
	}
}

void CInventoryComponent::funcGetItemCount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, useAssociatedInventory, false );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemCount( useAssociatedInventory ) );
}

void CInventoryComponent::funcGetItemName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	CName itemName;
	const SInventoryItem* item = GetItem( itemId );
	if( item )
	{
		itemName = item->GetName();
	}

	RETURN_NAME( itemName );
}

void CInventoryComponent::funcGetItemCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	CName itemCategory;
	const SInventoryItem* item = GetItem( itemId );
	if( item )
	{
		itemCategory = item->GetCategory();
	}

	RETURN_NAME( itemCategory );
}

void CInventoryComponent::funcGetItemClass( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	if ( result != NULL )
	{
		EInventoryItemClass itemClass = InventoryItemClass_Common;

		const SInventoryItem* item = GetItem( itemId );
		if( item )
		{
			itemClass = item->GetItemClass();
		}

		*( static_cast< EInventoryItemClass * >( result ) ) = itemClass;
	}
}

void CInventoryComponent::funcGetItemTags( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_REF( TDynArray< CName >, tags, TDynArray< CName >() );
	FINISH_PARAMETERS;

	RETURN_BOOL( GetItemTags( itemId , tags ) );
}

void CInventoryComponent::funcItemHasTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( ItemHasTag( itemId, tag ) );
}

void CInventoryComponent::funcAddItemTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( AddItemTag( itemId, tag ) );
}

void CInventoryComponent::funcRemoveItemTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( RemoveItemTag( itemId, tag ) );
}

void CInventoryComponent::funcGetItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* item = GetItem( itemId );
	if ( item != nullptr )
	{
		RETURN_STRUCT( SInventoryItem, *item );
	}
	else
	{
		RETURN_STRUCT( SInventoryItem, SInventoryItem::INVALID );
	}
}

void CInventoryComponent::funcActivateQuestBonus( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ActivateQuestBonus();
}

void CInventoryComponent::funcAddSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	const SInventoryItem* item = GetItem( itemId );	
	if ( item )
	{
		RETURN_BOOL( AddSlot( const_cast< SInventoryItem & >( *item ) ) );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CInventoryComponent::funcGetSlotItemsLimit( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	const SInventoryItem* item = GetItem( itemId );	
	if ( item )
	{
		RETURN_INT( GetSlotItemsLimit( const_cast< SInventoryItem & >( *item ) ) );
	}
	else
	{
		RETURN_INT( 0 );
	}
}

void CInventoryComponent::funcTotalItemStats( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	FINISH_PARAMETERS;

	RETURN_FLOAT( TotalItemStats( invItem ) );
}

void CInventoryComponent::funcGetItemWeight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_FLOAT( static_cast< Float >( GetItemWeight( itemId ) ) );
}

void CInventoryComponent::funcGetItemPrice( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_INT( static_cast< Int32 >( GetItemPrice( itemId ) ) );
}

void CInventoryComponent::funcGetItemPriceModified( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Bool, playerSellingItem, false );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceModified( itemId, playerSellingItem ) );
}

void CInventoryComponent::funcGetInventoryItemPriceModified( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	GET_PARAMETER( Bool, playerSellingItem, false );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceModified( invItem, playerSellingItem ) );
}

void CInventoryComponent::funcGetItemPriceRepair( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	GET_PARAMETER_REF( Int32, costRepairPoint, 1 );
	GET_PARAMETER_REF( Int32, costRepairTotal, 1 );
	FINISH_PARAMETERS;

	GetItemPriceRepair( invItem, costRepairPoint, costRepairTotal );
}

void CInventoryComponent::funcGetItemPriceRemoveUpgrade( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceRemoveUpgrade( invItem ) );
}

void CInventoryComponent::funcGetFundsModifier( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetFundsModifier() );
}

void CInventoryComponent::funcGetItemPriceCrafting( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceCrafting( invItem ) );
}

void CInventoryComponent::funcGetItemPriceAddSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceAddSlot( invItem ) );
}

void CInventoryComponent::funcGetItemPriceRemoveEnchantment( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceRemoveEnchantment( invItem ) );
}

void CInventoryComponent::funcGetItemPriceEnchantItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceEnchantItem( invItem ) );
}

void CInventoryComponent::funcGetItemPriceDisassemble( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SInventoryItem, invItem, SInventoryItem() );
	FINISH_PARAMETERS;

	RETURN_INT( GetItemPriceDisassemble( invItem ) );
}

void CInventoryComponent::funcGetItemByItemEntity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CItemEntity>, itemEntity, NULL );
	FINISH_PARAMETERS;

	SInventoryItem * item = GetItem( itemEntity.Get() );
	if ( item != NULL )
	{
		RETURN_STRUCT( SItemUniqueId, item->GetUniqueId() );
	}
	else
	{
		RETURN_STRUCT( SItemUniqueId, SItemUniqueId::INVALID );
	}
}

void CInventoryComponent::funcGetItemAttributes( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_REF( TDynArray<CName>, attributes, TDynArray<CName>() );
	FINISH_PARAMETERS;

	const SInventoryItem* item = GetItem( itemId );	
	if ( item )
	{
		item->GetAllAttributes( attributes );
	}
}

void CInventoryComponent::funcGetItemBaseAttributes( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_REF( TDynArray<CName>, attributes, TDynArray<CName>() );
	FINISH_PARAMETERS;

	const SInventoryItem* item = GetItem( itemId );	
	if ( item )
	{
		item->GetBaseAttributes( attributes );
	}
}

void CInventoryComponent::funcGiveItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CInventoryComponent >, otherInventory, NULL );
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( Int32, quantity, 1 );
	FINISH_PARAMETERS;

    TDynArray < SItemUniqueId > res = GiveItem( otherInventory.Get(), itemId, quantity );

    TDynArray< SItemUniqueId > & resultArr = *(TDynArray< SItemUniqueId >*) result;
    resultArr.PushBack( res );
}

void CInventoryComponent::funcHasItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasItem( itemName ) );
}

void CInventoryComponent::funcAddMultiItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER_OPT( Int32, quantity, 1 );
	GET_PARAMETER_OPT( Bool, informGui, true );
	GET_PARAMETER_OPT( Bool, markAsNew, false );
	GET_PARAMETER_OPT( Bool, lootable, true );
	FINISH_PARAMETERS;

	SAddItemInfo addItemInfo;
	addItemInfo.m_quantity = static_cast< Uint32 >( quantity );
	addItemInfo.m_informGui = informGui;
	addItemInfo.m_markAsNew = markAsNew;
	addItemInfo.m_isLootable = lootable;
    TDynArray< SItemUniqueId > returnVals = AddItem( itemName, addItemInfo );

    TDynArray< SItemUniqueId > & resultArr = *(TDynArray< SItemUniqueId >*) result;
    resultArr.PushBack( returnVals );
}

void CInventoryComponent::funcAddSingleItem( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, itemName, CName::NONE );
    GET_PARAMETER_OPT( Bool, informGui, true );
    GET_PARAMETER_OPT( Bool, markAsNew, false );
	GET_PARAMETER_OPT( Bool, lootable, true );
    FINISH_PARAMETERS;

	SAddItemInfo addItemInfo;
	addItemInfo.m_quantity = 1;
	addItemInfo.m_informGui = informGui;
	addItemInfo.m_markAsNew = markAsNew;
	addItemInfo.m_isLootable = lootable;

	TDynArray< SItemUniqueId > ids = AddItem( itemName, addItemInfo );
	if( ids.Empty() )
	{
		RETURN_STRUCT( SItemUniqueId, SItemUniqueId( 0 ) );
	}
	else
	{
		RETURN_STRUCT( SItemUniqueId, ids[0] );
	}
}

void CInventoryComponent::funcRemoveItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( Int32, quantity, 1 );
	FINISH_PARAMETERS;

	RETURN_BOOL( RemoveItem( itemId, quantity ) );
}

void CInventoryComponent::funcGetItemEntityUnsafe( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetItemEntityUnsafe( itemId ) );
}

void CInventoryComponent::funcInitInvFromTemplate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, handle, nullptr );
	FINISH_PARAMETERS;

	if( const CEntityTemplate* temp =  handle.Get() )
	{
		AddItemsFromTemplate( temp );
	}
}

void CInventoryComponent::funcMountItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( Bool, toHand, false );
	GET_PARAMETER_OPT( Bool, force, false );
	FINISH_PARAMETERS;

	SMountItemInfo info;
	info.m_toHand = toHand;
	info.m_force = force;
	RETURN_BOOL( MountItem( itemId, info ) );
}

void CInventoryComponent::funcUnmountItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Bool, destroyEntity, true );
	FINISH_PARAMETERS;

	Bool res = false;
	SInventoryItem* item = GetItem( itemId );
	if ( item != nullptr && ( item->IsHeld() || item->IsMounted() ) )
	{
		res = UnMountItem( itemId, destroyEntity );
	}

	RETURN_BOOL( res );
}

void CInventoryComponent::funcThrowAwayItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( Int32, quantity, 1 );
	FINISH_PARAMETERS;

	RETURN_BOOL( ThrowAwayItem( itemId, quantity ) != NULL );
}

void CInventoryComponent::funcThrowAwayAllItems( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( ThrowAwayAllItems() );
}

void CInventoryComponent::funcThrowAwayItemsFiltered( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, excludedTags, TDynArray< CName >() );
	FINISH_PARAMETERS;
	RETURN_OBJECT( ThrowAwayItemsFiltered( excludedTags ) );
}

void CInventoryComponent::funcThrowAwayLootableItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, skipNoDropNoShow, false );
	FINISH_PARAMETERS;
	RETURN_OBJECT( ThrowAwayLootableItems( skipNoDropNoShow ) );
}

void CInventoryComponent::funcPrintInfo( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	PrintInfo();
}

void CInventoryComponent::funcGetItemQuantity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;
	
	RETURN_INT( GetItemQuantity( itemId ) );
}

void CInventoryComponent::funcGetItemQuantityByName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER_OPT( Bool, useAssociatedInventory, false );
	GET_PARAMETER_OPT( TDynArray< CName >, ignoreTags, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Uint32 count = 0;
	if ( itemName != CName::NONE )
	{
		count = GetItemQuantityByName( itemName, useAssociatedInventory, ignoreTags.Size() > 0 ? &ignoreTags : nullptr );
	}

	RETURN_INT( count );
}

void CInventoryComponent::funcGetItemQuantityByCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemCategory, CName::NONE );
	GET_PARAMETER_OPT( Bool, useAssociatedInventory, false );
	GET_PARAMETER_OPT( TDynArray< CName >, ignoreTags, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Uint32 count = 0;
	if ( itemCategory != CName::NONE )
	{
		count = GetItemQuantityByCategory( itemCategory, useAssociatedInventory, ignoreTags.Size() > 0 ? &ignoreTags : nullptr );
	}

	RETURN_INT( count );
}

void CInventoryComponent::funcGetItemQuantityByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemTag, CName::NONE );
	GET_PARAMETER_OPT( Bool, useAssociatedInventory, false );
	GET_PARAMETER_OPT( TDynArray< CName >, ignoreTags, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Uint32 count = 0;
	if ( itemTag != CName::NONE )
	{
		count = GetItemQuantityByTag( itemTag, useAssociatedInventory, ignoreTags.Size() > 0 ? &ignoreTags : nullptr );
	}

	RETURN_INT( count );
}

void CInventoryComponent::funcGetAllItemsQuantity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, useAssociatedInventory, false );
	GET_PARAMETER_OPT( TDynArray< CName >, ignoreTags, TDynArray< CName >() );
	FINISH_PARAMETERS;

	RETURN_INT( GetAllItemsQuantity( useAssociatedInventory, ignoreTags.Size() > 0 ? &ignoreTags : nullptr ) );
}

void CInventoryComponent::funcDespawnItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	DespawnItem( itemId );
}

void CInventoryComponent::funcGetAllItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< SItemUniqueId >, items, TDynArray< SItemUniqueId >() );
	FINISH_PARAMETERS;

	items.Clear();
	for ( Uint32 i=0; i < m_items.Size(); ++i )
	{
		items.PushBack( m_items[i].GetUniqueId() );
	}
}

void CInventoryComponent::funcGetItemId( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	SItemUniqueId id = SItemUniqueId::INVALID;
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		const SInventoryItem& item = m_items[ i ];
		if ( item.GetName() == name )
		{
			id = item.GetUniqueId();
			break;
		}
	}
	RETURN_STRUCT( SItemUniqueId, id );
}

void CInventoryComponent::funcGetItemsIds( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< SItemUniqueId > & retVal = *(TDynArray< SItemUniqueId >*) result;
		retVal.ClearFast();

		const Uint32 itemsCount = m_items.Size();
		for ( Uint32 i = 0; i < itemsCount; ++i )
		{
			const SInventoryItem& item = m_items[ i ];
			if ( item.GetName() == name )
			{
				retVal.PushBack( item.GetUniqueId() );
			}
		}
	}
}

void CInventoryComponent::funcGetItemsByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< SItemUniqueId > & retVal = *(TDynArray< SItemUniqueId >*) result;
		
		retVal.ClearFast();
		
		const Uint32 itemsCount = m_items.Size();
		for ( Uint32 i = 0; i < itemsCount; ++i )
		{
			const SInventoryItem& item = m_items[ i ];
			if ( item.GetTags().Exist( tag ) )
			{
				retVal.PushBack( item.GetUniqueId() );
			}
		}
	}
}

void CInventoryComponent::funcGetItemsByCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, category, CName::NONE );
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< SItemUniqueId > & retVal = *(TDynArray< SItemUniqueId >*) result;
		retVal.ClearFast();
		GetItemsByCategory( category, retVal );
	}
}

void CInventoryComponent::funcIsIdValid( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsIdValid( itemId ) );
}

void CInventoryComponent::funcGetItemEnhancementSlotsCount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	Uint8 defCount = 0;
	Uint8 currentSlotCount;

	SInventoryItem* item = GetItem( itemId );
	if ( item )
	{
		const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );
		if ( itemDef )
		{
			defCount = itemDef->GetEnhancementSlotCount();
		}

		currentSlotCount = item->GetSlotItemsMax(); // GetSlotItems().Size();
		
		if ( currentSlotCount > defCount )
		{
			RETURN_INT( currentSlotCount );
		}
		else
		{
			RETURN_INT( defCount );
		}
		return;
	}

	RETURN_INT( 0 );
}

void CInventoryComponent::funcGetItemEnhancementItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_REF( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Int32 idx = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( idx ) )
	{
		ITEM_ERR( TXT( "funcGetItemSlotItems - invalid id" ) );
		return;
	}

	const SInventoryItem& item = m_items[ idx ];

	names.Clear();
	if ( item.IsEnchanted() )
	{
		names.PushBack( item.GetEnchantment() );
	}
	else
	{
		item.GetSlotItemsNames( names );
	}
}

void CInventoryComponent::funcGetItemEnhancementCount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	Int32 idx = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( idx ) )
	{
		ITEM_ERR( TXT( "funcGetItemSlotItems - invalid id" ) );
		RETURN_INT( 0 );
		return;
	}

	const SInventoryItem& item = m_items[ idx ];
	if ( item.IsEnchanted() )
	{
		RETURN_INT( 3 );
	}
	else
	{
		RETURN_INT( item.GetSlotItems().Size() );
	}
}

void CInventoryComponent::funcHasEnhancementItemTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, enhancedItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Int32, slotIndex, 0 );
	GET_PARAMETER( CName, tag, CName::NONE );

	FINISH_PARAMETERS;

	RETURN_BOOL( HasEnhancementItemTag( enhancedItemId, slotIndex, tag ) );
}

void CInventoryComponent::funcGetItemColor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* invItem = GetItem( itemId );
	if ( !invItem )
	{
		RETURN_NAME( CName::NONE );
		return;
	}

	RETURN_NAME( invItem->GetDyeColorName() );
}

void CInventoryComponent::funcIsItemColored( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* invItem = GetItem( itemId );
	if ( invItem != nullptr )
	{
		RETURN_BOOL( invItem->IsItemColored() );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CInventoryComponent::funcColorItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( SItemUniqueId, dyeId, SItemUniqueId::INVALID );

	FINISH_PARAMETERS;

	RETURN_BOOL( ColorItem( itemId, dyeId ) );
}

void CInventoryComponent::funcClearItemColor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );

	FINISH_PARAMETERS;

	RETURN_BOOL( ClearItemColor( itemId ) );
}

void CInventoryComponent::funcGetEnchantment( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* invItem = GetItem( itemId );
	if ( !invItem )
	{
		RETURN_NAME( CName::NONE );
		return;
	}

	RETURN_NAME( invItem->GetEnchantment() );
}

void CInventoryComponent::funcIsItemEnchanted( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* invItem = GetItem( itemId );
	if ( !invItem )
	{
		RETURN_BOOL( false );
		return;
	}

	RETURN_BOOL( invItem->IsEnchanted() );
}

void CInventoryComponent::funcEnchantItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, enhancedItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, enchantmentName, CName::NONE );
	GET_PARAMETER( CName, enchantmentStat, CName::NONE );

	FINISH_PARAMETERS;

	RETURN_BOOL( EnchantItem( enhancedItemId, enchantmentName, enchantmentStat ) );
}

void CInventoryComponent::funcUnenchantItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, enhancedItemId, SItemUniqueId::INVALID );

	FINISH_PARAMETERS;

	RETURN_BOOL( UnenchantItem( enhancedItemId ) );
}

void CInventoryComponent::funcEnhanceItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, enhancedItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( SItemUniqueId, extensionItemId, SItemUniqueId::INVALID );

	FINISH_PARAMETERS;

	RETURN_BOOL( EnhanceItem( enhancedItemId, extensionItemId ) );
}

void CInventoryComponent::funcRemoveItemEnhancementByIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, enhancedItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Int32, slotIndex, 0 );

	FINISH_PARAMETERS;

	RETURN_BOOL( RemoveEnhancementItem( enhancedItemId, slotIndex ) );
}

void CInventoryComponent::funcRemoveItemEnhancementByName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, enhancedItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, extensionItemName, CName::NONE );

	FINISH_PARAMETERS;

	RETURN_BOOL( RemoveEnhancementItem( enhancedItemId, extensionItemName ) );
}

void CInventoryComponent::funcGetItemAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );	
	GET_PARAMETER_REF( TDynArray< CName >, abilities, TDynArray< CName >() );
	FINISH_PARAMETERS;

	GetItemAbilities( itemId, abilities );
}

void CInventoryComponent::funcGetItemContainedAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );	
	GET_PARAMETER_REF( TDynArray< CName >, abilities, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Int32 index = UniqueIdToIndex( itemId );
	if ( !IsIndexValid( index ) )
	{
		ITEM_WARN( TXT("Using GetItemAbilities with invalid id") );
		return;
	}

	m_items[index].GetContainedAbilities( abilities );
}

void CInventoryComponent::funcGetCraftedItemName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_NAME( GetCraftedItemName( itemId ) );
}

void CInventoryComponent::funcIsItemMounted( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* item = GetItem( itemId );
	if ( !item )
	{
		RETURN_BOOL( false );
		return;
	}

	RETURN_BOOL( item->IsMounted() );
}

void CInventoryComponent::funcIsItemHeld( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* item = GetItem( itemId );
	if ( !item )
	{
		RETURN_BOOL( false );
		return;
	}

	RETURN_BOOL( item->IsHeld() );
}

void CInventoryComponent::funcGetDeploymentItemEntity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( Vector, position, Vector::ZERO_3D_POINT );
	GET_PARAMETER_OPT( EulerAngles, rotation, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Bool, allocateIdTag, false );
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetDeploymentItemEntity( itemId, position, rotation, allocateIdTag ) );
}

void CInventoryComponent::funcPlayItemEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, effectName, CName::NONE );
	FINISH_PARAMETERS;

	PlayItemEffect( itemId, effectName );
}

void CInventoryComponent::funcStopItemEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, effectName, CName::NONE );
	FINISH_PARAMETERS;

	StopItemEffect( itemId, effectName );
}

void CInventoryComponent::funcDropItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER_OPT( Bool, removeFromInv, false );
	FINISH_PARAMETERS;

	DropItem( itemId, removeFromInv );
}

void CInventoryComponent::funcGetItemHoldSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	CName holdSlotName;
	SInventoryItem* item = GetItem( itemId );
	if ( item )
	{
		const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );
		if ( itemDef )
		{
			holdSlotName = itemDef->GetHoldSlot( IsPlayerOwner() );
		}
	}
	RETURN_NAME( holdSlotName );
}

void CInventoryComponent::funcEnableLoot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;
	m_loot.Enable( enable );
}

void CInventoryComponent::funcUpdateLoot( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if( GetEntity()->IsInGame() )
	{
		m_loot.Update( this );
	}
}

void CInventoryComponent::funcAddItemsFromLootDefinition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, lootDefinitionName, CName::NONE );
	FINISH_PARAMETERS;
	m_loot.AddItemsFromDefinition( this, lootDefinitionName );
}

void CInventoryComponent::funcIsLootRenewable( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_loot.IsRenewable() );
}

void CInventoryComponent::funcIsReadyToRenew( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_loot.IsReadyToRenew() );
}

void CInventoryComponent::funcRemoveAllItems( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RemoveAllItems();
}

//! Get the UI-specific data for the item (script export)		 
void CInventoryComponent::funcGetInventoryItemUIData( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SItemUniqueId, uid, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_STRUCT( SInventoryItemUIData, GetInventoryItemUIData( uid ) );
}

//! Set the UI-specific data for the item (script export)	
void CInventoryComponent::funcSetInventoryItemUIData( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SItemUniqueId, uid, SItemUniqueId::INVALID );
	GET_PARAMETER_REF( SInventoryItemUIData, data, SInventoryItemUIData::INVALID );  
	FINISH_PARAMETERS;

	SetInventoryItemUIData( uid, data );
}

void CInventoryComponent::funcGetSchematicIngredients( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
    GET_PARAMETER_REF( TDynArray< Int32 >, outNumbers, TDynArray< Int32 >() );
    GET_PARAMETER_REF( TDynArray< CName >, outNames, TDynArray< CName >() );
    FINISH_PARAMETERS;

    SInventoryItem* invItem  = GetItem(itemId);

    const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );

    if(itemDef)
    {
		const TDynArray< SItemDefinition::SIngredientEntry >& ingredients = itemDef->GetIngredients();
        auto iter = ingredients.Begin();
		const auto endIt = ingredients.End();

        for( ; iter!= endIt; ++iter )
        {
            outNumbers.PushBack( iter->m_quantity );
            outNames.PushBack( iter->m_itemName );
        }
    }
}

void CInventoryComponent::funcGetSchematicRequiredCraftsmanType( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
    FINISH_PARAMETERS;

    SInventoryItem* invItem  = GetItem(itemId);

    const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );

    if(itemDef)
    {
        RETURN_NAME( itemDef->GetCraftsmanType() );
    }
}

void CInventoryComponent::funcGetSchematicRequiredCraftsmanLevel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* invItem  = GetItem(itemId);

	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );

	if(itemDef)
	{
		RETURN_NAME( itemDef->GetCraftsmanLevel() );
	}
}

void CInventoryComponent::funcSortInventoryUIData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	SortItems();
}

void CInventoryComponent::funcGetItemRecyclingParts( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* invItem  = GetItem(itemId);

	if ( invItem != nullptr )
	{
		const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );

		if ( itemDef )
		{
			typedef TDynArray< SItemDefinition::SItemPartsEntry > TItemPartsArray;
			const TItemPartsArray& itemParts = itemDef->GetRecyclingParts();

			TDynArray< SItemParts >& resultArr = *static_cast< TDynArray< SItemParts >* >( result );

			const auto itemPartsEndIt = itemParts.End();
			for ( auto it = itemParts.Begin(); it != itemPartsEndIt; ++it )
			{
				SItemParts parts;
				parts.m_itemName = it->m_name;
				parts.m_quantity = static_cast< Int32 >( it->m_count );
				resultArr.PushBack( parts );
			}
		}
		else
		{
			ERR_GAME( TXT("CInventoryComponent: failed to get recycling parts for item '%ls'"), invItem->GetName().AsChar() );
		}
	}
}

void CInventoryComponent::funcGetItemModifierFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID ); 
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Float , res, -1.f );
	FINISH_PARAMETERS;

	SInventoryItem* invItem  = GetItem(itemId);
	SInventoryItem::SItemModifierVal* itemMod = invItem ? invItem->GetItemMod( name ) : nullptr;
	res = itemMod && itemMod->m_type == SInventoryItem::SItemModifierVal::EMVT_Float ? itemMod->m_float : res ;
	RETURN_FLOAT( res );
}

void CInventoryComponent::funcSetItemModifierFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Float, mod, -1.f );
	FINISH_PARAMETERS;

	SInventoryItem* invItem  = GetItem(itemId);
	if( invItem )
	{
		SInventoryItem::SItemModifierVal* itemMod = invItem->GetItemMod( name, true );
		itemMod->m_type = SInventoryItem::SItemModifierVal::EMVT_Float;
		itemMod->m_float = mod;
	}	
}

void CInventoryComponent::funcGetItemModifierInt( CScriptStackFrame& stack, void* result )
{ 
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Int32, res, -1 );
	FINISH_PARAMETERS;
	
	SInventoryItem* invItem  = GetItem(itemId);
	SInventoryItem::SItemModifierVal* itemMod = invItem ? invItem->GetItemMod( name ) : nullptr;
	res = itemMod && itemMod->m_type == SInventoryItem::SItemModifierVal::EMVT_Int ? itemMod->m_int : res;
	RETURN_INT( res );
}

void CInventoryComponent::funcSetItemModifierInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, mod, -1 );
	FINISH_PARAMETERS;

	SInventoryItem* invItem  = GetItem(itemId);
	if( invItem )
	{
		SInventoryItem::SItemModifierVal* itemMod = invItem->GetItemMod( name, true );
		itemMod->m_type = SInventoryItem::SItemModifierVal::EMVT_Int;
		itemMod->m_int = mod;
	}
}

void CInventoryComponent::funcHasItemDurability( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasItemDurability( itemId ) );
}

void CInventoryComponent::funcGetItemDurability( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetItemDurability( itemId ) );
}

void CInventoryComponent::funcSetItemDurability( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Float, durability, -1 );
	FINISH_PARAMETERS;

	SetItemDurability( itemId, durability );
}

void CInventoryComponent::funcGetItemInitialDurability( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetItemInitialDurability( itemId ) );
}

void CInventoryComponent::funcGetItemMaxDurability( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetItemMaxDurability( itemId ) );
}

void CInventoryComponent::funcGetItemGridSize( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_INT( Uint64( GetItemGridSize( itemId ) ) );
}

void CInventoryComponent::funcSplitItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Int32, qty, 1 );
	FINISH_PARAMETERS;

	RETURN_STRUCT( SItemUniqueId, SplitItem( itemId, qty ) );
}

void CInventoryComponent::funcSetItemStackable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( Bool, flag, true );
	FINISH_PARAMETERS;

	if ( SInventoryItem* invItem  = GetItem( itemId ) )
    {
		if ( invItem->HasFlag( SInventoryItem::FLAG_STACKABLE ) == flag )
		{
			// nothing to do
			return;
		}

		if ( invItem->GetQuantity() != 1 )
		{
			RED_LOG( Items, TXT("Can't change stackable flag of item %ls, because current quantity == %ld (needs to be 1 to modify that flag)"), invItem->GetName().AsChar(), invItem->GetQuantity() );
			return;
		}

		invItem->SetFlag( SInventoryItem::FLAG_STACKABLE, flag );
	}
}

void CInventoryComponent::funcGetCategoryDefaultItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, category, CName::NONE );
	FINISH_PARAMETERS;
	CName itemName = GetCategoryDefaultItem( category );
	RETURN_NAME( itemName );
}

void CInventoryComponent::funcGetItemSetName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	SInventoryItem* invItem  = GetItem(itemId);
    if(invItem)
    {
	    const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );
	    if( itemDef )
        {
		    RETURN_NAME( itemDef->GetSetName() );
            return;
        }
    }

    RETURN_NAME( CName::NONE );
}

void CInventoryComponent::funcGetItemLocalizedNameByUniqueID( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
    FINISH_PARAMETERS;

    SInventoryItem* invItem  = GetItem(itemId);
    if(invItem)
    {
        const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );

        if( itemDef )
        {
            RETURN_STRING( itemDef->GetLocalizationKeyName() );
            return;
        }
    }

    RETURN_STRING( String::EMPTY );
}

void CInventoryComponent::funcGetItemLocalizedDescriptionByUniqueID( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
    FINISH_PARAMETERS;
    
    SInventoryItem* invItem  = GetItem(itemId);
    if(itemId)
    {
        const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() );

        if( itemDef )
        {
            RETURN_STRING( itemDef->GetLocalizationKeyDesc() );
            return;
        }
    }

    RETURN_STRING( String::EMPTY );
}

void CInventoryComponent::funcGetItemLocalizedNameByName( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, itemName, CName::NONE );
    FINISH_PARAMETERS;

    SItemDefinition* itemDef = const_cast<SItemDefinition*>( GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName ) );
    if( itemDef )
    {
        RETURN_STRING( itemDef->GetLocalizationKeyName() );
        return;
    }

    RETURN_STRING( String::EMPTY );
}

void CInventoryComponent::funcGetItemLocalizedDescriptionByName( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, itemName, CName::NONE );
    FINISH_PARAMETERS;

    SItemDefinition* itemDef = const_cast<SItemDefinition*>( GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName ) );
    if( itemDef )
    {
        RETURN_STRING( itemDef->GetLocalizationKeyDesc() );
        return;
    }

    RETURN_STRING( String::EMPTY );
}

void CInventoryComponent::funcGetItemIconPathByUniqueID( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
    FINISH_PARAMETERS;

    SInventoryItem*  invItem  = GetItem(itemId);
    if(invItem)
    {
        SItemDefinition* itemDef = const_cast<SItemDefinition*>( GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() ) );

        if( itemDef )
        {
            RETURN_STRING( itemDef->GetIconPath() );
            return;
        }
    }

    RETURN_STRING( String::EMPTY);
}

void CInventoryComponent::funcGetItemIconPathByName( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, itemName, CName::NONE );
    FINISH_PARAMETERS;

    SItemDefinition* itemDef = const_cast<SItemDefinition*>( GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName ) );

    if( itemDef )
    {
        RETURN_STRING( itemDef->GetIconPath() );
        return;
    }

    RETURN_STRING( String::EMPTY );
}

void CInventoryComponent::funcGetNumOfStackedItems( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
    FINISH_PARAMETERS;

    SInventoryItem* invItem  = GetItem(itemId);

    if(invItem)
    {
        SItemDefinition* itemDef = const_cast<SItemDefinition*>( GCommonGame->GetDefinitionsManager()->GetItemDefinition( invItem->GetName() ) );

        if( itemDef )
        {
            RETURN_INT( itemDef->GetStackSize() );
            return;
        }
    }

    RETURN_INT( 0 );
}

void CInventoryComponent::funcAddItemCraftedAbility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER_OPT( Bool, allowDuplicate, false );
	FINISH_PARAMETERS;

	SInventoryItem* inventoryItem = GetItem( itemId );
	if ( inventoryItem )
	{
		inventoryItem->AddCraftedAbility( abilityName, allowDuplicate );

		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
		if ( gameplayEntity != nullptr )
		{
			gameplayEntity->OnItemAbilityAdded( itemId, abilityName );
		}
	}
}

void CInventoryComponent::funcRemoveItemCraftedAbility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	SInventoryItem* inventoryItem = GetItem( itemId );
	if ( inventoryItem )
	{
		inventoryItem->RemoveCraftedAbility( abilityName );

		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
		if ( gameplayEntity != nullptr )
		{
			gameplayEntity->OnItemAbilityRemoved( itemId, abilityName );
		}
	}
}

void CInventoryComponent::funcAddItemBaseAbility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	SInventoryItem* inventoryItem = GetItem( itemId );
	if ( inventoryItem )
	{
		inventoryItem->AddBaseAbility( abilityName );

		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
		if ( gameplayEntity != nullptr )
		{
			gameplayEntity->OnItemAbilityAdded( itemId, abilityName );
		}
	}
}

void CInventoryComponent::funcRemoveItemBaseAbility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	SInventoryItem* inventoryItem = GetItem( itemId );
	if ( inventoryItem )
	{
		inventoryItem->RemoveBaseAbility( abilityName );

		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
		if ( gameplayEntity != nullptr )
		{
			gameplayEntity->OnItemAbilityRemoved( itemId, abilityName );
		}
	}
}

void CInventoryComponent::funcPreviewItemAttributeAfterUpgrade( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, baseItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( SItemUniqueId, upgradeItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER_OPT( THandle< CInventoryComponent >, baseInventory, NULL );
	GET_PARAMETER_OPT( THandle< CInventoryComponent >, upgradeInventory, NULL );
	FINISH_PARAMETERS;

	CInventoryComponent* baseInventoryComponent = baseInventory.Get();
	if ( baseInventoryComponent == NULL )
	{
		baseInventoryComponent = this;
	}
	CInventoryComponent* upgradeInventoryComponent = upgradeInventory.Get();
	if ( upgradeInventoryComponent == NULL )
	{
		upgradeInventoryComponent = this;
	}

	SInventoryItem* baseItem = baseInventoryComponent->GetItem( baseItemId );
	SInventoryItem* upgradeItem = upgradeInventoryComponent->GetItem( upgradeItemId );

	SAbilityAttributeValue attributeValue;

	if ( baseItem != NULL && upgradeItem != NULL )
	{
		attributeValue = baseItem->PreviewAttributeAfterUpgrade( upgradeItem, attributeName );
	}

	RETURN_STRUCT( SAbilityAttributeValue, attributeValue );
}

void CInventoryComponent::funcGetItemAttributeValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, baseItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER_OPT( TDynArray< CName >, abilityTags, TDynArray< CName >() );
	GET_PARAMETER_OPT( Bool, withoutTags, false );
	FINISH_PARAMETERS;

//	TagList tags( abilityTags );
	SAbilityAttributeValue finalValue;

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	SInventoryItem* baseItem = GetItem( baseItemId );

	if ( baseItem && defMgr )
	{
		TDynArray< CName > allAbilities;
		baseItem->GetAllAbilities( allAbilities );
		Int32 alwaysRandomSeed = GEngine->GetRandomNumberGenerator().Get< Int32 >();
		if( abilityTags.Empty() )
		{
			finalValue = defMgr->CalculateAttributeValue( allAbilities, attributeName, alwaysRandomSeed, baseItem->GetStaticRandomSeed() );
		}
		else
		{
			TDynArray< CName >	abilities;				
			defMgr->FilterAbilitiesByTags( abilities, allAbilities, abilityTags, withoutTags );
			finalValue = defMgr->CalculateAttributeValue( abilities, attributeName, alwaysRandomSeed, baseItem->GetStaticRandomSeed() );
		}		
	}	

	RETURN_STRUCT( SAbilityAttributeValue, finalValue );
}


void CInventoryComponent::funcGetItemAbilityAttributeValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, baseItemId, SItemUniqueId::INVALID );
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	SAbilityAttributeValue finalValue;

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	SInventoryItem* baseItem = GetItem( baseItemId );

	if ( baseItem && defMgr )
	{
		Int32 alwaysRandomSeed = GEngine->GetRandomNumberGenerator().Get< Int32 >();
		finalValue = defMgr->CalculateAttributeValue( abilityName, attributeName, alwaysRandomSeed, baseItem->GetStaticRandomSeed() );			
	}	

	RETURN_STRUCT( SAbilityAttributeValue, finalValue );
}



void CInventoryComponent::funcGetItemFromSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_STRUCT( SItemUniqueId , GetItemIdHeldInSlot( slotName ) );
}

void CInventoryComponent::funcNotifyScriptedListeners( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, notify, false );
	FINISH_PARAMETERS;
	m_notifyScriptedListeners = notify;
	RETURN_VOID();
}

void CInventoryComponent::funcBalanceItemsWithPlayerLevel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, playerLevel, 0 );
	FINISH_PARAMETERS;
	BalanceItemsWithPlayerLevel( playerLevel );
}

void CInventoryComponent::funcNotifyItemLooted( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RED_ASSERT( itemId.GetValue() > 0, TXT( "Item ID is invalid" ) );

	SInventoryItem* item = GetItem( itemId );
	if( !item )
	{
		return;
	}

	if( item->ShouldBeRebalanced() )
	{
		NotifyItemLooted( itemId.GetValue() );
	}
	else
	{
		NotifyQuestItemLooted( itemId.GetValue() );
	}
}

void CInventoryComponent::funcResetContainerData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ResetContainerData();
}

//! Get the UI-specific data for the item
const SInventoryItemUIData &CInventoryComponent::GetInventoryItemUIData( const SItemUniqueId &id ) const
{
	const Int32 index = UniqueIdToIndex( id );
	if ( IsIndexValid( index ) == false )
	{
		return SInventoryItemUIData::INVALID;
	}

	return m_items[ index ].m_uiData; 

}

//! Set the UI-specific data for the item
void CInventoryComponent::SetInventoryItemUIData( const SItemUniqueId &id, const SInventoryItemUIData &data )
{
	const Int32 index = UniqueIdToIndex( id );
	if ( IsIndexValid( index ) == false )
	{
		return;
	}

	m_items[ index ].m_uiData = data;
}

// Note: More just a test function at the moment. Also invalidates indices.
void CInventoryComponent::SortItems()
{
	TDynArray< SInventoryItem, MC_Inventory > items = m_items;

	struct SInventoryItemSortPredicate
	{
		Bool operator()( const SInventoryItem& a, const SInventoryItem& b ) const
		{
			return a.m_uiData.m_gridSize > b.m_uiData.m_gridSize;
		}
	} pred;

	::Sort( m_items.Begin(), m_items.End(), pred );

	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		m_items[ i ].m_uiData.m_gridPosition = i;
	}
}

void CInventoryComponent::StreamLoad( ISaveFile* loader, Uint32 version )
{
	// Cleanup
	ClearInventory();
	m_uniqueIdGenerator.StreamLoad( loader, version );
	
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( GetEntity() );
	if ( gameplayEntity )
	{
		gameplayEntity->InitInventory( false );
	}

	Uint16 count = 0;
	*loader << count;

	m_failedLoadItemsToMount.Clear();

	const Bool isPlayerOwner = IsPlayerOwner();

	for ( Uint16 i = 0; i < count; ++i )
	{
		SInventoryItem* item = ::new ( m_items ) SInventoryItem;

		Bool loadSuccess = item->StreamLoad( loader, version, isPlayerOwner );

		// HACKFIX: for some reasons some items have empty UID
		if ( loadSuccess && item->GetUniqueId() == SItemUniqueId::INVALID )
		{
			item->SetUniqueId( m_uniqueIdGenerator.GenerateNewId() );
		}

		if ( version >= SAVE_VERSION_STORE_QUEST_TAG_ON_ITEMS_AS_A_FLAG )
		{
			if ( item->HasFlag( SInventoryItem::FLAG_STORE_QUEST_TAG ) )
			{
				AddItemTag( item->GetUniqueId(), CNAME( Quest ) );
			}
			else
			{
				RemoveItemTag( item->GetUniqueId(), CNAME( Quest ) );
			}

			if ( item->HasFlag( SInventoryItem::FLAG_STORE_NOSHOW_TAG ) )
			{
				AddItemTag( item->GetUniqueId(), CNAME( NoShow ) );
			}
			else
			{
				RemoveItemTag( item->GetUniqueId(), CNAME( NoShow ) );
			}
		}

		if ( item->GetCategory() == CNAME( upgrade ) || item->GetCategory() == CName::NONE )
		{
			CName itemName = item->GetName();
			SAddItemInfo addItemInfo;
			addItemInfo.m_quantity = item->GetQuantity();

			m_items.Resize( m_items.Size() - 1 );

			if ( itemName == CNAME( DEP_STRIBOG_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_STRIBOG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_STRIBOG_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_STRIBOG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_DAZHBOG_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_DAZHBOG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_DAZHBOG_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_DAZHBOG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_DEVANA_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_DEVANA_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_DEVANA_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_DEVANA_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_ZORIA_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_ZORIA_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_ZORIA_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_ZORIA_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_MORANA_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_MORANA_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_MORANA_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_MORANA_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_TRIGLAV_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_TRIGLAV_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_TRIGLAV_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_TRIGLAV_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_SVAROG_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_SVAROG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_SVAROG_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_SVAROG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_VELES_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_VELES_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_VELES_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_VELES_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_PERUN_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_PERUN_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_PERUN_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_PERUN_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_ELEMENTAL_RUNE ) )
			{
				AddItem( CNAME( ITEM_RUNE_ELEMENTAL_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_ELEMENTAL_RUNE_RARE ) )
			{
				AddItem( CNAME( ITEM_RUNE_ELEMENTAL_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_FORGOTTEN_SOUL ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AARD_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_FORGOTTEN_SOUL_RARE ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AARD_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_PHILOSOPHERS_STONE ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AXII_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_PHILOSOPHERS_STONE_RARE ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AXII_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_SHATTERED_CORE ) )
			{
				AddItem( CNAME( ITEM_GLYPH_QUEN_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_SHATTERED_CORE_RARE ) )
			{
				AddItem( CNAME( ITEM_GLYPH_QUEN_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_PHOSPHORESCENT_CRYSTAL ) )
			{
				AddItem( CNAME( ITEM_GLYPH_IGNI_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( DEP_PHOSPHORESCENT_CRYSTAL_RARE ) )
			{
				AddItem( CNAME( ITEM_GLPYH_IGNI_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_STRIBOG_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_STRIBOG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_STRIBOG ) )
			{
				AddItem( CNAME( ITEM_RUNE_STRIBOG ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_STRIBOG_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_STRIBOG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_DAZHBOG_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_DAZHBOG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_DAZHBOG ) )
			{
				AddItem( CNAME( ITEM_RUNE_DAZHBOG ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_DAZHBOG_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_DAZHBOG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_DEVANA_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_DEVANA_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_DEVANA ) )
			{
				AddItem( CNAME( ITEM_RUNE_DEVANA ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_DEVANA_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_DEVANA_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_ZORIA_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_ZORIA_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_ZORIA ) )
			{
				AddItem( CNAME( ITEM_RUNE_ZORIA ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_ZORIA_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_ZORIA_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_MORANA_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_MORANA_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_MORANA ) )
			{
				AddItem( CNAME( ITEM_RUNE_MORANA ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_MORANA_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_MORANA_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_TRIGLAV_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_TRIGLAV_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_TRIGLAV ) )
			{
				AddItem( CNAME( ITEM_RUNE_TRIGLAV ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_TRIGLAV_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_TRIGLAV_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_SVAROG_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_SVAROG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_SVAROG ) )
			{
				AddItem( CNAME( ITEM_RUNE_SVAROG ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_SVAROG_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_SVAROG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_VELES_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_VELES_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_VELES ) )
			{
				AddItem( CNAME( ITEM_RUNE_VELES ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_VELES_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_VELES_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_PERUN_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_PERUN_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_PERUN ) )
			{
				AddItem( CNAME( ITEM_RUNE_PERUN ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_PERUN_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_PERUN_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_ELEMENTAL_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_ELEMENTAL_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_ELEMENTAL ) )
			{
				AddItem( CNAME( ITEM_RUNE_ELEMENTAL ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_ELEMENTAL_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_ELEMENTAL_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_PIEROG_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_PIEROG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_PIEROG ) )
			{
				AddItem( CNAME( ITEM_RUNE_PIEROG ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_PIEROG_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_PIEROG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_TVAROG_LESSER ) )
			{
				AddItem( CNAME( ITEM_RUNE_TVAROG_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_TVAROG ) )
			{
				AddItem( CNAME( ITEM_RUNE_TVAROG ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_RUNE_TVAROG_GREATER ) )
			{
				AddItem( CNAME( ITEM_RUNE_TVAROG_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_AARD_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AARD_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_AARD ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AARD ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_AARD_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AARD_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_AXII_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AXII_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_AXII ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AXII ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_AXII_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_AXII_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_QUEN_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_QUEN_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_QUEN ) )
			{
				AddItem( CNAME( ITEM_GLYPH_QUEN ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_QUEN_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_QUEN_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_IGNI_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_IGNI_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_IGNI ) )
			{
				AddItem( CNAME( ITEM_GLPYH_IGNI ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_IGNI_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLPYH_IGNI_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_YRDEN_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_YRDEN_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_YRDEN ) )
			{
				AddItem( CNAME( ITEM_GLPYH_YRDEN ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_YRDEN_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLPYH_YRDEN_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_BINDING_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_BINDING_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_BINDING ) )
			{
				AddItem( CNAME( ITEM_GLPYH_BINDING ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_BINDING_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLPYH_BINDING_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_MENDING_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_MENDING_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_MENDING ) )
			{
				AddItem( CNAME( ITEM_GLPYH_MENDING ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_MENDING_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLPYH_MENDING_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_REINFORCEMENT_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_REINFORCEMENT_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_REINFORCEMENT ) )
			{
				AddItem( CNAME( ITEM_GLPYH_REINFORCEMENT ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_REINFORCEMENT_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLPYH_REINFORCEMENT_GREATER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLYPH_WARDING_LESSER ) )
			{
				AddItem( CNAME( ITEM_GLYPH_WARDING_LESSER ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_WARDING ) )
			{
				AddItem( CNAME( ITEM_GLPYH_WARDING ), addItemInfo );
			}
			else if ( itemName == CNAME( ITEM_GLPYH_WARDING_GREATER ) )
			{
				AddItem( CNAME( ITEM_GLPYH_WARDING_GREATER ), addItemInfo );
			}
			continue;
		}

		Uint32 slot;
		TDynArray< CName > slotItems;
		item->GetSlotItemsNames( slotItems );

		if ( slotItems.Size() > 0 )
		{
			for ( slot = 0; slot < slotItems.Size(); ++slot )
			{
				item->RemoveSlotItem( slotItems[ slot ] );
			}

			for ( slot = 0; slot < slotItems.Size(); ++slot )
			{
				if ( slotItems[ slot ] == CNAME( DEP_STRIBOG_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_STRIBOG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_STRIBOG_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_STRIBOG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_DAZHBOG_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DAZHBOG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_DAZHBOG_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DAZHBOG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_DEVANA_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DEVANA_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_DEVANA_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DEVANA_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_ZORIA_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ZORIA_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_ZORIA_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ZORIA_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_MORANA_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_MORANA_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_MORANA_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_MORANA_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_TRIGLAV_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TRIGLAV_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_TRIGLAV_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TRIGLAV_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_SVAROG_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_SVAROG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_SVAROG_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_SVAROG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_VELES_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_VELES_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_VELES_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_VELES_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_PERUN_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PERUN_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_PERUN_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PERUN_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_ELEMENTAL_RUNE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ELEMENTAL_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_ELEMENTAL_RUNE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ELEMENTAL_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_FORGOTTEN_SOUL ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AARD_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_FORGOTTEN_SOUL_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AARD_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_PHILOSOPHERS_STONE ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AXII_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_PHILOSOPHERS_STONE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AXII_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_SHATTERED_CORE ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_QUEN_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_SHATTERED_CORE_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_QUEN_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_PHOSPHORESCENT_CRYSTAL ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_IGNI_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( DEP_PHOSPHORESCENT_CRYSTAL_RARE ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_IGNI_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_STRIBOG_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_STRIBOG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_STRIBOG ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_STRIBOG ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_STRIBOG_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_STRIBOG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_DAZHBOG_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DAZHBOG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_DAZHBOG ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DAZHBOG ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_DAZHBOG_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DAZHBOG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_DEVANA_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DEVANA_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_DEVANA ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DEVANA ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_DEVANA_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_DEVANA_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_ZORIA_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ZORIA_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_ZORIA ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ZORIA ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_ZORIA_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ZORIA_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_MORANA_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_MORANA_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_MORANA ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_MORANA ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_MORANA_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_MORANA_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_TRIGLAV_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TRIGLAV_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_TRIGLAV ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TRIGLAV ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_TRIGLAV_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TRIGLAV_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_SVAROG_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_SVAROG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_SVAROG ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_SVAROG ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_SVAROG_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_SVAROG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_VELES_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_VELES_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_VELES ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_VELES ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_VELES_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_VELES_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_PERUN_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PERUN_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_PERUN ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PERUN ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_PERUN_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PERUN_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_ELEMENTAL_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ELEMENTAL_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_ELEMENTAL ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ELEMENTAL ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_ELEMENTAL_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_ELEMENTAL_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_PIEROG_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PIEROG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_PIEROG ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PIEROG ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_PIEROG_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_PIEROG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_TVAROG_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TVAROG_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_TVAROG ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TVAROG ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_RUNE_TVAROG_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_RUNE_TVAROG_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_AARD_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AARD_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_AARD ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AARD ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_AARD_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AARD_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_AXII_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AXII_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_AXII ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AXII ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_AXII_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_AXII_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_QUEN_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_QUEN_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_QUEN ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_QUEN ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_QUEN_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_QUEN_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_IGNI_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_IGNI_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_IGNI ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_IGNI ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_IGNI_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_IGNI_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_YRDEN_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_YRDEN_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_YRDEN ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_YRDEN ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_YRDEN_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_YRDEN_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_BINDING_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_BINDING_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}				 
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_BINDING ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_BINDING ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_BINDING_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_BINDING_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_MENDING_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_MENDING_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_MENDING ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_MENDING ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_MENDING_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_MENDING_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_REINFORCEMENT_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_REINFORCEMENT_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_REINFORCEMENT ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_REINFORCEMENT ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_REINFORCEMENT_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_REINFORCEMENT_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLYPH_WARDING_LESSER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLYPH_WARDING_LESSER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_WARDING ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_WARDING ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
				else if ( slotItems[ slot ] == CNAME( ITEM_GLPYH_WARDING_GREATER ) )
				{
					item->AddSlotItem( CNAME( ITEM_GLPYH_WARDING_GREATER ), IsPlayerOwner(), item->GetStaticRandomSeed() );
				}
			}
		}

		if ( loadSuccess == false )
		{
			if( item->IsMounted() )
			{					
				m_failedLoadItemsToMount.PushBack( item->GetCategory() );					
			}

			// Failed to restore state
			m_items.Resize( m_items.Size() - 1 );
			continue;
		}

		if ( !item->IsCloned() )
		{
			if ( gameplayEntity )
			{
				// Notify gameplay entity about item addition
				gameplayEntity->OnAddedItem( item->GetUniqueId() );
			}
		}
		else
		{
			// Failed to restore state
			m_items.Resize( m_items.Size() - 1 );
		}
	}

	m_loot.StreamLoad( loader, version );

	if ( isPlayerOwner )
	{
		// hack fix for weapons that were saved as mounted but are not actually equipped
		FixMountedUnequippedWeapons();
	}
}

void CInventoryComponent::StreamSave( ISaveFile* saver )
{
	m_uniqueIdGenerator.StreamSave( saver );
	
	ASSERT( m_items.Size() < 0x10000 );
	Uint16 count = static_cast< Uint16 > ( m_items.Size() );
	*saver << count;

	// Save items
	TDynArray< CName > tags;
	for ( Uint16 i = 0; i < count; ++i )
	{
		SInventoryItem& item = m_items[ i ];

		item.SetFlag( SInventoryItem::FLAG_STORE_QUEST_TAG, false );
		item.SetFlag( SInventoryItem::FLAG_STORE_NOSHOW_TAG, false );
		if ( GetItemTags( item.GetUniqueId(), tags ) )
		{
			if ( tags.Exist( CNAME( Quest ) ) )
			{
				item.SetFlag( SInventoryItem::FLAG_STORE_QUEST_TAG, true );
			}

			if ( tags.Exist( CNAME( NoShow ) ) )
			{
				item.SetFlag( SInventoryItem::FLAG_STORE_NOSHOW_TAG, true );
			}
		}

		item.StreamSave( saver );
	}

	m_loot.StreamSave( saver );
}

void CInventoryComponent::OnAppearanceChanged( Bool added )
{
	TBaseClass::OnAppearanceChanged( added );
	for( auto categoryId : m_failedLoadItemsToMount )
	{
		CName defaultItemName = GetCategoryDefaultItem( categoryId );
		SItemUniqueId defaultItemId = GetItemId( defaultItemName );
		SInventoryItem* defaultItem = GetItem( defaultItemId );
		if( defaultItem )
		{
			defaultItem->SetIsMounted( true );
		}			
	}

	m_failedLoadItemsToMount.Clear();
}

//! Is the iterator valid ?
Bool CItemIterator::IsValid() const
{
	return m_index >= 0 && m_index < (Int32)m_inventory->GetItemCount();
}

//! Advance to next component
void CItemIterator::Next()
{
	while ( ++m_index < (Int32)m_inventory->GetItemCount() )
	{
		const SInventoryItem* item = m_inventory->GetItem( (SItemUniqueId)m_index );
		if ( item && ( item->GetFlags() & m_flagFilter ) == m_flagFilter )
		{
			break;
		}
	}
}

// EOF
