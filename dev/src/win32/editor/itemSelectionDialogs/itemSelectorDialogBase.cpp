#include "build.h"
#include "../build.h"
#include "itemSelectorDialogBase.h"

CEdItemSelectorImpl::CEdItemSelectorImpl( wxWindow* parent, const String& configPath, const String& tile, Bool storeFilter )
	: CEdPoppedUp( parent, wxGetMousePosition() - wxSize( 10, 10 ), wxSize( 400, 400 ), 0, CEdSizeConstaints( 200, 200, 600, 800 ), tile )
	, m_result( nullptr )
	, m_configPath( configPath )
	, m_storeFilter( storeFilter )
{
	SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_tree = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxTR_TWIST_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_HIDE_ROOT|wxTR_SINGLE );
	m_tree->Bind( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &CEdItemSelectorImpl::OnTreeItemActivated, this );
	m_tree->Bind( wxEVT_CHAR_HOOK, &CEdItemSelectorImpl::OnTreeCharHook, this );
	GetSizer()->Add( m_tree, 1, wxEXPAND, 0 );

	m_searchCtrl = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_NOHIDESEL|wxCLIP_CHILDREN );
	m_searchCtrl->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdItemSelectorImpl::OnSearchTextChanged, this );
	m_searchCtrl->ShowCancelButton( true );
	m_searchCtrl->SetDescriptiveText( TXT("Start typing to search") );
 	m_searchCtrl->Bind( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, &CEdItemSelectorImpl::OnSearchCancel, this );
	m_searchCtrl->Bind( wxEVT_CHAR_HOOK, &CEdItemSelectorImpl::OnSearchCtrlCharHook, this );
	GetSizer()->Add( m_searchCtrl, 0, wxEXPAND, 0 );

	m_searchTimer = new wxTimer( this, 0 );
	Bind( wxEVT_TIMER, &CEdItemSelectorImpl::OnTimer, this );

	LoadOptionsFromConfig();
}

CEdItemSelectorImpl::~CEdItemSelectorImpl()
{
	SaveOptionsToConfig();
}

void CEdItemSelectorImpl::SetImageList( wxImageList* imageList )
{
	m_tree->AssignImageList( imageList );
}

void CEdItemSelectorImpl::AddItem( const String& itemPath, void* data, Bool selectable, Int32 icon, Bool isSelected )
{
	String itemName = itemPath.StringAfter( String( &PATH_SEPARATOR, 1 ), true );

	if ( !itemName.Empty() )
	{
		m_elements.Insert( itemName, Element( itemPath, data, selectable, icon ) );
	}
	else
	{
		m_elements.Insert( itemPath, Element( itemPath, data, selectable, icon ) );
	}

	if ( isSelected )
	{
		m_lastSelectionPath = itemPath;
	}
}

void CEdItemSelectorImpl::AddItem( const String& itemName, void* data, const String& parentItemName, Bool selectable, Int32 icon, Bool isSelected )
{
	if ( Element* parent = m_elements.FindPtr( parentItemName ) )
	{
		String itemPath = parent->m_path + String( &PATH_SEPARATOR, 1 ) + itemName;
		m_elements.Insert( itemName, Element( itemPath, data, selectable, icon ) );

		if ( isSelected )
		{
			m_lastSelectionPath = itemPath;
		}
	}
	else
	{
		HALT( "Cannot find tree parent" );
	}
}


int CEdItemSelectorImpl::ShowModal( wxPoint* position )
{
	String filterVal = String( m_searchCtrl->GetValue().c_str() );
	RebuildTree( filterVal );

	if ( position )
	{
		wxSize size = GetSize();
		SmartSetSize( position->x, position->y, size.x, size.y );
	}

	return CEdPoppedUp< wxPanel >::ShowModal();
}

void CEdItemSelectorImpl::EndModal( int retCode )
{
	Bool wasWaitingForFilter = m_searchTimer->IsRunning();

	m_searchTimer->Stop(); // make sure that timer event won't be sent to non-existing window

	if ( retCode == wxID_OK ) // try to read a result and close only if it's valid
	{
		if ( wasWaitingForFilter )
		{
			// make sure we'll take most up-to-date search results
			String filterVal = String( m_searchCtrl->GetValue().c_str() );
			RebuildTree( filterVal );
		}

		wxTreeItemId selectedItem = m_tree->GetSelection();

		if ( selectedItem.IsOk() )
		{
			Element::Wrapper* elem = static_cast< Element::Wrapper* >( m_tree->GetItemData( selectedItem ) );

			if ( elem && elem->GetData()->m_selectable )
			{
				m_result = elem->GetData()->m_userData;
				CEdPoppedUp< wxPanel >::EndModal( wxID_OK );
			}
		}
	}
	else
	{
		CEdPoppedUp< wxPanel >::EndModal( retCode );
	}
}

void CEdItemSelectorImpl::RebuildTree( const String& filter )
{
	SaveSelection();

	m_tree->Freeze();

	m_tree->DeleteAllItems();
	wxTreeItemId root = m_tree->AddRoot( TXT( "ROOT" ) );

	THashMap< String, wxTreeItemId > elemMap;

	for ( const auto& mapElem : m_elements )
	{
		const Element& element = mapElem.m_second;

		if ( !filter.Empty() && !element.m_path.ContainsSubstring( filter, false, 0, true ) )
		{
			continue;
		}

		TDynArray< String > parts = element.m_path.Split( String( &PATH_SEPARATOR, 1 ) );

		if ( parts.Empty() )
		{
			continue;
		}

		// finds and/or create parents on the way
		wxTreeItemId curItem = root;
		for ( const String& part : parts )
		{
			if ( wxTreeItemId* existingElem = elemMap.FindPtr( part ) )
			{
				curItem = *existingElem;
			}
			else
			{
				curItem = m_tree->AppendItem( curItem, part.AsChar(), element.m_iconIdx, element.m_iconIdx );
				elemMap.Insert( part, curItem );
			}

			// find the element corresponding to the current part to see if it's selectable (if exists)
			Element* partElem = m_elements.FindPtr( part );
			if ( partElem == nullptr || !partElem->m_selectable )
			{
				m_tree->SetItemTextColour( curItem, wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
			}
		}

		ASSERT( curItem != root ); // sth should be already added
		m_tree->SetItemData( curItem, new Element::Wrapper( &element ) ); // leaf gets the actual item data, the rest are just path parts

	}

	// it should be before any scrolling
	m_tree->Thaw();	
	
	if ( m_tree->GetChildrenCount( root ) == 0 )
	{
		if ( !m_elements.Empty() )
		{
			m_searchCtrl->SetBackgroundColour( wxColour( 255, 200, 200 ) );
			m_searchCtrl->Refresh();

			wxTreeItemId noResultsItem = m_tree->AppendItem( root, TXT("No search results") );
			m_tree->SetItemTextColour( noResultsItem, wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );			
		}
	}
	else
	{
		m_searchCtrl->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
		m_searchCtrl->Refresh();

		if ( filter.Empty() )
		{
			m_tree->CollapseAll();
		}
		else
		{
			m_tree->ExpandAll();
		}
		

		wxTreeItemIdValue cookie;
		wxTreeItemId firstElem = m_tree->GetFirstChild( root, cookie );
		if ( firstElem.IsOk() )
		{
			m_tree->ScrollTo( firstElem );
		}

		RestoreSelection();
	}
}

void CEdItemSelectorImpl::SaveSelection()
{
	wxTreeItemId selectedItem = m_tree->GetSelection();
	if ( selectedItem.IsOk() )
	{
		if ( Element::Wrapper* elem = static_cast< Element::Wrapper* >( m_tree->GetItemData( selectedItem ) ) )
		{
			m_lastSelectionPath = elem->GetData()->m_path;
		}
	}
}

namespace 
{
	template < typename DataT, typename Pred >
	wxTreeItemId FindTreeItem( const wxTreeCtrl& tree, wxTreeItemId parent, Pred pred )
	{
		wxTreeItemIdValue cookie;
		wxTreeItemId child = tree.GetFirstChild( parent, cookie );
		while ( child.IsOk() )
		{
			if ( DataT* data = static_cast< DataT* >( tree.GetItemData( child ) ) )
			{
				if ( pred( data ) )
				{
					return child;
				}
			}

			wxTreeItemId childRes = FindTreeItem< DataT >( tree, child, pred );

			if ( childRes.IsOk() )
			{
				return childRes;
			}

			child = tree.GetNextChild( parent, cookie ); 
		}

		return wxTreeItemId();
	}
}

String CEdItemSelectorImpl::FindNameForData( void* data ) const
{
	wxTreeItemId item = 
		FindTreeItem< Element::Wrapper >( 
			*m_tree, m_tree->GetRootItem(),
			[ data ]( Element::Wrapper* elem ) { return elem->GetData()->m_userData == data; }
		);

	if ( item.IsOk() )
	{
		return String( m_tree->GetItemText( item ).c_str() );
	}
	else
	{
		return String::EMPTY;
	}
}

void CEdItemSelectorImpl::RestoreSelection()
{
	if ( m_tree->IsEmpty() )
	{
		return;
	}

	wxTreeItemId itemToSelect;

	if ( !m_lastSelectionPath.Empty() )
	{
		itemToSelect = 
			FindTreeItem< Element::Wrapper >( 
				*m_tree, m_tree->GetRootItem(),
				[ this ]( Element::Wrapper* elem ) { return m_lastSelectionPath == elem->GetData()->m_path; }
			);
	}

	// if there's no selection stored, select the first selectable encountered
	if ( !itemToSelect.IsOk() )
	{
		itemToSelect = 
			FindTreeItem< Element::Wrapper >( 
				*m_tree, m_tree->GetRootItem(),
				[ ]( Element::Wrapper* elem ) { return elem->GetData()->m_selectable; }
			);
	}

	if ( itemToSelect.IsOk() )
	{
		m_tree->SelectItem( itemToSelect );
		m_tree->EnsureVisible( itemToSelect );
	}
}

void CEdItemSelectorImpl::OnTreeCharHook( wxKeyEvent& event )
{
	int code = event.GetKeyCode();

	if ( code == WXK_ESCAPE || code == WXK_RETURN )
	{
		EndModal( code == WXK_ESCAPE ? wxID_ABORT : wxID_OK );
	}
	else if ( code == WXK_UP || code == WXK_DOWN || code == WXK_PAGEUP || code == WXK_PAGEDOWN )
	{
		event.DoAllowNextEvent();
	}
	else
	{
		m_searchCtrl->EmulateKeyPress( event );
	}
}

void CEdItemSelectorImpl::OnSearchCtrlCharHook( wxKeyEvent& event )
{
	int code = event.GetKeyCode();

	if ( code == WXK_ESCAPE || code == WXK_RETURN )
	{
		EndModal( code == WXK_ESCAPE ? wxID_ABORT : wxID_OK );
	}
	if ( code != WXK_UP && code != WXK_DOWN && code != WXK_PAGEUP && code != WXK_PAGEDOWN )
	{
		event.DoAllowNextEvent();
	}
	else
	{
		m_tree->SetFocus();
	}
}

void CEdItemSelectorImpl::OnTreeItemActivated( wxTreeEvent& event )
{
	EndModal( wxID_OK );
}

void CEdItemSelectorImpl::OnSearchTextChanged( wxCommandEvent& event )
{
	m_searchTimer->Start( 500, wxTIMER_ONE_SHOT );
}

void CEdItemSelectorImpl::OnSearchCancel( wxCommandEvent& event )
{
	m_searchCtrl->SetValue( wxEmptyString );
}

void CEdItemSelectorImpl::OnTimer( wxTimerEvent& event )
{
	String filterVal = String( m_searchCtrl->GetValue().c_str() );
	RebuildTree( filterVal );
}

void CEdItemSelectorImpl::SaveOptionsToConfig()
{
	if ( !m_configPath.Empty() )
	{
		if ( m_storeFilter )
		{
			CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
			CConfigurationScopedPathSetter( config, m_configPath );
			String filterVal = String( m_searchCtrl->GetValue().c_str() );
			config.Write( TXT("Filter"), filterVal );
		}

		SaveLayout( m_configPath, true );
	}
}

void CEdItemSelectorImpl::LoadOptionsFromConfig()
{
	if ( !m_configPath.Empty() )
	{
		if ( m_storeFilter )
		{
			CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
			CConfigurationScopedPathSetter( config, m_configPath );
			String filterVal = config.Read( TXT("Filter"), String::EMPTY );
			if ( !filterVal.Empty() )
			{
				m_searchCtrl->SetValue( filterVal.AsChar() );
				m_searchCtrl->SelectAll();
			}
		}

		LoadLayout( m_configPath, true );
	}
}
