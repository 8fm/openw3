#include "build.h"

#include "ghostConfigDialog.h"

wxDEFINE_EVENT( wxEVT_GHOSTCONFIGUPDATED, wxCommandEvent );

wxIMPLEMENT_CLASS(  CEdGhostConfigDialog, wxSmartLayoutDialog );

CEdGhostConfigDialog::CEdGhostConfigDialog( wxWindow* parent, Uint32 count, PreviewGhostContainer::EGhostType type )
{
	// Load window
	VERIFY( wxXmlResource::Get()->LoadDialog( this, parent, wxT( "AnimationGhostConfig" ) ) );

	// Extract widgets
	wxChoice* ghostTypeWidget = XRCCTRL( *this, "GhostType", wxChoice );
	ASSERT( ghostTypeWidget != NULL, TXT( "GhostType not defined in AnimationGhostConfig in editor_beh XRC" ) );

	wxSpinCtrl* ghostNumberWidget = XRCCTRL( *this, "NumberOfGhosts", wxSpinCtrl );
	ASSERT( ghostNumberWidget != NULL, TXT( "NumberOfGhosts not defined in AnimationGhostConfig in editor_beh XRC" ) );

	ghostTypeWidget->SetSelection( type );
	ghostNumberWidget->SetValue( count );

	ghostTypeWidget->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdGhostConfigDialog::OnTypeSelected, this );
	ghostNumberWidget->Bind( wxEVT_COMMAND_SPINCTRL_UPDATED, &CEdGhostConfigDialog::OnNumberOfInstancesChanged, this );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdGhostConfigDialog::OnOK, this, wxID_OK );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdGhostConfigDialog::OnOK, this, wxID_APPLY );

	m_numberOfInstances = count;
	m_type = type;
}

CEdGhostConfigDialog::~CEdGhostConfigDialog()
{

}

void CEdGhostConfigDialog::OnNumberOfInstancesChanged( wxSpinEvent& event )
{
	m_numberOfInstances = static_cast< Uint32 >( event.GetValue() );
}

void CEdGhostConfigDialog::OnTypeSelected( wxCommandEvent& event )
{
	m_type = static_cast< PreviewGhostContainer::EGhostType >( event.GetInt() );
}

void CEdGhostConfigDialog::OnOK( wxCommandEvent& okEvent )
{
	wxCommandEvent* event = new wxCommandEvent( wxEVT_GHOSTCONFIGUPDATED );
	event->SetEventObject( this );
	GetEventHandler()->QueueEvent( event );

	okEvent.Skip();
}
