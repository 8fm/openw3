/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "2dArrayEditor.h"
#include "sheet\sheet.h"
#include "assetBrowser.h"
#include "..\..\common\core\diskFile.h"
#include "..\..\common\core\feedback.h"

// Event table
BEGIN_EVENT_TABLE( CEd2dArrayEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "menuSave" ), CEd2dArrayEditor::OnSave )
	EVT_MENU( XRCID( "menuClose" ), CEd2dArrayEditor::OnExit )
	EVT_MENU( XRCID( "viewAutoSize" ), CEd2dArrayEditor::OnAutoSize )
	EVT_MENU( XRCID( "viewFitCells" ), CEd2dArrayEditor::OnFitCells )
	EVT_BUTTON( XRCID( "searchButton"), CEd2dArrayEditor::OnSearchButtonClicked )
	EVT_BUTTON( XRCID( "nextButton"), CEd2dArrayEditor::OnSearchNextResult )
	EVT_BUTTON( XRCID( "prevButton"), CEd2dArrayEditor::OnSearchPrevResult )
END_EVENT_TABLE()

namespace
{
	RED_INLINE Uint32 GetColumnInArrayCoords( Uint32 col )
	{
		return col - 1;
	}

	RED_INLINE Uint32 GetColumnInSheetCoords( Uint32 col )
	{
		return col + 1;
	}
}

class C2dArrayHelpInfo : public wxObject
{
public:
	Int32 m_num;

	C2dArrayHelpInfo(Int32 num) : m_num(num) {}
};

CEd2dArrayEditor::CEd2dArrayEditor(wxWindow* parent, C2dArray* array )
	: wxSmartLayoutPanel( parent, TXT("2dArrayEditor"), false )
    , m_array(array), m_panel(NULL), m_grid(NULL), m_arrayIsSortAsc(true)
	, m_currentSearchResultIdx(0)
{
	// Load designed frame from resource
	//wxXmlResource::Get()->LoadFrame( this, parent, TXT("2dArrayEditor") );

	// Add reference to resource
	m_array->AddToRootSet();

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );

	// Set title for newly created window
	if (m_array->GetFile())
	{
		String newTitle = m_array->GetFile()->GetFileName() + TXT(" - 2d Array Editor");
		SetTitle( wxString(newTitle.AsChar()) );
	}
	else
	{
		String newTitle = TXT("2d Array Editor");
		SetTitle( wxString(newTitle.AsChar()) );
	}

	m_panel = XRCCTRL( *this, "gridPanel", wxPanel );
    m_panel->Connect( wxEVT_SIZE, wxSizeEventHandler(CEd2dArrayEditor::OnSize), NULL, this );

	m_searchLine = XRCCTRL( *this, "findTextBox", wxTextCtrl );
	m_searchLabel = XRCCTRL( *this, "searchLabel", wxStaticText );

	m_searchLine->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler(CEd2dArrayEditor::OnSearchLineEnterDown), nullptr, this );

	m_grid = new wxSheet(m_panel, wxID_ANY);
	m_grid->SetMinimalAcceptableColWidth(30);
	m_grid->SetMinSize(wxSize(-1,-1));

	m_grid->Connect( wxEVT_SHEET_CELL_VALUE_CHANGED, wxSheetEventHandler(CEd2dArrayEditor::OnGridCellChange), NULL, this);
	m_grid->Connect( wxEVT_CHAR, wxKeyEventHandler(CEd2dArrayEditor::OnCopyPaste), NULL, this);

	m_grid->GetWindowChild( wxSheet::ID_COL_LABEL_WINDOW )->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler(CEd2dArrayEditor::OnMouseRightColumnLabel), NULL, this);
	m_grid->GetWindowChild( wxSheet::ID_ROW_LABEL_WINDOW )->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler(CEd2dArrayEditor::OnMouseRightRowLabel), NULL, this);
	m_grid->GetWindowChild( wxSheet::ID_GRID_WINDOW )->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler(CEd2dArrayEditor::OnMouseRightCell), NULL, this);
	//newGrid->Bind(wxEVT_GRID_CELL_RIGHT_CLICK, &CEdDataErrorReporterWindow::OnShowRowContextMenu, this );
	
	m_grid->CreateGrid(0,0);
	m_grid->SetAttrReadOnly(wxColLabelSheetCoords, false, wxSHEET_AttrDefault);
	m_grid->SetRowLabelWidth(0);

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadToConfirm ), this );

	m_originalArrayLayout.ResizeFast(m_array->GetNumberOfRows());
	for( Uint32 i=0; i<m_array->GetNumberOfRows(); ++i )
	{
		m_originalArrayLayout[i] = i;
	}

	// Fill grid
	UpdateGrid();

    // Update and finalize layout
	Fit();
	Layout();
    LoadOptionsFromConfig();
    Show();
}

CEd2dArrayEditor::~CEd2dArrayEditor()
{
	SaveOptionsToConfig();

	// Remove resource reference
	m_array->RemoveFromRootSet();

	// Unregister hooks
	SEvents::GetInstance().UnregisterListener( this );
}

void CEd2dArrayEditor::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Frames/2dArrayEditor"));
}

void CEd2dArrayEditor::LoadOptionsFromConfig()
{
	LoadLayout(TXT("/Frames/2dArrayEditor"));
}

void CEd2dArrayEditor::SetCellColor( const wxSheetCoords& cords, const wxColor& color )
{
	m_grid->SetAttrBackgroundColour( cords, color );
}

Uint32 CEd2dArrayEditor::GetRowsNumber()
{
	return m_grid->GetNumberRows();
}

Uint32 CEd2dArrayEditor::GetColsNumber()
{
	return m_grid->GetNumberCols();
}

void CEd2dArrayEditor::OnSave( wxCommandEvent& event )
{
	wxString msgCol = wxT("Columns names are not unique.");
	wxString msgIdx = wxT("Column index is not unique.");
	wxString msgSave = wxT("\n\nDo you want to save file?");

	// Disable (close) the cell editor (if any) to avoid losing changes while it is active
	m_grid->DisableCellEditControl( true );

	// No unique column and no unique column names
	if (!m_array->HasUniqueColumn(TXT("Index")) && !m_array->HasUniqueColumnNames())
	{
		wxMessageDialog dialog(0, msgCol+wxT("\n")+msgIdx+msgSave, wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
		if ( dialog.ShowModal() == wxID_YES )
		{
			m_array->Save();
		}
	}
	// No unique column
	else if (!m_array->HasUniqueColumn(TXT("Index")))
	{
		wxMessageDialog dialog(0, msgIdx+msgSave, wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
		if ( dialog.ShowModal() == wxID_YES )
		{
			m_array->Save();
		}
	}
	// No unique column names
	else if (!m_array->HasUniqueColumnNames())
	{
		wxMessageDialog dialog(0, msgCol+msgSave, wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
		if ( dialog.ShowModal() == wxID_YES )
		{
			m_array->Save();
		}
	}
	else
	{
		m_array->Save();
		SEvents::GetInstance().DispatchEvent( CNAME( CSVFileSaved ), CreateEventData( m_array->GetFile()->GetFileName() ) );
	}
}

void CEd2dArrayEditor::OnExit( wxCommandEvent& event )
{
	ClosePanel();
}

void CEd2dArrayEditor::OnAutoSize( wxCommandEvent& event )
{
	if (m_grid)
	{
		m_grid->AutoSize();
		Layout();
	}
}

void CEd2dArrayEditor::UpdateGrid()
{
	Uint32 uRowNum, uColNum;
	m_array->GetSize(uColNum, uRowNum);

	Int32 rowNum = uRowNum;
	Int32 colNum = GetColumnInSheetCoords( uColNum );

	// Clear all
	if (m_grid->GetNumberCols() > colNum) m_grid->DeleteCols( 1, m_grid->GetNumberCols()-colNum);
	if (m_grid->GetNumberCols() < colNum) m_grid->AppendCols( colNum-m_grid->GetNumberCols());

	if (m_grid->GetNumberRows() > rowNum) m_grid->DeleteRows( 1, m_grid->GetNumberRows()-rowNum);
	if (m_grid->GetNumberRows() < rowNum) m_grid->AppendRows( rowNum-m_grid->GetNumberRows());

	// Headers
	for (Uint32 col=0; col<uColNum; col++)
	{
		wxString data = m_array->GetHeader(col).AsChar();
		m_grid->SetColLabelValue( GetColumnInSheetCoords( col ), data );
		m_grid->SetAttrEditor( wxSheetCoords(-1, GetColumnInSheetCoords( col ) ), wxSheetCellEditor(new wxSheetCellAutoWrapStringEditorRefData()));
	}
	// Data
	for (Uint32 col=0; col<uColNum; col++)
	{
		for (Uint32 row=0; row<uRowNum; row++)
		{
			wxString data = m_array->GetValue(col,row).AsChar();
			//wxSize cellSize = m_array->get
			m_grid->SetCellValue( wxSheetCoords(row, GetColumnInSheetCoords( col ) ), data);
			m_grid->SetAttrEditor( wxSheetCoords(row, GetColumnInSheetCoords( col ) ), wxSheetCellEditor(new wxSheetCellAutoWrapStringEditorRefData()));
		}
	}

	// Line Numbers
	m_grid->SetColLabelValue( 0, wxString("Line") );
	m_grid->SetAttrReadOnly( wxSheetCoords(-1, 0), true );
	m_grid->SetAttrBackgroundColour( wxSheetCoords(-1, 0), *wxLIGHT_GREY );

	for (Uint32 row=0; row<uRowNum; row++)
	{
		wxString data;
		data << static_cast<int>( row + 1 );
		m_grid->SetCellValue( wxSheetCoords( row, 0 ), data );
		m_grid->SetAttrReadOnly( wxSheetCoords(row, 0), true );
		m_grid->SetAttrBackgroundColour( wxSheetCoords(row, 0), *wxLIGHT_GREY );
		m_grid->SetAttrAlignment( wxSheetCoords(row, 0), wxALIGN_RIGHT );
	}

	m_grid->SetVerticalScrollBarMode(wxSheet::SB_AS_NEEDED);
	m_grid->SetHorizontalScrollBarMode(wxSheet::SB_AS_NEEDED);
	m_grid->AdjustScrollbars();
	m_grid->Refresh();

	SEvents::GetInstance().DispatchEvent( CNAME( 2DArrayModified ), CreateEventData( m_array ) );
}

void CEd2dArrayEditor::OnGridCellChange( wxSheetEvent& event )
{
	String value = m_grid->GetCellValue( wxSheetCoords(event.GetCoords().GetRow(), event.GetCoords().GetCol()) );//event.GetString();
	
	if (event.GetCoords().GetRow()==-1)
	{
		m_array->SetHeader( value, GetColumnInArrayCoords( event.GetCoords().GetCol() ) );
	}
	else
	{
		m_array->SetValue( value, GetColumnInArrayCoords( event.GetCoords().GetCol() ), event.GetCoords().GetRow());
	}

	UpdateGrid();

	event.Skip();
}

void CEd2dArrayEditor::OnMouseRightRowLabel( wxMouseEvent& event )
{
	event.Skip();
}

void CEd2dArrayEditor::OnMouseRightColumnLabel( wxMouseEvent& event )
{
	int x,y;
	wxPoint mousePos = event.GetPosition();
	m_grid->CalcUnscrolledPosition( mousePos.x, mousePos.y, &x, &y );
	wxSheetCoords coords(m_grid->XYToGridCell( x, y ));
	//wxSheetCoords coords(m_grid->XYToGridCell( x, y ));
	//m_coords = m_grid->XYToGridCell( x, y );
	if (GetColumnInArrayCoords( coords.GetCol() ) != -1)
	{
		wxMenu menu;

		menu.Append( 1, TXT( "Insert column" ) );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnInsertColumn ), new C2dArrayHelpInfo(coords.GetCol()) , this );

		menu.Append( 2, TXT( "Add column" ) );
		menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnAddColumn ), NULL, this );

		menu.Append( 3, TXT( "Delete column" ) );
		menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnDeleteColumn ), new C2dArrayHelpInfo(coords.GetCol()), this );

		menu.Append( 4, TXT( "Sort" ) );
		menu.Connect( 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnSortByColumn ), new C2dArrayHelpInfo(coords.GetCol()), this );

		menu.Append( 5, TXT( "Revert sorting" ) );
		menu.Connect( 5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnSortByColumn ), new C2dArrayHelpInfo(m_array->GetNumberOfColumns()+1), this );

		wxTheFrame->PopupMenu( &menu );
	}
	else
	{
		wxMenu menu;

		menu.Append( 1, TXT( "Add column" ) );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnAddColumn ), NULL, this );

		wxTheFrame->PopupMenu( &menu );
	}
}

void CEd2dArrayEditor::OnMouseRightCell( wxMouseEvent& event )
{
	int x,y;
	wxPoint mousePos = event.GetPosition();
	m_grid->CalcUnscrolledPosition( mousePos.x, mousePos.y, &x, &y );

	wxSheetCoords coords(m_grid->XYToGridCell( x, y ));

	if (coords.GetRow() != -1)
	{
		wxString cellString = m_grid->GetCellValue(coords);
		m_cellString = cellString.c_str();
		wxMenu menu;

		menu.Append( 1, TXT( "Insert row" ) );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnInsertRow ), new C2dArrayHelpInfo(coords.GetRow()) , this );

		menu.Append( 2, TXT( "Add row" ) );
		menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnAddRow ), NULL, this );

		menu.Append( 3, TXT( "Delete row" ) );
		menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnDeleteRow ), new C2dArrayHelpInfo(coords.GetRow()), this );

		// create context menu for Cell with a path in
		if ( cellString.Contains( TXT("\\") ) )
		{
			menu.Append( 4, TXT("Show in asset browser") );
			menu.Append( 5, TXT("Show in explorer") );

			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEd2dArrayEditor::OnRowPopupClick, this );
		}

		wxTheFrame->PopupMenu( &menu );
	}
	else
	{
		wxMenu menu;

		menu.Append( 2, TXT( "Add row" ) );
		menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEd2dArrayEditor::OnAddRow ), NULL, this );;

		wxTheFrame->PopupMenu( &menu );
	}
}

void CEd2dArrayEditor::OnRowPopupClick( wxCommandEvent &event )
{
	Uint32 option = event.GetId();

	if( option == 4 )
	{
		ShowInAssetBrowser();
	}
	else if( option == 5 )
	{
		ShowInExplorer();
	}
}

void CEd2dArrayEditor::ShowInAssetBrowser()
{
	SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( m_cellString ) );
}

void CEd2dArrayEditor::ShowInExplorer()
{
		wxString cmd = wxT( "explorer /select," );
		cmd += ( GFileManager->GetDataDirectory() + m_cellString ).AsChar();

		::wxExecute( cmd, wxEXEC_ASYNC, nullptr );
}

void CEd2dArrayEditor::OnAddColumn( wxCommandEvent& event )
{
	m_array->AddColumn();
	UpdateGrid();
}

void CEd2dArrayEditor::OnAddRow( wxCommandEvent& event )
{
	TDynArray< String > row;
	m_array->AddRow(row);
	m_originalArrayLayout.PushBack(m_originalArrayLayout.Size());
	UpdateGrid();
}

void CEd2dArrayEditor::OnInsertColumn( wxCommandEvent& event )
{
	C2dArrayHelpInfo* helpInfo = static_cast< C2dArrayHelpInfo* >( event.m_callbackUserData );
	if ( helpInfo )
	{
		m_array->InsertColumn(helpInfo->m_num);
		UpdateGrid();
	}
}

void CEd2dArrayEditor::OnInsertRow( wxCommandEvent& event )
{
	C2dArrayHelpInfo* helpInfo = static_cast< C2dArrayHelpInfo* >( event.m_callbackUserData );
	if ( helpInfo )
	{
		TDynArray< String > row;
		m_array->InsertRow(helpInfo->m_num, row);
		UpdateGrid();
	}
}

void CEd2dArrayEditor::OnDeleteColumn( wxCommandEvent& event )
{
	C2dArrayHelpInfo* helpInfo = static_cast< C2dArrayHelpInfo* >( event.m_callbackUserData );
	if ( helpInfo )
	{
		m_array->DeleteColumn(helpInfo->m_num);
		UpdateGrid();
	}
}

void CEd2dArrayEditor::OnDeleteRow( wxCommandEvent& event )
{
	C2dArrayHelpInfo* helpInfo = static_cast< C2dArrayHelpInfo* >( event.m_callbackUserData );
	if ( helpInfo )
	{
		m_array->DeleteRow(helpInfo->m_num);
		Uint32 removedValue = m_originalArrayLayout[helpInfo->m_num];
		m_originalArrayLayout.RemoveAt(helpInfo->m_num);
		for( Uint32 i=0; i<m_originalArrayLayout.Size(); ++i )
		{
			if( m_originalArrayLayout[i] > removedValue )
			{
				m_originalArrayLayout[i] -= 1;
			}
		}
		UpdateGrid();
	}
}

void CEd2dArrayEditor::OnCopyPaste( wxKeyEvent& event )
{
	if (!m_grid->HasFocus())
		event.Skip();

	if (event.ControlDown())
	{
		if (event.GetKeyCode()==3)
		{
			m_grid->CopyCurrentSelectionInternal();
		}
		else if (event.GetKeyCode()==22)
		{
			m_grid->PasteInternalCopiedSelection();
			OnPostPaste();
		}
		else
			event.Skip();
	}
	else
		event.Skip();
}

void CEd2dArrayEditor::OnPostPaste()
{
	Uint32 colNum, rowNum;
	m_array->GetSize(colNum,rowNum);

	for (Uint32 col=0; col<colNum; col++)
	{
		for (Uint32 row=0; row<rowNum; row++)
		{
			String value = m_grid->GetCellValue(wxSheetCoords(row,GetColumnInSheetCoords(col)));
			m_array->SetValue(value , col, row);
		}
	}
	for (Uint32 col=0; col<colNum; col++)
	{
		String value = m_grid->GetCellValue(wxSheetCoords(-1,GetColumnInSheetCoords(col)));
		m_array->SetHeader(value , col);
	}

	UpdateGrid();
}

void CEd2dArrayEditor::OnSize(wxSizeEvent& event )
{
	if (m_panel && m_grid)
	{
		wxSize s = m_panel->GetSize();
		m_grid->SetSize( s );
	}
	event.Skip();
}

void CEd2dArrayEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == m_array )
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, this->GetTitle().wc_str() ) ) );
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		if ( reloadInfo.m_newResource->IsA<C2dArray >() )
		{
			C2dArray* oldArray = (C2dArray*)( reloadInfo.m_oldResource );
			C2dArray* newArray = (C2dArray*)( reloadInfo.m_newResource );
			if ( oldArray == m_array )
			{
				// Change
				m_array = (C2dArray*)(reloadInfo.m_newResource);
				m_array->AddToRootSet();
				UpdateGrid();

				// Update
				wxTheFrame->GetAssetBrowser()->OnEditorReload( m_array, this );
			}
		}
	}
}

void CEd2dArrayEditor::OnSortByColumn( wxCommandEvent& event )
{
	typedef TDynArray< String, MC_Arrays2D >		TStringArray;
	typedef TDynArray< TStringArray, MC_Arrays2D >	TStringArray2D;

	struct SLocalArrayComparer
	{
		typedef TDynArray< String, MC_Arrays2D >		TStringArray;

		enum EDataType
		{
			DT_Number,
			DT_String,
		};

		Int32 m_sortByColId;
		Bool m_ascendingSort;
		EDataType m_dataType;
		THashMap<String, Float> m_parsedFloats;

		SLocalArrayComparer( Int32 sortByColId, EDataType dataType = DT_String, Bool ascendingSort = true )
			: m_sortByColId( sortByColId )
			, m_ascendingSort( ascendingSort )
			, m_dataType(dataType)
		{

		}

		void InitializeComparerMode( TStringArray2D& originalArray, Uint32 selColIdx )
		{
			m_dataType = DT_Number;	// Let's assume that it's numbers and check the assumption

			Float checkBuf = 0.0f;
			for(Uint32 rowIdx=0; rowIdx < originalArray.Size(); ++rowIdx)
			{
				if( m_parsedFloats.Find( originalArray[rowIdx][selColIdx], checkBuf ) == false )
				{
					if( FromString<Float>(originalArray[rowIdx][selColIdx], checkBuf) == false )
					{
						m_dataType = SLocalArrayComparer::DT_String;
						m_parsedFloats.Clear();
						break;
					}
					else
					{
						m_parsedFloats.Insert(originalArray[rowIdx][selColIdx], checkBuf);
					}
				}
			}
		}

		RED_INLINE Int32 CompareAsStrings( TStringArray& rowsA, TStringArray& rowsB ) const
		{
			return Red::System::StringCompareNoCase( rowsA[m_sortByColId].AsChar(), rowsB[m_sortByColId].AsChar() );
		}

		RED_INLINE Int32 CompareAsNumbers( TStringArray& rowsA, TStringArray& rowsB ) const
		{
			Float a, b;

			if( rowsA.Data() == nullptr || rowsA.Empty() == true || rowsA[m_sortByColId].Size() == 0 )
				return 0;

			if( rowsB.Data() == nullptr || rowsB.Empty() == true || rowsB[m_sortByColId].Size() == 0 )
				return 0;

			// With hash table
			a = m_parsedFloats[rowsA[m_sortByColId]];
			b = m_parsedFloats[rowsB[m_sortByColId]];

			// Without hash table
			//FromString<Float>(rowsA[m_sortByColId], a);
			//FromString<Float>(rowsB[m_sortByColId], b);

			if( b > a)
			{
				return 1;
			}
			else if( b < a )
			{
				return -1;
			}
			//else
			return 0;
		}

		Bool operator()( TStringArray& rowsA, TStringArray& rowsB ) const
		{
			Int32 compareResult = 0;

			switch( m_dataType )
			{
			case DT_Number:
				compareResult = CompareAsNumbers( rowsA, rowsB );
				break;
			case DT_String:
				compareResult = CompareAsStrings( rowsA, rowsB );
				break;
			default:
				break;
			}
			if( m_ascendingSort == true )
			{
				return compareResult < 0;
			}
			else
			{
				return compareResult > 0;
			}
		}
	};

	C2dArrayHelpInfo* helpInfo = static_cast< C2dArrayHelpInfo* >( event.m_callbackUserData );
	if ( helpInfo )
	{
		// Get selected cell indices
		Uint32 selColIdx = static_cast<Uint32>( GetColumnInArrayCoords( helpInfo->m_num ) );

		// Check if selected column is in array range
		if( 0 <= selColIdx && selColIdx <= m_array->GetNumberOfColumns() )		// if selColIdx == Num of Cols -> recover original layout of the array
		{
			GFeedback->BeginTask( TXT("Sorting ..."), false );

			TStringArray2D& originalArray = m_array->GetData();

			// Add temporary column to keep track of the original layout of the array
			Uint32 tempColIdx = m_array->GetNumberOfColumns();
			m_array->AddColumn( TXT("temp_layout_tracker") );

			Int32 rowNumDiff = static_cast<Int32>( m_array->GetNumberOfRows() ) - static_cast<Int32>( m_originalArrayLayout.Size() );
			// Add rows to original array layout tracker
			for( Int32 r=rowNumDiff; r>0; --r )
			{
				m_originalArrayLayout.PushBack( m_originalArrayLayout.Size() );
			}
			// Subtract rows to original array layout tracker
			if( rowNumDiff < 0 )
			{
				m_originalArrayLayout.ResizeFast( m_originalArrayLayout.Size() + rowNumDiff );
			}

			for( Uint32 r=0; r<m_array->GetNumberOfRows(); ++r )
			{
				Uint32 tempVal = m_originalArrayLayout[r];
				String tempStr = ToString( tempVal );
				originalArray[r][tempColIdx] = tempStr;
			}

			// Create comparer
			bool sortAsc = (selColIdx == m_array->GetNumberOfColumns() - 1) ? false : m_arrayIsSortAsc;		// if selColIdx is last column, then sort reverting is on
			SLocalArrayComparer comparer( selColIdx, SLocalArrayComparer::DT_Number, sortAsc );
			comparer.InitializeComparerMode( originalArray, selColIdx );

			// Sort with comparer
			PC_SCOPE(HugeArray);
			Sort( originalArray.Begin(), originalArray.End(), comparer );

			// Erase temporary layout tracker from the array
			Uint32 tempBuf = 0;
			for( Uint32 r=0; r<m_array->GetNumberOfRows(); ++r )
			{
				FromString( originalArray[r][tempColIdx], tempBuf );
				m_originalArrayLayout[r] = tempBuf;
			}
			m_array->DeleteColumn(tempColIdx);

			// Update view in grid
			UpdateGrid();

			m_arrayIsSortAsc = !m_arrayIsSortAsc;	// Toggle ascend/descend mode

			GFeedback->EndTask();
		}
	}
}

void CEd2dArrayEditor::FindCellByContent(String& text)
{
	typedef TDynArray< String, MC_Arrays2D >		TStringArray;
	typedef TDynArray< TStringArray, MC_Arrays2D >	TStringArray2D;

	TStringArray2D& arrayData = m_array->GetData();

	for( Uint32 row = 0; row < m_array->GetNumberOfRows(); ++row )
	{
		for( Uint32 col = 0; col < m_array->GetNumberOfColumns(); ++col )
		{
			if(arrayData[row][col].ContainsSubstring( text ))
			{
				m_searchResultCoords.PushBack( TPair<Uint32, Uint32>( row, col ) );
			}
		}
	}
}

void CEd2dArrayEditor::OnSearchButtonClicked(wxCommandEvent& event)
{
	String textLine = m_searchLine->GetValue();

	m_searchResultCoords.Clear();
	m_currentSearchResultIdx = 0;
	
	if( textLine.Empty() == false )
	{
		FindCellByContent( textLine );
	}

	SkipToSelectedRow();
}

void CEd2dArrayEditor::SkipToSelectedRow()
{
	if( m_searchResultCoords.Size() > 0 )
	{
		Uint32 row = m_searchResultCoords[m_currentSearchResultIdx].m_first;
		Uint32 col = GetColumnInSheetCoords( m_searchResultCoords[m_currentSearchResultIdx].m_second );

		m_grid->SelectCell( wxSheetCoords(row, col ) );

		wxRect destCellRect = m_grid->CellToRect( wxSheetCoords( row, col ) );
		int rowPosition = destCellRect.GetPosition().y;

		m_grid->SetGridOrigin( -1, rowPosition );

		wxString labelText;
		labelText << (int)m_currentSearchResultIdx + 1 << " / " << (int)m_searchResultCoords.Size();
		m_searchLabel->SetLabelText( labelText );
	}
	else
	{
		m_searchLabel->SetLabelText( "0 / 0" );
	}
}

void CEd2dArrayEditor::OnSearchNextResult(wxCommandEvent& event)
{
	if( m_currentSearchResultIdx < m_searchResultCoords.Size() - 1 )
	{
		++m_currentSearchResultIdx;
	}

	SkipToSelectedRow();
}

void CEd2dArrayEditor::OnSearchPrevResult(wxCommandEvent& event)
{
	if( m_currentSearchResultIdx > 0 )
	{
		--m_currentSearchResultIdx;
	}

	SkipToSelectedRow();
}

void CEd2dArrayEditor::OnSearchLineEnterDown(wxCommandEvent& event)
{
	OnSearchButtonClicked(event);
}

void CEd2dArrayEditor::OnFitCells(wxCommandEvent& event)
{
	m_grid->Fit();
	Layout();
}
