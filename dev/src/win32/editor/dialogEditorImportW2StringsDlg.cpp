// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogEditorImportW2StringsDlg.h"
#include "w2StringImporter.h"
#include "dialogEditor.h"

wxBEGIN_EVENT_TABLE( CEdImportW2StringsDlg, wxDialog )
	EVT_BUTTON( XRCID("btnImportW2Strings"), CEdImportW2StringsDlg::OnBtnImportW2Strings )
	EVT_BUTTON( XRCID("btnClose"), CEdImportW2StringsDlg::OnBtnClose )
	EVT_CLOSE( CEdImportW2StringsDlg::OnClose )
wxEND_EVENT_TABLE()

/*
Ctor.

\param sceneEditor Scene editor that is a parent of this dialog. Must not be nullptr.
*/
CEdImportW2StringsDlg::CEdImportW2StringsDlg( CEdSceneEditor* sceneEditor )
: m_sceneEditor( sceneEditor )
{
	ASSERT( sceneEditor );

	wxXmlResource::Get()->LoadDialog( this, sceneEditor, "DialogImportW2Strings" );
}

/*
Dtor.
*/
CEdImportW2StringsDlg::~CEdImportW2StringsDlg()
{}

/*
Handles "Import W2 Strings" button.
*/
void CEdImportW2StringsDlg::OnBtnImportW2Strings( wxCommandEvent& event )
{
	W2StringImporter w2StringImporter;

	if( w2StringImporter. Initialize() )
	{
		const CStorySceneSection* section = m_sceneEditor->OnImportW2StringsDlg_GetCurrentSection();
		w2StringImporter.ProcessSection( *section );

		w2StringImporter.Uninitialize();

		wxMessageBox("Operation completed successfully. Please save your scene file.");

		m_sceneEditor->OnImportW2StringsDlg_RefreshScreenplay();
	}
	else
	{
		wxMessageBox("Couldn't initialize W2 String Importer. Most probably connection to W2/W3 String DB couldn't be established.");
	}
}

/*
Handles "Close" button.
*/
void CEdImportW2StringsDlg::OnBtnClose( wxCommandEvent& event )
{
	Close( true );
}

/*
Handles close event.
*/
void CEdImportW2StringsDlg::OnClose( wxCloseEvent& event )
{
	EndModal(0);
}
