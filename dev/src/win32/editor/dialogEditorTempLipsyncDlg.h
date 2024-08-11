// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

// forward declarations
class CEdSceneEditor;

/*
Temporary lipsync dialog for dialog editor (scene editor).
*/
class CEdDialogEditorTempLipsyncDlg : public wxDialog
{
public:
	CEdDialogEditorTempLipsyncDlg(CEdSceneEditor* mediator);
	virtual ~CEdDialogEditorTempLipsyncDlg() override;

private:
	void OnDeleteButton( wxCommandEvent& event );
	void OnRecreateButton( wxCommandEvent& event );
	void OnCloseButton( wxCommandEvent& event );

	void GetSectionsToProcess( TDynArray< const CStorySceneSection* >& sectionsToProcess ) const;

	CEdSceneEditor* m_mediator;

	wxDECLARE_EVENT_TABLE();
};
