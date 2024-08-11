/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "characterStats.h"
#include "definitionsManager.h"
#include "entityParams.h"
#include "../redMath/random/noise.h"
#include "../core/xmlReader.h"
#include "../core/xmlWriter.h"
#include "../core/dataError.h"
#include "../core/gameSave.h"
#include "../engine/gameSaveManager.h"
#include "../engine/utils.h"

RED_DEFINE_STATIC_NAME( vitality );
RED_DEFINE_STATIC_NAME( essence );

IMPLEMENT_ENGINE_CLASS( CCharacterStats );
IMPLEMENT_RTTI_ENUM( EBaseCharacterStats );
IMPLEMENT_RTTI_ENUM( ECharacterDefenseStats );

///////////////////////////////////////////////////////////////////////////////
// AbilitiesMultiSet
///////////////////////////////////////////////////////////////////////////////
void AbilitiesMultiSet::Add( CName ability )
{
	auto itFind = m_set.Find( ability );
	if ( itFind == m_set.End() )
	{
		m_set.Insert( ability, 1 );
	}
	else
	{
		++itFind->m_second;
	}
	m_list.PushBack( ability );
}

Bool AbilitiesMultiSet::Remove( CName ability )
{
	auto itFind = m_set.Find( ability );
	if ( itFind == m_set.End() )
	{
		return false;
	}
	if ( --itFind->m_second <= 0 )
	{
		m_set.Erase( itFind );
	}
	m_list.RemoveFast( ability );
	return true;
}

Bool AbilitiesMultiSet::Has( CName ability ) const
{
	return m_set.KeyExist( ability );
}
void AbilitiesMultiSet::Reserve( Uint32 size )
{
	m_set.Reserve( size );
	m_list.Reserve( size );
}
void AbilitiesMultiSet::Clear()
{
	m_set.ClearFast();
	m_list.ClearFast();
}
///////////////////////////////////////////////////////////////////////////////
// CCharacterStats::SEquipmentAbility
///////////////////////////////////////////////////////////////////////////////
CCharacterStats::SEquipmentAbility::SEquipmentAbility( SItemUniqueId itemId, const TDynArray< CName >& abilities, Uint16 staticRandomSeed )
	: m_itemId( itemId )
	, m_staticRandomSeed( staticRandomSeed )
{
	m_abilities.Reserve( abilities.Size() );
	for ( CName a : abilities )
	{
		m_abilities.Add( a );
	}
}

///////////////////////////////////////////////////////////////////////////////
// CCharacterStats
///////////////////////////////////////////////////////////////////////////////
CCharacterStats::CCharacterStats()
	: m_usedHealthType( BCS_Undefined )
	, m_abilityManager( nullptr )
{
	m_staticRandomSeed = GEngine->GetRandomNumberGenerator().Get< Uint16 >( 1, GEngine->GetRandomNumberGenerator().Max< Uint16 >() );
}

void CCharacterStats::AddAbilitiesFromParam( CCharacterStatsParam* param )
{
	for ( CName abilityName : param->GetAbilities() )
	{
		AddAbility( abilityName );
	}
}

Bool CCharacterStats::AddAbility( CName name, Bool allowMultiple, Bool runScriptCallback )
{
	PC_SCOPE_PIX( AddAbility );

	// prevent adding existing ability on game load
	if ( !allowMultiple && m_abilities.Has( name ) )
	{
		return false;
	}

	if ( m_abilityManager )
	{
		IAbilityManager::BlockedAbilityIdx idx = m_abilityManager->VFindBlockedAbility( name );
		if ( idx != IAbilityManager::INVALID_BLOCKED_ABILITY_IDX )
		{
			m_abilityManager->VIncreaseBlockedAbilityCount( idx );
			return false;
		}
	}

	// ABILITY_LOG( TXT("Adding ability %s for character %s"), name.AsString().AsChar(), GetParent()->GetFriendlyName().AsChar() );
	CGameplayEntity* gameplayEntity  = SafeCast< CGameplayEntity >( GetParent() );
	ASSERT( gameplayEntity );

	const SAbility* abilityDef = GCommonGame->GetDefinitionsManager()->GetAbilityDefinition( name );

	// in most cases this list will have zero or one element
	IScriptable* context = gameplayEntity;

	if( abilityDef )
	{
		m_abilities.Add( name );

		// Apply attribute modifiers from ability
		const TDynArray<SAbilityAttribute>& abilityAttributes = abilityDef->m_attributes;
		m_attributes.Reserve( m_attributes.Size() + abilityAttributes.Size() );
		for ( Uint32 i=0; i<abilityAttributes.Size(); ++i )
		{
			PC_SCOPE_PIX( ProcessAttribute );

			const SAbilityAttribute& attribute = abilityAttributes[i];
			const CName& attributeName = attribute.m_name;
			//if ( attribute.m_max != attribute.m_min  )
			//{
			//	ABILITY_ERR( TXT("Adding ability by name assumes non randomized modifiers !") );
			//	ABILITY_ERR( TXT("If this rule is not followed, it can, and most likely will cause 'attribute leaks' !") );
			//	ABILITY_ERR( TXT("The ability %s should be corrected by setting min and max attribute modifiers to the same value"), name.AsString().AsChar() );
			//}

			if ( ( attributeName == CNAME( vitality ) && UsesEssence() )
				|| ( attributeName == CNAME( essence ) && UsesVitality() ) )
			{
				continue;
			}
			
			Float noiseValue;
			SAttributeValue & attrData = m_attributes.GetRef( attributeName );
			if ( attribute.m_min < attribute.m_max )
			{
				Red::Math::Random::Generator< Red::Math::Random::Noise > noise;
				noise.Seed( m_staticRandomSeed );
				noiseValue = noise.Get< Float >( attribute.m_min, attribute.m_max );
			}
			else
			{
				noiseValue = attribute.m_min;
			}
			
			if( attribute.m_type == EAttrT_Multi )
			{
				attrData.m_base.m_valueMultiplicative += noiseValue;
			}
			else if( attribute.m_type == EAttrT_Additive )
			{
				attrData.m_base.m_valueAdditive += noiseValue;
			}
			else if( attribute.m_type == EAttrT_Base )
			{
				attrData.m_base.m_valueBase += noiseValue;
			}
		}

		{
			PC_SCOPE_PIX( NotifyScripts );
			if ( runScriptCallback )
			{
				gameplayEntity->CallEvent( CNAME( OnAbilityAdded ), name );
			}
		}
		return true;
	}
	else
	{
		ABILITY_WARN( TXT("CCharacterStats::AddAbility ability definition not found") );
		return false;
	}
}

Bool CCharacterStats::RemoveAbility( CName name, Bool runScriptCallback )
{
	PC_SCOPE_PIX( RemoveAbility );

	if ( !m_abilities.Remove( name ) )
	{
		ABILITY_WARN( TXT("RemoveAbility ability not found") );
		return false;
	}

	if ( m_abilityManager )
	{
		IAbilityManager::BlockedAbilityIdx idx = m_abilityManager->VFindBlockedAbility( name );
		if ( idx != IAbilityManager::INVALID_BLOCKED_ABILITY_IDX )
		{
			m_abilityManager->VDecreaseBlockedAbilityCount( idx );
			return false;
		}
	}

	CGameplayEntity* gameplayEntity = SafeCast< CGameplayEntity >( GetParent() );
	if ( gameplayEntity == nullptr )
	{
		return false;
	}

	const SAbility* abilityDef = GCommonGame->GetDefinitionsManager()->GetAbilityDefinition( name );
	if( abilityDef )
	{
		// Apply attribute modifiers from ability
		const TDynArray<SAbilityAttribute>& abilityAttributes = abilityDef->m_attributes;
		for ( Uint32 i=0; i<abilityAttributes.Size(); ++i )
		{
			const SAbilityAttribute& attribute = abilityAttributes[i];
			CName attributeName = attribute.m_name;
			//if ( attribute.m_max != attribute.m_min  )
			//{
			//	ABILITY_ERR( TXT("Remove ability by name assumes non randomized modifiers !") );
			//	ABILITY_ERR( TXT("If this rule is not followed, it can, and most likely will cause 'attribute leaks' !") );
			//	ABILITY_ERR( TXT("The ability %s should be corrected by setting min and max attribute modifiers to the same value"), name.AsString().AsChar() );
			//}
			SAttributeValue* attrData = m_attributes.FindPtr( attributeName );
			if ( attrData )
			{
				Float noiseValue = attribute.m_min;
				if ( attribute.m_min < attribute.m_max )
				{
					Red::Math::Random::Generator< Red::Math::Random::Noise > noise;
					noise.Seed( m_staticRandomSeed );
					noiseValue = noise.Get< Float >( attribute.m_min, attribute.m_max );
				}
				

				if( attribute.m_type == EAttrT_Multi )
				{
					attrData->m_base.m_valueMultiplicative -= noiseValue;
				}
				else if( attribute.m_type == EAttrT_Additive )
				{
					attrData->m_base.m_valueAdditive -= noiseValue;
				}
				else if( attribute.m_type == EAttrT_Base )
				{
					attrData->m_base.m_valueBase -= noiseValue;
				}
			}
		}

		{
			PC_SCOPE_PIX( NotifyScripts );

			if ( runScriptCallback )
			{
				gameplayEntity->CallEvent( CNAME( OnAbilityRemoved ), name );
			}
		}
		
		return true;
	}
	else
	{
		ABILITY_WARN( TXT("CCharacterStats::RemoveAbility ability definition not found") );
		return false;
	}
}

Bool CCharacterStats::ApplyItemAbilities( const SInventoryItem& item )
{
	ABILITY_LOG( TXT("Adding abilities of item %s for character %s"), item.GetName().AsChar(), GetParent()->GetFriendlyName().AsChar() );	

	TDynArray<CName> allAbilities;
	item.GetAllAbilities( allAbilities );

	ApplyItemAbilitiesInternal( allAbilities, item.GetStaticRandomSeed() );

	m_equipmentAbilities.PushBack( SEquipmentAbility( item.GetUniqueId(), allAbilities, item.GetStaticRandomSeed() ) );

	CallAbilityAddedEvents( allAbilities );
	return true;
}

Bool CCharacterStats::ApplyItemAbility( const SInventoryItem& item, CName ability )
{
	ABILITY_LOG( TXT("Adding ability %s of item %s for character %s"), ability.AsChar(), item.GetName().AsChar(), GetParent()->GetFriendlyName().AsChar() );	

	ApplyItemAbilityInternal( ability, item.GetStaticRandomSeed() );

	AddEquipmentAbility( item, ability );
	
	CallAbilityAddedEvent( ability );

	return true;
}

void CCharacterStats::CallAbilityAddedEvents( const TDynArray< CName >& abilities )
{
	for ( Uint32 i = 0; i<abilities.Size(); ++i )
	{
		CallAbilityAddedEvent( abilities[ i ] );
	}
}

void CCharacterStats::CallAbilityAddedEvent( CName ability )
{
	CGameplayEntity* gameplayEntity = SafeCast< CGameplayEntity >( GetParent() );
	if ( gameplayEntity != nullptr )
	{
		gameplayEntity->CallEvent( CNAME(OnAbilityAdded), ability );
	}
}

void CCharacterStats::ApplyItemAbilitiesInternal( const TDynArray< CName >& abilities, Uint16 staticRandomSeed )
{
	PC_SCOPE_PIX( ApplyItemAbilitiesInternal );

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	THashMap< CName, SAbilityAttributeValue > modifiers;

	defMgr->CalculateAttributeModifiers( modifiers, abilities, staticRandomSeed );

	ApplyAttributeModifiers( modifiers, staticRandomSeed );
}

void CCharacterStats::ApplyItemAbilityInternal( CName ability, Uint16 staticRandomSeed )
{
	PC_SCOPE_PIX( ApplyItemAbilityInternal );

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	THashMap< CName, SAbilityAttributeValue > modifiers;

	defMgr->CalculateAttributeModifiers( modifiers, ability, staticRandomSeed );

	ApplyAttributeModifiers( modifiers, staticRandomSeed );
}

void CCharacterStats::ApplyAttributeModifiers( const THashMap< CName, SAbilityAttributeValue >& modifiers, Uint16 staticRandomSeed )
{
	for ( THashMap<CName,SAbilityAttributeValue>::const_iterator it = modifiers.Begin(); it != modifiers.End(); ++it )
	{
		ABILITY_LOG( TXT("Adding attribute modifier %s with value: b %f, a %f, m %f"), it->m_first.AsString().AsChar(), it->m_second.m_valueBase, it->m_second.m_valueAdditive, it->m_second.m_valueMultiplicative );
		SAttributeValue* attribute = m_attributes.FindPtr( it->m_first );
		if ( !attribute )
		{
			SAttributeValue newAttributeValue;
			m_attributes.Insert( it->m_first, newAttributeValue );
			attribute = m_attributes.FindPtr( it->m_first );
		}
		if ( attribute )
		{
			attribute->m_items += it->m_second;
		}
	}
}

Bool CCharacterStats::ApplyItemSlotAbilities( const SInventoryItem& item, Int32 slotIndex )
{
	const TDynArray< SInventoryItem::SSlotItem >& slotItems = item.GetSlotItems();

	if ( slotIndex < 0 || slotIndex >= static_cast< Int32 >( slotItems.Size() ) )
	{
		return false;
	}

	const SInventoryItem::SSlotItem& slotItem = slotItems[ slotIndex ];
	ApplyItemAbilitiesInternal( slotItem.m_abilities, slotItem.m_randSeed );

	RemoveEquipmentAbility( item );
	
	TDynArray<CName> allAbilities;
	item.GetAllAbilities( allAbilities );
	m_equipmentAbilities.PushBack( SEquipmentAbility( item.GetUniqueId(), allAbilities, item.GetStaticRandomSeed() ) );

	CallAbilityAddedEvents( slotItem.m_abilities );

	return true;
}

void CCharacterStats::AddEquipmentAbility( const SInventoryItem& item, CName ability )
{
	for ( auto& equpmentAbility : m_equipmentAbilities )
	{
		if ( equpmentAbility.m_itemId == item.GetUniqueId() )
		{
			equpmentAbility.m_abilities.Add( ability );
			return;
		}
	}

	m_equipmentAbilities.PushBack( SEquipmentAbility( item.GetUniqueId(), item.GetStaticRandomSeed() ) );
	m_equipmentAbilities.Back().m_abilities.Add( ability );
}

Bool CCharacterStats::RemoveEquipmentAbility( const SInventoryItem& item )
{
	for ( Uint32 i=0; i<m_equipmentAbilities.Size(); ++i )
	{
		if ( m_equipmentAbilities[i].m_itemId == item.GetUniqueId() )
		{
			m_equipmentAbilities.RemoveAt( i );
			return true;
		}
	}
	return false;
}

Bool CCharacterStats::RemoveEquipmentAbility( const SInventoryItem& item, CName ability )
{
	for ( Uint32 i=0; i<m_equipmentAbilities.Size(); ++i )
	{
		if ( m_equipmentAbilities[i].m_itemId == item.GetUniqueId() )
		{
			return  m_equipmentAbilities[ i ].m_abilities.Remove( ability );
		}
	}
	return false;
}

Bool CCharacterStats::RemoveItemAbilities( const SInventoryItem& item )
{
	ABILITY_LOG( TXT("Removing abilities of item %s for character %s"), item.GetName().AsChar(), GetParent()->GetFriendlyName().AsChar() );

	TDynArray<CName> allAbilities;
	item.GetAllAbilities( allAbilities );
	
	if ( RemoveEquipmentAbility( item ) )
	{
		RemoveItemAbilitiesInternal( allAbilities, item.GetStaticRandomSeed() );
		return true;
	}
	return false;
}

Bool CCharacterStats::RemoveItemAbility( const SInventoryItem& item, CName ability )
{
	ABILITY_LOG( TXT("Removing %s abilities of item %s for character %s"), ability.AsChar(), item.GetName().AsChar(), GetParent()->GetFriendlyName().AsChar() );

	if ( RemoveEquipmentAbility( item, ability ) )
	{
		RemoveItemAbilityInternal( ability, item.GetStaticRandomSeed() );
		return true;
	}
	return false;
}

void CCharacterStats::RemoveItemAbilitiesInternal( const TDynArray< CName >& abilities, Uint16 staticRandomSeed )
{
	PC_SCOPE_PIX( RemoveItemAbilitiesInternal );

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT( defMgr );

	THashMap<CName, SAbilityAttributeValue> modifiers;

	defMgr->CalculateAttributeModifiers( modifiers, abilities, staticRandomSeed );

	RemoveAttributeModifiers( modifiers, staticRandomSeed );

	CallAbilityRemovedEvents( abilities );
}


void CCharacterStats::RemoveItemAbilityInternal( CName ability, Uint16 staticRandomSeed )
{
	PC_SCOPE_PIX( RemoveItemAbilityInternal );

	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT( defMgr );

	THashMap<CName, SAbilityAttributeValue> modifiers;

	defMgr->CalculateAttributeModifiers( modifiers, ability, staticRandomSeed );

	RemoveAttributeModifiers( modifiers, staticRandomSeed );

	CallAbilityRemovedEvent( ability );
}

void CCharacterStats::RemoveAttributeModifiers( const THashMap< CName, SAbilityAttributeValue >& modifiers, Uint16 staticRandomSeed )
{
	for ( THashMap<CName,SAbilityAttributeValue>::const_iterator it = modifiers.Begin(); it != modifiers.End(); ++it )
	{
		ABILITY_LOG( TXT("Removing attribute modifier %s with value %f"), it->m_first.AsString().AsChar(), it->m_second );
		SAttributeValue* attribute = m_attributes.FindPtr( it->m_first );
		if ( attribute )
		{
			attribute->m_items -= it->m_second;
		}
	}
}

void CCharacterStats::CallAbilityRemovedEvents( const TDynArray< CName >& abilities )
{
	for ( Uint32 i = 0; i<abilities.Size(); ++i )
	{
		CallAbilityRemovedEvent( abilities[ i ] );
	}
}

void CCharacterStats::CallAbilityRemovedEvent( CName ability )
{
	CGameplayEntity* gameplayEntity = SafeCast< CGameplayEntity >( GetParent() );
	if ( gameplayEntity != nullptr )
	{
		gameplayEntity->CallEvent( CNAME(OnAbilityRemoved), ability );
	}
}
Bool CCharacterStats::HasAbility( CName name, Bool includeEquipment /* = false */ ) const
{ 
	if ( m_abilities.Has( name ) )
	{
		return true;
	}
	if ( includeEquipment )
	{
		for ( const auto& it : m_equipmentAbilities )
		{
			if ( it.m_abilities.Has( name ) )
			{
				return true;
			}
		}
	}
	return false;
}

Bool CCharacterStats::HasAbilityWithTag( CName tag, Bool includeEquipment /* = false */ ) const
{
	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
	if ( dm == nullptr )
	{
		return false;
	}
	for ( const auto& abilityPair : m_abilities.m_set )
	{
		const SAbility* ability = dm->GetAbilityDefinition( abilityPair.m_first );
		if ( ability->m_tags.HasTag( tag ) )
		{
			return true;
		}
	}
	if ( includeEquipment )
	{
		for ( const SEquipmentAbility& it : m_equipmentAbilities )
		{
			for ( const auto& abilityPair : it.m_abilities.m_set )
			{
				const SAbility* ability = dm->GetAbilityDefinition( abilityPair.m_first );
				if ( ability->m_tags.HasTag( tag ) )
				{
					return true;
				}
			}
		}
	}
	return false;
}

Bool CCharacterStats::RemoveItemSlotAbilities( const SInventoryItem& item, Int32 slotIndex )
{
	const TDynArray< SInventoryItem::SSlotItem >& slotItems = item.GetSlotItems();

	if ( slotIndex < 0 || slotIndex >= static_cast< Int32 >( slotItems.Size() ) )
	{
		return false;
	}

	if ( RemoveEquipmentAbility( item ) )
	{
		TDynArray<CName> allAbilities;

		item.GetAllAbilities( allAbilities, slotIndex );
		m_equipmentAbilities.PushBack( SEquipmentAbility( item.GetUniqueId(), allAbilities, item.GetStaticRandomSeed() ) );

		const SInventoryItem::SSlotItem& slotItem = slotItems[ slotIndex ];
		RemoveItemAbilitiesInternal( slotItem.m_abilities, slotItem.m_randSeed );
	}

	return true;
}

void CCharacterStats::FilterAbilitiesByPrerequisites( TDynArray< CName > & abilities ) const
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	for ( Int32 i = (Int32)abilities.Size()-1 ; i >= 0; --i )
	{
		const SAbility* abilityDef = defMgr->GetAbilityDefinition( abilities[i] );
		if ( abilityDef )
		{
			const TDynArray< CName >& prerequisites = abilityDef->m_prerequisites;
			for ( Uint32 j=0; j<prerequisites.Size(); ++j )
			{
				if ( !HasAbility( prerequisites[j] ) )
				{
					ABILITY_LOG( TXT("Item application: Ability %s addition is skipped due to missing prerequisite ability %s"), abilityDef->m_name.AsString().AsChar(), prerequisites[j].AsString().AsChar() );
					abilities.RemoveAtFast( i );
					break;
				}
			}
		}
	}
}

void CCharacterStats::ReapplyAbilities()
{
	TDynArray< CName > tempAbilities;
	tempAbilities = m_abilities.m_list;

	m_abilities.m_list.ClearFast();
	m_abilities.m_set.ClearFast();

	for ( THashMap<CName, SAttributeValue>::iterator it = m_attributes.Begin(); it!=m_attributes.End(); ++it )
	{
		it->m_second = SAttributeValue();
	}
	for ( Uint32 i=0; i<tempAbilities.Size(); ++i )
	{
		AddAbility( tempAbilities[i], true );
	}
	for ( Uint32 i=0; i<m_equipmentAbilities.Size(); ++i )
	{
		ApplyItemAbilitiesInternal( m_equipmentAbilities[i].m_abilities.m_list, m_equipmentAbilities[i].m_staticRandomSeed );
	}
}

void CCharacterStats::GetAllAttributesNames( TDynArray<CName> & outNames ) const
{
	for( auto iter = m_attributes.Begin(), end = m_attributes.End(); iter!= end; ++iter )
	{
		outNames.PushBack(iter->m_first);
	}
}

void CCharacterStats::GetAttributeValue( SAbilityAttributeValue& outValue, CName attributeName, const TDynArray< CName > & abilityTags, Bool withoutTags /* = false */ ) const
{
	TagList tags( abilityTags );
	SAttributeValue finalAttribute = CalculateAttributeValue( attributeName, abilityTags, withoutTags );
	outValue = finalAttribute.m_base + finalAttribute.m_items;
}

void CCharacterStats::GetAbilityAttributeValue( SAbilityAttributeValue& outValue, CName attributeName, CName abilityName ) const
{
	TDynArray< CName > abilities;				
	TDynArray< SEquipmentAbility > equipmentAbilities;	

	for ( Uint32 i = 0; i < m_equipmentAbilities.Size(); ++i )
	{
		if ( m_equipmentAbilities[ i ].m_abilities.Has( abilityName ) )
		{ 
			equipmentAbilities.PushBack( m_equipmentAbilities[ i ] );
		}			
	}
	if ( m_abilities.Has( abilityName ) )
	{ 
		abilities.PushBack( abilityName );
	}	
	SAttributeValue finalAttribute = CalculateAttributeValueInternal( attributeName, abilities, equipmentAbilities );
	outValue = finalAttribute.m_base + finalAttribute.m_items;
}

CCharacterStats::SAttributeValue CCharacterStats::CalculateAttributeValue( const CName& attributeName, const TagList& abilityTags, Bool withoutTags ) const
{
	const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT( defMgr );

	if( abilityTags.Empty() )
	{
		return CalculateAttributeValueInternal( attributeName, m_abilities.m_list, m_equipmentAbilities );
	}
	else
	{
		TDynArray< CName >						abilities;				
		TDynArray< SEquipmentAbility >			equipmentAbilities;	
		for ( Uint32 i = 0; i < m_equipmentAbilities.Size(); ++i )
		{
			abilities.ClearFast();
			if ( defMgr->FilterAbilitiesByTags( abilities, m_equipmentAbilities[ i ].m_abilities.m_list, abilityTags, withoutTags ) )
			{
				equipmentAbilities.PushBack( SEquipmentAbility( m_equipmentAbilities[ i ].m_itemId ,abilities, m_equipmentAbilities[ i ].m_staticRandomSeed ) );
			}
		}
		defMgr->FilterAbilitiesByTags( abilities, m_abilities.m_list, abilityTags, withoutTags );
		return CalculateAttributeValueInternal( attributeName, abilities, equipmentAbilities );
	}
}

CCharacterStats::SAttributeValue CCharacterStats::CalculateAttributeValueInternal(	const CName& attributeName,
																					const  TDynArray< CName >& abilities,
																					const TDynArray< SEquipmentAbility >& equipmentAbilities ) const
{
	SAttributeValue finalAttribute;
	const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();

	ASSERT( defMgr );
	for ( Uint32 k = 0; k < abilities.Size(); ++k )
	{
		const SAbility* abilityDef = defMgr->GetAbilityDefinition( abilities[ k ] );
		if ( !abilityDef )
		{
			DATA_HALT( DES_Minor, CResourceObtainer::GetResource( GetParent() ), TXT( "Definitions" ), TXT( "Cannot find ability definition: %s"), abilities[ k ].AsChar() );
			continue;
		}
		const TDynArray< SAbilityAttribute >& abilityAttributes = abilityDef->m_attributes;
		for ( Uint32 i = 0; i < abilityAttributes.Size(); ++i )
		{
			const SAbilityAttribute& attribute = abilityAttributes[ i ];
			if ( attribute.m_name != attributeName )
			{
				continue;
			}
			Float noiseValue;
			if ( attribute.m_min < attribute.m_max )
			{
				Red::Math::Random::Generator< Red::Math::Random::Noise > noise;
				noise.Seed( m_staticRandomSeed );
				noiseValue = noise.Get< Float >( attribute.m_min, attribute.m_max );
			}
			else
			{
				noiseValue = attribute.m_min;
			}
			
			if( attribute.m_type == EAttrT_Multi )
			{
				finalAttribute.m_base.m_valueMultiplicative += noiseValue;
			}
			else if( attribute.m_type == EAttrT_Additive )
			{
				finalAttribute.m_base.m_valueAdditive += noiseValue;
			}
			else if( attribute.m_type == EAttrT_Base )
			{
				finalAttribute.m_base.m_valueBase += noiseValue;
			}
		}
	}

	for( auto i = equipmentAbilities.Begin() ; i != equipmentAbilities.End() ; ++i )
	{
		m_modifiers.ClearFast();

		defMgr->CalculateAttributeModifiers( m_modifiers, i->m_abilities.m_list, i->m_staticRandomSeed );	
		for ( THashMap<CName,SAbilityAttributeValue>::iterator it = m_modifiers.Begin(); it != m_modifiers.End(); ++it )
		{
			if ( it->m_first != attributeName )
			{
				continue;
			}

			finalAttribute.m_items	+= it->m_second;
			//finalAttribute.m_items.m_valueBase	+= it->m_second.m_valueBase*it->m_second.m_valueMultiplicative + it->m_second.m_valueAdditive ;
		}
	}
	return finalAttribute;
} 

void CCharacterStats::LogAbilities( const TDynArray< CName >& abilities )
{
	for ( Uint32 i=0; i<abilities.Size(); ++i )
	{
		ABILITY_LOG( TXT("-- %s"), abilities[i].AsString().AsChar() );
	}
}

// ----------------------------------------------------------------------------
// Game save
// ----------------------------------------------------------------------------

void CCharacterStats::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME( characterStats ) );

	const Uint32 count = m_abilities.m_list.Size();
	saver->WriteValue( CNAME( count ), count );
	if ( count > 0 )
	{
		saver->WriteValue( CNAME( value ), m_abilities.m_list );
	}
	saver->WriteValue( CNAME( staticRandomSeed ), m_staticRandomSeed );
}

void CCharacterStats::RestoreState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( characterStats ) );

	// Clear
	m_abilities.Clear();

	// Load count
	Uint32 count = 0;
	loader->ReadValue( CNAME( count ), count );

	if ( loader->GetSaveVersion() < SAVE_VERSION_USE_HASH_NAMES_WHERE_SAFE )
	{
		for ( Uint32 i=0; i<count; i++ )
		{
			CGameSaverBlock block2( loader, CNAME(ability) );

			// Save ability name
			CName abilityName = loader->ReadValue< CName >( CNAME(name), CName::NONE );
			m_abilities.m_list.PushBack( abilityName );
		}
	}
	else if ( loader->GetSaveVersion() < SAVE_VERSION_OPTIMIZED_CHARACTERSTATS_SAVING )
	{
		Uint32 hash;
		CName ability;
		m_abilities.m_list.Reserve( count );
		for ( Uint32 i = 0; i < count; ++i )
		{
			hash = loader->ReadValue< Uint32 > ( CNAME( name ), 0 );
			ability = CName::CreateFromHash( Red::CNameHash( hash ) );
			if ( ability )
			{
				m_abilities.m_list.PushBack( ability );
			}
		}
	}
	else if ( count > 0 )
	{
		m_abilities.m_list.Resize( count );
		loader->ReadValue( CNAME( value ), m_abilities.m_list );
	}

	loader->ReadValue( CNAME( staticRandomSeed ), m_staticRandomSeed );

	ReapplyAbilities();
}

void CCharacterStats::Export( CXMLWriter& writer )
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();

	writer.BeginNode( TXT("CharacterStats") );

	{
		const AbilitiesMultiSet::List& abilitiesList = m_abilities.m_list;

		// Abilities
		writer.BeginNode( TXT("Abilities") );

		// Save ability count
		const Uint32 count = abilitiesList.Size();
		writer.AttributeT( TXT("count"), count );

		// Save abilities
		for ( Uint32 i=0; i<count; i++ )
		{
			writer.BeginNode( TXT("Ability") );
			writer.SetAttribute( TXT("name"), abilitiesList[i].AsString() );

			{
				const SAbility* ab = defMgr->GetAbilityDefinition( abilitiesList[i] );
				if( ab )
				{
					writer.BeginNode( TXT("Attributes") );
					for( Uint32 a=0; a<ab->m_attributes.Size(); a++ )
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
			}

			writer.EndNode(); // Ability
		}

		writer.EndNode(); // Abilities
	}


	{
		writer.BeginNode( TXT("EquipmentAbilities") );

		for( Uint32 i=0; i<m_equipmentAbilities.Size(); i++ )
		{
			const AbilitiesMultiSet::List& abilitiesList = m_equipmentAbilities[i].m_abilities.m_list;

			for ( Uint32 j=0; j<abilitiesList.Size(); ++j )
			{
				writer.BeginNode( TXT("Ability") );
				writer.SetAttribute( TXT("name"), abilitiesList[j].AsString() );
				writer.EndNode();
			}
		}

		writer.EndNode(); // EquipmentAbilities
	}

	{
		writer.BeginNode( TXT("Attributes") );		

		for( THashMap< CName, SAttributeValue >::iterator iter = m_attributes.Begin(); iter != m_attributes.End(); ++iter )
		{
			writer.BeginNode( TXT("Attrib" ) );			
			writer.SetAttribute( TXT("name"), iter->m_first.AsString() );
			
			writer.AttributeT( TXT("baseAdd"), iter->m_second.m_base.m_valueAdditive );
			writer.AttributeT( TXT("baseMul"), iter->m_second.m_base.m_valueMultiplicative );
			writer.AttributeT( TXT("baseBase"), iter->m_second.m_base.m_valueBase );

			writer.AttributeT( TXT("itemsAdd"), iter->m_second.m_items.m_valueAdditive );
			writer.AttributeT( TXT("itemsMul"), iter->m_second.m_items.m_valueMultiplicative );
			writer.AttributeT( TXT("itemBase"), iter->m_second.m_base.m_valueBase );
			writer.EndNode(); // Attrib
		}

		writer.EndNode(); // Attributes
	}

	writer.EndNode(); // CharacterStats
}

// ------------- Scripting support --------------

void CCharacterStats::funcAddAbility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER_OPT( Bool , allowMultiple, false );
	FINISH_PARAMETERS;

	RETURN_BOOL( AddAbility( abilityName, allowMultiple ) );
}

void CCharacterStats::funcRemoveAbility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( RemoveAbility( abilityName ) );
}

void CCharacterStats::funcHasAbility( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER_OPT( Bool, includeEquipment, false );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasAbility( abilityName, includeEquipment ) );
}

void CCharacterStats::funcHasAbilityWithTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Bool, includeEquipment, false );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasAbilityWithTag( tag, includeEquipment ) );
}

void CCharacterStats::funcIsAbilityAvailableToBuy( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	const SAbility * ability = GCommonGame->GetDefinitionsManager()->GetAbilityDefinition( abilityName );
	if ( ability )
	{
		// No prerequisites
		if ( ability->m_prerequisites.Empty() )
		{
			RETURN_BOOL( true );
			return;
		}

		for ( auto i = ability->m_prerequisites.Begin(); i != ability->m_prerequisites.End(); ++i )
		{
			if ( HasAbility( *i ) )
			{
				RETURN_BOOL( true );
				return;
			}
		}
	}
	RETURN_BOOL( false );
}

void CCharacterStats::funcGetAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, outAbilities, TDynArray< CName >() );
	GET_PARAMETER_OPT( Bool, includeEquipment, false );
	FINISH_PARAMETERS;

	outAbilities = m_abilities.m_list;
	if ( includeEquipment )
	{
		for ( auto it : m_equipmentAbilities )
		{
			outAbilities.PushBack( it.m_abilities.m_list );
		}
	}
}

void CCharacterStats::funcGetAllAttributesNames( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, outAbttrib, TDynArray< CName >() );
	FINISH_PARAMETERS;

	GetAllAttributesNames( outAbttrib );
}

void CCharacterStats::funcGetAttributeValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER( TDynArray< CName >, abilityTags, TDynArray< CName >() );
	GET_PARAMETER_OPT( Bool, withoutTags, false );
	FINISH_PARAMETERS;

	SAbilityAttributeValue attrValue;
	GetAttributeValue( attrValue, attributeName, abilityTags, withoutTags );
	RETURN_STRUCT( SAbilityAttributeValue, attrValue );
}

void CCharacterStats::funcGetAbilityAttributeValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	SAbilityAttributeValue attrValue;
	GetAbilityAttributeValue( attrValue, attributeName, abilityName );			
	RETURN_STRUCT( SAbilityAttributeValue, attrValue );
}

void CCharacterStats::funcGetAllContainedAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, outAbttrib, TDynArray< CName >() );
	FINISH_PARAMETERS;

	outAbttrib.ClearFast();
	// iterate through unique abilities
	for( auto it = m_abilities.m_set.Begin(), end = m_abilities.m_set.End(); it != end; ++it )
	{
		const SAbility* abilityDef = GCommonGame->GetDefinitionsManager()->GetAbilityDefinition( it->m_first );
		if( abilityDef )
		{
			for( auto it2 = abilityDef->m_abilities.Begin(), end2 = abilityDef->m_abilities.End(); it2 != end2; ++it2 )
			{
				outAbttrib.PushBackUnique( *it2 );
			}
		}
	}
}

void CCharacterStats::funcAddAbilityMultiple( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER( Int32, count, 0 );
	FINISH_PARAMETERS;

	while(count > 0)
	{
		--count;
		AddAbility( abilityName, true, false );
	}
	CallAbilityAddedEvent( abilityName );
}

void CCharacterStats::funcRemoveAbilityMultiple( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	GET_PARAMETER( Int32, count, 0 );
	FINISH_PARAMETERS;
	Bool didRemove = false;
	while( count > 0 )
	{
		--count;
		if ( !RemoveAbility( abilityName, false ) )
		{
			break;
		}
		didRemove = true;
	}
	if ( didRemove )
	{
		CallAbilityRemovedEvent( abilityName );
	}
}

void CCharacterStats::funcRemoveAbilityAll( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;
	Bool didRemove = false;
	while ( RemoveAbility( abilityName, false ) )
	{
		didRemove = true;
	}
	if ( didRemove )
	{
		CallAbilityRemovedEvent( abilityName );
	}
}

void CCharacterStats::funcGetAbilityCount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	Int32 count = 0;
	// main ability list
	{
		auto itFind = m_abilities.m_set.Find( abilityName );
		if ( itFind != m_abilities.m_set.End() )
		{
			count += itFind->m_second;
		}
	}
	// iterate equipment
	for ( auto it : m_equipmentAbilities )
	{
		auto itFind = it.m_abilities.m_set.Find( abilityName );
		if ( itFind != it.m_abilities.m_set.End() )
		{
			count += itFind->m_second;
		}
	}
	RETURN_INT( count );
}

void CCharacterStats::funcGetAbilitiesWithTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_REF( TDynArray< CName >, outAbilities, TDynArray< CName >() );
	FINISH_PARAMETERS;

	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
	if ( dm )
	{
		// iterate main abilities
		for ( const auto& abilityPair : m_abilities.m_set )
		{
			CName abilityName = abilityPair.m_first;
			if ( dm->AbilityHasTag( abilityName, tag ) )
			{
				for ( Int32 i = 0; i < abilityPair.m_second; ++i )
				{
					outAbilities.PushBack( abilityName );
				}
			}
		}
		// iterate equipment
		for ( const auto& it : m_equipmentAbilities )
		{
			for ( const auto& abilityPair : it.m_abilities.m_set )
			{
				CName abilityName = abilityPair.m_first;
				if ( dm->AbilityHasTag( abilityName, tag ) )
				{
					for ( Int32 i = 0; i < abilityPair.m_second; ++i )
					{
						outAbilities.PushBack( abilityName );
					}
				}
			}
		}
	}
}
