#include "build.h"

#include "detachablePanel.h"

CEdDetachablePanel::CEdDetachablePanel()
:	m_parent( NULL )
,	m_panel( NULL )
,	m_dialog( NULL )
{

}

CEdDetachablePanel::~CEdDetachablePanel()
{
	if( m_dialog )
	{
		m_panel->Reparent( m_parent );

		m_dialog->Destroy();

		m_dialog = NULL;
	}
}

void CEdDetachablePanel::Initialize( wxPanel* panel, const String& title )
{
	ASSERT( panel != NULL );

	m_title = title;

	m_panel = panel;
	m_parent = m_panel->GetParent();

	// Swap out existing sizer with a new one so that we don't mess up the layout of the panel
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	wxSizer* existingSizer = m_panel->GetSizer();
	m_panel->SetSizer( sizer, false );

	wxBitmap icon = SEdResources::GetInstance().LoadBitmap( TXT( "IMG_DETACH_WINDOW" ) );

	wxBitmapToggleButton* button = new wxBitmapToggleButton( m_panel, wxID_ANY, icon, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );
	button->SetToolTip( TXT( "Float Window" ) );
	button->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdDetachablePanel::OnDetach, this );

	sizer->Add( button, 0 );
	sizer->Add( existingSizer, 1, wxEXPAND );
}

void CEdDetachablePanel::OnDetach( wxCommandEvent& event )
{
	if( event.IsChecked() )
	{
		ASSERT( m_dialog == NULL );

		m_dialog = new wxDialog( m_parent, wxID_ANY, m_title.AsChar(), wxDefaultPosition, m_panel->GetSize(), wxSTAY_ON_TOP | wxRESIZE_BORDER | wxCAPTION );

		m_panel->Hide();

		m_panel->Reparent( m_dialog );

		m_dialog->Layout();

		m_dialog->Show();

		m_panel->Show();
	}
	else
	{
		ASSERT( m_dialog != NULL );

		m_dialog->Hide();

		m_panel->Reparent( m_parent );

		m_parent->Layout();

		m_panel->Show();

		m_dialog->Destroy();

		m_dialog = NULL;

	}
}

