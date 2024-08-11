// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

// forward declarations
class CEdSceneEditor;

/*
Import W2 Strings dialog for dialog editor (scene editor).
*/
class CEdImportW2StringsDlg : public wxDialog
{
public:
	CEdImportW2StringsDlg( CEdSceneEditor* sceneEditor );
	virtual ~CEdImportW2StringsDlg() override;

private:
	void OnBtnImportW2Strings( wxCommandEvent& event );
	void OnBtnClose( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );

	CEdSceneEditor* m_sceneEditor;

	wxDECLARE_EVENT_TABLE();
};
