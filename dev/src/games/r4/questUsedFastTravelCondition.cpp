#include "build.h"
#include "r4QuestSystem.h"
#include "questUsedFastTravelCondition.h"
#include "mapFastTravel.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EFastTravelConditionType )
IMPLEMENT_ENGINE_CLASS( CQuestUsedFastTravelCondition )

//////////////////////////////////////////////////////////////////////////

CQuestUsedFastTravelCondition::CQuestUsedFastTravelCondition()
	: m_isFulfilled( false )
{
}

void CQuestUsedFastTravelCondition::OnEvent( const SUsedFastTravelEvent& event )
{
	if ( m_isFulfilled )
	{
		return;
	}

	if ( m_conditionType == FTCT_StartedFastTravel )
	{
		if (  m_pinTag == event.m_tag && event.m_onStart )
		{
			m_isFulfilled = true;
		}
	}
	else if ( m_conditionType == FTCT_FinishedFastTravel )
	{
		if (  m_pinTag == event.m_tag && !event.m_onStart )
		{
			m_isFulfilled = true;
		}
	}
}

void CQuestUsedFastTravelCondition::OnActivate()
{
	m_isFulfilled = false;
	if ( GCommonGame && GCommonGame->GetSystem< CR4QuestSystem >() )
	{
		GCommonGame->GetSystem< CR4QuestSystem >()->AttachFastTravelListener( *this );
	}
}

void CQuestUsedFastTravelCondition::OnDeactivate()
{
	if ( GCommonGame && GCommonGame->GetSystem< CR4QuestSystem >() )
	{
		GCommonGame->GetSystem< CR4QuestSystem >()->DetachFastTravelListener( *this );
	}
}

Bool CQuestUsedFastTravelCondition::OnIsFulfilled()
{
	return m_isFulfilled;
}
