/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "customControlList.h"

CEdCustomControl::CEdCustomControl( wxWindow* parent, const wxString& xrcName, Bool hasKillButton /*= false*/, Bool hasFindButton /*= false*/ )
	: m_killButton( NULL )
	, m_findButton( NULL )
	, m_selected( false )
{
	// Fetch XRC layout
	wxXmlResource::Get()->LoadPanel( this, parent, xrcName );

	// Store typed parent list for convenience
	m_parentList = static_cast< CEdCustomControlListPanel* >( parent );

	// Selection
	{
		Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CEdCustomControl::OnLeftDown ), NULL, this );
		Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CEdCustomControl::OnMouseEnter ), NULL, this );

		// Since wxWidgets is shit, I have to do idiocy like this
		wxWindowList& children = GetChildren();
		for ( wxWindowList::iterator it = children.begin(); it != children.end(); ++it )
		{
			wxWindow* childWindow = (*it);
			if ( dynamic_cast< wxStaticText* >( childWindow ) || dynamic_cast< wxStaticBitmap* >( childWindow ) )
			{
				(*it)->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CEdCustomControl::OnLeftDown ), NULL, this );
				(*it)->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CEdCustomControl::OnMouseEnter ), NULL, this );
			}
		}
	}


#if 0
	Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CEdCustomControl::OnMouseEnter ), NULL, this );
	Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( CEdCustomControl::OnMouseLeave ), NULL, this );
#endif

	// Connect the kill button (TODO: rethink this approach later)
	if ( hasKillButton )
	{
		m_killButton = XRCCTRL( *this, "m_killButton", wxBitmapButton );
		RED_ASSERT( m_killButton, TXT("If create custom control with 'hasKillButton' flag, then supply the control with such button.") );
		m_killButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdCustomControlListPanel::OnKillButtonClicked ), NULL, m_parentList );
	}

	if ( hasFindButton )
	{
		m_findButton = XRCCTRL( *this, "m_findButton", wxBitmapButton );
		RED_ASSERT( m_killButton, TXT("If create custom control with 'hasFindButton' flag, then supply the control with such button.") );
		m_findButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdCustomControlListPanel::OnFindButtonClicked ), NULL, m_parentList );
	}
}

CEdCustomControl::~CEdCustomControl()
{
	if ( m_killButton )
	{
		m_killButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdCustomControlListPanel::OnKillButtonClicked ), NULL, m_parentList );
	}
}

void CEdCustomControl::OnLeftDown( wxMouseEvent& event )
{
	m_parentList->OnItemClickedLMB( this );
}

void CEdCustomControl::OnMouseEnter( wxMouseEvent& event )
{
	m_parentList->OnItemHovered( this );
}

void CEdCustomControl::Select( Bool doRefresh /*= true*/ )
{
	SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	if ( doRefresh ) Refresh();
	m_selected = true;
}

void CEdCustomControl::Deselect( Bool doRefresh /*= true*/ )
{
	SetBackgroundColour( wxColour( 0xffffff ) );
	if ( doRefresh ) Refresh();

	m_selected = false;
}

//////////////////////////////////////////////////////////////////////////

CEdCustomControlListPanel::CEdCustomControlListPanel( wxWindow* parent, Bool allowMultiselection )
	: CDropTarget( parent )
	, wxScrolledWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxScrolledWindowStyle|wxCLIP_CHILDREN|wxTAB_TRAVERSAL )
	, m_allowMultiselection( allowMultiselection )
{
	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	SetScrollRate( 5, 5 );
	Layout();
}

CEdCustomControl* CEdCustomControlListPanel::AddItem( CObject* object )
{
	// Create custom control instance
	CEdCustomControl* entryPanel = CreateCustomControl( object );
	GetSizer()->Add( entryPanel, 0, wxBOTTOM|wxEXPAND, 1 );
	FitInside();
	Layout();

	// Create logical list entry
	m_items.Grow();
	SListItem& addedItem = m_items.Back();
	addedItem.m_object = object;
	addedItem.m_panel = entryPanel;

	// Added. Configure your custom control instance to properly picture the object.
	OnAddItem( addedItem.m_object.Get(), addedItem.m_panel );

	return addedItem.m_panel;
}

void CEdCustomControlListPanel::Clear()
{
	Freeze();

	for ( SListItem& item : m_items )
	{
		// Destroy custom control
		GetSizer()->Detach( item.m_panel );
		delete item.m_panel;
	}

	Layout();
	Thaw();

	m_items.Clear();
}

void CEdCustomControlListPanel::SelectItem( CObject* object, Bool state )
{
	if ( CEdCustomControl* panel = FindPanel( object ) )
	{
		SelectItem( panel, state );
	}
}

void CEdCustomControlListPanel::SelectItem( CEdCustomControl* control, Bool state )
{
	if ( !m_allowMultiselection )
	{
		DeselectAll();
	}

	if ( state )
	{
		control->Select();
	}
	else
	{
		control->Deselect();
	}

	//Refresh();

	if ( state )
	{
		PostItemSelection( FindObject( control ), control );
	}
}

void CEdCustomControlListPanel::DeselectAll()
{
	for ( SListItem& item : m_items )
	{
		if ( item.m_panel->IsSelected() )
		{
			item.m_panel->Deselect();
		}
	}
}

Bool CEdCustomControlListPanel::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	for ( Uint32 i=0; i<resources.Size(); ++i )
	{
		CResource* res = resources[i];
		if ( res->IsA( GetExpectedObjectClass() ) )
		{
			if ( FindPanel( res ) )
			{
				// Already added
				continue;
			}

			if ( !CanAddItem( res ) )
			{
				// Validation failed
				continue;
			}

			// perform add
			AddItem( res );
		}
	}

	return true;
}

wxDragResult CEdCustomControlListPanel::OnDragOver( wxCoord x, wxCoord y, wxDragResult def )
{
	return wxDragCopy;
}

void CEdCustomControlListPanel::OnMouseLeftDown( wxMouseEvent& event )
{
}

void CEdCustomControlListPanel::OnKillButtonClicked( wxCommandEvent& event )
{
	CEdCustomControl* control = (CEdCustomControl*)( (wxBitmapButton*)event.GetEventObject() )->GetParent();
	if ( control )
	{
		// Callback implementor
		//SListItem* item = FindItem( control );

		auto itemIt = FindIf( m_items.Begin(), m_items.End(), [ control ]( const SListItem& item ) { return item.m_panel == control; } );
		RED_ASSERT( itemIt != m_items.End(), TXT("An item kill button was clicked, but that item is not on the items list. Debug!") );

		if ( !OnItemRemove( itemIt->m_object.Get(), itemIt->m_panel ) )
		{
			// Can't remove atm
			return;
		}

		Freeze();

		// Destroy custom control
		GetSizer()->Detach( control );
		delete control;
		m_items.Erase( itemIt );

		Layout();
		Thaw();

		// Set focus here to avoid random focusing on other kill buttons (yeah..)
		SetFocus();
	}
}

void CEdCustomControlListPanel::OnFindButtonClicked( wxCommandEvent& event )
{
	CEdCustomControl* control = (CEdCustomControl*)( (wxBitmapButton*)event.GetEventObject() )->GetParent();
	if ( control )
	{
		CObject* object = FindObject( control );
		RED_ASSERT( object, TXT("An item kill button was clicked, but that item is not on the items list. Debug!") );
		CResource* res = SafeCast< CResource >( object );
		wxTheFrame->GetAssetBrowser()->SelectFile( res->GetDepotPath() );
	}
}

void CEdCustomControlListPanel::OnItemClickedLMB( CEdCustomControl* control )
{
	const Bool isMultiselection = m_allowMultiselection && ( wxGetKeyState( WXK_CONTROL ) );

	if ( !isMultiselection )
	{
		DeselectAll();
	}

	if ( control->IsSelected() && isMultiselection )
	{
		SelectItem( control, false );
	}
	else
	{
		SelectItem( control, true );
	}
}

CEdCustomControl* CEdCustomControlListPanel::FindPanel( CObject* object )
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		if ( m_items[i].m_object == object )
		{
			// Found
			return m_items[i].m_panel;
		}
	}

	// Not found
	return nullptr;
}

CObject* CEdCustomControlListPanel::FindObject( CEdCustomControl* panel )
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		if ( m_items[i].m_panel == panel )
		{
			// Found
			return m_items[i].m_object.Get();
		}
	}

	// Not found
	return nullptr;
}

Bool CEdCustomControlListPanel::IsItemSelected( CObject* object )
{
	if ( CEdCustomControl* panel = FindPanel( object ) )
	{
		return panel->IsSelected();
	}

	return false;
}

void CEdCustomControlListPanel::SetupControl( CObject* object )
{
	if ( CEdCustomControl* panel = FindPanel( object ) )
	{
		return panel->DoSetup();
	}
}