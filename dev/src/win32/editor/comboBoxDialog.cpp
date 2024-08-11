/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "comboBoxDialog.h"

CEdComboBoxDlg::CEdComboBoxDlg( wxWindow* parent, const String& title, const String &caption, const String &defaultChoice, const TDynArray<String>& choices )
: wxDialog( parent, wxID_ANY, title.AsChar(), wxDefaultPosition, wxSize( -1, 125 ) )
, m_selectText( defaultChoice )
{
	// Center
	SetSizeHints( wxDefaultSize, wxDefaultSize );
	Centre( wxBOTH );

	// Base sizer
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Caption
	wxStaticText* text = new wxStaticText( this, wxID_ANY, caption.AsChar(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	text->Wrap( -1 );
	sizer->Add( text, 0, wxALL|wxEXPAND, 5 );

	// Edit box
	wxArrayString arrChoices;
	for (Uint32 i=0; i<choices.Size(); i++)
	{
		arrChoices.push_back( choices[i].AsChar() );
	}
	m_box = new CEdChoice( this, wxDefaultPosition, wxSize(300,-1), true );
	m_box->Append( arrChoices );
	m_box->SetValue( defaultChoice.AsChar() );
	sizer->Add( m_box, 0, wxALL|wxEXPAND, 5 );

	// Sizer for buttons
	wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );
	sizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	// OK button
	wxButton *ok = new wxButton( this, wxID_ANY, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
	ok->SetDefault(); 
	sizer2->Add( ok, 0, wxALL, 5 );

	// Cancel button
	wxButton* cancel = new wxButton( this, wxID_ANY, wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer2->Add( cancel, 0, wxALL, 5 );

	// Finalize
	sizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	sizer->Add( sizer2, 2, wxEXPAND, 5 );

	// Connect Events
	ok->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdComboBoxDlg::OnOK ), NULL, this );
	cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdComboBoxDlg::OnCancel ), NULL, this );

	// Update layout
	SetSizer( sizer );
	Layout();
}

VOID CEdComboBoxDlg::OnOK( wxCommandEvent& event )
{
	m_selectText = m_box->GetValue().wc_str();
	EndDialog( 1 );
}

VOID CEdComboBoxDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( 0 );
}