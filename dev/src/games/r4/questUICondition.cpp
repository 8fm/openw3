#include "build.h"

#include "questUICondition.h"

#include "r4GuiManager.h"
#include "menuEvents.h"

IMPLEMENT_RTTI_ENUM( EOpenedUIPanel )
IMPLEMENT_RTTI_ENUM( EOpenedUIJournalTab );
IMPLEMENT_ENGINE_CLASS( IUIConditionType );

//////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQCIsOpenedMenu )

void CQCIsOpenedMenu::OnEvent( const SMenuEvent& event )
{
	if ( m_isFulfilled )
	{
		return;
	}

	if ( m_menuToBeOpened != CName::NONE )
	{
		// panel specified, checking if is expected
		if ( m_menuToBeOpened == event.m_openedMenu )
		{
			m_isFulfilled = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQCIsOpenedJournalEntry )

void CQCIsOpenedJournalEntry::OnEvent( const SMenuEvent& event )
{
	if ( m_isFulfilled )
	{
		return;
	}

	// expected journal tab opened, checking optional conditions
	if ( m_journalEntryToBeOpened )
	{
		CJournalBase* journalBase = m_journalEntryToBeOpened->GetTarget();
		// journal entry specified, checking if is expected
		if ( journalBase == event.m_openedJournalEntry.Get() )
		{
			// all conditions fulfilled
			m_isFulfilled = true;
		}
		}
	else
	{
		// no journal entry specified, condition fulfilled
		m_isFulfilled = true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQCIsSentCustomUIEvent )

	void CQCIsSentCustomUIEvent::OnEvent( const SMenuEvent& event )
{
	if ( m_isFulfilled )
	{
		return;
	}

	if ( m_eventName == event.m_eventName )
	{
		m_isFulfilled = true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQCIsObjectiveHighlighted )

Bool CQCIsObjectiveHighlighted::OnIsFulfilled()
{
	if ( m_objectiveEntry )
	{
		const CJournalBase* target = m_objectiveEntry->GetTarget();
		if ( target )
		{
			const CWitcherJournalManager* journalManager = GCommonGame->GetSystem< CWitcherJournalManager >();
			if ( journalManager )
			{
				return target == journalManager->GetHighlightedObjective();
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestUICondition )

void CQuestUICondition::OnEvent( const SMenuEvent& event )
{
	if ( !m_checkType )
	{
		return;
	}
	m_checkType->OnEvent( event );
}

void CQuestUICondition::OnActivate()
{
	TBaseClass::OnActivate();

	CR4GuiManager* guiManager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( guiManager )
	{
		guiManager->AttachUIListener( *this );
	}
}

void CQuestUICondition::OnDeactivate()
{
	TBaseClass::OnDeactivate();

	CR4GuiManager* guiManager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( guiManager )
	{
		guiManager->DetachUIListener( *this );
	}
}

Bool CQuestUICondition::OnIsFulfilled()
{
	if ( !m_checkType )
	{
		return false;
	}
	return m_checkType->OnIsFulfilled();
}
