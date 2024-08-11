/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#if 0

#include "spawnSetEditor.h"
#include "spawnSetActionEditor.h"
#include "tagEditor.h"
#include "tagViewEditor.h"
#include "assetBrowser.h"
#include "appearanceTagListUpdater.h"

// Event table
BEGIN_EVENT_TABLE( CEdSpawnSetEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "editCut" ), CEdSpawnSetEditor::OnEditCut )
	EVT_MENU( XRCID( "editCopy" ), CEdSpawnSetEditor::OnEditCopy )
	EVT_MENU( XRCID( "editPaste" ), CEdSpawnSetEditor::OnEditPaste )
	EVT_MENU( XRCID( "menuItemSave" ), CEdSpawnSetEditor::OnSave )
	EVT_MENU( XRCID( "menuItemExit" ), CEdSpawnSetEditor::OnExit )

	EVT_MOUSE_CAPTURE_LOST( CEdSpawnSetEditor::OnMouseCaptureLost )
END_EVENT_TABLE()

//RED_DEFINE_STATIC_NAME( FileReloadConfirm )
//RED_DEFINE_STATIC_NAME( FileReload )
//RED_DEFINE_STATIC_NAME( FileReloadToConfirm )
//RED_DEFINE_STATIC_NAME( SelectAsset )
//RED_DEFINE_STATIC_NAME( Comment )
RED_DEFINE_STATIC_NAMED_NAME( PointerToCEntityTemplate, "*CEntityTemplate" );

CEdSpawnSetEditor::CEdSpawnSetEditor( wxWindow* parent, CNPCSpawnSet* spawnSet )
	: wxSmartLayoutPanel( parent, TXT("SpawnSetEditor"), false )
	, m_spawnSet( spawnSet )
	, m_tagEditor( NULL )
	, m_actionEditor( NULL )
	, m_disabledCellColor( wxLIGHT_GREY->GetPixel() )
	, m_omitCellChange( false )
{
	// Add reference to resource
	m_spawnSet->AddToRootSet();

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "propertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		m_properties = new CEdPropertiesBrowserWithStatusbar( rp );
		//m_properties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdSpawnSetEditor::OnPropertiesChanged ), NULL, this );
		//m_properties->Connect( wxEVT_COMMAND_REFRESH_PROPERTIES, wxCommandEventHandler( CEdSpawnSetEditor::OnPropertiesRefresh ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND );
		rp->SetSizer( sizer1 );
		rp->Layout();

		// Read properties
		TDynArray< CNPCSpawnSet* > blocks;
		blocks.PushBack( m_spawnSet );
		m_properties->SetObjects( (TDynArray< CObject* >&) blocks );
	}

	// Set title for newly created window
	String newTitle = m_spawnSet->GetFile()->GetFileName()
		+ TXT(" - ") + m_spawnSet->GetFile()->GetDepotPath()
		+ TXT(" - Spawn Set Editor");
	SetTitle( wxString(newTitle.AsChar()) );

	m_guiGridPanel = XRCCTRL( *this, "panelGrid", wxPanel );

	m_grid = new CSpawnSetGridGrid( m_guiGridPanel, wxID_ANY );
	wxBoxSizer* sizerGrid = new wxBoxSizer( wxVERTICAL );
	sizerGrid->Add( m_grid->GetWindow(), 1, wxEXPAND );
	m_guiGridPanel->SetSizer( sizerGrid );
	m_guiGridPanel->Layout();

	m_grid->CreateGrid( 1, COL_COUNT );
	m_grid->DisableDragGridSize();
	m_grid->DisableDragColSize();
	m_grid->DisableDragRowSize();
	m_grid->SetAutoLayout( true );
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );

	// Capture mouse cursor movement over grid for tooltips display
	m_grid->GetGridWindow()->Connect( wxEVT_MOTION, wxMouseEventHandler( CEdSpawnSetEditor::OnGridMotion ), NULL, this );
	m_grid->GetGridWindow()->GetToolTip()->SetDelay( TOOLTIP_DELAY );

	// Capture right mouse click on grid
	m_grid->GetGridWindow()->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CEdSpawnSetEditor::OnGridPanelRightMouseClick ), NULL, this );
	m_grid->GetGridRowLabelWindow()->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CEdSpawnSetEditor::OnGridPanelRightMouseClick ), NULL, this );
	m_grid->GetGridWindow()->Connect( wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler( CEdSpawnSetEditor::OnMouseCaptureLost ), NULL, this );
	m_grid->GetGridRowLabelWindow()->Connect( wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler( CEdSpawnSetEditor::OnMouseCaptureLost ), NULL, this );

	// Connect grid events
	m_grid->ConnectCellLeftClick( wxGridEventHandler(CEdSpawnSetEditor::OnCellLeftClick), this );
	m_grid->ConnectCellLeftDClick( wxGridEventHandler(CEdSpawnSetEditor::OnCellLeftDoubleClick), this );
	m_grid->ConnectCellRightClick( wxGridEventHandler(CEdSpawnSetEditor::OnCellRightClick), this );      // for context menu
	m_grid->ConnectLabelRightClick( wxGridEventHandler(CEdSpawnSetEditor::OnLabelRightClick), this );    // for context menu
	m_grid->ConnectSelectCell( wxGridEventHandler(CEdSpawnSetEditor::OnCellSelect), this );              // The user moved to, and selected a cell
	m_grid->ConnectCellChange( wxGridEventHandler(CEdSpawnSetEditor::OnCellChange), this );              // The user changed the data in a cell
	//m_grid->ConnectCellChange( wxSheetEventHandler(CEdSpawnSetEditor::OnCellChange), this ); // for wxSheet

	// Capture keyboard input
	m_grid->CaptureKeyEvents( wxKeyEventHandler( CEdSpawnSetEditor::OnGridKeyDown ), this );

	LoadDataFromResource();

	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( CEdSpawnSetEditor::OnGridSize ), NULL, this );

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	// Update and finalize layout
	Layout();
	LoadOptionsFromConfig();
	Show();
}

CEdSpawnSetEditor::~CEdSpawnSetEditor()
{
	SaveOptionsToConfig();

	// Remove resource reference
	m_spawnSet->RemoveFromRootSet();

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdSpawnSetEditor::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Frames/SpawnSetEditor"));
}

void CEdSpawnSetEditor::LoadOptionsFromConfig()
{
	LoadLayout(TXT("/Frames/SpawnSetEditor"));
}

wxString CEdSpawnSetEditor::GetShortTitle()
{
    return m_spawnSet->GetFile()->GetFileName().AsChar() + wxString(TXT(" - Spawn Set Editor"));
}

void CEdSpawnSetEditor::OnSave( wxCommandEvent& event )
{
	m_spawnSet->Save();
}

void CEdSpawnSetEditor::OnExit( wxCommandEvent& event )
{
	ClosePanel();
}

void CEdSpawnSetEditor::OnEditCopy( wxCommandEvent& event )
{
	if ( m_grid->GetSelectedCellsCount() > 0 )
	{
		// Do nothing... (copying singly selected cells is not supported)
	}
	else if ( m_grid->GetSelectedColCount() > 0 ) // copy columns
	{
		if ( m_grid->GetSelectedColCount() == 1 )
		{
			CopyCellsInColumn( m_grid->GetSelectedColNumber(), 0, m_grid->GetNumberRows() - 1 );
		}
		else
		{
			// Do nothing... (copying multiple columns is not supported)
		}
	}
	else if ( m_grid->GetSelectedRowCount() > 0 ) // copy rows
	{
		if ( m_grid->GetSelectedRowCount() == 1 )
		{
			CopyCellsInRow( m_grid->GetSelectedRowNumber(), 0, m_grid->GetNumberCols() - 1 );
		}
		else
		{
			// Do nothing... (copying multiple rows is not supported)
		}
	}
	else
	{
		CSpawnSetGrid::SSelectionCellCords topLeft = m_grid->GetSelectionBlockTopLeft();
		CSpawnSetGrid::SSelectionCellCords bottomRight = m_grid->GetSelectionBlockBottomRight();
		if ( topLeft.m_count == 1 && bottomRight.m_count == 1 )
		{
			int topLeftCol = topLeft.m_col;
			int topLeftRow = topLeft.m_row;
			int bottomRightCol = bottomRight.m_col;
			int bottomRightRow = bottomRight.m_row;

			if ( topLeftCol == bottomRightCol ) // cells in single column selected
			{
				CopyCellsInColumn( topLeftCol, topLeftRow, bottomRightRow );
			}
			else if ( topLeftRow == bottomRightRow ) // cells in single row selected
			{
				CopyCellsInRow( topLeftRow, topLeftCol, bottomRightCol );
			}
			else
			{
				// Copy cells in selected rectangle
				CopyCells( topLeftCol, topLeftRow, bottomRightCol, bottomRightRow );
			}
		}
		else if ( topLeft.m_count == 0 && bottomRight.m_count == 0 ) // nothing selected
		{
			// copy current cell

			const int cursorRow = m_grid->GetCursorRow();
			const int cursorCol = m_grid->GetCursorColumn();
			if ( cursorRow >= 0 && cursorCol >= 0 )
			{
				CopyCellsInColumn( cursorCol, cursorRow, cursorRow );
			}
		}
	}
}

void CEdSpawnSetEditor::OnEditCut( wxCommandEvent& event )
{}

void CEdSpawnSetEditor::OnEditPaste( wxCommandEvent& event )
{
	if ( wxTheClipboard->Open() )
	{
		const int cursorGridRow = m_grid->GetCursorRow();
		const int cursorGridCol = m_grid->GetCursorColumn();

		if ( wxTheClipboard->IsSupported( TXT("RECTANGLE") ) )
		{
			// receive data from clipboard
			CClipboardData data( TXT("RECTANGLE") );
			wxTheClipboard->GetData( data );
			SClipboardData *clipboardData = reinterpret_cast< SClipboardData* >( data.GetData().Data() );

			for ( int gridCol = cursorGridCol; gridCol < (int)clipboardData->m_data.Size() + cursorGridCol; ++gridCol )
			{
				for ( int gridRow = cursorGridRow; gridRow < (int)clipboardData->m_data[gridCol-cursorGridCol].Size() + cursorGridRow; ++gridRow )
				{
					const int col = gridCol - cursorGridCol;
					const int row = gridRow - cursorGridRow;

					if ( IsCellEnabled( gridRow, gridCol ) )
					{
						CVariant copyValue = clipboardData->m_data[ col ][ row ];
						if ( copyValue.GetType() == RED_NAME( TagList ) && ( GetCellType( gridRow, gridCol ) == RED_NAME( TagList ) ) )
						{
							TagList val;
							if ( clipboardData->m_data[ col ][ row ].AsType( val ) )
							{
								SetCellRawValue( gridRow, gridCol, &val );
							}
						}
						else if ( copyValue.GetType() == RED_NAME( PointerToCEntityTemplate ) && ( GetCellType( gridRow, gridCol ) == RED_NAME( PointerToCEntityTemplate ) ) )
						{
							CEntityTemplate *val;
							if ( clipboardData->m_data[ col ][ row ].AsType( val ) )
							{
								SetCellRawValue( gridRow, gridCol, val );
							}
						}
						else if ( copyValue.GetType() == RED_NAME( GameTime ) && ( GetCellType( gridRow, gridCol ) == RED_NAME( GameTime ) ) )
						{
							GameTime val;
							if ( clipboardData->m_data[ col ][ row ].AsType( val ) )
							{
								SetCellRawValue( gridRow, gridCol, &val );
							}
						}
						else if ( copyValue.GetType() == RED_NAME( Int32 ) && ( GetCellType( gridRow, gridCol ) == RED_NAME( Int32 ) ) )
						{
							Int32 val;
							if ( clipboardData->m_data[ col ][ row ].AsType( val ) )
							{
								SetCellRawValue( gridRow, gridCol, &val );
							}
						}
						else if ( copyValue.GetType() == RED_NAME( CName ) && ( GetCellType( gridRow, gridCol ) == RED_NAME( CName ) ) )
						{
							CName val;
							if ( clipboardData->m_data[ col ][ row ].AsType( val ) )
							{
								SetCellRawValue( gridRow, gridCol, &val );
							}
						}
						else if ( copyValue.GetType() == RED_NAME( Float ) && ( GetCellType( gridRow, gridCol ) == RED_NAME( Float ) ) )
						{
							Float val;
							if ( clipboardData->m_data[ col ][ row ].AsType( val ) )
							{
								SetCellRawValue( gridRow, gridCol, &val );
							}
						}
						else if ( copyValue.GetType() == RED_NAME( Bool ) && ( GetCellType( gridRow, gridCol ) == RED_NAME( Bool ) ) )
						{
							Bool val;
							if ( clipboardData->m_data[ col ][ row ].AsType( val ) )
							{
								SetCellRawValue( gridRow, gridCol, &val );
							}
						}
						else
						{
							//ASSERT( 0 && TXT("Copying of type %s not supported"), copyValue.GetType().AsChar() );
						}
					}
				}
			}

			RefreshGrid();
		}

		wxTheClipboard->Close();
	}
}

void CEdSpawnSetEditor::OnEditDelete( wxCommandEvent& event )
{}

void CEdSpawnSetEditor::OnEditUndo( wxCommandEvent& event )
{}

void CEdSpawnSetEditor::OnEditRedo( wxCommandEvent& event )
{}

void CEdSpawnSetEditor::OnInsertRow( wxCommandEvent& event )
{
	const Int32 cursorRow = GetGridMainRowNum( m_grid->GetCursorRow() );

	if ( cursorRow != -1 && IsRow( cursorRow ) )
	{
		CSpawnSetTimetable timetable;
		timetable.m_templates.PushBack( CSpawnSetTimetableTemplate() );
		m_spawnSet->GetTimetable().Insert( GetSpawnSetRowNum(cursorRow), timetable );

		const Int32 shift = IsRowComment( cursorRow-1 ) ? 1 : 0;
		InsertNewRow( cursorRow - shift );

		CalcGridRowLabelValues();
		SetMainRows();
	}
}

void CEdSpawnSetEditor::OnInsertCommentRow( wxCommandEvent& event )
{
	const Int32 cursorRow = GetGridMainRowNum( m_grid->GetCursorRow() );

	if ( cursorRow != -1 && IsRow( cursorRow ) && !IsRowComment( cursorRow-1 ) && !IsRowComment( m_grid->GetCursorRow() ) )
	{
		InsertNewRowComment( cursorRow );

		CalcGridRowLabelValues();
		SetMainRows();
	}
}

void CEdSpawnSetEditor::OnRemoveCommentRow( wxCommandEvent& event )
{
	const Int32 cursorRow = m_grid->GetCursorRow();
	const Int32 gridMainRowNum = GetGridMainRowNum( cursorRow );

	if ( gridMainRowNum != -1 && IsRow( gridMainRowNum ) )
	{
		// Remove comment from spawn set
		m_spawnSet->GetTimetable()[ GetSpawnSetRowNum( cursorRow ) ].m_comment = String::EMPTY;

		// Remove comment from grid
		if ( IsRowComment( gridMainRowNum-1 ) )
		{
			m_grid->DeleteRows( gridMainRowNum-1, 1 );
		}
		else if ( IsRowComment( cursorRow ) )
		{
			m_grid->DeleteRows( cursorRow, 1 );
		}

		// Refresh grid
		CalcGridRowLabelValues();
		SetMainRows();
	}
}

void CEdSpawnSetEditor::OnAppendRow( wxCommandEvent& event )
{
	AppendRow();
}

void CEdSpawnSetEditor::OnRemoveRow( wxCommandEvent& event )
{
	const Int32 cursorRow = GetGridMainRowNum( m_grid->GetCursorRow() );
	const Int32 ssRowNum = GetSpawnSetRowNum( cursorRow );

	if ( cursorRow != -1 && IsRow( cursorRow ) )
	{
		// Remove row from spawn set
		m_spawnSet->GetTimetable().Erase( m_spawnSet->GetTimetable().Begin() + ssRowNum );

		// Remove row and subrows from the grid
		Int32 row = cursorRow;
		Int32 rowsToDel = 0;
		while ( ++rowsToDel, IsSubRow( ++row ) );
		m_grid->DeleteRows( cursorRow, rowsToDel );

		// Remove comment
		if ( IsRowComment( cursorRow-1 ) )
		{
			m_grid->DeleteRows( cursorRow-1, 1 );
		}

		CalcGridRowLabelValues();
		SetMainRows();
	}
}

void CEdSpawnSetEditor::OnEditCell( wxCommandEvent& event )
{
	const Int32 cursorRow = m_grid->GetCursorRow();
	const Int32 cursorCol = m_grid->GetCursorColumn();

	RunCustomEditor( cursorRow, cursorCol );
}

void CEdSpawnSetEditor::OnClearCell( wxCommandEvent& event )
{
	const Int32 cursorRow = m_grid->GetCursorRow();
	const Int32 cursorCol = m_grid->GetCursorColumn();
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(cursorRow) ];

	switch ( cursorCol )
	{
	case COL_ENTITY_TEMPLATE:
		ClearCellTemplate( cursorRow, cursorCol );
		break;
	}
}

void CEdSpawnSetEditor::OnGotoResource( wxCommandEvent& event )
{
	const Int32 cursorRow = m_grid->GetCursorRow();
	const Int32 cursorCol = m_grid->GetCursorColumn();
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(cursorRow) ];

	switch ( cursorCol )
	{
	case COL_ENTITY_TEMPLATE:
		BrowseCellTemplate( cursorRow, cursorCol );
		break;
	}
}

void CEdSpawnSetEditor::OnCellSelect( wxGridEvent &event )
{
	const Int32 col = event.GetCol();
	const Int32 row = event.GetRow();

	switch ( col )
	{
		case COL_ENTITY_TEMPLATE:
			if ( IsCellEnabled( row, col ) )
			{
				m_grid->ForceRefresh();
			}
			break;
	}

	event.Skip();
}

void CEdSpawnSetEditor::CellChange( const int row, const int col )
{
	// To deal with constant recursive calls on grid refreshing
	if ( m_omitCellChange ) return;

	// save cell data to 'm_spawnSet' resource

	const Int32 ssRowNum = GetSpawnSetRowNum( row );
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ ssRowNum ];
	const String cellValue = m_grid->GetCellValue( row, col ); // Cell value

	switch ( col )
	{
	case COL_COMMENT:
		{
			if ( IsRowComment( row ) )
			{
				sst.m_comment = cellValue;
			}
			break;
		}
	case COL_ENTITY_WEIGHT:
		{
			Float val;
			if ( FromString( cellValue, val ) ) // is correct float value ?
			{
				sst.m_templates[GetSpawnSetSubRowNum( row )].m_weight = val;
			}
			else
			{
				// restore old value
				m_grid->SetCellValue( row, col, String::Printf( TXT("%.2f"), sst.m_templates[GetSpawnSetSubRowNum( row )].m_weight ).AsChar() );
			}

			break;
		}
	case COL_SPAWN_TIME_ARRAY:
		{
			SetCellGameTime( row, col, &sst.m_spawnArray[GetSpawnSetSubRowNum( row )].m_time );
			break;
		}
	case COL_SPAWN_QUANTITY_ARRAY:
		{
			Int32 val;
			if ( FromString( cellValue, val ) ) // is correct integer value ?
			{
				sst.m_spawnArray[GetSpawnSetSubRowNum( row )].m_quantity = val;
			}
			else
			{
				// restore old value
				m_grid->SetCellValue( row, col, String::Printf( TXT("%d"), sst.m_spawnArray[GetSpawnSetSubRowNum( row )].m_quantity ).AsChar() );
			}
			break;
		}
	case COL_LAYER_ARRAY:
		{
			sst.m_layerArray[GetSpawnSetSubRowNum( row )].m_layer = CName( cellValue );
			RefreshColumn( col );
			break;
		}
	case COL_LAYER_WEIGHT_ARRAY:
		{
			Float val;
			if ( FromString( cellValue, val ) ) // is correct float value ?
			{
				sst.m_layerArray[GetSpawnSetSubRowNum( row )].m_weight = val;
			}
			else
			{
				// restore old value
				m_grid->SetCellValue( row, col, String::Printf( TXT("%.2f"), sst.m_layerArray[GetSpawnSetSubRowNum( row )].m_weight ).AsChar() );
			}

			break;
		}
	case COL_USE_EXISTING:
		{
			sst.m_useExistingCharacters = GetCellBool( row, col );
		}
	}
}

void CEdSpawnSetEditor::OnCellChange( wxGridEvent &event )
{
	CellChange( event.GetRow(), event.GetCol() );
}

void CEdSpawnSetEditor::OnCellChange( wxSheetEvent &event )
{
	CellChange( event.GetRow(), event.GetCol() );
	event.Skip();
}

void CEdSpawnSetEditor::OnCellLeftDoubleClick( wxGridEvent &event )
{
	// Run custom editors
	m_grid->SetGridCursor( event.GetRow(), event.GetCol() );
	RunCustomEditor( event.GetRow(), event.GetCol() );
}

void CEdSpawnSetEditor::OnCellLeftClick( wxGridEvent &event )
{
	int cursorRow = m_grid->GetCursorRow();
	int cursorCol = m_grid->GetCursorColumn();

	Int32 row = event.GetRow();
	Int32 col = event.GetCol();

	// Process click only if the clicked cell was already selected
	if ( cursorCol == col && cursorRow == row )
	{
		switch ( col )
		{
		case COL_ENTITY_TEMPLATE:
			// Get cell position and dimensions
			wxRect cellRect = m_grid->CellToRect( row, col );
			cellRect = m_grid->BlockToDeviceRect( row, col, row, col ); // deal with scrollbars
			cellRect.SetX( m_grid->GetRowLabelSize() + cellRect.GetX() );
			cellRect.SetY( m_grid->GetColLabelSize() + cellRect.GetY() );

			// Get cursor position
			wxPoint mouseClickPos = event.GetPosition();

			// Calculate buttons rect

			const Int32 yPos = cellRect.GetY() + (cellRect.GetHeight()/2 - CTemplateCellRenderer::ICON_HEIGHT/2);

			wxRect btnClearRect( cellRect );
			btnClearRect.SetX( cellRect.GetX() + cellRect.GetWidth() - CTemplateCellRenderer::ICON_WIDTH * 1 );
			btnClearRect.SetY( yPos );
			btnClearRect.SetWidth( CTemplateCellRenderer::ICON_WIDTH );
			btnClearRect.SetHeight( CTemplateCellRenderer::ICON_HEIGHT );

			wxRect btnUseRect( cellRect );
			btnUseRect.SetX( cellRect.GetX() + cellRect.GetWidth() - CTemplateCellRenderer::ICON_WIDTH * 2 );
			btnUseRect.SetY( yPos );
			btnUseRect.SetWidth( CTemplateCellRenderer::ICON_WIDTH );
			btnUseRect.SetHeight( CTemplateCellRenderer::ICON_HEIGHT );

			wxRect btnBrowseRect( cellRect );
			btnBrowseRect.SetX( cellRect.GetX() + cellRect.GetWidth() - CTemplateCellRenderer::ICON_WIDTH * 3 );
			btnBrowseRect.SetY( yPos );
			btnBrowseRect.SetWidth( CTemplateCellRenderer::ICON_WIDTH );
			btnBrowseRect.SetHeight( CTemplateCellRenderer::ICON_HEIGHT );

			// Check which button was clicked
			if ( btnClearRect.Contains( mouseClickPos ) )
			{
				ClearCellTemplate( row, col );
			}
			else if ( btnBrowseRect.Contains( mouseClickPos ) )
			{
				BrowseCellTemplate( row, col );
			}
			else if ( btnUseRect.Contains( mouseClickPos ) )
			{
				SetCellTemplate( row, col );
			}

			break;
		}
	}

	event.Skip();
}

void CEdSpawnSetEditor::OnCellRightClick( wxGridEvent &event )
{
	if ( event.ControlDown() )
	{
		
	}
	else if ( event.AltDown() )
	{
		
	}
	else
	{
		Int32 row = event.GetRow();
		Int32 col = event.GetCol();
		m_grid->SetGridCursor( row, col );
		
		ContextMenu( row, col );
	}
}

void CEdSpawnSetEditor::OnLabelRightClick( wxGridEvent &event )
{
	Int32 row = event.GetRow();

	if ( row == -1 )
	{
		wxMenu menu;
		menu.Append( 1, TXT("Append row") );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnAppendRow ), NULL, this );
		PopupMenu( &menu );
	}
	else
	{
		m_grid->SetGridCursor( row, COL_FIRST );

		wxMenu menu;
		menu.Append( 0, TXT("Insert row") );
		menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnInsertRow ), NULL, this );
		menu.Append( 1, TXT("Append row") );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnAppendRow ), NULL, this );
		menu.Append( 2, TXT("Remove row") );
		menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnRemoveRow ), NULL, this );
		
		if ( !IsRowComment( GetGridMainRowNum(m_grid->GetCursorRow())-1 ) )
		{
			menu.AppendSeparator();
			menu.Append( 3, TXT("Insert comment") );
			menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnInsertCommentRow), NULL, this );
		}
		else
		{
			menu.AppendSeparator();
			menu.Append( 3, TXT("Remove comment") );
			menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnRemoveCommentRow), NULL, this );
		}

		PopupMenu( &menu );
	}
}

void CEdSpawnSetEditor::OnAddSubRow( wxCommandEvent& event )
{
	AddSubRow();
}

void CEdSpawnSetEditor::AddSubRow( Bool addNewEntryToSST /* = true */ )
{
	Int32 cursorRow = m_grid->GetCursorRow();
	Int32 cursorCol = m_grid->GetCursorColumn();
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(cursorRow) ];

	// Select column to check for read only, so we can decide whether row is a subrow or not
	int colReadOnlyCheck = -1;
	switch ( cursorCol )
	{
	case COL_ENTITY_TEMPLATE:
		colReadOnlyCheck = COL_ENTITY_TEMPLATE;
		break;
	case COL_SPAWN_TIME_ARRAY:
		colReadOnlyCheck = COL_SPAWN_TIME_ARRAY;
		break;
	case COL_LAYER_ARRAY:
		colReadOnlyCheck = COL_LAYER_ARRAY;
		break;
	default:
		ASSERT( 0 );
		break;
	}

	// Find the appropriate row to insert subrow
	while ( true )
	{
		// We have found an unused space for a subrow, so we use it
		if ( !IsCellEnabled( cursorRow, colReadOnlyCheck ) )
		{
			AddSubRowEntry( cursorRow, cursorCol, addNewEntryToSST );
			break;
		}
		// We cannot find a free space for the subrow, so we have to create a new one
		else if ( ++cursorRow >= m_grid->GetNumberRows() || IsRow( cursorRow ) || IsRowComment( cursorRow ) )
		{
			// We are in new entry (there is no disabled=free cell), so we need to create new line
			InsertNewSubrow( cursorRow );
			AddSubRowEntry( cursorRow, cursorCol, addNewEntryToSST );
			break;
		}
	}

	CalcGridRowLabelValues();
	SetMainRows();
}

void CEdSpawnSetEditor::AddSubRowEntry( Int32 row, Int32 col, Bool addNewEntryToSST /* = true */ )
{
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ];

	switch ( col )
	{
	case COL_ENTITY_TEMPLATE:
		{
			if ( addNewEntryToSST ) sst.m_templates.PushBack( CSpawnSetTimetableTemplate() );
			EnableCell( row, COL_ENTITY_TEMPLATE );
			EnableCell( row, COL_ENTITY_APPEARANCES );
			EnableCell( row, COL_ENTITY_WEIGHT );
			// Set default values
			CEntityTemplate* emptyT = NULL;
			TDynArray<CName> emptyA;
			SetCell( row, COL_ENTITY_TEMPLATE, emptyT );
			SetCell( row, COL_ENTITY_APPEARANCES, emptyA );
			SetCell( row, COL_ENTITY_WEIGHT, 1.0f);
			break;
		}
	case COL_SPAWN_TIME_ARRAY:
		{
			if ( addNewEntryToSST ) sst.m_spawnArray.PushBack( CSpawnSetTimetableSpawnEntry() );
			EnableCell( row, COL_SPAWN_TIME_ARRAY );
			EnableCell( row, COL_SPAWN_QUANTITY_ARRAY );
			EnableCell( row, COL_SPAWN_TAG_ARRAY, true );
			// Set default values
			GameTime timeZero( 0 ); SetCell( row, COL_SPAWN_TIME_ARRAY, &timeZero );
			SetCell( row, COL_SPAWN_QUANTITY_ARRAY, 1 );
			break;
		}
	case COL_LAYER_ARRAY:
		{
			if ( addNewEntryToSST ) sst.m_layerArray.PushBack( CSpawnSetTimetableLayerEntry() );
			EnableCell( row, COL_LAYER_ARRAY );
			EnableCell( row, COL_LAYER_WEIGHT_ARRAY );
			// Set default values
			SetCell( row, COL_LAYER_ARRAY, CName() );
			SetCell( row, COL_LAYER_WEIGHT_ARRAY, 1.0f );
			break;
		}
	default:
		ASSERT( 0 );
		break;
	}

	RefreshGrid();
}

void CEdSpawnSetEditor::OnRemoveCurrentSubRow( wxCommandEvent& event )
{
	const Int32 cursorRow = m_grid->GetCursorRow();
	const Int32 cursorCol = m_grid->GetCursorColumn();
	const Int32 gridMainRowNum = GetGridMainRowNum( cursorRow );
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(cursorRow) ];

	switch ( cursorCol )
	{
	case COL_ENTITY_TEMPLATE:
		{
			if ( GetSpawnSetSubRowNum( cursorRow ) < (Int32)sst.m_templates.Size() )
			{
				sst.m_templates.Erase( sst.m_templates.Begin() + GetSpawnSetSubRowNum( cursorRow ) );

				const Int32 rowToDelNum = gridMainRowNum + sst.m_templates.Size();
				DisableCell( rowToDelNum, COL_ENTITY_TEMPLATE );
				DisableCell( rowToDelNum, COL_ENTITY_APPEARANCES );
				DisableCell( rowToDelNum, COL_ENTITY_WEIGHT );

				RemoveSubrowIfEmpty( rowToDelNum );

				// Copy entries from m_spawnSet to grid
				typedef TDynArray< CSpawnSetTimetableTemplate >::const_iterator CI;
				Int32 currRow = gridMainRowNum;
				for ( CI ci = sst.m_templates.Begin(); ci != sst.m_templates.End(); ++ci, ++currRow )
				{
					EnableCell( currRow, COL_ENTITY_TEMPLATE );
					EnableCell( currRow, COL_ENTITY_APPEARANCES );
					EnableCell( currRow, COL_ENTITY_WEIGHT );

					SetCell( currRow, COL_ENTITY_TEMPLATE, ci->m_template );
					SetCell( currRow, COL_ENTITY_APPEARANCES, ci->m_appearances );
					SetCell( currRow, COL_ENTITY_WEIGHT, ci->m_weight );
				}
			}
			break;
		}
	case COL_SPAWN_TIME_ARRAY:
		{
			if ( GetSpawnSetSubRowNum( cursorRow ) < (Int32)sst.m_spawnArray.Size() )
			{
				sst.m_spawnArray.Erase( sst.m_spawnArray.Begin() + GetSpawnSetSubRowNum( cursorRow ) );

				const Int32 rowToDelNum = gridMainRowNum + sst.m_spawnArray.Size();
				DisableCell( rowToDelNum, COL_SPAWN_TIME_ARRAY );
				DisableCell( rowToDelNum, COL_SPAWN_QUANTITY_ARRAY );
				DisableCell( rowToDelNum, COL_SPAWN_TAG_ARRAY );

				RemoveSubrowIfEmpty( rowToDelNum );

				// Copy entries from m_spawnSet to grid
				typedef TDynArray< CSpawnSetTimetableSpawnEntry >::const_iterator CI;
				Int32 currRow = gridMainRowNum;
				for ( CI ci = sst.m_spawnArray.Begin(); ci != sst.m_spawnArray.End(); ++ci, ++currRow )
				{
					EnableCell( currRow, COL_SPAWN_TIME_ARRAY );
					EnableCell( currRow, COL_SPAWN_QUANTITY_ARRAY );
					EnableCell( currRow, COL_SPAWN_TAG_ARRAY, true );

					SetCell( currRow, COL_SPAWN_TIME_ARRAY, &ci->m_time );
					SetCell( currRow, COL_SPAWN_QUANTITY_ARRAY, ci->m_quantity );
					SetCell( currRow, COL_SPAWN_TAG_ARRAY, ci->m_spawnpointTag );
 				}
			}
			break;
		}
	case COL_LAYER_ARRAY:
		{
			if ( GetSpawnSetSubRowNum( cursorRow ) < (Int32)sst.m_layerArray.Size() )
			{
				sst.m_layerArray.Erase( sst.m_layerArray.Begin() + GetSpawnSetSubRowNum( cursorRow ) );

				const Int32 rowToDelNum = gridMainRowNum + sst.m_layerArray.Size();
				DisableCell( rowToDelNum, COL_LAYER_ARRAY );
				DisableCell( rowToDelNum, COL_LAYER_WEIGHT_ARRAY );

				RemoveSubrowIfEmpty( rowToDelNum );

				// Copy entries from m_spawnSet to grid
				typedef TDynArray< CSpawnSetTimetableLayerEntry >::const_iterator CI;
				Int32 currRow = gridMainRowNum;
				for ( CI ci = sst.m_layerArray.Begin(); ci != sst.m_layerArray.End(); ++ci, ++currRow )
				{
					EnableCell( currRow, COL_LAYER_ARRAY );
					EnableCell( currRow, COL_LAYER_WEIGHT_ARRAY );

					SetCell( currRow, COL_LAYER_ARRAY, ci->m_layer );
					SetCell( currRow, COL_LAYER_WEIGHT_ARRAY, ci->m_weight );
				}
			}
			break;
		}
	default:
		ASSERT( 0 );
		break;
	}

	CalcGridRowLabelValues();
	SetMainRows();
}

void CEdSpawnSetEditor::OnAddBeforeCurrentSubRow( wxCommandEvent& event )
{
	const Int32 cursorRow = m_grid->GetCursorRow();
	const Int32 cursorCol = m_grid->GetCursorColumn();
	const Int32 gridMainRowNum = GetGridMainRowNum( cursorRow );
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(cursorRow) ];

	switch ( cursorCol )
	{
	case COL_ENTITY_TEMPLATE:
		{
			if ( GetSpawnSetSubRowNum( cursorRow ) < (Int32)sst.m_templates.Size() )
			{
				sst.m_templates.Insert( (Uint32)GetSpawnSetSubRowNum( cursorRow ), CSpawnSetTimetableTemplate() );

				AddSubRow( false );

				// Copy entries from m_spawnSet to grid
				typedef TDynArray< CSpawnSetTimetableTemplate >::const_iterator CI;
				Int32 currRow = gridMainRowNum;
				for ( CI ci = sst.m_templates.Begin(); ci != sst.m_templates.End(); ++ci, ++currRow )
				{
					SetCell( currRow, COL_ENTITY_TEMPLATE, ci->m_template );
					SetCell( currRow, COL_ENTITY_APPEARANCES, ci->m_appearances );
					SetCell( currRow, COL_ENTITY_WEIGHT, ci->m_weight );
				}
			}
			break;
		}
	case COL_SPAWN_TIME_ARRAY:
		{
			if ( GetSpawnSetSubRowNum( cursorRow ) < (Int32)sst.m_spawnArray.Size() )
			{
				sst.m_spawnArray.Insert( (Uint32)GetSpawnSetSubRowNum( cursorRow ), CSpawnSetTimetableSpawnEntry() );

				AddSubRow( false );

				// Copy entries from m_spawnSet to grid
				typedef TDynArray< CSpawnSetTimetableSpawnEntry >::const_iterator CI;
				Int32 currRow = gridMainRowNum;
				for ( CI ci = sst.m_spawnArray.Begin(); ci != sst.m_spawnArray.End(); ++ci, ++currRow )
				{
					SetCell( currRow, COL_SPAWN_TIME_ARRAY, &ci->m_time );
					SetCell( currRow, COL_SPAWN_QUANTITY_ARRAY, ci->m_quantity );
					SetCell( currRow, COL_SPAWN_TAG_ARRAY, ci->m_spawnpointTag );
				}
			}
			break;
		}
	case COL_LAYER_ARRAY:
		{
			if ( GetSpawnSetSubRowNum( cursorRow ) < (Int32)sst.m_layerArray.Size() )
			{
				sst.m_layerArray.Insert( (Uint32)GetSpawnSetSubRowNum( cursorRow ), CSpawnSetTimetableLayerEntry() );

				AddSubRow( false );

				// Copy entries from m_spawnSet to grid
				typedef TDynArray< CSpawnSetTimetableLayerEntry >::const_iterator CI;
				Int32 currRow = gridMainRowNum;
				for ( CI ci = sst.m_layerArray.Begin(); ci != sst.m_layerArray.End(); ++ci, ++currRow )
				{
					SetCell( currRow, COL_LAYER_ARRAY, ci->m_layer );
					SetCell( currRow, COL_LAYER_WEIGHT_ARRAY, ci->m_weight );
				}
			}
			break;
		}
	default:
		ASSERT( 0 );
		break;
	}

	CalcGridRowLabelValues();
	SetMainRows();
}

void CEdSpawnSetEditor::OnRemoveLastSubRow( wxCommandEvent& event )
{
	Int32 cursorRow = m_grid->GetCursorRow();
	Int32 cursorCol = m_grid->GetCursorColumn();
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(cursorRow) ];

	Int32 ssRowNum = GetSpawnSetRowNum( cursorRow );

	switch ( cursorCol )
	{
	case COL_ENTITY_TEMPLATE:
		{
			if ( Int32 size = sst.m_templates.Size() )
			{
				const Int32 rowToDelNum = cursorRow + size - 1;
				DisableCell( rowToDelNum, COL_ENTITY_TEMPLATE );
				DisableCell( rowToDelNum, COL_ENTITY_APPEARANCES );
				DisableCell( rowToDelNum, COL_ENTITY_WEIGHT );
				sst.m_templates.PopBack();
				RemoveSubrowIfEmpty( rowToDelNum );
			}
			break;
		}
	case COL_SPAWN_TIME_ARRAY:
		{
			if ( Int32 size = sst.m_spawnArray.Size() )
			{
				const Int32 rowToDelNum = cursorRow + size - 1;
				DisableCell( rowToDelNum, COL_SPAWN_TIME_ARRAY );
				DisableCell( rowToDelNum, COL_SPAWN_QUANTITY_ARRAY );
				DisableCell( rowToDelNum, COL_SPAWN_TAG_ARRAY );
				sst.m_spawnArray.PopBack();
				RemoveSubrowIfEmpty( rowToDelNum );
			}
			break;
		}
	case COL_LAYER_ARRAY:
		{
			if ( Int32 size = sst.m_layerArray.Size() )
			{
				const Int32 rowToDelNum = cursorRow + size - 1;
				DisableCell( rowToDelNum, COL_LAYER_ARRAY );
				DisableCell( rowToDelNum, COL_LAYER_WEIGHT_ARRAY );
				sst.m_layerArray.PopBack();
				RemoveSubrowIfEmpty( rowToDelNum );
			}
			break;
		}
	default:
		ASSERT( 0 );
		break;
	}
}

void CEdSpawnSetEditor::InsertNewRow( Int32 row )
{
	m_grid->InsertRows( row, 1 );
	
	// Set default values
	m_grid->SetCellValue( row, COL_ENTRY_TAGS , TXT("default"));
	m_grid->SetCellValue( row, COL_ENTITY_TEMPLATE, TXT("None") );
	m_grid->SetCellValue( row, COL_ENTITY_APPEARANCES, wxEmptyString );
	SetCell( row, COL_ENTITY_WEIGHT, 1.0f);

	// Disable empty cells
	DisableCell( row, COL_SPAWN_TIME_ARRAY );
	DisableCell( row, COL_SPAWN_QUANTITY_ARRAY );
	DisableCell( row, COL_SPAWN_TAG_ARRAY );
	DisableCell( row, COL_LAYER_ARRAY );
	DisableCell( row, COL_LAYER_WEIGHT_ARRAY );

	// Set read-only cells with custom editors
	m_grid->SetReadOnly( row, COL_ENTRY_TAGS );
	m_grid->SetReadOnly( row, COL_ENTITY_TEMPLATE );
	m_grid->SetReadOnly( row, COL_ENTITY_APPEARANCES );
	m_grid->SetReadOnly( row, COL_CHAR_TAGS );

	// Set cell editors
	m_grid->SetCellEditorBool( row, COL_USE_EXISTING );

	// Set cell renderers
	m_grid->SetCellRendererBool( row, COL_USE_EXISTING );
	m_grid->SetCellRendererTemplate( row, COL_ENTITY_TEMPLATE );

	// Set alignment
	m_grid->SetCellAlignment( row, COL_USE_EXISTING, wxALIGN_CENTRE, wxALIGN_CENTRE );
}

void CEdSpawnSetEditor::InsertNewSubrow( Int32 row )
{
	m_grid->InsertRows( row, 1 );

	for ( int i = 0; i < COL_COUNT; ++i )
	{
		m_grid->SetCellBackgroundColour( row, i, *wxLIGHT_GREY );
		m_grid->SetReadOnly( row, i );
	}

	// Disable editable sublayers
	DisableCell( row, COL_ENTITY_TEMPLATE );
	DisableCell( row, COL_ENTITY_APPEARANCES );
	DisableCell( row, COL_ENTITY_WEIGHT );
	DisableCell( row, COL_SPAWN_TIME_ARRAY );
	DisableCell( row, COL_SPAWN_QUANTITY_ARRAY );
	DisableCell( row, COL_SPAWN_TAG_ARRAY );
	DisableCell( row, COL_LAYER_ARRAY );
	DisableCell( row, COL_LAYER_WEIGHT_ARRAY );
}

void CEdSpawnSetEditor::LoadDataFromResource()
{
	m_grid->ClearGrid(); // only removes entries from grid, doesn't remove any rows or columns
	
	m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

	m_grid->AppendRows( m_spawnSet->GetTimetable().Size() );

	m_grid->SetColLabelValue( COL_ENTRY_TAGS,           TXT("Entry tags") );
	m_grid->SetColLabelValue( COL_ENTITY_TEMPLATE,      TXT("Entity template") );
	m_grid->SetColLabelValue( COL_ENTITY_APPEARANCES,   TXT("Appearances") );
	m_grid->SetColLabelValue( COL_ENTITY_WEIGHT,        TXT("Entity weight") );
	m_grid->SetColLabelValue( COL_CHAR_TAGS,            TXT("Character tags") );
	m_grid->SetColLabelValue( COL_SPAWN_TIME_ARRAY,     TXT("Time") );
	m_grid->SetColLabelValue( COL_SPAWN_QUANTITY_ARRAY, TXT("Quantity") );
	m_grid->SetColLabelValue( COL_SPAWN_TAG_ARRAY,      TXT("Spawnpoint tag") );
	m_grid->SetColLabelValue( COL_LAYER_ARRAY,          TXT("Layer") );
	m_grid->SetColLabelValue( COL_LAYER_WEIGHT_ARRAY,   TXT("Layer weight") );
	m_grid->SetColLabelValue( COL_USE_EXISTING,         TXT("Use\nexisting") );

	typedef TDynArray< CSpawnSetTimetable >::iterator I;
	int gridRow = 0;
	int ssRow = 0; // spawn set row
	for ( I i = m_spawnSet->GetTimetable().Begin(); i != m_spawnSet->GetTimetable().End(); ++i, ++gridRow, ++ssRow )
	{
		// Commentary
		if ( i->m_comment != String::EMPTY )
		{
			InsertNewRowComment( gridRow );
			m_grid->SetCellValue( gridRow, COL_COMMENT, i->m_comment.AsChar() );
			gridRow++;
		}

		// Set values for one timetable entry (it can be many rows)

		// Entry tags
		m_grid->SetCellValue( gridRow, COL_ENTRY_TAGS, TagListArrayToString( i->m_entryTags ).AsChar() );
		m_grid->SetReadOnly( gridRow, COL_ENTRY_TAGS );

		// Character tags
		m_grid->SetCellValue( gridRow, COL_CHAR_TAGS, TagListArrayToString( i->m_characterTags ).AsChar() );
		m_grid->SetReadOnly( gridRow, COL_CHAR_TAGS );

		// Use existing
		PrintCellBool( gridRow, COL_USE_EXISTING, i->m_useExistingCharacters );
		m_grid->SetCellEditorBool( gridRow, COL_USE_EXISTING );
		m_grid->SetCellRendererBool( gridRow, COL_USE_EXISTING );
		m_grid->SetCellAlignment( gridRow, COL_USE_EXISTING, wxALIGN_CENTRE, wxALIGN_CENTRE );

		// Tables...
		
		const TDynArray< CSpawnSetTimetableTemplate > &sstEntityArray = i->m_templates;
		const TDynArray< CSpawnSetTimetableSpawnEntry > &sstSpawnArray = i->m_spawnArray;
		const TDynArray< CSpawnSetTimetableLayerEntry > &sstLayerArray = i->m_layerArray;
		Uint32 maxI = max( sstSpawnArray.Size(), sstLayerArray.Size() );
		if ( maxI == 0 )
		{
			// Disable empty cells
			DisableCell( gridRow, COL_SPAWN_TIME_ARRAY );
			DisableCell( gridRow, COL_SPAWN_QUANTITY_ARRAY );
			DisableCell( gridRow, COL_SPAWN_TAG_ARRAY );
			DisableCell( gridRow, COL_LAYER_ARRAY );
			DisableCell( gridRow, COL_LAYER_WEIGHT_ARRAY );
		}
		maxI = max( sstEntityArray.Size(), maxI );
		for ( Uint32 i = 0; i < maxI; )
		{
			// Entity array
			if ( i < sstEntityArray.Size() )
			{
				// SetCell
				EnableCell( gridRow, COL_ENTITY_TEMPLATE );
				EnableCell( gridRow, COL_ENTITY_APPEARANCES );
				EnableCell( gridRow, COL_ENTITY_WEIGHT );
				SetCell( gridRow, COL_ENTITY_TEMPLATE, sstEntityArray[i].m_template );
				SetCell( gridRow, COL_ENTITY_APPEARANCES, sstEntityArray[i].m_appearances );
				SetCell( gridRow, COL_ENTITY_WEIGHT, sstEntityArray[i].m_weight );
			}
			else
			{
				DisableCell( gridRow, COL_ENTITY_TEMPLATE );
				DisableCell( gridRow, COL_ENTITY_APPEARANCES );
				DisableCell( gridRow, COL_ENTITY_WEIGHT );
			}

			// Spawn array
			if ( i < sstSpawnArray.Size() )
			{
				// SetCell
				PrintCellGameTime( gridRow, COL_SPAWN_TIME_ARRAY, &sstSpawnArray[i].m_time );
				m_grid->SetCellAlignment( gridRow, COL_SPAWN_TIME_ARRAY, wxALIGN_CENTRE, wxALIGN_CENTRE );

				// SetCell
				m_grid->SetCellValue( gridRow, COL_SPAWN_QUANTITY_ARRAY, String::Printf( TXT("%d"), sstSpawnArray[i].m_quantity ).AsChar() );
				m_grid->SetCellEditorNumber( gridRow, COL_SPAWN_QUANTITY_ARRAY );
				m_grid->SetCellAlignment( gridRow, COL_SPAWN_QUANTITY_ARRAY, wxALIGN_RIGHT, wxALIGN_CENTRE );

				// SetCell
				m_grid->SetCellValue( gridRow, COL_SPAWN_TAG_ARRAY, TagListArrayToString( sstSpawnArray[i].m_spawnpointTag ).AsChar() );
				m_grid->SetReadOnly( gridRow, COL_SPAWN_TAG_ARRAY );
			}
			else
			{
				DisableCell( gridRow, COL_SPAWN_TIME_ARRAY );
				DisableCell( gridRow, COL_SPAWN_QUANTITY_ARRAY );
				DisableCell( gridRow, COL_SPAWN_TAG_ARRAY );
			}

			// Layer array
			if ( i < sstLayerArray.Size() )
			{
				// SetCell
				SetCell( gridRow, COL_LAYER_ARRAY, sstLayerArray[i].m_layer );

				// SetCell
				SetCell( gridRow, COL_LAYER_WEIGHT_ARRAY, sstLayerArray[i].m_weight );
			}
			else
			{
				DisableCell( gridRow, COL_LAYER_ARRAY );
				DisableCell( gridRow, COL_LAYER_WEIGHT_ARRAY );
			}


			// Add new row if necessary
			if ( ++i < maxI )
			{
				++gridRow;
				m_grid->InsertRows( gridRow, 1 );

				for ( int i = 0; i < COL_COUNT; ++i )
				{
					// editable columns of arrays (without custom editors)
					if ( i != COL_SPAWN_TIME_ARRAY && i != COL_SPAWN_QUANTITY_ARRAY && i != COL_LAYER_ARRAY && i != COL_LAYER_WEIGHT_ARRAY )
					{
						if ( i != COL_SPAWN_TAG_ARRAY ) // editable columns of arrays with custom editors
						{
							m_grid->SetCellBackgroundColour( gridRow, i, *wxLIGHT_GREY );
						}
						m_grid->SetReadOnly( gridRow, i );
					}
				}

				m_grid->SetRowLabelValue( gridRow, wxString() );
			}
		}
	}

	CalcGridRowLabelValues();
	SetMainRows();
	RefreshGrid();
}

String CNameArrayToString( const TDynArray<CName> &names )
{
	String res;
	for ( TDynArray< CName >::const_iterator ci = names.Begin(); ci != names.End(); )
	{
		res += ci->AsChar();
		if ( ++ci != names.End() )
			res += TXT("\n");
	}
	return res;
}

String CEdSpawnSetEditor::TagListArrayToString( const TagList& tagList )
{
	if ( !tagList.Empty() )
	{
		return CNameArrayToString( tagList.GetTags() );
	}
	else
	{
		return String::EMPTY;
	}
}

void CEdSpawnSetEditor::CalcGridRowLabelValues()
{
	Int32 ssRow = 0;
	for ( Int32 i = 0 ; i < m_grid->GetNumberRows(); ++i )
	{
		if ( IsRow( i ) )
		{
			m_grid->SetRowLabelValue( i, String::Printf(TXT("%d"), ssRow).AsChar() );
			++ssRow;
		}
		else
		{
			m_grid->SetRowLabelValue( i, wxEmptyString );
		}
	}
}

void CEdSpawnSetEditor::SetMainRows()
{
	TSet< Int32 > mainRows;
	for ( Int32 row = 0; row < m_grid->GetNumberRows(); ++row )
	{
		if ( IsRow( row ) )
		{
			if ( IsRowComment( row-1 ))
				mainRows.Insert( row-2 );
			else
				mainRows.Insert( row-1 );
		}
	}
	m_grid->SetMainRows( mainRows );
}

// Returns spawn set row number based on editor grid row number
Int32 CEdSpawnSetEditor::GetSpawnSetRowNum( Int32 gridRowNum )
{
	ASSERT( gridRowNum >= 0 );

	Int32 ssRow = 0;
	for ( Int32 i = 0 ; i <= gridRowNum; ++i )
	{
		if ( IsRow( i ) && !IsRowComment( i ) ) ++ssRow;
	}
	if ( IsRowComment( gridRowNum ) ) ++ssRow;
	return ssRow-1;
}

Int32 CEdSpawnSetEditor::GetSpawnSetSubRowNum( Int32 gridRowNum )
{
	ASSERT( gridRowNum >= 0 );

	if ( IsRowComment( gridRowNum ) ) return -1;

	Int32 ssRow = 0;
	while( IsSubRow( gridRowNum-- ) )
	{
		++ssRow;
	}
	return ssRow;
}

Int32 CEdSpawnSetEditor::GetGridMainRowNum( Int32 gridRowNum )
{
	ASSERT( gridRowNum >= 0 );

	if ( IsRowComment( gridRowNum ) )
	{
		return gridRowNum+1;
	}

	while( IsSubRow( gridRowNum ) )
	{
		--gridRowNum;
	}
	return gridRowNum;
}

// Reads user input data from grid cell, converts it to GameTime and saves to the third parameter
Bool CEdSpawnSetEditor::SetCellGameTime( Int32 row, Int32 col, GameTime *gameTime /* out */ )
{
	String txt = m_grid->GetCellValue( row, col );
	Uint32 minutes, hours;
	if ( swscanf_s( txt.AsChar(), TXT("%u:%u"), &hours, &minutes ) == 2 && hours < 24 && minutes < 60 )
	{
		*gameTime = GameTime( 0, hours, minutes, 0 );
		PrintCellGameTime( row, col, gameTime );
		return true; // success
	}
	else
	{
		PrintCellGameTime( row, col, gameTime );
		return false; // failure, old value restored
	}
}

void CEdSpawnSetEditor::RunCustomEditor( const Int32 row, const Int32 col )
{
	// Run tag editor
	if ( (( col == COL_ENTRY_TAGS || col == COL_CHAR_TAGS ) && (IsRow( row )))
		 ||
		 ( col == COL_SPAWN_TAG_ARRAY && IsCellEnabled( row, col ) ) )
	{
		TagList tagList;
		if ( col == COL_SPAWN_TAG_ARRAY ) tagList = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_spawnArray[ GetSpawnSetSubRowNum(row) ].m_spawnpointTag;
		else if ( col == COL_ENTRY_TAGS ) tagList = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_entryTags;
		else if ( col == COL_CHAR_TAGS ) tagList = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_characterTags;
		else ASSERT( 0 );

		/*m_tagEditor = new CEdTagMiniEditor( this, tagList.GetTags() );
		m_tagEditor->Connect( wxEVT_TAGEDITOR_OK, wxCommandEventHandler( CEdSpawnSetEditor::OnTagsSaved ), NULL, this );
		m_tagEditor->Connect( wxEVT_TAGEDITOR_CANCEL, wxCommandEventHandler( CEdSpawnSetEditor::OnTagsCanceled ), NULL, this );
		m_tagEditor->ShowModal();*/
	}
	// Select template
	else if ( col == COL_ENTITY_TEMPLATE && IsCellEnabled( row, col ) )
	{
		SetCellTemplate( row, col );
	}
	else if ( col == COL_ENTITY_APPEARANCES && IsCellEnabled( row, col ) && m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_templates[ GetSpawnSetSubRowNum(row) ].m_template )
	{
		/*m_tagEditor = new CEdTagMiniEditor( this, m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_templates[ GetSpawnSetSubRowNum(row) ].m_appearances, true );
		TDynArray< CEdTagListUpdater* > updaters;
		updaters.PushBack( new CEdAppearanceTagListUpdater(m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_templates[ GetSpawnSetSubRowNum(row) ].m_template) );
		m_tagEditor->SetTagListUpdaters( updaters );
		m_tagEditor->Connect( wxEVT_TAGEDITOR_OK, wxCommandEventHandler( CEdSpawnSetEditor::OnTagsSaved ), NULL, this );
		m_tagEditor->Connect( wxEVT_TAGEDITOR_CANCEL, wxCommandEventHandler( CEdSpawnSetEditor::OnTagsCanceled ), NULL, this );
		m_tagEditor->ShowModal();*/
	}
}

// deprecated: remove this method
void CEdSpawnSetEditor::OnActionSaved( wxCommandEvent &event )
{
	if ( ISpawnSetTimetableAction *action = m_actionEditor->GetSSTAction() )
	{
		Int32 cursorRow = m_grid->GetCursorRow();
		Int32 cursorCol = m_grid->GetCursorColumn();
		Int32 ssRow = GetSpawnSetRowNum( cursorRow );

		m_grid->SetCellValue( cursorRow, cursorCol, action->GetStringDesc().AsChar() );

		RefreshGrid();
	}
}

// deprecated: remove this method
void CEdSpawnSetEditor::OnActionCanceled( wxCommandEvent &event )
{}

void CEdSpawnSetEditor::OnTagsSaved( wxCommandEvent &event )
{
	Int32 cursorRow = m_grid->GetCursorRow();
	Int32 cursorCol = m_grid->GetCursorColumn();
	Int32 ssRow = GetSpawnSetRowNum( cursorRow );

	// Grab tags
	ASSERT( m_tagEditor );
	TagList tagList( m_tagEditor->GetTags() );

	// Update tags
	if ( cursorCol == COL_SPAWN_TAG_ARRAY )
	{
		m_spawnSet->GetTimetable()[ssRow].m_spawnArray[ GetSpawnSetSubRowNum( cursorRow ) ].m_spawnpointTag = tagList;
	}
	else if ( cursorCol == COL_ENTRY_TAGS )
	{
		m_spawnSet->GetTimetable()[ssRow].m_entryTags = tagList;
	}
	else if ( cursorCol == COL_CHAR_TAGS )
	{
		m_spawnSet->GetTimetable()[ssRow].m_characterTags = tagList;
	}
	else if ( cursorCol == COL_ENTITY_APPEARANCES )
	{
		const TDynArray<CName> &tags = tagList.GetTags();
		TDynArray<CName>  properTags;
		CEntityTemplate  *eTemplate = m_spawnSet->GetTimetable()[ssRow].m_templates[ GetSpawnSetSubRowNum( cursorRow ) ].m_template;

		if ( eTemplate )
		{
			for ( Uint32 i = 0; i < tags.Size(); ++i )
				if ( eTemplate->GetAppearance( tags[i], true ) != NULL )
					properTags.PushBack( tags[i] );

			m_spawnSet->GetTimetable()[ssRow].m_templates[ GetSpawnSetSubRowNum( cursorRow ) ].m_appearances = properTags;
		}

		tagList.SetTags( properTags );
	}
	else
		ASSERT( 0 && TXT("Spawn Set Editor Error: Wrong column selected. Ask Rychu for assistance.") );

	// Unlink tag editor
	m_tagEditor = NULL;

	// Update grid cell
	SetCell( cursorRow, cursorCol, tagList );

	RefreshGrid();
}

void CEdSpawnSetEditor::OnTagsCanceled( wxCommandEvent &event )
{
	// Unlink tag editor
	ASSERT( m_tagEditor );
	m_tagEditor = NULL;
}

void CEdSpawnSetEditor::DisableCell( Int32 row, Int32 col )
{
	wxColour color;
	wxRGBToColour( color, m_disabledCellColor ); // BGR
	m_grid->SetCellValue( row, col , wxEmptyString);
	m_grid->SetReadOnly( row, col );
	m_grid->SetCellBackgroundColour( row, col, color );
}

Bool CEdSpawnSetEditor::IsCellEnabled( Int32 row, Int32 col )
{
	if ( row >= 0 && col >= 0 && row < m_grid->GetNumberRows() && col < m_grid->GetNumberCols() )
	{
		return m_grid->GetCellBackgroundColour( row, col ) != m_disabledCellColor && !IsRowComment( row );
	}
	else
	{
		return false;
	}
}

void CEdSpawnSetEditor::EnableCell( Int32 row, Int32 col, Bool readOnly /* = false */ )
{
	m_grid->SetReadOnly( row, col, readOnly );
	m_grid->SetCellBackgroundColour( row, col, *wxWHITE );
	m_grid->Refresh();
}

void CEdSpawnSetEditor::RemoveSubrowIfEmpty( Int32 row )
{
	if ( !IsCellEnabled( row, COL_ENTITY_TEMPLATE ) &&
		 m_grid->IsReadOnly( row, COL_SPAWN_TIME_ARRAY ) &&
		 m_grid->IsReadOnly( row, COL_LAYER_ARRAY ) &&
		 IsSubRow( row ) )
	{
		m_grid->DeleteRows( row, 1 );
		CalcGridRowLabelValues();
		SetMainRows();
	}
}

void CEdSpawnSetEditor::ShowMainSubmenu( Int32 row, Int32 colToReadOnlyCheck, const String &itemText, wxPoint menuPos /* = wxDefaultPosition */ )
{
	wxMenu menu;

	if ( IsCellEnabled( row, m_grid->GetCursorColumn() ) )
	{
		menu.Append( 10, TXT("Copy") );
		menu.Connect( 10, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnEditCopy ), NULL, this );
		menu.Append( 11, TXT("Paste") );
		menu.Connect( 11, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnEditPaste ), NULL, this );
		menu.AppendSeparator();
	}

	if ( colToReadOnlyCheck == COL_ENTRY_TAGS || colToReadOnlyCheck == COL_CHAR_TAGS || colToReadOnlyCheck == COL_SPAWN_TAG_ARRAY )
	{
		menu.Append( 0, String(TXT("Edit ") + itemText).AsChar() );
		menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnEditCell ), NULL, this );
	}
	else
	{
		menu.Append( 5, String(TXT("Append ") + itemText).AsChar() );
		menu.Connect( 5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnAddSubRow ), NULL, this );
		if ( ! m_grid->IsReadOnly( row, colToReadOnlyCheck ) )
		{
			menu.Append( 4, String( TXT("Insert ") + itemText ).AsChar() );
			menu.Connect( 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnAddBeforeCurrentSubRow ), NULL, this );
			menu.Append( 1, TXT("Remove") );
			menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnRemoveCurrentSubRow ), NULL, this );
		}
	}

	PopupMenu( &menu, menuPos );
}

void CEdSpawnSetEditor::ShowSubRowSubmenu( Int32 row, Int32 colToReadOnlyCheck, const String &itemText, wxPoint menuPos /* = wxDefaultPosition */ )
{
	if ( IsCellEnabled( row, colToReadOnlyCheck ) ) // show context menu only if cell is editable
	{
		wxMenu menu;

		if ( IsCellEnabled( row, m_grid->GetCursorColumn() ) )
		{
			menu.Append( 10, TXT("Copy") );
			menu.Connect( 10, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnEditCopy ), NULL, this );
			menu.Append( 11, TXT("Paste") );
			menu.Connect( 11, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnEditPaste ), NULL, this );
			menu.AppendSeparator();
		}

		if ( colToReadOnlyCheck == COL_ENTRY_TAGS || colToReadOnlyCheck == COL_CHAR_TAGS || colToReadOnlyCheck == COL_SPAWN_TAG_ARRAY )
		{
			menu.Append( 0, String(TXT("Edit ") + itemText).AsChar() );
			menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnEditCell ), NULL, this );
		}
		else
		{
			menu.Append( 4, String(TXT("Append ") + itemText).AsChar() );
			menu.Connect( 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnAddSubRow ), NULL, this );
			menu.Append( 3, String( TXT("Insert ") + itemText ).AsChar() );
			menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnAddBeforeCurrentSubRow ), NULL, this );
			menu.Append( 1, TXT("Remove") );
			menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnRemoveCurrentSubRow ), NULL, this );
		}

		PopupMenu( &menu, menuPos );
	}
}

void CEdSpawnSetEditor::InsertNewRowComment( int gridRowNum )
{
	m_grid->InsertRows( gridRowNum, 1 );
	m_grid->SetSize( gridRowNum, COL_FIRST, 1, COL_COUNT );
	m_grid->SetCellRendererComment( gridRowNum, COL_FIRST );
}

Bool CEdSpawnSetEditor::IsRowComment( int gridRowNum )
{
	if ( gridRowNum < 0 || gridRowNum >= m_grid->GetNumberRows() )
		return false;
	else
	{
		// For the commentary entry, the first column is spanned across the whole grid width
		return m_grid->GetSize( gridRowNum, 0 ).m_col != 1;
	}
}

Bool CEdSpawnSetEditor::IsRow( Int32 gridRowNum )
{
	if ( gridRowNum < 0 || gridRowNum >= m_grid->GetNumberRows() )
	{
		return false;
	}
	else
	{
		return IsCellEnabled( gridRowNum, COL_ENTRY_TAGS ) && !IsRowComment( gridRowNum );
		//return IsCellEnabled( gridRowNum, COL_ENTRY_TAGS );
	}
}

Bool CEdSpawnSetEditor::IsSubRow( Int32 gridRowNum )
{
	if ( gridRowNum < 0 || gridRowNum >= m_grid->GetNumberRows() )
		return false;
	else
		//return !IsRow( gridRowNum );
		return !IsRow( gridRowNum ) && !IsRowComment( gridRowNum );
}

Bool CEdSpawnSetEditor::SetCellTemplate( Int32 row, Int32 col )
{
	if ( col == COL_ENTITY_TEMPLATE && IsCellEnabled( row, col ) )
	{
		const Int32 ssRowNum = GetSpawnSetRowNum( row );
		CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ ssRowNum ];

		String selectedResource;
		if ( GetActiveResource( selectedResource ) )
		{
			if ( selectedResource.EndsWith( TXT(".w2ent") ) )
			{
				CResource *res = GDepot->LoadResource( selectedResource );
				sst.m_templates[GetSpawnSetSubRowNum(row)].m_template = Cast< CEntityTemplate >( res );
				ASSERT( sst.m_templates[GetSpawnSetSubRowNum(row)].m_template && TXT("Entity template shouldn't be null. Ask Rychu for assistance.") );
				m_grid->SetCellValue( row, col, res->GetFile()->GetFileName().AsChar() );

				RefreshGrid();
				return true; // report success
			}
		}		
	}

	return false; // report failure
}

Bool CEdSpawnSetEditor::ClearCellTemplate( Int32 row, Int32 col )
{
	if ( IsCellEnabled( row, col ) )
	{
		CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ];
		sst.m_templates[GetSpawnSetSubRowNum(row)].m_template = NULL;
		m_grid->SetCellValue( row, col, TXT("None") );
		return true; // report success
	}

	return false; // report failure
}

Bool CEdSpawnSetEditor::BrowseCellTemplate( Int32 row, Int32 col )
{
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ];
	if ( IsCellEnabled( row, col ) )
	{
		if ( const CEntityTemplate* templateRes = sst.m_templates[ GetSpawnSetSubRowNum(row) ].m_template )
		{
			String depotPath = templateRes->GetFile()->GetDepotPath();
			SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), &depotPath );

			return true; // report success
		}
	}

	return false; // report failure
}

void CEdSpawnSetEditor::PrintCellGameTime( Int32 row, Int32 col, const GameTime *gameTime )
{
	m_grid->SetCellValue( row, col, String::Printf( TXT("%d:%.2d"), gameTime->Hours(), gameTime->Minutes() ).AsChar() );
}

void CEdSpawnSetEditor::RefreshGrid()
{
	// int scrollPosBackup = m_grid->GetScrollPos( wxVERTICAL );
	m_grid->AutoSize();
	m_guiGridPanel->Layout();
	// m_grid->Scroll( 0, scrollPosBackup );
	// Below: alternative for Get/Set scroll position
	m_grid->MakeCellVisible( m_grid->GetCursorRow(), m_grid->GetCursorColumn() );
}

void CEdSpawnSetEditor::RefreshColumn( Int32 col )
{
	m_omitCellChange = true;
	m_grid->AutoSizeColumn( col );
	m_omitCellChange = false;
}

void CEdSpawnSetEditor::OnGridMotion( wxMouseEvent& event )
{
	// Deal with tooltips

	wxPoint pos = m_grid->CalcUnscrolledPosition( event.GetPosition() );
	Int32 row = m_grid->YToRow( pos.y );
	Int32 col = m_grid->XToCol( pos.x );

	if ( row >=0 && col == COL_ENTITY_TEMPLATE && IsCellEnabled( row, col ) &&
		m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_templates[ GetSpawnSetSubRowNum(row) ].m_template )
	{
		wxString tooltip = 
			m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ].m_templates[ GetSpawnSetSubRowNum(row) ].m_template->GetFile()->GetDepotPath().AsChar();
		m_grid->GetGridWindow()->SetToolTip( tooltip );
		m_grid->GetGridWindow()->GetToolTip()->Enable( true );
	}
	else
	{
		m_grid->GetGridWindow()->GetToolTip()->Enable( false );
		m_grid->GetGridWindow()->SetToolTip( wxEmptyString );
	}

	event.Skip();
}

void CEdSpawnSetEditor::OnGridPanelRightMouseClick( wxMouseEvent& event )
{
	if ( m_grid->XToCol( event.GetX() ) == wxNOT_FOUND || m_grid->YToRow( event.GetY() ) == wxNOT_FOUND )
	{
		wxMenu menu;
		menu.Append( 1, TXT("Append row") );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSpawnSetEditor::OnAppendRow ), NULL, this );
		PopupMenu( &menu );
	}

	event.Skip();
}

void CEdSpawnSetEditor::OnGridKeyDown( wxKeyEvent& event )
{
	const Int32 cursorRow = m_grid->GetCursorRow();
	const Int32 cursorCol = m_grid->GetCursorColumn();

	if ( event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER )
	{
		event.Skip();
		// TODO
		/*if ( cursorCol == COL_ACTION || cursorCol == COL_SPAWN_POINTS || cursorCol == COL_DESPAWN_POINTS || cursorCol == COL_ENTRY_TAGS
			|| cursorCol == COL_CHAR_TAGS )
		{
			RunCustomEditor( cursorRow, cursorCol );
		}
		else if ( cursorCol == COL_LAST && cursorRow + 1 < m_grid->GetNumberRows() )
		{
			m_grid->SetGridCursor( cursorRow + 1, COL_FIRST );
		}
		else
		{
			m_grid->MoveCursorRight( false );
		}*/
	}
	else if ( event.GetKeyCode() == WXK_F2 )
	{
		if ( cursorCol == COL_ENTRY_TAGS || cursorCol == COL_SPAWN_TAG_ARRAY || cursorCol == COL_CHAR_TAGS )
		{
			RunCustomEditor( cursorRow, cursorCol );
		}
		else
		{
			event.Skip();
		}
	}
	else if ( event.GetKeyCode() == WXK_WINDOWS_MENU )
	{
		wxRect cellRect = m_grid->CellToRect( cursorRow, cursorCol );

		ContextMenu( cursorRow, cursorCol,
			wxPoint(m_grid->GetRowLabelSize() + cellRect.GetWidth() + cellRect.GetX(), m_grid->GetColLabelSize() + cellRect.GetHeight() + cellRect.GetY()) );

		event.Skip();
	}
	else
	{
		event.Skip();
	}
}

void CEdSpawnSetEditor::OnGridSize( wxSizeEvent& event )
{
	wxSize gridSize = event.GetSize();
	event.Skip();
}

void CEdSpawnSetEditor::ContextMenu( Int32 row, Int32 col, wxPoint menuPos /* = wxDefaultPosition */ )
{
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(row) ];

	switch ( col )
	{
	case COL_ENTITY_WEIGHT:
	case COL_ENTITY_APPEARANCES:
		{
			col = COL_ENTITY_TEMPLATE;
			m_grid->SetGridCursor( row, col );
			/* pass through */
		}
	case COL_ENTITY_TEMPLATE:
		{
			if ( IsRow( row ) )
			{
				ShowMainSubmenu( row, COL_ENTITY_TEMPLATE, TXT("entity template"), menuPos );
			}
			else // we are in subrow
			{
				ShowSubRowSubmenu( row, COL_ENTITY_TEMPLATE, TXT("entity template"), menuPos );
			}
			break;
		}
	case COL_SPAWN_TAG_ARRAY:
	case COL_SPAWN_QUANTITY_ARRAY:
		{
			col = COL_SPAWN_TIME_ARRAY;
			m_grid->SetGridCursor( row, col );
			/* pass through */
		}
	case COL_SPAWN_TIME_ARRAY:
		{
			if ( IsRow( row ) )
			{
				ShowMainSubmenu( row, COL_SPAWN_TIME_ARRAY, TXT("spawn"), menuPos );
			}
			else // we are in subrow
			{
				ShowSubRowSubmenu( row, COL_SPAWN_TIME_ARRAY, TXT("spawn"), menuPos );
			}
			break;
		}

	case COL_LAYER_WEIGHT_ARRAY:
		{
			col = COL_LAYER_ARRAY;
			m_grid->SetGridCursor( row, col );
			/* pass through */
		}
	case COL_LAYER_ARRAY:
		{
			if ( IsRow( row ) )
			{
				ShowMainSubmenu( row, COL_LAYER_ARRAY, TXT("layer"), menuPos );
			}
			else // we are in subrow
			{
				ShowSubRowSubmenu( row, COL_LAYER_ARRAY, TXT("layer"), menuPos );
			}
			break;
		}
	case COL_ENTRY_TAGS:
		{
			if ( IsRowComment( row ) ) // comment row
			{

			}
			else // entry tags
			{
				if ( IsRow( row ) )
				{
					ShowMainSubmenu( row, COL_ENTRY_TAGS, TXT("entry tags"), menuPos );
				}
				else // we are in subrow
				{
					ShowSubRowSubmenu( row, COL_ENTRY_TAGS, TXT("entry tags"), menuPos );
				}
			}
			break;
		}
	case COL_CHAR_TAGS:
		{
			if ( IsRow( row ) )
			{
				ShowMainSubmenu( row, COL_CHAR_TAGS, TXT("character tags"), menuPos );
			}
			else // we are in subrow
			{
				ShowSubRowSubmenu( row, COL_CHAR_TAGS, TXT("character tags"), menuPos );
			}
			break;
		}
	}
}

// deprecated
void CEdSpawnSetEditor::OnRangeSelect( wxGridRangeSelectEvent &event )
{
	if ( event.Selecting() )
	{
		m_grid->ClearSelection();
		m_grid->SetGridCursor( event.GetBottomRow(), event.GetRightCol() );
	}
}

void CEdSpawnSetEditor::AppendRow()
{
	CSpawnSetTimetable timetable;
	timetable.m_templates.PushBack( CSpawnSetTimetableTemplate() );
	m_spawnSet->GetTimetable().PushBack( timetable );

	InsertNewRow( m_grid->GetNumberRows() );

	CalcGridRowLabelValues();
	SetMainRows();
}

void CEdSpawnSetEditor::CopyCellsInColumn( int columnNumber, int topRow, int bottomRow /* inclusive */ )
{
	CopyCells( columnNumber, topRow, columnNumber, bottomRow );
}

void CEdSpawnSetEditor::CopyCellsInRow( int rowNumber, int leftColumn, int rightColumn /* inclusive */ )
{
	CopyCells( leftColumn, rowNumber, rightColumn, rowNumber );
}

void CEdSpawnSetEditor::CopyCells( int topLeftCol, int topLeftRow, int bottomRightCol, int bottomRightRow )
{
	ASSERT( topLeftCol <= bottomRightCol );
	ASSERT( topLeftRow <= bottomRightRow );

	CClipboardData *clipboardData = NULL;
	SClipboardData *data = new SClipboardData();

	// Initialize data
	data->m_data.Grow( bottomRightCol - topLeftCol + 1 );
	for ( int col = 0; col < bottomRightCol - topLeftCol + 1; ++col )
	{
		data->m_data[ col ].Grow( bottomRightRow - topLeftRow + 1 );
	}

	// Fill in data
	for ( int gridCol = topLeftCol; gridCol <= bottomRightCol; ++gridCol )
	{
		for ( int gridRow = topLeftRow; gridRow <= bottomRightRow; ++gridRow )
		{
			const int col = gridCol - topLeftCol;
			const int row = gridRow - topLeftRow;
			CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ GetSpawnSetRowNum(gridRow) ];

			if ( IsCellEnabled( gridRow, gridCol ) )
			{
				CName cellType = GetCellType( gridRow, gridCol );
				if ( cellType == RED_NAME( TagList ) )
				{
					TagList* tagList = reinterpret_cast< TagList* >( GetCellRawValue( gridRow, gridCol ) );
					if ( tagList )
					{
						data->m_data[ col ][ row ] = CVariant( TTypeName<TagList>::GetTypeName(), tagList );
					}
					else
						data->m_data[ col ][ row ] = CVariant();
				}
				else if ( cellType == RED_NAME( PointerToCEntityTemplate ) )
				{
					const CEntityTemplate *entityTemplate = reinterpret_cast< CEntityTemplate* >( GetCellRawValue( gridRow, gridCol ) );
					if ( entityTemplate )
					{
						data->m_data[ col ][ row ] = CVariant( TTypeName<CEntityTemplate*>::GetTypeName(), &entityTemplate );
					}
					else
						data->m_data[ col ][ row ] = CVariant();
				}
				else if ( cellType == RED_NAME( GameTime ) )
				{
					data->m_data[ col ][ row ] = CVariant( CNAME( GameTime ), GetCellRawValue( gridRow, gridCol ) );
				}
				else if ( cellType == RED_NAME( Int32 ) )
				{
					data->m_data[ col ][ row ] = CVariant( CNAME( Int32 ), GetCellRawValue( gridRow, gridCol ) );
				}
				else if ( cellType == RED_NAME( CName ) )
				{
					data->m_data[ col ][ row ] = CVariant( CNAME( CName ), GetCellRawValue( gridRow, gridCol ) );
				}
				else if ( cellType == RED_NAME( Float ) )
				{
					data->m_data[ col ][ row ] = CVariant( CNAME( Float ), GetCellRawValue( gridRow, gridCol ) );
				}
				else if ( cellType == RED_NAME( Bool ) )
				{
					data->m_data[ col ][ row ] = CVariant( CNAME( Bool ), GetCellRawValue( gridRow, gridCol ) );
				}
				else
				{
					HALT( "Copying of type %s not supported", cellType.AsString().AsChar() );
				}
			} else
			{
				data->m_data[ col ][ row ] = CVariant();
			}
		}
	}

	clipboardData = new CClipboardData( TXT("RECTANGLE") );
	clipboardData->SetData( TXT("RECTANGLE"), sizeof(SClipboardData), (void*)data );

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( clipboardData );
		wxTheClipboard->Close();
	}
}

Bool CEdSpawnSetEditor::IsEnoughSpaceInColumn( int columnNumber, int topRow, int bottomRow )
{
	if ( topRow >= 0 && columnNumber >= 0 )
	{
		for ( Int32 row = topRow; row < bottomRow; ++row )
		{
			if ( !IsCellEnabled( row, columnNumber ) ) return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

void CEdSpawnSetEditor::PrintCellBool( Int32 row, Int32 col, Bool value )
{
	if ( value )
	{
		m_grid->SetCellValue( row, col, TXT("1") );
	}
	else
	{
		m_grid->SetCellValue( row, col, wxEmptyString );
	}
}

Bool CEdSpawnSetEditor::GetCellBool( Int32 row, Int32 col )
{
	return wxGridCellBoolEditor::IsTrueValue( m_grid->GetCellValue( row, col ) );
}

void CEdSpawnSetEditor::OnMouseCaptureLost( wxMouseCaptureLostEvent& WXUNUSED(event) )
{
	// don't call event.Skip() here...
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, Int32 value )
{
	m_grid->SetCellValue( row, col, String::Printf( TXT("%d"), value ).AsChar() );
	m_grid->SetCellEditorNumber( row, col );
	m_grid->SetCellAlignment( row, col, wxALIGN_RIGHT, wxALIGN_CENTRE );
	return true;
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, Float value )
{
	m_grid->SetCellValue( row, col, String::Printf( TXT("%.2f"), value).AsChar() );
	m_grid->SetCellEditorFloat( row, col );
	m_grid->SetCellAlignment( row, col, wxALIGN_RIGHT, wxALIGN_CENTRE );
	return true;
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, Bool value )
{
	PrintCellBool( row, col, value );
	m_grid->SetCellEditorBool( row, col );
	m_grid->SetCellRendererBool( row, col );
	m_grid->SetCellAlignment( row, col, wxALIGN_CENTRE, wxALIGN_CENTRE );
	return true;
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, const String &value )
{
	m_grid->SetCellValue( row, col, value.AsChar() );
	return true;
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, CName value )
{
	m_grid->SetCellValue( row, col, value.AsChar() );
	return true;
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, const GameTime *value )
{
	if ( value )
	{
		PrintCellGameTime( row, col, value );
		m_grid->SetCellAlignment( row, col, wxALIGN_CENTRE, wxALIGN_CENTRE );
		return true;
	}
	else
	{
		return false;
	}
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, const CEntityTemplate *value )
{
	if ( value )
		m_grid->SetCellValue( row, col, value->GetFile()->GetFileName().AsChar() );
	else
		m_grid->SetCellValue( row, col, TXT("None") );
	m_grid->SetReadOnly( row, col );
	m_grid->SetCellRendererTemplate( row, col);
	return true;
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, const TDynArray<CName> &value )
{
	m_grid->SetCellValue( row, col, CNameArrayToString(value).AsChar() );
	m_grid->SetReadOnly( row, col );
	return true;
}

Bool CEdSpawnSetEditor::SetCell( Int32 row, Int32 col, const TagList& value )
{
	m_grid->SetCellValue( row, col, TagListArrayToString( value ).AsChar() );
	m_grid->SetReadOnly( row, col );
	return true;
}

CName CEdSpawnSetEditor::GetCellType( Int32 row, Int32 col )
{
	if ( IsRowComment( row ) )
	{
		return CNAME( Comment );
	}
	else if ( IsCellEnabled( row, col ) )
	{
		switch( col )
		{
		case COL_ENTRY_TAGS:
			return TTypeName<TagList>::GetTypeName();
		case COL_ENTITY_TEMPLATE:
			return TTypeName<CEntityTemplate*>::GetTypeName();
		case COL_ENTITY_APPEARANCES:
			return TTypeName<TDynArray<CName>>::GetTypeName();
		case COL_ENTITY_WEIGHT:
			return CNAME( Float );
		case COL_CHAR_TAGS:
			return TTypeName<TagList>::GetTypeName();
		case COL_SPAWN_TIME_ARRAY:
			return CNAME( GameTime );
		case COL_SPAWN_QUANTITY_ARRAY:
			return CNAME( Int32 );
		case COL_SPAWN_TAG_ARRAY:
			return TTypeName<TagList>::GetTypeName();
		case COL_LAYER_ARRAY:
			return CNAME( CName );
		case COL_LAYER_WEIGHT_ARRAY:
			return CNAME( Float );
		case COL_USE_EXISTING:
			return CNAME( Bool );
		default:
			ASSERT( 0 && TXT("Unknown cell type.") );
			return CName();
		}
	}
	else
	{
		return CName();
	}
}

void* CEdSpawnSetEditor::GetCellRawValue( Int32 row, Int32 col )
{
	ASSERT( row >= 0 );
	ASSERT( col >= 0 );

	const Int32 ssRowNum = GetSpawnSetRowNum( row );
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ ssRowNum ];

	if ( IsRowComment( row ) )
	{
		return &sst.m_comment;
	}
	else if ( IsCellEnabled( row, col ) )
	{
		switch( col )
		{
		case COL_ENTRY_TAGS:
			{
				return &sst.m_entryTags;
			}
		case COL_ENTITY_TEMPLATE:
			{
				return sst.m_templates[ GetSpawnSetSubRowNum(row) ].m_template;
			}
		case COL_ENTITY_APPEARANCES:
			{
				return &sst.m_templates[ GetSpawnSetSubRowNum(row) ].m_appearances;
			}
		case COL_ENTITY_WEIGHT:
			{
				return &sst.m_templates[ GetSpawnSetSubRowNum(row) ].m_weight;
			}
		case COL_CHAR_TAGS:
			{
				return &sst.m_characterTags;
			}
		case COL_SPAWN_TIME_ARRAY:
			{
				return &sst.m_spawnArray[ GetSpawnSetSubRowNum(row) ].m_time;
			}
		case COL_SPAWN_QUANTITY_ARRAY:
			{
				return &sst.m_spawnArray[ GetSpawnSetSubRowNum(row) ].m_quantity;
			}
		case COL_SPAWN_TAG_ARRAY:
			{
				return &sst.m_spawnArray[ GetSpawnSetSubRowNum(row) ].m_spawnpointTag;
			}
		case COL_LAYER_ARRAY:
			{
				return &sst.m_layerArray[ GetSpawnSetSubRowNum(row) ].m_layer;
			}
		case COL_LAYER_WEIGHT_ARRAY:
			{
				return &sst.m_layerArray[ GetSpawnSetSubRowNum(row) ].m_weight;
			}
		case COL_USE_EXISTING:
			{
				return &sst.m_useExistingCharacters;
			}
		default:
			ASSERT( 0 && TXT("Unknown cell type.") );
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

void CEdSpawnSetEditor::SetCellRawValue( Int32 row, Int32 col, void *data )
{
	ASSERT( row >= 0 );
	ASSERT( col >= 0 );

	const Int32 ssRowNum = GetSpawnSetRowNum( row );
	CSpawnSetTimetable &sst = m_spawnSet->GetTimetable()[ ssRowNum ];

	if ( IsRowComment( row ) )
	{
		// &sst.m_comment;
	}
	else if ( IsCellEnabled( row, col ) )
	{
		switch( col )
		{
		case COL_ENTRY_TAGS:
			{
				sst.m_entryTags = *(TagList*)data;
				SetCell( row, col, *(TagList*)data );
				break;
			}
		case COL_ENTITY_TEMPLATE:
			{
				sst.m_templates[ GetSpawnSetSubRowNum(row) ].m_template = (CEntityTemplate*)data;
				SetCell( row, col, (CEntityTemplate*)data );
				break;
			}
		case COL_ENTITY_APPEARANCES:
			{
				sst.m_templates[ GetSpawnSetSubRowNum(row) ].m_appearances = *(TDynArray<CName>*)data;
				SetCell( row, col, *(TDynArray<CName>*)data );
				break;
			}
		case COL_ENTITY_WEIGHT:
			{
				sst.m_templates[ GetSpawnSetSubRowNum(row) ].m_weight = *(Float*)data;
				SetCell( row, col, *(Float*)data );
				break;
			}
		case COL_CHAR_TAGS:
			{
				sst.m_characterTags = *(TagList*)data;
				SetCell( row, col, *(TagList*)data );
				break;
			}
		case COL_SPAWN_TIME_ARRAY:
			{
				sst.m_spawnArray[ GetSpawnSetSubRowNum(row) ].m_time = *(GameTime*)data;
				SetCell( row, col, (GameTime*)data );
				break;
			}
		case COL_SPAWN_QUANTITY_ARRAY:
			{
				sst.m_spawnArray[ GetSpawnSetSubRowNum(row) ].m_quantity = *(Int32*)data;
				SetCell( row, col, *(Int32*)data );
				break;
			}
		case COL_SPAWN_TAG_ARRAY:
			{
				sst.m_spawnArray[ GetSpawnSetSubRowNum(row) ].m_spawnpointTag = *(TagList*)data;
				SetCell( row, col, *(TagList*)data );
				break;
			}
		case COL_LAYER_ARRAY:
			{
				sst.m_layerArray[ GetSpawnSetSubRowNum(row) ].m_layer = *(CName*)data;
				SetCell( row, col, *(CName*)data );
				break;
			}
		case COL_LAYER_WEIGHT_ARRAY:
			{
				sst.m_layerArray[ GetSpawnSetSubRowNum(row) ].m_weight = *(Float*)data;
				SetCell( row, col, *(Float*)data );
				break;
			}
		case COL_USE_EXISTING:
			{
				sst.m_useExistingCharacters = *(Bool*)data;
				SetCell( row, col, *(Bool*)data );
				break;
			}
		default:
			{
				ASSERT( 0 && TXT("Unknown cell type.") );
				break;
			}
		}
	}
	else
	{
		// do nothing
	}
}

//////////////////////////////////////////////////////////////////////////

void CTemplateCellRenderer::Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rectCell, int row, int col, bool isSelected )
{
	wxGridCellStringRenderer::Draw( grid, attr, dc, rectCell, row, col, isSelected );

	if ( grid.GetCursorRow() == row && grid.GetCursorColumn() == col && grid.GetCellValue( row, col ) != wxEmptyString )
	{
		// For drawing icons frames
		// We don't want icons frames for now as they looks ugly
		//const wxColour back( 255, 255, 255 );
		//const wxColour frame( 128, 128, 128 );
		//dc.SetPen( wxPen( frame ) );
		//dc.SetBrush( wxBrush( back ) );

		// Draw icons
		wxRect iconRect( 0, 0, m_iconUse.GetWidth(), m_iconUse.GetHeight() );
		iconRect = iconRect.CenterIn( rectCell );

		iconRect.SetX( rectCell.GetX() + rectCell.GetWidth() - ICON_WIDTH * 1 );
		//dc.DrawRoundedRectangle( wxRect(iconRect).Inflate(3), 1 );
		dc.DrawBitmap( m_iconClear, iconRect.x, iconRect.y, true );

		iconRect.SetX( rectCell.GetX() + rectCell.GetWidth() - ICON_WIDTH * 2 );
		//dc.DrawRoundedRectangle( wxRect(iconRect).Inflate(3), 1 );
		dc.DrawBitmap( m_iconUse, iconRect.x, iconRect.y, true );

		iconRect.SetX( rectCell.GetX() + rectCell.GetWidth() - ICON_WIDTH * 3 );
		//dc.DrawRoundedRectangle( wxRect(iconRect).Inflate(3), 1 );
		dc.DrawBitmap( m_iconBrowse, iconRect.x, iconRect.y, true );
	}
}

wxSize CTemplateCellRenderer::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
{
	const Int32 ICONS_WIDTH = ICON_WIDTH * 3;
	wxSize size = wxGridCellStringRenderer::GetBestSize( grid, attr, dc, row, col );
	size.SetWidth( size.GetWidth() + ICONS_WIDTH );
	return size;
}

CTemplateCellRenderer::CTemplateCellRenderer()
{
	m_iconUse    = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
	m_iconBrowse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_BROWSE") );
	m_iconClear  = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
}

//////////////////////////////////////////////////////////////////////////

wxPen CSpawnSetGridGrid::GetRowGridLinePen( Int32 row )
{
	if ( m_mainRows.Find( row ) != m_mainRows.End() )
	{
		return m_mainRowPen;
	}
	else
	{
		return GetDefaultGridLinePen();
	}
}

//////////////////////////////////////////////////////////////////////////

wxPen CSpawnSetGridSheet::GetRowGridLinePen( Int32 row )
{
	if ( m_mainRows.Find( row ) != m_mainRows.End() )
	{
		return m_mainRowPen;
	}
	else
	{
		return m_mainRowPen;
	}
}

#endif