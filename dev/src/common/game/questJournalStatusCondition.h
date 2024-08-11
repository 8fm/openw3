#pragma once

#include "questCondition.h"
#include "journalManager.h"

class CQuestJournalStatusCondition : public IQuestCondition, IJournalEventListener
{
	DECLARE_ENGINE_CLASS( CQuestJournalStatusCondition, IQuestCondition, 0 )

private:
	THandle< CJournalPath >		m_entry;
	EJournalStatus				m_status;
	Bool						m_inverted;

	Bool						m_wasRegistered;
	Bool						m_isFulfilled;

public:
	CQuestJournalStatusCondition()
		: m_status( JS_Inactive )
		, m_inverted( false )
		, m_wasRegistered( false )
		, m_isFulfilled( false )
	{}
	virtual ~CQuestJournalStatusCondition() {}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Quest journal status condition" ) ); 
	}
#endif

protected:
	//! IJournalEventListener implementation
	virtual void OnJournalBaseEvent( const SJournalEvent& event );

	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();

	Bool RegisterCallback( Bool reg );
	Bool EvaluateImpl( const SJournalStatusEvent* event );
};

BEGIN_CLASS_RTTI( CQuestJournalStatusCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_entry, TXT("Journal entry to check"), TXT( "JournalPropertyBrowserAll" ) )
	PROPERTY_EDIT( m_status, TXT("Status of journal entry") )
	PROPERTY_EDIT( m_inverted, TXT("Inverted condition") )
END_CLASS_RTTI()
