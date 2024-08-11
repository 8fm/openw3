#pragma once

#include "../../common/game/questCondition.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum EOpenedUIPanel
{
	OUP_Unknown = -1,
	OUP_Alchemy,
	OUP_Inventory,
	OUP_Container,
	OUP_Shop,
	OUP_Preparation,
	OUP_WorldMap,
	OUP_Crafting,
	OUP_Character,
	OUP_Journal,
};

BEGIN_ENUM_RTTI( EOpenedUIPanel );
	ENUM_OPTION( OUP_Unknown );
	ENUM_OPTION( OUP_Alchemy );
	ENUM_OPTION( OUP_Inventory );
	ENUM_OPTION( OUP_Container );
	ENUM_OPTION( OUP_Shop );
	ENUM_OPTION( OUP_Preparation );
	ENUM_OPTION( OUP_WorldMap );
	ENUM_OPTION( OUP_Crafting );
	ENUM_OPTION( OUP_Character );
	ENUM_OPTION( OUP_Journal );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum EOpenedUIJournalTab
{
	OUJT_Unknown = -1,
	OUJT_Quests,
	OUJT_Bestiary,
	OUJT_Storybook,
	OUJT_Characters,
	OUJT_Glossary,
	OUJT_Tutorial,
};

BEGIN_ENUM_RTTI( EOpenedUIJournalTab );
	ENUM_OPTION( OUJT_Unknown );
	ENUM_OPTION( OUJT_Quests );
	ENUM_OPTION( OUJT_Bestiary );
	ENUM_OPTION( OUJT_Storybook );
	ENUM_OPTION( OUJT_Characters );
	ENUM_OPTION( OUJT_Glossary );
	ENUM_OPTION( OUJT_Tutorial );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////////////

class IUIConditionType : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IUIConditionType, CObject )

protected:
	Bool		m_isFulfilled;

public:
	IUIConditionType():	m_isFulfilled( false ) {}
	virtual ~IUIConditionType() {}
	virtual void OnEvent( const SMenuEvent& event ) {};
	virtual Bool OnIsFulfilled()	{    return m_isFulfilled;    }

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return String::EMPTY; }
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( IUIConditionType )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CQCIsOpenedMenu : public IUIConditionType
{
	DECLARE_ENGINE_CLASS( CQCIsOpenedMenu, IUIConditionType, 0 )

private:
	CName			m_menuToBeOpened;

public:
	CQCIsOpenedMenu() {}
	virtual void OnEvent( const SMenuEvent& event );

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Opened UI panel" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCIsOpenedMenu )
	PARENT_CLASS( IUIConditionType )
	PROPERTY_EDIT( m_menuToBeOpened, TXT("Menu to be opened") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CQCIsOpenedJournalEntry : public IUIConditionType
{
	DECLARE_ENGINE_CLASS( CQCIsOpenedJournalEntry, IUIConditionType, 0 )

private:
	THandle< CJournalPath >		m_journalEntryToBeOpened;

public:
	CQCIsOpenedJournalEntry(): m_journalEntryToBeOpened( NULL ) {}
	virtual void OnEvent( const SMenuEvent& event );

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Opened journal tab" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCIsOpenedJournalEntry )
	PARENT_CLASS( IUIConditionType )
	PROPERTY_CUSTOM_EDIT( m_journalEntryToBeOpened, TXT("Journal entry to be opened"), TXT( "JournalPropertyBrowserAll" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CQCIsSentCustomUIEvent : public IUIConditionType
{
	DECLARE_ENGINE_CLASS( CQCIsSentCustomUIEvent, IUIConditionType, 0 )

private:
	CName						m_eventName;

public:
	CQCIsSentCustomUIEvent() {}
	virtual void OnEvent( const SMenuEvent& event );

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Sent custom UI event" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCIsSentCustomUIEvent )
	PARENT_CLASS( IUIConditionType )
	PROPERTY_EDIT( m_eventName, TXT("Event name to be sent") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CQCIsObjectiveHighlighted : public IUIConditionType
{
	DECLARE_ENGINE_CLASS( CQCIsObjectiveHighlighted, IUIConditionType, 0 )

private:
	THandle< CJournalPath >	m_objectiveEntry;

public:
	CQCIsObjectiveHighlighted() {}
	virtual Bool OnIsFulfilled();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Highlighted quest objective" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCIsObjectiveHighlighted )
	PARENT_CLASS( IUIConditionType )
	PROPERTY_CUSTOM_EDIT( m_objectiveEntry, TXT("Quest objective to be highlighted"), TXT( "JournalPropertyBrowserQuest_Objective" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CQuestUICondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestUICondition, IQuestCondition, 0 )

private:
	IUIConditionType*			m_checkType;

public:
	CQuestUICondition() {}
	virtual ~CQuestUICondition() {}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Used fast travel condition" ) ); 
	}
#endif

	// Called by the quest system when the world map fires an event
	void OnEvent( const SMenuEvent& event );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestUICondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_INLINED( m_checkType, TXT( "Condition" ) )
END_CLASS_RTTI()
