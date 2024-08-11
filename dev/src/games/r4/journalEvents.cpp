#include "build.h"
#include "journalEvents.h"
#include "r4JournalManager.h"

// ----------------------------------------------------------------------------------

IW3JournalEventListener::IW3JournalEventListener()
{

}

IW3JournalEventListener::~IW3JournalEventListener()
{

}

void IW3JournalEventListener::OnJournalBaseEvent( const SJournalEvent& baseEvent )
{
	// #define LOG_JOURNAL_EVENTS  

	#ifdef LOG_JOURNAL_EVENTS
		static Uint32 numEvents = 0;
		RED_LOG( Journal, TXT("Event %ld"), numEvents++ );	
	#endif

	if( baseEvent.GetClass()->IsA< SJournalStatusEvent >() )
	{
		const SJournalStatusEvent& statusEvent = static_cast< const SJournalStatusEvent& >( baseEvent );

		if( statusEvent.m_entry->IsA< CJournalQuest >() )
		{
			SW3JournalQuestStatusEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalQuestStatusEvent, quest: %s"), event.m_quest ? event.m_quest->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalQuestObjective >() )
		{
			SW3JournalObjectiveStatusEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalObjectiveStatusEvent, objective: %s"), event.m_objective ? event.m_objective->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalCreature >() )
		{
			SW3JournalCreatureEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalCreatureEvent, creature: %s"), event.m_creature ? event.m_creature->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalCreatureDescriptionEntry >() )
		{
			SW3JournalCreatureDescriptionEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalCreatureDescriptionEvent, description: %s"), event.m_creatureDescription ? event.m_creatureDescription->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalStoryBookPage >() )
		{
			SW3JournalStoryBookPageEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalStoryBookPageEvent, page: %s"), event.m_storyBookPage ? event.m_storyBookPage->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalPlace >() )
		{
			SW3JournalPlaceEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalPlaceEvent, page: %s"), event.m_place ? event.m_place->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalPlaceDescription >() )
		{
			SW3JournalPlaceDescriptionEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalPlaceDescriptionEvent, page: %s"), event.m_placeDescription ? event.m_placeDescription->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalTutorial >() )
		{
			SW3JournalTutorialEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalTutorialEvent, tutorial: %s"), event.m_tutorial ? event.m_tutorial->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalCharacter >() )
		{
			SW3JournalCharacterEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalCharacterEvent, character: %s"), event.m_character ? event.m_character->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalCharacterDescription >() )
		{
			SW3JournalCharacterDescriptionEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalCharacterDescriptionEvent, description: %s"), event.m_characterDescription ? event.m_characterDescription->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalGlossary >() )
		{
			SW3JournalGlossaryEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalGlossaryEvent, glossary: %s"), event.m_glossary ? event.m_glossary->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
		else if( statusEvent.m_entry->IsA< CJournalGlossaryDescription >() )
		{
			SW3JournalGlossaryDescriptionEvent event( statusEvent );
			#ifdef LOG_JOURNAL_EVENTS
				RED_LOG( Journal, TXT("SW3JournalGlossaryDescriptionEvent, description: %s"),  event.m_glossaryDescription ? event.m_glossaryDescription->GetFriendlyName().AsChar() : TXT("nullptr") );
			#endif
			OnJournalEvent( event );
		}
	}
	else if( baseEvent.GetClass()->IsA< SW3JournalTrackEvent >() )
	{
		const SW3JournalTrackEvent& event = static_cast< const SW3JournalTrackEvent& >( baseEvent );
		#ifdef LOG_JOURNAL_EVENTS
			RED_LOG( Journal, TXT("SW3JournalTrackEvent, quest: %s"), event.m_quest ? event.m_quest->GetFriendlyName().AsChar() : TXT("nullptr") );
		#endif
		OnJournalEvent( event );
	}
	else if( baseEvent.GetClass()->IsA< SW3JournalQuestTrackEvent >() )
	{
		const SW3JournalQuestTrackEvent& event = static_cast< const SW3JournalQuestTrackEvent& >( baseEvent );
		#ifdef LOG_JOURNAL_EVENTS
			RED_LOG( Journal, TXT("SW3JournalQuestTrackEvent, quest: %s"), event.m_quest ? event.m_quest->GetFriendlyName().AsChar() : TXT("nullptr") );
		#endif
		OnJournalEvent( event );
	}
	else if( baseEvent.GetClass()->IsA< SW3JournalQuestObjectiveTrackEvent >() )
	{
		const SW3JournalQuestObjectiveTrackEvent& event = static_cast< const SW3JournalQuestObjectiveTrackEvent& >( baseEvent );
		#ifdef LOG_JOURNAL_EVENTS
			RED_LOG( Journal, TXT("SW3JournalQuestTrackEvent, objective: %s"), event.m_objective ? event.m_objective->GetFriendlyName().AsChar() : TXT("nullptr") );
		#endif
		OnJournalEvent( event );
	}
	else if( baseEvent.GetClass()->IsA< SW3JournalQuestObjectiveCounterTrackEvent >() )
	{
		const SW3JournalQuestObjectiveCounterTrackEvent& event = static_cast< const SW3JournalQuestObjectiveCounterTrackEvent& >( baseEvent );
		#ifdef LOG_JOURNAL_EVENTS
			RED_LOG( Journal, TXT("SW3JournalQuestObjectiveCounterTrackEvent, quest: %s objective: %s, couter:%ld"), event.m_quest ? event.m_quest->GetFriendlyName().AsChar() : TXT("nullptr"), event.m_questObjective ? event.m_questObjective->GetFriendlyName().AsChar() : TXT("nullptr"), event.m_counter );
		#endif
		OnJournalEvent( event );
	}
	else if( baseEvent.GetClass()->IsA< SW3JournalHighlightEvent >() )
	{
		const SW3JournalHighlightEvent& event = static_cast< const SW3JournalHighlightEvent& >( baseEvent );
		#ifdef LOG_JOURNAL_EVENTS
			RED_LOG( Journal, TXT("SW3JournalHighlightEvent, objective: %s, previous objective: %s"), event.m_objective ? event.m_objective->GetFriendlyName().AsChar() : TXT("nullptr"), event.m_previousObjective ? event.m_previousObjective->GetFriendlyName().AsChar() : TXT("nullptr") );
		#endif
		OnJournalEvent( event );
	}
	else if( baseEvent.GetClass()->IsA< SW3JournalHuntingQuestClueFoundEvent >() )
	{
		const SW3JournalHuntingQuestClueFoundEvent& event = static_cast< const SW3JournalHuntingQuestClueFoundEvent& >( baseEvent );
		#ifdef LOG_JOURNAL_EVENTS
			RED_LOG( Journal, TXT("SW3JournalHuntingQuestClueFoundEvent, quest: %s clue: %s"), event.m_quest ? event.m_quest->GetFriendlyName().AsChar() : TXT("nullptr"), event.m_creatureClue ? event.m_creatureClue->GetFriendlyName().AsChar() : TXT("nullptr") );
		#endif
		OnJournalEvent( event );
	}
	else if( baseEvent.GetClass()->IsA< SW3JournalHuntingQuestAddedEvent >() )
	{
		const SW3JournalHuntingQuestAddedEvent& event = static_cast< const SW3JournalHuntingQuestAddedEvent& >( baseEvent );
		#ifdef LOG_JOURNAL_EVENTS
			RED_LOG( Journal, TXT("SW3JournalHuntingQuestAddedEvent, quest: %s"), event.m_quest ? event.m_quest->GetFriendlyName().AsChar() : TXT("nullptr") );
		#endif
		OnJournalEvent( event );
	}
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3BaseStatusEvent );
SW3BaseStatusEvent::SW3BaseStatusEvent( const SJournalStatusEvent& baseEvent )
:	m_oldStatus( baseEvent.m_oldStatus ),
	m_newStatus( baseEvent.m_newStatus )
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SW3JournalQuestStatusEvent );
SW3JournalQuestStatusEvent::SW3JournalQuestStatusEvent( const SJournalStatusEvent& baseEvent )
:	SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalQuest >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_quest = Cast< CJournalQuest >( baseEvent.m_entry );
	m_silent = baseEvent.m_silent;
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalObjectiveStatusEvent );
SW3JournalObjectiveStatusEvent::SW3JournalObjectiveStatusEvent( const SJournalStatusEvent& baseEvent )
:	SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalQuestObjective >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_objective = Cast< CJournalQuestObjective >( baseEvent.m_entry );
	m_silent = baseEvent.m_silent;
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalCreatureEvent );
SW3JournalCreatureEvent::SW3JournalCreatureEvent( const SJournalStatusEvent& baseEvent )
	: SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalCreature >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_creature = Cast< CJournalCreature >( baseEvent.m_entry );
	m_silent = baseEvent.m_silent;
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalCreatureDescriptionEvent );
SW3JournalCreatureDescriptionEvent::SW3JournalCreatureDescriptionEvent( const SJournalStatusEvent& baseEvent )
	: SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalCreatureDescriptionEntry >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_creatureDescription = Cast< CJournalCreatureDescriptionEntry >( baseEvent.m_entry );
	m_silent = baseEvent.m_silent;
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalStoryBookPageEvent );
SW3JournalStoryBookPageEvent::SW3JournalStoryBookPageEvent( const SJournalStatusEvent& baseEvent )
	: SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalStoryBookPage >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_storyBookPage = Cast< CJournalStoryBookPage >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalPlaceEvent );
SW3JournalPlaceEvent::SW3JournalPlaceEvent( const SJournalStatusEvent& baseEvent )
	: SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalPlace >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_place = Cast< CJournalPlace >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalPlaceDescriptionEvent );
SW3JournalPlaceDescriptionEvent::SW3JournalPlaceDescriptionEvent( const SJournalStatusEvent& baseEvent )
	: SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalPlaceDescription >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_placeDescription = Cast< CJournalPlaceDescription >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalTutorialEvent );
SW3JournalTutorialEvent::SW3JournalTutorialEvent( const SJournalStatusEvent& baseEvent )
	:	SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalTutorial >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_tutorial = Cast< CJournalTutorial >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalCharacterEvent );
SW3JournalCharacterEvent::SW3JournalCharacterEvent( const SJournalStatusEvent& baseEvent )
	:	SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalCharacter >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_character = Cast< CJournalCharacter >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalCharacterDescriptionEvent );
SW3JournalCharacterDescriptionEvent::SW3JournalCharacterDescriptionEvent( const SJournalStatusEvent& baseEvent )
	:	SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalCharacterDescription >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_characterDescription = Cast< CJournalCharacterDescription >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalGlossaryEvent );
SW3JournalGlossaryEvent::SW3JournalGlossaryEvent( const SJournalStatusEvent& baseEvent )
	:	SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalGlossary >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_glossary = Cast< CJournalGlossary >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalGlossaryDescriptionEvent );
SW3JournalGlossaryDescriptionEvent::SW3JournalGlossaryDescriptionEvent( const SJournalStatusEvent& baseEvent )
	:	SW3BaseStatusEvent( baseEvent )
{
	ASSERT( baseEvent.m_entry->IsA< CJournalGlossaryDescription >(), TXT( "baseEvent entry is incorrect type: '%ls'" ), baseEvent.m_entry->GetClass()->GetName().AsString().AsChar() );

	m_glossaryDescription = Cast< CJournalGlossaryDescription >( baseEvent.m_entry );
}

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalTrackEvent );

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalQuestTrackEvent );

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalQuestObjectiveTrackEvent );

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalQuestObjectiveCounterTrackEvent );

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalHighlightEvent );

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalHuntingQuestAddedEvent );

// ----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( SW3JournalHuntingQuestClueFoundEvent );

// ----------------------------------------------------------------------------------

