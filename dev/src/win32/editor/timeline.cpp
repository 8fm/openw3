/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "timeline.h"
#include "undoTimeLine.h"
#include "dialogEditorUtils.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/memoryFileReader.h"

#include <wx/font.h>

// =================================================================================================
namespace {
// =================================================================================================

class SnapResult
{
public:
	ITimelineItem* item;
	Float timePos;
};

/*

\param timePos Time position that is to be snapped.
\param snapTargets List of items to which timePos can be snapped (only start positions of items are considered).
\param snapTimeDist Snap distance, in seconds.
*/
SnapResult SnapToItemStart( Float timePos, const TDynArray< ITimelineItem* >& snapTargets, Float snapTimeDist )
{
	SnapResult snapResult = { nullptr, 0.0f };

	// TODO: use lower bound, upper bound
	for( auto it = snapTargets.Begin(), end = snapTargets.End(); it != end; ++it )
	{
		const Float itemStartTime = ( *it )->GetStart();
		const Float timeDist = std::fabs( itemStartTime - timePos );

		if( timeDist <= snapTimeDist && ( !snapResult.item || timeDist < std::fabs( snapResult.timePos - timePos ) ) )
		{
			snapResult.item = *it;
			snapResult.timePos = itemStartTime;
		}
	}

	return snapResult;
}

/*

\param timePos Time position that is to be snapped.
\param snapTargets List of items to which timePos can be snapped (only end positions of items are considered).
\param snapTimeDist Snap distance, in seconds.
*/
SnapResult SnapToItemEnd( Float timePos, const TDynArray< ITimelineItem* >& snapTargets, Float snapTimeDist )
{
	SnapResult snapResult = { nullptr, 0.0f };

	// TODO: use lower bound, upper bound - CAN'T do this for duration items.. or create another list with duration items sorted by end times
	for( auto it = snapTargets.Begin(), end = snapTargets.End(); it != end; ++it )
	{
		if( ( *it )->IsDuration() )
		{
			const Float itemEndTime = ( *it )->GetStart() + ( *it )->GetDuration();
			const Float timeDist = std::fabs( itemEndTime - timePos );

			if( timeDist <= snapTimeDist && ( !snapResult.item || timeDist < std::fabs( snapResult.timePos - timePos ) ) )
			{
				snapResult.item = *it;
				snapResult.timePos = itemEndTime;
			}
		}
	}

	return snapResult;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

enum ETimelineCommandId
{
	ID_COPY_ITEM = 5001,
	ID_CUT_ITEM,
	ID_PASTE_ITEM_MOUSE,
	ID_PASTE_ITEM_KEYBOARD,
	ID_PASTE_ITEM_SPECIAL,
	ID_DELETE_ITEM,
	ID_NEW_TRACK,
	ID_DELETE_TRACK,
	ID_RENAME_TRACK,
	ID_SEL_ITEM_FIRST = 5100,
	ID_SEL_ITEM_LAST = 5110,
};

wxDEFINE_EVENT( usrEVT_REFRESH_PREVIEW, wxCommandEvent );
wxDEFINE_EVENT( usrEVT_TIMELINE_REQUEST_SET_TIME, wxCommandEvent );
wxDEFINE_EVENT( usrEVT_TIMELINE_RESIZED, wxCommandEvent );

wxBEGIN_EVENT_TABLE( CEdTimeline, CEdCanvas )
	EVT_MOTION( CEdTimeline::OnMouseMove )
	EVT_LEAVE_WINDOW( CEdTimeline::OnMouseLeave )
	EVT_SIZE( CEdTimeline::OnResize )
	EVT_RIGHT_DOWN( CEdTimeline::OnRightMouseDown )
	EVT_RIGHT_UP( CEdTimeline::OnRightMouseUp )
	EVT_LEFT_DOWN( CEdTimeline::OnLeftMouseDown )
	EVT_LEFT_DCLICK( CEdTimeline::OnLeftMouseDoubleClick )
	EVT_LEFT_UP( CEdTimeline::OnLeftMouseUp )
	EVT_MIDDLE_DOWN( CEdTimeline::OnMiddleMouseDown )
	EVT_MIDDLE_UP( CEdTimeline::OnMiddleMouseUp )
	EVT_MOUSEWHEEL( CEdTimeline::OnMouseWheel )
	EVT_CHAR( CEdTimeline::OnChar )
	EVT_MENU( ID_COPY_ITEM, CEdTimeline::OnEditCopy )
	EVT_MENU( ID_CUT_ITEM, CEdTimeline::OnEditCut )
	EVT_MENU( ID_DELETE_ITEM, CEdTimeline::OnDeleteItem )
	EVT_MENU( ID_PASTE_ITEM_MOUSE, CEdTimeline::OnPaste )
	EVT_MENU( ID_PASTE_ITEM_KEYBOARD, CEdTimeline::OnPaste )
	EVT_MENU( ID_PASTE_ITEM_SPECIAL, CEdTimeline::OnPaste )
	EVT_MENU( ID_NEW_TRACK, CEdTimeline::OnNewTrack )
	EVT_MENU( ID_DELETE_TRACK, CEdTimeline::OnDeleteTrack )
	EVT_MENU( ID_RENAME_TRACK, CEdTimeline::OnRenameTrack )
wxEND_EVENT_TABLE()

class ItemPopupWrapper : public wxObject
{
public:
	ITimelineItem*	m_item;

	ItemPopupWrapper( ITimelineItem* item ) : m_item( item ) {}
};

const Int32    CEdTimeline::TIMELINE_TRACK_BTN_WIDTH = 130;
const Int32    CEdTimeline::TIMELINE_EVENT_GRABABLE_WIDTH = 8;
const String   CEdTimeline::CLIPBOARD( TXT( "TimelineEvent" ) ) ;
const Int32    CEdTimeline::TIMELINE_EXPAND_BTN_SIZE = 12;
const Int32    CEdTimeline::TIMELINE_EXPAND_BTN_CROSS_THICKNESS = 2;
const Int32    CEdTimeline::TIMELINE_DEPTH_SPACING = 11;
const Int32    CEdTimeline::TIMELINE_PIN_BTN_SIZE = 16;
const Char	   CEdTimeline::GROUP_SEPARATOR = '.';
const String   CEdTimeline::GROUP_SEPARATOR_STR = String( &GROUP_SEPARATOR, 1 );
const String   CEdTimeline::TIMELINE_DEFAULT_TRACK_NAME = TXT("__Default");

TDynArray< Float > CEdTimeline::s_copiedPositions = TDynArray< Float >();

CEdTimeline::CEdTimeline( wxPanel* parentPanel, CEdPropertiesPage* propertiesPage, CanvasType canvasType, const String& defaultTrackName )
	: CEdCanvas( parentPanel, 0, canvasType )
	, m_items()
	, m_currentGrid( -1.0f )
	, m_selectedItems()
	, m_hovered( true )
	, m_cursorTimePos( 0.0f )
	, m_prevCursorTimePos( 0.0f )
	, m_state( STATE_NORMAL )
	, m_startEdgeGrabbed( true )
	, m_cursorPos()
	, m_activeRangeTimeOffset( 0.0f )
	, m_visibleRangeDuration( 1.0f )
	, m_activeRangeDuration( 0.0f )
	, m_timeSelectorTimePos( 0.0f )
	, m_tracks()
	, m_selectedTrack( -1 )
	, m_verticalOffset( 0 )
	, m_propertiesPage( propertiesPage )
	, m_externalProperties( propertiesPage != NULL )
	, m_timeLimitsEnabled( false )
	, m_itemFont( NULL )
	, m_keyboardModifiers( 0 )
	, m_yScale( 1.0f )
	, m_frozen( false )
	, m_defaultTrackName( defaultTrackName )
	, m_pinnedGroup(0)
	, m_defaultGroup(0)
	, m_timebarGroup(0)
	, m_volatileGroup(0)
	, m_iconPinned(0)
	, m_iconUnpinned(0)
	, m_isSetSelectionLocked( false )
	, m_midlleMouseDown( false )
	, m_pinnedBuffer(canvasType, &wxClientDC(this))		// TODO: msvc extension used
	, m_defaultBuffer(canvasType, &wxClientDC(this))	// TODO: msvc extension used
	, m_timebarBuffer(canvasType, &wxClientDC(this))	// TODO: msvc extension used
	, m_volatileBuffer(canvasType, this)				// TODO: msvc extension used
{
	// Create properties page
	if ( m_propertiesPage == NULL )
	{
		PropertiesPageSettings settings;
		m_propertiesPage = new CEdPropertiesPage( this, settings, nullptr );
		m_propertiesPage->Show( false );
	}
	m_propertiesPage->Connect( wxEVT_COMMAND_PROPERTY_CHANGED,
		wxCommandEventHandler( CEdTimeline::OnItemPropertyChanged ), NULL, this );

	ASSERT( m_propertiesPage != NULL );

	RebuildFont();

	m_wxIconPinned = SEdResources::GetInstance().LoadBitmap( TXT( "IMG_PINNED" ) );
	m_wxIconUnpinned = SEdResources::GetInstance().LoadBitmap( TXT("IMG_UNPINNED") );

	m_iconPinned = ConvertToGDI( m_wxIconPinned );
	m_iconUnpinned = ConvertToGDI( m_wxIconUnpinned );
}

void CEdTimeline::SetUndoManager( CEdUndoManager* undoManager )
{
	m_undoManager = undoManager;
	if ( m_undoManager && m_propertiesPage && m_externalProperties == false )
	{
		m_propertiesPage->SetUndoManager( m_undoManager );
	}
}

void CEdTimeline::DeleteTracks()
{
	for ( Uint32 i = 0; i < m_tracks.Size(); ++i )
	{
		delete m_tracks[i];
	}
	m_tracks.ClearFast();

	m_pinnedGroup->RemoveAllTracks();
	m_defaultGroup->RemoveAllTracks();
}

CEdTimeline::~CEdTimeline()
{
	ClearItems();

	DeleteTracks();

	if ( m_itemFont )
	{
		delete m_itemFont;
		m_itemFont = NULL;
	}

	delete m_volatileGroup;
	delete m_timebarGroup;
	delete m_defaultGroup;
	delete m_pinnedGroup;
}

/*

*/
void CEdTimeline::UpdateLayout()
{
	Int32 width = 0;
	Int32 height = 0;
	GetClientSize(&width, &height);

	// This is the only place where layout is computed. 
	// If timeline is resized then UpdateLayout() is called.
	// If track is added/removed from default group then UpdateLayout() is called.
	// No other place is allowed to change layout.

	// compute preferred heights
	Int32 topBufferPreferredHeight = m_pinnedGroup->GetPreferredHeight();
	Int32 defaultBufferPreferredHeight = m_defaultGroup->GetPreferredHeight();
	Int32 timebarBufferPreferredHeight = m_timebarGroup->GetPreferredHeight();
	Int32 volatileBufferPreferredHeight = m_volatileGroup->GetPreferredHeight();

	m_pinnedBuffer.Resize(width, topBufferPreferredHeight);
	m_defaultBuffer.Resize(width, height - topBufferPreferredHeight - timebarBufferPreferredHeight);
	m_timebarBuffer.Resize(width, timebarBufferPreferredHeight);
	m_volatileBuffer.Resize(width, volatileBufferPreferredHeight);

	m_pinnedBuffer.SetTargetPos(wxPoint(0, 0));
	m_defaultBuffer.SetTargetPos(wxPoint(0, topBufferPreferredHeight));
	m_timebarBuffer.SetTargetPos(wxPoint(0, height - timebarBufferPreferredHeight));
	m_volatileBuffer.SetTargetPos(wxPoint(0, 0));
}

void CEdTimeline::PaintCanvas( Int32 width, Int32 height )
{
	if ( m_frozen )
	{
		return;
	}

	// If draw buffers are not initialized then initialize them. This happens
	// when after window creation we receive paint event before resize event.
	Bool drawBuffersInitialized = true;
	if( GetCanvasType() == CanvasType::gdiplus )
	{
		if( !m_defaultBuffer.GetGraphics() )
		{
			drawBuffersInitialized = false;
		}
	}
	else
	{
		if( !m_defaultBuffer.GetWxBitmap() )
		{
			drawBuffersInitialized = false;
		}
	}
	if( !drawBuffersInitialized )
	{
		UpdateLayout();
		drawBuffersInitialized = true;
	}

	if( m_currentGrid <= 0.0f )
	{
		CalculateNewGrid();
	}

	// draw all groups that use top buffer
	if(!m_pinnedBuffer.IsValid())
	{
		m_pinnedGroup->Draw();
		m_pinnedBuffer.SetValid(true);
	}

	// draw all groups that use default buffer
	if(!m_defaultBuffer.IsValid())
	{
		m_defaultGroup->Draw();
		m_defaultBuffer.SetValid(true);
	}

	// draw all groups that use timebar buffer
	if(!m_timebarBuffer.IsValid())
	{
		m_timebarGroup->Draw();
		m_timebarBuffer.SetValid(true);
	}

	wxPoint targetPosition(0, 0);

	if( GetCanvasType() == CanvasType::gdiplus )
	{
		// draw top buffer to canvas buffer
		m_pinnedBuffer.GetTargetPos(targetPosition);
		m_bufferDevice->DrawImage(m_pinnedBuffer.GetBitmap(), targetPosition.x, targetPosition.y);

		// draw default buffer to canvas buffer, just below top buffer
		m_defaultBuffer.GetTargetPos(targetPosition);
		m_bufferDevice->DrawImage(m_defaultBuffer.GetBitmap(), targetPosition.x, targetPosition.y);

		// draw timber buffer at the bottom of canvas buffer
		m_timebarBuffer.GetTargetPos(targetPosition);
		m_bufferDevice->DrawImage(m_timebarBuffer.GetBitmap(), targetPosition.x, targetPosition.y);

		// draw volatile buffer (note that volatile buffer is actually canvas buffer - that's why we draw it here)
		if(!m_volatileBuffer.IsValid())
		{
			m_volatileGroup->Draw();
			m_volatileBuffer.SetValid(true);
		}
	}
	else
	{
		m_pinnedBuffer.GetTargetPos(targetPosition); 
		m_memDC->Blit(targetPosition.x, targetPosition.y, m_pinnedBuffer.GetWxBitmap()->GetWidth(), m_pinnedBuffer.GetWxBitmap()->GetHeight(), m_pinnedBuffer.GetMemoryDC(), 0, 0);

		m_defaultBuffer.GetTargetPos(targetPosition); 
		m_memDC->Blit(targetPosition.x, targetPosition.y, m_defaultBuffer.GetWxBitmap()->GetWidth(), m_defaultBuffer.GetWxBitmap()->GetHeight(), m_defaultBuffer.GetMemoryDC(), 0, 0);

		m_timebarBuffer.GetTargetPos(targetPosition); 
		m_memDC->Blit(targetPosition.x, targetPosition.y, m_timebarBuffer.GetWxBitmap()->GetWidth(), m_timebarBuffer.GetWxBitmap()->GetHeight(), m_timebarBuffer.GetMemoryDC(), 0, 0);

		// draw volatile buffer (note that volatile buffer is actually canvas buffer - that's why we draw it here)
		if(!m_volatileBuffer.IsValid())
		{
			m_volatileGroup->Draw();
			m_volatileBuffer.SetValid(true);
		}
	}
}

/*
Sets current time.

SetCurrentTime() changes timeline state only, i.e. it ignores anything that might be associated
with a timeline (e.g. an animation or a scene section). It's meant to be called by something that
controls the timeline and everything that's associated with it.
*/
void CEdTimeline::SetCurrentTime( Float time )
{
	m_timeSelectorTimePos = time;

	// Invalidate appropriate draw buffer.
	m_volatileGroup->GetDrawBuffer()->SetValid( false );
}

/*
Requests time change.

RequestSetTime() emits "request set time" event so that timeline and anything associated with it
(e.g. an animation or a scene section) are updated together. "Request set time" handler should
call SetCurrentTime() at some point to update timeline state.
*/
void CEdTimeline::RequestSetTime( Float time )
{
	wxCommandEvent timeEvent( usrEVT_TIMELINE_REQUEST_SET_TIME );
	timeEvent.SetEventObject( this );
	//THIS WILL LEAK MEMORY
	timeEvent.SetClientObject( new TClientDataWrapper< Float >( time ) );
	ProcessEvent( timeEvent );
}

Float CEdTimeline::Snap( Float value )
{
	for( Float t = 0.0f; t < m_visibleRangeDuration - m_activeRangeTimeOffset; t += m_currentGrid / 16.0f )
	{
		if ( value >= t - m_currentGrid / 16.0f && value < t + m_currentGrid / 16.0f )
		{
			return t;
		}
	}

	// Out of positive range
	return value;
}

void CEdTimeline::RecreateTracks()
{
	DeleteTracks();


	// Add default track and pin it.
	AddTrack( m_defaultTrackName );
	Track* defaultTrack = m_tracks[ GetTrackIndex( m_defaultTrackName ) ];
	PinTrack(defaultTrack);

	// Create tracks mentioned by all existing items.
	for( auto itemIter = m_items.Begin(), end = m_items.End(); itemIter != end; ++itemIter )
	{
		const ITimelineItem* item = *itemIter;

		// Skip invalid events.
		if ( item == NULL )
		{
			continue;
		}

		// Get item track name. 'Default' if empty.
		String trackName = item->GetTrackName().Empty()? m_defaultTrackName : item->GetTrackName();
		AddTrack( trackName );
	}

	UpdateLayout();
}

/*
Calculates pixel position that corresponds to specified time position.

\param relativeTimePos Time position in seconds, relative to start of active range.
\return Pixel position in timeline space.
*/
Int32 CEdTimeline::CalculatePixelPos( Float timePos ) const
{
	Float relativePosition = m_visibleRangeDuration != 0.0f? ( m_activeRangeTimeOffset + timePos ) / ( m_visibleRangeDuration ) : 0.0f;
	return relativePosition * GetSize().GetWidth();
}

/*
Calculates time position that corresponds to specified pixel position.

\param pixelPos Pixel position in timeline space.
\return Time position in seconds, relative to start of active range.
*/
Float CEdTimeline::CalculateTimePos( Int32 pixelPos ) const
{
	Float relativePosition = static_cast< Float >( pixelPos ) / GetSize().GetWidth();
	return relativePosition * m_visibleRangeDuration - m_activeRangeTimeOffset;
}

void CEdTimeline::OnMouseMove( wxMouseEvent& event )
{
	m_cursorPos = event.GetPosition();
	m_keyboardModifiers	= event.GetModifiers();

	// Update mouse position
	m_hovered = true;

	// Flag to indicate if still moving timeline
	Bool stillMoving = false;
	static Int32 prevX = 0;
	static Int32 prevY = 0;

	UpdateMousePosition( event.GetX() );

	switch( m_state )
	{
	case STATE_DRAG_TRACK_OR_SELECT_ALL_EVENTS_ON_TRACK:
		if(!event.LeftIsDown())
		{
			m_state = STATE_NORMAL;
			break;
		}
		else
		{
			m_state = STATE_DRAG_TRACK;
		}
		// fall through
	case STATE_DRAG_TRACK:
		{
			ASSERT(m_selectedTrack >= 0); // we can't be dragging a track if we didn't select one

			wxPoint globalPos(event.GetX(), event.GetY());

			Track* hoveredTrack = GetTrackAt(globalPos);
			Track* selectedTrack = m_tracks[m_selectedTrack];

			if(hoveredTrack && hoveredTrack != selectedTrack)
			{
				// dragging tracks allowed only within the same draw group
				TimelineImpl::CDrawGroupTracks* drawGroup = GetTrackDrawGroup(selectedTrack);
				if(drawGroup == GetTrackDrawGroup(hoveredTrack))
				{
					// this if prevents ugly effect when dragging half-height track onto full-height track
					Int32 hoveredTrackY = drawGroup->GetDispTrackLocalPos(hoveredTrack).y;
					Int32 selectedTrackHeight = drawGroup->GetTrackHeight(selectedTrack);
					if(hoveredTrackY + selectedTrackHeight > drawGroup->GetLocalPos(globalPos).y)
					{
						// tracks must belong to the same track group or not belong to any track group
						if(hoveredTrack->m_parent == selectedTrack->m_parent)
						{
							drawGroup->ExchangePositions(selectedTrack, hoveredTrack);

							// Invalidate appropriate draw buffer.
							drawGroup->GetDrawBuffer()->SetValid( false );
						}
					}
				}
			}
		}
		break;
	case STATE_NORMAL:
	case STATE_SELECTED:
	case STATE_SELECTED_HOVERING_EDGE:
		{
			SetCursor( wxCURSOR_DEFAULT );

			SetState( IsAnySelected() ? STATE_SELECTED : STATE_NORMAL );

			if( event.RightIsDown() && ( event.GetX() != prevX || event.GetY() != prevY ) )
			{
				SetState( STATE_MOVING );
				break;
			}

			// handle hovering over one of edges of selected item
			if( GetSelectedItemCount() == 1 && GetSelectedItem( 0 )->IsDuration() )
			{
				ITimelineItem* item = GetSelectedItem( 0 );

				// first check if selected item is on a track pointed by mouse cursor
				Track* track = GetTrackAt(event.GetPosition());
				if( track && track->m_name == item->GetTrackName() )
				{
					// Check if hovering start edge
					if( event.GetX() - CalculatePixelPos( item->GetStart() ) >= 0
						&& event.GetX() - CalculatePixelPos( item->GetStart() ) <= TIMELINE_EVENT_GRABABLE_WIDTH
						&& item->IsLeftResizable() )
					{
						SetState( STATE_SELECTED_HOVERING_EDGE );
						m_startEdgeGrabbed = true;
						SetCursor( GetResizeCursor() );
					}
					else if( CalculatePixelPos( item->GetStart() + item->GetDuration() ) - event.GetX() >= 0
						&& CalculatePixelPos( item->GetStart() + item->GetDuration() ) - event.GetX() <= TIMELINE_EVENT_GRABABLE_WIDTH 
						&& item->IsRightResizable() )
					{
						SetState( STATE_SELECTED_HOVERING_EDGE );
						m_startEdgeGrabbed = false;
						SetCursor( GetResizeCursor() );
					}
					// Check second part of duration event
					else if( item->GetStart() + item->GetDuration() > m_activeRangeDuration )
					{
						Float dur = item->GetStart() + item->GetDuration() - m_activeRangeDuration;

						if(  CalculatePixelPos( dur ) - event.GetX() > 0 
							&& CalculatePixelPos( dur ) - event.GetX() < TIMELINE_EVENT_GRABABLE_WIDTH
							&& item->IsRightResizable() )
						{
							SetState( STATE_SELECTED_HOVERING_EDGE );
							m_startEdgeGrabbed = false;
							SetCursor( GetResizeCursor() );
						}
					}
				}
			}

			// Invalidate appropriate draw buffer.
			m_volatileGroup->GetDrawBuffer()->SetValid( false );

			break;
		}
	case STATE_RESIZING:
		{
			// Find moved edge
			ASSERT( IsOneSelected() );

			ITimelineItem* selectedItem = m_selectedItems[ 0 ];

			if ( m_undoManager )
			{
				CUndoTimelineItemLayout::PrepareStep( *m_undoManager, this, selectedItem );
			}

			// get modifiers
			TimelineKeyModifiers keyModifiers;
			keyModifiers.alt = event.AltDown();
			keyModifiers.ctrl = event.ControlDown();
			keyModifiers.shift = event.ShiftDown();

			if( m_startEdgeGrabbed )
			{
				// TODO: This code allows duration event to have duration 0.0f. Maybe HandleRepositionLeftEdge() should decide whether this is ok or not?
				const Float newTimePos = Clamp( m_cursorTimePos, 0.0f, selectedItem->GetStart() + selectedItem->GetDuration() );
				HandleRepositionLeftEdge( selectedItem, newTimePos, keyModifiers );
			}
			else // end edge grabbed
			{
				// TODO: This code allows duration event to have duration 0.0f. Maybe HandleRepositionRightEdge() should decide whether this is ok or not?
				const Float newTimePos = Clamp( m_cursorTimePos, selectedItem->GetStart(), m_activeRangeDuration );
				HandleRepositionRightEdge( selectedItem, newTimePos, keyModifiers );
			}

			// Update properties page
			m_propertiesPage->RefreshValues();

			// Invalidate appropriate buffers. When resizing items, we always invalidate both pinned and default buffers
			// because resizing one item may cause other items to move and we have no general way of detecting this here.
			m_pinnedGroup->GetDrawBuffer()->SetValid( false );
			m_defaultGroup->GetDrawBuffer()->SetValid( false );

			break;
		}

	case STATE_MOVING:
		{
			// Check if still holding right button
			if( ! event.RightIsDown() )
			{
				SetState( IsAnySelected() ? STATE_SELECTED : STATE_NORMAL );
			}
			else
			{
				stillMoving = true;

				// Horizontal movement
				if( prevX != 0 )
				{
					Int32 offset = event.GetX() - prevX;
					m_activeRangeTimeOffset += CalculateTimePos( offset ) - CalculateTimePos( 0.0f );

					// Invalidate appropriate draw buffers.
					m_pinnedBuffer.SetValid( false );
					m_defaultBuffer.SetValid( false );
					m_timebarBuffer.SetValid( false );
				}

				if( prevY != 0 )
				{
					// Vertical movement
					Int32 offset = prevY - event.GetY();

					// Scroll
					m_verticalOffset += offset;
					m_defaultBuffer.SetVerticalOffset(m_verticalOffset); // note that other buffers don't get it

					// Invalidate appropriate draw buffer ("pinned" and "timebar" buffers are not invalidated by vertical movement).
					m_defaultBuffer.SetValid( false );
				}
			}
		}
		break;
	case STATE_DRAGGING:
		{
			ASSERT( IsAnySelected() );

			// Check if still holding left button
			if( ! event.LeftIsDown() )
			{
				m_movingContext.Deinit();

				SetState( STATE_SELECTED );
			}
			else
			{
				// Moving between tracks (only for one event)
				Track* newTrack = GetTrackAt(wxPoint(event.GetX(), event.GetY()));
				if( event.AltDown() && IsOneSelected() && newTrack && !IsTrackGroup(newTrack) )
				{
					// Invalidate appropriate draw buffers. Note that item may be moved between "pinned" and "default"
					// groups in which case both "pinned" and "default" draw buffers should be invalidated.
					GetTrackDrawGroup( GetItemTrack( m_selectedItems[ 0 ] ) )->GetDrawBuffer()->SetValid( false );
					GetTrackDrawGroup( newTrack )->GetDrawBuffer()->SetValid( false );

					m_selectedItems[ 0 ]->SetTrackName( newTrack->m_name );
				}

				// TODO: by default don't allow duration events to wrap.
				// TODO: handle undo.

				if( m_movingContext.m_allItemsMovable )
				{
					// We're moving mouse cursor with offset
					Float resultingPos = m_cursorTimePos - m_movingContext.m_cursorToLeaderInitialDist;
					
					// Handle snapping.
					Bool snappingEnabled = event.ShiftDown();
					if( snappingEnabled )
					{
						// Convert snap distance from pixels to seconds.
						const Float snapTimeDistance = m_movingContext.m_snapPixelDistance / GetSize().GetWidth() * m_visibleRangeDuration;

						auto snapPosition = [ this, snapTimeDistance ] ( Float pos, Float refPos ) -> Float
						{
							SnapResult snapResult0 = SnapToItemStart( pos, m_movingContext.m_snapTargets, snapTimeDistance );
							SnapResult snapResult1 = SnapToItemEnd( pos, m_movingContext.m_snapTargets, snapTimeDistance );

							// choose best snap result
							if( snapResult0.item && ( !snapResult1.item || ( std::fabs( snapResult0.timePos - refPos ) <= std::fabs( snapResult1.timePos - refPos ) ) ) )
							{
								pos = snapResult0.timePos;
							}
							else if( snapResult1.item && ( !snapResult0.item || ( std::fabs( snapResult1.timePos - refPos ) <= std::fabs( snapResult0.timePos - refPos ) ) ) )
							{
								pos = snapResult1.timePos;
							}

							return pos;
						};

						const Bool snapItemStart = !m_movingContext.m_leaderItem->IsDuration() || m_movingContext.m_cursorToLeaderInitialDist <= 0.5f * m_movingContext.m_leaderItem->GetDuration();
						if( snapItemStart )
						{
							const Float posToSnap = resultingPos;
							resultingPos = snapPosition( posToSnap, m_movingContext.m_leaderItem->GetStart() );
						}
						else // snap item end
						{
							const Float posToSnap = resultingPos + m_movingContext.m_leaderItem->GetDuration();
							resultingPos = snapPosition( posToSnap, m_movingContext.m_leaderItem->GetStart() + m_movingContext.m_leaderItem->GetDuration() );
							resultingPos -= m_movingContext.m_leaderItem->GetDuration();
						}
					}

					// Compute delta - distance by which to move all items:
					// delta = resultingPos - m_movingContext.m_leader->GetStart()
					//
					// Before computing delta we may need to alter resultingPos so that computed delta doesn't
					// make sentinel items fall out of active range of timeline. Following must be satisfied:
					//
					// 1) sentinelMin + delta >= 0.0f
					//    which gives us:
					//    resultingPos >= m_movingContext.m_leader->GetStart() - sentinelMin
					//
					// 2) sentinelMax + delta < m_activeRangeDuration
					//    which gives us:
					//    resultingPos < m_activeRangeDuration + m_movingContext.m_leader->GetStart() - sentinelMax
					//
					const Float resultingPosMin = m_movingContext.m_leaderItem->GetStart() - m_movingContext.m_moveSet[ 0 ]->GetStart();
					const Float resultingPosMax = m_activeRangeDuration + m_movingContext.m_leaderItem->GetStart() - m_movingContext.m_moveSet.Back()->GetStart() - NumericLimits< Float >::Epsilon();
					resultingPos = Clamp( resultingPos, resultingPosMin, resultingPosMax );
					
					// Now we can compute delta.
					Float delta = resultingPos - m_movingContext.m_leaderItem->GetStart();

					// Shallow move all items by calculated distance. Shallow move means that moving an item doesn't move
					// any items that are linked with it (note that interpolation event keys are not considered to be linked
					// to interpolation event itself so moving interpolation event will also move all its keys - the same is
					// true for blend events).
					for( auto itItem = m_movingContext.m_moveSet.Begin(), endItems = m_movingContext.m_moveSet.End(); itItem != endItems; ++itItem )
					{
						ITimelineItem* item = *itItem;
						item->SetStart( item->GetStart() + delta, false );

						// Invalidate appropriate buffer.
						GetTrackDrawGroup( GetItemTrack( item ) )->GetDrawBuffer()->SetValid( false );
					}
				}
			}
		}
		break;
	case STATE_TIME_SETTING:
		{
			switch ( m_timeSettingTarget )
			{
			case TIME_SETTING_CURRENT_TIME:
				{
					RequestSetTime( m_cursorTimePos );
				}
				break;
			case TIME_SETTING_MIN_LIMIT:
				if ( m_cursorTimePos > m_timeLimitMax )
				{
					m_cursorTimePos = m_timeLimitMax;
				}
				SetTimeLimits( m_cursorTimePos, m_timeLimitMax );
				
				// Invalidate appropriate draw buffer.
				m_volatileGroup->GetDrawBuffer()->SetValid( false );

				break;

			case TIME_SETTING_MAX_LIMIT:
				if ( m_cursorTimePos < m_timeLimitMin )
				{
					m_cursorTimePos = m_timeLimitMin;
				}
				SetTimeLimits( m_timeLimitMin, m_cursorTimePos );
				
				// Invalidate appropriate draw buffer.
				m_volatileGroup->GetDrawBuffer()->SetValid( false );

				break;
			}

		}
		break;
	}

	// Update previous Y position
	prevY = ( stillMoving ) ? event.GetY() : 0;
	prevX = ( stillMoving ) ? event.GetX() : 0;

	// Update previous position
	m_prevCursorTimePos = m_cursorTimePos;

	// Refresh animation preview
	wxCommandEvent timeEvent( usrEVT_REFRESH_PREVIEW );
	timeEvent.SetEventObject( this );
	ProcessEvent( timeEvent );

	m_keyboardModifiers = 0;
}

void CEdTimeline::OnMouseLeave( wxMouseEvent& event )
{
	NotifyBufferIsInvalid();
	m_hovered = false;
}

void CEdTimeline::OnResize( wxSizeEvent& event )
{
	UpdateLayout();

	NotifyBufferIsInvalid();
	CalculateNewGrid();
	Refresh();
	UpdateSizeOfPropertiesPage();
}

void CEdTimeline::UpdateSizeOfPropertiesPage()
{
	if ( m_propertiesPage && m_propertiesPage->IsShown() && m_externalProperties == false )
	{
		wxSize size = m_propertiesPage->GetSize();
		size.SetWidth( Clamp( size.GetWidth(), 300, 10000 ) );
		size.SetHeight( Clamp( size.GetHeight(), 100, 10000 ) );
		m_propertiesPage->SetPosition( wxPoint( GetSize().GetWidth() - size.GetWidth(), 0 ) );
		m_propertiesPage->SetSize( size.GetWidth(), GetSize().GetHeight() );
	}
}

void CEdTimeline::CalculateNewGrid()
{
	m_currentGrid = CalculateTimePos( 150 ) - CalculateTimePos( 0 );
}


void CEdTimeline::SelectItemAtMousePos( wxMouseEvent& event )
{
	NotifyBufferIsInvalid();
	SetFocus();

	wxPoint pos(event.GetX(), event.GetY());

	Track* t = GetTrackAt(pos);
	SelectTrack( GetTrackIndex(t) );

	TDynArray< ITimelineItem* > itemsAtPos;
	Uint32 numItemsAtPos = GetItemsAt(pos, itemsAtPos);

	// Check if any of items at pos is already selected.
	Bool itemAtPosAlreadySelected = false;
	for( auto it = itemsAtPos.Begin(), end = itemsAtPos.End(); it != end; ++it )
	{
		if( IsSelected( *it ) )
		{
			itemAtPosAlreadySelected = true;
			break;
		}
	}

	// If any of items at pos is already selected then don't change selection as user most probably wants
	// to bring out context menu for all selected items. Otherwise, clear current selection and select item
	// under cursor. If there are many possible items to select then choose the "best" one. If none of items
	// is "best" then don't select any - in such case user is expected to select desired item using LMB and
	// perform RMB again (LMB selection is more powerful than RMB selection as it can show additional menu
	// to let the user choose desired item - RMB selection doesn't do this as this would be too confusing).
	if( !itemAtPosAlreadySelected )
	{
		m_selectedItems.Clear();

		ITimelineItem* item = ChooseItemToSelect( itemsAtPos );
		if( item != nullptr )
		{
			m_selectedItems.PushBack( item );
		}

		SelectionChanged();
	}

	UpdateMousePosition( event.GetX() );
	m_prevCursorTimePos = m_cursorTimePos;
}

void CEdTimeline::OnRightMouseDown( wxMouseEvent& event )
{
	m_lastRmbDownPos = event.GetPosition();
}

void CEdTimeline::OnRightMouseUp( wxMouseEvent& event )
{
	SetState( STATE_NORMAL );

	if( m_lastRmbDownPos != event.GetPosition() )
	{
		// This was not RMB click.
		return;
	}

	NotifyBufferIsInvalid();

	SelectItemAtMousePos( event );

	if( IsAnySelected() && IsItemMenuEnabled() )
	{
		ShowItemMenu();
		return;
	}

	if( m_selectedTrack != -1 )
	{
		Track* t = m_tracks[ m_selectedTrack ];

		if ( t->m_isGroup || event.GetX() < TIMELINE_TRACK_BTN_WIDTH )
		{
			if( IsTrackButtonMenuEnabled() )
			{
				ShowTrackButtonMenu( t );
			}
		}
		else
		{
			if( IsTrackMenuEnabled() )
			{
				ShowTrackMenu();
			}
		}
	}
	else
	{
		if( IsCanvasMenuEnabled() )
		{
			ShowCanvasMenu();
		}
	}
}

void CEdTimeline::OnMiddleMouseUp( wxMouseEvent& event )
{
	m_midlleMouseDown = false;
}

void CEdTimeline::OnMiddleMouseDown( wxMouseEvent& event )
{
	m_midlleMouseDown = true;
	SelectItemAtMousePos( event );
}

wxMenu* CEdTimeline::PrepareItemMenu()
{
	// TODO: This function doesn't seem to handle multiselection well
	// as it considers only the last selected item instead of all items.

	wxMenu* menu = new wxMenu();

	Bool addDefaults;
	ITimelineItem* lastSelectedItem = m_selectedItems[ m_selectedItems.Size() - 1 ];
	FillItemMenu( lastSelectedItem, menu, addDefaults );

	if( addDefaults && CanFillDefaultItemMenu() )
	{
		if( menu->GetMenuItemCount() > 0 )
		{
			menu->AppendSeparator();
		}

		if(lastSelectedItem->IsCopyable())
		{
			menu->Append( ID_COPY_ITEM, wxT( "Copy" ) );
		}

		if(lastSelectedItem->IsCopyable() && lastSelectedItem->IsRemovable())
		{
			menu->Append( ID_CUT_ITEM, wxT( "Cut" ) );
		}

		if(lastSelectedItem->IsRemovable())
		{
			menu->Append( ID_DELETE_ITEM, wxT( "Delete" ) );
		}
	}

	return menu;
}

void CEdTimeline::ShowItemMenu()
{
	if ( m_selectedItems.Empty() == true )
	{
		return;
	}

	wxMenu* menu = PrepareItemMenu();

	PopupMenu( menu );

	delete menu;
}

/*
Returns whether canvas menu is enabled.

This is a default implementation of this function. Derived classes may override this function.
*/
Bool CEdTimeline::IsCanvasMenuEnabled() const
{
	return true;
}

/*
Returns whether track menu is enabled.

This is a default implementation of this function. Derived classes may override this function.
*/
Bool CEdTimeline::IsTrackMenuEnabled() const
{
	return true;
}

/*
Returns whether track button menu is enabled.

This is a default implementation of this function. Derived classes may override this function.
*/
Bool CEdTimeline::IsTrackButtonMenuEnabled() const
{
	return true;
}

/*
Returns whether item menu is enabled.

This is a default implementation of this function. Derived classes may override this function.
*/
Bool CEdTimeline::IsItemMenuEnabled() const
{
	return true;
}

void CEdTimeline::ShowTrackMenu()
{
	wxMenu* menu = new wxMenu();

	if ( CanFillDefaultTrackMenu() )
	{
		if ( wxTheClipboard->Open() )
		{
			CClipboardData data( CLIPBOARD );
			if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
			{
				menu->Append( ID_PASTE_ITEM_MOUSE, wxT( "Paste" ) );
				menu->Append( ID_PASTE_ITEM_SPECIAL, wxT( "Paste special" ) );
				menu->AppendSeparator();
			}
			wxTheClipboard->Close();
		}
	}

	FillTrackMenu( m_tracks[ m_selectedTrack ]->m_name, menu );

	PopupMenu( menu );
	delete menu;
}

void CEdTimeline::ShowCanvasMenu()
{
	wxMenu* menu = new wxMenu();

	FillCanvasMenu( menu );

	if ( CanFillDefaultCanvasMenu() )
	{
		menu->Append( ID_NEW_TRACK, wxT( "Add new track" ) );
	}

	PopupMenu( menu );
	
	delete menu;
}

void CEdTimeline::OnLeftMouseDown( wxMouseEvent& event )
{
	NotifyBufferIsInvalid();
	SetFocus();

	if( ( event.ControlDown() && event.ShiftDown() ) || event.GetY() > GetSize().GetHeight() - m_timebarGroup->GetPreferredHeight() )
	{
		// Setting time
		SetState( STATE_TIME_SETTING );
		m_timeSettingTarget = TIME_SETTING_CURRENT_TIME;

		if ( m_timeLimitsEnabled && event.ControlDown() )
		{
			Float nd = Abs( CalculatePixelPos( m_timeLimitMin ) - CalculatePixelPos( m_cursorTimePos ) );
			if ( nd < 25 )
			{
				m_timeSettingTarget = TIME_SETTING_MIN_LIMIT;
			}
			nd = Abs( CalculatePixelPos( m_timeLimitMax ) - CalculatePixelPos( m_cursorTimePos ) );
			if ( nd < 25 )
			{
				m_timeSettingTarget = TIME_SETTING_MAX_LIMIT;
			}
		}
		if ( m_timeSettingTarget == TIME_SETTING_CURRENT_TIME )
		{
			RequestSetTime( m_cursorTimePos );
		}

		return;
	}

	// Click position
	wxPoint clickPosition( event.GetX(), event.GetY() );

	switch( m_state )
	{
	case STATE_NORMAL:
	case STATE_SELECTED:
		{
			wxPoint globalPos(event.GetX(), event.GetY());

			Track* t = GetTrackAt(globalPos);
			SelectTrack( GetTrackIndex(t) );

			if(t)
			{
				TimelineImpl::CDrawGroupTracks* drawGrp = GetTrackDrawGroup(t);
				wxPoint localPos = drawGrp->GetLocalPos(globalPos);

				// Check if selecting whole track
				if(localPos.x < TIMELINE_TRACK_BTN_WIDTH || IsTrackGroup(t))
				{
					// Other part of track button is clicked. Now, if we release LMB then
					// we will select all events on track (handled in OnLeftMouseUp()).
					// If we start moving the mouse then we're dragging the track (handled
					// in OnMouseMove()). For now we enter STATE_DRAG_TRACK_OR_SELECT_ALL_EVENTS_ON_TRACK
					m_state = STATE_DRAG_TRACK_OR_SELECT_ALL_EVENTS_ON_TRACK;
					break;
				}
			}

			TDynArray< ITimelineItem* > itemsAtPos;
			Uint32 numItemsAtPos = GetItemsAt(globalPos, itemsAtPos);
			if(numItemsAtPos == 0)
			{
				m_selectionStartPoint = event.GetPosition();
				m_state = STATE_SELECTING;
			}
			else // numItemsAtPos > 0
			{
				ITimelineItem* chosenItem = ChooseItemToSelect(itemsAtPos);

				if(chosenItem)
				{
					if( IsSelected( chosenItem ) )
					{
						// the item was already selected which means we may start dragging it
						m_state = STATE_DRAGGING;

						const Float cursorToLeaderInitialDist = CalculateTimePos( globalPos.x ) - chosenItem->GetStart();
						m_movingContext.Init( m_selectedItems, chosenItem, cursorToLeaderInitialDist, m_items );
					}
					else
					{
						if( ! event.ShiftDown() )
						{
							m_selectedItems.Clear();
						}
						m_selectedItems.PushBack( chosenItem );
					}

					OnItemSelected( chosenItem );
				}
				else if(CanUseSelectionItemMenu())
				{
					// we couldn't decide which event to choose - let the user make the choice
					ShowMenuChooseItemToSelect( itemsAtPos );
				}
			}

			SelectionChanged();
			break;
		}
	case STATE_SELECTED_HOVERING_EDGE:
		SetState( STATE_RESIZING );
		break;
	default:
		SetState( STATE_NORMAL );
		break;
	}

	if ( m_externalProperties == false )
	{
		// Hide properties page
		EditProperties( NULL, *m_propertiesPage );
		m_propertiesPage->Show( false );
		m_propertiesPage->SelectItem( nullptr );
	}
	else if ( IsOneSelected() == true )
	{
		EditProperties( m_selectedItems[ 0 ], *m_propertiesPage );
		m_propertiesPage->Show( true );
	}

	OnLeftMouseClick( event.GetX(), event.GetY() );
}

void CEdTimeline::OnItemSelected( ITimelineItem* chosenItem )
{
	// alt + lmb: current time = chosen item start time
	if( RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		// Setting current time to item start time should make effect of that item observable
		// (e.g. alt + lmb on camera item should change camera). Regrettably, to achieve this
		// we use the same ugly hack that's used by other code. TODO: do this properly.
		Float uglyHack = 0.001f;
		RequestSetTime( chosenItem->GetStart() + uglyHack );
	}
}

/*

\param itemsToMove List of items that are to be moved. Some items that are not on this list may also be moved if their position depends on position of items that are on this list.
\param snapTargets List of items denoting positions to which to snap. Both start positions and end positions are included.
*/
void CEdTimeline::MovingContext::Init( TDynArray< ITimelineItem* >& itemsToMove, ITimelineItem* leaderItem, Float cursorToLeaderInitialDist, const TDynArray< ITimelineItem* >& snapTargets )
{
	ASSERT( m_moveSet.Empty() );
	ASSERT( m_snapTargets.Empty() );

	auto compareStartTimes = [] ( const ITimelineItem* a, const ITimelineItem* b ) { return a->GetStart() < b->GetStart(); };

	// Create move set - list of items that should be shallow moved (it's not the same as itemsToMove).
	m_moveSet.Reserve( itemsToMove.Size() ); // approximate size
	for( Uint32 iItem = 0, numItems = itemsToMove.Size(); iItem < numItems; ++iItem )
	{
		itemsToMove[ iItem ]->GetMoveSet( m_moveSet );
	}
	Sort( m_moveSet.Begin(), m_moveSet.End(), compareStartTimes );

	m_leaderItem = leaderItem;
	m_cursorToLeaderInitialDist = cursorToLeaderInitialDist;

	m_snapPixelDistance = 20.0f;

	// Prepare list of snap targets.
	m_snapTargets = snapTargets;
	for( auto it = itemsToMove.Begin(), end = itemsToMove.End(); it != end; ++it )
	{
		m_snapTargets.RemoveFast( *it );
	}
	for( auto it = m_moveSet.Begin(), end = m_moveSet.End(); it != end; ++it )
	{
		m_snapTargets.RemoveFast( *it );
	}
	Sort( m_snapTargets.Begin(), m_snapTargets.End(), compareStartTimes );

	// Check if all items are movable.
	m_allItemsMovable = true;
	for( Uint32 iItem = 0, numItems = m_moveSet.Size(); iItem < numItems; ++iItem )
	{
		if( !m_moveSet[ iItem ]->IsMovable() )
		{
			m_allItemsMovable = false;
			break;
		}
	}
}

void CEdTimeline::MovingContext::Deinit()
{
	m_moveSet.ClearFast();

	m_leaderItem = nullptr;
	m_cursorToLeaderInitialDist = 0.0f;

	m_snapPixelDistance = 20.0f;
	m_snapTargets.ClearFast();

	m_allItemsMovable = false;
}

void CEdTimeline::OnLeftMouseDoubleClick( wxMouseEvent& event )
{
	NotifyBufferIsInvalid();
	m_keyboardModifiers	= event.GetModifiers();
	if( IsOneSelected() && m_selectedItems[ 0 ]->IsEditable() )
	{
		EditProperties( m_selectedItems[ 0 ], *m_propertiesPage );
		m_propertiesPage->Show( true );

		if ( m_externalProperties == false )
		{
			OnResize( wxSizeEvent() );
		}
	}
	else if( IsAnySelected() && m_selectedItems[ 0 ]->IsEditable() )
	{
		// Check if the same type
		Bool sameType = true;
		for( TDynArray< ITimelineItem* >::const_iterator itemIter = m_selectedItems.Begin();
			itemIter != m_selectedItems.End() - 1; ++itemIter )
		{
			ITimelineItem* currentItem = *itemIter;
			ITimelineItem* nextItem = *( itemIter + 1 );

			if( currentItem->GetTypeName() != nextItem->GetTypeName() )
			{
				sameType = false;
				break;
			}
		}

		if( sameType )
		{
			// We always edit [ 0 ] element's properties
			EditProperties( m_selectedItems[ 0 ], *m_propertiesPage );
			m_propertiesPage->Show( true );

			if ( m_externalProperties == false )
			{
				OnResize( wxSizeEvent() );
			}
		}
	}
	// If clicked in track button
	else if( m_selectedTrack >= 0 && event.GetX() < TIMELINE_TRACK_BTN_WIDTH )
	{
		OnTrackDoubleClicked();
	}

	OnLeftMouseDoubleClick( event.GetX(), event.GetY() );
	m_keyboardModifiers	= 0;
}

void CEdTimeline::OnStateSelected( wxPoint globalPos )
{
	Track* t = GetTrackAt( globalPos );
	if ( t )
	{
		TimelineImpl::CDrawGroupTracks* drawGrp = GetTrackDrawGroup( t );
		wxPoint localPos = drawGrp->GetLocalPos( globalPos );

		// Check if selecting whole track
		if( localPos.x < TIMELINE_TRACK_BTN_WIDTH )
		{
			// Select all events on track
			String trackName( t->m_name );
			m_selectedItems.Clear();
			for( TDynArray< ITimelineItem* >::iterator itemIter = m_items.Begin(); itemIter != m_items.End(); ++itemIter )
			{
				ITimelineItem* item = *itemIter;
				if( item->GetTrackName() == trackName )
				{
					m_selectedItems.PushBack( item );
				}
			}

			SelectionChanged();
		}
	}
}

void CEdTimeline::OnTrackDoubleClicked()
{
	OnRenameTrack( wxCommandEvent() );
}

void CEdTimeline::ShowMenuChooseItemToSelect( const TDynArray< ITimelineItem* >& candidates )
{
	if ( candidates.Size() > 1 )
	{
		wxMenu menu;

		Uint32 maxSize = Min< Uint32 >( candidates.Size(), ID_SEL_ITEM_LAST - ID_SEL_ITEM_FIRST );
		for ( Uint32 i=0; i<maxSize; ++i )
		{
			ITimelineItem* item = candidates[ i ];

			String label = item->GetTypeName();

			// include item tooltip in label
			String itemTooltip;
			item->GetTooltip( itemTooltip );
			if( !itemTooltip.Empty() )
			{
				label += TXT(": ") + itemTooltip;
			}

			menu.Append( ID_SEL_ITEM_FIRST + i, label.AsChar() );
			menu.Connect( ID_SEL_ITEM_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdTimeline::OnMenuSelectItem ), new ItemPopupWrapper( item ), this );
		}

		PopupMenu( &menu );
	}
}

/*
Chooses best item to select from list of possible candidates.

Obviously, this function is meant to be used in case of overlapping events.

Item is chosen as best candidate if:
1. it's the only candidate,
2. it's already selected,
3. it's the only point event among candidates.
*/
ITimelineItem* CEdTimeline::ChooseItemToSelect( const TDynArray< ITimelineItem* >& candidates )
{
	const Uint32 numCandidates = candidates.Size();
	ITimelineItem* bestCandidate = 0;

	if(numCandidates == 1)
	{
		// choose the only candidate
		bestCandidate = candidates[0];
	}
	else if(numCandidates > 1)
	{
		Uint32 numPointEvents = 0;

		for(Uint32 i = 0; i < numCandidates; ++i)
		{
			if(IsSelected(candidates[i]))
			{
				// choose candidate that's already selected
				bestCandidate = candidates[i];
				break;
			}

			if(!candidates[i]->IsDuration())
			{
				if(numPointEvents == 0)
				{
					// choose candidate if it's the only point event among all candidates
					bestCandidate = candidates[i];
				}
				else
				{
					// it turns out more than one candidate is a point event
					bestCandidate = 0;
				}
				++numPointEvents;
			}
		}
	}

	return bestCandidate;
}

void CEdTimeline::OnLeftMouseUp( wxMouseEvent& event )
{
	NotifyBufferIsInvalid();
	switch( m_state )
	{
	case STATE_DRAG_TRACK:
		m_state = STATE_NORMAL;
		break;
	case STATE_DRAG_TRACK_OR_SELECT_ALL_EVENTS_ON_TRACK:
		m_state = STATE_NORMAL;
		// fall through - we didn't move the mouse while holding LMB on track button
		// so we weren't dragging the track
	case STATE_NORMAL:
	case STATE_SELECTED:
		{
			wxPoint globalPos( event.GetX(), event.GetY() );
			OnStateSelected( globalPos );
			break;
		}
	case STATE_DRAGGING:
		if ( m_undoManager )
		{
			CUndoTimelineItemLayout::FinalizeStep( *m_undoManager );
		}
		m_movingContext.Deinit();
		SetState( STATE_SELECTED );
		break;
	case STATE_TIME_SETTING:
		SetState( STATE_NORMAL );
		break;
	case STATE_RESIZING:
		if ( m_undoManager )
		{
			CUndoTimelineItemLayout::FinalizeStep( *m_undoManager );
		}
		SetState( STATE_NORMAL );
		SetCursor( wxCURSOR_DEFAULT );
		break;
	case STATE_SELECTING:
		SetState( STATE_NORMAL );

		// get selection rectangle
		Int32 topLeftX = Min(m_selectionStartPoint.x, event.GetPosition().x);
		Int32 topLeftY = Min(m_selectionStartPoint.y, event.GetPosition().y);
		Int32 bottomRightX = Max(m_selectionStartPoint.x, event.GetPosition().x);
		Int32 bottomRightY = Max(m_selectionStartPoint.y, event.GetPosition().y);
		wxRect selRect(wxPoint(topLeftX, topLeftY), wxPoint(bottomRightX, bottomRightY));

		// get items in selection rectangle and select them
		m_selectedItems.Clear();
		GetItemsInRect(selRect, m_selectedItems);

		SelectionChanged();
	}
}

void CEdTimeline::RebuildFont()
{
	if ( m_itemFont )
	{
		delete m_itemFont;
	}

	Gdiplus::Font& f = GetGdiDrawFont();
	Gdiplus::FontFamily family;
	f.GetFamily( &family );
	Gdiplus::REAL newSize = f.GetSize() * ( GetVertScale() * 0.5 + 0.5 );
	m_itemFont = new Gdiplus::Font( &family, newSize, f.GetStyle(), f.GetUnit() );

	m_wxItemFont = GetWxDrawFont();
	m_wxItemFont.SetPointSize( m_wxItemFont.GetPointSize() * ( GetVertScale() * 0.5 + 0.5 ) );
}

void CEdTimeline::SetState( CEdTimeline::States newState )
{
	if ( m_state != newState )
	{
		States prevState = m_state;

		m_state = newState;

		OnStateChanged( prevState, newState );
	}
}

void CEdTimeline::OnStateChanged( CEdTimeline::States prevState, CEdTimeline::States newState )
{

}

void CEdTimeline::OnMouseWheel( wxMouseEvent& event )
{
	NotifyBufferIsInvalid();
	if ( event.ControlDown() )
	{
		// Vertical movement
		Int32 offset = event.GetWheelRotation()/3;

		if ( Abs( offset ) > 3 )
		{
			// Scroll
			m_verticalOffset += offset;
			m_defaultBuffer.SetVerticalOffset(m_verticalOffset);
			Refresh();
		}
	}
	else
	{
		if ( m_state != STATE_NORMAL && m_state != STATE_SELECTED )
		{
			return;
		}

		if ( event.AltDown() )
		{
			// Vertical zoom

			if ( event.GetWheelRotation() < 0 )
			{
				SetVertScale(GetVertScale() / 1.2f);
			}
			else
			{
				SetVertScale(GetVertScale() * 1.2f);
			}

			if ( GetVertScale() > 1.0f )
			{
				SetVertScale(1.0f);
			}
			else if ( GetVertScale() < 0.5f )
			{
				SetVertScale(0.5f);
			}

			RebuildFont();
			UpdateLayout();
		}
		else
		{
			// Horizontal zoom

			// We want to preserve m_cursorTimePos, i.e. cursor should point to the same time before
			// and after zooming as this is very convenient for the user. This equation will help us.
			Float cursorRelativePos = ( m_activeRangeTimeOffset + m_cursorTimePos ) / m_visibleRangeDuration;

			if ( event.GetWheelRotation() < 0 )
			{
				m_visibleRangeDuration *= 1.2f;
			}
			else
			{
				m_visibleRangeDuration /= 1.2f;
			}

			// Update active range time offset so cursor points to the same time as before zooming.
			m_activeRangeTimeOffset = cursorRelativePos * m_visibleRangeDuration - m_cursorTimePos;

			Float gridSpace = CalculatePixelPos( 2.0f * m_currentGrid / 16.0f ) - CalculatePixelPos( m_currentGrid / 16.0f );

			// Calculate new grid if necessary
			if ( gridSpace < 10 || gridSpace > 40 )
			{
				CalculateNewGrid();
			}

			Refresh();
		}
	}
}

void CEdTimeline::OnChar( wxKeyEvent& event )
{
	NotifyBufferIsInvalid();
	Int32 keyCode = event.GetKeyCode();

	// DEL
	if ( keyCode == WXK_DELETE )
	{
		wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED, ID_DELETE_ITEM);
		GetEventHandler()->ProcessEvent(ev);
		event.Skip();
	}
	else if ( event.ControlDown() )
	{
		// COPY
		if ( keyCode == WXK_CONTROL_C )
		{
			wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED, ID_COPY_ITEM);
			GetEventHandler()->ProcessEvent(ev);
		}
		// CUT
		else if ( keyCode == WXK_CONTROL_X )
		{
			wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED, ID_CUT_ITEM);
			GetEventHandler()->ProcessEvent(ev);
		}
		// PASTE SPECIAL
		else if ( event.ShiftDown() && keyCode == WXK_CONTROL_V )
		{
			wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED, ID_PASTE_ITEM_SPECIAL);
			GetEventHandler()->ProcessEvent(ev);
		}
		// PASTE
		else if ( keyCode == WXK_CONTROL_V )
		{
			wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED, ID_PASTE_ITEM_KEYBOARD);
			GetEventHandler()->ProcessEvent(ev);
		}
		// SELECT ALL
		else if( keyCode == WXK_CONTROL_A )
		{
			m_selectedItems = m_items;
			event.Skip();
		}
	}
}

void CEdTimeline::CenterPosition( Float time )
{
	m_activeRangeTimeOffset = m_visibleRangeDuration / 2 - time;
}

void CEdTimeline::SortTracks()
{
	Sort(m_tracks.Begin(),m_tracks.End(),GreaterTrack());
}

/*
Creates track object of appropriate dynamic type.

For CEdTimeline created track object is of Track class.
*/
Track* CEdTimeline::CreateTrack( String name, Uint32 depth, bool isGroup ) const
{
	return new Track( name, depth, isGroup );
}

/*
Adds track with specified name to timeline.

\param trackName Name of track to add. It may include group name (if such group doesn't exist then it will be created).

\return True - track added. False - track not added.
Possible causes of failure include:
1. Track with specified name already exists.
2. Track name includes group name but group name denotes existing track that is not a group.
*/
Bool CEdTimeline::AddTrack( const String& trackName )
{
	Track* track = CreateTrack( trackName, 0, false );

	Bool trackAdded = AddTrack( track );
	if( !trackAdded )
	{
		delete track;
	}

	return trackAdded;
}

/*
Adds specified track to timeline.

\param track Track that is to be added. Its name may include group name (if such group desn't exist then it will be created).

\return True - track added. False - track not added.
Possible causes of failure include:
1. Track with specified name already exists.
2. Track name includes group name but group name denotes existing track that is not a group.
*/
Bool CEdTimeline::AddTrack( Track* track )
{
	// track names have to be unique
	String trackName = track->m_name;
	for ( TDynArray< Track* >::iterator it = m_tracks.Begin(); it != m_tracks.End(); ++it )
	{
		if ( (*it)->m_name == trackName )
		{
			// track name is not unique
			return false;
		}
	}

	size_t dotPos;
	if ( trackName.FindCharacter( GROUP_SEPARATOR, dotPos ) )
	{
		// group name to which to add track
		String groupName = trackName.MidString( 0, dotPos );

		// Check if group with groupName exists. If it does - use it. If it doesn't - create it.
		// If track with groupName exists but is not a group then return - we can't use it.
		Track* group = 0;
		Int32 groupIndex = GetTrackIndex( groupName );
		if( groupIndex == -1 )
		{
			// create missing group
			group = CreateTrack( groupName, 0, true );
			m_tracks.PushBack( group );
			SortTracks();
			m_defaultGroup->AddTrack(group);
		}
		else
		{
			if( m_tracks[ groupIndex ]->m_isGroup )
			{
				// group exists - use it
				group = m_tracks[ groupIndex ];
			}
			else
			{
				// track name includes group name but group name denotes track that is not a group
				return false;
			}
		}

		// add track to group
		group->InsertTrack( track, this );
	}
	// standalone track
	else
	{
		m_tracks.PushBack( track );
		SortTracks();
		m_defaultGroup->AddTrack(track);
	}

	UpdateLayout();

	return true;
}

void CEdTimeline::OnNewTrack( wxCommandEvent& event )
{
	NotifyBufferIsInvalid();
	// Ask for track name
	String trackName;
	if ( ! InputBox( this, TXT( "Track name" ), TXT( "Enter track name" ), trackName ) )
	{
		return;
	}

	if( m_selectedTrack != -1 && m_tracks[ m_selectedTrack ]->m_isGroup )
	{
		// add group name so the track is created in selected track group
		String fullTrackName = m_tracks[ m_selectedTrack ]->m_name;
		fullTrackName += GROUP_SEPARATOR;
		fullTrackName += trackName;
		trackName = fullTrackName;
	}

	// Check if track name is unique
	if( GetTrackIndex( trackName ) >=0 )
	{
		wxMessageBox( TXT( "Track with this name already exists!" ) );
		return;
	}

	AddTrack( trackName );

	if ( m_undoManager )
	{
		CUndoTimelineTrackExistance::CreateCreationStep( *m_undoManager, this, trackName );
	}
}

void CEdTimeline::OnDeleteItem( wxCommandEvent& event )
{
	NotifyBufferIsInvalid();

	ASSERT( IsAnySelected() );

	// Hide properties page
	EditProperties( NULL, *m_propertiesPage );
	m_propertiesPage->Show( false );

	TDynArray< ITimelineItem* > removableItems;
	
	for ( Uint32 i=0; i<m_selectedItems.Size(); ++i )
	{
		if ( m_selectedItems[i]->IsRemovable() )
		{
			removableItems.PushBack( m_selectedItems[i] );
		}
	}

	if ( removableItems.Empty() && !m_selectedItems.Empty() )
	{
		wxMessageBox( TXT( "Selected events cannot be removed" ) );
		return;
	}

	// Ask if user is sure
	if( ! YesNo( TXT( "Do you really want to delete this event?" ) ) )
	{
		return;
	}

	// Delete all selected items
	for( TDynArray< ITimelineItem* >::iterator itemIter = removableItems.Begin();
		itemIter != removableItems.End(); ++itemIter )
	{
		ITimelineItem* item = *itemIter;

		if ( m_undoManager )
		{
			CUndoTimelineItemExistance::PrepareDeletionStep( *m_undoManager, this, item );
		}

		RemoveItem( item );
		delete item;
	}

	if ( m_undoManager )
	{
		CUndoTimelineItemExistance::FinalizeStep( *m_undoManager );
	}

	m_selectedItems.Clear();
	SelectionChanged();
}

void CEdTimeline::OnDeleteTrack( wxCommandEvent& event )
{
	NotifyBufferIsInvalid();
	ASSERT( m_selectedTrack < ( Int32 ) m_tracks.Size() );

	// Ask if user is sure
	if( ! YesNo( TXT( "Do you really want to delete track '%s' and *ALL* events on it ?" ),
		m_tracks[ m_selectedTrack ]->m_name.AsChar() ) )
	{
		return;
	}

	RemoveSelectedTrack();
}

void CEdTimeline::UpdateMousePosition( Int32 positionX )
{
	// Little hack for better mouse snapping
	Float a = CalculateTimePos( positionX );

	/*if( a < 0 )
	{
	a -= TIMELINE_SNAPPING / 2;
	}
	else
	{
	a += TIMELINE_SNAPPING / 2;
	}*/
	m_cursorTimePos = a; //Snap( a );
}

void CEdTimeline::ClearItems()
{
	SelectTrack( -1 );
	m_selectedItems.Clear();

	// Remove events
	for( TDynArray< ITimelineItem* >::iterator itemIter = m_items.Begin();
		itemIter != m_items.End(); ++itemIter )
	{
		delete *itemIter;
	}

	m_items.ClearFast();
}

void CEdTimeline::ShowTrackButtonMenu( Track* track )
{
	ASSERT( track );

	if ( !CanFillDefaultTrackButtonMenu() )
	{
		return;
	}

	wxMenu* menu = new wxMenu();

	// only track group tracks have "New track" menu item
	if( track->m_isGroup )
	{
		menu->Append( ID_NEW_TRACK, wxT( "New track" ) );
	}

	if( track->m_name != m_defaultTrackName || CanChangeDefaultTrack() )
	{
		menu->Append( ID_DELETE_TRACK, wxT( "Delete track" ) );
		menu->Append( ID_RENAME_TRACK, wxT( "Rename track" ) );
	}

	PopupMenu( menu );
}

void CEdTimeline::OnRenameTrack( wxCommandEvent& event)
{
	ASSERT( m_selectedTrack >= 0 && m_selectedTrack < ( Int32 ) m_tracks.Size() );

	if( m_tracks[ m_selectedTrack ]->m_name == m_defaultTrackName && ! CanChangeDefaultTrack() )
	{
		wxMessageBox( TXT( "You can't change name of this track" ) );
		return;
	}

	String oldTrackName = m_tracks[ m_selectedTrack ]->m_name;
	String newTrackName = oldTrackName;

	if( ! InputBox(this, TXT( "Track name" ), TXT( "Enter new track name:" ), newTrackName ) )
	{
		return;
	}
	if ( GetTrackIndex( newTrackName ) >= 0 )
	{
		return;
	}

	if ( m_undoManager )
	{
		CUndoTimelineTrackRename::CreateStep( *m_undoManager, this, oldTrackName, newTrackName );
	}

	Int32 index = GetTrackIndex( oldTrackName );
	if ( index >= 0 )
	{
		m_tracks[index]->Rename( this, newTrackName, true );
	}
	/*RenameTrack( oldTrackName, newTrackName );
	Sort(m_tracks.Begin(),m_tracks.End());*/
}

void CEdTimeline::RenameTrack( const String& oldTrackName , const String& newTrackName )
{
	// Check if track name is not busy
	if( GetTrackIndex( newTrackName ) >=0 )
	{
		wxMessageBox( TXT( "Track with this name already exists!" ) );
		return;
	}
	
	// Change track name
	for( TDynArray< Track* >::iterator trackIter = m_tracks.Begin();
		trackIter != m_tracks.End(); ++trackIter )
	{
		Track* track = *trackIter;

		if ( track->m_name == oldTrackName )
		{
			track->Rename( this, newTrackName, true );
			break;
		}
	}

	// Update all items' track name
	for( TDynArray< ITimelineItem* >::iterator itemIter = m_items.Begin();
		itemIter != m_items.End(); ++itemIter )
	{
		ITimelineItem* item = *itemIter;
		ASSERT( item != NULL );
		
		if( item->GetTrackName() == oldTrackName )
		{
			item->SetTrackName( newTrackName );
		}
	}
}

void CEdTimeline::OnMenuSelectItem( wxCommandEvent& event)
{
	if ( event.m_callbackUserData )
	{
		ItemPopupWrapper* wrapper = ( ItemPopupWrapper* ) event.m_callbackUserData;

		ITimelineItem* item = wrapper->m_item;

		m_selectedItems.Clear();
		m_selectedItems.PushBack( item );

		if( m_selectedItems[ 0 ]->IsEditable() )
		{
			EditProperties( m_selectedItems[ 0 ], *m_propertiesPage );
			m_propertiesPage->Show( true );

			if ( m_externalProperties == false )
			{
				OnResize( wxSizeEvent() );
			}
		}

		SelectionChanged();
	}
}

void CEdTimeline::SerializeItems( TDynArray< Uint8 >& buffer, const TDynArray< ITimelineItem* >& itemsToSerialize )
{
	CMemoryFileWriter writer( buffer );

	// Serialize number of events
	Uint32 numItems = itemsToSerialize.Size();
	writer << numItems;

	// Serialize items
	for( auto itemIter = itemsToSerialize.Begin(); itemIter != itemsToSerialize.End(); ++itemIter )
	{
		ITimelineItem* item = *itemIter;
		SerializeItem( item, writer );
	}
}

void CEdTimeline::DeserializeItems( const TDynArray< Uint8 >& buffer, TDynArray< ITimelineItem* >& deserializedItems )
{
	CMemoryFileReader reader( buffer, 0 );

	// Read number of items
	Uint32 numItems;
	reader << numItems;

	// Read items
	for( Uint32 i = 0; i<numItems; ++i )
	{
		// Read item
		ITimelineItem* item = DeserializeItem( reader );
		ASSERT( item != NULL );

		deserializedItems.PushBack( item );
		AddItem( item );
	}

	// Inform items of being pasted
	for( Uint32 i = 0; i < numItems; ++i )
	{
		deserializedItems[ i ]->OnPasted( deserializedItems );
	}
}

void CEdTimeline::CopyItems( const TDynArray< ITimelineItem* >& _itemsToCopy, Bool withCut )
{
	if ( ! wxTheClipboard->Open() )
	{
		return;
	}

	// Gather items to copy
	TDynArray< ITimelineItem* > itemsToCopy; 
	for( auto itemIter = _itemsToCopy.Begin(); itemIter != _itemsToCopy.End(); ++itemIter )
	{
		ITimelineItem* item = *itemIter;
		if ( item->IsCopyable() )
		{
			itemsToCopy.PushBack( item );

			// Include child items

			TDynArray< ITimelineItem* > childItems;
			item->GetChildItems( childItems );
			for ( auto it = childItems.Begin(), end = childItems.End(); it != end; ++it )
			{
				itemsToCopy.PushBack( *it );
			}
		}
	}

	s_copiedPositions.Clear();
	for ( auto item : itemsToCopy )
	{
		s_copiedPositions.PushBack( item->GetStart() );
	}

	// Serialize
	TDynArray< Uint8 > buffer;
	SerializeItems( buffer, itemsToCopy );

	if ( withCut )
	{
		// Cut
		wxCommandEvent fakeEvent; // TODO
		OnDeleteItem( fakeEvent );
	}

	wxTheClipboard->SetData( new CClipboardData( CLIPBOARD, buffer, !withCut ) );
	wxTheClipboard->Close();
}

void CEdTimeline::PasteItems( TDynArray< ITimelineItem* >& pastedItems, EPasteItemsMode mode, Float customTime, Bool pasteToSelectedTrack  )
{
	if (! wxTheClipboard->Open() )
	{
		return;
	}

	// get data from clipboard
	CClipboardData data( CLIPBOARD );
	Bool dataAvailable = wxTheClipboard->GetData( data );
	wxTheClipboard->Close();

	if( !dataAvailable )
	{
		// no data to paste
		return;
	}

	DeserializeItems( data.GetData(), pastedItems );

	String refTrack = String::EMPTY;
	Bool allItemsFromOneTrack = true;
	for( Uint32 i=0; i<pastedItems.Size(); ++i )
	{
		if ( i == 0 )
		{
			refTrack = pastedItems[i]->GetTrackName();
		}
		else
		{
			if ( pastedItems[ i ]->GetTrackName() != refTrack )
			{
				allItemsFromOneTrack = false;
				break;
			}
		}
	}

	// If pasting on the track, use its name instead
	if ( pasteToSelectedTrack && allItemsFromOneTrack && m_selectedTrack >= 0 )
	{
		for( Uint32 i=0; i<pastedItems.Size(); ++i )
		{
			pastedItems[ i ]->SetTrackName( m_tracks[ m_selectedTrack ]->m_name );
		}
	}

	// Special case for single event pasting
	if ( pastedItems.Size() == 1 )
	{
		Float pos = pastedItems[0]->GetStart(); // default paste operation: keyboard - don't move event anywhere, just put it where it was

		// if we're using "paste special" then preserve item position
		if( mode == PIM_Special )
		{
			pos = pastedItems[0]->GetStart();
		}
		// if we're pasting using the mouse then place the item at mouse position
		else if( mode == PIM_Mouse )
		{
			pos = m_cursorTimePos;
		}
		else if( mode == PIM_CustomTime || mode == PIM_AllToOneCustomTime )
		{
			pos = customTime;
		}
		else
		{
			ASSERT( mode == PIM_Default );
		}

		pastedItems[0]->SetStart( pos, true );
	}
	else if ( mode == PIM_AllToOneCustomTime )
	{
		for( Uint32 i = 0; i < pastedItems.Size(); ++i )
		{
			pastedItems[i]->SetStart( customTime, true );
		}
	}
	else if ( !s_copiedPositions.Empty() )
	{	
		Float minStart = s_copiedPositions[0];
		for ( Uint32 i = 1; i < s_copiedPositions.Size(); ++i )
		{
			minStart = Min( minStart, s_copiedPositions[i] );
		}

		Float pos = mode == PIM_CustomTime ? customTime : m_cursorTimePos;
		pos -= minStart;

		Uint32 itemInd = 0;
		for ( ITimelineItem* item : pastedItems )
		{
			Float newPos = pos + s_copiedPositions[itemInd++];
			Float actualpos = item->SetStart( Clamp( newPos, m_timeLimitMin, m_timeLimitMax * 0.99f ), true );
		}
	}

	if ( m_undoManager )
	{
		for( Uint32 i = 0; i < pastedItems.Size(); ++i )
		{
			CUndoTimelineItemExistance::PrepareCreationStep( *m_undoManager, this, pastedItems[i] );
		}

		CUndoTimelineItemExistance::FinalizeStep( *m_undoManager );
	}

	if( ! data.IsCopy() )
	{
		// Set clipboard to copy mode
		Bool ok = wxTheClipboard->Open();
		ASSERT( ok );
		wxTheClipboard->SetData( new CClipboardData( CLIPBOARD, data.GetData(), true ) );
		wxTheClipboard->Close();
	}

	// RecreateTracks() should not be called in OnPaste() so I removed it. However, it was here for a reason
	// so I had to restore it. The reason is that if we copy an item between two timelnies and destination
	// timeline doesn't have a track whose name is the same as source track then the item doesn't appear
	// until RecreateTracks() is called. This is not how it should be done. TODO: do it properly.
	RecreateTracks();
}

void CEdTimeline::OnPaste( wxCommandEvent& event )
{
	EPasteItemsMode mode( PIM_Default );

	if ( event.GetId() == ID_PASTE_ITEM_SPECIAL )
	{
		mode = PIM_Special;
	}
	else if ( event.GetId() == ID_PASTE_ITEM_MOUSE )
	{
		mode = PIM_Mouse;
	}

	StoreLayout();

	TDynArray< ITimelineItem* > pastedItems;
	PasteItems( pastedItems, mode );

	RestoreLayout();
}

void CEdTimeline::OnItemPropertyChanged( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );

	if ( CProperty* property = eventData->m_typedObject.m_class->FindProperty( eventData->m_propertyName ) )
	{
		if( m_selectedItems.Size() > 1 )
		{
			// We always edit [ 0 ] element's properties

			// Propagate to rest of selected events
			for( Uint32 i = 1; i < m_selectedItems.Size(); ++i )
			{
				m_selectedItems[ i ]->SetProperty( property, m_selectedItems[ 0 ] );
			}
		}

		ItemPropertyChanged( m_selectedItems, property->GetName() );
	}
}

void CEdTimeline::SetDurationEventLength( ITimelineItem* item )
{
	if( ! item->IsDuration() )
	{
		return;
	}

	m_selectedItems.ClearFast();
	// select item
	m_selectedItems.PushBackUnique( item );

	SetState( STATE_RESIZING );
	
	m_startEdgeGrabbed = false;
	m_cursorTimePos = item->GetStart() + item->GetDuration();
}

void CEdTimeline::RemoveSelectedTrack()
{
	if ( m_undoManager )
	{
		CUndoTimelineTrackExistance::CreateDeletionStep( *m_undoManager, this, m_tracks[m_selectedTrack]->m_name );
	}

	m_tracks[ m_selectedTrack ]->Delete( this );

	SelectTrack( -1 );
}

Int32 CEdTimeline::GetItemCount() const
{
	return m_items.Size();
}

ITimelineItem* CEdTimeline::GetItem( Int32 n ) const
{
	return m_items[n];
}

Int32 CEdTimeline::GetSelectedItemCount() const
{
	return m_selectedItems.Size();
}

ITimelineItem* CEdTimeline::GetSelectedItem( Int32 n ) const
{
	return m_selectedItems[n];
}

Bool CEdTimeline::IsSelected( const ITimelineItem* item )
{
	return m_selectedItems.Exist( const_cast< ITimelineItem* >( item ) );
}

Bool CEdTimeline::IsOnlySelected( const ITimelineItem* item )
{
	return IsOneSelected() && IsSelected( item );
}

Bool CEdTimeline::IsAnySelected()
{
	return m_selectedItems.Size() > 0;
}

Bool CEdTimeline::IsOneSelected()
{
	return m_selectedItems.Size() == 1;
}

void CEdTimeline::SetSelection( TDynArray< ITimelineItem* >& items, Bool goToEvent )
{
	if ( m_isSetSelectionLocked )
	{
		return;
	}

	m_selectedItems = items;
}

void CEdTimeline::SelectTrack( Int32 track )
{
	m_selectedTrack = track;

	OnTrackSelectionChanged();
}

void CEdTimeline::SetTimeLimits( Float minTime, Float maxTime )
{
	m_timeLimitMin = Max( 0.0f, minTime );
	m_timeLimitMax = Min( m_activeRangeDuration, maxTime );
	m_timeLimitsEnabled = true;
}

void CEdTimeline::SetTimeLimitMin( Float minTime )
{
	m_timeLimitMin = Max( 0.0f, minTime );
}

void CEdTimeline::SetTimeLimitMax( Float maxTime )
{
	m_timeLimitMax = Min( m_activeRangeDuration, maxTime );
}

void CEdTimeline::GetTimeLimits( Float& outMinTime, Float& outMaxTime ) const
{
	outMinTime = m_timeLimitMin;
	outMaxTime = m_timeLimitMax;
}

Int32 CEdTimeline::GetTrackIndex( const String& trackName ) const
{
	for ( Uint32 i = 0; i < m_tracks.Size(); ++i )
	{
		if ( m_tracks[i]->m_name == trackName )
		{
			return i;
		}
	}
	return -1;
}

Int32 CEdTimeline::GetTrackIndex( Track* track ) const
{
	for ( Uint32 i = 0; i < m_tracks.Size(); ++i )
	{
		if ( m_tracks[i] == track)
		{
			return i;
		}
	}
	return -1;
}

Float CEdTimeline::ResizeItem( ITimelineItem* item, Float duration )
{
	return item->SetDuration( duration );
}

Int32 CEdTimeline::GetKeyboardModifiers() const
{
	return m_keyboardModifiers;
}

void CEdTimeline::SchedulePropertiesRefresh()
{
	if ( m_propertiesPage == NULL )
	{
		return;
	}

	RunLaterOnce( [ this ](){ m_propertiesPage->RefreshValues(); } );
}


void CEdTimeline::RefreshPropPageValues()
{
	if ( m_propertiesPage && m_propertiesPage->IsShown() )
	{
		m_propertiesPage->RefreshValues();
	}
}
/*
\param Track to be pinned. It's ok if it's already pinned.
*/
void CEdTimeline::PinTrack( Track* t )
{
	if(!IsTrackPinned(t))
	{
		Uint32 indexInDefGroup = m_defaultGroup->GetTrackIndex(t);
		Uint32 count = CountTracksInHierarchy(t);
		for(Uint32 i = 0; i < count; ++i)
		{
			m_pinnedGroup->AddTrack(m_defaultGroup->GetTrack(indexInDefGroup));
			m_defaultGroup->RemoveTrack(m_defaultGroup->GetTrack(indexInDefGroup));
		}
	}
}

void CEdTimeline::PinTrack( const String& t )
{
	for ( Uint32 i = 0; i < m_tracks.Size(); ++i )
	{
		if ( m_tracks[i]->m_name == t )
		{
			PinTrack( m_tracks[i] );
			return;
		}
	}
}

/*

\param Track to unpin. It's ok if it's already unpinned.
*/
void CEdTimeline::UnpinTrack( Track* t )
{
	if(IsTrackPinned(t))
	{
		Uint32 indexInTopGroup = m_pinnedGroup->GetTrackIndex(t);
		Uint32 count = CountTracksInHierarchy(t);
		for(Uint32 i = 0; i < count; ++i)
		{
			m_defaultGroup->AddTrack(m_pinnedGroup->GetTrack(indexInTopGroup));
			m_pinnedGroup->RemoveTrack(m_pinnedGroup->GetTrack(indexInTopGroup));
		}
	}
}

void CEdTimeline::ExpandTrackGroup( Track* trackGroup )
{
	TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( trackGroup );
	drwGrp->ExpandTrackGroup( trackGroup );
}

void CEdTimeline::CollapseTrackGroup( Track* trackGroup )
{
	TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( trackGroup );
	drwGrp->CollapseTrackGroup( trackGroup );
}

void CEdTimeline::ShowTrack( Track* track )
{
	TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( track );
	drwGrp->ShowTrack( track );
}

void CEdTimeline::HideTrack( Track* track )
{
	TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( track );
	drwGrp->HideTrack( track );
}

/*
Returns number of tracks represented by specified track.

This function counts the track that was passed as argument and all its children. If any of children is a track
group then all child's children are also counted (and so on).
*/
Uint32 CEdTimeline::CountTracksInHierarchy( Track* t )
{
	Uint32 ret = 1;

	if(IsTrackGroup(t))
	{
		Uint32 numChildren = t->m_children.Size();
		for(Uint32 i = 0; i < numChildren; ++i)
		{
			ret += CountTracksInHierarchy(t->m_children[i]);
		}
	}

	return ret;
}

void CEdTimeline::NotifyBufferIsInvalid()
{
	m_pinnedBuffer.SetValid(false);
	m_defaultBuffer.SetValid(false);
	m_timebarBuffer.SetValid(false);
	m_volatileBuffer.SetValid(false);
}

void CEdTimeline::CollectItemsAt( Float time, TDynArray<ITimelineItem*>& items, Float timeEps )
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		ITimelineItem* item = m_items[ i ];
		if ( item && MAbs( item->GetStart() - time ) < timeEps )
		{
			items.PushBack( item );
		}
	}
}

/*
Gets track items.

\param track Track which items are to be found.
\param trackItems (out) Container where track items are to be stored. Container is not cleared before use.
\return Number of track items (number of items pushed to item container).
*/
size_t CEdTimeline::GetTrackItems(Track* track, TDynArray<ITimelineItem*>& trackItems) const
{
	size_t numTrackItems = 0;

	for(TDynArray<ITimelineItem*>::const_iterator itItem = m_items.Begin(); itItem != m_items.End(); ++itItem)
	{
		ITimelineItem* item = *itItem;
		if(item)
		{
			String itemTrackName = item->GetTrackName().Empty()? m_defaultTrackName : item->GetTrackName();
			if(itemTrackName == track->m_name)
			{
				// item belongs to this track
				trackItems.PushBack(item);
				++numTrackItems;
			}
		}
	}

	return numTrackItems;
}

/*
Gets items in rectangle.

\param globalRect Rectangle in global space.
\param itemsInRect (out) Container where items in rectangle are to be stored. Container is not cleared before use.
\return Number of items in rectangle (number of items pushed back to container).
*/
size_t CEdTimeline::GetItemsInRect( const wxRect& globalRect, TDynArray<ITimelineItem*>& itemsInRect ) const
{
	size_t numItems = m_pinnedGroup->GetItemsInRect(m_pinnedGroup->GetLocalRect(globalRect), itemsInRect);
	numItems += m_defaultGroup->GetItemsInRect(m_defaultGroup->GetLocalRect(globalRect), itemsInRect);

	return numItems;
}

void CEdTimeline::OnLeftMouseClick( Int32 x, Int32 y )
{
	wxPoint globalPos(x, y);

	Track* t = GetTrackAt(globalPos);

	if(t)
	{
		TimelineImpl::CDrawGroupTracks* drawGrp = GetTrackDrawGroup(t);
		drawGrp->OnLeftMouseClick(drawGrp->GetLocalPos(globalPos));
	}
}

Track* CEdTimeline::GetItemTrack( const ITimelineItem* item ) const
{
	ASSERT( item );

	String trackName = item->GetTrackName().Empty()? m_defaultTrackName : item->GetTrackName();
	return m_tracks[ GetTrackIndex( trackName ) ];
}

/*
Returns item global position.

\param item Item whose global position to return. Must not be nullptr.
\return Item global position, i.e. position in CEdTimeline space.
*/
wxPoint CEdTimeline::GetItemGlobalPosition( ITimelineItem* item ) const
{
	ASSERT( item );

	Track* track = GetItemTrack( item );
	TimelineImpl::CDrawGroupTracks* drawGrp = GetTrackDrawGroup( track );

	wxPoint trackLocalPos = drawGrp->GetDispTrackLocalPos( track );
	Int32 trackHeight  = drawGrp->GetTrackHeight( track );
	Int32 itemHeight   = (Int32) ( (Float) drawGrp->GetItemHeight( track ) * item->GetHeightScale() );

	wxPoint drawBufOrigin;
	drawGrp->GetDrawBuffer()->GetTargetPos( drawBufOrigin );

	Int32 itemPosX = drawBufOrigin.x + CalculatePixelPos( item->GetStart() );
	Int32 itemPosY = drawBufOrigin.y + trackLocalPos.y + trackHeight - itemHeight - 2;

	return wxPoint( itemPosX, itemPosY );
}

/*
Adds item to timeline.

\param item Item to add. Must not be nullptr. Timeline acquires ownership of the item.
*/
void CEdTimeline::AddItem( ITimelineItem* item )
{
	ASSERT( item );

	m_items.PushBack( item );

	// Invalidate appropriate draw buffer. Extra code is needed because track with given name may not exist.
	String trackName = item->GetTrackName().Empty()? m_defaultTrackName : item->GetTrackName();
	Int32 trackIndex = GetTrackIndex( trackName );
	if( trackIndex != -1 )
	{
		GetTrackDrawGroup( m_tracks[ trackIndex ] )->GetDrawBuffer()->SetValid( false );
	}
}

void CEdTimeline::RemoveItem( ITimelineItem* item )
{
	RemoveItemImpl( item );

	// Get draw buffer that is to be invalidated after item is removed.
	// Extra code is needed because track with given name may not exist.
	TimelineImpl::CDrawBuffer* bufferToInvalidate = nullptr;
	String trackName = item->GetTrackName().Empty()? m_defaultTrackName : item->GetTrackName();
	Int32 trackIndex = GetTrackIndex( trackName );
	if( trackIndex != -1 )
	{
		bufferToInvalidate = GetTrackDrawGroup( m_tracks[ trackIndex ] )->GetDrawBuffer();
	}

	m_items.RemoveFast( item );

	// Invalidate appropriate draw buffer.
	if( bufferToInvalidate )
	{
		bufferToInvalidate->SetValid( false );
	}
}

/*
Applies layout.

\param layout Layout that is to be applied.
*/
void CEdTimeline::ApplyLayout( const CTimelineLayout& layout )
{
	// handle tracks if layout contains track info
	if( !layout.trackInfo.Empty())
	{

		// Add tracks from layout one by one. Note that layout may reference tracks
		// that no longer exist (e.g. user deleted all lines spoken by an actor so
		// all tracks for that actor were removed) - such tracks are skipped.
		for( Uint32 iTrack = 0, numTracks = layout.trackInfo.Size(); iTrack < numTracks; ++iTrack )
		{
			CTimelineLayout::STrackInfo trackInfo = layout.trackInfo[ iTrack ];
			Int32 trackIndex = GetTrackIndex( trackInfo.m_name );
			if( trackIndex != -1 )
			{
				// set up data
				Track* track = m_tracks[ trackIndex ];
				track->m_isExpanded = trackInfo.m_isExpanded;
				track->m_isHidden = trackInfo.m_isHidden;

				// Remove track from its current draw group and add it to draw group specified
				// by layout (this also ensures that tracks are in the same order as in layout).
				GetTrackDrawGroup( track )->RemoveTrack( track );
				if( trackInfo.m_isPinned )
				{
					GetDrawGroupTracksPinned()->AddTrack( track );
				}
				else
				{
					GetDrawGroupTracksDefault()->AddTrack( track );
				}
			}
		}
	}

	m_verticalOffset = layout.m_verticalOffset;
	m_defaultBuffer.SetVerticalOffset( m_verticalOffset );

	m_visibleRangeDuration = layout.m_visibleRangeDuration;
	m_activeRangeTimeOffset = layout.m_activeRangeTimeOffset;

	m_currentGrid = layout.m_currentGrid;

	m_yScale = layout.m_vertScale;
	RebuildFont();
}

/*
Gets current layout.
*/
void CEdTimeline::GetLayout( CTimelineLayout& outLayout )
{
	TimelineImpl::CDrawGroupTracks* drwGrpPinned = GetDrawGroupTracksPinned();
	for( Uint32 iTrack = 0, numTracks = drwGrpPinned->GetNumTracks(); iTrack < numTracks; ++iTrack )
	{
		const Track* track = drwGrpPinned->GetTrack( iTrack );

		CTimelineLayout::STrackInfo trackInfo;
		trackInfo.m_name = track->m_name;
		trackInfo.m_isExpanded = track->m_isExpanded;
		trackInfo.m_isHidden = track->m_isHidden;
		trackInfo.m_isPinned = true;

		outLayout.trackInfo.PushBack( trackInfo );
	}

	TimelineImpl::CDrawGroupTracks* drwGrpDefault = GetDrawGroupTracksDefault();
	for( Uint32 iTrack = 0, numTracks = drwGrpDefault->GetNumTracks(); iTrack < numTracks; ++iTrack )
	{
		const Track* track = drwGrpDefault->GetTrack( iTrack );

		CTimelineLayout::STrackInfo trackInfo;
		trackInfo.m_name = track->m_name;
		trackInfo.m_isExpanded = track->m_isExpanded;
		trackInfo.m_isHidden = track->m_isHidden;
		trackInfo.m_isPinned = false;

		outLayout.trackInfo.PushBack( trackInfo );
	}

	outLayout.m_verticalOffset = m_verticalOffset;
	outLayout.m_visibleRangeDuration = m_visibleRangeDuration;
	outLayout.m_activeRangeTimeOffset = m_activeRangeTimeOffset;
	outLayout.m_currentGrid = m_currentGrid;
	outLayout.m_vertScale = m_yScale;
}

void CEdTimeline::OnEditCopy( wxCommandEvent& event )
{
	if( IsAnySelected() )
	{
		CopyItems( m_selectedItems, false );
	}
}

void CEdTimeline::OnEditCut( wxCommandEvent& event )
{
	if( IsAnySelected() )
	{
		CopyItems( m_selectedItems, true );
	}
}

void CEdTimeline::OnEditPaste( wxCommandEvent& event )
{
	StoreLayout();

	TDynArray< ITimelineItem* > pastedItems;
	PasteItems( pastedItems, PIM_CustomTime, m_timeSelectorTimePos, false );

	RestoreLayout();
}

/*
Default implementation - it reflects what used to be done till now.

Eventually it will be removed and each item will handle left edge reposition by itself.
This function is here so we can do this gradually..
*/
void CEdTimeline::HandleRepositionLeftEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers )
{
	Bool snappingEnabled = keyModifiers.shift;
	if( snappingEnabled )
	{
		newTimePos = Snap( newTimePos );
	}

	Float durOffset = item->GetStart() - newTimePos;

	item->SetStart( newTimePos, true );
	ResizeItem( item, item->GetDuration() + durOffset );
}

/*
Default implementation - it reflects what used to be done till now.

Eventually it will be removed and each item will handle left edge reposition by itself.
This function is here so we can do this gradually..
*/
void CEdTimeline::HandleRepositionRightEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers )
{
	if( item->GetStart() + item->GetDuration() > m_activeRangeDuration )
	{
		// Allow overlapping when Shift is pressed
		Bool allowOverlapping = keyModifiers.shift;
		if( allowOverlapping )
		{
			Float newPos = m_cursorTimePos;

			// Draging second part
			Float offset = newPos - m_prevCursorTimePos;

			if( m_cursorTimePos <= 0 )
			{
				offset = 0;
			}

			ResizeItem( item, item->GetDuration() + offset );
		}
		else
		{
			ResizeItem( item, m_activeRangeDuration - item->GetStart() );
		}
	}
	else // right edge is not wrapped
	{
		Bool snappingEnabled = keyModifiers.shift;

		Float newPos;

		if( CanResizeOverEnd() )
		{
			newPos = Max( snappingEnabled? Snap( m_cursorTimePos ) : m_cursorTimePos, item->GetStart() );
		}
		else
		{
			newPos = Clamp( snappingEnabled? Snap( m_cursorTimePos ) : m_cursorTimePos, item->GetStart() + 0.01f/*to avoid 0 duration*/, m_activeRangeDuration );
		}

		Float newDur = newPos - item->GetStart();

		ResizeItem( item, newDur );
	}
}

/*
Ctor.
*/
CEdTimeline::MovingContext::MovingContext()
: m_leaderItem( nullptr )
, m_cursorToLeaderInitialDist( 0.0f )
, m_snapPixelDistance( 20.0f )
, m_allItemsMovable( false )
{}
