/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behaviorGraphOutput.h"
#include "extAnimDurationEvent.h"

template < class EventType >
void CExtAnimEventsFile::GetEventsOfType( const CName& animName, TDynArray< EventType* >& list ) const
{
	for( TDynArray< CExtAnimEvent* >::const_iterator eventIter = m_events.Begin();
		eventIter != m_events.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;

		if( IsType< EventType >( event ) 
			&& event->GetAnimationName() == animName )
		{
			list.PushBack( static_cast< EventType* >( event ) );
		}
	}
}

template < class EventType, class Collector >
void CExtAnimEventsFile::GetEventsOfType( const CName& animName, Collector& collector ) const
{
	for( TDynArray< CExtAnimEvent* >::const_iterator eventIter = m_events.Begin();
		eventIter != m_events.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;

		if( IsType< EventType >( event ) 
			&& event->GetAnimationName() == animName )
		{
			collector( static_cast< EventType* >( event ) );
		}
	}
}
