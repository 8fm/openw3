/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxSystem.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItem );

IFXTrackItemPlayData::IFXTrackItemPlayData( CNode* node, const CFXTrackItem* trackItem )
	: m_node( node )
	, m_trackItem( const_cast< CFXTrackItem* >( trackItem ) )
{
}

IFXTrackItemPlayData::~IFXTrackItemPlayData()
{
}

CFXTrackItem::CFXTrackItem()
	: m_timeDuration( 1.0f )
{
}

void CFXTrackItem::UpdateDuration( Float timeEnd )
{
	m_timeDuration = Abs< Float >( timeEnd - m_timeBegin );
}

void CFXTrackItem::SetTimeBeginWithConstEnd( Float timeBegin )
{
	Float timeEnd = GetTimeEnd();

	m_timeBegin = timeBegin;
	UpdateDuration( timeEnd );

	DataChanged();
}

void CFXTrackItem::SetTimeBeginWithConstDuration( Float timeBegin )
{
	m_timeBegin = timeBegin;

	DataChanged();
}

void CFXTrackItem::SetTimeEnd( Float timeEnd )
{
	UpdateDuration( timeEnd );

	DataChanged();
}

void CFXTrackItem::SetTimeDuration( Float timeDuration )
{
	ASSERT( timeDuration > 0 );

	m_timeDuration = timeDuration;

	DataChanged();
}

void CFXTrackItem::Remove()
{
	if ( MarkModified() )
	{
		GetTrack()->RemoveTrackItem( this );
	}
}

Bool CFXTrackItem::UsesComponent( const CName& componentName ) const
{
	return false;
}
