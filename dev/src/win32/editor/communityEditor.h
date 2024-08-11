#pragma once

#include "communityValidator.h"

class CGridEditor;
class CCommunity;

class CEdCommunityEditor : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

public:

	CEdCommunityEditor(wxWindow* parent, CCommunity* community);
	~CEdCommunityEditor();

	// ISavableToConfig interface
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	// IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

protected:

	virtual void OnInternalIdle();

private:

    const CName m_communityTableProperty;
    const CName m_storyPhaseTimetableProperty;
    const CName m_sceneTableProperty;
    const CName m_reactionTableProperty;
    const CName m_layersProperty;

	wxPanel *m_guiGridPanel;
	wxNotebook *m_notebook;
	wxChoice*	m_typeChoice;
	CGridEditor *m_communityTableGrid;
	CGridEditor *m_storyPhaseTimetableGrid;
	CGridEditor *m_sceneTableGrid;
	CGridEditor *m_layoutsGrid;
	Bool m_invalidData;

	CEdPropertiesBrowserWithStatusbar *m_properties;
	CCommunity *m_community;
	CCommunityValidator m_communityValidator;

	// Event handlers
	void OnSave( wxCommandEvent& event );
	void OnExit( wxCommandEvent& event );
	void OnValidate( wxCommandEvent& event );
	void OnGridValueChanged( wxCommandEvent& event );
	void OnTypeChanged( wxCommandEvent& event );

	// Other methods
	CGridEditor *CreateGridFromProperty(wxWindow *parent, CClass *classPtr, String propertyName);
	void SetGridObject(CGridEditor *grid, CProperty *prop);
	void SaveModifiedResources();
	void CheckAndUpdateGUIDs();
    void CacheMapPinPositions();

protected:
	wxPanel*			m_spawnsetPanel;
	CEdPropertiesPage*	m_spawnsetProperties;

	wxPanel*			m_timetablePanel;
	CEdPropertiesPage*	m_timetableProperties;

};
