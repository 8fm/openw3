// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "dialogTimeline_items.h" // TODO: this is only because CTimelineItemEvent is defined there
								  // CTimelineItemEvent (and some others) should be defined in their own files.

class CStorySceneEventDebugComment;

// =================================================================================================
namespace DialogTimelineItems {
// =================================================================================================

class CTimelineItemDebugCommentEvent : public CTimelineItemEvent
{
public:
	CTimelineItemDebugCommentEvent( CEdDialogTimeline* timeline, CStorySceneEventDebugComment* scDebugCommentEvent, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements );
	virtual ~CTimelineItemDebugCommentEvent() override;

	virtual String GetTypeName() const override;
	virtual Bool IsLeftResizable() const override;
	virtual Bool IsRightResizable() const override;
	virtual Float SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual Float SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual wxColor GetColor() const override;
};

// =================================================================================================
} // namespace DialogTimelineItems
// =================================================================================================
