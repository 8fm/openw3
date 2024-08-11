// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "drawGroupDialogTracks.h"
#include "drawBuffer.h"
#include "drawing.h"
#include "../timeline.h"
#include "../dialogTimeline_items.h"

using namespace TimelineImpl;

// =================================================================================================
namespace DialogTimelineImpl {
// =================================================================================================
namespace {
// =================================================================================================
namespace BasicSizes {
// =================================================================================================

// note those are not meant to be used directly
const Int32 trackHeight = 50; // please use CDrawGroupDialogTracks::GetTrackHeight() instead
const Int32 itemHeight = 24;  // please use CDrawGroupDialogTracks::GetItemHeight() instead

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

wxColour colorDrawGroupBg(150, 150, 150);
wxColour colorDrawGroupBorder(0, 0, 0);

wxColour colorAnimRangeBg(180, 180, 180);
wxColour colorAnimRangeBorder(0, 0, 0);

wxColour colorGridLine(0, 0, 0);
wxColour colorGridHelpLine(150, 150, 150);

wxColour colorTrackSeparator(0, 0, 0);

wxColour colorTrackBg(184, 65, 71);
wxColour colorSubTrackBg(139, 126, 127);
wxColour colorSelectedTrackBg(192, 30, 35);

wxColour colorTrackGroupBg(172, 74, 74);
wxColour colorSelectedTrackGroupBg(161, 12, 21);
wxColour colorTrackGroupTree(64, 64, 64);

wxColour colorExpandBtnBg(128, 128, 128);
wxColour colorExpandBtnFg(0, 0, 0);

wxColour colorTrackName(255, 255, 255);

// =================================================================================================
} // unnamed namespace
// =================================================================================================

CDrawGroupDialogTracks::CDrawGroupDialogTracks(CEdTimeline& owner, TimelineImpl::CDrawBuffer* drawBuffer)
: CDrawGroupTracks(owner, drawBuffer)
, m_iconTrackLocked(0)
, m_iconTrackUnlocked(0)
{
	m_wxIconTrackLocked = SEdResources::GetInstance().LoadBitmap(TXT( "IMG_LOCKED"));
	m_wxIconTrackUnlocked = SEdResources::GetInstance().LoadBitmap(TXT("IMG_UNLOCKED"));

	m_iconTrackLocked = GetOwner().ConvertToGDI( m_wxIconTrackLocked );
	m_iconTrackUnlocked = GetOwner().ConvertToGDI( m_wxIconTrackUnlocked );

	m_wxIconsSubtrackShown[0] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_ADDITIVES_CLR"));
	m_wxIconsSubtrackShown[1] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_ANIMATIONS_CLR"));
	m_wxIconsSubtrackShown[2] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_LOOKATS_CLR"));
	m_wxIconsSubtrackShown[3] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_MIMICS_CLR"));
	m_wxIconsSubtrackShown[4] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_PLACEMENT_CLR"));
	m_wxIconsSubtrackShown[5] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_PLACEMENT_CLR"));
	m_wxIconsSubtrackShown[6] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_EYE"));

	m_iconsSubtrackShown[0] = GetOwner().ConvertToGDI(m_wxIconsSubtrackShown[0]);
	m_iconsSubtrackShown[1] = GetOwner().ConvertToGDI(m_wxIconsSubtrackShown[1]);
	m_iconsSubtrackShown[2] = GetOwner().ConvertToGDI(m_wxIconsSubtrackShown[2]);
	m_iconsSubtrackShown[3] = GetOwner().ConvertToGDI(m_wxIconsSubtrackShown[3]);
	m_iconsSubtrackShown[4] = GetOwner().ConvertToGDI(m_wxIconsSubtrackShown[4]);
	m_iconsSubtrackShown[5] = GetOwner().ConvertToGDI(m_wxIconsSubtrackShown[5]);
	m_iconsSubtrackShown[6] = GetOwner().ConvertToGDI(m_wxIconsSubtrackShown[6]);

	m_wxIconsSubtrackHidden[0] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_ADDITIVES_BW"));
	m_wxIconsSubtrackHidden[1] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_ANIMATIONS_BW"));
	m_wxIconsSubtrackHidden[2] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_LOOKATS_BW"));
	m_wxIconsSubtrackHidden[3] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_MIMICS_BW"));
	m_wxIconsSubtrackHidden[4] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_PLACEMENT_BW"));
	m_wxIconsSubtrackHidden[5] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_PLACEMENT_BW"));
	m_wxIconsSubtrackHidden[6] = SEdResources::GetInstance().LoadBitmap(TXT("IMG_EYE_CLOSED"));

	m_iconsSubtrackHidden[0] = GetOwner().ConvertToGDI(m_wxIconsSubtrackHidden[0]);
	m_iconsSubtrackHidden[1] = GetOwner().ConvertToGDI(m_wxIconsSubtrackHidden[1]);
	m_iconsSubtrackHidden[2] = GetOwner().ConvertToGDI(m_wxIconsSubtrackHidden[2]);
	m_iconsSubtrackHidden[3] = GetOwner().ConvertToGDI(m_wxIconsSubtrackHidden[3]);
	m_iconsSubtrackHidden[4] = GetOwner().ConvertToGDI(m_wxIconsSubtrackHidden[4]);
	m_iconsSubtrackHidden[5] = GetOwner().ConvertToGDI(m_wxIconsSubtrackHidden[5]);
	m_iconsSubtrackHidden[6] = GetOwner().ConvertToGDI(m_wxIconsSubtrackHidden[6]);
}

CDrawGroupDialogTracks::~CDrawGroupDialogTracks()
{
	for(Uint32 i = 0; i < m_numSubtrackIcons; ++i)
	{
		delete m_iconsSubtrackShown[i];
		delete m_iconsSubtrackHidden[i];
	}

	delete m_iconTrackUnlocked;
	delete m_iconTrackLocked;
}

void CDrawGroupDialogTracks::DrawBackground()
{
	ASSERT(GetDrawBuffer());

	Int32 width, height;
	GetDrawBuffer()->GetSize(width, height);

	GetDrawBuffer()->Clear(colorDrawGroupBg);
	GetDrawBuffer()->DrawRect(0, 0, width, height, colorDrawGroupBorder);

	// Draw animation range
	Int32 trackBtnWidth = GetOwner().TIMELINE_TRACK_BTN_WIDTH;
	Int32 vOff = GetDrawBuffer()->GetVerticalOffset();
	Int32 y0 = vOff < 0? -vOff : 0;
	Int32 y1 = -vOff + GetPreferredHeight() - 1;
	Int32 animStart = GetOwner().CalculatePixelPos(0.0f);
	Int32 animEnd = GetOwner().CalculatePixelPos(GetOwner().GetActiveRangeDuration());
	GetDrawBuffer()->FillRect(Max(animStart, trackBtnWidth), y0, animEnd - Max(animStart, trackBtnWidth), y1 - y0 + 1, colorAnimRangeBg);
	GetDrawBuffer()->DrawLine(Max(animStart, trackBtnWidth), y0, Max(animStart, trackBtnWidth), y1, colorAnimRangeBorder, 2.0f);
	GetDrawBuffer()->DrawLine(animEnd - 1, y0, animEnd - 1, y1, colorAnimRangeBorder, 2.0f);
}

void CDrawGroupDialogTracks::DrawTracks()
{
	Int32 timelineTrackBtnWidth = GetOwner().TIMELINE_TRACK_BTN_WIDTH;
	Int32 timelineDepthSpacing = GetOwner().TIMELINE_DEPTH_SPACING;
	Int32 timelineExpandBtnSize = GetOwner().TIMELINE_EXPAND_BTN_SIZE;
	Int32 timelineExpandBtnCrossThickness = GetOwner().TIMELINE_EXPAND_BTN_CROSS_THICKNESS;

	Int32 width, height;
	GetDrawBuffer()->GetSize(width, height);

	// for all tracks
	Int32 trackSlot = 0;
	Uint32 numDispTracks = GetNumDispTracks();
	for(Uint32 trackIter = 0; trackIter < numDispTracks; ++trackIter, ++trackSlot)
	{
		Track* track = GetDispTrack(trackIter);
		Int32 trackHeight = GetTrackHeight(track);
		Int32 trackPosY = GetDispTrackLocalPos(track).y;

		Int32 indent = track->m_depth * timelineDepthSpacing;
		Int32 tw = timelineTrackBtnWidth - indent; // TODO: obsolete

		// draw separator
		GetDrawBuffer()->DrawLine(indent, trackPosY, width, trackPosY, colorTrackSeparator, 1.0f);

		// Draw 'button'
		Bool isSelected = GetOwner().m_selectedTrack > -1 && track == GetOwner().m_tracks[GetOwner().m_selectedTrack];
		if(track->m_isGroup)
		{
			GetDrawBuffer()->FillRect(indent, trackPosY + 1, width - 1 - indent, trackHeight - 1, isSelected ? colorSelectedTrackGroupBg : colorTrackGroupBg);
		}
		else
		{
			wxColour c = track->m_parent? colorSubTrackBg : colorTrackBg;
			GetDrawBuffer()->FillRect(indent, trackPosY + 1, tw, trackHeight - 1, isSelected ? colorSelectedTrackBg : c);
		}

		if(track->m_depth > 0)
		{
			GetDrawBuffer()->DrawLine(indent, trackPosY , indent, trackPosY + trackHeight, colorTrackSeparator);
			Int32 n  = track->m_depth;

			Uint32 it2 = trackIter;
			++it2;
			GetDrawBuffer()->DrawLine((n - 1) * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY + (trackHeight >> 1), n * timelineDepthSpacing, trackPosY + (trackHeight >> 1), colorTrackGroupTree);
			if(it2 >= numDispTracks ||  GetDispTrack(it2)->m_depth < track->m_depth)
			{
				--n;
				GetDrawBuffer()->DrawLine(n * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY, n * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY + (trackHeight >> 1), colorTrackGroupTree);
			}
			for(Int32 i = 0; i < n; ++i)
			{
				GetDrawBuffer()->DrawLine(i * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY , i * timelineDepthSpacing + (timelineDepthSpacing >> 1), trackPosY + trackHeight, colorTrackGroupTree);
			}
		}

		// draw track name
		if(track->m_isGroup)
		{
			// name of actor track is displayed after show/hide buttons
			wxRect rect = GetShowBtnRect(track, track->m_children.Size());
			rect.x += 5;
			// btnRect is in track space so we have to offset it to get it in local space
			rect.Offset(0, trackPosY);

			if(GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				GetDrawBuffer()->DrawText(rect.GetTopLeft(), GetOwner().GetWxDrawFont(), track->m_name, colorTrackName);
			}
			else
			{
				GetDrawBuffer()->DrawText(rect.GetTopLeft(), GetOwner().GetGdiDrawFont(), track->m_name, colorTrackName);
			}
		}
		else
		{
			// name of non-actor track is displayed after lock/unlock button
			wxRect rect = GetLockBtnRect(track);
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
				GetDrawBuffer()->DrawText(rect.GetTopLeft(), GetOwner().GetWxDrawFont(), name, colorTrackName);
			}
			else
			{
				GetDrawBuffer()->DrawText(rect.GetTopLeft(), GetOwner().GetGdiDrawFont(), name, colorTrackName);
			}
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

			GetDrawBuffer()->FillRect(btnRect.x, btnRect.y, btnRect.width, btnRect.height, colorExpandBtnBg);
			GetDrawBuffer()->DrawRect(btnRect.x, btnRect.y, btnRect.width, btnRect.height, colorExpandBtnFg);
			GetDrawBuffer()->FillRect(
				btnRect.x,
				btnRect.y + ((btnRect.height - timelineExpandBtnCrossThickness) >> 1),
				btnRect.width, timelineExpandBtnCrossThickness, colorExpandBtnFg);
			if(!track->m_isExpanded)
			{
				GetDrawBuffer()->FillRect(
					btnRect.x + ((btnRect.width - timelineExpandBtnCrossThickness) >> 1),
					btnRect.y,
					timelineExpandBtnCrossThickness, btnRect.height, colorExpandBtnFg);
			}
		}

		// we know we're dealing with dialog track cause all tracks on dialog timeline are dialog tracks
		DialogTrack* dialogTrack = static_cast<DialogTrack*>(track);

		// draw lock/unlock button
		wxRect lockBtnRect = GetLockBtnRect(track);
		// btnRect is in track space so we have to offset it to get it in local space
		lockBtnRect.Offset(0, trackPosY);
		if(GetDrawBuffer()->GetType() == CanvasType::gdi)
		{
			const wxBitmap* iconLockedUnlocked = dialogTrack->m_isLocked? &m_wxIconTrackLocked : &m_wxIconTrackUnlocked;
			GetDrawBuffer()->DrawImage(iconLockedUnlocked, lockBtnRect.x, lockBtnRect.y, lockBtnRect.width, lockBtnRect.height);
		}
		else
		{
			Gdiplus::Bitmap* iconLockedUnlocked = dialogTrack->m_isLocked? m_iconTrackLocked : m_iconTrackUnlocked;
			GetDrawBuffer()->DrawImage(iconLockedUnlocked, lockBtnRect.x, lockBtnRect.y, lockBtnRect.width, lockBtnRect.height);
		}

		if(track->m_isGroup)
		{
			// draw show/hide buttons for subtracks
			Uint32 numChildren = track->m_children.Size();
			for(Uint32 i = 0; i < numChildren; ++i)
			{
				wxRect btnRect = GetShowBtnRect(track, i);
				// btnRect is in track space so we have to offset it to get it in local space
				btnRect.Offset(0, trackPosY);

				Uint32 iconIndex = GetSubtrackIconIndex(track->m_children[i]);

				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					const wxBitmap* icon = IsTrackHidden(track->m_children[i])? &m_wxIconsSubtrackHidden[iconIndex] : &m_wxIconsSubtrackShown[iconIndex];
					GetDrawBuffer()->DrawImage(icon, btnRect.x, btnRect.y, btnRect.width, btnRect.height);
				}
				else
				{
					Gdiplus::Bitmap* icon = IsTrackHidden(track->m_children[i])? m_iconsSubtrackHidden[iconIndex] : m_iconsSubtrackShown[iconIndex];
					GetDrawBuffer()->DrawImage(icon, btnRect.x, btnRect.y, btnRect.width, btnRect.height);
				}
				
			}
		}

		GetOwner().OnDrawTrack(track);
	}

	// draw top line
	GetDrawBuffer()->DrawLine(0, 0, width, 0, colorTrackSeparator, 1.0f);

	// bottom line
	if(numDispTracks > 0)
	{
		Track* lastTrack = GetDispTrack(numDispTracks - 1);
		Int32 y = GetDispTrackLocalPos(lastTrack).y + GetTrackHeight(lastTrack) - 1;
		GetDrawBuffer()->DrawLine(0, y, width, y, colorTrackSeparator, 1.0f);
	}
}

void CDrawGroupDialogTracks::DrawGrid()
{
	Int32 vOff = GetDrawBuffer()->GetVerticalOffset();
	Int32 y0 = vOff < 0? -vOff : 0;
	Int32 y1 = -vOff + GetPreferredHeight() - 1;

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
			GetDrawBuffer()->DrawLine(i, y0, i, y1, colorGridLine, 2.0f);
		}
		else if(num % 4 == 0)
		{
			// Draw half line
			GetDrawBuffer()->DrawLine(i, y0, i, y1, colorGridLine, 1.0f);
		}
		else
		{
			// Draw help line
			GetDrawBuffer()->DrawLine(i, y0, i, y1, colorGridHelpLine);
		}
	}
}

void CDrawGroupDialogTracks::OnLeftMouseClick(wxPoint localPos)
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
		else if (HasPinBtn(t) && OffsetRect(GetPinBtnRect(t), 0, trackLocalPos.y).Contains(localPos))
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
		// check if any show/hide btn was clicked
		else if(HasShowBtn(t) && localPos.x > GetShowBtnRect(t, 0).x) // no need to call OffsetRect() here as we check only x
		{
			Uint32 numChildren = t->m_children.Size();
			for(Uint32 i = 0; i < numChildren; ++i)
			{
				wxRect btnRect = GetShowBtnRect(t, i);
				if(OffsetRect(btnRect, 0, trackLocalPos.y).Contains(localPos))
				{
					Track* ch = t->m_children[i];
					if(IsTrackHidden(ch))
					{
						ShowTrack(ch);
					}
					else
					{
						HideTrack(ch);
					}
				}
			}
			GetOwner().UpdateLayout();
		}
		else if(HasLockBtn(t) && OffsetRect(GetLockBtnRect(t), 0, trackLocalPos.y).Contains(localPos))
		{
			LockUnlockTrack(t);
		}
	}
}

/*
Returns height that track would have if drawn by this draw group.

Track passed as arg doesn't have to be disp track and it doesn't even have to be in this draw group.
*/
Int32 CDrawGroupDialogTracks::GetTrackHeight(Track* track) const
{
	Int32 h = BasicSizes::trackHeight;
	return track->m_isGroup? h / 2 : h * GetOwner().GetVertScale(); // note group tracks are not scaled
}

/*
Returns height that items drawn on specified track would have if drawn by this draw group.

Track passed as arg doesn't have to be disp track and it doesn't even have to be in this draw group.
*/
Int32 CDrawGroupDialogTracks::GetItemHeight(Track* track) const
{
	Int32 h = BasicSizes::itemHeight;
	return track->m_isGroup? h / 2 : h * GetOwner().GetVertScale(); // note items on group tracks are not scaled
}

Bool CDrawGroupDialogTracks::HasLockBtn(Track* track) const
{
	// all tracks have lock button
	return true;
}

Bool CDrawGroupDialogTracks::HasShowBtn(Track* track) const
{
	return track->m_children.Size() > 0;
}

wxRect CDrawGroupDialogTracks::GetLockBtnRect(Track* track) const
{
	wxRect rect;

	if(track->m_isGroup)
	{
		// actor track - show lock/unlock after expand/collapse btn
		rect = GetPinBtnRect(track);
		rect.x += rect.width + 5;
	}
	else
	{
		if(track->m_parent)
		{
			// subtrack - show lock/unlock after some indent
			Int32 indent = track->m_depth * GetOwner().TIMELINE_DEPTH_SPACING;
			rect.x = indent + 5;
		}
		else
		{
			// standalone track - show lock/unlock after pin/unpin btn
			rect = GetPinBtnRect(track);
			rect.x += rect.width + 5;
		}
	}

	rect.y = 3;
	rect.width = 19;
	rect.height = 19;

	return rect;
}

/*
Returns rectangle (in track space) that would be associated with show/hide button if specified track was drawn by this draw group.

\param track Track. It doesn't have to be display track and it doesn't event have to be in this draw group.
\param subtrackIndex Index of subtrack whose show/hide button rectangle is to be returned.
\return Rectangle (in track space) that would be associated with show/hide button if specified track was drawn by this draw group.

This function doesn't check whether specified track should have show/hide buttons displayed and it doesn't check whether
subtrackIndex is valid.
*/
wxRect CDrawGroupDialogTracks::GetShowBtnRect(Track* track, Uint32 subtrackIndex) const
{
	// show/hide buttons are next to lock/unlock button so we start from here
	wxRect rect = GetLockBtnRect(track);

	rect.height = 19;
	rect.width = 19;
	
	rect.y = 3;
	rect.x += rect.width + 15 + subtrackIndex * (rect.width + 2);

	return rect;
}

void CDrawGroupDialogTracks::LockUnlockTrack(Track* track)
{
	ASSERT(track);
	ASSERT(GetTrackIndex(track) >= 0); // assert track exists

	// downcasting to DialogTrack* is safe because CDrawGroupDialogTracks deals only with DialogTracks
	DialogTrack* dialogTrack = static_cast<DialogTrack*>(track);

	Bool lockState = !dialogTrack->m_isLocked;
	dialogTrack->m_isLocked = lockState;

	//
	// Unlock parent track if we modify lock state of any of its children.
	//
	if(dialogTrack->m_parent)
	{
		DialogTrack* parentTrack = static_cast<DialogTrack*>(dialogTrack->m_parent);
		if(parentTrack)
		{
			parentTrack->m_isLocked = false;
		}
	}

	if(track->m_isGroup)
	{
		TDynArray<Track*>::iterator endIt = track->m_children.End();
		for(TDynArray< Track* >::iterator it = track->m_children.Begin(); it != endIt; ++it)
		{
			DialogTrack* child = static_cast<DialogTrack*>(*it);
			if(child)
			{
				child->m_isLocked = lockState;
			}
		}
	}
}

/*
Returns index of icon associated with specified track.

Returned index is to be used with CDrawGroupDialogTracks::m_iconsSubtrackShown
and CDrawGroupDialogTracks::m_iconsSubtrackHidden arrays.
*/
Uint32 CDrawGroupDialogTracks::GetSubtrackIconIndex(Track* track) const
{
	if(track->m_name.ContainsSubstring(TXT("additives"), true))
	{
		return 0;
	}
	else if(track->m_name.ContainsSubstring(TXT("animations"), true))
	{
		return 1;
	}
	else if(track->m_name.ContainsSubstring(TXT("lookats"), true))
	{
		return 2;
	}
	else if(track->m_name.ContainsSubstring(TXT("mimics"), true)) // this also handles "mimics2" track
	{
		return 3;
	}
	else if(track->m_name.ContainsSubstring(TXT("placement"), true))
	{
		return 4;
	}
	else if(track->m_name.ContainsSubstring(TXT("ik"), true))
	{
		return 5;
	}
	else
	{
		return 6;
	}
}

void CDrawGroupDialogTracks::DrawItem( const ITimelineItem* item )
{
	TimelineImpl::CDrawGroupTracks::DrawItem( item );
	String trackName = item->GetTrackName().Empty() ? GetOwner().TIMELINE_DEFAULT_TRACK_NAME : item->GetTrackName();
	Track* track = GetOwner().m_tracks[GetOwner().GetTrackIndex(trackName)];
	Int32 trackHeight  = GetTrackHeight(track);
	Int32 itemHeight   = GetItemHeight(track);
	Int32 imageSize    = GetItemHeight(track);
	wxPoint trackLocalPos = GetDispTrackLocalPos(track);
	Int32 itemPosY = trackLocalPos.y + trackHeight - itemHeight - 2;
	const Float start = item->GetStart();
	const Float duration = item->IsDuration() ? item->GetDuration() : 0.f;
	const Int32 startPos = GetOwner().CalculatePixelPos(start);
	const Int32 endPos = GetOwner().CalculatePixelPos(Min(start + duration, GetOwner().GetActiveRangeDuration()));
	wxRect rect(startPos, itemPosY, endPos - startPos, itemHeight);	
	const Uint32 state = item->GetState();

	Int32 blendInEndPos = rect.GetX() + rect.GetWidth();

	Float x1 = rect.GetLeft();
	Float y1 = rect.GetTop();
	Float x2 = rect.GetRight();
	Float y2 = rect.GetBottom();
	if( !item->IsDuration() )
	{
		x1 -= 10.f;
		x2 += 10.f;
	}
	if( state == DialogTimelineItems::EVENTSTATE_MARKED )
	{
		GetDrawBuffer()->DrawLine(x1, y1, x2, y2, wxColour( 237, 28, 36 ), 4.f );
		GetDrawBuffer()->DrawLine(x1, y2, x2, y1, wxColour( 237, 28, 36 ), 4.f );
	}
	else if( state == DialogTimelineItems::EVENTSTATE_LOCKED )
	{
		GetDrawBuffer()->DrawLine(x1, y2, x2, y2, wxColour( 34, 180, 76 ), 4.f );
		GetDrawBuffer()->DrawLine(x2, y2, x2, y1, wxColour( 34, 180, 76 ), 4.f );
		GetDrawBuffer()->DrawLine(x1, y1, x1, y2, wxColour( 34, 180, 76 ), 4.f );
		GetDrawBuffer()->DrawLine(x2, y1, x1, y1, wxColour( 34, 180, 76 ), 4.f );
	}
}

// =================================================================================================
} // namespace DialogTimelineImpl
// =================================================================================================
