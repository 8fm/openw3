#include "build.h"
#include "gridOutline.h"
#include "gridCustomCellEditors.h"
#include "sceneExplorer.h"

#include "tagEditor.h"

#include "../../common/core/depot.h"

CGridCellTagEditor::CGridCellTagEditor()
: wxGridCellEditor()
, m_bitmapUse(NULL)
{

}

CGridCellTagEditor::~CGridCellTagEditor()
{
	Destroy();
}

void CGridCellTagEditor::Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler)
{
	m_control = new wxTextCtrl(parent, wxID_ANY);
	wxGridCellEditor::Create(parent, id, evtHandler);

	wxBitmap iconUse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
	m_bitmapUse = new wxStaticBitmap
	(
		parent,
		wxID_ANY,
		iconUse,
		wxDefaultPosition,
		wxSize
		(
			CGridCellResourceRenderer::ICON_SIZE,
			CGridCellResourceRenderer::ICON_SIZE
		),
		wxBORDER_NONE
	);

	m_bitmapUse->Bind( wxEVT_LEFT_DOWN, &CGridCellTagEditor::OnOpenTagEditorExt, this );
	m_bitmapUse->Hide();
}

void CGridCellTagEditor::Destroy()
{
	wxGridCellEditor::Destroy();
}

void CGridCellTagEditor::SetSize(const wxRect& rect)
{
	if (IsCreated())
	{
		if ( m_bitmapUse->IsShown() )
		{
			wxRect rectText = rect;
			rectText.width -= CGridCellResourceRenderer::ICON_SIZE;
			
			m_control->SetMinSize(rectText.GetSize());
			m_control->SetSize(rectText);

			m_bitmapUse->SetPosition( rectText.GetTopRight() );
		}
		else
		{
			m_control->SetMinSize(rect.GetSize());
			m_control->SetSize(rect);
		}
	}
}

void CGridCellTagEditor::BeginEdit(int row, int col, wxGrid* grid)
{
	m_bitmapUse->Show();

	if ( wxTextCtrl *editCtrl = wxDynamicCast( m_control, wxTextCtrl ) )
	{
		wxString value = grid->GetCellValue(row, col);

		TagList tagList;
		tagList.FromString( value.wc_str() );

		if( !tagList.Empty() )
		{
			String noBrackets = tagList.ToString( false );
			noBrackets = noBrackets.MidString( 1, noBrackets.GetLength() - 2 );

			editCtrl->ChangeValue( noBrackets.AsChar() );
		}

		SetSize( editCtrl->GetRect() );
	}
}

bool CGridCellTagEditor::EndEdit(int row, int col, const wxGrid *grid, const wxString& oldval, wxString *newval)
{
	bool retValue = false;

	if ( wxTextCtrl *editCtrl = wxDynamicCast( m_control, wxTextCtrl ) )
	{
		wxString ctrlValue = editCtrl->GetValue();

		TagList tagList;
		tagList.FromString( editCtrl->GetValue().wc_str() );

		if( !tagList.Empty() )
		{
			*newval = tagList.ToString().AsChar();
			m_newValue = *newval;
		}
		else
		{
			m_newValue.Clear();
		}

		retValue = true;
	}

	m_bitmapUse->Hide();

	return retValue;
}

void CGridCellTagEditor::ApplyEdit(int row, int col, wxGrid* grid)
{
	grid->SetCellValue( row, col, m_newValue );
}

void CGridCellTagEditor::Reset()
{
	if ( wxTextCtrl *editCtrl = wxDynamicCast( m_control, wxTextCtrl ) )
	{
		editCtrl->SetValue( wxEmptyString );
	}
}

wxString CGridCellTagEditor::GetValue() const
{
	if ( wxTextCtrl *editCtrl = wxDynamicCast( m_control, wxTextCtrl ) )
	{
		return editCtrl->GetValue();
	}

	return wxEmptyString;
}

void CGridCellTagEditor::OnOpenTagEditorExt( wxMouseEvent& event )
{
	if( !m_tagEditorExt )
	{
		if ( wxTextCtrl *editCtrl = wxDynamicCast( m_control, wxTextCtrl ) )
		{
			TagList tagList;
			tagList.FromString( editCtrl->GetValue().wc_str() );

			// Open tag editor
			m_tagEditorExt = new CEdTagEditor( editCtrl->GetParent(), tagList );
			m_tagEditorExt->Bind( wxEVT_TAGEDITOR_OK, &CGridCellTagEditor::OnTagEditorExtOK, this );
			m_tagEditorExt->Bind( wxEVT_TAGEDITOR_CANCEL, &CGridCellTagEditor::OnTagEditorExtCancelled, this );

			m_tagEditorExt->ShowModal();
		}
	}
}

void CGridCellTagEditor::OnTagEditorExtOK( wxCommandEvent &event )
{
	ASSERT( m_tagEditorExt );
	// Grab tags
	if ( m_tagEditorExt )
	{
		// Update tags
		TagList tagList;

		// Set tags
		tagList.SetTags( m_tagEditorExt->GetTags() );

		if ( wxTextCtrl *editCtrl = wxDynamicCast( m_control, wxTextCtrl ) )
		{
			editCtrl->SetValue( tagList.ToString().AsChar() );
		}

		// Unlink tag editor
		m_tagEditorExt = NULL;
	}
}

void CGridCellTagEditor::OnTagEditorExtCancelled( wxCommandEvent &event )
{
	// Unlink tag editor
	m_tagEditorExt = NULL;
}

//////////////////////////////////////////////////////////////////////////

CGridStringsArrayEditor::CGridStringsArrayEditor( const TDynArray< String >& choices )
: m_choices( choices )
, m_ctrl( NULL )
{

}

CGridStringsArrayEditor::~CGridStringsArrayEditor()
{
	Destroy();
}

void CGridStringsArrayEditor::Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler )
{
	ASSERT( m_ctrl == NULL );

	wxArrayString choices;
	Uint32 count = m_choices.Size();
	choices.push_back( wxT("") );
	for ( Uint32 i = 0; i < count; ++i )
	{
		choices.push_back( m_choices[ i ].AsChar() );
	}

	m_ctrl = new wxListBox( parent, id, wxDefaultPosition, wxDefaultSize, choices, wxLB_SINGLE ); 
	m_ctrl->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrl->SetFocus();

	m_ctrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED,
		wxCommandEventHandler( CGridStringsArrayEditor::OnItemActivated ), NULL, this );

	m_control = m_ctrl;
	wxGridCellEditor::Create( parent, id, evtHandler );
}

void CGridStringsArrayEditor::OnItemActivated( wxCommandEvent& event )
{
	if ( m_ctrl != NULL && m_ctrl->GetParent() != NULL )
	{
		m_ctrl->GetParent()->SetFocus();
	}
}

void CGridStringsArrayEditor::Destroy()
{
	wxGridCellEditor::Destroy();
}

void CGridStringsArrayEditor::SetSize( const wxRect& rect )
{
	ASSERT( m_ctrl );

	wxRect newSize = rect;
	newSize.SetHeight( rect.GetHeight() * 10 );
	m_ctrl->SetSize( newSize );
}

void CGridStringsArrayEditor::BeginEdit( int row, int col, wxGrid* grid )
{
	ASSERT( m_ctrl );

	// show the widget
	m_ctrl->Show();
}

bool CGridStringsArrayEditor::EndEdit(int row, int col, const wxGrid *grid,
	const wxString& oldval, wxString *newval)
{
	ASSERT( m_ctrl );

	m_newValue = GetValue();
	*newval = m_newValue;

	m_ctrl->Show( false );

	return true;
}

void CGridStringsArrayEditor::ApplyEdit(int row, int col, wxGrid* grid)
{
	grid->SetCellValue( row, col, m_newValue );
}

void CGridStringsArrayEditor::Reset()
{
	ASSERT( m_ctrl );

	m_ctrl->Clear();
}

wxString CGridStringsArrayEditor::GetValue() const
{
	ASSERT( m_ctrl );

	Int32 selectionIdx = m_ctrl->GetSelection();
	String val = String::EMPTY;
	if ( selectionIdx != -1 )
	{
		val = m_ctrl->GetString( selectionIdx );
	}
	return wxString( val.AsChar() );
}

//////////////////////////////////////////////////////////////////////////

CLayerNameControl::CLayerNameControl(wxWindow *parent, wxWindowID id)
: wxControl(parent, id)
{
	m_text = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

	const Int32 iconSize = CGridCellResourceRenderer::ICON_SIZE;
	
	wxBitmap iconUse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
	m_bitmapUse = new wxStaticBitmap(this, wxID_ANY, iconUse, wxDefaultPosition, wxSize(iconSize, iconSize), wxBORDER_NONE);
	m_bitmapUse->Connect(wxEVT_LEFT_DOWN, wxObjectEventFunction(&CLayerNameControl::OnUseLeftDown), NULL, this);

	wxBitmap iconClear = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
	m_bitmapClear = new wxStaticBitmap(this, wxID_ANY, iconClear, wxDefaultPosition, wxSize(iconSize, iconSize), wxBORDER_NONE);
	m_bitmapClear->Connect(wxEVT_LEFT_DOWN, wxObjectEventFunction(&CLayerNameControl::OnClearLeftDown), NULL, this);
}

wxString CLayerNameControl::GetValue() const
{
	return m_text->GetLabel();
}

void CLayerNameControl::SetValue(const wxString &text)
{
	m_text->SetLabel(text);
}

void CLayerNameControl::DoSetClientSize(Int32 width, Int32 height)
{
	wxControl::DoSetClientSize( width, height );

	const wxRect rect( 0, 0, width, height );
	const wxSize useSize = m_bitmapUse->GetSize();
	const wxSize clearSize = m_bitmapClear->GetSize();
	const Int32 buttonsWidth = useSize.GetWidth() + clearSize.GetWidth();

	wxRect textRect( 0, 0, rect.GetWidth() - buttonsWidth, rect.GetHeight() );
	textRect.Deflate( 3, 2 );
	m_text->SetSize( textRect );

	const Int32 useY = (textRect.GetHeight() - useSize.GetHeight()) / 2;
	const Int32 clearY = (textRect.GetHeight() - clearSize.GetHeight()) / 2;
	m_bitmapUse->SetPosition( wxPoint(rect.GetWidth() - useSize.GetWidth() - clearSize.GetWidth(), useY) );
	m_bitmapClear->SetPosition( wxPoint(rect.GetWidth() - clearSize.GetWidth(), clearY) );
}

void CLayerNameControl::DoSetSize(Int32 x, Int32 y, Int32 width, Int32 height, Int32 sizeFlags /*= wxSIZE_AUTO*/)
{
	wxControl::DoSetSize(x, y, width, height, sizeFlags);
	DoSetClientSize(width, height);
}

//! Returns relative path between the closest world file (.w2w) and given path
Bool GetRelativePathToResource( const String &path, String &relativePath )
{
    CDiskFile *file = GDepot->FindFile( path );
    if ( !file )
    {
        return false;
    }
    
    String tempPath = file->GetFileName();
    CDirectory *dir = file->GetDirectory();
    Bool worldFound = false;
    while( dir )
    {
        const TFiles & filesMap = dir->GetFiles();
        TFiles::const_iterator it;
        for ( it = filesMap.Begin(); it != filesMap.End(); ++it )
        {
            if ( (*it)->GetFileName().EndsWith( TXT(".w2w") ) )
            {
                relativePath = tempPath;
                return true;
            }
        }

        tempPath = dir->GetName() + TXT("\\") + tempPath;
        dir = dir->GetParent();
    }

    return false;
}

Bool GetContainingWorldPath( const String &path, String &relativeWorldPath )
{
	CDiskFile *file = GDepot->FindFile( path );
	if ( !file )
	{
		return false;
	}

	CDirectory *dir = file->GetDirectory();
	Bool worldFound = false;
	while( dir )
	{
		const TFiles & filesMap = dir->GetFiles();
		TFiles::const_iterator it;
		for ( it = filesMap.Begin(); it != filesMap.End(); ++it )
		{
			if ( (*it)->GetFileName().EndsWith( TXT(".w2w") ) )
			{
				relativeWorldPath = (*it)->GetDepotPath();
				return true;
			}
		}		
		dir = dir->GetParent();
	}

	return false;
}

void CLayerNameControl::OnUseLeftDown(wxMouseEvent& event)
{
    // First check is any layer is selected in the Asset Browser
    String resourcePath;
    if ( GetActiveResource( resourcePath ) )
    {
        if ( resourcePath.EndsWith( TXT(".w2l") ) )
        {
            String relativePath;
            if ( GetRelativePathToResource( resourcePath, relativePath ) )
            {
                // Remove an extension
                String leftPart;
                if ( relativePath.Split( TXT(".w2l"), &leftPart, NULL ) )
                {
                    relativePath = leftPart;
                }

				String worldPath;
				if( GetContainingWorldPath( resourcePath, worldPath ) )
				{
					m_text->SetLabel( ( relativePath + TXT(";") + worldPath ).AsChar() ); 
				}
				else
				{
					m_text->SetLabel( relativePath.AsChar() );        
				}
				
                return;
            }
        }
    }

    // then check currently selected layer in the scene
	if ( wxTheFrame && wxTheFrame->GetWorldEditPanel() )
	{
		if ( CLayer *activeLayer = wxTheFrame->GetSceneExplorer()->GetActiveLayer() )
		{
			if ( CLayerInfo *layerInfo = activeLayer->GetLayerInfo() )
			{
                String path;
                layerInfo->GetHierarchyPath( path, true );				                

				path = path + TXT(";") + layerInfo->GetWorld()->GetDepotPath( );
				String leftPart;
				path.Split( TXT(".w2w"), &leftPart, NULL );				
								
				m_text->SetLabel( leftPart.AsChar() );				

                return;
			}
		}
	}
}

void CLayerNameControl::OnClearLeftDown(wxMouseEvent& event)
{
	m_text->SetLabel( wxEmptyString );
}

CGridCellLayerNameEditor::CGridCellLayerNameEditor()
: wxGridCellEditor()
{

}

void CGridCellLayerNameEditor::Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler)
{
	m_control = new CLayerNameControl( parent, wxID_ANY );
	m_control->SetWindowStyle( wxBORDER_NONE );
	wxGridCellEditor::Create( parent, id, evtHandler );
}

void CGridCellLayerNameEditor::SetSize(const wxRect& rect)
{
	if (IsCreated())
	{
		m_control->SetSize( rect );
	}
}

void CGridCellLayerNameEditor::BeginEdit(int row, int col, wxGrid* grid)
{
	wxString value = grid->GetCellValue(row, col);
	((CLayerNameControl *)m_control)->SetValue(value);
	m_control->SetFocus();
}

bool CGridCellLayerNameEditor::EndEdit(int row, int col, const wxGrid *grid,
	const wxString& oldval, wxString *newval)
{
	wxString ctrlValue = ((CLayerNameControl *)m_control)->GetValue();
	wxString cellValue = grid->GetCellValue(row, col);

	if (ctrlValue != cellValue)
	{
		*newval = ctrlValue;
		m_newValue = ctrlValue;
		return true;
	}

	return false;
}

void CGridCellLayerNameEditor::ApplyEdit(int row, int col, wxGrid* grid)
{
	grid->SetCellValue( row, col, m_newValue );
}

void CGridCellLayerNameEditor::Reset()
{
	if (CLayerNameControl *layerNameControl = dynamic_cast<CLayerNameControl *>(m_control))
	{
		layerNameControl->SetLabel( wxEmptyString );
	}
}

wxString CGridCellLayerNameEditor::GetValue() const
{
	if (CLayerNameControl *layerNameControl = dynamic_cast<CLayerNameControl *>(m_control))
	{
		return layerNameControl->GetValue();
	}

	return wxEmptyString;
}

CGridCellLayerNameRenderer::CGridCellLayerNameRenderer()
: wxGridCellStringRenderer()
{
    m_iconUse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
    m_iconClear = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
}

void CGridCellLayerNameRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected )
{
    const wxColour backColor = isSelected ? grid.GetSelectionBackground() : attr.GetBackgroundColour();
    wxBrush backBrush(backColor);
    wxPen backPen(backColor);

    wxString value = grid.GetCellValue(row, col);
    wxRect textRect(rect.GetLeft(), rect.GetTop(), rect.GetWidth() - 2 * CGridCellResourceRenderer::ICON_SIZE, rect.GetHeight());
    dc.SetBrush(backBrush);
    dc.SetPen(backPen);
    dc.DrawRectangle(textRect.GetRight() + 1, rect.GetTop(), rect.GetWidth() - textRect.GetWidth(), rect.GetHeight());
    wxGridCellStringRenderer::Draw( grid, attr, dc, textRect, row, col, isSelected );

    if ( grid.GetGridCursorRow() == row && grid.GetGridCursorCol() == col && grid.GetCellValue( row, col ) != wxEmptyString )
    {
        // Draw icons
        Int32 useX = rect.GetRight() - 2 * CGridCellResourceRenderer::ICON_SIZE + (CGridCellResourceRenderer::ICON_SIZE - m_iconUse.GetWidth()) / 2;
        Int32 useY = rect.GetTop() + (rect.GetHeight() - m_iconUse.GetHeight()) / 2;
        dc.DrawBitmap( m_iconUse, useX, useY, true );

        Int32 clearX = rect.GetRight() - 1 * CGridCellResourceRenderer::ICON_SIZE + (CGridCellResourceRenderer::ICON_SIZE - m_iconClear.GetWidth()) / 2;
        Int32 clearY = rect.GetTop() + (rect.GetHeight() - m_iconClear.GetHeight()) / 2;
        dc.DrawBitmap( m_iconClear, clearX, clearY, true );
    }

    IGridCellOutlineRenderer::DrawOutline(grid, attr, dc, rect, row, col, isSelected);
}

wxSize CGridCellLayerNameRenderer::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
{
    wxSize size = wxGridCellStringRenderer::GetBestSize( grid, attr, dc, row, col );
    size.SetWidth( size.GetWidth() + 2 * CGridCellResourceRenderer::ICON_SIZE );
    return size;
}

//////////////////////////////////////////////////////////////////////////

CGridCellChoiceEditor::CGridCellChoiceEditor(const wxArrayString& choices, bool allowOthers)
	: wxGridCellEditor()
	, m_choices( choices )
	, m_allowOthers( m_allowOthers )
{
}

void CGridCellChoiceEditor::Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler)
{
	m_control = new CEdChoice( parent, wxDefaultPosition, wxDefaultSize, m_allowOthers );
	m_control->SetId( id );

	// fill the combobox with the choices using a single Append call inside
	// a Freeze/Thaw pair
	m_control->Freeze();
	((CEdChoice*)m_control)->Append( m_choices );
	m_control->Thaw();

	// skip wxGridCellChoiceEditor's Create method and call wxGridCellEditor's one
	wxGridCellEditor::Create(parent, id, evtHandler);
}

void CGridCellChoiceEditor::SetSize( const wxRect& rect )
{
	wxRect rectTallEnough = rect;
	wxSize bestSize = m_control->GetBestSize();
	wxCoord diffY = bestSize.GetHeight() - rectTallEnough.GetHeight();

	if ( diffY > 0 )
	{
		rectTallEnough.height += diffY;
		rectTallEnough.y -= diffY/2;
	}

	wxGridCellEditor::SetSize( rect );
}

void CGridCellChoiceEditor::BeginEdit(int row, int col, wxGrid* grid)
{
	wxGridCellEditorEvtHandler* handler = NULL;

	if ( m_control )
	{
		handler = wxDynamicCast( m_control->GetEventHandler(), wxGridCellEditorEvtHandler );
	}

	if ( handler )
	{
		handler->SetInSetFocus( true );
	}

	m_value = grid->GetTable()->GetValue( row, col );
	Reset();

	Combo()->SetValue( m_value );

	// open the drop down menu if we can only make specific choices
	if ( !m_allowOthers )
		Combo()->ShowPopup();
}

bool CGridCellChoiceEditor::EndEdit(int row, int col, const wxGrid* grid, const wxString& WXUNUSED(oldval), wxString *newval)
{
	wxString value = GetValue();
			
	if ( value == m_value )
		return false;

	m_value = value;

	grid->GetTable()->SetValue(row, col, value);

	if ( newval )
	{
		*newval = value;
	}

	return true;
}

void CGridCellChoiceEditor::ApplyEdit(int row, int col, wxGrid* grid)
{
	grid->GetTable()->SetValue( row, col, m_value );
}

void CGridCellChoiceEditor::Reset()
{
	int pos = Combo()->FindString( m_value );
	if (pos == wxNOT_FOUND)
	{
		Combo()->SetValue( m_value );
	}
	else
	{
		Combo()->SetSelection( pos );
	}
	m_control->SetFocus();
}

wxString CGridCellChoiceEditor::GetValue() const
{
	return Combo()->GetValue();
}
