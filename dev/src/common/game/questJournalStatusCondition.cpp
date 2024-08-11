#include "build.h"

#include "questJournalStatusCondition.h"
#include "journalPath.h"

IMPLEMENT_ENGINE_CLASS( CQuestJournalStatusCondition )

void CQuestJournalStatusCondition::OnJournalBaseEvent( const SJournalEvent& event )
{
	if ( event.GetClass()->IsA< SJournalStatusEvent >() )
	{
		const SJournalStatusEvent& statusEvent = static_cast< const SJournalStatusEvent& >( event );
		m_isFulfilled = EvaluateImpl( &statusEvent );
	}
}

void CQuestJournalStatusCondition::OnActivate()
{
	TBaseClass::OnActivate();

	CJournalManager* manager = GCommonGame->GetSystem< CJournalManager >();
	if ( manager )
	{
		m_isFulfilled = EvaluateImpl( nullptr );
		if ( !m_isFulfilled )
		{
			RegisterCallback( true );
		}
	}
}

void CQuestJournalStatusCondition::OnDeactivate()
{
	TBaseClass::OnDeactivate();

	if ( m_wasRegistered )
	{
		RegisterCallback( false );
	}
}

Bool CQuestJournalStatusCondition::OnIsFulfilled()
{
	return m_isFulfilled;
}

Bool CQuestJournalStatusCondition::RegisterCallback( Bool reg )
{
	CJournalManager* manager = GCommonGame->GetSystem< CJournalManager >();
	if ( manager )
	{
		if ( reg )
		{
			manager->RegisterEventListener( this );
		}
		else
		{
			manager->UnregisterEventListener( this );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

Bool CQuestJournalStatusCondition::EvaluateImpl( const SJournalStatusEvent* event )
{
	if ( m_isFulfilled )
	{
		return m_isFulfilled;
	}

	const CJournalPath* path = m_entry.GetConst();
	if ( path )
	{
		const CJournalBase* target = path->GetTarget();
		if ( target )
		{
			if ( event )
			{
				if ( m_inverted )
				{
					return event->m_newStatus != m_status && event->m_entry == target;
				}
				else
				{
					return event->m_newStatus == m_status && event->m_entry == target;
				}
			}
			else
			{
				const CJournalManager* manager = GCommonGame->GetSystem< CJournalManager >();
				if ( manager )
				{
					if ( m_inverted )
					{
						return manager->GetEntryStatus( target ) != m_status;
					}
					else
					{
						return manager->GetEntryStatus( target ) == m_status;
					}
				}
			}
		}
	}
	return false;
}
