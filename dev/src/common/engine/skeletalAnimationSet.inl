/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "skeletalAnimationEntry.h"

template < class EventType >
void CSkeletalAnimationSetEntry::GetEventsOfType( TDynArray< EventType* >& list ) const
{
	for( TDynArray< CExtAnimEvent* >::const_iterator eventIter = m_events.Begin();
		eventIter != m_events.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;

		if( IsType< EventType >( event ) )
		{
			list.PushBack( static_cast< EventType* >( event ) );
		}
	}

#ifdef USE_EXT_ANIM_EVENTS

	if( GetAnimSet() != NULL )
	{
		GetAnimSet()->GetExternalEventsOfType( GetName(), list );
	}

#endif
}

template < class EventType, class Collector >
void CSkeletalAnimationSetEntry::GetEventsOfType( Collector& collector ) const
{
	for( TDynArray< CExtAnimEvent* >::const_iterator eventIter = m_events.Begin();
		eventIter != m_events.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;

		if( IsType< EventType >( event ) )
		{
			collector( event );
		}
	}

#ifdef USE_EXT_ANIM_EVENTS

	if( GetAnimSet() != NULL )
	{
		GetAnimSet()->GetExternalEventsOfType< EventType >( GetName(), collector );
	}

#endif
}

#ifdef USE_EXT_ANIM_EVENTS

template < class EventType >
void CSkeletalAnimationSet::GetExternalEventsOfType( const CName& animName, TDynArray< EventType* >& list ) const
{
	// Get events from external files
	for( TDynArray< THandle< CExtAnimEventsFile > >::const_iterator fileIter = m_extAnimEvents.Begin();
		fileIter != m_extAnimEvents.End(); ++fileIter )
	{
		const CExtAnimEventsFile* file = (*fileIter).Get();

		// Skip broken entries
		if( file == NULL )
		{
			continue;
		}

		file->GetEventsOfType< EventType >( animName, list );
	}
}

template < class EventType, class Collector >
void CSkeletalAnimationSet::GetExternalEventsOfType( const CName& animName, Collector& collector ) const
{
	// Get events from external files
	for( TDynArray< THandle< CExtAnimEventsFile > >::const_iterator fileIter = m_extAnimEvents.Begin();
		fileIter != m_extAnimEvents.End(); ++fileIter )
	{
		const CExtAnimEventsFile* file = (*fileIter).Get();

		// Skip broken entries
		if( file == NULL )
		{
			continue;
		}

		file->GetEventsOfType< EventType, Collector >( animName, collector );
	}
}

#endif
