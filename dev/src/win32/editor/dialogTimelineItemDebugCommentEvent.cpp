// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogTimelineItemDebugCommentEvent.h"
#include "../../common/game/storySceneEventDebugComment.h"

// =================================================================================================
namespace DialogTimelineItems {
// =================================================================================================

/*
Ctor.
*/
CTimelineItemDebugCommentEvent::CTimelineItemDebugCommentEvent( CEdDialogTimeline* timeline, CStorySceneEventDebugComment* scDebugCommentEvent, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
: CTimelineItemEvent( timeline, scDebugCommentEvent, elementItem, elements )
{}

/*
Dtor.
*/
CTimelineItemDebugCommentEvent::~CTimelineItemDebugCommentEvent()
{}

String CTimelineItemDebugCommentEvent::GetTypeName() const 
{
	return String( TXT( "CTimelineItemDebugCommentEvent" ) );
}

Bool CTimelineItemDebugCommentEvent::IsLeftResizable() const
{
	return IsEditable();
}

Bool CTimelineItemDebugCommentEvent::IsRightResizable() const
{
	return IsEditable();
}

Float CTimelineItemDebugCommentEvent::SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
{
	SCENE_ASSERT( IsEditable() );

	CStorySceneEventDebugComment* ev = Cast< CStorySceneEventDebugComment >( GetEvent() );

	// Section is approved, these should be the same.
	SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

	const Float requestedDuration = GetEnd() - requestedTimePos;
	ev->SetDuration( requestedDuration );
	// duration has changes so we must update it (it would be best if we edited instance event
	// so that evInst->SetDuration() would do everything that's necessary)
	m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

	SetStart( requestedTimePos, true );

	UpdatePresentation();

	return requestedTimePos;
}

Float CTimelineItemDebugCommentEvent::SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
{
	SCENE_ASSERT( IsEditable() );

	CStorySceneEventDebugComment* ev = Cast< CStorySceneEventDebugComment >( GetEvent() );

	// Section is approved, these should be the same.
	SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

	const Float requestedDuration = requestedTimePos - m_timeline->GetEventInstanceStartTime( *GetEvent() );
	ev->SetDuration( requestedDuration );
	// duration has changes so we must update it (it would be best if we edited instance event
	// so that evInst->SetDuration() would do everything that's necessary)
	m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

	UpdatePresentation();

	return requestedTimePos;
}

wxColor CTimelineItemDebugCommentEvent::GetColor() const
{
	return wxColor( 100, 100, 100 );
}

// =================================================================================================
} // namespace DialogTimelineItems
// =================================================================================================
