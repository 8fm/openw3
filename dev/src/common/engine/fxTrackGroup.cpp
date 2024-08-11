/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxSystem.h"
#include "baseEngine.h"
#include "entity.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackGroup );

static Uint8 GetRandomColor()
{
	return GEngine->GetRandomNumberGenerator().Get< Uint8 >( 127 , 255 );
}

CFXTrackGroup::CFXTrackGroup( CFXDefinition* def, const String& name )
	: m_trackGroupColor( GetRandomColor(), GetRandomColor(), GetRandomColor(), 100 )
	, m_name( name )
	, m_isEnabled( false )
{
	SetParent( def );
}

CFXTrack* CFXTrackGroup::AddTrack( const String& trackName )
{
	if ( MarkModified() )
	{
		CFXTrack *newTrack = new CFXTrack( this, trackName );
		if ( newTrack )
		{
			m_tracks.Insert( 0, newTrack );
			return newTrack;
		}
	}

	// Not added
	return NULL;	
}

Bool CFXTrackGroup::RemoveTrack( CFXTrack *effectTrack )
{
	if ( MarkModified() )
	{
		for ( Uint32 i = 0; i < m_tracks.Size(); i++ )
		{
			if ( m_tracks[i] == effectTrack )
			{
				// Remove from array
				m_tracks.Erase( m_tracks.Begin() + i );

				// Discard track
				effectTrack->Discard();
				return true;
			}
		}
	}

	// Not removed
	return false;
}

CComponent* CFXTrackGroup::GetAffectedComponent( const CFXState& state )
{
	// Linear search
	CEntity* entity = state.GetEntity();
	if ( entity )
	{
		const TDynArray< CComponent* >& components = entity->GetComponents();
		for ( Uint32 i=0; i<components.Size(); i++ )
		{
			CComponent* component = components[i];
			if ( component && m_componentName == CName( component->GetName() ) )
			{
				return component;
			}
		}
	}

	// Not found
	return NULL;
}

void CFXTrackGroup::Expand()
{
	m_isExpanded = true;
}

void CFXTrackGroup::Collapse()
{
	m_isExpanded = false;
}

void CFXTrackGroup::SetName( const String &name )
{
	// Change object name
	if ( MarkModified() )
	{
		m_name = name;
	}
}

void CFXTrackGroup::Remove()
{
	if ( MarkModified() )
	{
		GetFX()->RemoveTrackGroup( this );
	}
}
