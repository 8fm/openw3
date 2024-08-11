/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameResourceEditor.h"

BEGIN_EVENT_TABLE( CGameResourceEditor, wxFrame )
		EVT_MENU( XRCID( "menuItemSave" ), CGameResourceEditor::OnSave )
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

CGameResourceEditor::CGameResourceEditor( wxWindow* parent, CGameResource* gameResource )
	: m_gameResource( gameResource )
{
	ASSERT( m_gameResource.IsValid() );
	if ( !m_gameResource.IsValid() )
	{
		return;
	}

	m_gameResource->AddToRootSet();

	// Load window
	wxXmlResource::Get()->LoadFrame( this, parent, wxT( "GameResourceEditor" ) );

	SetSize( 700, 400 );

	// Set title
	SetTitle( wxString::Format( wxT("Game Resource Editor [%s]"), m_gameResource->GetDepotPath().AsChar() ) );

	// Game Resource panel
	{
		wxPanel* panel = XRCCTRL( *this, "GameResourcePanel", wxPanel );
		ASSERT( panel, TXT("Properties panel placeholder is missing") );
		PropertiesPageSettings settings;
		m_nodeProperties = new CEdPropertiesPage( panel, settings, nullptr );
		m_nodeProperties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CGameResourceEditor::OnPropertiesChanged ), NULL, this );
		m_nodeProperties->SetObject( m_gameResource.Get() );
		panel->Layout();
	}
}

//////////////////////////////////////////////////////////////////////////

CGameResourceEditor::~CGameResourceEditor()
{
	if ( m_gameResource->IsModified() )
	{
		if ( wxMessageBox( TXT("Resource has been modified. Do you wish to save it?"), TXT("Modified resource"), wxYES_NO | wxCENTRE ) == wxYES )
		{
			m_gameResource->Save();
		}
	}

	m_gameResource->RemoveFromRootSet();
}

//////////////////////////////////////////////////////////////////////////

void CGameResourceEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	m_gameResource->MarkModified();
}

//////////////////////////////////////////////////////////////////////////

void CGameResourceEditor::OnSave( wxCommandEvent& event )
{
	if ( ! m_gameResource->Save() )
	{
		ERR_EDITOR(TXT("Failed to save file: %s"), m_gameResource->GetDepotPath().AsChar() );
	}
}
