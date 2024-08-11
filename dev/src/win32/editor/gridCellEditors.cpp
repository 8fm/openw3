#include "build.h"
#include "gridCellEditors.h"
#include "gridOutline.h"
#include "gridPropertyWrapper.h"
#include "gridEditor.h"

#include "../../common/core/depot.h"

void IGridCellOutlineRenderer::DrawOutline( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
	ASSERT(dynamic_cast<COutlineGrid *>(&grid) != NULL);
	COutlineGrid &outlineGrid = *(COutlineGrid *)&grid;
	Int32 outline = outlineGrid.GetCellOutline(row, col);
	if (outline != Outline_None)
	{
		wxColour color;
		Int32 thickness;
		wxPen pen;
		pen.SetWidth(1);

		if (outline & Outline_Left)
		{
			outlineGrid.GetCellOutlineStyle(row, col, Outline_Left, color, thickness);
			thickness = min(4, thickness);
			pen.SetColour(color);
			dc.SetPen(pen);
			for (Int32 i = 0; i < thickness; ++i)
				dc.DrawLine(rect.GetLeft() + i, rect.GetTop(), rect.GetLeft() + i, rect.GetBottom() + 1);
		}

		if (outline & Outline_Right)
		{
			outlineGrid.GetCellOutlineStyle(row, col, Outline_Right, color, thickness);
			thickness = min(4, thickness);
			pen.SetColour(color);
			dc.SetPen(pen);
			for (Int32 i = 0; i < thickness; ++i)
				dc.DrawLine(rect.GetRight() - i, rect.GetTop(), rect.GetRight() - i, rect.GetBottom() + 1);
		}

		if (outline & Outline_Top)
		{
			outlineGrid.GetCellOutlineStyle(row, col, Outline_Top, color, thickness);
			thickness = min(4, thickness);
			pen.SetColour(color);
			dc.SetPen(pen);
			for (Int32 i = 0; i < thickness; ++i)
				dc.DrawLine(rect.GetLeft(), rect.GetTop() + i, rect.GetRight() + 1, rect.GetTop() + i);
		}


		if (outline & Outline_Bottom)
		{
			outlineGrid.GetCellOutlineStyle(row, col, Outline_Bottom, color, thickness);
			thickness = min(4, thickness);
			pen.SetColour(color);
			dc.SetPen(pen);
			for (Int32 i = 0; i < thickness; ++i)
				dc.DrawLine(rect.GetLeft(), rect.GetBottom() - i, rect.GetRight() + 1, rect.GetBottom() - i);
		}
	}

	wxRect range = outlineGrid.GetHighlightRange();
	if (!range.IsEmpty())
	{
		const Int32 thickness = 2;
		const Int32 arrowHeadSize = 5;
		wxPen highlightPen(wxColour(255, 0, 0), 1);

		// Draw marker at separator column
		if (col == range.GetLeft() && range.GetTop() <= row && row <= range.GetBottom())
		{
			dc.SetPen(highlightPen);

			for (Int32 i = 0; i < thickness; ++i)
				dc.DrawLine(rect.GetLeft() + i, rect.GetTop(), rect.GetLeft() + i, rect.GetBottom() + 1);

			if (row == range.GetTop())
			{
				for (Int32 i = 0; i < arrowHeadSize; ++i)
					dc.DrawLine(rect.GetLeft(), rect.GetTop() + i, rect.GetLeft() + arrowHeadSize - i, rect.GetTop() + i);
			}

			if (row == range.GetBottom())
			{
				for (Int32 i = 0; i < arrowHeadSize; ++i)
					dc.DrawLine(rect.GetLeft(), rect.GetBottom() - i, rect.GetLeft() + arrowHeadSize - i, rect.GetBottom() - i);
			}
		}

		// Draw highlighted outline
		if (range.GetLeft() < col && col <= range.GetRight() && range.GetTop() <= row && row <= range.GetBottom())
		{
			dc.SetPen(highlightPen);
			if (range.GetLeft() + 1 == col)
			{
				for (Int32 i = 0; i < thickness; ++i)
					dc.DrawLine(rect.GetLeft() + i, rect.GetTop(), rect.GetLeft() + i, rect.GetBottom() + 1);
			}

			if (range.GetRight() == col)
			{
				for (Int32 i = 0; i < thickness; ++i)
					dc.DrawLine(rect.GetRight() - i, rect.GetTop(), rect.GetRight() - i, rect.GetBottom() + 1);
			}

			if (range.GetTop() == row)
			{
				for (Int32 i = 0; i < thickness; ++i)
					dc.DrawLine(rect.GetLeft(), rect.GetTop() + i, rect.GetRight() + 1, rect.GetTop() + i);
			}

			if (range.GetBottom() == row)
			{
				for (Int32 i = 0; i < thickness; ++i)
					dc.DrawLine(rect.GetLeft(), rect.GetBottom() - i, rect.GetRight() + 1, rect.GetBottom() - i);
			}
		}
	}
}

wxString CGridCellDefaultRenderer::GetText( wxGrid& grid, int row, int col )
{
	return grid.GetCellValue(row, col);
}

wxSize CGridCellDefaultRenderer::GetBestSize(wxGrid& grid,
				   wxGridCellAttr& attr,
				   wxDC& dc,
				   int row, int col)
{
	return DoGetBestSize(attr, dc, GetText( grid, row, col ) );
}

void CGridCellDefaultRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rectCell, int row, int col, bool isSelected )
{
	{
		wxRect rect = rectCell;
		rect.Inflate(-1);

		// erase only this cells background, overflow cells should have been erased
		wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

		int hAlign, vAlign;
		attr.GetAlignment(&hAlign, &vAlign);

		int overflowCols = 0;

		if (attr.GetOverflow())
		{
			int cols = grid.GetNumberCols();
			int best_width = GetBestSize(grid,attr,dc,row,col).GetWidth();
			int cell_rows, cell_cols;
			attr.GetSize( &cell_rows, &cell_cols ); // shouldn't get here if <= 0
			if ((best_width > rectCell.width) && (col < cols) && grid.GetTable())
			{
				int i, c_cols, c_rows;
				for (i = col+cell_cols; i < cols; i++)
				{
					bool is_empty = true;
					for (int j=row; j < row + cell_rows; j++)
					{
						// check w/ anchor cell for multicell block
						grid.GetCellSize(j, i, &c_rows, &c_cols);
						if (c_rows > 0)
							c_rows = 0;
						if (!grid.GetTable()->IsEmptyCell(j + c_rows, i))
						{
							is_empty = false;
							break;
						}
					}

					if (is_empty)
					{
						rect.width += grid.GetColSize(i);
					}
					else
					{
						i--;
						break;
					}

					if (rect.width >= best_width)
						break;
				}

				overflowCols = i - col - cell_cols + 1;
				if (overflowCols >= cols)
					overflowCols = cols - 1;
			}

			if (overflowCols > 0) // redraw overflow cells w/ proper hilight
			{
				hAlign = wxALIGN_LEFT; // if oveflowed then it's left aligned
				wxRect clip = rect;
				clip.x += rectCell.width;
				// draw each overflow cell individually
				int col_end = col + cell_cols + overflowCols;
				if (col_end >= grid.GetNumberCols())
					col_end = grid.GetNumberCols() - 1;
				for (int i = col + cell_cols; i <= col_end; i++)
				{
					clip.width = grid.GetColSize(i) - 1;
					dc.DestroyClippingRegion();
					dc.SetClippingRegion(clip);

					SetTextColoursAndFont(grid, attr, dc,
						grid.IsInSelection(row,i));

					grid.DrawTextRectangle(dc, grid.GetCellValue(row, col),
						rect, hAlign, vAlign);
					clip.x += grid.GetColSize(i) - 1;
				}

				rect = rectCell;
				rect.Inflate(-1);
				rect.width++;
				dc.DestroyClippingRegion();
			}
		}

		// now we only have to draw the text
		SetTextColoursAndFont(grid, attr, dc, isSelected);

		grid.DrawTextRectangle(dc, GetText( grid, row, col ), rect, hAlign, vAlign);
	}

	IGridCellOutlineRenderer::DrawOutline( grid, attr, dc, rectCell, row, col, isSelected );
}

void CGridCellEmptyRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
	//wxColour backgroundColor = grid.GetBackgroundColour();
	wxColour backgroundColor = grid.GetCellBackgroundColour(row, col);
	wxBrush backBrush(backgroundColor);
	wxPen backPen(backgroundColor, 0);
	dc.SetBrush(backBrush);
	dc.SetPen(backPen);
	dc.DrawRectangle(rect);
	IGridCellOutlineRenderer::DrawOutline( grid, attr, dc, rect, row, col, isSelected );
}

void CGridCellSeparatorRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
	const wxColour backgroundColor = grid.GetCellBackgroundColour(row, col);

	wxBrush backBrush(backgroundColor);
	wxPen backPen(backgroundColor, 0);
	dc.SetBrush(backBrush);
	dc.SetPen(backPen);
	dc.DrawRectangle(rect);

	// Draw expandable cells
	int size = min(rect.GetWidth(), rect.GetHeight()) - 6;
	size = max(7, size);

	wxPen whitePen(wxColour(255, 255, 255, 255), 1);
	dc.SetPen(whitePen);
	wxString value = grid.GetCellValue(row, col);
	if (value.Cmp(TXT("-")) == 0)
	{
		wxRect rc(rect.GetLeft() + (rect.GetWidth() - size) / 2, rect.GetTop() + (rect.GetHeight() - size) / 2, size, size);
		dc.DrawRectangle(rc);
		rc = rc.Deflate(wxSize(2, 2));
		dc.DrawLine(rc.GetLeft(), rc.GetTop() + rc.GetHeight() / 2, rc.GetRight() + 1, rc.GetTop() + rc.GetHeight() / 2);
	}
	else if (value.Cmp(TXT("+")) == 0)
	{
		wxRect rc(rect.GetLeft() + (rect.GetWidth() - size) / 2, rect.GetTop() + (rect.GetHeight() - size) / 2, size, size);
		dc.DrawRectangle(rc);
		rc = rc.Deflate(wxSize(2, 2));
		dc.DrawLine(rc.GetLeft(), rc.GetTop() + rc.GetHeight() / 2, rc.GetRight() + 1, rc.GetTop() + rc.GetHeight() / 2);
		dc.DrawLine(rc.GetLeft() + rc.GetWidth() / 2, rc.GetTop(), rc.GetLeft() + rc.GetWidth() / 2, rc.GetBottom() + 1);
	}
	else if (value.Cmp(TXT("0")) == 0)
	{
		wxRect rc(rect.GetLeft() + (rect.GetWidth() - size) / 2, rect.GetTop() + (rect.GetHeight() - size) / 2, size, size);
		dc.DrawRectangle(rc);
	}

	IGridCellOutlineRenderer::DrawOutline( grid, attr, dc, rect, row, col, isSelected );
}

void CGridCellBoolRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
	const wxColour backColor = isSelected ? grid.GetSelectionBackground() : grid.GetCellBackgroundColour(row, col);

	wxBrush backBrush(backColor);
	wxPen backPen(backColor, 0);
	dc.SetBrush(backBrush);
	dc.SetPen(backPen);
	dc.DrawRectangle(rect);

	// Draw check box
	wxBrush checkBoxBrush(wxColour(255, 255, 255));
	wxPen checkBoxPen(wxColour(28, 81, 128), 0);
	dc.SetBrush(checkBoxBrush);
	dc.SetPen(checkBoxPen);
	const Int32 size = 13;
	wxRect rc(rect.x + (rect.width - size) / 2, rect.y + (rect.height - size) / 2, size, size);
	dc.DrawRectangle(rc);

	// Draw tick
	if (grid.GetCellValue(row, col) == L"true")
	{
		wxPen tickPen;
		tickPen.SetColour(wxColour(33, 161, 33));
		tickPen.SetWidth(1);
		dc.SetPen(tickPen);
		Int32 x = rc.x + 3;
		Int32 y = rc.y + 5;
		dc.DrawLine(x, y, x, y + 3); x++; y++;
		dc.DrawLine(x, y, x, y + 3); x++; y++;
		dc.DrawLine(x, y, x, y + 3); x++; y--;
		dc.DrawLine(x, y, x, y + 3); x++; y--;
		dc.DrawLine(x, y, x, y + 3); x++; y--;
		dc.DrawLine(x, y, x, y + 3); x++; y--;
		dc.DrawLine(x, y, x, y + 3);
	}

	IGridCellOutlineRenderer::DrawOutline( grid, attr, dc, rect, row, col, isSelected );
}

wxSize CGridCellBoolRenderer::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
{
	const Int32 width = grid.GetColSize( col );
	const Int32 height = grid.GetRowHeight( row );
	const Int32 size = min( width, height );
	return wxSize( size, size );
}

void CGridCellFloatRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
	wxGridCellFloatRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
	IGridCellOutlineRenderer::DrawOutline(grid, attr, dc, rect, row, col, isSelected);
}

void CGridCellNumberRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
	wxGridCellNumberRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
	IGridCellOutlineRenderer::DrawOutline(grid, attr, dc, rect, row, col, isSelected);
}

//////////////////////////////////////////////////////////////////////////

CResourceControl::CResourceControl( wxWindow *parent, wxWindowID id, const wxString &extension )
: wxControl(parent, id)
, m_extension(extension)
{
    m_text = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

    const Int32 iconSize = CGridCellResourceRenderer::ICON_SIZE;
    wxBitmap iconBrowse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_BROWSE") );
    m_bitmapBrowse = new wxStaticBitmap(this, wxID_ANY, iconBrowse, wxDefaultPosition, wxSize(iconSize, iconSize), wxBORDER_NONE);
    m_bitmapBrowse->Connect(wxEVT_LEFT_DOWN, wxObjectEventFunction(&CResourceControl::OnBrowseLeftDown), NULL, this);

    wxBitmap iconUse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
    m_bitmapUse = new wxStaticBitmap(this, wxID_ANY, iconUse, wxDefaultPosition, wxSize(iconSize, iconSize), wxBORDER_NONE);
    m_bitmapUse->Connect(wxEVT_LEFT_DOWN, wxObjectEventFunction(&CResourceControl::OnUseLeftDown), NULL, this);

    wxBitmap iconClear = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
    m_bitmapClear = new wxStaticBitmap(this, wxID_ANY, iconClear, wxDefaultPosition, wxSize(iconSize, iconSize), wxBORDER_NONE);
    m_bitmapClear->Connect(wxEVT_LEFT_DOWN, wxObjectEventFunction(&CResourceControl::OnClearLeftDown), NULL, this);
}

wxString CResourceControl::GetValue() const
{
    return m_text->GetLabel();
}

void CResourceControl::SetValue( const wxString &text )
{
    m_text->SetLabel(text);
}

void CResourceControl::DoSetClientSize( Int32 width, Int32 height )
{
    wxControl::DoSetClientSize(width, height);

    const wxRect rect( 0, 0, width, height );
    const wxSize browseSize = m_bitmapBrowse->GetSize();
    const wxSize useSize = m_bitmapUse->GetSize();
    const wxSize clearSize = m_bitmapClear->GetSize();
    const Int32 buttonsWidth = browseSize.GetWidth() + useSize.GetWidth() + clearSize.GetWidth();

    wxRect textRect( 0, 0, rect.GetWidth() - buttonsWidth, rect.GetHeight() );
    textRect.Deflate( 3, 2 );
    m_text->SetSize( textRect );

    const Int32 browseY = (textRect.GetHeight() - browseSize.GetHeight()) / 2;
    const Int32 useY = (textRect.GetHeight() - useSize.GetHeight()) / 2;
    const Int32 clearY = (textRect.GetHeight() - clearSize.GetHeight()) / 2;
    m_bitmapBrowse->SetPosition(wxPoint(rect.GetWidth() - buttonsWidth, browseY));
    m_bitmapUse->SetPosition(wxPoint(rect.GetWidth() - useSize.GetWidth() - clearSize.GetWidth(), useY));
    m_bitmapClear->SetPosition(wxPoint(rect.GetWidth() - clearSize.GetWidth(), clearY));
}

void CResourceControl::DoSetSize(Int32 x, Int32 y, Int32 width, Int32 height, Int32 sizeFlags)
{
    wxControl::DoSetSize(x, y, width, height, sizeFlags);
    DoSetClientSize(width, height);
}

void CResourceControl::OnBrowseLeftDown( wxMouseEvent& event )
{
    String depotPath = m_text->GetLabel().wc_str();
    SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
}

void CResourceControl::OnUseLeftDown( wxMouseEvent& event )
{
    String selectedResource;
    if (GetActiveResource(selectedResource))
    {
        if (selectedResource.EndsWith(m_extension.wc_str()))
        {
			m_text->SetLabel( TXT("Loading...") );
            CResource *res = GDepot->LoadResource(selectedResource);
            if (res)
            {
                m_text->SetLabel(res->GetFile()->GetDepotPath().AsChar());
            }
            else
            {
				m_text->SetLabel( TXT("Error on loading.") );
                WARN_EDITOR(TXT("A resource shouldn't be null."));
            }
        }
        else
        {
            WARN_EDITOR(TXT("Selected resource doesn't match the filter '%s'"), m_extension);
        }
    }
}

void CResourceControl::OnClearLeftDown( wxMouseEvent& event )
{
    m_text->SetLabel( TXT("None") );
}

//////////////////////////////////////////////////////////////////////////

CGridCellResourceEditor::CGridCellResourceEditor( const wxString &extension )
: wxGridCellEditor()
, m_extension( extension )
{
}

void CGridCellResourceEditor::Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler )
{
    m_control = new CResourceControl( parent, wxID_ANY, m_extension );
    m_control->SetWindowStyle( wxBORDER_NONE );
    wxGridCellEditor::Create( parent, id, evtHandler );
}

void CGridCellResourceEditor::SetSize(const wxRect& rect)
{
    if (IsCreated())
    {
        m_control->SetSize( rect );
    }
}

void CGridCellResourceEditor::BeginEdit( int row, int col, wxGrid* grid )
{
    wxString value = grid->GetCellValue(row, col);
    if (value.IsEmpty() || value.CmpNoCase(TXT("NULL")) == 0)
        value = TXT("None");

    ((CResourceControl *)m_control)->SetValue(value);
    m_control->SetFocus();
}

bool CGridCellResourceEditor::EndEdit(int row, int col, const wxGrid *grid,
	const wxString& oldval, wxString *newval)
{
    wxString ctrlValue = ((CResourceControl *)m_control)->GetValue();
    wxString cellValue = grid->GetCellValue(row, col);

    if (ctrlValue == TXT("None"))
    {
        ctrlValue = TXT("NULL");
    }

    if (ctrlValue != cellValue)
    {
		*newval = ctrlValue;
		m_newValue = ctrlValue;
        return true;
    }

    return false;
}

void CGridCellResourceEditor::ApplyEdit(int row, int col, wxGrid* grid)
{
	grid->SetCellValue( row, col, m_newValue );
}

void CGridCellResourceEditor::Reset()
{
    if (CResourceControl *resourceControl = dynamic_cast<CResourceControl *>(m_control))
    {
        resourceControl->SetLabel(TXT("None"));
    }
}

wxString CGridCellResourceEditor::GetValue() const
{
    if (CResourceControl *resourceControl = dynamic_cast<CResourceControl *>(m_control))
    {
        return resourceControl->GetValue();
    }

    return wxEmptyString;
}

CGridCellResourceRenderer::CGridCellResourceRenderer()
: wxGridCellStringRenderer()
{
    m_iconUse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
    m_iconBrowse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_BROWSE") );
    m_iconClear = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
}

void CGridCellResourceRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
    const wxColour backColor = isSelected ? grid.GetSelectionBackground() : attr.GetBackgroundColour();
    wxBrush backBrush(backColor);
    wxPen backPen(backColor);

    wxRect textRect(rect.GetLeft(), rect.GetTop(), rect.GetWidth() - 3 * ICON_SIZE, rect.GetHeight());

    wxString value = grid.GetCellValue(row, col);
    if (value.IsEmpty() || value.CmpNoCase(TXT("NULL")) == 0)
    {
        dc.SetBrush(backBrush);
        dc.SetPen(backPen);
        dc.DrawRectangle(rect);

        //if (attr.HasTextColour())
        dc.SetTextForeground(attr.GetTextColour());
        //if (attr.HasBackgroundColour())
        dc.SetTextBackground(attr.GetBackgroundColour());
        //if (attr.HasFont())
        dc.SetFont(attr.GetFont());
        Int32 horizontalAlignment;
        Int32 verticalAlignment;
        attr.GetAlignment(&horizontalAlignment, &verticalAlignment);
        grid.DrawTextRectangle(dc, TXT("None"), textRect, horizontalAlignment, verticalAlignment);
    }
    else
    {
        dc.SetBrush(backBrush);
        dc.SetPen(backPen);
        dc.DrawRectangle(textRect.GetRight() + 1, rect.GetTop(), rect.GetWidth() - textRect.GetWidth(), rect.GetHeight());
        wxGridCellStringRenderer::Draw( grid, attr, dc, textRect, row, col, isSelected );
    }

    if ( grid.GetGridCursorRow() == row && grid.GetGridCursorCol() == col && grid.GetCellValue( row, col ) != wxEmptyString )
    {
        // Draw icons
        Int32 browseX = rect.GetRight() - 3 * ICON_SIZE + (ICON_SIZE - m_iconBrowse.GetWidth()) / 2;
        Int32 browseY = rect.GetTop() + (rect.GetHeight() - m_iconBrowse.GetHeight()) / 2;
        dc.DrawBitmap( m_iconBrowse, browseX, browseY, true );

        Int32 useX = rect.GetRight() - 2 * ICON_SIZE + (ICON_SIZE - m_iconUse.GetWidth()) / 2;
        Int32 useY = rect.GetTop() + (rect.GetHeight() - m_iconUse.GetHeight()) / 2;
        dc.DrawBitmap( m_iconUse, useX, useY, true );

        Int32 clearX = rect.GetRight() - 1 * ICON_SIZE + (ICON_SIZE - m_iconClear.GetWidth()) / 2;
        Int32 clearY = rect.GetTop() + (rect.GetHeight() - m_iconClear.GetHeight()) / 2;
        dc.DrawBitmap( m_iconClear, clearX, clearY, true );
    }

    IGridCellOutlineRenderer::DrawOutline(grid, attr, dc, rect, row, col, isSelected);
}

wxSize CGridCellResourceRenderer::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
{
    wxSize size = wxGridCellStringRenderer::GetBestSize( grid, attr, dc, row, col );
    size.SetWidth( size.GetWidth() + ICON_SIZE * 3 );
    return size;
}

///////////////////////////////////////////////////////////////////
CGridCellObjectEditor::CGridCellObjectEditor()
: wxGridCellEditor()
{
}

void CGridCellObjectEditor::Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler )
{
	m_control = new wxTextCtrl( parent, wxID_ANY );
	m_control->SetWindowStyle( wxBORDER_NONE );
	wxGridCellEditor::Create( parent, id, evtHandler );
}

void CGridCellObjectEditor::SetSize(const wxRect& rect)
{
	if (IsCreated())
	{
		m_control->SetSize( rect );
	}
}

void CGridCellObjectEditor::BeginEdit( int row, int col, wxGrid* grid )
{
	CGridEditor* gridEditor = static_cast< CGridEditor* >( grid );
	Int32 rowRel=row;
	Int32 colRel=col;
	CObject* obj = NULL;
	if ( CGridPropertyWrapper *wrapper = gridEditor->GetRelativeCoords( rowRel, colRel ) )
	{
		SGridCellDescriptor desc;
		wrapper->GetCellDescriptor( rowRel, colRel, desc );
		if( desc.cellType )
		{
			ASSERT( desc.cellType->GetType() == RT_Pointer );
			if( desc.cellType->GetType() == RT_Pointer )
			{
				CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( desc.cellType );
				obj = (CObject*)pointerType->GetPointed( desc.cellData );		
			}
		}
	}

	if( obj )
	{
		gridEditor->ShowObjectEditor( obj );
	}
}

bool CGridCellObjectEditor::EndEdit(int row, int col, const wxGrid *grid, const wxString& oldval, wxString *newval)
{
	//CGridEditor* gridEditor = static_cast< CGridEditor* >( grid );
	//gridEditor->HideObjectEditor();

	return true;
}

void CGridCellObjectEditor::ApplyEdit(int row, int col, wxGrid* grid)
{

}

void CGridCellObjectEditor::Reset()
{
	m_control->SetLabel( wxEmptyString );
}

wxString CGridCellObjectEditor::GetValue() const
{
	ASSERT( 0 );
	return wxEmptyString;
}

CGridCellObjectRenderer::CGridCellObjectRenderer()
: wxGridCellStringRenderer()
{
}

void CGridCellObjectRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
	wxGridCellStringRenderer::Draw( grid, attr, dc, rect, row, col, isSelected );	
	IGridCellOutlineRenderer::DrawOutline(grid, attr, dc, rect, row, col, isSelected);
}

wxSize CGridCellObjectRenderer::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
{
	return wxGridCellStringRenderer::GetBestSize( grid, attr, dc, row, col );
}
