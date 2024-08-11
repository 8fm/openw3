/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Extends the wxListCtrl with the auto-resizing of the last column, and adds some helper methods
class CEdAutosizeListCtrl: public wxListCtrl
{
public:

	enum class SortArrow
	{
		None, Up, Down
	};

	CEdAutosizeListCtrl();

	CEdAutosizeListCtrl(
		wxWindow *parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxLC_REPORT,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxListCtrlNameStr
		);

	//! Return list of selected items, empty array = no selection
	TDynArray< Int32 > GetSelectedItems() const;

	//! Select or de-select the given item
	void Select( Int32 item, Bool state = true );

	//! Highlight or un-highlight the given item
	void Highlight( Int32 item, Bool state = true );

	//! Single-select the given item (de-selecting others)
	void SetSelection( Int32 item );

	//! Single-highlight the given item (un-highlighting others)
	void SetHighlight( Int32 item );

	//! Sets the top visible item (missing counterpart to wxListCtrl GetTopItem, allows to restore saved scroll position).
	void SetTopItem( Int32 item );

	// Sets the column sorting indicator
	Bool SetSortArrow( Int32 columnIdx, SortArrow type );

private:
	wxDECLARE_DYNAMIC_CLASS( CEdAutosizeListCtrl );
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS( CEdAutosizeListCtrl );

	Bool m_internalColResize;

	void UpdateAutoColumnWidth();
	void OnSize( wxSizeEvent& event );
	void OnColDragging( wxListEvent& event );
	void SetExclusiveState( Int32 item, Int32 flag );
};
