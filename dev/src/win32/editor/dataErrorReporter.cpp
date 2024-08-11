/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DATA_VALIDATION

#include <wx/wupdlock.h>
#include "dataErrorReporter.h"
#include "resourceFinder.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

RED_DEFINE_NAME( ShowDataErrorReporter );

namespace
{
	Int32 GRowHeight = 25;
}

CEdDataErrorReporterWindow::CEdDataErrorReporterWindow( wxWindow* parent )
	: m_criticalFilter( nullptr )
	, m_errorFilter( nullptr )
	, m_warningFilter( nullptr )
	, m_bugsFilter( nullptr )
	, m_searchButton( nullptr )
	, m_additionalReporting( nullptr )
	, m_searchLine( nullptr )
	, m_categoryFilterList( nullptr )
	, m_bugsPanel( nullptr )
	, m_refresh( nullptr )
	, m_checkErrors( nullptr )
	, m_notClearBugs( true )
	, m_flushAllToWindow( true )
	, m_selectedCategory( nullptr )
	, m_selectedRowInCategory( -1 )	
	, m_displayedErrors( 0 )
	, m_displayedErrorsLimit( 200 )
{
	m_priorities[ DES_Uber ] = true;
	m_priorities[ DES_Major ] = true;
	m_priorities[ DES_Minor ] = true;
	m_priorities[ DES_Tiny ] = true;

	// Load layout from XRC file
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("NewDataErrorReporter") );

	// Load control and connect event with handlers
	m_criticalFilter = XRCCTRL( *this, "m_criticalFilter", wxCheckBox );
	m_errorFilter = XRCCTRL( *this, "m_errorFilter", wxCheckBox );
	m_warningFilter = XRCCTRL( *this, "m_warningFilter", wxCheckBox );
	m_bugsFilter = XRCCTRL( *this, "m_bugsFilter", wxCheckBox );
	m_continuousReporting = XRCCTRL( *this, "m_continuousReporting", wxCheckBox );
	m_additionalReporting = XRCCTRL( *this, "m_additionalReporting", wxCheckBox );
	m_clearContent = XRCCTRL(*this, "m_clearContent", wxButton );
	m_refresh = XRCCTRL( *this, "m_refresh", wxButton );
	m_export = XRCCTRL( *this, "m_export", wxButton );
	m_checkErrors = XRCCTRL( *this, "m_checkErrors", wxButton );
	m_exportAll = XRCCTRL( *this, "m_exportAll", wxButton );

	m_categoryFilterList = XRCCTRL( *this, "m_categoriesFilterList", wxCheckListBox );
	m_splitterWindow = XRCCTRL( *this, "m_splitterWindow", wxSplitterWindow );
	m_bugsPanel = XRCCTRL( *this, "m_panelWithBugs", wxScrolledWindow );

	m_searchLine = XRCCTRL( *this, "m_searchLine", wxTextCtrl );
	m_searchButton = XRCCTRL( *this, "m_searchButton", wxButton );	
	m_leftResult = XRCCTRL( *this, "m_leftResult", wxButton );
	m_rightResult = XRCCTRL( *this, "m_rightResult", wxButton );
	m_searchResultsLabel = XRCCTRL( *this, "m_searchingResultLabel", wxStaticText );
	m_summaryLabel = XRCCTRL( *this, "m_summaryLabel", wxStaticText );

	// connect with callback functions
	m_criticalFilter->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdDataErrorReporterWindow::OnSelectPriorityFilter, this);
	m_errorFilter->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdDataErrorReporterWindow::OnSelectPriorityFilter, this );
	m_warningFilter->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdDataErrorReporterWindow::OnSelectPriorityFilter, this );
	m_bugsFilter->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdDataErrorReporterWindow::OnSelectPriorityFilter, this );
	m_criticalFilter->SetClientData( &m_priorities[ DES_Uber ] );
	m_errorFilter->SetClientData( &m_priorities[ DES_Major ] );
	m_warningFilter->SetClientData( &m_priorities[ DES_Minor ] );
	m_bugsFilter->SetClientData( &m_priorities[ DES_Tiny ] );

	m_continuousReporting->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdDataErrorReporterWindow::OnContinuousReportingClicked, this );
	m_additionalReporting->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdDataErrorReporterWindow::OnAdditionalReportingButtonClicked, this );
	m_refresh->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnRefresh, this );
	m_export->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnExportToCSVButtonClicked, this );
	m_checkErrors->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnCheckErrorsButtonClicked, this );
	m_exportAll->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnExportAllButtonClicked, this );
	m_clearContent->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnClearContent, this );

	m_categoryFilterList->Bind(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &CEdDataErrorReporterWindow::OnSelectCategoryFilter, this );

	m_searchButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnSearchButtonClicked, this );	
	m_leftResult->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnSelectPreviousResult, this );
	m_rightResult->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CEdDataErrorReporterWindow::OnSelectNextResult, this );
	m_searchLine->Bind(wxEVT_COMMAND_TEXT_ENTER, &CEdDataErrorReporterWindow::OnSearchEnterClicked, this );

	// create context menu for column
	m_columnContextMenu = new wxMenu();
	m_columnContextMenu->Append( DEC_AssetName, TXT("Asset name"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( DEC_Priority, TXT("Priority"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( DEC_LastEditBy, TXT("Last edit by"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( DEC_Hit, TXT("Hit"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( DEC_Description, TXT("Description"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Append( DEC_Path, TXT("Path"), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( true );
	m_columnContextMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, &CEdDataErrorReporterWindow::OnColumnPopupClick, this );

	// create context menu for row
	m_rowContextMenu = new wxMenu();
	m_rowContextMenu->Append( 0, TXT("Show in asset browser"), nullptr );
	m_rowContextMenu->Append( 1, TXT("Show in explorer"), nullptr );
	m_rowContextMenu->Append( 2, TXT("Show all resource instances"), nullptr );
	m_rowContextMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, &CEdDataErrorReporterWindow::OnRowPopupClick, this );

	// Connect with editor events
	SEvents::GetInstance().RegisterListener( RED_NAME( ShowDataErrorReporter ), this );

	// Connect to Data error reporter
	if( GDataError != nullptr )
	{
		GDataError->RegisterListener( this );
	}

	m_additionalReporting->SetValue( true );
}

CEdDataErrorReporterWindow::~CEdDataErrorReporterWindow()
{
	// Disconnect to Data error reporter
	if( GDataError != nullptr )
	{
		GDataError->UnregisterListener( this );
	}

	// Disconnect with editor events
	SEvents::GetInstance().UnregisterListener( RED_NAME( ShowDataErrorReporter ), this );
}

void CEdDataErrorReporterWindow::ShowWindow()
{
	if ( !IsShown() )
	{
		wxCommandEvent fakeEvt;
		OnCheckErrorsButtonClicked( fakeEvt );
	}

	wxDialog::SetFocus();
	wxDialog::Raise();
	wxDialog::Show();
}

void CEdDataErrorReporterWindow::RequestClearContent()
{
	wxWindowUpdateLocker localUpdateLocker( this );

	ClearSearchResult();

	if( m_notClearBugs == false )
	{
		OnClearContent( wxCommandEvent() );
	}
}

void CEdDataErrorReporterWindow::ProcessDataErrors( const TDynArray< SDataError >& errors )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	ClearSearchResult();

	if ( m_flushAllToWindow == true )
	{
		for ( Uint32 i = 0; i < errors.Size(); ++i )
		{
			if ( m_displayedErrors >= m_displayedErrorsLimit )
			{
				m_dataErrorsNotDisplayed.PushBackUnique( errors.SubArray( i ) );
				break;
			}
			AddDataErrorToWindow( errors[i] );
		}
	}
	else
	{
		m_dataErrorsNotDisplayed.PushBack( errors );
	}
	UpdateSummaryLabel();
}

void CEdDataErrorReporterWindow::StoreNonProcessedErrors( const TDynArray< SDataError >& errors )
{
	for ( Uint32 i = 0; i < errors.Size(); ++i )
	{
		SDataError* error = m_dataErrorsNotProcessed.FindPtr( errors[i].m_uid );
		if ( error != nullptr )
		{
			error->m_errorCount += errors[i].m_errorCount;
		}
		else
		{
			SDataError& newError = m_dataErrorsNotProcessed.GetRef( errors[i].m_uid );
			newError.m_category = errors[i].m_category;
			newError.m_errorCount = errors[i].m_errorCount;
			newError.m_errorMessage = errors[i].m_errorMessage;
			newError.m_lastEditBy = errors[i].m_lastEditBy;
			newError.m_resourcePath = errors[i].m_resourcePath;
			newError.m_severity = errors[i].m_severity;
			newError.m_uid = errors[i].m_uid;
		}
	}
	UpdateSummaryLabel();
}

void CEdDataErrorReporterWindow::UpdateContent()
{
	if( wxDialog::IsShown() == true )
	{
		InternalRefresh();
	}
}


void CEdDataErrorReporterWindow::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == RED_NAME( ShowDataErrorReporter ) )
	{
		ShowWindow();
	}
}

wxGrid* CEdDataErrorReporterWindow::ProcessCategory(const String& categoryName )
{
	wxGrid** categoryGridPtr = m_bugsList.FindPtr( categoryName );
	if( categoryGridPtr != nullptr )
	{
		return *categoryGridPtr;
	}

	// add new category filter
	m_categoryFilterList->Append( categoryName.AsChar() );
	m_categoryFilterList->Check( m_categoryFilterList->GetCount() - 1, true );

	// add new sizer with tree view for new category
	wxPanel* newPanel = new wxPanel( m_bugsPanel, wxID_ANY );
	m_bugsPanel->GetSizer()->Add( newPanel, 0, wxALL | wxEXPAND );
	m_categoriesPanels.Insert( categoryName, newPanel );

	wxBoxSizer* newCategorySizer = new wxBoxSizer( wxVERTICAL );
	newPanel->SetSizer ( newCategorySizer );

	wxStaticText* captionText = new wxStaticText( newPanel, wxID_ANY, categoryName.AsChar() );
	wxFont temporaryFont = captionText->GetFont();
	temporaryFont.SetWeight(wxFONTWEIGHT_BOLD);
	captionText->SetFont(temporaryFont);
	newCategorySizer->Add( captionText, 0, wxALL | wxEXPAND );

	wxGrid* newGrid = new wxGrid( newPanel, wxID_ANY );
	newCategorySizer->Add( newGrid, 0, wxALL | wxEXPAND );
	m_bugsList.Insert( categoryName, newGrid );

	// set grid properties
	newGrid->CreateGrid( 0, DEC_Count, wxGrid::wxGridSelectRows);
	newGrid->SetRowLabelSize( 1 );

	newGrid->SetColLabelValue( DEC_UID, TXT("UID") );
	newGrid->SetColLabelValue( DEC_AssetName, TXT("Asset name") );
	newGrid->SetColLabelValue( DEC_Priority, TXT("Priority") );
	newGrid->SetColLabelValue( DEC_LastEditBy, TXT("Last edited by") );
	newGrid->SetColLabelValue( DEC_Hit, TXT("Hit") );
	newGrid->SetColLabelValue( DEC_Description, TXT("Description") );
	newGrid->SetColLabelValue( DEC_Path, TXT("Path") );
	newGrid->SetColLabelSize( 20 );

	// set default width
	newGrid->SetColSize( DEC_UID, 0 );
	newGrid->SetColSize( DEC_AssetName, 200 );
	newGrid->SetColSize( DEC_Priority, 50 );
	newGrid->SetColSize( DEC_LastEditBy, 150 );
	newGrid->SetColSize( DEC_Hit, 30 );
	newGrid->SetColSize( DEC_Description, 500 );
	newGrid->SetColSize( DEC_Path, 500 );

	newGrid->Bind(wxEVT_GRID_SELECT_CELL, &CEdDataErrorReporterWindow::OnSelectRowInGrid, this );
	newGrid->Bind(wxEVT_GRID_COL_SIZE, &CEdDataErrorReporterWindow::OnColumnSizeChanged, this );
	newGrid->Bind(wxEVT_GRID_LABEL_RIGHT_CLICK, &CEdDataErrorReporterWindow::OnShowColumnContextMenu, this );
	newGrid->Bind(wxEVT_GRID_CELL_RIGHT_CLICK, &CEdDataErrorReporterWindow::OnShowRowContextMenu, this );
	newGrid->Bind(wxEVT_GRID_COL_SORT, &CEdDataErrorReporterWindow::OnSortByColumn, this );

	// return new category grid
	return newGrid;
}

void CEdDataErrorReporterWindow::ProcessBugs( const SDataError& dataError, wxGrid* categoryGrid )
{
	Int32 existErrorIndex = SearchExistingError( dataError.m_uid, categoryGrid );

	if( existErrorIndex != -1 )
	{
		categoryGrid->SetCellValue( existErrorIndex, DEC_Hit, ToString( dataError.m_errorCount ).AsChar() );
	}
	else
	{
		++m_displayedErrors;
		// add new row to grid
		categoryGrid->AppendRows();

		// prepare information for row
		Uint32 rowIndex = categoryGrid->GetNumberRows() - 1;
		CFilePath filePath( dataError.m_resourcePath );

		// fill row
		categoryGrid->SetDefaultRowSize( GRowHeight, true );
		categoryGrid->DisableRowResize( rowIndex );
		categoryGrid->SetCellValue( rowIndex, DEC_UID, ToString( dataError.m_uid ).AsChar() );
		categoryGrid->SetCellValue( rowIndex, DEC_AssetName, filePath.GetFileName().AsChar() );
		categoryGrid->SetCellValue( rowIndex, DEC_Priority, ToString( ( (Uint32)dataError.m_severity ) + 1 ).AsChar() );
		categoryGrid->SetCellValue( rowIndex, DEC_LastEditBy, dataError.m_lastEditBy.AsChar() );
		categoryGrid->SetCellValue( rowIndex, DEC_Hit, ToString( dataError.m_errorCount ).AsChar() );
		categoryGrid->SetCellValue( rowIndex, DEC_Description, dataError.m_errorMessage.AsChar() );
		categoryGrid->SetCellValue( rowIndex, DEC_Path, dataError.m_resourcePath.AsChar() );
	}

	RefreshWindow();
}

void CEdDataErrorReporterWindow::UpdateSummaryLabel()
{
	m_summaryLabel->SetLabel( String::Printf( TXT( "Displaying %d from %d errors" ), m_displayedErrors, m_displayedErrors + m_dataErrorsNotDisplayed.Size() + m_dataErrorsNotProcessed.Size() ).AsChar() );
}

void CEdDataErrorReporterWindow::OnSelectCategoryFilter( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	String categoryName = m_categoryFilterList->GetString( event.GetInt() );
	wxPanel* panel = *m_categoriesPanels.FindPtr( categoryName );

	if( m_categoryFilterList->IsChecked( event.GetInt() ) == true )
	{
		panel->Show();
	}
	else
	{
		panel->Hide();
	}

	RefreshWindow();
}

void CEdDataErrorReporterWindow::OnSelectPriorityFilter( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	// set visible
	Bool visible = ( event.GetInt() == 1 );

	// set priority
	wxCheckBox* sender = wxDynamicCast( event.GetEventObject(), wxCheckBox );
	Bool* priority = static_cast< Bool* >( sender->GetClientData() );
	(*priority) = visible;

	// set visible
	for(THashMap< String, wxGrid* >::iterator it = m_bugsList.Begin(); it != m_bugsList.End(); ++it)
	{
		wxGrid* grid = it->m_second;
		Uint32 rowCount = grid->GetNumberRows();

		for(Uint32 j=0; j<rowCount; ++j)
		{
			String cellPriorityText = grid->GetCellValue( j, DEC_Priority );
			Uint32 cellPriority;
			FromString< Uint32 >( cellPriorityText, cellPriority );

			if( m_priorities[ cellPriority - 1] == true )
			{
				grid->ShowRow( j );
				grid->AutoSizeRow( j );
			}
			else
			{
				grid->HideRow( j );
			}
		}
	}
	RefreshWindow();
}

void CEdDataErrorReporterWindow::OnSearchButtonClicked( wxCommandEvent& event )
{
	String phrase = m_searchLine->GetValue();

	// clear old results
	ClearSearchResult();

	// search
	for(THashMap< String, wxGrid* >::iterator it = m_bugsList.Begin(); it != m_bugsList.End(); ++it)
	{
		wxGrid* grid = it->m_second;
		Uint32 rowCount = grid->GetNumberRows();

		for(Uint32 rowIndex=0; rowIndex<rowCount; ++rowIndex)
		{
			for(Uint32 k=0; k<DEC_Count; ++k)
			{
				String tempShit = grid->GetCellValue( rowIndex, k ).c_str().AsWChar();
				if( tempShit.ContainsSubstring( phrase ) == true )
				{
					m_searchingResults.PushBack( TPair< wxGrid*, Uint32 >( grid, rowIndex ) );
					break;
				}
			}
		}
	}

	// if results are existed, enable button
	if( m_searchingResults.Size() > 0 )
	{
		m_leftResult->Enable( true );
		m_rightResult->Enable( true );

		OnSelectNextResult( wxCommandEvent() );
	}

	UpdateSearchLabel();
}

void CEdDataErrorReporterWindow::OnExportToCSVButtonClicked( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	String localDepotPath;
	if ( GetLocalPathToSaveFile( localDepotPath ) )
	{
		THashMap< String, TDynArray< Uint32 > > categoriesErrors;

		// export category only when it is selected
		for( Uint32 i=0; i<m_categoryFilterList->GetCount(); ++i)
		{
			if( m_categoryFilterList->IsChecked( i ) == true )
			{
				String categoryName = m_categoryFilterList->GetString( i );
				CollectErrorsUidsFromGrid( categoriesErrors, categoryName, true );
			}
		}

		// Save bugs information
		CResource::FactoryInfo< C2dArray > info;
		C2dArray* bugsInfo = info.CreateResource();
		FillBugsInfoHeaders( bugsInfo );
		FillBugsInfo( bugsInfo, categoriesErrors, false );

		SaveExportedErrors( bugsInfo, localDepotPath );
	}
}

Bool CEdDataErrorReporterWindow::GetLocalPathToSaveFile( String& localDepotPath )
{
	// select path to save file
	CEdFileDialog m_saveDialogWindow;
	m_saveDialogWindow.SetMultiselection( false );
	m_saveDialogWindow.AddFormat( TXT("csv"), TXT("CSV file") );

	// set default name
	time_t now = time(0);
	tm* localTime = localtime(&now);
	String dateTime = ToString(localTime->tm_year+1900)+TXT("-")+ToString(localTime->tm_mon+1)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
	dateTime += ToString(localTime->tm_hour)+TXT("-")+ToString(localTime->tm_min)+TXT("-")+ToString(localTime->tm_sec);
	
	if ( m_saveDialogWindow.DoSave( (HWND)GetHandle(), String::Printf( TXT("ErrorsList[%s]"), dateTime.AsChar() ).AsChar(),  true ) == true )
	{
		if ( GDepot->ConvertToLocalPath( m_saveDialogWindow.GetFile(), localDepotPath ) == false )
		{
			GFeedback->ShowWarn( TXT("File cannot be saved in %s. Please choose other location in %s."), m_saveDialogWindow.GetFile().AsChar(), GDepot->GetRootDataPath().AsChar() );
			return false;
		}
		return true;
	}
	return false;
}

void CEdDataErrorReporterWindow::SaveExportedErrors( C2dArray* bugsInfo, const String& localDepotPath )
{
	CFilePath filePath( localDepotPath );
	CDirectory* directory = GDepot->CreateNewDirectory( localDepotPath.AsChar() );

	if ( CDiskFile *diskFile = GDepot->FindFile( localDepotPath ) )
	{
		String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), localDepotPath.AsChar() );
		if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
		{
			return;
		}
	}

	// save to file
	bugsInfo->SaveAs( directory, filePath.GetFileName(), true );
}

void CEdDataErrorReporterWindow::FillBugsInfoHeaders( C2dArray* bugsInfo )
{
	bugsInfo->AddColumn( TXT( "" ) );
	bugsInfo->AddColumn( TXT( "Asset name" ) );
	bugsInfo->AddColumn( TXT( "Priority" ) );
	bugsInfo->AddColumn( TXT( "Last edited by" ) );
	bugsInfo->AddColumn( TXT( "Hit" ) );
	bugsInfo->AddColumn( TXT( "Description" ) );
	bugsInfo->AddColumn( TXT( "Path" ) );

	// add headers
	bugsInfo->AddRow();
	bugsInfo->SetValue( TXT( "Asset name" )		, DEC_AssetName, 0 );
	bugsInfo->SetValue( TXT( "Priority" )		, DEC_Priority, 0 );
	bugsInfo->SetValue( TXT( "Last edited by" )	, DEC_LastEditBy, 0 );
	bugsInfo->SetValue( TXT( "Hit" )			, DEC_Hit, 0 );
	bugsInfo->SetValue( TXT( "Description" )	, DEC_Description, 0 );
	bugsInfo->SetValue( TXT( "Path" )			, DEC_Path, 0 );
}

void CEdDataErrorReporterWindow::FillBugsInfo( C2dArray* bugsInfo, const THashMap< String, TDynArray< Uint32 > >& categoriesErrorsUids, Bool includeNonProcessedErrors )
{
	Uint32 ind = 0;
	for ( THashMap< String, TDynArray< Uint32 > >::const_iterator it = categoriesErrorsUids.Begin(); it != categoriesErrorsUids.End(); ++it, ++ind )
	{
		GFeedback->UpdateTaskProgress( ind, categoriesErrorsUids.Size() );

		String categoryName = it->m_first;
		InsertNewCategoryToBugsInfo( bugsInfo, categoryName );

		wxGrid* grid;
		if ( m_bugsList.KeyExist( categoryName ) )
		{
			grid = *m_bugsList.FindPtr( categoryName );
		}

		const TDynArray< Uint32 >& uids = it->m_second;
		for ( Uint32 i = 0; i < uids.Size(); ++i )
		{
			bugsInfo->AddRow();

			Int32 gridInd = -1;
			if ( grid && ( gridInd = SearchExistingError( uids[i], grid ) ) >= 0 )
			{
				FillBugsInfoRow( bugsInfo, grid, gridInd );
			}

			if ( includeNonProcessedErrors )
			{
				SDataError* error = m_dataErrorsNotProcessed.FindPtr( uids[i] );
				if ( error != nullptr )
				{
					if ( gridInd >= 0 )
					{
						Uint32 newHitCount = wxAtoi( grid->GetCellValue( gridInd, DEC_Hit ) ) + error->m_errorCount;
						Uint32 activeRowIndex = bugsInfo->GetNumberOfRows() - 1;
						bugsInfo->SetValue( ToString( newHitCount ).AsChar(), DEC_Hit, activeRowIndex );
					}
					else
					{
						FillBugsInfoRow( bugsInfo, *error );
					}
				}
			}
		}
	}
}

void CEdDataErrorReporterWindow::InsertNewCategoryToBugsInfo( C2dArray* bugsInfo, const String& categoryName )
{
	// add empty line between categories
	bugsInfo->AddRow();
	bugsInfo->AddRow();

	// add information about category
	Uint32 activeRowIndex = bugsInfo->GetNumberOfRows() - 1;
	bugsInfo->SetValue( categoryName, 0, activeRowIndex );
}

void CEdDataErrorReporterWindow::FillBugsInfoRow( C2dArray* bugsInfo, const SDataError& error )
{
	Uint32 activeRowIndex = bugsInfo->GetNumberOfRows() - 1;

	CFilePath filePath( error.m_resourcePath );
	bugsInfo->SetValue( filePath.GetFileName().AsChar(), DEC_AssetName, activeRowIndex );
	bugsInfo->SetValue( ToString( error.m_severity ), DEC_Priority, activeRowIndex );
	bugsInfo->SetValue( error.m_lastEditBy, DEC_LastEditBy, activeRowIndex );
	bugsInfo->SetValue( ToString( error.m_errorCount ), DEC_Hit, activeRowIndex );
	bugsInfo->SetValue( error.m_errorMessage, DEC_Description, activeRowIndex );
	bugsInfo->SetValue( error.m_resourcePath, DEC_Path, activeRowIndex );
}

void CEdDataErrorReporterWindow::FillBugsInfoRow( C2dArray* bugsInfo, const wxGrid* grid, const Uint32 gridRow )
{
	Uint32 activeRowIndex = bugsInfo->GetNumberOfRows() - 1;

	// start from DEC_AssetName because DEC_UID = 0 is hidden column for error hash number
	for ( Uint32 k = DEC_AssetName; k < DEC_Count; ++k)
	{
		bugsInfo->SetValue( grid->GetCellValue( gridRow, k ).c_str().AsWChar(), k, activeRowIndex );
	}
}

void CEdDataErrorReporterWindow::CollectErrorsUidsFromGrid( THashMap< String, TDynArray< Uint32 > >& errorsUids, const String& categoryName, Bool checkPriority )
{
	TDynArray< Uint32 >& uids = errorsUids.GetRef( categoryName );

	wxGrid** grid = m_bugsList.FindPtr( categoryName );
	Uint32 rowCount = (*grid)->GetNumberRows();
	for( Uint32 j = 0; j < rowCount; ++j)
	{
		// export all rows or only the ones with priority selected on the filter list
		Uint32 rowPriority;
		if ( !checkPriority || ( FromString< Uint32 >( (*grid)->GetCellValue( j, DEC_Priority ).c_str().AsWChar(), rowPriority ) && m_priorities[ rowPriority - 1 ] ) )
		{
			String uid = (*grid)->GetCellValue( j, DEC_UID );
			Uint32 id;
			FromString< Uint32 >( uid, id );
			uids.PushBackUnique( id );
		}
	}
}

void CEdDataErrorReporterWindow::CollectNonProcessedErrorsUids( THashMap< String, TDynArray< Uint32 > >& errorsUids )
{
	Uint32 ind = 0;
	for ( THashMap< Uint32, SDataError >::iterator it = m_dataErrorsNotProcessed.Begin(); it != m_dataErrorsNotProcessed.End(); ++it, ++ind )
	{
		GFeedback->UpdateTaskProgress( ind, m_dataErrorsNotProcessed.Size() );

		SDataError& error = it->m_second;
		//get info from perforce
		if ( it->m_second.m_lastEditBy == String::EMPTY && GDataError != nullptr )
		{
			GDataError->FillErrorInfoFromPerforce( error );
		}

		TDynArray< Uint32 >& uids = errorsUids.GetRef( error.m_category );
		uids.PushBackUnique( error.m_uid );
	}
}

void CEdDataErrorReporterWindow::OnAdditionalReportingButtonClicked( wxCommandEvent& event )
{
	m_notClearBugs = ( event.GetInt() != 0 );
}

void CEdDataErrorReporterWindow::ShowInAssetBrowser()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	if( m_selectedCategory != nullptr && m_selectedRowInCategory != -1 )
	{
		String path = m_selectedCategory->GetCellValue( m_selectedRowInCategory, DEC_Path );
		SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( path ) );
	}
#endif
}

void CEdDataErrorReporterWindow::ShowInExplorer()
{
	if( m_selectedCategory != nullptr && m_selectedRowInCategory != -1 )
	{
		wxString cmd = wxT( "explorer /select," );
		String path = m_selectedCategory->GetCellValue( m_selectedRowInCategory, DEC_Path );
		cmd += ( GFileManager->GetDataDirectory() + path ).AsChar();

		::wxExecute( cmd, wxEXEC_ASYNC, nullptr );
	}
}

void CEdDataErrorReporterWindow::OnSelectRowInGrid( wxGridEvent& event )
{
	wxGrid* tempCategory = wxDynamicCast( event.GetEventObject(), wxGrid );
	SelectRow( tempCategory, event.GetRow() );
}

void CEdDataErrorReporterWindow::OnColumnSizeChanged( wxGridSizeEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	wxGrid* tempCategory = wxDynamicCast( event.GetEventObject(), wxGrid );
	Uint32 columnIndex = event.GetRowOrCol();
	Uint32 columnSize = tempCategory->GetColSize( columnIndex );

	for(THashMap< String, wxGrid* >::iterator it = m_bugsList.Begin(); it != m_bugsList.End(); ++it)
	{
		wxGrid* grid = it->m_second;
		grid->SetColSize( columnIndex, columnSize );
	}

	RefreshWindow();
}

void CEdDataErrorReporterWindow::OnRefresh( wxCommandEvent& event )
{
	InternalRefresh();
	FlushAllCoughtIssue();
}

void CEdDataErrorReporterWindow::OnCheckErrorsButtonClicked( wxCommandEvent& event )
{
	if ( GGame->GetActiveWorld() )
	{
		TDynArray< String >	resourcesPaths;
		if ( CollectResourcesPaths( resourcesPaths ) )
		{
			CheckDataErrorsForSpecifiedResources( resourcesPaths );		
		}
	}

	SetFocus();
	InternalRefresh();
}

void CEdDataErrorReporterWindow::OnExportAllButtonClicked( wxCommandEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	String localDepotPath;
	if ( GetLocalPathToSaveFile( localDepotPath ) )
	{
		THashMap< String, TDynArray< Uint32 > > categoriesErrorsUids;

		// export every category
		GFeedback->BeginTask( TXT( "Exporting errors..." ), false );
		Uint32 ind = 0;
		for ( THashMap< String, wxGrid* >::const_iterator it = m_bugsList.Begin(); it != m_bugsList.End(); ++it, ++ind )
		{
			GFeedback->UpdateTaskProgress( ind, m_bugsList.Size() );

			String categoryName = it->m_first;
			CollectErrorsUidsFromGrid( categoriesErrorsUids, categoryName, false );
		}

		CollectNonProcessedErrorsUids( categoriesErrorsUids );

		// Save bugs information
		CResource::FactoryInfo< C2dArray > info;
		C2dArray* bugsInfo = info.CreateResource();
		FillBugsInfoHeaders( bugsInfo );

		GFeedback->UpdateTaskInfo( TXT( "Saving errors..." ) );
		FillBugsInfo( bugsInfo, categoriesErrorsUids, true );

		SaveExportedErrors( bugsInfo, localDepotPath );
		GFeedback->EndTask();
	}
}

Bool CEdDataErrorReporterWindow::CollectResourcesPaths( TDynArray< String >& resourcesPaths )
{
	Bool pathsCollected = true;
	GFeedback->BeginTask( TXT( "Checking world's dependencies errors - collecting layers' dependencies" ), true );

	TDynArray< CResource::DependencyInfo > dependencies;
	GGame->GetActiveWorld()->GetDependentResourcesPaths( dependencies, TDynArray< String >() );
	for ( const CResource::DependencyInfo& info : dependencies )
	{
		resourcesPaths.PushBack( info.m_path );
	}
	dependencies.ClearFast();

	TDynArray< CLayerInfo* > layersInfo;
	GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( layersInfo, true, true );
	for ( Uint32 i = 0; i < layersInfo.Size(); ++i )
	{
		if ( GFeedback->IsTaskCanceled() )
		{
			pathsCollected = false;
			break;
		}

		String path;
		layersInfo[i]->GetHierarchyPath( path, true );
		GFeedback->UpdateTaskInfo( path.AsChar() );
		GFeedback->UpdateTaskProgress( i, layersInfo.Size() );

		layersInfo[i]->GetLayer()->GetDependentResourcesPaths( dependencies, resourcesPaths );
		for ( const CResource::DependencyInfo& info : dependencies )
		{
			resourcesPaths.PushBackUnique( info.m_path );
		}
	}

	GFeedback->EndTask();
	return pathsCollected;
}

void CEdDataErrorReporterWindow::CheckDataErrorsForSpecifiedResources( const TDynArray< String > resourcesPaths )
{
	for ( Uint32 i = 0; i < resourcesPaths.Size(); ++i )
	{
		CDiskFile* file = GDepot->FindFileUseLinks( resourcesPaths[i], 0 );
		if ( file )
		{
			CResource* res = file->GetResource();
			if ( res )
			{
				res->OnCheckDataErrors();
			}
		}
	}
}

void CEdDataErrorReporterWindow::OnSelectPreviousResult( wxCommandEvent& event )
{
	if( m_activeSelectedResult > 0 )
	{
		--m_activeSelectedResult;
	}
	SelectRow( m_searchingResults[ m_activeSelectedResult ].m_first, m_searchingResults[ m_activeSelectedResult ].m_second );
	UpdateSearchLabel();
	SkipToSelectedRow();
}

void CEdDataErrorReporterWindow::OnSelectNextResult( wxCommandEvent& event )
{
	if ( m_searchingResults.Empty() )
	{
		return;
	}

	if( m_activeSelectedResult < (Int32)m_searchingResults.Size() - 1 )
	{
		++m_activeSelectedResult;
	}

	wxGrid* temporaryGrid =  m_searchingResults[ m_activeSelectedResult ].m_first;
	Uint32 temporaryRowIndex = m_searchingResults[ m_activeSelectedResult ].m_second;

	SelectRow( temporaryGrid, temporaryRowIndex );
	UpdateSearchLabel();
	SkipToSelectedRow();
}

void CEdDataErrorReporterWindow::SelectRow( wxGrid* category, Uint32 rowIndex )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	if( m_selectedCategory != nullptr && m_selectedRowInCategory != -1 )
	{
		// unselect old row
		if( category != m_selectedCategory )
		{
			m_selectedCategory->SelectRow( -1 );
		}
	}

	m_selectedCategory = category;
	m_selectedRowInCategory = rowIndex;
	m_selectedCategory->SelectRow( rowIndex );
}

void CEdDataErrorReporterWindow::UpdateSearchLabel()
{
	wxWindowUpdateLocker localUpdateLocker( this );

	String labelText = String::EMPTY;

	if( m_searchingResults.Size() == 0 || m_activeSelectedResult == -1 )
	{
		labelText = TXT("0 / 0");
	}
	else
	{
		labelText = String::Printf( TXT("%d / %d"), m_activeSelectedResult + 1, m_searchingResults.Size() );
	}
	m_searchResultsLabel->SetLabel( labelText.AsChar() );
	m_searchResultsLabel->Refresh();
	RefreshWindow();
}

void CEdDataErrorReporterWindow::ClearSearchResult()
{
	m_activeSelectedResult = -1;
	m_searchingResults.Clear();
}

void CEdDataErrorReporterWindow::OnSortByColumn( wxGridEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	// helper typdefs
	typedef TDynArray< String > RowStrings;
	typedef TPair< String, RowStrings > RowsToSort;
	typedef TDynArray< RowsToSort > SortedRows;

	// data sorter
	struct LocalSorter
	{
		static Bool SortStringAscending( const TPair< String, RowStrings >& p1, const TPair< String, RowStrings >& p2 )
		{
			return Red::System::StringCompareNoCase( p1.m_first.AsChar(), p2.m_first.AsChar() ) < 0;
		}

		static Bool SortDigitAscending( const TPair< String, RowStrings >& p1, const TPair< String, RowStrings >& p2 )
		{
			Uint32 d1, d2;
			FromString< Uint32 >(p1.m_first, d1);
			FromString< Uint32 >(p2.m_first, d2);
			return d2 > d1;
		}
	};

	// get basic information
	wxGrid* tempCategory = wxDynamicCast( event.GetEventObject(), wxGrid );
	Uint32 columnIndex = event.GetCol();
	Bool ascendingOrder = tempCategory->IsSortOrderAscending();

	// sort all table by current column
	for(THashMap< String, wxGrid* >::iterator it = m_bugsList.Begin(); it != m_bugsList.End(); ++it)
	{
		wxGrid* grid = it->m_second;
		Uint32 rowCount = grid->GetNumberRows();

		// sorting
		SortedRows rows;
		for( Uint32 i=0; i<rowCount; ++i )
		{
			RowStrings values;

			for( Uint32 j=0; j<DEC_Count; ++j )
			{
				values.PushBack( grid->GetCellValue( i, j ).c_str().AsWChar() );
			}

			// create pair
			rows.PushBack( RowsToSort( grid->GetCellValue( i, columnIndex ).c_str().AsWChar(), values ) );
		}

		// sort
		if( columnIndex == DEC_Hit || columnIndex == DEC_Priority )
		{
			Sort( rows.Begin(), rows.End(), LocalSorter::SortDigitAscending );
		}
		else
		{
			Sort( rows.Begin(), rows.End(), LocalSorter::SortStringAscending );
		}
	
		// set correct values
		for( Uint32 i=0; i<rows.Size(); ++i)
		{
			const RowStrings& values = rows[i].m_second;

			for( Uint32 j=0; j<DEC_Count; ++j )
			{
				if( ascendingOrder == true )
				{
					grid->SetCellValue( i, j, values[j].AsChar() );
				}
				else
				{
					grid->SetCellValue( rows.Size() - ( i + 1 ), j, values[j].AsChar() );
				}
			}
		}

		grid->SetSortingColumn( columnIndex, ascendingOrder );
		grid->GetGridColLabelWindow()->Refresh();
	}
}

void CEdDataErrorReporterWindow::OnShowColumnContextMenu( wxGridEvent& event )
{
	wxWindowBase::PopupMenu( m_columnContextMenu );
}

void CEdDataErrorReporterWindow::OnColumnPopupClick( wxCommandEvent &event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	Uint32 maxColumnWidth = 0;

	for(THashMap< String, wxGrid* >::iterator it = m_bugsList.Begin(); it != m_bugsList.End(); ++it)
	{
		wxGrid* grid = it->m_second;

		if( m_columnContextMenu->IsChecked( event.GetId() ) == true )
		{
			grid->ShowCol( event.GetId() );
			grid->AutoSizeColumn( event.GetId(), true );
			maxColumnWidth = Max< Uint32 >( maxColumnWidth, grid->GetColSize( event.GetId() ) );
		}
		else
		{
			grid->HideCol( event.GetId() );
		}
	}

	// set the same width for edited columns
	for(THashMap< String, wxGrid* >::iterator it = m_bugsList.Begin(); it != m_bugsList.End(); ++it)
	{
		wxGrid* grid = it->m_second;
		grid->SetColSize( event.GetId(), maxColumnWidth );
	}
}

void CEdDataErrorReporterWindow::OnRowPopupClick( wxCommandEvent &event )
{
	Uint32 option = event.GetId();

	if( option == 0 )
	{
		ShowInAssetBrowser();
	}
	else if( option == 1 )
	{
		ShowInExplorer();
	}
	else if( option == 2 )
	{
		ShowAllResourceInstances();
	}
}

void CEdDataErrorReporterWindow::OnShowRowContextMenu( wxGridEvent& event )
{
	wxGrid* tempCategory = wxDynamicCast( event.GetEventObject(), wxGrid );
	SelectRow( tempCategory, event.GetRow() );

	wxWindowBase::PopupMenu( m_rowContextMenu );
}

void CEdDataErrorReporterWindow::InternalRefresh()
{
	if( GDataError != nullptr )
	{
		GDataError->Flush();
	}
}

void CEdDataErrorReporterWindow::OnSearchEnterClicked( wxCommandEvent& event )
{
	OnSearchButtonClicked( wxCommandEvent() );
}

void CEdDataErrorReporterWindow::OnContinuousReportingClicked( wxCommandEvent& event )
{
	m_flushAllToWindow = ( event.GetInt() != 0 );

	if( m_flushAllToWindow == true )
	{
		m_refresh->Disable();
		FlushAllCoughtIssue();
	}
	else
	{
		m_refresh->Enable();
	}
}

void CEdDataErrorReporterWindow::FlushAllCoughtIssue()
{
	Uint32 ind = 0;
	for( ; ind < m_dataErrorsNotDisplayed.Size(), m_displayedErrors < m_displayedErrorsLimit; ++ind )
	{
		AddDataErrorToWindow( m_dataErrorsNotDisplayed[ ind ] );
	}
	m_dataErrorsNotDisplayed = m_dataErrorsNotDisplayed.SubArray( ind );
	UpdateSummaryLabel();
}

void CEdDataErrorReporterWindow::AddDataErrorToWindow( const SDataError& dataError )
{
	wxGrid* categoryGrid = ProcessCategory( dataError.m_category );
	ProcessBugs( dataError, categoryGrid );
}

void CEdDataErrorReporterWindow::StorageDataError( const SDataError& dataError )
{
	m_dataErrorsNotDisplayed.PushBack( dataError );
}

void CEdDataErrorReporterWindow::OnClearContent( wxCommandEvent& event )
{
	for(THashMap< String, wxPanel* >::iterator it = m_categoriesPanels.Begin(); it != m_categoriesPanels.End(); ++it)
	{
		wxPanel* panel = it->m_second;
		m_bugsPanel->RemoveChild( panel );
		wxDELETE( panel );
	}
	m_bugsPanel->GetSizer()->Clear();
	m_categoryFilterList->Clear();

	m_bugsList.Clear();
	m_categoriesPanels.Clear();

	m_selectedCategory = nullptr;
	m_selectedRowInCategory = -1;

	m_dataErrorsNotProcessed.Clear();
	m_displayedErrors = 0;

	UpdateSummaryLabel();
}

void CEdDataErrorReporterWindow::RefreshWindow()
{
	LayoutRecursively( this, false );
}

void CEdDataErrorReporterWindow::SkipToSelectedRow()
{
	int stepx, stepy;
	m_bugsPanel->GetScrollPixelsPerUnit(&stepx, &stepy);

	if ( stepy > 0 )
	{
		int vx, vy;
		m_bugsPanel->GetViewStart( &vx, &vy );

		int rowPosition = ( m_selectedCategory->GetParent()->GetPosition().y + m_selectedRowInCategory * (int)GRowHeight ) / stepy + vy;

		m_bugsPanel->Scroll( -1, rowPosition );
	}
}

void CEdDataErrorReporterWindow::ShowAllResourceInstances()
{
	if( m_selectedCategory != nullptr && m_selectedRowInCategory != -1 )
	{
		String depotPath = m_selectedCategory->GetCellValue( m_selectedRowInCategory, DEC_Path );

		if ( CResource* resource = GDepot->LoadResource( depotPath ) )
		{
			CEdResourceFinder::ShowForResource( resource );
		}
	}
}

void CEdDataErrorReporterWindow::OnDataErrorReported( const SDataError& error )
{
	/* intentionally empty */
}

void CEdDataErrorReporterWindow::StartProcessing()
{
	RequestClearContent();
}

void CEdDataErrorReporterWindow::StopProcessing()
{
	/* intentionally empty */
}

Int32 CEdDataErrorReporterWindow::SearchExistingError( Uint32 uid, wxGrid* grid  ) const
{
	Uint32 rowCount = grid->GetNumberRows();

	for( Uint32 i=0; i<rowCount; ++i )
	{
		// export row only when it's priority is selected on the filter list
		Uint32 errorUID;
		FromString< Uint32 >( grid->GetCellValue( i, DEC_UID ).c_str().AsWChar(), errorUID );

		if( errorUID == uid )
		{
			return i;
		}
	}
	return -1;
}

#endif	// NO_DATA_VALIDATION
	
