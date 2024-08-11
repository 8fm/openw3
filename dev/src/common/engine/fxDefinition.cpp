/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxSystem.h"
#include "fxDefinition.h"
#include "cutscene.h"

IMPLEMENT_ENGINE_CLASS( CFXDefinition );

CFXDefinition::CFXDefinition( CEntityTemplate* entityTemplate, CName effectName )
	: m_name( effectName )
	, m_length( 1.0f )
	, m_loopStart( 0.0f )
	, m_loopEnd( 1.0f )
	, m_randomStart( false )
	, m_stayInMemory( false )
	, m_showDistance( 12.0f )
{
	ASSERT( entityTemplate );
	SetParent( entityTemplate );
}

CFXDefinition::CFXDefinition( CCutsceneTemplate* cutsceneTemplate, CName effectName )
	: m_name( effectName )
	, m_length( 1.0f )
	, m_loopStart( 0.0f )
	, m_loopEnd( 1.0f )
	, m_randomStart( false )
	, m_stayInMemory( false )
	, m_showDistance( 12.0f )
{
	ASSERT( cutsceneTemplate );
	SetParent( cutsceneTemplate );
}

void CFXDefinition::OnPostLoad()
{
#ifndef NO_EDITOR
	// Cleanup NULL items
	for ( Uint32 i=0; i<m_trackGroups.Size(); i++ )
	{
		CFXTrackGroup* group = m_trackGroups[i];

		const TDynArray< CFXTrack* >& tracks = group->GetTracks();
		for ( Uint32 j=0; j<tracks.Size(); j++ )
		{
			CFXTrack* track = tracks[j];
			while( track->GetTrackItems().Remove( NULL ) ){}
		}
	}
#endif
	// Pass to base class
	TBaseClass::OnPostLoad();
}

Bool CFXDefinition::RemoveTrackGroup( CFXTrackGroup* effectTrackGroup )
{
	for ( Uint32 i = 0; i < m_trackGroups.Size(); i++ )
	{
		if ( m_trackGroups[i] == effectTrackGroup )
		{
			m_trackGroups.EraseFast( m_trackGroups.Begin() + i );
			effectTrackGroup->Discard();
			return true;
		}
	}
	return false;
}

CFXTrackGroup* CFXDefinition::AddTrackGroup( const String &trackGroupName )
{
	// Create track group
	CFXTrackGroup *newTrackGroup = new CFXTrackGroup( this, trackGroupName );
	if ( newTrackGroup )
	{
		m_trackGroups.Insert( 0, newTrackGroup );
		return newTrackGroup;
	}

	// Not added
	return NULL;
}

Bool CFXDefinition::BindToAnimation( const CName& animationName )
{
	m_animationName = animationName;
	return true;
}

void CFXDefinition::UnbindFromAnimation()
{
	m_animationName = CName::NONE;

}

void CFXDefinition::SetLength( Float length )
{
	m_length = Max< Float >( 0.0f, length );
	m_loopStart = Clamp< Float >( m_loopStart, 0.0f, m_length );
	m_loopEnd = Clamp< Float >( m_loopEnd, m_loopStart, m_length );
}

void CFXDefinition::SetLoopStart( Float time )
{
	m_loopStart = Clamp< Float >( time, 0.0f, m_loopEnd );
}

void CFXDefinition::SetLoopEnd( Float time )
{
	m_loopEnd = Clamp< Float >( time, m_loopStart, m_length );
}

static Bool ShouldBeCollected( const CFXTrackItem* item, Float prevTime, Float curTime )
{
	const Float effectStart = item->GetTimeBegin();
	const Float effectEnd = item->GetTimeEnd();

	// Collect on frame start
	if ( effectStart >= prevTime && effectStart < curTime )
	{
		return true;
	}

	// Do not collect
	return false;
}

static Bool CheckValidLoopTimeConstaints( Float loopStart, Float loopEnd, const CFXTrackItem* item )
{
	const Float effectStart = item->GetTimeBegin();
	const Float effectEnd = item->GetTimeEnd();

	// Crosses the loop border - started within a loop but no end within the loop, this is not allowed
	if ( effectEnd > loopEnd && effectStart >= loopStart && effectStart < loopEnd )
	{
		return false;
	}

	// Valid
	return true;
}

void CFXDefinition::CollectTracksToStart( Float prevTime, Float curTime, TDynArray< const CFXTrackItem* >& trackItemsToStart ) const
{
	// Zero time, no effect
	if ( curTime <= prevTime )
	{
		return;
	}

	// Collect from all track groups
	for ( Uint32 i=0; i<m_trackGroups.Size(); i++ )
	{
		CFXTrackGroup* group = m_trackGroups[i];
		/*if ( group->IsEnabled() )
		{
			continue;
		}*/

		// Collect from all tracks in groups
		const TDynArray< CFXTrack* >& tracks = group->GetTracks();
		for ( Uint32 j=0; j<tracks.Size(); j++ )
		{
			CFXTrack* track = tracks[j];

			// Collect from all track items
			const TDynArray< CFXTrackItem* >& items = track->GetTrackItems();
			for ( Uint32 k=0; k<items.Size(); k++ )
			{
				const CFXTrackItem* item = items[k];

				if ( item )
				{
					// Skip items that violate the looping constraints
					if ( m_isLooped )
					{
						if ( !CheckValidLoopTimeConstaints( m_loopStart, m_loopEnd, item ) )
						{
							continue;
						}
					}

					// Test if track item should be collected
					if ( ShouldBeCollected( item, prevTime, curTime ) )
					{
						trackItemsToStart.PushBack( item );
					}
				}
			}
		}
	}
}

void CFXDefinition::SetName( const CName& name )
{
	m_name = name;
}

void CFXDefinition::PrefetchResources(TDynArray< TSoftHandle< CResource > >& requiredResources) const
{
	// Prefetch from all track groups
	for ( Uint32 i=0; i<m_trackGroups.Size(); i++ )
	{
		CFXTrackGroup* group = m_trackGroups[i];

		// Collect from all tracks in groups
		const TDynArray< CFXTrack* >& tracks = group->GetTracks();
		for ( Uint32 j=0; j<tracks.Size(); j++ )
		{
			CFXTrack* track = tracks[j];

			// Collect from all track items
			const TDynArray< CFXTrackItem* >& items = track->GetTrackItems();
			for ( Uint32 k=0; k<items.Size(); k++ )
			{
				const CFXTrackItem* item = items[k];
				if ( item )
				{
					item->PrefetchResources(requiredResources);
				}
			}
		}
	}
}
