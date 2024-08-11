/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "iconGrid.h"
#include "../../common/core/feedback.h"

IMPLEMENT_DYNAMIC_CLASS( CEdIconGrid, wxControl )

BEGIN_EVENT_TABLE( CEdIconGrid, wxControl )
	EVT_PAINT( CEdIconGrid::OnPaint )
	EVT_SIZE( CEdIconGrid::OnSize )
	EVT_SCROLLWIN( CEdIconGrid::OnScrollWin )
	EVT_MOTION( CEdIconGrid::OnMotion )
	EVT_MOUSEWHEEL( CEdIconGrid::OnMouseWheel )
	EVT_LEFT_DOWN( CEdIconGrid::OnMouseButtonDown )
	EVT_MIDDLE_DOWN( CEdIconGrid::OnMouseButtonDown )
	EVT_RIGHT_DOWN( CEdIconGrid::OnMouseButtonDown )
	EVT_KEY_DOWN( CEdIconGrid::OnKeyDownEvent )
END_EVENT_TABLE()

#define M_ENTRY ((CEdIconGrid::Entry*)m_entry)

Int32 CEdIconGridEntryInfo::GetIndex() const
{
	return M_ENTRY->index;
}

wxPoint CEdIconGridEntryInfo::GetPosition() const
{
	return wxPoint( M_ENTRY->x, M_ENTRY->y );
}

wxSize CEdIconGridEntryInfo::GetSize() const
{
	return m_grid->GetEntrySize( M_ENTRY );
}

Bool CEdIconGridEntryInfo::IsSelected() const
{
	return m_grid->GetAllSelected().Exist( M_ENTRY->index ) || ( m_grid->GetSecondary() == M_ENTRY->index && m_grid->GetAllowSecondary() );
}

CEdIconGrid::CEdIconGrid()
{
	Init();
}

CEdIconGrid::CEdIconGrid( wxWindow* parent, Int32 style )
	: wxControl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS )
{
	Init();
}

CEdIconGrid::~CEdIconGrid()
{
	Clear();
}

void CEdIconGrid::Init()
{
	m_allowSecondary = false;
	m_originalThumbnailSize = m_thumbnailSize = 192;
	m_middleClickSelectsBoth = true;
	m_allowMultiSelection = false;
	Clear();
	m_hook = NULL;
	SetDoubleBuffered( true );
}

void CEdIconGrid::AdjustScrollbars()
{
	wxRect size( GetClientSize() );
	SetScrollbar( wxVERTICAL, m_scroll, size.GetHeight(), m_totalInnerHeight );
}

void CEdIconGrid::SetScroll( Int32 scroll )
{
	if ( m_scroll == scroll ) return;

	wxRect size( GetClientSize() );
	if ( scroll < 0 )
	{
		scroll = 0;
	}
	else if ( scroll > m_totalInnerHeight - size.GetHeight() )
	{
		scroll = max( 0, m_totalInnerHeight - size.GetHeight() );
	}

	m_scroll = scroll;
	SetScrollPos( wxVERTICAL, scroll, true );
	Refresh( false );
}

void CEdIconGrid::MakePointVisible( Int32 x, Int32 y )
{
	wxSize size = GetClientSize();
	UNREFERENCED_PARAMETER(x);

	y -= m_scroll;

	if ( y < 0 )
	{
		SetScroll( m_scroll + y );
	}
	else if ( y >= size.GetHeight() )
	{
		SetScroll( m_scroll + ( y - size.GetHeight() + 1 ) );
	}
}

void CEdIconGrid::DoSetSelected( const TDynArray<Int32>& indices, bool add, bool secondary )
{
	if ( !secondary && !add ) m_selected.Clear();

	for ( Uint32 i=0; i<indices.Size(); ++i )
	{
		Int32 index = indices[i];
		if ( index >= (Int32)m_entries.Size() ) continue;
		if ( secondary )
		{
			m_secondary = index;
		}
		else
		{
			if ( index >= 0 ) m_selected.PushBack( index );
		}
		if ( index > -1 )
		{
			Entry* e = m_entries[index];
			wxSize entrySize = GetEntrySize( e );
			MakePointVisible( e->x, e->y - 10 );
			MakePointVisible( e->x, e->y + entrySize.GetHeight() + 10 );
		}
	}
	if ( m_hook )
	{
		m_hook->OnIconGridSelectionChange( this, !secondary );
	}
	Refresh( false );
}

void CEdIconGrid::DoSetSelected( Int32 index, bool add, bool secondary )
{
	TDynArray<Int32> single( 1 );
	single[0] = index;
	m_lastSelected = index;
	DoSetSelected( single, add, secondary );
}

HBITMAP CEdIconGrid::GetThumbnailForEntry( Entry* entry )
{
	Int32 visibleThumbnailSize = floor( m_thumbnailSize );

	// Recreate thumbnail bitmap if the thumbnail size has changed
	if ( entry->thumbBmp && entry->usedThumbnailSize != visibleThumbnailSize )
	{
		::DeleteObject( entry->thumbBmp );
		entry->thumbBmp = NULL;
	}

	// If there is no thumbnail bitmap (or was invalidated above) create it
	if ( !entry->thumbBmp )
	{
		HBITMAP fullBmp, bmp, prevBmp1, prevBmp2;
		HDC deskdc = ::GetDC( GetDesktopWindow() );
		HDC tmpdc1 = CreateCompatibleDC( deskdc );
		HDC tmpdc2 = CreateCompatibleDC( deskdc );
		::ReleaseDC( GetDesktopWindow(), deskdc );
		entry->thumb->GetBitmap()->GetHBITMAP( Gdiplus::Color::Black, &fullBmp );
		prevBmp1 = (HBITMAP)::SelectObject( tmpdc1, (HGDIOBJ)fullBmp );
		bmp = CreateCompatibleBitmap( tmpdc1, visibleThumbnailSize, visibleThumbnailSize );
		prevBmp2 = (HBITMAP)::SelectObject( tmpdc2, (HGDIOBJ)bmp );
		entry->usedThumbnailSize = visibleThumbnailSize;
		::SetStretchBltMode( tmpdc2, HALFTONE );
		::SetBrushOrgEx( tmpdc2, 0, 0, NULL );
		::StretchBlt( tmpdc2, 0, 0, visibleThumbnailSize, visibleThumbnailSize, tmpdc1, 0, 0, entry->thumb->GetWidth(), entry->thumb->GetHeight(), SRCCOPY );
		::SelectObject( tmpdc2, (HGDIOBJ)prevBmp2 );
		::SelectObject( tmpdc1, (HGDIOBJ)prevBmp1 );
		::DeleteDC( tmpdc2 );
		::DeleteDC( tmpdc1 );
		::DeleteObject( fullBmp );
		entry->thumbBmp = bmp;
	}
	return entry->thumbBmp;
}

wxSize CEdIconGrid::GetEntrySize( Entry* entry ) const
{
	wxSize& textSize = entry->captionSize;
	if ( !textSize.GetWidth() || !textSize.GetHeight() )
	{
		wxScreenDC dc;
		if ( entry->etype == ET_SEPARATOR)
		{
			dc.SetFont( GetFont().Larger() );
		}
		entry->captionSize = dc.GetTextExtent( entry->caption );
	}

	if ( entry->etype == ET_ENTRY )
	{
		Int32 visibleThumbnailSize = floor( m_thumbnailSize );
		wxSize bitmapSize = wxSize( visibleThumbnailSize, visibleThumbnailSize );
		return wxSize( bitmapSize.GetWidth(), bitmapSize.GetHeight() + textSize.GetHeight() + 5 );
	}
	else if ( entry->etype == ET_SEPARATOR )
	{
		wxSize size = GetClientSize();
		return wxSize( size.GetWidth() - 20, entry->captionSize.GetHeight() + 10 );
	}

	return wxSize( 0, 0 );
}

void CEdIconGrid::DoAddEntry( Entry* entry )
{
	entry->group = m_group;
	m_entries.PushBack( entry );
	EntryLayout();
}

void CEdIconGrid::DoAddEntries( const TDynArray<Entry*>& entries )
{
	for ( Uint32 i=0; i<entries.Size(); ++i ) entries[i]->group = m_group;
	m_entries.PushBack( entries );
	EntryLayout();
}

Int32 CEdIconGrid::GetNearestEntryOfTypeTo( EntryType et, Int32 index, EntrySearchDirection est )
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		Int32 a = index - i;
		Int32 b = index + i;
		if ( ( est == EST_PREV || est == EST_BOTH ) && a >= 0 && a < (Int32)m_entries.Size() && m_entries[a]->etype == et && !m_entries[a]->hidden ) return a;
		if ( ( est == EST_NEXT || est == EST_BOTH ) && b >= 0 && b < (Int32)m_entries.Size() && m_entries[b]->etype == et && !m_entries[b]->hidden ) return b;
	}
	return -1;
}

void CEdIconGrid::EntryLayout()
{
	Int32 x = 10, y = 10, lineHeight = 0;
	wxSize size = GetClientSize();

	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		Entry* e = m_entries[i];
		e->index = i;

		if ( e->hidden ) continue;

		wxSize entrySize = GetEntrySize( e );

		if ( e->etype == ET_ENTRY )
		{
			if ( entrySize.GetHeight() > lineHeight )
			{
				lineHeight = entrySize.GetHeight();
			}

			if ( x + entrySize.GetWidth() + 10 >= size.GetWidth() && x > 10 )
			{
				x = 10;
				y += lineHeight + 10;
				lineHeight = entrySize.GetHeight();
			}

			e->x = x;
			e->y = y;

			x += entrySize.GetWidth() + 10;
		}
		else if ( e->etype == ET_SEPARATOR )
		{
			if ( x > 10 )
			{
				x = 10;
				y += lineHeight + 20;
			}

			e->x = 10;
			e->y = y;

			y += entrySize.GetHeight() + 10;

			lineHeight = 0;
		}
	}

	m_totalInnerHeight = y + lineHeight + 10;
	AdjustScrollbars();
}

void CEdIconGrid::HandleClickOnEntry( Entry* entry, Int32 x, Int32 y, wxMouseButton button )
{
	if ( button == wxMOUSE_BTN_LEFT || ( button == wxMOUSE_BTN_MIDDLE && m_middleClickSelectsBoth && m_allowSecondary ) )
	{
		if ( entry->etype == ET_ENTRY )
		{
			if ( m_allowMultiSelection && button == wxMOUSE_BTN_LEFT && wxGetKeyState( WXK_CONTROL ) )
			{
				if ( m_selected.Exist( entry->index ) )
				{
					TDynArray<Int32> selected = GetAllSelected();
					selected.Remove( entry->index );
					SetAllSelected( selected );
				}
				else
				{
					DoSetSelected( entry->index, true, false );
				}
			}
			else
			{
				SetSelected( entry->index );
			}
		}
		else
		{
			ToggleGroup( entry->group );
		}
	}
	if ( button == wxMOUSE_BTN_RIGHT || ( button == wxMOUSE_BTN_MIDDLE && m_middleClickSelectsBoth && m_allowSecondary ) )
	{
		if ( m_allowSecondary && entry->etype == ET_ENTRY )
		{
			SetSecondary( entry->index );
		}
	}
}

void CEdIconGrid::ShowGroupNavigator( Entry* entry )
{
	TDynArray<Entry*> groupEntries;
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->etype == ET_SEPARATOR )
		{
			groupEntries.PushBack( m_entries[i] );
		}
	}

	GroupNavigator* navigator = new GroupNavigator( this, this, entry, groupEntries );
	wxSize entrySize = GetEntrySize( entry );
	wxPoint navPosition = ClientToScreen( wxPoint( entry->captionPosition.x - 11, entry->captionPosition.y - 6 - groupEntries.GetIndex( entry )*entry->captionSize.GetHeight() ) );
	wxPoint top = ClientToScreen( wxPoint( 0, 0 ) );
	if ( navPosition.y < top.y )
	{
		POINT pos;
		Int32 delta = top.y - navPosition.y;
		::GetCursorPos( &pos );
		::SetCursorPos( pos.x, pos.y + delta );
		navigator->m_initialY += delta;
		navPosition.y = top.y;
	}
	navigator->Move( navPosition.x, navPosition.y, wxSIZE_USE_EXISTING );
	navigator->Show();
}

void CEdIconGrid::OnPaint( wxPaintEvent& event )
{
	wxBitmap unknown = wxArtProvider::GetBitmap( wxART_MISSING_IMAGE );
	wxPaintDC dc( this );
	wxRect rect( GetClientRect() );
	wxSize size = rect.GetSize();

	dc.SetPen( *wxTRANSPARENT_PEN );

	wxBrush background( wxSystemSettings::GetColour( wxSYS_COLOUR_3DDKSHADOW ) );
	dc.SetBrush( background );
	dc.DrawRectangle( rect );

	HDC cdc = CreateCompatibleDC( dc.GetHDC() );
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		Entry* e = m_entries[i];

		if ( e->hidden ) continue;

		wxSize entitySize = GetEntrySize( e );

		if ( e->y - m_scroll + entitySize.GetHeight() + 5 >= 0 &&
			e->y - m_scroll - 5 < size.GetHeight() )
		{
			if ( e->etype == ET_ENTRY )
			{
				HBITMAP prevBmp, bitmap = GetThumbnailForEntry( e );
				Int32 height = floor( m_thumbnailSize );
				if ( m_selected.Exist( i ) )
				{
					dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) ) );
					dc.DrawRoundedRectangle( e->x - 5, e->y - 5 - m_scroll, entitySize.GetWidth() + 10, entitySize.GetHeight() + 10, 3 );
					if ( i == m_secondary )
					{
						wxColor color = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
						dc.SetBrush( wxBrush( wxColor( color.Red(), color.Blue(), color.Green() ) ) );
						dc.DrawRoundedRectangle( e->x + entitySize.GetWidth()/2, e->y - 5 - m_scroll, entitySize.GetWidth() - entitySize.GetWidth()/2 + 5, entitySize.GetHeight() + 10, 3 );
						dc.DrawRectangle( e->x + entitySize.GetWidth()/2, e->y - 5 - m_scroll, 10, entitySize.GetHeight() + 10 );
					}
					dc.SetBrush( background );
				}
				else if ( m_allowSecondary && i == m_secondary )
				{
					wxColor color = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
					dc.SetBrush( wxBrush( wxColor( color.Red(), color.Blue(), color.Green() ) ) );
					dc.DrawRoundedRectangle( e->x - 5, e->y - 5 - m_scroll, entitySize.GetWidth() + 10, entitySize.GetHeight() + 10, 3 );
					dc.SetBrush( background );
				}


				if ( bitmap )
				{
					Gdiplus::Color background( 96, 96, 96 );
					prevBmp = (HBITMAP)SelectObject( cdc, (HGDIOBJ)bitmap );
					BitBlt( dc.GetHDC(), e->x, e->y - m_scroll, floor( m_thumbnailSize ), floor( m_thumbnailSize ), cdc, 0, 0, SRCCOPY );
					SelectObject( cdc, prevBmp );
					height = floor( m_thumbnailSize );
				}
				dc.SetClippingRegion( e->x, e->y - m_scroll, entitySize.GetWidth(), entitySize.GetHeight() );
				if ( m_selected.Exist( i ) )
				{
					wxColor color = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
					dc.SetTextForeground( wxColor( color.Red()/2, color.Green()/2, color.Blue()/2 ) );
					dc.DrawText( e->caption, e->x, e->y - m_scroll + height + 6 );
					dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
				}
				else if ( m_allowSecondary && i == m_secondary )
				{
					wxColor color = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
					dc.SetTextForeground( wxColor( color.Red()/2, color.Blue()/2, color.Green()/2 ) );
					dc.DrawText( e->caption, e->x, e->y - m_scroll + height + 6 );
					dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
				}
				else
				{
					dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ) );
				}
				dc.DrawText( e->caption, e->x, e->y - m_scroll + height + 5 );
				dc.DestroyClippingRegion();
			}
			else if ( e->etype == ET_SEPARATOR )
			{
				wxFont prevFont = dc.GetFont();
				wxFont font = prevFont;
				wxColor shadow = wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW );
				shadow = wxColor( shadow.Red()/3, shadow.Green()/3, shadow.Blue()/3 );
				font.MakeLarger();
				dc.SetFont( font );
				dc.SetTextForeground( shadow );
				dc.SetPen( wxPen( shadow, 1, wxPENSTYLE_SOLID ) );
				dc.SetBrush( wxBrush( shadow, wxBRUSHSTYLE_SOLID ) );
				dc.DrawText( e->caption, size.GetWidth() - 10 - e->captionSize.GetWidth(),
					e->y - m_scroll + ( entitySize.GetHeight() - e->captionSize.GetHeight() )/2 + 1 );
				dc.DrawLine( e->x + 13, e->y - m_scroll + entitySize.GetHeight()/2 + 2,
					size.GetWidth() - 15 - e->captionSize.GetWidth(), e->y - m_scroll + entitySize.GetHeight()/2 + 2 );
				wxPoint tri[3];
				if ( e->collapsed )
				{
					tri[0] = wxPoint( e->x, e->y - m_scroll + entitySize.GetHeight()/2 - 2);
					tri[1] = wxPoint( e->x, e->y - m_scroll + entitySize.GetHeight()/2 + 6);
					tri[2] = wxPoint( e->x + 4, e->y - m_scroll + entitySize.GetHeight()/2 + 2);
				}
				else
				{
					tri[0] = wxPoint( e->x, e->y - m_scroll + entitySize.GetHeight()/2 );
					tri[1] = wxPoint( e->x + 8, e->y - m_scroll + entitySize.GetHeight()/2 );
					tri[2] = wxPoint( e->x + 4, e->y - m_scroll + entitySize.GetHeight()/2 + 4 );
				}
				dc.DrawPolygon( 3, tri );
				dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ) );
				dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ), 1, wxPENSTYLE_SOLID ) );
				dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ), wxBRUSHSTYLE_SOLID ) );
				e->captionPosition.x = size.GetWidth() - 10 - e->captionSize.GetWidth();
				e->captionPosition.y = e->y - m_scroll + ( entitySize.GetHeight() - e->captionSize.GetHeight() )/2;
				dc.DrawText( e->caption, e->captionPosition );
				dc.DrawLine( e->x + 13, e->y - m_scroll + entitySize.GetHeight()/2 + 1,
					size.GetWidth() - 15 - e->captionSize.GetWidth(), e->y - m_scroll + entitySize.GetHeight()/2 + 1 );
				dc.DrawPolygon( 3, tri, 0, -1 );
				dc.SetPen( *wxTRANSPARENT_PEN );
				dc.SetFont( prevFont );
			}
		}
	}
	DeleteDC( cdc );

	if ( m_paintIsWrong )
	{
		EntryLayout();
		Refresh( false );
		m_paintIsWrong = false;
	}
}

void CEdIconGrid::OnSize( wxSizeEvent& event )
{
	EntryLayout();
}

void CEdIconGrid::OnScrollWin( wxScrollWinEvent& event )
{
	if ( event.GetOrientation() == wxVERTICAL )
	{
		if ( event.GetEventType() == wxEVT_SCROLLWIN_TOP )
		{
			SetScroll( 0 );
		}
		else if ( event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM )
		{
			SetScroll( m_totalInnerHeight );
		}
		else if ( event.GetEventType() == wxEVT_SCROLLWIN_LINEUP ||
			event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP )
		{
			SetScroll( m_scroll - 24 );
		}
		else if ( event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN ||
			event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN )
		{
			SetScroll( m_scroll + 24 );
		}
		else if ( event.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE ||
			event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK )
		{
			SetScroll( event.GetPosition() );
		}
	}
}

void CEdIconGrid::OnMotion( wxMouseEvent& event )
{
	Int32 item = GetEntryAt( event.m_x, event.m_y );
	Bool anyHover = false;

	if ( item > -1 ) 
	{
		Entry* entry = m_entries[item];
		if ( entry->etype == ET_SEPARATOR )
		{
			wxSize entrySize = GetEntrySize( entry );
			if ( event.m_x > entry->x + entrySize.GetWidth() - entry->captionSize.GetWidth() )
			{
				anyHover = true;
				if ( item != m_ignoreHover )
				{
					ShowGroupNavigator( entry );
				}
			}
		}
	}

	if ( !anyHover )
	{
		m_ignoreHover = -1;
	}
}

void CEdIconGrid::OnMouseWheel( wxMouseEvent& event )
{
	if ( event.ControlDown() && !event.AltDown() && !event.ShiftDown() )
	{
		if ( event.GetWheelRotation() > 0 )
		{
			SetThumbnailZoom( GetThumbnailZoom()*1.2 );
		}
		else
		{
			SetThumbnailZoom( GetThumbnailZoom()/1.2 );
		}
	}
	else
	{
		SetScroll( m_scroll - event.GetWheelRotation() );
	}
}

void CEdIconGrid::OnMouseButtonDown( wxMouseEvent& event )
{
	Int32 item = GetEntryAt( event.m_x, event.m_y );
	SetFocus();
	if ( item > -1 )
	{
		HandleClickOnEntry( m_entries[item], event.m_x, event.m_y, static_cast<wxMouseButton>( event.GetButton() ) );
	}
}

void CEdIconGrid::OnKeyDownEvent( wxKeyEvent& event )
{
	if ( GetSelected() > -1 )
	{
		Int32 selected = m_selected[m_selected.Size() - 1];
		Entry* e = m_entries[selected];
		Int32 toSelect = -1;
		Int32 visibleThumbnailSize = floor( m_thumbnailSize );
		if ( event.GetKeyCode() == WXK_LEFT )
		{
			Int32 newItem = GetNearestEntryAt( e->x - visibleThumbnailSize, e->y );
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, newItem == selected ? ( selected - ( selected > 0 ? 1 : 0 ) ) : newItem, EST_PREV );
		}
		else if ( event.GetKeyCode() == WXK_RIGHT )
		{
			Int32 newItem = GetNearestEntryAt( e->x + visibleThumbnailSize, e->y );
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, newItem == selected ? ( selected + 1 ) : newItem, EST_NEXT );
		}
		else if ( event.GetKeyCode() == WXK_UP )
		{
			Int32 newItem = GetNearestEntryAt( e->x, e->y - visibleThumbnailSize*3/2 );
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, newItem == selected ? 0 : newItem, EST_PREV );
		}
		else if ( event.GetKeyCode() == WXK_DOWN )
		{
			Int32 newItem = GetNearestEntryAt( e->x, e->y + visibleThumbnailSize*3/2 );
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, newItem == selected ? ( m_entries.Size() - 1 ) : newItem, EST_NEXT );
		}
		else if ( event.GetKeyCode() == WXK_PAGEUP )
		{
			wxSize size = GetClientSize();
			Int32 newItem = GetNearestEntryAt( e->x, e->y - size.GetHeight()/3*2 );
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, newItem == selected ? 0 : newItem, EST_PREV );
		}
		else if ( event.GetKeyCode() == WXK_PAGEDOWN )
		{
			wxSize size = GetClientSize();
			Int32 newItem = GetNearestEntryAt( e->x, e->y + size.GetHeight()/3*2 );
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, newItem == selected ? ( m_entries.Size() - 1 ) : newItem, EST_NEXT );
		}
		else if ( event.GetKeyCode() == WXK_HOME )
		{
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, 0 );
		}
		else if ( event.GetKeyCode() == WXK_END )
		{
			toSelect = GetNearestEntryOfTypeTo( ET_ENTRY, m_entries.Size() - 1 );
		}

		if ( toSelect != -1 ) SetSelected( toSelect );
	}
	event.Skip();
}

void CEdIconGrid::SetAllowSecondary( Bool allowSecondary /* = true */ )
{
	if ( m_allowSecondary != allowSecondary )
	{
		m_allowSecondary = allowSecondary;
		Refresh( false );
	}
}

void CEdIconGrid::SetMiddleClickSelectsBoth( Bool middleClickSelectsBoth )
{
	m_middleClickSelectsBoth = middleClickSelectsBoth;
}

void CEdIconGrid::SetMultiSelection( Bool multiSelection )
{
	m_allowMultiSelection = multiSelection;

	if ( !m_allowMultiSelection && m_selected.Size() > 1 )
	{
		SetSelected( m_selected[0] );
	}
}

void CEdIconGrid::SetSelected( Int32 index )
{
	DoSetSelected( index, false, false );
}

void CEdIconGrid::SetAllSelected( const TDynArray<Int32>& selected )
{
	if ( m_allowMultiSelection )
	{
		DoSetSelected( selected, false, false );
	}
	else
	{
		SetSelected( selected.Size() > 0 ? selected[0] : -1 );
	}
}

void CEdIconGrid::SetSecondary( Int32 index )
{
	DoSetSelected( index, false, true );
}

void CEdIconGrid::SetThumbnailSize( Int32 thumbnailSize )
{
	thumbnailSize = Clamp( thumbnailSize, 16, 1024 );

	if ( thumbnailSize != m_originalThumbnailSize )
	{
		m_originalThumbnailSize = m_thumbnailSize = thumbnailSize;
		EntryLayout();
		Refresh( false );
	}
}

void CEdIconGrid::SetThumbnailZoom( Double zoom )
{
	if ( abs( zoom - GetThumbnailZoom() ) < 0.01 )
	{
		return;
	}

	Double newSize = zoom*(Double)m_originalThumbnailSize;

	if ( newSize != m_thumbnailSize )
	{
		m_thumbnailSize = newSize;
		EntryLayout();
		Refresh( false );
	}
}

void CEdIconGrid::Clear()
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		delete m_entries[i];
	}
	m_entries.Clear();
	m_group = 0;
	m_scroll = 0;
	m_totalInnerHeight = 0;
	m_selected.Clear();
	m_secondary = -1;
	m_lastSelected = -1;
	AdjustScrollbars();
}

Int32 CEdIconGrid::AddGroup( const String& title )
{
	m_group++;
	DoAddEntry( new Entry( NULL, ET_SEPARATOR, title ) );
	return m_group;
}

void CEdIconGrid::AddEntry( CEdIconGridEntryInfo* info )
{
	TDynArray<CEdIconGridEntryInfo*> infos( 1 );
	infos[0] = info;
	AddEntries( infos );
}

void CEdIconGrid::AddEntries( TDynArray<CEdIconGridEntryInfo*>& infos )
{
	TDynArray<Entry*> entries( infos.Size() );
	LONGLONG startTime = GetTickCount64();
	Bool feedbackShown = false;

	for ( Uint32 i=0; i<infos.Size(); ++i )
	{
		// After about a second, show some feedback
		if ( !feedbackShown && GetTickCount64() - startTime > 1500 )
		{
			feedbackShown = true;
			GFeedback->BeginTask( TXT("Populating the grid..."), false );
		}

		// Update feedback
		if ( feedbackShown )
		{
			String msg = String::Printf( TXT("Obtaining information about entry %d of %d..."), (int)(i + 1), (int)infos.Size() );
			GFeedback->UpdateTaskInfo( msg.AsChar() );
			GFeedback->UpdateTaskProgress( i + 1, infos.Size() );
		}

		entries[i] = new Entry( infos[i], ET_ENTRY, TXT("") );
		infos[i]->m_grid = this;
		infos[i]->m_entry = entries[i];
	}

	// Hide the feedback if it was shown
	if ( feedbackShown )
	{
		GFeedback->EndTask();
	}

	DoAddEntries( entries );
}

void CEdIconGrid::RemoveEntry( Int32 index )
{
	if ( index >= 0 && index < (Int32)m_entries.Size() )
	{
		delete m_entries[index];
		m_entries.RemoveAt( index );
		if ( m_selected.Size() == 1 )
		{
			if ( GetSelected() >= (Int32)m_entries.Size() )
			{
				SetSelected( GetSelected() - 1 );
			}
		}
		else
		{
			if ( m_selected.Exist( index ) )
			{
				m_selected.Remove( index );
			}
		}
		if ( m_secondary >= (Int32)m_entries.Size() )
		{
			m_secondary--;
		}
		EntryLayout();
		Refresh( false );
	}
}

Int32 CEdIconGrid::GetEntryAt( Int32 x, Int32 y ) const
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		Entry* e = m_entries[i];
		if ( e->hidden ) continue;
		wxSize entrySize = GetEntrySize( e );
		if ( x >= e->x && y >= e->y - m_scroll && x < e->x + entrySize.GetWidth() && y < e->y + entrySize.GetHeight() - m_scroll )
		{
			return i;
		}
	}

	return -1;
}

Int32 CEdIconGrid::GetNearestEntryAt( Int32 x, Int32 y ) const
{
	Int32 nearest = -1;
	Float nearestDistance = 0;
	Vector pos( x, y, 0 );
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->hidden ) continue;
		if ( m_entries[i]->etype == ET_ENTRY )
		{
			Float distance = pos.DistanceSquaredTo2D( Vector( m_entries[i]->x, m_entries[i]->y, 0 ) );
			if ( nearest == -1 || distance < nearestDistance )
			{
				nearestDistance = distance;
				nearest = i;
			}
		}
	}
	return nearest;
}

CEdIconGridEntryInfo* CEdIconGrid::GetEntryInfo( Int32 index ) const
{
	return m_entries[index]->info;
}

Int32 CEdIconGrid::FindEntry( CEdIconGridEntryInfo* info ) const
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->info == info) return i;
	}
	return -1;
}

void CEdIconGrid::ShowGroup( Int32 group )
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->group == group )
		{
			if ( m_entries[i]->etype == ET_SEPARATOR )
				m_entries[i]->collapsed = false;
			else
				m_entries[i]->hidden = false;
		}
	}
	EntryLayout();
	Refresh( false );
}

void CEdIconGrid::HideGroup( Int32 group )
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->group == group )
		{
			if ( m_entries[i]->etype == ET_SEPARATOR )
				m_entries[i]->collapsed = true;
			else
				m_entries[i]->hidden = true;
		}
	}
	EntryLayout();
	Refresh( false );
}

Bool CEdIconGrid::IsGroupHidden( Int32 group )
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->group == group )
		{
			return m_entries[i]->etype == ET_SEPARATOR ? m_entries[i]->collapsed : m_entries[i]->hidden;
		}
	}
	return true;
}

void CEdIconGrid::ToggleGroup( Int32 group )
{
	if ( IsGroupHidden( group ) )
	{
		ShowGroup( group );
	}
	else
	{
		HideGroup( group );
	}
}

void CEdIconGrid::SetHook( IEdIconGridHook* hook )
{
	m_hook = hook;
}
