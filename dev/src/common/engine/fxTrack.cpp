/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxSystem.h"

IMPLEMENT_ENGINE_CLASS( CFXTrack );

CFXTrack::CFXTrack( CFXTrackGroup* group, const String& name )
	: m_name( name )
{
	SetParent( group );
}

Bool CFXTrack::RemoveTrackItem( CFXTrackItem* trackItem )
{
	if ( MarkModified() )
	{
		for ( Uint32 i = 0; i < m_trackItems.Size(); i++ )
		{
			if ( m_trackItems[i] == trackItem )
			{
				// Remove from array
				m_trackItems.Erase( m_trackItems.Begin() + i );

				// Discard effect
				trackItem->Discard();
				return true;
			}
		}
	}

	// Not removed
	return false;
}

void CFXTrack::AddTrackItem( CFXTrackItem* newTrackItem, Float position )
{
	if ( newTrackItem )
	{
		newTrackItem->SetTimeBeginWithConstDuration( position );
		m_trackItems.PushBack( newTrackItem );
	}
}

CFXTrackItem* CFXTrack::AddTrackItem( CClass *trackItemClass, Float position )
{
	if ( MarkModified() )
	{
		// Create track item
		CFXTrackItem *newTrackItem = ::CreateObject< CFXTrackItem >( trackItemClass, this );
		if ( newTrackItem )
		{
			// Update position
			newTrackItem->SetTimeBeginWithConstDuration( position );

			// Add to track list
			m_trackItems.PushBack( newTrackItem );
			return newTrackItem;
		}
	}

	// Not created	
	return NULL;
}

void CFXTrack::SetName( const String &name )
{
	if ( MarkModified() )
	{
		m_name = name;
	}
}

void CFXTrack::Remove()
{
	if ( MarkModified() )
	{
		GetTrackGroup()->RemoveTrack( this );
	}
}