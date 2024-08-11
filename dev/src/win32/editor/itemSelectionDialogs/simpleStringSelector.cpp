#include "build.h"

#include "simpleStringSelector.h"
#include "../classHierarchyMapper.h"
#include "../../../common/engine/localizationManager.h"
#include "../redUserPrivileges.h"

//-----------------------------------------------------------------------------------------

// WX RTTI
IMPLEMENT_CLASS( CEdCreateLocalizedStringDialog, wxDialog );

CEdCreateLocalizedStringDialog::CEdCreateLocalizedStringDialog( wxWindow* parent, const String& defaultStringValue )
:	m_keyCtrl( NULL ),
	m_helpBubble( NULL )
{
	VERIFY( wxXmlResource::Get()->LoadDialog( this, parent, wxT( "AddNewStringDialog" ) ), TXT( "AddNewStringDialog is missing from editor_tools XRC" ) );

	SetSize( 500, 128 );

	m_keyCtrl = XRCCTRL( *this, "KeyCtrl", wxTextCtrl );
	ASSERT( m_keyCtrl != NULL, TXT( "KeyCtrl wxTextCtrl is missing from AddNewStringDialog in editor_tools XRC" ) );

	wxTextValidator keyValidator( wxFILTER_ASCII | wxFILTER_EXCLUDE_CHAR_LIST );
	keyValidator.SetCharExcludes( wxT( "'\";" ) );
	m_keyCtrl->SetValidator( keyValidator );

	m_stringCtrl = XRCCTRL( *this, "StringCtrl", wxTextCtrl );
	ASSERT( m_stringCtrl != NULL, TXT( "StringCtrl wxTextCtrl is missing from AddNewStringDialog in editor_tools XRC" ) );
	m_stringCtrl->SetValue( defaultStringValue.AsChar() );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdCreateLocalizedStringDialog::OnOK, this, wxID_OK );

	Bind( wxEVT_CLOSE_WINDOW, &CEdCreateLocalizedStringDialog::OnCancel, this );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdCreateLocalizedStringDialog::OnCancel, this, wxID_CANCEL );

	m_keyCtrl->Bind( wxEVT_CHAR, &CEdCreateLocalizedStringDialog::OnChar, this );
	m_stringCtrl->Bind( wxEVT_CHAR, &CEdCreateLocalizedStringDialog::OnChar, this );

	m_helpBubble = new CEdHelpBubble( this );
}

CEdCreateLocalizedStringDialog::~CEdCreateLocalizedStringDialog()
{

}

String CEdCreateLocalizedStringDialog::GetLocalizedString() const
{
	return m_stringCtrl->GetValue().wc_str();
}

String CEdCreateLocalizedStringDialog::GetKey() const
{
	return m_keyCtrl->GetValue().wc_str();
}

void CEdCreateLocalizedStringDialog::OnChar( wxKeyEvent& event )
{
	wxTextCtrl* textCtrl = wxStaticCast( event.GetEventObject(), wxTextCtrl );

	textCtrl->SetBackgroundColour( *wxWHITE );

	event.Skip();
}

void CEdCreateLocalizedStringDialog::OnOK( wxCommandEvent& event )
{
	wxColour problemColour = wxColour( 224, 130, 130 );

	wxString key = m_keyCtrl->GetValue();
	wxString str = m_stringCtrl->GetValue();

	if( str.IsEmpty() )
	{
		m_stringCtrl->SetBackgroundColour( problemColour );

		m_helpBubble->SetLabel( TXT( "String can't be empty" ) );
		m_helpBubble->SetPosition();
		m_helpBubble->Show();
	}
	else if( key.IsEmpty() )
	{
		m_keyCtrl->SetBackgroundColour( problemColour );

		m_helpBubble->SetLabel( TXT( "Key can't be empty" ) );
		m_helpBubble->SetPosition();
		m_helpBubble->Show();
	}
	else if( SLocalizationManager::GetInstance().DoesStringKeyExist( m_keyCtrl->GetValue().wc_str() ) )
	{
		// Key already exists!
		m_keyCtrl->SetBackgroundColour( problemColour );

		m_helpBubble->SetLabel( TXT( "This key already exists in the database, please specify another" ) );
		m_helpBubble->SetPosition();
		m_helpBubble->Show();
	}
	else
	{
		m_success = true;

		Hide();
	}
}

void CEdCreateLocalizedStringDialog::OnCancel( wxEvent& event )
{
	Hide();
}

//-----------------------------------------------------------------------------------------

wxDEFINE_EVENT( wxEVT_ITEM_SELECTOR_OK, wxCommandEvent );

// WX RTTI
IMPLEMENT_CLASS( CEdItemSelectorDialogBase, wxSmartLayoutDialog );

struct CEdItemSelectorDialogBaseTreeData : public wxTreeItemData
{
	void* m_data;
	Bool m_selectable;

	CEdItemSelectorDialogBaseTreeData( void* data, Bool selectable )
	:	m_data( data ),
		m_selectable( selectable )
	{

	}
};

CEdItemSelectorDialogBase::CEdItemSelectorDialogBase( wxWindow* parent, const Char* configPath )
:	m_tree( NULL ),
	m_search( NULL ),
	m_initialised( false ),
	m_configPath( configPath )
{
	// Load window
	VERIFY( wxXmlResource::Get()->LoadDialog( this, parent, wxT( "ItemSelector" ) ) );

	SetSize( 400, 500 );

	// Extract widgets
	m_tree = XRCCTRL( *this, "itemTree", wxTreeCtrl );
	ASSERT( m_tree != NULL, TXT( "itemTree not defined in frame ItemSelector in editor_tools XRC" ) );

	m_searchPanel = XRCCTRL( *this, "SearchPanel", wxPanel );
	ASSERT( m_searchPanel != NULL, TXT( "SearchPanel not defined in frame ItemSelector in editor_tools XRC" ) );

	m_search = XRCCTRL( *this, "Search", wxTextCtrl );
	ASSERT( m_search != NULL, TXT( "Search not defined in frame ItemSelector in editor_tools XRC" ) );

	m_searchPanel->Hide();

	SetToolTip( wxT( "Double click or single click and press enter to select a component" ) );

	m_rootItem = m_tree->AddRoot( TXT( "ROOT" ) );

	Bind( wxEVT_INIT_DIALOG, &CEdItemSelectorDialogBase::OnInitialize, this );
	Bind( wxEVT_SHOW, &CEdItemSelectorDialogBase::OnShow, this );
	Bind( wxEVT_CLOSE_WINDOW, &CEdItemSelectorDialogBase::OnClose, this );
	Bind( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &CEdItemSelectorDialogBase::OnItemSelected, this );
	Bind( wxEVT_SET_FOCUS , &CEdItemSelectorDialogBase::OnFocus, this );
	Bind( wxEVT_COMMAND_TREE_KEY_DOWN, &CEdItemSelectorDialogBase::OnStartSearchTree, this, XRCID( "itemTree" ) );
	Bind( wxEVT_KEY_DOWN, &CEdItemSelectorDialogBase::OnStartSearch, this, XRCID( "ItemSelector" ) );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdItemSelectorDialogBase::OnCloseSearchPanel, this, XRCID( "CloseSearchPanel" ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdItemSelectorDialogBase::OnSearchTextChange, this, XRCID( "Search" ) );
	Bind( wxEVT_COMMAND_TEXT_ENTER, &CEdItemSelectorDialogBase::OnSearchItemSelected, this, XRCID( "Search" ) );

	m_search->Bind( wxEVT_KEY_DOWN, &CEdItemSelectorDialogBase::OnSearchKeyDown, this );

	m_normalItemColour.Set( 255, 255, 255 );
	m_highlightedItemColour.Set( 130, 223, 224 );
	m_normalSearchColour.Set( 255, 255, 255 );
	m_nothingFoundSearchColour.Set( 224, 130, 130 );

	m_helpBubble = new CEdHelpBubble( this, TXT( "Start typing to search" ) );
}

CEdItemSelectorDialogBase::~CEdItemSelectorDialogBase()
{

}

void CEdItemSelectorDialogBase::Initialize()
{
	if( !m_initialised )
	{
		Populate();
		m_initialised = true;

		m_tree->SetFocus();
		Layout();
	}
}

void CEdItemSelectorDialogBase::OnInitialize( wxInitDialogEvent& event )
{
	Initialize();
}

void CEdItemSelectorDialogBase::OnShow( wxShowEvent& event )
{
	if( event.IsShown() )
	{
		LoadOptionsFromConfig();

		m_helpBubble->SetPosition();

		m_helpBubble->Show();
	}
	else
	{
		SaveOptionsToConfig();
	}
}

void CEdItemSelectorDialogBase::OnClose( wxCloseEvent& event )
{
	// Close window
	Destroy();
}

void CEdItemSelectorDialogBase::OnItemSelected( wxTreeEvent& event )
{
	wxTreeItemId selectedItem = event.GetItem();
	SelectAndClose( selectedItem );
}

void CEdItemSelectorDialogBase::OnSearchItemSelected( wxCommandEvent& event )
{
	if( m_highlightedItem.IsOk() )
	{
		SelectAndClose( m_highlightedItem );
	}
}

void CEdItemSelectorDialogBase::SelectAndClose( const wxTreeItemId selectedItem )
{
	ASSERT( selectedItem.IsOk() );

	CEdItemSelectorDialogBaseTreeData* data = static_cast< CEdItemSelectorDialogBaseTreeData* >( m_tree->GetItemData( selectedItem ) );

	if ( data->m_selectable )
	{
		// Hide the window and prepare a task to destroy it
		Hide();

		RunLaterOnce( [ this, data ]() {
			wxCommandEvent okPressedEvent( wxEVT_ITEM_SELECTOR_OK );
			okPressedEvent.SetClientData( data->m_data );
			okPressedEvent.SetEventObject( this );
			ProcessEvent( okPressedEvent );
			// Close window
			Destroy();
		} );
	}
}

void CEdItemSelectorDialogBase::OnStartSearchTree( wxTreeEvent& event )
{
	OnStartSearch( event.GetKeyEvent() );
}

void CEdItemSelectorDialogBase::OnStartSearch( const wxKeyEvent& event )
{
	m_helpBubble->Hide();

	int key = event.GetKeyCode();

	if( key == WXK_ESCAPE )
	{
		if( m_searchPanel->IsShown() )
		{
			CloseSearchPanel();
		}
		else
		{
			Close();
		}
	}
	else if( key > WXK_SPACE && key < WXK_DELETE )
	{
		m_search->SetFocus();
		m_search->WriteText( wxString( event.GetUnicodeKey() ) );

		m_searchPanel->Show();

		Layout();
	}
}

void CEdItemSelectorDialogBase::OnSearchKeyDown( wxKeyEvent& event )
{
	wxChar key = event.GetUnicodeKey();
	
	if( key == WXK_ESCAPE )
	{
		CloseSearchPanel();
	}
	else
	{
		event.Skip();
	}
}

void CEdItemSelectorDialogBase::OnSearchTextChange( wxCommandEvent& event )
{
	wxString searchTerm = event.GetString();

	if( searchTerm.Length() > 0 )
	{
		for( TNameToItemMap::iterator iter = m_nameToItemMap.Begin(); iter != m_nameToItemMap.End(); ++iter )
		{
			String& currentName = iter->m_first;

			if( currentName.ContainsSubstring( searchTerm ) )
			{
				if( iter->m_second != m_highlightedItem )
				{
					m_tree->EnsureVisible( iter->m_second );
					m_tree->SetItemBackgroundColour( iter->m_second, m_highlightedItemColour );
					if( m_highlightedItem.IsOk() )
					{
						m_tree->SetItemBackgroundColour( m_highlightedItem, m_normalItemColour );
					}
					m_highlightedItem = iter->m_second;

					m_tree->Refresh();
				}
				m_search->SetBackgroundColour( m_normalSearchColour );
				m_search->SetFocus();
				m_search->Refresh();

				return;
			}
		}
		m_search->SetBackgroundColour( m_nothingFoundSearchColour );
	}
	else
	{
		m_search->SetBackgroundColour( m_normalSearchColour );
	}

	m_search->Refresh();
}

void CEdItemSelectorDialogBase::OnCloseSearchPanel( wxCommandEvent& event )
{
	CloseSearchPanel();
}

void CEdItemSelectorDialogBase::CloseSearchPanel()
{
	if( m_highlightedItem.IsOk() )
	{
		m_tree->SetItemBackgroundColour( m_highlightedItem, m_normalItemColour );
		m_highlightedItem.Unset();
	}

	m_search->Clear();

	m_searchPanel->Hide();
	m_tree->SetFocus();
	Layout();
}

void CEdItemSelectorDialogBase::OnFocus( wxFocusEvent& event )
{
	if( !m_helpBubble->IsShown() )
	{
		m_helpBubble->Show();
	}
}

void CEdItemSelectorDialogBase::SetImageList( wxImageList* imageList )
{
	m_tree->AssignImageList( imageList );
}

void CEdItemSelectorDialogBase::AddItem( const String& name, void* data, Bool isSelectable, Int32 icon, Bool selected )
{
	wxTreeItemId newItem = m_tree->AppendItem( m_rootItem, name.AsChar(), icon, icon, new CEdItemSelectorDialogBaseTreeData( data, isSelectable ) );

	m_nameToItemMap.Insert( name, newItem );

	if( selected )
	{
		m_tree->SelectItem( newItem );
	}

	if( !isSelectable )
	{
		m_tree->SetItemTextColour( newItem, wxColour( 84, 0, 255 ) );
	}
}

void CEdItemSelectorDialogBase::AddItem( const String& name, void* data, const String& parentName, Bool isSelectable, Int32 icon, Bool selected )
{
	wxTreeItemId parentItem;

	if( m_nameToItemMap.Find( parentName, parentItem ) )
	{
		wxTreeItemId newItem = m_tree->AppendItem( parentItem, name.AsChar(), icon, icon, new CEdItemSelectorDialogBaseTreeData( data, isSelectable ) );

		m_nameToItemMap.Insert( name, newItem );

		if( selected )
		{
			m_tree->SelectItem( newItem );
		}

		if( !isSelectable )
		{
			m_tree->SetItemTextColour( newItem, wxColour( 84, 0, 255 ) );
		}
	}
	else
	{
		HALT( "Couldn't find parent item" );
	}
}

void CEdItemSelectorDialogBase::SaveOptionsToConfig()
{
	SaveLayout( m_configPath );
}

void CEdItemSelectorDialogBase::LoadOptionsFromConfig()
{
	LoadLayout( m_configPath );
}

//-----------------------------------------------------------------------------------------

// WX RTTI
IMPLEMENT_CLASS( CEdSimpleStringSelectorDialog, CEdItemSelectorDialogBase );

CEdSimpleStringSelectorDialog::CEdSimpleStringSelectorDialog( wxWindow* parent, const String& category, const CResource* newStringResource )
:	CEdItemSelectorDialogBase( parent, TXT( "/Frames/ComponentSelectorDialog" ) ),
	m_category( category ),
	m_newStringResource( newStringResource )
{
	SetTitle( wxString::Format( TXT( "Select %s" ), m_category.AsChar() ) );

	wxBitmap addItemBitmap = SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_PB_ADD" ) );

	wxBitmapButton* createNewButton = new wxBitmapButton( m_searchPanel, wxID_ANY, addItemBitmap );
	createNewButton->SetToolTip( wxString::Format( TXT( "Create new %s" ), m_category.AsChar() ) );
	createNewButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdSimpleStringSelectorDialog::OnCreateNewEntry, this );

	m_searchPanel->GetSizer()->Add( createNewButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );
}

CEdSimpleStringSelectorDialog::~CEdSimpleStringSelectorDialog()
{

}

void CEdSimpleStringSelectorDialog::Populate()
{
	TDynArray< String > categories;
	categories.PushBack( m_category );

	SLocalizationManager::GetInstance().SearchForStringsByCategory( TXT( "" ), categories, m_ids, NULL, &m_displayNames, false, SearchOrder_Text );

	for( Uint32 i = 0; i < m_ids.Size(); ++i )
	{
		AddItem( m_displayNames[ i ], &m_ids[ i ], true );
	}
}

void CEdSimpleStringSelectorDialog::OnCreateNewEntry( wxCommandEvent& event )
{
	CEdCreateLocalizedStringDialog* newStringDialog = new CEdCreateLocalizedStringDialog( this, m_search->GetValue().wc_str() );
	newStringDialog->ShowModal();

	if( newStringDialog->Success() )
	{
		String newEntry = newStringDialog->GetLocalizedString();
		m_newKey = newStringDialog->GetKey();

		if( !newEntry.Empty() )
		{
			m_newString.SetString( newEntry );

			SLocalizationManager::GetInstance().UpdateStringDatabase( this, true );

			// A temporary index is ok since the event is processed immediately
			Uint32 stringId = m_newString.GetIndex();

			// Send to parent
			wxCommandEvent okPressedEvent( wxEVT_ITEM_SELECTOR_OK );
			okPressedEvent.SetClientData( &stringId );
			okPressedEvent.SetEventObject( this );
			ProcessEvent( okPressedEvent );

			// Close the dialog
			Close();
		}
	}
}

void CEdSimpleStringSelectorDialog::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings )
{
	LocalizedStringEntry entry( &m_newString, m_category, m_newStringResource, String::EMPTY, m_newKey );

	localizedStrings.PushBack( entry );
}

//-----------------------------------------------------------------------------------------

CEdLocalizedStringPropertyEditorReadOnly::CEdLocalizedStringPropertyEditorReadOnly( CPropertyItem* item, const Char* category )
:	ICustomPropertyEditor( item ),
	m_focusPanel( NULL ),
	m_textDisplay( NULL ),
	m_idDisplay( NULL ),
	m_category( category )
{
	m_propertyItem->Read( &m_string );
}

CEdLocalizedStringPropertyEditorReadOnly::~CEdLocalizedStringPropertyEditorReadOnly()
{

}

void CEdLocalizedStringPropertyEditorReadOnly::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	if( !m_focusPanel )
	{
		m_focusPanel = new wxPanel( m_propertyItem->GetPage(), wxID_ANY, propRect.GetTopLeft(), propRect.GetSize() );

		wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );

		m_textDisplay = new wxTextCtrlEx( m_focusPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_NO_VSCROLL | wxNO_BORDER | wxTE_RICH2 );
		sizer->Add( m_textDisplay, 4, wxEXPAND, 0 );

		m_idDisplay = new wxTextCtrl( m_focusPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_NO_VSCROLL | wxNO_BORDER );
		m_idDisplay->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVECAPTION ) );
		sizer->Add( m_idDisplay, 1, wxEXPAND, 0 );

		wxBitmapButton* selectStringButton = new wxBitmapButton( m_focusPanel, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TXT( "IMG_PB_PICK" ) ) );
		selectStringButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdLocalizedStringPropertyEditorReadOnly::OnSelectStringDialog, this );
		sizer->Add( selectStringButton, 0, wxEXPAND, 0 );

		m_focusPanel->SetSizer( sizer );
		m_focusPanel->Layout();

		m_textDisplay->SetFocus();

		m_textDisplay->Bind( wxEVT_CHAR, &CEdLocalizedStringPropertyEditorReadOnly::OnChar, this );
		m_textDisplay->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdLocalizedStringPropertyEditorReadOnly::OnTextUpdated, this );

		CRedUserPrivileges priviliges = RetrieveRedUserPrivileges();
		m_focusPanel->Enable( priviliges.m_editRedStrings );
	}

	SetDisplayCtrlValues();
}

void CEdLocalizedStringPropertyEditorReadOnly::CloseControls()
{
	if( m_focusPanel )
	{
		ASSERT( !IsUnstableStringId( m_string.GetIndex() ) );
		SLocalizationManager::GetInstance().UpdateStringDatabase( m_string.GetIndex() );

		m_focusPanel->Destroy();
		m_focusPanel = NULL;
	}
}

void CEdLocalizedStringPropertyEditorReadOnly::GrabValue( Uint32& id, String& text )
{
	id = m_string.GetIndex();
	text = m_string.GetString( SLocalizationManager::GetInstance().GetCurrentLocale() );
}

Bool CEdLocalizedStringPropertyEditorReadOnly::GrabValue( String& displayValue )
{
	Uint32 unused;
	GrabValue( unused, displayValue );

	return true;
}

void CEdLocalizedStringPropertyEditorReadOnly::OnSelectStringDialog( wxCommandEvent& event )
{
	const CObject* object = m_propertyItem->GetRootObject( 0 ).AsObject();

	// I think there must be a more generic way to get the associated resource for a cobject
	const CResource* resource = NULL;
	if( object->IsA( ClassID< CEntity >() ) )
	{
		const CEntity* entity = static_cast< const CEntity* >( object );

		resource = Cast< const CResource >( entity->GetTemplate() );
	}

	CEdSimpleStringSelectorDialog* m_dialog = new CEdSimpleStringSelectorDialog( m_focusPanel, m_category, resource );

	m_dialog->Bind( wxEVT_ITEM_SELECTOR_OK, &CEdLocalizedStringPropertyEditorReadOnly::OnStringSelected, this );
	m_dialog->ShowModal();
}

void CEdLocalizedStringPropertyEditorReadOnly::OnStringSelected( wxCommandEvent& event )
{
	Uint32 id = *static_cast< Uint32* >( event.GetClientData() );

	m_string.SetIndex( id );

	m_propertyItem->Write( &m_string );

	SetDisplayCtrlValues();
}

void CEdLocalizedStringPropertyEditorReadOnly::SetDisplayCtrlValues()
{
	Uint32 id;
	String text;
	GrabValue( id, text );

	m_textDisplay->ChangeValue( text.AsChar() );
	m_idDisplay->ChangeValue( ToString( id ).AsChar() );

	if( id == 0 )
	{
		m_textDisplay->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVECAPTION ) );
		m_readOnly = true;
	}
	else
	{
		m_textDisplay->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
		m_readOnly = false;
	}
}

void CEdLocalizedStringPropertyEditorReadOnly::OnChar( wxKeyEvent& event )
{
	if( !m_readOnly )
	{
		event.Skip();
	}
}

void CEdLocalizedStringPropertyEditorReadOnly::OnTextUpdated( wxCommandEvent& event )
{
	ASSERT( !m_readOnly, TXT( "This function should get called when set to read only" ) );
	ASSERT( m_string.GetIndex() != 0, TXT( "New string creation is not allowed through the grid editor directly" ) );

	m_string.SetString( event.GetString().wc_str() );
}
