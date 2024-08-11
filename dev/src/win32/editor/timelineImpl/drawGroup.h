// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

class CEdTimeline;
struct Track;
class ITimelineItem;

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

class CDrawBuffer;

/*
Group of elements to be drawn.

Group knows what to draw but needs CDrawBuffer to actually draw anything.
*/
class CDrawGroup
{
public:
	CDrawGroup(CEdTimeline& owner, CDrawBuffer* drawBuffer);
	virtual ~CDrawGroup();

	CEdTimeline& GetOwner() const;
	CDrawBuffer* GetDrawBuffer() const;
	void SetDrawBuffer(CDrawBuffer* drawBuffer);

	wxPoint GetLocalPos(wxPoint globalPos) const;
	wxPoint GetGlobalPos(wxPoint globalPos) const;

	wxRect GetLocalRect(const wxRect& globalRect) const;
	wxRect GetGlobalRect(const wxRect& globalRect) const;

	virtual void Draw();
	virtual Track* GetTrackAt(wxPoint localPos) const;
	virtual ITimelineItem* GetItemAt(wxPoint localPos) const;
	virtual size_t GetItemsInRect(const wxRect& localRect, TDynArray<ITimelineItem*>& itemsInRect) const;
	virtual Int32 GetPreferredHeight() const;

	virtual void OnLeftMouseClick(wxPoint localPos);

private:
	CDrawGroup(const CDrawGroup&);				// cctor - not defined
	CDrawGroup& operator=(const CDrawGroup&);	// op= - not defined

	CEdTimeline& m_owner;
	CDrawBuffer* m_drawBuffer;					// Draw buffer used by this draw group. Draw group doesn't own it.
};

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE CDrawGroup::CDrawGroup(CEdTimeline& owner, CDrawBuffer* drawBuffer)
: m_owner(owner)
, m_drawBuffer(drawBuffer)
{}

RED_INLINE CDrawGroup::~CDrawGroup()
{}

RED_INLINE CEdTimeline& CDrawGroup::GetOwner() const
{
	return m_owner;
}

RED_INLINE CDrawBuffer* CDrawGroup::GetDrawBuffer() const
{
	return m_drawBuffer;
}

RED_INLINE void CDrawGroup::SetDrawBuffer(CDrawBuffer* drawBuffer)
{
	m_drawBuffer = drawBuffer;
}

RED_INLINE void CDrawGroup::Draw()
{}

RED_INLINE Track* CDrawGroup::GetTrackAt(wxPoint /* localPos */) const
{
	return 0;
}

RED_INLINE ITimelineItem* CDrawGroup::GetItemAt(wxPoint /* localPos */) const
{
	return 0;
}

RED_INLINE Int32 CDrawGroup::GetPreferredHeight() const
{
	return 0;
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
