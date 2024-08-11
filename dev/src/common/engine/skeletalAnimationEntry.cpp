
#include "build.h"
#include "skeletalAnimationEntry.h"
#include "behaviorGraphOutput.h"
#include "extAnimDurationEvent.h"
#include "animationEvent.h"
#include "skeletalAnimationSet.h"
#include "skeletalAnimation.h"
#include "extAnimExplorationEvent.h"

IMPLEMENT_ENGINE_CLASS( ISkeletalAnimationSetEntryParam );
IMPLEMENT_ENGINE_CLASS( CSkeletalAnimationSetEntry );
IMPLEMENT_ENGINE_CLASS( SEventGroupsRanges );

CSkeletalAnimationSetEntry::CSkeletalAnimationSetEntry()
	: m_animation( NULL )
	, m_animSet( NULL )
	, m_compressedPoseBlend( CPBT_Normal )
	, m_nextInGlobalMapList( nullptr )
{
}

CSkeletalAnimationSetEntry::~CSkeletalAnimationSetEntry()
{
	// Delete events
	for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::iterator eventIter = m_events.Begin();
		eventIter != m_events.End(); ++eventIter )
	{
		if ( *eventIter )
		{
			delete *eventIter;
		}
	}

	// Delete animation data
	if ( NULL != m_animation )
	{
		delete m_animation;
		m_animation = NULL;
	}
}

void CSkeletalAnimationSetEntry::Bind( CSkeletalAnimationSet* animationSet )
{
	// bind animation set
	m_animSet = animationSet;

	// bind animation to this set entry
	if ( NULL != m_animation )
	{
		m_animation->Bind( this, animationSet );
	}
}

void CSkeletalAnimationSetEntry::SetAnimation( CSkeletalAnimation *anim )
{
	m_animation = anim;
}

void CSkeletalAnimationSetEntry::AddParam( ISkeletalAnimationSetEntryParam* param )
{
	m_params.PushBack( param );
}

Bool CSkeletalAnimationSetEntry::RemoveParam( const ISkeletalAnimationSetEntryParam* param )
{
	const Uint32 size = m_params.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const ISkeletalAnimationSetEntryParam* p = m_params[i];

		if ( p == param )
		{
			m_params.Erase( m_params.Begin() + i );
			return true;
		}
	}

	return false;
}

void CSkeletalAnimationSetEntry::ClearParams()
{
	m_params.Clear();
}

const ISkeletalAnimationSetEntryParam* CSkeletalAnimationSetEntry::FindParamByClass( const CClass* c ) const
{
	const Uint32 size = m_params.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const ISkeletalAnimationSetEntryParam* param = m_params[i];

		if ( param && param->GetClass()->IsA( c ) )
		{
			return param;
		}
	}
	return NULL;
}

#ifndef NO_EDITOR
void CSkeletalAnimationSetEntry::OnPostLoad()
{
	m_events.Sort();
	ISerializable::OnPostLoad();
}
#endif

Int32 CSkeletalAnimationSetEntry::FindEventGroupRangeIndex( const CName& tag )
{
	if( tag == CName::NONE )
	{
		return -1;
	}
	const Uint32 count = m_eventsGroupsRanges.Size();
	for( Uint32 i = 0; i != count; ++i )
	{
		if( m_eventsGroupsRanges[ i ].m_tag == tag )
		{
			return i;
		}
	}
	return -1;
}

void CSkeletalAnimationSetEntry::GetEventsByTime( Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, const CName& tag, Int32 eventGroupRangeIndex ) const
{
	PC_SCOPE( GetEventsByTime );
	if( eventGroupRangeIndex < 0 )
	{
		GetEventsByTime( prevTime, currTime, numLoops, alpha, events, output, m_events.Begin(), m_events.End() );
	}
	else
	{
		GetEventsByTime( prevTime, currTime, numLoops, alpha, events, output, m_events.Begin(), m_events.TypedData() + m_eventsGroupsRanges[ 0 ].m_beginIndex );
		const SEventGroupsRanges& range = m_eventsGroupsRanges[ eventGroupRangeIndex ];
		GetEventsByTime( prevTime, currTime, numLoops, alpha, events, output, m_events.TypedData() + range.m_beginIndex, m_events.TypedData() + range.m_endIndex );
	}

#ifdef USE_EXT_ANIM_EVENTS
	if( CSkeletalAnimationSet* animset = GetAnimSet() )
	{
		animset->GetExternalEventsByTime( this, prevTime, currTime, numLoops, alpha, events, output, tag );
	}
#endif
}

void CSkeletalAnimationSetEntry::GetEventsByTime( Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, TAnimSetEntryEvents::const_iterator begin, TAnimSetEntryEvents::const_iterator end ) const
{
	if(!m_events.Empty() )
	{
		if( numLoops > 0 || ( numLoops == 0 && currTime >= prevTime ) )
		{
			// Iterate through all events in file
			for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::const_iterator eventIter = begin;
				eventIter != end; ++eventIter )
			{
				const CExtAnimEvent* event = *eventIter;

				ASSERT( event != NULL );

				// Special case for duration events
				if( IsType< CExtAnimDurationEvent>( event ) )
				{
					const CExtAnimDurationEvent* durEvent = static_cast< const CExtAnimDurationEvent* >( event );

					const Float startTime = durEvent->GetStartTime();
					const Float duration = durEvent->GetDuration();
					Float endTime = startTime + duration;

					// Overlap test
					if( endTime > GetDuration() )
					{
						endTime -= GetDuration();
					}

					// if we are looping and the end time is later then start time,
					// we need to swap the order of adding events to fire them correctly
					// example: given an event which is exactly the length of the animation
					// on every loop of the animation start event will be fired first instead of the end event
					if( numLoops > 0 && endTime > startTime )
					{
						// End event first
						if( prevTime <= endTime || currTime > endTime )
						{
							CAnimationEventFired fired;
							fired.m_alpha = alpha;
							fired.m_extEvent = durEvent;
							fired.m_type = AET_DurationEnd;
							fired.m_animInfo.m_eventName = event->GetEventName();
							fired.m_animInfo.m_animation = this;
							fired.m_animInfo.m_localTime = currTime;
							fired.m_animInfo.m_eventEndsAtTime = endTime;
							fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

							if( events != NULL )
							{
								events->PushBack( fired );
							}

							if( output != NULL )
							{
								output->AppendEvent( fired, alpha );
							}
						}

						// Start event next
						if( prevTime <= startTime || currTime > startTime )
						{
							CAnimationEventFired fired;
							fired.m_alpha = alpha;
							fired.m_extEvent = durEvent;
							fired.m_type = AET_DurationStart;
							fired.m_animInfo.m_eventName = event->GetEventName();
							fired.m_animInfo.m_animation = this;
							fired.m_animInfo.m_localTime = currTime;
							fired.m_animInfo.m_eventEndsAtTime = endTime;
							fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

							if( events != NULL )
							{
								events->PushBack( fired );
							}

							if( output != NULL )
							{
								output->AppendEvent( fired, alpha );
							}
						}

						// Duration at the end, it doesn't matter anyway
						if( prevTime < endTime || currTime > startTime )
						{
							CAnimationEventFired fired;
							fired.m_alpha = alpha;
							fired.m_extEvent = durEvent;
							fired.m_type = AET_Duration;
							fired.m_animInfo.m_eventName = event->GetEventName();
							fired.m_animInfo.m_animation = this;
							fired.m_animInfo.m_localTime = currTime;
							fired.m_animInfo.m_eventEndsAtTime = endTime;
							fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

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
					else
					{
						if( ( numLoops == 0 && prevTime <= startTime && currTime > startTime )
							|| ( numLoops > 0 && ( prevTime <= startTime || currTime > startTime ) ) )
						{
							// Event just started
							CAnimationEventFired fired;
							fired.m_alpha = alpha;
							fired.m_extEvent = durEvent;
							fired.m_type = AET_DurationStart;
							fired.m_animInfo.m_eventName = event->GetEventName();
							fired.m_animInfo.m_animation = this;
							fired.m_animInfo.m_localTime = currTime;
							fired.m_animInfo.m_eventEndsAtTime = endTime;
							fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

							if( events != NULL )
							{
								events->PushBack( fired );
							}

							if( output != NULL )
							{
								output->AppendEvent( fired, alpha );
							}
						}
						if ( duration >= GetDuration()	// Case for events longer than animation
							|| ( numLoops == 0 && // Single pass case
							( ( startTime < endTime && ( prevTime > startTime && currTime < endTime ) )     // not overlapped case
							|| ( startTime > endTime && ( currTime < endTime || prevTime > startTime ) ) ) ) // overlapped case

							|| (  numLoops > 0 && // Looped case
							( prevTime > startTime || currTime < endTime ) ) )
						{
							// Event is still fired
							CAnimationEventFired fired;
							fired.m_alpha = alpha;
							fired.m_extEvent = durEvent;
							fired.m_type = AET_Duration;
							fired.m_animInfo.m_eventName = event->GetEventName();
							fired.m_animInfo.m_animation = this;
							fired.m_animInfo.m_localTime = currTime;
							fired.m_animInfo.m_eventEndsAtTime = endTime;
							fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

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
							|| ( numLoops == 0 && prevTime < endTime && currTime >= endTime )// ED: added this case to cover the curTime being == endTime 
							|| ( numLoops > 0 && ( prevTime <= endTime || currTime > endTime ) ) )
						{
							// Event just ended
							CAnimationEventFired fired;
							fired.m_alpha = alpha;
							fired.m_extEvent = durEvent;
							fired.m_type = AET_DurationEnd;
							fired.m_animInfo.m_eventName = event->GetEventName();
							fired.m_animInfo.m_animation = this;
							fired.m_animInfo.m_localTime = currTime;
							fired.m_animInfo.m_eventEndsAtTime = endTime;
							fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

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
				// Normal events
				else if( ( numLoops == 0 && ( ( event->GetStartTime() == m_animation->GetDuration() && event->GetStartTime() <= currTime && event->GetStartTime() > prevTime ) || // to catch events at the very end of animation
												( event->GetStartTime() < currTime && event->GetStartTime() >= prevTime ) ) )
						|| ( numLoops > 0 && ( event->GetStartTime() < currTime || event->GetStartTime() >= prevTime ) ) )
				{
					// Add event to the list
					CAnimationEventFired fired;
					fired.m_alpha = alpha;
					fired.m_extEvent = event;
					fired.m_type = AET_Tick;
					fired.m_animInfo.m_eventName = event->GetEventName();
					fired.m_animInfo.m_animation = this;
					fired.m_animInfo.m_localTime = currTime;
					fired.m_animInfo.m_eventEndsAtTime = event->GetStartTime();
					fired.m_animInfo.m_eventDuration = 0.0f;

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
			for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::const_iterator eventIter = end - 1;
				eventIter >= begin; --eventIter )
			{
				const CExtAnimEvent* event = *eventIter;
				ASSERT( event != NULL );

				// Special case for duration events
				if( IsType< CExtAnimDurationEvent>( event ) )
				{
					const CExtAnimDurationEvent* durEvent = static_cast< const CExtAnimDurationEvent* >( event );

					Float startTime = durEvent->GetStartTime();
					Float duration = durEvent->GetDuration();
					Float endTime = startTime + duration;

					// Overlap test
					if( endTime > GetDuration() )
					{
						endTime -= GetDuration();
					}

					if( ( numLoops == 0 && prevTime >= startTime && currTime < startTime )
						|| ( numLoops < 0 && ( prevTime >= startTime || currTime < startTime ) ) )
					{
						// Event just started
						CAnimationEventFired fired;
						fired.m_alpha = alpha;
						fired.m_extEvent = durEvent;
						fired.m_type = AET_DurationStart;
						fired.m_animInfo.m_eventName = event->GetEventName();
						fired.m_animInfo.m_animation = this;
						fired.m_animInfo.m_localTime = currTime;
						fired.m_animInfo.m_eventEndsAtTime = startTime;
						fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

						if( events != NULL )
						{
							events->PushBack( fired );
						}

						if( output != NULL )
						{
							output->AppendEvent( fired, alpha );
						}
					}

					if (  duration >= GetDuration()	// Case for events longer than animation
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
						fired.m_animInfo.m_eventName = event->GetEventName();
						fired.m_animInfo.m_animation = this;
						fired.m_animInfo.m_localTime = currTime;
						fired.m_animInfo.m_eventEndsAtTime = startTime;
						fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

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
						fired.m_animInfo.m_eventName = event->GetEventName();
						fired.m_animInfo.m_animation = this;
						fired.m_animInfo.m_localTime = currTime;
						fired.m_animInfo.m_eventEndsAtTime = startTime;
						fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

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
					fired.m_animInfo.m_eventName = event->GetEventName();
					fired.m_animInfo.m_animation = this;
					fired.m_animInfo.m_localTime = currTime;
					fired.m_animInfo.m_eventEndsAtTime = event->GetStartTime();
					fired.m_animInfo.m_eventDuration = 0.0f;

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
			WARN_ENGINE( TXT( "Invalid animation sample: prevTime = %.2f, currTime = %.2f, numloops = %d" ), prevTime, currTime, numLoops );
		}
	}
}

void CSkeletalAnimationSetEntry::AddEvent( CExtAnimEvent* event )
{
	m_events.Insert( event );
	// Commented until resave!
	//MarkModified();
}

void CSkeletalAnimationSetEntry::RemoveEvent( CExtAnimEvent* event )
{
	m_events.Remove( event );
	// Commented until resave!
	//MarkModified();
}

void CSkeletalAnimationSetEntry::GetAllEvents( TDynArray< CExtAnimEvent* >& events ) const
{
	events.PushBack( m_events );
}

void CSkeletalAnimationSetEntry::GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events )
{
	if( animName == GetName() )
	{
		events.PushBack( m_events );
	}
}

void CSkeletalAnimationSetEntry::GetTrajectoryBlendPoints( TSortedArray< CSlotAnimationShiftingInterval >& outIntervals ) const
{
	const CSkeletalAnimation *skAnimation = GetAnimation();

	// Get exploration events
	TDynArray< const CExtAnimExplorationEvent* > events;
	GetEventsOfType( events );

	for( TDynArray< const CExtAnimExplorationEvent* >::const_iterator eventIter = events.Begin(); eventIter != events.End(); ++eventIter )
	{
		const CExtAnimExplorationEvent* event = *eventIter;
		RedQsTransform relPosition;
		if (skAnimation->HasExtractedMotion())
		{
			relPosition = skAnimation->GetMovementBetweenTime( 0.f, event->GetStartTime()+event->GetDuration(), 0 );
		}
		else
		{
			relPosition.SetZero();
		}

		// Lets store relative position from animation start in the shift field (normally used to store delta during the time interval).
		outIntervals.Insert( CSlotAnimationShiftingInterval( event->GetStartTime(), event->GetStartTime() + event->GetDuration(), reinterpret_cast< const Vector& >( relPosition.GetTranslation() ) ) );
	}
}

void CSkeletalAnimationSetEntry::OnSerialize( IFile& file )
{
	ISerializable::OnSerialize( file );

	// Serialize events
	CExtAnimEvent::Serialize( m_events, file );
}

const CResource* CSkeletalAnimationSetEntry::GetParentResource() const 
{ 
	return GetAnimSet(); 
}

void CSkeletalAnimationSetEntry::AddEventGroup( const CName& tagName, TAnimSetEntryEvents& events )
{
	Uint32 beginIndex = m_events.Size();
	for( auto i : events )
	{
		m_events.PushBack( i );
	}
	m_eventsGroupsRanges.PushBack( SEventGroupsRanges( tagName, beginIndex, m_events.Size() ) );
}

Uint32 CSkeletalAnimationSetEntry::GetNumEventsOfType( const CClass* c ) const
{
	Uint32 num = 0;

	for ( TDynArray< CExtAnimEvent* >::const_iterator eventIter = m_events.Begin(); eventIter != m_events.End(); ++eventIter )
	{
		const CExtAnimEvent* e = *eventIter;
		if ( e && e->GetClass()->IsA( c ) )
		{
			++num;
		}
	}

#ifdef USE_EXT_ANIM_EVENTS
	if ( const CSkeletalAnimationSet* set = GetAnimSet() )
	{
		num += set->GetNumExternalEventsOfType( GetName(), c );
	}
#endif

	return num;
}
