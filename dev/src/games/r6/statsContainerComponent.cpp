#include "build.h"

#include "statsContainerComponent.h"

IMPLEMENT_RTTI_ENUM( EStatsType );
IMPLEMENT_ENGINE_CLASS( SSingleStat );
IMPLEMENT_ENGINE_CLASS( SStatsList );
IMPLEMENT_ENGINE_CLASS( CStatsContainerComponent );
/*

void CStatsContainerComponent::Get2dArrayPropertyAdditionalProperties( IProperty *property, S2daValueProperties &valueProperties )
{
	valueProperties.m_array = resStats.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Name");
}*/

void CStatsContainerComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	
	ResetStats( EST_Default		);
	ResetStats( EST_BaseStats	);
}

TDynArray< SSingleStat >* CStatsContainerComponent::GetStats( EStatsType statType )
{
	for( Uint32 i=0; i<m_statsLists.Size(); ++i )
	{
		if( m_statsLists[i].m_type == statType )
			return &( m_statsLists[i].m_stats );
	}
	return NULL;
}

Float CStatsContainerComponent::GetStatValue( CName name )
{	
	return GetStatValue( name, EST_Default );
}

Float CStatsContainerComponent::GetStatValue( CName name, EStatsType statType )
{
	TDynArray< SSingleStat >* stats = GetStats( statType );
	if( !stats )
		return 0;

	for( Uint32 i=0; i<stats->Size(); ++i )
	{
		SSingleStat& stat = (*stats)[ i ];
		if( stat.m_name == name )
		{
			return stat.GetValue();
		}
	}
	return 0;
}

Float CStatsContainerComponent::GetStatBaseValue( CName name, EStatsType statType )
{
	TDynArray< SSingleStat >* stats = GetStats( statType );
	if( !stats )
		return 0;

	for( Uint32 i=0; i<stats->Size(); ++i )
	{
		SSingleStat& stat = (*stats)[ i ];
		if( stat.m_name == name )
		{
			return stat.GetBaseValue();
		}
	}
	return 0;
}

void CStatsContainerComponent::AddStatValue( CName name, Float valueToAdd, Bool addIfNotExists /*= false*/ )
{
	AddStatValue( name, valueToAdd, EST_Default, addIfNotExists );
}

void CStatsContainerComponent::AddStatValue( CName name, Float valueToAdd, EStatsType statType, Bool addIfNotExists /*= false*/ )
{
	TDynArray< SSingleStat >* stats = GetStats( statType );
	if( !stats )
		return;

	for( Uint32 i=0; i<stats->Size(); ++i )
	{
		SSingleStat& stat = (*stats)[ i ];
		if( stat.m_name == name )
		{
			stat.m_modifiedValue += valueToAdd;
			return;
		}
	}		
	if( addIfNotExists )
	{
		stats->PushBack( SSingleStat( name, 0, valueToAdd ) );
	}	
}

Bool CStatsContainerComponent::SetStatValue( CName name, Float valueToAdd )
{
	return SetStatValue( name, valueToAdd, EST_Default );
}

Bool CStatsContainerComponent::SetStatValue( CName name, Float valueToAdd, EStatsType statType )
{
	TDynArray< SSingleStat >* stats = GetStats( statType );
	if( !stats )
		return false;

	for( Uint32 i=0; i<stats->Size(); ++i )
	{
		SSingleStat& stat = (*stats)[ i ];
		if( stat.m_name == name )
		{
			stat.m_modifiedValue = valueToAdd;
			stat.m_baseValue	 = valueToAdd;
			return true;
		}
	}	
	return false;
}

void CStatsContainerComponent::ResetStats()
{
	ResetStats( EST_Default );
}

void CStatsContainerComponent::ResetStats( EStatsType statType )
{
	TDynArray< SSingleStat >* stats = GetStats( statType );
	if( !stats )
		return;
	
	for( Uint32 i=0; i<stats->Size(); ++i )
	{
		(*stats)[ i ].Reset();
	}
}


void CStatsContainerComponent::funcGetStatValue		( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName	 , statName, CName::NONE );
	GET_PARAMETER( EStatsType, statType, EST_Default );
	FINISH_PARAMETERS;

	Float statVal = GetStatValue( statName, statType );
	
	RETURN_FLOAT( statVal );
}

void CStatsContainerComponent::funcGetStatBaseValue	( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName	 , statName, CName::NONE );
	GET_PARAMETER( EStatsType, statType, EST_Default );
	FINISH_PARAMETERS;

	Float statVal = GetStatBaseValue( statName, statType );

	RETURN_FLOAT( statVal );
}

void CStatsContainerComponent::funcSetBaseValue		( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName		, statName	, CName::NONE	);
	GET_PARAMETER( EStatsType	, statType	, EST_Default	);
	GET_PARAMETER( Float		, sVal		, 0				);
	FINISH_PARAMETERS;

	Bool ret = SetStatValue( statName, sVal, statType );

	RETURN_BOOL( ret );
}

void CStatsContainerComponent::funcAddNewStat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName		, statName	, CName::NONE	);
	GET_PARAMETER( EStatsType	, statType	, EST_Default	);
	GET_PARAMETER( Float		, sVal		, 0				);
	FINISH_PARAMETERS;
	
	TDynArray< SSingleStat >* stats = GetStats( statType );
	if( !stats )
		return;

	for( Uint32 i=0; i<stats->Size(); ++i )
	{
		SSingleStat& stat = (*stats)[ i ];
		if( stat.m_name == statName )
		{			
			RETURN_BOOL( false );
			return;
		}
	}		

	stats->PushBack( SSingleStat( statName, sVal, sVal ) );
	RETURN_BOOL( true );
}

void CStatsContainerComponent::funcAddStatValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName		, statName		, CName::NONE	);
	GET_PARAMETER( EStatsType	, statType		, EST_Default	);
	GET_PARAMETER( Float		, sVal			, 0				);
	GET_PARAMETER( Bool			, addIfNotExists, 0				);
	FINISH_PARAMETERS;

	AddStatValue( statName, sVal, statType, addIfNotExists );
}