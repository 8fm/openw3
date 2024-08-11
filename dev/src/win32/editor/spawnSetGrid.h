/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "sheet\sheet.h"


class CTemplateCellRenderer : public wxGridCellStringRenderer
{
public:
	static const Int32 ICON_WIDTH = 17;
	static const Int32 ICON_HEIGHT = 17;

	CTemplateCellRenderer();

	virtual void Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rectCell, int row, int col, bool isSelected );
	virtual wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col );

private:
	wxBitmap m_iconUse;
	wxBitmap m_iconBrowse;
	wxBitmap m_iconClear;
};

class CCommentCellRenderer : public wxGridCellStringRenderer
{
public:
	CCommentCellRenderer() {}

	virtual wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
	{
		wxSize size = wxGridCellStringRenderer::GetBestSize( grid, attr, dc, row, col );
		size.SetWidth( 1 );
		return size;
	}
};


class CSpawnSetGrid
{
public:
	struct SCellCords
	{
		int m_row;
		int m_col;
	};
	struct SSelectionCellCords : public SCellCords
	{
		int m_count;
	};

	virtual wxPen GetRowGridLinePen( Int32 row ) = 0;
	virtual void SetMainRows( const TSet< Int32 > &rows ) = 0;
	virtual void SetCellValue( int row, int col, const wxString &val ) = 0;
	virtual void SetCellValue( const wxString &val, int row, int col ) = 0;
	virtual int GetCursorRow() = 0;
	virtual int GetCursorColumn() = 0;
	virtual wxString GetCellValue( int row, int col ) = 0;
	virtual void SetGridCursor( int row, int col ) = 0;
	virtual wxRect CellToRect( int row, int col ) = 0;
	virtual wxRect BlockToDeviceRect( int topLeftRow, int topLeftCol, int bottomRightRow, int bottomRightCol ) = 0;
	virtual int GetRowLabelSize() = 0;
	virtual int GetColLabelSize() = 0;
	virtual bool IsReadOnly(int row, int col) const = 0;
	virtual void SetCellAlignment( int align, int row, int col ) = 0;
	virtual void SetCellAlignment( int row, int col, int horiz, int vert ) = 0;
	virtual void SetReadOnly( int row, int col, bool isReadOnly = true ) = 0;
	virtual void SetCellBackgroundColour( int row, int col, const wxColour& color ) = 0;
	virtual void ClearGrid() = 0;
	virtual wxColour GetCellBackgroundColour( int row, int col ) = 0;
	virtual int YToRow( int y ) = 0;
	virtual int XToCol( int x, bool clipToMinMax = false ) = 0;
	virtual void SetCellEditorNumber( int row, int col ) = 0;
	virtual void SetCellEditorFloat( int row, int col ) = 0;
	virtual void SetCellEditorBool( int row, int col ) = 0;
	virtual void SetCellEditorChoice( int row, int col ) = 0;
	virtual void SetCellEditorText( int row, int col ) = 0;
	virtual void SetCellRendererNumber( int row, int col ) = 0;
	virtual void SetCellRendererFloat( int row, int col ) = 0;
	virtual void SetCellRendererBool( int row, int col ) = 0;
	virtual void SetCellRendererText( int row, int col ) = 0;
	virtual void SetCellRendererTemplate( int row, int col ) = 0;
	virtual void SetCellRendererComment( int row, int col ) = 0;
	virtual wxWindow* GetWindow() = 0;
	virtual wxWindow* GetGridWindow() = 0;
	virtual void Connect( int eventType, wxObjectEventFunction func, wxObject *userData = (wxObject *) NULL, wxEvtHandler *eventSink = (wxEvtHandler *) NULL ) = 0;
	virtual bool CreateGrid( int numRows, int numCols ) = 0;
	virtual void DisableDragGridSize() = 0;
	virtual void DisableDragColSize() = 0;
	virtual void DisableDragRowSize() = 0;
	virtual void SetAutoLayout( bool autoLayout ) = 0;
	virtual int GetNumberCols() = 0;
	virtual int GetNumberRows() = 0;
	virtual void ForceRefresh() = 0;
	virtual bool InsertRows( size_t row, size_t numRows ) = 0;
	virtual bool DeleteRows( size_t row, size_t numRows ) = 0;
	virtual bool AppendRows( size_t numRows ) = 0;
	virtual void SetColLabelValue( int col, const wxString& value ) = 0;
	virtual void Refresh() = 0;
	virtual void SetRowLabelValue( int row, const wxString& value ) = 0;
	virtual int GetScrollPos( int orient ) const = 0;
	virtual void AutoSize( bool setAsMin = true ) = 0;
	virtual wxPoint CalcUnscrolledPosition(const wxPoint& pt) const = 0;
	virtual bool MoveCursorRight( bool expandSelection ) = 0;
	virtual bool ClearSelection( bool send_event = false ) = 0;
	virtual wxWindow* GetGridRowLabelWindow() = 0;
	virtual void CaptureKeyEvents( wxObjectEventFunction func, wxEvtHandler *eventSink ) = 0;
	virtual void SetDefaultCellAlignment( int horiz, int vert ) = 0;
	virtual void ConnectCellLeftClick( wxObjectEventFunction func, wxEvtHandler *eventSink ) = 0;
	virtual void ConnectCellLeftDClick( wxObjectEventFunction func, wxEvtHandler *eventSink ) = 0;
	virtual void ConnectCellRightClick( wxObjectEventFunction func, wxEvtHandler *eventSink ) = 0;
	virtual void ConnectLabelRightClick( wxObjectEventFunction func, wxEvtHandler *eventSink ) = 0;
	virtual void ConnectSelectCell( wxObjectEventFunction func, wxEvtHandler *eventSink ) = 0;
	virtual void ConnectCellChange( wxObjectEventFunction func, wxEvtHandler *eventSink ) = 0;
	virtual int GetSelectedCellsCount() = 0;
	virtual int GetSelectedColCount() = 0;
	virtual int GetSelectedRowCount() = 0;
	virtual int GetSelectedColNumber() = 0;
	virtual int GetSelectedRowNumber() = 0;
	virtual SSelectionCellCords GetSelectionBlockTopLeft() = 0;
	virtual SSelectionCellCords GetSelectionBlockBottomRight() = 0;
	virtual void MakeCellVisible( int row, int col ) = 0;
	virtual void AutoSizeColumn( int col ) = 0;
	virtual SCellCords GetSize( int row, int col ) = 0;
	virtual void SetSize( int row, int col, int numRows, int numCols ) = 0;
};


class CSpawnSetGridGrid : public CSpawnSetGrid, public wxGrid
{
public:
	CSpawnSetGridGrid( wxWindow *parent, wxWindowID id )
		: wxGrid( parent, id )
		, m_mainRowPen( wxColour(0uL)/*wxLIGHT_GREY->GetPixel()*/, 3 )
	{}

	virtual wxPen GetRowGridLinePen( Int32 row );
	virtual void SetMainRows( const TSet< Int32 > &rows )
	{
		m_mainRows = rows;
	}

	virtual void SetCellValue( int row, int col, const wxString &val )
	{
		wxGrid::SetCellValue( row, col, val );
	}
	virtual void SetCellValue( const wxString &val, int row, int col )
	{
		ASSERT( 0 && TXT("Don't use this method.") );
	}
	virtual int GetCursorRow()
	{
		return wxGrid::GetCursorRow();
	}
	virtual int GetCursorColumn()
	{
		return wxGrid::GetCursorColumn();
	}
	virtual wxString GetCellValue( int row, int col )
	{
		return wxGrid::GetCellValue( row, col );
	}
	virtual void SetGridCursor( int row, int col )
	{
		wxGrid::SetGridCursor( row, col );
	}
	virtual wxRect CellToRect( int row, int col )
	{
		return wxGrid::CellToRect( row, col );
	}
	virtual wxRect BlockToDeviceRect( int topLeftRow, int topLeftCol, int bottomRightRow, int bottomRightCol )
	{
		return wxGrid::BlockToDeviceRect( wxGridCellCoords( topLeftRow, topLeftCol ), wxGridCellCoords( bottomRightRow, bottomRightCol ) );
	}
	virtual int GetRowLabelSize()
	{
		return wxGrid::GetRowLabelSize();
	}
	virtual int GetColLabelSize()
	{
		return wxGrid::GetColLabelSize();
	}
	virtual bool IsReadOnly( int row, int col ) const
	{
		return wxGrid::IsReadOnly( row, col );
	}
	virtual void SetCellAlignment( int align, int row, int col )
	{
		wxGrid::SetCellAlignment( align, row, col );
	}
	virtual void SetCellAlignment( int row, int col, int horiz, int vert )
	{
		wxGrid::SetCellAlignment( row, col, horiz, vert );
	}
	virtual void SetReadOnly( int row, int col, bool isReadOnly = true )
	{
		wxGrid::SetReadOnly( row, col, isReadOnly );
	}
	virtual void SetCellBackgroundColour( int row, int col, const wxColour& color )
	{
		wxGrid::SetCellBackgroundColour( row, col, color );
	}
	virtual void ClearGrid()
	{
		wxGrid::ClearGrid();
	}
	virtual wxColour GetCellBackgroundColour( int row, int col )
	{
		return wxGrid::GetCellBackgroundColour( row, col );
	}
	virtual int YToRow( int y )
	{
		return wxGrid::YToRow( y );
	}
	virtual int XToCol( int x, bool clipToMinMax = false )
	{
		return wxGrid::XToCol( x, clipToMinMax );
	}
	virtual void SetCellEditorNumber( int row, int col )
	{
		wxGrid::SetCellEditor( row, col, new wxGridCellNumberEditor() );
	}
	virtual void SetCellEditorFloat( int row, int col )
	{
		wxGrid::SetCellEditor( row, col, new wxGridCellFloatEditor() );
	}
	virtual void SetCellEditorBool( int row, int col )
	{
		wxGrid::SetCellEditor( row, col, new wxGridCellBoolEditor() );
	}
	virtual void SetCellEditorChoice( int row, int col )
	{
		wxGrid::SetCellEditor( row, col, new CGridCellChoiceEditor() );
	}
	virtual void SetCellEditorText( int row, int col )
	{
		wxGrid::SetCellEditor( row, col, new wxGridCellTextEditor() );
	}
	virtual void SetCellRendererNumber( int row, int col )
	{
		wxGrid::SetCellRenderer( row, col, new wxGridCellNumberRenderer() );
	}
	virtual void SetCellRendererFloat( int row, int col )
	{
		wxGrid::SetCellRenderer( row, col, new wxGridCellFloatRenderer() );
	}
	virtual void SetCellRendererBool( int row, int col )
	{
		wxGrid::SetCellRenderer( row, col, new wxGridCellBoolRenderer() );
	}
	virtual void SetCellRendererText( int row, int col )
	{
		wxGrid::SetCellRenderer( row, col, new wxGridCellStringRenderer() );
	}
	virtual void SetCellRendererTemplate( int row, int col )
	{
		wxGrid::SetCellRenderer( row, col, new CTemplateCellRenderer() );
	}
	virtual void SetCellRendererComment( int row, int col )
	{
		wxGrid::SetCellRenderer( row, col, new CCommentCellRenderer() );
		wxGrid::SetCellBackgroundColour( wxColour( 230, 230, 0 ), row, col );
	}
	virtual wxWindow* GetWindow()
	{
		return this;
	}
	virtual wxWindow* GetGridWindow()
	{
		return wxGrid::GetGridWindow();
	}
	virtual void Connect( int eventType, wxObjectEventFunction func, wxObject *userData = (wxObject *) NULL, wxEvtHandler *eventSink = (wxEvtHandler *) NULL )
	{
		wxGrid::Connect( eventType, func, userData, eventSink );
	}
	virtual bool CreateGrid( int numRows, int numCols )
	{
		return wxGrid::CreateGrid( numRows, numCols );
	}
	virtual void DisableDragGridSize()
	{
		wxGrid::DisableDragGridSize();
	}
	virtual void DisableDragColSize()
	{
		wxGrid::DisableDragColSize();
	}
	virtual void DisableDragRowSize()
	{
		wxGrid::DisableDragRowSize();
	}
	virtual void SetAutoLayout( bool autoLayout )
	{
		wxGrid::SetAutoLayout( autoLayout );
	}
	virtual int GetNumberCols()
	{
		return wxGrid::GetNumberCols();
	}
	virtual int GetNumberRows()
	{
		return wxGrid::GetNumberRows();
	}
	virtual void ForceRefresh()
	{
		wxGrid::ForceRefresh();
	}
	virtual bool InsertRows( size_t row, size_t numRows )
	{
		return wxGrid::InsertRows( row, numRows );
	}
	virtual bool DeleteRows( size_t row, size_t numRows )
	{
		return wxGrid::DeleteRows( row, numRows );
	}
	virtual bool AppendRows( size_t numRows )
	{
		return wxGrid::AppendRows( numRows );
	}
	virtual void SetColLabelValue( int col, const wxString& value )
	{
		wxGrid::SetColLabelValue( col, value );
	}
	virtual void Refresh()
	{
		wxGrid::Refresh();
	}
	virtual void SetRowLabelValue( int row, const wxString& value )
	{
		wxGrid::SetRowLabelValue( row, value );
	}
	virtual int GetScrollPos( int orient ) const
	{
		return wxGrid::GetScrollPos( orient );
	}
	virtual void AutoSize( bool setAsMin = true )
	{
		wxGrid::AutoSize();
	}
	virtual wxPoint CalcUnscrolledPosition(const wxPoint& pt) const
	{
		return wxGrid::CalcUnscrolledPosition( pt );
	}
	virtual bool MoveCursorRight( bool expandSelection )
	{
		return wxGrid::MoveCursorRight( expandSelection );
	}
	virtual bool ClearSelection( bool send_event = false )
	{
		wxGrid::ClearSelection();
		return true;
	}
	virtual wxWindow* GetGridRowLabelWindow()
	{
		return wxGrid::GetGridRowLabelWindow();
	}
	virtual void CaptureKeyEvents( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxGrid::GetEventHandler()->Connect( wxEVT_KEY_DOWN, func, NULL, eventSink );
	}
	virtual void SetDefaultCellAlignment( int horiz, int vert )
	{
		wxGrid::SetDefaultCellAlignment( horiz, vert );
	}
	virtual void ConnectCellLeftClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxGrid::Connect( wxEVT_GRID_CELL_LEFT_CLICK, func, NULL, eventSink );
	}
	virtual void ConnectCellLeftDClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxGrid::Connect( wxEVT_GRID_CELL_LEFT_DCLICK, func, NULL, eventSink );
	}
	virtual void ConnectCellRightClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxGrid::Connect( wxEVT_GRID_CELL_RIGHT_CLICK, func, NULL, eventSink );
	}
	virtual void ConnectLabelRightClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxGrid::Connect( wxEVT_GRID_LABEL_RIGHT_CLICK, func, NULL, eventSink );
	}
	virtual void ConnectSelectCell( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxGrid::Connect( wxEVT_GRID_SELECT_CELL, func, NULL, eventSink );
	}
	virtual void ConnectCellChange( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxGrid::Connect( wxEVT_GRID_CELL_CHANGE, func, NULL, eventSink );
	}
	virtual int GetSelectedCellsCount()
	{
		return wxGrid::GetSelectedCells().Count();
	}
	virtual int GetSelectedColCount()
	{
		return wxGrid::GetSelectedCols().Count();
	}
	virtual int GetSelectedRowCount()
	{
		return wxGrid::GetSelectedRows().Count();
	}
	virtual int GetSelectedColNumber()
	{
		return wxGrid::GetSelectedCols()[0];
	}
	virtual int GetSelectedRowNumber()
	{
		return wxGrid::GetSelectedRows()[0];
	}
	virtual SSelectionCellCords GetSelectionBlockTopLeft()
	{
		SSelectionCellCords cords;
		wxGridCellCoordsArray topLeft = wxGrid::GetSelectionBlockTopLeft();
		cords.m_count = topLeft.GetCount();
		if ( cords.m_count > 0 )
		{
			cords.m_col = topLeft[0].GetCol();
			cords.m_row = topLeft[0].GetRow();
		}
		else
		{
			cords.m_col = -1;
			cords.m_row = -1;
		}
		return cords;
	}
	virtual SSelectionCellCords GetSelectionBlockBottomRight()
	{
		SSelectionCellCords cords;
		wxGridCellCoordsArray bottomRight = wxGrid::GetSelectionBlockBottomRight();
		cords.m_count = bottomRight.GetCount();
		if ( cords.m_count > 0 )
		{
			cords.m_col = bottomRight[0].GetCol();
			cords.m_row = bottomRight[0].GetRow();
		}
		else
		{
			cords.m_col = -1;
			cords.m_row = -1;
		}
		return cords;
	}
	virtual void MakeCellVisible( int row, int col )
	{
		wxGrid::MakeCellVisible( row, col );
	}
	virtual void AutoSizeColumn( int col )
	{
		wxGrid::AutoSizeColumn( col );
	}
	virtual void SetSize( int row, int col, int numRows, int numCols )
	{
		//wxGridCellAttr *cellAttr = GetCellAttr( row, col );
		////cellAttr->IncRef();
		//cellAttr->SetSize( numRows, numCols );
		//wxGrid::SetAttr( row, col, cellAttr );

		wxGrid::SetCellSize( row, col, numRows, numCols );
	}
	virtual SCellCords GetSize( int row, int col )
	{
		SCellCords cellSize;
		GetCellAttr( row, col )->GetSize( &cellSize.m_row, &cellSize.m_col );
		return cellSize;
	}

private:
	TSet< Int32 > m_mainRows;
	wxPen		m_mainRowPen;
};


class CSpawnSetGridSheet : public CSpawnSetGrid, public wxSheet
{
public:
	CSpawnSetGridSheet( wxWindow *parent, wxWindowID id )
		: wxSheet( parent, id )
		, m_mainRowPen( wxColour(0uL)/*wxLIGHT_GREY->GetPixel()*/, 3 )
	{}

	virtual wxPen GetRowGridLinePen( Int32 row );
	virtual void SetMainRows( const TSet< Int32 > &rows )
	{
		m_mainRows = rows;
	}

	virtual void SetCellValue( int row, int col, const wxString &val )
	{
		wxSheet::SetCellValue( wxSheetCoords(row, col), val );
	}
	virtual void SetCellValue( const wxString &val, int row, int col )
	{
		ASSERT( 0 && TXT("Don't use this method.") );
	}
	virtual int GetCursorRow()
	{
		return wxSheet::GetGridCursorRow();
	}
	virtual int GetCursorColumn()
	{
		return wxSheet::GetGridCursorCol();
	}
	virtual wxString GetCellValue( int row, int col )
	{
		return wxSheet::GetCellValue( wxSheetCoords(row, col) );
	}
	virtual void SetGridCursor( int row, int col )
	{
		wxSheet::SetGridCursorCell( wxSheetCoords( row, col ) );
	}
	virtual wxRect CellToRect( int row, int col )
	{
		return wxSheet::CellToRect( wxSheetCoords(row, col) );
	}
	virtual wxRect BlockToDeviceRect( int topLeftRow, int topLeftCol, int bottomRightRow, int bottomRightCol )
	{
		return wxSheet::BlockToDeviceRect( wxSheetBlock( wxSheetCoords( topLeftRow, topLeftCol ), wxSheetCoords( bottomRightRow, bottomRightCol ) ) );
	}
	virtual int GetRowLabelSize()
	{
		return wxSheet::GetRowLabelWidth();
	}
	virtual int GetColLabelSize()
	{
		return wxSheet::GetColLabelHeight();
	}
	virtual bool IsReadOnly(int row, int col) const
	{
		return wxSheet::GetAttrReadOnly( wxSheetCoords(row,col) );
	}
	virtual void SetCellAlignment( int align, int row, int col )
	{
		// TOCHECK: the align parameter should be the same for wxGrid and wxSheet, but check this once again
		wxSheet::SetAttrAlignment( wxSheetCoords(row,col), align );
	}
	virtual void SetCellAlignment( int row, int col, int horiz, int vert )
	{
		// TODO
	}
	virtual void SetReadOnly( int row, int col, bool isReadOnly = true )
	{
		wxSheet::SetAttrReadOnly( wxSheetCoords(row,col), isReadOnly );
	}
	virtual void SetCellBackgroundColour( int row, int col, const wxColour& color )
	{
		wxSheet::SetAttrBackgroundColour( wxSheetCoords(row,col), color );
	}
	virtual void ClearGrid()
	{
		wxSheet::ClearValues();
	}
	virtual wxColour GetCellBackgroundColour( int row, int col )
	{
		return wxSheet::GetAttrBackgroundColour( wxSheetCoords(row,col) );
	}
	virtual int YToRow( int y )
	{
		return wxSheet::YToGridRow( y );
	}
	virtual int XToCol( int x, bool clipToMinMax = false )
	{
		return wxSheet::XToGridCol( x, clipToMinMax );
	}
	virtual void SetCellEditorNumber( int row, int col )
	{
		wxSheet::SetAttrEditor( wxSheetCoords(row, col), wxSheetCellEditor( new wxSheetCellNumberEditorRefData() ) );
	}
	virtual void SetCellEditorFloat( int row, int col )
	{
		wxSheet::SetAttrEditor( wxSheetCoords(row, col), wxSheetCellEditor( new wxSheetCellFloatEditorRefData() ) );
	}
	virtual void SetCellEditorBool( int row, int col )
	{
		wxSheet::SetAttrEditor( wxSheetCoords(row, col), wxSheetCellEditor( new wxSheetCellBoolEditorRefData() ) );
	}
	virtual void SetCellEditorChoice( int row, int col )
	{
		wxSheet::SetAttrEditor( wxSheetCoords(row, col), wxSheetCellEditor( new wxSheetCellChoiceEditorRefData() ) );
	}
	virtual void SetCellEditorText( int row, int col )
	{
		wxSheet::SetAttrEditor( wxSheetCoords(row, col), wxSheetCellEditor( new wxSheetCellAutoWrapStringEditorRefData() ) );
	}
	virtual void SetCellRendererNumber( int row, int col )
	{
		wxSheet::SetAttrRenderer( wxSheetCoords(row,col), wxSheetCellRenderer( new wxSheetCellNumberRendererRefData() ) );
	}
	virtual void SetCellRendererFloat( int row, int col )
	{
		wxSheet::SetAttrRenderer( wxSheetCoords(row,col), wxSheetCellRenderer( new wxSheetCellFloatRendererRefData() ) );
	}
	virtual void SetCellRendererBool( int row, int col )
	{
		wxSheet::SetAttrRenderer( wxSheetCoords(row,col), wxSheetCellRenderer( new wxSheetCellBoolRendererRefData() ) );
	}
	virtual void SetCellRendererText( int row, int col )
	{
		// wxSheetCellAutoWrapStringRendererRefData
		wxSheet::SetAttrRenderer( wxSheetCoords(row,col), wxSheetCellRenderer( new wxSheetCellStringRendererRefData() ) );
	}
	virtual void SetCellRendererTemplate( int row, int col )
	{
		// TODO
	}
	virtual void SetCellRendererComment( int row, int col )
	{
		// TODO
	}
	virtual wxWindow* GetWindow()
	{
		return this;
	}
	virtual bool CreateGrid( int numRows, int numCols )
	{
		return wxSheet::CreateGrid( numRows, numCols );
	}
	virtual void DisableDragGridSize()
	{
		wxSheet::DisableDragGridSize();
	}
	virtual void DisableDragColSize()
	{
		wxSheet::DisableDragColSize();
	}
	virtual void DisableDragRowSize()
	{
		wxSheet::DisableDragRowSize();
	}
	virtual void SetAutoLayout( bool autoLayout )
	{
		wxSheet::SetAutoLayout( autoLayout );
	}
	virtual wxWindow* GetGridWindow()
	{
		return wxSheet::GetGridWindow();
	}
	virtual void Connect( int eventType, wxObjectEventFunction func, wxObject *userData = (wxObject *) NULL, wxEvtHandler *eventSink = (wxEvtHandler *) NULL )
	{
		wxSheet::Connect( eventType, func, userData, eventSink );
	}
	virtual int GetNumberRows()
	{
		return wxSheet::GetNumberRows();
	}
	virtual int GetNumberCols()
	{
		return wxSheet::GetNumberCols();
	}
	virtual void ForceRefresh()
	{
		wxSheet::ForceRefresh();
	}
	virtual bool InsertRows( size_t row, size_t numRows )
	{
		return wxSheet::InsertRows( row, numRows );
	}
	virtual bool DeleteRows( size_t row, size_t numRows )
	{
		return wxSheet::DeleteRows( row, numRows );
	}
	virtual bool AppendRows( size_t numRows )
	{
		return wxSheet::AppendRows( numRows );
	}
	virtual void SetColLabelValue( int col, const wxString& value )
	{
		wxSheet::SetColLabelValue( col, value );
	}
	virtual void Refresh()
	{
		wxSheet::Refresh();
	}
	virtual void SetRowLabelValue( int row, const wxString& value )
	{
		wxSheet::SetRowLabelValue( row, value );
	}
	virtual int GetScrollPos( int orient ) const
	{
		return wxSheet::GetScrollPos( orient );
	}
	virtual void AutoSize( bool setAsMin = true )
	{
		wxSheet::AutoSize( setAsMin );
	}
	virtual wxPoint CalcUnscrolledPosition(const wxPoint& pt) const
	{
		return wxSheet::CalcUnscrolledPosition( pt );
	}
	virtual bool MoveCursorRight( bool expandSelection )
	{
		return wxSheet::MoveCursorRight( expandSelection );
	}
	virtual bool ClearSelection( bool send_event = false )
	{
		return wxSheet::ClearSelection( send_event );
	}
	virtual wxWindow* GetGridRowLabelWindow()
	{
		return wxSheet::GetRowLabelWindow();
	}
	virtual void CaptureKeyEvents( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxSheet::GetEventHandler()->Connect( wxEVT_KEY_DOWN, func, NULL, eventSink );
	}
	virtual void SetDefaultCellAlignment( int horiz, int vert )
	{
		wxSheetCellAttr defaultCellAttr = wxSheet::GetDefaultGridCellAttr();
		defaultCellAttr.SetAlignment( horiz, vert );
		wxSheet::SetDefaultGridCellAttr( defaultCellAttr );
	}
	virtual void ConnectCellLeftClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		// TODO
	}
	virtual void ConnectCellLeftDClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		// TODO
	}
	virtual void ConnectCellRightClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		// TODO
	}
	virtual void ConnectLabelRightClick( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		// TODO
	}
	virtual void ConnectSelectCell( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		// TODO
	}
	virtual void ConnectCellChange( wxObjectEventFunction func, wxEvtHandler *eventSink )
	{
		wxSheet::Connect( wxEVT_SHEET_CELL_VALUE_CHANGED, func, NULL, eventSink );
	}
	virtual int GetSelectedCellsCount()
	{
		// TODO
		return 0;
	}
	virtual void MakeCellVisible( int row, int col )
	{
		wxSheet::MakeCellVisible( wxSheetCoords(row,col) );
	}
	virtual void AutoSizeColumn( int col )
	{
		wxSheet::AutoSizeCol( col );
	}
	virtual int GetSelectedColCount()
	{
		// TODO
		return -1;
	}
	virtual int GetSelectedRowCount()
	{
		// TODO
		return -1;
	}
	virtual int GetSelectedColNumber()
	{
		// TODO
		return -1;
	}
	virtual int GetSelectedRowNumber()
	{
		// TODO
		return -1;
	}
	virtual SSelectionCellCords GetSelectionBlockTopLeft()
	{
		// TODO
		return SSelectionCellCords();
	}
	virtual SSelectionCellCords GetSelectionBlockBottomRight()
	{
		// TODO
		return SSelectionCellCords();
	}
	virtual SCellCords GetSize( int row, int col )
	{
		// TODO
		return SCellCords();
	}
	virtual void SetSize( int row, int col, int numRows, int numCols )
	{
		// TODO
	}

private:
	TSet< Int32 > m_mainRows;
	wxPen		m_mainRowPen;
};
