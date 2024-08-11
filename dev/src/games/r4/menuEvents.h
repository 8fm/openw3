#pragma once

struct SMenuEvent
{
	DECLARE_RTTI_STRUCT( SMenuEvent );

	CName					m_eventName;
	CName					m_openedMenu;
	THandle< CJournalBase >	m_openedJournalEntry;

	SMenuEvent() {};
	SMenuEvent( const CName& eventName, const CName& openedMenu, const THandle< CJournalBase >& openedJournalEntry )
		: m_eventName( eventName )
		, m_openedMenu( openedMenu )
		, m_openedJournalEntry( openedJournalEntry )
	{};
};

BEGIN_CLASS_RTTI( SMenuEvent );
	PROPERTY( m_eventName );
	PROPERTY( m_openedMenu );
	PROPERTY( m_openedJournalEntry );
END_CLASS_RTTI();