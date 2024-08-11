/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "r4Enums.h"

//////////////////////////////////////////////////////////////////////////

struct SBaseStat
{
	DECLARE_RTTI_STRUCT( SBaseStat );

	Float				m_current;
	Float				m_max;
	EBaseCharacterStats	m_type;

	SBaseStat()
		: m_current( 0.0f )
		, m_max( 0.0f )
		, m_type( static_cast< EBaseCharacterStats >( 0 ) )
	{}

	RED_INLINE Float GetPercent() const { return ( m_max != 0.0f ) ? ( m_current / m_max ) : 0.0f; }
};

BEGIN_CLASS_RTTI( SBaseStat );
	PROPERTY_SAVED( m_current );
	PROPERTY_SAVED( m_max );
	PROPERTY_SAVED( m_type );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SResistanceValue
{
	DECLARE_RTTI_STRUCT( SResistanceValue );

	SAbilityAttributeValue	m_points;
	SAbilityAttributeValue	m_percents;
	ECharacterDefenseStats	m_type;

	SResistanceValue()
		: m_type( static_cast< ECharacterDefenseStats >( 0 ) )
	{}
};

BEGIN_CLASS_RTTI( SResistanceValue );
	PROPERTY_SAVED( m_points );
	PROPERTY_SAVED( m_percents );
	PROPERTY_SAVED( m_type );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
struct SBlockedAbility
{
	DECLARE_RTTI_STRUCT( SBlockedAbility );

	CName m_abilityName;
	Float m_timeWhenEnabledd;											//lock remaining time or -1 if disabled until manually enabled again
	Int32 m_count;														//how many instances of ability does actor have (for multiple abilities)
};

BEGIN_CLASS_RTTI( SBlockedAbility );
	PROPERTY_EDIT_SAVED( m_abilityName, TXT("Ability names") );
	PROPERTY_EDIT_SAVED( m_timeWhenEnabledd, TXT("Time to activate ability") );
	PROPERTY_SAVED( m_count );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class W3AbilityManager : public IScriptable, public IAbilityManager
{
	template< typename U >
	struct EnumHashFunc
	{
		static RED_FORCE_INLINE Uint32 GetHash( const U key ) { return static_cast< Uint32 >( key ); }
	};

	DECLARE_RTTI_SIMPLE_CLASS( W3AbilityManager );

	THandle< CActor >							m_owner;
	THandle< CCharacterStats >					m_charStats;
	EDifficultyMode								m_usedDifficultyMode;
	EBaseCharacterStats							m_usedHealthType;
	Bool										m_ignoresDifficultySettings;

	TDynArray< TDynArray< CName > >				m_difficultyAbilities;

	TDynArray< SBaseStat >						m_statPoints;
	TDynArray< SResistanceValue >				m_resistStats;
	TDynArray< SBlockedAbility >				m_blockedAbilities;

	typedef THashMap< EBaseCharacterStats, Int32, EnumHashFunc< EBaseCharacterStats > >			TBaseStatsIndexes;
	typedef THashMap< ECharacterDefenseStats, Int32, EnumHashFunc< ECharacterDefenseStats > >	TResistStatsIndexes;
	mutable TBaseStatsIndexes					m_baseStatsIndexes;
	mutable TResistStatsIndexes					m_resistStatsIndexes;

public:

	// cached maps
	typedef THashMap< CName, EBaseCharacterStats >												TBaseCharacterStatsNameToEnum;
	typedef THashMap< EBaseCharacterStats, CName, EnumHashFunc< EBaseCharacterStats > >			TBaseCharacterStatsEnumToName;
	typedef THashMap< CName, ECharacterDefenseStats >											TCharacterDefenseStatsNameToEnum;
	typedef THashMap< ECharacterDefenseStats, CName, EnumHashFunc< ECharacterDefenseStats > >	TCharacterDefenseStatsEnumToName;

protected:

	// dummy
	static TDynArray< CName >					s_emptyTagsList;

	static Bool									s_scriptedDataCached;
	static TBaseCharacterStatsNameToEnum		s_baseStatNameToEnum;
	static TBaseCharacterStatsEnumToName		s_baseStatEnumToName;
	static TCharacterDefenseStatsNameToEnum		s_resistStatPointNameToEnum;
	static TCharacterDefenseStatsEnumToName		s_resistStatPointEnumToName;
	static TCharacterDefenseStatsNameToEnum		s_resistStatPercentNameToEnum;
	static TCharacterDefenseStatsEnumToName		s_resistStatPercentEnumToName;
	static CName								BASE_ABILITY_TAG;
	static CName								DIFFICULTY_TAG_DIFF_ABILITY;
	static CName								DIFFICULTY_TAG_EASY;
	static CName								DIFFICULTY_TAG_MEDIUM;
	static CName								DIFFICULTY_TAG_HARD;
	static CName								DIFFICULTY_TAG_HARDCORE;
	static CName								DIFFICULTY_HP_MULTIPLIER;

public:

	W3AbilityManager();
	~W3AbilityManager();

	static W3AbilityManager* Get( CActor* actor );
	////////////////////////////////////////////////////////////////////////
	// Hp type managment
	Bool UsesVitality() const { return m_usedHealthType == BCS_Vitality; }
	Bool UsesEssence() const { return m_usedHealthType == BCS_Essence; }
	EBaseCharacterStats UsedHPType() const { return m_usedHealthType; }
	////////////////////////////////////////////////////////////////////////
	Bool IsAbilityBlocked( CName abilityName ) const;

	////////////////////////////////////////////////////////////////////////
	// IAbilityManager interface
	virtual BlockedAbilityIdx VFindBlockedAbility( CName abilityName ) const;
	virtual void VIncreaseBlockedAbilityCount( BlockedAbilityIdx idx );
	virtual void VDecreaseBlockedAbilityCount( BlockedAbilityIdx idx );
	////////////////////////////////////////////////////////////////////////
protected:
	void DetermineUsedHealthType();

	Bool SetInitialStats( EDifficultyMode difficultyMode );
	Bool FixInitialStats( EDifficultyMode difficultyMode );

	Float CalculateAttributeValue( const SAbilityAttributeValue& att, Bool disallowNegativeNult = false ) const;
	void GetAttributeValueInternal( SAbilityAttributeValue& outValue, CName attributeName, TDynArray< CName > * tags = nullptr ) const;
	void GetAttributeRandomizedValue( SAbilityAttributeValue& outValue, const SAbilityAttributeValue& min, const SAbilityAttributeValue& max ) const;

	void CacheDifficultyAbilities();
	void UpdateStatsForDifficultyLevel( EDifficultyMode difficultyMode );
	void UpdateDifficultyAbilities( EDifficultyMode difficultyMode );
	CName GetDifficultyTagForMode( EDifficultyMode difficultyMode );

	Int32 GetStatIndex( EBaseCharacterStats stat ) const;
	Bool HasStat( EBaseCharacterStats stat ) const;
	void StatAddNew( EBaseCharacterStats stat, Float max = 0.0f );
	void RestoreStat( EBaseCharacterStats stat );
	void RestoreStats();
	Float GetStatCurrent( EBaseCharacterStats stat ) const;
	Float GetStatMax( EBaseCharacterStats stat ) const;
	Float GetStatPercents( EBaseCharacterStats stat ) const;
	Bool GetStats( EBaseCharacterStats stat, Float& current, Float& max ) const;
	void SetStatPointCurrent( EBaseCharacterStats stat, Float val );
	void SetStatPointMax( EBaseCharacterStats stat, Float val );
	void UpdateStatMax( EBaseCharacterStats stat );

	Int32 GetResistStatIndex( ECharacterDefenseStats stat ) const;
	Bool HasResistStat( ECharacterDefenseStats stat ) const;
	Bool GetResistStat( ECharacterDefenseStats stat, SResistanceValue& resistStat ) const;
	void SetResistStat( ECharacterDefenseStats stat, const SResistanceValue& resistStat );
	void ResistStatAddNew( ECharacterDefenseStats stat );
	void RecalcResistStat( ECharacterDefenseStats stat );

	static void CacheStaticScriptData();
	static EBaseCharacterStats GetBaseStatNameToEnum( CName statName );
	static CName GetBaseStatEnumToName( EBaseCharacterStats stat );
	static ECharacterDefenseStats GetResistStatNameToEnum( CName statName, Bool& outIsPointResistance );
	static CName GetResistStatEnumToName( ECharacterDefenseStats stat, Bool isPointResistance );

	// backward saves compatibility
	// virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	// scripts support
	void funcCacheStaticScriptData( CScriptStackFrame& stack, void* result );
	void funcSetCharacterStats( CScriptStackFrame& stack, void* result );
	void funcFixInitialStats( CScriptStackFrame& stack, void* result );
	void funcSetInitialStats( CScriptStackFrame& stack, void* result );
	void funcHasStat( CScriptStackFrame& stack, void* result );
	void funcStatAddNew( CScriptStackFrame& stack, void* result );
	void funcRestoreStat( CScriptStackFrame& stack, void* result );
	void funcRestoreStats( CScriptStackFrame& stack, void* result );
	void funcGetStat( CScriptStackFrame& stack, void* result );
	void funcGetStatMax( CScriptStackFrame& stack, void* result );
	void funcGetStatPercents( CScriptStackFrame& stack, void* result );
	void funcGetStats( CScriptStackFrame& stack, void* result );
	void funcSetStatPointCurrent( CScriptStackFrame& stack, void* result );
	void funcSetStatPointMax( CScriptStackFrame& stack, void* result );
	void funcUpdateStatMax( CScriptStackFrame& stack, void* result );
	void funcHasResistStat( CScriptStackFrame& stack, void* result );
	void funcGetResistStat( CScriptStackFrame& stack, void* result );
	void funcSetResistStat( CScriptStackFrame& stack, void* result );
	void funcResistStatAddNew( CScriptStackFrame& stack, void* result );
	void funcRecalcResistStat( CScriptStackFrame& stack, void* result );
	void funcGetAttributeValueInternal( CScriptStackFrame& stack, void* result );
	void funcCacheDifficultyAbilities( CScriptStackFrame& stack, void* result );
	void funcUpdateStatsForDifficultyLevel( CScriptStackFrame& stack, void* result );
	void funcUpdateDifficultyAbilities( CScriptStackFrame& stack, void* result );
	void funcIsAbilityBlocked( CScriptStackFrame& stack, void* result );
	void funcGetAllStats_Debug( CScriptStackFrame& stack, void* result );
	void funcGetAllResistStats_Debug( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( W3AbilityManager );
	PARENT_CLASS( IScriptable );
	PROPERTY_SAVED( m_statPoints );
	PROPERTY_SAVED( m_resistStats );
	PROPERTY_SAVED( m_blockedAbilities );
	PROPERTY( m_owner );
	PROPERTY( m_charStats );
	PROPERTY_SAVED( m_usedDifficultyMode );
	PROPERTY_NOSERIALIZE( m_usedHealthType );
	PROPERTY( m_difficultyAbilities );
	PROPERTY( m_ignoresDifficultySettings );
	NATIVE_FUNCTION( "CacheStaticScriptData", funcCacheStaticScriptData );
	NATIVE_FUNCTION( "SetCharacterStats", funcSetCharacterStats );
	NATIVE_FUNCTION( "SetInitialStats", funcSetInitialStats );
	NATIVE_FUNCTION( "FixInitialStats", funcFixInitialStats );
	NATIVE_FUNCTION( "HasStat", funcHasStat );
	NATIVE_FUNCTION( "StatAddNew", funcStatAddNew );
	NATIVE_FUNCTION( "RestoreStat", funcRestoreStat );
	NATIVE_FUNCTION( "RestoreStats", funcRestoreStats );
	NATIVE_FUNCTION( "GetStat", funcGetStat );
	NATIVE_FUNCTION( "GetStatMax", funcGetStatMax );
	NATIVE_FUNCTION( "GetStatPercents", funcGetStatPercents );
	NATIVE_FUNCTION( "GetStats", funcGetStats );
	NATIVE_FUNCTION( "SetStatPointCurrent", funcSetStatPointCurrent );
	NATIVE_FUNCTION( "SetStatPointMax", funcSetStatPointMax );
	NATIVE_FUNCTION( "UpdateStatMax", funcUpdateStatMax );
	NATIVE_FUNCTION( "HasResistStat", funcHasResistStat );
	NATIVE_FUNCTION( "GetResistStat", funcGetResistStat );
	NATIVE_FUNCTION( "SetResistStat", funcSetResistStat );
	NATIVE_FUNCTION( "ResistStatAddNew", funcResistStatAddNew );
	NATIVE_FUNCTION( "RecalcResistStat", funcRecalcResistStat );
	NATIVE_FUNCTION( "GetAttributeValueInternal", funcGetAttributeValueInternal );
	NATIVE_FUNCTION( "CacheDifficultyAbilities", funcCacheDifficultyAbilities );
	NATIVE_FUNCTION( "UpdateStatsForDifficultyLevel", funcUpdateStatsForDifficultyLevel );
	NATIVE_FUNCTION( "UpdateDifficultyAbilities", funcUpdateDifficultyAbilities );
	NATIVE_FUNCTION( "IsAbilityBlocked", funcIsAbilityBlocked );
	NATIVE_FUNCTION( "GetAllStats_Debug", funcGetAllStats_Debug );
	NATIVE_FUNCTION( "GetAllResistStats_Debug", funcGetAllResistStats_Debug );
END_CLASS_RTTI();
