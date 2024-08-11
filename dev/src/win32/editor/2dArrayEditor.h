/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "sheet\sheet.h"

class CEd2dArrayEditor : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	C2dArray*			m_array;
	wxSheet*			m_grid;
	wxPanel*			m_panel;
	String				m_cellString;
	Bool				m_arrayIsSortAsc;		//! Keeps track of sorting modes in array (ascend, descend)
	TDynArray<TPair<Uint32, Uint32>> m_searchResultCoords;
	Uint32				m_currentSearchResultIdx;
	wxTextCtrl*			m_searchLine;
	wxStaticText*		m_searchLabel;
	TDynArray<Uint32>	m_originalArrayLayout;

public:
	CEd2dArrayEditor( wxWindow* parent, C2dArray* array );
	~CEd2dArrayEditor();

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void SetCellColor( const wxSheetCoords& cords, const wxColor& color );
	Uint32 GetRowsNumber(); 
	Uint32 GetColsNumber(); 

protected:
	void UpdateGrid();

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void OnSize( wxSizeEvent& event );

	void OnSave( wxCommandEvent& event );
	void OnExit( wxCommandEvent& event );
	void OnAutoSize( wxCommandEvent& event );

	void OnGridCellChange( wxSheetEvent& event );

	void OnMouseRightRowLabel( wxMouseEvent& event );
	void OnMouseRightColumnLabel( wxMouseEvent& event );
	void OnMouseRightCell( wxMouseEvent& event );

	void OnAddColumn( wxCommandEvent& event );
	void OnAddRow( wxCommandEvent& event );

	void OnInsertColumn( wxCommandEvent& event );
	void OnInsertRow( wxCommandEvent& event );

	void OnDeleteColumn( wxCommandEvent& event );
	void OnDeleteRow( wxCommandEvent& event );

	void OnCopyPaste( wxKeyEvent& event );
	void OnPostPaste();

	void OnRowPopupClick( wxCommandEvent &event );
	void ShowInAssetBrowser();
	void ShowInExplorer();

	void OnSortByColumn( wxCommandEvent& event );

	void FindCellByContent( String& text );
	void OnSearchLineEnterDown( wxCommandEvent& event );
	void OnSearchButtonClicked( wxCommandEvent& event );
	void OnSearchNextResult( wxCommandEvent& event );
	void OnSearchPrevResult( wxCommandEvent& event );

	void SkipToSelectedRow();

	void OnFitCells( wxCommandEvent& event );

};
