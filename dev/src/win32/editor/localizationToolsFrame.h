/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "resourceSorter.h"

#include "exporters/localizationExporter.h"

// wxWrapper for directory
class DirectorySelectionItemWrapper : public DirectoryItemWrapper
{
public:
	Int32			m_selectedFiles;			// The number of selected items in this directory
	Int32			m_selectedFilesRecursive;	// The recursive number of selected items in this directory
	Int32			m_allFilesRecursive;		// The recursive number of all items in this directory

public:
	DirectorySelectionItemWrapper( CDirectory* directory )
		: DirectoryItemWrapper( directory )
		, m_selectedFiles( 0 )
		, m_selectedFilesRecursive( 0 )
		, m_allFilesRecursive( 0 )
	{};
};

class ExportGroupSelectionItemWrapper : public wxTreeItemData
{
public:
	BatchExportGroup*	m_exportGroup;
	Int32					m_selectedFiles;			// The number of selected items in this directory
	Int32					m_selectedFilesRecursive;	// The recursive number of selected items in this directory
	Int32					m_allFilesRecursive;		// The recursive number of all items in this directory

public:
	ExportGroupSelectionItemWrapper( BatchExportGroup* exportGroup )
		: m_exportGroup( exportGroup )
		, m_selectedFiles( 0 )
		, m_selectedFilesRecursive( 0 )
		, m_allFilesRecursive( 0 )
	{};

};

class CEdLocalizationTool : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();

	typedef THashMap< String, CDirectory* > TDirectories;
	typedef THashMap< String, String >		TFilePaths;

	enum EExporterType 
	{ 
		ET_StoryScene = 0,
		ET_GameplayEntity,
		ET_LosStrWithKeys,
		ET_SceneForLipsync,
		ET_SceneFact,
		ET_SceneUsage,
		ET_Journal,
		ET_LocAnalyzer,
		ET_AnimationReporter
	};

private:
	// Export Panel
	wxChoice*		m_exportTypeChoice;
	wxTextCtrl*		m_exportFilter;
	wxTreeCtrl*		m_directoryTree;
	wxCheckListBox*	m_exportFileList;
	wxButton*		m_exportButton;
	wxGauge*		m_statusBar;
	wxStaticText*	m_statusLabel;
	wxChoice*		m_sourceLanguageChoice;
	wxCheckBox*		m_recursiveFilesCheckbox;
	wxTextCtrl*		m_presetFileText;

	// Export Filter Panel
	wxCheckBox*		m_voicetagFilterEnableCheckbox;
	wxCheckListBox*	m_voiceTagFilterList;
	wxRadioBox*		m_voicetagFilterOperatorRadio;

	// Options Panel
	wxCheckBox*		m_checkoutCheckbox;
	wxCheckBox*		m_validateExportEntriesCheckbox;
	wxCheckBox*		m_markImportantVoicetagsCheckbox;
	wxCheckBox*		m_cutsceneDescriptionsCheckbox;
	wxCheckBox*		m_onlyCutscenesCheckbox;

	wxCheckListBox*	m_targetLanguageList;
	wxSpinCtrl*		m_emptyColumnsNumberSpin;
	wxSpinCtrl*		m_minPathDepthSpin;
	wxSpinCtrl*		m_maxPathDepthSpin;

	// Import Panel
	wxChoice*		m_importLanguageChoice;
	wxFilePickerCtrl* m_importFilePicker;
	wxButton*		m_importButton;

	String			m_importFile;


	TFilePaths		m_filepaths;

	TSet< String >  m_selFilesToExp; // selected files (depot paths) to export

	Bool			m_blockDirectorySelectionEvent;

	CResourceSorter	m_resourceSorter;

	EExporterType					m_currentExporterType;
	AbstractLocalizationExporter*	m_exporter;

	BatchExportGroup*				m_rootBatchExportGroup;

public:
	CEdLocalizationTool( wxWindow* parent );

protected:
	void OnExportButtonClick( wxCommandEvent& event );

	
	void OnDirectorySelected( wxTreeEvent& event );

	void OnOpenPresetButton( wxCommandEvent& event );
	void OnSavePresetButton( wxCommandEvent& event );

	void OnFileSelected( wxFileDirPickerEvent& event );
	void OnImportButtonClick( wxCommandEvent& event );

	String DetermineExportedResourceExtension();
	
	void OnSelectAllButton( wxCommandEvent& event );
	void OnDeselectAllButton( wxCommandEvent& event );
	void OnToggleAllButton( wxCommandEvent& event );
	void OnRecursiveFilesChanged( wxCommandEvent& event );
	void OnExportFileTypeChanged( wxCommandEvent& event );


	void OnExportFileListCheckListBox( wxCommandEvent& event );
	void UpdateExportFileCheck( Int32 checkListBoxIndex );

	wxTreeItemId UpdateTreeItem( const wxTreeItemId& root, const String& resourcePath, Int32 count );
	void UpdateTreeItemUpwards( const wxTreeItemId& leaf, Int32 count );
	void UpdateDirTreeItemName( DirectorySelectionItemWrapper* itemDataWrapper, wxTreeItemId selectedDirectoryId );

protected:
	void AddExportGroupToTree( BatchExportGroup& exportGroup, wxTreeItemId parentItem );
	void ListExportGroupEntries( BatchExportGroup& exportGroup );
	void UpdateGroupTreeItemName( ExportGroupSelectionItemWrapper* itemDataWrapper, wxTreeItemId selectedGroupId );

	void FillExportResourcesPaths( TDynArray< String > &exportResourcesPaths );


protected:
	AbstractLocalizationExporter* SetupExporter( const String& savePath, const String& errorsFilePath );
	AbstractLocalizationExporter* CreateExporter( EExporterType exporterType );

	Bool DetermineSavePath( String& savePath, String& errorsFilePath );
	Bool DetermineSaveDirectoryPath( String& savePath, String& errorsFilePath );
	Bool DetermineSaveFilePath( String& savePath, String& errorsFilePath );

	Bool HandleResourceValidation( const TDynArray< String >& exportResourcesPaths );
	
};


