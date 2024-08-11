/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#if 0

#include "../../games/witcher3/npcSpawnSet.h"
#include "spawnSetActionEditor.h"

DEFINE_EVENT_TYPE( wxEVT_SPAWNSETACTIONEDITOR_OK )
DEFINE_EVENT_TYPE( wxEVT_SPAWNSETACTIONEDITOR_CANCEL )

// Event table
BEGIN_EVENT_TABLE( CEdSpawnSetActionEditor, wxDialog )
	EVT_BUTTON  ( XRCID( "buttonOK" ),           CEdSpawnSetActionEditor::OnOK )
	EVT_BUTTON  ( XRCID( "buttonCancel" ),       CEdSpawnSetActionEditor::OnCancel )
	EVT_COMBOBOX( XRCID( "comboBoxActionType" ), CEdSpawnSetActionEditor::OnComboBox )
END_EVENT_TABLE()

CEdSpawnSetActionEditor::CEdSpawnSetActionEditor( wxWindow* parent, CNPCSpawnSet *actionParent, ISpawnSetTimetableAction *sstActionInit )
	: m_sstAction( NULL )
	, m_sstActionParent( actionParent )
{
	// Load designed dialog from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TEXT("SpawnSetActionEditor") );

	m_guiPanelMain = XRCCTRL( *this, "panelMain", wxPanel );
	m_guiActionType = XRCCTRL( *this, "comboBoxActionType", wxComboBox );
	m_guiTextCtrl0 = XRCCTRL( *this, "textCtrl0", wxTextCtrl );
	m_guiTextCtrl1 = XRCCTRL( *this, "textCtrl1", wxTextCtrl );
	m_guiTextCtrl0Label = XRCCTRL( *this, "staticText0", wxStaticText );
	m_guiTextCtrl1Label = XRCCTRL( *this, "staticText1", wxStaticText );

	if ( CSpawnSetTimetableActionCategory *sstActionCategory = Cast<CSpawnSetTimetableActionCategory>(sstActionInit) )
	{
		ChangeToActionCategory();
		m_guiTextCtrl0->SetValue( sstActionCategory->GetActionCategory().AsChar() );
		m_guiTextCtrl1->SetValue( sstActionCategory->GetRole().AsChar() );
	}

	// Custom controls - we don't need these for now
	//wxPanel *panel = XRCCTRL( *this, "panelMain", wxPanel );
	//wxBoxSizer *sizer = new wxBoxSizer( wxVERTICAL );
	//wxTextCtrl *textCtrl = new wxTextCtrlEx( panel, wxID_ANY );
	//sizer->Add( textCtrl, 1, wxEXPAND, 0 );
	//panel->SetSizer( sizer );
}

CEdSpawnSetActionEditor::~CEdSpawnSetActionEditor()
{
}

void CEdSpawnSetActionEditor::OnOK( wxCommandEvent& event )
{
	// Create ISpawnSetTimetableAction implementation
	String value = m_guiActionType->GetValue();
	if ( value == TXT("Action category") )
	{
		CSpawnSetTimetableActionCategory *sstActionCategory = CreateObject< CSpawnSetTimetableActionCategory >( m_sstActionParent );
		sstActionCategory->SetActionCategory( CName( m_guiTextCtrl0->GetValue() ) );
		sstActionCategory->SetRole( CName( m_guiTextCtrl1->GetValue() ) );

		m_sstAction = sstActionCategory;
	}

	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_SPAWNSETACTIONEDITOR_OK );
	ProcessEvent( okPressedEvent );

	// Close window
	MakeModal( false );
	Destroy();
}

void CEdSpawnSetActionEditor::OnCancel( wxCommandEvent& event )
{
	// Send to parent
	wxCommandEvent cancelPressedEvent( wxEVT_SPAWNSETACTIONEDITOR_CANCEL );
	ProcessEvent( cancelPressedEvent );

	// Close window
	MakeModal( false );
	Destroy();
}

ISpawnSetTimetableAction* CEdSpawnSetActionEditor::GetSSTAction()
{
	ISpawnSetTimetableAction* tmp = m_sstAction;
	m_sstAction = NULL;
	return tmp;
}

void CEdSpawnSetActionEditor::OnComboBox( wxCommandEvent& event )
{
	String value = m_guiActionType->GetValue();

	if ( value == TXT("Action category") )
	{
		ChangeToActionCategory();
	}

	m_guiPanelMain->Layout();
}

void CEdSpawnSetActionEditor::ChangeToActionCategory()
{
	m_guiActionType->SetValue( TXT("Action category") );

	m_guiTextCtrl1Label->Show();
	m_guiTextCtrl1->Show();

	m_guiTextCtrl0Label->SetLabel( TXT("Action category") );
	m_guiTextCtrl1Label->SetLabel( TXT("Role") );
}

#endif
