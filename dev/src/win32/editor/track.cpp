// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "track.h"
#include "timeline.h"
#include "timelineImpl/drawGroupTracks.h"

void Track::Rename( CEdTimeline* timeline, const String& name, bool reAdd, Uint32 newDepth  )
{
	Uint32 nameLength = m_name.Size();

	Track* newGroup = NULL;
	if ( reAdd )
	{
		if ( m_depth >= 1 )
		{
			m_parent->m_children.RemoveFast( this );
		}
		timeline->m_tracks.RemoveFast( this );
		timeline->m_selectedTrack = -1;
		timeline->AddTrack( name );

		TimelineImpl::CDrawGroupTracks* drawGroup = timeline->GetTrackDrawGroup(this);
		drawGroup->RemoveTrack(this);
		drawGroup->AddTrack(newGroup);

		Int32 index = timeline->GetTrackIndex( name );
		ASSERT( index >= 0 );
		newGroup = timeline->m_tracks[ index ];
		newGroup->m_children = m_children;
		newGroup->m_isExpanded = m_isExpanded;
		newGroup->m_isGroup = m_isGroup;
		newDepth = newGroup->m_depth;
	}

	m_depth = newDepth;

	if ( !m_isGroup )
	{
		for( TDynArray< ITimelineItem* >::iterator itemIter = timeline->m_items.Begin(); itemIter != timeline->m_items.End(); ++itemIter )
		{
			ITimelineItem* item = *itemIter;
			ASSERT( item != NULL );

			if( item->GetTrackName() == m_name )
			{
				item->SetTrackName( name );
			}
		}
	}
	else
	{
		for ( TDynArray< Track* >::iterator it = m_children.Begin(); it != m_children.End(); ++it )
		{
			Track* child = *it;
			String newName = name + child->m_name.MidString( nameLength - 1, child->m_name.Size() );
			if ( newGroup)
			{
				child->m_parent = newGroup;
			}
			child->Rename( timeline, newName, false, newDepth + 1 );
		}
	}

	if ( reAdd )
	{
		timeline->SortTracks();
		delete this;
		return;
	}
	m_name = name;
}

void Track::Delete( CEdTimeline* timeline )
{
	Int32 trackIndex = timeline->GetTrackIndex( m_name );
	ASSERT( trackIndex>=0 );

	while ( m_children.Size() )
	{
		(*m_children.Begin())->Delete( timeline );
	}

	if ( !m_isGroup )
	{
		TDynArray<ITimelineItem*> toRemove;
		timeline->GetTrackItems( this, toRemove );

		Bool selectionChanged = false;
		for( auto it = toRemove.Begin(), end = toRemove.End(); it != end; ++it )
		{
			ITimelineItem* item = *it;
			if( timeline->IsSelected( item ) )
			{
				timeline->m_selectedItems.Remove( item );
				selectionChanged = true;
			}

			timeline->RemoveItem( item );
			delete item;
		}

		if( selectionChanged )
		{
			timeline->SelectionChanged();
		}
	}

	// Remove track
	timeline->m_tracks.RemoveFast( this );
	TimelineImpl::CDrawGroupTracks* drawGroup = timeline->GetTrackDrawGroup(this);
	drawGroup->RemoveTrack(this);

	if ( m_depth == 0 )
	{
		timeline->SortTracks();
	}
	else
	{
		m_parent->m_children.RemoveFast( this );
		Sort( m_parent->m_children.Begin(), m_parent->m_children.End(), GreaterTrack() );
	}
	delete this;
}

Bool Track::InsertTrack( Track* track, CEdTimeline *timeline )
{
	Uint32 depth = ++track->m_depth;
	size_t dotPos = 0;
	Bool hasFound = true;
	for ( Uint32 i = 0; i <= depth; ++i )
	{
		size_t startPos = dotPos + 1;
		if ( !track->m_name.FindCharacter( timeline->GROUP_SEPARATOR, dotPos, startPos, false ) )
		{
			hasFound = false;
			break;
		}
	}
	if ( hasFound ) // we are searching for proper sub-group
	{
		String trackGroup;
		trackGroup.PushBack( track->m_name.MidString( 0, dotPos ) );
		for ( TDynArray< Track* >::iterator it = m_children.Begin(); it != m_children.End(); ++it )
		{
			if ( (*it)->m_name == trackGroup ) // group found, inserting in
			{
				return InsertTrack( track, timeline );
			}
		}
		// group not found, adding new one
		Track* group = Create( trackGroup, depth, true );
		group->m_parent = this;
		timeline->m_tracks.PushBack( group );
		TimelineImpl::CDrawGroupTracks* drawGroup = timeline->GetTrackDrawGroup(this);
		drawGroup->AddTrack(group);
		timeline->SortTracks();
		m_children.PushBack( group );
		Sort( m_children.Begin(), m_children.End(),GreaterTrack() );
		group->InsertTrack( track, timeline );
		timeline->UpdateLayout();
		return true;
	}
	else
	{
		for ( TDynArray< Track* >::iterator it = m_children.Begin(); it != m_children.End(); ++it )
		{
			if ( (*it)->m_name == track->m_name ) // track with this name already exists, failing
			{
				return false;
			}

		}
		// adding track
		track->m_parent = this;
		m_children.PushBack( track );
		Sort( m_children.Begin(), m_children.End(),GreaterTrack() );
		timeline->m_tracks.PushBack( track );
		TimelineImpl::CDrawGroupTracks* drawGroup = timeline->GetTrackDrawGroup(this);
		drawGroup->AddTrack(track);
		drawGroup->SortTracks(); // this put added track next to other tracks from the same group
		timeline->SortTracks();
		timeline->UpdateLayout();
		return true;
	}
}

/*
Virtual constructor.
*/
Track* Track::Create( String name, Uint32 depth, bool isGroup ) const
{
	return new Track( name, depth, isGroup );
}

/*
Returns whether ancestorTrack is ancestor of descendantTrack.
*/
Bool IsRelationship( Track* ancestorTrack, Track* descendantTrack )
{
	if (descendantTrack)
	{
		while(descendantTrack->m_parent)
		{
			if(descendantTrack->m_parent == ancestorTrack)
			{
				return true;
			}
			descendantTrack = descendantTrack->m_parent;
		}
	}

	return false;
}
