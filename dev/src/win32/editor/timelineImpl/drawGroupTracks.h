// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

#include "drawGroup.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

/*
Group for drawing tracks and their items.
*/
class CDrawGroupTracks : public CDrawGroup
{
public:
	CDrawGroupTracks(CEdTimeline& owner, CDrawBuffer* drawBuffer);
	virtual ~CDrawGroupTracks();

	virtual void Draw();
	virtual Track* GetTrackAt(wxPoint localPos) const;
	virtual Uint32 GetItemsAt(wxPoint localPos, TDynArray<ITimelineItem*>& items) const;
	virtual ITimelineItem* GetItemAt(wxPoint localPos) const;
	virtual size_t GetItemsInRect(const wxRect& localRect, TDynArray<ITimelineItem*>& itemsInRect) const;
	virtual Int32 GetPreferredHeight() const;
	virtual Int32 GetTrackHeight(Track* track) const;
	virtual Int32 GetItemHeight(Track* track) const;

	void AddTrack(Track* track);
	void RemoveTrack(Track* track);
	void RemoveAllTracks();

	Uint32 GetNumTracks() const;
	Track* GetTrack(Uint32 position);
	Int32 GetTrackIndex(Track* track) const;
	Bool TrackExists(Track* track) const;

	Uint32 GetNumDispTracks() const;
	Track* GetDispTrack(Uint32 dispIndex) const;
	Int32 GetDispTrackIndex(Track* track) const;
	virtual wxPoint GetDispTrackLocalPos(Track* track) const;

	void ExchangePositions(Track* tA, Track* tB);
	void SortTracks();

	Bool IsTrackGroupExpanded(Track* trackGroup) const;
	void ExpandTrackGroup(Track* trackGroup);
	void CollapseTrackGroup(Track* trackGroup);

	Bool IsTrackHidden(Track* track) const;
	void ShowTrack(Track* track);
	void HideTrack(Track* track);

	virtual void OnLeftMouseClick(wxPoint localPos);

protected:
	virtual void DrawBackground();
	virtual void DrawGrid();
	virtual void DrawTracks();
	virtual void DrawItems();
	virtual void DrawItem(const ITimelineItem* item);

	virtual Bool HasPinBtn(Track* track) const;
	virtual Bool HasExpandBtn(Track* track) const;

	virtual wxRect GetPinBtnRect(Track* track) const;
	virtual wxRect GetExpandBtnRect(Track* track) const;

private:
	CDrawGroupTracks(const CDrawGroupTracks&);				// cctor - not defined
	CDrawGroupTracks& operator=(const CDrawGroupTracks&);	// op= - not defined

	Bool IsToBeDisplayed(Track* track) const;
	Uint32 CDrawGroupTracks::ComputeDispInsertPosition(Track* track) const;
	
	TDynArray<Track*> m_tracks;								// Tracks in this draw group. Draw group
															// doesn't own them.

	TDynArray<Track*> m_dispTracks;							// Subset of m_tracks containing only those
															// tracks that are to be displayed, i.e. it
															// doesn't contain hidden tracks and tracks
															// in collapsed groups. Order of tracks is
															// compatible with m_tracks order.
};

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE CDrawGroupTracks::CDrawGroupTracks(CEdTimeline& owner, CDrawBuffer* drawBuffer)
: CDrawGroup(owner, drawBuffer)
{}

RED_INLINE CDrawGroupTracks::~CDrawGroupTracks()
{}

RED_INLINE void CDrawGroupTracks::AddTrack(Track* track)
{
	if(IsToBeDisplayed(track))
	{
		m_dispTracks.PushBack(track);
	}

	m_tracks.PushBack(track);
}

RED_INLINE void CDrawGroupTracks::RemoveTrack(Track* track)
{
	m_dispTracks.Remove(track);
	m_tracks.Remove(track);
}

RED_INLINE void CDrawGroupTracks::RemoveAllTracks()
{
	m_dispTracks.ClearFast();
	m_tracks.ClearFast();
}

RED_INLINE Uint32 CDrawGroupTracks::GetNumTracks() const
{
	return m_tracks.Size();
}

RED_INLINE Track* CDrawGroupTracks::GetTrack(Uint32 position)
{
	ASSERT(position < m_tracks.Size());
	return m_tracks[position];
}

RED_INLINE Uint32 CDrawGroupTracks::GetNumDispTracks() const
{
	return m_dispTracks.Size();
}

RED_INLINE Track* CDrawGroupTracks::GetDispTrack(Uint32 dispIndex) const
{
	ASSERT(dispIndex < m_dispTracks.Size());
	return m_dispTracks[dispIndex];
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
