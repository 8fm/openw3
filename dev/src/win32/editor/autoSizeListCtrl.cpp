
#include "build.h"
#include "autoSizeListCtrl.h"

wxIMPLEMENT_DYNAMIC_CLASS( CEdAutosizeListCtrl, wxListCtrl );

wxBEGIN_EVENT_TABLE( CEdAutosizeListCtrl, wxListCtrl )
	EVT_SIZE( CEdAutosizeListCtrl::OnSize )
	EVT_LIST_COL_DRAGGING( wxID_ANY, CEdAutosizeListCtrl::OnColDragging )
wxEND_EVENT_TABLE()

CEdAutosizeListCtrl::CEdAutosizeListCtrl() 
	: m_internalColResize( false )
{ 
	Init(); 
}

CEdAutosizeListCtrl::CEdAutosizeListCtrl( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name )
	: m_internalColResize( false )
{
	Init();
	Create( parent, id, pos, size, style, validator, name );
}

TDynArray< Int32 > CEdAutosizeListCtrl::GetSelectedItems() const
{
	TDynArray< Int32 > result;
	for ( Int32 itemIndex = -1; ; ) 
	{
		itemIndex = GetNextItem( itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( itemIndex == -1 ) break;
		result.PushBack( itemIndex );
	}
	return result;
}

void CEdAutosizeListCtrl::Select( Int32 item, Bool state )
{
	SetItemState( item, state ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED );
}

void CEdAutosizeListCtrl::Highlight( Int32 item, Bool state )
{
	SetItemState( item, state ? wxLIST_STATE_DROPHILITED : 0, wxLIST_STATE_DROPHILITED );
}

void CEdAutosizeListCtrl::SetSelection( Int32 item )
{
	SetExclusiveState( item, wxLIST_STATE_SELECTED );
}

void CEdAutosizeListCtrl::SetHighlight( Int32 item )
{
	SetExclusiveState( item, wxLIST_STATE_DROPHILITED );
}

void CEdAutosizeListCtrl::SetTopItem( Int32 item )
{ 
	// It's kinda hacky, but it works, although only if 'EnsureVisible' always shows one element after requested (which is the case on Windows).
	int bottom = Min( item + GetCountPerPage(), GetItemCount() - 1 );
	EnsureVisible( bottom - 1 ); // ensure the visibility of the item that is meant to be the second-last
}

void CEdAutosizeListCtrl::UpdateAutoColumnWidth()
{
	if ( InReportView() && GetColumnCount() >= 0 )
	{
		wxSize size = GetClientSize();
		int total = 0;

		for ( int i = 0; i < GetColumnCount()-1; ++i )
		{
			total += GetColumnWidth( i );
		}

		int width = Max( size.x - total - 5, 10 );

		m_internalColResize = true;
		SetColumnWidth( GetColumnCount()-1, width );
		m_internalColResize = false;
	}
}

void CEdAutosizeListCtrl::OnSize( wxSizeEvent& event )
{
	UpdateAutoColumnWidth();

	event.Skip();
}

void CEdAutosizeListCtrl::OnColDragging( wxListEvent& event )
{
	if ( !m_internalColResize )
	{
		UpdateAutoColumnWidth();
	}

	event.Skip();
}

void CEdAutosizeListCtrl::SetExclusiveState( Int32 item, Int32 flag )
{
	for ( int itemIndex = -1; ; ) 
	{
		itemIndex = GetNextItem( itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE );
		if ( itemIndex == -1 ) break;
		Bool stateToSet = ( itemIndex == item );
		Bool state = ( GetItemState( itemIndex, flag ) != 0 );
		if ( state != stateToSet ) // wxListCtrl doesn't check the previous event and sends sometimes redundant events
		{
			SetItemState( itemIndex, stateToSet ? flag : 0, flag );
		}
	}
}

Bool CEdAutosizeListCtrl::SetSortArrow( Int32 columnIdx, SortArrow type )
{
	if ( ::HWND hHeader = ListView_GetHeader( (::HWND)GetHandle() ) )
	{
		HDITEMW hdrItem  = { 0 };
		hdrItem.mask = HDI_FORMAT;

		if ( Header_GetItem( hHeader, columnIdx, &hdrItem ) )
		{
			switch ( type )
			{
			case SortArrow::None:
				hdrItem.fmt = hdrItem.fmt & ~(HDF_SORTDOWN | HDF_SORTUP);
				break;
			case SortArrow::Up:
				hdrItem.fmt = (hdrItem.fmt & ~HDF_SORTDOWN) | HDF_SORTUP;
				break;
			case SortArrow::Down:
				hdrItem.fmt = (hdrItem.fmt & ~HDF_SORTUP) | HDF_SORTDOWN;
				break;
			}

			return Header_SetItem( hHeader, columnIdx, &hdrItem ) != 0;
		}
	}

	return false;
}