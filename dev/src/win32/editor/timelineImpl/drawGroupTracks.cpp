// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "drawGroupTracks.h"
#include "drawBuffer.h"
#include "drawing.h"
#include "../timeline.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================
namespace {
// =================================================================================================
namespace BasicSizes {
// =================================================================================================

// note those are not meant to be used directly
const Int32 trackHeight = 50; // please use CDrawGroupTracks::GetTrackHeight() instead
const Int32 itemHeight = 24;  // please use CDrawGroupTracks::GetItemHeight() instead

// =================================================================================================
} // namespace Sizes
// =================================================================================================

/*
Helper function that simplifies some code.
*/
wxRect& OffsetRect(wxRect& rect, Int32 dx, Int32 dy)
{
	rect.Offset(dx, dy);
	return rect;
}

/*
Exchange track positions in specified container.

\param tA First track. Must not be 0. Must exist in arr.
\param tB Second track. Must not be 0. Must exist in arr.

Preconditions:
1. tA != tB
2. tA->m_parent == tB->m_parent (tracks must belong to the same track group or not belong to any track group)
*/
void ExchangePositions(TDynArray<Track*>& arr, Track* tA, Track* tB)
{
	ASSERT(tA && tB);
	ASSERT(tA != tB);
	ASSERT(arr.Exist(tA) && arr.Exist(tB));
	ASSERT(tA->m_parent == tB->m_parent);

	Uint32 indexA = arr.GetIndex(tA);
	Uint32 indexB = arr.GetIndex(tB);

	// arrange things so that tA indexes are greater that tB indexes (this will make things simpler)
	if(indexB > indexA)
	{
		Track* tempTrack = tA;
		tA = tB;
		tB = tempTrack;

		Uint32 tempIndex = indexA;
		indexA = indexB;
		indexB = tempIndex;
	}

	const Uint32 arrSize = arr.Size();

	// count a tracks
	Uint32 numTracksA = 1;
	while(indexA + numTracksA < arrSize && IsRelationship(tA, arr[indexA + numTracksA]))
	{
		++numTracksA;
	}

	// count b tracks
	Uint32 numTracksB = 1;
	while(indexB + numTracksB < arrSize && IsRelationship(tB, arr[indexB + numTracksB]))
	{
		++numTracksB;
	}

	// exchange one for one until we can
	Uint32 numExchanged = 0;
	while(numExchanged < numTracksA && numExchanged < numTracksB)
	{
		Swap(arr[indexA + numExchanged], arr[indexB + numExchanged]);
		++numExchanged;
	}

	// now handle the rest
	if(numTracksA == numTracksB)
	{
		// we're done exchanging
	}
	else if(numExchanged == numTracksA)
	{
		// a tracks done, move b tracks
		Uint32 removalOffset = indexB + numExchanged;
		Uint32 insertionOffset = indexA + numExchanged - 1;
		while(numExchanged < numTracksB)
		{
			Track* t = arr[removalOffset];
			arr.RemoveAt(removalOffset);
			arr.Insert(insertionOffset, t);
			++numExchanged;
		}
	}
	else if(numExchanged == numTracksB)
	{
		// b tracks done, move a tracks
		while(numExchanged < numTracksA)
		{
			Track* t = arr[indexA + numExchanged];
			arr.RemoveAt(indexA + numExchanged);
			arr.Insert(indexB + numExchanged, t);
			++numExchanged;
		}
	}
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

void CDrawGroupTracks::Draw()
{
	DrawBackground();
	DrawGrid();
	DrawItems();
	DrawTracks();
}

void CDrawGroupTracks::DrawBackground()
{
	ASSERT(GetDrawBuffer());

	Int32 width, height;
	GetDrawBuffer()->GetSize(width, height);

	GetDrawBuffer()->Clear(wxColour(150, 150, 150));
	GetDrawBuffer()->DrawRect(0, 0, width, height, wxColour(0, 0, 0));

	Int32 animStart = GetOwner().CalculatePixelPos(0.0f);
	Int32 animEnd = GetOwner().CalculatePixelPos(GetOwner().GetActiveRangeDuration());

	// Draw animation range
	Int32 trackBtnWidth = GetOwner().TIMELINE_TRACK_BTN_WIDTH;
	GetDrawBuffer()->FillRect(Max(animStart, trackBtnWidth), 0, animEnd - Max(animStart, trackBtnWidth), height, wxColour(180, 180, 180));
	GetDrawBuffer()->DrawLine(Max(animStart, trackBtnWidth), 0, Max(animStart, trackBtnWidth), height, wxColour(0, 0, 0), 2.0f);
	GetDrawBuffer()->DrawLine(animEnd - 1, 0, animEnd - 1, height, wxColour(0, 0, 0), 2.0f);
}

void CDrawGroupTracks::DrawGrid()
{
	Int32 width, height;
	GetDrawBuffer()->GetSize(width, height);

	Int32 animStart = GetOwner().CalculatePixelPos(0.0f);
	Int32 animEnd =  GetOwner().CalculatePixelPos(GetOwner().GetActiveRangeDuration());

	Int32 num = 0;

	for(Float t = 0.0f; t < GetOwner().GetVisibleRangeDuration() - GetOwner().m_activeRangeTimeOffset; t += GetOwner().m_currentGrid / 16.0f, ++num)
	{
		Int32 i = GetOwner().CalculatePixelPos(t);

		// Skip lines which are not in animation range
		if(i < animStart || i > animEnd)
		{
			// Check to avoid lock
			if ( GetOwner().CalculatePixelPos(t) == GetOwner().CalculatePixelPos(t + GetOwner().m_currentGrid / 16.0f) )
			{
				break;
			}

			continue;
		}

		// Skip invisible lines and these under track buttons
		if(i < GetOwner().TIMELINE_TRACK_BTN_WIDTH)
		{
			continue;
		}

		if(num % 16 == 0)
		{
			// Draw main line
			GetDrawBuffer()->DrawLine(i, 0, i, height, wxColour(0, 0, 0), 2.0f);
		}
		else if(num % 4 == 0)
		{
			// Draw half line
			GetDrawBuffer()->DrawLine(i, 0, i, height, wxColour(0, 0, 0), 1.0f);
		}
		else
		{
			// Draw help line
			GetDrawBuffer()->DrawLine(i, 0, i, height, wxColour(150, 150, 150));
		}
	}
}

void CDrawGroupTracks::DrawTracks()
{
	Int32 timelineTrackBtnWidth = GetOwner().TIMELINE_TRACK_BTN_WIDTH;
	Int32 timelineDepthSpacing = GetOwner().TIMELINE_DEPTH_SPACING;
	Int32 timelineExpandBtnSize = GetOwner().TIMELINE_EXPAND_BTN_SIZE;
	Int32 timelineExpandBtnCrossThickness = GetOwner().TIMELINE_EXPAND_BTN_CROSS_THICKNESS;

	Int32 width, height;
	GetDrawBuffer()->GetSize(width, height);

	// draw background of track buttons
	GetDrawBuffer()->FillRect(0, 0, timelineTrackBtnWidth, height, wxColour(170, 170, 170));

	// for all tracks
	Int32 trackSlot = 0;
	Uint32 numDispTracks = GetNumDispTracks();
	for(Uint32 trackIter = 0; trackIter < numDispTracks; ++trackIter, ++trackSlot)
	{
		Track* track = GetDispTrack(trackIter);
		Int32 trackHeight = GetTrackHeight(track);
		Int32 trackPosY = GetDispTrackLocalPos(track).y;

		Int32 indent = track->m_depth * timelineDepthSpacing;
		Int32 tw = timelineTrackBtnWidth - indent;

		// draw separator
		GetDrawBuffer()->DrawLine(indent, trackPosY, width, trackPosY, wxColour(0, 0, 0), 1.0f);

		// Draw 'button'
		wxColour trackBgColor;
		Bool isSelected = GetOwner().m_selectedTrack > -1 && track == GetOwner().m_tracks[GetOwner().m_selectedTrack];
		if(track->m_isGroup)
		{
			trackBgColor = isSelected ? wxColour(161, 12, 21) : wxColour(139, 126, 127);
		}
		else
		{
			trackBgColor = isSelected ? wxColour(161, 12, 21) : wxColour(172, 74, 74);
		}
		GetDrawBuffer()->FillRect(indent, trackPosY + 1, tw, trackHeight - 1, trackBgColor);

		if(track->m_depth > 0)
		{
			GetDrawBuffer()->DrawLine(indent, trackPosY , indent, trackPosY + trackHeight, wxColour(0, 0, 0));
			Int32 n  = track->m_depth;

			Uint32 it2 = trackIter;
			++it2;
			GetDrawBuffer()->DrawLine((n - 1) * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY + (trackHeight >> 1), n * timelineDepthSpacing, trackPosY + (trackHeight >> 1), wxColour(64, 64, 64));
			if(it2 >= numDispTracks ||  GetDispTrack(it2)->m_depth < track->m_depth)
			{
				--n;
				GetDrawBuffer()->DrawLine(n * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY, n * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY + (trackHeight >> 1), wxColour(64, 64, 64));
			}
			for(Int32 i = 0; i < n; ++i)
			{
				GetDrawBuffer()->DrawLine(i * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY , i * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY + trackHeight, wxColour(64, 64, 64));
			}
		}

		// draw track name

		// name of non-actor track is displayed after pin/unpin button
		wxRect rect = GetPinBtnRect(track);
		rect.x += rect.width + 5;
		// btnRect is in track space so we have to offset it to get it in local space
		rect.Offset(0, trackPosY);

		String name;
		size_t strPos;
		if(track->m_name.FindCharacter(GetOwner().GROUP_SEPARATOR, strPos, true))
		{
			name = track->m_name.MidString(strPos + 1, track->m_name.Size());
		}
		else
		{
			name = track->m_name;
		}

		if(GetDrawBuffer()->GetType() == CanvasType::gdi)
		{
			GetDrawBuffer()->DrawText(rect.GetTopLeft(), GetOwner().GetWxDrawFont(), name, wxColour(255, 255, 255));
		}
		else
		{
			GetDrawBuffer()->DrawText(rect.GetTopLeft(), GetOwner().GetGdiDrawFont(), name, wxColour(255, 255, 255));
		}

		// draw pinned/unpinned icon but don't draw it for tracks inside groups
		if(!track->m_parent)
		{
			wxRect btnRect = GetPinBtnRect(track);
			// btnRect is in track space so we have to offset it to get it in local space
			btnRect.Offset(0, trackPosY);

			if(GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				GetDrawBuffer()->DrawImage(GetOwner().IsTrackPinned(track)? &GetOwner().m_wxIconPinned : &GetOwner().m_wxIconUnpinned, btnRect.x, btnRect.y, btnRect.width, btnRect.height);
			}
			else
			{
				GetDrawBuffer()->DrawImage(GetOwner().IsTrackPinned(track)? GetOwner().m_iconPinned : GetOwner().m_iconUnpinned, btnRect.x, btnRect.y, btnRect.width, btnRect.height);
			}
		}

		// draw expand/collapse button
		if(track->m_isGroup)
		{
			wxRect btnRect = GetExpandBtnRect(track);
			// btnRect is in track space so we have to offset it to get it in local space
			btnRect.Offset(0, trackPosY);

			GetDrawBuffer()->FillRect(btnRect.x, btnRect.y, btnRect.width, btnRect.height, wxColour(128, 128, 128));
			GetDrawBuffer()->DrawRect(btnRect.x, btnRect.y, btnRect.width, btnRect.height, wxColour(0, 0, 0));
			GetDrawBuffer()->FillRect(
				btnRect.x,
				btnRect.y + ((btnRect.height - timelineExpandBtnCrossThickness) >> 1),
				btnRect.width, timelineExpandBtnCrossThickness, wxColour(0, 0, 0));
			if(!track->m_isExpanded)
			{
				GetDrawBuffer()->FillRect(
					btnRect.x + ((btnRect.width - timelineExpandBtnCrossThickness) >> 1),
					btnRect.y,
					timelineExpandBtnCrossThickness, btnRect.height, wxColour(0, 0, 0));
			}
		}

		GetOwner().OnDrawTrack(track);
	}

	// draw top line
	GetDrawBuffer()->DrawLine(0, 0, width, 0, wxColour(0, 0, 0), 1.0f);

	// bottom line
	if(numDispTracks > 0)
	{
		Track* lastTrack = GetDispTrack(numDispTracks - 1);
		Int32 y = GetDispTrackLocalPos(lastTrack).y + GetTrackHeight(lastTrack) - 1;
		GetDrawBuffer()->DrawLine(0, y, width, y, wxColour(0, 0, 0), 1.0f);
	}
}

void CDrawGroupTracks::DrawItems()
{
	// layer 0 - duration events
	// layer 1 - point events
	// layer 2 - selected duration events
	// layer 3 - selected point events
	const Uint32 numLayers = 4;
	TDynArray<ITimelineItem*> layers[numLayers];

	// for all disp tracks
	Uint32 numDispTracks = GetNumDispTracks();
	for(Uint32 itTrack = 0; itTrack < numDispTracks; ++itTrack)
	{
		const Track* track = GetDispTrack(itTrack);

		// for all items
		for(TDynArray<ITimelineItem*>::const_iterator itItem = GetOwner().m_items.Begin(); itItem != GetOwner().m_items.End(); ++itItem)
		{
			ITimelineItem* item = *itItem;
			if(item)
			{
				String itemTrackName = item->GetTrackName().Empty()? GetOwner().TIMELINE_DEFAULT_TRACK_NAME : item->GetTrackName();
				if(itemTrackName == track->m_name)
				{
					// item belongs to this track - push it to appropriate layer
					Uint32 itemLayer = item->IsDuration()? 0 : 1;
					itemLayer += GetOwner().IsSelected(item)? 2 : 0;
					layers[itemLayer].PushBack(item);
				}
			}
		}

		// draw all layers
		for(Uint32 i = 0; i < numLayers; ++i)
		{
			for(TDynArray<ITimelineItem*>::const_iterator itItem = layers[i].Begin(); itItem != layers[i].End(); ++itItem)
			{
				DrawItem(*itItem);
			}
			layers[i].ClearFast();
		}
	}
}

/*

\return Track slot number or -1 if track is not in this CDrawGroupTracks or is in a track group that is collapsed.
*/
Int32 CDrawGroupTracks::GetDispTrackIndex(Track* track) const
{
	return m_dispTracks.GetIndex(track);
}

Track* CDrawGroupTracks::GetTrackAt(wxPoint localPos) const
{
	ASSERT(GetDrawBuffer());

	Uint32 i = 0;
	Uint32 numDispTracks = GetNumDispTracks();
	wxPoint trackPos(0, -GetDrawBuffer()->GetVerticalOffset());	// local pos of first track

	while(i < numDispTracks)
	{
		Track* track = GetDispTrack(i);
		Int32 trackHeight = GetTrackHeight(track);

		if(trackPos.y <= localPos.y && localPos.y <= trackPos.y + trackHeight)
		{
			return track;
		}

		// position of next track
		trackPos.y += trackHeight;

		++i;
	}

	return 0;
}

/*
Gets items at specified position.

\param localPos Position in question, in local space.
\param items Container to which to put items. It's not cleared before use.
\return Number of items found at specified position (and put to items container).
*/
Uint32 CDrawGroupTracks::GetItemsAt(wxPoint localPos, TDynArray<ITimelineItem*>& items) const
{
	Track* track = GetTrackAt(localPos);
	Uint32 numItemsFound = 0;

	if(track)
	{
		const wxPoint trackLocalPos = GetDispTrackLocalPos( track );
		const Int32 trackHeight = GetTrackHeight( track );
		const Int32 itemHeight = GetItemHeight( track );

		// for all items
		for(TDynArray<ITimelineItem*>::iterator itItem = GetOwner().m_items.Begin(); itItem != GetOwner().m_items.End(); ++itItem)
		{
			ITimelineItem* item = *itItem;

			// Skip invalid events
			if(item == NULL)
			{
				continue;
			}

			// Skip events on other tracks
			ASSERT(! item->GetTrackName().Empty());
			if(item->GetTrackName() != track->m_name)
			{
				continue;
			}

			// Check height
			const Float itemHeightScale = item->GetHeightScale();
			if ( itemHeightScale != 1.0f &&
				localPos.y < trackLocalPos.y + trackHeight - (Int32) ( (Float) itemHeight * itemHeightScale ) )
			{
				continue;
			}

			Int32 startPos = GetOwner().CalculatePixelPos(item->GetStart());

			// Check if it's a duration event
			if(item->IsDuration())
			{
				Int32 endPos = GetOwner().CalculatePixelPos(Clamp(item->GetStart() + item->GetDuration(), 0.0f, GetOwner().GetActiveRangeDuration()));
				if(localPos.x > startPos && localPos.x < endPos)
				{
					items.PushBack(item);
					++numItemsFound;
				}
				// Check second part (if exists)
				else if(item->GetStart() + item->GetDuration() > GetOwner().GetActiveRangeDuration())
				{
					Float dur = item->GetStart() + item->GetDuration() - GetOwner().GetActiveRangeDuration();

					Int32 startPos = Clamp(GetOwner().CalculatePixelPos(0.0f), GetOwner().TIMELINE_TRACK_BTN_WIDTH, INT_MAX);
					Int32 endPos = GetOwner().CalculatePixelPos(dur);

					if(localPos.x > startPos && localPos.x < endPos)
					{
						items.PushBack(item);
						++numItemsFound;
					}
				}
			}
			else
			{
				// Check if click in event range
				if(Abs(localPos.x - startPos) < 8)
				{
					items.PushBack(item);
					++numItemsFound;
				}
			}
		}
	}

	return numItemsFound;
}

/*
Gets item at specified position.

The function will choose which event to return if there are many items at specified position.
Point events have preference over duration events.
*/
ITimelineItem* CDrawGroupTracks::GetItemAt(wxPoint localPos) const
{
	TDynArray<ITimelineItem*> candidates;
	GetItemsAt(localPos, candidates);

	if(!candidates.Empty())
	{
		Uint32 numCandidates = candidates.Size();

		// choose best candidate - give preference to point events (as duration events can be easily selected by clicking somewhere else)
		if(numCandidates > 1)
		{
			for(Uint32 i = 0; i < numCandidates; ++i)
			{
				if(!candidates[i]->IsDuration())
				{
					return candidates[i];
				}
			}
		}

		// there's only one candidate or there were no point events among candidates
		return candidates[0];
	}

	return 0;
}

/*
Gets items in rectangle.

\param localRect Rectangle in local space.
\param itemsInRect (out) Container where items in rectangle are to be stored. Container is not cleared before use.
\return Number of items in rectangle (number of items pushed back to container).
*/
size_t CDrawGroupTracks::GetItemsInRect(const wxRect& localRect, TDynArray<ITimelineItem*>& itemsInRect) const
{
	wxRect r = localRect;

	// Crop local rect to get only the part that intersects with visible part of draw group.
	if(r.y < 0)
	{
		r.height += r.y;
		if(r.height < 0)
		{
			r.height = 0;
		}
		r.y = 0;
	}

	Uint32 numDispTracks = GetNumDispTracks();
	for(Uint32 i = 0; i < numDispTracks; ++i)
	{
		Track* track = GetDispTrack(i);
		Int32 trackCenterY = GetDispTrackLocalPos(track).y + GetTrackHeight(track) / 2;
		if(r.GetTop() <= trackCenterY && trackCenterY <= r.GetBottom())
		{
			TDynArray<ITimelineItem*> trackItems;
			GetOwner().GetTrackItems(track, trackItems);

			Uint32 numTrackItems = trackItems.Size();
			for(Uint32 j = 0; j < numTrackItems; ++j)
			{
				ITimelineItem* item = trackItems[j];
				Int32 itemX = GetOwner().CalculatePixelPos(item->GetStart());
				if(r.GetLeft() <= itemX && itemX <= r.GetRight())
				{
					itemsInRect.PushBack(item);
				}
			}
		}
	}

	return 0;
}

Bool CDrawGroupTracks::TrackExists(Track* track) const
{
	for(Uint32 i = 0; i < m_tracks.Size(); ++i)
	{
		if(m_tracks[i] == track)
		{
			return true;
		}
	}
	return false;
}

Int32 CDrawGroupTracks::GetTrackIndex(Track* track) const
{
	for(Uint32 i = 0; i < m_tracks.Size(); ++i)
	{
		if(m_tracks[i] == track)
		{
			return i;
		}
	}
	return -1;
}

/*
Exchange track positions.

\param tA First track. Must not be 0. Must belong to this draw group.
\param tB Second track. Must not be 0. Must belong to this draw group.

Preconditions:
1. tA != tB
2. tA->m_parent == tB->m_parent (tracks must belong to the same track group or not belong to any track group)
*/
void CDrawGroupTracks::ExchangePositions(Track* tA, Track* tB)
{
	ASSERT(tA && tB);
	ASSERT(tA != tB);
	ASSERT(TrackExists(tA) && TrackExists(tB));
	ASSERT(tA->m_parent == tB->m_parent);

	TimelineImpl::ExchangePositions(m_dispTracks, tA, tB);
	TimelineImpl::ExchangePositions(m_tracks, tA, tB);
}

/*
Returns preferred height of group.

Preferred height of group depends on number of tracks it contains.
*/
Int32 CDrawGroupTracks::GetPreferredHeight() const
{
	Int32 preferredHeight = 0;

	Uint32 numDispTracks = GetNumDispTracks();
	for(Uint32 i = 0; i < numDispTracks; ++i)
	{
		preferredHeight += GetTrackHeight(GetDispTrack(i));
	}

	return preferredHeight;
}

void CDrawGroupTracks::DrawItem(const ITimelineItem* item)
{
	ASSERT(item != NULL);

	// Get event track name. 'Default' if empty.
	String trackName = item->GetTrackName().Empty() 
		? GetOwner().TIMELINE_DEFAULT_TRACK_NAME 
		: item->GetTrackName();

	Track* track = GetOwner().m_tracks[GetOwner().GetTrackIndex(trackName)];

	// assert we're not trying to draw item that is on a track that is not displayed or that doesn't belong to this draw group
	ASSERT(GetDispTrackIndex(track) >= 0);

	wxPoint trackLocalPos = GetDispTrackLocalPos(track);

	// If event is selected, light it up
	const wxColour color = !GetOwner().IsSelected(item) ? item->GetColor() : wxColour(255, 255, 0, 150);
	const wxColour borderColor = item->GetBorderColor();

	Int32 trackHeight  = GetTrackHeight(track);
	Int32 itemHeight   = (Int32) ( (Float) GetItemHeight(track) * item->GetHeightScale() );
	Int32 imageSize    = itemHeight;

	Int32 itemPosY = trackLocalPos.y + trackHeight - itemHeight - 2;
	Int32 imgPosY  = itemPosY + itemHeight/2 - imageSize/2;

	if(item->IsDuration())
	{
		// Draw part on the right side of timeline (if event is overlapping)
		Int32 startPos = GetOwner().CalculatePixelPos(item->GetStart());
		Int32 endPos = GetOwner().CalculatePixelPos(Min(item->GetStart() + item->GetDuration(), GetOwner().GetActiveRangeDuration()));

		{
			wxRect rect(startPos, itemPosY, endPos - startPos, itemHeight);

			// Draw event rectangle
			GetDrawBuffer()->FillRect(rect, color);
			GetDrawBuffer()->DrawRect(rect, borderColor);

			// Draw icons
			if(GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				if(item->GetWxIcon() != NULL)
				{
					for(Int32 i = startPos + 30; i < endPos - 30; i += 48)
					{
						GetDrawBuffer()->DrawImage(item->GetWxIcon(), i, imgPosY, imageSize, imageSize);
					}
				}
			}
			else
			{
				if(item->GetIcon() != NULL)
				{
					for(Int32 i = startPos + 30; i < endPos - 30; i += 48)
					{
						GetDrawBuffer()->DrawImage(item->GetIcon(), i, imgPosY, imageSize, imageSize);
					}
				}
			}
		}

		Int32 timelineEventGrabableWidth = GetOwner().TIMELINE_EVENT_GRABABLE_WIDTH;

		// Draw grabable areas
		if(GetOwner().IsSelected(item) && item->IsLeftResizable() && GetOwner().CalculatePixelPos(GetOwner().GetActiveRangeDuration()) - startPos > timelineEventGrabableWidth)
		{
			wxRect rect1(startPos, itemPosY, timelineEventGrabableWidth, itemHeight);
			GetDrawBuffer()->FillRect(rect1, wxColour(Max(0, color.Red() - 60), Max(0, color.Green() - 60), Max(0, color.Blue() - 60)));
		}

		// Draw only if event is no overlapping
		if(GetOwner().CalculatePixelPos(GetOwner().GetActiveRangeDuration()) - GetOwner().CalculatePixelPos(item->GetStart() + item->GetDuration()) > -5)
		{
			if(GetOwner().IsSelected(item) && item->IsRightResizable())
			{
				wxRect rect2(GetOwner().CalculatePixelPos(item->GetStart() + item->GetDuration()) - timelineEventGrabableWidth,
					itemPosY, timelineEventGrabableWidth, itemHeight);
				GetDrawBuffer()->FillRect(rect2, wxColour(Max(0, color.Red() - 60), Max(0, color.Green() - 60), Max(0, color.Blue() - 60)));
			}
		}
		// Draw second part
		else
		{
			Float dur = item->GetStart() + item->GetDuration() - GetOwner().GetActiveRangeDuration();

			Int32 startPos = GetOwner().CalculatePixelPos(0.0f);
			Int32 endPos = GetOwner().CalculatePixelPos(dur);

			{
				wxRect rect(startPos, itemPosY, endPos - startPos, itemHeight);

				// Draw event rectangle
				GetDrawBuffer()->FillRect(rect, color);
				GetDrawBuffer()->DrawRect(rect, wxColour(0, 0, 0));

				// Draw icons
				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					if(item->GetWxIcon() != NULL)
					{
						for(Int32 i = startPos + 30; i < endPos - 30; i += 48)
						{
							GetDrawBuffer()->DrawImage(item->GetWxIcon(), i, itemPosY, imageSize, imageSize);
						}
					}
				}
				else
				{
					if(item->GetIcon() != NULL)
					{
						for(Int32 i = startPos + 30; i < endPos - 30; i += 48)
						{
							GetDrawBuffer()->DrawImage(item->GetIcon(), i, itemPosY, imageSize, imageSize);
						}
					}
				}
				
			}

			// Draw grabable areas
			if(GetOwner().IsSelected(item) && item->IsRightResizable())
			{
				wxRect rect2(endPos - timelineEventGrabableWidth,
					itemPosY, timelineEventGrabableWidth, itemHeight);
				GetDrawBuffer()->FillRect(rect2, wxColour(Max(0, color.Red() - 60), Max(0, color.Green() - 60), Max(0, color.Blue() - 60)));
			}
		}
	}

	// custom draw
	const Float start = item->GetStart();
	const Float duration = item->IsDuration() ? item->GetDuration() : 0.f;
	const Int32 startPos = GetOwner().CalculatePixelPos(start);
	const Int32 endPos = GetOwner().CalculatePixelPos(Min(start + duration, GetOwner().GetActiveRangeDuration()));
	wxRect rect(startPos, itemPosY, endPos - startPos, itemHeight);
	item->CustomDraw(&GetOwner(), rect);

	if(item->IsDuration())
	{
		// Draw text
		String text;
		if(item->GetTopText(text))
		{
			if(GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				wxPoint textPos(startPos + 3, trackLocalPos.y + 2);
				wxPoint textSize = GetDrawBuffer()->TextExtents(GetOwner().m_wxItemFont, text);
				GetDrawBuffer()->FillRect(textPos.x, textPos.y+1, textSize.x, textSize.y-2, wxColour(180, 180, 180));

				GetDrawBuffer()->DrawText(textPos, endPos - startPos - 3, 15, GetOwner().m_wxItemFont, text, wxColour(255, 255, 255));
			}
			else
			{
				wxPoint textPos(startPos + 3, trackLocalPos.y + 2);
				wxPoint textSize = GetDrawBuffer()->TextExtents(*GetOwner().m_itemFont, text);
				GetDrawBuffer()->FillRect(textPos.x, textPos.y+1, textSize.x, textSize.y-2, wxColour(180, 180, 180));

				GetDrawBuffer()->DrawText(textPos, endPos - startPos - 3, 15, *GetOwner().m_itemFont, text, wxColour(255, 255, 255));
			}
		}
		if(item->GetMiddleText(text))
		{
			if(GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				GetDrawBuffer()->DrawText(wxPoint(startPos + 5, itemPosY), endPos - startPos - 5, 15, GetOwner().m_wxItemFont, text, wxColour(0, 0, 0));
			}
			else
			{
				GetDrawBuffer()->DrawText(wxPoint(startPos + 5, itemPosY), endPos - startPos - 5, 15, *GetOwner().m_itemFont, text, wxColour(0, 0, 0));
			}
		}
	}

	if(!item->IsDuration())
	{
		// point-events (without duration)

		Int32 eventPos = GetOwner().CalculatePixelPos(item->GetStart());

		wxPoint pt0(eventPos, itemPosY - 11);
		wxPoint pt1(eventPos - 6, itemPosY - 2);
		wxPoint pt2(eventPos + 6, itemPosY - 2);

		GetDrawBuffer()->FillTriangle(pt0.x, pt0.y, pt1.x, pt1.y, pt2.x, pt2.y, color);
		GetDrawBuffer()->DrawTriangle(pt0.x, pt0.y, pt1.x, pt1.y, pt2.x, pt2.y, borderColor);

		if(GetDrawBuffer()->GetType() == CanvasType::gdi)
		{
			if(item->GetWxIcon() != NULL)
			{
				GetDrawBuffer()->DrawImage(item->GetWxIcon(), eventPos - imageSize/2, imgPosY, imageSize, imageSize);
			}
		}
		else
		{
			if(item->GetIcon() != NULL)
			{
				GetDrawBuffer()->DrawImage(item->GetIcon(), eventPos - imageSize/2, imgPosY, imageSize, imageSize);
			}
		}
	}

	if( !item->IsValid() )
	{
		String text( TXT("(invalid)" ) );

		if(GetDrawBuffer()->GetType() == CanvasType::gdi)
		{
			GetDrawBuffer()->DrawText(wxPoint(startPos + 5, itemPosY), GetOwner().m_wxItemFont, text, wxColour(255, 0, 0));
		}
		else
		{
			GetDrawBuffer()->DrawText(wxPoint(startPos + 5, itemPosY), *GetOwner().m_itemFont, text, wxColour(255, 0, 0));
		}
	}

	if(GetOwner().IsOnlySelected(item))
	{
		ITimelineItem* selectedItem = GetOwner().m_selectedItems[0];

		// Draw resizing tooltips
		{
			if(GetOwner().m_state == CEdTimeline::STATE_RESIZING && item->IsDuration() && ! GetOwner().m_startEdgeGrabbed)
			{
				Float endPos = selectedItem->GetStart() + selectedItem->GetDuration();
				if(endPos > GetOwner().GetActiveRangeDuration())
				{
					endPos -= GetOwner().GetActiveRangeDuration();
				}

				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(endPos), itemPosY), String::Printf(TXT("Duration: %.2f s"),
						selectedItem->GetDuration()), wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
				}
				else
				{
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(endPos), itemPosY), String::Printf(TXT("Duration: %.2f s"),
						selectedItem->GetDuration()), wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
				}
			}
		}

		// Draw moving tooltips
		{
			if(GetOwner().m_state == CEdTimeline::STATE_DRAGGING && item->IsDuration())
			{
				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(selectedItem->GetStart()),
						itemPosY), String::Printf(TXT("Start: %.2f s"), selectedItem->GetStart()),
						wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
				}
				else
				{
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(selectedItem->GetStart()),
						itemPosY), String::Printf(TXT("Start: %.2f s"), selectedItem->GetStart()),
						wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
				}

				Float endPos = selectedItem->GetStart() + selectedItem->GetDuration();
				if(endPos > GetOwner().GetActiveRangeDuration())
				{
					endPos -= GetOwner().GetActiveRangeDuration();
				}

				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(endPos), itemPosY), String::Printf(TXT("End: %.2f s"), endPos),
					wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
				}
				else
				{
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(endPos), itemPosY), String::Printf(TXT("End: %.2f s"), endPos),
						wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
				}
			}
		}
	}

	GetOwner().OnDrawItem(item, rect);
}


void CDrawGroupTracks::SortTracks()
{
	Sort(m_dispTracks.Begin(), m_dispTracks.End(), GreaterTrack());
	Sort(m_tracks.Begin(), m_tracks.End(), GreaterTrack());
}

/*
Returns whether track is to be displayed.

Track is to be displayed unless it is hidden or is in a track group that is collapsed or hidden.
*/
Bool CDrawGroupTracks::IsToBeDisplayed(Track* track) const
{
	if (! track)
	{
		return false;
	}

	Bool isToBeDisplayed = !track->m_isHidden;

	while((track = track->m_parent) && isToBeDisplayed)
	{
		if(!track->m_isExpanded || track->m_isHidden)
		{
			isToBeDisplayed = false;
		}
	}

	return isToBeDisplayed;
}

/*
Returns height that track would have if drawn by this draw group.

Track passed as arg doesn't have to be disp track and it doesn't even have to be in this draw group.
*/
Int32 CDrawGroupTracks::GetTrackHeight(Track* track) const
{
	return BasicSizes::trackHeight * GetOwner().GetVertScale();
}

/*
Returns height that items drawn on specified track would have if drawn by this draw group.

Track passed as arg doesn't have to be disp track and it doesn't even have to be in this draw group.
*/
Int32 CDrawGroupTracks::GetItemHeight(Track* track) const
{
	return BasicSizes::itemHeight * GetOwner().GetVertScale();
}

/*
Returns local position of disp track.

Track passed as arg has to be disp track in this draw group.

Global position - position in CEdTimeline space.
Local position - position in space of CDrawBuffer used by this CDrawGroupTracks.
*/
wxPoint CDrawGroupTracks::GetDispTrackLocalPos(Track* track) const
{
	// assert that track is in this draw group and that it is a disp track
	ASSERT(GetDispTrackIndex(track) >= 0);

	wxPoint pos(0, 0);

	for(Uint32 i = 0; GetDispTrack(i) != track; ++i)
	{
		pos.y += GetTrackHeight(GetDispTrack(i));
	}

	pos.y -= GetDrawBuffer()->GetVerticalOffset();

	return pos;
}

Bool CDrawGroupTracks::IsTrackGroupExpanded(Track* trackGroup) const
{
	ASSERT(trackGroup);
	return trackGroup->m_isExpanded;
}

/*

\param trackGroup Track group. Must be disp track.
*/
void CDrawGroupTracks::ExpandTrackGroup(Track* trackGroup)
{
	ASSERT(trackGroup);
	ASSERT(m_dispTracks.Exist(trackGroup));

	if(!IsTrackGroupExpanded(trackGroup))
	{
		// note we need to set it before IsToBeDisplayed() is used
		trackGroup->m_isExpanded = true;

		Uint32 sourceIndex = m_tracks.GetIndex(trackGroup) + 1;
		Uint32 insertionIndex = m_dispTracks.GetIndex(trackGroup) + 1;
		Uint32 numTracks = m_tracks.Size();
		while(sourceIndex < numTracks && IsRelationship(trackGroup, m_tracks[sourceIndex]))
		{
			if(IsToBeDisplayed(m_tracks[sourceIndex]))
			{
				m_dispTracks.Insert(insertionIndex, m_tracks[sourceIndex]);
				++insertionIndex;
			}
			++sourceIndex;
		}
	}
}

/*

\param trackGroup Track group. Must be disp track.
*/
void CDrawGroupTracks::CollapseTrackGroup(Track* trackGroup)
{
	ASSERT(trackGroup);
	ASSERT(m_dispTracks.Exist(trackGroup));

	if(IsTrackGroupExpanded(trackGroup))
	{
		trackGroup->m_isExpanded = false;

		Uint32 indx = m_dispTracks.GetIndex(trackGroup) + 1;
		while(indx < GetNumDispTracks() && IsRelationship(trackGroup, m_dispTracks[indx]))
		{
			m_dispTracks.RemoveAt(indx);
		}
	}
}

/*
Computes position at which track should be inserted into m_dispTracks.

\param track Track for which to find place in m_dispTracks. Must exist in m_tracks. Must not exist in m_dispTracks.
\return Position at which track should be inserted into m_dispTracks.

This functions makes it easier to keep m_dispTracks order compatible with m_tracks order.

This function assumes that m_tracks and m_dispTracks order is compatible.
*/
Uint32 CDrawGroupTracks::ComputeDispInsertPosition(Track* track) const
{
	ASSERT(m_tracks.Exist(track));
	ASSERT(!m_dispTracks.Exist(track));

	Uint32 numTracks = m_tracks.Size();
	Uint32 numDispTracks = m_dispTracks.Size();

	Uint32 position = m_tracks.GetIndex(track) + 1;
	Uint32 dispPosition = numDispTracks; // assume we should insert at the end

	while(position < numTracks && dispPosition == numDispTracks)
	{
		Track* trackToFind = m_tracks[position];
		for(Uint32 i = 0; i < numDispTracks; ++i)
		{
			if(m_dispTracks[i] == trackToFind)
			{
				dispPosition = i;
				break;
			}
		}

		++position;
	}

	return dispPosition;
}

/*
Returns whether track is hidden.
*/
Bool CDrawGroupTracks::IsTrackHidden(Track* track) const
{
	return track->m_isHidden;
}

void CDrawGroupTracks::ShowTrack(Track* track)
{
	ASSERT(track);
	ASSERT(m_tracks.Exist(track));

	if(IsTrackHidden(track))
	{
		// note we need to set it before IsToBeDisplayed() is used
		track->m_isHidden = false;

		if(IsToBeDisplayed(track))
		{
			// show track
			Uint32 insertionIndex = ComputeDispInsertPosition(track);
			m_dispTracks.Insert(insertionIndex, track);
			++insertionIndex;

			// show track descendants
			Uint32 sourceIndex = m_tracks.GetIndex(track) + 1;
			Uint32 numTracks = m_tracks.Size();
			while(sourceIndex < numTracks && IsRelationship(track, m_tracks[sourceIndex]))
			{
				if(IsToBeDisplayed(m_tracks[sourceIndex]))
				{
					m_dispTracks.Insert(insertionIndex, m_tracks[sourceIndex]);
					++insertionIndex;
				}
				++sourceIndex;
			}
		}
	}
}

/*

*/
void CDrawGroupTracks::HideTrack(Track* track)
{
	ASSERT(track);
	ASSERT(m_tracks.Exist(track));

	if(!IsTrackHidden(track))
	{
		track->m_isHidden = true;

		Uint32 indx = m_dispTracks.GetIndex(track);

		// track is not hidden but it may be in a collapsed group so it may not be present in m_dispTracks
		if(indx != -1)
		{
			m_dispTracks.RemoveAt(indx);

			// hide track descendants
			while(indx < GetNumDispTracks() && IsRelationship(track, m_dispTracks[indx]))
			{
				m_dispTracks.RemoveAt(indx);
			}
		}
	}
}

Bool CDrawGroupTracks::HasPinBtn(Track* track) const
{
	// only top level tracks have pin/unpin button
	return !track->m_parent;
}

Bool CDrawGroupTracks::HasExpandBtn(Track* track) const
{
	// only track group tracks have expand/collapse button
	return track->m_isGroup;
}

/*
Returns rectangle (in track space) that would be associated with pin/unpin button if specified track was drawn by this draw group.

\param track Track. It doesn't have to be display track and it doesn't event have to be in this draw group.
\return Rectangle (in track space) that would be associated with pin/unpin button if specified track was drawn by this draw group.
*/
wxRect CDrawGroupTracks::GetPinBtnRect(Track* track) const
{
	wxRect expandBtnRect(0, 0, 0, 0);
	if(HasExpandBtn(track))
	{
		expandBtnRect = GetExpandBtnRect(track);
	}

	return wxRect(expandBtnRect.x + expandBtnRect.width + 5, 3, 19, 19);
}

/*
Returns rectangle (in track space) that would be associated with expand/collapse button if specified track was drawn by this draw group.

\param track Track. It doesn't have to be display track and it doesn't event have to be in this draw group.
\return Rectangle (in track space) that would be associated with expand/collapse button if specified track was drawn by this draw group.
*/
wxRect CDrawGroupTracks::GetExpandBtnRect(Track* track) const
{
	Int32 btnSize = GetOwner().TIMELINE_EXPAND_BTN_SIZE;
	Int32 indent = track->m_depth * GetOwner().TIMELINE_DEPTH_SPACING;

	return wxRect(5 + indent, 7, btnSize, btnSize);
}

void CDrawGroupTracks::OnLeftMouseClick(wxPoint localPos)
{
	Track* t = GetTrackAt(localPos);

	if(t)
	{
		wxPoint trackLocalPos = GetDispTrackLocalPos(t);

		// check if expand/collapse btn was clicked
		if(HasExpandBtn(t) && OffsetRect(GetExpandBtnRect(t), 0, trackLocalPos.y).Contains(localPos))
		{
			if(IsTrackGroupExpanded(t))
			{
				CollapseTrackGroup(t);
			}
			else
			{
				ExpandTrackGroup(t);
			}
			GetOwner().m_selectedItems.Clear();
			GetOwner().UpdateLayout();
		}
		// check if pin/unpin btn rectangle was clicked
		else if(HasPinBtn(t) && OffsetRect(GetPinBtnRect(t), 0, trackLocalPos.y).Contains(localPos))
		{
			// pin/unpin track
			if(GetOwner().IsTrackPinned(t))
			{
				GetOwner().UnpinTrack(t);
			}
			else
			{
				GetOwner().PinTrack(t);
			}
			GetOwner().UpdateLayout();
		}
	}
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
