#include "build.h"
#include "questFactsDBCondition.h"
#include "factsDB.h"

IMPLEMENT_ENGINE_CLASS( CQuestFactsDBConditionBase )

CQuestFactsDBConditionBase::CQuestFactsDBConditionBase()
	: m_isFulfilled( false )
	, m_wasRegistered( false )
{
}

CQuestFactsDBConditionBase::~CQuestFactsDBConditionBase()
{
	RegisterCallback( false );
}

void CQuestFactsDBConditionBase::OnActivate()
{
	TBaseClass::OnActivate();

	m_isFulfilled = false;
	m_wasRegistered = false;

	QueryFact();
	if ( !IsFactCondFulfilled() )
	{
		RegisterCallback( true );
	}
}

void CQuestFactsDBConditionBase::OnDeactivate()
{
	RegisterCallback( false );
	m_wasRegistered = false;

	TBaseClass::OnDeactivate();
}

Bool CQuestFactsDBConditionBase::OnIsFulfilled()
{
	if ( !IsFactCondFulfilled() && !m_wasRegistered )
	{
		if ( RegisterCallback( true ) )
		{
			QueryFact();
		}
	}
	return IsFactCondFulfilled();
}

Bool CQuestFactsDBConditionBase::RegisterCallback( Bool reg )
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

void CQuestFactsDBConditionBase::QueryFact()
{
	CFactsDB *factsDB = GCommonGame? GCommonGame->GetSystem< CFactsDB >() : nullptr;
	if ( factsDB != nullptr )
	{
		m_isFulfilled = CFactsDBEditorQuery::Evaluate( *factsDB, m_queryFact, m_factId, m_value,
													    m_compareFunc );
	}
}

void CQuestFactsDBConditionBase::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Fact && param.Get< String >() == m_factId )
	{
		QueryFact();
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestFactsDBCondition )

Bool CQuestFactsDBCondition::IsFactCondFulfilled()
{
	return m_isFulfilled;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestFactsDBForbiddenCondition )

Bool CQuestFactsDBForbiddenCondition::IsFactCondFulfilled()
{
	return !m_isFulfilled;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestFactsDBExCondition )

CQuestFactsDBExCondition::CQuestFactsDBExCondition()
	: m_isFulfilled( false )
{
}

CQuestFactsDBExCondition::~CQuestFactsDBExCondition()
{
	RegisterCallback( false );
}

void CQuestFactsDBExCondition::OnActivate()
{
	TBaseClass::OnActivate();

	m_isFulfilled = false;
	m_wasRegistered = false;

	QueryFact();
	if ( !m_isFulfilled )
	{
		RegisterCallback( true );
	}
}

void CQuestFactsDBExCondition::OnDeactivate()
{
	RegisterCallback( false );
	m_wasRegistered = false;

	TBaseClass::OnDeactivate();
}

Bool CQuestFactsDBExCondition::OnIsFulfilled()
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

Bool CQuestFactsDBExCondition::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		TDynArray< String > ids;
		ids.PushBack( m_factId1 );
		ids.PushBack( m_factId2 );
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Fact, this, ids );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Fact, this, ids );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

void CQuestFactsDBExCondition::QueryFact()
{
	CFactsDB *factsDB = GCommonGame? GCommonGame->GetSystem< CFactsDB >() : nullptr;
	if ( factsDB != nullptr )
	{
		m_isFulfilled = CFactsDBEditorQuery::Evaluate( *factsDB, m_queryFact, m_factId1,
														m_factId2, m_compareFunc );
	}
}

void CQuestFactsDBExCondition::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Fact )
	{
		String id = param.Get< String >();
		if ( id == m_factId1 || id == m_factId2 )
		{
			QueryFact();
		}
	}
}
