/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "localizedStringsEditor.h"
#include "localizedStringsWithKeysManager.h"
#include "../../common/engine/localizationManager.h"

// Event table
BEGIN_EVENT_TABLE( CLocalizedStringsEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "menuItemExit" ), CLocalizedStringsEditor::OnMenuExit )
END_EVENT_TABLE()


#define LOCSTRED_LOG( format, ... )	RED_LOG( LocStringsEd, format, ## __VA_ARGS__ )


class CLocStrEdWidgetsData : public wxObject
{
public:
	CLocStrEdWidgetsData( Int32 guiIndex, Int32 dataIndex ) : m_guiIndex( guiIndex ), m_dataIndex( dataIndex ) {}

	CLocStrEdWidgetsData* Clone() { return new CLocStrEdWidgetsData( m_guiIndex, m_dataIndex ); }

	Int32 GetGuiIndex() { return m_guiIndex; }
	Int32 GetDataIndex() { return m_dataIndex; }

protected:
	Int32 m_guiIndex;
	Int32 m_dataIndex;
};

CLocalizedStringsEditor::CLocalizedStringsEditor( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("LocalizedStringsEditor"), true )
	, m_idToolAdd( XRCID("toolAdd") )
	, m_idToolSave( XRCID("toolSave") )
	, m_idNextPage( XRCID("toolNextPage") )
	, m_idPrevPage( XRCID("toolPrevPage") )
	, m_idRefresh( XRCID("toolRefresh") )
	, m_idFind( XRCID("toolFind") )
	, m_idFindNext( XRCID("toolFindNext") )
	, m_sizerMain( NULL )
	, m_currentCategoryFilter( String::EMPTY )
	, m_defaultCategory( TXT("Default") )
	, m_numLinesPerPage( 20 )
	, m_curPageNum( 0 )
	, m_maxPagesNum( 1 )
	, m_staticTextPagesInfo( NULL )
	, m_textCtrlFind( NULL )
	, m_lastFindDataLineNum( 0 )
	, m_lastFindGuiLineNum( 0 )
{
	m_locStrings = new CLocalizedStringsWithKeys();
	m_locStrings->Initialize();

	// Get GUI elements
	{
		m_toolBar = XRCCTRL( *this, "toolBar", wxToolBar );
		m_toolBar->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CLocalizedStringsEditor::OnToolBar ), 0, this );

		m_panelMain = XRCCTRL( *this, "panelMain", wxPanel );
		m_sizerMain = new wxBoxSizer( wxVERTICAL );
		m_panelMain->SetSizer( m_sizerMain );

		m_scrolledWindowMain = XRCCTRL( *this, "scrolledWindowMain", wxScrolledWindow );
		m_scrolledWindowMain->SetScrollbars( 20, 20, 50, 50 );
		m_sizerScrolledWindowMain = new wxBoxSizer( wxVERTICAL );
		m_scrolledWindowMain->SetSizer( m_sizerScrolledWindowMain );

		m_choiceCategoriesFilter = XRCCTRL( *this, "choiceCategories", wxChoice );
		m_choiceCategoriesFilter->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CLocalizedStringsEditor::OnCategoryFilterChanged ), NULL, this );

		m_staticTextPagesInfo = XRCCTRL( *this, "staticTextPagesInfo", wxStaticText );

		m_textCtrlFind = XRCCTRL( *this, "textCtrlFind", wxTextCtrl );
		m_textCtrlFind->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CLocalizedStringsEditor::OnFind ), NULL, this );
	}

	InitializeGui();

	SEvents::GetInstance().RegisterListener( CNAME( CurrentLocaleChanged ), this );

	LoadOptionsFromConfig();
	Show();
}

CLocalizedStringsEditor::~CLocalizedStringsEditor()
{
	SaveOptionsToConfig();

	delete m_locStrings;
}

void CLocalizedStringsEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( CurrentLocaleChanged ) )
	{
		InitializeGUIEntries();
	}
}

void CLocalizedStringsEditor::OnToolBar( wxCommandEvent &event )
{
	const int idSelected = event.GetId();

	if ( idSelected == m_idToolSave )
	{
		SaveEntriesToDataBase();
	}
	else if ( idSelected == m_idToolAdd )
	{
		CreateNewEntry();
	}
	else if ( idSelected == m_idNextPage )
	{
		NextPage();
	}
	else if ( idSelected == m_idPrevPage )
	{
		PrevPage();
	}
	else if ( idSelected == m_idRefresh )
	{
		ReinitializeGUIEntries();
	}
	else if ( idSelected == m_idFind )
	{
		FindAndGoto( String( m_textCtrlFind->GetValue() ), false );
	}
	else if ( idSelected == m_idFindNext )
	{
		FindAndGoto( String( m_textCtrlFind->GetValue() ), true );
	}
}

void CLocalizedStringsEditor::OnButtonMenuClicked( wxCommandEvent &event )
{
	wxBitmapButton *obj =  dynamic_cast< wxBitmapButton* >( event.GetEventObject() );

	CLocStrEdWidgetsData *userData = dynamic_cast< CLocStrEdWidgetsData* >( event.m_callbackUserData );

	wxMenu *menu = new wxMenu();
	menu->Append( 0, wxT("Remove") );
	menu->Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CLocalizedStringsEditor::OnRemoveEntry ), userData->Clone(), this );
	obj->PopupMenu( menu );
}

void CLocalizedStringsEditor::OnStringKeyUpdated( wxCommandEvent &event )
{
	CLocStrEdWidgetsData *userData = dynamic_cast< CLocStrEdWidgetsData* >( event.m_callbackUserData );
	const Int32 dataIndex = userData->GetDataIndex();
	const Int32 guiIndex = userData->GetGuiIndex();

	String val = m_entriesKeys[ guiIndex ]->GetValue();

	m_locStrings->UpdateLocStringKey( dataIndex, val );
}

// Don't allow for creating many entries with the same key
void CLocalizedStringsEditor::OnStringKeyLeaveWindow( wxFocusEvent &event )
{
	CLocStrEdWidgetsData *userData = dynamic_cast< CLocStrEdWidgetsData* >( event.m_callbackUserData );
	const Int32 dataIndex = userData->GetDataIndex();
	const Int32 guiIndex = userData->GetGuiIndex();

	String val = m_entriesKeys[ guiIndex ]->GetValue();

	if ( val != String::EMPTY && m_locStrings->DoesKeyExist( dataIndex, val ) )
	{
		val = m_locStrings->GenerateUniqueKey( val );
		m_locStrings->UpdateLocStringKey( dataIndex, val );
		SetEntryValue( 0, guiIndex, val );
		wxMessageBox( TXT("Editor has changed your string key as it wasn't unique."), TXT("Unique key required!") );	
	}
}

void CLocalizedStringsEditor::OnStringCategoryComboSelected( wxCommandEvent &event )
{
	CLocStrEdWidgetsData *userData = dynamic_cast< CLocStrEdWidgetsData* >( event.m_callbackUserData );

	String val = m_entriesCategories[ userData->GetGuiIndex() ]->GetValue();
	if ( m_locStrings->GetCategoryIndex( val ) == -1 )
	{
		ASSERT( !TXT("Chosen non-existing category") );
		return;
	}
	m_locStrings->UpdateLocStringCategory( userData->GetDataIndex(), val );
}

void CLocalizedStringsEditor::OnStringCategoryEnterPressed( wxCommandEvent &event )
{
	CLocStrEdWidgetsData *userData = dynamic_cast< CLocStrEdWidgetsData* >( event.m_callbackUserData );

	String val = m_entriesCategories[ userData->GetGuiIndex() ]->GetValue();
	m_locStrings->UpdateCategory( val );
	m_locStrings->UpdateLocStringCategory( userData->GetDataIndex(), val );
	UpdateGUICategories();
}

void CLocalizedStringsEditor::OnStringValueUpdated( wxCommandEvent &event )
{
	CLocStrEdWidgetsData *userData = dynamic_cast< CLocStrEdWidgetsData* >( event.m_callbackUserData );

	const String val = m_entriesValues[ userData->GetGuiIndex() ]->GetValue();
	const Uint32 stringIndex = userData->GetDataIndex();
	m_locStrings->UpdateLocStringText( stringIndex, val );
}

void CLocalizedStringsEditor::OnCategoryFilterChanged( wxCommandEvent &event )
{
	String currentSelection = m_choiceCategoriesFilter->GetStringSelection();
	if ( m_currentCategoryFilter != currentSelection )
	{
		m_currentCategoryFilter = currentSelection;
		SetCurrentPageNum( 0 ); // reset page number
		ReinitializeGUIEntries();
	}
}

void CLocalizedStringsEditor::OnRemoveEntry( wxCommandEvent &event )
{
	CLocStrEdWidgetsData *userData = dynamic_cast< CLocStrEdWidgetsData* >( event.m_callbackUserData );

	m_locStrings->RemoveLocString( userData->GetDataIndex() );
	ReinitializeGUIEntries();
}

void CLocalizedStringsEditor::OnFind( wxCommandEvent &event )
{
	FindAndGoto( String( m_textCtrlFind->GetValue() ), false );
}

void CLocalizedStringsEditor::OnMenuExit( wxCommandEvent &event )
{
	SaveOptionsToConfig();
	Close();
}

void CLocalizedStringsEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/LocalizedStringsEditor") );
}

void CLocalizedStringsEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/LocalizedStringsEditor") );
}

void CLocalizedStringsEditor::InitializeGui()
{
	m_guiButtonsImages.Clear();
	m_guiButtonsImages.PushBack( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ARROW_RIGHT") ) );

	m_guiColumnsNames.Clear();
	m_guiColumnsNames.PushBack( TXT("Key") );
	m_guiColumnsNames.PushBack( TXT("Category") );
	m_guiColumnsNames.PushBack( TXT("Value") );
	
	CreateHeader();

	InitializeGUIEntries();
}

void CLocalizedStringsEditor::InitializeGUIEntries()
{
	Freeze();

	// Clear
	m_guiIndexToDataIndex.Clear();
	m_entriesKeys.Clear();
	m_entriesCategories.Clear();
	m_entriesValues.Clear();
	m_entriesMainBtns.Clear();
	m_entriesSizers.Clear();

	m_sizerScrolledWindowMain->Clear( true );

	const Uint32 locStringsSize = m_locStrings->GetSize();

	Int32 guiLineNum = 0;
	Int32 totalLineCandidatesNum = 0; // the number of lines that would be shown without paging system
	for ( Uint32 dataLineNum = 0; dataLineNum < locStringsSize; ++dataLineNum )
	{
		// check for filter
		if ( !m_locStrings->DoesMatchStringCategory( dataLineNum, m_currentCategoryFilter ) )
		{
			continue;
		}

		// check for deleted entries (marked for deletion)
		if ( m_locStrings->IsMarkedForDelete( dataLineNum ) )
		{
			continue;
		}

		// pages
		++totalLineCandidatesNum;
		if ( totalLineCandidatesNum <= m_curPageNum * m_numLinesPerPage )
		{
			continue;
		}
		if ( guiLineNum >= m_numLinesPerPage )
		{
			// lines per page limit exceeded
			continue;
		}

		AppendEntry( guiLineNum, dataLineNum );

		SetEntryValue( 0, guiLineNum, m_locStrings->GetStringKey( dataLineNum ) );
		SetEntryValue( 2, guiLineNum, m_locStrings->GetLocString( dataLineNum ).GetString() );

		m_guiIndexToDataIndex.Insert( guiLineNum, dataLineNum );

		guiLineNum++;
	}

	m_maxPagesNum = totalLineCandidatesNum / m_numLinesPerPage;
	if ( totalLineCandidatesNum % m_numLinesPerPage > 0 )
	{
		++m_maxPagesNum;
	}

	// Set pages info
	// Update GUI text (current number page/max number pages)
	m_staticTextPagesInfo->SetLabel( String::Printf( TXT("%d/%d"), (m_curPageNum+1), m_maxPagesNum ).AsChar() );

	UpdateGUICategories();

	Thaw();

	m_panelMain->Layout();
	m_sizerScrolledWindowMain->FitInside( m_scrolledWindowMain );
	m_scrolledWindowMain->Layout();
	Layout();
}

void CLocalizedStringsEditor::ReinitializeGUIEntries()
{
	Freeze();

	// Clear
	m_guiIndexToDataIndex.Clear();

	const Uint32 locStringsSize = m_locStrings->GetSize();

	Int32 guiLineNum = 0;
	Int32 totalLineCandidatesNum = 0; // the number of lines that would be shown without paging system
	for ( Uint32 dataLineNum = 0; dataLineNum < locStringsSize; ++dataLineNum )
	{
		// check for filter
		if ( !m_locStrings->DoesMatchStringCategory( dataLineNum, m_currentCategoryFilter ) )
		{
			continue;
		}

		// check for deleted entries (marked for deletion)
		if ( m_locStrings->IsMarkedForDelete( dataLineNum ) )
		{
			continue;
		}

		// pages
		++totalLineCandidatesNum;
		if ( totalLineCandidatesNum <= m_curPageNum * m_numLinesPerPage )
		{
			continue;
		}
		if ( guiLineNum >= m_numLinesPerPage )
		{
			// lines per page limit exceeded
			continue;
		}

		ReuseEntry( guiLineNum, dataLineNum );

		SetEntryValue( 0, guiLineNum, m_locStrings->GetStringKey( dataLineNum ) );
		SetEntryValue( 2, guiLineNum, m_locStrings->GetLocString( dataLineNum ).GetString() );

		m_guiIndexToDataIndex.Insert( guiLineNum, dataLineNum );

		guiLineNum++;
	}

	// Remove unused entries
	while ( (Int32)m_entriesKeys.Size() > guiLineNum )
	{
		m_entriesSizers.Back()->Clear( true );
		m_entriesKeys.PopBack();
		m_entriesCategories.PopBack();
		m_entriesValues.PopBack();
		m_entriesMainBtns.PopBack();
		m_entriesSizers.PopBack();
	}

	m_maxPagesNum = totalLineCandidatesNum / m_numLinesPerPage;
	if ( totalLineCandidatesNum % m_numLinesPerPage > 0 )
	{
		++m_maxPagesNum;
	}

	// Set pages info
	// Update GUI text (current number page/max number pages)
	m_staticTextPagesInfo->SetLabel( String::Printf( TXT("%d/%d"), (m_curPageNum+1), m_maxPagesNum ).AsChar() );

	UpdateGUICategories();

	Thaw();

	m_panelMain->Layout();
	m_sizerScrolledWindowMain->FitInside( m_scrolledWindowMain );
	m_scrolledWindowMain->Layout();
	Layout();
}

void CLocalizedStringsEditor::UpdateGUICategories()
{
	// Load data
	m_wxCategories.Clear();
	const TDynArray< String > &categories = m_locStrings->GetCategories();
	for ( Uint32 i = 0; i < categories.Size(); ++i )
	{
		m_wxCategories.Add( categories[i].AsChar() );
	}

	// Update GUI - header
	{
		// Categories choice box
		m_choiceCategoriesFilter->Clear();
		m_choiceCategoriesFilter->Append( wxEmptyString );
		m_choiceCategoriesFilter->Append( m_wxCategories );
		Int32 categorySelectionIndex = m_choiceCategoriesFilter->FindString( m_currentCategoryFilter.AsChar() );
		if ( categorySelectionIndex != wxNOT_FOUND )
		{
			m_choiceCategoriesFilter->SetSelection( categorySelectionIndex );
		}
		else
		{
			// Clear current filter, as there is no filter available
			m_currentCategoryFilter = String::EMPTY;
		}
	}

	// Update GUI - entries - combo boxes choices
	for ( TDynArray< wxComboBox * >::iterator i = m_entriesCategories.Begin();
		  i != m_entriesCategories.End();
		  ++i )
	{
		if ( (*i)->GetStrings() != m_wxCategories )
		{
			(*i)->Clear();
			(*i)->Append( m_wxCategories );
		}
	}

	// Update GUI - entries - combo boxes current values
	for ( Uint32 i = 0; i < m_entriesCategories.Size(); ++i )
	{
		Int32* guiIndex = m_guiIndexToDataIndex.FindPtr( i );
		if ( guiIndex )
		{
			const String &category = m_locStrings->GetStringCategory( *guiIndex );
			SetEntryValue( 1, i, category );
		}
	}
}

void CLocalizedStringsEditor::SaveEntriesToDataBase()
{
	SLocalizationManager::GetInstance().UpdateStringDatabase( m_locStrings, true );

	InitializeGUIEntries(); // Recreate GUI
}

void CLocalizedStringsEditor::CreateNewEntry()
{
	String currentSelection = m_choiceCategoriesFilter->GetStringSelection();
	const Int32 dataLineNum = m_locStrings->CreateLocStringEntry( TXT(""), TXT(""), currentSelection.Empty() ? m_defaultCategory : currentSelection );
	if ( dataLineNum > 0 )
	{
		const Int32 guiLineNum = m_entriesKeys.Size(); // newly created gui entry will have this number
		AppendEntry( guiLineNum, dataLineNum );
		m_guiIndexToDataIndex.Insert( guiLineNum, dataLineNum );
		UpdateGUICategories();
		m_sizerScrolledWindowMain->FitInside( m_scrolledWindowMain );
		m_scrolledWindowMain->Layout();
	}
}

void CLocalizedStringsEditor::CreateHeader()
{
	wxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );

	// Append spacer
	const Int32 spacerWidth = 30 * m_guiButtonsImages.Size();
	sizer->Add( spacerWidth, 0, 0, 0 );
	
	for ( Uint32 i = 0; i < m_guiColumnsNames.Size(); ++i )
	{
		// First column
		wxSizer *sizer1 = new wxBoxSizer( wxVERTICAL );
		wxStaticText *staticText = new wxStaticText( m_panelMain, wxID_ANY, m_guiColumnsNames[i].AsChar() );
		sizer1->Add( staticText, 1 /* try 0 */, 0, 5, NULL );
		sizer->Add( sizer1, 1, wxEXPAND, 5, NULL );
	}

	m_sizerMain->Add( sizer, 0, wxEXPAND, 5, NULL );
}

void CLocalizedStringsEditor::AppendEntry( Int32 guiIndex, Int32 dataIndex )
{
	wxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );
	wxBitmapButton *bitmapButton = new wxBitmapButton( m_scrolledWindowMain, wxID_ANY, m_guiButtonsImages[0] );
	wxTextCtrl *textCtrl1 = new wxTextCtrl( m_scrolledWindowMain, wxID_ANY, wxT("") );
	wxComboBox *textCtrl2 = new wxComboBox( m_scrolledWindowMain, wxID_ANY, wxT("") );
	textCtrl2->SetWindowStyle( textCtrl2->GetWindowStyle() | wxTE_PROCESS_ENTER );
	wxTextCtrl *textCtrl3 = new wxTextCtrl( m_scrolledWindowMain, wxID_ANY, wxT("") );
	sizer->Add( bitmapButton, 0, 0, 5, NULL );
	sizer->Add( textCtrl1, 1, 0, 5, NULL );
	sizer->Add( textCtrl2, 1, 0, 5, NULL );
	sizer->Add( textCtrl3, 1, 0, 5, NULL );

	m_sizerScrolledWindowMain->Add( sizer, 0, wxEXPAND, 5, NULL );

	// Connect handlers
	CLocStrEdWidgetsData *userData = new CLocStrEdWidgetsData( guiIndex, dataIndex );
	bitmapButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CLocalizedStringsEditor::OnButtonMenuClicked ), userData, this );
	textCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CLocalizedStringsEditor::OnStringKeyUpdated ), userData->Clone(), this );
	textCtrl1->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CLocalizedStringsEditor::OnStringKeyLeaveWindow ), userData->Clone(), this );
	textCtrl2->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CLocalizedStringsEditor::OnStringCategoryComboSelected ), userData->Clone(), this );
	textCtrl2->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CLocalizedStringsEditor::OnStringCategoryEnterPressed ), userData->Clone(), this );
	textCtrl3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CLocalizedStringsEditor::OnStringValueUpdated ), userData->Clone(), this );

	// Save GUI controls
	m_entriesKeys.PushBack( textCtrl1 );
	m_entriesCategories.PushBack( textCtrl2 );
	m_entriesValues.PushBack( textCtrl3 );
	m_entriesMainBtns.PushBack( bitmapButton );
	m_entriesSizers.PushBack( sizer );
}

void CLocalizedStringsEditor::ReuseEntry( Int32 guiIndex, Int32 dataIndex )
{
	// Reuse entry
	if ( guiIndex < (Int32)m_entriesKeys.Size() )
	{
		// Disconnect old handlers
		m_entriesKeys      [ guiIndex ]->Disconnect();
		m_entriesCategories[ guiIndex ]->Disconnect();
		m_entriesValues    [ guiIndex ]->Disconnect();
		m_entriesMainBtns  [ guiIndex ]->Disconnect();

		// Connect new handlers
		CLocStrEdWidgetsData *userData = new CLocStrEdWidgetsData( guiIndex, dataIndex );
		m_entriesMainBtns  [ guiIndex ]->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CLocalizedStringsEditor::OnButtonMenuClicked ), userData, this );
		m_entriesKeys      [ guiIndex ]->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CLocalizedStringsEditor::OnStringKeyUpdated ), userData->Clone(), this );
		m_entriesKeys      [ guiIndex ]->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CLocalizedStringsEditor::OnStringKeyLeaveWindow ), userData->Clone(), this );
		m_entriesCategories[ guiIndex ]->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CLocalizedStringsEditor::OnStringCategoryComboSelected ), userData->Clone(), this );
		m_entriesCategories[ guiIndex ]->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CLocalizedStringsEditor::OnStringCategoryEnterPressed ), userData->Clone(), this );
		m_entriesValues    [ guiIndex ]->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CLocalizedStringsEditor::OnStringValueUpdated ), userData->Clone(), this );
	}
	// Create new entry
	else
	{
		AppendEntry( guiIndex, dataIndex );
	}
}

Bool CLocalizedStringsEditor::SetEntryValue( Int32 x, Int32 y, const String& text )
{
	if ( y < 0 || y >= (Int32)m_entriesKeys.Size() || x < 0 || x > 2 )
	{
		return false;
	}

	if ( x == 0 )
	{
		m_entriesKeys[ y ]->ChangeValue( text.AsChar() );
	}
	else if ( x == 1 )
	{
		m_entriesCategories[ y ]->SetValue( text.AsChar() );
	}
	else if ( x == 2 )
	{
		m_entriesValues[ y ]->ChangeValue( text.AsChar() );
	}

	return true;
}

Int32 CLocalizedStringsEditor::GetMaxPagesNum()
{
	return m_maxPagesNum;
}

Int32 CLocalizedStringsEditor::GetCurrentPageNum()
{
	return m_curPageNum;
}

Bool CLocalizedStringsEditor::NextPage()
{
	return SetCurrentPageNum( GetCurrentPageNum() + 1 );
}

Bool CLocalizedStringsEditor::PrevPage()
{
	return SetCurrentPageNum( GetCurrentPageNum() - 1 );
}

Bool CLocalizedStringsEditor::SetCurrentPageNum( Int32 currentPageNum )
{
	if ( currentPageNum < 0 || currentPageNum >= GetMaxPagesNum() )
	{
		return false;
	}

	// We are currently on that page
	if ( m_curPageNum == currentPageNum )
	{
		return true;
	}
	
	m_curPageNum = currentPageNum;

	//InitializeGUIEntries();
	ReinitializeGUIEntries();

	return true;
}

Bool CLocalizedStringsEditor::FindAndGoto( const String &text, Bool continueFind /* = false */ )
{
	// Find

	const Uint32 locStringsSize = m_locStrings->GetSize();
	Int32 guiLineNum = continueFind ? m_lastFindGuiLineNum + 1 : 0;
	Bool isFoundKey = false;
	Bool isFoundValue = false;
	Uint32 dataLineNum = continueFind ? m_lastFindDataLineNum + 1 : 0;
	for ( ; dataLineNum < locStringsSize; ++dataLineNum )
	{
		// check for filter
		if ( !m_locStrings->DoesMatchStringCategory( dataLineNum, m_currentCategoryFilter ) )
		{
			continue;
		}

		// check for deleted entries (marked for deletion)
		if ( m_locStrings->IsMarkedForDelete( dataLineNum ) )
		{
			continue;
		}

		// Comparison method
		if ( m_locStrings->GetStringKey( dataLineNum ).ToUpper().ContainsSubstring( text.ToUpper() ) )
		{
			isFoundKey = true;
			break;
		}
		if ( m_locStrings->GetLocString( dataLineNum ).GetString().ToUpper().ContainsSubstring( text.ToUpper() ) )
		{
			isFoundValue = true;
			break;
		}

		guiLineNum++;
	}
	if ( !isFoundKey && !isFoundValue ) return false;
	m_lastFindDataLineNum = dataLineNum;
	m_lastFindGuiLineNum = guiLineNum;
	// Calculate the page number
	Int32 foundEntryPageNum = guiLineNum / m_numLinesPerPage;
	// Jump to that page
	SetCurrentPageNum( foundEntryPageNum );

	// Highlight found entry
	Int32 guiLineNumOnPage = guiLineNum - ( foundEntryPageNum * m_numLinesPerPage );
	if ( isFoundKey )
	{
		m_entriesKeys[ guiLineNumOnPage ]->SetFocus();
	}
	else if ( isFoundValue )
	{
		m_entriesValues[ guiLineNumOnPage ]->SetFocus();
	}

	return true;
}
