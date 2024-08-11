/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_LOG

#define ABILITY_LOG( format, ... )	RED_LOG( Ability, format, ## __VA_ARGS__ )
#define ABILITY_WARN( format, ... )	RED_LOG( Ability, format, ## __VA_ARGS__ )
#define ABILITY_ERR( format, ... )	RED_LOG( Ability, format, ## __VA_ARGS__ )

#else

#define ABILITY_LOG( format, ... )	
#define ABILITY_WARN( format, ... )	
#define ABILITY_ERR( format, ... )	

#endif

struct SInventoryItem;
class  CCharacterStatsParam;

#include "abilities.h"
#include "itemUniqueId.h"

//////////////////////////////////////////////////////////////////////////

enum EBaseCharacterStats
{
	BCS_Vitality,
	BCS_Essence,
	BCS_Stamina,
	BCS_Toxicity,
	BCS_Focus,
	BCS_Morale,
	BCS_Air,
	BCS_Panic,			// default panic val is now 100
	BCS_PanicStatic,	// Used when reducing BCS_Panic. BCS_Panic can't go below BCS_PanicStatic value
	BCS_SwimmingStamina,
	BCS_Undefined,
	BCS_Total,	// has to be at the end
};

BEGIN_ENUM_RTTI( EBaseCharacterStats );
	ENUM_OPTION( BCS_Vitality );
	ENUM_OPTION( BCS_Essence );
	ENUM_OPTION( BCS_Stamina );
	ENUM_OPTION( BCS_Toxicity );
	ENUM_OPTION( BCS_Focus );
	ENUM_OPTION( BCS_Morale );
	ENUM_OPTION( BCS_Air );
	ENUM_OPTION( BCS_Panic );			// default panic val is now 100
	ENUM_OPTION( BCS_PanicStatic );		// Used when reducing BCS_Panic. BCS_Panic can't go below BCS_PanicStatic value
	ENUM_OPTION( BCS_SwimmingStamina );
	ENUM_OPTION( BCS_Undefined );
END_ENUM_RTTI();


//////////////////////////////////////////////////////////////////////////
// character resistances

enum ECharacterDefenseStats
{
	CDS_None,
	CDS_PhysicalRes,
	CDS_BleedingRes,
	CDS_PoisonRes,
	CDS_FireRes,
	CDS_FrostRes,
	CDS_ShockRes,
	CDS_ForceRes,
	CDS_FreezeRes,	// #B deprecated
	CDS_WillRes,
	CDS_BurningRes,
	CDS_SlashingRes,
	CDS_PiercingRes,
	CDS_BludgeoningRes,
	CDS_RendingRes,
	CDS_ElementalRes,
	CDS_DoTBurningDamageRes,
	CDS_DoTPoisonDamageRes,
	CDS_DoTBleedingDamageRes,
	CDS_Total,	// has to be at the end
};

BEGIN_ENUM_RTTI( ECharacterDefenseStats );
	ENUM_OPTION( CDS_None );
	ENUM_OPTION( CDS_PhysicalRes );
	ENUM_OPTION( CDS_BleedingRes );
	ENUM_OPTION( CDS_PoisonRes );
	ENUM_OPTION( CDS_FireRes );
	ENUM_OPTION( CDS_FrostRes );
	ENUM_OPTION( CDS_ShockRes );
	ENUM_OPTION( CDS_ForceRes );
	ENUM_OPTION( CDS_FreezeRes );	// #B deprecated
	ENUM_OPTION( CDS_WillRes );
	ENUM_OPTION( CDS_BurningRes );
	ENUM_OPTION( CDS_SlashingRes );
	ENUM_OPTION( CDS_PiercingRes );
	ENUM_OPTION( CDS_BludgeoningRes );
	ENUM_OPTION( CDS_RendingRes );
	ENUM_OPTION( CDS_ElementalRes );
	ENUM_OPTION( CDS_DoTBurningDamageRes );
	ENUM_OPTION( CDS_DoTPoisonDamageRes );
	ENUM_OPTION( CDS_DoTBleedingDamageRes );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

struct AbilitiesMultiSet
{
	typedef THashMap< CName, Int32 > MultiSet;
	typedef TDynArray< CName > List;

	MultiSet										m_set;
	List											m_list;

	void											Add( CName ability );
	Bool											Remove( CName ability );
	Bool											Has( CName ability ) const;
	void											Reserve( Uint32 size );
	void											Clear();
};


// An interface for ability manager
class IAbilityManager
{
public:
	typedef Int32 BlockedAbilityIdx;
	static const BlockedAbilityIdx INVALID_BLOCKED_ABILITY_IDX = -1; 

	virtual ~IAbilityManager() {}

	virtual BlockedAbilityIdx VFindBlockedAbility( CName abilityName ) const = 0;
	virtual void VIncreaseBlockedAbilityCount( BlockedAbilityIdx idx ) = 0;
	virtual void VDecreaseBlockedAbilityCount( BlockedAbilityIdx idx ) = 0;
};

class CCharacterStats : public CObject
{
	DECLARE_ENGINE_CLASS( CCharacterStats, CObject, 0 );

protected:
	struct SAttributeValue
	{
		SAbilityAttributeValue	m_base;
		SAbilityAttributeValue	m_items;
	};

	struct SEquipmentAbility
	{
		SEquipmentAbility( SItemUniqueId itemId, const TDynArray< CName >& abilities, Uint16 staticRandomSeed );
		SEquipmentAbility( SItemUniqueId itemId, Uint16 staticRandomSeed )
			: m_itemId( itemId ), m_staticRandomSeed( staticRandomSeed )	{}
			

		SItemUniqueId			m_itemId;
		AbilitiesMultiSet		m_abilities;
		Uint16					m_staticRandomSeed;
	};

	
	AbilitiesMultiSet						m_abilities;					//!< current character abilities 
	TDynArray< SEquipmentAbility >			m_equipmentAbilities;			//!< Current abilities coming from equipment
	THashMap< CName, SAttributeValue >		m_attributes;					//!< character attributes
	mutable THashMap< CName, SAbilityAttributeValue > m_modifiers;
	EBaseCharacterStats						m_usedHealthType;				//!< Thats about to be set from W3AbilityManager. 
	Uint16									m_staticRandomSeed;
	IAbilityManager*						m_abilityManager;
public:
	CCharacterStats();

	void SetAbilityManager( IAbilityManager* owner ) { m_abilityManager = owner; }

	Bool UsesVitality() const { return m_usedHealthType == BCS_Vitality; }
	Bool UsesEssence() const { return m_usedHealthType == BCS_Essence; }
	EBaseCharacterStats UsedHPType() const { return m_usedHealthType; }
	void SetUsedHealthType( EBaseCharacterStats healthType ) { m_usedHealthType = healthType; }

	//! Add ability - should be used with abilities that have the same min and max modifier values
	Bool AddAbility( CName name, Bool allowMultiple = false, Bool runScriptCallback = true );
	//! Remove ability - opposite to AddAbility
	Bool RemoveAbility( CName name, Bool runScriptCallback = true );

	void AddAbilitiesFromParam( CCharacterStatsParam * param );

	//! Apply all item abilities
	Bool ApplyItemAbilities( const SInventoryItem& item );

	// Apply given item ability
	Bool ApplyItemAbility(  const SInventoryItem& item, CName ability );

	Bool ApplyItemSlotAbilities( const SInventoryItem& item, Int32 slotIndex );

	//! Take out all item abilities
	Bool RemoveItemAbilities( const SInventoryItem& item );

	// Remove given item ability
	Bool RemoveItemAbility( const SInventoryItem& item, CName ability );

	Bool RemoveItemSlotAbilities( const SInventoryItem& item, Int32 slotIndex );

	//! Has ability?
	Bool HasAbility( CName name, Bool includeEquipment = false ) const;

	//! Has ability with given tag?
	Bool HasAbilityWithTag( CName tag, Bool includeEquipment = false ) const;

	//! Clear all attributes and reapply abilities
	void ReapplyAbilities();

	//! Get abilities
	const AbilitiesMultiSet& GetAbilities() const { return m_abilities; }

	//! Get all attributes names
	void GetAllAttributesNames( TDynArray<CName> & outNames ) const;

	//! Get (final) attribute value (base + items) for all abilities - optionally filter by tags
	void GetAttributeValue( SAbilityAttributeValue& outValue, CName attributeName, const TDynArray< CName > & abilityTags, Bool withoutTags = false ) const;

	//! Get (final) attribute value (base + items) for ability name
	void GetAbilityAttributeValue( SAbilityAttributeValue& outValue, CName attributeName, const CName abilityName ) const;

	//! Calc attribute value with tag filtering
	SAttributeValue CalculateAttributeValue( const CName& attributeName, const TagList& abilityTags, Bool withoutTags = false ) const;

	//! Log abilities
	void LogAbilities( const TDynArray< CName >& abilities );

	//! Save to save game
	void SaveState( IGameSaver* saver );

	//! Load from game save
	void RestoreState( IGameLoader* loader );

	//! Export to xml
	void Export( CXMLWriter& writer );

	void FilterAbilitiesByPrerequisites( TDynArray< CName > & abilities ) const;
	
protected:
	SAttributeValue CalculateAttributeValueInternal( const CName& attributeName, const TDynArray< CName >& abilities, const TDynArray< SEquipmentAbility >& equipmentAbilities ) const;
	void ApplyItemAbilitiesInternal( const TDynArray< CName >& abilities, Uint16 staticRandomSeed );
	void ApplyItemAbilityInternal( CName ability, Uint16 staticRandomSeed );
	void ApplyAttributeModifiers( const THashMap< CName, SAbilityAttributeValue >& modifiers, Uint16 staticRandomSeed );
	void CallAbilityAddedEvents( const TDynArray< CName >& abilities );
	void CallAbilityAddedEvent( CName ability );
	void CallAbilityRemovedEvents( const TDynArray< CName >& abilities );
	void CallAbilityRemovedEvent( CName ability );
	void RemoveItemAbilitiesInternal( const TDynArray< CName >& abilities, Uint16 staticRandomSeed );
	void RemoveItemAbilityInternal( CName ability, Uint16 staticRandomSeed );
	Bool RemoveEquipmentAbility( const SInventoryItem& item );
	Bool RemoveEquipmentAbility( const SInventoryItem& item, CName ability );
	void AddEquipmentAbility( const SInventoryItem& item, CName ability );
	void RemoveAttributeModifiers( const THashMap< CName, SAbilityAttributeValue >& modifiers, Uint16 staticRandomSeed );

	void funcAddAbility( CScriptStackFrame& stack, void* result );
	void funcRemoveAbility( CScriptStackFrame& stack, void* result );
	void funcHasAbility( CScriptStackFrame& stack, void* result );
	void funcHasAbilityWithTag( CScriptStackFrame& stack, void* result );
	void funcGetAllAttributesNames( CScriptStackFrame& stack, void* result );
	void funcIsAbilityAvailableToBuy( CScriptStackFrame& stack, void* result );
	void funcGetAbilities( CScriptStackFrame& stack, void* result );
	void funcGetAttributeValue( CScriptStackFrame& stack, void* result );
	void funcGetAbilityAttributeValue( CScriptStackFrame& stack, void* result );
	void funcGetAllContainedAbilities( CScriptStackFrame& stack, void* result );

	void funcAddAbilityMultiple( CScriptStackFrame& stack, void* result );
	void funcRemoveAbilityMultiple( CScriptStackFrame& stack, void* result );
	void funcRemoveAbilityAll( CScriptStackFrame& stack, void* result );
	void funcGetAbilityCount( CScriptStackFrame& stack, void* result );
	void funcGetAbilitiesWithTag( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCharacterStats );
	PARENT_CLASS( CObject );
	//PROPERTY_CUSTOM_EDIT_ARRAY( m_abilities, TXT("Array of abilities"), TXT("AbilitySelection") );
	NATIVE_FUNCTION( "AddAbility", funcAddAbility );
	NATIVE_FUNCTION( "RemoveAbility", funcRemoveAbility );
	NATIVE_FUNCTION( "HasAbility", funcHasAbility );
	NATIVE_FUNCTION( "HasAbilityWithTag", funcHasAbilityWithTag );
	NATIVE_FUNCTION( "GetAllAttributesNames", funcGetAllAttributesNames );
	NATIVE_FUNCTION( "IsAbilityAvailableToBuy", funcIsAbilityAvailableToBuy );
	NATIVE_FUNCTION( "GetAbilities", funcGetAbilities );
	NATIVE_FUNCTION( "GetAttributeValue", funcGetAttributeValue );
	NATIVE_FUNCTION( "GetAbilityAttributeValue", funcGetAbilityAttributeValue );
	NATIVE_FUNCTION( "GetAllContainedAbilities", funcGetAllContainedAbilities );
	NATIVE_FUNCTION( "AddAbilityMultiple", funcAddAbilityMultiple );
	NATIVE_FUNCTION( "RemoveAbilityMultiple", funcRemoveAbilityMultiple );
	NATIVE_FUNCTION( "RemoveAbilityAll", funcRemoveAbilityAll );
	NATIVE_FUNCTION( "GetAbilityCount", funcGetAbilityCount );
	NATIVE_FUNCTION( "GetAbilitiesWithTag", funcGetAbilitiesWithTag );
END_CLASS_RTTI();