#include "build.h"
#include "stringSelectorTool.h"
#include "../../common/engine/localizationManager.h"

wxDEFINE_EVENT( wxEVT_CHOOSE_STRING_OK, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_CHOOSE_STRING_CANCEL, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_CHOOSE_STRING_NEW, wxCommandEvent );

CEdStringSelector::CEdStringSelector( wxWindow* parent )
:	m_searchKeys( false )
{
	// Load window
	VERIFY( wxXmlResource::Get()->LoadFrame( this, parent, wxT( "stringSelector" ) ) );

	LoadOptionsFromConfig();

	TDynArray< String > categories;

	SLocalizationManager::GetInstance().ReadAllStringsCategories( categories, false );

	// Extract widgets
	m_categoryList = XRCCTRL( *this, "categoryList", wxCheckListBox );
	ASSERT( m_categoryList != NULL, TXT( "categoryList wxCheckListBox missing from stringSelector XRC" ) );

	m_results = XRCCTRL( *this, "searchResults", wxListCtrl );
	ASSERT( m_results != NULL, TXT( "searchResults wxListCtrl missing from stringSelector XRC" ) );

	wxArrayString wxCategories;

	for( Uint32 i = 0; i < categories.Size(); ++i )
	{
		wxCategories.Add( categories[ i ].AsChar() );
	}

	m_categoryList->InsertItems( wxCategories, 0 );


	// Event Bindings
	 
	Bind( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &CEdStringSelector::OnCategorySelected, this, XRCID( "categoryList" ) );
	Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdStringSelector::OnSearchTypeChange, this, XRCID( "searchType" ) );

	Bind( wxEVT_COMMAND_TEXT_ENTER, &CEdStringSelector::OnSearchExecute, this, XRCID( "searchBox" ) );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStringSelector::OnOK, this, wxID_OK );
	Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &CEdStringSelector::OnItemDoubleClicked, this, XRCID( "searchResults" ) );
	Bind( wxEVT_COMMAND_LIST_ITEM_SELECTED, &CEdStringSelector::OnItemClicked, this, XRCID( "searchResults" ) );
	 
	Bind( wxEVT_CLOSE_WINDOW, &CEdStringSelector::OnClose, this );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStringSelector::OnCancel, this, wxID_CANCEL );
}

CEdStringSelector::~CEdStringSelector()
{
	Unbind( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &CEdStringSelector::OnCategorySelected, this, XRCID( "categoryList" ) );
	Unbind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdStringSelector::OnSearchTypeChange, this, XRCID( "searchType" ) );

	Unbind( wxEVT_COMMAND_TEXT_ENTER, &CEdStringSelector::OnSearchExecute, this, XRCID( "searchBox" ) );

	Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStringSelector::OnOK, this, XRCID( "ok" ) );
	Unbind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &CEdStringSelector::OnItemDoubleClicked, this, XRCID( "searchResults" ) );
	Unbind( wxEVT_COMMAND_LIST_ITEM_SELECTED, &CEdStringSelector::OnItemClicked, this, XRCID( "searchResults" ) );

	Unbind( wxEVT_CLOSE_WINDOW, &CEdStringSelector::OnClose, this );
	Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStringSelector::OnCancel, this, XRCID( "cancel" ) );

	SaveOptionsToConfig();
}

void CEdStringSelector::OnCategorySelected( wxCommandEvent& event )
{
	int categoryIndex = event.GetInt();

	String category( m_categoryList->GetString( categoryIndex ) );

	if( m_categoryList->IsChecked( categoryIndex ) )
	{
		m_selectedCategories.PushBackUnique( category );
	}
	else
	{
		m_selectedCategories.Remove( category );
	}
}

void CEdStringSelector::OnSearchTypeChange( wxCommandEvent& event )
{
#define SEARCH_KEYS_CHOICE_INDEX 1

	if( event.GetInt() == SEARCH_KEYS_CHOICE_INDEX )
	{
		m_searchKeys = true;
	}
	else
	{
		m_searchKeys = false;
	}
}

void CEdStringSelector::OnSearchExecute( wxCommandEvent& event )
{
	if( m_selectedCategories.Size() > 0 )
	{
		String query( event.GetString() );

		TDynArray< Uint32 > ids;
		TDynArray< String > keys;
		TDynArray< String > strings;

		SLocalizationManager::GetInstance().SearchForStringsByCategory( query, m_selectedCategories, ids, &keys, &strings, m_searchKeys );

		m_results->ClearAll();

		m_results->InsertColumn( 0, wxT( "ID" ) );
		m_results->InsertColumn( 1, wxT( "Key" ) );
		m_results->InsertColumn( 2, wxT( "Text" ) );

		for( Uint32 i = 0; i < ids.Size(); ++ i )
		{
			long rowId = m_results->InsertItem( i, ToString( ids[ i ] ).AsChar() );
			m_results->SetItem( rowId, 1, keys[ i ].AsChar() );
			m_results->SetItem( rowId, 2, strings[ i ].AsChar() );

			m_results->SetItemData( rowId, ids[ i ] );
		}

		m_results->SetColumnWidth( 0, wxLIST_AUTOSIZE );
		m_results->SetColumnWidth( 2, wxLIST_AUTOSIZE );

		Layout();
	}
}

void CEdStringSelector::OnItemDoubleClicked( wxListEvent& event )
{
	m_selectedItem = event.GetIndex();

	SubmitAndClose();
}

void CEdStringSelector::OnItemClicked( wxListEvent& event )
{
	m_selectedItem = event.GetIndex();
}

void CEdStringSelector::OnOK( wxCommandEvent& event )
{
	SubmitAndClose();
}

void CEdStringSelector::SubmitAndClose()
{
	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_CHOOSE_STRING_OK );
	okPressedEvent.SetInt( m_results->GetItemData( m_selectedItem ) );
	okPressedEvent.SetEventObject( this );
	ProcessEvent( okPressedEvent );

	// Close window
	Destroy();
}

void CEdStringSelector::OnCancel( wxCommandEvent& event )
{
	CancelAndClose();
}

void CEdStringSelector::OnClose( wxCloseEvent& event )
{
	CancelAndClose();
}

void CEdStringSelector::CancelAndClose()
{
	wxCommandEvent cancelEvent( wxEVT_CHOOSE_STRING_CANCEL );
	cancelEvent.SetEventObject( this );
	ProcessEvent( cancelEvent );

	// Close window
	Destroy();
}

void CEdStringSelector::SaveOptionsToConfig()
{
	SaveLayout( TXT( "/Frames/StringSelectorTool" ) );
}

void CEdStringSelector::LoadOptionsFromConfig()
{
	LoadLayout( TXT( "/Frames/StringSelectorTool" ) );
}
