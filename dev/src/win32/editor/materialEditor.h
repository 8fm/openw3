/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "materialGraphEditor.h"
#include "materialPreviewPanel.h"
#include "undoManager.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/materialGraph.h"

/// Material editor
class CEdMaterialEditor : public wxSmartLayoutPanel
						, public GraphEditorHook
						, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	CMaterialGraph*							m_graph;
	CEdPropertiesBrowserWithStatusbar*		m_properties;
	CEdMaterialGraphEditor*					m_graphEditor;
	CEdMaterialPreviewPanel*				m_preview;
	wxTextCtrl*								m_blockGeneratedCodeViewPS;
	wxTextCtrl*								m_fullShaderViewPS;
	wxTextCtrl*								m_blockGeneratedCodeViewVS;
	wxTextCtrl*								m_fullShaderViewVS;
	wxChoice*								m_shaderPassChoice;
	CEdUndoManager*							m_undoManager;

public:
	CEdMaterialEditor( wxWindow* parent, CMaterialGraph* graph );
	~CEdMaterialEditor();

	virtual wxString GetShortTitle() { return m_graph->GetFile()->GetFileName().AsChar() + wxString(TXT(" - Material Editor")); }
	IMaterial* GetMaterial() const { return m_graph; }

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
protected:
	//! Structure of the graph was modified
	virtual void OnGraphStructureModified( IGraphContainer* graph );

	//! Selection has changed
	virtual void OnGraphSelectionChanged();

protected:
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
	void OnEditUndo( wxCommandEvent& event );
	void OnEditRedo( wxCommandEvent& event );
	void OnShaderView (wxCommandEvent& event );
	void OnUpdateProxies ( wxCommandEvent& event );
	void OnActivate( wxActivateEvent& event );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );
};
