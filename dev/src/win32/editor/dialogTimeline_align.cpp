
#include "build.h"

#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventLookat.h"
#include "../../common/game/storySceneEventDespawn.h"
#include "../../common/game/storySceneEventFade.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventCameraBlend.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventEquipItem.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneEventMimicPose.h"
#include "../../common/game/storySceneEventMimicFilter.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventGroup.h"
#include "../../common/game/storySceneVideo.h"
#include "../../common/game/storySceneEventCameraAnimation.h"
#include "../../common/game/storySceneEventPoseKey.h"
#include "../../common/core/feedback.h"

#include "timeline.h"
#include "dialogTimeline.h"
#include "dialogEditorPage.h"
#include "dialogPreview.h"
#include "dialogEditorActions.h"
#include "undoTimeLine.h"
#include "dialogTimeline_includes.h"
#include "dialogTimeline_items.h"

using namespace DialogTimelineItems;

Float CEdDialogTimeline::Snap( Float value )
{
	Float snapResolution = GetCurrentGrid() / 16.0f / 2.0f;
	for ( TDynArray< ITimelineItem* >::iterator itemIter = m_items.Begin(); itemIter != m_items.End(); ++itemIter )	
	{
		ITimelineItem*  item = *itemIter;

		if ( m_selectedItems.Exist( item ) == true )
		{
			continue;
		}

		// snap to voice start, end or max amp position
		CTimelineItemLineElement* lineItem = dynamic_cast< CTimelineItemLineElement* >( item );
		if ( lineItem )
		{
			Float voiceStartPos = lineItem->GetStart() + lineItem->GetVoiceStartRelPos() * lineItem->GetDuration();
			Float voiceEndPos = lineItem->GetStart() + lineItem->GetVoiceEndRelPos() * lineItem->GetDuration();
			const TDynArray< TPair< Float, Float > >& voiceMaxAmpPos = lineItem->GetVoiceMaxAmpRelPos();

			for( Uint32 i = 0; i < voiceMaxAmpPos.Size() ; i++ )
			{
				Float voiceMaxAmp = lineItem->GetStart() + voiceMaxAmpPos[i].m_first* lineItem->GetDuration();
				if( Abs( voiceMaxAmp - value ) < snapResolution )
				{
					return voiceMaxAmp;
				}				
			}
			if ( Abs( voiceStartPos - value ) < snapResolution )
			{
				return voiceStartPos;
			}
			else if ( Abs( voiceEndPos - value ) < snapResolution )
			{
				return voiceEndPos;
			}
		}

		// snap to item start
		if ( ::Abs< Float >( item->GetStart() - value ) < snapResolution )
		{
			return item->GetStart();
		}
		// snap to item end
		else if ( ::Abs< Float >( item->GetStart() + item->GetDuration() - value ) < snapResolution )
		{
			return item->GetStart() + item->GetDuration();
		}
	}
	return value;
}

Float CEdDialogTimeline::ResizeItem( ITimelineItem* item, Float duration )
{
	ASSERT( IsTimelineEditingEnabled() );

	// TODO: all this should be handled by items themselves

	// Pause elements undergo some special handling - events associated with pause element:
	// a) keep their relative position if ctrl is pressed,
	// b) keep their absolute distance to pause end if alt is pressed,
	// c) keep their absolute position if neither ctrl nor alt is pressed.
	CTimelineItemPause* pauseItem = dynamic_cast< CTimelineItemPause* >( item );
	if ( pauseItem != NULL )
	{
		Bool ctrlDown = wxGetKeyState(WXK_CONTROL);
		Bool altDown = wxGetKeyState(WXK_ALT);

		if(ctrlDown)
		{
			// items associated with pause element keep their relative position - nothing to do in this case
		}
		else
		{
			// get all items associated with pause element
			TDynArray< DialogTimelineItems::CTimelineItemEvent* > events;
			Uint32 numEvents = GetElementEvents( pauseItem, events );

			if(altDown)
			{
				// items associated with pause element keep their absolute distance to pause end

				// get abs distances between all events and pause end
				TDynArray< float > eventAbsDist;
				eventAbsDist.Resize(numEvents);
				Float maxAbsDist = -1;
				for(Uint32 i = 0; i < numEvents; ++i )
				{
					eventAbsDist[i] = pauseItem->GetStart() + pauseItem->GetDuration() - events[i]->GetStart();
					if(eventAbsDist[i] > maxAbsDist)
					{
						maxAbsDist = eventAbsDist[i];
					}
				}

				// change pause duration and move its items only if none of them fall out of pause range because of this
				if(duration > maxAbsDist)
				{
					pauseItem->SetDuration( duration );

					// restore abs distance between each event and pause end
					for(Uint32 i = 0; i < numEvents; ++i)
					{
						events[i]->SetStart(pauseItem->GetStart() + pauseItem->GetDuration() - eventAbsDist[i], false);
					}
				}
			}
			else
			{
				// items associated with pause element keep their absolute position

				// get abs times of all events associated with pause element
				TDynArray< float > eventAbsTime;
				eventAbsTime.Resize(numEvents);
				Float maxAbsTime = -1;
				for(Uint32 i = 0; i < numEvents; ++i )
				{
					eventAbsTime[i] = events[i]->GetStart();
					if(eventAbsTime[i] > maxAbsTime)
					{
						maxAbsTime = eventAbsTime[i];
					}
				}

				// change pause duration and move its items only if none of them fall out of pause range because of this
				if(pauseItem->GetStart() + duration > maxAbsTime)
				{
					pauseItem->SetDuration( duration );

					// restore abs time of each event
					for(Uint32 i = 0; i < numEvents; ++i )
					{
						events[i]->SetStart(eventAbsTime[i], false);
					}
				}
			}

			// If any of events associated with pause element were moved then pause element middle text shows
			// relative position of one of moved events. We don't want that - we want pause element to show
			// length of pause. To do this we have to turn off highlight.
			pauseItem->SetHighlight(false, 0.0f);

			return pauseItem->GetDuration();
		}
	}

	CTimelineItemBlocking* blockingItem = dynamic_cast< CTimelineItemBlocking* >( item );
	if ( blockingItem != NULL )
	{
		Float newDuration = item->SetDuration( duration );
		for( TDynArray< ITimelineItem* >::iterator itemIter = m_items.Begin(); itemIter != m_items.End(); ++itemIter )
		{
			CTimelineItemEventGroup* groupItem = dynamic_cast< CTimelineItemEventGroup* >( *itemIter );
			if ( groupItem != NULL )
			{
				groupItem->UpdateGroupTimes();

				//m_mediator->OnTimeline_StartEventChanging( m_section, groupItem->GetEvent() );
			}
		}
		return newDuration;
	}
	
	return CEdTimeline::ResizeItem( item, duration );
}

const wxCursor CEdDialogTimeline::GetResizeCursor() const
{
	if ( wxIsCtrlDown() == true )
	{
		return wxCURSOR_SIZEWE;
	}
	return m_startEdgeGrabbed == true ? wxCURSOR_POINT_LEFT : wxCURSOR_POINT_RIGHT;
}

void CEdDialogTimeline::PerformClipRoll( Float offset )
{
	Float clipRollOffset = offset;
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		if ( eventItem == NULL )
		{
			continue;
		}

		CStorySceneEventAnimClip* animClipEvent = Cast< CStorySceneEventAnimClip >( eventItem->GetEvent() );
		if ( animClipEvent == NULL )
		{
			continue;
		}

		Float clippingStart = animClipEvent->GetAnimationClipStart();
		Float clippingEnd = animClipEvent->GetAnimationClipEnd();

		Float animationDuration = m_mediator->OnTimeline_GetAnimationDurationFromEvent( animClipEvent );

		if ( clippingEnd > clippingStart && clippingEnd - clippingStart < animationDuration )
		{

			if ( clipRollOffset < 0.0f && clippingStart + clipRollOffset < 0.0f )
			{
				clipRollOffset = -clippingStart;
			}
			else if ( clipRollOffset > 0.0f && clippingEnd + clipRollOffset > animationDuration )
			{
				clipRollOffset = animationDuration - clippingEnd;
			}
			clippingStart += clipRollOffset;
			clippingEnd += clipRollOffset;

			animClipEvent->SetAnimationClipStart( clippingStart );
			animClipEvent->SetAnimationClipEnd( clippingEnd );

			animClipEvent->RefreshDuration( animationDuration );
			eventItem->UpdatePresentation();
		}

		m_mediator->OnTimeline_EventChanged( m_section, animClipEvent );
	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::OnAlignStart( wxCommandEvent& event )
{
	if ( m_selectedItems.Empty() == true )
	{
		return;
	}

	// Sort selected items by time
	::Sort( m_selectedItems.Begin(), m_selectedItems.End(), TimelineItemStartSorter() );

	CTimelineItemEvent* firstItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );

	ASSERT( firstItem != NULL, TXT( "OnAlignStart() was called with item which is not an event" ) );
	if ( firstItem == NULL )
	{
		return;
	}

	for ( Uint32 i = 1; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );

		ASSERT( item != NULL, TXT( "OnAlignStart() was called with item which is not an event" ) );	
		if ( item == NULL )
		{
			continue;
		}

		item->SetStart( firstItem->GetStart(), true );

		m_mediator->OnTimeline_EventChanged( m_section, item->GetEvent() );
	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::OnAlignStartToEnd( wxCommandEvent& event )
{
	if ( m_selectedItems.Size() != 2 )
	{
		return;
	}

	// Sort selected items by time
	::Sort( m_selectedItems.Begin(), m_selectedItems.End(), TimelineItemStartSorter() );

	CTimelineItemEvent* firstItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );
	CTimelineItemEvent* secondItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 1 ] );

	ASSERT( firstItem != NULL && secondItem != NULL, TXT( "OnAlignStartToEnd() was called with item which is not an event" ) );	
	if ( firstItem != NULL && secondItem != NULL )
	{
		secondItem->SetStart( firstItem->GetEnd(), true );

		m_mediator->OnTimeline_EventChanged( m_section, secondItem->GetEvent() );
	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::OnAlignEnd( wxCommandEvent& event )
{
	if ( m_selectedItems.Empty() == true )
	{
		return;
	}

	TDynArray< ITimelineItem* > seletedItemsCopy = m_selectedItems;

	// Sort selected items by time
	::Sort( seletedItemsCopy.Begin(), seletedItemsCopy.End(), TimelineItemEndSorter() );

	CTimelineItemEvent* lastItem = dynamic_cast< CTimelineItemEvent* >( seletedItemsCopy[ 0 ] );

	ASSERT( lastItem != NULL, TXT( "OnAlignEnd() was called with item which is not an event" ) );	
	if ( lastItem == NULL )
	{
		return;
	}

	for ( Uint32 i = 1; i < seletedItemsCopy.Size(); ++i )
	{
		CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( seletedItemsCopy[ i ] );

		ASSERT( item != NULL, TXT( "OnAlignEnd() was called with item which is not an event" ) );	
		if ( item == NULL )
		{
			continue;
		}

		item->SetStart( lastItem->GetEnd() - item->GetDuration(), true );

		m_mediator->OnTimeline_EventChanged( m_section, item->GetEvent() );
	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::OnAlignToCamera( wxCommandEvent& event )
{
	ITimelineItem* itemToAlignTo = NULL;
	for ( Uint32 i = 1; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemCamera* cameraItem = dynamic_cast< CTimelineItemCamera* >( m_selectedItems[ i ] );
		CTimelineItemCameraBlend* cameraBlendItem = dynamic_cast< CTimelineItemCameraBlend* >( m_selectedItems[ i ] );
		CTimelineItemEnhancedCameraBlend* enhancedCameraBlendItem = dynamic_cast< CTimelineItemEnhancedCameraBlend* >( m_selectedItems[ i ] );
		CTimelineItemCustomCamera* customCameraItem = dynamic_cast< CTimelineItemCustomCamera* >( m_selectedItems[ i ] );
		CTimelineItemCustomCameraInstance* customCameraInstanceItem = dynamic_cast< CTimelineItemCustomCameraInstance* >( m_selectedItems[ i ] );

		if ( cameraItem != NULL || cameraBlendItem != NULL || enhancedCameraBlendItem != NULL || customCameraItem != NULL || customCameraInstanceItem != NULL )
		{
			if ( itemToAlignTo == NULL )
			{
				itemToAlignTo = m_selectedItems[ i ];
			}
			else
			{
				GFeedback->ShowError( TXT("More than one camera found in selection. Cannot perform align to camera" ) );
				return;
			}
		}
	}

	if ( itemToAlignTo != NULL )
	{
		for ( Uint32 i = 1; i < m_selectedItems.Size(); ++i )
		{
			CTimelineItemEvent* itemEvent = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
			if ( itemEvent == itemToAlignTo )
			{
				continue;
			}

			itemEvent->SetStart( itemToAlignTo->GetStart(), true );

			m_mediator->OnTimeline_EventChanged( m_section, itemEvent->GetEvent() );
		}

		NotifyBufferIsInvalid();
	}
}

void CEdDialogTimeline::OnAlignEndToSectionsEnd( wxCommandEvent& event )
{
	if ( !IsOneSelected() )
	{
		return;
	}

	CTimelineItemEvent* item = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ 0 ] );
	if ( item )
	{
		const Float time = m_activeRangeDuration - item->GetDuration();
		item->SetStart( time, true );

		m_mediator->OnTimeline_EventChanged( m_section, item->GetEvent() );
	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::OnSetClippingFront( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		if ( eventItem == NULL )
		{
			continue;
		}

		CStorySceneEventAnimClip* animClipEvent = Cast< CStorySceneEventAnimClip >( eventItem->GetEvent() );
		if ( animClipEvent == NULL )
		{
			continue;
		}

		animClipEvent->SetAnimationClipStart( m_timeSelectorTimePos - eventItem->GetStart() + animClipEvent->GetAnimationClipStart() );

		eventItem->SetStart( m_timeSelectorTimePos, true );

		m_mediator->OnTimeline_EventChanged( m_section, animClipEvent );
	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::OnSetClippingBack( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		if ( eventItem == NULL )
		{
			continue;
		}

		CStorySceneEventAnimClip* animClipEvent = Cast< CStorySceneEventAnimClip >( eventItem->GetEvent() );
		if ( animClipEvent == NULL )
		{
			continue;
		}

		animClipEvent->SetAnimationClipEnd( m_timeSelectorTimePos - eventItem->GetStart() + animClipEvent->GetAnimationClipStart() );
		
		m_mediator->OnTimeline_EventChanged( m_section, animClipEvent );

	}

	NotifyBufferIsInvalid();
}

void CEdDialogTimeline::OnCutAnimClip( wxCommandEvent& event )
{
	ASSERT( IsTimelineEditingEnabled() );

	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CTimelineItemEvent* eventItem = dynamic_cast< CTimelineItemEvent* >( m_selectedItems[ i ] );
		if ( eventItem == NULL || m_timeSelectorTimePos <= eventItem->GetStart() || m_timeSelectorTimePos >= eventItem->GetEnd() )
		{
			continue;
		}

		CStorySceneEventAnimation* animationEvent = Cast< CStorySceneEventAnimation >( eventItem->GetEvent() );
		if ( animationEvent == NULL )
		{
			continue;
		}

		CStorySceneEventAnimation* newEvent = new CStorySceneEventAnimation( 
			animationEvent->GetEventName(), 
			animationEvent->GetSceneElement(), 
			animationEvent->GetStartPosition(), 
			animationEvent->GetActor(), 
			animationEvent->GetAnimationName().AsString(), 
			animationEvent->GetTrackName() );

		// DIALOG_TOMSIN_TODO - do wyjabania
		newEvent->SetAnimationClipStart( animationEvent->GetAnimationClipStart() );
		newEvent->SetAnimationClipEnd( animationEvent->GetAnimationClipEnd() );
		newEvent->SetAnimationBlendIn( animationEvent->GetAnimationBlendIn() );
		newEvent->SetAnimationBlendOut( animationEvent->GetAnimationBlendOut() );
		newEvent->SetAnimationStretch( animationEvent->GetAnimationStretch() ); // section is approved so GetAnimationStretch() == instance animation stretch

		// DIALOG_TOMSIN_TODO - tylko to
		newEvent->CopyFrom( animationEvent );

		CTimelineItemEvent* newEventItem = CreateTimelineItemEvent( newEvent, eventItem->GetElementItem() );
		AddItem( newEventItem );

		newEvent->SetAnimationBlendIn( 0.0f );
		newEvent->SetAnimationClipStart( ( m_timeSelectorTimePos - eventItem->GetStart() ) / newEvent->GetAnimationStretch() + newEvent->GetAnimationClipStart() );  // section is approved so GetAnimationStretch() == instance animation stretch

		newEventItem->SetStart( m_timeSelectorTimePos, true );

		animationEvent->SetAnimationBlendOut( 0.0f );
		animationEvent->SetAnimationClipEnd( ( m_timeSelectorTimePos - eventItem->GetStart() ) / animationEvent->GetAnimationStretch() + animationEvent->GetAnimationClipStart() );  // section is approved so GetAnimationStretch() == instance animation stretch
		
		m_mediator->OnTimeline_AddEvent( m_section, newEvent, GetSectionVariantId() );
	}

	NotifyBufferIsInvalid();
}
