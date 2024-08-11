// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

// forward declarations
class CEdSceneEditor;

// =================================================================================================
namespace SceneEditor {
// =================================================================================================

/*
Main control panel of Scene Editor.
*/
class CMainControlPanel : public wxPanel
{
public:
	CMainControlPanel( wxPanel* parent, CEdSceneEditor* sceneEditor );
	virtual ~CMainControlPanel() override;

	void UpdateUI();

private:
	void SetupControls();

	void OnVariantActivated( wxListEvent& event );
	void OnBtnSetCurrentVariantAsDefault( wxCommandEvent& event );
	void OnBtnToggleForceCurrentVariantInEditor( wxCommandEvent& event );
	void OnBtnCreateEmptyVariant( wxCommandEvent& event );
	void OnBtnCloneCurrentVariant( wxCommandEvent& event );
	void OnBtnDeleteCurrentVariant( wxCommandEvent& event );
	void OnBtnCreateUnscaledVariant( wxCommandEvent& event );
	void OnBtnApproveLocalVo( wxCommandEvent& event );
	void OnBtnBaseView( wxCommandEvent& event );
	void OnBtnViewUsingApprovedVoDurations( wxCommandEvent& event );
	void OnBtnViewUsingLocalVoDurations( wxCommandEvent& event );

	void OnComboVariantBaseSelected( wxCommandEvent& event );

	void OnComboPlVariantSelected( wxCommandEvent& event );
	void OnComboEnVariantSelected( wxCommandEvent& event );
	void OnComboDeVariantSelected( wxCommandEvent& event );
	void OnComboFrVariantSelected( wxCommandEvent& event );
	void OnComboRuVariantSelected( wxCommandEvent& event );
	void OnComboJpVariantSelected( wxCommandEvent& event );
	void OnComboBrVariantSelected( wxCommandEvent& event );

	CEdSceneEditor* m_sceneEditor;
	wxStaticText* m_staticTextSectionName;
	wxStaticText* m_staticTextVariantName;
	wxStaticText* m_staticTextDefaultVariant;
	wxStaticText* m_staticTextForcedVariant;
	wxListCtrl* m_listCtrlSectionVariants;

	wxSizer* m_sizerVariantPanel;
	wxSizer* m_sizerLocalVoMatchApprovedVo;
	wxSizer* m_sizerUseApprovedVoDurations;
	wxSizer* m_sizerUseLocalVoDurations;
	wxSizer* m_sizerVariantNonBaseLangView;

	wxComboBox* m_comboVariantBases;

	wxComboBox* m_comboPlVariant;
	wxComboBox* m_comboEnVariant;
	wxComboBox* m_comboDeVariant;
	wxComboBox* m_comboFrVariant;
	wxComboBox* m_comboRuVariant;
	wxComboBox* m_comboJpVariant;
	wxComboBox* m_comboBrVariant;

	wxStaticText* m_staticTextVariantStatus;
	wxButton* m_btnApproveLocalVo;
};

// =================================================================================================
} // namespace SceneEditor
// =================================================================================================
