/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "solutionExplorer.h"
#include "closeAllDialog.h"

#include "solution/file.h"

wxIMPLEMENT_CLASS( CSSCloseAllDialog, wxDialog );

BEGIN_EVENT_TABLE( CSSCloseAllDialog, wxDialog )
	EVT_BUTTON( XRCID("btnSelectAll"), CSSCloseAllDialog::OnBtnSelectAll )
	EVT_BUTTON( XRCID("btnDeselectAll"), CSSCloseAllDialog::OnBtnDeselectAll )
	EVT_BUTTON( XRCID("btnSave"), CSSCloseAllDialog::OnBtnSave )
	EVT_BUTTON( XRCID("btnClose"), CSSCloseAllDialog::OnBtnClose )
	EVT_BUTTON( XRCID("btnCancel"), CSSCloseAllDialog::OnBtnCancel )
	EVT_CLOSE( CSSCloseAllDialog::OnClose )	
END_EVENT_TABLE()

CSSCloseAllDialog::CSSCloseAllDialog( wxWindow* parent )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("CloseAllDialog") );

	SetSize( wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, 300 );

	m_checkList = XRCCTRL( *this, "checkList", wxCheckListBox );
}

CSSCloseAllDialog::~CSSCloseAllDialog()
{

}

void CSSCloseAllDialog::Init( const vector< SolutionFilePtr >& modifiedFiles )
{	
	m_modifiedFiles = modifiedFiles;
	m_checkList->Clear();
	for( unsigned int i = 0; i < m_modifiedFiles.size(); ++i )
	{		
		m_checkList->Insert( m_modifiedFiles[ i ]->m_solutionPath.c_str(), i );
		m_checkList->Check( i, true );
	}
}

void CSSCloseAllDialog::GetChecked( vector< SolutionFilePtr >& checkedFiles )
{
	checkedFiles.clear();
	for( unsigned int i = 0; i < m_checkList->GetCount(); ++i )
	{
		if( m_checkList->IsChecked( i ) )
		{
			RED_ASSERT( m_checkList->GetItem( i )->GetName() == m_modifiedFiles[ i ]->m_solutionPath.c_str() );
			checkedFiles.push_back( m_modifiedFiles[ i ] );
		}
	}
}

void CSSCloseAllDialog::OnBtnSelectAll( wxCommandEvent& event )
{
	for( unsigned int i = 0; i < m_checkList->GetCount(); ++i )
	{
		m_checkList->Check( i, true );
	}
}

void CSSCloseAllDialog::OnBtnDeselectAll( wxCommandEvent& event )
{
	for( unsigned int i = 0; i < m_checkList->GetCount(); ++i )
	{
		m_checkList->Check( i, false );
	}
}

void CSSCloseAllDialog::OnBtnSave( wxCommandEvent& event )
{
	EndModal(END_SAVE);
	event.Skip();
}

void CSSCloseAllDialog::OnBtnClose( wxCommandEvent& event )
{
	EndModal(END_CLOSE);
	event.Skip();
}

void CSSCloseAllDialog::OnBtnCancel( wxCommandEvent& event )
{
	EndModal(END_CANCEL);
	event.Skip();
}

void CSSCloseAllDialog::OnClose( wxCloseEvent& event )
{
	EndModal(END_CANCEL);
	event.Skip();
}
