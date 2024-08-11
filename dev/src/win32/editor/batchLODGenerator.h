/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/meshLODGenerator.h"
#include "assetBrowser.h"

class CEdAddLODDialog
	: private wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CEdAddLODDialog( wxWindow* parent, Bool editMode = false );
	
	~CEdAddLODDialog();

	Bool Execute( SLODPresetDefinition& definition );

	Bool Execute( SLODPresetDefinition& definition, Int32 numLods, Int32& outLodIdx );

private:
	wxSlider*   m_reductionSlider;
	wxTextCtrl* m_reductionEdit;
	wxTextCtrl* m_distance;
	wxTextCtrl* m_faceLimit;
	wxCheckBox* m_removeSkinning;
	wxChoice*   m_insertAsChoice;

	wxCheckBox* m_recalculateNormals;
	wxSpinCtrl* m_hardEdgeAngle;

	wxSlider*   m_geometryImportance;
	wxSlider*   m_textureImportance;
	wxSlider*   m_materialImportance;
	wxSlider*   m_groupImportance;
	wxSlider*   m_vertexColorImportance;
	wxSlider*   m_shadingImportance;
	
	Int32 m_lodIdx;

	void OnBySliderRB( wxCommandEvent& event );
	void OnByEditRB( wxCommandEvent& event );
	void OnReductionSlider( wxCommandEvent& event );
};


class CEdBatchLodGenerator 
	: private wxDialog, private ISavableToConfig
{
	DECLARE_EVENT_TABLE();

public:
	//! Creates the generator for all meshes in given directories
	CEdBatchLodGenerator( wxWindow* parent, const CContextMenuDir& contextMenuDir );

	//! Creates the generator for the given mesh (and optional ability to undo generated stuff)
	CEdBatchLodGenerator( wxWindow* parent, CMesh* mesh, CEdUndoManager* undoManager = nullptr  );

	//! Shows the dialog modally
	Bool Execute();

	~CEdBatchLodGenerator();

private:
	class CJobGenerateLOD;

	CEdUndoManager*				m_undoManager;
	CMesh*						m_singleMeshToProcess;
	CContextMenuDir				m_contextMenuDir;
	TDynArray< String >			m_filesToProcess;
	TDynArray< ILoadJob* >		m_jobs;
	String						m_logFilePath;

	TDynArray< SLODPreset >  m_presets;
	SLODPreset               m_currentPreset;

	Int32  m_currentPresetIdx;
	Bool   m_currentPresetDirty;
	Bool   m_running;
	Bool   m_internalSet;

	void CommonInit( wxWindow* parent );

	SLODPreset GetDefaultPreset() const;
	Int32 FindPresetIndexByName( const String& name );
	void SetPresetDirty();
	void SetPresetClean();
	void AfterPresetChanged();

	bool IsSupportedFile( CDiskFile* file ) const;
	void ScanDir( CDirectory* dir );
	void GenerateLODs();
	void StopLODGeneration();
	void UpdatePresetDisplay();
	void Log( const wxString& message );

	void OnPresetChanged( wxCommandEvent& event );
	void OnSavePreset( wxCommandEvent& event );
	void OnRemovePreset( wxCommandEvent& event );

	void OnAddLOD( wxCommandEvent& event );
	void OnEditLOD( wxCommandEvent& event );
	void OnRemoveLOD( wxCommandEvent& event );
	void OnLODSelected( wxCommandEvent& event );
	void OnFaceLimitChanged( wxCommandEvent& event );

	void OnStartStop( wxCommandEvent& event );
	void OnClose( wxCommandEvent& event );

	void OnJobStarted( wxCommandEvent& event );
	void OnJobMessage( wxCommandEvent& event );
	void OnJobEnded( wxCommandEvent& event );

	void OnUpdateUI( wxUpdateUIEvent& event );
	void UpdateState();

	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;

	wxChoice* m_presetsChoice;
	wxButton* m_savePreset;
	wxButton* m_removePreset;

	wxButton* m_addLodBtn;
	wxButton* m_editLodBtn;
	wxButton* m_removeLodBtn;

	wxListBox*    m_lodList;
	wxTextCtrlEx* m_faceLimitEdit;

	wxGauge*    m_progress;
	wxListBox*  m_log;

	wxButton* m_startStopBtn;
	wxButton* m_closeBtn;
};
/**/
