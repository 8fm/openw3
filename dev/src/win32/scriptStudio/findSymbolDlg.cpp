/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "Scintilla.h"
#include "findSymbolDlg.h"
#include "frame.h"
#include "fileStubs.h"

#include "solution/slnContainer.h"
#include "solution/file.h"

BEGIN_EVENT_TABLE( CSSFindSymbolDialog, wxDialog )
	EVT_BUTTON( XRCID("btnOK"), CSSFindSymbolDialog::OnOK )
	EVT_BUTTON( XRCID("btnCancel"), CSSFindSymbolDialog::OnCancel )
	EVT_CHAR_HOOK( CSSFindSymbolDialog::OnKeyDown )
	EVT_CLOSE( CSSFindSymbolDialog::OnClose )
	EVT_LIST_ITEM_SELECTED( XRCID("listCtrlSymbols"), CSSFindSymbolDialog::OnItemSelected )
	EVT_LIST_ITEM_ACTIVATED( XRCID("listCtrlSymbols"), CSSFindSymbolDialog::OnItemActivated )
	EVT_TEXT( XRCID("txtFilter"), CSSFindSymbolDialog::OnFilterTextUpdate )
	EVT_TIMER( 3041984, CSSFindSymbolDialog::OnFindTimer )
END_EVENT_TABLE()

CSSFindSymbolDialog::CSSFindSymbolDialog( wxWindow* parent, Solution* solution )
	: wxDialog()
	, m_frame( wxTheFrame )
	, m_selectedItemIndex( -1 )
	, m_selectedDoc( NULL )
	, m_forceReparse( true )
	, m_lastFilterText( wxEmptyString )
	, m_findTimer( this, 3041984 )
	, m_wasEntriesDataUpdated( false )
	, m_solution( solution )
	, m_numClasses( 0 )
	, m_numFunctions( 0 )
	, m_numEnums( 0 )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("FindSymbolDialog") );

	m_symbolEntriesFiltered = &m_symbolEntriesFilteredBuffer0;
	m_symbolEntriesFilteredBuffer = &m_symbolEntriesFilteredBuffer1;

	m_listCtrlSymbols	= XRCCTRL( *this, "listCtrlSymbols", wxListCtrl );
	m_txtFilter			= XRCCTRL( *this, "txtFilter", wxTextCtrl );
	m_btnOK				= XRCCTRL( *this, "btnOK", wxButton );
	m_btnCancel			= XRCCTRL( *this, "btnCancel", wxButton );

	// Create parsing thread
	m_parsingThread = new CSSFindSymbolParsingThread( this );
	m_parsingThread->Create();
	m_parsingThread->Run();

	SetSize( 1000, 500 );
	Layout();
	Center();
}

CSSFindSymbolDialog::~CSSFindSymbolDialog()
{
	if ( m_parsingThread )
	{
		m_parsingThread->RequestExit();
		// delete m_parsingThread;
	}
}

void CSSFindSymbolDialog::Init()
{
	m_selectedItemIndex = -1;
	GetAllSymbolEntries();
	FilterTextUpdate( m_wasEntriesDataUpdated );

	// Set dialog title bar
	wxString dialogTitle;
	unsigned int filesToParseCount = GStubSystem.GetFilesCountToProcess();
	if ( filesToParseCount > 0 )
	{
		dialogTitle.Printf( wxT("Find Symbol (%d files not parsed yet)"), filesToParseCount );
	}
	else
	{
		dialogTitle.Printf( wxT("Find Symbol (%d symbols loaded)"), m_symbolEntriesAll.size() );
	}
	SetTitle( dialogTitle );

	m_selectedDoc = NULL;

	m_txtFilter->SetFocus();
}

void CSSFindSymbolDialog::SelectCurrentInput()
{
	m_txtFilter->SelectAll();
}

void CSSFindSymbolDialog::UpdateSymbolsListCtrl()
{
	m_listCtrlSymbols->Freeze();

	m_listCtrlSymbols->ClearAll();

	m_listCtrlSymbols->InsertColumn( 0, wxT("Name") );
	m_listCtrlSymbols->InsertColumn( 1, wxT("Type") );
	m_listCtrlSymbols->InsertColumn( 2, wxT("Signature") );
	m_listCtrlSymbols->InsertColumn( 3, wxT("File") );

	for ( unsigned int i = 0; i < m_symbolEntriesFiltered->size(); ++i )
	{
		SSymbolEntry& entry = (*m_symbolEntriesFiltered)[ i ];
		m_listCtrlSymbols->InsertItem( i, entry.m_symbolName );
		m_listCtrlSymbols->SetItem( i, 1, entry.m_symbolTypeName );
		m_listCtrlSymbols->SetItem( i, 2, entry.m_signatureName );
		m_listCtrlSymbols->SetItem( i, 3, entry.m_fileName );
	}

	m_listCtrlSymbols->SetColumnWidth( 0, wxLIST_AUTOSIZE );
	m_listCtrlSymbols->SetColumnWidth( 1, wxLIST_AUTOSIZE );
	m_listCtrlSymbols->SetColumnWidth( 2, wxLIST_AUTOSIZE );
	m_listCtrlSymbols->SetColumnWidth( 3, wxLIST_AUTOSIZE );

	m_listCtrlSymbols->Thaw();
}

void CSSFindSymbolDialog::OnKeyDown( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_ESCAPE )
	{
		Hide();
	}
	else if (	event.GetKeyCode() == WXK_DOWN && 
				FindFocus() == m_txtFilter && 
				m_listCtrlSymbols->GetItemCount() > 0 )
	{
		m_listCtrlSymbols->SetFocus();
	}
	else
	{
		event.Skip();
	}
}

void CSSFindSymbolDialog::OnClose( wxCloseEvent& event )
{
	event.Veto();
	Hide();
}

void CSSFindSymbolDialog::OnOK( wxCommandEvent& event )
{
	if ( GotoSymbol() )
	{
		Hide();
	}
}

void CSSFindSymbolDialog::OnCancel( wxCommandEvent& event )
{
	Hide();
}

void CSSFindSymbolDialog::OnItemSelected( wxListEvent& event )
{
	m_selectedItemIndex = static_cast< int >( event.m_itemIndex );
	event.Skip();
}

void CSSFindSymbolDialog::OnItemActivated( wxListEvent& event )
{
	m_selectedItemIndex = static_cast< int >( event.m_itemIndex );

	if ( GotoSymbol() )
	{
		Hide();
	}
}

void CSSFindSymbolDialog::OnFilterTextUpdate( wxCommandEvent& event )
{
	const unsigned int EntriesNumThresholdForInstantRefresh = 500;

	// Check if we can instant refresh or we should schedule refresh
	// Instant refresh => the amout of items to parse is low 
	// (i.e. the filter hasn't changed significantly so we can use current
	// filtered items as a base and that base isn't too big)
	wxString filterText = m_txtFilter->GetValue();
	filterText.MakeLower();
	if ( filterText.StartsWith( m_lastFilterText ) &&
		 m_symbolEntriesFiltered->size() < EntriesNumThresholdForInstantRefresh )
	{
		// Instant refresh
		m_findTimer.Stop();
		FilterTextUpdate( false );
	}
	else
	{
		// Schedule filter text update
		m_findTimer.Stop();
		m_findTimer.Start( 400, true );
	}
}

void CSSFindSymbolDialog::OnFindTimer( wxTimerEvent& event )
{
	FilterTextUpdate( false );
}

void CSSFindSymbolDialog::FilterTextUpdate( bool hasDataChanged )
{
	wxString filterText = m_txtFilter->GetValue();
	filterText.MakeLower();

	// If internal data hasn't changed and the filter is the same - do nothing
	if ( !hasDataChanged && filterText == m_lastFilterText )
	{
		return;
	}

	if( hasDataChanged && !filterText.IsEmpty() )
	{
		*m_symbolEntriesFiltered = m_symbolEntriesAll;
	}

	if ( filterText == wxEmptyString )
	{
		(*m_symbolEntriesFiltered) = m_symbolEntriesAll;
	}
	else
	{
		vector< SSymbolEntry >* symbolEntriesToCheck = NULL;

		if ( filterText.StartsWith( m_lastFilterText ) )
		{
			 symbolEntriesToCheck = m_symbolEntriesFiltered;
		}
		else
		{
			symbolEntriesToCheck = &m_symbolEntriesAll;
		}

		for ( unsigned int i = 0; i < symbolEntriesToCheck->size(); ++i )
		{
			SSymbolEntry& entry = (*symbolEntriesToCheck)[ i ];
			wxString entryTextToCompare = entry.m_symbolName;
			entryTextToCompare.MakeLower();
			if ( entryTextToCompare.Contains( filterText ) )
			{
				m_symbolEntriesFilteredBuffer->push_back( entry );
			}
		}

		// Swap
		vector< SSymbolEntry >* tmp = m_symbolEntriesFiltered;
		m_symbolEntriesFiltered = m_symbolEntriesFilteredBuffer;
		m_symbolEntriesFilteredBuffer = tmp;
		m_symbolEntriesFilteredBuffer->clear();
	}

	m_lastFilterText = filterText;

	// Update GUI
	UpdateSymbolsListCtrl();

	m_wasEntriesDataUpdated = false;
}

bool CSSFindSymbolDialog::GetAllSymbolEntries( bool isSortingEnabled /* = true */ )
{
	RED_LOG( FindSymbolDialog, TXT( "GetAllSymbolEntries()" ) );
	SSStubSystemReadLock stubReadLock;

	const map< wstring, SSClassStub* >& classes = GStubSystem.GetAllClasses();
	const map< wstring, SSFunctionStub* >& functions = GStubSystem.GetAllFunctions();
	const map< wstring, SSEnumStub* >& enums = GStubSystem.GetAllEnums();

	// Decide if we should parse all symbols
	unsigned int filesToParseCount = GStubSystem.GetFilesCountToProcess();
	if
	(
		!m_forceReparse &&
		filesToParseCount == 0 &&
		m_numClasses == classes.size() &&
		m_numFunctions == functions.size() &&
		m_numEnums == enums.size()
	)
	{
		return false;
	}

	RED_LOG( RED_LOG_CHANNEL( FindSymbol ), TXT( "Starting parse" ) );

	m_symbolEntriesAll.clear();

	for ( map< wstring, SSFunctionStub* >::const_iterator ci = functions.begin(); ci != functions.end(); ++ci )
	{
		SSymbolEntry symbolEntry;

		// Symbol Name
		symbolEntry.m_symbolName = ci->first.c_str();

		// Signature Name
		symbolEntry.m_signatureName = GetFunctionSignature( ci->second );

		// Type Name
		symbolEntry.m_symbolTypeName = wxT("function");

		// File name
		symbolEntry.m_fileName = ci->second->m_context.m_file.c_str();

		// Data
		symbolEntry.m_symbolType = SET_Function;
		symbolEntry.m_index = ci->first.c_str();

		m_symbolEntriesAll.push_back( symbolEntry );
	}

	for ( map< wstring, SSClassStub* >::const_iterator ci = classes.begin(); ci != classes.end(); ++ci )
	{
		SSymbolEntry symbolEntry;

		// Symbol name
		symbolEntry.m_symbolName = ci->first.c_str();

		// Signature name
		wxString symbolSignature;
		symbolSignature = ((ci->second->m_stateMachine == L"" ? wxT("class ") : wxT("state ")) + ci->first).c_str();
		if ( ci->second->m_stateMachine != L"" )
		{
			symbolSignature += ( wxT(" in ") + ci->second->m_stateMachine ).c_str();
		}
		if ( ci->second->m_extends != L"" )
		{
			symbolSignature += ( wxT(" extends ") + ci->second->m_extends ).c_str();
		}
		symbolEntry.m_signatureName = symbolSignature;

		// Symbol Type Name
		symbolEntry.m_symbolTypeName = wxT("class");

		// File name
		symbolEntry.m_fileName = ci->second->m_context.m_file.c_str();

		// Data
		symbolEntry.m_symbolType = SET_Class;
		symbolEntry.m_index = ci->first;

		m_symbolEntriesAll.push_back( symbolEntry );

		// Parse class methods
		{
			const vector< SSFunctionStub* >& functions = ci->second->m_functions;
			for ( unsigned int i = 0; i < functions.size(); ++i )
			{
				const SSFunctionStub* functionStub = functions[ i ];

				SSymbolEntry symbolEntry;

				// Symbol Name
				symbolEntry.m_symbolName = functionStub->m_name.c_str();

				// Signature Name
				symbolEntry.m_signatureName = (wxT("class ") + ci->first + wxT(" => ")).c_str() + GetFunctionSignature( functionStub );

				// Symbol Type Name
				symbolEntry.m_symbolTypeName = wxT("method");

				// File name
				symbolEntry.m_fileName = ci->second->m_context.m_file.c_str();

				// Data
				symbolEntry.m_symbolType = SET_ClassMethod;
				symbolEntry.m_index = ci->first;
				symbolEntry.m_subIndex = i;

				m_symbolEntriesAll.push_back( symbolEntry );
			}
		}
		
		// Parse class properties
		{
			const vector< SSPropertyStub* >& fields = ci->second->m_fields;
			for ( unsigned int i = 0; i < fields.size(); ++i )
			{
				const SSPropertyStub* propertyStub = fields[ i ];

				SSymbolEntry symbolEntry;

				// Symbol Name
				symbolEntry.m_symbolName = propertyStub->m_name.c_str();

				// Signature Name
				symbolEntry.m_signatureName = ( wxT("class ") + ci->first + wxT(" => ") + propertyStub->m_name + wxT(" : ") + propertyStub->m_typeName ).c_str();

				// Type Name
				symbolEntry.m_symbolTypeName = wxT("field");

				// File name
				symbolEntry.m_fileName = ci->second->m_context.m_file.c_str();

				// Data
				symbolEntry.m_symbolType = SET_ClassField;
				symbolEntry.m_index = ci->first;
				symbolEntry.m_subIndex = i;

				m_symbolEntriesAll.push_back( symbolEntry );
			}
		}
	}

	for ( map< wstring, SSEnumStub* >::const_iterator ci = enums.begin(); ci != enums.end(); ++ci )
	{
		SSymbolEntry symbolEntry;

		// Symbol name
		symbolEntry.m_symbolName = ci->first.c_str();

		// Signature name
		const unsigned int EnumOptionsLimit = 4;
		wxString symbolSignature = (wxT("enum ") + ci->first + wxT(" { ")).c_str();
		const vector< SSEnumOptionStub* >& options = ci->second->m_options;
		for ( unsigned int i = 0; i < options.size(); )
		{
			const SSEnumOptionStub* enumOption = options[ i ];
			symbolSignature += enumOption->m_name.c_str();
			++i;
			if ( i >= EnumOptionsLimit )
			{
				symbolSignature += wxT(", ...");
				break;
			}
			if ( i < options.size() )
			{
				symbolSignature += wxT(", ");
			}
		}
		symbolSignature += wxT(" }");
		symbolEntry.m_signatureName = symbolSignature;

		// Symbol Type Name
		symbolEntry.m_symbolTypeName = wxT("enum");

		// File name
		symbolEntry.m_fileName = ci->second->m_context.m_file.c_str();

		// Data
		symbolEntry.m_symbolType = SET_Enum;
		symbolEntry.m_index = ci->first;

		m_symbolEntriesAll.push_back( symbolEntry );
	}

	if ( isSortingEnabled )
	{
		sort( m_symbolEntriesAll.begin(), m_symbolEntriesAll.end() );
	}

	if ( filesToParseCount == 0 )
	{
		m_forceReparse = false;
	}

	RED_LOG( RED_LOG_CHANNEL( FindSymbol ), wxT( "Finished parsing data" ) );

	m_wasEntriesDataUpdated = true;
	return true;
}

bool CSSFindSymbolDialog::GotoSymbol()
{
	SSStubSystemReadLock();

	// Invalid item selected
	if ( m_selectedItemIndex < 0 ) return false;

	const map< wstring, SSClassStub* >& classes = GStubSystem.GetAllClasses();
	const map< wstring, SSFunctionStub* >& functions = GStubSystem.GetAllFunctions();
	const map< wstring, SSEnumStub* >& enums = GStubSystem.GetAllEnums();

	map< wstring, SSFunctionStub* >	pie = functions;

	const SSymbolEntry& entry = (*m_symbolEntriesFiltered)[ m_selectedItemIndex ];

	SSBasicStub* stub = nullptr;
	switch ( entry.m_symbolType )
	{
	case SET_Function:
		stub = functions.at( entry.m_index );
		break;

	case SET_Class:
		stub = classes.at( entry.m_index );
		break;

	case SET_Enum:
		stub = enums.at( entry.m_index );
		break;

	case SET_ClassMethod:
		{
			auto it = classes.find( entry.m_index );

			if( it != classes.end() )
			{
				stub = it->second->m_functions.at( entry.m_subIndex );
			}
			else
			{
				stub = nullptr;
			}
		}
		break;

	case SET_ClassField:
		{
			auto it = classes.find( entry.m_index );

			if( it != classes.end() )
			{
				stub = it->second->m_fields.at( entry.m_subIndex );
			}
			else
			{
				stub = nullptr;
			}
		}
		break;
	}

	if ( stub && stub->m_name == entry.m_symbolName.wc_str() )
	{
		SolutionFilePtr solutionFile = m_solution->FindFile( stub->m_context.m_file );
		if ( solutionFile )
		{
			wxTheFrame->OpenFileAndGotoLine( solutionFile, true, stub->m_context.m_line );
			m_selectedDoc = solutionFile->m_document;
			return true;
		}
	}
	
	// chosen stub doesn't exist anymore - try reloading
	GetAllSymbolEntries( true );
	UpdateSymbolsListCtrl();
	FilterTextUpdate( true );

	return false;
}

CSSDocument* CSSFindSymbolDialog::GetSelectedDoc()
{
	return m_selectedDoc;
}

wxString CSSFindSymbolDialog::GetFunctionSignature( const SSFunctionStub* functionStub )
{
	wxString symbolSignature = functionStub->m_retValueType == L"" ? L"void" : functionStub->m_retValueType.c_str();
	symbolSignature += ( wxT(" ") + functionStub->m_name + wxT(" ( ") ).c_str();
	const vector< SSPropertyStub* >& params = functionStub->m_params;
	for ( unsigned int i = 0; i < params.size(); )
	{
		symbolSignature += ( params[i]->m_name + wxT(" : ") + params[i]->m_typeName ).c_str();
		if ( ++i < params.size() )
		{
			symbolSignature += wxT(", ");
		}
	}
	symbolSignature += wxT(" )");

	return symbolSignature;
}

//////////////////////////////////////////////////////////////////////////

bool CSSFindSymbolDialog::SSymbolEntry::operator < ( const SSymbolEntry& entry )
{
	return m_symbolName < entry.m_symbolName;
}

//////////////////////////////////////////////////////////////////////////

CSSFindSymbolParsingThread::CSSFindSymbolParsingThread( CSSFindSymbolDialog* findSymbolDialog )
	: m_requestExit( false )
	, m_findSymbolDialog( findSymbolDialog )
{
}

CSSFindSymbolParsingThread::~CSSFindSymbolParsingThread()
{

}

void CSSFindSymbolParsingThread::OnExit()
{
	m_requestExit = true;
}

wxThread::ExitCode CSSFindSymbolParsingThread::Entry()
{
	while ( !m_requestExit )
	{
// 		if ( ShouldParse() )
// 		{
// 			ParseSymbols();
// 		}

		wxThread::Sleep( 1000 ); // in milliseconds
	}

	return NULL;
}

bool CSSFindSymbolParsingThread::ShouldParse()
{
	unsigned int filesToParseCount = GStubSystem.GetFilesCountToProcess();
	if ( filesToParseCount == 0 )
	{
		return true;
	}

	return false;
}

void CSSFindSymbolParsingThread::ParseSymbols()
{
	m_findSymbolDialog->GetAllSymbolEntries( true );
}
