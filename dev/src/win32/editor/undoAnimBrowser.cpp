/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#include "build.h"
#include "undoAnimBrowser.h"
#include "animBrowser.h"
#include "animEventsTimeline.h"
#include "../../common/core/memoryFileReader.h"
#include "../../common/core/memoryFileWriter.h"

#define for_each(obj, container) \
	for ( auto obj = container.Begin(), end = container.End(); obj != end; ++ obj )

#define for_each_ptr(obj, container) \
	for ( auto iObj = container.Begin(), end = container.End(); iObj != end; ++ iObj ) \
	if ( auto obj = *iObj )

IMPLEMENT_ENGINE_CLASS( CUndoAnimBrowserAnimChange );

/*static*/ 
void CUndoAnimBrowserAnimChange::CreateStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor )
{
	CUndoAnimBrowserAnimChange* step = new CUndoAnimBrowserAnimChange( undoManager, editor );
	step->PushStep();
}

CUndoAnimBrowserAnimChange::CUndoAnimBrowserAnimChange( CEdUndoManager& undoManager, CEdAnimBrowser* editor )
	: IUndoStep( undoManager )
	, m_editor( editor )
{
	m_animEntry = m_editor->m_selectedAnimation;
}

void CUndoAnimBrowserAnimChange::DoStep()
{
	CSkeletalAnimationSetEntry* prevAnim = m_editor->m_selectedAnimation;
	m_editor->SelectAnimation( m_animEntry );
	m_editor->RefreshPage();
	m_animEntry = prevAnim;
}

/*virtual*/ 
void CUndoAnimBrowserAnimChange::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoAnimBrowserAnimChange::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoAnimBrowserAnimChange::GetName()
{
	return TXT("animation change");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SUndoAnimBrowserEventRef::SUndoAnimBrowserEventRef( CExtAnimEvent* event )
{
	m_eventName = event->GetEventName();
	m_trackName = event->GetTrackName();
	m_startTime = event->GetStartTime();
}

void SUndoAnimBrowserEventRef::RemoveFrom( CSkeletalAnimationSetEntry* entry )
{
	TDynArray< CExtAnimEvent* > events;
	entry->GetAllEvents( events );
	for_each_ptr( event, events )
	{
		if ( Matches( event ) )
		{
			entry->RemoveEvent( event );
		}
	}
}

void SUndoAnimBrowserEventRef::MoveTo( CSkeletalAnimationSetEntry* entry, Float moveTo )
{
	TDynArray< CExtAnimEvent* > events;
	entry->GetAllEvents( events );
	for_each_ptr( event, events )
	{
		if ( Matches( event ) )
		{
			event->SetStartTime( moveTo );
		}
	}
}

Bool SUndoAnimBrowserEventRef::Matches( CExtAnimEvent* event ) const
{
	return event->GetStartTime() == m_startTime &&
		   event->GetEventName() == m_eventName &&
		   event->GetTrackName() == m_trackName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CUndoAnimBrowserPasteEvents );

//////////////////////////////////////////////////////////////////////////

SUndoAnimBrowserPasteEventsEntryInfo::SUndoAnimBrowserPasteEventsEntryInfo( CSkeletalAnimationSetEntry* entry, TDynArray< CExtAnimEvent* > const & events )
{
	m_entry = entry;
	StoreEvents( events );
}

void SUndoAnimBrowserPasteEventsEntryInfo::StoreEvents( TDynArray< CExtAnimEvent* > const & events )
{
	for_each_ptr( event, events )
	{
		m_existingEvents.PushBack( SUndoAnimBrowserEventRef( event ) );
	}
}

void SUndoAnimBrowserPasteEventsEntryInfo::DoUndo( CEdAnimBrowser* editor )
{
	if ( m_existingEvents.Size() )
	{
		for_each( existing, m_existingEvents )
		{
			existing->RemoveFrom( m_entry );
		}
		m_existingEvents.ClearFast();
	}
	Refresh( editor );
}

void SUndoAnimBrowserPasteEventsEntryInfo::DoRedo( CEdAnimBrowser* editor, TDynArray< Uint8 > & serializedItem, CName const & relativeToEventName, Float relPos )
{
	CMemoryFileReader reader( serializedItem, 0 );
	TDynArray< CExtAnimEvent* > events;
	editor->PasteTimelineEventsToAnimation( reader, m_entry, relativeToEventName, relPos, &events );
	StoreEvents( events );
	Refresh( editor );
}

void SUndoAnimBrowserPasteEventsEntryInfo::Refresh( CEdAnimBrowser* editor )
{
	if ( editor->m_selectedAnimation == m_entry )
	{
		editor->SelectAnimation( m_entry );
	}
}

//////////////////////////////////////////////////////////////////////////

CUndoAnimBrowserPasteEvents::CUndoAnimBrowserPasteEvents( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CMemoryFileReader & source, CName const & relativeToEventName, Float relPos )
: IUndoStep( undoManager )
, m_editor( editor )
, m_relativeToEventName( relativeToEventName )
, m_relPos( relPos )
{
	CMemoryFileWriter writer( m_serializedItem );
	source.Seek(0);

	// create copy
	Uint32 numItems;
	source << numItems;
	writer << numItems;
	for( Uint32 i = 0; i < numItems; ++i )
	{
		CExtAnimEvent* animEvent = CEdAnimEventsTimeline::DeserializeAsEvent( source );
		CEdAnimEventsTimeline::SerializeEvent( animEvent, writer );
		delete animEvent;
	}
}

void CUndoAnimBrowserPasteEvents::PrepareCreationStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CMemoryFileReader & source, CName const & relativeToEventName, Float relPos )
{
	CUndoAnimBrowserPasteEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserPasteEvents >();

	if ( !step )
	{
		step = new CUndoAnimBrowserPasteEvents( undoManager, editor, source, relativeToEventName, relPos );
		undoManager.SetStepToAdd( step );
	}
}

void CUndoAnimBrowserPasteEvents::PrepareAddForEntry( CEdUndoManager& undoManager, CSkeletalAnimationSetEntry* entry, TDynArray< CExtAnimEvent* > const & events )
{
	if ( CUndoAnimBrowserPasteEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserPasteEvents >() )
	{
		step->m_entries.PushBack( SUndoAnimBrowserPasteEventsEntryInfo( entry, events ) );
	}
}

void CUndoAnimBrowserPasteEvents::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoAnimBrowserPasteEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserPasteEvents >() )
	{
		step->PushStep();
	}
}

void CUndoAnimBrowserPasteEvents::DoUndo()
{
	for_each( entry, m_entries )
	{
		entry->DoUndo( m_editor );
	}
}

void CUndoAnimBrowserPasteEvents::DoRedo()
{
	for_each( entry, m_entries )
	{
		entry->DoRedo( m_editor, m_serializedItem, m_relativeToEventName, m_relPos );
	}
}

String CUndoAnimBrowserPasteEvents::GetName()
{
	return TXT("Paste events relative to event");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CUndoAnimBrowserFindAndRemoveEvents );

//////////////////////////////////////////////////////////////////////////

SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo::SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo( CSkeletalAnimationSetEntry* entry, CExtAnimEvent* event )
{
	m_entry = entry;
	m_eventRef = SUndoAnimBrowserEventRef( event );
	CMemoryFileWriter writer( m_serializedItem );
	CEdAnimEventsTimeline::SerializeEvent( event, writer );
}

void SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo::DoUndo( CEdAnimBrowser* editor )
{
	CMemoryFileReader reader( m_serializedItem, 0 );

	CExtAnimEvent* animEvent = CEdAnimEventsTimeline::DeserializeAsEvent( reader );
	m_entry->AddEvent( animEvent );

	Refresh( editor );
}

void SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo::DoRedo( CEdAnimBrowser* editor )
{
	m_eventRef.RemoveFrom( m_entry );
	Refresh( editor );
}

void SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo::Refresh( CEdAnimBrowser* editor )
{
	if ( editor->m_selectedAnimation == m_entry )
	{
		editor->SelectAnimation( m_entry );
	}
}

//////////////////////////////////////////////////////////////////////////

CUndoAnimBrowserFindAndRemoveEvents::CUndoAnimBrowserFindAndRemoveEvents( CEdUndoManager& undoManager, CEdAnimBrowser* editor )
: IUndoStep( undoManager )
, m_editor( editor )
{
}

void CUndoAnimBrowserFindAndRemoveEvents::PrepareCreationStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor )
{
	CUndoAnimBrowserFindAndRemoveEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserFindAndRemoveEvents >();

	if ( !step )
	{
		step = new CUndoAnimBrowserFindAndRemoveEvents( undoManager, editor );
		undoManager.SetStepToAdd( step );
	}
}

void CUndoAnimBrowserFindAndRemoveEvents::PrepareAddRemovedEvent( CEdUndoManager& undoManager, CSkeletalAnimationSetEntry* entry, CExtAnimEvent* event )
{
	if ( CUndoAnimBrowserFindAndRemoveEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserFindAndRemoveEvents >() )
	{
		step->m_entries.PushBack( SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo( entry, event ) );
	}
}

void CUndoAnimBrowserFindAndRemoveEvents::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoAnimBrowserFindAndRemoveEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserFindAndRemoveEvents >() )
	{
		step->PushStep();
	}
}

void CUndoAnimBrowserFindAndRemoveEvents::DoUndo()
{
	for_each( entry, m_entries )
	{
		entry->DoUndo( m_editor );
	}
}

void CUndoAnimBrowserFindAndRemoveEvents::DoRedo()
{
	for_each( entry, m_entries )
	{
		entry->DoRedo( m_editor );
	}
}

String CUndoAnimBrowserFindAndRemoveEvents::GetName()
{
	return TXT("Find and remove events");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CUndoAnimBrowserFindAndMoveEvents );

CUndoAnimBrowserFindAndMoveEvents::CUndoAnimBrowserFindAndMoveEvents( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CName const & eventName, Float relPos )
: IUndoStep( undoManager )
, m_editor( editor )
, m_eventName( eventName )
, m_relPos( relPos )
{
}

void CUndoAnimBrowserFindAndMoveEvents::PrepareCreationStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CName const & eventName, Float relPos )
{
	CUndoAnimBrowserFindAndMoveEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserFindAndMoveEvents >();

	if ( !step )
	{
		step = new CUndoAnimBrowserFindAndMoveEvents( undoManager, editor, eventName, relPos );
		undoManager.SetStepToAdd( step );
	}
}

void CUndoAnimBrowserFindAndMoveEvents::PrepareAddForEntry( CEdUndoManager& undoManager, CSkeletalAnimationSetEntry* entry )
{
	if ( CUndoAnimBrowserFindAndMoveEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserFindAndMoveEvents >() )
	{
		step->m_entries.PushBack( entry );
	}
}

void CUndoAnimBrowserFindAndMoveEvents::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoAnimBrowserFindAndMoveEvents* step = undoManager.SafeGetStepToAdd< CUndoAnimBrowserFindAndMoveEvents >() )
	{
		step->PushStep();
	}
}

void CUndoAnimBrowserFindAndMoveEvents::DoUndo()
{
	for_each_ptr( entry, m_entries )
	{
		MoveBy( entry, -m_relPos );
	}
}

void CUndoAnimBrowserFindAndMoveEvents::DoRedo()
{
	for_each_ptr( entry, m_entries )
	{
		MoveBy( entry, m_relPos );
	}
}

void CUndoAnimBrowserFindAndMoveEvents::MoveBy( CSkeletalAnimationSetEntry* entry, Float moveBy ) const
{
	TDynArray< CExtAnimEvent* > events;
	entry->GetAllEvents( events );
	for_each_ptr( event, events )
	{
		if ( event->GetEventName() == m_eventName )
		{
			event->SetStartTime( event->GetStartTime() + moveBy );
		}
	}

	if ( m_editor->m_selectedAnimation == entry )
	{
		m_editor->SelectAnimation( entry );
	}
}

String CUndoAnimBrowserFindAndMoveEvents::GetName()
{
	return TXT("Find and move events");
}

#undef for_each
#undef for_each_ptr
