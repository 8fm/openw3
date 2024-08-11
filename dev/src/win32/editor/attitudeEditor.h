/**8
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/attitude.h"

class CAttitudesEditor : public CAttitudes
{
public:
	Bool SaveDataToXmlFile( const String& filePath );
	CDiskFile* GetDiskFile( const String& filePath );
};

class CEdAttitudeEditor : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE()

public:
	CEdAttitudeEditor( wxWindow* parent );
	~CEdAttitudeEditor();

	// Menu
	void OnSelectTargetDLC( wxCommandEvent &event );
	void OnSave( wxCommandEvent &event );
	void OnSubmit( wxCommandEvent &event );
	void OnExit( wxCommandEvent &event );
	void OnClose( wxCloseEvent &event );

	// Events handlers
	void OnGridCellChange( wxGridEvent &event );
	void OnAttitudeChange( Uint32 row, wxString text );
	void OnGridCellClicked( wxGridEvent &event );
	void OnGridCellDoubleClicked( wxGridEvent &event );
	void OnToolBar( wxCommandEvent &event );
	void OnGroupSelected( wxCommandEvent &event );
	void OnComboBox( wxNotifyEvent &event );
	void OnParentSelected( wxCommandEvent &event );
	void OnTextFilterChanged( wxCommandEvent &event );
	void OnParentsListDoubleClicked( wxCommandEvent &event );
	void OnChildrenListDoubleClicked( wxCommandEvent &event );
	void OnColumnSort( wxGridEvent &event );

private:
	void InitGuiData();
	void InitGrid();
	void FillGrid();
	void UpdateGridRowValue( Uint32 row );
	void UpdateGridRowAttributes( Uint32 row );
	void UpdateFilters( Bool fillGrid = true );
	void UpdateParentComboValue();
	void UpdateInheritanceControls( Bool updateComboParent = true );
	void JumpToGroup( const CName& groupName );
	void LoadData();
	void LoadConfig();
	void SaveData(); // saves data to the XML config file
	void SaveConfig();
	Bool SubmitData();
	Bool CheckOutConfigFile();
	Bool UndoCheckOutConfigFile();

	IAttitudesManager* GetAttitudesManager( Bool updateGrid = true );
	CAttitudeManager* GetInGameAttitudesManager();
	Bool SetDebugMode( Bool debugMode );
	Bool IsDebugMode();
	void UpdateDebugControls();

	// GUI
	Bool		m_debugMode;
	wxGrid*		m_guiGrid;
	wxFont      m_defaultGridFont;
	wxFont      m_boldGridFont;
	wxToolBar*	m_toolBarSave;
	wxToolBar*	m_toolBarDebug;
	wxToolBar*	m_toolBarLocal;
	wxComboBox*	m_comboGroups;
	wxComboBox*	m_comboParent;
	wxTextCtrl* m_textCtrlFilter;
	wxStaticText* m_textDebugMode;
	wxListBox*  m_listParents;
	wxListBox*  m_listChildren;
	const Int32	m_idSaveTool;
	const Int32	m_idSubmitTool;
	const Int32	m_idHideNeutralTool;
	const Int32	m_idHideNonCustomTool;
	const Int32	m_idFilterTool;
	const Int32 m_idDebugModeTool;
	const Int32 m_idRefreshTool;

	// GUI data
	wxArrayString m_attitudeChoices;

	// Data
	CAttitudesEditor	m_attitudes;
	TDynArray< String >	m_attitudeGroups;
	TDynArray< String >	m_visibleAttitudeGroups;
	TDynArray< CName >  m_possibleParentGroups;
	Bool				m_hideNeutral;
	Bool				m_hideNonCustom;
	String				m_filter;
	Bool                m_hasParent;
	Bool				m_dataChangedNotSaved;
	Bool				m_dataCheckedOutNotSaved;

	// config
	String m_attitudeXmlFilePath;
	String m_attitudeGroupsFilePath;

	String m_targetDLC;
};
