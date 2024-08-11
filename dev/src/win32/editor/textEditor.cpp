/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CEdTextEditor::CEdTextEditor( wxWindow *parent, const String& caption )
	: wxFrame( parent, wxID_ANY, wxT("") )
	, m_hook( NULL )
{
	SetTitle( caption.AsChar() );
	SetSizeHints( wxDefaultSize, wxDefaultSize );
	SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE ) );
	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdTextEditor::OnFrameClose ), NULL, this );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	bSizer1->SetMinSize( wxSize( 480, 320 ) ); 
	m_textCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_MULTILINE|wxTE_RICH|wxTE_PROCESS_TAB );
	m_textCtrl->SetFont( wxFont( 10, 76, 90, 90, false, wxT("Consolas") ) );
	m_textCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdTextEditor::OnTextUpdated ), NULL, this );

	bSizer1->Add( m_textCtrl, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxPanel* m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_openButton = new wxButton( m_panel1, wxID_ANY, wxT("&Open..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_openButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextEditor::OnOpen ), NULL, this );
	bSizer2->Add( m_openButton, 0, wxALL, 5 );

	m_saveButton = new wxButton( m_panel1, wxID_ANY, wxT("&Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_saveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextEditor::OnSave ), NULL, this );
	bSizer2->Add( m_saveButton, 0, wxALL, 5 );

	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_closeButton = new wxButton( m_panel1, wxID_ANY, wxT("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
	m_closeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextEditor::OnClose ), NULL, this );
	bSizer2->Add( m_closeButton, 0, wxALL, 5 );

	m_panel1->SetSizer( bSizer2 );
	m_panel1->Layout();
	bSizer2->Fit( m_panel1 );
	bSizer1->Add( m_panel1, 0, wxEXPAND, 5 );

	SetSizer( bSizer1 );
	SetSize( 850, 570 );
	Layout();

	Centre( wxBOTH );
}

void CEdTextEditor::OnOpen( wxCommandEvent& event )
{
	wxFileDialog* dlg = new wxFileDialog( this, wxT("Open text file"), wxEmptyString, wxEmptyString, wxT("*.*"), wxFD_OPEN|wxFD_FILE_MUST_EXIST );
	if ( dlg->ShowModal() == wxID_OK )
	{
		if ( !m_textCtrl->LoadFile( dlg->GetPath() ) )
		{
			wxMessageBox( wxT("Failed to load '") + dlg->GetPath() + wxT("'"), wxT("Error"), wxCENTRE|wxOK|wxICON_ERROR, this );
		}
		else
		{
			if ( m_hook )
			{
				m_hook->OnTextEditorModified( this );
			}
		}
	}
	dlg->Destroy();
}

void CEdTextEditor::OnSave( wxCommandEvent& event )
{
	wxFileDialog* dlg = new wxFileDialog( this, wxT("Save text file"), wxEmptyString, wxEmptyString, wxT("*.*"), wxFD_SAVE );
	if ( dlg->ShowModal() == wxID_OK )
	{
		if ( !m_textCtrl->SaveFile( dlg->GetPath() ) )
		{
			wxMessageBox( wxT("Failed to save '") + dlg->GetPath() + wxT("'"), wxT("Error"), wxCENTRE|wxOK|wxICON_ERROR, this );
		}
	}
	dlg->Destroy();
}

void CEdTextEditor::OnClose( wxCommandEvent& event )
{
	Close();
}

void CEdTextEditor::OnFrameClose( wxCloseEvent& event )
{
	if ( m_hook )
	{
		m_hook->OnTextEditorClosed( this );
	}
	Destroy();
}

void CEdTextEditor::OnTextUpdated( wxCommandEvent& event )
{
	if ( m_hook )
	{
		m_hook->OnTextEditorModified( this );
	}
}

void CEdTextEditor::SetText( const String& text )
{
	m_textCtrl->SetValue( text.AsChar() );
}

String CEdTextEditor::GetText() const
{
	return String( m_textCtrl->GetValue().c_str() );
}

void CEdTextEditor::SetHook( IEdTextEditorHook* hook )
{
	m_hook = hook;
}
