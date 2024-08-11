// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "dialogTimeline_items.h" // TODO: this is only because CTimelineItemEvent is defined there
                                  // CTimelineItemEvent (and some others) should be defined in their own files.

class CStorySceneEventInterpolation;

// =================================================================================================
namespace DialogTimelineItems {
// =================================================================================================

/*
This class handles all types of interpolation events.
*/
class CTimelineItemInterpolationEvent : public CTimelineItemEvent
{
public:
	CTimelineItemInterpolationEvent( CEdDialogTimeline* timeline, CStorySceneEventInterpolation* scInterpolationEvent, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements );

	void Reinitialize();

	Uint32 GetNumKeys() const;
	ITimelineItem* GetKey( Uint32 index ) const;

	CStorySceneEventInterpolation* GetSceneInterpolationEvent();
	const CStorySceneEventInterpolation* GetSceneInterpolationEvent() const;

	virtual String GetTypeName() const override;
	virtual Float SetStart( Float start, Bool deepUpdate ) override;
	virtual Bool IsRightResizable() const override;
	virtual Bool IsLeftResizable() const override;
	virtual Bool IsMovable() const override;
	virtual Bool IsCopyable() const override;
	virtual wxColor GetColor() const override;
	virtual Gdiplus::Bitmap* GetIcon() const override;
	virtual const wxBitmap* GetWxIcon() const override;
	virtual Float GetHeightScale() const override;
	virtual void OnDeleted() override;

	virtual void GetMoveSet( TDynArray< ITimelineItem* >& outMoveSet ) override;

private:
	TDynArray< ITimelineItem* > m_keys;						// TODO: Make it hold CTimelineItemEvent*?
															// TODO: We can get rid of m_keys altogether..
};

Bool IsInterpolationEvent( const ITimelineItem* item );
Bool CanCreateInterpolationEvent( const TDynArray< ITimelineItem* >& items );
Bool GetInterpolateEventAndKeys( const TDynArray< ITimelineItem* >& itemList, ITimelineItem*& outInterpolateItem, TDynArray< ITimelineItem* >& outKeys );

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE Uint32 CTimelineItemInterpolationEvent::GetNumKeys() const
{
	return m_keys.Size();
}

RED_INLINE ITimelineItem* CTimelineItemInterpolationEvent::GetKey( Uint32 index ) const
{
	return m_keys[ index ];
}

// =================================================================================================
} // namespace DialogTimelineItems
// =================================================================================================
