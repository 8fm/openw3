/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/minimapGenerator.h"
#include "../../common/core/uniquePtr.h"
#include "configManager.h"

class CMinimapGeneratorTool : public wxDialog, public ISavableToConfig
{
	DECLARE_EVENT_TABLE();

	enum EToolGenerationMode
	{
		TGM_Exteriors,
		TGM_Interiors,

		TGM_Count
	};

public:
	CMinimapGeneratorTool();
	~CMinimapGeneratorTool();

private:
	// ISavableToConfig
	RED_INLINE void SaveOptionsToConfig();
	RED_INLINE void LoadOptionsFromConfig();
	void SaveSession( CConfigurationManager &config );
	void RestoreSession( CConfigurationManager &config );

	// wxWidgets callbacks
	void OnGenerateExteriors( wxCommandEvent& event );
	void OnGenerateInteriors( wxCommandEvent& event );
	void OnTileSelectionRadioButton( wxCommandEvent& event );
	void OnSelectAllMasks( wxCommandEvent& event );
	void OnSelectSingleMask( wxCommandEvent& event );
	void OnOutputDirChanged( wxFileDirPickerEvent& event );
	void OnForcedEnvSettings( wxCommandEvent& event );

	// function related with wxWidgets controls
	void LoadControls();
	void SetEnableControls( Bool enable );
	void FillMinimapSettings( SMinimapSettings &minimapSettings );
	void CheckPreviousSession();
	void Generate( EToolGenerationMode mode );

	// new way of creation interiors
	void OnGatherAllEntityTemplates( wxCommandEvent& event );
	void OnCreateEntityTemplatesInstances( wxCommandEvent& event );
	void OnCreateNavmesh( wxCommandEvent& event );
	void OnCreateNewInterior( wxCommandEvent& event );

private:
	// general controls
	wxDirPickerCtrl*	m_dpOutputDirectory;
	wxCheckBox*			m_cbForcedEnvSettings;
	wxFilePickerCtrl*	m_fpEnvFile;
	wxRadioButton*		m_rbOnlyCameraTile;
	wxRadioButton*		m_rbAroundCamera;
	wxRadioButton*		m_rbTileRange;
	wxRadioButton*		m_rbAllTiles;
	wxSpinCtrl*			m_scExtraTiles;
	wxSpinCtrl*			m_scTileRangeMinX;
	wxSpinCtrl*			m_scTileRangeMinY;
	wxSpinCtrl*			m_scTileRangeMaxX;
	wxSpinCtrl*			m_scTileRangeMaxY;

	// exteriors
	wxTextCtrl*			m_fileNamePrefix;
	wxRadioButton*		m_rbDefaultDirLayout;
	wxRadioButton*		m_rbPhotoshopDirLayout;
	wxChoice*			m_cImageSize;
	wxSpinCtrl*			m_scTileZoom;
	wxSpinCtrl*			m_scTileOffset;
	wxCheckBox*			m_cbSelectAll;
	wxCheckListBox*		m_clbMasks;
	wxPanel*			m_pContinueGenerationPanel;
	wxCheckBox*			m_cbContinueGeneration;
	wxButton*			m_bGenerateExteriors;

	// interiors
	wxTextCtrl*			m_tcInteriorErrors;
	wxButton*			m_bGenerateInteriors;
	wxTextCtrl*			m_tcInteriorEntities;

	// generator
	Red::TUniquePtr< CMinimapGenerator > m_minimapGenerator;

	TDynArray< String > pathToEntityTemplates;
	TDynArray< CEntity* > entities;
};
