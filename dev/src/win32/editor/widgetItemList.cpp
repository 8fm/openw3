#include "build.h"

#include <wx/artprov.h>
#include <wx/listimpl.cpp>
#include <wx/spinctrl.h>

wxDEFINE_EVENT( edEVT_COMMAND_ITEMCONTAINERWITHBUTTON_CLICKED, wxCommandEvent );

BEGIN_EVENT_TABLE(CEdItemContainerWithButton, wxControl)
	EVT_SIZE(CEdItemContainerWithButton::OnSize)
	END_EVENT_TABLE()

	CEdItemContainerWithButton::CMiniButton::CMiniButton( wxWindow* parent, const wxString& title, long style )
	: wxButton( parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, style )
{
}

bool CEdItemContainerWithButton::CMiniButton::SetBackgroundColour( const wxColour& colour )
{
	if ( !wxControl::SetBackgroundColour( colour ) )
		return false;

	Refresh();

	return true;
}

CEdItemContainerWithButton::CItemContainer::CItemContainer( wxWindow* parent )
	: wxControl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxTRANSPARENT_WINDOW )
	, m_widget( NULL )
{
	wxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
	SetSizer( sizer );
	Bind( wxEVT_ERASE_BACKGROUND, &CItemContainer::OnEraseBackground, this );
}

void CEdItemContainerWithButton::CItemContainer::SetWidget( wxWindow* widget )
{
	if ( m_widget )
	{
		m_widget->Destroy();
	}
	if ( widget )
	{
		widget->Reparent( this );
		GetSizer()->Add( widget, 1, wxALIGN_CENTER_VERTICAL, 0 );
		Layout();
	}
	m_widget = widget;
}

CEdItemContainerWithButton::CEdItemContainerWithButton( wxWindow* parent )
	: wxControl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER )
{
	SetBackgroundColour( parent->GetBackgroundColour() );
	m_removeButton = new CMiniButton( this, wxT("\u2716"));
	m_removeButton->SetBackgroundColour( parent->GetBackgroundColour() );
	m_removeButton->SetFont( m_removeButton->GetFont().Larger() );
	m_removeButton->SetToolTip( wxT("Remove") );
	m_removeButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdItemContainerWithButton::OnRemoveClicked, this );

	m_itemContainer = new CItemContainer( this );
}

wxSize CEdItemContainerWithButton::DoGetBestSize() const
{
	return wxSize( 100, 28 );
}

void CEdItemContainerWithButton::OnSize( wxSizeEvent& event )
{
	wxRect client = GetClientRect();
	m_removeButton->SetPosition( wxPoint( client.GetWidth() - client.GetHeight(), 0 ) );
	m_removeButton->SetSize( wxSize( client.GetHeight(), client.GetHeight() ) );

	m_itemContainer->SetPosition( wxPoint( 0, 0 ) );
	m_itemContainer->SetSize( wxSize( client.GetWidth() - client.GetHeight() - 4, client.GetHeight() ) );
}

void CEdItemContainerWithButton::OnRemoveClicked( wxCommandEvent& event )
{
	wxCommandEvent e( edEVT_COMMAND_ITEMCONTAINERWITHBUTTON_CLICKED );
	e.SetEventObject( this );
	wxPostEvent( this, e );
}

//////////////////////////////////////////////////////////////////////////

WX_DEFINE_LIST( CEdWidgetItemListItemList );

void CEdWidgetItemList::OnAddButtonClicked( wxCommandEvent& event )
{
	OnAddItem();
}

void CEdWidgetItemList::OnRemoveButtonClicked( wxCommandEvent& event )
{
	wxString msg;
	uintptr_t index = (uintptr_t)((CEdItemContainerWithButton*)event.GetEventObject())->GetClientData();
	CEdWidgetItemListItem* item = index < m_items->GetCount() ? (*m_items)[index] : NULL;
	if ( item && ShouldRemoveItem( item ) )
	{
		RemoveItem( item );
	}
}

void CEdWidgetItemList::OnAddButtonPopupPaint( wxEraseEvent& event )
{
	wxClientDC dc( m_addButtonPopup );
	dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOX ) ) );
	dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ), 1, wxPENSTYLE_SOLID ) );
	dc.DrawRectangle( 0, 0, m_addButtonPopup->GetClientSize().GetWidth(), m_addButtonPopup->GetClientSize().GetHeight() + 1);
}

void CEdWidgetItemList::OnSize( wxSizeEvent& event )
{
	UpdateAddButtonPopupPosition();
}

void CEdWidgetItemList::UpdateAddButtonPopupPosition()
{
	if ( !m_addButtonPopup || !m_addButton )
	{
		return;
	}

	wxSize size = GetSize();
	wxPoint pos = GetPosition();

	pos = wxPoint( pos.x + size.GetWidth() - m_addButtonPopup->GetSize().GetWidth(), pos.y - m_addButtonPopup->GetSize().GetHeight() + 1 );

	if ( pos != m_addButtonPopup->GetPosition() )
	{
		m_addButtonPopup->SetPosition( pos );
		m_addButtonPopup->Layout();
		m_addButtonPopup->Refresh();
	}
}

void CEdWidgetItemList::CreateAddButton()
{
	if ( m_addButton )
	{
		DestroyAddButton();
	}

	m_addButtonPopup = new wxPanel( GetParent() );
	m_addButtonPopup->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );
	m_addButtonPopup->Bind( wxEVT_ERASE_BACKGROUND, &CEdWidgetItemList::OnAddButtonPopupPaint, this );

	m_addButton = new CEdItemContainerWithButton::CMiniButton( m_addButtonPopup, m_addButtonTitle );
	m_addButton->SetBackgroundColour( GetBackgroundColour() );
	m_addButtonPopup->GetSizer()->Add( m_addButton, 0, wxALL | wxALIGN_RIGHT, 3 );
	m_addButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdWidgetItemList::OnAddButtonClicked, this );

	m_addButtonPopup->Show();
	m_addButtonPopup->Layout();
	m_addButtonPopup->Fit();

	m_timer.owner = this;
	m_timer.Start( 10 );
}

void CEdWidgetItemList::DestroyAddButton()
{
	if ( m_addButtonPopup )
	{
		m_timer.Stop();

		m_addButtonPopup->Destroy();
		m_addButton = NULL;
		m_addButtonPopup = NULL;
	}
}

void CEdWidgetItemList::OnAddItem()
{
	AddItem( new wxStaticText( this, wxID_ANY, wxT("Override CEdWidgetItemList::OnAddItem to put a proper widget here") ) );
}

CEdWidgetItemList::CEdWidgetItemList( wxWindow* parent, wxPoint pos, wxSize size, long style )
	: wxScrolledWindow( parent, wxID_ANY, pos, size, style|wxCLIP_CHILDREN )
	, m_addButtonTitle( "Add" )
	, m_items( NULL )
	, m_addButton( NULL )
{
	m_items = new CEdWidgetItemListItemList();
	m_removeButtonTip = wxT("Remove");

	SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOX ) );
	SetScrollRate( 5, 5 );
	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	CreateAddButton();
	Layout();
	Bind( wxEVT_SIZE, &CEdWidgetItemList::OnSize, this );
}

CEdWidgetItemList::~CEdWidgetItemList()
{
	Unbind( wxEVT_SIZE, &CEdWidgetItemList::OnSize, this );
	RemoveAllItems();
	DestroyAddButton();
	delete m_items;
}

bool CEdWidgetItemList::Enable( bool enable /* = true */ )
{
	if ( m_addButtonPopup )
	{
		m_addButtonPopup->Enable( enable );
		m_addButton->Enable( enable );
	}
	return wxScrolledWindow::Enable( enable );
}

void CEdWidgetItemList::SetAddButtonTitle( const wxString& title )
{
	m_addButtonTitle = title;
	m_addButton->SetLabelText( title );
	m_addButtonPopup->Layout();
	m_addButtonPopup->Fit();
}

wxString CEdWidgetItemList::GetAddButtonTitle() const
{
	return m_addButton->GetLabelText();
}

void CEdWidgetItemList::SetRemoveButtonTip( const wxString& tip )
{
	m_removeButtonTip = tip;

	for ( size_t i=0; i<m_items->GetCount(); ++i )
	{
		(*m_items)[i]->m_containerWithButton->GetButton()->SetToolTip( tip );
	}
}

wxString CEdWidgetItemList::GetRemoveButtonTip() const
{
	return m_removeButtonTip;
}

void CEdWidgetItemList::RemoveAllItems()
{
	Freeze();
	DestroyAddButton();

	for ( size_t i=0; i<m_items->GetCount(); ++i )
	{
		(*m_items)[i]->m_containerWithButton->Destroy();
		delete (*m_items)[i];
	}
	m_items->Clear();

	FitInside();
	Thaw();

	if ( m_addButtonPopup )
	{
		m_addButtonPopup->Refresh();
	}
}

CEdWidgetItemListItem* CEdWidgetItemList::AddItem( wxWindow* widget )
{
	CEdWidgetItemListItem* item = new CEdWidgetItemListItem();

	Freeze();

	item->m_containerWithButton = new CEdItemContainerWithButton( this );
	item->m_containerWithButton->Bind( edEVT_COMMAND_ITEMCONTAINERWITHBUTTON_CLICKED, &CEdWidgetItemList::OnRemoveButtonClicked, this );
	item->m_containerWithButton->SetClientData( (void*)(uintptr_t)m_items->GetCount() );
	item->m_containerWithButton->GetButton()->SetToolTip( m_removeButtonTip );

	GetSizer()->Add( item->m_containerWithButton, 0, wxALL | wxEXPAND, 2 );
	item->SetWidget( widget );

	FitInside();

	wxApp::GetInstance()->ProcessPendingEvents();

	Scroll( 0, GetScrollRange( wxVERTICAL ) );

	Thaw();

	m_items->Append( item );

	if ( m_addButtonPopup )
	{
		m_addButtonPopup->Refresh();
	}

	return item;
}

bool CEdWidgetItemList::ShouldRemoveItem( CEdWidgetItemListItem* item ) const
{
	return true;
}

void CEdWidgetItemList::RemoveItem( CEdWidgetItemListItem* item )
{
	Freeze();
	m_items->DeleteObject( item );
	item->m_containerWithButton->Destroy();
	delete item;
	for ( size_t i=0; i<m_items->GetCount(); ++i )
	{
		(*m_items)[i]->m_containerWithButton->SetClientData( (void*)(uintptr_t)i );
	}
	FitInside();
	Thaw();
	if ( m_addButtonPopup )
	{
		m_addButtonPopup->Refresh();
	}
}

