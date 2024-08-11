/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "utils.h"
#include "particlePreviewPanel.h"
#include "particlePropertiesBrowser.h"
#include "particleCurveEditor.h"
#include "materialListManager.h"
#include "../../common/core/diskFile.h"

class CEdEmitterGraphEditor;

/// List of materials with manager for static mesh
class CEdParticleMaterialList : public CEdMaterialListManager
{
public:

	CEdParticleMaterialList( wxWindow* parent, CParticleSystem* particleSystem, CEdEmitterGraphEditor* graphEditor, CEdParticleEditor* editor );

	// Get number of materials
	virtual Int32 GetNumMaterials() const override;

	//! Get n-th material
	virtual IMaterial* GetMaterial( Int32 index ) const override;

	//! Get n-th material name
	virtual String GetMaterialName( Int32 index ) const override;

	//! Set n-th material
	virtual void SetMaterial( Int32 index, IMaterial* material ) override;

	//! Highlight n-th material
	virtual void HighlightMaterial( Int32 index, Bool state ) override;

	//! Called after material property has changed
	virtual void MaterialPropertyChanged( CName propertyName, Bool finished ) override;

protected:

	CParticleSystem* m_particleSystem;
	CEdEmitterGraphEditor* m_graphEditor;
	CEdParticleEditor* m_editor;
};

/// Particle editor
class CEdParticleEditor : public wxFrame
						, public ISmartLayoutWindow
						, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	CEdParticleCurveEditor*				m_curveEditor;
	CEdGradientEditor*					m_gradientEditor;
	CEdPropertiesBrowserWithStatusbar*	m_properties;
	CEdPropertiesBrowserWithStatusbar*	m_lodProperties;
	CEdParticlePreviewPanel*			m_previewPanel;
	CEdEmitterGraphEditor*				m_emitterGraphEditor;
	CEdParticleMaterialList*			m_materialList;
	Bool								m_internalMatSelection;
	wxAuiManager						m_auiManager;
	
protected:
	CParticleSystem*					m_particleSystem;
	wxAuiNotebook*						m_auiNotebook;
	wxToolBar*							m_particleToolbar;
	wxBitmap							m_playIcon;
	wxBitmap							m_pauseIcon;
	String								m_defaultLayout;

	wxSpinCtrl*							m_selectedLod;
	wxTextCtrl*							m_lodDistance;
	wxString							m_lodDistValue;			// Required so we can use wxTextValidator on m_lodDistance

public:
	//! Get the particle system we are editing
	CParticleSystem* GetParticleSystem() const { return m_particleSystem; }

	//! Get window title
	virtual wxString GetShortTitle();

public:
	CEdParticleEditor( wxWindow *parent ); // dummy constructor for shortcuts only
	CEdParticleEditor( wxWindow* parent, CParticleSystem* particleSystem );
	~CEdParticleEditor();

	void UpdateProperties();
	void UpdateGraphEditor(Bool fitToClient);

	void EditCurve( CurveParameter* curve, const String& moduleName, Bool pinned = false );

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

protected:
	void OpenContextMenu( Uint32 pageIndex );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void OnResetLayout( wxCommandEvent& event );
	String SaveDefaultLayout( wxCommandEvent& event );
	void SaveCustomLayout( wxCommandEvent& event );
	void LoadCustomLayout( wxCommandEvent& event );

public:
	void OnSave( wxCommandEvent& event );

	void OnPlayPause( wxCommandEvent& event );
	void UpdatePlayPauseButton();
	void OnTimeRestart( wxCommandEvent& event );
	void OnTimeNormal( wxCommandEvent& event );
	void OnTimePause( wxCommandEvent& event );
	void OnTimeSpeedup( wxCommandEvent& event );
	void OnTimeSlowdown( wxCommandEvent& event );

	void OnSystemPropertyChanged( wxCommandEvent& event );
	void OnPropertyChanged( wxCommandEvent& event );
	void OnShowTarget( wxCommandEvent& event );

public:
	void OnModuleRemove( IParticleModule* module );
	void DeleteAllCurveGroups();
	void DeleteCurveGroup( const String moduleName );
	void OnEmitterChanged( CParticleEmitter* emitter );
	void OnEmitterAdded( CParticleEmitter* emitter );
	void OnEmitterRemove( CParticleEmitter* emitter );
	void OnCurvesChanged( const TDynArray< CCurve* >& curves );


	void UpdateLodProperties();
	void OnLodPropertyChanged( wxCommandEvent& event );
	void OnSelectLod( wxSpinEvent& event );
	void OnSetLodDistance( wxCommandEvent& event );
	void OnAddLod( wxCommandEvent& event );
	void OnRemoveLod( wxCommandEvent& event );

	Uint32 GetEditingLOD() const;
	void RefreshPreviewRenderingProxy();

private:
	void ResetAllEmitters();
	void ResetSelectedEmitter();
};
