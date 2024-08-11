// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogTimelineItemInterpolationEvent.h"
#include "../../common/game/storySceneEventInterpolation.h"

// =================================================================================================
namespace DialogTimelineItems {
// =================================================================================================

/*
Returns whether item is interpolate cameras event.
*/
Bool IsInterpolationEvent( const ITimelineItem* item )
{
	const CTimelineItemInterpolationEvent* result = dynamic_cast< const CTimelineItemInterpolationEvent* >( item );
	return result != nullptr;
}

/*
If one or both of keys is/are null, or is/are not CTimelineItemEvent or is/are not interpolable
then result is false. 

\param keyItemA Must not be nullptr.
\param keyItemB Must not be nullptr.
*/
Bool CompatibleWithRespectToInterpolation( const ITimelineItem* keyItemA, const ITimelineItem* keyItemB )
{
	ASSERT( keyItemA && keyItemB );

	Bool compatible = false;

	const CTimelineItemEvent* keyItemEventA = dynamic_cast< const CTimelineItemEvent* >( keyItemA );
	const CTimelineItemEvent* keyItemEventB = dynamic_cast< const CTimelineItemEvent* >( keyItemB );

	if( keyItemEventA && keyItemEventB )
	{
		CClass* interpolationClassA = keyItemEventA->GetInterpolationEventClass();
		CClass* interpolationClassB = keyItemEventB->GetInterpolationEventClass();

		if( interpolationClassA && interpolationClassB )
		{
			compatible = (interpolationClassA == interpolationClassB);
		}
		// else one or both are nullptr which means items are not compatible
	}
	// else one or both items are nto CTimelineItemEvents which means they are not compatible

	return compatible;
}

/*
Returns whether items can be used to create interpolation event.
*/
Bool CanCreateInterpolationEvent( const TDynArray< ITimelineItem* >& items )
{
	Bool compatible = true;

	// TODO: cleaning
	if( items.Size() >= 2 )
	{
		for( auto itItem = items.Begin(), endItems = items.End(); itItem != endItems; ++itItem )
		{
			if( !CompatibleWithRespectToInterpolation( *items.Begin(), *itItem ) )
			{
				compatible = false;
				break;
			}
		}
	}
	else
	{
		// we need at least 2 events to create interpolation event
		compatible = false;
	}

	return compatible;
}

/*

\param itemList List of timeline items.
\param interpolateItem (out) Item which is interpolate camera event.
\param keys List of items that are keys compatible with interpolate camera event. This container is not cleard before use.
\return True - items list contained one interpolate camera event and the rest of items were keys compatible with it, false - otherwise (and interpolateItem
and keys are undefined).

Note that if itemList contains only one item and this item is interpolate event item then the function will return true because interpolate event
item was found and all other items (zero of them!) from list were compatible with it.
*/
Bool GetInterpolateEventAndKeys( const TDynArray< ITimelineItem* >& itemList, ITimelineItem*& outInterpolateItem, TDynArray< ITimelineItem* >& outKeys )
{
	// we're gonna need copy of item list (TODO: actually we can use outKeys for this)
	TDynArray< ITimelineItem* > items = itemList;

	// find first interpolate event
	auto itInterpolationEvent = items.End();
	for( auto itItem = items.Begin(), endItems = items.End(); itItem != endItems; ++itItem )
	{
		if( IsInterpolationEvent( *itItem ) )
		{
			itInterpolationEvent = itItem;
			break;
		}
	}

	if( itInterpolationEvent == items.End() )
	{
		// no interpolation event found
		return false;
	}

	// store result
	outInterpolateItem = *itInterpolationEvent;

	// remove interpolate event item from list of items
	items.Erase( itInterpolationEvent );


	const CTimelineItemInterpolationEvent* tiInterpolationEvent = dynamic_cast< const CTimelineItemInterpolationEvent* >( outInterpolateItem );
	ASSERT( tiInterpolationEvent ); // we found interpolation event earlier, right?

	// check whether all other items are compatible with interpolate event
	Bool allItemsCompatible = true;
	for( auto itItem = items.Begin(), endItems = items.End(); itItem != endItems; ++itItem )
	{
		if( CompatibleWithRespectToInterpolation( tiInterpolationEvent->GetKey( 0 ), *itItem ) )
		{
			outKeys.PushBack( *itItem );
		}
		else
		{
			allItemsCompatible = false;
			break;
		}
	}

	return allItemsCompatible;
}

/*
Ctor.
*/
CTimelineItemInterpolationEvent::CTimelineItemInterpolationEvent( CEdDialogTimeline* timeline, CStorySceneEventInterpolation* scInterpolationEvent, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
: CTimelineItemEvent( timeline, scInterpolationEvent, elementItem, elements )
{}

/*
Initializes interpolation event item.

Preconditions:
1. Timeline event items for all interpolation event keys must already exist.

This function reinitializes interpolation event item so it is "synced" with scene interpolation event.
Scene interpolation event is not affected in any way.
*/
void CTimelineItemInterpolationEvent::Reinitialize()
{
	CStorySceneEventInterpolation* scInterpolationEvent = GetSceneInterpolationEvent();

	// get element item associated with first key - this item should be associated with the same key
	CGUID firstKeyGuid = scInterpolationEvent->GetKeyGuid( 0 );
	CTimelineItemEvent* tiFirstKey = m_timeline->FindItemEvent( firstKeyGuid );
	m_elementItem = tiFirstKey->GetElementItem();

	// reinitialize m_keys (note - we search for the first key for the second time in this function - that's
	// ok as it's temporary, we're gonna remove m_keys altogether)
	m_keys.ClearFast();
	for( Uint32 iKey = 0, numKeys = scInterpolationEvent->GetNumKeys(); iKey < numKeys; ++iKey )
	{
		CGUID keyGuid = scInterpolationEvent->GetKeyGuid( iKey );
		CTimelineItemEvent* tiKey = m_timeline->FindItemEvent( keyGuid );
		m_keys.PushBack( tiKey );
	}
}

CStorySceneEventInterpolation* CTimelineItemInterpolationEvent::GetSceneInterpolationEvent()
{
	return static_cast< CStorySceneEventInterpolation* >( GetEvent() );
}

const CStorySceneEventInterpolation* CTimelineItemInterpolationEvent::GetSceneInterpolationEvent() const
{
	return static_cast< const CStorySceneEventInterpolation* >( GetEvent() );
}

String CTimelineItemInterpolationEvent::GetTypeName() const
{
	return String( TXT( "CTimelineItemInterpolationEvent" ) );
}

Float CTimelineItemInterpolationEvent::SetStart( Float start, Bool deepUpdate )
{
	const Float oldStartTime = GetStart();
	const Float newStartTime = CTimelineItemEvent::SetStart( start, deepUpdate );
	const Float offset = newStartTime - oldStartTime;

	for( auto itKey = m_keys.Begin(), endKeys = m_keys.End(); itKey != endKeys; ++itKey )
	{
		ITimelineItem* key = ( *itKey );
		key->SetStart( key->GetStart() + offset, deepUpdate );
	}

	return newStartTime;
}

Bool CTimelineItemInterpolationEvent::IsRightResizable() const
{
	// item resizes when its keys are moved
	return false;
}

Bool CTimelineItemInterpolationEvent::IsLeftResizable() const
{
	// item resizes when its keys are moved
	return false;
}

Bool CTimelineItemInterpolationEvent::IsMovable() const
{
	return IsEditable();
}

Bool CTimelineItemInterpolationEvent::IsCopyable() const
{
	// nope, we can't copy this item and paste it somewhere else - it's tied to its keys
	return false;
}

wxColor CTimelineItemInterpolationEvent::GetColor() const
{
	return wxColor( 255, 100, 100 ); // temporary color to distinguish this item from others
}

Gdiplus::Bitmap* CTimelineItemInterpolationEvent::GetIcon() const
{
	return nullptr;
}

const wxBitmap* CTimelineItemInterpolationEvent::GetWxIcon() const
{
	return nullptr;
}

Float CTimelineItemInterpolationEvent::GetHeightScale() const
{
	return 0.5f;
}

void CTimelineItemInterpolationEvent::OnDeleted()
{
	// Make all key events aware that they are no longer attached as keys.
	for( auto itKey = m_keys.Begin(), endKeys = m_keys.End(); itKey != endKeys; ++itKey )
	{
		CTimelineItemEvent* tiEvent = static_cast< CTimelineItemEvent* >( *itKey );
		tiEvent->GetEvent()->SetInterpolationEventGUID( CGUID::ZERO );
	}

	CTimelineItemEvent::OnDeleted();
}

void CTimelineItemInterpolationEvent::GetMoveSet( TDynArray< ITimelineItem* >& outMoveSet )
{
	// Note we're not doing this:
	// outMoveSet.PushBackUnique( this );

	CStorySceneEventInterpolation* scInterpolationEvent = GetSceneInterpolationEvent();
	for ( Uint32 iKey = 0, numKeys = scInterpolationEvent->GetNumKeys(); iKey < numKeys; ++iKey )
	{
		CTimelineItemEvent* key = m_timeline->FindItemEvent( scInterpolationEvent->GetKeyGuid( iKey ) );
		key->GetMoveSet( outMoveSet );
	}

	const TDynArray< CGUID >& children = m_event->GetLinkChildrenGUID();
	for( auto it = children.Begin(), end = children.End(); it != end; ++it )
	{
		CTimelineItemEvent* ch = m_timeline->FindItemEvent( *it );
		ch->GetMoveSet( outMoveSet );
	}
}

// =================================================================================================
} // namespace DialogTimelineItems
// =================================================================================================
