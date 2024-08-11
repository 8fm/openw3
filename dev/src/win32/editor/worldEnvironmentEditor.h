/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once 

class CEdWorldEnvironmentToolPanel;

/// Editor tool for world environment
class CEdWorldEnvironmentTool : public wxFrame, public IEdEventListener, public ISavableToConfig
{

protected:
	CEdWorldEnvironmentToolPanel*	m_panel;
	Bool							m_curveDisplayTimeChanged;
	Bool							m_globalEnvListUpdateNeeded;

public:
	CEdWorldEnvironmentTool();
	~CEdWorldEnvironmentTool();

	Bool Show( CEdRenderingPanel* viewport );

	// shows the tool and injects data properties into main tab
	void ShowAndEdit( CObject* data );

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

public:
	void NotifyCurveDisplayTimeChanged();

protected:
	void OnClose( wxCloseEvent& event );

	virtual void SaveSession( CConfigurationManager &config );
	virtual void RestoreSession( CConfigurationManager &config );
};

/// World environment mesh tool panel
class CEdWorldEnvironmentToolPanel : public CEdDraggablePanel
{
	friend class CEdWorldEnvironmentTool;

	enum ECurveActionType
	{
		AT_Copy,
		AT_Paste,
		AT_Remove,
		AT_AddNewKey,

		AT_Count
	};

	DECLARE_EVENT_TABLE();

protected:
	CEdWorldEnvironmentTool*					m_tool;
	CAreaEnvironmentComponent*					m_areaEnvComponent;
	wxBitmapToggleButton*						m_stayOnTop;
	wxTreeCtrl*									m_definitionsTree;
	wxImageList*								m_defTreeImageList;
	THashMap< IDepotObject*, wxTreeItemId >		m_defTreeMap;
	wxListBox*									m_activeEnvs;
	wxButton*									m_save;
	CObject*									m_lastData;
	wxTreeItemId								m_popupSource;
	wxCheckBox*									m_pause;
	CEdPropertiesPage*							m_browser;
	wxTimePickerCtrl*							m_fakeDayCycleTimePicker;
	wxTimePickerCtrl*							m_leftSelectPicker;
	wxTimePickerCtrl*							m_rightSelectPicker;

	wxTextCtrl*									m_currentWeatheConditionTextCtrl;
	wxGauge*									m_currentWeatherConditionStatus;
	
public:
	CEdWorldEnvironmentToolPanel( CEdWorldEnvironmentTool* tool, wxWindow* parent );
	~CEdWorldEnvironmentToolPanel();

public:
	void SynchronizeData( bool localToPanel );
	void UpdateGlobalEnvironmentsPanel();
	void UpdateDefinitionsTree();
	void UpdateWeatherConditions();

	void SetStayOnTop( bool stayOnTop );
	bool GetStayOnTop() const;		

protected:	
	virtual void SaveSession( CConfigurationManager &config );
	virtual void RestoreSession( CConfigurationManager &config );

	CEnvironmentManager* GetEnvironmentManager() const;

	void OnOpenWeatherTemplate( wxCommandEvent &event );
	void OnStabilizeAreaEnvironments( wxCommandEvent &event );
	void OnStdEnvChoiceSelected( wxCommandEvent &event );
	void OnWStdWeatherConditionsChoiceSelected( wxCommandEvent &event );	

	void EditPropertiesOf( CObject* obj );
	void ShowAndEdit( CObject* data );

	void DeleteSingleFile( CDiskFile* file );

	Bool GetActiveEnvironmentsSelection( Uint32& selection ) const;

	// pass button events
	void OnEditWorldEnvironment( wxCommandEvent &event );
	// just synchronize
	void OnEdition( wxCommandEvent &event );

	void OnStayOnTop( wxCommandEvent& event );
	void OnSaveClicked( wxCommandEvent& event );

	void OnActiveEnvironmentsContextMenu( wxContextMenuEvent& event );

	void OnDefMenuNewDefinition( wxCommandEvent& event );
	void OnDefMenuNewFolder( wxCommandEvent& event );
	void OnDefMenuRename( wxCommandEvent& event );
	void OnDefMenuDelete( wxCommandEvent& event );

	void OnActEnvMenuEdit( wxCommandEvent& event );
	void OnActEnvMenuSelect( wxCommandEvent& event );

	void OnDefinitionsTreeItemActivated( wxTreeEvent& event );
	void OnDefinitionsTreeItemMenu( wxTreeEvent& event );

	void OnPropertiesModified( wxCommandEvent& event );
	void OnPauseChecked( wxCommandEvent& event );	

	void OnChangeFakeDayCycleTimePicker( wxDateEvent& event );
	void OnChangeFakeDayCycleSlider( wxCommandEvent &event );
	void ShowHideAdvancedTools( wxCommandEvent &event );

	// copy paste selection curve tool
	typedef TPair< Float, Vector > PointEntry;
	typedef TDynArray< PointEntry > CurvePoints;
	typedef THashMap< String, CurvePoints > CurvePointsCollection;

	void OnUpdateSelection( wxCommandEvent& event );
	void OnCopySelection( wxCommandEvent& event );
	void OnPasteSelection( wxCommandEvent& event );
	void OnRemovePoints( wxCommandEvent& event );
	void OnClearSelection( wxCommandEvent& event );
	void OnAddNewKey( wxCommandEvent& event );
	void OnChangeLeftTimePicker( wxDateEvent& event );
	void OnChangeRightTimePicker( wxDateEvent& event );

	void RecursiveStuff( void* obj, IRTTIType* type, const String& path, ECurveActionType action, CurvePointsCollection& values );
	void ManageCurve( SSimpleCurve* curve, const String& curvePath, ECurveActionType action, CurvePointsCollection& values );
	void FillCopyAreasControl();
	void RecursiveFillAreas( void* obj, IRTTIType* type, wxChoice* choice );
	void SendSceneEditorRefreshNotification() const;

	void UpdateEnv();
	void UpdateWeatherPresetsChoiceBox();
	void UpdateWeatherEnvironments();
	void UpdateWorldStatus();
	void RefreshAdvancedTools();
	void ClearAdvancedTools();
};
