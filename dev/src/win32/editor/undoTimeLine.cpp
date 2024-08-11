/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#include "build.h"
#include "undoProperty.h"
#include "undoTimeLine.h"
#include "timeline.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/memoryFileReader.h"

STimelineItemLayout::STimelineItemLayout( ITimelineItem* item )
	: m_start( item->GetStart() )
	, m_duration( item->IsDuration() ? item->GetDuration() : 0 )
	, m_isDuration( item->IsDuration() )
	{ }

void STimelineItemLayout::Apply( ITimelineItem* item ) const
{
	item->SetStart( m_start, true );

	if ( m_isDuration )
	{
		item->SetDuration( m_duration );
	}
}

Bool STimelineItemLayout::operator==( const STimelineItemLayout& r ) const
{ 
	return m_start == r.m_start && m_duration == r.m_duration; 
}

Bool STimelineItemLayout::operator< ( const STimelineItemLayout& r ) const
{ 
	return m_start == r.m_start ? m_duration < r.m_duration : m_start < r.m_start; 
}

// ----------------------------------------------------------

STimelineItemStamp::STimelineItemStamp( ITimelineItem* item )
	: m_trackName( item->GetTrackName() ), m_layout( item )
	{ }

Bool STimelineItemStamp::operator==( const STimelineItemStamp& r ) const
{ 
	return m_trackName == r.m_trackName && m_layout == r.m_layout; 
}

Bool STimelineItemStamp::operator< ( const STimelineItemStamp& r ) const
{ 
	return m_trackName == r.m_trackName ? m_layout < r.m_layout : m_trackName < r.m_trackName; 
}

// --------------------------------------

STimelineItemInfo::STimelineItemInfo( ITimelineItem* item ) 
	: m_existingItem( STimelineItemStamp( item ) ) 
	{ }

STimelineItemInfo::STimelineItemInfo( ITimelineItem* item, CEdTimeline* timeLine ) 
	: m_existingItem( STimelineItemStamp( item ) ) 
{ 
}

// --------------------------------------

void CUndoTimelineStep::Invalidate()
{
	m_timeline->NotifyBufferIsInvalid();
}

ITimelineItem* CUndoTimelineStep::FindItem( const STimelineItemStamp& stamp ) const
{
	const TDynArray< ITimelineItem* >& items = m_timeline->m_items;

	for ( Uint8 i=0; i<items.Size(); ++i )
	{
		STimelineItemLayout layout( items[i] );
		if ( items[i]->GetTrackName() == stamp.m_trackName && layout == stamp.m_layout )
		{
			return items[i];
		}
	}

	ASSERT ( false, TXT("Cannot find event by its stamp") );
	return NULL;
}

// --------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoTimelineItemExistance );

/*static*/
CUndoTimelineItemExistance* CUndoTimelineItemExistance::PrepareStep( CEdUndoManager& undoManager, CEdTimeline* timeline )
{
	CUndoTimelineItemExistance* step = undoManager.SafeGetStepToAdd< CUndoTimelineItemExistance >();

	if ( !step )
	{
		step = new CUndoTimelineItemExistance( undoManager, timeline );
		undoManager.SetStepToAdd( step );
	}

	return step;
}

CUndoTimelineItemExistance::CUndoTimelineItemExistance( CEdUndoManager& undoManager, CEdTimeline* timeline )
	: CUndoTimelineStep( undoManager, timeline )
{
}

/*static*/ 
void CUndoTimelineItemExistance::PrepareCreationStep( CEdUndoManager& undoManager, CEdTimeline* timeline, ITimelineItem* item )
{
	CUndoTimelineItemExistance* step = PrepareStep( undoManager, timeline );

	STimelineItemInfo info( item );
	step->m_created.PushBack( info );
}

/*static*/ 
void CUndoTimelineItemExistance::PrepareDeletionStep( CEdUndoManager& undoManager, CEdTimeline* timeline, ITimelineItem* item )
{
	CUndoTimelineItemExistance* step = PrepareStep( undoManager, timeline );

	STimelineItemInfo info( item );
	
	CMemoryFileWriter writer( info.m_serializedItem );
	step->GetTimeLine()->SerializeItem( item, writer );

	step->m_deleted.PushBack( info );
}

/*static*/ 
void CUndoTimelineItemExistance::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoTimelineItemExistance* step = undoManager.SafeGetStepToAdd< CUndoTimelineItemExistance >() )
	{
		step->PushStep();
	}
}

void CUndoTimelineItemExistance::DoCreationOn( TDynArray< STimelineItemInfo >& infos )
{
	CEdTimeline* timeline = GetTimeLine();

	for ( Uint8 i=0; i < infos.Size(); ++i )
	{
		CMemoryFileReader reader( infos[i].m_serializedItem, 0 );
		ITimelineItem* item = timeline->DeserializeItem( reader );

		infos[i].m_serializedItem.Clear();
		timeline->AddItem( item );
	}
}

void CUndoTimelineItemExistance::DoDeletionOn( TDynArray< STimelineItemInfo >& infos )
{
	CEdTimeline* timeline = GetTimeLine();

	for ( Uint8 i=0; i < infos.Size(); ++i )
	{
		if ( ITimelineItem* item = FindItem( infos[i].m_existingItem ) )
		{
			ASSERT ( infos[i].m_serializedItem.Empty() );
			CMemoryFileWriter writer( infos[i].m_serializedItem );
			timeline->SerializeItem( item, writer );

			timeline->RemoveItem( item );
			delete item;
		}
	}
}

/*virtual*/ 
void CUndoTimelineItemExistance::DoUndo()
{
	DoDeletionOn( m_created );
	DoCreationOn( m_deleted );
	Invalidate();
}

/*virtual*/ 
void CUndoTimelineItemExistance::DoRedo()
{
	DoDeletionOn( m_deleted );
	DoCreationOn( m_created );
	Invalidate();
}

/*virtual*/ 
String CUndoTimelineItemExistance::GetName()
{
	if ( !m_created.Empty() )
	{
		if ( !m_deleted.Empty() )
		{
			return TXT("creating and removing items");
		}
		else
		{
			return TXT("creating items");
		}
	}
	else
	{
		return TXT("removing items");
	}
}

// ----------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoTimelineItemLayout );

CUndoTimelineItemLayout::CUndoTimelineItemLayout( CEdUndoManager& undoManager, CEdTimeline* timeline )
	: CUndoTimelineStep( undoManager, timeline )
{
}

void CUndoTimelineItemLayout::PrepareStep( CEdUndoManager& undoManager, CEdTimeline* timeline, ITimelineItem* item )
{
	CUndoTimelineItemLayout* step = undoManager.SafeGetStepToAdd< CUndoTimelineItemLayout >();

	if ( !step )
	{
		step = new CUndoTimelineItemLayout( undoManager, timeline );
		undoManager.SetStepToAdd( step );
	}

	step->m_tmpInfos.Insert( item, STimelineItemLayout( item ) );
}

/*static*/ 
void CUndoTimelineItemLayout::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoTimelineItemLayout* step = undoManager.SafeGetStepToAdd< CUndoTimelineItemLayout >() )
	{
		for ( auto it = step->m_tmpInfos.Begin(); it != step->m_tmpInfos.End(); ++it )
		{
			STimelineItemStamp stamp( it->m_first );
			step->m_infos.Insert( stamp, it->m_second );
		}

		step->m_tmpInfos.Clear();
		step->PushStep();
	}
}

void CUndoTimelineItemLayout::DoStep()
{
	THashMap< STimelineItemStamp, STimelineItemLayout > newInfos;
	for ( auto it = m_infos.Begin(); it != m_infos.End(); ++it )
	{
		if ( ITimelineItem* item = FindItem( it->m_first ) )
		{
			it->m_second.Apply( item );
		
			STimelineItemStamp newStamp( item );
			newInfos.Insert( newStamp, it->m_first.m_layout );
		}
	}

	Swap( newInfos, m_infos );
	Invalidate();
}

/*virtual*/ 
void CUndoTimelineItemLayout::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoTimelineItemLayout::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoTimelineItemLayout::GetName()
{
	return TXT("moving/resizing events");
}

// -----------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoTimelineTrackExistance );

/*static*/ 
void CUndoTimelineTrackExistance::CreateCreationStep( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& trackName )
{
	CUndoTimelineTrackExistance* step = new CUndoTimelineTrackExistance( undoManager, timeline, trackName, true );
	step->PushStep();
}

/*static*/ 
void CUndoTimelineTrackExistance::CreateDeletionStep( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& trackName )
{
	CUndoTimelineTrackExistance* step = new CUndoTimelineTrackExistance( undoManager, timeline, trackName, false );
	step->StoreTrackItems();
	step->PushStep();
}

CUndoTimelineTrackExistance::CUndoTimelineTrackExistance( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& trackName, Bool creating )
	: CUndoTimelineStep( undoManager, timeline )
	, m_name( trackName )
	, m_creating( creating )
	{ }

void CUndoTimelineTrackExistance::StoreTrackItems()
{
	ASSERT ( m_infos.Empty() );

	CEdTimeline* timeLine = GetTimeLine();

	for ( Uint8 i=0; i < timeLine->m_items.Size(); ++i )
	{
		if ( timeLine->m_items[i]->GetTrackName() == m_name )
		{
			ITimelineItem* item = timeLine->m_items[i];
			
			STimelineItemInfo info( item );

			CMemoryFileWriter writer( info.m_serializedItem );
			timeLine->SerializeItem( item, writer );

			m_infos.PushBack( info );
		}
	}
}

void CUndoTimelineTrackExistance::DoStep( Bool creating )
{
	CEdTimeline* timeline = GetTimeLine();

	if ( creating )
	{
		timeline->AddTrack( m_name );

		for ( Uint8 i=0; i < m_infos.Size(); ++i )
		{
			CMemoryFileReader reader( m_infos[i].m_serializedItem, 0 );
			ITimelineItem* item = timeline->DeserializeItem( reader );
			m_infos[i].m_serializedItem.Clear();

			timeline->AddItem( item );
		}

		m_infos.Clear();
	}
	else
	{
		StoreTrackItems();
		Int32 idx = timeline->GetTrackIndex( m_name );
		ASSERT ( idx != -1 );
		timeline->m_tracks[ idx ]->Delete( timeline ); // removes all items
	}

	Invalidate();
}

 /*virtual*/ 
void CUndoTimelineTrackExistance::DoUndo()
{
	DoStep( !m_creating );
}

/*virtual*/ 
void CUndoTimelineTrackExistance::DoRedo()
{
	DoStep( m_creating );
}

/*virtual*/ 
String CUndoTimelineTrackExistance::GetName()
{
	return m_creating ? TXT("creating track") : TXT("removing track");
}

// -------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoTimelineTrackRename );

CUndoTimelineTrackRename::CUndoTimelineTrackRename( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& oldName, const String& newName )
	: CUndoTimelineStep( undoManager, timeline )
	, m_prevName( oldName )
	, m_curName( newName )
	{ }

/*static*/ 
void CUndoTimelineTrackRename::CreateStep( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& oldName, const String& newName )
{
	CUndoTimelineTrackRename* step = new CUndoTimelineTrackRename( undoManager, timeline, oldName, newName );
	step->PushStep();
}

void CUndoTimelineTrackRename::DoStep()
{
	GetTimeLine()->RenameTrack( m_curName, m_prevName );
	Swap( m_curName, m_prevName );
	Invalidate();
}

/*virtual*/ 
void CUndoTimelineTrackRename::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoTimelineTrackRename::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoTimelineTrackRename::GetName()
{
	return TXT("renaming track");
}
