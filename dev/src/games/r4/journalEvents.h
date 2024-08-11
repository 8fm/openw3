#pragma once

#include "r4JournalManager.h"
//#include "..\..\..\common\core\names.h"

// ----------------------------------------------------------------------------------

class SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3BaseStatusEvent );

public:
	EJournalStatus m_oldStatus;
	EJournalStatus m_newStatus;

	SW3BaseStatusEvent() {}
	SW3BaseStatusEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3BaseStatusEvent )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class SW3JournalQuestStatusEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalQuestStatusEvent );

public:
	const CJournalQuest* m_quest;
	Bool                 m_silent;

	SW3JournalQuestStatusEvent() {}
	SW3JournalQuestStatusEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalQuestStatusEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalObjectiveStatusEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalObjectiveStatusEvent );

public:
	const CJournalQuestObjective* m_objective;
	Bool                          m_silent;

	SW3JournalObjectiveStatusEvent() {}
	SW3JournalObjectiveStatusEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalObjectiveStatusEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
	END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalTrackEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalTrackEvent );

public:
	const CJournalQuest* m_quest;

	SW3JournalTrackEvent() {}
};

BEGIN_CLASS_RTTI( SW3JournalTrackEvent )
	PARENT_CLASS( SJournalEvent )
	END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalQuestTrackEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalQuestTrackEvent );

public:
	const CJournalQuest* m_quest;

	SW3JournalQuestTrackEvent() {}
};

BEGIN_CLASS_RTTI( SW3JournalQuestTrackEvent )
	PARENT_CLASS( SJournalEvent )
	END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalQuestObjectiveTrackEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalQuestObjectiveTrackEvent );

public:
	const CJournalQuestObjective*	m_objective;

	SW3JournalQuestObjectiveTrackEvent() {}
};

BEGIN_CLASS_RTTI( SW3JournalQuestObjectiveTrackEvent )
	PARENT_CLASS( SJournalEvent )
	END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalQuestObjectiveCounterTrackEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalQuestObjectiveCounterTrackEvent );

public:
	const CJournalQuest* m_quest;
	const CJournalQuestObjective* m_questObjective;
	Int32 m_counter;

	SW3JournalQuestObjectiveCounterTrackEvent() {}
};

BEGIN_CLASS_RTTI( SW3JournalQuestObjectiveCounterTrackEvent )
	PARENT_CLASS( SJournalEvent )
	END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalHighlightEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalHighlightEvent );

public:
	const CJournalQuestObjective* m_objective;
	Int32 m_objectiveIndex;

	SW3JournalHighlightEvent() {}
};

BEGIN_CLASS_RTTI( SW3JournalHighlightEvent )
	PARENT_CLASS( SJournalEvent )
	END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalCharacterEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalCharacterEvent );

public:
	const CJournalCharacter* m_character;

	SW3JournalCharacterEvent() {}
	SW3JournalCharacterEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalCharacterEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalCharacterDescriptionEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalCharacterDescriptionEvent );

public:
	const CJournalCharacterDescription* m_characterDescription;

	SW3JournalCharacterDescriptionEvent() {}
	SW3JournalCharacterDescriptionEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalCharacterDescriptionEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalGlossaryEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalGlossaryEvent );

public:
	const CJournalGlossary* m_glossary;

	SW3JournalGlossaryEvent() {}
	SW3JournalGlossaryEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalGlossaryEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalGlossaryDescriptionEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalGlossaryDescriptionEvent );

public:
	const CJournalGlossaryDescription* m_glossaryDescription;

	SW3JournalGlossaryDescriptionEvent() {}
	SW3JournalGlossaryDescriptionEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalGlossaryDescriptionEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalTutorialEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalTutorialEvent );

public:
	const CJournalTutorial* m_tutorial;

	SW3JournalTutorialEvent() {}
	SW3JournalTutorialEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalTutorialEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalCreatureEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalCreatureEvent );

public:
	const CJournalCreature* m_creature;
	Bool                    m_silent;

	SW3JournalCreatureEvent() {}
	SW3JournalCreatureEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalCreatureEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalCreatureDescriptionEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalCreatureDescriptionEvent );

public:
	const CJournalCreatureDescriptionEntry* m_creatureDescription;
	Bool                                    m_silent;

	SW3JournalCreatureDescriptionEvent() {}
	SW3JournalCreatureDescriptionEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalCreatureDescriptionEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalStoryBookPageEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalStoryBookPageEvent );

public:
	const CJournalStoryBookPage* m_storyBookPage;

	SW3JournalStoryBookPageEvent() {}
	SW3JournalStoryBookPageEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalStoryBookPageEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalPlaceEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalPlaceEvent );

public:
	const CJournalPlace* m_place;

	SW3JournalPlaceEvent() {}
	SW3JournalPlaceEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalPlaceEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalPlaceDescriptionEvent : public SW3BaseStatusEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalPlaceDescriptionEvent );

public:
	const CJournalPlaceDescription* m_placeDescription;

	SW3JournalPlaceDescriptionEvent() {}
	SW3JournalPlaceDescriptionEvent( const SJournalStatusEvent& baseEvent );
};

BEGIN_CLASS_RTTI( SW3JournalPlaceDescriptionEvent )
	PARENT_CLASS( SW3BaseStatusEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalHuntingQuestAddedEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalHuntingQuestAddedEvent );

public:
	const CJournalQuest* m_quest;

	SW3JournalHuntingQuestAddedEvent() {}
};

BEGIN_CLASS_RTTI( SW3JournalHuntingQuestAddedEvent )
	PARENT_CLASS( SJournalEvent )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class SW3JournalHuntingQuestClueFoundEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SW3JournalHuntingQuestClueFoundEvent );

public:

	// The Hunting quest
	const CJournalQuest* m_quest;

	// The Hunting Clue that has been found
	const CJournalCreatureHuntingClue* m_creatureClue;

	SW3JournalHuntingQuestClueFoundEvent() {}
};

BEGIN_CLASS_RTTI( SW3JournalHuntingQuestClueFoundEvent )
	PARENT_CLASS( SJournalEvent )
END_CLASS_RTTI();


// ----------------------------------------------------------------------------------

class IW3JournalEventListener : public IJournalEventListener
{
public:
	IW3JournalEventListener();
	virtual ~IW3JournalEventListener();

	// Quests
	virtual void OnJournalEvent( const SW3JournalQuestStatusEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalObjectiveStatusEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalTrackEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalQuestTrackEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalQuestObjectiveTrackEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalQuestObjectiveCounterTrackEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalHighlightEvent& event ) {}

    // Journal
	virtual void OnJournalEvent( const SW3JournalCharacterEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalCharacterDescriptionEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalGlossaryEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalGlossaryDescriptionEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalTutorialEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalCreatureEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalCreatureDescriptionEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalStoryBookPageEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalPlaceEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalPlaceDescriptionEvent& event ) {}

	// Tracked Quest - You will only ever receive events for the quest that is currently visible to the user

	virtual void OnJournalEvent( const SW3JournalHuntingQuestAddedEvent& event ) {}
	virtual void OnJournalEvent( const SW3JournalHuntingQuestClueFoundEvent& event ) {}

private:
	virtual void OnJournalBaseEvent( const SJournalEvent& event );
};
