/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "wxThumbnailImageLoader.h"

class CEdIconGrid;

class IEdIconGridHook
{
public:
	virtual ~IEdIconGridHook(){};
	virtual void OnIconGridSelectionChange( class CEdIconGrid* grid, Bool primary ){}
};

class CEdIconGridEntryInfo
{
	friend class CEdIconGrid;

	void*							m_entry;
	CEdIconGrid*					m_grid;

public:
	virtual ~CEdIconGridEntryInfo(){}

	RED_INLINE CEdIconGrid* GetGrid() const { return m_grid; }

	Int32 GetIndex() const;
	wxPoint GetPosition() const;
	wxSize GetSize() const;
	Bool IsSelected() const;

	virtual String GetCaption() const=0;
	virtual CWXThumbnailImage* GetThumbnail()=0;
};

class CEdIconGrid : public wxControl
{
	DECLARE_DYNAMIC_CLASS( CEdIconGrid )
	DECLARE_EVENT_TABLE()

	friend class CEdIconGridEntryInfo;
	friend class GroupNavigator;

	typedef enum
	{
		ET_ENTRY,
		ET_SEPARATOR
	} EntryType;

	typedef enum
	{
		EST_BOTH,
		EST_PREV,
		EST_NEXT
	} EntrySearchDirection;

	// TODO: move the inlined code to iconGrid.cpp
	struct Entry
	{
		CEdIconGridEntryInfo* info;
		Int32 x, y;
		Uint32 index;
		EntryType etype;
		wxString caption;
		CWXThumbnailImage* thumb;
		HBITMAP thumbBmp;
		Int32 usedThumbnailSize;
		wxPoint captionPosition;
		wxSize captionSize;
		Bool collapsed;
		Uint32 group;
		Bool hidden;

		Entry( CEdIconGridEntryInfo* entryInfo, EntryType entryType, const String& altCaption )
			: info( entryInfo )
			, x( 0 )
			, y( 0 )
			, index( 0 )
			, etype( entryType )
			, thumb( NULL )
			, thumbBmp( NULL )
			, usedThumbnailSize( 0 )
			, collapsed( false )
			, group( 0 )
			, hidden( false )
		{
			if ( info )
			{
				caption = info->GetCaption().AsChar();
				thumb = info->GetThumbnail();
			}
			else
			{
				caption = altCaption.AsChar();
				thumb = NULL;
			}
		}

		~Entry()
		{
			if ( thumbBmp )
			{
				::DeleteObject( thumbBmp );
			}
			if ( info )
			{
				delete info;
			}
		}
	};

	// TODO: move the inlined code to iconGrid.cpp
	class GroupNavigator : public wxPopupWindow
	{
	public:
		CEdIconGrid*		m_grid;
		TDynArray<Entry*>	m_entries;
		TDynArray<Int32>		m_entryY;
		Entry*				m_currentEntry;
		CEdTimer			m_destroyTimer;
		Int32					m_initialIndex;
		Int32					m_initialScroll;
		Int32					m_initialY;

		GroupNavigator( wxWindow* parent, CEdIconGrid* grid, Entry* currentEntry, const TDynArray<Entry*>& entries )
			: wxPopupWindow( parent )
			, m_grid( grid )
			, m_currentEntry( currentEntry )
			, m_entries( entries )
		{
			SetDoubleBuffered( true );
			SetFont( grid->GetFont().Larger() );
			CalculateLayout();
			m_entryY.Resize( entries.Size() );
			m_destroyTimer.SetOwner( this );
			m_initialIndex = entries.GetIndex( currentEntry );
			m_initialScroll = grid->m_scroll;
			m_initialY = currentEntry->y;
			Connect( wxEVT_PAINT, wxPaintEventHandler( GroupNavigator::OnPaint ), NULL, this );
			Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( GroupNavigator::OnLeaveWindow ), NULL, this );
			Connect( wxEVT_TIMER, wxTimerEventHandler( GroupNavigator::OnTimer ), NULL, this );
			Connect( wxEVT_MOTION, wxMouseEventHandler( GroupNavigator::OnMotion ), NULL, this );
			Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( GroupNavigator::OnLeftDown ), NULL, this );
		}

		~GroupNavigator()
		{
		}

		void SetCurrentEntry( Entry* entry )
		{
			if ( entry == m_currentEntry ) return;
			m_currentEntry = entry;
			Refresh( false );
			m_grid->SetScroll( m_initialScroll + ( entry->y - m_initialY ) + ( m_initialIndex - m_entries.GetIndex( entry ) )*entry->captionSize.GetHeight() );
		}

		void CalculateLayout()
		{
			int w = 0, h = 0;
			wxScreenDC dc;

			dc.SetFont( GetFont() );
			for ( Uint32 i=0; i<m_entries.Size(); ++i )
			{
				wxSize& size = m_entries[i]->captionSize;
				if ( w < size.GetWidth() ) w = size.GetWidth();
				h += size.GetHeight();
			}

			SetClientSize( w + 20, h + 12 );
		}

		void OnPaint( wxPaintEvent& event )
		{
			wxPaintDC dc( this );
			wxSize size = GetClientSize();
			dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_3DDKSHADOW ), wxBRUSHSTYLE_SOLID ) );
			dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE ), 1, wxPENSTYLE_SOLID ) );
			dc.DrawRectangle( 0, 0, size.GetWidth(), size.GetHeight() - 1 );
			wxColor shadow = wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW );
			shadow = wxColor( shadow.Red()/3, shadow.Green()/3, shadow.Blue()/3 );
			dc.SetPen( wxPen( shadow, 1, wxPENSTYLE_SOLID ) );
			dc.DrawLine( 0, size.GetHeight() - 1, size.GetWidth(), size.GetHeight() - 1 );
			dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ) );
			dc.SetPen( *wxTRANSPARENT_PEN );

			int y = 6;
			for ( Uint32 i=0; i<m_entries.Size(); ++i )
			{
				Entry* e = m_entries[i];
				if ( e == m_currentEntry )
				{
					dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ) ) );
					dc.DrawRectangle( 0, y, size.GetWidth(), e->captionSize.GetHeight() + 1 );
					dc.SetTextForeground( shadow );
					dc.SetPen( *wxTRANSPARENT_PEN );
				}
				dc.DrawText( e->caption, 11, y );
				if ( e == m_currentEntry )
				{
					dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ) );
				}
				m_entryY[i] = y;
				y += e->captionSize.GetHeight();
			}
		}

		void OnLeaveWindow( wxMouseEvent& event )
		{
			m_destroyTimer.Start( 1, true );
		}

		void OnTimer( wxTimerEvent& event )
		{
			if ( !IsMouseInWindow() )
			{
				Destroy();
			}
		}

		void OnMotion( wxMouseEvent& event )
		{
			for ( Int32 i=(Int32)m_entries.Size() - 1; i>=0; --i )
			{
				if ( m_entryY[i] < event.m_y )
				{
					SetCurrentEntry( m_entries[i] );
					break;
				}
			}
		}

		void OnLeftDown( wxMouseEvent& event )
		{
			m_grid->m_ignoreHover = m_currentEntry->index;
			Destroy();
		}
	};

protected:
	TDynArray<Entry*>				m_entries;
	bool							m_paintIsWrong;
	Int32								m_scroll;
	Int32								m_totalInnerHeight;
	TDynArray<Int32>					m_selected;
	Int32								m_secondary;
	Bool							m_allowSecondary;
	Bool							m_middleClickSelectsBoth;
	Bool							m_allowMultiSelection;
	Int32								m_ignoreHover;
	IEdIconGridHook*				m_hook;
	Uint32							m_group;
	Double							m_thumbnailSize;
	Int32								m_originalThumbnailSize;
	Int32								m_lastSelected;

	void Init();
	void AdjustScrollbars();
	void SetScroll( Int32 scroll );
	void MakePointVisible( Int32 x, Int32 y );

	void DoSetSelected( const TDynArray<Int32>& indices, bool add, bool secondary );
	void DoSetSelected( Int32 index, bool add, bool secondary );

	HBITMAP GetThumbnailForEntry( Entry* entry );
	wxSize GetEntrySize( Entry* entry ) const;

	void DoAddEntry( Entry* entry );
	void DoAddEntries( const TDynArray<Entry*>& entries );
	Int32 GetNearestEntryOfTypeTo( EntryType et, Int32 index, EntrySearchDirection est=EST_BOTH );

	void EntryLayout();

	virtual void HandleClickOnEntry( Entry* entry, Int32 x, Int32 y, wxMouseButton button );
	void ShowGroupNavigator( Entry* entry );

	void OnPaint( wxPaintEvent& event );
	void OnSize( wxSizeEvent& event );
	void OnScrollWin( wxScrollWinEvent& event );
	void OnMotion( wxMouseEvent& event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnMouseButtonDown( wxMouseEvent& event );
	void OnKeyDownEvent( wxKeyEvent& event );

public:
	CEdIconGrid();
	CEdIconGrid( wxWindow* parent, Int32 style );
	~CEdIconGrid();

	void SetAllowSecondary( Bool allowSecondary = true );
	RED_INLINE Bool GetAllowSecondary() const { return m_allowSecondary; }
	void SetMiddleClickSelectsBoth( Bool middleClickSelectsBoth = true );
	RED_INLINE Bool GetMiddleClickSelectsBoth() const { return m_middleClickSelectsBoth; }
	void SetMultiSelection( Bool multiSelection = true );
	RED_INLINE Bool GetMultiSelection() const { return m_allowMultiSelection; }

	void SetSelected( Int32 index );
	void SetAllSelected( const TDynArray<Int32>& selected );
	void SetSecondary( Int32 index );
	RED_INLINE Int32 GetSelected() const { return m_selected.Empty() ? -1 : m_selected[0]; }
	RED_INLINE const TDynArray<Int32>& GetAllSelected() const { return m_selected; }
	RED_INLINE Int32 GetSecondary() const { return m_secondary; }
	RED_INLINE CEdIconGridEntryInfo* GetSelectedEntryInfo() const { return GetSelected() == -1 ? NULL : m_entries[GetSelected()]->info; }
	RED_INLINE CEdIconGridEntryInfo* GetSecondaryEntryInfo() const { return m_secondary == -1 ? NULL : m_entries[m_secondary]->info; }
	RED_INLINE Int32 GetLastSelected() const { return m_lastSelected; }

	void SetThumbnailSize( Int32 thumbnailSize );
	void SetThumbnailZoom( Double zoom );
	RED_INLINE Int32 GetThumbnailSize() const { return m_originalThumbnailSize; }
	RED_INLINE Double GetThumbnailZoom() const { return m_thumbnailSize/(Double)m_originalThumbnailSize; }

	void Clear();
	Int32 AddGroup( const String& title );
	void AddEntry( CEdIconGridEntryInfo* info );
	void AddEntries( TDynArray<CEdIconGridEntryInfo*>& infos );
	void RemoveEntry( Int32 index );
	Int32 GetEntryAt( Int32 x, Int32 y ) const;
	Int32 GetNearestEntryAt( Int32 x, Int32 y ) const;
	CEdIconGridEntryInfo* GetEntryInfo( Int32 index ) const;
	Int32 FindEntry( CEdIconGridEntryInfo* info ) const;

	void ShowGroup( Int32 group );
	void HideGroup( Int32 group );
	Bool IsGroupHidden( Int32 group );
	void ToggleGroup( Int32 group );

	void SetHook( IEdIconGridHook* hook );
	RED_INLINE IEdIconGridHook* GetHook() const { return m_hook; }
};
