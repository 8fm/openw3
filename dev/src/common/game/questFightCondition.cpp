#include "build.h"
#include "questFightCondition.h"
#include "factsDB.h"
#include "../core/dataError.h"
#include "../engine/utils.h"

IMPLEMENT_RTTI_ENUM( EQueryFightMode )
IMPLEMENT_ENGINE_CLASS( CQuestFightCondition )
IMPLEMENT_ENGINE_CLASS( CQuestInCombatCondition )

///////////////////////////////////////////////////////////////////////////////

CQuestFightCondition::CQuestFightCondition()
	: m_factId( String::EMPTY )
	, m_isFulfilled( false )
	, m_wasRegistered( false )
{
}

CQuestFightCondition::~CQuestFightCondition()
{
	RegisterCallback( false );
}

void CQuestFightCondition::OnActivate()
{
	TBaseClass::OnActivate();

	m_isFulfilled = false;
	m_wasRegistered = false;

	Bool res = CreateFactId();
	if ( !res )
	{
		HandleFactIdCreationFailure();
	}
	QueryFact();
	if ( !m_isFulfilled )
	{
		RegisterCallback( true );
	}
}

void CQuestFightCondition::OnDeactivate()
{
	RegisterCallback( false );
	m_wasRegistered = false;

	TBaseClass::OnDeactivate();
}

Bool CQuestFightCondition::OnIsFulfilled()
{
	if ( !m_isFulfilled && !m_wasRegistered )
	{
		if ( RegisterCallback( true ) )
		{
			QueryFact();
		}
	}
	return m_isFulfilled;
}

Bool CQuestFightCondition::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Fact, this, m_factId );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Fact, this, m_factId );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

void CQuestFightCondition::CreateHitFact( const Char* hitType, String& hitFact)
{
	hitFact = m_tag.AsString() + hitType;

	if ( m_attackerTag != CName::NONE )
	{
		hitFact += TXT("_by_") + m_attackerTag.AsString();
	}
}

Bool CQuestFightCondition::CreateFactId()
{
	CFactsDB *factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;

	if ( !factsDB )
	{
		return false;
	}

	if ( m_tag == CName::NONE )
	{
		return false;
	}
	
	static String actorPrefix( TXT( "actor_" ) );
	switch ( m_damageMode )
	{
	case QFM_Killed:		CreateHitFact( TXT( "_was_killed" ), m_factId ); m_factId = actorPrefix + m_factId; break;
	case QFM_Stunned:		m_factId = actorPrefix;   m_factId += m_tag.AsString() + TXT( "_was_stunned" ); break;
	case QFM_Hit:			CreateHitFact( TXT( "_weapon_hit" ), m_factId ); break;
	case QFM_Hit_By_Bomb:	CreateHitFact( TXT( "_bomb_hit" ), m_factId ); break;
	case QFM_Hit_By_Bolt:	CreateHitFact( TXT( "_bolt_hit" ), m_factId ); break;
	case QFM_Hit_By_Aard:	CreateHitFact( TXT( "_aard_hit" ), m_factId ); break;
	case QFM_Hit_By_Igni:	CreateHitFact( TXT( "_igni_hit" ), m_factId ); break;
	case QFM_Hit_By_Yrden:	CreateHitFact( TXT( "_yrden_hit" ), m_factId ); break;
	case QFM_Hit_By_Axii:	CreateHitFact( TXT( "_axii_hit" ), m_factId ); break;
	case QFM_KnockedUnconscious:
		{
			m_factId = actorPrefix;   m_factId += m_tag.AsString() + TXT( "_was_knocked_unconscious" ); break;
			break;
		}
	default:			return false;	// unknown damage mode value - exiting...
	}

	return true;
}

void CQuestFightCondition::QueryFact()
{
	CFactsDB *factsDB = GCommonGame? GCommonGame->GetSystem< CFactsDB >() : nullptr;
	if ( factsDB != nullptr )
	{
		m_isFulfilled = factsDB->QuerySum( m_factId ) >= m_referenceValue;
	}
}

void CQuestFightCondition::HandleFactIdCreationFailure() const
{
	DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("Quests"), TXT( "Cannot create fact id for QuestFightCondition") );
}

void CQuestFightCondition::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( m_factId == String::EMPTY )
	{
		Bool res = CreateFactId();
		if ( !res )
		{
			HandleFactIdCreationFailure();
		}
	}
	if ( param.Get< String >() == m_factId )
	{
		QueryFact();
	}
}

///////////////////////////////////////////////////////////////////////////////

CQuestInCombatCondition::CQuestInCombatCondition()
	: m_isInCombat( true )
{
}

Bool CQuestInCombatCondition::OnIsFulfilled()
{
	return GCommonGame->GetPlayer()->IsInCombat() == m_isInCombat;
}

///////////////////////////////////////////////////////////////////////////////
