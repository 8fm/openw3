/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dlcListDlg.h"

#include "../../common/game/dlcManager.h"

CEdDLCListDlg::CEdDLCListDlg( wxWindow* parent )
	: wxDialog( parent, wxID_ANY, wxT("DLC List & Configurator") )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("Enable DLCs you want to play with:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer1->Add( m_staticText1, 0, wxALL, 5 );

	wxArrayString m_dlcListChoices;
	m_dlcList = new wxCheckListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dlcListChoices, 0 );
	bSizer1->Add( m_dlcList, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_buttonOK = new wxButton( this, wxID_ANY, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdDLCListDlg::OnOK ), NULL, this );
	m_buttonOK->SetDefault(); 
	bSizer2->Add( m_buttonOK, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


	bSizer1->Add( bSizer2, 0, wxEXPAND, 5 );

	SetSizer( bSizer1 );
	Layout();

	Centre( wxBOTH );
	FillList();
}

void CEdDLCListDlg::FillList()
{
	// refresh the list
	GCommonGame->GetDLCManager()->Scan();

	// get avaiable DLCs
	TDynArray< CName > dlcIds;
	GCommonGame->GetDLCManager()->GetDLCs( dlcIds );

	m_dlcList->Clear();

	for ( CName id : dlcIds )
	{
		if ( id.Empty() )
			continue;

		// format name
		String desc = GCommonGame->GetDLCManager()->GetDLCName( id );
		if ( desc.Empty() )
		{
			desc = id.AsChar();
		}

		// is enabled ?
		const Bool isEnabled = GCommonGame->GetDLCManager()->IsDLCEnabled(id);

		// add to list
		const Int32 index = m_dlcList->Insert( desc.AsChar(), m_dlcList->GetCount() );
		m_dlcList->Check( index, isEnabled );
		m_dlcIds[ index ] = id;
	}

	m_dlcList->Refresh();
}

void CEdDLCListDlg::OnOK( wxCommandEvent& event )
{
	// copy the state 
	for ( Uint32 i=0; i<m_dlcList->GetCount(); ++i )
	{
		// get DLC ID
		CName id;
		if ( !m_dlcIds.Find( i, id ) )
			continue;

		// update the flag (enabled/disabled)
		const Bool isEnabled = m_dlcList->IsChecked( i );
		GCommonGame->GetDLCManager()->EnableDLC( id, isEnabled );
	}

	// store config
	SConfig::GetInstance().Save();

	// end the dialog
	EndDialog(0);
}
	