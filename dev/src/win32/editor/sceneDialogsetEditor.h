/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/storySceneDialogset.h"
#include "sceneDialogsetPreviewPanel.h"

class CEdSceneDialogsetPropertyPage;

class CEdSceneDialogsetEditor : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();
private:
	CStorySceneDialogset*	m_dialogset;
	
	CEdSceneDialogsetPreviewPanel*	m_dialogsetPreview;

	CEdPropertiesPage*	m_dialogsetProperties;
	CEdSceneDialogsetPropertyPage*	m_personalCamerasProperties;
	CEdSceneDialogsetPropertyPage*	m_masterCamerasProperties;
	CEdSceneDialogsetPropertyPage*	m_customCamerasProperties;

	wxListBox*			m_personalCamerasList;
	wxListBox*			m_masterCamerasList;
	wxListBox*			m_customCamerasList;

	wxListBox*			m_cameraShotList;
	wxStaticText*		m_characterTrajectoriesNumberLabel;
	wxStaticText*		m_cameraTrajectoriesNumberLabel;

public:
	CEdSceneDialogsetEditor( wxWindow* parent, CStorySceneDialogset* dialogset );
	~CEdSceneDialogsetEditor();

	void UpdateDialogsetProperties();
	CStorySceneDialogset*	GetDialogset(){ return m_dialogset; }
private:
	CName GetSelectedCameraAnimationName();

protected:
	void OnPersonalCameraPropertyChanged( wxCommandEvent& event );
	void OnMasterCameraPropertyChanged( wxCommandEvent& event );
	void OnCustomCameraPropertyChanged( wxCommandEvent& event );

	void OnPersonalCameraSelected( wxCommandEvent& event );
	void OnMasterCameraSelected( wxCommandEvent& event );
	void OnCustomCameraSelected( wxCommandEvent& event );

	void OnAddPersonalCamera( wxCommandEvent& event );
	void OnRemovePersonalCamera( wxCommandEvent& event );
	void OnAddMasterCamera( wxCommandEvent& event );
	void OnRemoveMasterCamera( wxCommandEvent& event );
	void OnAddCustomCamera( wxCommandEvent& event );
	void OnRemoveCustomCamera( wxCommandEvent& event );

protected:
	void OnSave( wxCommandEvent& event );
	void OnClose( wxCommandEvent& event );
	void OnImportOldDialogset( wxCommandEvent& event );

	void OnImportCharacterTrajectoriesButton( wxCommandEvent& event );
	void OnImportCameraTrajectoriesButton( wxCommandEvent& event );
	void OnTogglePlaceables( wxCommandEvent& event );
	void OnToggleCharacters( wxCommandEvent& event );

protected:
	void OnCameraAnimationsRightClick( wxMouseEvent& event );
	void OnCameraAnimationRemove( wxCommandEvent& event );
	void OnCameraAnimationReimport( wxCommandEvent& event );
	void OnCameraAnimationRename( wxCommandEvent& event );
	void OnCameraAnimationImport( wxCommandEvent& event );
	void OnCameraAnimationRemoveAll( wxCommandEvent& event );
	void OnCameraAnimationReimportAll( wxCommandEvent& event );
	void OnActivateCamAdjust( wxCommandEvent& event );
};


class CEdSceneDialogsetPropertyPage : public CEdPropertiesPage
{
private:
	CStorySceneDialogset*	m_dialogset;
	Uint32*					m_cameraNumber;
	SScenePersonalCameraDescription*	m_personalCameraDescription;
	SSceneMasterCameraDescription*		m_masterCameraDescription;

public:
	CEdSceneDialogsetPropertyPage( wxWindow* parent, const PropertiesPageSettings& settings, CStorySceneDialogset* dialogset, CEdUndoManager* undoManager )
		: CEdPropertiesPage( parent, settings, undoManager )
		, m_dialogset( dialogset )
		, m_personalCameraDescription( NULL )
		, m_masterCameraDescription( NULL )
	{}

	CStorySceneDialogset*	GetDialogset() const { return m_dialogset; }
	Uint32					GetCameraNumber() const 
	{ 
		if ( m_personalCameraDescription != NULL )
		{
			return m_personalCameraDescription->m_cameraNumber;
		}
		else if ( m_masterCameraDescription != NULL )
		{
			return m_masterCameraDescription->m_cameraNumber;
		}
		return 0;
	}

	void SetPersonalCameraDescription ( SScenePersonalCameraDescription* personalCamera )
	{
		m_masterCameraDescription = NULL;
		m_personalCameraDescription = personalCamera;
		SetObject( personalCamera );
	}

	void SetMasterCameraDescription ( SSceneMasterCameraDescription* masterCamera )
	{
		m_masterCameraDescription = masterCamera;
		m_personalCameraDescription = NULL;
		SetObject( masterCamera );
	}

	SSceneMasterCameraDescription* GetMasterCameraDescription() const { return m_masterCameraDescription; }
	SScenePersonalCameraDescription* GetPersonalCameraDescription() const { return m_personalCameraDescription; }
};
