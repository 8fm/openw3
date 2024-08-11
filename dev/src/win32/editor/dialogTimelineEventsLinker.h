
#pragma once

#include "timeline.h"

class CEdDialogTimeline;

namespace DialogTimelineItems
{
	class CTimelineItemEvent;
}

class DialogTimelineEventsLinker
{
	CEdDialogTimeline*						m_timeline;

	DialogTimelineItems::CTimelineItemEvent*					m_itemToLink;
	TDynArray< const DialogTimelineItems::CTimelineItemEvent* >	m_selectedChildEvents;
	TDynArray< const DialogTimelineItems::CTimelineItemEvent* >	m_selectedParentEvents;
	TDynArray< wxRect >						m_selectedChildRects;						// Rects are in global space, i.e. timeline space.
	TDynArray< wxRect >						m_selectedParentRects;						// Rects are in global space, i.e. timeline space.
	TDynArray< Bool >						m_selectedChildValid;
	TDynArray< Bool >						m_selectedParentValid;

public:
	DialogTimelineEventsLinker( CEdDialogTimeline* timeline );

	Bool IsRunning() const;
	void Reset();

	void StartLinkingProcess( DialogTimelineItems::CTimelineItemEvent* e );
	void StopLinkingProcess();

	void LinkEvent( DialogTimelineItems::CTimelineItemEvent* parent, DialogTimelineItems::CTimelineItemEvent* child ) const;
	void UnlinkEvent( DialogTimelineItems::CTimelineItemEvent* item );
	void UnlinkAllChildrenEvents( DialogTimelineItems::CTimelineItemEvent* parentItem );

	void PrintCanvas( const wxPoint& cursorPosition );

public:
	void OnSelectionChanged( const TDynArray< ITimelineItem* >& selectedItems );
	void OnDrawItem( const ITimelineItem* item, const wxRect& rect );

private:
	void FinilizeLinkingProcess( CStorySceneEvent* e );
};
