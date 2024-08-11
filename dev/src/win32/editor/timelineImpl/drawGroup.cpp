// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "drawGroup.h"
#include "drawBuffer.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

/*
Converts global position to local position.

Global position - position in CEdTimeline space.
Local position - position in space of CDrawBuffer used by this CDrawGroup.
*/
wxPoint CDrawGroup::GetLocalPos(wxPoint globalPos) const
{
	ASSERT(m_drawBuffer);

	wxPoint origin(0, 0);
	GetDrawBuffer()->GetTargetPos(origin);
	return globalPos - origin;
}

/*
Converts local position to global position.

Global position - position in CEdTimeline space.
Local position - position in space of CDrawBuffer used by this CDrawGroup.
*/
wxPoint CDrawGroup::GetGlobalPos(wxPoint localPos) const
{
	ASSERT(m_drawBuffer);

	wxPoint origin(0, 0);
	GetDrawBuffer()->GetTargetPos(origin);
	return origin + localPos;
}

/*
Converts global rectangle to local rectangle.

Global space - CEdTimeline space.
Local space - space of CDrawBuffer used by this CDrawGroup.
*/
wxRect CDrawGroup::GetLocalRect(const wxRect& globalRect) const
{
	ASSERT(m_drawBuffer);

	wxPoint localPos = GetLocalPos(globalRect.GetPosition());
	return wxRect(localPos.x, localPos.y, globalRect.width, globalRect.height);
}

/*
Converts local rectangle to global rectangle.

Global space - CEdTimeline space.
Local space - space of CDrawBuffer used by this CDrawGroup.
*/
wxRect CDrawGroup::GetGlobalRect(const wxRect& localRect) const
{
	ASSERT(m_drawBuffer);

	wxPoint globalPos = GetGlobalPos(localRect.GetPosition());
	return wxRect(globalPos.x, globalPos.y, localRect.width, localRect.height);
}

/*
Gets items in rectangle.

\param localRect Rectangle in local space.
\param itemsInRect (out) Container where items in rectangle are to be stored. Container is not cleared before use.
\return Number of items in rectangle.
*/
size_t CDrawGroup::GetItemsInRect(const wxRect& localRect, TDynArray<ITimelineItem*>& itemsInRect) const
{
	return 0;
}

void CDrawGroup::OnLeftMouseClick(wxPoint localPos)
{}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
