// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

// forward declarations
class CEdSceneEditor;

/*
Edit mimics dialog for Scene Editor.
*/
class CEdEditMimicsDlg : public wxDialog
{
public:
	CEdEditMimicsDlg( CEdSceneEditor* sceneEditor );
	virtual ~CEdEditMimicsDlg() override;

private:
	void SetupControls();
	void UpdateUI();
	void UpdateCustomMimicsUI( const CMimicFace* customMimics );
	void UpdateCategoryMimicsUI( const CMimicFace* categoryMimics );
	void UpdateCommonMimicsUI( const CMimicFace* commonMimics );
	
	CActor* GetSelectedActor() const;
	CMimicFace* GetSelectedActorCustomMimics() const;
	CMimicFace* GetSelectedActorCategoryMimics() const;
	CMimicFace* GetSelectedActorCommonMimics() const;

	void EnableCustomMimicsControls( Bool state );
	void EnableCategoryMimicsControls( Bool state );
	void EnableCommonMimicsControls( Bool state );

	void OnActorSelected( wxCommandEvent& event );
	void OnBtnClose( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );

	void OnBtnEditCustomMimics( wxCommandEvent& event );
	void OnBtnEditCategoryMimics( wxCommandEvent& event );
	void OnBtnEditCommonMimics( wxCommandEvent& event );

	void OnBtnReimportCustomMimics( wxCommandEvent& event );
	void OnBtnReimportCategoryMimics( wxCommandEvent& event );
	void OnBtnReimportCommonMimics( wxCommandEvent& event );

	CEdSceneEditor* m_sceneEditor;

	// custom mimics controls
	wxTextCtrl* m_textCtrlCustomMimicsImportFile;
	wxTextCtrl* m_textCtrlCustomMimicsFacFile;
	wxButton* m_btnEditCustomMimics;
	wxButton* m_btnReimportCustomMimics;

	// category mimics controls
	wxTextCtrl* m_textCtrlCategoryMimicsImportFile;
	wxTextCtrl* m_textCtrlCategoryMimicsFacFile;
	wxButton* m_btnEditCategoryMimics;
	wxButton* m_btnReimportCategoryMimics;

	// common mimics controls
	wxTextCtrl* m_textCtrlCommonMimicsImportFile;
	wxTextCtrl* m_textCtrlCommonMimicsFacFile;
	wxButton* m_btnEditCommonMimics;
	wxButton* m_btnReimportCommonMimics;

	// other controls
	wxComboBox* m_comboActors;

	wxDECLARE_EVENT_TABLE();
};
