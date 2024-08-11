/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "encounterImpl.h"

#include "../../common/engine/gameTimeManager.h"

#include "../../common/game/commonGame.h"
#include "../../common/game/factsDB.h"


IMPLEMENT_ENGINE_CLASS( CPlayerLevelCondition )
IMPLEMENT_ENGINE_CLASS( CDaytimeCondition )
IMPLEMENT_ENGINE_CLASS( CSpawnConditionFact )

Bool CPlayerLevelCondition::Test( CSpawnTreeInstance& instance )
{
	CPlayer* player = GCommonGame ? GCommonGame->GetPlayer() : NULL;
	if( player )
	{
		
	}

	return false;
}

Bool CDaytimeCondition::Test( CSpawnTreeInstance& instance )
{
	GameTime time = GGame->GetTimeManager()->GetTime();
	Int32 checkSeconds = time.m_seconds % GameTime::DAY.m_seconds;
	if ( m_begin.m_seconds >= m_end.m_seconds )
	{
		return checkSeconds >= m_begin.m_seconds || checkSeconds < m_end.m_seconds;
	}
	else
	{
		return checkSeconds >= m_begin.m_seconds && checkSeconds < m_end.m_seconds;
	}
}

Bool CDaytimeCondition::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName.AsString() == TXT("minTimeHours") )
	{
		Uint32 val; 
		if ( readValue.AsType( val ) )
		{
			m_begin = GameTime( 0, val, 0, 0 );
		}
		return true;
	}
	else if ( propertyName.AsString() == TXT("maxTimeHours") )
	{
		Uint32 val; 
		if ( readValue.AsType( val ) )
		{
			m_end = GameTime( 0, val, 0, 0 );
		}
		return true;
	}

	return ISpawnCondition::OnPropertyMissing( propertyName, readValue );
}

Bool CSpawnConditionFact::Test( CSpawnTreeInstance& instance )
{
	Int32 sum = 0;;
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() )
	{
		sum = GCommonGame->GetSystem< CFactsDB >()->QuerySum( m_fact );
	}
	switch ( m_compare )
	{
	case CF_Equal:
		return sum == m_value;
	case CF_NotEqual:
		return sum != m_value;
	case CF_Less:
		return sum < m_value;
	case CF_LessEqual:
		return sum <= m_value;
	case CF_Greater:
		return sum > m_value;
	case CF_GreaterEqual:
		return sum >= m_value;
	default:
		RED_WARNING( false, "CSpawnConditionFact has invalid 'compare' property value." );
		return false;
	}
}