/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "abilityManager.h"

IMPLEMENT_ENGINE_CLASS( SBaseStat );
IMPLEMENT_ENGINE_CLASS( SResistanceValue );
IMPLEMENT_ENGINE_CLASS( SBlockedAbility );
IMPLEMENT_ENGINE_CLASS( W3AbilityManager );

RED_DEFINE_STATIC_NAME( BASE_ABILITY_TAG );
RED_DEFINE_STATIC_NAME( DIFFICULTY_TAG_DIFF_ABILITY );
RED_DEFINE_STATIC_NAME( DIFFICULTY_TAG_EASY );
RED_DEFINE_STATIC_NAME( DIFFICULTY_TAG_MEDIUM );
RED_DEFINE_STATIC_NAME( DIFFICULTY_TAG_HARD );
RED_DEFINE_STATIC_NAME( DIFFICULTY_TAG_HARDCORE );
RED_DEFINE_STATIC_NAME( DIFFICULTY_HP_MULTIPLIER );
RED_DEFINE_STATIC_NAME( GetAbilityManager );
RED_DEFINE_STATIC_NAME( GetNonBlockedSkillAbilitiesList );
RED_DEFINE_STATIC_NAME( OnAirChanged );
RED_DEFINE_STATIC_NAME( StatNameToEnum );
RED_DEFINE_STATIC_NAME( StatEnumToName );
RED_DEFINE_STATIC_NAME( ResistStatPointNameToEnum );
RED_DEFINE_STATIC_NAME( ResistStatPointEnumToName );
RED_DEFINE_STATIC_NAME( ResistStatPercentNameToEnum );
RED_DEFINE_STATIC_NAME( ResistStatPercentEnumToName );
RED_DEFINE_STATIC_NAME( statPoints );
RED_DEFINE_STATIC_NAME( resistStats );


// #define PROFILE_ABILITY_MANAGER

TDynArray< CName >									W3AbilityManager::s_emptyTagsList( 0 );
Bool												W3AbilityManager::s_scriptedDataCached = false;
W3AbilityManager::TBaseCharacterStatsNameToEnum		W3AbilityManager::s_baseStatNameToEnum;
W3AbilityManager::TBaseCharacterStatsEnumToName		W3AbilityManager::s_baseStatEnumToName;
W3AbilityManager::TCharacterDefenseStatsNameToEnum	W3AbilityManager::s_resistStatPointNameToEnum;
W3AbilityManager::TCharacterDefenseStatsEnumToName	W3AbilityManager::s_resistStatPointEnumToName;
W3AbilityManager::TCharacterDefenseStatsNameToEnum	W3AbilityManager::s_resistStatPercentNameToEnum;
W3AbilityManager::TCharacterDefenseStatsEnumToName	W3AbilityManager::s_resistStatPercentEnumToName;
CName												W3AbilityManager::BASE_ABILITY_TAG;
CName												W3AbilityManager::DIFFICULTY_TAG_DIFF_ABILITY;
CName												W3AbilityManager::DIFFICULTY_TAG_EASY;
CName												W3AbilityManager::DIFFICULTY_TAG_MEDIUM;
CName												W3AbilityManager::DIFFICULTY_TAG_HARD;
CName												W3AbilityManager::DIFFICULTY_TAG_HARDCORE;
CName												W3AbilityManager::DIFFICULTY_HP_MULTIPLIER;

//////////////////////////////////////////////////////////////////////////

W3AbilityManager::W3AbilityManager()
	: m_usedDifficultyMode( EDM_NotSet )
	, m_usedHealthType( BCS_Undefined )
	, m_ignoresDifficultySettings( false )
{
	EnableReferenceCounting( true );
}

W3AbilityManager::~W3AbilityManager()
{
	if ( CCharacterStats* stats = m_charStats.Get() )
	{
		stats->SetAbilityManager( nullptr );
	}
}

//////////////////////////////////////////////////////////////////////////
W3AbilityManager* W3AbilityManager::Get( CActor* actor )
{
	THandle< W3AbilityManager > retValue;
	::CallFunctionRet< THandle< W3AbilityManager > >( actor, CNAME( GetAbilityManager ), retValue );
	return retValue.Get();
}

Bool W3AbilityManager::IsAbilityBlocked( CName abilityName ) const
{
	for ( const SBlockedAbility& b : m_blockedAbilities )
	{
		if ( b.m_abilityName == abilityName )
		{
			return true;
		}
	}
	return false;
}

W3AbilityManager::BlockedAbilityIdx W3AbilityManager::VFindBlockedAbility( CName abilityName ) const
{
	for ( Int32 i = 0, n = m_blockedAbilities.Size(); i != n; ++i )
	{
		if ( m_blockedAbilities[ i ].m_abilityName == abilityName )
		{
			return i;
		}
	}
	return INVALID_BLOCKED_ABILITY_IDX;
}
void W3AbilityManager::VIncreaseBlockedAbilityCount( BlockedAbilityIdx idx )
{
	++m_blockedAbilities[ idx ].m_count;
}
void W3AbilityManager::VDecreaseBlockedAbilityCount( BlockedAbilityIdx idx )
{
	--m_blockedAbilities[ idx ].m_count;
}

void W3AbilityManager::DetermineUsedHealthType()
{
	Bool hasEss = HasStat( BCS_Essence ) && GetStatMax( BCS_Essence ) > 0.0f;
	Bool hasVit = HasStat( BCS_Vitality ) && GetStatMax( BCS_Vitality ) > 0.0f;

	if( hasVit && !hasEss )
	{
		m_usedHealthType = BCS_Vitality;
	}
	else if( !hasVit && hasEss )
	{
		m_usedHealthType = BCS_Essence;
	}
	else
	{
		m_usedHealthType = BCS_Undefined;
	}
	if ( CCharacterStats* charStats = m_charStats.Get() )
	{
		charStats->SetUsedHealthType( m_usedHealthType );
	}
}

Bool W3AbilityManager::FixInitialStats( EDifficultyMode difficultyMode )
{
	if ( m_statPoints.Empty() && SetInitialStats( difficultyMode ) )
	{
		return true;
	}

	DetermineUsedHealthType();

	return false;
}

Bool W3AbilityManager::SetInitialStats( EDifficultyMode difficultyMode )
{
	if ( !m_owner.IsValid() || !m_charStats.IsValid() )
	{
		return false;
	}

	{
#ifdef PROFILE_ABILITY_MANAGER
		PC_SCOPE_PIX( SetInitialStats_HasBaseAbility );
#endif

		if ( !m_charStats->HasAbilityWithTag( BASE_ABILITY_TAG, true ) )
		{
			RED_LOG( AbilityManager, TXT( "W3AbilityManager.SetInitialStats: actor<<%s>> has no default ability defined - don't know what to set, actor WILL NOT HAVE ANY STATS, aborting!!!"), m_owner->GetName().AsChar() );
			return false;
		}

	}

	{
#ifdef PROFILE_ABILITY_MANAGER
		PC_SCOPE_PIX( SetInitialStats_Difficulty );
#endif

		if ( !m_ignoresDifficultySettings )
		{
			CacheDifficultyAbilities();
			// if difficulty mode has been initialized
			if ( difficultyMode != EDM_NotSet )
			{
				UpdateStatsForDifficultyLevel( difficultyMode );
			}
		}
	}

	{
#ifdef PROFILE_ABILITY_MANAGER
		PC_SCOPE_PIX( SetInitialStats_BaseCharacterStats );
#endif

		// base character stats
		SAbilityAttributeValue attrValue;
		Uint32 count = static_cast< Uint32 >( BCS_Total );
		for ( Uint32 i = 0; i < count; i++ )
		{
			EBaseCharacterStats stat = static_cast< EBaseCharacterStats >( i );
			GetAttributeValueInternal( attrValue, GetBaseStatEnumToName( stat ) );
			Float val = CalculateAttributeValue( attrValue );
			if ( val > 0.0f )
			{
				if ( !HasStat( stat ) )
				{
					StatAddNew( stat, val );
				}
				else
				{					
					UpdateStatMax( stat );
					RestoreStat( stat );
				}
			}
		}
	}

	{
#ifdef PROFILE_ABILITY_MANAGER
		PC_SCOPE_PIX( SetInitialStats_ResistStats );
#endif

		// resist stats
		Uint32 count = static_cast< Uint32 >( CDS_Total );
		SResistanceValue res;
		for ( Uint32 i = 0; i < count; i++ )
		{
			ECharacterDefenseStats stat = static_cast< ECharacterDefenseStats >( i );
			if ( stat != static_cast< Uint32 >( CDS_None ) )
			{
				GetAttributeValueInternal( res.m_points, GetResistStatEnumToName( stat, true ) );
				GetAttributeValueInternal( res.m_percents, GetResistStatEnumToName( stat, false ) );
				if ( !res.m_points.IsEmpty() || !res.m_percents.IsEmpty() )
				{
					if ( !HasResistStat( stat ) )
					{
						ResistStatAddNew( stat );
					}
					else
					{					
						RecalcResistStat( stat );
					}
				}
			}
		}
	}

	DetermineUsedHealthType();

	return true;
}

//////////////////////////////////////////////////////////////////////////

Float W3AbilityManager::CalculateAttributeValue( const SAbilityAttributeValue& att, Bool disallowNegativeMult /* = false */ ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_CalculateAttributeValue );
#endif

	Float mult = att.m_valueMultiplicative;
	if ( disallowNegativeMult && mult < 0.0f )
	{
		mult = 0.001f;
	}

	return att.m_valueBase * mult + att.m_valueAdditive;
}

void W3AbilityManager::GetAttributeValueInternal( SAbilityAttributeValue& outValue, CName attributeName, TDynArray< CName > * tags /* = nullptr */ ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetAttributeValueInternal );
#endif

	if ( tags == nullptr )
	{
		tags = &s_emptyTagsList;
	}

	RED_ASSERT( m_charStats.IsValid() );

	m_charStats->GetAttributeValue( outValue, attributeName, *tags );

	// the following works only for player, for CActor(s) GetNonBlockedSkillAbilitiesList returns empty array
	if ( m_owner.IsValid() && m_owner->IsPlayer() && tags->Size() > 0 )
	{
		CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
		if ( dm != nullptr )
		{
			TDynArray< CName > nonBlockedSkillAbilities;
			IScriptable* context = const_cast< IScriptable* >( static_cast< const IScriptable* >( this ) );
			CallFunctionRet( context, CNAME( GetNonBlockedSkillAbilitiesList ), *tags, nonBlockedSkillAbilities );
			if ( nonBlockedSkillAbilities.Size() > 0 )
			{
				SAbilityAttributeValue min;
				SAbilityAttributeValue max;
				dm->GetAbilitiesAttributeValue( nonBlockedSkillAbilities, attributeName, min, max, s_emptyTagsList );
				SAbilityAttributeValue add;
				GetAttributeRandomizedValue( add, min, max );
				outValue += add;
			}
		}
	}
}

void W3AbilityManager::GetAttributeRandomizedValue( SAbilityAttributeValue& outValue, const SAbilityAttributeValue& min, const SAbilityAttributeValue& max ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetAttributeRandomizedValue );
#endif

	CStandardRand& rand = GEngine->GetRandomNumberGenerator();
	outValue.m_valueBase			= rand.Get< Float >( min.m_valueBase,			max.m_valueBase );
	outValue.m_valueAdditive		= rand.Get< Float >( min.m_valueAdditive,		max.m_valueAdditive );
	outValue.m_valueMultiplicative	= rand.Get< Float >( min.m_valueMultiplicative, max.m_valueMultiplicative );
}

//////////////////////////////////////////////////////////////////////////

void W3AbilityManager::CacheDifficultyAbilities()
{
	PC_SCOPE_PIX( AbilityManager_CacheDifficultyAbilities );

	RED_ASSERT( m_charStats.IsValid() );

	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
	if ( dm == nullptr )
	{
		return;
	}

	if ( m_difficultyAbilities.Size() != static_cast< Uint32 >( EDM_Total ) )
	{
		m_difficultyAbilities.Clear();
		m_difficultyAbilities.Resize( static_cast< Uint32 >( EDM_Total ) );
	}

	const AbilitiesMultiSet& abilities = m_charStats->GetAbilities();

	for( const auto& abilityPair : abilities.m_set )
	{
		CName uniqueAbilityName = abilityPair.m_first;
		const SAbility*	abilityDef = dm->GetAbilityDefinition( uniqueAbilityName );
		const TDynArray< CName >& containedAbilities = abilityDef->m_abilities;
		for ( CName containedAbilityName : containedAbilities )
		{
			const SAbility* ability = dm->GetAbilityDefinition( containedAbilityName );
			if( ability )
			{
				const auto& tags = ability->m_tags.GetTags();
				
				// not a difficulty ability
				if ( !tags.Exist( DIFFICULTY_TAG_DIFF_ABILITY ) )
				{
					continue;
				}

				if ( tags.Exist( DIFFICULTY_TAG_EASY ) )
				{
					m_difficultyAbilities[ static_cast< Uint32 >( EDM_Easy ) ].PushBackUnique( containedAbilityName );
				}
				if ( tags.Exist( DIFFICULTY_TAG_MEDIUM ) )
				{
					m_difficultyAbilities[ static_cast< Uint32 >( EDM_Medium ) ].PushBackUnique( containedAbilityName );
				}
				if ( tags.Exist( DIFFICULTY_TAG_HARD ) )
				{
					m_difficultyAbilities[ static_cast< Uint32 >( EDM_Hard ) ].PushBackUnique( containedAbilityName );
				}
				if ( tags.Exist( DIFFICULTY_TAG_HARDCORE ) )
				{
					m_difficultyAbilities[ static_cast< Uint32 >( EDM_Hardcore ) ].PushBackUnique( containedAbilityName );
				}
			}	
		}
	}
}

void W3AbilityManager::UpdateStatsForDifficultyLevel( EDifficultyMode difficultyMode )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_UpdateStatsForDifficultyLevel );
#endif

	if ( m_usedDifficultyMode == difficultyMode )
	{
		return;
	}

	UpdateDifficultyAbilities( difficultyMode );
	m_usedDifficultyMode = difficultyMode;
}

void W3AbilityManager::UpdateDifficultyAbilities( EDifficultyMode difficultyMode )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_UpdateDifficultyAbilities );
#endif

	RED_ASSERT( m_charStats.IsValid() );

	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
	if ( dm == nullptr )
	{
		return;
	}

	CCharacterStats* charStats = m_charStats.Get();

	TDynArray< CName > abilitiesToRemove;
	const AbilitiesMultiSet& abilities =  charStats->GetAbilities();

	CName difficultyTag = GetDifficultyTagForMode( difficultyMode );

	// remove old difficulty abilities
	for ( auto abilityPair : abilities.m_set )
	{
		CName abilityName = abilityPair.m_first;
		if ( dm->AbilityHasTag( abilityName, DIFFICULTY_TAG_DIFF_ABILITY ) && !dm->AbilityHasTag( abilityName, difficultyTag ) )
		{
			abilitiesToRemove.PushBack( abilityName );
		}
	}

	for ( CName abilityName : abilitiesToRemove )
	{
		charStats->RemoveAbility( abilityName );
	}

	// add new
	for( CName ability : m_difficultyAbilities[ difficultyMode ] )
	{
		if ( !charStats->HasAbility( ability ) )
		{
			charStats->AddAbility( ability, false );
		}
	}
}

CName W3AbilityManager::GetDifficultyTagForMode( EDifficultyMode difficultyMode )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetDifficultyTagForMode );
#endif

	switch ( difficultyMode )
	{
	case EDM_Easy : 		return DIFFICULTY_TAG_EASY;
	case EDM_Medium : 		return DIFFICULTY_TAG_MEDIUM;
	case EDM_Hard : 		return DIFFICULTY_TAG_HARD;
	case EDM_Hardcore : 	return DIFFICULTY_TAG_HARDCORE;
	default : 				return CName::NONE;
	}
}

//////////////////////////////////////////////////////////////////////////

Int32 W3AbilityManager::GetStatIndex( EBaseCharacterStats stat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetStatIndex );
#endif

	TBaseStatsIndexes::const_iterator it = m_baseStatsIndexes.Find( stat );
	if ( it != m_baseStatsIndexes.End() )
	{
		return it->m_second;
	}
	Int32 index = -1;
	for ( Int32 i = 0; i < m_statPoints.SizeInt(); i++ )
	{
		if ( m_statPoints[ i ].m_type == stat )
		{
			index = i;
			break;
		}
	}
	m_baseStatsIndexes[ stat ] = index;
	return index;
}

Bool W3AbilityManager::HasStat( EBaseCharacterStats stat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_HasStat );
#endif

	return GetStatIndex( stat ) != -1;
}

void W3AbilityManager::StatAddNew( EBaseCharacterStats stat, Float max /* = 0.0f */ )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_StatAddNew );
#endif

	SBaseStat newStat;

	if ( max > 0.0f )
	{
		newStat.m_max = max;
	}
	else
	{
		SAbilityAttributeValue attr;
		GetAttributeValueInternal( attr, GetBaseStatEnumToName( stat ) );
		newStat.m_max = CalculateAttributeValue( attr );
	}

	if ( stat == BCS_Vitality || stat == BCS_Essence )
	{
		SAbilityAttributeValue attr;
		GetAttributeValueInternal( attr, DIFFICULTY_HP_MULTIPLIER );			
		newStat.m_max = newStat.m_max * CalculateAttributeValue( attr );
	}

	m_baseStatsIndexes[ stat ] = m_statPoints.SizeInt();
	newStat.m_type = stat;
	m_statPoints.PushBack( newStat );
	RestoreStat( stat );
}

void W3AbilityManager::RestoreStat( EBaseCharacterStats stat )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_RestoreStat );
#endif

	switch ( stat )
	{
	case BCS_Toxicity:
	case BCS_Focus:
		SetStatPointCurrent( stat, 0 );
		break;
	default:
		SetStatPointCurrent( stat, GetStatMax( stat ) );
	}
}

void W3AbilityManager::RestoreStats()
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_RestoreStats );
#endif

	for ( const SBaseStat& stat : m_statPoints )
	{
		RestoreStat( stat.m_type );
	}
}

Float W3AbilityManager::GetStatCurrent( EBaseCharacterStats stat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetStatCurrent );
#endif

	Int32 index = GetStatIndex( stat );
	if ( index != -1 )
	{
		return m_statPoints[ index ].m_current;
	}
	return -1.0f;
}

Float W3AbilityManager::GetStatMax( EBaseCharacterStats stat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetStatMax );
#endif

	Int32 index = GetStatIndex( stat );
	if ( index != -1 )
	{
		return m_statPoints[ index ].m_max;
	}
	return -1.0f;
}

Float W3AbilityManager::GetStatPercents( EBaseCharacterStats stat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetStatPercents );
#endif

	Int32 index = GetStatIndex( stat );
	if ( index != -1 )
	{
		return m_statPoints[ index ].GetPercent();
	}
	return -1.0f;
}

Bool W3AbilityManager::GetStats( EBaseCharacterStats stat, Float& current, Float& max ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetStats );
#endif

	Int32 index = GetStatIndex( stat );
	if ( index != -1 )
	{
		current = m_statPoints[ index ].m_current;
		max = m_statPoints[ index ].m_max;
		return true;
	}
	current = -1.0f;
	max = -1.0f;
	return false;
}

void W3AbilityManager::SetStatPointCurrent( EBaseCharacterStats stat, Float val )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_SetStatPointCurrent );
#endif

	Int32 index = GetStatIndex( stat );
	if ( index != -1 )
	{
		m_statPoints[ index ].m_current = val;
		if ( stat == BCS_Air )
		{
			CallFunction( this, CNAME( OnAirChanged ) );
		}
	}
}

void W3AbilityManager::SetStatPointMax( EBaseCharacterStats stat, Float val )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_SetStatPointMax );
#endif

	Int32 index = GetStatIndex( stat );
	if ( index != -1 )
	{
		m_statPoints[ index ].m_max = val;
		if ( stat == BCS_Air )
		{
			CallFunction( this, CNAME( OnAirChanged ) );
		}
	}
}

void W3AbilityManager::UpdateStatMax( EBaseCharacterStats stat )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_UpdateStatMax );
#endif

	CName statName = GetBaseStatEnumToName( stat );
	SAbilityAttributeValue attr;
	GetAttributeValueInternal( attr, statName );
	Float statValue = CalculateAttributeValue( attr, stat == BCS_Vitality );

	if ( stat == BCS_Vitality || stat == BCS_Essence )
	{
		GetAttributeValueInternal( attr, DIFFICULTY_HP_MULTIPLIER );
		statValue *= CalculateAttributeValue( attr );
	}

	SetStatPointMax( stat, statValue );
}

//////////////////////////////////////////////////////////////////////////

Int32 W3AbilityManager::GetResistStatIndex( ECharacterDefenseStats stat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetResistStatIndex );
#endif

	TResistStatsIndexes::const_iterator it = m_resistStatsIndexes.Find( stat );
	if ( it != m_resistStatsIndexes.End() )
	{
		return it->m_second;
	}
	Int32 index = -1;
	for ( Int32 i = 0; i < m_resistStats.SizeInt(); i++ )
	{
		if ( m_resistStats[ i ].m_type == stat )
		{
			index = i;
			break;
		}
	}
	m_resistStatsIndexes[ stat ] = index;
	return index;
}

Bool W3AbilityManager::HasResistStat( ECharacterDefenseStats stat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_HasResistStat );
#endif

	return GetResistStatIndex( stat ) != -1;
}

Bool W3AbilityManager::GetResistStat( ECharacterDefenseStats stat, SResistanceValue& resistStat ) const
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetResistStat );
#endif

	Int32 index = GetResistStatIndex( stat );
	if ( index != -1 )
	{
		resistStat = m_resistStats[ index ];
		return true;
	}
	return false;
}

void W3AbilityManager::SetResistStat( ECharacterDefenseStats stat, const SResistanceValue& resistStat )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_SetResistStat );
#endif

	Int32 index = GetResistStatIndex( stat );
	if ( index != -1 )
	{
		m_resistStats[ index ] = resistStat;
	}
	else
	{
		m_resistStatsIndexes[ stat ] = m_resistStats.SizeInt();
		m_resistStats.PushBack( resistStat );
	}
}

void W3AbilityManager::ResistStatAddNew( ECharacterDefenseStats stat )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_ResistStatAddNew );
#endif

	m_resistStatsIndexes[ stat ] = m_resistStats.SizeInt();
	SResistanceValue newStat;
	newStat.m_type = stat;
	m_resistStats.PushBack( newStat );
	RecalcResistStat( stat );
}

void W3AbilityManager::RecalcResistStat( ECharacterDefenseStats stat )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_RecalcResistStat );
#endif

	Int32 index = GetResistStatIndex( stat );	
	if ( index != -1 )
	{
		GetAttributeValueInternal( m_resistStats[ index ].m_points, GetResistStatEnumToName( stat, true ) );
		GetAttributeValueInternal( m_resistStats[ index ].m_percents, GetResistStatEnumToName( stat, false ) );
	}
}

//////////////////////////////////////////////////////////////////////////
// static script data

namespace
{

template < typename T, typename U >
T GetOrCacheEnum( CName name, U& map, CName getValueFunction, T defaultValue = T() )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetOrCacheEnum );
#endif

	typename U::iterator it = map.Find( name );
	if ( it != map.End() )
	{
		return it->m_second;
	}

	T val = defaultValue;
	CallFunctionRet( nullptr, getValueFunction, name, val );
	map[ name ] = val;
	return val;
}

template < typename T, typename U >
CName GetOrCacheName( T val, U& map, CName getValueFunction )
{
#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_GetOrCacheName );
#endif

	typename U::iterator it = map.Find( val );
	if ( it != map.End() )
	{
		return it->m_second;
	}

	CName name = CName::NONE;
	CallFunctionRet( nullptr, getValueFunction, val, name );
	map[ val ] = name;
	return name;
}

}

void W3AbilityManager::CacheStaticScriptData()
{
	if ( s_scriptedDataCached )
	{
		return;
	}

#ifdef PROFILE_ABILITY_MANAGER
	PC_SCOPE_PIX( AbilityManager_CacheStaticScriptData );
#endif

	CFunction* statEnumToName = SRTTI::GetInstance().FindGlobalFunction(  CNAME( StatEnumToName ) );
	RED_ASSERT( statEnumToName != nullptr, TXT( "Cannot find global function: StatEnumToName" ) );

	for ( Uint32 i = 0; i < static_cast< Uint32 >( BCS_Total ); i++ )
	{
		EBaseCharacterStats stat = static_cast< EBaseCharacterStats >( i );
		CName statName = CName::NONE;
		if ( statEnumToName->Call( nullptr, &stat, &statName ) )
		{
			s_baseStatEnumToName[ stat ] = statName;
			s_baseStatNameToEnum[ statName ] = stat;
		}
	}

	CFunction* resistStatPointEnumToName = SRTTI::GetInstance().FindGlobalFunction( CNAME( ResistStatPointEnumToName ) );
	RED_ASSERT( resistStatPointEnumToName != nullptr, TXT( "Cannot find global function: ResistStatPointEnumToName" ) );
	CFunction* resistStatPercentEnumToName = SRTTI::GetInstance().FindGlobalFunction( CNAME( ResistStatPercentEnumToName ) );
	RED_ASSERT( resistStatPercentEnumToName != nullptr, TXT( "Cannot find global function: ResistStatPercentEnumToName" ) );
	
	for ( Uint32 i = 0; i < static_cast< Uint32 >( CDS_Total ); i++ )
	{
		ECharacterDefenseStats stat = static_cast< ECharacterDefenseStats >( i );
		CName statName = CName::NONE;
		if ( resistStatPointEnumToName->Call( nullptr, &stat, &statName ) && statName != CName::NONE )
		{
			s_resistStatPointEnumToName[ stat ] = statName;
			s_resistStatPointNameToEnum[ statName ] = stat;
			s_resistStatPercentNameToEnum[ statName ] = CDS_None;
		}
		statName = CName::NONE;
		if ( resistStatPercentEnumToName->Call( nullptr, &stat, &statName ) && statName != CName::NONE )
		{
			s_resistStatPercentEnumToName[ stat ] = statName;
			s_resistStatPercentNameToEnum[ statName ] = stat;
			s_resistStatPointNameToEnum[ statName ] = CDS_None;
		}
	}

	CR4Game* game = Cast< CR4Game >( GCommonGame );
	RED_ASSERT( game != nullptr, TXT( "W3AbilityManager::CacheStaticScriptData: Cannot find CR4Game" ) );
	W3GameParams* params = game->GetGameParams();
	RED_ASSERT( game != nullptr, TXT( "W3AbilityManager::CacheStaticScriptData: Cannot find W3GameParams" ) );

	BASE_ABILITY_TAG				= params->GetNameParam( CNAME( BASE_ABILITY_TAG ) );
	DIFFICULTY_TAG_DIFF_ABILITY		= params->GetNameParam( CNAME( DIFFICULTY_TAG_DIFF_ABILITY ) );
	DIFFICULTY_TAG_EASY				= params->GetNameParam( CNAME( DIFFICULTY_TAG_EASY ) );
	DIFFICULTY_TAG_MEDIUM			= params->GetNameParam( CNAME( DIFFICULTY_TAG_MEDIUM ) );
	DIFFICULTY_TAG_HARD				= params->GetNameParam( CNAME( DIFFICULTY_TAG_HARD ) );
	DIFFICULTY_TAG_HARDCORE			= params->GetNameParam( CNAME( DIFFICULTY_TAG_HARDCORE ) );
	DIFFICULTY_HP_MULTIPLIER		= params->GetNameParam( CNAME( DIFFICULTY_HP_MULTIPLIER ) );

	s_scriptedDataCached = true;
}

EBaseCharacterStats W3AbilityManager::GetBaseStatNameToEnum( CName statName )
{
	return GetOrCacheEnum< EBaseCharacterStats >( statName, s_baseStatNameToEnum, CNAME( StatNameToEnum ) );
}

CName W3AbilityManager::GetBaseStatEnumToName( EBaseCharacterStats stat )
{
	return GetOrCacheName( stat, s_baseStatEnumToName, CNAME( StatEnumToName ) );
}

ECharacterDefenseStats W3AbilityManager::GetResistStatNameToEnum( CName statName, Bool& outIsPointResistance )
{
	ECharacterDefenseStats stat = GetOrCacheEnum( statName, s_resistStatPointNameToEnum, CNAME( ResistStatPointNameToEnum ), CDS_None );
	if ( stat != CDS_None )
	{
		outIsPointResistance = true;
		return stat;
	}
	outIsPointResistance = false;
	return GetOrCacheEnum( statName, s_resistStatPercentNameToEnum, CNAME( ResistStatPercentNameToEnum ), CDS_None );
}

CName W3AbilityManager::GetResistStatEnumToName( ECharacterDefenseStats stat, Bool isPointResistance )
{
	if ( isPointResistance )
	{
		return GetOrCacheName( stat, s_resistStatPointEnumToName, CNAME( ResistStatPointEnumToName ) );
	}
	else
	{
		return GetOrCacheName( stat, s_resistStatPercentEnumToName, CNAME( ResistStatPercentEnumToName ) );
	}
}

//////////////////////////////////////////////////////////////////////////

void W3AbilityManager::funcCacheStaticScriptData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CacheStaticScriptData();
}

void W3AbilityManager::funcSetCharacterStats( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CCharacterStats >, charStats, nullptr );
	FINISH_PARAMETERS;

	m_charStats = charStats;
	charStats->SetAbilityManager( this );
}

void W3AbilityManager::funcFixInitialStats( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EDifficultyMode, difficultyMode, EDM_NotSet );
	FINISH_PARAMETERS;

	RETURN_BOOL( FixInitialStats( difficultyMode ) );
}

void W3AbilityManager::funcSetInitialStats( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EDifficultyMode, difficultyMode, EDM_NotSet );
	FINISH_PARAMETERS;

	RETURN_BOOL( SetInitialStats( difficultyMode ) );
}

void W3AbilityManager::funcHasStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasStat( stat ) );
}

void W3AbilityManager::funcStatAddNew( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	GET_PARAMETER_OPT( Float, max, 0.0f );
	FINISH_PARAMETERS;

	if ( stat != BCS_Total )
	{
		StatAddNew( stat, max );
	}
}

void W3AbilityManager::funcRestoreStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	FINISH_PARAMETERS;

	if ( stat != BCS_Total )
	{
		RestoreStat( stat );
	}
}

void W3AbilityManager::funcRestoreStats( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RestoreStats();
}

void W3AbilityManager::funcGetStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	GET_PARAMETER_OPT( Float, skipLock, false ); // unused, left here for scripts compatibility
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetStatCurrent( stat ) );
}

void W3AbilityManager::funcGetStatMax( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetStatMax( stat ) );
}

void W3AbilityManager::funcGetStatPercents( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetStatPercents( stat ) );
}

void W3AbilityManager::funcGetStats( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	GET_PARAMETER_REF( Float, current, -1.0f );
	GET_PARAMETER_REF( Float, max, -1.0f );
	FINISH_PARAMETERS;

	RETURN_BOOL( GetStats( stat, current, max ) );
}

void W3AbilityManager::funcSetStatPointCurrent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	GET_PARAMETER( Float, val, 1.0f );
	FINISH_PARAMETERS;

	if ( stat != BCS_Total )
	{
		SetStatPointCurrent( stat, val );
	}
}

void W3AbilityManager::funcSetStatPointMax( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	GET_PARAMETER( Float, val, 1.0f );
	FINISH_PARAMETERS;

	if ( stat != BCS_Total )
	{
		SetStatPointMax( stat, val );
	}
}

void W3AbilityManager::funcUpdateStatMax( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBaseCharacterStats, stat, BCS_Total );
	FINISH_PARAMETERS;

	if ( stat != BCS_Total )
	{
		UpdateStatMax( stat );
	}
}

void W3AbilityManager::funcHasResistStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ECharacterDefenseStats, stat, CDS_None );
	FINISH_PARAMETERS;	

	RETURN_BOOL( HasResistStat( stat ) );
}

void W3AbilityManager::funcGetResistStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ECharacterDefenseStats, stat, CDS_None );
	GET_PARAMETER_REF( SResistanceValue, resistStat, SResistanceValue() );
	FINISH_PARAMETERS;	

	RETURN_BOOL( GetResistStat( stat, resistStat ) );
}

void W3AbilityManager::funcSetResistStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ECharacterDefenseStats, stat, CDS_None );
	GET_PARAMETER_REF( SResistanceValue, resistStat, SResistanceValue() );
	FINISH_PARAMETERS;	

	if ( stat != CDS_None )
	{
		SetResistStat( stat, resistStat );
	}
}

void W3AbilityManager::funcResistStatAddNew( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ECharacterDefenseStats, stat, CDS_None );
	FINISH_PARAMETERS;

	if ( stat != CDS_None )
	{
		ResistStatAddNew( stat );
	}
}

void W3AbilityManager::funcRecalcResistStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ECharacterDefenseStats, stat, CDS_None );
	FINISH_PARAMETERS;

	if ( stat != CDS_None )
	{
		RecalcResistStat( stat );
	}
}

void W3AbilityManager::funcGetAttributeValueInternal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, attributeName, CName::NONE );
	GET_PARAMETER_OPT( TDynArray< CName >, tags, s_emptyTagsList );
	FINISH_PARAMETERS;

	SAbilityAttributeValue attr;
	GetAttributeValueInternal( attr, attributeName, &tags );
	RETURN_STRUCT( SAbilityAttributeValue, attr );
}

void W3AbilityManager::funcCacheDifficultyAbilities( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CacheDifficultyAbilities();
}

void W3AbilityManager::funcUpdateStatsForDifficultyLevel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EDifficultyMode, difficultyMode, EDM_NotSet );
	FINISH_PARAMETERS;

	UpdateStatsForDifficultyLevel( difficultyMode );
}

void W3AbilityManager::funcUpdateDifficultyAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EDifficultyMode, difficultyMode, EDM_NotSet );
	FINISH_PARAMETERS;

	UpdateDifficultyAbilities( difficultyMode );
}

void W3AbilityManager::funcIsAbilityBlocked( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, abilityName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsAbilityBlocked( abilityName ) );
}

void W3AbilityManager::funcGetAllStats_Debug( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< SBaseStat >, stats, TDynArray< SBaseStat >() );
	FINISH_PARAMETERS;

#ifndef RED_FINAL_BUILD
	stats = m_statPoints;
	RETURN_BOOL( stats.Size() > 0 );
#else
	RETURN_BOOL( false );
#endif
}

void W3AbilityManager::funcGetAllResistStats_Debug( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< SResistanceValue >, stats, TDynArray< SResistanceValue >() );
	FINISH_PARAMETERS;

#ifndef RED_FINAL_BUILD
	stats = m_resistStats;
	RETURN_BOOL( stats.Size() > 0 );
#else
	RETURN_BOOL( false );
#endif
}

/*
void W3AbilityManager::OnSerialize( IFile& file )
{
	IScriptable::OnSerialize( file );


	Uint32 size = m_baseStats.Size();
	Uint32 size2 = m_resistanceStats.Size();
}
*/