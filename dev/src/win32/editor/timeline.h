/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "track.h"
#include "dialogTrack.h"
#include "timelineLayout.h"
#include "timelineImpl/drawBuffer.h"
#include "timelineImpl/drawGroupTracks.h"		// TODO: no longer need to include this
#include "timelineImpl/drawGroupDialogTracks.h"	// TODO: we shouldn't include this here
#include "timelineImpl/drawGroupTimebar.h"
#include "timelineImpl/drawGroupVolatile.h"

// TODO: rename it and put into namespace
struct TimelineKeyModifiers
{
	Bool alt;
	Bool ctrl;
	Bool shift;
};

class CEdTimeline;

// Interface for timeline objects
class ITimelineItem
{
public:
	virtual ~ITimelineItem() {}

	virtual String				GetTypeName() const = 0;
	virtual void				UpdatePresentation() = 0;
	virtual Float				GetStart() const = 0;
	virtual Float				SetStart( Float start, Bool deepUpdate ) = 0;
	virtual Bool				IsDuration() const = 0;
	virtual Float				GetDuration() const = 0;
	virtual Float				SetDuration( Float duration ) = 0;
	virtual Bool				GetTopText( String& text ) const = 0;
	virtual Bool				GetMiddleText( String& text ) const = 0;
	virtual Bool				GetTooltip( String& text ) const = 0;
	virtual String				GetTrackName() const = 0;
	virtual void				SetTrackName( const String& trackName ) = 0;
	virtual Bool				IsRightResizable() const = 0;
	virtual Bool				IsLeftResizable() const = 0;
	virtual Bool				IsMovable() const = 0;
	virtual Bool				IsCopyable() const = 0;
	virtual wxColor				GetColor() const = 0;
	virtual wxColor				GetBorderColor() const { return wxColor( 0, 0, 0 ); }
	virtual Gdiplus::Bitmap*	GetIcon() const = 0;
	virtual const wxBitmap*		GetWxIcon() const = 0;
	virtual Bool				IsEditable() const = 0;
	virtual Bool				IsRemovable() const = 0;
	virtual void				SetProperty( IProperty* property, ITimelineItem* sourceItem ) = 0;
	virtual void				CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const = 0;
	virtual void				SetState( Int32 value ) = 0;
	virtual Int32				GetState() const = 0;
	virtual Float				GetHeightScale() const { return 1.0f; }
	virtual void				OnDeleted() {}
	virtual void				OnSelected() {}
	virtual void				GetChildItems( TDynArray< ITimelineItem* >& childItems ) {}
	virtual void				OnPasted( TDynArray< ITimelineItem* >& allPastedItems ) {}
	virtual void				GetMoveSet( TDynArray< ITimelineItem* >& outMoveSet ) { outMoveSet.PushBackUnique( this ); }
	virtual Bool				IsValid() const { return true; }
	
	// TODO: These functions are currently used only by dialog timeline events
	// but eventually, all items (for which it makes sense) should use it.
	virtual Float				SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) { return 0.0f; }
	virtual Float				SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) { return 0.0f; }
};

wxDECLARE_EVENT( usrEVT_REFRESH_PREVIEW, wxCommandEvent );
wxDECLARE_EVENT( usrEVT_TIMELINE_REQUEST_SET_TIME, wxCommandEvent );
wxDECLARE_EVENT( usrEVT_TIMELINE_RESIZED, wxCommandEvent );

class CEdPropertiesPage;

class CEdTimeline : public CEdCanvas
{
	wxDECLARE_EVENT_TABLE();

	friend class CUndoTimelineStep;
	friend class CEdTimelineUndoManager;
	friend class CUndoTimelineItemExistance;
	friend class CUndoTimelineItemLayout;
	friend class CUndoTimelineTrackExistance;
	friend class CUndoTimelineTrackRename;
	friend struct Track;
	friend struct DialogTrack;
	friend class TimelineImpl::CDrawBuffer;
	friend class TimelineImpl::CDrawGroupTracks;
	friend class DialogTimelineImpl::CDrawGroupDialogTracks;
	friend class TimelineImpl::CDrawGroupTimebar;
	friend class TimelineImpl::CDrawGroupVolatile;

protected:
	enum States
	{
		STATE_NORMAL,
		STATE_DRAGGING,
		STATE_SELECTED,
		STATE_MOVING,
		STATE_TIME_SETTING,
		STATE_SELECTED_HOVERING_EDGE,
		STATE_RESIZING,
		STATE_SELECTING,
		STATE_DRAG_TRACK_OR_SELECT_ALL_EVENTS_ON_TRACK,
		STATE_DRAG_TRACK
	};

public:
	enum EPasteItemsMode
	{
		PIM_Default,
		PIM_Special,
		PIM_Mouse,
		PIM_CustomTime,
		PIM_AllToOneCustomTime,
	};

private:
	enum TimeSetting
	{
		TIME_SETTING_CURRENT_TIME,
		TIME_SETTING_MIN_LIMIT,
		TIME_SETTING_MAX_LIMIT,
	};

protected:
	/*
	Used when moving items.
	*/
	class MovingContext
	{
	public:
		MovingContext();

		void Init( TDynArray< ITimelineItem* >& itemsToMove, ITimelineItem* leaderItem, Float cursorToLeaderInitialDist, const TDynArray< ITimelineItem* >& snapTargets );
		void Deinit();

		TDynArray< ITimelineItem* > m_moveSet;				// Move set, sorted by start time of items.

		ITimelineItem* m_leaderItem;						// Leader item - item grabbed by the user.
		Float m_cursorToLeaderInitialDist;					// Initial distance between mouse cursor and leader item.

		Float m_snapPixelDistance;							// Snap distance in pixels.
		TDynArray< ITimelineItem* > m_snapTargets;			// List of items to which leader item can be snapped.

		Bool m_allItemsMovable;								// True if all items from move set are movable, false - otherwise.
	};

	TDynArray< Track* >				m_tracks;
	Int32							m_selectedTrack;

	TDynArray< ITimelineItem* >		m_items;
	TDynArray< ITimelineItem* >		m_selectedItems;

	Float							m_visibleRangeDuration;	// Duration of visible range in seconds.
	Float							m_activeRangeDuration;	// Duration of active range in seconds.
	Float							m_activeRangeTimeOffset;// Offset from the start of visible range where active range starts. In seconds.

	Float							m_timeSelectorTimePos;	// Time selector position in seconds.
	Float							m_cursorTimePos;		// Cursor position in seconds.
	Float							m_prevCursorTimePos;	// Previous cursor position in seconds.
	wxPoint							m_cursorPos;			// Mouse position in pixels.
	wxPoint							m_lastRmbDownPos;		// Position associated with last RMB down event.
	Bool							m_midlleMouseDown;

	MovingContext					m_movingContext;

	Bool							m_timeLimitsEnabled;
	Float							m_timeLimitMin;
	Float							m_timeLimitMax;

	wxFont							m_wxItemFont;
	Gdiplus::Font*					m_itemFont;

	Uint32							m_verticalOffset;

	Bool							m_isSetSelectionLocked; // Blocks SetSelection() call in case of undesired callbacks coming from external systems avout changes being result of callbacks registered by these external systems locally
	Bool							m_startEdgeGrabbed;

	CEdPropertiesPage*				m_propertiesPage;
	Bool							m_externalProperties;

	CEdUndoManager*					m_undoManager;

private:
	Float							m_yScale;
	Float							m_currentGrid;
	Bool							m_hovered;
	States							m_state;
	TimeSetting						m_timeSettingTarget;
	Int32							m_keyboardModifiers;
	wxPoint							m_selectionStartPoint;
	Bool							m_frozen;
	const static String				CLIPBOARD;
	static TDynArray< Float >		s_copiedPositions;

protected:
	TimelineImpl::CDrawGroupTracks*		m_pinnedGroup;		// draw group for pinned tracks, owned by derived class
	TimelineImpl::CDrawGroupTracks*		m_defaultGroup;		// draw group for normal (unpinned) tracks, owned by derived class
	TimelineImpl::CDrawGroupTimebar*	m_timebarGroup;		// draw group for time bar elements
	TimelineImpl::CDrawGroupVolatile*	m_volatileGroup;	// draw group for frequently changing elements

	TimelineImpl::CDrawBuffer			m_pinnedBuffer;		// draw buffer for pinned tracks
	TimelineImpl::CDrawBuffer			m_defaultBuffer;	// draw buffer for normal (unpinned) tracks
	TimelineImpl::CDrawBuffer			m_timebarBuffer;	// draw buffer for time bar elements
	TimelineImpl::CDrawBuffer			m_volatileBuffer;	// draw buffer for frequently changing elements

	Gdiplus::Bitmap*					m_iconPinned;		// pinned icon
	Gdiplus::Bitmap*					m_iconUnpinned;		// unpinned icon

	wxBitmap							m_wxIconPinned;		// pinned icon
	wxBitmap							m_wxIconUnpinned;	// unpinned icon
	String								m_defaultTrackName; // Default track name

public:
	const static Int32				TIMELINE_TRACK_BTN_WIDTH;
	const static Int32				TIMELINE_EVENT_GRABABLE_WIDTH;
	const static Int32				TIMELINE_EXPAND_BTN_SIZE;
	const static Int32				TIMELINE_EXPAND_BTN_CROSS_THICKNESS;
	const static Int32				TIMELINE_DEPTH_SPACING;
	const static Int32				TIMELINE_PIN_BTN_SIZE;
	const static Char				GROUP_SEPARATOR;
	const static String				GROUP_SEPARATOR_STR;
	const static String             TIMELINE_DEFAULT_TRACK_NAME;
	

public:
	CEdTimeline( wxPanel* parentPanel, CEdPropertiesPage* propertiesPage = NULL, CanvasType canvasType = CanvasType::gdiplus, const String& defaultTrackName = TIMELINE_DEFAULT_TRACK_NAME );
	virtual ~CEdTimeline();

	void SetCurrentTime( Float time );
	void RequestSetTime( Float time );

	void SetTimeLimits( Float minTime, Float maxTime );
	void SetTimeLimitMin( Float minTime );
	void SetTimeLimitMax( Float maxTime );
	void GetTimeLimits( Float& minTime, Float& maxTime ) const;
	Bool TimeLimitsEnabled() const;
	void TimeLimitsEnabled( Bool state );

	Float GetActiveRangeDuration() const;
	Float GetVisibleRangeDuration() const;

	Int32 CalculatePixelPos( Float timePos ) const;
	Float CalculateTimePos( Int32 pixelPos ) const;

	// Called to refresh itself
	virtual void PaintCanvas( Int32 width, Int32 height );

	// Enables Undo/Redo of the timeline operations
	void SetUndoManager( CEdUndoManager* undoManager );

	CEdPropertiesPage* GetPropertiesPage() { return m_propertiesPage; }
	void UpdateSizeOfPropertiesPage();

	void AddItem( ITimelineItem* item );
	void RemoveItem( ITimelineItem* item );

	void ApplyLayout( const CTimelineLayout& layout );
	void GetLayout( CTimelineLayout& outLayout ); // TODO: const

protected:
	void NotifyBufferIsInvalid();

	void ClearItems();
	void CenterPosition( Float time );

	void SetDurationEventLength( ITimelineItem* item );

	RED_INLINE Float GetCurrentGrid() const
	{ return m_currentGrid; }
	
	virtual void RecreateTracks();

	virtual void SelectionChanged() {}
	virtual void OnTrackSelectionChanged() {}
	virtual void ItemPropertyChanged( TDynArray< ITimelineItem* >& items, const CName& propertyName ) {}

	virtual void FillCanvasMenu( wxMenu* menu ) {}
	virtual void FillTrackMenu( const String& name, wxMenu* menu ) {}
	virtual void FillItemMenu( ITimelineItem* item, wxMenu* menu, Bool& addDefaults ) { addDefaults = true; }

	virtual Bool CanFillDefaultCanvasMenu() const { return true; }
	virtual Bool CanFillDefaultTrackMenu() const { return true; }
	virtual Bool CanFillDefaultItemMenu() const { return true; }
	virtual Bool CanFillDefaultTrackButtonMenu() const { return true; }

	virtual void EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const = 0;

	virtual Bool CanChangeDefaultTrack() const = 0;
	virtual Bool CanResizeOverEnd() const = 0;
	virtual Bool CanUseSelectionItemMenu() const { return false; }

	virtual void SerializeItem( ITimelineItem* item, IFile& file ) = 0;
	virtual ITimelineItem* DeserializeItem( IFile& file ) = 0;

	virtual void OnDrawItem( const ITimelineItem* item, const wxRect& rect ) {}
	virtual void OnDrawTrack( Track* track ) {}

	virtual void OnLeftMouseDoubleClick( Int32 x, Int32 y ) {}
	virtual void OnLeftMouseClick( Int32 x, Int32 y );
	virtual void OnItemSelected( ITimelineItem* chosenItem );

	virtual void OnStateSelected( wxPoint globalPos );
	virtual void OnTrackDoubleClicked();

	void RemoveSelectedTrack();

	void CalculateNewGrid();

public:
	Int32 GetItemCount() const;
	ITimelineItem* GetItem( Int32 n ) const;
	Int32 GetSelectedItemCount() const;
	ITimelineItem* GetSelectedItem( Int32 n ) const;
	Bool IsSelected( const ITimelineItem* item );
	Bool IsOnlySelected( const ITimelineItem* item );
	Bool IsAnySelected();
	Bool IsOneSelected();
	virtual void SetSelection( TDynArray< ITimelineItem* >& items, Bool goToEvent = true );
	void SelectTrack( Int32 track );

	size_t GetTrackItems(Track* track, TDynArray<ITimelineItem*>& trackItems) const;
	Track* GetItemTrack(const ITimelineItem* item) const;
	wxPoint GetItemGlobalPosition( ITimelineItem* item ) const;

	TimelineImpl::CDrawGroupTracks* GetTrackDrawGroup( Track* track ) const;

	Int32 GetTrackIndex( const String& trackName ) const;
	Int32 GetTrackIndex( Track* track ) const;
	Float GetCurrentPosition() const { return m_timeSelectorTimePos; }

	const wxFont& GetWxItemFont() const { return m_wxItemFont; }
	Gdiplus::Font& GetItemFont() const { return *m_itemFont; }

	Float GetVertScale() const;
	void SetVertScale(Float scale);

	void SerializeItems( TDynArray< Uint8 >& buffer, const TDynArray< ITimelineItem* >& itemsToSerialize );
	void DeserializeItems( const TDynArray< Uint8 >& buffer, TDynArray< ITimelineItem* >& deserializedItems );

protected:
	void LockSetSelection( Bool lock ) { m_isSetSelectionLocked = lock; }
	virtual void OnItemPropertyChanged( wxCommandEvent& event );

private:
	// Events input
	void OnMouseMove( wxMouseEvent& event );
	void OnMouseLeave( wxMouseEvent& event );
	void OnResize( wxSizeEvent& event );
	void OnRightMouseUp( wxMouseEvent& event );
	void OnRightMouseDown( wxMouseEvent& event );
	void OnMiddleMouseUp( wxMouseEvent& event );
	void OnMiddleMouseDown( wxMouseEvent& event );
	void OnLeftMouseUp( wxMouseEvent& event );
	void OnLeftMouseDown( wxMouseEvent& event );
	void OnLeftMouseDoubleClick( wxMouseEvent& event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnChar( wxKeyEvent& event );
	void SelectItemAtMousePos( wxMouseEvent& event );

	// Events logic
	void OnNewTrack( wxCommandEvent& event );
	void OnDeleteTrack( wxCommandEvent& event );

	virtual void OnDeleteItem( wxCommandEvent& event );

	void OnPaste( wxCommandEvent& event );
	void OnRenameTrack( wxCommandEvent& event);
	void OnMenuSelectItem( wxCommandEvent& event);

	void ShowCanvasMenu();
	void ShowTrackMenu();

	virtual Bool IsCanvasMenuEnabled() const;
	virtual Bool IsTrackMenuEnabled() const;
	virtual Bool IsTrackButtonMenuEnabled() const;
	virtual Bool IsItemMenuEnabled() const;

public:
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );

public:
	virtual wxMenu* PrepareItemMenu();
	virtual void ShowItemMenu();

	void RefreshPropPageValues();

private:
	void ShowTrackButtonMenu( Track* track );

	Int32 GetNumOfGridLines();

private:
	void UpdateMousePosition( Int32 positionX );

	ITimelineItem* ChooseItemToSelect( const TDynArray< ITimelineItem* >& candidates );
	void ShowMenuChooseItemToSelect( const TDynArray< ITimelineItem* >& candidates );

	void RebuildFont();

	void SetState( States newState );

protected:
	virtual Float			Snap( Float value );
	virtual Float			ResizeItem( ITimelineItem* item, Float duration );
	virtual const wxCursor	GetResizeCursor() const { return wxCURSOR_SIZEWE; }
	
	virtual void SortTracks();
	virtual Track* CreateTrack( String name, Uint32 depth, bool isGroup ) const;
	virtual Bool AddTrack( const String& trackName );
	virtual Bool AddTrack( Track* track );
	virtual void RenameTrack( const String& trackName, const String& newTrackName );
	void DeleteTracks();

	Int32	GetKeyboardModifiers() const;

public: // otherwise all classes that use this to draw, would have to become friends of this class :)

	// getting registered draw groups
	TimelineImpl::CDrawGroupTracks* GetDrawGroupTracksPinned();
	TimelineImpl::CDrawGroupTracks* GetDrawGroupTracksDefault();
	TimelineImpl::CDrawGroupTimebar* GetDrawGroupTimebar();
	TimelineImpl::CDrawGroupVolatile* GetDrawGroupVolatile();

	String GetDefaultTrackName() const { return m_defaultTrackName; }

protected:
	void SchedulePropertiesRefresh();
	
	virtual void OnStateChanged( States prevState, States newState );

	void UpdateLayout();

	Track* GetTrackAt( wxPoint globalPos );
	Uint32 GetItemsAt( wxPoint globalPos, TDynArray<ITimelineItem*>& items );
	ITimelineItem* GetItemAt( wxPoint globalPos );
	size_t GetItemsInRect( const wxRect& globalRect, TDynArray<ITimelineItem*>& itemsInRect ) const;

	void CollectItemsAt( Float time, TDynArray<ITimelineItem*>& items, Float timeEps = 0.001f );

	Bool IsTrackPinned( Track* t );
	void PinTrack( Track* t );
	void UnpinTrack( Track* t );

public:
	void PinTrack( const String& t );

	void CopyItems( const TDynArray< ITimelineItem* >& itemsToCopy, Bool withCut );
	void PasteItems( TDynArray< ITimelineItem* >& pastedItems, EPasteItemsMode mode, Float customTime = 0.f, Bool pasteToSelectedTrack = true );

protected:
	Bool IsTrackGroupExpanded( Track* trackGroup ) const;
	void ExpandTrackGroup( Track* trackGroup );
	void CollapseTrackGroup( Track* trackGroup );
	void ShowTrack( Track* track );
	void HideTrack( Track* track );

	Uint32 CountTracksInHierarchy( Track* t );
	Bool IsTrackGroup( Track* t );
	Bool IsInTrackGroup( Track* t );

	// class invariant - m_pinnedGroup != 0 && m_defaultGroup != 0

	// registering draw groups
	void RegisterDrawGroupTracksPinned(TimelineImpl::CDrawGroupTracks* g);
	void RegisterDrawGroupTracksDefault(TimelineImpl::CDrawGroupTracks* g);
	void RegisterDrawGroupTimebar(TimelineImpl::CDrawGroupTimebar* g);
	void RegisterDrawGroupVolatile(TimelineImpl::CDrawGroupVolatile* g);

	virtual void HandleRepositionLeftEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers );
	virtual void HandleRepositionRightEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers );

private:
	virtual void RemoveItemImpl( ITimelineItem* item ) {}

	virtual void StoreLayout() = 0;
	virtual void RestoreLayout() = 0;
};

// =================================================================================================
// inlines
// =================================================================================================

RED_INLINE Bool CEdTimeline::TimeLimitsEnabled() const
{
	return m_timeLimitsEnabled;
}

RED_INLINE void CEdTimeline::TimeLimitsEnabled( Bool state )
{
	m_timeLimitsEnabled = state;
}

RED_INLINE Bool CEdTimeline::IsTrackGroup( Track* t )
{
	ASSERT(t);
	return t->m_isGroup;
}

RED_INLINE Bool CEdTimeline::IsInTrackGroup( Track* t )
{
	ASSERT(t);
	return t->m_parent != 0;
}

RED_INLINE TimelineImpl::CDrawGroupTracks* CEdTimeline::GetTrackDrawGroup( Track* track ) const
{
	if(m_pinnedGroup->TrackExists(track))
	{
		return m_pinnedGroup;
	}
	else if(m_defaultGroup->TrackExists(track))
	{
		return m_defaultGroup;
	}

	return 0;
}

RED_INLINE Track* CEdTimeline::GetTrackAt( wxPoint globalPos )
{
	Track* t = m_pinnedGroup->GetTrackAt(m_pinnedGroup->GetLocalPos(globalPos));
	if(!t)
	{
		t = m_defaultGroup->GetTrackAt(m_defaultGroup->GetLocalPos(globalPos));
	}

	return t;
}

/*
Gets items at specified position.

\param globalPos Position in question.
\param items Container to which to put items. It's not cleared before use.
\return Number of items found at specified position (and put to items container).
*/
RED_INLINE Uint32 CEdTimeline::GetItemsAt( wxPoint globalPos, TDynArray<ITimelineItem*>& items )
{
	Uint32 numItemsFound = m_pinnedGroup->GetItemsAt( m_pinnedGroup->GetLocalPos( globalPos ), items );
	if(numItemsFound == 0)
	{
		numItemsFound = m_defaultGroup->GetItemsAt( m_defaultGroup->GetLocalPos( globalPos ), items );
	}

	return numItemsFound;
}

/*
Gets item at specified position.

The function will choose which event to return if there are many items at specified position.
Point events have preference over duration events.
*/
RED_INLINE ITimelineItem* CEdTimeline::GetItemAt( wxPoint globalPos )
{
	ITimelineItem* item = m_pinnedGroup->GetItemAt(m_pinnedGroup->GetLocalPos(globalPos));
	if(!item)
	{
		item = m_defaultGroup->GetItemAt(m_defaultGroup->GetLocalPos(globalPos));
	}

	return item;
}

RED_INLINE Bool CEdTimeline::IsTrackPinned(Track* t)
{
	ASSERT(t);
	return m_pinnedGroup->TrackExists(t);
}

RED_INLINE Bool CEdTimeline::IsTrackGroupExpanded( Track* trackGroup ) const
{
	ASSERT( trackGroup );
	return trackGroup->m_isExpanded;
}

RED_INLINE void CEdTimeline::RegisterDrawGroupTracksPinned(TimelineImpl::CDrawGroupTracks* g)
{
	m_pinnedGroup = g;
}

RED_INLINE void CEdTimeline::RegisterDrawGroupTracksDefault(TimelineImpl::CDrawGroupTracks* g)
{
	m_defaultGroup = g;
}

RED_INLINE void CEdTimeline::RegisterDrawGroupTimebar(TimelineImpl::CDrawGroupTimebar* g)
{
	m_timebarGroup = g;
}

RED_INLINE void CEdTimeline::RegisterDrawGroupVolatile(TimelineImpl::CDrawGroupVolatile* g)
{
	m_volatileGroup = g;
}

RED_INLINE TimelineImpl::CDrawGroupTracks* CEdTimeline::GetDrawGroupTracksPinned()
{
	return m_pinnedGroup;
}

RED_INLINE TimelineImpl::CDrawGroupTracks* CEdTimeline::GetDrawGroupTracksDefault()
{
	return m_defaultGroup;
}

RED_INLINE TimelineImpl::CDrawGroupTimebar* CEdTimeline::GetDrawGroupTimebar()
{
	return m_timebarGroup;
}

RED_INLINE TimelineImpl::CDrawGroupVolatile* CEdTimeline::GetDrawGroupVolatile()
{
	return m_volatileGroup;
}

RED_INLINE Float CEdTimeline::GetVertScale() const
{
	return m_yScale;
}

RED_INLINE void CEdTimeline::SetVertScale(Float scale)
{
	m_yScale = scale;
}

/*
Returns duration of active range in seconds.

\return Duration of active range in seconds.
*/
RED_INLINE Float CEdTimeline::GetActiveRangeDuration() const
{
	return m_activeRangeDuration;
}

/*
Returns duration of visible range in seconds.

\return Duration of visible range in seconds.
*/
RED_INLINE Float CEdTimeline::GetVisibleRangeDuration() const
{
	return m_visibleRangeDuration;
}
