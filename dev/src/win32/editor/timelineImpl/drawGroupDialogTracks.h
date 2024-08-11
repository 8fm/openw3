// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

#include "drawGroupTracks.h"

// =================================================================================================
namespace DialogTimelineImpl {
// =================================================================================================

/*
Group for drawing dialog tracks and their items.
*/
class CDrawGroupDialogTracks : public TimelineImpl::CDrawGroupTracks
{
public:
	CDrawGroupDialogTracks(CEdTimeline& owner, TimelineImpl::CDrawBuffer* drawBuffer);
	virtual ~CDrawGroupDialogTracks();

	virtual Int32 GetTrackHeight(Track* track) const;
	virtual Int32 GetItemHeight(Track* track) const;
	virtual void OnLeftMouseClick(wxPoint localPos);
	void LockUnlockTrack(Track* track);

private:
	CDrawGroupDialogTracks(const CDrawGroupDialogTracks&);				// cctor - not defined
	CDrawGroupDialogTracks& operator=(const CDrawGroupDialogTracks&);	// op= - not defined

	virtual void DrawGrid();
	virtual void DrawBackground();
	virtual void DrawTracks();
	virtual void DrawItem(const ITimelineItem* item);


	

	virtual Bool HasLockBtn(Track* track) const;
	virtual Bool HasShowBtn(Track* track) const;

	virtual wxRect GetLockBtnRect(Track* track) const;
	virtual wxRect GetShowBtnRect(Track* track, Uint32 subtrackIndex) const;

	Uint32 GetSubtrackIconIndex(Track* track) const;

	wxBitmap		 m_wxIconTrackLocked;
	wxBitmap		 m_wxIconTrackUnlocked;

	Gdiplus::Bitmap* m_iconTrackLocked;
	Gdiplus::Bitmap* m_iconTrackUnlocked;

	static const Uint32 m_numSubtrackIcons = 7;

	wxBitmap m_wxIconsSubtrackShown[m_numSubtrackIcons];
	wxBitmap m_wxIconsSubtrackHidden[m_numSubtrackIcons];

	Gdiplus::Bitmap* m_iconsSubtrackShown[m_numSubtrackIcons];
	Gdiplus::Bitmap* m_iconsSubtrackHidden[m_numSubtrackIcons];
};

// =================================================================================================
} // namespace DialogTimelineImpl
// =================================================================================================
