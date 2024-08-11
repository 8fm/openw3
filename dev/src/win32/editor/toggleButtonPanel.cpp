/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CEdToggleButtonPanel::CEdToggleButtonPanel()
	: m_parent( NULL )
	, m_button( NULL )
	, m_panel( NULL )
{
}

void CEdToggleButtonPanel::Init( wxWindow* window, const Char* baseName )
{
	ASSERT( window );
	m_parent = window;

	// Find button
	String buttonName = String( TXT("Tab") ) + baseName;
	m_button = (wxToggleButton*) m_parent->FindWindow( buttonName.AsChar() );
	ASSERT( m_button );

	// Find panel
	String panelName = String( baseName ) + TXT("Panel");
	m_panel = (wxPanel*) m_parent->FindWindow( panelName.AsChar() );
	ASSERT( m_panel );

	// Hide panel
	m_button->SetValue( false );
	m_panel->Show( false );

	// Connect events
	m_button->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdToggleButtonPanel::OnToggle ), NULL, this );
}

void CEdToggleButtonPanel::OnToggle( wxCommandEvent& event )
{
	Expand( m_button->GetValue() );
}

void CEdToggleButtonPanel::Expand( Bool expand )
{
	m_panel->Freeze();
	m_parent->Freeze();
	m_button->SetValue( expand );
	m_panel->Show( expand );
	m_parent->Layout();
	m_panel->Thaw();
	m_parent->Thaw();
	m_panel->Refresh( false );
}