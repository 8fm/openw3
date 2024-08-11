
#pragma once
#include "spawnTreeEditor.h"
#include "gridEditor.h"

class CSpawnTree;
class CEdSpawnTreeEditor;

class CEdEncounterEditor 
	: public wxSmartLayoutPanel
	, public IEdEventListener
	, CEdTreeEditor::IHook
{
	DECLARE_EVENT_TABLE()

public:

	CEdEncounterEditor( wxWindow* parent, CEncounter* encounter );
	CEdEncounterEditor( wxWindow* parent, CSpawnTree* spawnTree );
	~CEdEncounterEditor();

	// ISavableToConfig interface
	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;

private:
	// Events
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnPropertiesRefresh( wxCommandEvent& event );
	void OnDeleteSelection( wxCommandEvent& event );
	void OnUndo( wxCommandEvent& event );
	void OnRedo( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnAutoLayout( wxCommandEvent& event );
	void OnZoomExtents( wxCommandEvent& event );
	void OnShowStats( wxCommandEvent& event );
	void OnCreatureListGridValueChanged( wxCommandEvent& event );
	void OnCreatureListGridElementsChanged( wxCommandEvent& event );
	void OnCreatureListGriSelectCell( wxGridEvent& event );
	void OnAddCreatureDef( wxCommandEvent& event );
	void OnRemoveCreatureDef( wxCommandEvent& event );
	void OnActivate( wxActivateEvent& event );

	// IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

	// CEdTreeEditor::IHook
	virtual void OnGraphSelectionChanged() override;

	void CommonInit();

	Bool IsLocked() const;

	void UpdateTree( Bool lazy = false );
	void UpdateProperties();
	void UpdateMenuState();
	void UpdateToolbarState();
	void UpdateGrid();
	void UpdateAll();

	CObject* m_editedObject;
	CEdUndoManager* m_undoManager;

	CEdPropertiesBrowserWithStatusbar* m_propsBrowser;
	CEdSpawnTreeEditor*		m_treeEditor;
	wxToolBar*				m_creatureDefToolbar;
	CGridEditor*			m_creatureDefGrid;
	TDynArray< CObject* >	m_creatureDefObjs;

	Float m_timeSinceLastDebugUpdate;
};