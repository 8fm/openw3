/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../core/factory.h"
#include "extAnimEventsFile.h"
#include "../core/tagList.h"
#include "skeletalAnimationEntry.h"
#include "entity.h"
#include "../core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimEventsFile );

#ifdef USE_EXT_ANIM_EVENTS

CGatheredResource sfxEnum( TXT("sounds\\gameplay\\sfx_enum.csv"), RGF_Startup );	

CExtAnimEventsFile::CExtAnimEventsFile()
{
}

CExtAnimEventsFile::~CExtAnimEventsFile()
{
	// Delete events
	for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::iterator eventIter = m_events.Begin();
		eventIter != m_events.End(); ++eventIter )
	{
		delete *eventIter;
	}
}

Bool CExtAnimEventsFile::RenameAnimationInAllEvents( CName const & prevName, CName const & newName )
{
	Bool anythingChanged = false;
	for ( auto iEvent = m_events.Begin(); iEvent != m_events.End(); ++ iEvent )
	{
		if ( CExtAnimEvent* event = *iEvent )
		{
			if ( event->GetAnimationName() == prevName )
			{
				event->SetAnimationName( newName );
				anythingChanged = true;
			}
		}
	}
	return anythingChanged;
}

void CExtAnimEventsFile::GetEventsByTime( const CSkeletalAnimationSetEntry* animation, Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, const CName& tag ) const
{
	if( m_events.Empty() )
	{
		return;
	}

	if( m_requiredSfxTag != CName::NONE && m_requiredSfxTag != tag )
	{
		return;
	}

	if( numLoops > 0 || ( numLoops == 0 && currTime >= prevTime ) )
	{
		// Forward events

		// Iterate through all events in file
		for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::const_iterator eventIter = m_events.Begin();
			eventIter != m_events.End(); ++eventIter )
		{
			const CExtAnimEvent* event = *eventIter;
			ASSERT( event != NULL );

			// Skip events for other animations
			if( event->GetAnimationName() != animation->GetName() )
			{
				continue;
			}

			// Special case for duration events
			if( IsType< CExtAnimDurationEvent>( event ) )
			{
				const CExtAnimDurationEvent* durEvent = static_cast< const CExtAnimDurationEvent* >( event );

				Float startTime = durEvent->GetStartTime();
				Float duration = durEvent->GetDuration();
				Float endTime = startTime + duration;
				
				// Overlap test
				if( endTime > animation->GetDuration() )
				{
					endTime -= animation->GetDuration();
				}

				if( ( numLoops == 0 && prevTime <= startTime && currTime > startTime )
					|| ( numLoops > 0 && ( prevTime <= startTime || currTime > startTime ) ) )
				{
					// Event just started
					CAnimationEventFired fired;
					fired.m_alpha = alpha;
					fired.m_extEvent = durEvent;
					fired.m_type = AET_DurationStart;

					if( events != NULL )
					{
						events->PushBack( fired );
					}

					if( output != NULL )
					{
						output->AppendEvent( fired, alpha );
					}
				}
				if ( duration >= animation->GetDuration()	// Case for events longer than animation
					|| ( numLoops == 0 && // Single pass case
						( ( startTime < endTime && ( prevTime > startTime && currTime < endTime ) )     // not overlapped case
						|| ( startTime > endTime && ( currTime < endTime || prevTime > startTime ) ) ) ) // overlapped case

					|| ( numLoops > 0 && // Looped case
						( prevTime > startTime || currTime < endTime ) ) )
				{
					// Event is still fired
					CAnimationEventFired fired;
					fired.m_alpha = alpha;
					fired.m_extEvent = durEvent;
					fired.m_type = AET_Duration;

					if( events != NULL )
					{
						events->PushBack( fired );
					}

					if( output != NULL )
					{
						output->AppendEvent( fired, alpha );
					}
				}
				if( ( numLoops == 0 && prevTime <= endTime && currTime > endTime )
					|| ( numLoops > 0 && ( prevTime <= endTime || currTime > endTime ) ) )
				{
					// Event just ended
					CAnimationEventFired fired;
					fired.m_alpha = alpha;
					fired.m_extEvent = durEvent;
					fired.m_type = AET_DurationEnd;

					if( events != NULL )
					{
						events->PushBack( fired );
					}

					if( output != NULL )
					{
						output->AppendEvent( fired, alpha );
					}
				}
			}
			// Normal events
			else if( ( numLoops == 0 && ( ( event->GetStartTime() == animation->GetDuration() && event->GetStartTime() <= currTime && event->GetStartTime() > prevTime ) || // to catch events at the very end of animation
										  ( event->GetStartTime() < currTime && event->GetStartTime() >= prevTime ) ) )
				  || ( numLoops > 0 && ( event->GetStartTime() < currTime || event->GetStartTime() >= prevTime ) ) )
			{
				// Add event to the list
				CAnimationEventFired fired;
				fired.m_alpha = alpha;
				fired.m_extEvent = event;
				fired.m_type = AET_Tick;

				if( events != NULL )
				{
					events->PushBack( fired );
				}

				if( output != NULL )
				{
					output->AppendEvent( fired, alpha );
				}
			}
		}
	}
	else if( numLoops < 0 || ( numLoops == 0 && currTime < prevTime ) )
	{
		// Backwards events.

		// Iterate through all events in file
		for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::const_iterator eventIter = m_events.End() - 1;
			eventIter >= m_events.Begin(); --eventIter )
		{
			const CExtAnimEvent* event = *eventIter;
			ASSERT( event != NULL );

			// Skip events for other animations
			if( event->GetAnimationName() != animation->GetName() )
			{
				continue;
			}

			// Special case for duration events
			if( IsType< CExtAnimDurationEvent>( event ) )
			{
				const CExtAnimDurationEvent* durEvent = static_cast< const CExtAnimDurationEvent* >( event );

				Float startTime = durEvent->GetStartTime();
				Float duration = durEvent->GetDuration();
				Float endTime = startTime + duration;

				// Overlap test
				if( endTime > animation->GetDuration() )
				{
					endTime -= animation->GetDuration();
				}

				if( ( numLoops == 0 && prevTime >= startTime && currTime < startTime )
					|| ( numLoops < 0 && ( prevTime >= startTime || currTime < startTime ) ) )
				{
					// Event just started
					CAnimationEventFired fired;
					fired.m_alpha = alpha;
					fired.m_extEvent = durEvent;
					fired.m_type = AET_DurationStart;

					if( events != NULL )
					{
						events->PushBack( fired );
					}

					if( output != NULL )
					{
						output->AppendEvent( fired, alpha );
					}
				}
				if (  duration >= animation->GetDuration()	// Case for events longer than animation
					|| ( numLoops == 0 && // Single pass case
					( ( startTime < endTime && ( prevTime < endTime && currTime > startTime ) )     // not overlapped case
					|| ( startTime > endTime && ( currTime < endTime || prevTime > startTime ) ) ) ) // overlapped case

					|| ( numLoops < 0 && // Looped case
					( prevTime < endTime || currTime > startTime ) ) )
				{
					// Event is still fired
					CAnimationEventFired fired;
					fired.m_alpha = alpha;
					fired.m_extEvent = durEvent;
					fired.m_type = AET_Duration;

					if( events != NULL )
					{
						events->PushBack( fired );
					}

					if( output != NULL )
					{
						output->AppendEvent( fired, alpha );
					}
				}
				if( ( numLoops == 0 && prevTime >= endTime && currTime < endTime )
					|| ( numLoops < 0 && ( prevTime >= endTime || currTime < endTime ) ) )
				{
					// Event just ended
					CAnimationEventFired fired;
					fired.m_alpha = alpha;
					fired.m_extEvent = durEvent;
					fired.m_type = AET_DurationEnd;

					if( events != NULL )
					{
						events->PushBack( fired );
					}

					if( output != NULL )
					{
						output->AppendEvent( fired, alpha );
					}
				}
			}
			// Normal events
			else if( ( numLoops == 0 && event->GetStartTime() > currTime && event->GetStartTime() <= prevTime )
				|| ( ( numLoops < 0 ) && ( event->GetStartTime() > currTime || event->GetStartTime() <= prevTime ) ) )
			{
				// Add event to the list
				CAnimationEventFired fired;
				fired.m_alpha = alpha;
				fired.m_extEvent = event;
				fired.m_type = AET_Tick;

				if( events != NULL )
				{
					events->PushBack( fired );
				}

				if( output != NULL )
				{
					output->AppendEvent( fired, alpha );
				}
			}
		}
	}
	else
	{
		WARN_ENGINE( TXT( "Invalid animation sample: prevTime = %.2f, currTime = %.2f, numloops = %d" ),
			prevTime, currTime, numLoops );
	}
}

void CExtAnimEventsFile::AddEvent( CExtAnimEvent* event )
{ 
	m_events.Insert( event );
	MarkModified();
}

void CExtAnimEventsFile::RemoveEvent( CExtAnimEvent* event )
{ 
	m_events.Remove( event );
	MarkModified();
}

void CExtAnimEventsFile::GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events )
{
	GetEventsOfType( animName, events );
}

const CResource* CExtAnimEventsFile::GetParentResource() const
{
	return this;
}

void CExtAnimEventsFile::OnSerialize( IFile& file )
{
	// Serialize base
	TBaseClass::OnSerialize( file );

	// Serialize animation events
	CExtAnimEvent::Serialize( m_events, file );
}

Bool CExtAnimEventsFile::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == TXT("events") )
	{
		CAnimEventSerializer* serializer = *( CAnimEventSerializer** ) readValue.GetData(); 
		
		m_crappySerializer = serializer;

		return true;
	}
	else if ( propertyName == TXT("requiredEntityTags") )
	{
		TagList tagList = *( TagList* ) readValue.GetData();

		if ( !tagList.Empty() && m_requiredSfxTag == CName::NONE )
		{
			m_requiredSfxTag = tagList.GetTag( 0 );
		}

		return true;
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

Uint32 CExtAnimEventsFile::GetNumEventsOfType( const CName& animName, const CClass* c ) const
{
	Uint32 num = 0;
	for ( TDynArray< CExtAnimEvent* >::const_iterator eventIter = m_events.Begin(); eventIter != m_events.End(); ++eventIter )
	{
		CExtAnimEvent* e = *eventIter;
		if ( e && e->GetAnimationName() == animName && e->GetClass()->IsA( c ) )
		{
			++num;
		}
	}
	return num;
}

void CExtAnimEventsFile::CleanupSourceData()
{
	TBaseClass::CleanupSourceData();

	m_events.Clear();
}

void CExtAnimEventsFile::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = sfxEnum.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Name");
}

//////////////////////////////////////////////////////////////////////////
///// Factory for allowing creation in Asset Browser /////////////////////
//////////////////////////////////////////////////////////////////////////

class CExtAnimEventFileFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CExtAnimEventFileFactory, IFactory, 0 );

public:
	CExtAnimEventFileFactory()
	{
		m_resourceClass = ClassID< CExtAnimEventsFile >();
	}

	CResource* DoCreate( const FactoryOptions& options )
	{
		CExtAnimEventsFile *obj = ::CreateObject< CExtAnimEventsFile >( options.m_parentObject );
		return obj;
	}
};

BEGIN_CLASS_RTTI( CExtAnimEventFileFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CExtAnimEventFileFactory );

#endif